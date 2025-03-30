use anyhow::{bail, Context};
use botnet::{Message, MessageBody, Node};
use tracing::error;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

fn main() -> Result<(), anyhow::Error> {
    // Stderr is used for logs according to the protoccol
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::fmt::layer()
                .json()
                .with_writer(std::io::stderr),
        )
        .init();

    let mut node = match Node::try_init(std::io::stdin(), std::io::stdout()) {
        Ok(node) => node,
        Err(msg) => {
            error!("Unable to initialize node. error: {:?}", msg);
            bail!(msg);
        }
    };

    loop {
        let req = match node.recv() {
            Ok(node) => node,
            Err(msg) => {
                error!("Error while receiving message: {:?}", msg);
                bail!(msg);
            }
        };

        use MessageBody::*;
        match req.body {
            Echo { echo, msg_id } => {
                node.send(Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: EchoOk {
                        echo,
                        in_reply_to: msg_id,
                    },
                })?;
            }

            Generate { msg_id } => {
                node.send(Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: GenerateOk {
                        // A unique id from the node id and current messge id
                        id: format!("{}{}", node.id, node.next_msg_id().to_string()),
                        in_reply_to: msg_id,
                    },
                })?;
            }

            Topology { msg_id, topology } => {
                node.neighbours = topology
                    .get(&node.id)
                    .context("Neighbours not provided")?
                    .clone();

                node.send(Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: TopologyOk {
                        msg_id: node.next_msg_id(),
                        in_reply_to: msg_id,
                    },
                })?;
            }

            Broadcast { msg_id, message } => {
                node.send(Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: BroadcastOk {
                        msg_id: node.next_msg_id(),
                        in_reply_to: msg_id,
                    },
                })?;
                node.messages.push(message.clone());

                for neighbour in node.neighbours.clone() {
                    node.send(Message {
                        src: node.id.clone(),
                        dest: neighbour,
                        body: Broadcast {
                            message: message.clone(),
                            msg_id: node.next_msg_id(),
                        },
                    })?;
                }
            }

            Read { msg_id } => {
                node.send(Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: ReadOk {
                        msg_id: node.next_msg_id(),
                        messages: node.messages.clone(),
                        in_reply_to: msg_id,
                    },
                })?;
            }

            msg => {
                error!("Unhandled message {msg:?}");
            }
        }
    }
}
