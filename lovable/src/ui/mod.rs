pub mod palette;

use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*};
use palette::Palette;

use crate::ui::palette::GAMING_THEME;

#[derive(Resource)]
struct UiState {
    active_tab: Tab,
    pets: Vec<Pet>,
    monitors: Vec<Monitor>,
    active_pet_count: usize,
}

#[derive(Clone, PartialEq, Eq, Hash, Debug)]
enum Tab {
    Dashboard,
    Pets,
    Settings,
    About,
}

#[derive(Component, Clone)]
struct Pet {
    id: String,
    name: String,
    emoji: String,
    is_active: bool,
    is_owned: bool,
    assigned_monitor: Option<u32>,
}

#[derive(Component, Clone)]
struct Monitor {
    id: u32,
    name: String,
    resolution: String,
}

#[derive(Component)]
struct UiRoot;

#[derive(Component)]
struct TabContent(Tab);

#[derive(Component)]
struct TabButton(Tab);

pub struct LovableUI;

impl Plugin for LovableUI {
    fn build(&self, app: &mut App) {
        app.insert_resource(UiState {
            active_tab: Tab::Dashboard,
            pets: vec![
                Pet {
                    id: "1".to_string(),
                    name: "Biscuit".to_string(),
                    emoji: "🍪".to_string(),
                    is_active: true,
                    is_owned: true,
                    assigned_monitor: Some(1),
                },
                Pet {
                    id: "2".to_string(),
                    name: "Whiskers".to_string(),
                    emoji: "🐱".to_string(),
                    is_active: false,
                    is_owned: true,
                    assigned_monitor: Some(1),
                },
            ],
            monitors: vec![
                Monitor {
                    id: 1,
                    name: "Primary Monitor".to_string(),
                    resolution: "1920x1080".to_string(),
                },
                Monitor {
                    id: 2,
                    name: "Secondary Monitor".to_string(),
                    resolution: "1440x900".to_string(),
                },
            ],
            active_pet_count: 3,
        })
        .insert_resource(GAMING_THEME)
        .add_systems(Startup, setup_ui)
        .add_systems(
            Update,
            (handle_tab_selection, update_tab_content_visibility),
        );
    }
}

fn setup_ui(mut commands: Commands, ui_state: Res<UiState>, palette: Res<Palette>) {
    let palette = palette.into_inner();
    commands.spawn(Camera2d);
    commands
        .spawn((
            Node {
                width: Val::Percent(100.0),
                height: Val::Percent(100.0),
                flex_direction: FlexDirection::Column,
                align_items: AlignItems::Center,
                padding: UiRect::all(Val::Px(20.0)),
                ..default()
            },
            BackgroundColor(palette.background),
            UiRoot,
        ))
        .with_children(|parent| {
            // Header
            parent.spawn((
                Text::new("LOVABLE"),
                TextColor(palette.primary),
                TextFont {
                    font_size: 40.0,
                    ..default()
                },
            ));
            parent.spawn((
                Text::new("Level up your desktop with digital companions!"),
                TextColor(palette.foreground),
                TextFont {
                    font_size: 20.0,
                    ..default()
                },
            ));

            // Tabs
            parent
                .spawn((Node {
                    flex_direction: FlexDirection::Row,
                    margin: UiRect::top(Val::Px(20.0)),
                    ..default()
                },))
                .with_children(|parent| {
                    spawn_tab_button(parent, "Dashboard", Tab::Dashboard, palette);
                    spawn_tab_button(parent, "Pets", Tab::Pets, palette);
                    spawn_tab_button(parent, "Settings", Tab::Settings, palette);
                    spawn_tab_button(parent, "About", Tab::About, palette);
                });

            // Tab Content Area
            parent
                .spawn(Node {
                    width: Val::Percent(80.0),
                    height: Val::Percent(100.0),
                    margin: UiRect::top(Val::Px(20.0)),
                    ..default()
                })
                .with_children(|parent| {
                    spawn_dashboard_tab(parent, &ui_state, palette);
                    spawn_pets_tab(parent, &ui_state, palette);
                    spawn_settings_tab(parent, &ui_state, palette);
                    spawn_about_tab(parent, &ui_state, palette);
                });
        });
}

// ANCHOR: Tab button spawning
fn spawn_tab_button(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    text: &str,
    tab: Tab,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                padding: UiRect::all(Val::Px(10.0)),
                margin: UiRect::horizontal(Val::Px(5.0)),
                border: UiRect::all(Val::Px(1.0)),
                ..default()
            },
            BorderColor(palette.border),
            BackgroundColor(palette.card),
            TabButton(tab),
        ))
        .with_children(|parent| {
            parent.spawn((
                Text::new(text),
                TextFont {
                    font_size: 16.0,
                    ..default()
                },
                BackgroundColor(palette.card_foreground),
            ));
        });
}

