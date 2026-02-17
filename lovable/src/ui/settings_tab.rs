use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*};

use crate::ui::{
    events::SettingToggleId,
    palette::Palette,
    state::{AppSettingsUiState, MonitorList, Tab},
    widgets::*,
};

/// Spawns the Settings tab content.
pub fn spawn_settings_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    settings: &AppSettingsUiState,
    monitors: &MonitorList,
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
            TabContent(Tab::Settings),
        ))
        .with_children(|tab| {
            spawn_app_settings_section(tab, settings, palette);
            spawn_separator(tab, palette);
            spawn_critter_settings_section(tab, settings, palette);
            spawn_separator(tab, palette);
            spawn_monitor_management_section(tab, settings, monitors, palette);
            spawn_separator(tab, palette);
            spawn_updates_section(tab, palette);
        });
}

// ─── Application settings ─────────────────────────────────────────────────────

fn spawn_app_settings_section(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    settings: &AppSettingsUiState,
    palette: &Palette,
) {
    spawn_section_header(parent, "Application Settings", palette);

    spawn_toggle_row(
        parent,
        "Launch Lovable when I log in",
        "Startup behavior",
        SettingToggleId::StartOnBoot,
        settings.start_on_boot,
        palette,
    );

    spawn_toggle_row(
        parent,
        "Go straight to the tray, skip the window",
        "Start minimized",
        SettingToggleId::StartMinimized,
        settings.start_minimized,
        palette,
    );

    spawn_toggle_row(
        parent,
        "Keep an icon in the system tray",
        "Tray visibility",
        SettingToggleId::ShowTrayIcon,
        settings.show_tray_icon,
        palette,
    );

    spawn_toggle_row(
        parent,
        "Closing this window keeps critters running",
        "Close to tray",
        SettingToggleId::CloseToTray,
        settings.close_to_tray,
        palette,
    );

    spawn_toggle_row(
        parent,
        "Check for new versions when Lovable starts",
        "Auto-update checks",
        SettingToggleId::CheckForUpdates,
        settings.check_for_updates,
        palette,
    );

    spawn_toggle_row(
        parent,
        "Share anonymous usage data to help improve Lovable",
        "Analytics",
        SettingToggleId::SendAnalytics,
        settings.send_analytics,
        palette,
    );
}

// ─── Global critter settings ──────────────────────────────────────────────────

fn spawn_critter_settings_section(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    settings: &AppSettingsUiState,
    palette: &Palette,
) {
    spawn_section_header(parent, "Global Critter Settings", palette);

    spawn_toggle_row(
        parent,
        "Let my critters respond to the mouse",
        "Interactions enabled",
        SettingToggleId::InteractionsEnabled,
        settings.interactions_enabled,
        palette,
    );

    spawn_toggle_row(
        parent,
        "Show their name when I hover over them",
        "Hover display",
        SettingToggleId::ShowNameOnHover,
        settings.show_name_on_hover,
        palette,
    );

    // Sound toggle — shown always; note below shown when sound is enabled
    spawn_toggle_row(
        parent,
        "Let critters make sounds",
        "Audio control",
        SettingToggleId::SoundEnabled,
        settings.sound_enabled,
        palette,
    );

    if settings.sound_enabled {
        // Volume display - a full Select widget would replace this in a future pass
        parent.spawn((
            Text::new("Volume: 70%  (use Settings > Audio to adjust)"),
            TextFont {
                font_size: 11.0,
                ..default()
            },
            TextColor(palette.muted_foreground),
            Node {
                margin: UiRect {
                    left: Val::Px(12.0),
                    bottom: Val::Px(12.0),
                    ..default()
                },
                ..default()
            },
        ));
    }
}

// ─── Monitor management ───────────────────────────────────────────────────────

