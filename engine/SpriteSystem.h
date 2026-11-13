#pragma once

#include <bitset>
#include <unordered_map>

#include "CameraComponent.h"
#include "EntityManager.h"
#include "MathUtil.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpriteSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entityManager = engine->GetEntityManager();
        auto& spriteManager = engine->GetSpriteManager();
        auto& windowManager = engine->GetWindowManager();

        auto window = windowManager.GetWindow();

        for (const Entity& entity: entityManager) {
            const EntityID eid = entity.GetID();

            if (!entityManager.IsValid(eid) ||
                !entityManager.template HasComponent<SpriteComponent>(eid) ||
                !entityManager.template HasComponent<LocationComponent>(eid)
            ) {
                continue;
            }

            auto& spriteComponent = entityManager.template GetComponent<SpriteComponent>(eid);
            auto& locationComponent = entityManager.template GetComponent<LocationComponent>(eid);

            Point2D drawLocation{
                locationComponent.loc.x + spriteComponent.bounds.topLeft.x,
                locationComponent.loc.y + spriteComponent.bounds.topLeft.y
            };

            if (spriteComponent.worldSpace) {
                drawLocation = windowManager.WorldToWindow(drawLocation);
            }

            sf::Sprite& sprite = spriteManager.GetSprite(spriteComponent.textureID);
            sprite.setPosition({drawLocation.x, drawLocation.y});
            window->draw(sprite);
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
