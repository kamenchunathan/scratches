use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*};

use crate::ui::{
    palette::Palette,
    state::{CRITTER_TEMPLATES, CritterRoster, MonitorList, Tab},
    widgets::*,
};

/// Spawns the Home tab content. Visibility is driven by the tab system.
pub fn spawn_home_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    roster: &CritterRoster,
    monitors: &MonitorList,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                display: Display::Flex,
                flex_direction: FlexDirection::Column,
                width: Val::Percent(100.0),
                ..default()
            },
            TabContent(Tab::Home),
        ))
        .with_children(|tab| {
            spawn_greeting_card(tab, roster, palette);
            spawn_separator(tab, palette);
            spawn_companions_strip(tab, roster, palette);
            spawn_separator(tab, palette);
            spawn_monitor_layout(tab, roster, monitors, palette);
            spawn_separator(tab, palette);
            spawn_quick_actions(tab, palette);
            spawn_discovery_nudge(tab, roster, palette);
        });
}

// ─── Greeting card ────────────────────────────────────────────────────────────

fn spawn_greeting_card(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    roster: &CritterRoster,
    palette: &Palette,
) {
    let status = build_status_message(roster);
    // Greeting and status are computed at spawn time from current roster state.
    // based on time-of-day if desired.
    spawn_card(parent, palette, UiRect::bottom(Val::Px(0.0)), |card| {
        card.spawn((
            Text::new("Good day!"),
            TextFont {
                font_size: 22.0,
                ..default()
            },
            TextColor(palette.card_foreground),
        ));
        card.spawn((
            Text::new(status),
            TextFont {
                font_size: 14.0,
                ..default()
            },
            TextColor(palette.muted_foreground),
            Node {
                margin: UiRect::top(Val::Px(6.0)),
                ..default()
            },
        ));
    });
}

fn build_status_message(roster: &CritterRoster) -> String {
    let visible: Vec<_> = roster.owned.iter().filter(|c| c.is_visible).collect();
    match visible.len() {
        0 => "All your companions are hiding right now.".to_string(),
        1 => format!("{} is settled in and ready for the day.", visible[0].name),
        2 => format!(
            "{} and {} are both out and about.",
            visible[0].name, visible[1].name
        ),
        n => format!("Your {n} companions are out and about."),
    }
}

// ─── Companions strip ─────────────────────────────────────────────────────────

fn spawn_companions_strip(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    roster: &CritterRoster,
    palette: &Palette,
) {
    spawn_section_header(parent, "Your Companions", palette);
    parent.spawn((
        Text::new("Tap a companion card to view details in the My Critters tab."),
        TextFont {
            font_size: 11.0,
            ..default()
        },
        TextColor(palette.muted_foreground),
        Node {
            margin: UiRect::bottom(Val::Px(10.0)),
            ..default()
        },
    ));

    parent
        .spawn((Node {
            flex_direction: FlexDirection::Row,
            flex_wrap: FlexWrap::Wrap,
            ..default()
        },))
        .with_children(|strip| {
            for critter in &roster.owned {
                let visibility_color = if critter.is_visible {
                    palette.primary
                } else {
                    palette.muted_foreground
                };
                strip
                    .spawn((
                        Node {
                            flex_direction: FlexDirection::Column,
                            align_items: AlignItems::Center,
                            padding: UiRect::all(Val::Px(12.0)),
                            border: UiRect::all(Val::Px(1.0)),
                            margin: UiRect::right(Val::Px(8.0)),
                            width: Val::Px(90.0),
                            ..default()
                        },
                        BorderRadius::all(Val::Px(8.0)),
                        BorderColor(palette.border),
                        BackgroundColor(palette.card),
                    ))
                    .with_children(|card| {
                        // Species label acts as the visual identifier (emoji requires custom font)
                        card.spawn((
                            Text::new(critter.template_id.to_uppercase()),
                            TextFont {
                                font_size: 11.0,
                                ..default()
                            },
                            TextColor(palette.primary),
                            Node {
                                margin: UiRect::bottom(Val::Px(6.0)),
                                ..default()
                            },
                        ));
                        card.spawn((
                            Text::new(&critter.name),
                            TextFont {
                                font_size: 11.0,
                                ..default()
                            },
                            TextColor(palette.card_foreground),
                        ));
                        // Visibility indicator dot
                        card.spawn((
                            Node {
                                width: Val::Px(8.0),
                                height: Val::Px(8.0),
                                margin: UiRect::top(Val::Px(6.0)),
                                ..default()
                            },
                            BorderRadius::all(Val::Px(4.0)),
                            BackgroundColor(visibility_color),
                        ));
                    });
            }
        });
}

// ─── Monitor layout preview ───────────────────────────────────────────────────

