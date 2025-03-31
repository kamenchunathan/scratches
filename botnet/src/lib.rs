#![allow(unused)]
pub mod broadcast;
pub mod echo;
pub mod generate;

use std::collections::HashMap;
use std::fmt::Debug;
use std::io::{BufRead, BufReader};

use anyhow::{bail, Context};
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use tracing::error;
use tracing::info;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Message<Body> {
    /// Identifies the node this message came from
    pub src: String,

    /// Identifies the node this message came from
    pub dest: String,

    /// Payload of the message
    pub body: Body,
}

impl<T> Message<T> {
    pub fn map<U>(self, f: fn(T) -> U) -> Message<U> {
        let body = f(self.body);
        Message {
            src: self.src,
            dest: self.dest,
            body,
        }
    }
}

#[derive(Debug, Clone, Serialize)]
pub struct ErrorBody {
    /// `msg_id` of the request which caused this error.
    in_reply_to: u32,

    /// code is an integer which indicates the type of error which occurred.
    /// Maelstrom defines several error types, and you can also invent your own.
    /// Codes 0-999 are reserved for Maelstrom's use;
    /// codes 1000 and above are free for your own purposes.
    code: u32,

    /// optional, and may contain any explanatory message
    text: String,
}

pub trait Layer {
    type Request: DeserializeOwned;
    type Response: Serialize;

    fn handle(
        &mut self,
        node: impl NodeData,
        req: Message<Self::Request>,
    ) -> Vec<Message<Result<Self::Response, ErrorBody>>>;
}

impl Layer for () {
    type Request = ();

    type Response = ();

    fn handle(
        &mut self,
        node: impl NodeData,
        req: Message<Self::Request>,
    ) -> Vec<Message<Result<Self::Response, ErrorBody>>> {
        Vec::new()
    }
}

pub trait NodeData {
    fn node_id(&self) -> String;
    fn all_nodes(&self) -> Vec<String>;
    fn next_message_id(&self) -> u32;
}

#[derive(Debug, Clone, Deserialize)]
struct Init {
    msg_id: u32,

    ///  ID of the node which is receiving this message
    node_id: String,

    /// All nodes in the cluster, including the recipient.
    node_ids: Vec<String>,
}

#[derive(Debug, Clone, Serialize)]
#[serde(tag = "type", rename_all = "snake_case")]
enum InitResponse {
    InitOk { in_reply_to: u32 },
}

#[derive(Debug)]
pub struct Node<R, W, L> {
    pub id: String,
    next_msg_id: u32,
    pub all_nodes: Vec<String>,
    pub inner: L,
    stream: BufReader<R>,
    sink: W,
}

impl<R, W> Node<R, W, NodeLayer<(), ()>>
where
    R: std::io::Read,
    W: std::io::Write,
{
    pub fn try_init(stream: R, mut sink: W) -> anyhow::Result<Self> {
        let mut stream = BufReader::new(stream);
        let mut buf = String::new();
        stream
            .read_line(&mut buf)
            .context("could not read from stream")?;

        let req: Message<Init> = serde_json::de::from_str(&buf).context(format!(
            "Unable to deserialize {:?} as message",
            json::parse(&buf),
        ))?;

        let resp = Message {
            src: req.body.node_id.clone(),
            dest: req.src,
            body: InitResponse::InitOk {
                in_reply_to: req.body.msg_id,
            },
        };
        writeln!(sink, "{}", serde_json::ser::to_string(&resp)?)?;

        Ok(Self {
            id: req.body.node_id,
            next_msg_id: 1,
            sink,
            stream,
            all_nodes: req.body.node_ids,
            inner: NodeLayer {
                layer: (),
                next: (),
            },
        })
    }
}

impl<R, W, L, N> Node<R, W, NodeLayer<L, N>>
where
    R: std::io::Read,
    W: std::io::Write,
{
    pub fn send<M>(&mut self, msg: Message<Result<M, ErrorBody>>) -> anyhow::Result<()>
    where
        M: Serialize + Debug,
    {
        let resp = serde_json::ser::to_string(&msg.map(|body| match body {
            Ok(inner) => serde_json::to_value(inner).unwrap(),
            Err(err) => serde_json::to_value(err).unwrap(),
        }))?;

        info!(type_ = "sending", msg = resp);
        writeln!(self.sink, "{}", resp)?;
        self.sink.flush()?;

        self.next_msg_id += 1;
        Ok(())
    }

    pub fn recv(&mut self) -> anyhow::Result<String> {
        let mut buf = String::new();
        self.stream
            .read_line(&mut buf)
            .context("could not read from stream")?;
        info!(type_ = "recv", msg = buf.strip_suffix("\n").unwrap_or(&buf));

        Ok(buf)
    }
}

