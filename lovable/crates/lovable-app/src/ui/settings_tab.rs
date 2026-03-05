use bevy::{ecs::relationship::RelatedSpawnerCommands, prelude::*, window::Monitor};

use crate::{
    preferences::Preferences,
    ui::{
        palette::Palette,
        state::{AppSettingsUiState, Tab},
        widgets::*,
    },
};

pub fn spawn_settings_tab(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    settings: &AppSettingsUiState,
    prefs: &Preferences,
    monitors: &[(&Monitor, u64)],
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
            spawn_monitor_management_section(tab, settings, prefs, monitors, palette);
            spawn_separator(tab, palette);
            spawn_updates_section(tab, palette);
        });
}

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
        SettingId::StartOnBoot,
        settings.start_on_boot,
        palette,
    );
    spawn_toggle_row(
        parent,
        "Go straight to the tray, skip the window",
        "Start minimized",
        SettingId::StartMinimized,
        settings.start_minimized,
        palette,
    );
    spawn_toggle_row(
        parent,
        "Keep an icon in the system tray",
        "Tray visibility",
        SettingId::ShowTrayIcon,
        settings.show_tray_icon,
        palette,
    );
    spawn_toggle_row(
        parent,
        "Closing this window keeps critters running",
        "Close to tray",
        SettingId::CloseToTray,
        settings.close_to_tray,
        palette,
    );
    spawn_toggle_row(
        parent,
        "Check for new versions when Lovable starts",
        "Auto-update checks",
        SettingId::CheckForUpdates,
        settings.check_for_updates,
        palette,
    );
    spawn_toggle_row(
        parent,
        "Share anonymous usage data",
        "Analytics",
        SettingId::SendAnalytics,
        settings.send_analytics,
        palette,
    );
}

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
        SettingId::InteractionsEnabled,
        settings.interactions_enabled,
        palette,
    );
    spawn_toggle_row(
        parent,
        "Show their name when I hover over them",
        "Hover display",
        SettingId::ShowNameOnHover,
        settings.show_name_on_hover,
        palette,
    );
    spawn_toggle_row(
        parent,
        "Let critters make sounds",
        "Audio control",
        SettingId::SoundEnabled,
        settings.sound_enabled,
        palette,
    );

    if settings.sound_enabled {
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

fn spawn_monitor_management_section(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    settings: &AppSettingsUiState,
    prefs: &Preferences,
    monitors: &[(&Monitor, u64)],
    palette: &Palette,
) {
    spawn_section_header(parent, "Monitor Management", palette);

    // Cross-monitor roaming
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
                SettingId::AllowCritterRoaming,
                settings.allow_critter_roaming,
                palette,
            );
        });

    // Per-monitor cards — derived from live Monitor query + MonitorSettings from Preferences
    for (monitor, fp) in monitors {
        let monitor_settings = prefs.monitors.iter().find(|m| m.fingerprint == *fp);
        let enabled = monitor_settings.map(|s| s.enabled).unwrap_or(true);
        let exclusion_count = monitor_settings
            .map(|s| s.exclusion_zones.len())
            .unwrap_or(0);

        let name = monitor
            .name
            .clone()
            .unwrap_or_else(|| format!("Monitor {:x}", fp));
        let resolution = format!("{}x{}", monitor.physical_width, monitor.physical_height);
        let refresh_hz = monitor
            .refresh_rate_millihertz
            .map(|r| r / 1000)
            .unwrap_or(60);

        spawn_monitor_card(
            parent,
            &name,
            &resolution,
            refresh_hz,
            enabled,
            exclusion_count,
            palette,
        );
    }
}

fn spawn_monitor_card(
    parent: &mut RelatedSpawnerCommands<'_, ChildOf>,
    name: &str,
    resolution: &str,
    refresh_rate: u32,
    enabled: bool,
    exclusion_count: usize,
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
                            info.spawn((
                                Text::new(format!("{resolution} · {refresh_rate}Hz")),
                                TextFont {
                                    font_size: 11.0,
                                    ..default()
                                },
                                TextColor(palette.muted_foreground),
                            ));
                        });
                    let (status, status_color) = if enabled {
                        ("Enabled", palette.primary)
                    } else {
                        ("Disabled", palette.muted_foreground)
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
                        Text::new(format!("{exclusion_count} zone(s) configured")),
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
