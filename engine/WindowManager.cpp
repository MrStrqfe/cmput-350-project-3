#include <algorithm>
#include <SFML/Graphics/Rect.hpp>

#include "WindowManager.h"

namespace ECSEngine {


// The camera is an sf::View, resized to fit the window on every call
// SetLogicalSize fixes its size instead and adds bars to keep its shape.
// Conversions keep fractions of a pixel, so tiles don't get 1 px gaps.
// Converted values use the current camera, so don't keep them across frames.
WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)) {
        mView = GetPixelView();
    }

WindowManager::WindowManager(std::shared_ptr<sf::RenderWindow> window) : mWindow(std::move(window)) {
    mView = GetPixelView();
}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

// The whole window as a rect at (0, 0), in window pixels.
Rect WindowManager::GetWindowBounds() const {
    sf::Vector2f windowBounds{mWindow->getSize()};
    return Rect{
        .topLeft = Point2D{0.0f, 0.0f},
        .width = windowBounds.x,
        .height = windowBounds.y
    };
}

// Fills `r` (window coordinates) with the given colour.
void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    sf::RectangleShape rectangle{sf::Vector2f{r.width, r.height}};
    rectangle.setPosition({r.topLeft.x, r.topLeft.y});
    rectangle.setFillColor(sf::Color{
        static_cast<uint8_t>(red),
        static_cast<uint8_t>(green),
        static_cast<uint8_t>(blue),
        static_cast<uint8_t>(alpha)
    });
    mWindow->draw(rectangle);
}

// Draws a 1-pixel outline of `r` (window coordinates).
void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    sf::RectangleShape rectangle{sf::Vector2f{r.width, r.height}};
    rectangle.setPosition({r.topLeft.x, r.topLeft.y});
    rectangle.setFillColor(sf::Color::Transparent);
    rectangle.setOutlineColor(sf::Color{
        static_cast<uint8_t>(red),
        static_cast<uint8_t>(green),
        static_cast<uint8_t>(blue),
        static_cast<uint8_t>(alpha)
    });
    rectangle.setOutlineThickness(-1);  // negative draws the outline inside the rect
    mWindow->draw(rectangle);
}

// Rects: the corner converts as a point, the width and height as distances.
Rect WindowManager::WindowToWorld(const Rect& rect) const {
    return Rect{WindowToWorld(rect.topLeft),WindowToWorld(rect.width), WindowToWorld(rect.height)};
}

// Reverse of WorldtoWindow
Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    const sf::View view = GetView();
    const sf::FloatRect port {mWindow->getViewport(view)};
    const sf::Vector2f ndc{(pt.x - port.position.x) / port.size.x * 2.0f - 1.0f, 1.0f - (pt.y - port.position.y) / port.size.y * 2.0f};
    const sf::Vector2f world = view.getInverseTransform().transformPoint(ndc);
    return Point2D{world.x, world.y};
}

Rect WindowManager::WorldToWindow(const Rect& rect) const {
    return Rect{WorldToWindow(rect.topLeft), WorldToWindow(rect.width), WorldToWindow(rect.height)};
}

// The view's transform maps world points to -1 to 1 across its viewport
// the viewport then places that in window pixels (y down)
Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    const sf::View view = GetView();
    const sf::FloatRect port{mWindow->getViewport(view)};
    const sf::Vector2f ndc = view.getTransform().transformPoint({pt.x, pt.y});
    return Point2D{port.position.x + (ndc.x + 1.0f) / 2.0f * port.size.x,
                    port.position.y + (1.0f - ndc.y) / 2.0f * port.size.y};
}

// Moves the camera so `worldPt` shows up at window position `screenPt`.
void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    const Point2D now = WindowToWorld(screenPt);    // what is at screenPt now
    mView.move({worldPt.x - now.x, worldPt.y - now.y});
}

Point2D WindowManager::GetWorldCenter() const {
    return Point2D{mView.getCenter().x, mView.getCenter().y};
}

float WindowManager::GetWorldScale() const {
    return WindowToWorld(1.0f);
}

void WindowManager::SetWorldCenter(const Point2D& worldCenter) {
    mView.setCenter({worldCenter.x, worldCenter.y});
}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    mWorldUnitsPerPixel = worldUnitsPerPixel;
    mFixedSize = false;
    mView.setViewport(sf::FloatRect{{0.0f, 0.0f}, {1.0f, 1.0f}});
}

// Centers the camera on the rect. The zoom is left unchanged.
void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    SetWorldCenter(Point2D{worldCameraRect.topLeft.x + worldCameraRect.width / 2,
                            worldCameraRect.topLeft.y + worldCameraRect.height / 2});
}

// Distances only need the zoom: view width in world units vs viewport in pixels.
float WindowManager::WindowToWorld(float x) const {
    const sf::View view = GetView();
    return x * view.getSize().x / static_cast<float>(mWindow->getViewport(view).size.x);
}

float WindowManager::WorldToWindow(float x) const {
    const sf::View view = GetView();
    return x * static_cast<float>(mWindow->getViewport(view).size.x) / view.getSize().x;
}

sf::View WindowManager::GetView() const {
    sf::View view = mView;
    const sf::Vector2f window{mWindow->getSize()};
    if (!mFixedSize) {
        view.setSize(window * mWorldUnitsPerPixel);
        return view;
    }
    // Letterbox: the biggest rect with the view's shape that fits the window
    // centered. Viewports are fractions of the window (0 to 1)
    const sf::Vector2f size = view.getSize();
    const float fit = std::min(window.x / size.x, window.y / size.y);
    const sf::Vector2f part{size.x * fit / window.x, size.y * fit / window.y};
    view.setViewport(sf::FloatRect{{(1.0f - part.x) / 2.0f, (1.0f - part.y) / 2.0f}, part});
    return view;
}

sf::View WindowManager::GetPixelView() const {
    return sf::View{sf::FloatRect{{0.0f, 0.0f}, sf::Vector2f{mWindow->getSize()}}};
}

void WindowManager::SetLogicalSize(float width, float height) {
    mView = sf::View{sf::FloatRect{{0.0f, 0.0f}, {width, height}}};
    mFixedSize = true;
}


}  // namespace ECSEngine
