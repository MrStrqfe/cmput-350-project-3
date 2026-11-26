#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include "CameraComponent.h"
#include "CameraFollowerComponent.h"
#include "CameraShakeComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "GravityComponent.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementComponent.h"
#include "RespawnComponent.h"
#include "Scene.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

// What systems see of the engine: the shared managers plus the scene stack.
//   - Entities, systems and the camera belong to each scene (see SceneView).
//   - Sounds and sprites are shared by every scene, so they live here.
//   - The returned references are valid for the engine's whole life. Data you
//     get from a manager has its own rules (see that manager's comment).
template <typename... Components>
class EngineView {
public:
    virtual ~EngineView() = default;
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;

    // The scene on top of the stack (the one running), or nullptr if there is none.
    virtual std::shared_ptr<SceneView<Components...>> GetScene() = 0;
    // A new, empty scene. It does nothing until it is passed to AddScene.
    virtual std::shared_ptr<SceneView<Components...>> MakeScene() = 0;
    // Pushes `scene` on top of the stack at the end of the current frame.
    virtual void AddScene(std::shared_ptr<SceneView<Components...>> scene) = 0;
    // Removes the top scene. Safe to call from a system of that scene: the
    // scene stays alive until its frame finishes.
    virtual void PopScene() = 0;
};

template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    // Runs the top scene once per frame until no scenes are left or the window closes.
    void Run();

    // The engine owns the shared managers. Each reference is valid as long as the engine exists.
    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    std::shared_ptr<SceneView<Components...>> GetScene() override {
        return mScenes.empty() ? nullptr : mScenes.back();
    }

    // ECSScene's constructor is private (only the engine makes scenes), so
    // make_shared can't be used here.
    std::shared_ptr<SceneView<Components...>> MakeScene() override {
        auto ptr = new ECSScene<Components...>(mWindowManager);
        return std::shared_ptr<ECSScene<Components...>>(ptr);
    }

    void AddScene(std::shared_ptr<SceneView<Components...>> scene) override {
        mNewScenes.push_back(scene);
    }

    void PopScene() override {
        if (!mScenes.empty()) mScenes.pop_back();
    }

private:
    // Handles window events and updates every InputComponent in `scene`.
    // Returns false when the window is closed.
    bool ProcessEvents(SceneView<Components...>& scene);
    // Sets or clears one key's bit in every InputComponent in `entities`.
    void SetKey(EntityManager<Components...>& entities, sf::Keyboard::Scancode key, bool down);
    // Handles the F1/F2/F3 debug keys. Returns true if `key` was one of them.
    bool HandleDebugKey(sf::Keyboard::Scancode key);
    // Outlines every collision box in `scene` (red = moving, green = static).
    void DebugDraw(SceneView<Components...>& scene);
    // Moves scenes added during the last frame onto the top of the stack.
    void PushNewScenes();

    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;  // owns the window; scenes share it

    std::vector<std::shared_ptr<SceneView<Components...>>> mScenes;     // back() is on top
    std::vector<std::shared_ptr<SceneView<Components...>>> mNewScenes;  // added this frame

    // For debugging - pause and step the engine
    bool running;  // false while paused (F1)
    bool step;     // run exactly one frame while paused (F2)
    int debug;     // 0 = off, 1 = draw collision boxes (F3)
};

// Systems such as the dash timer count frames, so the game runs at a fixed rate.
inline constexpr unsigned int kFramesPerSecond = 60;
// The engine clears the window to this once per frame, before any scene draws
inline constexpr sf::Color kClearColor{135, 206, 235};  // sky blue

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name), running(true), step(false), debug(0) {
    mWindowManager.GetWindow()->setFramerateLimit(kFramesPerSecond);
}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {}

