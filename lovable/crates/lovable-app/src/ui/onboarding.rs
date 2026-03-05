use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*, window::Monitor};

use crate::{
    critter::CritterRegistry,
    ui::{
        palette::Palette,
        state::{AppScreen, OnboardingState},
        widgets::*,
    },
};

pub fn spawn_onboarding(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    screen: &AppScreen,
    registry: &CritterRegistry,
    monitors: &[(&Monitor, u64)],
    palette: &Palette,
) {
    spawn_welcome_screen(parent, palette);
    spawn_pick_critter_screen(parent, screen, registry, palette);
    spawn_choose_monitor_screen(parent, screen, monitors, palette);
    spawn_info_screen(parent, palette);
}

fn spawn_welcome_screen(parent: &mut RelatedSpawnerCommands<'_, ChildOf>, palette: &Palette) {
    parent
        .spawn((
            Node {
                display: Display::Flex,
                flex_direction: FlexDirection::Column,
                width: Val::Percent(100.0),
                height: Val::Percent(100.0),
                align_items: AlignItems::Center,
                justify_content: JustifyContent::Center,
                padding: UiRect::all(Val::Px(40.0)),
                ..default()
            },
            OnboardingScreen(0),
        ))
        .with_children(|screen| {
            screen.spawn((
                Text::new("Someone's been waiting to meet you."),
                TextFont {
                    font_size: 32.0,
                    ..default()
                },
                TextColor(palette.foreground),
                Node {
                    margin: UiRect::bottom(Val::Px(16.0)),
                    ..default()
                },
            ));
            screen.spawn((
                Text::new(
                    "Lovable puts a tiny companion on your desktop. Let's get yours settled in.",
                ),
                TextFont {
                    font_size: 16.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
                Node {
                    margin: UiRect::bottom(Val::Px(32.0)),
                    ..default()
                },
            ));
            spawn_primary_button(screen, "Meet them", palette, OnboardingPrimaryButton);
        });
}

fn spawn_pick_critter_screen(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    screen: &AppScreen,
    registry: &CritterRegistry,
    palette: &Palette,
) {
    let selected_def_id = match screen {
        AppScreen::Onboarding(OnboardingState::PickCritter { selected_def_id }) => {
            selected_def_id.as_str()
        }
        _ => "",
    };

    parent
        .spawn((
            Node {
                display: Display::None,
                flex_direction: FlexDirection::Column,
                width: Val::Percent(100.0),
                padding: UiRect::all(Val::Px(40.0)),
                align_items: AlignItems::Center,
                ..default()
            },
            OnboardingScreen(1),
        ))
        .with_children(|screen_node| {
            screen_node.spawn((
                Text::new("Pick your first companion"),
                TextFont {
                    font_size: 28.0,
                    ..default()
                },
                TextColor(palette.foreground),
                Node {
                    margin: UiRect::bottom(Val::Px(8.0)),
                    ..default()
                },
            ));
            screen_node.spawn((
                Text::new("You can adopt more later — this is just your first one."),
                TextFont {
                    font_size: 14.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
                Node {
                    margin: UiRect::bottom(Val::Px(24.0)),
                    ..default()
                },
            ));

            screen_node
                .spawn((Node {
                    flex_direction: FlexDirection::Row,
                    flex_wrap: FlexWrap::Wrap,
                    justify_content: JustifyContent::Center,
                    width: Val::Percent(100.0),
                    margin: UiRect::bottom(Val::Px(24.0)),
                    ..default()
                },))
                .with_children(|grid| {
                    for def in &registry.defs {
                        let is_selected = def.id == selected_def_id;
                        grid.spawn((
                            Node {
                                flex_direction: FlexDirection::Column,
                                align_items: AlignItems::Center,
                                padding: UiRect::all(Val::Px(16.0)),
                                margin: UiRect::all(Val::Px(8.0)),
                                border: UiRect::all(Val::Px(2.0)),
                                width: Val::Px(120.0),
                                ..default()
                            },
                            BorderRadius::all(Val::Px(8.0)),
                            BorderColor(if is_selected {
                                palette.primary
                            } else {
                                palette.border
                            }),
                            BackgroundColor(if is_selected {
                                palette.accent
                            } else {
                                palette.card
                            }),
                            Button,
                            OnboardingCritterCard {
                                def_id: def.id.clone(),
                            },
                        ))
                        .with_children(|card| {
                            card.spawn((
                                Text::new(&def.name),
                                TextFont {
                                    font_size: 14.0,
                                    ..default()
                                },
                                TextColor(palette.card_foreground),
                            ));
                        });
                    }
                });

            screen_node
                .spawn((Node {
                    flex_direction: FlexDirection::Row,
                    ..default()
                },))
                .with_children(|actions| {
                    let label = if selected_def_id.is_empty() {
                        "Adopt".to_string()
                    } else {
                        registry
                            .find(selected_def_id)
                            .map(|d| format!("Adopt {}", d.name))
                            .unwrap_or_else(|| "Adopt".to_string())
                    };
                    spawn_primary_button(actions, &label, palette, OnboardingPrimaryButton);
                    spawn_outline_button(actions, "Skip setup", palette, OnboardingSkipButton);
                });
        });
}

fn spawn_choose_monitor_screen(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    screen: &AppScreen,
    monitors: &[(&Monitor, u64)],
    palette: &Palette,
) {
    let selected_fp = match screen {
        AppScreen::Onboarding(OnboardingState::ChooseMonitor {
            selected_monitor, ..
        }) => *selected_monitor,
        _ => None,
    };

    parent
        .spawn((
            Node {
                display: Display::None,
                flex_direction: FlexDirection::Column,
                width: Val::Percent(100.0),
                padding: UiRect::all(Val::Px(40.0)),
                align_items: AlignItems::Center,
                ..default()
            },
            OnboardingScreen(2),
        ))
        .with_children(|screen_node| {
            screen_node.spawn((
                Text::new("Choose a home"),
                TextFont {
                    font_size: 28.0,
                    ..default()
                },
                TextColor(palette.foreground),
                Node {
                    margin: UiRect::bottom(Val::Px(8.0)),
                    ..default()
                },
            ));
            screen_node.spawn((
                Text::new("You can always move them later."),
                TextFont {
                    font_size: 14.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
                Node {
                    margin: UiRect::bottom(Val::Px(24.0)),
                    ..default()
                },
            ));

            for (monitor, fp) in monitors {
                let is_selected = selected_fp == Some(*fp);
                let name = monitor
                    .name
                    .clone()
                    .unwrap_or_else(|| format!("Monitor {:x}", fp));
                let resolution = format!("{}x{}", monitor.physical_width, monitor.physical_height);
                let refresh_hz = monitor
                    .refresh_rate_millihertz
                    .map(|r| r / 1000)
                    .unwrap_or(60);

                screen_node
                    .spawn((
                        Node {
                            flex_direction: FlexDirection::Column,
                            padding: UiRect::all(Val::Px(16.0)),
                            border: UiRect::all(Val::Px(2.0)),
                            width: Val::Percent(100.0),
                            margin: UiRect::bottom(Val::Px(12.0)),
                            ..default()
                        },
                        BorderColor(if is_selected {
                            palette.primary
                        } else {
                            palette.border
                        }),
                        BackgroundColor(if is_selected {
                            palette.accent
                        } else {
                            palette.card
                        }),
                        BorderRadius::all(Val::Px(8.0)),
                        Button,
                        OnboardingMonitorCard {
                            monitor_fingerprint: *fp,
                        },
                    ))
                    .with_children(|card| {
                        card.spawn((
                            Text::new(name),
                            TextFont {
                                font_size: 16.0,
                                ..default()
                            },
                            TextColor(palette.card_foreground),
                        ));
                        card.spawn((
                            Text::new(format!("{resolution} · {refresh_hz}Hz")),
                            TextFont {
                                font_size: 12.0,
                                ..default()
                            },
                            TextColor(palette.muted_foreground),
                            Node {
                                margin: UiRect::vertical(Val::Px(8.0)),
                                ..default()
                            },
                        ));
                    });
            }

            screen_node
                .spawn((Node {
                    flex_direction: FlexDirection::Row,
                    ..default()
                },))
                .with_children(|actions| {
                    spawn_primary_button(actions, "Looks good", palette, OnboardingPrimaryButton);
                    spawn_outline_button(actions, "Skip setup", palette, OnboardingSkipButton);
                });
        });
}

fn spawn_info_screen(parent: &mut RelatedSpawnerCommands<'_, ChildOf>, palette: &Palette) {
    parent
        .spawn((
            Node {
                display: Display::None,
                flex_direction: FlexDirection::Column,
                width: Val::Percent(100.0),
                padding: UiRect::all(Val::Px(40.0)),
                align_items: AlignItems::Center,
                ..default()
            },
            OnboardingScreen(3),
        ))
        .with_children(|screen| {
            screen.spawn((
                Text::new("A couple of things worth knowing"),
                TextFont {
                    font_size: 28.0,
                    ..default()
                },
                TextColor(palette.foreground),
                Node {
                    margin: UiRect::bottom(Val::Px(24.0)),
                    ..default()
                },
            ));

            screen
                .spawn((
                    Node {
                        flex_direction: FlexDirection::Column,
                        padding: UiRect::all(Val::Px(20.0)),
                        border: UiRect::all(Val::Px(1.0)),
                        width: Val::Percent(100.0),
                        margin: UiRect::bottom(Val::Px(24.0)),
                        ..default()
                    },
                    BorderColor(palette.border),
                    BackgroundColor(palette.card),
                    BorderRadius::all(Val::Px(8.0)),
                ))
                .with_children(|card| {
                    spawn_info_row(
                        card,
                        "On your desktop",
                        "They'll live on your desktop — keep working normally around them.",
                        palette,
                    );
                    spawn_info_row(
                        card,
                        "System tray",
                        "Right-click the tray icon any time to show or hide this window.",
                        palette,
                    );
                    spawn_info_row(
                        card,
                        "Customisation",
                        "Tweak their size, opacity, and where they wander in Settings.",
                        palette,
                    );
                });

            spawn_primary_button(screen, "Take me home", palette, OnboardingPrimaryButton);
        });
}

fn spawn_info_row(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    title: &str,
    body: &str,
    palette: &Palette,
) {
    parent
        .spawn((Node {
            flex_direction: FlexDirection::Row,
            align_items: AlignItems::FlexStart,
            margin: UiRect::bottom(Val::Px(16.0)),
            ..default()
        },))
        .with_children(|row| {
            row.spawn((
                Node {
                    width: Val::Px(8.0),
                    height: Val::Px(8.0),
                    margin: UiRect {
                        top: Val::Px(5.0),
                        right: Val::Px(12.0),
                        ..default()
                    },
                    ..default()
                },
                BorderRadius::all(Val::Px(4.0)),
                BackgroundColor(palette.primary),
            ));
            row.spawn((Node {
                flex_direction: FlexDirection::Column,
                ..default()
            },))
                .with_children(|col| {
                    col.spawn((
                        Text::new(title),
                        TextFont {
                            font_size: 14.0,
                            ..default()
                        },
                        TextColor(palette.card_foreground),
                    ));
                    col.spawn((
                        Text::new(body),
                        TextFont {
                            font_size: 13.0,
                            ..default()
                        },
                        TextColor(palette.muted_foreground),
                    ));
                });
        });
}
