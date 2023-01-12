#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include "opencl.hpp"

typedef struct WindowSettings {
    uint32_t width, height;
    uint32_t windowW, windowH;
    float zoom = 1, centerX = 0, centerY = 0;
    bool grid = false;
} WindowSettings;

typedef struct MouseState {
    int xDown, yDown;
    int x, y;
    int state;
} MouseState;

typedef struct ViewSettings {
    float scaleX, scaleY;
    float centerX, centerY;
    float theta, sinTheta, cosTheta;
} ViewSettings;

void displayFW();

void createFractalWindow(char *name, uint32_t width, uint32_t height);
void destroyFractalWindow();

extern ViewSettings viewFW;
extern uint32_t *pixelsFW;
extern OpenCl *opencl;

#endif
