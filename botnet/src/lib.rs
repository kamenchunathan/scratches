use json::JsonValue;
use serde::{Deserialize, Serialize};

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

    #[serde(other)]
    Other,
}
