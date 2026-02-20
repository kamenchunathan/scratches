pub mod about_tab;
pub mod critters_tab;
pub mod home_tab;
pub mod messages;
pub mod onboarding;
pub mod palette;
pub mod settings_tab;
pub mod state;
pub mod systems;
pub mod widgets;

use bevy::{prelude::*, window::Monitor};

use crate::{
    AppState,
    critter::CritterRegistry,
    preferences::{Preferences, PreferencesHandle, RegistryHandle, generate_monitor_fingerprint},
    ui::{
        about_tab::spawn_about_tab,
        critters_tab::spawn_critters_tab,
        home_tab::spawn_home_tab,
        messages::Msg,
        onboarding::spawn_onboarding,
        palette::{GAMING_THEME, Palette},
        settings_tab::spawn_settings_tab,
        state::{AppScreen, AppSettingsUiState},
        systems::*,
        widgets::*,
    },
};

pub struct LovableUI;

impl Plugin for LovableUI {
    fn build(&self, app: &mut App) {
        app.add_event::<Msg>()
            .insert_resource(GAMING_THEME)
            .add_systems(
                OnEnter(AppState::Running),
                (build_app_screen_from_prefs, setup_ui).chain(),
            )
            .add_systems(
                Update,
                (
                    read_inp_and_dispatch_msg,
                    update,
                    (
                        handle_tab_press,
                        sync_tab_visibility,
                        sync_onboarding_main_visibility,
                        sync_onboarding_step,
                        sync_critter_expand,
                        sync_toggle_visuals,
                        sync_onboarding_critter_cards,
                        sync_tab_button_visuals,
                    ),
                )
                    .chain()
                    .run_if(in_state(AppState::Running)),
            );
    }
}

/// Derives `AppScreen` and `AppSettingsUiState` from loaded `Preferences`.
/// Runs once on transition into `Running`.
pub fn build_app_screen_from_prefs(
    mut commands: Commands,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
) {
    let prefs = prefs_store
        .get(&prefs_handle.0)
        .expect("Preferences must be loaded before Running is entered");

    commands.insert_resource(AppScreen::from_preferences(prefs));

    commands.insert_resource(AppSettingsUiState {
        start_on_boot: prefs.app_settings.start_on_boot,
        start_minimized: prefs.app_settings.start_minimized,
        show_tray_icon: prefs.app_settings.show_tray_icon,
        close_to_tray: prefs.app_settings.close_to_tray,
        check_for_updates: prefs.app_settings.check_updates,
        send_analytics: prefs.app_settings.send_analytics,
        interactions_enabled: prefs.global_critter_settings.interactions_enabled,
        show_name_on_hover: prefs.global_critter_settings.display_critter_name_on_hover,
        sound_enabled: prefs.global_critter_settings.sound_enabled,
        allow_critter_roaming: false,
    });
}

/// Spawns the full UI tree. Runs once after `build_app_screen_from_prefs`.
fn setup_ui(
    mut commands: Commands,
    screen: Res<AppScreen>,
    settings: Res<AppSettingsUiState>,
    palette: Res<Palette>,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    registry_handle: Res<RegistryHandle>,
    registry_store: Res<Assets<CritterRegistry>>,
    monitors: Query<&Monitor>,
) {
    let palette = palette.into_inner();
    let screen = screen.into_inner();
    let settings = settings.into_inner();
    let prefs = prefs_store.get(&prefs_handle.0).unwrap();
    let registry = registry_store.get(&registry_handle.0).unwrap();

    // Build the (monitor, fingerprint) slice once; passed into every tab that needs it.
    let monitor_list: Vec<(&Monitor, u64)> = monitors
        .iter()
        .map(|m| (m, generate_monitor_fingerprint(m)))
        .collect();

    commands.spawn(Camera2d);

    commands
        .spawn((
            Node {
                width: Val::Percent(100.0),
                height: Val::Percent(100.0),
                flex_direction: FlexDirection::Column,
                align_items: AlignItems::Center,
                ..default()
            },
            BackgroundColor(palette.background),
            UiRoot,
        ))
        .with_children(|root| {
            // Onboarding root
            root.spawn((
                Node {
                    display: if matches!(screen, AppScreen::Onboarding(_)) {
                        Display::Flex
                    } else {
                        Display::None
                    },
                    flex_direction: FlexDirection::Column,
                    width: Val::Percent(100.0),
                    height: Val::Percent(100.0),
                    align_items: AlignItems::Center,
                    ..default()
                },
                OnboardingRoot,
            ))
            .with_children(|ob| {
                spawn_onboarding(ob, screen, registry, &monitor_list, palette);
            });

            // Main UI root
            root.spawn((
                Node {
                    display: if matches!(screen, AppScreen::Main(_)) {
                        Display::Flex
                    } else {
                        Display::None
                    },
                    flex_direction: FlexDirection::Column,
                    width: Val::Px(680.0),
                    height: Val::Percent(100.0),
                    padding: UiRect::axes(Val::Px(0.0), Val::Px(24.0)),
                    ..default()
                },
                MainUiRoot,
            ))
            .with_children(|main| {
                spawn_header(main, palette);
                spawn_tab_bar(main, palette);
                spawn_tab_area(
                    main,
                    screen,
                    settings,
                    prefs,
                    registry,
                    &monitor_list,
                    palette,
                );
                spawn_footer(main, palette);
            });
        });
}

