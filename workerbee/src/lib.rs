use axum::{routing::get, Router};
use serde::{Deserialize, Serialize};
use serde_json::json;
use tower_service::Service;
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

#[axum_macros::debug_handler]
async fn value(
    axum::extract::State(AppState { env }): axum::extract::State<AppState>,
) -> impl axum::response::IntoResponse {
    let namespace = env.durable_object("data").unwrap();
    let counter = namespace
        .id_from_name("Counter")
        .and_then(|id| id.get_stub())
        .unwrap();

    let mut ret = counter.fetch_with_str("/increment").await.unwrap();
    let CounterResponse { count } = ret.json().await.unwrap();

    serde_json::to_string(&json!({"value": 0})).unwrap()
}

#[durable_object]
struct Counter {
    state: State,
    _env: Env,
    count: i32,
}

#[derive(Serialize, Deserialize)]
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
