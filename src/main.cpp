#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <math.h>

#include <GLUT/glut.h>

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

chrono::high_resolution_clock::time_point frameTime;
unsigned int frameCount = 0;

typedef struct FractalCoord {
    cl_float2 pos;
} FractalCoord;

/**
 * OpenCL
 */

OpenCl *opencl;
uint64_t *initState, *initSeq;

vector<BufferSpec> bufferSpecs;
void createBufferSpecs() {
    bufferSpecs = {
        {"image",     {NULL, 3 * config->width * config->height * sizeof(uint32_t)}},
        {"count",     {NULL, 3 * config->width * config->height * sizeof(uint32_t)}},
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

vector<KernelSpec> kernelSpecs;
void createKernelSpecs() {
    kernelSpecs = {
        {"seedNoise",     {NULL, 1, {config->particle_count, 0}, {128, 0}, "seedNoise"}},
        {"initParticles", {NULL, 1, {config->particle_count, 0}, {128, 0}, "initParticles"}},
        {"mandelStep",    {NULL, 1, {config->particle_count, 0}, {128, 0}, "mandelStep"}},
        {"resetCount",    {NULL, 1, {config->threshold_count * maximaKernelSize, 0}, {120, 0}, "resetCount"}},
        {"findMax1",      {NULL, 1, {config->threshold_count * maximaKernelSize, 0}, {120, 0}, "findMax1"}},
        {"findMax2",      {NULL, 1, {config->threshold_count, 0}, {config->threshold_count, 0}, "findMax2"}},
        {"renderImage",   {NULL, 2, {config->width, config->height}, {16, 16}, "renderImage"}},
    };
}

void setKernelArgs() {
    opencl->setKernelBufferArg("seedNoise", 0, "randomState");
    opencl->setKernelBufferArg("seedNoise", 1, "randomIncrement");
    opencl->setKernelBufferArg("seedNoise", 2, "initState");
    opencl->setKernelBufferArg("seedNoise", 3, "initSeq");

    opencl->setKernelBufferArg("mandelStep", 0, "particles");
    opencl->setKernelBufferArg("mandelStep", 1, "count");
    opencl->setKernelBufferArg("mandelStep", 2, "threshold");
    opencl->setKernelBufferArg("mandelStep", 3, "path");
    opencl->setKernelBufferArg("mandelStep", 4, "randomState");
    opencl->setKernelBufferArg("mandelStep", 5, "randomIncrement");
    opencl->setKernelArg("mandelStep", 6, sizeof(int), (void*)&(config->threshold_count));
    opencl->setKernelArg("mandelStep", 7, sizeof(ViewSettings), (void*)&viewFW);
    
    opencl->setKernelBufferArg("initParticles", 0, "particles");
    opencl->setKernelBufferArg("initParticles", 1, "threshold");
    opencl->setKernelBufferArg("initParticles", 2, "path");
    opencl->setKernelBufferArg("initParticles", 3, "randomState");
    opencl->setKernelBufferArg("initParticles", 4, "randomIncrement");
    opencl->setKernelArg("initParticles", 5, sizeof(int), (void*)&(config->threshold_count));
    
    opencl->setKernelBufferArg("resetCount", 0, "count");
    opencl->setKernelArg("resetCount", 1, sizeof(unsigned int), (void*)&(config->maximum_size));

    opencl->setKernelBufferArg("findMax1", 0, "count");
    opencl->setKernelBufferArg("findMax1", 1, "maxima");
    opencl->setKernelArg("findMax1", 2, sizeof(unsigned int), (void*)&(config->maximum_size));
    
    opencl->setKernelBufferArg("findMax2", 0, "maxima");
    opencl->setKernelBufferArg("findMax2", 1, "maximum");
    opencl->setKernelArg("findMax2", 2, sizeof(unsigned int), (void*)&maximaKernelSize);
    
    opencl->setKernelBufferArg("renderImage", 0, "count");
    opencl->setKernelBufferArg("renderImage", 1, "maximum");
    opencl->setKernelBufferArg("renderImage", 2, "image");
    opencl->setKernelArg("renderImage", 3, sizeof(int), (void*)&(config->threshold_count));
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
        config->profile
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

    float scaleY = 1.3;
    viewFW = {
        scaleY / (float)config->height * (float)config->width, scaleY,
        -0.5, 0.,
        0., 0., 1.,
        (int)config->width, (int)config->height
    };

    defaultView = viewFW;

    prepareOpenCl();
}

void display() {
    frameCount++;

    if (frameCount % 2 == 0) {
        return;
    }

    opencl->step("mandelStep", config->frame_steps);
    opencl->step("findMax1");
    opencl->step("findMax2");
    opencl->step("renderImage");
    opencl->readBuffer("image", pixelsFW);

    displayFW();

    chrono::high_resolution_clock::time_point temp = chrono::high_resolution_clock::now();
    chrono::duration<float> time_span = chrono::duration_cast<chrono::duration<float>>(temp - frameTime);
    fprintf(stderr, "Step = %d, time = %.4g            \n", frameCount / 2, time_span.count());
    fprintf(stderr, "\x1b[5A");
    frameTime = temp; 
}

void cleanAll() {
    fprintf(stderr, "\n\n\n\n\n\n\nExiting\n");
    destroyFractalWindow();
    opencl->cleanup();
}

int main(int argc, char **argv) {
    config = new Config("config.cfg");
    // config->printValues();

    int remainder = config->width * config->height % config->maximum_size;

    if (remainder != 0) {
        fprintf(stderr, "image size (%d) %% sampling size (%d) != 0 (%d)\n", config->width * config->height, config->maximum_size, remainder);
        return 1;
    }

    maximaKernelSize = (config->width * config->height / config->maximum_size);
    frameTime = chrono::high_resolution_clock::now();

    prepare();

    atexit(&cleanAll);

    // opencl->step("mandelStep", 10 * config->frame_steps);
    // opencl->step("findMax1");
    // opencl->step("findMax2");
    // opencl->step("renderImage");
    // opencl->readBuffer("image", pixelsFW);
    // fprintf(stderr, "\x1b[6A");



    glutInit(&argc, argv);
    createFractalWindow("Fractal Window", config->width, config->height);
    glutDisplayFunc(&display);

    glutIdleFunc(&display);

    fprintf(stderr, "\nStarting main loop\n\n");
    glutMainLoop();

    return 0;
}
