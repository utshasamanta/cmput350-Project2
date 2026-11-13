#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Audio.hpp>

namespace ECSEngine {

class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;
    void RegisterSound(const std::string& soundPath, const std::string& soundName);
    sf::Sound& GetSound(const std::string& soundName);

private:
    std::unordered_map<std::string, sf::SoundBuffer> mSoundBuffers;
    std::unordered_map<std::string, size_t> mSounds;
    std::vector<sf::Sound> mSoundInstances;  // Store sound instances to keep them alive while playing
};

}  // namespace ECSEngine
