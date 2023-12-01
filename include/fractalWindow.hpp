#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include <GLFW/glfw3.h>

#include "config.hpp"
#include "opencl.hpp"

typedef struct Particle {
    cl_float2 pos;
    cl_float2 offset, prevOffset;
    unsigned int iterCount, bestIter;
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
    int windowW, windowH;
    float zoom = 1, centerX = 0, centerY = 0;
    bool grid = false;
    bool showParticles = false;
    bool showDiff = false;
    bool crossPollinate = false;
    int pathType = PathOptions::PATH_SQUARE;
    int scoreType = ScoreOptions::SCORE_SQUARE;
    bool updateView = true;
} WindowSettings;

typedef struct MouseState {
    double xDown, yDown;
    double x, y;
    int state = GLFW_RELEASE;
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
extern uint32_t *maximumCounts;
extern GLFWwindow *windowFW;

extern uint32_t prevMax;

extern float frameTime;
extern uint32_t iterCount;
extern uint64_t stepCount;

extern std::vector<std::string> getMandelNames();

#endif
