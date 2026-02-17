use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*};

use crate::ui::{
    palette::Palette,
    state::{CRITTER_TEMPLATES, CritterRoster, MonitorList, UiState},
    widgets::*,
};

/// Spawns all four onboarding screens as siblings. Only the active screen is visible;
/// visibility is driven by `update_onboarding_step_visibility` in systems.rs.
pub fn spawn_onboarding(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    ui_state: &UiState,
    roster: &CritterRoster,
    monitors: &MonitorList,
    palette: &Palette,
) {
    spawn_welcome_screen(parent, palette);
    spawn_pick_critter_screen(parent, ui_state, palette);
    spawn_choose_monitor_screen(parent, roster, monitors, palette);
    spawn_info_screen(parent, palette);
}

// ─── Step 0: Welcome ─────────────────────────────────────────────────────────

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

// ─── Step 1: Pick your first companion ───────────────────────────────────────

fn spawn_pick_critter_screen(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    ui_state: &UiState,
    palette: &Palette,
) {
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
        .with_children(|screen| {
            screen.spawn((
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
            screen.spawn((
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

            // Critter selection grid
            screen
                .spawn((Node {
                    flex_direction: FlexDirection::Row,
                    flex_wrap: FlexWrap::Wrap,
                    justify_content: JustifyContent::Center,
                    width: Val::Percent(100.0),
                    margin: UiRect::bottom(Val::Px(24.0)),
                    ..default()
                },))
                .with_children(|grid| {
                    for (i, template) in CRITTER_TEMPLATES.iter().enumerate() {
                        let is_selected = i == ui_state.selected_onboarding_critter;
                        let border_col = if is_selected {
                            palette.primary
                        } else {
                            palette.border
                        };
                        let bg_col = if is_selected {
                            palette.accent
                        } else {
                            palette.card
                        };

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
                            BorderColor(border_col),
                            BackgroundColor(bg_col),
                            Button,
                            OnboardingCritterCard(i),
                        ))
                        .with_children(|card| {
                            card.spawn((
                                Text::new(template.species),
                                TextFont {
                                    font_size: 28.0,
                                    ..default()
                                },
                                TextColor(palette.primary),
                                Node {
                                    margin: UiRect::bottom(Val::Px(8.0)),
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
                            ));
                            card.spawn((
                                Text::new(template.species),
                                TextFont {
                                    font_size: 11.0,
                                    ..default()
                                },
                                TextColor(palette.muted_foreground),
                            ));
                        });
                    }
                });

            screen
                .spawn((Node {
                    flex_direction: FlexDirection::Row,
                    ..default()
                },))
                .with_children(|actions| {
                    let selected_name =
                        CRITTER_TEMPLATES[ui_state.selected_onboarding_critter].name;
                    let adopt_label = format!("Adopt {selected_name}");
                    spawn_primary_button(actions, &adopt_label, palette, OnboardingPrimaryButton);
                    spawn_outline_button(actions, "Skip setup", palette, OnboardingSkipButton);
                });
        });
}

// ─── Step 2: Choose a home (monitor assignment) ───────────────────────────────

fn spawn_choose_monitor_screen(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    _roster: &CritterRoster,
    monitors: &MonitorList,
    palette: &Palette,
) {
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
        .with_children(|screen| {
            screen.spawn((
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
            screen.spawn((
                Text::new("Drag them anywhere you like — you can always move them later."),
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

            for monitor in &monitors.monitors {
                if !monitor.enabled {
                    continue;
                }
                screen
                    .spawn((
                        Node {
                            flex_direction: FlexDirection::Column,
                            padding: UiRect::all(Val::Px(16.0)),
                            border: UiRect::all(Val::Px(1.0)),
                            width: Val::Percent(100.0),
                            margin: UiRect::bottom(Val::Px(12.0)),
                            ..default()
                        },
                        BorderColor(palette.border),
                        BackgroundColor(palette.card),
                        BorderRadius::all(Val::Px(8.0)),
                    ))
                    .with_children(|card| {
                        card.spawn((
                            Text::new(&monitor.name),
                            TextFont {
                                font_size: 16.0,
                                ..default()
                            },
                            TextColor(palette.card_foreground),
                        ));
                        let sub = format!("{} · {}Hz", monitor.resolution, monitor.refresh_rate);
                        card.spawn((
                            Text::new(sub),
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
                        // Preview area representing the monitor canvas
                        card.spawn((
                            Node {
                                width: Val::Percent(100.0),
                                height: Val::Px(80.0),
                                align_items: AlignItems::Center,
                                justify_content: JustifyContent::Center,
                                ..default()
                            },
                            BackgroundColor(palette.muted),
                            BorderRadius::all(Val::Px(6.0)),
                        ))
                        .with_children(|preview| {
                            preview.spawn((
                                Text::new("[ desktop preview ]"),
                                TextFont {
                                    font_size: 12.0,
                                    ..default()
                                },
                                TextColor(palette.muted_foreground),
                            ));
                        });
                    });
            }

            screen
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

// ─── Step 3: Information screen ───────────────────────────────────────────────

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
                        "They'll live on your desktop — you can keep working normally around them.",
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
