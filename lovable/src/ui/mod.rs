pub mod about_tab;
pub mod critters_tab;
pub mod events;
pub mod home_tab;
pub mod onboarding;
pub mod palette;
pub mod settings_tab;
pub mod state;
pub mod systems;
pub mod widgets;

use bevy::prelude::*;

use crate::ui::{
    about_tab::spawn_about_tab,
    critters_tab::spawn_critters_tab,
    events::*,
    home_tab::spawn_home_tab,
    onboarding::spawn_onboarding,
    palette::{GAMING_THEME, Palette},
    settings_tab::spawn_settings_tab,
    state::*,
    systems::*,
    widgets::*,
};

pub struct LovableUI;

impl Plugin for LovableUI {
    fn build(&self, app: &mut App) {
        app
            // Resources
            .insert_resource(UiState::default())
            // TODO: Fetch from web
            .insert_resource(CritterRoster::default())
            .insert_resource(AppSettingsUiState::default())
            .insert_resource(MonitorList::stub_primary())
            .insert_resource(GAMING_THEME)
            // Events (backend systems subscribe to these to react to user actions)
            .add_event::<AdoptCritterRequested>()
            .add_event::<DeleteCritterConfirmed>()
            .add_event::<ToggleCritterVisibility>()
            .add_event::<HideAllCritters>()
            .add_event::<ShowAllCritters>()
            .add_event::<SettingChanged>()
            .add_event::<AssignCritterToMonitor>()
            .add_event::<CheckForUpdatesRequested>()
            .add_event::<OnboardingComplete>()
            .add_event::<OnboardingStepAdvanced>()
            // Setup
            .add_systems(Startup, setup_ui)
            // Navigation systems
            .add_systems(
                Update,
                (
                    handle_tab_buttons,
                    update_tab_visibility,
                    update_onboarding_main_visibility,
                    update_onboarding_step_visibility,
                    handle_onboarding_primary,
                    handle_onboarding_skip,
                    handle_onboarding_critter_selection,
                    update_onboarding_critter_card_visuals,
                ),
            )
            // Interaction systems
            .add_systems(
                Update,
                (
                    handle_toggle_clicks,
                    handle_critter_header_click,
                    update_critter_expand_visibility,
                    handle_delete_button,
                    handle_delete_confirm,
                    handle_delete_cancel,
                    update_delete_confirm_visibility,
                    handle_critter_visibility_toggle,
                    handle_adopt_button,
                    handle_hide_all,
                    handle_show_all,
                    handle_check_for_updates,
                ),
            );
    }
}

fn setup_ui(
    mut commands: Commands,
    ui_state: Res<UiState>,
    roster: Res<CritterRoster>,
    settings: Res<AppSettingsUiState>,
    monitors: Res<MonitorList>,
    palette: Res<Palette>,
) {
    let palette = palette.into_inner();
    let ui_state = ui_state.into_inner();
    let roster = roster.into_inner();
    let settings = settings.into_inner();
    let monitors = monitors.into_inner();

    commands.spawn(Camera2d);

    // ── Full-screen background layer ─────────────────────────────────────────
    // The outermost node fills the window and horizontally centres its single
    // child (the content column). This gives us the "max-width centred panel"
    // layout from the prototype without hard-coding pixel positions.
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
            // ── Onboarding flow (hidden after first launch) ───────────────
            root.spawn((
                Node {
                    display: if ui_state.onboarding_complete {
                        Display::None
                    } else {
                        Display::Flex
                    },
                    flex_direction: FlexDirection::Column,
                    width: Val::Percent(100.0),
                    height: Val::Percent(100.0),
                    align_items: AlignItems::Center,
                    ..default()
                },
                OnboardingRoot,
            ))
            .with_children(|onboarding| {
                spawn_onboarding(onboarding, ui_state, roster, monitors, palette);
            });

            // ── Main application UI (hidden during onboarding) ────────────
            // Capped at 680 px so it reads like a compact control panel rather
            // than stretching across an ultrawide screen.
            root.spawn((
                Node {
                    display: if ui_state.onboarding_complete {
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
                spawn_tab_area(main, ui_state, roster, settings, monitors, palette);
                spawn_footer(main, palette);
            });
        });
}

// ─── Header ───────────────────────────────────────────────────────────────────

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

// ─── Tab bar ──────────────────────────────────────────────────────────────────

fn spawn_tab_bar(
    parent: &mut bevy::ecs::relationship::RelatedSpawnerCommands<'_, ChildOf>,
    palette: &Palette,
) {
    // The outer wrapper centres the pill without stretching it to full width.
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
                    spawn_tab_button(bar, "Home", Tab::Home, palette);
                    spawn_tab_button(bar, "My Critters", Tab::Critters, palette);
                    spawn_tab_button(bar, "Settings", Tab::Settings, palette);
                    spawn_tab_button(bar, "About", Tab::About, palette);
                });
        });
}

fn spawn_tab_button(
    parent: &mut bevy::ecs::relationship::RelatedSpawnerCommands<'_, ChildOf>,
    label: &str,
    tab: Tab,
    palette: &Palette,
) {
    parent
        .spawn((
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

// ─── Tab content area ─────────────────────────────────────────────────────────

fn spawn_tab_area(
    parent: &mut bevy::ecs::relationship::RelatedSpawnerCommands<'_, ChildOf>,
    ui_state: &UiState,
    roster: &CritterRoster,
    settings: &AppSettingsUiState,
    monitors: &MonitorList,
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
            spawn_home_tab(area, roster, monitors, palette);
            spawn_critters_tab(area, ui_state, roster, monitors, palette);
            spawn_settings_tab(area, settings, monitors, palette);
            spawn_about_tab(area, palette);
        });
}

// ─── Footer ───────────────────────────────────────────────────────────────────

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
                Text::new(format!("Lovable  · {}", env!("CARGO_PKG_VERSION"))),
                TextFont {
                    font_size: 11.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
            ));
        });
}
