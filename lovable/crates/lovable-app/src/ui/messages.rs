use bevy::prelude::*;

use crate::critter::CritterId;

/// messages sent after UI events. Modelled after the elm model
#[derive(Event, Debug, Clone)]
pub enum Msg {
    Home(HomeMsg),
    Critters(CrittersMsg),
    Settings(SettingsMsg),
    Onboarding(OnboardingMsg),
    System(SystemMsg),
}

// Home tab
#[derive(Debug, Clone)]
pub enum HomeMsg {
    HideAll,
    ShowAll,
}

// Critters tab
#[derive(Debug, Clone)]
pub enum CrittersMsg {
    ToggleExpand(CritterId),
    ToggleVisibility(CritterId),
    RequestDelete(CritterId),
    ConfirmDelete(CritterId),
    CancelDelete,
    Adopt(String),
    AssignMonitor {
        critter: CritterId,
        monitor_fingerprint: u64,
    },
}

//  Settings tab
/// One variant per persisted setting. The compiler enforces exhaustive handling
/// whenever a new setting is added — no stringly-typed dispatch.
#[derive(Debug, Clone)]
pub enum SettingsMsg {
    SetStartOnBoot(bool),
    SetStartMinimized(bool),
    SetShowTrayIcon(bool),
    SetCloseToTray(bool),
    SetCheckForUpdates(bool),
    SetSendAnalytics(bool),
    SetInteractionsEnabled(bool),
    SetShowNameOnHover(bool),
    SetSoundEnabled(bool),
    AllowCritterRoaming(bool),
}

// Onboarding
#[derive(Debug, Clone)]
pub enum OnboardingMsg {
    Next,
    Skip,
    SelectCritter(String),
    SelectMonitor(u64),
}

// System-level messages
#[derive(Debug, Clone)]
pub enum SystemMsg {
    CheckForUpdates,
    OpenUrl(String),
    TrayShowHide,
    TrayQuit,
    TrayToggleCritter(CritterId),
    SaveNow,
}
