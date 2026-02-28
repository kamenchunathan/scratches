use bevy::{input::mouse::MouseWheel, prelude::*, window::PrimaryWindow};

use crate::{
    critter::{Critter, CritterId, CritterRegistry},
    preferences::{Preferences, PreferencesHandle, RegistryHandle},
    ui::{
        messages::{CrittersMsg, HomeMsg, Msg, OnboardingMsg, SettingsMsg, SystemMsg},
        palette::Palette,
        state::{
            AppScreen, AppSettingsUiState, CrittersTabState, DeleteConfirmState, DirtyFlag,
            MainState, MainWindowVisible, OnboardingState,
        },
        widgets::*,
    },
};

const AUTO_SAVE_INTERVAL_SECS: f32 = 300.0;

/// Marks preferences as dirty whenever they change, and auto-saves every
/// `AUTO_SAVE_INTERVAL_SECS` seconds.
pub fn periodic_save(
    time: Res<Time>,
    mut dirty: ResMut<DirtyFlag>,
    prefs_handle: Res<PreferencesHandle>,
    prefs_store: Res<Assets<Preferences>>,
    asset_server: Res<AssetServer>,
) {
    if !dirty.dirty {
        return;
    }

    dirty.last_save_timer += time.delta_secs();
    if dirty.last_save_timer < AUTO_SAVE_INTERVAL_SECS {
        return;
    }

    dirty.last_save_timer = 0.0;
    dirty.dirty = false;

    if let Some(prefs) = prefs_store.get(&prefs_handle.0) {
        save_prefs(prefs.clone(), &asset_server);
    }
}

pub fn save_prefs(mut prefs: Preferences, asset_server: &AssetServer) {
    use bevy::asset::AssetPath;
    use bevy::tasks::AsyncComputeTaskPool;

    prefs.first_start = false;
    let serialized = match bevy::scene::ron::ser::to_string_pretty(
        &prefs,
        bevy::scene::ron::ser::PrettyConfig::new(),
    ) {
        Ok(s) => s,
        Err(e) => {
            error!("Failed to serialize preferences: {e}");
            return;
        }
    };

    AsyncComputeTaskPool::get()
        .spawn({
            let asset_server = asset_server.clone();
            async move {
                let asset_path = AssetPath::parse("appdata://preferences.ron");
                let path = asset_path.path();
                let source = asset_server.get_source(asset_path.source()).unwrap();
                let writer = source.writer().unwrap();
                if let Err(e) = writer.write_bytes(path, serialized.as_bytes()).await {
                    error!("Failed to write preferences: {e}");
                }
            }
        })
        .detach();
}

// ─── Main update ─────────────────────────────────────────────────────────────

/// Single system that processes every `Msg` event, mirroring the Elm `update`
/// function. Mutates `AppScreen`, `AppSettingsUiState`, and `Preferences` only
/// here; all other systems are read-only consumers of those resources.
pub fn update(
    mut events: EventReader<Msg>,
    mut screen: ResMut<AppScreen>,
    mut settings: ResMut<AppSettingsUiState>,
    mut dirty: ResMut<DirtyFlag>,
    prefs_handle: Res<PreferencesHandle>,
    mut prefs_store: ResMut<Assets<Preferences>>,
    registry_handle: Res<RegistryHandle>,
    registry_store: Res<Assets<CritterRegistry>>,
    asset_server: Res<AssetServer>,
    mut rebuild: ResMut<NeedsRebuild>,
    mut app_exit: EventWriter<AppExit>,
    mut primary_window: Query<&mut Window, With<PrimaryWindow>>,
    mut main_visible: ResMut<MainWindowVisible>,
) {
    let Some(prefs) = prefs_store.get_mut(&prefs_handle.0) else {
        return;
    };

    for msg in events.read() {
        match msg {
            Msg::Home(m) => {
                handle_home(m, prefs, &mut rebuild);
                dirty.dirty = true;
            }
            Msg::Critters(m) => {
                handle_critters(
                    m,
                    &mut screen,
                    prefs,
                    &registry_store,
                    &registry_handle,
                    &mut rebuild,
                );
                dirty.dirty = true;
            }
            Msg::Settings(m) => {
                handle_settings(m, &mut settings, prefs);
                dirty.dirty = true;
            }
            Msg::Onboarding(m) => {
                handle_onboarding(
                    m,
                    &mut screen,
                    prefs,
                    &registry_store,
                    &registry_handle,
                    &mut rebuild,
                );
                dirty.dirty = true;
            }
            Msg::System(m) => {
                handle_system_events(
                    m,
                    prefs,
                    &asset_server,
                    &mut app_exit,
                    &mut primary_window,
                    &mut main_visible,
                    &mut settings,
                    &mut dirty,
                    &mut rebuild,
                );
            }
        }
    }
}

