use serde::{Deserialize, Serialize};

use crate::{ErrorBody, Layer, Message, NodeData};

#[derive(Debug)]
pub struct EchoLayer;

impl EchoLayer {
    pub fn new() -> Self {
        Self
    }
}

// NOTE: Having messages be enums with only one value is a hack to have serde handle the type field
#[derive(Debug, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Req {
    Echo { echo: String, msg_id: u32 },
}

// NOTE: Having messages be enums with only one value is a hack to have serde handle the type field
#[derive(Debug, Serialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Resp {
    EchoOk { echo: String, in_reply_to: u32 },
}

impl Layer for EchoLayer {
    type Request = Req;

    type Response = Resp;

    fn handle(
        &mut self,
        node: impl NodeData,
        req: Message<Self::Request>,
    ) -> Vec<Message<Result<Self::Response, ErrorBody>>> {
        let Req::Echo { echo, msg_id } = req.body;
        vec![Message {
            src: node.node_id(),
            dest: req.src,
            body: Ok(Resp::EchoOk {
                echo,
                in_reply_to: msg_id,
            }),
        }]
    }
}
