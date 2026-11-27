#include "LevelLoader.h"

#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

// Spawner tuning, in frames (60 per second).
const int kFirstSpawnDelay = 60;
const int kFramesBetweenSpawns = 180;
const int kTotalSpawns = 20;

// One dictionary line of a map file.
struct MapTile {
    ECSEngine::SpriteID sprite;
    bool solid;               // dictionary 2: the tile has a collision box
    ECSEngine::Rect collide;  // relative to the tile's top-left corner
};

}  // namespace

// Map files, one entity per tile. Sprites go in the engine's shared SpriteManager.
//
// File format (from the project spec)
//   dictionary <1|2>
//   <char> <sheet> <sprite rect> [<collision box>]   one line per tile type
//   map origin <left top> tile <width height> size <columns rows>
//   <the grid: one character per tile, unknown characters are empty>
//
// Every tile gets a location and a sprite. With "dictionary 2" tiles are also
// solid, and 'S' tiles also spawn stars.
std::vector<MapMarker> LoadMap(EngineViewT& engine, Scene& scene, const std::string& path,
                               const std::string& file, int layer,
                               const std::string& markerSymbols) {
    std::ifstream in(path + file);
    if (!in) throw std::runtime_error("LoadMap: could not open '" + path + file + "'");
    auto fail = [&](const std::string& why) {
        throw std::runtime_error("LoadMap: " + file + ": " + why);
    };

    auto& sprites = engine.GetSpriteManager();
    auto& entities = scene.GetEntityManager();

    // 1. Dictionary: map each character to a sprite (and collision box).
    std::string word;
    int dictionaryType = 0;
    if (!(in >> word >> dictionaryType) || word != "dictionary" ||
        (dictionaryType != 1 && dictionaryType != 2)) {
        fail("expected 'dictionary 1' or 'dictionary 2'");
    }
    std::unordered_map<char, MapTile> dictionary;
    while (in >> word && word != "map") {
        if (word.size() != 1) fail("tile name '" + word + "' must be one character");
        std::string sheet;
        ECSEngine::Rect source{};
        in >> sheet >> source.topLeft.x >> source.topLeft.y >> source.width >> source.height;
        MapTile tile{0, dictionaryType == 2, {}};
        if (tile.solid) {
            in >> tile.collide.topLeft.x >> tile.collide.topLeft.y >> tile.collide.width >>
                tile.collide.height;
        }
        if (!in) fail("bad dictionary line for '" + word + "'");
        tile.sprite = sprites.RegisterTexture(path + sheet, source);
        dictionary[word[0]] = tile;
    }

    // 2. Header: "map origin <left top> tile <width height> size <columns rows>".
    std::string originWord, tileWord, sizeWord;
    ECSEngine::Point2D origin;
    float tileWidth = 0, tileHeight = 0;
    int columns = 0, rows = 0;
    in >> originWord >> origin.x >> origin.y >> tileWord >> tileWidth >> tileHeight >> sizeWord >>
        columns >> rows;
    if (!in || word != "map" || originWord != "origin" || tileWord != "tile" ||
        sizeWord != "size") {
        fail("bad 'map origin ... tile ... size ...' line");
    }

    // Only registered when the map has a spawner, so sky.map adds no extra sprite.
    std::optional<ECSEngine::SpriteID> starSprite;
    std::vector<MapMarker> markers;

    // 3. Grid: one entity per dictionary character.
    for (int row = 0; row < rows; ++row) {
        std::string line;
        if (!(in >> line) || static_cast<int>(line.size()) < columns) {
            fail("row " + std::to_string(row) + " is missing or too short");
        }
        for (int col = 0; col < columns; ++col) {
            const char symbol = line[col];
            const ECSEngine::Point2D loc{origin.x + col * tileWidth, origin.y + row * tileHeight};
            if (markerSymbols.find(symbol) != std::string::npos) {
                markers.push_back({symbol, loc});
                continue;
            }
            auto found = dictionary.find(symbol);
            if (found == dictionary.end()) continue;  // e.g. '.', empty space
            const MapTile& tile = found->second;

            ECSEngine::EntityID id = entities.CreateEntity(file + " " + symbol);
            entities.AddComponent(id, ECSEngine::LocationComponent{loc});
            // Sprite and collision boxes are relative to the location (see LocationComponent).
            entities.AddComponent(
                id, ECSEngine::SpriteComponent{tile.sprite, {{0.f, 0.f}, tileWidth, tileHeight},
                                               true, layer});
            if (tile.solid) {
                ECSEngine::CollisionComponent collision{};
                collision.staticEntity = 1;
                collision.doesDamage = (symbol == 'x');
                collision.collisionBox = tile.collide;
                collision.previousCollisionBox = tile.collide;
                entities.AddComponent(id, collision);
            }
            if (symbol == 'S') {
                if (!starSprite) starSprite = sprites.RegisterTexture(path + kStarSheet, kStarSource);
                entities.AddComponent(
                    id, ECSEngine::SpawnComponent{id, "star", *starSprite, kFirstSpawnDelay,
                                                  kFramesBetweenSpawns, kTotalSpawns});
            }
        }
    }
    return markers;
}