fn handle_home(msg: &HomeMsg, prefs: &mut Preferences, rebuild: &mut NeedsRebuild) {
    match msg {
        HomeMsg::HideAll => {
            prefs.critters.iter_mut().for_each(|c| c.is_visible = false);
            rebuild.0 = true;
        }
        HomeMsg::ShowAll => {
            prefs.critters.iter_mut().for_each(|c| c.is_visible = true);
            rebuild.0 = true;
        }
    }
}

fn handle_critters(
    msg: &CrittersMsg,
    screen: &mut AppScreen,
    prefs: &mut Preferences,
    registry_store: &Assets<CritterRegistry>,
    registry_handle: &RegistryHandle,
    rebuild: &mut NeedsRebuild,
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
                rebuild.0 = true;
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
            rebuild.0 = true;
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
            let monitor_fp = prefs.monitors.first().map(|m| m.fingerprint).unwrap_or(0);
            prefs.critters.push(Critter {
                id: CritterId::new(),
                def_id: def.id.clone(),
                name: def.name.clone(),
                position: bevy::math::Vec2::ZERO,
                scale: 1.0,
                opacity: 1.0,
                is_visible: true,
                monitor_fingerprint: monitor_fp,
                interactible: true,
            });
            rebuild.0 = true;
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
        SettingsMsg::AllowCritterRoaming(v) => {
            settings.allow_critter_roaming = *v;
            // TODO: Add setting to preferences
        }
    }
}

fn handle_onboarding(
    msg: &OnboardingMsg,
    screen: &mut AppScreen,
    prefs: &mut Preferences,
    registry_store: &Assets<CritterRegistry>,
    registry_handle: &RegistryHandle,
    rebuild: &mut NeedsRebuild,
) {
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
            let def_id = selected_def_id.clone();
            *screen = AppScreen::Onboarding(OnboardingState::ChooseMonitor {
                selected_def_id: def_id,
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
        (
            OnboardingState::ChooseMonitor {
                selected_def_id,
                selected_monitor,
            },
            OnboardingMsg::Next,
        ) => {
            let def_id = selected_def_id.clone();
            let monitor = *selected_monitor;
            *screen = AppScreen::Onboarding(OnboardingState::InfoScreen {
                selected_def_id: def_id,
                selected_monitor: monitor,
            });
        }
        (
            OnboardingState::InfoScreen {
                selected_def_id,
                selected_monitor,
            },
            OnboardingMsg::Next,
        ) => {
            let def_id = selected_def_id.clone();
            let chosen_monitor = *selected_monitor;

            if !def_id.is_empty() {
                if let Some(registry) = registry_store.get(&registry_handle.0) {
                    if let Some(def) = registry.find(&def_id) {
                        let already_owned = prefs.critters.iter().any(|c| c.def_id == def_id);
                        if !already_owned {
                            let monitor_fp = chosen_monitor
                                .or_else(|| prefs.monitors.first().map(|m| m.fingerprint))
                                .unwrap_or(0);
                            prefs.critters.push(Critter {
                                id: CritterId::new(),
                                def_id: def.id.clone(),
                                name: def.name.clone(),
                                position: bevy::math::Vec2::ZERO,
                                scale: 1.0,
                                opacity: 1.0,
                                is_visible: true,
                                monitor_fingerprint: monitor_fp,
                                interactible: true,
                            });
                            rebuild.0 = true;
                        }
                    }
                }
            }

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

fn handle_system_events(
    msg: &SystemMsg,
    prefs: &mut Preferences,
    asset_server: &AssetServer,
    app_exit: &mut EventWriter<AppExit>,
    primary_window: &mut Query<&mut Window, With<PrimaryWindow>>,
    main_visible: &mut MainWindowVisible,
    _settings: &mut AppSettingsUiState,
    dirty: &mut DirtyFlag,
    rebuild: &mut NeedsRebuild,
) {
    match msg {
        SystemMsg::CheckForUpdates => {
            info!("Check for updates requested (stub)");
        }

        SystemMsg::OpenUrl(url) => {
            info!("Opening URL: {url}");
            #[cfg(target_os = "windows")]
            {
                let _ = std::process::Command::new("cmd")
                    .args(["/C", "start", url.as_str()])
                    .spawn();
            }
            #[cfg(target_os = "linux")]
            {
                let _ = std::process::Command::new("xdg-open")
                    .arg(url.as_str())
                    .spawn();
            }
            #[cfg(target_os = "macos")]
            {
                let _ = std::process::Command::new("open").arg(url.as_str()).spawn();
            }
        }

        SystemMsg::TrayShowHide => {
            if let Ok(mut window) = primary_window.single_mut() {
                window.visible = !window.visible;
            }
        }

        SystemMsg::TrayQuit => {
            dirty.dirty = false; // save happens in save_preferences_on_exit
            app_exit.write(AppExit::Success);
        }

        SystemMsg::TrayToggleCritter(id) => {
            if let Some(c) = prefs.critters.iter_mut().find(|c| &c.id == id) {
                c.is_visible = !c.is_visible;
                dirty.dirty = true;
                rebuild.0 = true;
            }
        }

        SystemMsg::SaveNow => {
            save_prefs(prefs.clone(), asset_server);
            dirty.dirty = false;
            dirty.last_save_timer = 0.0;
            info!("Preferences saved");
        }
    }
}

/// Set to `true` whenever any state mutation requires the UI or tray to be
/// re-synced. Every consumer clears the flag independently after acting on it.
#[derive(Resource, Default)]
pub struct NeedsRebuild(pub bool);

// Each system handles one logical group of buttons so no single system
// exceeds Bevy's system-parameter limit and the responsibilities stay clear.

pub fn dispatch_settings_toggles(
    mut writer: EventWriter<Msg>,
    mut toggle_buttons: Query<
        (&Interaction, &mut ToggleWidget),
        (Changed<Interaction>, With<Button>),
    >,
) {
    for (interaction, btn) in &mut toggle_buttons {
        if *interaction != Interaction::Pressed {
            continue;
        }
        let msg = match btn.id {
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
        writer.write(Msg::Settings(msg));
    }
}

pub fn dispatch_critter_buttons(
    mut writer: EventWriter<Msg>,
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
) {
    for (i, btn) in &adopt_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::Critters(CrittersMsg::Adopt(btn.def_id.clone())));
        }
    }
    for (i, btn) in &expand_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::Critters(CrittersMsg::ToggleExpand(btn.critter_id)));
        }
    }
    for (i, btn) in &vis_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::Critters(CrittersMsg::ToggleVisibility(btn.critter_id)));
        }
    }
    for (i, btn) in &delete_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::Critters(CrittersMsg::RequestDelete(btn.critter_id)));
        }
    }
    for (i, btn) in &confirm_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::Critters(CrittersMsg::ConfirmDelete(btn.critter_id)));
        }
    }
    for (i, _) in &cancel_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::Critters(CrittersMsg::CancelDelete));
        }
    }
    for (i, btn) in &monitor_assign_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::Critters(CrittersMsg::AssignMonitor {
                critter: btn.critter_id,
                monitor_fingerprint: btn.monitor_fingerprint,
            }));
        }
    }
}

