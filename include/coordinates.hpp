#ifndef COORDINATES_H
#define COORDINATES_H

#include "fractalWindow.hpp"

// Forward declarations

typedef struct FractalCoordinates FractalCoordinates;
typedef struct PixelCoordinates PixelCoordinates;
typedef struct ScreenCoordinates ScreenCoordinates;

/**
 * Coordinates in the fractal space, which is the complex plane.
 */
struct FractalCoordinates {
    float x, y;
};

/**
 * Coordinates in the pixel array that is drawn to the screen.
 */
struct PixelCoordinates {
    int x, y;
    ScreenCoordinates toScreen(WindowSettings settings);
};

/**
 * Pixel coordinates on the screen. These will be identical to
 * the PixelCoordinates on the screen if the window hasn't
 * been resized and there is no zoom/translating happening.
*/
struct ScreenCoordinates {
    int x, y;
    PixelCoordinates toScreen(WindowSettings settings);
};

#endif