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

std::string gResourcePath = "../assets/";
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
            const char symbol = line[column];
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

            if (symbol == 'S') {
                ECSEngine::SpawnComponent spawner{};

                spawner.entityID = entity;
                spawner.spawnType = "star";
                spawner.spawnTextureID = starSpriteID;
                spawner.timeToNextSpawn = 120;
                spawner.timeBetweenSpawns = 120;
                spawner.totalSpawns = 20;

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

    // Engine test code
    Engine engine(1024, 768, "Test Engine");

    // Load sounds

    // Load maps
    LoadMap(engine, gResourcePath, "sky.map");
    LoadMap(engine, gResourcePath, "world.map");

    // Place player
    auto& entityManager = engine.GetEntityManager();

    const auto playerSprite = engine.GetSpriteManager().RegisterTexture(
        gResourcePath + "spritesheet-characters-default.png",
        {{0.0f, 0.0f}, 128.0f, 128.0f}
    );

    auto player = entityManager.CreateEntity("player");
    entityManager.AddComponent(player, ECSEngine::LocationComponent{{700.0f, 400.f}});
    entityManager.AddComponent(player, ECSEngine::MovementComponent{{1.0f, 0.0f}});
    entityManager.AddComponent(player, ECSEngine::SpriteComponent{playerSprite, {{0.0f, 0.0f}, 128.0f, 128.0f}, true});
    entityManager.AddComponent(player, ECSEngine::GravityComponent{{0.0f, 1.0f}});

    ECSEngine::CollisionComponent playerCollision{};
    playerCollision.staticEntity = false;
    playerCollision.doesDamage = false;
    playerCollision.collisionBox = {{25.0f, 15.0f}, 78.0f, 110.0f};
    playerCollision.previousCollisionBox = {{725.0f, 415.0f}, 78.0f, 110.0f};

    entityManager.AddComponent(player, playerCollision);
    entityManager.AddComponent(player, ECSEngine::InputComponent{player});
    entityManager.AddComponent(player, ECSEngine::RespawnComponent{{700.0f, 400.0f}});

    // Setup score component

    // Setup Camera

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
