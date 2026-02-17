use bevy::prelude::*;

use crate::{
    preferences::{Preferences, PreferencesHandle},
    ui::{
        events::*,
        palette::Palette,
        state::{AppSettingsUiState, CritterRoster, OnboardingStep, UiState},
        widgets::*,
    },
};

// ─── Tab navigation ───────────────────────────────────────────────────────────

pub fn handle_tab_buttons(
    mut query: Query<
        (&Interaction, &TabButton, &mut BackgroundColor),
        (Changed<Interaction>, With<Button>),
    >,
    mut ui_state: ResMut<UiState>,
    palette: Res<Palette>,
) {
    for (interaction, tab_button, mut bg) in &mut query {
        match *interaction {
            Interaction::Pressed => {
                ui_state.active_tab = tab_button.0.clone();
            }
            Interaction::Hovered => {
                *bg = BackgroundColor(palette.accent);
            }
            Interaction::None => {
                *bg = BackgroundColor(palette.card);
            }
        }
    }
}

pub fn update_tab_visibility(ui_state: Res<UiState>, mut query: Query<(&mut Node, &TabContent)>) {
    if !ui_state.is_changed() {
        return;
    }
    for (mut node, content) in &mut query {
        node.display = if content.0 == ui_state.active_tab {
            Display::Flex
        } else {
            Display::None
        };
    }
}

// ─── Onboarding visibility ────────────────────────────────────────────────────

/// Shows either the onboarding root or the main UI root based on completion state.
pub fn update_onboarding_main_visibility(
    ui_state: Res<UiState>,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    mut onboarding_query: Query<&mut Node, (With<OnboardingRoot>, Without<MainUiRoot>)>,
    mut main_query: Query<&mut Node, (With<MainUiRoot>, Without<OnboardingRoot>)>,
) {
    if !ui_state.is_changed() {
        return;
    }

    let Some(prefs) = prefs_store.get(&prefs_handle.0) else {
        return;
    };

    let show_onboarding = prefs.first_start && !ui_state.onboarding_complete;

    if let Ok(mut node) = onboarding_query.single_mut() {
        node.display = if show_onboarding {
            Display::Flex
        } else {
            Display::None
        };
    }
    if let Ok(mut node) = main_query.single_mut() {
        node.display = if show_onboarding {
            Display::None
        } else {
            Display::Flex
        };
    }
}

pub fn update_onboarding_step_visibility(
    ui_state: Res<UiState>,
    mut query: Query<(&mut Node, &OnboardingScreen)>,
) {
    if !ui_state.is_changed() {
        return;
    }

    let active = match ui_state.onboarding_step {
        OnboardingStep::Welcome => 0,
        OnboardingStep::PickCritter => 1,
        OnboardingStep::ChooseMonitor => 2,
        OnboardingStep::InfoScreen => 3,
    };

    for (mut node, screen) in &mut query {
        node.display = if screen.0 == active {
            Display::Flex
        } else {
            Display::None
        };
    }
}

// ─── Onboarding navigation ────────────────────────────────────────────────────

pub fn handle_onboarding_primary(
    query: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<OnboardingPrimaryButton>,
        ),
    >,
    mut ui_state: ResMut<UiState>,
    mut events: EventWriter<OnboardingComplete>,
) {
    for interaction in &query {
        if *interaction != Interaction::Pressed {
            continue;
        }

        match ui_state.onboarding_step {
            OnboardingStep::Welcome => ui_state.onboarding_step = OnboardingStep::PickCritter,
            OnboardingStep::PickCritter => ui_state.onboarding_step = OnboardingStep::ChooseMonitor,
            OnboardingStep::ChooseMonitor => ui_state.onboarding_step = OnboardingStep::InfoScreen,
            OnboardingStep::InfoScreen => {
                ui_state.onboarding_complete = true;
                events.send(OnboardingComplete);
            }
        }
    }
}

pub fn handle_onboarding_skip(
    query: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<OnboardingSkipButton>,
        ),
    >,
    mut ui_state: ResMut<UiState>,
    mut events: EventWriter<OnboardingComplete>,
) {
    for interaction in &query {
        if *interaction == Interaction::Pressed {
            ui_state.onboarding_complete = true;
            events.send(OnboardingComplete);
        }
    }
}

/// Updates border/background on onboarding critter cards to reflect selection.
pub fn handle_onboarding_critter_selection(
    query: Query<(&Interaction, &OnboardingCritterCard), (Changed<Interaction>, With<Button>)>,
    mut ui_state: ResMut<UiState>,
) {
    for (interaction, card) in &query {
        if *interaction == Interaction::Pressed {
            ui_state.selected_onboarding_critter = card.0;
        }
    }
}

