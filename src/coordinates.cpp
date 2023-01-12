#include "coordinates.hpp"
#include "fractalWindow.hpp"

ScreenCoordinates PixelCoordinates::toScreen(WindowSettings settings) {
    return (ScreenCoordinates) {
        (int)((x / settings.width  - settings.centerX) * settings.zoom * settings.windowW),
        (int)((x / settings.height - settings.centerY) * settings.zoom * settings.windowH)
    };
}

PixelCoordinates ScreenCoordinates::toScreen(WindowSettings settings) {
    return (PixelCoordinates) {
        (int)((x / (settings.windowW * settings.zoom) + settings.centerX) * settings.width),
        (int)((x / (settings.windowH * settings.zoom) + settings.centerY) * settings.height)
    };
}