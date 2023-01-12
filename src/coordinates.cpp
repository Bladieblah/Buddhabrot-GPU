#include "coordinates.hpp"
#include "fractalWindow.hpp"

void FractalCoordinate::rotate(float sinTheta, float cosTheta) {
    float tmp = cosTheta * x - sinTheta * y;

    y = sinTheta * x + cosTheta * y;
    x = tmp;
}

PixelCoordinate FractalCoordinate::toPixel(ViewSettings view) {
    FractalCoordinate tmp({
        (x - view.centerX) / view.scaleX,
        (y - view.centerX) / view.scaleY
    });

    tmp.rotate(view.sinTheta, view.cosTheta);

    return (PixelCoordinate) {
        (int)((1 + x) / 2 * view.sizeX),
        (int)((1 + y) / 2 * view.sizeY)
    };
}

FractalCoordinate PixelCoordinate::toFractal(ViewSettings view) {
    FractalCoordinate tmp({
        x * 2 / (float)view.sizeX - 1,
        y * 2 / (float)view.sizeY - 1
    });

    tmp.rotate(-view.sinTheta, view.cosTheta);

    return (FractalCoordinate) {
        tmp.x * view.scaleX + view.centerX,
        tmp.y * view.scaleY + view.centerY
    };
}

ScreenCoordinate PixelCoordinate::toScreen(WindowSettings settings) {
    return (ScreenCoordinate) {
        (int)((x / settings.width  - settings.centerX) * settings.zoom * settings.windowW),
        (int)((x / settings.height - settings.centerY) * settings.zoom * settings.windowH)
    };
}

PixelCoordinate ScreenCoordinate::toPixel(WindowSettings settings) {
    return (PixelCoordinate) {
        (int)((x / (settings.windowW * settings.zoom) + settings.centerX) * settings.width),
        (int)((x / (settings.windowH * settings.zoom) + settings.centerY) * settings.height)
    };
}
