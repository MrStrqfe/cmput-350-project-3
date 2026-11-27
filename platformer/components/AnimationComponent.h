#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <vector>

#include "SpriteManager.h"

// Flip-book animation, one clip (list of sprites) per state, each frame
// shown for ticksPerFrame frames, looping.
// An empty clip falls back to idle, so idle must not be empty

enum AnimState : std::size_t {
    kAnimIdle,
    kAnimWalk,
    kAnimJump,
    kAnimFall,
    kAnimStateCount
};

struct AnimationComponent {
    std::array<std::vector<ECSEngine::SpriteID>, kAnimStateCount> clips;
    AnimState state = kAnimIdle;
    int ticksPerFrame = 8;
    int tick = 0;           // frames the current sprite has been shown
    std::size_t frame = 0;  // index into the current clip
};

// Changes state, the clip restarts only if the state is new.
inline void SetAnimState(AnimationComponent& anim, AnimState state) {
    if (anim.state == state) {
        return;
    }
    anim.state = state;
    anim.tick = 0;
    anim.frame = 0;
}

// Advances one frame and returns the sprite to show now
inline ECSEngine::SpriteID StepAnimation(AnimationComponent& anim) {
    const std::vector<ECSEngine::SpriteID>& clip = anim.clips[anim.state].empty() ? anim.clips[kAnimIdle] : anim.clips[anim.state];
    assert(!clip.empty());
    if (++anim.tick >= anim.ticksPerFrame) {
        anim.tick = 0;
        ++anim.frame;
    }
    if (anim.frame >= clip.size()) {
        anim.frame = 0;
    }
    return clip[anim.frame];
}