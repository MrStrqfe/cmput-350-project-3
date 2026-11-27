#pragma once

#include "../Game.h"
#include "../Scenes.h"
#include "InputComponent.h"
#include "SystemManager.h"

// Main menu: Enter starts a new game, Esc quits.
class MenuSystem : public ECSEngine::System<COMPONENTS> {

public:
    const std::string& GetName() const override {
        static const std::string name("MenuSystem");
        return name;
    }

    ECSEngine::tSystemReturn Run(EngineViewT* engine, Scene* scene) override {
        const ECSEngine::InputComponent* input = FindInput(*scene);
        if (!input) {
            return ECSEngine::kContinue;
        }
        if (JustPressed(*input, kKeyBack)) {
            return ECSEngine::kExit;    // the menu is the bottom scene, so this ends the game
        }
        if (JustPressed(*input, kKeyConfirm)) {
            engine->AddScene(MakeGameplayScene(*engine));
        }
        return ECSEngine::kContinue;
    }
};
