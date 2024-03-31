#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <math.h>

#include <GLFW/glfw3.h>

#include "config.hpp"
#include "fractalWindow.hpp"
#include "opencl.hpp"
#include "pcg.hpp"

using namespace std;

/**
 * Declarations
 */

Config *config;
unsigned int maximaKernelSize;

chrono::high_resolution_clock::time_point timePoint;
unsigned int frameCount = 0;
float frameTime = 0;
uint32_t iterCount = 0;
uint64_t stepCount = 0;

typedef struct FractalCoord {
    cl_float2 pos;
} FractalCoord;

/**
 * OpenCL
 */

OpenCl *opencl;
uint64_t *initState, *initSeq;
uint32_t *maximumCounts;

uint32_t prevMax = 0;

vector<BufferSpec> bufferSpecs;
void createBufferSpecs() {
    bufferSpecs = {
        {"image",     {NULL, 3 * config->width * config->height * sizeof(uint32_t)}},
        {"count",     {NULL, config->threshold_count * config->width * config->height * sizeof(uint32_t)}},
        {"prevCount", {NULL, config->threshold_count * config->width * config->height * sizeof(uint32_t)}},
        {"countDiff", {NULL, config->threshold_count * config->width * config->height * sizeof(uint32_t)}},
        {"particles", {NULL, config->particle_count * sizeof(Particle)}},
        {"path",      {NULL, config->particle_count * config->thresholds[config->threshold_count - 1] * sizeof(FractalCoord)}},
        {"threshold", {NULL, config->threshold_count * sizeof(uint32_t)}},

        {"maxima", {NULL, config->threshold_count * maximaKernelSize * sizeof(uint32_t)}},
        {"maximum", {NULL, config->threshold_count * sizeof(uint32_t)}},

        {"randomState",     {NULL, config->particle_count * sizeof(uint64_t)}},
        {"randomIncrement", {NULL, config->particle_count * sizeof(uint64_t)}},
        {"initState",       {NULL, config->particle_count * sizeof(uint64_t)}},
        {"initSeq",         {NULL, config->particle_count * sizeof(uint64_t)}},
    };
}

vector<string> pathExtenstions = {
    "constant",
    "sqrt",
    "linear",
    "square",
};

vector<string> scoreExtenstions = {
    "none",
    "sqrt",
    "square",
    "norm",
    "sqnorm",
};

vector<string> getMandelNames() {
    vector<string> names;

    char tmp[100];
    for (string path : pathExtenstions) {
        for (string score : scoreExtenstions) {
            sprintf(tmp, "mandelStep_%s_%s", path.c_str(), score.c_str());
            names.push_back(tmp);
        }
    }

    return names;
}

string getMandelName() {
    string path;
    string score;

    switch (settingsFW.pathType) {
        default:
        case (PathOptions::PATH_CONSTANT):
            path = "constant";
            break;
        case (PathOptions::PATH_SQRT):
            path = "sqrt";
            break;
        case (PathOptions::PATH_LINEAR):
            path = "linear";
            break;
        case (PathOptions::PATH_SQUARE):
            path = "square";
            break;
    }

    switch (settingsFW.scoreType) {
        default:
        case (ScoreOptions::SCORE_NONE):
            score = "none";
            break;
        case (ScoreOptions::SCORE_SQRT):
            score = "sqrt";
            break;
        case (ScoreOptions::SCORE_SQUARE):
            score = "square";
            break;
        case (ScoreOptions::SCORE_NORM):
            score = "norm";
            break;
        case (ScoreOptions::SCORE_SQNORM):
            score = "sqnorm";
            break;
    }

    char result[100];
    sprintf(result, "mandelStep_%s_%s", path.c_str(), score.c_str());

    return result;
}