// ANCHOR: Dashboard tab
fn spawn_dashboard_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    ui_state: &UiState,
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
            TabContent(Tab::Dashboard),
        ))
        .with_children(|parent| {
            // Card
            parent
                .spawn((
                    Node {
                        padding: UiRect::all(Val::Px(20.0)),
                        border: UiRect::all(Val::Px(1.0)),
                        flex_direction: FlexDirection::Column,
                        ..default()
                    },
                    BorderColor(palette.border),
                    BackgroundColor(palette.card),
                ))
                .with_children(|parent| {
                    // Card Header
                    parent.spawn((
                        Text::new("Your digital companions are ready for action!"),
                        TextColor(palette.card_foreground),
                        TextFont {
                            font_size: 24.0,
                            ..default()
                        },
                    ));
                    parent.spawn((
                        Text::new("Control how many adorable companions appear on your desktop"),
                        TextColor(palette.muted_foreground),
                        TextFont {
                            font_size: 16.0,
                            ..default()
                        },
                    ));

                    // Monitor Layout
                    parent.spawn((
                        Text::new("Monitor Layout"),
                        TextColor(palette.primary),
                        TextFont {
                            font_size: 20.0,
                            ..default()
                        },
                        Node {
                            margin: UiRect::top(Val::Px(20.0)),
                            ..default()
                        },
                    ));
                    parent
                        .spawn((Node {
                            flex_direction: FlexDirection::Row,
                            justify_content: JustifyContent::SpaceAround,
                            margin: UiRect::top(Val::Px(10.0)),
                            ..default()
                        },))
                        .with_children(|parent| {
                            for monitor in &ui_state.monitors {
                                parent
                                    .spawn((
                                        Node {
                                            flex_direction: FlexDirection::Column,
                                            align_items: AlignItems::Center,
                                            padding: UiRect::all(Val::Px(10.0)),
                                            border: UiRect::all(Val::Px(1.0)),
                                            margin: UiRect::horizontal(Val::Px(10.0)),
                                            ..default()
                                        },
                                        BorderColor(palette.border),
                                        BackgroundColor(palette.muted),
                                    ))
                                    .with_children(|parent| {
                                        parent.spawn((
                                            Text::new(&monitor.name),
                                            TextFont {
                                                font_size: 16.0,
                                                ..default()
                                            },
                                            TextColor(palette.muted_foreground),
                                        ));
                                        parent.spawn((
                                            Text::new(&monitor.resolution),
                                            TextFont {
                                                font_size: 12.0,
                                                ..default()
                                            },
                                            TextColor(palette.muted_foreground),
                                        ));
                                        // Pets on monitor
                                        parent
                                            .spawn((Node {
                                                flex_direction: FlexDirection::Row,
                                                margin: UiRect::top(Val::Px(10.0)),
                                                ..default()
                                            },))
                                            .with_children(|parent| {
                                                for pet in ui_state.pets.iter().filter(|p| {
                                                    p.is_active
                                                        && p.assigned_monitor == Some(monitor.id)
                                                }) {
                                                    parent.spawn((
                                                        Text::new(&pet.emoji),
                                                        TextFont {
                                                            font_size: 24.0,
                                                            ..default()
                                                        },
                                                    ));
                                                }
                                            });
                                    });
                            }
                        });
                });
        });
}

