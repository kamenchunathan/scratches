use bevy::{
    log::tracing,
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

/// Added to a `CritterWindow` entity that is scheduled for removal.
/// The render entities (camera, mesh) are despawned in the same tick this
/// marker is inserted. The window entity itself is despawned one tick later,
/// giving the render world one full extraction cycle to drop its reference to
/// the now-gone camera before the window target disappears.
#[derive(Component)]
pub struct CritterWindowPendingDespawn;

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

    let size = 128.0 * critter.scale.max(0.25);

    commands.spawn((
        Window {
            title: critter.name.clone(),
            decorations: false,
            transparent: true,
            window_level: WindowLevel::AlwaysOnTop,
            skip_taskbar: true,
            has_shadow: false,
            composite_alpha_mode: bevy::window::CompositeAlphaMode::Inherit,
            resizable: false,
            movable_by_window_background: critter.interactible,
            resolution: WindowResolution::new(size, size),
            position: WindowPosition::At(abs_pos),
            focused: false,
            window_theme: None,
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

// ── Mark-and-sweep window lifecycle ──────────────────────────────────────────
//
// Despawning a Window entity while a Camera still holds a RenderTarget pointing
// at it causes a render-thread deadlock on Windows (Vulkan). The fix is a
// two-phase teardown:
//
//   Phase 1  (mark_critter_windows_for_despawn)
//     • Decides which windows need to open or close this tick.
//     • Opens new windows immediately (safe — no render state to clean up).
//     • For windows that must close:
//         1. Despawns all CritterRenderEntity nodes (camera, mesh) immediately.
//         2. Tags the Window entity with CritterWindowPendingDespawn.
//         3. Flushes the current OS position back into Preferences.
//     • Never touches an entity that is already tagged.
//
//   Phase 2  (despawn_pending_critter_windows)
//     • Runs in the same schedule, after Phase 1.
//     • Despawns every Window entity carrying CritterWindowPendingDespawn.
//     • By the time this runs the render world has already extracted a frame
//       without the camera, so the window reference is safe to remove.

/// Phase 1 — evaluate desired state and begin teardown of closing windows.
pub fn mark_critter_windows_for_despawn(
    mut commands: Commands,
    prefs_handle: Res<PreferencesHandle>,
    mut prefs_store: ResMut<Assets<Preferences>>,
    existing_windows: Query<
        (Entity, &CritterWindow, &Window),
        Without<CritterWindowPendingDespawn>,
    >,
    render_entities: Query<(Entity, &CritterRenderEntity)>,
    monitors: Query<&Monitor>,
) {
    if !prefs_store.is_changed() {
        return;
    }

    let Some(prefs) = prefs_store.get_mut(&prefs_handle.0) else {
        return;
    };

    for critter in &mut prefs.critters {
        let window_state = existing_windows
            .iter()
            .find(|(_, cw, _)| cw.critter_id == critter.id)
            .map(|(entity, _, window)| (entity, window.position.clone()));

        match (critter.is_visible, window_state) {
            (true, None) => {
                spawn_window_for_critter(&mut commands, critter, &monitors);
            }

            (false, Some((window_entity, pos))) => {
                // 1. Persist the current dragged position before closing.
                if let WindowPosition::At(abs) = pos {
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
                        .unwrap_or((IVec2::ZERO, critter.monitor_fingerprint));

                    critter.position =
                        Vec2::new((abs.x - origin.x) as f32, (abs.y - origin.y) as f32);
                    critter.monitor_fingerprint = new_fp;
                }

                // 2. Despawn render entities FIRST — camera must go before its
                //    window target or the render thread will deadlock.
                for (render_entity, marker) in &render_entities {
                    if marker.critter_id == critter.id {
                        tracing::debug!(
                            critter_id = %critter.id.0,
                            ?render_entity,
                            "despawning render entity before window"
                        );
                        commands.entity(render_entity).despawn();
                    }
                }

                // 3. Tag the window for despawn in Phase 2.
                tracing::debug!(
                    critter_id = %critter.id.0,
                    ?window_entity,
                    "marking critter window for deferred despawn"
                );
                commands
                    .entity(window_entity)
                    .insert(CritterWindowPendingDespawn);
            }

            // Already in the correct state — nothing to do.
            _ => {}
        }
    }

    // ── Orphan cleanup — windows for deleted critters ─────────────────────────

    let live_critter_ids: Vec<CritterId> = prefs.critters.iter().map(|c| c.id).collect();

    for (window_entity, cw, _) in &existing_windows {
        if !live_critter_ids.contains(&cw.critter_id) {
            // Despawn render entities first.
            for (render_entity, marker) in &render_entities {
                if marker.critter_id == cw.critter_id {
                    tracing::debug!(
                        critter_id = %cw.critter_id.0,
                        ?render_entity,
                        "despawning orphaned render entity"
                    );
                    commands.entity(render_entity).despawn();
                }
            }

            tracing::debug!(
                critter_id = %cw.critter_id.0,
                ?window_entity,
                "marking orphaned critter window for deferred despawn"
            );
            commands
                .entity(window_entity)
                .insert(CritterWindowPendingDespawn);
        }
    }
}

/// Phase 2 — despawn windows that were tagged in Phase 1.
///
/// Runs after Phase 1 in the same schedule. The render world has already
/// extracted one frame without the cameras that targeted these windows, so it
/// is safe to remove the window entities.
pub fn despawn_pending_critter_windows(
    mut commands: Commands,
    pending: Query<(Entity, &CritterWindow), With<CritterWindowPendingDespawn>>,
) {
    for (entity, cw) in &pending {
        tracing::debug!(
            critter_id = %cw.critter_id.0,
            ?entity,
            "despawning pending critter window"
        );
        commands.entity(entity).despawn();
    }
}

/// Writes the current window position back to `Preferences` whenever the window moves.
/// Positions are stored monitor-relative so they survive monitor reconfigurations.
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