vector<KernelSpec> kernelSpecs;
void createKernelSpecs() {
    kernelSpecs = {
        {"seedNoise",      {NULL, 1, {config->particle_count, 0}, {128, 0}, "seedNoise"}},
        {"initParticles",  {NULL, 1, {config->particle_count, 0}, {128, 0}, "initParticles"}},
        {"resetCount",     {NULL, 1, {config->threshold_count * maximaKernelSize, 0}, {0, 0}, "resetCount"}},
        {"findMax1",       {NULL, 1, {config->threshold_count * maximaKernelSize, 0}, {0, 0}, "findMax1"}},
        {"findMax2",       {NULL, 1, {config->threshold_count, 0}, {config->threshold_count, 0}, "findMax2"}},
        {"findMaxDiff",    {NULL, 1, {config->threshold_count * maximaKernelSize, 0}, {0, 0}, "findMax1"}},
        {"renderImage",    {NULL, 2, {config->width, config->height}, {0, 0}, "renderImage"}},
        {"renderImageD",   {NULL, 2, {config->width, config->height}, {0, 0}, "renderImage"}},
        {"updateDiff",     {NULL, 2, {config->width, config->height}, {0, 0}, "updateDiff"}},
    };

    for (string name : getMandelNames()) {
        kernelSpecs.push_back({name, {NULL, 1, {config->particle_count, 0}, {128, 0}, name}});
    }
}

void setKernelArgs() {
    opencl->setKernelBufferArg("seedNoise", 0, "randomState");
    opencl->setKernelBufferArg("seedNoise", 1, "randomIncrement");
    opencl->setKernelBufferArg("seedNoise", 2, "initState");
    opencl->setKernelBufferArg("seedNoise", 3, "initSeq");

    for (string name : getMandelNames()) {
        opencl->setKernelBufferArg(name, 0, "particles");
        opencl->setKernelBufferArg(name, 1, "count");
        opencl->setKernelBufferArg(name, 2, "threshold");
        opencl->setKernelBufferArg(name, 3, "path");
        opencl->setKernelBufferArg(name, 4, "randomState");
        opencl->setKernelBufferArg(name, 5, "randomIncrement");
        opencl->setKernelArg(name, 6, sizeof(unsigned int), (void*)&(config->threshold_count));
        opencl->setKernelArg(name, 7, sizeof(ViewSettings), (void*)&viewFW);
    }
    
    opencl->setKernelBufferArg("initParticles", 0, "particles");
    opencl->setKernelBufferArg("initParticles", 1, "threshold");
    opencl->setKernelBufferArg("initParticles", 2, "path");
    opencl->setKernelBufferArg("initParticles", 3, "randomState");
    opencl->setKernelBufferArg("initParticles", 4, "randomIncrement");
    opencl->setKernelArg("initParticles", 5, sizeof(unsigned int), (void*)&(config->threshold_count));
    
    opencl->setKernelBufferArg("resetCount", 0, "count");
    opencl->setKernelArg("resetCount", 1, sizeof(unsigned int), (void*)&(config->maximum_size));

    opencl->setKernelBufferArg("findMax1", 0, "count");
    opencl->setKernelBufferArg("findMax1", 1, "maxima");
    opencl->setKernelArg("findMax1", 2, sizeof(unsigned int), (void*)&(config->maximum_size));
    
    opencl->setKernelBufferArg("findMax2", 0, "maxima");
    opencl->setKernelBufferArg("findMax2", 1, "maximum");
    opencl->setKernelArg("findMax2", 2, sizeof(unsigned int), (void*)&maximaKernelSize);

    opencl->setKernelBufferArg("findMaxDiff", 0, "countDiff");
    opencl->setKernelBufferArg("findMaxDiff", 1, "maxima");
    opencl->setKernelArg("findMaxDiff", 2, sizeof(unsigned int), (void*)&(config->maximum_size));
    
    opencl->setKernelBufferArg("renderImage", 0, "count");
    opencl->setKernelBufferArg("renderImage", 1, "maximum");
    opencl->setKernelBufferArg("renderImage", 2, "image");
    opencl->setKernelArg("renderImage", 3, sizeof(unsigned int), (void*)&(config->threshold_count));
    
    opencl->setKernelBufferArg("renderImageD", 0, "countDiff");
    opencl->setKernelBufferArg("renderImageD", 1, "maximum");
    opencl->setKernelBufferArg("renderImageD", 2, "image");
    opencl->setKernelArg("renderImageD", 3, sizeof(unsigned int), (void*)&(config->threshold_count));
    
    opencl->setKernelBufferArg("updateDiff", 0, "count");
    opencl->setKernelBufferArg("updateDiff", 1, "prevCount");
    opencl->setKernelBufferArg("updateDiff", 2, "countDiff");
    opencl->setKernelArg("updateDiff", 3, sizeof(float), (void*)&(config->alpha));
    opencl->setKernelArg("updateDiff", 4, sizeof(unsigned int), (void*)&(config->threshold_count));
}

