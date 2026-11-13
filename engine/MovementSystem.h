#pragma once

namespace ECSEngine {

template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entityManager = engine->GetEntityManager();

        for (const auto& entity : entityManager) {
            const EntityID eid = entity.GetID();

            if (!entityManager.IsValid(eid) ||
                !entityManager.template HasComponent<MovementComponent>(eid) ||
                !entityManager.template HasComponent<LocationComponent>(eid)
            ) {
                continue;
            }

            auto& location = entityManager.template GetComponent<LocationComponent>(eid);
            auto& movement = entityManager.template GetComponent<MovementComponent>(eid);

            location.loc.x += movement.velocity.x;
            location.loc.y += movement.velocity.y;
        }

        return kContinue;
    }
};

}  // namespace ECSEngine
