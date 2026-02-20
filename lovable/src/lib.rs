mod critter;
mod preferences;
mod store;
mod ui;

use bevy::{asset::io::AssetSourceId, prelude::*, window::WindowTheme};

use crate::preferences::{
    PreferencesLoader, check_loading_complete, unpack_default_critter_manifest,
};
use crate::{
    critter::loader::CritterRegistryLoader,
    preferences::{
        Preferences, create_default_prefs_on_fail, query_monitor_info, save_preferences_on_exit,
        start_loading_assets,
    },
    ui::LovableUI,
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
            .init_asset::<crate::critter::CritterDef>()
            .init_asset::<crate::critter::CritterRegistry>()
            .init_state::<AppState>()
            .add_systems(Startup, start_loading_assets)
            .add_systems(
                Update,
                (
                    create_default_prefs_on_fail,
                    unpack_default_critter_manifest,
                    query_monitor_info,
                    check_loading_complete,
                )
                    .run_if(in_state(AppState::Loading)),
            )
            .add_systems(
                Update,
                (save_preferences_on_exit, quit_on_esc).run_if(in_state(AppState::Running)),
            );
    }
}

fn quit_on_esc(keys: ResMut<ButtonInput<KeyCode>>, mut app_exit: EventWriter<AppExit>) {
    if keys.just_pressed(KeyCode::Escape) || keys.just_pressed(KeyCode::KeyQ) {
        app_exit.write(AppExit::Success);
    }
}