fn spawn_monitor_management_section(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    settings: &AppSettingsUiState,
    monitors: &MonitorList,
    palette: &Palette,
) {
    spawn_section_header(parent, "Monitor Management", palette);

    // Cross-monitor roaming toggle
    parent
        .spawn((
            Node {
                flex_direction: FlexDirection::Row,
                align_items: AlignItems::Center,
                justify_content: JustifyContent::SpaceBetween,
                padding: UiRect::all(Val::Px(12.0)),
                border: UiRect::all(Val::Px(1.0)),
                width: Val::Percent(100.0),
                margin: UiRect::bottom(Val::Px(12.0)),
                ..default()
            },
            BorderRadius::all(Val::Px(8.0)),
            BorderColor(palette.border),
            BackgroundColor(palette.card),
        ))
        .with_children(|row| {
            row.spawn((Node {
                flex_direction: FlexDirection::Column,
                ..default()
            },))
                .with_children(|col| {
                    col.spawn((
                        Text::new("Let critters wander between monitors"),
                        TextFont {
                            font_size: 14.0,
                            ..default()
                        },
                        TextColor(palette.card_foreground),
                    ));
                    col.spawn((
                        Text::new("Cross-monitor roaming"),
                        TextFont {
                            font_size: 11.0,
                            ..default()
                        },
                        TextColor(palette.muted_foreground),
                    ));
                });
            spawn_toggle_widget(
                row,
                SettingToggleId::AllowCritterRoaming,
                settings.allow_critter_roaming,
                palette,
            );
        });

    // Per-monitor cards
    for (idx, monitor) in monitors.monitors.iter().enumerate() {
        spawn_monitor_card(
            parent,
            idx,
            monitor.name.as_str(),
            &monitor.resolution,
            monitor.refresh_rate,
            monitor.enabled,
            palette,
        );
    }
}

fn spawn_monitor_card(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    _idx: usize,
    name: &str,
    resolution: &str,
    refresh_rate: u32,
    enabled: bool,
    palette: &Palette,
) {
    parent
        .spawn((
            Node {
                flex_direction: FlexDirection::Column,
                padding: UiRect::all(Val::Px(16.0)),
                border: UiRect::all(Val::Px(1.0)),
                width: Val::Percent(100.0),
                margin: UiRect::bottom(Val::Px(10.0)),
                ..default()
            },
            BorderRadius::all(Val::Px(8.0)),
            BorderColor(palette.border),
            BackgroundColor(palette.card),
        ))
        .with_children(|card| {
            // Monitor header row
            card.spawn((Node {
                flex_direction: FlexDirection::Row,
                align_items: AlignItems::Center,
                justify_content: JustifyContent::SpaceBetween,
                margin: UiRect::bottom(Val::Px(10.0)),
                ..default()
            },))
                .with_children(|header| {
                    header
                        .spawn((Node {
                            flex_direction: FlexDirection::Column,
                            ..default()
                        },))
                        .with_children(|info| {
                            info.spawn((
                                Text::new(name),
                                TextFont {
                                    font_size: 14.0,
                                    ..default()
                                },
                                TextColor(palette.card_foreground),
                            ));
                            let sub = format!("{resolution} · {refresh_rate}Hz");
                            info.spawn((
                                Text::new(sub),
                                TextFont {
                                    font_size: 11.0,
                                    ..default()
                                },
                                TextColor(palette.muted_foreground),
                            ));
                        });
                    // Enable/disable toggle — uses a simple bool display for now.
                    // A per-monitor ToggleWidget variant would be wired in future.
                    let status = if enabled { "Enabled" } else { "Disabled" };
                    let status_color = if enabled {
                        palette.primary
                    } else {
                        palette.muted_foreground
                    };
                    header.spawn((
                        Text::new(status),
                        TextFont {
                            font_size: 12.0,
                            ..default()
                        },
                        TextColor(status_color),
                    ));
                });

            if enabled {
                // Exclusion zones section
                card.spawn((
                    Node {
                        border: UiRect::top(Val::Px(1.0)),
                        padding: UiRect::top(Val::Px(10.0)),
                        flex_direction: FlexDirection::Column,
                        ..default()
                    },
                    BorderColor(palette.border),
                ))
                .with_children(|section| {
                    section.spawn((
                        Text::new("Exclusion Zones"),
                        TextFont {
                            font_size: 12.0,
                            ..default()
                        },
                        TextColor(palette.foreground),
                        Node {
                            margin: UiRect::bottom(Val::Px(6.0)),
                            ..default()
                        },
                    ));
                    section.spawn((
                        Text::new("0 zones configured"),
                        TextFont {
                            font_size: 11.0,
                            ..default()
                        },
                        TextColor(palette.muted_foreground),
                        Node {
                            margin: UiRect::bottom(Val::Px(8.0)),
                            ..default()
                        },
                    ));
                    // Add exclusion zone button (action stubbed — opens zone editor)
                    section
                        .spawn((
                            Node {
                                padding: UiRect::axes(Val::Px(12.0), Val::Px(8.0)),
                                border: UiRect::all(Val::Px(1.0)),
                                align_items: AlignItems::Center,
                                justify_content: JustifyContent::Center,
                                width: Val::Percent(100.0),
                                ..default()
                            },
                            BorderRadius::all(Val::Px(6.0)),
                            BorderColor(palette.border),
                            BackgroundColor(palette.card),
                            Button,
                            // TODO: AddExclusionZoneButton(monitor_idx) — implement with zone editor
                        ))
                        .with_children(|btn| {
                            btn.spawn((
                                Text::new("+ Add exclusion zone"),
                                TextFont {
                                    font_size: 12.0,
                                    ..default()
                                },
                                TextColor(palette.muted_foreground),
                            ));
                        });
                });
            }
        });
}

