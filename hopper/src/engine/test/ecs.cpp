#include "ecs/query.hpp"
#include "ecs/world.hpp"
#include <algorithm> // For std::ranges::for_each, std::ranges::distance
#include <cstdio>    // For printf
#include <ranges>    // For std::ranges

struct Position {
  float x, y;

  Position(float x, float y) : x(x), y(y) {}
};

struct Velocity {
  float dx, dy;

  Velocity(float dx, float dy) : dx(dx), dy(dy) {}
};

struct Health {
  int current, max;

  Health(int max_health) : current(max_health), max(max_health) {}
};

int main() {
  auto world = engine::ecs::World();

  // Create entities with different component combinations
  world.create_entity(Position{10.0f, 20.0f}, Velocity{1.0f, 2.0f},
                      Health{100});
  world.create_entity(Position{30.0f, 40.0f}, Health{80});
  world.create_entity(Position{50.0f, 60.0f}, Velocity{-1.0f, 3.0f});

  // Showcase 1: Entities with Position and Velocity
  printf("--- Entities with Position and Velocity ---\n");
  auto query_pos_vel = engine::ecs::Query<Position, Velocity>(&world);
  for (auto [entity, pos, vel] : query_pos_vel) {
    printf("Entity: %u, Position: (%.2f, %.2f), Velocity: (%.2f, %.2f)\n",
           entity, pos.x, pos.y, vel.dx, vel.dy);
  }
  printf("\n");

  // Showcase 2: Entities with Position and Health
  printf("--- Entities with Position and Health ---\n");
  auto query_pos_health = engine::ecs::Query<Position, Health>(&world);
  for (auto [entity, pos, health] : query_pos_health) {
    printf("Entity: %u, Position: (%.2f, %.2f), Health: %d/%d\n", entity, pos.x,
           pos.y, health.current, health.max);
  }
  printf("\n");

  // Showcase 3: All entities with Position (using 'with' builder)
  printf("--- All Entities with Position ---\n");
  auto query_all_pos = engine::ecs::Query<Position>(&world);
  for (auto [entity, pos] : query_all_pos) {
    printf("Entity: %u, Position: (%.2f, %.2f)\n", entity, pos.x, pos.y);
  }
  printf("\n");

  // Showcase 4: Entities with Position and Health, but without Velocity
  printf("--- Entities with Position and Health, without Velocity ---\n");
  auto query_pos_health_no_vel =
      engine::ecs::Query<Position, Health>(&world).without<Velocity>();
  for (auto [entity, pos, health] : query_pos_health_no_vel) {
    printf("Entity: %u, Position: (%.2f, %.2f), Health: %d/%d\n", entity, pos.x,
           pos.y, health.current, health.max);
  }
  printf("\n");

  return 0;
}
