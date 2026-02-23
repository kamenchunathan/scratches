use bevy::{prelude::*, window::Monitor};

use crate::{
    critter::{Critter, CritterId, CritterRegistry},
    preferences::{Preferences, PreferencesHandle, RegistryHandle, generate_monitor_fingerprint},
    ui::{
        messages::{CrittersMsg, HomeMsg, Msg, OnboardingMsg, SettingsMsg},
        palette::Palette,
        state::{
            AppScreen, AppSettingsUiState, CrittersTabState, DeleteConfirmState, MainState,
            OnboardingState, Tab,
        },
        widgets::*,
    },
};

/// Single system that processes every `Msg` event, mirroring the Elm `update`
/// function. Mutates `AppScreen`, `AppSettingsUiState`, and `Preferences` only
/// here; all other systems are read-only consumers of those resources.
pub fn update(
    mut events: EventReader<Msg>,
    mut screen: ResMut<AppScreen>,
    mut settings: ResMut<AppSettingsUiState>,
    prefs_handle: Res<PreferencesHandle>,
    mut prefs_store: ResMut<Assets<Preferences>>,
    registry_handle: Res<RegistryHandle>,
    registry_store: Res<Assets<CritterRegistry>>,
) {
    let Some(prefs) = prefs_store.get_mut(&prefs_handle.0) else {
        return;
    };

    for msg in events.read() {
        match msg {
            Msg::Home(m) => handle_home(m, prefs),
            Msg::Critters(m) => {
                handle_critters(m, &mut screen, prefs, &registry_store, &registry_handle)
            }
            Msg::Settings(m) => handle_settings(m, &mut settings, prefs),
            Msg::Onboarding(m) => handle_onboarding(m, &mut screen, prefs),
        }
    }
}

fn handle_home(msg: &HomeMsg, prefs: &mut Preferences) {
    match msg {
        HomeMsg::HideAll => prefs.critters.iter_mut().for_each(|c| c.is_visible = false),
        HomeMsg::ShowAll => prefs.critters.iter_mut().for_each(|c| c.is_visible = true),
    }
}

fn handle_critters(
    msg: &CrittersMsg,
    screen: &mut AppScreen,
    prefs: &mut Preferences,
    registry_store: &Assets<CritterRegistry>,
    registry_handle: &RegistryHandle,
) {
    match msg {
        CrittersMsg::ToggleExpand(id) => {
            if let AppScreen::Main(main) = screen {
                main.critters_tab = match &main.critters_tab {
                    CrittersTabState::Expanded { critter_id, .. } if critter_id == id => {
                        CrittersTabState::Collapsed
                    }
                    _ => CrittersTabState::Expanded {
                        critter_id: *id,
                        confirm: DeleteConfirmState::None,
                    },
                };
            }
        }

        CrittersMsg::ToggleVisibility(id) => {
            if let Some(c) = prefs.critters.iter_mut().find(|c| &c.id == id) {
                c.is_visible = !c.is_visible;
            }
        }

        CrittersMsg::RequestDelete(id) => {
            if let AppScreen::Main(main) = screen {
                if let CrittersTabState::Expanded {
                    critter_id,
                    confirm,
                } = &mut main.critters_tab
                {
                    if critter_id == id {
                        *confirm = match confirm {
                            DeleteConfirmState::None => DeleteConfirmState::Pending,
                            DeleteConfirmState::Pending => DeleteConfirmState::None,
                        };
                    }
                }
            }
        }

        CrittersMsg::ConfirmDelete(id) => {
            prefs.critters.retain(|c| &c.id != id);
            if let AppScreen::Main(main) = screen {
                main.critters_tab = CrittersTabState::Collapsed;
            }
        }

        CrittersMsg::CancelDelete => {
            if let AppScreen::Main(main) = screen {
                if let CrittersTabState::Expanded { confirm, .. } = &mut main.critters_tab {
                    *confirm = DeleteConfirmState::None;
                }
            }
        }

        CrittersMsg::Adopt(def_id) => {
            let already_owned = prefs.critters.iter().any(|c| &c.def_id == def_id);
            if already_owned {
                return;
            }
            let Some(registry) = registry_store.get(&registry_handle.0) else {
                return;
            };
            let Some(def) = registry.find(def_id) else {
                return;
            };
            prefs.critters.push(Critter {
                id: CritterId::new(),
                def_id: def.id.clone(),
                name: def.name.clone(),
                position: bevy::math::Vec2::ZERO,
                scale: 1.0,
                opacity: 1.0,
                is_visible: true,
                monitor_fingerprint: prefs.monitors.first().map(|m| m.fingerprint).unwrap_or(0),
                interactible: true,
            });
        }

        CrittersMsg::AssignMonitor {
            critter,
            monitor_fingerprint,
        } => {
            if let Some(c) = prefs.critters.iter_mut().find(|c| &c.id == critter) {
                c.monitor_fingerprint = *monitor_fingerprint;
            }
        }
    }
}

