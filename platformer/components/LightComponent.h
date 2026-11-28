#pragma once

// Makes an entity give off light (see LightSystem and lighting.frag)
// The light is centred on the entity's sprite
struct LightComponent {
    float radius;   // world units
    float flicker;  // how much the radius wobbles, e.g. 0.03 = 3%
};