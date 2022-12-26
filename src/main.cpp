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
    float x, y;
    float cx, cy;
    uint32_t iter_count;
} Particle;

typedef struct FractalCoord {
    float x, y;
} FractalCoord;

/**
 * OpenCL
 */

OpenCl *opencl;
uint64_t *init_state, *init_seq;

vector<BufferArgument> bufferArgs;
void createBufferArgs() {
    bufferArgs = {
        {"image",     {NULL, 3 * config->width * config->height * sizeof(uint32_t)}},
        {"count0",    {NULL, config->width * config->height * sizeof(uint32_t)}},
        {"count1",    {NULL, config->width * config->height * sizeof(uint32_t)}},
        {"count2",    {NULL, config->width * config->height * sizeof(uint32_t)}},
        {"particles", {NULL, config->particle_count * sizeof(Particle)}},
        {"path",      {NULL, config->particle_count * config->thresholds[config->threshold_count - 1] * sizeof(FractalCoord)}},

        {"q_list",     {NULL, config->particle_count * sizeof(float)}},
        {"state",      {NULL, config->particle_count * sizeof(uint64_t)}},
        {"inc",        {NULL, config->particle_count * sizeof(uint64_t)}},
        {"init_state", {NULL, config->particle_count * sizeof(uint64_t)}},
        {"init_seq",   {NULL, config->particle_count * sizeof(uint64_t)}},
    };
}

vector<KernelArgument> kernelArgs;
void createKernelArgs() {
    kernelArgs = {
        {"seed_noise", {NULL, 1, {config->particle_count, 0}, "seed_noise"}},
        {"fill_noise", {NULL, 1, {config->particle_count, 0}, "fill_noise"}},
    };
}

void setKernelArgs() {
    opencl->setKernelBufferArg("seed_noise", "state", 0);
    opencl->setKernelBufferArg("seed_noise", "inc", 1);
    opencl->setKernelBufferArg("seed_noise", "init_state", 2);
    opencl->setKernelBufferArg("seed_noise", "init_seq", 3);

    opencl->setKernelBufferArg("fill_noise", "q_list", 0);
    opencl->setKernelBufferArg("fill_noise", "state", 1);
    opencl->setKernelBufferArg("fill_noise", "inc", 2);
}

void initPcg() {
    for (int i = 0; i < config->particle_count; i++) {
        init_state[i] = pcg32_random();
        init_seq[i] = pcg32_random();
    }

    opencl->writeBuffer("init_state", (void *)init_state);
    opencl->writeBuffer("init_seq", (void *)init_seq);
    opencl->step("seed_noise");
    opencl->flush();
}

void prepareOpenCl() {
    fprintf(stderr, "createBufferArgs\n");
    createBufferArgs();
    fprintf(stderr, "createKernelArgs\n");
    createKernelArgs();

    fprintf(stderr, "OpenCl\n");
    opencl = new OpenCl(
        "shaders/buddha.cl",
        bufferArgs,
        kernelArgs
    );

    fprintf(stderr, "setKernelArgs\n");
    setKernelArgs();
    fprintf(stderr, "initPcg\n");
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
