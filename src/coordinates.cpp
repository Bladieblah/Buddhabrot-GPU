#include "coordinates.hpp"
#include "fractalWindow.hpp"

void FractalCoordinate::rotate(float sinTheta, float cosTheta) {
    float tmp = cosTheta * x - sinTheta * y;

    y = sinTheta * x + cosTheta * y;
    x = tmp;
}

PixelCoordinate FractalCoordinate::toPixel(ViewSettings view) {
    FractalCoordinate tmp({
        (x - view.centerX),
        (y - view.centerY)
    });

    tmp.rotate(view.sinTheta, view.cosTheta);

    return (PixelCoordinate) {
        (int)((1 + tmp.x / view.scaleX) / 2 * view.sizeX),
        (int)((1 + tmp.y / view.scaleY) / 2 * view.sizeY)
    };
}

FractalCoordinate PixelCoordinate::toFractal(ViewSettings view) {
    FractalCoordinate tmp({
        (2 * (x / (float)view.sizeX) - 1) * view.scaleX,
        (2 * (y / (float)view.sizeY) - 1) * view.scaleY
    });

    tmp.rotate(-view.sinTheta, view.cosTheta);

    return (FractalCoordinate) {
        tmp.x + view.centerX,
        tmp.y + view.centerY
    };
}

ScreenCoordinate PixelCoordinate::toScreen(WindowSettings settings) {
    return (ScreenCoordinate) {
        (int)((x / (float)settings.width  - settings.centerX) * settings.zoom * settings.windowW),
        (int)((y / (float)settings.height - settings.centerY) * settings.zoom * settings.windowH)
    };
}

PixelCoordinate ScreenCoordinate::toPixel(WindowSettings settings) {
    return (PixelCoordinate) {
        (int)((x / (float)(settings.windowW * settings.zoom) + settings.centerX) * settings.width),
        (int)(((settings.windowH - y) / (float)(settings.windowH * settings.zoom) + settings.centerY) * settings.height)
    };
}