// One loop pass is one frame, and only the top scene runs:
//  1. ProcessEvents    - read the keyboard and window events into the top scene
//  2. clear            - the engine clears the window once
//  3. DrawOneFrame     - scenes under an overlay are drawn but not updated (frozen)
//  4. DoOneFrame       - run the top scene's systems; it draws on top
//  5. kSceneDone       - a scene that finished is popped
//  6. DebugDraw        - collision boxes on top, if turned on
//  7. display()        - show the finished frame (this also waits to keep 60 FPS)
//  8. PushNewScenes    - scenes added this frame go on top for the next frame
//  9. ReleaseAllKeys   - if the top scene changed, it starts with no keys held,
//                      since it missed key releases while it wasn't on top
// The loop ends when the window is closed or no scenes are left.
template <typename... Components>
void ECSEngine<Components...>::Run() {
    auto window = mWindowManager.GetWindow();
    PushNewScenes();  // scenes added before Run()
    while (window->isOpen() && !mScenes.empty()) {
        // A local copy keeps the scene alive even if a system pops it mid-frame.
        std::shared_ptr<SceneView<Components...>> scene = mScenes.back();
        if (!ProcessEvents(*scene)) break;

        if (!running && !step) {
            // Paused: keep showing the last frame. display() is what normally
            // waits for the next frame, so sleep instead of spinning the CPU.
            sf::sleep(sf::milliseconds(1000 / kFramesPerSecond));
            continue;
        }
        step = false;

        window->clear(kClearColor);
        for (size_t i = FirstSceneToDraw(mScenes); i+1 < mScenes.size(); ++i) {
            mScenes[i]->DrawOneFrame(this);
        }

        // Only pop it if a system didn't already pop it (or push over it).
        if (scene->DoOneFrame(this) == kSceneDone && !mScenes.empty() && mScenes.back() == scene) {
            mScenes.pop_back();
        }
        if (debug) DebugDraw(*scene);
        window->display();
        PushNewScenes();

        if (!mScenes.empty() && mScenes.back() != scene) {
            ReleaseAllKeys(mScenes.back()->GetEntityManager());
        }
    }
    window->close();
}

template <typename... Components>
void ECSEngine<Components...>::PushNewScenes() {
    for (auto& scene : mNewScenes) mScenes.push_back(std::move(scene));
    mNewScenes.clear();
}

template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents(SceneView<Components...>& scene) {
    // Remember last frame's keys so systems can tell a new press
    // (keydown set, lastKeydown clear) from a key that is being held.
    auto& entities = scene.GetEntityManager();
    auto& inputs = entities.template GetComponentStorage<InputComponent>();
    for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
        if (inputs.mValid[i]) inputs.mStorage[i].lastKeydown = inputs.mStorage[i].keydown;
    }

    std::shared_ptr<sf::RenderWindow> window = mWindowManager.GetWindow();
    while (const std::optional<sf::Event> event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            return false;
        } else if (const auto* pressed = event->getIf<sf::Event::KeyPressed>()) {
            if (!HandleDebugKey(pressed->scancode)) SetKey(entities, pressed->scancode, true);
        } else if (const auto* released = event->getIf<sf::Event::KeyReleased>()) {
            SetKey(entities, released->scancode, false);
        } else if (event->is<sf::Event::FocusLost>()) {
            ReleaseAllKeys(entities);
        }
    }
    return true;
}

template <typename... Components>
void ECSEngine<Components...>::SetKey(EntityManager<Components...>& entities,
                                      sf::Keyboard::Scancode key, bool down) {
    if (key == sf::Keyboard::Scancode::Unknown) return;  // -1, not a valid bit
    const auto bit = static_cast<size_t>(key);

    auto& inputs = entities.template GetComponentStorage<InputComponent>();
    for (size_t i = 0; i < inputs.mStorage.size(); ++i) {
        if (inputs.mValid[i]) inputs.mStorage[i].keydown.set(bit, down);
    }
}

template <typename... Components>
bool ECSEngine<Components...>::HandleDebugKey(sf::Keyboard::Scancode key) {
    switch (key) {
        case sf::Keyboard::Scancode::F1:  // pause / resume
            running = !running;
            std::cout << (running ? "Resumed\n" : "Paused (F2 steps one frame)\n");
            return true;
        case sf::Keyboard::Scancode::F2:  // advance one frame, staying paused
            running = false;
            step = true;
            return true;
        case sf::Keyboard::Scancode::F3:  // collision boxes on / off
            debug = debug ? 0 : 1;
            return true;
        default:
            return false;
    }
}

template <typename... Components>
void ECSEngine<Components...>::DebugDraw(SceneView<Components...>& scene) {
    auto& entities = scene.GetEntityManager();
    WindowManager& window = scene.GetWindowManager();  // the scene's camera
    auto& collisions = entities.template GetComponentStorage<CollisionComponent>();
    for (size_t i = 0; i < collisions.mStorage.size(); ++i) {
        if (!collisions.mValid[i]) continue;
        const CollisionComponent& c = collisions.mStorage[i];
        // The box is stored relative to the entity's location.
        Rect box = c.collisionBox;
        const EntityID owner = collisions.mOwner[i];
        if (entities.template HasComponent<LocationComponent>(owner)) {
            const Point2D& loc = entities.template GetComponent<LocationComponent>(owner).loc;
            box.topLeft = Point2D{box.topLeft.x + loc.x, box.topLeft.y + loc.y};
        }
        box = window.WorldToWindow(box);
        if (c.staticEntity) {
            window.DebugFrameRect(box, 0, 255, 0);
        } else {
            window.DebugFrameRect(box, 255, 0, 0);
        }
    }
}

}  // namespace ECSEngine
