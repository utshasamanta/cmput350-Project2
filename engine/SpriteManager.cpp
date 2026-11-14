#include "SpriteManager.h"

#include <stdexcept>

namespace ECSEngine {

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    auto result = mTextures.try_emplace(texturePath);

    auto textureIterator = result.first;
    const bool wasInserted = result.second;

    if (wasInserted) {
        if (!textureIterator->second.loadFromFile(texturePath)) {
            mTextures.erase(textureIterator);
            throw std::runtime_error("Failed to load texture: " + texturePath);
        }
    }

    const sf::IntRect textureRectangle(
        {static_cast<int>(sourceRect.topLeft.x), static_cast<int>(sourceRect.topLeft.y)},
        {static_cast<int>(sourceRect.width), static_cast<int>(sourceRect.height)});

    mSprites.emplace_back(textureIterator->second, textureRectangle);
    return mSprites.size() - 1;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    if (id >= mSprites.size()) {
        throw std::out_of_range("Invalid SpriteID");
    }
    return mSprites[id];
}

}  // namespace ECSEngine
