#pragma once

#include <string>
#include <unordered_map>

#include <SFML/Graphics/Shader.hpp>

namespace ECSEngine {

// Loads fragment shaders and hands them out by name,
// e.g. shaders.RegisterShader(path + "shaders/lighting.frag", "lighting")
// while building a scene sprite.shader = shaders.GetShader("lighting");
// Each scene owns one (see SceneView::GetShaderManager).
// See ShaderManager.cpp for how it works on how long pointers stay valid

class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager() = default;

    // No copying: sprites hold pointers to these exact shaders.
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    // Loads the fragment shader at 'shaderPath' as 'shaderName'
    // Registering a name again replaces that shader in place.
    // Throws if shaders aren't supported or the file can't be loaded or compiled.
    // nothing changes when it throws. Ever shader must declar uniform sampler2D texture.
    void RegisterShader(const std::string& shaderPath, const std::string& shaderName);

    // Returns the shader name 'shaderName', or nullptr for an unknown name
    // (the SpriteSystem draws a nullptr shader as plain, unshaded sprites)
    sf::Shader* GetShader(const std::string& shaderName);

private:
    std::unordered_map<std::string, sf::Shader> mShaders;   //keyed by shader name
};

} // namespace ECSEngine