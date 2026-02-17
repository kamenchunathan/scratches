use bevy::prelude::*;

// ─── Navigation State ────────────────────────────────────────────────────────

#[derive(Clone, PartialEq, Eq, Hash, Debug, Default)]
pub enum Tab {
    #[default]
    Home,
    Critters,
    Settings,
}

// Onboarding is a linear 4-step flow shown only on first launch.
#[derive(Clone, PartialEq, Eq, Hash, Debug, Default)]
pub enum OnboardingStep {
    #[default]
    Welcome,
    PickCritter,
    ChooseMonitor,
    InfoScreen,
}

#[derive(Resource)]
pub struct UiState {
    pub active_tab: Tab,
    pub onboarding_complete: bool,
    pub onboarding_step: OnboardingStep,
    /// Index into CRITTER_TEMPLATES; tracks selection in onboarding step 1.
    pub selected_onboarding_critter: usize,
    /// Index into CritterRoster::owned; which row is expanded in the critters tab.
    pub expanded_critter_idx: Option<usize>,
    /// Index into CritterRoster::owned; which row has delete confirmation shown.
    pub delete_confirm_idx: Option<usize>,
}

impl Default for UiState {
    fn default() -> Self {
        Self {
            active_tab: Tab::default(),
            onboarding_complete: false,
            onboarding_step: OnboardingStep::default(),
            selected_onboarding_critter: 0,
            expanded_critter_idx: None,
            delete_confirm_idx: None,
        }
    }
}

// ─── Critter Data ─────────────────────────────────────────────────────────────

pub struct CritterTemplate {
    pub id: &'static str,
    pub name: &'static str,
    pub species: &'static str,
}

pub const CRITTER_TEMPLATES: &[CritterTemplate] = &[
    CritterTemplate {
        id: "cat",
        name: "Whiskers",
        species: "Cat",
    },
    CritterTemplate {
        id: "dog",
        name: "Biscuit",
        species: "Dog",
    },
    CritterTemplate {
        id: "fish",
        name: "Goldie",
        species: "Fish",
    },
    CritterTemplate {
        id: "dragon",
        name: "Ember",
        species: "Dragon",
    },
    CritterTemplate {
        id: "penguin",
        name: "Pip",
        species: "Penguin",
    },
    CritterTemplate {
        id: "butterfly",
        name: "Luna",
        species: "Butterfly",
    },
    CritterTemplate {
        id: "rabbit",
        name: "Snow",
        species: "Rabbit",
    },
];

#[derive(Clone)]
pub struct OwnedCritter {
    pub template_id: &'static str,
    pub name: String,
    pub is_visible: bool,
    pub scale: u32,
    pub opacity: u32,
    /// Index into a monitor list; 0 = primary.
    pub assigned_monitor_idx: usize,
}

/// Mutable UI-side critter roster, kept in sync with Preferences on save.
/// Starts with two demo critters to mirror the prototype's initial state.
#[derive(Resource)]
pub struct CritterRoster {
    pub owned: Vec<OwnedCritter>,
}

impl Default for CritterRoster {
    fn default() -> Self {
        Self {
            owned: vec![
                OwnedCritter {
                    template_id: "cat",
                    name: "Whiskers".to_string(),
                    is_visible: true,
                    scale: 100,
                    opacity: 100,
                    assigned_monitor_idx: 0,
                },
                OwnedCritter {
                    template_id: "dog",
                    name: "Biscuit".to_string(),
                    is_visible: true,
                    scale: 100,
                    opacity: 100,
                    assigned_monitor_idx: 0,
                },
            ],
        }
    }
}

impl CritterRoster {
    pub fn available_templates(&self) -> Vec<&'static CritterTemplate> {
        CRITTER_TEMPLATES
            .iter()
            .filter(|t| !self.owned.iter().any(|o| o.template_id == t.id))
            .collect()
    }
}

// ─── Settings State ───────────────────────────────────────────────────────────

/// Settings values as reflected in the UI. Initialised from Preferences on load,
/// and written back to Preferences on change (via SettingChanged event).
#[derive(Resource)]
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

impl Default for AppSettingsUiState {
    fn default() -> Self {
        // Defaults mirror the TS prototype's initial state, which differs from
        // Preferences::default() (all-false) for some quality-of-life options.
        Self {
            start_on_boot: false,
            start_minimized: false,
            show_tray_icon: true,
            close_to_tray: true,
            check_for_updates: true,
            send_analytics: false,
            interactions_enabled: true,
            show_name_on_hover: true,
            sound_enabled: true,
            allow_critter_roaming: true,
        }
    }
}

// ─── Monitor Display ──────────────────────────────────────────────────────────

/// A display-only snapshot of a monitor, shown in the home and settings tabs.
/// Populated from Preferences/Monitor queries; stubbed with defaults for initial render.
#[derive(Clone)]
pub struct MonitorDisplay {
    pub name: String,
    pub resolution: String,
    pub refresh_rate: u32,
    pub enabled: bool,
}

#[derive(Resource, Default)]
pub struct MonitorList {
    pub monitors: Vec<MonitorDisplay>,
}

impl Default for MonitorDisplay {
    fn default() -> Self {
        Self {
            name: "Primary Monitor".to_string(),
            resolution: "1920x1080".to_string(),
            refresh_rate: 60,
            enabled: true,
        }
    }
}

impl MonitorList {
    pub fn stub_primary() -> Self {
        Self {
            monitors: vec![MonitorDisplay::default()],
        }
    }
}