fn spawn_monitor_layout(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    roster: &CritterRoster,
    monitors: &MonitorList,
    palette: &Palette,
) {
    parent
        .spawn((Node {
            flex_direction: FlexDirection::Row,
            align_items: AlignItems::Center,
            justify_content: JustifyContent::SpaceBetween,
            margin: UiRect::bottom(Val::Px(10.0)),
            ..default()
        },))
        .with_children(|header| {
            header.spawn((
                Text::new("Monitor Layout"),
                TextFont {
                    font_size: 13.0,
                    ..default()
                },
                TextColor(palette.foreground),
            ));
        });

    parent
        .spawn((Node {
            flex_direction: FlexDirection::Row,
            flex_wrap: FlexWrap::Wrap,
            ..default()
        },))
        .with_children(|grid| {
            for (monitor_idx, monitor) in monitors.monitors.iter().enumerate() {
                if !monitor.enabled {
                    continue;
                }
                let occupants: Vec<_> = roster
                    .owned
                    .iter()
                    .filter(|c| c.assigned_monitor_idx == monitor_idx)
                    .collect();

                grid.spawn((
                    Node {
                        flex_direction: FlexDirection::Column,
                        padding: UiRect::all(Val::Px(12.0)),
                        border: UiRect::all(Val::Px(1.0)),
                        margin: UiRect {
                            right: Val::Px(12.0),
                            bottom: Val::Px(12.0),
                            ..default()
                        },
                        width: Val::Px(160.0),
                        ..default()
                    },
                    BorderRadius::all(Val::Px(8.0)),
                    BorderColor(palette.border),
                    BackgroundColor(palette.muted),
                ))
                .with_children(|card| {
                    card.spawn((
                        Text::new(&monitor.name),
                        TextFont {
                            font_size: 12.0,
                            ..default()
                        },
                        TextColor(palette.foreground),
                        Node {
                            margin: UiRect::bottom(Val::Px(8.0)),
                            ..default()
                        },
                    ));
                    // Inner canvas
                    card.spawn((
                        Node {
                            width: Val::Percent(100.0),
                            height: Val::Px(70.0),
                            align_items: AlignItems::Center,
                            justify_content: JustifyContent::Center,
                            border: UiRect::all(Val::Px(1.0)),
                            flex_wrap: FlexWrap::Wrap,
                            ..default()
                        },
                        BorderRadius::all(Val::Px(4.0)),
                        BorderColor(palette.border),
                        BackgroundColor(palette.background),
                    ))
                    .with_children(|canvas| {
                        canvas.spawn((
                            Text::new(&monitor.resolution),
                            TextFont {
                                font_size: 9.0,
                                ..default()
                            },
                            TextColor(palette.muted_foreground),
                        ));
                        if occupants.is_empty() {
                            canvas.spawn((
                                Text::new("No companions"),
                                TextFont {
                                    font_size: 10.0,
                                    ..default()
                                },
                                TextColor(palette.muted_foreground),
                            ));
                        } else {
                            for critter in occupants {
                                canvas.spawn((
                                    Text::new(&critter.name),
                                    TextFont {
                                        font_size: 10.0,
                                        ..default()
                                    },
                                    TextColor(palette.primary),
                                    Node {
                                        margin: UiRect::all(Val::Px(2.0)),
                                        ..default()
                                    },
                                ));
                            }
                        }
                    });
                });
            }
        });
}

// ─── Quick actions ────────────────────────────────────────────────────────────

fn spawn_quick_actions(parent: &mut RelatedSpawnerCommands<'_, ChildOf>, palette: &Palette) {
    parent
        .spawn((Node {
            flex_direction: FlexDirection::Row,
            flex_wrap: FlexWrap::Wrap,
            ..default()
        },))
        .with_children(|row| {
            spawn_primary_button(row, "Hide all", palette, HideAllButton);
            spawn_outline_button(row, "Show all", palette, ShowAllButton);
        });
}

// ─── Discovery nudge ─────────────────────────────────────────────────────────

/// Shows a preview of up to 2 adoptable critters. Hidden when none are available.
fn spawn_discovery_nudge(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    roster: &CritterRoster,
    palette: &Palette,
) {
    let available = roster.available_templates();
    if available.is_empty() || roster.owned.len() >= 5 {
        return;
    }

    spawn_separator(parent, palette);
    spawn_section_header(
        parent,
        "A couple of critters who'd love to meet you",
        palette,
    );

    parent
        .spawn((Node {
            flex_direction: FlexDirection::Row,
            flex_wrap: FlexWrap::Wrap,
            ..default()
        },))
        .with_children(|grid| {
            for template in available.iter().take(2) {
                grid.spawn((
                    Node {
                        flex_direction: FlexDirection::Column,
                        align_items: AlignItems::Center,
                        padding: UiRect::all(Val::Px(16.0)),
                        border: UiRect::all(Val::Px(1.0)),
                        margin: UiRect::right(Val::Px(12.0)),
                        width: Val::Px(140.0),
                        ..default()
                    },
                    BorderRadius::all(Val::Px(8.0)),
                    BorderColor(palette.border),
                    BackgroundColor(palette.card),
                ))
                .with_children(|card| {
                    card.spawn((
                        Text::new(template.species),
                        TextFont {
                            font_size: 13.0,
                            ..default()
                        },
                        TextColor(palette.primary),
                        Node {
                            margin: UiRect::bottom(Val::Px(4.0)),
                            ..default()
                        },
                    ));
                    card.spawn((
                        Text::new(template.name),
                        TextFont {
                            font_size: 14.0,
                            ..default()
                        },
                        TextColor(palette.card_foreground),
                        Node {
                            margin: UiRect::bottom(Val::Px(12.0)),
                            ..default()
                        },
                    ));
                    // Adopt button
                    card.spawn((
                        Node {
                            padding: UiRect::axes(Val::Px(16.0), Val::Px(8.0)),
                            border: UiRect::all(Val::Px(0.0)),
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