fn handle_settings(msg: &SettingsMsg, settings: &mut AppSettingsUiState, prefs: &mut Preferences) {
    match msg {
        SettingsMsg::SetStartOnBoot(v) => {
            settings.start_on_boot = *v;
            prefs.app_settings.start_on_boot = *v;
        }
        SettingsMsg::SetStartMinimized(v) => {
            settings.start_minimized = *v;
            prefs.app_settings.start_minimized = *v;
        }
        SettingsMsg::SetShowTrayIcon(v) => {
            settings.show_tray_icon = *v;
            prefs.app_settings.show_tray_icon = *v;
        }
        SettingsMsg::SetCloseToTray(v) => {
            settings.close_to_tray = *v;
            prefs.app_settings.close_to_tray = *v;
        }
        SettingsMsg::SetCheckForUpdates(v) => {
            settings.check_for_updates = *v;
            prefs.app_settings.check_updates = *v;
        }
        SettingsMsg::SetSendAnalytics(v) => {
            settings.send_analytics = *v;
            prefs.app_settings.send_analytics = *v;
        }
        SettingsMsg::SetInteractionsEnabled(v) => {
            settings.interactions_enabled = *v;
            prefs.global_critter_settings.interactions_enabled = *v;
        }
        SettingsMsg::SetShowNameOnHover(v) => {
            settings.show_name_on_hover = *v;
            prefs.global_critter_settings.display_critter_name_on_hover = *v;
        }
        SettingsMsg::SetSoundEnabled(v) => {
            settings.sound_enabled = *v;
            prefs.global_critter_settings.sound_enabled = *v;
        }
        SettingsMsg::AllowCritterRoaming(_) => {
            // TODO: Add setting to preferences
            error!("Unimplemented setting")
        }
    }
}

fn handle_onboarding(msg: &OnboardingMsg, screen: &mut AppScreen, prefs: &mut Preferences) {
    let AppScreen::Onboarding(state) = screen else {
        return;
    };

    match (state, msg) {
        (OnboardingState::Welcome, OnboardingMsg::Next) => {
            *screen = AppScreen::Onboarding(OnboardingState::PickCritter {
                selected_def_id: String::new(),
            });
        }
        (OnboardingState::PickCritter { selected_def_id }, OnboardingMsg::SelectCritter(id)) => {
            *selected_def_id = id.clone();
        }
        (OnboardingState::PickCritter { selected_def_id }, OnboardingMsg::Next) => {
            *screen = AppScreen::Onboarding(OnboardingState::ChooseMonitor {
                selected_def_id: selected_def_id.clone(),
                selected_monitor: None,
            });
        }
        (
            OnboardingState::ChooseMonitor {
                selected_monitor, ..
            },
            OnboardingMsg::SelectMonitor(fp),
        ) => {
            *selected_monitor = Some(*fp);
        }
        (OnboardingState::ChooseMonitor { .. }, OnboardingMsg::Next) => {
            *screen = AppScreen::Onboarding(OnboardingState::InfoScreen);
        }
        (OnboardingState::InfoScreen, OnboardingMsg::Next) => {
            prefs.first_start = false;
            *screen = AppScreen::Main(MainState::default());
        }
        (_, OnboardingMsg::Skip) => {
            prefs.first_start = false;
            *screen = AppScreen::Main(MainState::default());
        }
        _ => {}
    }
}

