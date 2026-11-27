#pragma once

#include "../Game.h"
#include "InputComponent.h"
#include "SystemManager.h"

// Victory screen: Enter (or Esc) goes back to the menu underneath.
class VictorySystem : public ECSEngine::System<COMPONENTS> {
public:
    const std::string& GetName() const override {
        static const std::string name("VictorySystem");
        return name;
    }

    ECSEngine::tSystemReturn Run(EngineViewT* engine, Scene* scene) override {
        const ECSEngine::InputComponent* input = FindInput(*scene);
        if (input && (JustPressed(*input, kKeyConfirm) || JustPressed(*input, kKeyBack))) {
            engine->PopScene();
        }
        return ECSEngine::kContinue;
    }
};