pub fn update_onboarding_critter_card_visuals(
    ui_state: Res<UiState>,
    mut query: Query<(
        &OnboardingCritterCard,
        &mut BorderColor,
        &mut BackgroundColor,
    )>,
    palette: Res<Palette>,
) {
    if !ui_state.is_changed() {
        return;
    }
    for (card, mut border, mut bg) in &mut query {
        if card.0 == ui_state.selected_onboarding_critter {
            *border = BorderColor(palette.primary);
            *bg = BackgroundColor(palette.accent);
        } else {
            *border = BorderColor(palette.border);
            *bg = BackgroundColor(palette.card);
        }
    }
}

// ─── Settings toggles ─────────────────────────────────────────────────────────

/// Handles clicks on ToggleWidget buttons, flipping their visual state and updating
/// AppSettingsUiState. Sends a SettingChanged event for the backend to persist.
pub fn handle_toggle_clicks(
    mut query: Query<
        (
            &Interaction,
            &mut ToggleWidget,
            &mut BackgroundColor,
            &mut Node,
        ),
        (Changed<Interaction>, With<Button>),
    >,
    mut settings: ResMut<AppSettingsUiState>,
    mut events: EventWriter<SettingChanged>,
    palette: Res<Palette>,
) {
    for (interaction, mut toggle, mut bg, mut node) in &mut query {
        if *interaction != Interaction::Pressed {
            continue;
        }

        toggle.is_on = !toggle.is_on;
        let is_on = toggle.is_on;

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

        match &toggle.id {
            SettingToggleId::StartOnBoot => settings.start_on_boot = is_on,
            SettingToggleId::StartMinimized => settings.start_minimized = is_on,
            SettingToggleId::ShowTrayIcon => settings.show_tray_icon = is_on,
            SettingToggleId::CloseToTray => settings.close_to_tray = is_on,
            SettingToggleId::CheckForUpdates => settings.check_for_updates = is_on,
            SettingToggleId::SendAnalytics => settings.send_analytics = is_on,
            SettingToggleId::InteractionsEnabled => settings.interactions_enabled = is_on,
            SettingToggleId::ShowNameOnHover => settings.show_name_on_hover = is_on,
            SettingToggleId::SoundEnabled => settings.sound_enabled = is_on,
            SettingToggleId::AllowCritterRoaming => settings.allow_critter_roaming = is_on,
        }

        events.send(SettingChanged(toggle.id.clone(), is_on));
    }
}

// ─── Critter expand / collapse ────────────────────────────────────────────────

pub fn handle_critter_header_click(
    query: Query<(&Interaction, &CritterHeaderButton), (Changed<Interaction>, With<Button>)>,
    mut ui_state: ResMut<UiState>,
) {
    for (interaction, btn) in &query {
        if *interaction == Interaction::Pressed {
            if ui_state.expanded_critter_idx == Some(btn.0) {
                ui_state.expanded_critter_idx = None;
            } else {
                ui_state.expanded_critter_idx = Some(btn.0);
                // Collapse any open delete confirmation when switching rows.
                ui_state.delete_confirm_idx = None;
            }
        }
    }
}

pub fn update_critter_expand_visibility(
    ui_state: Res<UiState>,
    mut query: Query<(&mut Node, &CritterExpandContent)>,
) {
    if !ui_state.is_changed() {
        return;
    }
    for (mut node, content) in &mut query {
        node.display = if ui_state.expanded_critter_idx == Some(content.0) {
            Display::Flex
        } else {
            Display::None
        };
    }
}

// ─── Delete confirmation flow ─────────────────────────────────────────────────

pub fn handle_delete_button(
    query: Query<(&Interaction, &CritterDeleteButton), (Changed<Interaction>, With<Button>)>,
    mut ui_state: ResMut<UiState>,
) {
    for (interaction, btn) in &query {
        if *interaction == Interaction::Pressed {
            // Toggle: pressing again dismisses confirmation.
            if ui_state.delete_confirm_idx == Some(btn.0) {
                ui_state.delete_confirm_idx = None;
            } else {
                ui_state.delete_confirm_idx = Some(btn.0);
            }
        }
    }
}

