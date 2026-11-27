#include "Scenes.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>

#include "CameraSystem.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "GravitySystem.h"
#include "InputSystem.h"
#include "LevelLoader.h"
#include "MovementSystem.h"
#include "ScoreSystem.h"
#include "SpawnSystem.h"
#include "SpriteSystem.h"
#include "systems/MenuSystem.h"
#include "systems/PauseSystem.h"
#include "systems/VictorySystem.h"

// Builds the four scenes. Menu screens use a fixed kUiWidth x kUiHeight view,
// so they scale with the window (bonus 2); gameplay uses the camera.

namespace {

// The UI images in assets/ui/, each used whole.
const ECSEngine::Rect kWhiteSource{{0.f, 0.f}, 4.f, 4.f};
const ECSEngine::Rect kTitleSource{{0.f, 0.f}, 800.f, 140.f};
const ECSEngine::Rect kPromptsSource{{0.f, 0.f}, 600.f, 100.f};
const ECSEngine::Rect kPausedSource{{0.f, 0.f}, 600.f, 160.f};
const ECSEngine::Rect kVictorySource{{0.f, 0.f}, 640.f, 330.f};

// Where victory.png leaves room for its two numbers (relative to the image).
const float kVictoryDigitsX = 420.f;
const float kVictoryTimeY = 126.f;
const float kVictoryBraziersY = 196.f;

const float kDigitSize = 48.f;
const float kDigitStep = 40.f;  // the digit images have empty sides, so they overlap a little

const sf::Color kMenuBackdrop{16, 14, 24};
const sf::Color kPauseBackdrop{0, 0, 0, 160};  // see-through, so the frozen game shows

// An entity with only a location and a sprite.
ECSEngine::EntityID AddSprite(Scene& scene, ECSEngine::SpriteID sprite, ECSEngine::Point2D at,
                              float width, float height, int layer, bool worldSpace = true) {
    auto& entities = scene.GetEntityManager();
    const ECSEngine::EntityID id = entities.CreateEntity("sprite");
    entities.AddComponent(id, ECSEngine::LocationComponent{at});
    entities.AddComponent(
        id, ECSEngine::SpriteComponent{sprite, {{0.f, 0.f}, width, height}, worldSpace, layer});
    return id;
}

// A whole UI image, centred left to right, with its top at `top`.
void AddCenteredImage(EngineViewT& engine, Scene& scene, const std::string& file,
                      const ECSEngine::Rect& source, float top) {
    const ECSEngine::SpriteID sprite =
        engine.GetSpriteManager().RegisterTexture(gResourcePath + file, source);
    AddSprite(scene, sprite, {(kUiWidth - source.width) / 2.f, top}, source.width,
              source.height, kLayerWorld);
}

// The digit sprites 0-9. They are stacked 9 (top) to 0 (bottom) in one column of the tile sheet.
std::array<ECSEngine::SpriteID, 10> RegisterDigits(ECSEngine::SpriteManager& sprites) {
    std::array<ECSEngine::SpriteID, 10> digits{};
    for (int d = 0; d < 10; ++d) {
        digits[d] = sprites.RegisterTexture(gResourcePath + "spritesheet-tiles-default.png",
                                            {{832.f, 832.f - 64.f * d}, 64.f, 64.f});
    }
    return digits;
}

// Shows `value` as `count` digits (with leading zeros) starting at `topLeft`.
void AddNumber(Scene& scene, const std::array<ECSEngine::SpriteID, 10>& digits, int value,
               int count, ECSEngine::Point2D topLeft) {
    int largest = 1;
    for (int i = 0; i < count; ++i) {
        largest *= 10;
    }
    value = std::clamp(value, 0, largest - 1);
    for (int i = count - 1; i >= 0; --i) {  // ones digit first, on the right
        AddSprite(scene, digits[value % 10], {topLeft.x + kDigitStep * i, topLeft.y}, kDigitSize,
                  kDigitSize, kLayerWorld);
        value /= 10;
    }
}

// A menu-style scene: fixed UI view, a colour behind everything, and a keyboard entity.
std::shared_ptr<Scene> MakeUiScene(EngineViewT& engine, sf::Color backdrop) {
    std::shared_ptr<Scene> scene = engine.MakeScene();
    scene->GetWindowManager().SetLogicalSize(kUiWidth, kUiHeight);

    // In window pixels and far bigger than any window, so it also covers the letterbox bars.
    const ECSEngine::SpriteID white =
        engine.GetSpriteManager().RegisterTexture(gResourcePath + "ui/white.png", kWhiteSource);
    const ECSEngine::EntityID back =
        AddSprite(*scene, white, {0.f, 0.f}, 10000.f, 10000.f, kLayerBackdrop, false);
    scene->GetEntityManager().GetComponent<ECSEngine::SpriteComponent>(back).tint = backdrop;

    auto& entities = scene->GetEntityManager();
    const ECSEngine::EntityID keyboard = entities.CreateEntity("keyboard");
    entities.AddComponent(keyboard, ECSEngine::InputComponent{keyboard});
    return scene;
}

}  // namespace

