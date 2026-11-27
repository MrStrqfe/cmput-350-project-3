#pragma once

#include <map>
#include <string>

#include "ECSEngine.h"
#include "InputComponent.h"
#include "Keyboard.h"
#include "components/AnimationComponent.h"

// Shared by all game code: the component list, type names, draw layers and keybinds

#define COMPONENTS                                                                              \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent,  \
    ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,          \
    ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,          \
    ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent, AnimationComponent

using Engine = ECSEngine::ECSEngine<COMPONENTS>;
using EngineViewT = ECSEngine::EngineView<COMPONENTS>;
using Scene = ECSEngine::SceneView<COMPONENTS>;

// Folder with the assets. main changes it with "-path"
inline std::string gResourcePath = "../assets/";

inline constexpr unsigned int kWindowWidth = 1024;
inline constexpr unsigned int kWindowHeight = 768;

// Menu screens are laid out in this many units and letterboxed to fit the window
// 16:9, and 1 unit = 1 pixel at the default window width
inline constexpr float kUiWidth = 1024.0f;
inline constexpr float kUiHeight = 576.0f;

// Draw layers, back to front. The world is 0, so sprites made without a layer are in it
inline constexpr int kLayerBackdrop = -10;  // window-filling colour behind a menu
inline constexpr int kLayerFarWall = -3;
inline constexpr int kLayerStalactites = -2;
inline constexpr int kLayerNearRocks = -1;
inline constexpr int kLayerWorld = 0;
inline constexpr int kLayerForeground = 1;
inline constexpr int kLayerHud = 10;

// Each layer's scroll speed compared to the world; unlisted layers move with it
// The foreground moves faster than the world, which makes the depth read well
inline const std::map<int, float> kParallax{{kLayerFarWall, 0.15f},
                                            {kLayerStalactites, 0.4f},
                                            {kLayerNearRocks, 0.7f},
                                            {kLayerForeground, 1.25f}};

// Key bindings
inline constexpr unsigned int kKeyInteract = ECSEngine::Key::E;
inline constexpr unsigned int kKeyFlare = ECSEngine::Key::F;
inline constexpr unsigned int kKeyBack = ECSEngine::Key::Escape;
inline constexpr unsigned int kKeyConfirm = ECSEngine::Key::Enter;
inline constexpr unsigned int kKeyQuit = ECSEngine::Key::Q;

// True only on the frame 'key' goes down, not while it is held
inline bool JustPressed(const ECSEngine::InputComponent& input, unsigned int key) {
    return input.keydown[key] && !input.lastKeydown[key];
}

// The scene's first InputComponent, or nullptr if it has none.
// valid until a component is added to or removed from the scene
inline const ECSEngine::InputComponent* FindInput(Scene& scene) {
    auto& inputs = scene.GetEntityManager().GetComponentStorage<ECSEngine::InputComponent>();
    for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
        if (inputs.mValid[i]) {
            return &inputs.mStorage[i];
        }
    }
    return nullptr;
}