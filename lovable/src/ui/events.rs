use bevy::prelude::*;

// These events are fired by UI interactions that require backend work.
// The UI module sends them; other modules (critter, preferences) consume them.

/// User pressed "Adopt" for a critter template.
#[derive(Event)]
pub struct AdoptCritterRequested(pub &'static str); // template id

/// User confirmed deletion of an owned critter.
#[derive(Event)]
pub struct DeleteCritterConfirmed(pub usize); // index in CritterRoster::owned

/// User toggled visibility of an owned critter from the critters tab.
#[derive(Event)]
pub struct ToggleCritterVisibility(pub usize);

/// User pressed "Hide All" on the home tab.
#[derive(Event)]
pub struct HideAllCritters;

/// User pressed "Show All" on the home tab.
#[derive(Event)]
pub struct ShowAllCritters;

/// User toggled a settings switch. The backend should persist to Preferences.
#[derive(Event, Clone)]
pub struct SettingChanged(pub SettingToggleId, pub bool);

/// User assigned a critter to a different monitor via drag or selection.
#[derive(Event)]
pub struct AssignCritterToMonitor {
    pub critter_idx: usize,
    pub monitor_idx: usize,
}

/// User pressed "Check for updates".
#[derive(Event)]
pub struct CheckForUpdatesRequested;

/// Identifies which settings toggle was interacted with, used in SettingChanged.
#[derive(Clone, PartialEq, Eq, Debug)]
pub enum SettingToggleId {
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

/// User completed onboarding (either finished or skipped).
#[derive(Event)]
pub struct OnboardingComplete;

/// User advanced to a specific onboarding step.
#[derive(Event)]
pub struct OnboardingStepAdvanced(pub usize); // 1-indexed step number
