use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*, window::Monitor};

use crate::{
    critter::{Critter, CritterRegistry},
    preferences::{Preferences, generate_monitor_fingerprint},
    ui::{
        palette::Palette,
        state::{AppScreen, CrittersTabState, DeleteConfirmState, Tab},
        widgets::*,
    },
};

pub fn spawn_critters_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    screen: &AppScreen,
    prefs: &Preferences,
    registry: &CritterRegistry,
    monitors: &[(&Monitor, u64)],
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                display: Display::None,
                flex_direction: FlexDirection::Column,
                width: Val::Percent(100.0),
                ..default()
            },
            TabContent(Tab::Critters),
            CrittersTabRoot,
        ))
        .with_children(|tab| {
            spawn_owned_section(tab, screen, prefs, registry, monitors, palette);
            spawn_separator(tab, palette);
            spawn_available_section(tab, prefs, registry, palette);
        });
}

// ─── Owned critters ───────────────────────────────────────────────────────────

fn spawn_owned_section(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    screen: &AppScreen,
    prefs: &Preferences,
    registry: &CritterRegistry,
    monitors: &[(&Monitor, u64)],
    palette: &Palette,
) {
    let label = format!("Your Companions ({})", prefs.critters.len());
    spawn_section_header(parent, &label, palette);

    if prefs.critters.is_empty() {
        parent.spawn((
            Text::new("No critters adopted yet. Check out the gallery below!"),
            TextFont {
                font_size: 14.0,
                ..default()
            },
            TextColor(palette.muted_foreground),
            Node {
                margin: UiRect::vertical(Val::Px(16.0)),
                ..default()
            },
        ));
        return;
    }

    let (expanded_id, confirm_pending) = match screen {
        AppScreen::Main(m) => match &m.critters_tab {
            CrittersTabState::Expanded {
                critter_id,
                confirm,
            } => (Some(*critter_id), *confirm == DeleteConfirmState::Pending),
            CrittersTabState::Collapsed => (None, false),
        },
        _ => (None, false),
    };

    for critter in &prefs.critters {
        let def_name = registry
            .find(&critter.def_id)
            .map(|d| d.name.as_str())
            .unwrap_or("Unknown");

        spawn_critter_row(
            parent,
            critter,
            def_name,
            expanded_id,
            confirm_pending,
            monitors,
            palette,
        );
    }
}

