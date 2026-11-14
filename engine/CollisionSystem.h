#pragma once

#include <algorithm>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        for (const Entity& dynamicEntity : entities) {
            const EntityID dynamicID = dynamicEntity.GetID();

            if (!entities.IsValid(dynamicID)) {
                continue;
            }

            if (!entities.template HasComponent<LocationComponent>(dynamicID) ||
                !entities.template HasComponent<CollisionComponent>(dynamicID)) {
                continue;
            }

            auto& dynamicCollision = entities.template GetComponent<CollisionComponent>(dynamicID);

            // Only move and resolve dynamic objects.
            if (dynamicCollision.staticEntity) {
                continue;
            }

            dynamicCollision.onGround = false;
            dynamicCollision.onCeiling = false;
            dynamicCollision.onLeftWall = false;
            dynamicCollision.onRightWall = false;

            auto& dynamicLocation = entities.template GetComponent<LocationComponent>(dynamicID);

            for (const Entity& staticEntity : entities) {
                const EntityID staticID = staticEntity.GetID();

                if (staticID == dynamicID || !entities.IsValid(staticID)) {
                    continue;
                }

                if (!entities.template HasComponent<LocationComponent>(staticID) ||
                    !entities.template HasComponent<CollisionComponent>(staticID)) {
                    continue;
                }

                const auto& staticCollision =
                    entities.template GetComponent<CollisionComponent>(staticID);

                if (!staticCollision.staticEntity) {
                    continue;
                }

                const auto& staticLocation =
                    entities.template GetComponent<LocationComponent>(staticID);

                Rect dynamicBox = MakeWorldBox(dynamicLocation, dynamicCollision);

                const Rect staticBox = MakeWorldBox(staticLocation, staticCollision);

                if (!Overlaps(dynamicBox, staticBox)) {
                    continue;
                }

                // Damage tiles are currently spikes.
                if (staticCollision.doesDamage &&
                    entities.template HasComponent<RespawnComponent>(dynamicID)) {

                    const auto& respawn = entities.template GetComponent<RespawnComponent>(dynamicID);
                    dynamicLocation.loc = respawn.loc;

                    if (entities.template HasComponent<MovementComponent>(dynamicID)) {
                        auto& movement = entities.template GetComponent<MovementComponent>(dynamicID);
                        movement.velocity = {0.0f, 0.0f};
                    }

                    // The player moved, so stop testing this
                    // old position against other tiles.
                    break;
                }

                ResolveStaticCollision(entities, dynamicID, dynamicLocation, dynamicCollision, staticBox);
            }
        }

        return kContinue;
    }

private:
    static Rect MakeWorldBox(const LocationComponent& location, const CollisionComponent& collision) {
        return {
            {
                location.loc.x + collision.collisionBox.topLeft.x,
                location.loc.y + collision.collisionBox.topLeft.y
            },
            collision.collisionBox.width,
            collision.collisionBox.height
        };
    }

    static bool Overlaps(const Rect& first, const Rect& second) {
        const bool separate = 
            first.topLeft.x + first.width < second.topLeft.x ||
            first.topLeft.x > second.topLeft.x + second.width ||
            first.topLeft.y + first.height < second.topLeft.y ||
            first.topLeft.y > second.topLeft.y + second.height;
        
            return !separate;
    }

    static void ResolveStaticCollision(
        EntityManager<Components...>& entities, EntityID dynamicID,
        LocationComponent& location, CollisionComponent& collision, const Rect& staticBox) {

        Rect currentBox = MakeWorldBox(location, collision);
        const Rect& previousBox = collision.previousCollisionBox;

        MovementComponent* movement = nullptr;

        if (entities.template HasComponent<MovementComponent>(dynamicID)) {
            movement = &entities.template GetComponent<MovementComponent>(dynamicID);
        }

        const float staticRight = staticBox.topLeft.x + staticBox.width;
        const float staticBottom = staticBox.topLeft.y + staticBox.height;
        const float previousRight = previousBox.topLeft.x + previousBox.width;
        const float previousBottom = previousBox.topLeft.y + previousBox.height;

        // Landed on top of the static object.
        if (previousBottom <= staticBox.topLeft.y) {
            location.loc.y = staticBox.topLeft.y - collision.collisionBox.topLeft.y - collision.collisionBox.height;

            collision.onGround = true;

            if (movement != nullptr && movement->velocity.y > 0.0f) {
                movement->velocity.y = 0.0f;
            }

            return;
        }

        // Hit the bottom of the static object.
        if (previousBox.topLeft.y >= staticBottom) {
            location.loc.y = staticBottom - collision.collisionBox.topLeft.y;

            collision.onCeiling = true;

            if (movement != nullptr && movement->velocity.y < 0.0f) {
                movement->velocity.y = 0.0f;
            }

            return;
        }

        // Dynamic object approached from the left.
        if (previousRight <= staticBox.topLeft.x) {
            location.loc.x = staticBox.topLeft.x - collision.collisionBox.topLeft.x - collision.collisionBox.width;

            collision.onRightWall = true;

            if (movement != nullptr && movement->velocity.x > 0.0f) {
                movement->velocity.x = 0.0f;
            }

            return;
        }

        // Dynamic object approached from the right.
        if (previousBox.topLeft.x >= staticRight) {
            location.loc.x = staticRight - collision.collisionBox.topLeft.x;

            collision.onLeftWall = true;

            if (movement != nullptr && movement->velocity.x < 0.0f) {
                movement->velocity.x = 0.0f;
            }

            return;
        }

        // Fallback for an entity already overlapping a
        // tile. Push it out along the smallest overlap.
        currentBox = MakeWorldBox(location, collision);
        const float overlapLeft = currentBox.topLeft.x + currentBox.width - staticBox.topLeft.x;
        const float overlapRight = staticRight - currentBox.topLeft.x;
        const float overlapTop = currentBox.topLeft.y + currentBox.height - staticBox.topLeft.y;
        const float overlapBottom = staticBottom - currentBox.topLeft.y;
        const float horizontalOverlap = std::min(overlapLeft, overlapRight);
        const float verticalOverlap = std::min(overlapTop, overlapBottom);

        if (verticalOverlap < horizontalOverlap) {
            if (overlapTop < overlapBottom) {
                location.loc.y -= overlapTop;
                collision.onGround = true;

                if (movement != nullptr) {
                    movement->velocity.y = 0.0f;
                }
            } else {
                location.loc.y += overlapBottom;
                collision.onCeiling = true;

                if (movement != nullptr) {
                    movement->velocity.y = 0.0f;
                }
            }
        } else {
            if (overlapLeft < overlapRight) {
                location.loc.x -= overlapLeft;
                collision.onRightWall = true;
            } else {
                location.loc.x += overlapRight;
                collision.onLeftWall = true;
            }

            if (movement != nullptr) {
                movement->velocity.x = 0.0f;
            }
        }
    }
};

}  // namespace ECSEngine