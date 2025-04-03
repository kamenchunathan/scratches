use std::{path::Path, str::FromStr};

use pgn_reader::BufferedReader;
use sqlx::{migrate::Migrator, Connection, SqliteConnection};
use tokio;

use chessa::{
    db::{self, insert_summaries},
    SummaryAnalysis,
};
use tracing::info;
use tracing_subscriber::{
    filter::LevelFilter, layer::SubscriberExt, util::SubscriberInitExt, Layer,
};

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::registry()
        .with(
            tracing_subscriber::fmt::layer()
                .pretty()
                .with_filter(LevelFilter::from_str("info").unwrap()),
        )
        .init();

    let mut conn = SqliteConnection::connect("../data/scraped.db").await?;

    info!("Running database migrations");
    let m = Migrator::new(Path::new("./migrations")).await?;
    m.run(&mut conn).await?;

    info!("Beginning analysis");
    loop {
        let pgns = db::get_unprocessed_pgns(&mut conn).await?;
        let batch_size = pgns.len();

        info!("Processing batch of {}", batch_size);
        let mut summaries = Vec::with_capacity(batch_size);
        for (game_id, pgn) in pgns {
            let mut reader = BufferedReader::new(pgn.as_bytes());
            let mut analysis = SummaryAnalysis::new();
            let summary = reader.read_game(&mut analysis).unwrap().unwrap();
            summaries.push((game_id, summary));
        }
        if !summaries.is_empty() {
            insert_summaries(&mut conn, summaries).await?;
        }

        if batch_size < 50 {
            break;
        }
    }

    Ok(())
}
