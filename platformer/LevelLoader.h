#pragma once

#include <string>
#include <vector>

#include "Game.h"

// A map character the game handles itself (e.g. 'P' player start) instead of a tile
struct MapMarker {
    char symbol;
    ECSEngine::Point2D loc; // top-left of its grid cell, in world units.
};

// The star the old level's spawners create, in spritesheet-tiles-default.png
inline const std::string kStarSheet = "spritesheet-tiles-default.png";
inline const ECSEngine::Rect kStarSource{{640.0f, 320.0f}, 64.0f, 64.0f};

// Loads file into scene, drawing its tiles on layer
// Characters in markerSymbols are returned as markers instead of made into tiles
// Throws if the file is missing or broken
std::vector<MapMarker> LoadMap(EngineViewT& engine, Scene& scene, const std::string& path, const std::string& file, int layer, const std::string& markerSymbols);