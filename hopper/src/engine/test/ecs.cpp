#include "ecs/world.hpp"
#include <iostream>

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
  auto entity1 = world.create_entity(Position{10.0f, 20.0f},
                                     Velocity{1.0f, 2.0f}, Health{100});
  world.create_entity(Position{30.0f, 40.0f}, Health{80});
  world.create_entity(Position{50.0f, 60.0f}, Velocity{-1.0f, 3.0f});

  std::cout << "=== All entities with Position ===" << std::endl;
  world.each<Position>([](engine::ecs::Entity entity, const Position &pos) {
    std::cout << "Entity " << entity << ": Position(" << pos.x << ", " << pos.y
              << ")" << std::endl;
  });

  std::cout << "\n=== Entities with Position and Velocity ===" << std::endl;
  world.each<Position, Velocity>([](engine::ecs::Entity entity,
                                    const Position &pos, const Velocity &vel) {
    std::cout << "Entity " << entity << ": Position(" << pos.x << ", " << pos.y
              << "), Velocity(" << vel.dx << ", " << vel.dy << ")" << std::endl;
  });

  std::cout << "\n=== Entities with Health ===" << std::endl;
  world.each<Health>([](engine::ecs::Entity entity, const Health &health) {
    std::cout << "Entity " << entity << ": Health(" << health.current << "/"
              << health.max << ")" << std::endl;
  });

  // Test entity destruction
  std::cout << "\n=== Destroying Entity " << entity1 << " ===" << std::endl;
  world.destroy_entity(entity1);

  std::cout << "=== Remaining entities with Position ===" << std::endl;
  world.each<Position>([](engine::ecs::Entity entity, const Position &pos) {
    std::cout << "Entity " << entity << ": Position(" << pos.x << ", " << pos.y
              << ")" << std::endl;
  });

  std::cout << "\nECS Test completed successfully!" << std::endl;
  std::cout << "Total entities: " << world.entity_count() << std::endl;
  std::cout << "Total archetypes: " << world.archetype_count() << std::endl;

  return 0;
}
