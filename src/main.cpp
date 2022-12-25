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

/**
 * OpenCL
 */

OpenCl *opencl;
uint64_t *init_state, *init_seq;

vector<string> bufferNames = {
    "particle_x",
    "particle_y",
    "iter_count",

    "path",

    // Noise
    "q_list",
    "state",
    "inc",
    "init_state",
    "init_seq",
};

vector<size_t> bufferSizes;
void setBufferSizes() {
    bufferSizes = {
        config->particle_count * sizeof(float),
        config->particle_count * sizeof(float),
        config->particle_count * sizeof(uint32_t),

        config->particle_count * config->thresholds[config->threshold_count - 1],

        // Noise
        config->particle_count * sizeof(float),
        config->particle_count * sizeof(uint64_t),
        config->particle_count * sizeof(uint64_t),
        config->particle_count * sizeof(uint64_t),
        config->particle_count * sizeof(uint64_t),
    };
}

vector<string> kernelNames = {
    "seed_noise",
    "fill_noise",
};

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
    setBufferSizes();

    opencl = new OpenCl(
        config->particle_count,
        "shaders/buddha.cl",
        false,
        bufferNames,
        bufferSizes,
        kernelNames
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