// ANCHOR: Pets tab
fn spawn_pets_tab(
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
                ..default()
            },
            TabContent(Tab::Pets),
        ))
        .with_children(|parent| {
            // Owned Pets Card
            parent
                .spawn((
                    Node {
                        padding: UiRect::all(Val::Px(20.0)),
                        border: UiRect::all(Val::Px(1.0)),
                        flex_direction: FlexDirection::Column,
                        margin: UiRect::bottom(Val::Px(20.0)),
                        ..default()
                    },
                    BorderColor(palette.border),
                    BackgroundColor(palette.card),
                ))
                .with_children(|parent| {
                    parent.spawn((
                        Text::new("Owned Pets"),
                        TextFont {
                            font_size: 24.0,
                            ..default()
                        },
                        TextColor(palette.primary),
                    ));
                    parent.spawn((
                        Text::new("Manage your adopted pets."),
                        TextFont {
                            font_size: 16.0,
                            ..default()
                        },
                        TextColor(palette.muted_foreground),
                    ));

                    for pet in ui_state.pets.iter().filter(|p| p.is_owned) {
                        parent
                            .spawn((
                                Node {
                                    flex_direction: FlexDirection::Row,
                                    align_items: AlignItems::Center,
                                    justify_content: JustifyContent::SpaceBetween,
                                    padding: UiRect::all(Val::Px(10.0)),
                                    margin: UiRect::top(Val::Px(5.0)),
                                    border: UiRect::all(Val::Px(1.0)),
                                    ..default()
                                },
                                BorderColor(palette.border),
                            ))
                            .with_children(|parent| {
                                parent.spawn((
                                    Text::new(&pet.emoji),
                                    TextFont {
                                        font_size: 24.0,
                                        ..default()
                                    },
                                ));
                                parent.spawn((
                                    Text::new(&pet.name),
                                    TextFont {
                                        font_size: 16.0,
                                        ..default()
                                    },
                                    TextColor(palette.foreground),
                                ));
                                // TODO: Add rename and hide/show buttons
                            });
                    }
                });

            // Available Pets Card
            parent
                .spawn((
                    Node {
                        padding: UiRect::all(Val::Px(20.0)),
                        border: UiRect::all(Val::Px(1.0)),
                        flex_direction: FlexDirection::Column,
                        ..default()
                    },
                    BorderColor(palette.border),
                    BackgroundColor(palette.card),
                ))
                .with_children(|parent| {
                    parent.spawn((
                        Text::new("Unlock New Companions"),
                        TextFont {
                            font_size: 24.0,
                            ..default()
                        },
                        TextColor(palette.primary),
                    ));
                    parent.spawn((
                        Text::new("Expand your digital companion roster."),
                        TextFont {
                            font_size: 16.0,
                            ..default()
                        },
                        TextColor(palette.muted_foreground),
                    ));

                    parent
                        .spawn((Node {
                            flex_direction: FlexDirection::Row,
                            flex_wrap: FlexWrap::Wrap,
                            margin: UiRect::top(Val::Px(10.0)),
                            ..default()
                        },))
                        .with_children(|parent| {
                            for pet in ui_state.pets.iter().filter(|p| !p.is_owned) {
                                parent
                                    .spawn((
                                        Node {
                                            flex_direction: FlexDirection::Column,
                                            align_items: AlignItems::Center,
                                            padding: UiRect::all(Val::Px(10.0)),
                                            margin: UiRect::all(Val::Px(5.0)),
                                            border: UiRect::all(Val::Px(1.0)),
                                            ..default()
                                        },
                                        BorderColor(palette.border),
                                    ))
                                    .with_children(|parent| {
                                        parent.spawn((
                                            Text::new(&pet.emoji),
                                            TextFont {
                                                font_size: 36.0,
                                                ..default()
                                            },
                                        ));
                                        parent.spawn((
                                            Text::new(&pet.name),
                                            TextFont {
                                                font_size: 16.0,
                                                ..default()
                                            },
                                            TextColor(palette.foreground),
                                        ));
                                        // TODO: Add unlock button
                                    });
                            }
                        });
                });
        });
}

// ANCHOR: Settings tab
fn spawn_settings_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    _ui_state: &UiState,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                display: Display::None,
                flex_direction: FlexDirection::Column,
                ..default()
            },
            TabContent(Tab::Settings),
        ))
        .with_children(|parent| {
            parent.spawn((
                Text::new("Settings"),
                TextFont {
                    font_size: 24.0,
                    ..default()
                },
                TextColor(palette.primary),
            ));
            parent.spawn((
                Text::new("Customize how Lovable works for you"),
                TextFont {
                    font_size: 16.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
            ));
        });
}

// ANCHOR: About tab
fn spawn_about_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    _ui_state: &UiState,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                display: Display::None,
                flex_direction: FlexDirection::Column,
                align_items: AlignItems::Center,
                ..default()
            },
            TabContent(Tab::About),
        ))
        .with_children(|parent| {
            parent.spawn((
                Text::new("About Lovable"),
                TextFont {
                    font_size: 24.0,
                    ..default()
                },
                TextColor(palette.primary),
            ));
            parent.spawn((
                Text::new("Version: 0.2 Beta"),
                TextFont {
                    font_size: 16.0,
                    ..default()
                },
                TextColor(palette.foreground),
            ));
            parent.spawn((
                Text::new("Created by Sandboxedideas"),
                TextFont {
                    font_size: 16.0,
                    ..default()
                },
                TextColor(palette.foreground),
            ));
        });
}

// ANCHOR: Systems
fn handle_tab_selection(
    mut interaction_query: Query<
        (&Interaction, &TabButton, &mut BackgroundColor),
        (Changed<Interaction>, With<Button>),
    >,
    mut ui_state: ResMut<UiState>,
    palette: Res<Palette>,
) {
    for (interaction, tab_button, mut color) in &mut interaction_query {
        match *interaction {
            Interaction::Pressed => {
                ui_state.active_tab = tab_button.0.clone();
            }
            Interaction::Hovered => {
                *color = BackgroundColor(palette.primary);
            }
            Interaction::None => {
                *color = BackgroundColor(palette.card);
            }
        }
    }
}

fn update_tab_content_visibility(
    ui_state: Res<UiState>,
    mut query: Query<(&mut Node, &TabContent)>,
) {
    if ui_state.is_changed() {
        for (mut node, tab_content) in &mut query {
            if tab_content.0 == ui_state.active_tab {
                node.display = Display::Flex;
            } else {
                node.display = Display::None;
            }
        }
    }
}
