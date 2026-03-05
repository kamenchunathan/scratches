use std::path::PathBuf;

use bevy::{asset::io::AssetSourceBuilder, log::warn};

fn app_data_dir() -> Option<PathBuf> {
    let data_dir = if cfg!(target_os = "windows") {
        PathBuf::from(std::env::var("LOCALAPPDATA").ok()?)
    } else if cfg!(target_os = "linux") {
        PathBuf::from(std::env::var("HOME").ok()?)
            .join(".local")
            .join("share")
    } else {
        return None;
    };
    Some(data_dir.join("lovable"))
}

fn fallback_dir(name: &str) -> PathBuf {
    warn!(
        "Unable to resolve {} directory — changes will not be persisted",
        name
    );
    std::env::temp_dir()
}

/// Asset source for user preferences and save data (`appdata://`).
pub fn appdata_asset_source() -> AssetSourceBuilder {
    let path = app_data_dir().unwrap_or_else(|| fallback_dir("app data"));
    AssetSourceBuilder::platform_default(path.as_os_str().to_str().unwrap(), None)
}

/// Asset source for downloaded critter definitions (`critters://`).
/// Shipped critters live in the normal `assets/` tree; this source is for
/// user-installed additions placed in `<data dir>/critters/`.
pub fn critters_asset_source() -> AssetSourceBuilder {
    let path = app_data_dir()
        .unwrap_or_else(|| fallback_dir("critters"))
        .join("critters");
    AssetSourceBuilder::platform_default(path.as_os_str().to_str().unwrap(), None)
}
