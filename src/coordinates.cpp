#include "coordinates.hpp"
#include "fractalWindow.hpp"

void FractalCoordinate::rotate(float sinTheta, float cosTheta) {
    float tmp = cosTheta * x - sinTheta * y;

    y = sinTheta * x + cosTheta * y;
    x = tmp;
}

PixelfCoordinate FractalCoordinate::toPixelf(ViewSettings view) {
    FractalCoordinate tmp({
        (x - view.centerX),
        (y - view.centerY)
    });

    tmp.rotate(view.sinTheta, view.cosTheta);

    return (PixelfCoordinate) {
        (1 + tmp.x / view.scaleX) / 2 * view.sizeX,
        (1 + tmp.y / view.scaleY) / 2 * view.sizeY
    };
}

PixelCoordinate FractalCoordinate::toPixel(ViewSettings view) {
    PixelfCoordinate tmp = toPixelf(view);
    
    return (PixelCoordinate){
        (int)tmp.x,
        (int)tmp.y
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

FractalCoordinate complex_mul(FractalCoordinate f1, FractalCoordinate f2) {
    return {f1.x * f2.x - f1.y * f2.y, f1.x * f2.y + f1.y * f2.x};
}

FractalCoordinate complex_square(FractalCoordinate f) {
    return {f.x * f.x - f.y * f.y, 2 * f.x * f.y};
}

FractalCoordinate operator+(FractalCoordinate f1, FractalCoordinate f2) {
    return {f1.x + f2.x, f1.y + f2.y};
}

FractalCoordinate operator*(float x, FractalCoordinate f) {
    return {x * f.x, x * f.y};
}

FractalCoordinate operator*(FractalCoordinate f, float x) {
    return {x * f.x, x * f.y};
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
