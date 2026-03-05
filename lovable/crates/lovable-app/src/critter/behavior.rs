use bevy::{prelude::*, window::Monitor};

use crate::preferences::{Preferences, PreferencesHandle, generate_monitor_fingerprint};

use super::window::CritterWindow;

/// How a critter window moves each frame.
#[derive(Component, Debug, Clone)]
pub enum CritterBehavior {
    /// Pinned to the bottom-right corner of its assigned monitor.
    Still,

    /// Bounces around its assigned monitor like a DVD screensaver logo.
    DvdBounce {
        /// Current velocity in physical pixels per second.
        velocity: Vec2,
    },
}

const BOUNCE_SPEED: f32 = 200.0; // px / s
const CORNER_MARGIN: f32 = 24.0; // px from screen edge for the still critter

/// Attach a `CritterBehavior` to every `CritterWindow` that doesn't already
/// have one. The first critter (by index in `Preferences`) gets `Still`;
/// the second gets `DvdBounce`. Any further critters also get `DvdBounce`.
pub fn assign_critter_behaviors(
    mut commands: Commands,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    windows: Query<(Entity, &CritterWindow), Without<CritterBehavior>>,
) {
    let Some(prefs) = prefs_store.get(&prefs_handle.0) else {
        return;
    };

    for (entity, cw) in &windows {
        // Find the position of this critter in the ordered prefs list.
        let index = prefs
            .critters
            .iter()
            .position(|c| c.id == cw.critter_id)
            .unwrap_or(1);

        let behavior = if index == 0 {
            CritterBehavior::Still
        } else {
            // Give each bouncing critter a slightly different starting angle so
            // two bouncing critters don't perfectly overlap.
            let angle = std::f32::consts::FRAC_PI_4 + index as f32 * 0.4;
            CritterBehavior::DvdBounce {
                velocity: Vec2::new(angle.cos(), angle.sin()) * BOUNCE_SPEED,
            }
        };

        commands.entity(entity).insert(behavior);
    }
}

/// Every frame: move bouncing windows and keep still windows pinned to the
/// bottom-right corner of their monitor.
pub fn tick_critter_behaviors(
    time: Res<Time>,
    monitors: Query<&Monitor>,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    mut windows: Query<(&mut Window, &CritterWindow, &mut CritterBehavior)>,
) {
    let dt = time.delta_secs();
    let Some(prefs) = prefs_store.get(&prefs_handle.0) else {
        return;
    };

    for (mut window, cw, mut behavior) in &mut windows {
        // Resolve which monitor this critter belongs to.
        let critter = prefs.critters.iter().find(|c| c.id == cw.critter_id);
        let monitor_fp = critter.map(|c| c.monitor_fingerprint).unwrap_or(0);

        let monitor = monitors
            .iter()
            .find(|m| generate_monitor_fingerprint(m) == monitor_fp)
            .or_else(|| monitors.iter().next());

        let Some(monitor) = monitor else {
            continue;
        };

        let mon_x = monitor.physical_position.x as f32;
        let mon_y = monitor.physical_position.y as f32;
        let mon_w = monitor.physical_width as f32;
        let mon_h = monitor.physical_height as f32;

        let win_w = window.resolution.physical_width() as f32;
        let win_h = window.resolution.physical_height() as f32;

        match behavior.as_mut() {
            CritterBehavior::Still => {
                // Anchor to bottom-right with a small margin.
                let target_x = (mon_x + mon_w - win_w - CORNER_MARGIN) as i32;
                let target_y = (mon_y + mon_h - win_h - CORNER_MARGIN) as i32;
                window.position = WindowPosition::At(IVec2::new(target_x, target_y));
            }

            CritterBehavior::DvdBounce { velocity } => {
                // Retrieve current absolute position.
                let WindowPosition::At(pos) = window.position else {
                    continue;
                };

                let mut x = pos.x as f32 + velocity.x * dt;
                let mut y = pos.y as f32 + velocity.y * dt;

                // Clamp to monitor bounds and reflect velocity on collision.
                let min_x = mon_x;
                let max_x = mon_x + mon_w - win_w;
                let min_y = mon_y;
                let max_y = mon_y + mon_h - win_h;

                if x <= min_x {
                    x = min_x;
                    velocity.x = velocity.x.abs();
                } else if x >= max_x {
                    x = max_x;
                    velocity.x = -velocity.x.abs();
                }

                if y <= min_y {
                    y = min_y;
                    velocity.y = velocity.y.abs();
                } else if y >= max_y {
                    y = max_y;
                    velocity.y = -velocity.y.abs();
                }

                window.position = WindowPosition::At(IVec2::new(x as i32, y as i32));
            }
        }
    }
}
