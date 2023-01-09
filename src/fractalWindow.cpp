#include <cstdlib>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "fractalWindow.hpp"

int windowIdFW;
uint32_t widthFW, heightFW;
uint32_t *pixelsFW;

void displayFW() {

}

void keyPressedFW(unsigned char key, int x, int y) {
    switch (key) {
        case 'q':
            destroyFractalWindow();
            exit(0);
    }
}

void specialKeyPressedFW(int key, int x, int y) {

}

void mousePressedFW(int button, int state, int x, int y) {

}

void mouseMovedFW(int x, int y) {

}

void onReshapeFW(int w, int h) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
}

void createFractalWindow(char *name, uint32_t width, uint32_t height) {
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    windowIdFW = glutCreateWindow(name);

    pixelsFW = (uint32_t *)malloc(3 * width * height * sizeof(uint32_t));
    
    glutDisplayFunc(&displayFW);
    glutKeyboardFunc(&keyPressedFW);
    glutSpecialFunc(&specialKeyPressedFW);
    glutMouseFunc(&mousePressedFW);
    glutMotionFunc(&mouseMovedFW);
    glutReshapeFunc(&onReshapeFW);
}

void destroyFractalWindow() {
    free(pixelsFW);
}
