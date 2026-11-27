#pragma once

#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

// Owns the game window and one scene's camera (an sf::View), and converts
// between world and window coordinates.
// Before drawing, call window->setView(GetView()) for world coordinates or
// window->setView(GetPixelView()) for window pixels. Never rely on the view
// set by whatever drew last.
class WindowManager {
public:
    WindowManager(unsigned int width, unsigned int height, const std::string& title);
    // Shares an existing window but keeps its own camera (used by each scene).
    explicit WindowManager(std::shared_ptr<sf::RenderWindow> window);
    ~WindowManager() = default;
    std::shared_ptr<sf::RenderWindow> GetWindow() const;
    Rect GetWindowBounds() const;

    void SetCamera(const Point2D& worldPt, const Point2D& screenPt);
    void SetWorldCenter(const Point2D& worldCenter);
    void SetWorldRect(const Rect& worldCameraRect);
    void SetWorldScale(float worldUnitsPerPixel);
    Point2D GetWorldCenter() const;
    float GetWorldScale() const;

    float WindowToWorld(float x) const;
    float WorldToWindow(float x) const;
    Rect WindowToWorld(const Rect& rect) const;
    Rect WorldToWindow(const Rect& rect) const;
    Point2D WorldToWindow(const Point2D& pt) const;
    Point2D WindowToWorld(const Point2D& pt) const;
    sf::View GetView() const;
    sf::View GetPixelView() const;
    void SetLogicalSize(float width, float height);

    void DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha = 255);
    void DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha = 255);

private:
    std::shared_ptr<sf::RenderWindow> mWindow;
    sf::View mView;                                 // its center is the camera; its size is used only if fixed
    float mWorldUnitsPerPixel = 1.0f;               // 1:1 until the game sets a scale
    bool mFixedSize = false;                        // true after SetLogicalSize
};

}  // namespace ECSEngine
