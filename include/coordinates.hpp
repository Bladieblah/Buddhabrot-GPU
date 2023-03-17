#ifndef COORDINATES_H
#define COORDINATES_H

#include "fractalWindow.hpp"

// Forward declarations

typedef struct FractalCoordinate FractalCoordinate;
typedef struct PixelCoordinate PixelCoordinate;
typedef struct ScreenCoordinate ScreenCoordinate;

/**
 * Coordinates in the fractal space, which is the complex plane.
 */
struct FractalCoordinate {
    float x, y;
    PixelCoordinate toPixel(ViewSettings view);
    void rotate(float sinTheta, float cosTheta);
};

FractalCoordinate complex_mul(FractalCoordinate f1, FractalCoordinate f2) {
    return {f1.x * f2.x - f1.y * f2.y, f1.x * f2.y + f1.y * f2.x};
}

FractalCoordinate complex_square(FractalCoordinate f) {
    return {f.x * f.x - f.y * f.y, 2 * f.x * f.y};
}

FractalCoordinate operator+(FractalCoordinate f1, FractalCoordinate f2) {
    return {f1.x + f2.x, f1.y + f2.y};
}

/**
 * Coordinates in the pixel array that is drawn to the screen.
 */
struct PixelCoordinate {
    int x, y;
    FractalCoordinate toFractal(ViewSettings view);
    ScreenCoordinate toScreen(WindowSettings settings);
};

/**
 * Pixel coordinates on the screen. These will be identical to
 * the PixelCoordinate on the screen if the window hasn't
 * been resized and there is no zoom/translating happening.
*/
struct ScreenCoordinate {
    int x, y;
    PixelCoordinate toPixel(WindowSettings settings);
};

#endif