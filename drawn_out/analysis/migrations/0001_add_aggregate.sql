CREATE TABLE IF NOT EXISTS summary  (
  id                  INTEGER           PRIMARY KEY,
  white_elo           INTEGER,
  black_elo           INTEGER,
  t_5men              TEXT,
  t_4men              TEXT, 
  t_3men              TEXT, 
  t_5men_wdl          CHAR(2),
  t_4men_wdl          CHAR(2), 
  t_3men_wdl          CHAR(2), 
  actual_outcome      CHAR(1) NOT NULL,
  termination         TEXT    NOT NULL,
  game_id             TEXT    NOT NULL  UNIQUE,
  endgame_sequence    TEXT    NOT NULL,

  FOREIGN KEY(game_id) REFERENCES game(id),

  CHECK (
    actual_outcome IN ('w', 'd', 'b') AND
    t_5men_wdl IN ('w', 'd', 'l', 'cw', 'bl', 'ml', 'mw') AND 
    t_4men_wdl IN ('w', 'd', 'l', 'cw', 'bl', 'ml', 'mw') AND 
    t_3men_wdl IN ('w', 'd', 'l', 'cw', 'bl', 'ml', 'mw'))
);

CREATE INDEX IF NOT EXISTS idx_summary_game_id ON summary(game_id);

CREATE INDEX IF NOT EXISTS idx_summary_tags ON summary(t_5men, t_4men, t_3men);


