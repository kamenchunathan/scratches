#include "ecs/query.hpp"
#include "ecs/world.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <ranges>

struct Position {
    float x, y;
    Position(float x, float y): x(x), y(y) {}
};

struct Velocity {
    float dx, dy;
    Velocity(float dx, float dy): dx(dx), dy(dy) {}

    float magnitude() const {
        return std::sqrt(dx * dx + dy * dy);
    }
};

struct Health {
    int current, max;
    Health(int max_health): current(max_health), max(max_health) {}

    bool is_alive() const {
        return current > 0;
    }
    float health_ratio() const {
        return static_cast<float>(current) / max;
    }
};

struct Enemy {
    float aggression;
    Enemy(float aggro = 1.0f): aggression(aggro) {}
};

struct Player {
    std::string name;
    int level;
    Player(std::string n, int lvl = 1): name(std::move(n)), level(lvl) {}
};

struct Collectible {
    int value;
    enum Type { COIN, GEM, POWERUP } type;
    Collectible(Type t, int v): value(v), type(t) {}
};

struct Dead {};

void spawn_grid_entities(ecs::World& world, float spacing = 2.0f, int grid_size = 10) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> vel_dist(-2.0f, 2.0f);
    std::uniform_int_distribution<int> health_dist(50, 150);
    std::uniform_real_distribution<float> aggro_dist(0.1f, 2.0f);
    std::uniform_int_distribution<int> collectible_type(0, 2);
    std::uniform_int_distribution<int> collectible_value(10, 100);

    printf("Spawning %dx%d grid of entities (spacing: %.1fm)\n", grid_size, grid_size, spacing);

    for (int x = 0; x < grid_size; ++x) {
        for (int y = 0; y < grid_size; ++y) {
            float pos_x = x * spacing;
            float pos_y = y * spacing;

            // Create different types of entities based on position
            if (x == 0 && y == 0) {
                // Player at origin
                world.create_entity(
                    Position { pos_x, pos_y },
                    Velocity { 0.0f, 0.0f },
                    Health { 200 },
                    Player { "Hero", 5 }
                );
            } else if ((x + y) % 7 == 0) {
                // Some enemies (every 7th position)
                world.create_entity(
                    Position { pos_x, pos_y },
                    Velocity { vel_dist(gen), vel_dist(gen) },
                    Health { health_dist(gen) },
                    Enemy { aggro_dist(gen) }
                );
            } else if ((x + y) % 5 == 0) {
                // Some collectibles (every 5th position)
                auto type = static_cast<Collectible::Type>(collectible_type(gen));
                world.create_entity(
                    Position { pos_x, pos_y },
                    Collectible { type, collectible_value(gen) }
                );
            } else if ((x * y) % 3 == 0 && x > 0 && y > 0) {
                // Moving entities without special components
                world.create_entity(
                    Position { pos_x, pos_y },
                    Velocity { vel_dist(gen), vel_dist(gen) },
                    Health { health_dist(gen) }
                );
            } else {
                // Static entities
                world.create_entity(Position { pos_x, pos_y }, Health { health_dist(gen) });
            }
        }
    }
}

void create_some_dead_entities(ecs::World& world) {
    // Create some dead entities for filtering demonstration
    world.create_entity(Position { -5.0f, -5.0f }, Health { 0 }, Dead {});
    world.create_entity(Position { -3.0f, -3.0f }, Velocity { 0.0f, 0.0f }, Health { 0 }, Dead {});
}

template<typename Range>
void print_range_info(const std::string& name, Range&& range) {
    auto count = std::ranges::distance(range);
    printf("\n=== %s ===\n", name.c_str());
    printf("Count: %zu\n", count);

    int printed = 0;
    for (auto&& item: range) {
        if (printed >= 5) {
            printf("... (showing first 5 of %zu)\n", count);
            break;
        }

        // Handle different tuple types
        if constexpr (std::tuple_size_v<std::decay_t<decltype(item)>> >= 3) {
            auto [entity, pos, comp] = item;
            printf("  Entity %u: pos(%.1f, %.1f)\n", entity, pos.x, pos.y);
        } else {
            printf("  Item: %u\n", item);
        }
        ++printed;
    }
}

