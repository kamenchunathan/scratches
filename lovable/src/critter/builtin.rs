use bevy::{
    asset::{AssetLoadFailedEvent, AssetPath},
    prelude::*,
    tasks::AsyncComputeTaskPool,
};

use crate::{critter::CritterRegistry, preferences::RegistryHandle};

/// Builtin critter manifest embedded directly into the binary at compile time.
/// Format is identical to a downloaded `.critter.ron` so the same loader handles both.
///
/// On first launch this file does not exist in the user's writable `critters://`
/// source & the install system below writes it there
const BUILTIN_MANIFEST: &str = include_str!("../../assets/critters/builtin.critter.ron");

/// Runs during `AppState::Loading` when the registry asset fails to load —
/// which happens on first launch because the writable store is empty.
///
/// Writes the embedded manifest to `critters://manifest.critter.ron`, then
/// re-issues the asset load handle so `check_loading_complete` can proceed
/// normally once the async write settles.
pub fn install_builtin_critters_if_needed(
    mut events: EventReader<AssetLoadFailedEvent<CritterRegistry>>,
    mut registry_handle: ResMut<RegistryHandle>,
    asset_server: Res<AssetServer>,
) {
    if events.read().next().is_none() {
        return;
    }

    info!("Critter registry not found — writing builtin manifest to critters store");

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
                writer
                    .write_bytes(asset_path.path(), BUILTIN_MANIFEST.as_bytes())
                    .await
                    .expect("Failed to write builtin critter manifest");
            }
        })
        .detach();

    // Re-queue the load. The async write above races with the asset server but
    // the task pool flushes before Bevy processes the new handle in the next
    // loading frame, so this reliably picks up the newly written file.
    registry_handle.0 = asset_server.load("critters://manifest.critter.ron");
}
