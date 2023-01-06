#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "config.hpp"
#include "opencl.hpp"
#include "pcg.hpp"

using namespace std;

/**
 * Declarations
 */

Config *config;

typedef struct Particle {
    cl_float2 pos;
    cl_float2 offset;
    uint32_t iter_count;
} Particle;

typedef struct FractalCoord {
    cl_float2 pos;
} FractalCoord;

/**
 * OpenCL
 */

OpenCl *opencl;
uint64_t *init_state, *init_seq;

vector<BufferSpec> bufferSpecs;
void createBufferSpecs() {
    bufferSpecs = {
        {"image",     {NULL, 3 * config->width * config->height * sizeof(uint32_t)}},
        {"count",     {NULL, 3 * config->width * config->height * sizeof(uint32_t)}},
        {"particles", {NULL, config->particle_count * sizeof(Particle)}},
        {"path",      {NULL, config->particle_count * config->thresholds[config->threshold_count - 1] * sizeof(FractalCoord)}},
        {"threshold", {NULL, config->threshold_count * sizeof(uint32_t)}},

        {"state",      {NULL, config->particle_count * sizeof(uint64_t)}},
        {"inc",        {NULL, config->particle_count * sizeof(uint64_t)}},
        {"init_state", {NULL, config->particle_count * sizeof(uint64_t)}},
        {"init_seq",   {NULL, config->particle_count * sizeof(uint64_t)}},
    };
}

vector<KernelSpec> kernelSpecs;
void createKernelSpecs() {
    kernelSpecs = {
        {"seedNoise", {NULL, 1, {config->particle_count, 0}, "seedNoise"}},
    };
}

void setKernelArgs() {
    opencl->setKernelBufferArg("seedNoise", "state", 0);
    opencl->setKernelBufferArg("seedNoise", "inc", 1);
    opencl->setKernelBufferArg("seedNoise", "init_state", 2);
    opencl->setKernelBufferArg("seedNoise", "init_seq", 3);
}

void initPcg() {
    for (int i = 0; i < config->particle_count; i++) {
        init_state[i] = pcg32_random();
        init_seq[i] = pcg32_random();
    }

    opencl->writeBuffer("init_state", (void *)init_state);
    opencl->writeBuffer("init_seq", (void *)init_seq);
    opencl->step("seedNoise");
    opencl->flush();
}

void prepareOpenCl() {
    createBufferSpecs();
    createKernelSpecs();

    opencl = new OpenCl(
        "shaders/buddha.cl",
        bufferSpecs,
        kernelSpecs
    );

    setKernelArgs();
    initPcg();
}

void prepare() {
    init_state = (uint64_t *)malloc(config->particle_count * sizeof(uint64_t));
    init_seq = (uint64_t *)malloc(config->particle_count * sizeof(uint64_t));

    prepareOpenCl();
}

int main() {
    config = new Config("config.cfg");

    prepare();

    return 0;
}
