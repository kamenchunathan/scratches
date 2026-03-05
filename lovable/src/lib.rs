#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod critter;
mod preferences;
mod store;
mod ui;

use std::env;

use bevy::{asset::io::AssetSourceId, prelude::*, window::WindowTheme};

use crate::{
    critter::{
        CritterDef, CritterRegistry,
        behavior::{assign_critter_behaviors, tick_critter_behaviors},
        builtin::install_builtin_critters_if_needed,
        loader::CritterRegistryLoader,
        window::{
            despawn_pending_critter_windows, mark_critter_windows_for_despawn,
            persist_critter_window_positions, spawn_critter_windows, spawn_placeholder_geometry,
        },
    },
    preferences::{
        Preferences, PreferencesHandle, PreferencesLoader, check_loading_complete,
        create_default_prefs_on_fail, query_monitor_info, save_preferences_on_exit,
        start_loading_assets,
    },
    ui::{LovableUI, build_app_screen_from_prefs},
};

pub struct Lovable;

#[derive(States, Debug, Clone, PartialEq, Eq, Hash, Default)]
pub enum AppState {
    #[default]
    /// Asset loading, platform queries, first-run detection.
    Loading,

    /// Normal application loop; nothing substantial runs before this.
    Running,
}

impl Plugin for Lovable {
    fn build(&self, app: &mut bevy::app::App) {
        let appdata_source = store::appdata_asset_source();
        let critters_source = store::critters_asset_source();

        app.register_asset_source(AssetSourceId::new(Some("appdata")), appdata_source)
            .register_asset_source(AssetSourceId::new(Some("critters")), critters_source)
            .add_plugins((
                DefaultPlugins.set(WindowPlugin {
                    primary_window: Some(Window {
                        title: String::from("Lovable"),
                        window_theme: Some(WindowTheme::Dark),
                        decorations: false,
                        position: WindowPosition::Centered(MonitorSelection::Primary),
                        ..default()
                    }),
                    ..default()
                }),
                LovableUI,
            ))
            .register_asset_loader(PreferencesLoader)
            .register_asset_loader(CritterRegistryLoader)
            .init_asset::<Preferences>()
            .init_asset::<CritterDef>()
            .init_asset::<CritterRegistry>()
            .init_state::<AppState>()
            .add_systems(Startup, start_loading_assets)
            .add_systems(
                Update,
                (
                    create_default_prefs_on_fail,
                    install_builtin_critters_if_needed,
                    query_monitor_info,
                    check_loading_complete,
                )
                    .chain()
                    .run_if(in_state(AppState::Loading)),
            )
            .add_systems(
                OnEnter(AppState::Running),
                (
                    spawn_main_window,
                    configure_auto_launch,
                    (spawn_critter_windows).after(build_app_screen_from_prefs),
                ),
            )
            .add_systems(
                Update,
                (
                    spawn_placeholder_geometry,
                    mark_critter_windows_for_despawn,
                    despawn_pending_critter_windows.after(mark_critter_windows_for_despawn),
                    persist_critter_window_positions,
                    save_preferences_on_exit,
                    assign_critter_behaviors.after(spawn_placeholder_geometry),
                    tick_critter_behaviors.after(assign_critter_behaviors),
                )
                    .run_if(in_state(AppState::Running)),
            );
    }
}

///
fn spawn_main_window(
    mut commands: Commands,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
) {
    // let Some(prefs) = prefs_store.get(prefs_handle.0.id()) else {
    //     return;
    // };
    //
    // let should_be_visible = !prefs.app_settings.start_minimized;
    //
    // commands.spawn(Window {
    //     title: String::from("Lovable"),
    //     window_theme: Some(WindowTheme::Dark),
    //     decorations: false,
    //     position: WindowPosition::Centered(MonitorSelection::Primary),
    //     visible: should_be_visible,
    //     ..default()
    // });
}

/// Configures autolaunch on system startup based on user preferences.
fn configure_auto_launch(
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
) {
    let Some(prefs) = prefs_store.get(prefs_handle.0.id()) else {
        warn!("Unable to get preferences for autolaunch configuration");
        return;
    };

    let app_path = determine_app_path();

    let Ok(auto_launch) = auto_launch::AutoLaunchBuilder::new()
        .set_app_name("Lovable")
        .set_app_path(&app_path)
        .set_macos_launch_mode(auto_launch::MacOSLaunchMode::LaunchAgent)
        .build()
    else {
        error!("Failed to initialize AutoLaunch builder");
        return;
    };

    // Sync the system autolaunch state with the app preferences
    if prefs.auto_launch {
        if let Err(e) = auto_launch.enable() {
            error!("Failed to enable autolaunch: {}", e);
        } else {
            info!("Autolaunch enabled successfully");
        }
    } else {
        if let Err(e) = auto_launch.disable() {
            error!("Failed to disable autolaunch: {}", e);
        } else {
            info!("Autolaunch disabled successfully");
        }
    }
}

/// Gets the absolute path to the current executable.
fn determine_app_path() -> String {
    env::current_exe()
        .map(|path| path.to_string_lossy().into_owned())
        .unwrap_or_else(|_| {
            error!("Could not determine executable path, defaulting to 'lovable'");
            "lovable".to_string()
        })
}
