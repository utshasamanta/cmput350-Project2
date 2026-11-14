#pragma once

#include <bitset>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Window/Keyboard.hpp>

namespace ECSEngine {

struct InputComponent {
    InputComponent(EntityID whichEntity)
        : entity(whichEntity), lastDash(0), jumpFrames(0), wallJumpFrames(0), wallSide(0) {}
    EntityID entity;
    int lastDash;
    int jumpFrames;
    int wallJumpFrames;
    int wallSide;
    std::bitset<sf::Keyboard::ScancodeCount> keydown;
    std::bitset<sf::Keyboard::ScancodeCount> lastKeydown;
};

}  // namespace ECSEngine
