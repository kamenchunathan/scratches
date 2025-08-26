mod room;

use tracing_subscriber::fmt::format::Pretty;
use tracing_subscriber::fmt::time::UtcTime;
use tracing_subscriber::prelude::*;
use tracing_web::{performance_layer, MakeConsoleWriter};

use worker::*;

#[event(start)]
fn start() {
    let fmt_layer = tracing_subscriber::fmt::layer()
        .json()
        .with_ansi(false)
        .with_timer(UtcTime::rfc_3339())
        .with_writer(MakeConsoleWriter);
    let perf_layer = performance_layer().with_details_from_fields(Pretty::default());
    tracing_subscriber::registry()
        .with(fmt_layer)
        .with(perf_layer)
        .init();
}

#[event(fetch)]
async fn main(req: Request, env: Env, _ctx: Context) -> Result<Response> {
    let router_env = env.clone();
    Router::new()
        .get_async("/", async move |_req, _ctx| {
            let mut headers = Headers::new();
            headers.append("Content-Type", "text/html")?;
            Ok(Response::ok(include_str!("../static/index.html"))?.with_headers(headers))
        })
        .get_async("/styles.css", async move |_req, _ctx| {
            let mut headers = Headers::new();
            headers.append("Content-Type", "text/css")?;
            Ok(Response::ok(include_str!("../static/styles.css"))?.with_headers(headers))
        })
        .get_async("/script.js", async move |_req, _ctx| {
            let mut headers = Headers::new();
            headers.append("Content-Type", "application/javascript")?;
            Ok(Response::ok(include_str!("../static/script.js"))?.with_headers(headers))
        })
        .get_async("/ws/:room", |req, ctx| {
            let namespace = router_env
                .durable_object("CHATROOM")
                .expect("Chatroom not found");
            let obj_id = namespace
                .id_from_name(ctx.param("room").unwrap_or(&"missing".to_string()))
                .expect("unable to get id");
            let stub = obj_id.get_stub().expect("Could not get stub");

            async move { stub.fetch_with_request(req).await }
        })
        .run(req, env)
        .await
}
