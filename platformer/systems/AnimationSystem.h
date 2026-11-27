#pragma once

#include <cmath>

#include "../Game.h"
#include "../components/AnimationComponent.h"
#include "CollisionComponent.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

// Moves every animation on one frame and shows it in the entity's SpriteComponent
// Player-controlled entities also pick idle/walk/jump/fall from their movement and face
// the way they move. Run it after CollisionSystem, so onGround is this frame's
class AnimationSystem : public ECSEngine::System<COMPONENTS> {

public:
    const std::string& GetName() const override {
        static const std::string name("AnimationSystem");
        return name;
    }

    ECSEngine::tSystemReturn Run(EngineViewT*, Scene* scene) override {
        auto& entities = scene->GetEntityManager();
        auto& anims = entities.GetComponentStorage<AnimationComponent>();

        // Only animated entities are visited, not every entity in the scene
        for (std::size_t i = 0; i < anims.mStorage.size(); ++i) {
            if (!anims.mValid[i]) {
                continue;
            }
            const ECSEngine::EntityID id = anims.mOwner[i];
            if (!entities.HasComponent<ECSEngine::SpriteComponent>(id)) {
                continue;
            }
            AnimationComponent& anim = anims.mStorage[i];
            ECSEngine::SpriteComponent& sprite = entities.GetComponent<ECSEngine::SpriteComponent>(id);

            if (entities.HasComponent<ECSEngine::InputComponent>(id) &&
                entities.HasComponent<ECSEngine::MovementComponent>(id) &&
                entities.HasComponent<ECSEngine::CollisionComponent>(id)) {
                    const ECSEngine::Point2D v = entities.GetComponent<ECSEngine::MovementComponent>(id).velocity;
                    const bool onGround = entities.GetComponent<ECSEngine::CollisionComponent>(id).onGround;
                    SetAnimState(anim, PickState(v, onGround));
                    if (v.x != 0.0f) {
                        sprite.flipX = v.x < 0.0f;  // standing still keeps the last facing
                    }
                }
                sprite.textureID = StepAnimation(anim);
        }
        return ECSEngine::kContinue;
    }

private:

    static constexpr float kWalkSpeed = 0.5f;   // slower than this counts as standing
    static AnimState PickState(ECSEngine::Point2D velocity, bool onGround) {
        if (!onGround) {
            return velocity.y < 0.0f ? kAnimJump : kAnimFall;
        }
        return std::abs(velocity.x) > kWalkSpeed ? kAnimWalk : kAnimIdle;
    }
};