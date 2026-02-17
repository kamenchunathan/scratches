use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*};

use crate::ui::{
    palette::Palette,
    state::{CritterRoster, MonitorList, Tab, UiState},
    widgets::*,
};

/// Spawns the My Critters tab content.
/// Expandable rows and delete confirmations are spawned in full; their visibility
/// is toggled by `update_critter_expand_visibility` and `update_delete_confirm_visibility`
/// in systems.rs based on `UiState.expanded_critter_idx` / `delete_confirm_idx`.
///
/// NOTE: Adopting or deleting critters changes `CritterRoster`, which requires
/// rebuilding this tab. Mark this tab with a dirty flag and respawn when the
/// roster changes — that system is left as a future integration point.
pub fn spawn_critters_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    ui_state: &UiState,
    roster: &CritterRoster,
    monitors: &MonitorList,
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
        ))
        .with_children(|tab| {
            spawn_owned_critters_section(tab, ui_state, roster, monitors, palette);
            spawn_separator(tab, palette);
            spawn_available_critters_section(tab, roster, palette);
        });
}

// ─── Owned critters section ───────────────────────────────────────────────────

fn spawn_owned_critters_section(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    ui_state: &UiState,
    roster: &CritterRoster,
    monitors: &MonitorList,
    palette: &Palette,
) {
    let label = format!("Your Companions ({})", roster.owned.len());
    spawn_section_header(parent, &label, palette);

    if roster.owned.is_empty() {
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

    for (idx, critter) in roster.owned.iter().enumerate() {
        spawn_critter_row(
            parent,
            idx,
            critter.template_id,
            &critter.name,
            critter.is_visible,
            critter.scale,
            critter.opacity,
            critter.assigned_monitor_idx,
            ui_state,
            monitors,
            palette,
        );
    }
}

#[allow(clippy::too_many_arguments)]
fn spawn_critter_row(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    idx: usize,
    template_id: &'static str,
    name: &str,
    is_visible: bool,
    scale: u32,
    opacity: u32,
    monitor_idx: usize,
    ui_state: &UiState,
    monitors: &MonitorList,
    palette: &Palette,
) {
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
            // ── Collapsed header (always visible, clickable to expand) ─────
            row.spawn((
                Node {
                    flex_direction: FlexDirection::Row,
                    align_items: AlignItems::Center,
                    justify_content: JustifyContent::SpaceBetween,
                    padding: UiRect::all(Val::Px(14.0)),
                    ..default()
                },
                Button,
                CritterHeaderButton(idx),
            ))
            .with_children(|header| {
                // Species + name + monitor assignment
                header
                    .spawn((Node {
                        flex_direction: FlexDirection::Row,
                        align_items: AlignItems::Center,
                        ..default()
                    },))
                    .with_children(|info| {
                        info.spawn((
                            Text::new(template_id.to_uppercase()),
                            TextFont {
                                font_size: 11.0,
                                ..default()
                            },
                            TextColor(palette.primary),
                            Node {
                                width: Val::Px(36.0),
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
                                    Text::new(name),
                                    TextFont {
                                        font_size: 14.0,
                                        ..default()
                                    },
                                    TextColor(palette.card_foreground),
                                ));
                                let monitor_name = monitors
                                    .monitors
                                    .get(monitor_idx)
                                    .map(|m| m.name.as_str())
                                    .unwrap_or("Unassigned");
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

                // Visibility dot + chevron
                header
                    .spawn((Node {
                        flex_direction: FlexDirection::Row,
                        align_items: AlignItems::Center,
                        ..default()
                    },))
                    .with_children(|right| {
                        let dot_color = if is_visible {
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
                        let chevron = if ui_state.expanded_critter_idx == Some(idx) {
                            "v"
                        } else {
                            ">"
                        };
                        right.spawn((
                            Text::new(chevron),
                            TextFont {
                                font_size: 12.0,
                                ..default()
                            },
                            TextColor(palette.muted_foreground),
                        ));
                    });
            });

            // ── Expanded content (hidden by default, toggled by system) ────
            let expand_display = if ui_state.expanded_critter_idx == Some(idx) {
                Display::Flex
            } else {
                Display::None
            };

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
                CritterExpandContent(idx),
            ))
            .with_children(|expanded| {
                // Scale selector (shown as text for now; a Select widget could replace)
                expanded.spawn((
                    Text::new(format!("Scale: {scale}%")),
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
                // Opacity selector
                expanded.spawn((
                    Text::new(format!("Opacity: {opacity}%")),
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

                // Action row: visibility toggle + delete button
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
                        let vis_label = if is_visible { "Showing" } else { "Hidden" };
                        spawn_outline_button(
                            actions,
                            vis_label,
                            palette,
                            CritterVisibilityButton(idx),
                        );

                        // Delete button
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
                                CritterDeleteButton(idx),
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

                // ── Delete confirmation panel (hidden until delete is pressed) ──
                let confirm_display = if ui_state.delete_confirm_idx == Some(idx) {
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
                        DeleteConfirmPanel(idx),
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
                                    DeleteConfirmButton(idx),
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
                                spawn_outline_button(
                                    btns,
                                    "Cancel",
                                    palette,
                                    DeleteCancelButton(idx),
                                );
                            });
                    });
            });
        });
}

// ─── Available critters gallery ───────────────────────────────────────────────

fn spawn_available_critters_section(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    roster: &CritterRoster,
    palette: &Palette,
) {
    spawn_section_header(parent, "Available Companions", palette);

    let available = roster.available_templates();
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
            for template in available {
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
                        Text::new(template.species.to_uppercase()),
                        TextFont {
                            font_size: 10.0,
                            ..default()
                        },
                        TextColor(palette.muted_foreground),
                        Node {
                            margin: UiRect::bottom(Val::Px(4.0)),
                            ..default()
                        },
                    ));
                    card.spawn((
                        Text::new(template.name),
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
                        CritterAdoptButton(template.id),
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
