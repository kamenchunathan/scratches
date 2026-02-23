use bevy::{
    asset::{AssetLoadFailedEvent, AssetPath},
    prelude::*,
    scene::ron,
    tasks::AsyncComputeTaskPool,
};

use crate::{critter::CritterRegistry, preferences::RegistryHandle};

/// Builtin critter manifest embedded directly into the binary at compile time.
const BUILTIN_MANIFEST: &str = include_str!("../../assets/critters/builtin.critter.ron");

/// Runs during `AppState::Loading` when the registry asset fails to load —
/// which happens on first launch because the writable store is empty.
///
/// On parse failure, logs the error and immediately inserts an in-memory
/// registry built from the embedded manifest bytes rather than silently
/// presenting an empty registry.
pub fn install_builtin_critters_if_needed(
    mut events: EventReader<AssetLoadFailedEvent<CritterRegistry>>,
    mut registry_handle: ResMut<RegistryHandle>,
    mut registry_store: ResMut<Assets<CritterRegistry>>,
    asset_server: Res<AssetServer>,
) {
    for event in events.read() {
        error!(
            "Critter registry failed to load ({}): {}",
            event.path, event.error
        );

        // Try to parse the embedded manifest so we always have something usable.
        match ron::from_str::<BuiltinManifest>(BUILTIN_MANIFEST) {
            Ok(manifest) => {
                info!(
                    "Falling back to {} embedded critter(s)",
                    manifest.defs.len()
                );
                let registry = CritterRegistry {
                    defs: manifest.defs,
                };
                registry_store.insert(&registry_handle.0, registry);
            }
            Err(parse_err) => {
                error!("Failed to parse embedded builtin manifest: {parse_err}");
                // Insert an empty registry so loading can complete.
                registry_store.insert(&registry_handle.0, CritterRegistry::default());
            }
        }

        // Also write the embedded manifest to the writable critters store so
        // subsequent launches can load it from disk normally.
        info!("Writing builtin manifest to critters store for future launches");
        AsyncComputeTaskPool::get()
            .spawn({
                let asset_server = asset_server.clone();
                async move {
                    let asset_path = AssetPath::parse("critters://manifest.critter.ron");
                    let source = asset_server
                        .get_source(asset_path.source())
                        .expect("critters:// asset source must be registered");
                    let writer = source
                        .writer()
                        .expect("critters:// asset source must be writable");
                    if let Err(e) = writer
                        .write_bytes(asset_path.path(), BUILTIN_MANIFEST.as_bytes())
                        .await
                    {
                        error!("Failed to write builtin critter manifest: {e}");
                    }
                }
            })
            .detach();

        // Also re-queue the handle so subsequent hot-reloads pick up the file.
        registry_handle.0 = asset_server.load("critters://manifest.critter.ron");
    }
}

/// Deserialised form of the builtin manifest. Mirrors `CritterManifest` in the
/// loader but is kept private here since it is only used for fallback parsing.
#[derive(serde::Deserialize)]
struct BuiltinManifest {
    pub defs: Vec<crate::critter::CritterDef>,
}
