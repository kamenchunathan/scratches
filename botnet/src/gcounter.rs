use std::collections::HashMap;

use serde::{Deserialize, Serialize};

use crate::{ErrorBody, Layer, Message, NodeData};

#[derive(Debug)]
/// A Grow only counter
pub struct GCounterLayer(HashMap<String, u32>);

impl GCounterLayer {
    pub fn new() -> Self {
        Self(HashMap::new())
    }
}

#[derive(Debug, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Req {
    Add {
        delta: u32,
        msg_id: u32,
    },

    Read {
        msg_id: u32,
    },

    PeerAdd {
        key: String,
        delta: u32,
        msg_id: u32,
    },
}

#[derive(Debug, Serialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Resp {
    AddOk {
        msg_id: u32,
        in_reply_to: u32,
    },

    ReadOk {
        msg_id: u32,
        in_reply_to: u32,
        value: u32,
    },

    PeerAdd {
        key: String,
        delta: u32,
        msg_id: u32,
    },

    PeerAddOk {
        msg_id: u32,
        in_reply_to: u32,
    },
}

impl Layer for GCounterLayer {
    type Request = Req;

    type Response = Resp;

    fn handle(
        &mut self,
        node: impl NodeData,
        req: Message<Self::Request>,
    ) -> Vec<Message<Result<Self::Response, ErrorBody>>> {
        match req.body {
            Req::Add { delta, msg_id } => {
                let key = format!("{}{}", req.src, msg_id);
                self.0.insert(key.clone(), delta);

                node.all_nodes()
                    .clone()
                    .into_iter()
                    .map(|neighbour| Message {
                        src: node.node_id(),
                        dest: neighbour,
                        body: Ok(Resp::PeerAdd {
                            key: key.clone(),
                            delta,
                            // BUG: Multiple messages use the same node id
                            msg_id: node.next_message_id(),
                        }),
                    })
                    .chain(std::iter::once(Message {
                        src: node.node_id(),
                        dest: req.src,
                        body: Ok(Resp::AddOk {
                            msg_id: node.next_message_id(),
                            in_reply_to: msg_id,
                        }),
                    }))
                    .collect()
            }

            Req::Read { msg_id } => vec![Message {
                src: node.node_id(),
                dest: req.src,
                body: Ok(Resp::ReadOk {
                    msg_id: node.next_message_id(),
                    in_reply_to: msg_id,
                    value: self.0.values().sum(),
                }),
            }],

            // Join operation
            Req::PeerAdd { key, delta, msg_id } => {
                self.0.entry(key).or_insert(delta);

                vec![Message {
                    src: node.node_id(),
                    dest: req.src,
                    body: Ok(Resp::PeerAddOk {
                        msg_id: node.next_message_id(),
                        in_reply_to: msg_id,
                    }),
                }]
            }
        }
    }
}
