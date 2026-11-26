#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "MathUtil.h"
#include "SFML/Graphics/Image.hpp"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace ECSEngine {

// Index of a sprite inside the SpriteManager, returned by RegisterTexture.
using SpriteID = size_t;

// Packs every sprite into one spritesheet texture and hands out sprites cut from it
// e.g. SpriteID star = sprites.RegisterTexture(path + "tiles.png", {{640, 320}, 64, 64});
// sprites.GetSprite(star); // in a system, its texture rect is where the star sits in the spritesheet
// sprites.GetTexture();    // the one texture every sprite is drawn from
// See SpriteManager.cpp for how the packing works and how long references stay valid
class SpriteManager {
public:
    static constexpr unsigned int kSheetSize = 2048;    // spritesheet width and height, in pixels
    static constexpr unsigned int kSheetPadding = 2;    // empty pixels between sprites.
    
    SpriteManager();
    ~SpriteManager() = default;

    // No copying or moving: every sprite points at this object's mTexture.
    SpriteManager(const SpriteManager&) = delete;
    SpriteManager& operator=(const SpriteManager&) = delete;
    SpriteManager(SpriteManager&&) = delete;
    SpriteManager& operator=(SpriteManager&&) = delete;

    // Makes a sprite showing the `sourceRect` part (in pixels) of the image. Throws if it can't load.
    [[nodiscard]] SpriteID RegisterTexture(const std::string& texturePath, const Rect& sourceRect);

    // Returns the sprite for `id`. Callers may set its position/scale before drawing.
    sf::Sprite& GetSprite(SpriteID id);

    // The spritesheet: the one texture every sprite is cut from
    sf::Texture& GetTexture();

private:
    // Returns the image for 'path', loading it the first time.
    const sf::Image& LoadImage(const std::string& path);
    // Sends pixels added since the last upload to the GPU texture
    void UploadIfDirty();

    // Shelf packing: sprites fill a row left to right, and a sprite that doesn't
    // fit starts a new row below the tallest sprite in the current one.
    unsigned int mRowMinY = 0;  // top of the current row
    unsigned int mRowMaxY = 0;  // bottom of the current row's tallest sprite
    unsigned int mColMinX = 0;  // where the next sprite in the row goes
    sf::Image mSheet;           // CPU copy: new sprites are copied in here first
    sf::Texture mTexture;       // GPU copy: what actually gets drawn
    bool mDirty = false;        // mSheet has pixels that mTexture doesn't yet

    std::unordered_map<std::string, sf::Image> mSourceImages;   // keyed by file path
    std::unordered_map<std::string, SpriteID> mSpriteMap;       // path to ID
    std::vector<sf::Sprite> mSprites;                           // indexed by SpriteID
};

}  // namespace ECSEngine
