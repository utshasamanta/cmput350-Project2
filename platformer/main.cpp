#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <limits>

#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"

std::string gResourcePath = "assets/";
#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

struct TileDefinition {
    ECSEngine::SpriteID spriteID;
    ECSEngine::Rect spriteBounds;
    ECSEngine::Rect collisionBounds;
};

void LoadMap(Engine& engine, const std::string& path, const std::string& file) {
    std::ifstream input(path + file);

    if (!input) {
        throw std::runtime_error("Could not open map: " + path + file);
    }

    auto& sprites = engine.GetSpriteManager();
    auto& entities = engine.GetEntityManager();

    std::string word;
    int dictionaryType;

    input >> word >> dictionaryType;
    input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::unordered_map<char, TileDefinition> tiles;

    std::string line;

    // Read dictionary definitions until the map header.
    while (std::getline(input, line)) {
        if (line.rfind("map ", 0) == 0) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::istringstream entry(line);

        char symbol;
        std::string textureFile;

        float sourceX;
        float sourceY;
        float sourceWidth;
        float sourceHeight;

        entry >> symbol >> textureFile >> sourceX >> sourceY >> sourceWidth >> sourceHeight;

        TileDefinition definition{};

        definition.spriteID = sprites.RegisterTexture(
            path + textureFile, {{sourceX, sourceY}, sourceWidth, sourceHeight});

        definition.spriteBounds = {{0.0f, 0.0f}, sourceWidth, sourceHeight};

        if (dictionaryType == 2) {
            entry >> definition.collisionBounds.topLeft.x >> definition.collisionBounds.topLeft.y >>
                definition.collisionBounds.width >> definition.collisionBounds.height;
        }

        tiles[symbol] = definition;
    }

    // Parse:
    // map origin X Y tile W H size COLUMNS ROWS
    std::istringstream mapHeader(line);

    std::string mapWord;
    std::string originWord;
    std::string tileWord;
    std::string sizeWord;

    float originX;
    float originY;
    float tileWidth;
    float tileHeight;

    std::size_t mapWidth;
    std::size_t mapHeight;

    mapHeader >> mapWord >> originWord >> originX >> originY >> tileWord >> tileWidth >>
        tileHeight >> sizeWord >> mapWidth >> mapHeight;

    // Star image used by the S spawner.
    ECSEngine::SpriteID starSpriteID = 0;

    if (dictionaryType == 2) {
        starSpriteID = sprites.RegisterTexture(
            path + "spritesheet-tiles-default.png",
            {{640.0f, 320.0f}, 64.0f, 64.0f}
        );
    }

    // Read the grid.
    for (std::size_t row = 0; row < mapHeight; ++row) {
        std::getline(input, line);

        for (std::size_t column = 0; column < mapWidth; ++column) {
            const char symbol = column < line.size() ? line[column] : '.';
            const auto tileIterator = tiles.find(symbol);

            // '.' and other undefined symbols are empty.
            if (tileIterator == tiles.end()) {
                continue;
            }

            const TileDefinition& tile = tileIterator->second;

            const float x = originX + column * tileWidth;

            const float y = originY + row * tileHeight;

            const auto entity = entities.CreateEntity(std::string("map-") + symbol);

            entities.AddComponent(entity, ECSEngine::LocationComponent{{x, y}});

            entities.AddComponent(entity, ECSEngine::SpriteComponent{tile.spriteID, tile.spriteBounds, true});

            if (dictionaryType == 2) {
                ECSEngine::CollisionComponent collision{};

                collision.staticEntity = true;
                collision.doesDamage = symbol == 'x';
                collision.collisionBox = tile.collisionBounds;

                collision.previousCollisionBox = {
                    {x + tile.collisionBounds.topLeft.x, y + tile.collisionBounds.topLeft.y},
                    tile.collisionBounds.width,
                    tile.collisionBounds.height
                };

                entities.AddComponent(entity, collision);
            }

            if (dictionaryType == 2 && symbol == 'S') {
                ECSEngine::SpawnComponent spawner{};

                spawner.entityID = entity;
                spawner.spawnType = "star";
                spawner.spawnTextureID = starSpriteID;
                spawner.timeToNextSpawn = 120;
                spawner.timeBetweenSpawns = 120;
                spawner.totalSpawns = -1;  // Keep spawning throughout the game.

                entities.AddComponent(entity, spawner);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    if (!gResourcePath.empty() && gResourcePath.back() != '/') {
        gResourcePath += '/';
    }
    Engine engine(1024, 768, "Platformer");

    engine.GetWindowManager().GetWindow()->setFramerateLimit(60);

    auto& sounds = engine.GetSoundManager();
    sounds.RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
    sounds.RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");
    sounds.RegisterSound(gResourcePath + "sfx_gem.ogg", "collect");
    sounds.RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wall");

    // Load maps
    LoadMap(engine, gResourcePath, "sky.map");
    LoadMap(engine, gResourcePath, "world.map");

    // Place player
    auto& entityManager = engine.GetEntityManager();

    const auto playerSprite = engine.GetSpriteManager().RegisterTexture(
        gResourcePath + "spritesheet-characters-default.png",
        {{0.0f, 640.0f}, 128.0f, 128.0f}
    );

    auto player = entityManager.CreateEntity("player");
    entityManager.AddComponent(player, ECSEngine::LocationComponent{{700.0f, 400.f}});
    entityManager.AddComponent(player, ECSEngine::MovementComponent{{0.0f, 0.0f}});
    entityManager.AddComponent(player, ECSEngine::SpriteComponent{playerSprite, {{0.0f, 0.0f}, 128.0f, 128.0f}, true});
    entityManager.AddComponent(player, ECSEngine::GravityComponent{{0.0f, 1.0f}});

    ECSEngine::CollisionComponent playerCollision{};
    playerCollision.staticEntity = false;
    playerCollision.doesDamage = false;
    playerCollision.collisionBox = {{40.0f, 25.0f}, 48.0f, 95.0f};
    playerCollision.previousCollisionBox = {
        {
            700.0f + playerCollision.collisionBox.topLeft.x, 
            400.0f + playerCollision.collisionBox.topLeft.y
        }, 
        playerCollision.collisionBox.width, 
        playerCollision.collisionBox.height
    };

    entityManager.AddComponent(player, playerCollision);
    entityManager.AddComponent(player, ECSEngine::InputComponent{player});
    entityManager.AddComponent(player, ECSEngine::RespawnComponent{{700.0f, 400.0f}});
    entityManager.AddComponent(player, ECSEngine::CameraFollower{player});

    // Place one collectible outside the left wall for wall-jump testing.
    const auto starSprite = engine.GetSpriteManager().RegisterTexture(
        gResourcePath + "spritesheet-tiles-default.png", {{640.0f, 320.0f}, 64.0f, 64.0f});
    ECSEngine::CreateStar(entityManager, {512.0f, 256.0f}, {0.0f, 0.0f}, starSprite);

    // The sheet stores 9 through 0 down column 13.
    ECSEngine::ScoreComponent score{};
    for (int digit = 0; digit < 10; ++digit) {
        score.digitTextureID[digit] = engine.GetSpriteManager().RegisterTexture(
            gResourcePath + "spritesheet-tiles-default.png",
            {{832.0f, static_cast<float>((13 - digit) * 64)}, 64.0f, 64.0f});
    }
    for (int digit = 0; digit < 3; ++digit) {
        const auto id = entityManager.CreateEntity("score-digit");
        score.digitID[digit] = id;
        entityManager.AddComponent(id, ECSEngine::LocationComponent{{16.0f + digit * 48.0f, 16.0f}});
        entityManager.AddComponent(id, ECSEngine::SpriteComponent{
            score.digitTextureID[0], {{0.0f, 0.0f}, 48.0f, 48.0f}, false});
    }
    const auto scoreEntity = entityManager.CreateEntity("score");
    entityManager.AddComponent(scoreEntity, score);

    const auto camera = entityManager.CreateEntity("camera");
    entityManager.AddComponent(camera, ECSEngine::CameraComponent{
        {764.0f, 384.0f}, 1.0f,
        {{102.4f, 0.0f}, 819.2f, 768.0f},
        {{307.2f, 0.0f}, 409.6f, 768.0f}, 0.08f});
    entityManager.AddComponent(camera, ECSEngine::CameraShake{0, {0.0f, 0.0f}});

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
