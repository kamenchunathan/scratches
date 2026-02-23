use bevy::{
    prelude::*,
    render::camera::RenderTarget,
    window::{Monitor, WindowLevel, WindowRef, WindowResolution},
};

use crate::{
    critter::{Critter, CritterId},
    preferences::{Preferences, PreferencesHandle, generate_monitor_fingerprint},
};

/// Marker attached to the Bevy `Window` entity that belongs to a critter.
#[derive(Component)]
pub struct CritterWindow {
    pub critter_id: CritterId,
}

/// Marker attached to rendering entities (camera, mesh) that live inside a
/// critter's window. Used to find and clean them up when the window closes.
#[derive(Component)]
pub struct CritterRenderEntity {
    pub critter_id: CritterId,
}

/// Spawns one `Window` entity per visible owned critter, placed at its saved
/// monitor-relative position on the correct monitor.
pub fn spawn_critter_windows(
    mut commands: Commands,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    monitors: Query<&Monitor>,
) {
    let Some(prefs) = prefs_store.get(&prefs_handle.0) else {
        return;
    };

    for critter in prefs.critters.iter().filter(|c| c.is_visible) {
        spawn_window_for_critter(&mut commands, critter, &monitors);
    }
}

pub(crate) fn spawn_window_for_critter(
    commands: &mut Commands,
    critter: &Critter,
    monitors: &Query<&Monitor>,
) {
    let monitor_origin = resolve_monitor_origin(critter.monitor_fingerprint, monitors);
    let abs_pos = IVec2::new(
        monitor_origin.x + critter.position.x as i32,
        monitor_origin.y + critter.position.y as i32,
    );

    // Window size scales with the critter's scale setting; base size 128 px.
    let size = 128.0 * critter.scale.max(0.25);

    commands.spawn((
        Window {
            title: critter.name.clone(),
            decorations: false,
            transparent: true,
            window_level: WindowLevel::AlwaysOnTop,
            skip_taskbar: true,
            resizable: false,
            movable_by_window_background: critter.interactible,
            resolution: WindowResolution::new(size, size),
            position: WindowPosition::At(abs_pos),
            ..default()
        },
        CritterWindow {
            critter_id: critter.id,
        },
    ));
}

/// Resolves the physical-pixel origin of the monitor whose fingerprint matches,
/// falling back to the primary (first reported) monitor if none matches.
fn resolve_monitor_origin(fingerprint: u64, monitors: &Query<&Monitor>) -> IVec2 {
    monitors
        .iter()
        .find(|m| generate_monitor_fingerprint(m) == fingerprint)
        .or_else(|| monitors.iter().next())
        .map(|m| IVec2::new(m.physical_position.x, m.physical_position.y))
        .unwrap_or(IVec2::ZERO)
}

/// Reacts to every `Added<CritterWindow>` — spawns a dedicated `Camera2d` that
/// renders into that window and a coloured triangle as a placeholder shape.
// TODO: Swap `Triangle2d` for a `SceneRoot` GLTF spawn when the animator is ready.
pub fn spawn_placeholder_geometry(
    mut commands: Commands,
    mut meshes: ResMut<Assets<Mesh>>,
    mut materials: ResMut<Assets<ColorMaterial>>,
    new_windows: Query<(Entity, &CritterWindow), Added<CritterWindow>>,
) {
    for (window_entity, cw) in &new_windows {
        // Stable hue: first byte of the UUID, mapped to [0, 360)
        let hue = (cw.critter_id.0.as_bytes()[0] as f32 / 255.0) * 360.0;
        let color = Color::hsl(hue, 0.75, 0.55);

        commands.spawn((
            Camera2d,
            Camera {
                target: RenderTarget::Window(WindowRef::Entity(window_entity)),
                clear_color: ClearColorConfig::Custom(Color::NONE),
                ..default()
            },
            CritterRenderEntity {
                critter_id: cw.critter_id,
            },
        ));

        // Triangle centred in the 128 × 128 logical window space.
        commands.spawn((
            Mesh2d(meshes.add(Triangle2d::new(
                Vec2::new(0.0, 46.0),
                Vec2::new(-40.0, -30.0),
                Vec2::new(40.0, -30.0),
            ))),
            MeshMaterial2d(materials.add(color)),
            Transform::default(),
            CritterRenderEntity {
                critter_id: cw.critter_id,
            },
        ));
    }
}

