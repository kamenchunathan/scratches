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
                .with_ansi(false) // Output is redirected to a file
                .with_writer(std::io::stderr),
        )
        .init();

    let mut node = None;
    let mut buf = String::new();
    while std::io::stdin().read_line(&mut buf).is_ok() {
        info!(
            "Received message: {}",
            buf.strip_suffix("\n").unwrap_or(&buf)
        );
        let Ok(req) = serde_json::de::from_str::<Message>(&buf) else {
            error!("Unable to deserialize {:?} as message", json::parse(&buf));
            bail!("");
        };
        debug!("Parsed {req:?}");

        let mut stdout = std::io::stdout();
        match req.body {
            MessageBody::Init {
                msg_id, node_id, ..
            } => {
                node = Some(Node::new(node_id.clone()));
                let resp = Message {
                    src: node_id,
                    dest: req.src,
                    body: botnet::MessageBody::InitOk {
                        in_reply_to: msg_id,
                    },
                };
                writeln!(stdout, "{}", serde_json::ser::to_string(&resp)?)?;
            }

            MessageBody::Echo { echo, msg_id } => {
                let Some(node) = node.as_mut() else {
                    bail!("An initialization message has to be sent before all other message");
                };

                node.previous_msg_id += 1;

                let resp = Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: botnet::MessageBody::EchoOk {
                        echo,
                        in_reply_to: msg_id,
                    },
                };
                writeln!(stdout, "{}", serde_json::ser::to_string(&resp)?)?;
            }

            MessageBody::Generate { msg_id } => {
                let Some(node) = node.as_mut() else {
                    bail!("An initialization message has to be sent before all other message");
                };

                node.previous_msg_id += 1;

                let resp = Message {
                    src: node.id.clone(),
                    dest: req.src,
                    body: botnet::MessageBody::GenerateOk {
                        id: format!("{}{}", node.id, node.previous_msg_id.to_string()),
                        in_reply_to: msg_id,
                    },
                };
                writeln!(stdout, "{}", serde_json::ser::to_string(&resp)?)?;
            }

            _ => todo!(),
        }

        buf.clear();
    }
    Ok(())
}
