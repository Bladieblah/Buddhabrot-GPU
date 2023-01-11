#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include "opencl.hpp"

typedef struct WindowSettings {
    uint32_t width, height;
    float scale = 1, centerX = 0, centerY = 0;
} WindowSettings;

typedef struct MouseState {
    int xDown, yDown;
    int x, y;
    int state;
} MouseState;

typedef struct ViewSettings {
    float scale, centerX, centerY;
    float theta;
} ViewSettings;

void displayFW();

void createFractalWindow(char *name, uint32_t width, uint32_t height);
void destroyFractalWindow();

extern uint32_t *pixelsFW;
extern OpenCl *opencl;

#endif
