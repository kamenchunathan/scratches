use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*, window::Monitor};

use crate::{
    critter::CritterRegistry,
    preferences::Preferences,
    ui::{palette::Palette, state::Tab, widgets::*},
};

pub fn spawn_home_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    prefs: &Preferences,
    registry: &CritterRegistry,
    monitors: &[(&Monitor, u64)],
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
            spawn_greeting_card(tab, prefs, palette);
            spawn_separator(tab, palette);
            spawn_companions_strip(tab, prefs, registry, palette);
            spawn_separator(tab, palette);
            spawn_monitor_layout(tab, prefs, registry, monitors, palette);
            spawn_separator(tab, palette);
            spawn_quick_actions(tab, palette);
            spawn_discovery_nudge(tab, prefs, registry, palette);
        });
}

fn spawn_greeting_card(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    prefs: &Preferences,
    palette: &Palette,
) {
    let visible: Vec<_> = prefs.critters.iter().filter(|c| c.is_visible).collect();
    let status = match visible.len() {
        0 => "All your companions are hiding right now.".to_string(),
        1 => format!("{} is settled in and ready for the day.", visible[0].name),
        2 => format!(
            "{} and {} are both out and about.",
            visible[0].name, visible[1].name
        ),
        n => format!("Your {n} companions are out and about."),
    };

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

fn spawn_companions_strip(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    prefs: &Preferences,
    registry: &CritterRegistry,
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
            for critter in &prefs.critters {
                let def_name = registry
                    .find(&critter.def_id)
                    .map(|d| d.name.as_str())
                    .unwrap_or("Unknown");
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
                        card.spawn((
                            Text::new(def_name),
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

fn spawn_monitor_layout(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    prefs: &Preferences,
    registry: &CritterRegistry,
    monitors: &[(&Monitor, u64)],
    palette: &Palette,
) {
    parent.spawn((
        Text::new("Monitor Layout"),
        TextFont {
            font_size: 13.0,
            ..default()
        },
        TextColor(palette.foreground),
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
        .with_children(|grid| {
            for (monitor, fp) in monitors {
                let settings = prefs.monitors.iter().find(|m| m.fingerprint == *fp);
                if settings.map(|s| !s.enabled).unwrap_or(false) {
                    continue;
                }

                let name = monitor
                    .name
                    .clone()
                    .unwrap_or_else(|| format!("Monitor {:x}", fp));
                let width = monitor.physical_width;
                let height = monitor.physical_height;
                let resolution = format!("{width}x{height}");

                let occupants: Vec<_> = prefs
                    .critters
                    .iter()
                    .filter(|c| c.monitor_fingerprint == *fp)
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
                        Text::new(name),
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
                            Text::new(resolution),
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

fn spawn_discovery_nudge(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    prefs: &Preferences,
    registry: &CritterRegistry,
    palette: &Palette,
) {
    let available: Vec<_> = registry
        .defs
        .iter()
        .filter(|d| !prefs.critters.iter().any(|c| c.def_id == d.id))
        .collect();

    if available.is_empty() || prefs.critters.len() >= 5 {
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
            for def in available.iter().take(2) {
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
                        Text::new(&def.name),
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
                    card.spawn((
                        Node {
                            padding: UiRect::axes(Val::Px(16.0), Val::Px(8.0)),
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
