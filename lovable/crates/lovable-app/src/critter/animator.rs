// use bevy::prelude::*;

// pub fn spawn_critter(
//     mut commands: Commands,
//     asset_server: Res<AssetServer>,
//     mut graphs: ResMut<Assets<AnimationGraph>>,
// ) {
// let scene = SceneRoot(asset_server.load(GltfAssetLabel::Scene(0).from_asset(GLTF_PATH)));
//
// let (graph, index) = AnimationGraph::from_clip(
//     asset_server.load(GltfAssetLabel::Animation(0).from_asset(GLTF_PATH)),
// );
//
// let jump_graph = graphs.add(graph);
// let jump_animation = JumpAnimation {
//     graph: jump_graph,
//     index,
// };
//
// commands
//     .spawn((scene, jump_animation))
//     .observe(play_animation);
// }
