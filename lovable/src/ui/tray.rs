use bevy::prelude::*;
use tray_icon::{TrayIconBuilder, menu::Menu};

use crate::preferences::{Preferences, PreferencesHandle};

#[derive(Debug, Resource)]
pub struct Tray {
    icon: Handle<Image>,
}

pub fn load_tray_icon(mut commands: Commands, asset_server: Res<AssetServer>) {
    commands.insert_resource(Tray {
        icon: asset_server.load("icons/tray.png"),
    });
}

pub fn create_tray(
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    tray: Res<Tray>,
    images: Res<Assets<Image>>,
) {
    let prefs = prefs_store
        .get(&prefs_handle.0)
        .expect("Preferences not loaded");

    if prefs.app_settings.show_tray_icon {
        info!("Hello world");
        let Some(tray_icon_img) = images.get(&tray.icon) else {
            return;
        };

        info!("Hello world");

        match TrayIconBuilder::new()
            .with_title("Lovable")
            .with_icon(
                tray_icon::Icon::from_rgba(
                    tray_icon_img.data.clone().expect("Unable to load data"),
                    tray_icon_img.width(),
                    tray_icon_img.height(),
                )
                .expect("wrong icon format"),
            )
            .with_menu(Box::new(Menu::new()))
            .build()
        {
            Ok(tray_icon) => {
                // TODO: Add this as a resource to be able to mutate the tray icon later
                info!("Tray icon built")
            }
            Err(e) => {
                error!("Error in building tray icon")
            }
        }
    }
}

// pub fn update_tray() {}
