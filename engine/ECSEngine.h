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
    : mWindowManager(width, height, name), running(false), step(false), debug(0) {}

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

        if (debug != 0) {
            DebugDraw();
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
        if (event->template is<sf::Event::Closed>()) {
            window->close();
            return false;
        }

        if (const auto* keyPressed = event->template getIf<sf::Event::KeyPressed>()) {
            const size_t keyIndex = static_cast<size_t>(keyPressed->scancode);
            bool wasDown = false;

            // Ignore repeated KeyPressed events while a key is held.
            for (const Entity& entity : mEntityManager) {
                const EntityID eid = entity.GetID();

                if (mEntityManager.IsValid(eid) &&
                    mEntityManager.template HasComponent<InputComponent>(eid)) {
                    const auto& input = mEntityManager.template GetComponent<InputComponent>(eid);
                    wasDown = keyIndex < input.keydown.size() && input.keydown[keyIndex];
                    break;
                }
            }

            setKeyState(keyPressed->scancode, true);

            if (!wasDown && keyPressed->scancode == sf::Keyboard::Scancode::F1) {
                debug = debug == 0 ? 1 : 0;
            }

            if (!wasDown && keyPressed->scancode == sf::Keyboard::Scancode::R) {
                for (const Entity& entity : mEntityManager) {
                    const EntityID eid = entity.GetID();

                    if (!mEntityManager.IsValid(eid) ||
                        !mEntityManager.template HasComponent<RespawnComponent>(eid) ||
                        !mEntityManager.template HasComponent<LocationComponent>(eid)) {
                        continue;
                    }

                    const auto& respawn =
                        mEntityManager.template GetComponent<RespawnComponent>(eid);
                    auto& location =
                        mEntityManager.template GetComponent<LocationComponent>(eid);
                    location.loc = respawn.loc;

                    if (mEntityManager.template HasComponent<MovementComponent>(eid)) {
                        auto& movement =
                            mEntityManager.template GetComponent<MovementComponent>(eid);
                        movement.velocity = {0.0f, 0.0f};
                    }

                    if (mEntityManager.template HasComponent<CollisionComponent>(eid)) {
                        auto& collision =
                            mEntityManager.template GetComponent<CollisionComponent>(eid);
                        collision.onGround = false;
                        collision.onCeiling = false;
                        collision.onLeftWall = false;
                        collision.onRightWall = false;
                        collision.previousCollisionBox = {
                            {
                                location.loc.x + collision.collisionBox.topLeft.x,
                                location.loc.y + collision.collisionBox.topLeft.y
                            },
                            collision.collisionBox.width,
                            collision.collisionBox.height
                        };
                    }
                }
            }
        } else if (const auto* keyReleased = event->template getIf<sf::Event::KeyReleased>()) {
            setKeyState(keyReleased->scancode, false);
        }
    }
    return true;
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw() {
    for (const Entity& entity : mEntityManager) {
        const EntityID eid = entity.GetID();

        if (!mEntityManager.IsValid(eid) ||
            !mEntityManager.template HasComponent<LocationComponent>(eid) ||
            !mEntityManager.template HasComponent<CollisionComponent>(eid)) {
            continue;
        }

        const auto& location = mEntityManager.template GetComponent<LocationComponent>(eid);
        const auto& collision = mEntityManager.template GetComponent<CollisionComponent>(eid);
        const Rect worldBox = {
            {
                location.loc.x + collision.collisionBox.topLeft.x,
                location.loc.y + collision.collisionBox.topLeft.y
            },
            collision.collisionBox.width,
            collision.collisionBox.height
        };

        if (collision.doesDamage) {
            mWindowManager.DebugFrameRect(worldBox, 255, 0, 255);
        } else if (collision.staticEntity) {
            mWindowManager.DebugFrameRect(worldBox, 255, 0, 0);
        } else {
            mWindowManager.DebugFrameRect(worldBox, 0, 255, 0);
        }
    }
}

}  // namespace ECSEngine
