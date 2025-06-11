use serde::{Deserialize, Serialize};
use worker::*;

#[durable_object]
struct Counter {
    state: State,
    _env: Env,
    count: i32,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct CounterResponse {
    pub count: i32,
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
