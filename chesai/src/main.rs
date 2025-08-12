use std::env;

use anyhow::Context;
use reqwest::header::{self, HeaderMap, HeaderValue};
use serde_json::json;
use tracing::{debug, info};
use tracing_subscriber::{fmt, layer::SubscriberExt, util::SubscriberInitExt, EnvFilter, Layer};

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::registry()
        .with(fmt::layer().with_filter(EnvFilter::from("debug")))
        .init();
    info!("Starting application");

    let api_key =
        dbg!(env::var("OPENROUTER_API_KEY").context("Failure extracting openrouter api key")?);
    let site_name = "Ping";

    let client = reqwest::Client::new();
    let res = client
        .post("https://openrouter.ai/api/v1/chat/completions")
        .headers(HeaderMap::from_iter([
            (header::AUTHORIZATION, HeaderValue::from_str(&api_key)?),
            (header::REFERER, HeaderValue::from_static(site_name)),
            (
                header::HeaderName::from_static("x-title"),
                HeaderValue::from_static(site_name),
            ),
        ]))
        .json(&json!({
            "model": "gemini-2.5-flash",
            "messages": [
                {
                    "role": "user",
                    "content": "What is the meaning of life?"
                }
            ]
        }))
        .send()
        .await
        .context("")?;
    debug!(?res, "Fetch result");

    Ok(())
}