fn spawn_header(
    parent: &mut bevy::ecs::relationship::RelatedSpawnerCommands<'_, ChildOf>,
    palette: &Palette,
) {
    parent
        .spawn((Node {
            flex_direction: FlexDirection::Column,
            align_items: AlignItems::Center,
            width: Val::Percent(100.0),
            margin: UiRect::bottom(Val::Px(24.0)),
            ..default()
        },))
        .with_children(|header| {
            header.spawn((
                Text::new("LOVABLE"),
                TextFont {
                    font_size: 36.0,
                    ..default()
                },
                TextColor(palette.primary),
            ));
            header.spawn((
                Text::new("Your digital companion management center"),
                TextFont {
                    font_size: 14.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
            ));
        });
}

fn spawn_tab_bar(
    parent: &mut bevy::ecs::relationship::RelatedSpawnerCommands<'_, ChildOf>,
    palette: &Palette,
) {
    parent
        .spawn((Node {
            flex_direction: FlexDirection::Row,
            justify_content: JustifyContent::Center,
            width: Val::Percent(100.0),
            margin: UiRect::bottom(Val::Px(20.0)),
            ..default()
        },))
        .with_children(|wrapper| {
            wrapper
                .spawn((
                    Node {
                        flex_direction: FlexDirection::Row,
                        border: UiRect::all(Val::Px(1.0)),
                        padding: UiRect::all(Val::Px(4.0)),
                        ..default()
                    },
                    BorderRadius::all(Val::Px(8.0)),
                    BorderColor(palette.border),
                    BackgroundColor(palette.muted),
                ))
                .with_children(|bar| {
                    use crate::ui::state::Tab;
                    for (label, tab) in [
                        ("Home", Tab::Home),
                        ("My Critters", Tab::Critters),
                        ("Settings", Tab::Settings),
                        ("About", Tab::About),
                    ] {
                        bar.spawn((
                            Node {
                                padding: UiRect::axes(Val::Px(18.0), Val::Px(8.0)),
                                margin: UiRect::right(Val::Px(4.0)),
                                align_items: AlignItems::Center,
                                justify_content: JustifyContent::Center,
                                ..default()
                            },
                            BorderRadius::all(Val::Px(6.0)),
                            BackgroundColor(palette.card),
                            Button,
                            TabButton(tab),
                        ))
                        .with_children(|btn| {
                            btn.spawn((
                                Text::new(label),
                                TextFont {
                                    font_size: 14.0,
                                    ..default()
                                },
                                TextColor(palette.card_foreground),
                            ));
                        });
                    }
                });
        });
}

fn spawn_tab_area(
    parent: &mut bevy::ecs::relationship::RelatedSpawnerCommands<'_, ChildOf>,
    screen: &AppScreen,
    settings: &AppSettingsUiState,
    prefs: &Preferences,
    registry: &CritterRegistry,
    monitors: &[(&Monitor, u64)],
    palette: &Palette,
) {
    parent
        .spawn((Node {
            flex_direction: FlexDirection::Column,
            flex_grow: 1.0,
            width: Val::Percent(100.0),
            overflow: Overflow::scroll_y(),
            ..default()
        },))
        .with_children(|area| {
            spawn_home_tab(area, prefs, registry, monitors, palette);
            spawn_critters_tab(area, screen, prefs, registry, monitors, palette);
            spawn_settings_tab(area, settings, prefs, monitors, palette);
            spawn_about_tab(area, palette);
        });
}

fn spawn_footer(
    parent: &mut bevy::ecs::relationship::RelatedSpawnerCommands<'_, ChildOf>,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                border: UiRect::top(Val::Px(1.0)),
                padding: UiRect::top(Val::Px(12.0)),
                margin: UiRect::top(Val::Px(12.0)),
                justify_content: JustifyContent::Center,
                width: Val::Percent(100.0),
                ..default()
            },
            BorderColor(palette.border),
        ))
        .with_children(|footer| {
            footer.spawn((
                Text::new(format!("Lovable · {}", env!("CARGO_PKG_VERSION"))),
                TextFont {
                    font_size: 11.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
            ));
        });
}
