#include "ecs/query.hpp"
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

  auto query = engine::ecs::Query<Position, Velocity>(&world);

  for (auto [entity, pos, vel] : query) {
    std::cout << "Entity: " << entity << ", Position: (" << pos.x << ", "
              << pos.y << "), Velocity: (" << vel.dx << ", " << vel.dy << ")\n";
  }

  return 0;
}
