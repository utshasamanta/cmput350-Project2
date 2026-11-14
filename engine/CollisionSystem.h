#pragma once

#include <algorithm>
#include <cmath>
#include <utility>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
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

        CollectStars(entities, engine->GetSoundManager());
        ResolveStarCollisions(entities);

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

            const bool wasOnGround = dynamicCollision.onGround;
            const bool wasOnLeftWall = dynamicCollision.onLeftWall;
            const bool wasOnRightWall = dynamicCollision.onRightWall;
            Point2D startingVelocity{0.0f, 0.0f};

            if (entities.template HasComponent<MovementComponent>(dynamicID)) {
                startingVelocity = entities.template GetComponent<MovementComponent>(dynamicID).velocity;
            }

            dynamicCollision.onGround = false;
            dynamicCollision.onCeiling = false;
            dynamicCollision.onLeftWall = false;
            dynamicCollision.onRightWall = false;

            auto& dynamicLocation = entities.template GetComponent<LocationComponent>(dynamicID);
            bool respawned = false;

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
                    respawned = true;
                    break;
                }

                const bool groundBefore = dynamicCollision.onGround;
                const bool ceilingBefore = dynamicCollision.onCeiling;
                const bool leftWallBefore = dynamicCollision.onLeftWall;
                const bool rightWallBefore = dynamicCollision.onRightWall;
                Point2D velocityBefore{0.0f, 0.0f};

                if (entities.template HasComponent<MovementComponent>(dynamicID)) {
                    velocityBefore = entities.template GetComponent<MovementComponent>(dynamicID).velocity;
                }

                ResolveStaticCollision(entities, dynamicID, dynamicLocation, dynamicCollision, staticBox);

                if (dynamicEntity.GetType() == "star" &&
                    entities.template HasComponent<MovementComponent>(dynamicID)) {
                    auto& movement = entities.template GetComponent<MovementComponent>(dynamicID);

                    const bool hitFloorOrCeiling =
                        (!groundBefore && dynamicCollision.onGround) ||
                        (!ceilingBefore && dynamicCollision.onCeiling);
                    const bool hitWall =
                        (!leftWallBefore && dynamicCollision.onLeftWall) ||
                        (!rightWallBefore && dynamicCollision.onRightWall);

                    if (hitFloorOrCeiling) {
                        movement.velocity.y = -velocityBefore.y * 0.65f;

                        if (std::abs(movement.velocity.y) < 1.0f) {
                            movement.velocity.y = 0.0f;
                        }
                    }

                    if (hitWall) {
                        movement.velocity.x = -velocityBefore.x * 0.65f;

                        if (std::abs(movement.velocity.x) < 1.0f) {
                            movement.velocity.x = 0.0f;
                        }
                    }
                }
            }

            if (dynamicEntity.GetType() == "player" && !respawned) {
                const bool landed = !wasOnGround && dynamicCollision.onGround;
                const bool touchedWall =
                    (!wasOnLeftWall && dynamicCollision.onLeftWall) ||
                    (!wasOnRightWall && dynamicCollision.onRightWall);

                if (landed && startingVelocity.y > 2.0f) {
                    engine->GetSoundManager().GetSound("land").play();
                }

                if (touchedWall && startingVelocity.y > 0.0f) {
                    engine->GetSoundManager().GetSound("wall").play();
                }

                if (landed || touchedWall || dynamicCollision.onCeiling) {
                    StartCameraShake(entities, startingVelocity, touchedWall);
                }
            }
        }

        return kContinue;
    }

