#pragma once

#include <algorithm>

#include "Keyboard.h"

namespace ECSEngine {

const float kMaxRunVelocity = 8;
const float kDashVelocity = 50;
const float kJumpVelocity = 25;
const float kGroundRunAcceleration = 2;
const float kAirRunAcceleration = 1;
const float kDashInterval = 60;
const float kDashDuration = 6;
const float kWallJumpVelocity = 12;
const float kWallSlideVelocity = 4;

template <typename... Components>
class InputSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("InputSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entityManager = engine->GetEntityManager();

        for (const Entity& entity : entityManager) {
            const EntityID id = entity.GetID();

            if (!entityManager.IsValid(id) || !entityManager.template HasComponent<InputComponent>(id)) {
                continue;
            }

            auto& input = entityManager.template GetComponent<InputComponent>(id);

            const EntityID controlledID = input.entity;

            if (!entityManager.IsValid(controlledID) ||
                !entityManager.template HasComponent<MovementComponent>(controlledID) ||
                !entityManager.template HasComponent<CollisionComponent>(controlledID)) {
                continue;
            }

            auto& movement = entityManager.template GetComponent<MovementComponent>(controlledID);

            const auto& collision = entityManager.template GetComponent<CollisionComponent>(controlledID);
            const bool left = input.keydown[Key::Left];
            const bool right = input.keydown[Key::Right];
            const bool down = input.keydown[Key::Down];
            const bool jump = input.keydown[Key::Z];
            const bool previousJump = input.lastKeydown[Key::Z];
            const bool dash = input.keydown[Key::C];
            const bool previousDash = input.lastKeydown[Key::C];

            // Decrease dash cooldown every frame.
            if (input.lastDash > 0) {
                --input.lastDash;
            }

            // Preserve dash speed for several frames instead of
            // immediately clamping it back to normal running speed.
            const bool currentlyDashing =
                input.lastDash > kDashInterval - kDashDuration;

            if (currentlyDashing) {
                continue;
            }

            // Horizontal movement.
            const float acceleration = collision.onGround ? kGroundRunAcceleration : kAirRunAcceleration;

            if (left && !right) {
                movement.velocity.x -= acceleration;
            } else if (right && !left) {
                movement.velocity.x += acceleration;
            } else {
                // Gradually slow down when neither
                // direction is pressed.
                if (movement.velocity.x > 0.0f) {
                    movement.velocity.x -= 1.0f;

                    if (movement.velocity.x < 0.0f) {
                        movement.velocity.x = 0.0f;
                    }
                } else if (movement.velocity.x < 0.0f) {
                    movement.velocity.x += 1.0f;

                    if (movement.velocity.x > 0.0f) {
                        movement.velocity.x = 0.0f;
                    }
                }
            }

            movement.velocity.x = std::clamp(movement.velocity.x, -kMaxRunVelocity, kMaxRunVelocity);

            // New Z press.
            bool jumped = false;

            if (jump && !previousJump) {
                if (collision.onGround) {
                    movement.velocity.y = -kJumpVelocity;
                    jumped = true;
                } else if (collision.onLeftWall) {
                    movement.velocity.x = kWallJumpVelocity;

                    movement.velocity.y = -kJumpVelocity;
                    jumped = true;
                } else if (collision.onRightWall) {
                    movement.velocity.x = -kWallJumpVelocity;

                    movement.velocity.y = -kJumpVelocity;
                    jumped = true;
                }
            }

            if (jumped) {
                engine->GetSoundManager().GetSound("jump").play();
            }

            // Z was released while moving upward.
            if (!jump && previousJump && movement.velocity.y < 0.0f) {
                movement.velocity.y *= 0.45f;
            }

            // Slow wall slide.
            if (movement.velocity.y > 0.0f) {
                const bool slidingOnLeft = collision.onLeftWall && left;
                const bool slidingOnRight = collision.onRightWall && right;

                if ((slidingOnLeft || slidingOnRight) && movement.velocity.y > kWallSlideVelocity) {
                    movement.velocity.y = kWallSlideVelocity;
                }
            }

            // Faster downward movement.
            if (down && movement.velocity.y > 0.0f) {
                movement.velocity.y += 1.0f;
            }

            // New C press with completed cooldown.
            if (dash && !previousDash && input.lastDash == 0) {
                float direction = 1.0f;

                if (left) {
                    direction = -1.0f;
                } else if (right) {
                    direction = 1.0f;
                } else if (movement.velocity.x < 0.0f) {
                    direction = -1.0f;
                }

                movement.velocity.x = direction * kDashVelocity;
                movement.velocity.y = 0.0f;

                input.lastDash = kDashInterval;
            }
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
