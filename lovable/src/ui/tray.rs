use bevy::{ecs::system::SystemState, prelude::*};
use tray_icon::{TrayIconBuilder, menu::Menu};

use crate::preferences::{Preferences, PreferencesHandle};

#[derive(Debug, Resource)]
/// Changing these updates system tray icon
pub struct SystemTray {
    pub icon: Handle<Image>,

    pub tooltip: String,
}

/// Wraps the `tray_icon::TrayIcon` handle which must be kept alive for the tray icon
/// to remain on the system tray
/// Inserted as a nonsend bevy resource as `tray_icon::TrayIcon` is `!Send`
pub struct PlatformTrayIcon(tray_icon::TrayIcon);

impl PlatformTrayIcon {
    // TODO: anyhow error?
    pub fn set_icon(&mut self, img: &Image) -> Result<(), String> {
        let rgba = img
            .clone()
            .try_into_dynamic()
            .map_err(|e| format!("Failed conversion into a dynamic image {e}"))?
            .into_rgba8()
            .into_raw();

        let icon = tray_icon::Icon::from_rgba(rgba, img.width(), img.height())
            .map_err(|e| format!("{e}"))?;
        if let Err(e) = self.0.set_icon(Some(icon)) {
            error!(error=?e, "Unable to set tray icon");
        };

        Ok(())
    }

    pub fn set_menu(&mut self, menu: tray_icon::menu::Menu) -> Result<(), ()> {
        self.0.set_menu(Some(Box::new(menu)));
        Ok(())
    }
}

pub fn load_tray_assets(mut commands: Commands, asset_server: Res<AssetServer>) {
    commands.insert_resource(SystemTray {
        icon: asset_server.load("icons/tray.png"),
        tooltip: "Lovable — desktop companions".into(),
    });
}

pub fn create_tray(world: &mut World) {
    let mut system_state: SystemState<(
        EventReader<AssetEvent<Image>>,
        Res<SystemTray>,
        Res<PreferencesHandle>,
        Res<Assets<Preferences>>,
        Option<NonSend<PlatformTrayIcon>>,
    )> = SystemState::new(world);
    let (mut image_events, tray, prefs_handle, prefs_store, platform_icon) =
        system_state.get_mut(world);

    if platform_icon.is_some() {
        return;
    }

    let tray_assets_loaded = image_events.read().any(|ev| match ev {
        AssetEvent::LoadedWithDependencies { id } => *id == tray.icon.id(),
        _ => false,
    });
    if !tray_assets_loaded {
        return;
    }

    let prefs = prefs_store
        .get(&prefs_handle.0)
        .expect("Preferences not loaded");

    if !prefs.app_settings.show_tray_icon {
        info!("Tray icon disabled in preferences");
        return;
    }

    if world.get_non_send_resource::<PlatformTrayIcon>().is_some() {
        return;
    }

    let (rgba, w, h) = {
        let tray = world.resource::<SystemTray>();
        let images = world.resource::<Assets<Image>>();

        let Some(img) = images.get(&tray.icon) else {
            error!("Tray image handle not in image assets");
            return;
        };

        info!(
            format = ?img.texture_descriptor.format,
            width = img.width(),
            height = img.height(),
            "Converting tray icon image to rgba"
        );

        let rgba = img
            .clone()
            .try_into_dynamic()
            .map(|a| a.into_rgba8().into_raw())
            .expect("Failed conversion into a dynamic image");

        (rgba, img.width(), img.height())
    };

    let icon = tray_icon::Icon::from_rgba(rgba, w, h).expect("Unable to create icon");

    match TrayIconBuilder::new()
        .with_title("Lovable")
        .with_icon(icon)
        .with_menu(Box::new(Menu::new()))
        .build()
    {
        Ok(raw) => {
            world.insert_non_send_resource(PlatformTrayIcon(raw));
        }
        Err(e) => {
            error!("Error in building tray icon: {e}")
        }
    }
}

// pub fn update_tray() {}
