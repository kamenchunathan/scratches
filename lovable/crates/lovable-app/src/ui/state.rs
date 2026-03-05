use bevy::prelude::*;

use crate::critter::CritterId;

///  UI state resource. Constructed from `Preferences` in the `OnEnter(AppState::Running)` setup
#[derive(Resource)]
pub enum AppScreen {
    Onboarding(OnboardingState),
    Main(MainState),
}

impl AppScreen {
    pub fn from_preferences(prefs: &crate::preferences::Preferences) -> Self {
        if prefs.auto_launch {
            AppScreen::Onboarding(OnboardingState::Welcome)
        } else {
            AppScreen::Main(MainState::default())
        }
    }
}

pub enum OnboardingState {
    Welcome,
    PickCritter {
        selected_def_id: String,
    },
    ChooseMonitor {
        selected_def_id: String,
        selected_monitor: Option<u64>,
    },
    InfoScreen {
        selected_def_id: String,
        selected_monitor: Option<u64>,
    },
}

#[derive(Default)]
pub struct MainState {
    pub active_tab: Tab,
    pub critters_tab: CrittersTabState,
}

#[derive(Clone, PartialEq, Eq, Default, Debug)]
pub enum Tab {
    #[default]
    Home,
    Critters,
    Settings,
    About,
}

#[derive(Default)]
pub enum CrittersTabState {
    #[default]
    Collapsed,
    Expanded {
        critter_id: CritterId,
        confirm: DeleteConfirmState,
    },
}

#[derive(Default, PartialEq, Eq)]
pub enum DeleteConfirmState {
    #[default]
    None,
    Pending,
}

/// A flat mirror of the settings stored in `Preferences`, held as a `Resource`
/// so that UI systems can read it without going through the asset store.
/// Initialised from `Preferences` on `OnEnter(Running)`; kept in sync via
/// `SettingsMsg` handling in the central `update` system.
#[derive(Resource, Default)]
pub struct AppSettingsUiState {
    pub start_on_boot: bool,
    pub start_minimized: bool,
    pub show_tray_icon: bool,
    pub close_to_tray: bool,
    pub check_for_updates: bool,
    pub send_analytics: bool,
    pub interactions_enabled: bool,
    pub show_name_on_hover: bool,
    pub sound_enabled: bool,
    pub allow_critter_roaming: bool,
}

/// Tracks whether preferences have unsaved changes.
#[derive(Resource, Default)]
pub struct DirtyFlag {
    pub dirty: bool,
    pub last_save_timer: f32,
}

/// Whether the main window is currently visible (for close-to-tray).
#[derive(Resource, Default)]
pub struct MainWindowVisible(pub bool);
