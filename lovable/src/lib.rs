mod critter;
mod preferences;
mod store;
mod ui;

use bevy::{asset::io::AssetSourceId, prelude::*, window::WindowTheme};

use crate::{
    critter::{
        CritterDef, CritterRegistry,
        builtin::install_builtin_critters_if_needed,
        loader::CritterRegistryLoader,
        window::{
            persist_critter_window_positions, spawn_critter_windows, spawn_placeholder_geometry,
            sync_critter_render_despawn, sync_critter_window_visibility,
        },
    },
    preferences::{
        Preferences, PreferencesLoader, check_loading_complete, create_default_prefs_on_fail,
        query_monitor_info, save_preferences_on_exit, start_loading_assets,
    },
    ui::{LovableUI, build_app_screen_from_prefs},
};

pub struct Lovable;

#[derive(States, Debug, Clone, PartialEq, Eq, Hash, Default)]
pub enum AppState {
    #[default]
    /// asset loading, platform queries, first-run detection.
    Loading,

    /// normal application loop; nothing substantial runs before this.
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
                spawn_critter_windows.after(build_app_screen_from_prefs),
            )
            .add_systems(
                Update,
                (
                    spawn_placeholder_geometry,
                    sync_critter_window_visibility,
                    sync_critter_render_despawn.after(sync_critter_window_visibility),
                    persist_critter_window_positions,
                    save_preferences_on_exit,
                    quit_on_esc,
                )
                    .run_if(in_state(AppState::Running)),
            );
    }
}

fn quit_on_esc(keys: ResMut<ButtonInput<KeyCode>>, mut app_exit: EventWriter<AppExit>) {
    if keys.just_pressed(KeyCode::Escape) || keys.just_pressed(KeyCode::KeyQ) {
        app_exit.write(AppExit::Success);
    }
}
