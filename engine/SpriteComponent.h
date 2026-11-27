#pragma once

#include "MathUtil.h"
#include "SpriteManager.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// A sprite drawn at the entity's LocationComponent; bounds is relative to it.
// shader comes from this scene's ShaderManager and is valid while the scene lives.
class SpriteComponent {
public:
    SpriteComponent(SpriteID id, Rect spriteBounds, bool inWorld, int drawLayer = 0) :
                    textureID(id), bounds(spriteBounds), worldSpace(inWorld), layer(drawLayer) {}
    SpriteID textureID;
    Rect bounds;
    bool worldSpace;    // If false, this sprite is drawn in screen space instead of world space
    int layer;          // lower layers are drawn first
    sf::Shader* shader = nullptr;       // nullptr = plain texture
    sf::Color tint = sf::Color::White;  // vertex colour; shaders can read it as gl_Color
    bool flipX = false;                 // mirror left-right
};

}  // namespace ECSEngine
