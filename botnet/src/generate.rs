use serde::{Deserialize, Serialize};

use crate::{ErrorBody, Layer, Message, NodeData};

#[derive(Debug)]
pub struct GenerateLayer {
    counter: u32,
}

impl GenerateLayer {
    pub fn new() -> Self {
        Self { counter: 0 }
    }
}

// NOTE: Having messages be enums with only one value is a hack to have serde handle the type field
#[derive(Debug, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Req {
    Generate { msg_id: u32 },
}
#[derive(Debug, Serialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Resp {
    GenerateOk { id: String, in_reply_to: u32 },
}

impl Layer for GenerateLayer {
    type Request = Req;

    type Response = Resp;

    fn handle(
        &mut self,
        node: impl NodeData,
        req: Message<Self::Request>,
    ) -> Vec<Message<Result<Self::Response, ErrorBody>>> {
        self.counter += 1;
        let Req::Generate { msg_id } = req.body;
        vec![Message {
            src: node.node_id(),
            dest: req.src,
            body: Ok(Resp::GenerateOk {
                id: format!("{}{}", node.node_id(), self.counter),
                in_reply_to: msg_id,
            }),
        }]
    }
}
