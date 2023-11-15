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

enum PathOptions {
    PATH_CONSTANT,
    PATH_SQRT,
    PATH_LINEAR,
    PATH_SQUARE,
};

enum ScoreOptions {
    SCORE_NONE,
    SCORE_SQRT,
    SCORE_SQUARE,
    SCORE_NORM,
    SCORE_SQNORM,
};

std::string getMandelName();

typedef struct WindowSettings {
    uint32_t width, height;
    uint32_t windowW, windowH;
    float zoom = 1, centerX = 0, centerY = 0;
    bool grid = false;
    bool showParticles = false;
    bool showDiff = false;
    bool crossPollinate = false;
    int pathType = PathOptions::PATH_CONSTANT;
    int scoreType = ScoreOptions::SCORE_NONE;
    bool updateView = true;
} WindowSettings;

typedef struct MouseState {
    int xDown, yDown;
    int x, y;
    int state = 1; // GLUT_UP
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
extern uint64_t stepCount;

extern std::vector<std::string> getMandelNames();

#endif