// ─── Updates section ──────────────────────────────────────────────────────────

fn spawn_updates_section(parent: &mut RelatedSpawnerCommands<'_, ChildOf>, palette: &Palette) {
    spawn_section_header(parent, "Updates", palette);

    parent
        .spawn((
            Node {
                flex_direction: FlexDirection::Column,
                padding: UiRect::all(Val::Px(16.0)),
                border: UiRect::all(Val::Px(1.0)),
                width: Val::Percent(100.0),
                ..default()
            },
            BorderRadius::all(Val::Px(8.0)),
            BorderColor(palette.border),
            BackgroundColor(palette.card),
        ))
        .with_children(|card| {
            card.spawn((Node {
                flex_direction: FlexDirection::Row,
                align_items: AlignItems::Center,
                justify_content: JustifyContent::SpaceBetween,
                margin: UiRect::bottom(Val::Px(8.0)),
                ..default()
            },))
                .with_children(|row| {
                    row.spawn((
                        Text::new("Check now"),
                        TextFont {
                            font_size: 14.0,
                            ..default()
                        },
                        TextColor(palette.card_foreground),
                    ));
                    row.spawn((
                        Node {
                            padding: UiRect::axes(Val::Px(14.0), Val::Px(8.0)),
                            border: UiRect::all(Val::Px(1.0)),
                            align_items: AlignItems::Center,
                            justify_content: JustifyContent::Center,
                            ..default()
                        },
                        BorderRadius::all(Val::Px(6.0)),
                        BorderColor(palette.border),
                        BackgroundColor(palette.card),
                        Button,
                        CheckForUpdatesButton,
                    ))
                    .with_children(|btn| {
                        btn.spawn((
                            Text::new("Check for updates"),
                            TextFont {
                                font_size: 13.0,
                                ..default()
                            },
                            TextColor(palette.card_foreground),
                        ));
                    });
                });
            card.spawn((
                Text::new("Last checked: just now"),
                TextFont {
                    font_size: 11.0,
                    ..default()
                },
                TextColor(palette.muted_foreground),
            ));
        });
}
