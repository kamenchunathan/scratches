use std::{collections::HashMap, sync::Arc};

use futures::{lock::Mutex, StreamExt};
use serde::{Deserialize, Serialize};
use tracing::info;
use wasm_bindgen_futures::spawn_local;
use worker::*;

#[derive(Serialize, Deserialize, Debug)]
pub struct ClientMessage {
    #[serde(rename = "type")]
    pub message_type: String,
    pub payload: String,
}

#[derive(Serialize, Debug)]
pub struct ServerMessage {
    #[serde(rename = "type")]
    pub message_type: String,
    pub sender: String,
    pub payload: String,
}

#[durable_object]
#[derive(Debug)]
struct Room {
    connected_clients: Arc<Mutex<HashMap<String, WebSocket>>>,
}

#[durable_object]
impl DurableObject for Room {
    fn new(state: State, _env: Env) -> Self {
        Self {
            connected_clients: Arc::new(Mutex::new(HashMap::new())),
        }
    }

    async fn fetch(&mut self, req: Request) -> Result<Response> {
        let WebSocketPair { client, server } = WebSocketPair::new()?;
        server.accept()?;

        let client_id = format!("user_{}", uuid::Uuid::new_v4());
        info!("Client {} connected", client_id);

        {
            let mut clients = self.connected_clients.lock().await;
            clients.insert(client_id.clone(), server.clone());
        }

        let clients_arc = Arc::clone(&self.connected_clients);
        spawn_local(async move {
            let mut event_stream = server.events().unwrap();
            while let Some(event) = event_stream.next().await {
                match event {
                    Ok(WebsocketEvent::Message(message)) => {
                        info!(?message, "Received message from {}", client_id);
                        if let Some(text) = message.text() {
                            match serde_json::from_str::<ClientMessage>(&text) {
                                Ok(client_msg) => {
                                    if client_msg.message_type == "message" {
                                        let server_msg = ServerMessage {
                                            message_type: "message".to_string(),
                                            sender: client_id.clone(),
                                            payload: client_msg.payload,
                                        };
                                        let serialized_msg = serde_json::to_string(&server_msg)
                                            .expect("Failed to serialize server message");

                                        let clients = clients_arc.lock().await;
                                        for (_, c) in clients.iter() {
                                            if let Err(e) = c.send_with_str(&serialized_msg) {
                                                info!(
                                                    "Failed to send message to client {}: {:?}",
                                                    client_id, e
                                                );
                                            } else {
                                                info!(
                                                    "Successfully sent message to client {}",
                                                    client_id
                                                );
                                            }
                                        }
                                    }
                                }
                                Err(e) => info!("Failed to parse client message: {:?}", e),
                            }
                        }
                    }
                    Ok(WebsocketEvent::Close(_)) => {
                        info!("Client {} disconnected", client_id);
                        let mut clients = clients_arc.lock().await;
                        clients.retain(|id, _| id != &client_id);
                        break;
                    }
                    Err(e) => {
                        info!("WebSocket error for client {}: {:?}", client_id, e);
                        let mut clients = clients_arc.lock().await;
                        clients.retain(|id, _| id != &client_id);
                        break;
                    }
                }
            }
        });

        Response::from_websocket(client)
    }
}
