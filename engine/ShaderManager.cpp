#include "ShaderManager.h"

#include <stdexcept>
#include <utility>

namespace ECSEngine {

// Loads fragment shaders and hands them out by name
// How it works
//  - mShaders maps a name to a compiled sf::Shader (a GLSL fragment program)
//  - Only fragment shaders: SFML's built-in vertex stage already passes the
// texture coordinate (gl_TexCoord[0]) and the vertex colour (gl_Color, the sprite's tint)
// through to them
//  - A shader is compiled into a local first and moved into the map only if it worked
//  so a bad file never leaves a broken or half-added entry.
//
// Lifetimes
//  - The ShaderManager owns every shader, callers never free them
//  - A GetShader pointer stays valid for the ShaderManager's while life
// it's elements when it grows, and re-registering a name moves the new shader into the same element
// So a SpriteComponent may keep the pointer, but only for a sprite in the same scene as this manager

void ShaderManager::RegisterShader(const std::string& shaderPath, const std::string& shaderName) {
    if (!sf::Shader::isAvailable()) {
        throw std::runtime_error("ShaderManager: shaders are not supported on this system");
    }

    sf::Shader shader;
    if (!shader.loadFromFile(shaderPath, sf::Shader::Type::Fragment)) {
        throw std::runtime_error("ShaderManager: could not load shader '" + shaderPath + "'");
    }

    // Every draw with this shader samples whatever texture that draw uses
    shader.setUniform("texture", sf::Shader::CurrentTexture);

    mShaders[shaderName] = std::move(shader);
}

sf::Shader* ShaderManager::GetShader(const std::string& shaderName) {
    auto found = mShaders.find(shaderName);
    return found == mShaders.end() ? nullptr : &found->second;
}

} // namespace ECSEngine