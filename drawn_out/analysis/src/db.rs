use anyhow::bail;
use sqlx::{QueryBuilder, Sqlite, SqliteConnection};
use tracing::info;

#[derive(Debug)]
pub struct GameSummary {
    pub white_elo: Option<u32>,
    pub black_elo: Option<u32>,
    pub t_5men: String,
    pub t_4men: String,
    pub t_3men: String,
    pub t_5men_wdl: Option<String>,
    pub t_4men_wdl: Option<String>,
    pub t_3men_wdl: Option<String>,
    pub actual_outcome: String,
    pub termination: String,
    pub endgame_sequence: String,
}

pub async fn get_unprocessed_pgns(
    conn: &mut SqliteConnection,
) -> anyhow::Result<Vec<(String, String)>> {
    let query = r#"
SELECT 
  g.id, g.pgn
FROM 
  game AS g 
  LEFT JOIN summary AS s ON s.game_id = g.id 
WHERE 
  game_id IS NULL 
LIMIT 
  100;
"#;

    let res = sqlx::query_as::<Sqlite, (String, String)>(query)
        .fetch_all(conn)
        .await?;

    Ok(res)
}

pub async fn insert_summary<'a, 'b>(
    conn: &'a mut SqliteConnection,
    game_id: &'b str,
    summary: GameSummary,
) -> anyhow::Result<()> {
    let query_frag = r#" 
INSERT INTO summary (
  white_elo, black_elo, t_5men, t_4men, 
  t_3men, t_5men_wdl, t_4men_wdl, t_3men_wdl, 
  actual_outcome, termination, game_id, 
  endgame_sequence
)
VALUES
  ( "#;

    let mut builder: QueryBuilder<Sqlite> = QueryBuilder::new(query_frag);
    let mut separated = builder.separated(", ");
    separated
        .push_bind(summary.white_elo)
        .push_bind(summary.black_elo)
        .push_bind(summary.t_5men)
        .push_bind(summary.t_4men)
        .push_bind(summary.t_3men)
        .push_bind(summary.t_5men_wdl)
        .push_bind(summary.t_4men_wdl)
        .push_bind(summary.t_3men_wdl)
        .push_bind(summary.actual_outcome)
        .push_bind(summary.termination)
        .push_bind(game_id)
        .push_bind(summary.endgame_sequence);
    separated.push_unseparated(" ) ;");

    builder.build().execute(conn).await?;

    Ok(())
}

pub async fn insert_summaries(
    conn: &mut SqliteConnection,
    summaries: Vec<(String, GameSummary)>,
) -> anyhow::Result<()> {
    info!("inserting {} entries", summaries.len());
    if summaries.is_empty() {
        bail!("inserting an empty array");
    }

    let query_frag = r#" 
INSERT INTO summary (
  white_elo, black_elo, t_5men, t_4men, 
  t_3men, t_5men_wdl, t_4men_wdl, t_3men_wdl, 
  actual_outcome, termination, game_id, 
  endgame_sequence
)"#;

    let mut builder: QueryBuilder<Sqlite> = QueryBuilder::new(query_frag);

    builder.push_values(summaries, |mut b, (game_id, s)| {
        b.push_bind(s.white_elo)
            .push_bind(s.black_elo)
            .push_bind(s.t_5men)
            .push_bind(s.t_4men)
            .push_bind(s.t_3men)
            .push_bind(s.t_5men_wdl)
            .push_bind(s.t_4men_wdl)
            .push_bind(s.t_3men_wdl)
            .push_bind(s.actual_outcome)
            .push_bind(s.termination)
            .push_bind(game_id)
            .push_bind(s.endgame_sequence);
    });
    builder.build().execute(conn).await?;

    Ok(())
}
