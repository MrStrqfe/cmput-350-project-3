#include "SpriteManager.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <utility>

namespace ECSEngine {

// Packs every sprite into one 2048x2048 spritesheet texture
//
// How it works
//  - Sprites are placed left to right in rows; when a row is full, a new one
//  starts below its tallest sprite. 2 px of padding stops neighbours bleeding.
//  - Pixels are copied into mSheet (CPU) and upload to mTexture (GPU) once,
//  on the next GetSprite/GetTexture, not on every RegisterTexture.
//  - Registering the same file and rect again returns the same ID.
//
// Lifetimes
//  - The SpriteManager owns the spritesheet and every sprite
//  - SpriteIDs and the GetTexture reference stay valid for its whole life.
//  - GetSprite references last only until the next RegisterTexture (the vector may grow),
//  so store the SpriteID, not the reference.

namespace {
    // The mSpriteMap key for one (file, rect) pair
    std::string SpriteKey(const std::string& path, const sf::IntRect& r) {
        return path + "@" + std::to_string(r.position.x) + "," + std::to_string(r.position.y) + ","
        + std::to_string(r.size.x) + "," + std::to_string(r.size.y);
    }
}   // namespace

// The texture constructor throws if the GPU can't make a texture this big
SpriteManager::SpriteManager() : mSheet({kSheetSize, kSheetSize}, sf::Color::Transparent),
                                    mTexture(sf::Vector2u{kSheetSize, kSheetSize}) {}

SpriteID SpriteManager::RegisterTexture(const std::string& texturePath, const Rect& sourceRect) {
    const sf::IntRect source ({static_cast<int>(sourceRect.topLeft.x), static_cast<int>(sourceRect.topLeft.y)},
                                        {static_cast<int>(sourceRect.width), static_cast<int>(sourceRect.height)});
    const std::string key = SpriteKey(texturePath, source);
    if (auto found = mSpriteMap.find(key); found != mSpriteMap.end()) {
        return found->second;
    }

    // The rect is inside the image exactly when cutting it to the image changes
    // nothing (this also rejects empty and negative sizes)
    const sf::Image& image = LoadImage(texturePath);
    const sf::IntRect wholeImage({0, 0}, sf::Vector2i(image.getSize()));
    if (wholeImage.findIntersection(source) != source) {
        throw std::runtime_error("SpriteManager: rect is outside the image for '" + key + "'");
    }

    const auto width = static_cast<unsigned int>(source.size.x);
    const auto height = static_cast<unsigned int>(source.size.y);

    // Find the spot first and only move the shelf after the copy worked, so a throw leaves the packing unchanged
    unsigned int x = mColMinX;
    unsigned int y = mRowMinY;
    if (x + width > kSheetSize) {
        // no room left in this row, start a new one
        x = 0;
        y = mRowMaxY;
    }
    if (x + width > kSheetSize || y + height > kSheetSize) {
        throw std::runtime_error("SpriteManager: spritesheet is full, no room for '" + key + "'");
    }
    if (!mSheet.copy(image, {x,y}, source)) {
        throw std::runtime_error("SpriteManager: could not copy '" + key + "' into the spritesheet");
    }

    mRowMinY = y;
    mColMinX = x + width + kSheetPadding;
    mRowMaxY = std::max(mRowMaxY, y + height + kSheetPadding);
    mDirty = true;

    mSprites.emplace_back(mTexture, sf::IntRect({static_cast<int>(x), static_cast<int>(y)}, source.size));
    const SpriteID id = mSprites.size() - 1;
    mSpriteMap.emplace(key, id);
    return id;
}

sf::Sprite& SpriteManager::GetSprite(SpriteID id) {
    assert(id < mSprites.size());
    UploadIfDirty();    // the sprite draws from mTexture, so it must be up to date
    return mSprites[id];
}

sf::Texture& SpriteManager::GetTexture() {
    UploadIfDirty();
    return mTexture;
}

void SpriteManager::UploadIfDirty() {
    if (!mDirty) {
        return;
    }
    mTexture.update(mSheet);
    mDirty = false;
}

const sf::Image& SpriteManager::LoadImage(const std::string& path) {
    auto found = mSourceImages.find(path);
    if (found != mSourceImages.end()) {
        return found->second;
    }

    sf::Image image;
    if (!image.loadFromFile(path)) {
        throw std::runtime_error("SpriteManager: could not load texture '" + path + "'");
    }
    return mSourceImages.emplace(path, std::move(image)).first->second;
}

}  // namespace ECSEngine