/// Watches `Preferences` for changes and opens or closes windows to match.
/// Handles three cases:
///   - Critter marked visible but no window exists  → open window
///   - Critter marked hidden but window exists      → despawn window
///   - Critter deleted entirely                     → despawn window
///
/// Rendering entities (camera, mesh) are cleaned up by
/// `sync_critter_render_despawn`, which runs after this system.
pub fn sync_critter_window_visibility(
    mut commands: Commands,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    existing_windows: Query<(Entity, &CritterWindow)>,
    monitors: Query<&Monitor>,
) {
    // Only run when preferences have actually been mutated this frame.
    if !prefs_store.is_changed() {
        return;
    }

    let Some(prefs) = prefs_store.get(&prefs_handle.0) else {
        return;
    };

    for critter in &prefs.critters {
        let window_entity = existing_windows
            .iter()
            .find(|(_, cw)| cw.critter_id == critter.id)
            .map(|(e, _)| e);

        match (critter.is_visible, window_entity) {
            (true, None) => spawn_window_for_critter(&mut commands, critter, &monitors),
            (false, Some(entity)) => {
                commands.entity(entity).despawn();
            }
            _ => {}
        }
    }

    // Windows for critters that were deleted
    for (entity, cw) in &existing_windows {
        if !prefs.critters.iter().any(|c| c.id == cw.critter_id) {
            commands.entity(entity).despawn();
        }
    }
}

/// Cleans up orphaned `CritterRenderEntity` nodes (cameras, meshes) after
/// their window has been despawned by `sync_critter_window_visibility`.
pub fn sync_critter_render_despawn(
    mut commands: Commands,
    windows: Query<&CritterWindow>,
    render_entities: Query<(Entity, &CritterRenderEntity)>,
) {
    for (entity, marker) in &render_entities {
        if !windows.iter().any(|cw| cw.critter_id == marker.critter_id) {
            commands.entity(entity).despawn();
        }
    }
}

/// Writes the current window position back to `Preferences` whenever the window moves
/// Positions are stored monitor-relative so they survive monitor reconfigurations
pub fn persist_critter_window_positions(
    windows: Query<(&Window, &CritterWindow), Changed<Window>>,
    monitors: Query<&Monitor>,
    prefs_handle: Res<PreferencesHandle>,
    mut prefs_store: ResMut<Assets<Preferences>>,
) {
    let Some(prefs) = prefs_store.get_mut(&prefs_handle.0) else {
        return;
    };

    for (window, cw) in &windows {
        let WindowPosition::At(abs) = window.position else {
            continue;
        };

        // Find the monitor whose rectangle contains the window's top-left corner.
        let containing_monitor = monitors.iter().find(|m| {
            abs.x >= m.physical_position.x
                && abs.x < m.physical_position.x + m.physical_width as i32
                && abs.y >= m.physical_position.y
                && abs.y < m.physical_position.y + m.physical_height as i32
        });

        let (origin, new_fp) = containing_monitor
            .map(|m| {
                (
                    IVec2::new(m.physical_position.x, m.physical_position.y),
                    generate_monitor_fingerprint(m),
                )
            })
            .unwrap_or_else(|| {
                // Window is off all known monitors (edge case); keep existing fp.
                let fallback_fp = prefs
                    .critters
                    .iter()
                    .find(|c| c.id == cw.critter_id)
                    .map(|c| c.monitor_fingerprint)
                    .unwrap_or(0);
                (IVec2::ZERO, fallback_fp)
            });

        let local_pos = Vec2::new((abs.x - origin.x) as f32, (abs.y - origin.y) as f32);

        if let Some(critter) = prefs.critters.iter_mut().find(|c| c.id == cw.critter_id) {
            critter.position = local_pos;
            critter.monitor_fingerprint = new_fp;
        }
    }
}
