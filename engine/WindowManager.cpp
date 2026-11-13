#include "WindowManager.h"

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)),
      mWorldCenter(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f),
      mWorldUnitsPerPixel(1.0f) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const {
    const sf::Vector2u size = mWindow->getSize();
    return {
        Point2D(0, 0),
        static_cast<float>(size.x),
        static_cast<float>(size.y)
    };
}

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {}

Rect WindowManager::WindowToWorld(const Rect& rect) const {
    return {
        WindowToWorld(rect.topLeft),
        WindowToWorld(rect.width),
        WindowToWorld(rect.height)
    };
}

Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    const sf::Vector2u size = mWindow->getSize();

    return {
        (pt.x - static_cast<float>(size.x) / 2.0f) * mWorldUnitsPerPixel + mWorldCenter.x,
        (pt.y - static_cast<float>(size.y) / 2.0f) * mWorldUnitsPerPixel + mWorldCenter.y
    };
}

Rect WindowManager::WorldToWindow(const Rect& rect) const {
    return {
        WorldToWindow(rect.topLeft),
        WorldToWindow(rect.width),
        WorldToWindow(rect.height)
    };
}

Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    const sf::Vector2u size = mWindow->getSize();

    return {
        (pt.x - mWorldCenter.x) / mWorldUnitsPerPixel + static_cast<float>(size.x) / 2.0f,
        (pt.y - mWorldCenter.y) / mWorldUnitsPerPixel + static_cast<float>(size.y) / 2.0f
    };
}

void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    const sf::Vector2u size = mWindow->getSize();
    mWorldCenter = {
        worldPt.x - (screenPt.x - static_cast<float>(size.x) / 2.0f) * mWorldUnitsPerPixel,
        worldPt.y - (screenPt.y - static_cast<float>(size.y) / 2.0f) * mWorldUnitsPerPixel
    };
}

Point2D WindowManager::GetWorldCenter() const { return mWorldCenter; }

float WindowManager::GetWorldScale() const { return mWorldUnitsPerPixel; }

void WindowManager::SetWorldCenter(const Point2D& worldCenter) {
    mWorldCenter = worldCenter;
}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    if (worldUnitsPerPixel <= 0) {
        throw std::invalid_argument("World units per pixel must be positive.");
    }
    mWorldUnitsPerPixel = worldUnitsPerPixel;
}

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {}

float WindowManager::WindowToWorld(float x) const { return x * mWorldUnitsPerPixel; }

float WindowManager::WorldToWindow(float x) const { return x / mWorldUnitsPerPixel; }

}  // namespace ECSEngine