/// Reads all Bevy `Interaction` components and translates them into `Msg` events.
pub fn read_inp_and_dispatch_msg(
    mut writer: EventWriter<Msg>,
    tab_buttons: Query<(&Interaction, &TabButton), (Changed<Interaction>, With<Button>)>,
    toggle_buttons: Query<(&Interaction, &ToggleWidget), (Changed<Interaction>, With<Button>)>,
    adopt_buttons: Query<(&Interaction, &AdoptButton), (Changed<Interaction>, With<Button>)>,
    expand_buttons: Query<
        (&Interaction, &CritterExpandButton),
        (Changed<Interaction>, With<Button>),
    >,
    vis_buttons: Query<
        (&Interaction, &CritterVisibilityButton),
        (Changed<Interaction>, With<Button>),
    >,
    delete_buttons: Query<
        (&Interaction, &CritterDeleteButton),
        (Changed<Interaction>, With<Button>),
    >,
    confirm_buttons: Query<
        (&Interaction, &DeleteConfirmButton),
        (Changed<Interaction>, With<Button>),
    >,
    cancel_buttons: Query<
        (&Interaction, &DeleteCancelButton),
        (Changed<Interaction>, With<Button>),
    >,
    monitor_assign_buttons: Query<
        (&Interaction, &MonitorAssignButton),
        (Changed<Interaction>, With<Button>),
    >,
    hide_all: Query<&Interaction, (Changed<Interaction>, With<Button>, With<HideAllButton>)>,
    show_all: Query<&Interaction, (Changed<Interaction>, With<Button>, With<ShowAllButton>)>,
    onboarding_primary: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<OnboardingPrimaryButton>,
        ),
    >,
    onboarding_skip: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<OnboardingSkipButton>,
        ),
    >,
    onboarding_critter_cards: Query<
        (&Interaction, &OnboardingCritterCard),
        (Changed<Interaction>, With<Button>),
    >,
    onboarding_monitor_cards: Query<
        (&Interaction, &OnboardingMonitorCard),
        (Changed<Interaction>, With<Button>),
    >,
) {
    for (i, btn) in &tab_buttons {
        if *i == Interaction::Pressed {
            // Tab switching is handled directly in the render sync system via TabButton marker.
            // Emit as a Critters no-op here; tab state lives in MainState.active_tab which
            // the render sync reads. We handle this specially below via AppScreen mutation.
            // For now route through a dedicated message — add TabMsg if needed.
        }
    }

    for (i, btn) in &toggle_buttons {
        if *i == Interaction::Pressed {
            let msg = match &btn.id {
                SettingId::StartOnBoot => SettingsMsg::SetStartOnBoot(!btn.is_on),
                SettingId::StartMinimized => SettingsMsg::SetStartMinimized(!btn.is_on),
                SettingId::ShowTrayIcon => SettingsMsg::SetShowTrayIcon(!btn.is_on),
                SettingId::CloseToTray => SettingsMsg::SetCloseToTray(!btn.is_on),
                SettingId::CheckForUpdates => SettingsMsg::SetCheckForUpdates(!btn.is_on),
                SettingId::SendAnalytics => SettingsMsg::SetSendAnalytics(!btn.is_on),
                SettingId::InteractionsEnabled => SettingsMsg::SetInteractionsEnabled(!btn.is_on),
                SettingId::ShowNameOnHover => SettingsMsg::SetShowNameOnHover(!btn.is_on),
                SettingId::SoundEnabled => SettingsMsg::SetSoundEnabled(!btn.is_on),
                SettingId::AllowCritterRoaming => SettingsMsg::AllowCritterRoaming(!btn.is_on),
            };
            writer.send(Msg::Settings(msg));
        }
    }

    for (i, btn) in &adopt_buttons {
        if *i == Interaction::Pressed {
            writer.send(Msg::Critters(CrittersMsg::Adopt(btn.def_id.clone())));
        }
    }

    for (i, btn) in &expand_buttons {
        if *i == Interaction::Pressed {
            writer.send(Msg::Critters(CrittersMsg::ToggleExpand(btn.critter_id)));
        }
    }

    for (i, btn) in &vis_buttons {
        if *i == Interaction::Pressed {
            writer.send(Msg::Critters(CrittersMsg::ToggleVisibility(btn.critter_id)));
        }
    }

    for (i, btn) in &delete_buttons {
        if *i == Interaction::Pressed {
            writer.send(Msg::Critters(CrittersMsg::RequestDelete(btn.critter_id)));
        }
    }

    for (i, btn) in &confirm_buttons {
        if *i == Interaction::Pressed {
            writer.send(Msg::Critters(CrittersMsg::ConfirmDelete(btn.critter_id)));
        }
    }

    for (i, _) in &cancel_buttons {
        if *i == Interaction::Pressed {
            writer.send(Msg::Critters(CrittersMsg::CancelDelete));
        }
    }

    for (i, btn) in &monitor_assign_buttons {
        if *i == Interaction::Pressed {
            writer.send(Msg::Critters(CrittersMsg::AssignMonitor {
                critter: btn.critter_id,
                monitor_fingerprint: btn.monitor_fingerprint,
            }));
        }
    }

    for i in &hide_all {
        if *i == Interaction::Pressed {
            writer.send(Msg::Home(HomeMsg::HideAll));
        }
    }

    for i in &show_all {
        if *i == Interaction::Pressed {
            writer.send(Msg::Home(HomeMsg::ShowAll));
        }
    }

    for i in &onboarding_primary {
        if *i == Interaction::Pressed {
            writer.send(Msg::Onboarding(OnboardingMsg::Next));
        }
    }

    for i in &onboarding_skip {
        if *i == Interaction::Pressed {
            writer.send(Msg::Onboarding(OnboardingMsg::Skip));
        }
    }

    for (i, card) in &onboarding_critter_cards {
        if *i == Interaction::Pressed {
            writer.send(Msg::Onboarding(OnboardingMsg::SelectCritter(
                card.def_id.clone(),
            )));
        }
    }

    for (i, card) in &onboarding_monitor_cards {
        if *i == Interaction::Pressed {
            writer.send(Msg::Onboarding(OnboardingMsg::SelectMonitor(
                card.monitor_fingerprint,
            )));
        }
    }
}

