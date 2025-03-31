use anyhow::bail;
use botnet::{broadcast::BroadcastLayer, gcounter::GCounterLayer, generate::GenerateLayer, Node};
use tracing::error;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt};

use botnet::echo::EchoLayer;

fn main() -> Result<(), anyhow::Error> {
    // Stderr is used for logs according to the protoccol
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::fmt::layer()
                .json()
                .with_writer(std::io::stderr),
        )
        .init();

    let node = match Node::try_init(std::io::stdin(), std::io::stdout()) {
        Ok(node) => node,
        Err(msg) => {
            error!("Unable to initialize node. error: {:?}", msg);
            bail!(msg);
        }
    };
    let mut node = node
        .with_layer(EchoLayer)
        .with_layer(GenerateLayer::new())
        .with_layer(BroadcastLayer::new())
        .with_layer(GCounterLayer::new());

    loop {
        node.handle_incoming_message()?;
    }
}