fn spawn_critter_row(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    critter: &Critter,
    def_name: &str,
    expanded_id: Option<crate::critter::CritterId>,
    confirm_pending: bool,
    monitors: &[(&Monitor, u64)],
    palette: &Palette,
) {
    let is_expanded = expanded_id == Some(critter.id);
    let expand_display = if is_expanded {
        Display::Flex
    } else {
        Display::None
    };

    parent
        .spawn((
            Node {
                flex_direction: FlexDirection::Column,
                border: UiRect::all(Val::Px(1.0)),
                width: Val::Percent(100.0),
                margin: UiRect::bottom(Val::Px(8.0)),
                ..default()
            },
            BorderRadius::all(Val::Px(8.0)),
            BorderColor(palette.border),
            BackgroundColor(palette.card),
        ))
        .with_children(|row| {
            // ── Collapsed header ──────────────────────────────────────────
            row.spawn((
                Node {
                    flex_direction: FlexDirection::Row,
                    align_items: AlignItems::Center,
                    justify_content: JustifyContent::SpaceBetween,
                    padding: UiRect::all(Val::Px(14.0)),
                    ..default()
                },
                Button,
                CritterExpandButton {
                    critter_id: critter.id,
                },
            ))
            .with_children(|header| {
                header
                    .spawn((Node {
                        flex_direction: FlexDirection::Row,
                        align_items: AlignItems::Center,
                        ..default()
                    },))
                    .with_children(|info| {
                        info.spawn((
                            Text::new(def_name.to_uppercase()),
                            TextFont {
                                font_size: 11.0,
                                ..default()
                            },
                            TextColor(palette.primary),
                            Node {
                                width: Val::Px(60.0),
                                margin: UiRect::right(Val::Px(12.0)),
                                ..default()
                            },
                        ));
                        info.spawn((Node {
                            flex_direction: FlexDirection::Column,
                            ..default()
                        },))
                            .with_children(|col| {
                                col.spawn((
                                    Text::new(&critter.name),
                                    TextFont {
                                        font_size: 14.0,
                                        ..default()
                                    },
                                    TextColor(palette.card_foreground),
                                ));
                                let monitor_name = monitors
                                    .iter()
                                    .find(|(_, fp)| *fp == critter.monitor_fingerprint)
                                    .and_then(|(m, _)| m.name.clone())
                                    .unwrap_or_else(|| "Unassigned".to_string());
                                col.spawn((
                                    Text::new(monitor_name),
                                    TextFont {
                                        font_size: 11.0,
                                        ..default()
                                    },
                                    TextColor(palette.muted_foreground),
                                ));
                            });
                    });

                header
                    .spawn((Node {
                        flex_direction: FlexDirection::Row,
                        align_items: AlignItems::Center,
                        ..default()
                    },))
                    .with_children(|right| {
                        let dot_color = if critter.is_visible {
                            palette.primary
                        } else {
                            palette.muted_foreground
                        };
                        right.spawn((
                            Node {
                                width: Val::Px(10.0),
                                height: Val::Px(10.0),
                                margin: UiRect::right(Val::Px(10.0)),
                                ..default()
                            },
                            BorderRadius::all(Val::Px(5.0)),
                            BackgroundColor(dot_color),
                        ));
                        right.spawn((
                            Text::new(if is_expanded { "v" } else { ">" }),
                            TextFont {
                                font_size: 12.0,
                                ..default()
                            },
                            TextColor(palette.muted_foreground),
                        ));
                    });
            });

            // ── Expanded content ──────────────────────────────────────────
            row.spawn((
                Node {
                    display: expand_display,
                    flex_direction: FlexDirection::Column,
                    padding: UiRect::all(Val::Px(14.0)),
                    border: UiRect::top(Val::Px(1.0)),
                    ..default()
                },
                BorderColor(palette.border),
                BackgroundColor(palette.muted),
                CritterExpandContent {
                    critter_id: critter.id,
                },
            ))
            .with_children(|expanded| {
                expanded.spawn((
                    Text::new(format!("Scale: {}%", (critter.scale * 100.0) as u32)),
                    TextFont {
                        font_size: 12.0,
                        ..default()
                    },
                    TextColor(palette.muted_foreground),
                    Node {
                        margin: UiRect::bottom(Val::Px(4.0)),
                        ..default()
                    },
                ));
                expanded.spawn((
                    Text::new(format!("Opacity: {}%", (critter.opacity * 100.0) as u32)),
                    TextFont {
                        font_size: 12.0,
                        ..default()
                    },
                    TextColor(palette.muted_foreground),
                    Node {
                        margin: UiRect::bottom(Val::Px(12.0)),
                        ..default()
                    },
                ));

                // ── Monitor assignment chips ───────────────────────────────
                expanded.spawn((
                    Text::new("Assign to monitor"),
                    TextFont {
                        font_size: 11.0,
                        ..default()
                    },
                    TextColor(palette.muted_foreground),
                    Node {
                        margin: UiRect::bottom(Val::Px(6.0)),
                        ..default()
                    },
                ));
                expanded
                    .spawn((Node {
                        flex_direction: FlexDirection::Row,
                        flex_wrap: FlexWrap::Wrap,
                        margin: UiRect::bottom(Val::Px(12.0)),
                        ..default()
                    },))
                    .with_children(|chips| {
                        for (monitor, fp) in monitors {
                            let is_assigned = *fp == critter.monitor_fingerprint;
                            let name = monitor
                                .name
                                .clone()
                                .unwrap_or_else(|| format!("Monitor {:x}", fp));
                            chips
                                .spawn((
                                    Node {
                                        padding: UiRect::axes(Val::Px(10.0), Val::Px(6.0)),
                                        border: UiRect::all(Val::Px(1.0)),
                                        margin: UiRect::right(Val::Px(6.0)),
                                        align_items: AlignItems::Center,
                                        justify_content: JustifyContent::Center,
                                        ..default()
                                    },
                                    BorderRadius::all(Val::Px(6.0)),
                                    BorderColor(if is_assigned {
                                        palette.primary
                                    } else {
                                        palette.border
                                    }),
                                    BackgroundColor(if is_assigned {
                                        palette.accent
                                    } else {
                                        palette.card
                                    }),
                                    Button,
                                    MonitorAssignButton {
                                        critter_id: critter.id,
                                        monitor_fingerprint: *fp,
                                    },
                                ))
                                .with_children(|chip| {
                                    chip.spawn((
                                        Text::new(name),
                                        TextFont {
                                            font_size: 11.0,
                                            ..default()
                                        },
                                        TextColor(if is_assigned {
                                            palette.primary
                                        } else {
                                            palette.muted_foreground
                                        }),
                                    ));
                                });
                        }
                    });

                // ── Action row ────────────────────────────────────────────
                expanded
                    .spawn((
                        Node {
                            flex_direction: FlexDirection::Row,
                            align_items: AlignItems::Center,
                            justify_content: JustifyContent::SpaceBetween,
                            border: UiRect::top(Val::Px(1.0)),
                            padding: UiRect::top(Val::Px(10.0)),
                            ..default()
                        },
                        BorderColor(palette.border),
                    ))
                    .with_children(|actions| {
                        let vis_label = if critter.is_visible {
                            "Showing"
                        } else {
                            "Hidden"
                        };
                        spawn_outline_button(
                            actions,
                            vis_label,
                            palette,
                            CritterVisibilityButton {
                                critter_id: critter.id,
                            },
                        );
                        actions
                            .spawn((
                                Node {
                                    padding: UiRect::axes(Val::Px(12.0), Val::Px(8.0)),
                                    border: UiRect::all(Val::Px(1.0)),
                                    align_items: AlignItems::Center,
                                    justify_content: JustifyContent::Center,
                                    ..default()
                                },
                                BorderRadius::all(Val::Px(6.0)),
                                BorderColor(palette.destructive),
                                BackgroundColor(palette.card),
                                Button,
                                CritterDeleteButton {
                                    critter_id: critter.id,
                                },
                            ))
                            .with_children(|btn| {
                                btn.spawn((
                                    Text::new("Delete"),
                                    TextFont {
                                        font_size: 13.0,
                                        ..default()
                                    },
                                    TextColor(palette.destructive),
                                ));
                            });
                    });

                // ── Delete confirmation panel ─────────────────────────────
                let confirm_display = if is_expanded && confirm_pending {
                    Display::Flex
                } else {
                    Display::None
                };
                expanded
                    .spawn((
                        Node {
                            display: confirm_display,
                            flex_direction: FlexDirection::Column,
                            padding: UiRect::all(Val::Px(12.0)),
                            border: UiRect::all(Val::Px(1.0)),
                            margin: UiRect::top(Val::Px(10.0)),
                            ..default()
                        },
                        BorderRadius::all(Val::Px(6.0)),
                        BorderColor(palette.destructive),
                        BackgroundColor(palette.card),
                        DeleteConfirmPanel(critter.id),
                    ))
                    .with_children(|panel| {
                        panel.spawn((
                            Text::new("Say goodbye? This can't be undone."),
                            TextFont {
                                font_size: 13.0,
                                ..default()
                            },
                            TextColor(palette.card_foreground),
                            Node {
                                margin: UiRect::bottom(Val::Px(10.0)),
                                ..default()
                            },
                        ));
                        panel
                            .spawn((Node {
                                flex_direction: FlexDirection::Row,
                                ..default()
                            },))
                            .with_children(|btns| {
                                btns.spawn((
                                    Node {
                                        padding: UiRect::axes(Val::Px(12.0), Val::Px(8.0)),
                                        margin: UiRect::right(Val::Px(8.0)),
                                        align_items: AlignItems::Center,
                                        justify_content: JustifyContent::Center,
                                        ..default()
                                    },
                                    BorderRadius::all(Val::Px(6.0)),
                                    BackgroundColor(palette.destructive),
                                    Button,
                                    DeleteConfirmButton {
                                        critter_id: critter.id,
                                    },
                                ))
                                .with_children(|btn| {
                                    btn.spawn((
                                        Text::new("Confirm"),
                                        TextFont {
                                            font_size: 13.0,
                                            ..default()
                                        },
                                        TextColor(palette.destructive_foreground),
                                    ));
                                });
                                spawn_outline_button(btns, "Cancel", palette, DeleteCancelButton);
                            });
                    });
            });
        });
}

