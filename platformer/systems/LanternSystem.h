#pragma once

#include "../Game.h"
#include "../Lighting.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MovementComponent.h"
#include "RespawnComponent.h"
#include "SystemManager.h"

// Burns lantern oil each frame, starts a flare when F is pressed
// and sets the entity's light radius from its oil. Out of oil 
// sends it back to it's respawn point
class LanternSystem : public ECSEngine::System<COMPONENTS> {
public:
    const std::string& GetName() const override {
        static const std::string name("LanternSystem");
        return name;
    }

    ECSEngine::tSystemReturn Run(EngineViewT*, Scene* scene) override {
        auto& entities = scene->GetEntityManager();
        auto& lanterns = entities.GetComponentStorage<LanternComponent>();
        for (std::size_t i = 0; i < lanterns.mStorage.size(); ++i) {
            if (!lanterns.mValid[i]) {
                continue;
            }
            const ECSEngine::EntityID id = lanterns.mOwner[i];
            LanternComponent& lantern = lanterns.mStorage[i];

            DrainOil(lantern);
            if (entities.HasComponent<ECSEngine::InputComponent>(id) &&
                JustPressed(entities.GetComponent<ECSEngine::InputComponent>(id), kKeyFlare)) {    
                TryFlare(lantern);
            }

            if (lantern.oil <= 0.0f && entities.HasComponent<ECSEngine::LocationComponent>(id) &&
                entities.HasComponent<ECSEngine::MovementComponent>(id) &&
                entities.HasComponent<ECSEngine::RespawnComponent>(id)) {
                    RespawnLantern(lantern, entities.GetComponent<ECSEngine::LocationComponent>(id).loc,
                        entities.GetComponent<ECSEngine::MovementComponent>(id).velocity,
                        entities.GetComponent<ECSEngine::RespawnComponent>(id));
            }

            if (entities.HasComponent<LightComponent>(id)) {
                entities.GetComponent<LightComponent>(id).radius = LanternRadius(lantern);
            }
        }
        return ECSEngine::kContinue;
    }
};