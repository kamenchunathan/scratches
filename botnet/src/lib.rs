#![allow(unused)]
pub mod echo;
mod node;

use std::collections::HashMap;
use std::fmt::Debug;
use std::io::BufRead;
use std::io::BufReader;

use anyhow::{bail, Context};
use node::Layer;
use node::NodeData;
use serde::{Deserialize, Serialize};
use tracing::error;
use tracing::info;

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Message {
    /// Identifies the node this message came from
    pub src: String,

    /// Identifies the node this message came from
    pub dest: String,

    /// Payload of the message
    pub body: MessageBody,
}

#[derive(Debug, Clone, Deserialize)]
struct Init {
    msg_id: u32,

    ///  ID of the node which is receiving this message
    node_id: String,

    /// All nodes in the cluster, including the recipient.
    node_ids: Vec<String>,
}

struct InitOk {
    in_reply_to: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum MessageBody {
    Init {
        msg_id: u32,

        ///  ID of the node which is receiving this message
        node_id: String,

        /// All nodes in the cluster, including the recipient.
        node_ids: Vec<String>,
    },

    InitOk {
        in_reply_to: u32,
    },

    Error {
        /// `msg_id` of the request which caused this error.
        in_reply_to: u32,

        /// code is an integer which indicates the type of error which occurred.
        /// Maelstrom defines several error types, and you can also invent your own.
        /// Codes 0-999 are reserved for Maelstrom's use;
        /// codes 1000 and above are free for your own purposes.
        code: u32,

        /// optional, and may contain any explanatory message
        text: String,
    },

    Echo {
        echo: String,
        msg_id: u32,
    },

    EchoOk {
        echo: String,
        in_reply_to: u32,
    },

    Generate {
        msg_id: u32,
    },

    GenerateOk {
        id: String,
        in_reply_to: u32,
    },

    Topology {
        msg_id: u32,
        topology: HashMap<String, Vec<String>>,
    },

    TopologyOk {
        msg_id: u32,
        in_reply_to: u32,
    },

    Broadcast {
        // Ideally is Any type
        message: serde_json::Value,
        msg_id: u32,
    },

    BroadcastOk {
        msg_id: u32,
        in_reply_to: u32,
    },

    Read {
        msg_id: u32,
    },

    ReadOk {
        msg_id: u32,
        messages: Vec<serde_json::Value>,
        in_reply_to: u32,
    },

    #[serde(other)]
    Other,
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

        let req: node::Message<Init> = serde_json::de::from_str(&buf).context(format!(
            "Unable to deserialize {:?} as message",
            json::parse(&buf),
        ))?;

        let resp = Message {
            src: req.body.node_id.clone(),
            dest: req.src,
            body: MessageBody::InitOk {
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
    pub fn send<M>(
        &mut self,
        msg: crate::node::Message<Result<M, crate::node::ErrorBody>>,
    ) -> anyhow::Result<()>
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
        match self
            .inner
            .parse_and_handle_layer_msg((self.id.clone(), self.all_nodes.clone()), buf)
        {
            Some(resp) => {
                self.send(resp)?;
            }

            None => {
                error!("Unable to handle message");
                bail!("unable to handle");
            }
        }

        Ok(())
    }
}

impl NodeData for (String, Vec<String>) {
    fn node_id(&self) -> String {
        self.0.clone()
    }

    fn all_nodes(&self) -> Vec<String> {
        self.1.clone()
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
}

// Implements 'failover' where if one type doesn't deseserialize into the expected type, we
// try the inner one
#[derive(Debug)]
pub struct NodeLayer<L, N> {
    layer: L,
    next: N,
}

pub trait TryHandleLayerMsg {
    type T;
    /// Returns None if we could not handle the message
    fn parse_and_handle_layer_msg(&self, data: impl NodeData, buf: String) -> Option<Self::T>;
}

impl TryHandleLayerMsg for () {
    type T = crate::node::Message<Result<serde_json::Value, crate::node::ErrorBody>>;

    fn parse_and_handle_layer_msg(&self, data: impl NodeData, buf: String) -> Option<Self::T> {
        None
    }
}

impl<L, N> TryHandleLayerMsg for NodeLayer<L, N>
where
    L: Layer,
    N: TryHandleLayerMsg,
{
    type T = node::Message<Result<serde_json::Value, node::ErrorBody>>;

    fn parse_and_handle_layer_msg(&self, data: impl NodeData, buf: String) -> Option<Self::T> {
        let req = serde_json::de::from_str::<node::Message<L::Request>>(buf.as_str()).context(
            format!("Unable to deserialize {:?} as message", json::parse(&buf),),
        );
        match req {
            Ok(req) => {
                let resp = L::handle(data, req);

                Some(node::Message {
                    src: resp.src,
                    dest: resp.dest,
                    body: resp.body.map(|body| {
                        serde_json::to_value(body).expect("Could not serialize as value")
                    }),
                })
            }
            _ => todo!(), // _ => self.next.parse_and_handle_layer_msg(data, buf),
        }
    }
}
