#ifndef COORDINATES_H
#define COORDINATES_H

#include "fractalWindow.hpp"

// Forward declarations

typedef struct FractalCoordinate FractalCoordinate;
typedef struct PixelCoordinate PixelCoordinate;
typedef struct PixelfCoordinate PixelfCoordinate;
typedef struct ScreenCoordinate ScreenCoordinate;

/**
 * Coordinates in the fractal space, which is the complex plane.
 */
struct FractalCoordinate {
    float x, y;
    PixelCoordinate toPixel(ViewSettings view);
    PixelfCoordinate toPixelf(ViewSettings view);
    void rotate(float sinTheta, float cosTheta);
};

FractalCoordinate complex_mul(FractalCoordinate f1, FractalCoordinate f2);
FractalCoordinate complex_square(FractalCoordinate f);
FractalCoordinate operator+(FractalCoordinate f1, FractalCoordinate f2);
FractalCoordinate operator*(float x, FractalCoordinate f);
FractalCoordinate operator*(FractalCoordinate f, float x);

/**
 * Coordinates in the pixel array that is drawn to the screen.
 */
struct PixelCoordinate {
    int x, y;
    FractalCoordinate toFractal(ViewSettings view);
    ScreenCoordinate toScreen(WindowSettings settings);
};

/**
 * Coordinates in the pixel array that is drawn to the screen as floats.
 */
struct PixelfCoordinate {
    float x, y;
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