int main() {
    auto world = ecs::World();

    // Spawn a reasonable grid of entities
    spawn_grid_entities(world, 2.0f, 8); // 8x8 grid with 2m spacing
    create_some_dead_entities(world);

    printf("\nWorld Stats:\n");
    printf("Total entities: %zu\n", world.entity_count());
    printf("Archetypes: %zu\n", world.archetype_count());

    // =====================================================
    // RANGES PIPELINE DEMONSTRATIONS
    // =====================================================

    printf("\n%s\n", std::string(60, '=').c_str());
    printf("RANGES PIPELINE DEMONSTRATIONS\n");
    printf("%s\n", std::string(60, '=').c_str());

    // 1. All moving entities (Position + Velocity)
    auto moving_entities = ecs::Query<Position, Velocity>(&world);

    print_range_info("All Moving Entities", moving_entities);

    // 2. Fast moving entities (speed > 2.0)
    auto fast_movers = moving_entities | std::views::filter([](const auto& tuple) {
                           auto [entity, pos, vel] = tuple;
                           return vel.magnitude() > 2.0f;
                       });

    print_range_info("Fast Moving Entities (speed > 2.0)", fast_movers);

    // 3. Entities in first quadrant, transformed to show just positions
    auto first_quadrant_positions = moving_entities | std::views::filter([](const auto& tuple) {
                                        auto [entity, pos, vel] = tuple;
                                        return pos.x >= 0 && pos.y >= 0;
                                    })
        | std::views::transform([](const auto& tuple) {
                                        auto [entity, pos, vel] = tuple;
                                        return std::make_pair(entity, pos);
                                    });

    printf("\n=== First Quadrant Entities (Entity ID, Position) ===\n");
    for (auto [entity, pos]: first_quadrant_positions | std::views::take(8)) {
        printf("  Entity %u: (%.1f, %.1f)\n", entity, pos.x, pos.y);
    }

    // 4. Living entities query with health filtering
    auto living_entities = ecs::Query<Position, Health>(&world).without<Dead>();

    auto healthy_entities = living_entities | std::views::filter([](const auto& tuple) {
                                auto [entity, pos, health] = tuple;
                                return health.health_ratio() > 0.7f; // More than 70% health
                            });

    print_range_info("Healthy Living Entities (>70% health)", healthy_entities);

    // 5. Enemy analysis pipeline
    auto enemy_query = ecs::Query<Position, Enemy, Health>(&world);

    auto dangerous_enemies = enemy_query | std::views::filter([](const auto& tuple) {
                                 auto [entity, pos, enemy, health] = tuple;
                                 return enemy.aggression > 1.5f && health.is_alive();
                             })
        | std::views::transform([](const auto& tuple) {
                                 auto [entity, pos, enemy, health] = tuple;
                                 return std::make_tuple(entity, enemy.aggression, health.current);
                             });

    printf("\n=== Dangerous Enemies (Entity, Aggression, Health) ===\n");
    for (auto [entity, aggro, hp]: dangerous_enemies) {
        printf("  Entity %u: aggression=%.2f, hp=%d\n", entity, aggro, hp);
    }

    // 6. Collectibles by value
    auto collectibles = ecs::Query<Position, Collectible>(&world);

    auto valuable_collectibles = collectibles | std::views::filter([](const auto& tuple) {
                                     auto [entity, pos, item] = tuple;
                                     return item.value >= 50;
                                 })
        | std::views::transform([](const auto& tuple) {
                                     auto [entity, pos, item] = tuple;
                                     return std::make_tuple(entity, item.value, pos.x, pos.y);
                                 });

    printf("\n=== Valuable Collectibles (>=50 value) ===\n");
    for (auto [entity, value, x, y]: valuable_collectibles) {
        printf("  Entity %u: value=%d at (%.1f, %.1f)\n", entity, value, x, y);
    }

    // 7. Distance-based queries (entities within 10 units of origin)
    auto nearby_entities = moving_entities | std::views::filter([](const auto& tuple) {
                               auto [entity, pos, vel] = tuple;
                               float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
                               return distance <= 10.0f;
                           })
        | std::views::take(5); // Just first 5

    printf("\n=== Nearby Moving Entities (within 10 units of origin) ===\n");
    for (auto [entity, pos, vel]: nearby_entities) {
        float distance = std::sqrt(pos.x * pos.x + pos.y * pos.y);
        printf("  Entity %u: distance=%.2f, speed=%.2f\n", entity, distance, vel.magnitude());
    }

    // 8. Complex multi-stage pipeline
    auto complex_pipeline = ecs::Query<Position, Velocity, Health>(&world).without<Dead>()
        | std::views::filter([](const auto& tuple) {
                                auto [entity, pos, vel, health] = tuple;
                                return health.is_alive() && vel.magnitude() > 1.0f;
                            })
        | std::views::transform([](const auto& tuple) {
                                auto [entity, pos, vel, health] = tuple;
                                struct EntityInfo {
                                    uint32_t id;
                                    float speed;
                                    float health_pct;
                                    float distance_from_origin;
                                };

                                return EntityInfo { entity,
                                                    vel.magnitude(),
                                                    health.health_ratio(),
                                                    std::sqrt(pos.x * pos.x + pos.y * pos.y) };
                            })
        | std::views::take(10);

    printf("\n=== Complex Pipeline: Living, Moving Entities (Analysis) ===\n");
    for (const auto& info: complex_pipeline) {
        printf(
            "  Entity %u: speed=%.2f, health=%.0f%%, distance=%.1f\n",
            info.id,
            info.speed,
            (info.health_pct * 100),
            info.distance_from_origin
        );
    }

    // 9. Aggregate operations using ranges algorithms
    auto all_health_entities = ecs::Query<Health>(&world).without<Dead>();

    auto health_values = all_health_entities | std::views::transform([](const auto& tuple) {
                             auto [entity, health] = tuple;
                             return health.current;
                         });

    // auto total_health = std::ranges::views::fold(health_values, 0, std::plus
    // {}); auto max_health = std::ranges::max(health_values); auto min_health =
    // std::ranges::min(health_values); auto entity_count =
    // std::ranges::distance(health_values);
    //
    // printf("\n=== Health Statistics ===\n");
    // printf("Living entities: %ld\n", (long)entity_count);
    // printf("Total health: %d\n", total_health);
    // printf("Average health: %ld\n", (long)(entity_count > 0 ? total_health /
    // entity_count : 0)); printf("Health range: %d - %d\n", min_health,
    // max_health);
    //
    // return 0;
}
