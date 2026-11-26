#pragma once

#include "EntityManager.h"
#include "SystemManager.h"
#include "WindowManager.h"

namespace ECSEngine {

enum tSceneReturn {
    kSceneContinue,
    kSceneDone
};

// What systems see of the scene they run in.
// Each reference is valid for as long as the scene exists
// while it is on the engine's scene stack

template <typename... Components>
class SceneView {

public:
    virtual ~SceneView() = default;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
    virtual SystemManager<Components...>& GetSystemManager() = 0;
    virtual tSceneReturn DoOneFrame(EngineView<Components...>* engine) = 0;
};

// One scene: its own entities, systems and camera (WindowManager).
//   - Made only by ECSEngine::MakeScene (the constructor is private).
//   - Its WindowManager shares the engine's window but has its own camera,
//     so each scene keeps its own world coordinates.
template <typename... Components>
class ECSScene : public SceneView<Components...> {
public:
    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }
    WindowManager& GetWindowManager() override { return mWindowManager; }
    SystemManager<Components...>& GetSystemManager() override { return mSystemManager; }

    // Runs every system once. The scene is done when any system returns kExit.
    [[nodiscard]] tSceneReturn DoOneFrame(EngineView<Components...>* engine) override {
        if (mSystemManager.Run(engine, this) == kExit) {
            return kSceneDone;
        }
        return kSceneContinue;
    }

private:
    template <typename... T>
    friend class ECSEngine;

    explicit ECSScene(WindowManager& windowManager) : mWindowManager(windowManager.GetWindow()) {}

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    WindowManager mWindowManager;
};

} // namespace ECSEngine