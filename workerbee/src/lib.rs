use axum::{
    body::Body,
    extract,
    http::StatusCode,
    response::{self, IntoResponse},
    routing::get,
    Router,
};
use futures_channel::oneshot;
use serde::{Deserialize, Serialize};
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
        .route("/:user/value", get(value))
        .with_state(AppState { env })
        .call(req)
        .await?)
}

// #[axum_macros::debug_handler]
async fn value(
    extract::Path(user_id): extract::Path<String>,
    extract::State(AppState { env }): axum::extract::State<AppState>,
) -> impl axum::response::IntoResponse {
    let (tx, rx) = oneshot::channel::<Result<CounterResponse>>();
    spawn_local(async move {
        let mut i: u64 = 0;
        loop {
            if i > 1000_000_000_000 {
                break;
            }
            i += 1;
        }

        let namespace = env.durable_object("COUNTER").unwrap();
        let counter = namespace
            .id_from_name(&user_id)
            .and_then(|id| id.get_stub())
            .unwrap();

        match counter.fetch_with_str("http://stub/value").await {
            Ok(mut ret) => {
                tx.send(ret.json().await).unwrap();
            }

            Err(err) => {
                console_log!("{err:?}")
            }
        }
    });

    match rx.await {
        Ok(Ok(count)) => {
            console_log!("{count:?}");
            response::Json(json!({"value": count})).into_response()
        }

        Ok(err) => {
            console_log!("{err:?}");
            (StatusCode::INTERNAL_SERVER_ERROR, "").into_response()
        }

        Err(err) => {
            console_log!("{err:?}");
            response::Json(json!(0)).into_response()
        }
    }
}

#[durable_object]
struct Counter {
    state: State,
    _env: Env,
    count: i32,
}

#[derive(Debug, Serialize, Deserialize)]
struct CounterResponse {
    count: i32,
}

#[durable_object]
impl DurableObject for Counter {
    fn new(state: State, env: Env) -> Self {
        Self {
            state,
            _env: env,
            count: 0,
        }
    }
    async fn fetch(&mut self, req: Request) -> Result<Response> {
        console_log!("-------------------------------------------------------------------------");
        if self.count == 0 {
            self.count = self.state.storage().get::<i32>("count").await.unwrap_or(0);
        }

        match dbg!(req.path().as_str()) {
            "/value" => Response::from_json(&CounterResponse { count: self.count }),

            "/increment" => {
                self.count += 1;
                self.state.storage().put("count", self.count).await?;
                Response::from_json(&CounterResponse { count: self.count })
            }

            "/decrement" => {
                self.count -= 1;
                self.state.storage().put("count", self.count).await?;
                Response::from_json(&CounterResponse { count: self.count })
            }

            _ => Response::error("Unrecognized method", 500),
        }
    }
}
