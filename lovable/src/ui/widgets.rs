use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*};

use crate::critter::CritterId;
use crate::ui::{palette::Palette, state::Tab};

/// Identifies which setting a `ToggleWidget` controls.
/// Used only for reading current value from `AppSettingsUiState` and dispatching
/// the correct `SettingsMsg` variant — no string dispatch anywhere.
#[derive(Clone, PartialEq, Eq, Debug)]
pub enum SettingId {
    StartOnBoot,
    StartMinimized,
    ShowTrayIcon,
    CloseToTray,
    CheckForUpdates,
    SendAnalytics,
    InteractionsEnabled,
    ShowNameOnHover,
    SoundEnabled,
    AllowCritterRoaming,
}

//  Marker components

#[derive(Component)]
pub struct UiRoot;

#[derive(Component)]
pub struct OnboardingRoot;

#[derive(Component)]
pub struct MainUiRoot;

/// Marks the top-level node of each onboarding screen. `0` = Welcome, `1` = Pick, `2` = Monitor, `3` = Info.
#[derive(Component, Clone)]
pub struct OnboardingScreen(pub u8);

#[derive(Component)]
pub struct TabContent(pub Tab);

#[derive(Component)]
pub struct TabButton(pub Tab);

// Onboarding navigation
#[derive(Component)]
pub struct OnboardingPrimaryButton;

#[derive(Component)]
pub struct OnboardingSkipButton;

/// Card in the critter-selection grid during onboarding.
#[derive(Component, Clone)]
pub struct OnboardingCritterCard {
    pub def_id: String,
}

/// Card in the monitor-selection grid during onboarding.
#[derive(Component, Clone)]
pub struct OnboardingMonitorCard {
    pub monitor_fingerprint: u64,
}

// Home tab
#[derive(Component)]
pub struct HideAllButton;

#[derive(Component)]
pub struct ShowAllButton;

// Critters tab — all carry `CritterId` so the single interaction reader can
// dispatch the right `Msg` variant without per-marker query fanout.
#[derive(Component)]
pub struct CritterExpandButton {
    pub critter_id: CritterId,
}

#[derive(Component)]
pub struct CritterExpandContent {
    pub critter_id: CritterId,
}
#[derive(Component)]
pub struct CritterVisibilityButton {
    pub critter_id: CritterId,
}
#[derive(Component)]
pub struct CritterDeleteButton {
    pub critter_id: CritterId,
}
#[derive(Component)]
pub struct DeleteConfirmButton {
    pub critter_id: CritterId,
}
#[derive(Component)]
pub struct DeleteCancelButton;
#[derive(Component)]
pub struct DeleteConfirmPanel(pub CritterId);

/// Assign button chip inside an expanded critter row.
#[derive(Component, Clone)]
pub struct MonitorAssignButton {
    pub critter_id: CritterId,
    pub monitor_fingerprint: u64,
}

#[derive(Component)]
pub struct AdoptButton {
    pub def_id: String,
}

// Settings
#[derive(Component)]
pub struct ToggleWidget {
    pub id: SettingId,
    pub is_on: bool,
}

#[derive(Component)]
pub struct ToggleThumb;
#[derive(Component)]
pub struct CheckForUpdatesButton;

// ─── Layout helpers ───────────────────────────────────────────────────────────

pub fn spawn_separator(parent: &mut RelatedSpawnerCommands<'_, ChildOf>, palette: &Palette) {
    parent.spawn((
        Node {
            width: Val::Percent(100.0),
            height: Val::Px(1.0),
            margin: UiRect::vertical(Val::Px(12.0)),
            ..default()
        },
        BackgroundColor(palette.border),
    ));
}

pub fn spawn_section_header(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    text: &str,
    palette: &Palette,
) {
    parent.spawn((
        Text::new(text),
        TextFont {
            font_size: 13.0,
            ..default()
        },
        TextColor(palette.foreground),
        Node {
            margin: UiRect::bottom(Val::Px(8.0)),
            ..default()
        },
    ));
}

