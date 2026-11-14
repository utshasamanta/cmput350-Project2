#pragma once

#include <algorithm>

#include "ScoreComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class ScoreSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("ScoreSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();
        for (const auto& entity : entities) {
            const auto id = entity.GetID();
            if (!entities.template HasComponent<ScoreComponent>(id)) {
                continue;
            }
            const auto& score = entities.template GetComponent<ScoreComponent>(id);
            int value = std::clamp(score.score, 0, 999);
            for (int digit = 2; digit >= 0; --digit) {
                if (entities.template HasComponent<SpriteComponent>(score.digitID[digit])) {
                    auto& sprite = entities.template GetComponent<SpriteComponent>(score.digitID[digit]);
                    sprite.textureID = score.digitTextureID[value % 10];
                }
                value /= 10;
            }
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
