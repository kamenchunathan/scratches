use std::io::BufRead;
use std::io::BufReader;

use anyhow::{bail, Context};
use serde::{Deserialize, Serialize};
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

    #[serde(other)]
    Other,
}

#[derive(Debug)]
pub struct Node<R, W> {
    pub id: String,
    next_msg_id: u32,
    stream: BufReader<R>,
    sink: W,
}

impl<R, W> Node<R, W>
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

        let req: Message = serde_json::de::from_str(&buf).context(format!(
            "Unable to deserialize {:?} as message",
            json::parse(&buf),
        ))?;

        let MessageBody::Init {
            msg_id, node_id, ..
        } = req.body
        else {
            bail!("An initialization message has to be sent before all other message");
        };

        let resp = Message {
            src: node_id.clone(),
            dest: req.src,
            body: MessageBody::InitOk {
                in_reply_to: msg_id,
            },
        };
        writeln!(sink, "{}", serde_json::ser::to_string(&resp)?)?;

        Ok(Self {
            id: node_id,
            next_msg_id: 1,
            sink,
            stream,
        })
    }

    pub fn send(&mut self, msg: Message) -> anyhow::Result<()> {
        writeln!(self.sink, "{}", serde_json::ser::to_string(&msg)?)?;
        self.sink.flush()?;

        Ok(())
    }

    pub fn recv(&mut self) -> anyhow::Result<Message> {
        let mut buf = String::new();
        self.stream
            .read_line(&mut buf)
            .context("could not read from stream")?;
        info!(
            "Received message: {}",
            buf.strip_suffix("\n").unwrap_or(&buf)
        );

        serde_json::de::from_str(&buf).context(format!(
            "Unable to deserialize {:?} as message",
            json::parse(&buf),
        ))
    }

    pub fn next_msg_id(&self) -> u32 {
        self.next_msg_id
    }
}