pub fn handle_delete_confirm(
    query: Query<(&Interaction, &DeleteConfirmButton), (Changed<Interaction>, With<Button>)>,
    mut ui_state: ResMut<UiState>,
    mut roster: ResMut<CritterRoster>,
    mut events: EventWriter<DeleteCritterConfirmed>,
) {
    for (interaction, btn) in &query {
        if *interaction == Interaction::Pressed {
            let idx = btn.0;
            if idx < roster.owned.len() {
                roster.owned.remove(idx);
                events.send(DeleteCritterConfirmed(idx));
            }
            ui_state.delete_confirm_idx = None;
            ui_state.expanded_critter_idx = None;
            // TODO: rebuild the critters tab to reflect the removed row.
        }
    }
}

pub fn handle_delete_cancel(
    query: Query<(&Interaction, &DeleteCancelButton), (Changed<Interaction>, With<Button>)>,
    mut ui_state: ResMut<UiState>,
) {
    for (interaction, _) in &query {
        if *interaction == Interaction::Pressed {
            ui_state.delete_confirm_idx = None;
        }
    }
}

pub fn update_delete_confirm_visibility(
    ui_state: Res<UiState>,
    mut query: Query<(&mut Node, &DeleteConfirmPanel)>,
) {
    if !ui_state.is_changed() {
        return;
    }
    for (mut node, panel) in &mut query {
        node.display = if ui_state.delete_confirm_idx == Some(panel.0) {
            Display::Flex
        } else {
            Display::None
        };
    }
}

// ─── Critter visibility toggle ────────────────────────────────────────────────

pub fn handle_critter_visibility_toggle(
    query: Query<(&Interaction, &CritterVisibilityButton), (Changed<Interaction>, With<Button>)>,
    mut roster: ResMut<CritterRoster>,
    mut events: EventWriter<ToggleCritterVisibility>,
) {
    for (interaction, btn) in &query {
        if *interaction == Interaction::Pressed {
            let idx = btn.0;
            if let Some(critter) = roster.owned.get_mut(idx) {
                critter.is_visible = !critter.is_visible;
                events.send(ToggleCritterVisibility(idx));
                // TODO: update the visibility button label and indicator dot.
            }
        }
    }
}

// ─── Adopt critter ────────────────────────────────────────────────────────────

pub fn handle_adopt_button(
    query: Query<(&Interaction, &CritterAdoptButton), (Changed<Interaction>, With<Button>)>,
    mut roster: ResMut<CritterRoster>,
    mut events: EventWriter<AdoptCritterRequested>,
) {
    use crate::ui::state::{CRITTER_TEMPLATES, OwnedCritter};

    for (interaction, btn) in &query {
        if *interaction != Interaction::Pressed {
            continue;
        }

        let already_owned = roster.owned.iter().any(|o| o.template_id == btn.0);
        if already_owned {
            continue;
        }

        if let Some(template) = CRITTER_TEMPLATES.iter().find(|t| t.id == btn.0) {
            roster.owned.push(OwnedCritter {
                template_id: template.id,
                name: template.name.to_string(),
                is_visible: true,
                scale: 100,
                opacity: 100,
                assigned_monitor_idx: 0,
            });
            events.send(AdoptCritterRequested(template.id));
            // TODO: rebuild the critters tab to show the newly adopted critter.
        }
    }
}

// ─── Quick actions (Home tab) ─────────────────────────────────────────────────

pub fn handle_hide_all(
    query: Query<&Interaction, (Changed<Interaction>, With<Button>, With<HideAllButton>)>,
    mut roster: ResMut<CritterRoster>,
    mut events: EventWriter<HideAllCritters>,
) {
    for interaction in &query {
        if *interaction == Interaction::Pressed {
            for critter in &mut roster.owned {
                critter.is_visible = false;
            }
            events.send(HideAllCritters);
        }
    }
}

pub fn handle_show_all(
    query: Query<&Interaction, (Changed<Interaction>, With<Button>, With<ShowAllButton>)>,
    mut roster: ResMut<CritterRoster>,
    mut events: EventWriter<ShowAllCritters>,
) {
    for interaction in &query {
        if *interaction == Interaction::Pressed {
            for critter in &mut roster.owned {
                critter.is_visible = true;
            }
            events.send(ShowAllCritters);
        }
    }
}

// ─── Updates section ──────────────────────────────────────────────────────────

pub fn handle_check_for_updates(
    query: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<CheckForUpdatesButton>,
        ),
    >,
    mut events: EventWriter<CheckForUpdatesRequested>,
) {
    for interaction in &query {
        if *interaction == Interaction::Pressed {
            events.send(CheckForUpdatesRequested);
        }
    }
}
