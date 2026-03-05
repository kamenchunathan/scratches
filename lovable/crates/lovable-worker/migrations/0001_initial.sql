-- migrations/0001_initial.sql
CREATE TABLE IF NOT EXISTS releases (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    platform      TEXT    NOT NULL,          -- linux-x86_64 | windows-x86_64 | macos-arm | macos-x86_64
    channel       TEXT    NOT NULL,          -- stable | beta | nightly
    version       TEXT    NOT NULL,
    filename      TEXT    NOT NULL,
    sha256        TEXT    NOT NULL,
    min_supported TEXT    NOT NULL DEFAULT '0.0.0',
    mandatory     INTEGER NOT NULL DEFAULT 0,
    created_at    INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE INDEX IF NOT EXISTS idx_releases_lookup
    ON releases (platform, channel, created_at DESC);

CREATE TABLE IF NOT EXISTS critter_registry (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    name       TEXT    NOT NULL UNIQUE,
    r2_key     TEXT    NOT NULL,
    uploaded_at INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE TABLE IF NOT EXISTS audit_log (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    action     TEXT    NOT NULL,
    actor      TEXT    NOT NULL DEFAULT 'ci',
    payload    TEXT,
    created_at INTEGER NOT NULL DEFAULT (unixepoch())
);
