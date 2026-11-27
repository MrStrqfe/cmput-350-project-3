#pragma once

#include <map>
#include <tuple>
#include <utility>

#include "EntityManager.h"
#include "LocationComponent.h"
#include "SpriteComponent.h"
#include "WindowManager.h"
#include "Scene.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/View.hpp>

namespace ECSEngine {

// Texture edges are pulled in by this many texels so rounding never samples the
// spritesheet padding (which shows up as 1 px seams between tiles)
constexpr float kTexInset = 0.05f;

// Adds one texture rectangle to 'va' as two triangles (6 vertices)
inline void AppendQuad(sf::VertexArray& va, const sf::FloatRect& dest, const sf::IntRect& tex, sf::Color tint, bool flipX) {
    const float left = dest.position.x;
    const float top = dest.position.y;
    const float right = left + dest.size.x;
    const float bottom = top + dest.size.y;

    float texLeft = static_cast<float>(tex.position.x) + kTexInset;
    float texRight = static_cast<float>(tex.position.x + tex.size.x) - kTexInset;
    const float texTop = static_cast<float>(tex.position.y) + kTexInset;
    const float texBottom = static_cast<float>(tex.position.y + tex.size.y) - kTexInset;

    if (flipX) {
        std::swap(texLeft, texRight);
    }

    const sf::Vertex topLeft{{left, top}, tint, {texLeft, texTop}};
    const sf::Vertex topRight{{right, top}, tint, {texRight, texTop}};
    const sf::Vertex bottomLeft{{left, bottom}, tint, {texLeft, texBottom}};
    const sf::Vertex bottomRight{{right, bottom}, tint, {texRight, texBottom}};

    va.append(topLeft);
    va.append(topRight);
    va.append(bottomLeft);
    va.append(bottomLeft);
    va.append(topRight);
    va.append(bottomRight);
}

// The camera for a layer that scrolls at 'factor' times the world's speed
// (1 = with the world, 0 = fixed). Only x changes, so layers stay lined up vertically
inline sf::View ParallaxView(const sf::View& camera, float factor) {
    sf::View view = camera;
    view.setCenter({camera.getCenter().x * factor, camera.getCenter().y});
    return view;
}

// Draws every sprite with one draw call per (layer, shader) batch.
// Lower layers first, screen-space sprites (worldSpace == false) last, in window pixels
template <typename... Components>
class SpriteSystem : public System<Components...> {
public:
    // parallaxByLayer[layer] is that layer's scroll speed; unlisted layers use 1.
    explicit SpriteSystem(std::map<int, float> parallaxByLayer = {}) : mParallax(std::move(parallaxByLayer)) {}
    const std::string& GetName() const override {
        static std::string name("SpriteSystem");
        return name;
    }

    bool IsDrawSystem() const override { return true; }

    tSystemReturn Run(EngineView<Components...>* engine, SceneView<Components...>* scene) override {
        auto& entities = scene->GetEntityManager();
        auto& sprites = engine->GetSpriteManager();
        auto& windowManager = scene->GetWindowManager();
        auto window = windowManager.GetWindow();
        const sf::View camera = windowManager.GetView();
        const sf::View pixels = windowManager.GetPixelView();

        // clear() keeps each array's memory, so nothing is allocated after the first frames.
        for (auto& [key, batch] : mBatches) {
            batch.clear();
        }

        // Slots are in creation order, so within a batch earlier sprites are drawn underneath.
        auto& spriteStore = entities.template GetComponentStorage<SpriteComponent>();
        for (size_t i = 0; i < spriteStore.mStorage.size(); ++i) {
            if (!spriteStore.mValid[i]) {
                continue;
            }
            const EntityID owner = spriteStore.mOwner[i];
            if (!entities.template HasComponent<LocationComponent>(owner)) {
                continue;
            }

            const SpriteComponent& sc = spriteStore.mStorage[i];
            const Point2D& loc = entities.template GetComponent<LocationComponent>(owner).loc;
            const sf::FloatRect dest{{loc.x + sc.bounds.topLeft.x, loc.y + sc.bounds.topLeft.y}, {sc.bounds.width, sc.bounds.height}};
            const sf::View view = sc.worldSpace ? ParallaxView(camera, Parallax(sc.layer)) : pixels;

            if (!dest.findIntersection(Visible(view))) {
                // off screen
                continue;
            }

            const BatchKey key{sc.layer, !sc.worldSpace, sc.shader};
            sf::VertexArray& batch = mBatches.try_emplace(key, sf::PrimitiveType::Triangles).first->second;
            AppendQuad(batch, dest, sprites.GetSprite(sc.textureID).getTextureRect(), sc.tint, sc.flipX);
        }

        // The map is sorted by key: world before screen, then by layer.
        sf::RenderStates states;
        states.texture = &sprites.GetTexture();
        for (auto& [key, batch] : mBatches) {
            if (batch.getVertexCount() == 0) {
                continue;
            }
            const auto& [layer, screen, shader] = key;
            window->setView(screen ? pixels : ParallaxView(camera, Parallax(layer)));
            states.shader = shader;
            window->draw(batch, states);
        }

        // No display() here: ECSEngine::Run calls it after the debug drawing
        return kContinue;
    }

private:
    using BatchKey = std::tuple<int, bool, sf::Shader*>;    // (layer, screen space, shader)
    float Parallax(int layer) const {
        auto found = mParallax.find(layer);
        return found == mParallax.end() ? 1.0f : found->second;
    }

    static sf::FloatRect Visible(const sf::View& view) {
        return {view.getCenter() - view.getSize() / 2.0f, view.getSize()};
    }

    std::map<int, float> mParallax;
    std::map<BatchKey, sf::VertexArray> mBatches;   // kept between frames to reuse memory

};

}  // namespace ECSEngine