void initPcg() {
    for (int i = 0; i < config->particle_count; i++) {
        initState[i] = pcg32_random();
        initSeq[i] = pcg32_random();
    }

    opencl->writeBuffer("initState", (void *)initState);
    opencl->writeBuffer("initSeq", (void *)initSeq);
    opencl->step("seedNoise");
    opencl->flush();

    free(initState);
    free(initSeq);
}

void prepareOpenCl() {
    createBufferSpecs();
    createKernelSpecs();

    opencl = new OpenCl(
        "shaders/buddha.cl",
        bufferSpecs,
        kernelSpecs,
        config->profile,
        true,
        config->verbose
    );

    setKernelArgs();
    
    initPcg();
    opencl->writeBuffer("threshold", &(config->thresholds));
    opencl->step("initParticles");
}

void prepare() {
    pcg32_srandom(time(NULL) ^ (intptr_t)&printf, (intptr_t)&(config->particle_count));

    initState = (uint64_t *)malloc(config->particle_count * sizeof(uint64_t));
    initSeq = (uint64_t *)malloc(config->particle_count * sizeof(uint64_t));

    maximumCounts = (uint32_t *)malloc(config->threshold_count * sizeof(uint32_t));

    float scaleY = config->scale;
    viewFW = {
        scaleY / (float)config->height * (float)config->width, scaleY,
        config->center_x, config->center_y,
        config->theta, sin(config->theta), cos(config->theta),
        (int)config->width, (int)config->height
    };

    defaultView = viewFW;

    prepareOpenCl();
}

void display() {
    frameCount++;
    opencl->startFrame();

    if (frameCount % 2 == 0) {
        return;
    }
    
    displayFW();

    // if (maximumCounts[config->threshold_count - 1] - prevMax > config->reset_count) {
    //     prevMax = maximumCounts[config->threshold_count - 1];
    //     opencl->step("initParticles");
    // }

    opencl->step(getMandelName(), config->frame_steps);
    opencl->step("updateDiff");

    if (settingsFW.showDiff) {
        opencl->step("findMaxDiff");
        opencl->step("findMax2");
        opencl->step("renderImageD");
    } else {
        opencl->step("findMax1");
        opencl->step("findMax2");
        opencl->step("renderImage");
    }
    
    opencl->readBuffer("maximum", maximumCounts);

    if (settingsFW.updateView) {
        opencl->readBuffer("image", pixelsFW);
    }

    opencl->flush();

    iterCount++;
    stepCount += config->frame_steps * config->particle_count * 4000;

    chrono::high_resolution_clock::time_point temp = chrono::high_resolution_clock::now();
    chrono::duration<float> time_span = chrono::duration_cast<chrono::duration<float>>(temp - timePoint);
    frameTime = time_span.count();
    
    fprintf(stderr, "Step = %d, time = %.4g            \n", frameCount / 2, frameTime);
    fprintf(stderr, "\x1b[%dA", opencl->printCount + 1);
    timePoint = temp;
}

void cleanAll() {
    fprintf(stderr, "\n\n\n\n\n\n\nExiting\n");
    destroyFractalWindow();
    opencl->cleanup();
}

static void glfwHandleErrors(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main() {
    config = new Config("config.cfg");
    config->printValues();

    int remainder = config->width * config->height % config->maximum_size;

    if (remainder != 0) {
        fprintf(stderr, "image size (%d) %% sampling size (%d) != 0 (%d)\n", config->width * config->height, config->maximum_size, remainder);
        return 1;
    }

    maximaKernelSize = (config->width * config->height / config->maximum_size);
    timePoint = chrono::high_resolution_clock::now();

    prepare();
    atexit(&cleanAll);

    glfwSetErrorCallback(glfwHandleErrors);

    if (!glfwInit()) return 1;

    createFractalWindow("Fractal Window", config->width, config->height);

    while (!glfwWindowShouldClose(windowFW)) {
        glfwPollEvents();
        display();
    }

    glfwTerminate();

    return 0;
}