pub fn dispatch_home_buttons(
    mut writer: EventWriter<Msg>,
    hide_all: Query<&Interaction, (Changed<Interaction>, With<Button>, With<HideAllButton>)>,
    show_all: Query<&Interaction, (Changed<Interaction>, With<Button>, With<ShowAllButton>)>,
) {
    for i in &hide_all {
        if *i == Interaction::Pressed {
            writer.write(Msg::Home(HomeMsg::HideAll));
        }
    }
    for i in &show_all {
        if *i == Interaction::Pressed {
            writer.write(Msg::Home(HomeMsg::ShowAll));
        }
    }
}

pub fn dispatch_onboarding_buttons(
    mut writer: EventWriter<Msg>,
    primary: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<OnboardingPrimaryButton>,
        ),
    >,
    skip: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<OnboardingSkipButton>,
        ),
    >,
    critter_cards: Query<
        (&Interaction, &OnboardingCritterCard),
        (Changed<Interaction>, With<Button>),
    >,
    monitor_cards: Query<
        (&Interaction, &OnboardingMonitorCard),
        (Changed<Interaction>, With<Button>),
    >,
) {
    for i in &primary {
        if *i == Interaction::Pressed {
            writer.write(Msg::Onboarding(OnboardingMsg::Next));
        }
    }
    for i in &skip {
        if *i == Interaction::Pressed {
            writer.write(Msg::Onboarding(OnboardingMsg::Skip));
        }
    }
    for (i, card) in &critter_cards {
        if *i == Interaction::Pressed {
            writer.write(Msg::Onboarding(OnboardingMsg::SelectCritter(
                card.def_id.clone(),
            )));
        }
    }
    for (i, card) in &monitor_cards {
        if *i == Interaction::Pressed {
            writer.write(Msg::Onboarding(OnboardingMsg::SelectMonitor(
                card.monitor_fingerprint,
            )));
        }
    }
}

