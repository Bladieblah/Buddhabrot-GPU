#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include "opencl.hpp"

void displayFW();

void createFractalWindow(char *name, uint32_t width, uint32_t height);
void destroyFractalWindow();

extern uint32_t *pixelsFW;
extern OpenCl *opencl;

#endif
