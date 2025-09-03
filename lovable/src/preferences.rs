use bevy::{
    asset::{Asset, AssetLoader, AsyncReadExt, uuid::Uuid},
    asset::{AssetLoadFailedEvent, AssetPath},
    log::warn,
    prelude::*,
    reflect::Reflect,
    scene::ron,
    tasks::AsyncComputeTaskPool,
};
use serde::{Deserialize, Serialize};
use thiserror::Error;

#[derive(Serialize, Deserialize, Reflect, Asset, Debug, Default)]
pub struct Preferences {
    /// Critters that should be displayed on screen
    critter_ids: Vec<Uuid>,
}

#[derive(Error, Debug)]
#[non_exhaustive]
pub enum PreferencesLoadError {
    #[error("IO Error")]
    IoError(#[from] std::io::Error),

    #[error("deserialization Error")]
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
        warn!("Preferences not found, Creating default preferences");
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
