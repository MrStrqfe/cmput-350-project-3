#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <vector>

namespace ECSEngine {

// What a system returns from Run: kContinue keeps the game going, kExit ends it.
enum tSystemReturn { kContinue, kExit };

// Forward declarations: systems only hold pointers to these, so the full
// definitions aren't needed here. This also avoids an include cycle, since
// a Scene owns a SystemManager
template <typename... Components>
class EngineView;
template <typename... Components>
class SceneView;

// Base class for every system. Run is called once per frame and gets the
// engine: resources shared by all scenes (sounds, sprites, the scene stack)
// scene: resources owned by the running scene (entities, window/camera, systems)
// Both pointers are only valid during the call, fetch managers through them
// on every Run and never store them.
template <typename... Components>
class System {
public:
    virtual ~System() = default;
    virtual tSystemReturn Run(EngineView<Components...>* engine,
                              SceneView<Components...>* scene) = 0;
    virtual const std::string& GetName() const = 0;
};

// Holds a scene's system and runs them in the order they were added.
//   - AddSystem takes ownership; systems are freed with the SystemManager.
//   - Run returns kExit as soon as a system does (later systems are skipped).
template <typename... Components>
class SystemManager {
public:
    tSystemReturn Run(EngineView<Components...>* engine, SceneView<Components...>* scene) {
        for (auto& system: mSystems) {
            if (system->Run(engine, scene) == kExit) {
                std::cout << system->GetName() << " requested exit from system manager\n";
                return kExit;
            }
        }
        return kContinue;
    }
    void AddSystem(std::unique_ptr<System<Components...>>&& system) {
        mSystems.push_back(std::move(system));
    }
    void ClearSystems() { mSystems.clear(); }

private:
    std::vector<std::unique_ptr<System<Components...>>> mSystems;
};

}  // namespace ECSEngine
