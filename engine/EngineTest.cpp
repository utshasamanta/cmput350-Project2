#include <stdexcept>

#include "ECSEngine.h"
#include "CollisionSystem.h"
#include "ScoreSystem.h"
#include "SpawnSystem.h"

using namespace ECSEngine;

#define TEST_COMPONENTS LocationComponent, MovementComponent, GravityComponent, CollisionComponent, \
    SpriteComponent, SpawnComponent, ScoreComponent, CameraShake, RespawnComponent

// Spawning and score updates do not need a window or loaded textures.
class TestEngine : public EngineView<TEST_COMPONENTS> {
public:
    EntityManager<TEST_COMPONENTS>& GetEntityManager() override { return entities; }
    SoundManager& GetSoundManager() override { return sounds; }
    SpriteManager& GetSpriteManager() override { return sprites; }
    WindowManager& GetWindowManager() override { throw std::runtime_error("No test window"); }

    EntityManager<TEST_COMPONENTS> entities;
    SoundManager sounds;
    SpriteManager sprites;
};

void Check(bool condition) {
    if (!condition) {
        throw std::runtime_error("World feature check failed");
    }
}

void TestStarCollisionVelocities() {
    for (int axis = 0; axis < 2; ++axis) {
        for (int separating = 0; separating < 2; ++separating) {
            TestEngine engine;
            CreateStar(engine.entities, {0.0f, 0.0f}, {}, 0);
            CreateStar(engine.entities,
                       axis == 0 ? Point2D{40.0f, 0.0f} : Point2D{0.0f, 40.0f}, {}, 0);
            const float firstSpeed = separating ? -1.0f : 4.0f;
            const float secondSpeed = separating ? 4.0f : -1.0f;
            engine.entities.GetComponent<MovementComponent>(0).velocity =
                axis == 0 ? Point2D{firstSpeed, 3.0f} : Point2D{3.0f, firstSpeed};
            engine.entities.GetComponent<MovementComponent>(1).velocity =
                axis == 0 ? Point2D{secondSpeed, -2.0f} : Point2D{-2.0f, secondSpeed};
            CollisionSystem<TEST_COMPONENTS> collision;
            collision.Run(&engine);
            const auto a = engine.entities.GetComponent<MovementComponent>(0).velocity;
            const auto b = engine.entities.GetComponent<MovementComponent>(1).velocity;
            Check((axis == 0 ? a.x : a.y) == (separating ? firstSpeed : secondSpeed));
            Check((axis == 0 ? b.x : b.y) == (separating ? secondSpeed : firstSpeed));
            Check((axis == 0 ? a.y : a.x) == 3.0f);
            Check((axis == 0 ? b.y : b.x) == -2.0f);
            const auto first = engine.entities.GetComponent<LocationComponent>(0).loc;
            const auto second = engine.entities.GetComponent<LocationComponent>(1).loc;
            Check((axis == 0 ? second.x - first.x : second.y - first.y) == 48.0f);
        }
    }
    std::cout << "Star collision direction and separation checks passed\n";
}

int main() {
    TestStarCollisionVelocities();
    TestEngine engine;
    auto& entities = engine.entities;
    // Multiple spawners exercise creation without invalidating the active iteration.
    for (int i = 0; i < 40; ++i) {
        const auto id = entities.CreateEntity("spawner");
        entities.AddComponent(id, LocationComponent{{100.0f, 200.0f}});
        entities.AddComponent(id, SpawnComponent{id, "star", 7, 2, 2, 2});
    }
    SpawnSystem<TEST_COMPONENTS> spawn;
    spawn.Run(&engine);
    Check(entities.GetComponent<SpawnComponent>(0).totalSpawns == 2);
    spawn.Run(&engine);
    int stars = 0;
    for (const auto& entity : entities) {
        const auto id = entity.GetID();
        if (entity.GetType() != "star") {
            continue;
        }
        ++stars;
        Check(entities.HasComponent<LocationComponent>(id));
        Check(entities.HasComponent<MovementComponent>(id));
        Check(entities.HasComponent<GravityComponent>(id));
        Check(entities.HasComponent<CollisionComponent>(id));
        Check(entities.HasComponent<SpriteComponent>(id));
        const auto& location = entities.GetComponent<LocationComponent>(id);
        const auto& movement = entities.GetComponent<MovementComponent>(id);
        Check(location.loc.x == 100.0f && location.loc.y == 136.0f);
        Check(movement.velocity.x >= -5.0f && movement.velocity.x <= 5.0f);
        Check(movement.velocity.y >= -9.0f && movement.velocity.y <= -5.0f);
        Check(!entities.GetComponent<CollisionComponent>(id).staticEntity);
        Check(entities.GetComponent<SpriteComponent>(id).textureID == 7);
    }
    Check(stars == 40);
    for (int frame = 0; frame < 10; ++frame) {
        spawn.Run(&engine);
    }
    stars = 0;
    for (const auto& entity : entities) {
        if (entity.GetType() == "star") {
            ++stars;
        }
    }
    Check(stars == 80);
    Check(entities.GetComponent<SpawnComponent>(0).totalSpawns == 0);

    const auto continuous = entities.CreateEntity("spawner");
    entities.AddComponent(continuous, LocationComponent{{100.0f, 200.0f}});
    entities.AddComponent(continuous, SpawnComponent{continuous, "star", 7, 2, 2, -1});
    spawn.Run(&engine);
    Check(entities.GetComponent<SpawnComponent>(continuous).timeToNextSpawn == 1);
    for (int frame = 1; frame < 50; ++frame) {
        spawn.Run(&engine);
    }
    stars = 0;
    for (const auto& entity : entities) {
        if (entity.GetType() == "star") {
            ++stars;
        }
    }
    Check(stars == 105);  // 80 finite spawns plus 25 continuous spawns.
    Check(entities.GetComponent<SpawnComponent>(continuous).totalSpawns == -1);
    Check(entities.GetComponent<SpawnComponent>(continuous).timeToNextSpawn == 2);

    ScoreComponent score{};
    for (int digit = 0; digit < 10; ++digit) {
        score.digitTextureID[digit] = 100 + digit;
    }
    for (int digit = 0; digit < 3; ++digit) {
        score.digitID[digit] = entities.CreateEntity("digit");
        entities.AddComponent(score.digitID[digit], SpriteComponent{});
    }
    const auto scoreID = entities.CreateEntity("score");
    entities.AddComponent(scoreID, score);
    ScoreSystem<TEST_COMPONENTS> scoreSystem;
    const int values[] = {0, 9, 10, 105, 999, 1000, -1};
    const int displayed[] = {0, 9, 10, 105, 999, 999, 0};
    for (int i = 0; i < 7; ++i) {
        entities.GetComponent<ScoreComponent>(scoreID).score = values[i];
        scoreSystem.Run(&engine);
        int value = displayed[i];
        for (int digit = 2; digit >= 0; --digit) {
            Check(entities.GetComponent<SpriteComponent>(score.digitID[digit]).textureID ==
                  score.digitTextureID[value % 10]);
            value /= 10;
        }
        Check(entities.GetComponent<ScoreComponent>(scoreID).score == values[i]);
    }
    entities.RemoveEntity(score.digitID[0]);
    scoreSystem.Run(&engine);
    std::cout << "Star spawning and score checks passed\n";
}