// ─── Render sync systems ──────────────────────────────────────────────────────

/// Syncs tab content visibility to `AppScreen::Main::active_tab`.
pub fn sync_tab_visibility(screen: Res<AppScreen>, mut query: Query<(&mut Node, &TabContent)>) {
    if !screen.is_changed() {
        return;
    }
    let active = match screen.as_ref() {
        AppScreen::Main(m) => &m.active_tab,
        _ => return,
    };
    for (mut node, content) in &mut query {
        node.display = if &content.0 == active {
            Display::Flex
        } else {
            Display::None
        };
    }
}

/// Syncs onboarding / main root visibility.
pub fn sync_onboarding_main_visibility(
    screen: Res<AppScreen>,
    mut onboarding: Query<&mut Node, (With<OnboardingRoot>, Without<MainUiRoot>)>,
    mut main: Query<&mut Node, (With<MainUiRoot>, Without<OnboardingRoot>)>,
) {
    if !screen.is_changed() {
        return;
    }
    let is_onboarding = matches!(screen.as_ref(), AppScreen::Onboarding(_));
    if let Ok(mut node) = onboarding.single_mut() {
        node.display = if is_onboarding {
            Display::Flex
        } else {
            Display::None
        };
    }
    if let Ok(mut node) = main.single_mut() {
        node.display = if is_onboarding {
            Display::None
        } else {
            Display::Flex
        };
    }
}

/// Syncs which onboarding screen panel is visible.
pub fn sync_onboarding_step(
    screen: Res<AppScreen>,
    mut query: Query<(&mut Node, &OnboardingScreen)>,
) {
    if !screen.is_changed() {
        return;
    }
    let active: u8 = match screen.as_ref() {
        AppScreen::Onboarding(OnboardingState::Welcome) => 0,
        AppScreen::Onboarding(OnboardingState::PickCritter { .. }) => 1,
        AppScreen::Onboarding(OnboardingState::ChooseMonitor { .. }) => 2,
        AppScreen::Onboarding(OnboardingState::InfoScreen) => 3,
        _ => return,
    };
    for (mut node, screen_marker) in &mut query {
        node.display = if screen_marker.0 == active {
            Display::Flex
        } else {
            Display::None
        };
    }
}

