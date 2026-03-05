use askama::Template;
use serde::Deserialize;
use worker::{Request, Response, RouteContext, query};

#[derive(Template)]
#[template(path = "landing.html")]
struct Landing;

#[derive(Template)]
#[template(path = "downloads.html")]
struct Downloads {
    releases: Vec<Release>,
}

#[derive(Deserialize)]
struct Release {
    platform: String,
    channel: String,
    version: String,
    sha256: String,
}

pub async fn landing(_req: Request, _ctx: RouteContext<()>) -> worker::Result<Response> {
    let landing = Landing;
    Response::from_html(landing.render().unwrap())
}

pub async fn downloads(_req: Request, ctx: RouteContext<()>) -> worker::Result<Response> {
    let db = ctx.env.d1("DB")?;
    let rows = query!(
        &db,
        "SELECT platform, channel, version, filename, sha256 \
                  FROM releases ORDER BY created_at DESC LIMIT 20"
    )
    .all()
    .await?;

    let page = Downloads {
        releases: rows.results::<Release>()?,
    };
    Response::from_html(page.render().unwrap())
}
