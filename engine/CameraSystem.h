#pragma once

#include <algorithm>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "CameraSystem";
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();
        auto& window = engine->GetWindowManager();

        for (const auto& entity : entities) {
            const auto id = entity.GetID();
            if (!entities.template HasComponent<CameraComponent>(id)) {
                continue;
            }
            auto& camera = entities.template GetComponent<CameraComponent>(id);
            window.SetWorldScale(camera.worldUnitsPerPixel);
            window.SetWorldCenter(camera.position);

            for (const auto& followed : entities) {
                const auto followedID = followed.GetID();
                if (!entities.template HasComponent<CameraFollower>(followedID)) {
                    continue;
                }
                const auto target = entities.template GetComponent<CameraFollower>(followedID).entityID;
                if (entities.template HasComponent<LocationComponent>(target)) {
                    auto position = entities.template GetComponent<LocationComponent>(target).loc;
                    float width = 0.0f;
                    if (entities.template HasComponent<SpriteComponent>(target)) {
                        const auto& sprite = entities.template GetComponent<SpriteComponent>(target);
                        position.x += sprite.bounds.topLeft.x;
                        width = sprite.bounds.width;
                    }
                    const float left = window.WorldToWindow(position).x;
                    const float right = left + width / camera.worldUnitsPerPixel;
                    const float softLeft = camera.softExclusion.topLeft.x;
                    const float softRight = softLeft + camera.softExclusion.width;
                    const float hardLeft = camera.hardExclusion.topLeft.x;
                    const float hardRight = hardLeft + camera.hardExclusion.width;
                    float shift = 0.0f;
                    if (left < softLeft) {
                        shift = (left - softLeft) * camera.speed;
                    } else if (right > softRight) {
                        shift = (right - softRight) * camera.speed;
                    }
                    // Clamp the whole player inside the outer camera limits immediately.
                    shift = std::clamp(shift, right - hardRight, left - hardLeft);
                    camera.position.x += shift * camera.worldUnitsPerPixel;
                    break;  // This game follows one player.
                }
            }

            auto center = camera.position;
            if (entities.template HasComponent<CameraShake>(id)) {
                auto& shake = entities.template GetComponent<CameraShake>(id);
                if (shake.frameLimit > 0) {
                    const float direction = shake.frameLimit % 2 == 0 ? 1.0f : -1.0f;
                    center.x += direction * shake.offset.x;
                    center.y += direction * shake.offset.y;
                    --shake.frameLimit;
                    if (shake.frameLimit == 0) {
                        shake.offset = {0.0f, 0.0f};
                    }
                }
            }
            window.SetWorldCenter(center);
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
