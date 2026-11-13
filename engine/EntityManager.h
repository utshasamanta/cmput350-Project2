#pragma once

#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Component.h"
#include "Entity.h"

namespace ECSEngine {

template <typename... Components>
class EntityManager {
public:
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        const EntityID id = mEntities.size();

        mEntities.push_back(Entity(id, name));
        mEntityValid.push_back(true);

        ComponentIndexArray indices;
        indices.fill(kNullComponent);
        mComponentIndices.push_back(indices);

        return id;
    }

    void RemoveEntity(EntityID entity) {
        if (!IsValid(entity)) {
            return;
        }

        (RemoveComponent<Components>(entity), ...);
        mEntityValid[entity] = false;
    }

    template <typename T>
    void AddComponent(EntityID entity, T component) {
        if (!IsValid(entity)) {
            std::cerr << "Error: Invalid entity ID " << entity << std::endl;
            return;
        }

        const size_t componentIndex = GetComponentIndex<T>();
        if (componentIndex == kNullComponent) {
            std::cerr << "Error: Component type not found for entity " << entity << std::endl;
            return;
        }

        if (HasComponent<T>(entity)) {
            GetComponent<T>(entity) = std::move(component);
            return;
        }

        auto& storage = GetComponentStorage<T>();

        if (storage.mFreeList.empty()) {
            storage.mStorage.push_back(std::move(component));
            storage.mValid.push_back(true);
            mComponentIndices[entity][componentIndex] = storage.mStorage.size() - 1;
        } else {
            size_t freeIndex = storage.mFreeList.back();
            storage.mFreeList.pop_back();
            storage.mStorage[freeIndex] = std::move(component);
            storage.mValid[freeIndex] = true;
            mComponentIndices[entity][componentIndex] = freeIndex;
        }
    }

    template <typename T>
    void RemoveComponent(EntityID entity) {
        if (!IsValid(entity)) {
            return;
        }

        if (!HasComponent<T>(entity)) {
            return;
        }

        const size_t componentIndex = GetComponentIndex<T>();
        if (componentIndex == kNullComponent) {
            std::cerr << "Error: Component type not found for entity " << entity << std::endl;
            return;
        }

        size_t componentStorageIndex = mComponentIndices[entity][componentIndex];

        mComponentIndices[entity][componentIndex] = kNullComponent;
        auto& storage = GetComponentStorage<T>();
        storage.mValid[componentStorageIndex] = false;
        storage.mFreeList.push_back(componentStorageIndex);
    }

    template <typename T>
    bool HasComponent(EntityID entity) const {
        if (!IsValid(entity)) {
            return false;
        }

        constexpr size_t componentIndex = GetComponentIndex<T>();
        if (componentIndex == kNullComponent) {
            std::cerr << "Error: Component type not found for entity " << entity << std::endl;
            return false;  // Component type not found
        }

        return mComponentIndices[entity][componentIndex] != kNullComponent;
    }

    template <typename T>
    T& GetComponent(EntityID entity) {
        if (!IsValid(entity)) {
            throw std::runtime_error("Invalid entity ID");
        }

        if (!HasComponent<T>(entity)) {
            throw std::runtime_error("Entity does not have that component type");
        }

        size_t componentIndex = GetComponentIndex<T>();
        size_t componentStorageIndex = mComponentIndices[entity][componentIndex];

        auto& storage = GetComponentStorage<T>();
        return storage.mStorage[componentStorageIndex];
    }

    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        return std::get<ComponentStorage<T>>(mComponentStorages);
    }

    bool IsValid(EntityID entity) const {
        return entity < mEntities.size() && mEntityValid[entity];
    }

    using tEntity = std::vector<Entity>;
    using const_iterator = tEntity::const_iterator;
    const_iterator cbegin() const { return mEntities.cbegin(); }
    const_iterator cend() const { return mEntities.cend(); }
    const_iterator begin() { return mEntities.begin(); }
    const_iterator end() { return mEntities.end(); }

private:
    template <typename T>
    static constexpr size_t GetComponentIndex() {
        constexpr bool matches[] = {std::is_same_v<T, Components>...};

        for (std::size_t index = 0; index < sizeof...(Components); ++index) {
            if (matches[index]) {
                return index;
            }
        }
        return kNullComponent;  // Component type not found
    }

    using ComponentIndexArray = std::array<size_t, sizeof...(Components)>;

    tEntity mEntities;
    std::vector<bool> mEntityValid;  // Quickly track if an entity is valid
    std::tuple<ComponentStorage<Components>...> mComponentStorages;
    std::vector<ComponentIndexArray> mComponentIndices;  // Store component indices for each entity
};
}  // namespace ECSEngine
