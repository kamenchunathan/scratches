use std::io::Write;

use anyhow::bail;
use botnet::{Message, MessageBody, Node};
use tracing::{debug, error, info};
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

        match req.body {
            MessageBody::Echo { echo, msg_id } => {
                node.send(Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: botnet::MessageBody::EchoOk {
                        echo,
                        in_reply_to: msg_id,
                    },
                })?;
            }

            MessageBody::Generate { msg_id } => {
                node.send(Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: botnet::MessageBody::GenerateOk {
                        // A unique id from the node id and current messge id
                        id: format!("{}{}", node.id, node.next_msg_id().to_string()),
                        in_reply_to: msg_id,
                    },
                })?;
            }

            _ => todo!(),
        }
    }
}
