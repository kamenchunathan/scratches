use bevy::{asset::uuid, ecs::system::SystemState, prelude::*};
use tray_icon::{
    TrayIconBuilder,
    menu::{Menu, MenuItem, PredefinedMenuItem},
};

use crate::{
    critter::CritterId,
    preferences::{Preferences, PreferencesHandle},
    ui::messages::Msg,
};

#[derive(Debug, Resource)]
/// Changing these updates system tray icon
pub struct SystemTray {
    pub icon: Handle<Image>,

    pub tooltip: String,

    pub menu: TrayMenu,
}

#[derive(Debug, Clone)]
pub struct TrayMenu(Vec<TrayMenuItem>);

#[derive(Debug, Clone)]
pub enum TrayMenuItem {
    Label(String),
    Separator,
    Action {
        id: String,
        label: String,
        enabled: bool,
    },
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

mod menu_ids {
    pub(crate) const SHOW_WINDOW: &str = "show_window";
    pub(crate) const QUIT: &str = "quit";
    pub(crate) const TOGGLE_CRITTER_PREFIX: &str = "toggle_critter";
}

pub fn create_tray(
    mut commands: Commands,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    asset_server: Res<AssetServer>,
) {
    let prefs = prefs_store
        .get(&prefs_handle.0)
        .expect("Preferences not loaded");

    commands.insert_resource(SystemTray {
        icon: asset_server.load("icons/tray.png"),
        tooltip: "Lovable — desktop companions".into(),
        menu: populate_menu_from_prefs(prefs),
    });
}

pub fn build_platform_tray(world: &mut World) {
    let mut system_state: SystemState<(
        EventReader<AssetEvent<Image>>,
        Res<SystemTray>,
        Res<PreferencesHandle>,
        Res<Assets<Preferences>>,
        Res<Assets<Image>>,
        Option<NonSend<PlatformTrayIcon>>,
    )> = SystemState::new(world);
    let (mut image_events, tray, prefs_handle, prefs_store, images, platform_icon) =
        system_state.get(world);

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

    let icon =
        tray_icon::Icon::from_rgba(rgba, img.width(), img.height()).expect("Unable to create icon");

    let menu = build_platform_menu(&tray.menu);

    match TrayIconBuilder::new()
        .with_title("Lovable")
        .with_icon(icon)
        .with_menu(Box::new(menu))
        .with_tooltip(&tray.tooltip)
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

fn build_platform_menu(menu: &TrayMenu) -> tray_icon::menu::Menu {
    let platform_menu = tray_icon::menu::Menu::new();

    for item in &menu.0 {
        match item {
            TrayMenuItem::Label(text) => {
                let _ = platform_menu.append(&MenuItem::new(text, false, None));
            }
            TrayMenuItem::Separator => {
                let _ = platform_menu.append(&PredefinedMenuItem::separator());
            }
            TrayMenuItem::Action { id, label, enabled } => {
                let _ = platform_menu.append(&MenuItem::with_id(id, label, *enabled, None));
            }
        }
    }

    platform_menu
}

fn populate_menu_from_prefs(prefs: &Preferences) -> TrayMenu {
    let mut items = vec![
        TrayMenuItem::Label("Lovable".into()),
        TrayMenuItem::Separator,
        TrayMenuItem::Action {
            id: menu_ids::SHOW_WINDOW.to_string(),
            label: "Show window".to_string(),
            enabled: true,
        },
    ];

    for critter in &prefs.critters {
        let label = if critter.is_visible {
            format!("✓  {}", critter.name)
        } else {
            format!("      {}", critter.name)
        };
        items.push(TrayMenuItem::Action {
            id: format!("{}{}", menu_ids::TOGGLE_CRITTER_PREFIX, critter.id.0),
            label,
            enabled: true,
        });
    }

    if !prefs.critters.is_empty() {
        items.push(TrayMenuItem::Separator);
    }

    items.push(TrayMenuItem::Action {
        id: menu_ids::QUIT.to_string(),
        label: "Quit".to_string(),
        enabled: true,
    });

    TrayMenu(items)
}

pub fn poll_menu_events(mut writer: EventWriter<Msg>) {
    while let Ok(event) = tray_icon::menu::MenuEvent::receiver().try_recv() {
        match event.id.0.as_str() {
            menu_ids::SHOW_WINDOW => {
                writer.write(Msg::System(super::messages::SystemMsg::TrayShowHide));
            }
            menu_ids::QUIT => {
                writer.write(Msg::System(super::messages::SystemMsg::TrayQuit));
            }
            id => {
                if let Some(uuid_str) = id.strip_prefix(menu_ids::TOGGLE_CRITTER_PREFIX) {
                    match uuid::Uuid::parse_str(uuid_str) {
                        Ok(uuid) => {
                            writer.write(Msg::System(
                                super::messages::SystemMsg::TrayToggleCritter(CritterId(uuid)),
                            ));
                        }
                        _ => {
                            error!("Malformed critter uuid in system tray menu")
                        }
                    }
                } else {
                    error!("Unhandled tray menu event {id}");
                }
            }
        }
    }
}

pub fn update_tray() {}
