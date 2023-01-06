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
uint32_t maximaKernelSize;

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

        {"maxima", {NULL, maximaKernelSize * sizeof(uint32_t)}},
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
        {"seedNoise", {NULL, 1, {config->particle_count, 0}, "seedNoise"}},
        {"mandelStep", {NULL, 1, {config->particle_count, 0}, "mandelStep"}},
        {"findMax1", {NULL, 1, {maximaKernelSize, 0}, "findMax1"}},
        {"findMax2", {NULL, 1, {config->threshold_count, 0}, "findMax2"}},
    };
}

void setKernelArgs() {
    opencl->setKernelBufferArg("seedNoise", 0, "randomState");
    opencl->setKernelBufferArg("seedNoise", 1, "randomIncrement");
    opencl->setKernelBufferArg("seedNoise", 2, "initState");
    opencl->setKernelBufferArg("seedNoise", 3, "initSeq");

    cl_uint2 resolution = {config->width, config->height};
    opencl->setKernelBufferArg("mandelStep", 0, "particles");
    opencl->setKernelBufferArg("mandelStep", 1, "count");
    opencl->setKernelBufferArg("mandelStep", 2, "threshold");
    opencl->setKernelBufferArg("mandelStep", 3, "path");
    opencl->setKernelBufferArg("mandelStep", 4, "randomState");
    opencl->setKernelBufferArg("mandelStep", 5, "randomIncrement");
    opencl->setKernelArg("mandelStep", 6, sizeof(int), (void*)&(config->threshold_count));
    opencl->setKernelArg("mandelStep", 7, sizeof(cl_int2), (void*)&resolution);
    
    opencl->setKernelBufferArg("findMax1", 0, "count");
    opencl->setKernelBufferArg("findMax1", 1, "maxima");
    opencl->setKernelArg("findMax1", 2, sizeof(cl_uint2), (void*)&resolution);
    
    opencl->setKernelBufferArg("findMax2", 0, "maxima");
    opencl->setKernelBufferArg("findMax2", 1, "maximum");
    opencl->setKernelArg("findMax2", 2, sizeof(cl_uint2), (void*)&resolution);
}

void initPcg() {
    for (int i = 0; i < config->particle_count; i++) {
        init_state[i] = pcg32_random();
        init_seq[i] = pcg32_random();
    }

    opencl->writeBuffer("initState", (void *)init_state);
    opencl->writeBuffer("initSeq", (void *)init_seq);
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
    int remainder = config->width * config->height % config->maximum_size;

    if (remainder != 0) {
        fprintf(stderr, "image size (%d) %% sampling size (%d) != 0 (%d)\n", config->width * config->height, config->maximum_size, remainder);
        return 1;
    }

    maximaKernelSize = config->threshold_count * (config->width * config->height / config->maximum_size);

    prepare();

    return 0;
}