// ─── Available critters gallery ───────────────────────────────────────────────

fn spawn_available_section(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    prefs: &Preferences,
    registry: &CritterRegistry,
    palette: &Palette,
) {
    spawn_section_header(parent, "Available Companions", palette);

    let available: Vec<_> = registry
        .defs
        .iter()
        .filter(|d| !prefs.critters.iter().any(|c| c.def_id == d.id))
        .collect();

    if available.is_empty() {
        parent.spawn((
            Text::new("You've adopted all available companions!"),
            TextFont {
                font_size: 14.0,
                ..default()
            },
            TextColor(palette.muted_foreground),
        ));
        return;
    }

    parent
        .spawn((Node {
            flex_direction: FlexDirection::Row,
            flex_wrap: FlexWrap::Wrap,
            ..default()
        },))
        .with_children(|grid| {
            for def in available {
                grid.spawn((
                    Node {
                        flex_direction: FlexDirection::Column,
                        align_items: AlignItems::Center,
                        padding: UiRect::all(Val::Px(16.0)),
                        border: UiRect::all(Val::Px(1.0)),
                        margin: UiRect {
                            right: Val::Px(12.0),
                            bottom: Val::Px(12.0),
                            ..default()
                        },
                        width: Val::Px(130.0),
                        ..default()
                    },
                    BorderRadius::all(Val::Px(8.0)),
                    BorderColor(palette.border),
                    BackgroundColor(palette.card),
                ))
                .with_children(|card| {
                    card.spawn((
                        Text::new(&def.name),
                        TextFont {
                            font_size: 15.0,
                            ..default()
                        },
                        TextColor(palette.card_foreground),
                        Node {
                            margin: UiRect::bottom(Val::Px(12.0)),
                            ..default()
                        },
                    ));
                    card.spawn((
                        Node {
                            padding: UiRect::axes(Val::Px(12.0), Val::Px(8.0)),
                            align_items: AlignItems::Center,
                            justify_content: JustifyContent::Center,
                            width: Val::Percent(100.0),
                            ..default()
                        },
                        BorderRadius::all(Val::Px(6.0)),
                        BackgroundColor(palette.primary),
                        Button,
                        AdoptButton {
                            def_id: def.id.clone(),
                        },
                    ))
                    .with_children(|btn| {
                        btn.spawn((
                            Text::new("Adopt"),
                            TextFont {
                                font_size: 13.0,
                                ..default()
                            },
                            TextColor(palette.primary_foreground),
                        ));
                    });
                });
            }
        });
}
