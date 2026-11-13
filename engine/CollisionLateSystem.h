#pragma once

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entityManager = engine->GetEntityManager();

        for (const auto& entity : entityManager) {
            const EntityID eid = entity.GetID();

            if (!entityManager.IsValid(eid) ||
                !entityManager.template HasComponent<CollisionComponent>(eid) ||
                !entityManager.template HasComponent<LocationComponent>(eid)
            ) {
                continue;
            }
            
            auto& collision = entityManager.template GetComponent<CollisionComponent>(eid);
            auto& location = entityManager.template GetComponent<LocationComponent>(eid);

            collision.previousCollisionBox = {
                {
                    location.loc.x + collision.collisionBox.topLeft.x,
                    location.loc.y + collision.collisionBox.topLeft.y
                },
                collision.collisionBox.width,
                collision.collisionBox.height
            };

            collision.onGround = false;
            collision.onCeiling = false;
            collision.onLeftWall = false;
            collision.onRightWall = false;
            
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
