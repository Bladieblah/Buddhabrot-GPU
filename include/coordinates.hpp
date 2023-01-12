#ifndef COORDINATES_H
#define COORDINATES_H

#include "fractalWindow.hpp"

/**
 * Coordinates in the fractal space, which is the complex plane.
 */
typedef struct FractalCoordinates {
    float x, y;

    PixelCoordinates toPixel() {

    }
} FractalCoordinates;

/**
 * Coordinates in the pixel array that is drawn to the screen.
 */
typedef struct PixelCoordinates {
    int x, y;

    ScreenCoordinates toScreen(WindowSettings settings) {
        return (ScreenCoordinates) {
            (x / settings.width  - settings.centerX) * settings.zoom * settings.windowW,
            (x / settings.height - settings.centerY) * settings.zoom * settings.windowH
        };
    }
} PixelCoordinates;

/**
 * Pixel coordinates on the screen. These will be identical to
 * the PixelCoordinates on the screen if the window hasn't
 * been resized and there is no zoom/translating happening.
*/
typedef struct ScreenCoordinates {
    int x, y;

    PixelCoordinates toScreen(WindowSettings settings) {
        return (PixelCoordinates) {
            (x / (settings.windowW * settings.zoom) + settings.centerX) * settings.width,
            (x / (settings.windowH * settings.zoom) + settings.centerY) * settings.height
        };
    }
} ScreenCoordinates;

#endif