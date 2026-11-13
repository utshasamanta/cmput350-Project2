#include "SoundManager.h"

#include <stdexcept>

namespace ECSEngine {

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    if (mSounds.find(soundName) != mSounds.end()) {
        throw std::runtime_error("Sound name already registered: " + soundName);
    }

    auto result = mSoundBuffers.try_emplace(soundPath);
    auto bufferIterator = result.first;
    auto wasInserted = result.second;

    if (wasInserted) {
        if (!bufferIterator->second.loadFromFile(soundPath)) {
            mSoundBuffers.erase(bufferIterator);
            throw std::runtime_error("Failed to load sound: " + soundPath);
        }
    }

    mSoundInstances.emplace_back(bufferIterator->second);
    mSounds[soundName] = mSoundInstances.size() - 1;
}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    const auto iterator = mSounds.find(soundName);
    if (iterator == mSounds.end()) {
        throw std::runtime_error("Sound name not found: " + soundName);
    }

    return mSoundInstances[iterator->second];
}

}  // namespace ECSEngine