std::shared_ptr<Scene> MakeMenuScene(EngineViewT& engine) {
    std::shared_ptr<Scene> scene = MakeUiScene(engine, kMenuBackdrop);
    AddCenteredImage(engine, *scene, "ui/title.png", kTitleSource, 150.f);
    AddCenteredImage(engine, *scene, "ui/prompts.png", kPromptsSource, 340.f);

    auto& systems = scene->GetSystemManager();
    systems.AddSystem(std::make_unique<MenuSystem>());
    systems.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    return scene;
}

std::shared_ptr<Scene> MakePauseScene(EngineViewT& engine) {
    std::shared_ptr<Scene> scene = MakeUiScene(engine, kPauseBackdrop);
    scene->SetOverlay(true);
    AddCenteredImage(engine, *scene, "ui/paused.png", kPausedSource,
                     (kUiHeight - kPausedSource.height) / 2.f);

    auto& systems = scene->GetSystemManager();
    systems.AddSystem(std::make_unique<PauseMenuSystem>());
    systems.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    return scene;
}

std::shared_ptr<Scene> MakeVictoryScene(EngineViewT& engine, int framesTaken, int braziersLit) {
    std::shared_ptr<Scene> scene = MakeUiScene(engine, kMenuBackdrop);
    const float left = (kUiWidth - kVictorySource.width) / 2.f;
    const float top = (kUiHeight - kVictorySource.height) / 2.f;
    AddCenteredImage(engine, *scene, "ui/victory.png", kVictorySource, top);

    const auto digits = RegisterDigits(engine.GetSpriteManager());
    const int seconds = framesTaken / static_cast<int>(ECSEngine::kFramesPerSecond);
    AddNumber(*scene, digits, seconds, 4, {left + kVictoryDigitsX, top + kVictoryTimeY});
    AddNumber(*scene, digits, braziersLit, 2, {left + kVictoryDigitsX, top + kVictoryBraziersY});

    auto& systems = scene->GetSystemManager();
    systems.AddSystem(std::make_unique<VictorySystem>());
    systems.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    return scene;
}

