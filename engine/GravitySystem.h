#pragma once

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine
{

template <typename... Components>
class GravitySystem : public System<Components...> {
public:
	const std::string & GetName() const override
	{ static std::string name("GravitySystem"); return name; }
	
	tSystemReturn Run(EngineView<Components...> *engine) override
	{
		auto& entityManager = engine->GetEntityManager();

        for (const auto& entity : entityManager) {
            const EntityID eid = entity.GetID();

            if (!entityManager.IsValid(eid) ||
                !entityManager.template HasComponent<MovementComponent>(eid) ||
                !entityManager.template HasComponent<GravityComponent>(eid)
            ) {
                continue;
            }

			auto& gravity = entityManager.template GetComponent<GravityComponent>(eid);
			auto& movement = entityManager.template GetComponent<MovementComponent>(eid);

			if (gravity.disabledTimer > 0) {
				--gravity.disabledTimer;
				continue;
			}

			// Apply gravity to the entity.
			bool isOnGround = false;

			if (entityManager.template HasComponent<CollisionComponent>(eid)) {
				const auto& collision = entityManager.template GetComponent<CollisionComponent>(eid);
				isOnGround = collision.onGround;
			}

			// if the entity is on the ground, we don't apply gravity
			if (isOnGround && movement.velocity.y >= 0.0f) {
				movement.velocity.y = 0.0f;
				continue;
			}

			movement.velocity.x += gravity.acceleration.x;
			movement.velocity.y += gravity.acceleration.y;
		}

		return kContinue;
	}
	
};


}



