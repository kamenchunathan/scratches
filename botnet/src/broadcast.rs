use std::collections::HashMap;

use anyhow::Context;
use serde::{Deserialize, Serialize};

use crate::{ErrorBody, Layer, Message, NodeData};

#[derive(Debug)]
pub struct BroadcastLayer {
    neighbours: Vec<String>,
    received: Vec<serde_json::Value>,
}

impl BroadcastLayer {
    pub fn new() -> Self {
        Self {
            neighbours: Vec::new(),
            received: Vec::new(),
        }
    }
}

// NOTE: Having messages be enums with only one value is a hack to have serde handle the type field
#[derive(Debug, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Req {
    Topology {
        msg_id: u32,
        topology: HashMap<String, Vec<String>>,
    },

    Broadcast {
        message: serde_json::Value,
        msg_id: u32,
    },

    Read {
        msg_id: u32,
    },
}
#[derive(Debug, Serialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Resp {
    TopologyOk {
        msg_id: u32,
        in_reply_to: u32,
    },

    BroadcastOk {
        msg_id: u32,
        in_reply_to: u32,
    },

    Broadcast {
        message: serde_json::Value,
        msg_id: u32,
    },

    ReadOk {
        msg_id: u32,
        messages: Vec<serde_json::Value>,
        in_reply_to: u32,
    },
}

impl Layer for BroadcastLayer {
    type Request = Req;

    type Response = Resp;

    fn handle(
        &mut self,
        node: impl NodeData,
        req: Message<Self::Request>,
    ) -> Vec<Message<Result<Self::Response, ErrorBody>>> {
        match req.body {
            Req::Topology { msg_id, topology } => {
                self.neighbours = topology
                    .get(&node.node_id())
                    .expect("Neighbours not provided")
                    .clone();

                vec![Message {
                    src: node.node_id(),
                    dest: req.src,
                    body: Ok(Resp::TopologyOk {
                        msg_id: node.next_message_id(),
                        in_reply_to: msg_id,
                    }),
                }]
            }

            Req::Broadcast { message, msg_id } => {
                self.received.push(message.clone());

                self.neighbours
                    .clone()
                    .into_iter()
                    .map(|neighbour| Message {
                        src: node.node_id(),
                        dest: neighbour,
                        body: Ok(Resp::Broadcast {
                            message: message.clone(),
                            msg_id: node.next_message_id(),
                        }),
                    })
                    .chain(std::iter::once(Message {
                        src: node.node_id(),
                        dest: req.src,
                        body: Ok(Resp::BroadcastOk {
                            msg_id: node.next_message_id(),
                            in_reply_to: msg_id,
                        }),
                    }))
                    .collect()
            }

            Req::Read { msg_id } => {
                vec![Message {
                    src: node.node_id(),
                    dest: req.src,
                    body: Ok(Resp::ReadOk {
                        msg_id: node.next_message_id(),
                        messages: self.received.clone(),
                        in_reply_to: msg_id,
                    }),
                }]
            }
        }
    }
}
