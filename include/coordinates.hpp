#ifndef COORDINATES_H
#define COORDINATES_H

/**
 * Coordinates in the pixel array that is drawn to the screen
 * 
 */
typedef struct PixelCoordinates {
    int x, y;
} PixelCoordinates;

/**
 * Coordinates in the fractal space, which is the complex plane.
 * 
 */
typedef struct FractalCoordinates {
    float x, y;
} FractalCoordinates;

/**
 * Pixel coordinates on the screen. These will be identical to
 * the PixelCoordinates on the screen if the window hasn't
 * been resized and there is no zoom/translating happening.
*/
typedef struct ScreenCoordinates {
    int x, y;
} ScreenCoordinates;

#endif