/// Syncs critter row expand/collapse and delete confirm panel visibility.
pub fn sync_critter_expand(
    screen: Res<AppScreen>,
    mut expand_query: Query<(&mut Node, &CritterExpandContent)>,
    mut confirm_query: Query<(&mut Node, &DeleteConfirmPanel), Without<CritterExpandContent>>,
) {
    if !screen.is_changed() {
        return;
    }
    let AppScreen::Main(main) = screen.as_ref() else {
        return;
    };

    let (expanded_id, confirm_pending) = match &main.critters_tab {
        CrittersTabState::Expanded {
            critter_id,
            confirm,
        } => (Some(*critter_id), *confirm == DeleteConfirmState::Pending),
        CrittersTabState::Collapsed => (None, false),
    };

    for (mut node, content) in &mut expand_query {
        node.display = if Some(content.critter_id) == expanded_id {
            Display::Flex
        } else {
            Display::None
        };
    }

    for (mut node, panel) in &mut confirm_query {
        node.display = if Some(panel.0) == expanded_id && confirm_pending {
            Display::Flex
        } else {
            Display::None
        };
    }
}

/// Syncs toggle widget visuals after settings change.
pub fn sync_toggle_visuals(
    screen: Res<AppScreen>,
    settings: Res<AppSettingsUiState>,
    mut query: Query<(&mut BackgroundColor, &mut Node, &ToggleWidget)>,
    palette: Res<Palette>,
) {
    if !settings.is_changed() {
        return;
    }
    for (mut bg, mut node, toggle) in &mut query {
        let is_on = match toggle.id {
            SettingId::StartOnBoot => settings.start_on_boot,
            SettingId::StartMinimized => settings.start_minimized,
            SettingId::ShowTrayIcon => settings.show_tray_icon,
            SettingId::CloseToTray => settings.close_to_tray,
            SettingId::CheckForUpdates => settings.check_for_updates,
            SettingId::SendAnalytics => settings.send_analytics,
            SettingId::InteractionsEnabled => settings.interactions_enabled,
            SettingId::ShowNameOnHover => settings.show_name_on_hover,
            SettingId::SoundEnabled => settings.sound_enabled,
            SettingId::AllowCritterRoaming => settings.allow_critter_roaming,
        };
        *bg = if is_on {
            BackgroundColor(palette.primary)
        } else {
            BackgroundColor(palette.muted)
        };
        node.justify_content = if is_on {
            JustifyContent::FlexEnd
        } else {
            JustifyContent::FlexStart
        };
    }
}

/// Syncs onboarding critter card border/background to the selected def.
pub fn sync_onboarding_critter_cards(
    screen: Res<AppScreen>,
    mut query: Query<(
        &OnboardingCritterCard,
        &mut BorderColor,
        &mut BackgroundColor,
    )>,
    palette: Res<Palette>,
) {
    if !screen.is_changed() {
        return;
    }
    let selected = match screen.as_ref() {
        AppScreen::Onboarding(OnboardingState::PickCritter { selected_def_id }) => {
            selected_def_id.as_str()
        }
        _ => return,
    };
    for (card, mut border, mut bg) in &mut query {
        if card.def_id == selected {
            *border = BorderColor(palette.primary);
            *bg = BackgroundColor(palette.accent);
        } else {
            *border = BorderColor(palette.border);
            *bg = BackgroundColor(palette.card);
        }
    }
}

/// Syncs tab button hover/active visuals.
pub fn sync_tab_button_visuals(
    screen: Res<AppScreen>,
    mut query: Query<(&Interaction, &TabButton, &mut BackgroundColor), Changed<Interaction>>,
    palette: Res<Palette>,
    mut writer: EventWriter<Msg>,
) {
    let active = match screen.as_ref() {
        AppScreen::Main(m) => Some(&m.active_tab),
        _ => None,
    };

    for (interaction, btn, mut bg) in &mut query {
        match interaction {
            Interaction::Pressed => {
                // Directly mutate AppScreen for tab switches (not routed through Msg
                // to avoid the round-trip; tabs are pure navigation with no persistence).
                // Handled in a separate system below.
            }
            Interaction::Hovered => *bg = BackgroundColor(palette.accent),
            Interaction::None => *bg = BackgroundColor(palette.card),
        }
    }
}

/// Handles tab button presses directly — tab selection is navigation-only and
/// does not need to go through the `Msg` event loop.
pub fn handle_tab_press(
    query: Query<(&Interaction, &TabButton), (Changed<Interaction>, With<Button>)>,
    mut screen: ResMut<AppScreen>,
) {
    for (interaction, btn) in &query {
        if *interaction == Interaction::Pressed {
            if let AppScreen::Main(main) = screen.as_mut() {
                main.active_tab = btn.0.clone();
            }
        }
    }
}
