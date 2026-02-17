use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*};

use crate::ui::{palette::Palette, state::Tab, widgets::spawn_separator};

pub fn spawn_about_tab(parent: &mut RelatedSpawnerCommands<'_, ChildOf>, palette: &Palette) {
    parent
        .spawn((
            Node {
                display: Display::None,
                flex_direction: FlexDirection::Column,
                width: Val::Percent(100.0),
                align_items: AlignItems::Center,
                ..default()
            },
            crate::ui::widgets::TabContent(Tab::About),
        ))
        .with_children(|tab| {
            tab.spawn((
                Text::new("Pets for your desktop"),
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

            // ── Version card ────────────────────────────────────────────────
            spawn_info_card(tab, "Version", "0.2 Beta", palette);
            spawn_info_card(tab, "Created by", "Sandboxedideas", palette);
            spawn_info_card(tab, "Built with", "Bevy 0.16", palette);

            spawn_separator(tab, palette);

            // ── Description ─────────────────────────────────────────────────
            tab.spawn((
                Node {
                    flex_direction: FlexDirection::Column,
                    width: Val::Percent(100.0),
                    padding: UiRect::all(Val::Px(20.0)),
                    border: UiRect::all(Val::Px(1.0)),
                    margin: UiRect::bottom(Val::Px(16.0)),
                    ..default()
                },
                BorderRadius::all(Val::Px(8.0)),
                BorderColor(palette.border),
                BackgroundColor(palette.card),
            ))
            .with_children(|card| {
                card.spawn((
                    Text::new("About"),
                    TextFont {
                        font_size: 14.0,
                        ..default()
                    },
                    TextColor(palette.muted_foreground),
                    Node {
                        margin: UiRect::bottom(Val::Px(8.0)),
                        ..default()
                    },
                ));
                card.spawn((
                    Text::new(
                        "Lovable places tiny animated companions on your desktop. \
                         They wander around, react to your mouse, and generally make \
                         your day a little more delightful.",
                    ),
                    TextFont {
                        font_size: 14.0,
                        ..default()
                    },
                    TextColor(palette.card_foreground),
                ));
            });

            // ── Links row ────────────────────────────────────────────────────
            tab.spawn((Node {
                flex_direction: FlexDirection::Row,
                justify_content: JustifyContent::Center,
                ..default()
            },))
                .with_children(|row| {
                    spawn_link_chip(row, "Privacy Policy", palette);
                    spawn_link_chip(row, "Terms of Use", palette);
                    spawn_link_chip(row, "Support", palette);
                });
        });
}

fn spawn_info_card(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    label: &str,
    value: &str,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                flex_direction: FlexDirection::Row,
                align_items: AlignItems::Center,
                justify_content: JustifyContent::SpaceBetween,
                padding: UiRect::axes(Val::Px(20.0), Val::Px(12.0)),
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
            row.spawn((
                Text::new(label),
                TextFont {
                    font_size: 14.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
            ));
            row.spawn((
                Text::new(value),
                TextFont {
                    font_size: 14.0,
                    ..default()
                },
                TextColor(palette.primary),
            ));
        });
}

fn spawn_link_chip(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    label: &str,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                padding: UiRect::axes(Val::Px(14.0), Val::Px(8.0)),
                border: UiRect::all(Val::Px(1.0)),
                margin: UiRect::right(Val::Px(8.0)),
                align_items: AlignItems::Center,
                justify_content: JustifyContent::Center,
                ..default()
            },
            BorderRadius::all(Val::Px(20.0)),
            BorderColor(palette.border),
            BackgroundColor(palette.card),
            Button,
        ))
        .with_children(|chip| {
            chip.spawn((
                Text::new(label),
                TextFont {
                    font_size: 12.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
            ));
        });
}
