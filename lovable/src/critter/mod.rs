use bevy::{asset::uuid, ecs::component::Component, math::Vec2, reflect::Reflect};
use serde::{Deserialize, Serialize};

mod animator;

/// Unique identifier for a critter instance
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize, Reflect)]
pub struct CritterId(pub uuid::Uuid);

impl CritterId {
    pub fn new() -> Self {
        Self(uuid::Uuid::new_v4())
    }
}

/// Data model for the critters
#[derive(Debug, Clone, Deserialize, Serialize, Component, Reflect)]
pub struct Critter {
    /// Unique identifier
    pub id: CritterId,

    /// Display name (user customizable)
    pub name: String,

    /// Current position on screen (in pixels from top-left of assigned monitor)
    pub position: Vec2,

    /// Size multiplier
    pub scale: f32,

    /// Transparency
    pub opacity: f32,
}
