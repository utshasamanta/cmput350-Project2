#pragma once

#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

template <typename... Components>
class EngineView {
public:
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

private:
    bool ProcessEvents();
    void DebugDraw();

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;

    // For debugging - pause and step the engine
    bool running, step;
    int debug;
};

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name) {}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {}

template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();

    while (window->isOpen()) {
        if (!ProcessEvents()) {
            break;
        }

        window->clear(sf::Color::Black);

        if (!mSystemManager.Run(this)) {
            break;
        }

        window->display();
    }
}

template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    auto window = mWindowManager.GetWindow();

    // Before processing new events, preserve the keyboard state from the previous frame.
    for (const Entity& entity : mEntityManager) {
        const EntityID eid = entity.GetID();

        if (!mEntityManager.IsValid(eid) || !mEntityManager.template HasComponent<InputComponent>(eid)) {
            continue;
        }

        auto& input = mEntityManager.template GetComponent<InputComponent>(eid);
        input.lastKeydown = input.keydown;
    }

    // helper used by both KeyPressed and KeyReleased
    const auto setKeyState = [this](sf::Keyboard::Scancode scancode, bool isPressed) {
        const size_t keyIndex = static_cast<size_t>(scancode);

        for (const Entity& entity : mEntityManager) {
            const EntityID eid = entity.GetID();

            if (!mEntityManager.IsValid(eid) || !mEntityManager.template HasComponent<InputComponent>(eid)) {
                continue;
            }

            auto& input = mEntityManager.template GetComponent<InputComponent>(eid);

            if (keyIndex < input.keydown.size()) {
                input.keydown.set(keyIndex, isPressed);
            }
        }
    };


    while (const auto event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window->close();
            return false;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            setKeyState(keyPressed->scancode, true);
        } else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            setKeyState(keyReleased->scancode, false);
        }
    }
    return true;
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {}

}  // namespace ECSEngine
