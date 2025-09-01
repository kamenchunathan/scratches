use bevy::prelude::*;

fn main() {
    App::new()
        .add_plugins(DefaultPlugins.set(WindowPlugin {
            primary_window: Some(Window {
                name: Some("Lovable".to_string()),
                decorations: false,
                transparent: true,
                composite_alpha_mode: bevy::window::CompositeAlphaMode::Opaque,
                window_level: bevy::window::WindowLevel::AlwaysOnTop,
                skip_taskbar: true,
                movable_by_window_background: true,
                position: WindowPosition::At(IVec2::splat(100)),
                ..default()
            }),
            ..default()
        }))
        .add_systems(Startup, setup)
        .add_systems(Update, (update, quit_on_esc))
        .run();
}

fn setup(mut commands: Commands, asset_server: Res<AssetServer>) {
    commands.spawn(Camera2d);

    let sprite_handle = asset_server.load("Sprite-0001.png");
    commands.spawn((Sprite::from_image(sprite_handle.clone()),));
}

fn update(mut query: Query<&mut Transform, With<Sprite>>, time: Res<Time>) {
    for mut transform in &mut query {
        transform.translation.y += time.delta_secs() * 20.0;
    }
}

fn quit_on_esc(keys: ResMut<ButtonInput<KeyCode>>, mut app_exit_events: EventWriter<AppExit>) {
    if keys.just_pressed(KeyCode::Escape) | keys.just_pressed(KeyCode::KeyQ) {
        app_exit_events.write(AppExit::Success);
    }
}
