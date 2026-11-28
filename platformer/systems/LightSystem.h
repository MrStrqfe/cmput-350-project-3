#pragma once

#include <array>
#include <vector>

#include <SFML/Graphics/Glsl.hpp>

#include "../Game.h"
#include "../Lighting.h"
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

// Sends the lights nearest the camera (at most 16) to this scene's "lighting"
// shader, in the shader's pixels. Runs after the camera moves and before the
// SpriteSystem draws. It is a draw system so a paused scene's lights still line
// up after the window is resized. Does nothing if the scene has no "lighting" shader

class LightSystem : public ECSEngine::System<COMPONENTS> {
public:
    const std::string& GetName() const override {
        static const std::string name("LightSystem");
        return name;
    }

    bool IsDrawSystem() const override { return true; }

    ECSEngine::tSystemReturn Run(EngineViewT*, Scene* scene) override {
        sf::Shader* shader = scene->GetShaderManager().GetShader("lighting");
        if (!shader) {
            return ECSEngine::kContinue;
        }
        auto& entities = scene->GetEntityManager();
        auto& windowManager = scene->GetWindowManager();

        // Every light, centred on its sprite (or at its location if it has none)
        mLights.clear();
        auto& lights = entities.GetComponentStorage<LightComponent>();

        for (std::size_t i = 0; i < lights.mStorage.size(); ++i) {
            const ECSEngine::EntityID id = lights.mOwner[i];
            if (!lights.mValid[i] || !entities.HasComponent<ECSEngine::LocationComponent>(id)) {
                continue;
            }
            ECSEngine::Point2D center = entities.GetComponent<ECSEngine::LocationComponent>(id).loc;

            if (entities.HasComponent<ECSEngine::SpriteComponent>(id)) {
                const ECSEngine::Rect& bounds = entities.GetComponent<ECSEngine::SpriteComponent>(id).bounds;
                center.x += bounds.topLeft.x + bounds.width / 2.0f;
                center.y += bounds.topLeft.y + bounds.height / 2.0f;
            }
            mLights.push_back({center, lights.mStorage[i].radius, lights.mStorage[i].flicker});
        }
        NearestLights(mLights, windowManager.GetWorldCenter());

        // World units to gl_FragCoord pixels (origin bottom-left)
        const float windowHeight = static_cast<float>(windowManager.GetWindow()->getSize().y);
        for (std::size_t i = 0; i < mLights.size(); ++i) {
            const LightSample& light = mLights[i];
            const ECSEngine::Point2D frag = ToFragCoord(windowManager.WorldToWindow(light.pos), windowHeight);
            mUniforms[i] = {frag.x, frag.y, windowManager.WorldToWindow(light.radius), light.flicker};
        }

        if (!mLights.empty()) {
            shader->setUniformArray("lights", mUniforms.data(), mLights.size());
        }
        shader->setUniform("lightCount", static_cast<int>(mLights.size()));
        shader->setUniform("ambient", kAmbient);
        shader->setUniform("time", static_cast<float>(++mFrame) / ECSEngine::kFramesPerSecond);
        shader->setUniform("oil", OilLeft(entities));
        return ECSEngine::kContinue;
    }

private:
    static constexpr float kAmbient = 0.08f; // brightness far from any light

    // The first lantern's oil, 0 to 1, or 1 if there is no lantern
    static float OilLeft(ECSEngine::EntityManager<COMPONENTS>& entities) {
        auto& lanterns = entities.GetComponentStorage<LanternComponent>();
        for (std::size_t i = 0; i < lanterns.mStorage.size(); ++i) {
            if (lanterns.mValid[i]) {
                return lanterns.mStorage[i].oil / lanterns.mStorage[i].maxOil;
            }
        }
        return 1.0f;
    }

    std::vector<LightSample> mLights;   // kept between frames to reuse memory
    std::array<sf::Glsl::Vec4, kMaxLights> mUniforms{};
    unsigned int mFrame = 0;            // drives the flicker

};