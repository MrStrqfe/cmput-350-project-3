#pragma once

#include "../Game.h"
#include "../Scenes.h"
#include "InputComponent.h"
#include "SystemManager.h"

// Gameplay: Esc opens the pause screen on top of the game
class PauseSystem : public ECSEngine::System<COMPONENTS> {
public:
    const std::string& GetName() const override {
        static const std::string name("PauseSystem");
        return name;
    }

    ECSEngine::tSystemReturn Run(EngineViewT* engine, Scene* scene) override {
        const ECSEngine::InputComponent* input = FindInput(*scene);
        if (input && JustPressed(*input, kKeyBack)) {
            engine->AddScene(MakePauseScene(*engine));
        }
        return ECSEngine::kContinue;
    }
};

// Pause screen: Esc or Enter resumes the game, Q quits to the menu.
class PauseMenuSystem : public ECSEngine::System<COMPONENTS> {
public:
    const std::string& GetName() const override {
        static const std::string name("PauseMenuSystem");
        return name;
    }

    ECSEngine::tSystemReturn Run(EngineViewT* engine, Scene* scene) override {
        const ECSEngine::InputComponent* input = FindInput(*scene);
        if (!input) {
            return ECSEngine::kContinue;
        }
        if (JustPressed(*input, kKeyBack) || JustPressed(*input, kKeyConfirm)) {
            engine->PopScene(); // this pause screen
        }
        else if (JustPressed(*input, kKeyQuit)) {
            engine->PopScene(); // this pause screen
            engine->PopScene(); // and the game under it
        }
        return ECSEngine::kContinue;
    }
};