pub fn spawn_card(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    palette: &Palette,
    margin: UiRect,
    children: impl FnOnce(&mut RelatedSpawnerCommands<'_, ChildOf>),
) {
    parent
        .spawn((
            Node {
                padding: UiRect::all(Val::Px(16.0)),
                border: UiRect::all(Val::Px(1.0)),
                flex_direction: FlexDirection::Column,
                width: Val::Percent(100.0),
                margin,
                ..default()
            },
            BorderColor(palette.border),
            BackgroundColor(palette.card),
            BorderRadius::all(Val::Px(8.0)),
        ))
        .with_children(children);
}

pub fn spawn_toggle_row(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    label: &str,
    sublabel: &str,
    id: SettingId,
    is_on: bool,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                flex_direction: FlexDirection::Row,
                align_items: AlignItems::Center,
                justify_content: JustifyContent::SpaceBetween,
                padding: UiRect::all(Val::Px(12.0)),
                border: UiRect::all(Val::Px(1.0)),
                width: Val::Percent(100.0),
                margin: UiRect::bottom(Val::Px(8.0)),
                ..default()
            },
            BorderColor(palette.border),
            BackgroundColor(palette.card),
            BorderRadius::all(Val::Px(8.0)),
        ))
        .with_children(|row| {
            row.spawn((Node {
                flex_direction: FlexDirection::Column,
                ..default()
            },))
                .with_children(|col| {
                    col.spawn((
                        Text::new(label),
                        TextFont {
                            font_size: 14.0,
                            ..default()
                        },
                        TextColor(palette.card_foreground),
                    ));
                    col.spawn((
                        Text::new(sublabel),
                        TextFont {
                            font_size: 11.0,
                            ..default()
                        },
                        TextColor(palette.muted_foreground),
                    ));
                });
            spawn_toggle_widget(row, id, is_on, palette);
        });
}

pub fn spawn_toggle_widget(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    id: SettingId,
    is_on: bool,
    palette: &Palette,
) {
    let bg_color = if is_on {
        palette.primary
    } else {
        palette.muted
    };
    let justify = if is_on {
        JustifyContent::FlexEnd
    } else {
        JustifyContent::FlexStart
    };

    parent
        .spawn((
            Node {
                width: Val::Px(44.0),
                height: Val::Px(24.0),
                padding: UiRect::all(Val::Px(2.0)),
                align_items: AlignItems::Center,
                justify_content: justify,
                ..default()
            },
            BorderRadius::all(Val::Px(12.0)),
            BackgroundColor(bg_color),
            Button,
            ToggleWidget { id, is_on },
        ))
        .with_children(|toggle| {
            toggle.spawn((
                Node {
                    width: Val::Px(20.0),
                    height: Val::Px(20.0),
                    ..default()
                },
                BorderRadius::all(Val::Px(10.0)),
                BackgroundColor(palette.primary_foreground),
                ToggleThumb,
            ));
        });
}

pub fn spawn_primary_button(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    text: &str,
    palette: &Palette,
    marker: impl Bundle,
) {
    parent
        .spawn((
            Node {
                padding: UiRect::axes(Val::Px(20.0), Val::Px(10.0)),
                margin: UiRect::right(Val::Px(8.0)),
                align_items: AlignItems::Center,
                justify_content: JustifyContent::Center,
                ..default()
            },
            BorderRadius::all(Val::Px(6.0)),
            BackgroundColor(palette.primary),
            Button,
            marker,
        ))
        .with_children(|btn| {
            btn.spawn((
                Text::new(text),
                TextFont {
                    font_size: 14.0,
                    ..default()
                },
                TextColor(palette.primary_foreground),
            ));
        });
}

pub fn spawn_outline_button(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    text: &str,
    palette: &Palette,
    marker: impl Bundle,
) {
    parent
        .spawn((
            Node {
                padding: UiRect::axes(Val::Px(20.0), Val::Px(10.0)),
                margin: UiRect::right(Val::Px(8.0)),
                border: UiRect::all(Val::Px(1.0)),
                align_items: AlignItems::Center,
                justify_content: JustifyContent::Center,
                ..default()
            },
            BorderRadius::all(Val::Px(6.0)),
            BorderColor(palette.border),
            BackgroundColor(palette.card),
            Button,
            marker,
        ))
        .with_children(|btn| {
            btn.spawn((
                Text::new(text),
                TextFont {
                    font_size: 14.0,
                    ..default()
                },
                TextColor(palette.card_foreground),
            ));
        });
}
