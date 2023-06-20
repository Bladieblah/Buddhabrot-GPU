#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include "config.hpp"
#include "opencl.hpp"

typedef struct Particle {
    cl_float2 pos;
    cl_float2 offset, prevOffset;
    unsigned int iterCount;
    float score, prevScore;
} Particle;

typedef struct WindowSettings {
    uint32_t width, height;
    uint32_t windowW, windowH;
    float zoom = 1, centerX = 0, centerY = 0;
    bool grid = false;
    bool showParticles = false;
    bool showDiff = false;
    bool crossPollinate = false;
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
    int sizeX, sizeY;
} ViewSettings;

void displayFW();

void createFractalWindow(char *name, uint32_t width, uint32_t height);
void destroyFractalWindow();

extern ViewSettings viewFW, defaultView;
extern WindowSettings settingsFW;
extern uint32_t *pixelsFW;
extern OpenCl *opencl;
extern Config *config;

extern float frameTime;
extern uint32_t iterCount;

#endif
