#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "MathUtil.h"
#include "RespawnComponent.h"
#include "components/LanternComponent.h"

// Lantern and light maths with no engine state, shared by LanternSystem.
// LightSystem and (later) the shadows. lighting.frag uses the same falloff.

inline constexpr float kFlareCost = 15.0f;
inline constexpr int kFlareFrames = 60;
inline constexpr float kFlareScale = 1.8f;
inline constexpr float kRespawnOil = 0.6;       // fraction of maxOil after running out
inline constexpr std::size_t kMaxLights = 16;   // must match lights[] in lighting.frag

// One light as LightSystem sees it, in world units
struct LightSample {
    ECSEngine::Point2D pos;
    float radius;
    float flicker;
};

// From minRadius (empty) to maxRadius (full), bigger while a flare burns
inline float LanternRadius(const LanternComponent& lantern) {
    const float fuel = std::clamp(lantern.oil / lantern.maxOil, 0.0f, 1.0f);
    const float radius = lantern.minRadius + (lantern.maxRadius - lantern.minRadius) * fuel;
    return lantern.flareFrames > 0 ? radius * kFlareScale : radius;
}

// One frame of burning: oil goes down (never below 0) and any flare counts down
inline void DrainOil(LanternComponent& lantern) {
    lantern.oil = std::max(0.0f, lantern.oil - lantern.drainPerFrame);
    if (lantern.flareFrames > 0) {
        --lantern.flareFrames;
    }
}

// Starts a flare if there is enough oil and none is burning
// Returns true if it started
inline bool TryFlare(LanternComponent& lantern) {
    if (lantern.flareFrames > 0 || lantern.oil <= kFlareCost) {
        return false;
    }
    lantern.oil -= kFlareCost;
    lantern.flareFrames = kFlareFrames;
    return true;
}

// Out of oil: back to the respawn point, stopped, with some oil so it doesn't happen the next frame
inline void RespawnLantern(LanternComponent& lantern, ECSEngine::Point2D& loc, ECSEngine::Point2D& velocity, const ECSEngine::RespawnComponent& respawn) {
    lantern.oil = kRespawnOil * lantern.maxOil;
    lantern.flareFrames = 0;
    loc = respawn.loc;
    velocity = {0.0f, 0.0f};
}

inline float DistanceSquared(ECSEngine::Point2D a, ECSEngine::Point2D b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

// Keeps only the max lights closest to center (in no particular order)
// Works in place, so it never allocates
inline void NearestLights(std::vector<LightSample>& lights, ECSEngine::Point2D center, std::size_t max = kMaxLights) {
    if (lights.size() <= max) {
        return;
    }
    auto closer = [center](const LightSample& a, const LightSample& b) {
        return DistanceSquared(a.pos, center) < DistanceSquared(b.pos, center);
    };
    std::nth_element(lights.begin(), lights.begin() + static_cast<std::ptrdiff_t>(max), lights.end(), closer);
    lights.resize(max);
}

// Window pixel (origin top-left) to gl_FragCoord (origin bottom-left)
inline ECSEngine::Point2D ToFragCoord(ECSEngine::Point2D windowPixel, float windowHeight) {
    return {windowPixel.x, windowHeight - windowPixel.y};
}

// How lit point p is, 0 (dark) to 1 (centre of a light). Same falloff as the shader
// without flicker or ambient: full inside 35% of the radius, 0 at the edge
inline float LightIntensityAt(ECSEngine::Point2D p, const std::vector<LightSample>& lights) {
    float best = 0.0f;
    for (const LightSample& light : lights) {
        const float d = std::sqrt(DistanceSquared(p, light.pos));
        const float t = std::clamp((light.radius - d) / (0.65f * light.radius), 0.0f, 1.0f);
        best = std::max(best, t * t * (3.0f - 2.0f * t));   // smoothstep
    }
    return best;
}