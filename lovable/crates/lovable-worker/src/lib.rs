mod auth;
mod routes;

use worker::{Context, Env, Request, Response, Router, event};

#[event(fetch)]
pub async fn main(req: Request, env: Env, _ctx: Context) -> worker::Result<Response> {
    Router::new()
        // Public routes
        .get_async("/", routes::public::landing)
        .get_async("/downloads", routes::public::downloads)
        // Admin routes
        .post_async("/release", routes::admin::register_release)
        .run(req, env)
        .await
}
