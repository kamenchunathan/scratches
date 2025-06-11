mod durable_object;

use askama::Template;
use axum::{
    extract,
    http::{self, StatusCode},
    response::{self, IntoResponse},
    routing::get,
    Router,
};
use durable_object::CounterResponse;
use futures_channel::oneshot;
use serde::Deserialize;
use serde_json::json;
use tower_service::Service;
use wasm_bindgen_futures::spawn_local;
use worker::*;

#[derive(Clone)]
struct AppState {
    env: Env,
}

#[event(fetch)]
async fn fetch(
    req: HttpRequest,
    env: Env,
    _ctx: Context,
) -> Result<axum::http::Response<axum::body::Body>> {
    console_error_panic_hook::set_once();

    Ok(Router::new()
        .route("/", get(counter))
        .route("/:user_id/:action", get(value))
        .route(
            "/static/style.css",
            get(async || {
                (
                    [(
                        http::header::CONTENT_TYPE,
                        http::HeaderValue::from_static(mime::TEXT_CSS.as_ref()),
                    )],
                    include_str!("static/style.css"),
                )
            }),
        )
        .route(
            "/static/index.js",
            get(async || {
                (
                    [(
                        http::header::CONTENT_TYPE,
                        http::HeaderValue::from_static(mime::APPLICATION_JAVASCRIPT.as_ref()),
                    )],
                    include_str!("static/index.js"),
                )
            }),
        )
        .with_state(AppState { env })
        .call(req)
        .await?)
}

#[allow(unused)]
#[derive(Template)]
#[template(path = "index.html")]
struct CounterPage {
    count: i32,
}

async fn counter() -> impl IntoResponse {
    let template = CounterPage { count: 0 };
    axum::response::Html(template.render().unwrap_or("".to_string()))
}

#[derive(Deserialize)]
struct Params {
    user_id: String,
    action: String,
}

// #[axum_macros::debug_handler]
async fn value(
    extract::Path(Params { user_id, action }): extract::Path<Params>,
    extract::State(AppState { env }): axum::extract::State<AppState>,
) -> impl axum::response::IntoResponse {
    let (tx, rx) = oneshot::channel();
    spawn_local(async move {
        let namespace = env.durable_object("COUNTER").unwrap();
        let counter = namespace
            .id_from_name(&user_id)
            .and_then(|id| id.get_stub())
            .unwrap();

        let uri = format!("http://stub/{action}");
        match counter.fetch_with_str(&uri).await {
            Ok(mut ret) => {
                tx.send(ret.json().await.map_err(|_| {
                    StatusCode::from_u16(ret.status_code())
                        .unwrap_or(StatusCode::INTERNAL_SERVER_ERROR)
                }))
                .unwrap();
            }

            Err(err) => {
                console_log!("{err:?}")
            }
        }
    });

    match rx.await {
        Ok(Ok(CounterResponse { count })) => {
            response::Json(json!({"value": count})).into_response()
        }

        Ok(Err(status)) => (status, "Error").into_response(),

        Err(err) => (StatusCode::INTERNAL_SERVER_ERROR, format!("{err:?}")).into_response(),
    }
}
