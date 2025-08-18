#include "world.h"
#include <cstdio>

struct Position {
  float x, y;
};

struct Velocity {
  float dx, dy;
};

int main() {
  World world;

  // Create entities
  // NOTE
  Entity entity1 =
      world.create_entity(Position{10.0f, 20.0f}, Velocity{1.0f, 2.0f});
  Entity entity2 = world.create_entity(Position{30.0f, 40.0f});
  Entity entity3 =
      world.create_entity(Position{50.0f, 60.0f}, Velocity{3.0f, 4.0f});

  std::printf("Entities with Position and Velocity:\n");
  world.each<Position>([](Entity entity, Position pos) {
    std::printf("Okay here we go \n %u \n", entity);
    // NOTE: We have memory corruption issues here.
    std::printf("Entity %u: Position(%.2f, %.2f) \n", entity, pos.x, pos.y);
  });

  std::printf("\nEntities with only Position:\n");
  world.each<Position>([](Entity entity, Position &pos) {
    std::printf("Entity %u: Position(%.2f, %.2f)\n", entity, pos.x, pos.y);
  });

  // Destroy an entity
  world.destroy_entity(entity1);

  std::printf(
      "\nAfter destroying Entity %u, entities with Position and Velocity:\n",
      entity1);
  world.each<Position>([](Entity entity, Position &pos) {
    std::printf("Entity %u: Position(%.2f, %.2f)\n", entity, pos.x, pos.y);
  });

  return 0;
}
