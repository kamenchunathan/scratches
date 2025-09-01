use std::f32::consts::PI;

use bevy::{
    animation::graph::AnimationGraph,
    core_pipeline::tonemapping::Tonemapping,
    gltf::GltfAssetLabel,
    pbr::CascadeShadowConfigBuilder,
    prelude::*,
    scene::SceneInstanceReady,
    window::{Monitor, WindowResolution},
};

const WINDOW_SIZE: u32 = 256;
const GLTF_PATH: &str = "models/Lovable.glb";

fn main() {
    App::new()
        .add_plugins((DefaultPlugins.set(WindowPlugin {
            primary_window: Some(Window {
                name: Some("Lovable".to_string()),
                decorations: false,
                transparent: true,
                composite_alpha_mode: bevy::window::CompositeAlphaMode::Opaque,
                window_level: bevy::window::WindowLevel::AlwaysOnTop,
                skip_taskbar: true,
                movable_by_window_background: true,
                resolution: WindowResolution::new(WINDOW_SIZE as f32, WINDOW_SIZE as f32),
                ..default()
            }),
            ..default()
        }),))
        .add_systems(
            Startup,
            (set_initial_window_position, setup_scene, setup_camera),
        )
        .add_systems(Update, quit_on_esc)
        .run();
}

fn set_initial_window_position(monitors: Query<&Monitor>, mut win_query: Query<&mut Window>) {
    if let Ok(mut window) = win_query.single_mut() {
        for monitor in monitors {
            window.position = WindowPosition::At(IVec2 {
                x: (monitor.physical_width - WINDOW_SIZE) as i32,
                y: (monitor.physical_height - WINDOW_SIZE) as i32,
            })
        }
    } else {
        error!("No window created. Should not happen");
    }
}

#[derive(Component, Debug, Clone)]
struct JumpAnimation {
    graph: Handle<AnimationGraph>,
    index: AnimationNodeIndex,
}

fn setup_scene(
    mut commands: Commands,
    asset_server: Res<AssetServer>,
    mut graphs: ResMut<Assets<AnimationGraph>>,
) {
    let scene = SceneRoot(asset_server.load(GltfAssetLabel::Scene(0).from_asset(GLTF_PATH)));

    let (graph, index) = AnimationGraph::from_clip(
        asset_server.load(GltfAssetLabel::Animation(0).from_asset(GLTF_PATH)),
    );

    let jump_graph = graphs.add(graph);
    let jump_animation = JumpAnimation {
        graph: jump_graph,
        index,
    };

    commands
        .spawn((scene, jump_animation))
        .observe(play_animation);
}

fn quit_on_esc(keys: ResMut<ButtonInput<KeyCode>>, mut app_exit_events: EventWriter<AppExit>) {
    if keys.just_pressed(KeyCode::Escape) | keys.just_pressed(KeyCode::KeyQ) {
        app_exit_events.write(AppExit::Success);
    }
}

fn play_animation(
    event: Trigger<SceneInstanceReady>,
    mut commands: Commands,
    anims: Query<&JumpAnimation>,
    mut players: Query<&mut AnimationPlayer>,
    children: Query<&Children>,
) {
    if let Ok(anim) = anims.get(event.target()) {
        for child in children.iter_descendants(event.target()) {
            if let Ok(mut player) = players.get_mut(child) {
                player.play(anim.index).repeat();
            }

            commands
                .entity(child)
                .insert(AnimationGraphHandle(anim.graph.clone()));
        }
    }
}

fn setup_camera(
    mut commands: Commands,
    mut meshes: ResMut<Assets<Mesh>>,
    mut materials: ResMut<Assets<StandardMaterial>>,
) {
    commands.spawn((
        Transform::from_xyz(7.6, -8.7, 5.0).looking_at(Vec3::new(0.0, 1.0, 0.0), Vec3::Y),
        Tonemapping::None,
        Camera {
            clear_color: ClearColorConfig::Custom(Color::Srgba(Srgba::NONE)),
            ..default()
        },
        Camera3d::default(),
    ));

    commands.spawn((
        Mesh3d::from(meshes.add(Plane3d::default().mesh().size(5000.0, 5000.0))),
        MeshMaterial3d(materials.add(Color::Srgba(Srgba::BLUE))),
    ));

    commands.spawn((
        Transform::from_rotation(Quat::from_euler(EulerRot::ZYX, 0.0, 1.0, -PI / 4.)),
        DirectionalLight {
            shadows_enabled: true,
            ..default()
        },
        CascadeShadowConfigBuilder {
            first_cascade_far_bound: 200.0,
            maximum_distance: 400.0,
            ..default()
        }
        .build(),
    ));
}