pub fn dispatch_system_buttons(
    mut writer: EventWriter<Msg>,
    mut app_exit: EventWriter<AppExit>,
    settings: Res<AppSettingsUiState>,
    check_update_buttons: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<CheckForUpdatesButton>,
        ),
    >,
    link_chip_buttons: Query<(&Interaction, &LinkChipButton), (Changed<Interaction>, With<Button>)>,
    close_buttons: Query<
        &Interaction,
        (Changed<Interaction>, With<Button>, With<CloseWindowButton>),
    >,
    minimize_buttons: Query<
        &Interaction,
        (
            Changed<Interaction>,
            With<Button>,
            With<MinimizeToTrayButton>,
        ),
    >,
) {
    for i in &check_update_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::System(SystemMsg::CheckForUpdates));
        }
    }
    for (i, chip) in &link_chip_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::System(SystemMsg::OpenUrl(chip.url.clone())));
        }
    }
    for i in &minimize_buttons {
        if *i == Interaction::Pressed {
            writer.write(Msg::System(SystemMsg::TrayShowHide));
        }
    }
    for i in &close_buttons {
        if *i == Interaction::Pressed {
            if settings.close_to_tray {
                writer.write(Msg::System(SystemMsg::TrayShowHide));
            } else {
                app_exit.write(AppExit::Success);
            }
        }
    }
}

// ─── Scrolling ────────────────────────────────────────────────────────────────

/// Drives the scroll position on `TabScrollArea` from `MouseWheel` events.
pub fn scroll_tab_area(
    mut mouse_wheel: EventReader<MouseWheel>,
    mut scroll_query: Query<&mut ScrollPosition, With<TabScrollArea>>,
) {
    let Ok(mut scroll) = scroll_query.single_mut() else {
        return;
    };

    for ev in mouse_wheel.read() {
        let delta = match ev.unit {
            bevy::input::mouse::MouseScrollUnit::Line => ev.y * 20.0,
            bevy::input::mouse::MouseScrollUnit::Pixel => ev.y,
        };
        scroll.offset_y -= delta;
        scroll.offset_y = scroll.offset_y.max(0.0);
    }
}

/// Resets scroll to top whenever the active tab changes.
pub fn reset_scroll_on_tab_change(
    screen: Res<AppScreen>,
    mut scroll_query: Query<&mut ScrollPosition, With<TabScrollArea>>,
) {
    if !screen.is_changed() {
        return;
    }
    if let Ok(mut scroll) = scroll_query.single_mut() {
        scroll.offset_y = 0.0;
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
        AppScreen::Onboarding(OnboardingState::InfoScreen { .. }) => 3,
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
    settings: Res<AppSettingsUiState>,
    mut query: Query<(&mut BackgroundColor, &mut Node, &mut ToggleWidget)>,
    palette: Res<Palette>,
) {
    if !settings.is_changed() {
        return;
    }
    for (mut bg, mut node, mut toggle) in &mut query {
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
        toggle.is_on = is_on;
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

/// Syncs tab button hover/active visuals, including highlighting the active tab.
pub fn sync_tab_button_visuals(
    screen: Res<AppScreen>,
    mut query: Query<(&Interaction, &TabButton, &mut BackgroundColor)>,
    palette: Res<Palette>,
) {
    let active = match screen.as_ref() {
        AppScreen::Main(m) => Some(&m.active_tab),
        _ => None,
    };

    for (interaction, btn, mut bg) in &mut query {
        let is_active = active == Some(&btn.0);
        *bg = match (interaction, is_active) {
            (Interaction::Pressed, _) | (_, true) => BackgroundColor(palette.accent),
            (Interaction::Hovered, false) => BackgroundColor(palette.muted),
            (Interaction::None, false) => BackgroundColor(palette.card),
        };
    }
}

/// Handles tab button presses directly.
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

/// Intercepts window close requests.
pub fn handle_window_close(
    mut close_events: EventReader<bevy::window::WindowCloseRequested>,
    settings: Res<AppSettingsUiState>,
    mut writer: EventWriter<Msg>,
    mut app_exit: EventWriter<AppExit>,
) {
    for _ in close_events.read() {
        if settings.close_to_tray {
            writer.write(Msg::System(SystemMsg::TrayShowHide));
        } else {
            app_exit.write(AppExit::Success);
        }
    }
}

pub fn quit_on_esc(
    keys: Res<ButtonInput<KeyCode>>,
    settings: Res<AppSettingsUiState>,
    mut msg_writer: EventWriter<Msg>,
    mut app_exit: EventWriter<AppExit>,
) {
    if keys.just_pressed(KeyCode::Escape) || keys.just_pressed(KeyCode::KeyQ) {
        if settings.close_to_tray {
            info!("Keyboard close — hiding to tray");
            msg_writer.write(Msg::System(SystemMsg::TrayShowHide));
        } else {
            info!("Keyboard close — exiting");
            app_exit.write(AppExit::Success);
        }
    }
}
