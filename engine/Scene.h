#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "EntityManager.h"
#include "InputComponent.h"
#include "ShaderManager.h"
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
    virtual ShaderManager& GetShaderManager() = 0;
    virtual tSceneReturn DoOneFrame(EngineView<Components...>* engine) = 0;
    virtual void DrawOneFrame(EngineView<Components...>* engine) = 0;
    virtual bool IsOverlay() const = 0;
    virtual void SetOverlay(bool overlay) = 0;
};

// One scene: its own entities, systems, shaders and camera (WindowManager).
//   - Made only by ECSEngine::MakeScene (the constructor is private).
//   - Its WindowManager shares the engine's window but has its own camera,
//     so each scene keeps its own world coordinates.
template <typename... Components>
class ECSScene : public SceneView<Components...> {
public:
    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }
    WindowManager& GetWindowManager() override { return mWindowManager; }
    ShaderManager& GetShaderManager() override { return mShaderManager; }
    SystemManager<Components...>& GetSystemManager() override { return mSystemManager; }

    // Runs every system once. The scene is done when any system returns kExit.
    [[nodiscard]] tSceneReturn DoOneFrame(EngineView<Components...>* engine) override {
        if (mSystemManager.Run(engine, this) == kExit) {
            return kSceneDone;
        }
        return kSceneContinue;
    }
    void DrawOneFrame(EngineView<Components...>* engine) override {
        mSystemManager.Draw(engine, this);
    }
    bool IsOverlay() const override { return mOverlay; }
    void SetOverlay(bool overlay) override { mOverlay = overlay; }

private:
    template <typename... T>
    friend class ECSEngine;

    explicit ECSScene(WindowManager& windowManager) : mWindowManager(windowManager.GetWindow()) {}

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    WindowManager mWindowManager;
    ShaderManager mShaderManager;
    bool mOverlay = false;
};

// Index of the lowest scene that is still visible. Walking down from the top,
// overlays let the scene blow show through, the first scene that
// isn't an overlay hides everything under it. Returns 0 for an empty stack.
template <typename... Components>
size_t FirstSceneToDraw(const std::vector<std::shared_ptr<SceneView<Components...>>>& scenes) {
    if (scenes.empty()) {
        return 0;
    }
    size_t i = scenes.size() - 1;
    while (i > 0 && scenes[i]->IsOverlay()) {
        --i;
    }
    return i;
}
// Let's go of every key: clears keydown and lastKeydown on every InputComponent
// Used when a scene becomes the top (it missed the key releases while it was covered) and when the window
// loses focus
template <typename... Components>
void ReleaseAllKeys(EntityManager<Components...>& entities) {
    for (auto& input : entities.template GetComponentStorage<InputComponent>().mStorage) {
        input.keydown.reset();
        input.lastKeydown.reset();
    }
}

} // namespace ECSEngine