impl<R, W, L, N> Node<R, W, NodeLayer<L, N>> {
    pub fn with_layer<I>(self, layer: I) -> Node<R, W, NodeLayer<I, NodeLayer<L, N>>>
    where
        I: Layer,
    {
        Node {
            id: self.id,
            all_nodes: self.all_nodes,
            next_msg_id: self.next_msg_id,
            stream: self.stream,
            sink: self.sink,
            inner: NodeLayer {
                layer,
                next: self.inner,
            },
        }
    }
}
// Node<Stdin, Stdout, NodeLayer<EchoLayer, NodeLayer<(), ()>>>
impl<R, W, L, N> Node<R, W, NodeLayer<L, N>>
where
    R: std::io::Read,
    W: std::io::Write,
    L: Layer,
    <L as Layer>::Response: Debug,
    N: TryHandleLayerMsg,
{
    pub fn handle_incoming_message(&mut self) -> anyhow::Result<()> {
        let buf = self.recv()?;
        let responses = self.inner.parse_and_handle_layer_msg(
            (self.id.clone(), self.all_nodes.clone(), self.next_msg_id),
            buf,
        );
        for resp in responses {
            self.send(resp)?;
        }

        Ok(())
    }
}

impl NodeData for (String, Vec<String>, u32) {
    fn node_id(&self) -> String {
        self.0.clone()
    }

    fn all_nodes(&self) -> Vec<String> {
        self.1.clone()
    }

    fn next_message_id(&self) -> u32 {
        self.2
    }
}

impl<T> NodeData for &T
where
    T: NodeData,
{
    fn node_id(&self) -> String {
        (*self).node_id()
    }

    fn all_nodes(&self) -> Vec<String> {
        (*self).all_nodes()
    }

    fn next_message_id(&self) -> u32 {
        (*self).next_message_id()
    }
}

// Implements 'failover' where if one type doesn't deseserialize into the expected type, we
// try the inner one
#[derive(Debug)]
pub struct NodeLayer<L, N> {
    layer: L,
    next: N,
}

pub trait TryHandleLayerMsg {
    /// Returns None if we could not handle the message
    fn parse_and_handle_layer_msg(
        &mut self,
        data: impl NodeData,
        buf: String,
    ) -> Vec<Message<Result<serde_json::Value, ErrorBody>>>;
}

impl TryHandleLayerMsg for () {
    fn parse_and_handle_layer_msg(
        &mut self,
        data: impl NodeData,
        buf: String,
    ) -> Vec<Message<Result<serde_json::Value, ErrorBody>>> {
        vec![]
    }
}

impl<L, N> TryHandleLayerMsg for NodeLayer<L, N>
where
    L: Layer,
    N: TryHandleLayerMsg,
{
    fn parse_and_handle_layer_msg(
        &mut self,
        data: impl NodeData,
        buf: String,
    ) -> Vec<Message<Result<serde_json::Value, ErrorBody>>> {
        let req = serde_json::de::from_str::<Message<L::Request>>(buf.as_str()).context(format!(
            "Unable to deserialize {:?} as message",
            json::parse(&buf),
        ));
        match req {
            Ok(req) => {
                let responses = self.layer.handle(data, req);

                responses
                    .into_iter()
                    .map(|resp| Message {
                        src: resp.src,
                        dest: resp.dest,
                        body: resp.body.map(|body| {
                            serde_json::to_value(body).expect("Could not serialize as value")
                        }),
                    })
                    .collect()
            }
            _ => self
                .next
                .parse_and_handle_layer_msg(data, buf)
                .into_iter()
                .map(|resp| Message {
                    src: resp.src,
                    dest: resp.dest,
                    body: resp.body.map(|body| {
                        serde_json::to_value(body).expect("Could not serialize as value")
                    }),
                })
                .collect(),
        }
    }
}