private:
    static void CollectStars(EntityManager<Components...>& entities, SoundManager& sounds) {
        for (const Entity& player : entities) {
            const EntityID playerID = player.GetID();

            if (player.GetType() != "player" || !entities.IsValid(playerID) ||
                !entities.template HasComponent<LocationComponent>(playerID) ||
                !entities.template HasComponent<CollisionComponent>(playerID)) {
                continue;
            }

            const Rect playerBox = MakeWorldBox(
                entities.template GetComponent<LocationComponent>(playerID),
                entities.template GetComponent<CollisionComponent>(playerID));

            for (const Entity& star : entities) {
                const EntityID starID = star.GetID();

                if (star.GetType() != "star" || !entities.IsValid(starID) ||
                    !entities.template HasComponent<LocationComponent>(starID) ||
                    !entities.template HasComponent<CollisionComponent>(starID)) {
                    continue;
                }

                const Rect starBox = MakeWorldBox(
                    entities.template GetComponent<LocationComponent>(starID),
                    entities.template GetComponent<CollisionComponent>(starID));

                if (!Overlaps(playerBox, starBox)) {
                    continue;
                }

                for (const Entity& scoreEntity : entities) {
                    const EntityID scoreID = scoreEntity.GetID();

                    if (entities.IsValid(scoreID) &&
                        entities.template HasComponent<ScoreComponent>(scoreID)) {
                        ++entities.template GetComponent<ScoreComponent>(scoreID).score;
                        break;
                    }
                }

                sounds.GetSound("collect").play();
                entities.RemoveEntity(starID);
            }

            break;
        }
    }

    static void ResolveStarCollisions(EntityManager<Components...>& entities) {
        for (const Entity& first : entities) {
            const EntityID firstID = first.GetID();

            if (first.GetType() != "star" || !HasMovingCollision(entities, firstID)) {
                continue;
            }

            for (const Entity& second : entities) {
                const EntityID secondID = second.GetID();

                if (secondID <= firstID || second.GetType() != "star" ||
                    !HasMovingCollision(entities, secondID)) {
                    continue;
                }

                auto& firstLocation = entities.template GetComponent<LocationComponent>(firstID);
                auto& secondLocation = entities.template GetComponent<LocationComponent>(secondID);
                const auto& firstCollision = entities.template GetComponent<CollisionComponent>(firstID);
                const auto& secondCollision = entities.template GetComponent<CollisionComponent>(secondID);
                Rect firstBox = MakeWorldBox(firstLocation, firstCollision);
                Rect secondBox = MakeWorldBox(secondLocation, secondCollision);

                if (!Overlaps(firstBox, secondBox)) {
                    continue;
                }

                auto& firstMovement = entities.template GetComponent<MovementComponent>(firstID);
                auto& secondMovement = entities.template GetComponent<MovementComponent>(secondID);
                std::swap(firstMovement.velocity, secondMovement.velocity);
                firstMovement.velocity.x *= 0.8f;
                firstMovement.velocity.y *= 0.8f;
                secondMovement.velocity.x *= 0.8f;
                secondMovement.velocity.y *= 0.8f;

                const float overlapX = std::min(
                    firstBox.topLeft.x + firstBox.width - secondBox.topLeft.x,
                    secondBox.topLeft.x + secondBox.width - firstBox.topLeft.x);
                const float overlapY = std::min(
                    firstBox.topLeft.y + firstBox.height - secondBox.topLeft.y,
                    secondBox.topLeft.y + secondBox.height - firstBox.topLeft.y);

                if (overlapX < overlapY) {
                    const float direction = firstBox.topLeft.x < secondBox.topLeft.x ? -1.0f : 1.0f;
                    firstLocation.loc.x += direction * overlapX * 0.5f;
                    secondLocation.loc.x -= direction * overlapX * 0.5f;
                } else {
                    const float direction = firstBox.topLeft.y < secondBox.topLeft.y ? -1.0f : 1.0f;
                    firstLocation.loc.y += direction * overlapY * 0.5f;
                    secondLocation.loc.y -= direction * overlapY * 0.5f;
                }
            }
        }
    }

    static bool HasMovingCollision(EntityManager<Components...>& entities, EntityID id) {
        return entities.IsValid(id) &&
               entities.template HasComponent<LocationComponent>(id) &&
               entities.template HasComponent<CollisionComponent>(id) &&
               entities.template HasComponent<MovementComponent>(id);
    }

    static void StartCameraShake(EntityManager<Components...>& entities,
                                 const Point2D& velocity, bool hitWall) {
        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();

            if (!entities.IsValid(id) || !entities.template HasComponent<CameraShake>(id)) {
                continue;
            }

            auto& shake = entities.template GetComponent<CameraShake>(id);
            shake.frameLimit = 4;
            shake.offset = {0.0f, 0.0f};

            if (hitWall) {
                shake.offset.x = std::min(std::abs(velocity.x) * 0.2f, 6.0f);
            } else {
                shake.offset.y = std::min(std::abs(velocity.y) * 0.2f, 6.0f);
            }

            break;
        }
    }

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
            first.topLeft.x + first.width <= second.topLeft.x ||
            first.topLeft.x >= second.topLeft.x + second.width ||
            first.topLeft.y + first.height <= second.topLeft.y ||
            first.topLeft.y >= second.topLeft.y + second.height;

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
