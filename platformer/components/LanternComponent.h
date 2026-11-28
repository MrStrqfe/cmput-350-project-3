#pragma once

// The player's lantern. Oil drains every frame and sets the light's radius:
// full = maxRadius, empty = minRadius. A flare (F) costs oil and briefly
// makes the light bigger. Running out sends the player back to the respawn point
struct LanternComponent {
    float oil = 100.0f;
    float maxOil = 100.0f;
    float drainPerFrame = 100.0f / (45 * 60);   // a full tank lasts 45 s
    float minRadius = 90.0f;                    // world units
    float maxRadius = 260.0f;
    int flareFrames = 0;                        // frames of flare left, 0 = not flaring             
};
