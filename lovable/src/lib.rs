mod camera_controller;
mod critter;
mod preferences;
mod store;
mod ui;

use bevy::{asset::io::AssetSourceId, prelude::*, window::WindowTheme};

use crate::{
    preferences::{
        Preferences, PreferencesLoader, access_prefs, monitor_preferences_loading,
        save_preferences_on_exit,
    },
    ui::LovableUI,
};

pub struct Lovable;

impl Plugin for Lovable {
    fn build(&self, app: &mut bevy::app::App) {
        let app_data = store::appdata_asset_source();

        app.register_asset_source(AssetSourceId::new(Some("appdata")), app_data)
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
            .init_asset::<Preferences>()
            .add_systems(Startup, access_prefs)
            .add_systems(
                Update,
                (
                    monitor_preferences_loading,
                    save_preferences_on_exit,
                    quit_on_esc,
                ),
            );
    }
}

fn quit_on_esc(keys: ResMut<ButtonInput<KeyCode>>, mut app_exit_events: EventWriter<AppExit>) {
    if keys.just_pressed(KeyCode::Escape) | keys.just_pressed(KeyCode::KeyQ) {
        app_exit_events.write(AppExit::Success);
    }
}

// critter window settings: Some(Window {
//                     name: Some("Lovable".to_string()),
//                     decorations: false,
//                     transparent: true,
//                     composite_alpha_mode: bevy::window::CompositeAlphaMode::Opaque,
//                     window_level: bevy::window::WindowLevel::AlwaysOnTop,
//                     skip_taskbar: true,
//                     movable_by_window_background: true,
//                     resolution: WindowResolution::new(WINDOW_SIZE as f32, WINDOW_SIZE as f32),
//                     ..default()
//                 }),
//

// fn set_initial_window_position(monitors: Query<&Monitor>, mut win_query: Query<&mut Window>) {
//     if let Ok(mut window) = win_query.single_mut() {
//         for monitor in monitors {
//             window.position = WindowPosition::At(IVec2 {
//                 x: (monitor.physical_width - WINDOW_SIZE) as i32,
//                 y: (monitor.physical_height - WINDOW_SIZE) as i32,
//             })
//         }
//     } else {
//         error!("No window created. Should not happen");
//     }
// }
