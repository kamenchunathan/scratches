use bevy::{
    asset::{AssetLoader, AsyncReadExt},
    prelude::*,
    scene::ron,
};
use serde::{Deserialize, Serialize};
use thiserror::Error;

use crate::critter::{CritterDef, CritterRegistry};

#[derive(Error, Debug)]
pub enum CritterRegistryLoadError {
    #[error("IO error")]
    Io(#[from] std::io::Error),

    #[error("Deserialisation error")]
    Ron(#[from] ron::de::SpannedError),
}

/// Deserialised form of a single critter manifest file.
/// The loader reads all `.critter.ron` files and merges them into one `CritterRegistry`.
#[derive(Serialize, Deserialize)]
struct CritterManifest {
    pub defs: Vec<CritterDef>,
}

pub struct CritterRegistryLoader;

impl AssetLoader for CritterRegistryLoader {
    type Asset = CritterRegistry;
    type Settings = ();
    type Error = CritterRegistryLoadError;

    fn load(
        &self,
        reader: &mut dyn bevy::asset::io::Reader,
        _: &Self::Settings,
        _: &mut bevy::asset::LoadContext,
    ) -> impl bevy::tasks::ConditionalSendFuture<Output = Result<Self::Asset, Self::Error>> {
        async {
            let mut buf = String::new();
            reader.read_to_string(&mut buf).await?;
            let manifest: CritterManifest = ron::from_str(&buf)?;
            Ok(CritterRegistry {
                defs: manifest.defs,
            })
        }
    }

    fn extensions(&self) -> &[&str] {
        &["critter.ron"]
    }
}
