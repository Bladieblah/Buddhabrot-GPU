#include <cstdlib>
#include <cstdio>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "fractalWindow.hpp"
#include "opencl.hpp"

int windowIdFW;
uint32_t *pixelsFW;

typedef struct ViewSettings {
    uint32_t width, height;
    double scale = 1, centerX = 0, centerY = 0;
} ViewSettings;

ViewSettings settings;

void displayFW() {
    glutSetWindow(windowIdFW);

    glClearColor( 0, 0, 0, 1 );
    glColor3f(1, 1, 1);
    glClear( GL_COLOR_BUFFER_BIT );

    glEnable (GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D (
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        settings.width,
        settings.height,
        0,
        GL_RGB,
        GL_UNSIGNED_INT,
        &(pixelsFW[0])
    );

    glPushMatrix();
    glScalef(settings.scale, settings.scale, 1.);
    glTranslatef(-settings.centerX, -settings.centerY, 0.);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0, -1.0);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0, -1.0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0,  1.0);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0,  1.0);
    glEnd();

    glDisable (GL_TEXTURE_2D);

    glPopMatrix();

    glFlush();
    glutSwapBuffers();
}

void keyPressedFW(unsigned char key, int x, int y) {
    switch (key) {
        case 'e':
            glutSetWindow(windowIdFW);
            glutPostRedisplay();
            break;
        case 'q':
            exit(0);
            break;
        case 'r':
            opencl->step("resetCount");
            break;
        default:
            break;
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
    settings.width = width;
    settings.height = height;

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    windowIdFW = glutCreateWindow(name);

    pixelsFW = (uint32_t *)malloc(3 * width * height * sizeof(uint32_t));

    for (int i = 0; i < 3 * width * height; i++) {
        pixelsFW[i] = 4294967296/2;
    }
    
    glutKeyboardFunc(&keyPressedFW);
    glutSpecialFunc(&specialKeyPressedFW);
    glutMouseFunc(&mousePressedFW);
    glutMotionFunc(&mouseMovedFW);
    glutReshapeFunc(&onReshapeFW);
}

void destroyFractalWindow() {
    free(pixelsFW);
}
