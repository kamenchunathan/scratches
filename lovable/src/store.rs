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

pub fn appdata_asset_source() -> AssetSourceBuilder {
    let path = match app_data_dir() {
        Some(t) => t,
        None => {
            warn!(
                "Unable to create app data directory Any settings you make will not be persisted",
            );
            std::env::temp_dir()
        }
    };
    AssetSourceBuilder::platform_default(path.as_os_str().to_str().unwrap(), None)
}