// For now this is the Project 2 level, so the scene flow can be tried out.
std::shared_ptr<Scene> MakeGameplayScene(EngineViewT& engine) {
    std::shared_ptr<Scene> scene = engine.MakeScene();
    auto& entities = scene->GetEntityManager();
    auto& sprites = engine.GetSpriteManager();

    // The sky goes on the far wall layer, behind the world.
    LoadMap(engine, *scene, gResourcePath, "sky.map", kLayerFarWall, "");
    LoadMap(engine, *scene, gResourcePath, "world.map", kLayerWorld, "");

    // One extra star past the first wall, to test wall jumping.
    // It has no MovementComponent, so it floats in place until collected.
    const ECSEngine::EntityID bonusStar = entities.CreateEntity("star");
    const ECSEngine::SpriteID starSprite = sprites.RegisterTexture(gResourcePath + kStarSheet, kStarSource);
    entities.AddComponent(bonusStar, ECSEngine::LocationComponent{{33 * 64.0f, 2 * 64.0f}});
    entities.AddComponent(bonusStar, ECSEngine::SpriteComponent{starSprite, {{0.0f, 0.0f}, 64.0f, 64.0f}, true});
    ECSEngine::CollisionComponent bonusCollision{};
    bonusCollision.collisionBox = {{0.0f, 0.0f}, 64.0f, 64.0f};
    entities.AddComponent(bonusStar, bonusCollision);

    // Player
    const ECSEngine::Point2D kPlayerStart{832.f, 512.f};
    const ECSEngine::SpriteID playerSprite = sprites.RegisterTexture(
        gResourcePath + "spritesheet-characters-default.png", {{0.0f, 0.0f}, 128.0f, 128.0f});
    const ECSEngine::EntityID player = entities.CreateEntity("player");
    entities.AddComponent(player, ECSEngine::LocationComponent{kPlayerStart});
    entities.AddComponent(player, ECSEngine::MovementComponent{});
    entities.AddComponent(player, ECSEngine::GravityComponent{{0.0f, 2.0f}});
    entities.AddComponent(player, ECSEngine::InputComponent{player});
    entities.AddComponent(player, ECSEngine::RespawnComponent{kPlayerStart});
    entities.AddComponent(player, ECSEngine::SpriteComponent{playerSprite, {{0.0f, 0.0f}, 64.0f, 64.0f}, true});
    ECSEngine::CollisionComponent playerCollision{};
    playerCollision.collisionBox = {{12.0f, 12.0f}, 40.0f, 52.0f};
    playerCollision.previousCollisionBox = playerCollision.collisionBox;
    entities.AddComponent(player, playerCollision);
    entities.AddComponent(player, ECSEngine::CameraFollower{player});

    // Score: three digits in the top-left corner, in window pixels
    ECSEngine::ScoreComponent score{};
    const auto digits = RegisterDigits(sprites);
    std::copy(digits.begin(), digits.end(), score.digitTextureID);
    for (int i = 0; i < 3; ++i) {
        const ECSEngine::EntityID digit = entities.CreateEntity("score digit");
        entities.AddComponent(digit, ECSEngine::LocationComponent{{16.f + 40.f * i, 16.f}});
        entities.AddComponent(digit, ECSEngine::SpriteComponent{digits[0], {{0.f, 0.f}, 48.f, 48.f}, false, kLayerHud});
        score.digitID[i] = digit;
    }
    const ECSEngine::EntityID scoreEntity = entities.CreateEntity("score");
    entities.AddComponent(scoreEntity, score);

    // Camera
    const float kWorldHeight = 14 * 64.0f;
    const float w = static_cast<float>(kWindowWidth);
    const float h = static_cast<float>(kWindowHeight);

    ECSEngine::CameraComponent camera{};
    camera.position = {kPlayerStart.x, kWorldHeight / 2};
    camera.worldUnitsPerPixel = kWorldHeight / h;
    camera.hardExclusion = {{0.1f * w, 0.0f}, 0.8f * w, h};
    camera.softExclusion = {{0.3f * w, 0.0f}, 0.4f * w, h};
    camera.speed = 0.05f;

    const ECSEngine::EntityID cameraEntity = entities.CreateEntity("camera");
    entities.AddComponent(cameraEntity, camera);
    entities.AddComponent(cameraEntity, ECSEngine::CameraShake{0, {0.0f, 0.0f}});
    scene->GetWindowManager().SetWorldScale(camera.worldUnitsPerPixel);
    scene->GetWindowManager().SetWorldCenter(camera.position);

    // Systems. PauseSystem goes first so Esc is seen before anything moves.
    auto& systems = scene->GetSystemManager();
    systems.AddSystem(std::make_unique<PauseSystem>());
    systems.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    systems.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    systems.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    systems.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    systems.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    systems.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    systems.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    systems.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>(kParallax));
    systems.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());
    return scene;
}
