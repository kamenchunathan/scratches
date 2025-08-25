
use anyhow::Context;
use chesai::{
    client::GeminiClient,
    chess::{all_chess_functions, chess_system_prompt},
    types::Content,
};
use std::env;
use tracing::{info, error};
use tracing_subscriber::{fmt, layer::SubscriberExt, util::SubscriberInitExt, EnvFilter, Layer };

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::registry()
        .with(fmt::layer().with_filter(EnvFilter::from("debug")))
        .init();
    info!("Starting chess AI application");

    let google_api_key =
        env::var("GEMINI_API_KEY").context("Failure extracting Gemini API key")?;

    let client = GeminiClient::new(google_api_key)
        .with_system_prompt(chess_system_prompt())
        .with_functions(all_chess_functions());

    let user_message = "The current board state is: rnbqkb1r/pppp1ppp/5n2/4p3/4P3/3P1N2/PPP2PPP/RNBQKB1R w KQkq - 0 4. Suggest the best move for White.";

    match client.generate_content("gemini-2.0-flash", vec![Content::user(user_message)]).await {
        Ok(response) => {
            if let Some(text) = response.get_text() {
                println!("\nModel response:\n{}", text);
            } else {
                println!("No text response received.");
            }
        }
        Err(e) => {
            error!("Error while requesting chess move: {:?}", e);
        }
    }

    Ok(())
}

