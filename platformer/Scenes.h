#pragma once

#include <memory>

#include "Game.h"

// Make the game's four scenes; pass the result to engine's AddScene
// Flow: menu -> gameplay -> pause (overlay) -> back to gameplay or menu
// At the end, gameplay pops itself and adds the victory scene, which returns to the menu

std::shared_ptr<Scene> MakeMenuScene(EngineViewT& engine);
std::shared_ptr<Scene> MakeGameplayScene(EngineViewT& engine);
std::shared_ptr<Scene> MakePauseScene(EngineViewT& engine);
std::shared_ptr<Scene> MakeVictoryScene(EngineViewT& engine, int framesTaken, int braziersLit);