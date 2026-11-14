#pragma once

#include <random>
#include <vector>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
void CreateStar(EntityManager<Components...>& entities, Point2D position,
                Point2D velocity, SpriteID texture) {
    const auto star = entities.CreateEntity("star");
    entities.AddComponent(star, LocationComponent{position});
    entities.AddComponent(star, MovementComponent{velocity});
    entities.AddComponent(star, GravityComponent{{0.0f, 0.25f}});
    CollisionComponent collision{};
    collision.collisionBox = {{8.0f, 8.0f}, 48.0f, 48.0f};
    collision.previousCollisionBox = {{position.x + 8.0f, position.y + 8.0f}, 48.0f, 48.0f};
    entities.AddComponent(star, collision);
    entities.AddComponent(star, SpriteComponent{texture, {{0.0f, 0.0f}, 64.0f, 64.0f}, true});
}

template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();
        struct PendingStar {
            Point2D position;
            SpriteID texture;
        };
        std::vector<PendingStar> pending;

        for (const auto& entity : entities) {
            const auto id = entity.GetID();
            if (!entities.template HasComponent<SpawnComponent>(id)) {
                continue;
            }
            auto& spawn = entities.template GetComponent<SpawnComponent>(id);
            if (spawn.spawnType != "star" || spawn.totalSpawns == 0 ||
                !entities.template HasComponent<LocationComponent>(spawn.entityID)) {
                continue;
            }
            if (spawn.timeToNextSpawn > 0) {
                --spawn.timeToNextSpawn;
            }
            if (spawn.timeToNextSpawn > 0) {
                continue;
            }
            auto position = entities.template GetComponent<LocationComponent>(spawn.entityID).loc;
            position.y -= 64.0f;
            pending.push_back({position, spawn.spawnTextureID});
            if (spawn.totalSpawns > 0) {
                --spawn.totalSpawns;
            }
            spawn.timeToNextSpawn = spawn.timeBetweenSpawns;
        }

        // Creating entities can invalidate iterators and component references.
        for (const auto& star : pending) {
            CreateStar(entities, star.position, {mHorizontal(mRandom), mVertical(mRandom)}, star.texture);
        }
        return kContinue;
    }

private:
    std::mt19937 mRandom{std::random_device{}()};
    std::uniform_real_distribution<float> mHorizontal{-5.0f, 5.0f};
    std::uniform_real_distribution<float> mVertical{-9.0f, -5.0f};
};

}  // namespace ECSEngine
