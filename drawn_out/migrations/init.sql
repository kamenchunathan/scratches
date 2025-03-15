CREATE TABLE IF NOT EXISTS game (
  id            TEXT    PRIMARY KEY,
  status        TEXT    NOT NULL DEFAULT 'not_asked',
  retries       INTEGER NOT NULL DEFAULT 0,
  pgn           TEXT    NOT NULL,
  tournament_id TEXT    NOT NULL, 
  round         INTEGER NOT NULL,
  
  FOREIGN KEY(tournament_id) REFERENCES tournament(id)
);


CREATE TABLE if NOT EXISTS tournament (
  id              TEXT PRIMARY KEY,
  rounds_fetched  INTEGER NOT NULL DEFAULT 0
);


CREATE TABLE if NOT EXISTS jobs (
  id            INTEGER PRIMARY KEY,
  status        TEXT    NOT NULL DEFAULT 'not_asked',
  tournament_id TEXT    NOT NULL,

  FOREIGN KEY(tournament_id) REFERENCES tournament(id)
);
