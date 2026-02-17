use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*};

use crate::ui::{events::SettingToggleId, palette::Palette, state::Tab};

// ─── Marker Components ────────────────────────────────────────────────────────

#[derive(Component)]
pub struct UiRoot;

#[derive(Component)]
pub struct OnboardingRoot;

#[derive(Component)]
pub struct MainUiRoot;

/// Marks the top-level node of each onboarding screen.
#[derive(Component, Clone, PartialEq, Eq)]
pub struct OnboardingScreen(pub u8); // 0=Welcome, 1=Pick, 2=Monitor, 3=Info

/// Marks each tab's content node.
#[derive(Component)]
pub struct TabContent(pub Tab);

/// Marks the tab bar button for a tab.
#[derive(Component)]
pub struct TabButton(pub Tab);

// Onboarding navigation
#[derive(Component)]
pub struct OnboardingPrimaryButton;

#[derive(Component)]
pub struct OnboardingSkipButton;

/// Card in the onboarding critter selection grid.
#[derive(Component)]
pub struct OnboardingCritterCard(pub usize); // index into CRITTER_TEMPLATES

// Home tab actions
#[derive(Component)]
pub struct HideAllButton;

#[derive(Component)]
pub struct ShowAllButton;

// Critter rows in the critters tab
#[derive(Component)]
pub struct CritterHeaderButton(pub usize); // index in CritterRoster::owned

#[derive(Component)]
pub struct CritterExpandContent(pub usize);

#[derive(Component)]
pub struct DeleteConfirmPanel(pub usize);

#[derive(Component)]
pub struct CritterDeleteButton(pub usize);

#[derive(Component)]
pub struct DeleteConfirmButton(pub usize);

#[derive(Component)]
pub struct DeleteCancelButton(pub usize);

#[derive(Component)]
pub struct CritterVisibilityButton(pub usize);

#[derive(Component)]
pub struct CritterAdoptButton(pub &'static str); // template id

// Settings toggles
#[derive(Component)]
pub struct ToggleWidget {
    pub id: SettingToggleId,
    pub is_on: bool,
}

/// The sliding thumb inside a ToggleWidget. Updated by toggle system.
#[derive(Component)]
pub struct ToggleThumb;

#[derive(Component)]
pub struct CheckForUpdatesButton;

// ─── Layout Helpers ───────────────────────────────────────────────────────────

/// Spawns a full-width horizontal divider line.
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

/// Spawns a small section heading (e.g. "Application Settings").
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

/// Spawns a card container (rounded border, card background) and passes a child spawner
/// to `children` for populating its contents.
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

/// Spawns a labelled toggle row:
///   [label column] [toggle widget]
/// The toggle widget is a Button; clicking it fires toggle logic in systems.rs.
pub fn spawn_toggle_row(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    label: &str,
    sublabel: &str,
    id: SettingToggleId,
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
            // Label column
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

            // Toggle pill button
            spawn_toggle_widget(row, id, is_on, palette);
        });
}

/// Spawns a pill-shaped toggle button with a sliding thumb.
pub fn spawn_toggle_widget(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    id: SettingToggleId,
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

/// Spawns a styled action button (filled primary style).
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
                border: UiRect::all(Val::Px(0.0)),
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

/// Spawns a styled action button (outline secondary style).
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
