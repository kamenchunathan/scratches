use bevy::{
    asset::{Asset, AssetLoadFailedEvent, AssetLoader, AssetPath, AsyncReadExt},
    platform::collections::HashMap,
    prelude::*,
    reflect::Reflect,
    scene::ron,
    tasks::AsyncComputeTaskPool,
};
use serde::{Deserialize, Serialize};
use thiserror::Error;

#[derive(Serialize, Deserialize, Reflect, Asset, Debug)]
pub struct Preferences {
    /// Version for migrations
    pub version: String,

    /// Active theme
    pub theme: crate::ui::palette::Palette,

    /// Per Monitor settings
    pub monitors: Vec<MonitorSettings>,

    /// Application level settings
    pub app_settings: ApplicationSettings,

    /// Map of monitor Id to critter
    pub critters: HashMap<u32, crate::critter::Critter>,

    /// Global Critter settings
    pub global_critter_settings: GlobalCritterSettings,

    /// If this is the first launch of the application
    /// Useful for running an onboarding process
    pub first_start: bool,
}

impl Default for Preferences {
    fn default() -> Self {
        Self {
            version: Default::default(),
            theme: Default::default(),
            monitors: Default::default(),
            app_settings: Default::default(),
            critters: Default::default(),
            global_critter_settings: Default::default(),
            first_start: true,
        }
    }
}

/// Monitor specific settings for critters
#[derive(Debug, Default, Serialize, Deserialize, Reflect)]
pub struct MonitorSettings {
    /// Monitor identifier
    pub id: u32,

    /// Whether critters are enabled on this monitor
    pub enabled: bool,

    /// Safe zones where critters shouldn't go (for taskbars, docks, etc.)
    /// Format: (x, y, width, height) as percentages of screen size
    pub exclusion_zones: Vec<(f32, f32, f32, f32)>,
}

#[derive(Debug, Default, Serialize, Deserialize, Reflect)]
pub struct ApplicationSettings {
    /// Start application on system startup
    pub start_on_boot: bool,

    /// Start minimized to system tray
    pub start_minimized: bool,

    /// Show system tray icon
    pub show_tray_icon: bool,

    /// Close to tray instead of exiting
    pub close_to_tray: bool,

    /// Check for updates on startup
    pub check_updates: bool,

    /// Send anonymous usage statistics
    pub send_analytics: bool,
}

#[derive(Debug, Default, Serialize, Deserialize, Reflect)]
pub struct GlobalCritterSettings {
    /// Whether critters can interact with the mouse or respond to user actions
    pub interactions_enabled: bool,

    /// Display critter names on hover
    pub display_critter_name_on_hover: bool,

    /// Whether critters can play sounds
    pub sound_enabled: bool,
}

#[derive(Error, Debug)]
#[non_exhaustive]
pub enum PreferencesLoadError {
    #[error("IO Error")]
    IoError(#[from] std::io::Error),

    #[error("Deserialization Error")]
    DeserializationError(#[from] ron::de::SpannedError),
}

pub struct PreferencesLoader;

impl AssetLoader for PreferencesLoader {
    type Asset = Preferences;
    type Settings = ();
    type Error = PreferencesLoadError;

    fn load(
        &self,
        reader: &mut dyn bevy::asset::io::Reader,
        _: &Self::Settings,
        _: &mut bevy::asset::LoadContext,
    ) -> impl bevy::tasks::ConditionalSendFuture<Output = Result<Self::Asset, Self::Error>> {
        async {
            let mut buf = String::new();
            reader.read_to_string(&mut buf).await?;
            Ok(ron::from_str(&buf)?)
        }
    }
}

#[derive(Resource)]
pub struct PreferencesHandle(Handle<Preferences>);

pub fn access_prefs(mut commands: Commands, asset_server: Res<AssetServer>) {
    let prefs: Handle<Preferences> = asset_server.load("appdata://preferences.ron");
    commands.insert_resource(PreferencesHandle(prefs.clone()));
}

pub fn monitor_preferences_loading(
    prefs_handle: Res<PreferencesHandle>,
    mut prefs_store: ResMut<Assets<Preferences>>,
    mut events: EventReader<AssetLoadFailedEvent<Preferences>>,
) {
    if events.read().next().is_some() {
        info!("Preferences not found, Creating default preferences");
        prefs_store.insert(&prefs_handle.0, Preferences::default());
    }
}

pub fn save_preferences_on_exit(
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    asset_server: Res<AssetServer>,
    mut app_exit_events: EventReader<AppExit>,
) {
    if app_exit_events.read().next().is_some() {
        if let Some(prefs) = prefs_store.get(&prefs_handle.0) {
            AsyncComputeTaskPool::get()
                .spawn({
                    let asset_server = asset_server.clone();
                    let serialized_prefs =
                        ron::to_string(prefs).expect("Unable to serialize preferences");

                    async move {
                        let asset_path = AssetPath::parse("appdata://preferences.ron");
                        let path = asset_path.path();
                        let source = asset_server.get_source(asset_path.source()).unwrap();
                        let writer = source.writer().unwrap();
                        writer
                            .write_bytes(path, serialized_prefs.as_bytes())
                            .await
                            .expect("Unable to write preferences");
                    }
                })
                .detach();
        }
    }
}
