use axum::{
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
        .route("/:user_id/:action", get(value))
        .with_state(AppState { env })
        .call(req)
        .await?)
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
