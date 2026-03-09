use serde::Deserialize;
use worker::{Request, Response, RouteContext, query};

use crate::auth;

#[derive(Debug, Deserialize)]
struct RegisterReleasePayload {
    platform: String,
    channel: String,
    version: String,
    filename: String,
    sha256: String,
    min_supported: String,
    mandatory: bool,
}

/// Inserts a release record into the D1 and writes the latest release to KV
/// Called by the release CI job after uploading artifacts to R2
pub async fn register_release(mut req: Request, ctx: RouteContext<()>) -> worker::Result<Response> {
    let body = require_admin(&mut req, &ctx).await?;
    let release: RegisterReleasePayload = serde_json::from_slice(&body)?;

    let db = ctx.d1("DB")?;
    let _ =    query!(
        &db,
        "INSERT INTO releases (platform, channel, version, filename, sha256, min_supported, mandatory) \
         VALUES (?, ?, ?, ?, ?, ?, ?)",
        release.platform,
        release.channel,
        release.version,
        release.filename,
        release.sha256,
        release.min_supported,
        release.mandatory
    )?
    .run()
    .await?;

    // Invalidate cache
    let kv = ctx.kv("RELEASE_CACHE")?;
    let cache_key = format!("latest:{}:{}", release.platform, release.channel);
    let _ = kv.delete(&cache_key).await?;

    Response::from_json(&serde_json::json!({ "ok": true }))
}

/// Reads the full request body and verifies the admin HMAC signature.
/// Returns the raw body bytes on success so the caller can deserialise them.
async fn require_admin(req: &mut Request, ctx: &RouteContext<()>) -> worker::Result<Vec<u8>> {
    let body = req.bytes().await?;
    let secret = ctx.env.secret("ADMIN_SECRET")?.to_string();
    auth::verify(&body, req.headers(), secret.as_bytes())
        .map_err(|e| worker::Error::RustError(format!("401: {e}")))?;

    Ok(body)
}
