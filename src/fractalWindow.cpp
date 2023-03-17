#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <stack>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "coordinates.hpp"
#include "fractalWindow.hpp"
#include "opencl.hpp"

int windowIdFW;
uint32_t *pixelsFW;
Particle *particles;

WindowSettings settingsFW;
MouseState mouseFW;
ViewSettings viewFW, defaultView;
std::stack<ViewSettings> viewStackFW;
bool selecting = true;

void showParticles() {
    opencl->readBuffer("particles", particles);

    for (int i = 0; i < config->particle_count; i++) {
        Particle particle = particles[i];
        PixelCoordinate coord = ((FractalCoordinate){particle.prevOffset.s[0], particle.prevOffset.s[1]}).toPixel(defaultView);

        if (particle.prevScore == -1) {
            glColor3f(1,0,0);
        } else if (particle.prevScore == 0) {
            glColor3f(0,0,1);
        } else if (particle.prevScore == 1) {
            glColor3f(0,1,0);
        } else {
            glColor3f(1,1,1);
        }

        glVertex2f(
            2 * coord.x / (float)viewFW.sizeX - 1,
            2 * coord.y / (float)viewFW.sizeY - 1
        );

    }
}

void drawGrid() {
    glBegin(GL_LINES);
        glVertex2f(-1,0); glVertex2f(1,0);
        glVertex2f(-1,0.5); glVertex2f(1,0.5);
        glVertex2f(-1,-0.5); glVertex2f(1,-0.5);
        glVertex2f(0,-1); glVertex2f(0,1);
        glVertex2f(0.5,-1); glVertex2f(0.5,1);
        glVertex2f(-0.5,-1); glVertex2f(-0.5,1);
    glEnd();
}

void drawBox() {
    float aspectRatio = (float)settingsFW.windowW / (float)settingsFW.windowH;
    float xc = 2 * mouseFW.xDown / (float)settingsFW.windowW - 1;
    float yc = 1 - 2 * mouseFW.yDown / (float)settingsFW.windowH;

    float dx1 = 2 * (mouseFW.x - mouseFW.xDown) / (float)settingsFW.windowW;
    float dy1 = -2 * (mouseFW.y - mouseFW.yDown) / (float)settingsFW.windowH;

    float dx2 = 2 * (mouseFW.y - mouseFW.yDown) * aspectRatio / settingsFW.windowW;
    float dy2 = 2 * (mouseFW.x - mouseFW.xDown) * aspectRatio / settingsFW.windowH;

    glColor4f(1,1,1,1);

    glBegin(GL_LINE_STRIP);
        glVertex2f(xc + dx1 + dx2, yc + dy1 + dy2);
        glVertex2f(xc + dx1 - dx2, yc + dy1 - dy2);
        glVertex2f(xc - dx1 - dx2, yc - dy1 - dy2);
        glVertex2f(xc - dx1 + dx2, yc - dy1 + dy2);
        glVertex2f(xc + dx1 + dx2, yc + dy1 + dy2);
    glEnd();
}

void drawPath() {
    ScreenCoordinate screen({mouseFW.x, mouseFW.y});
    FractalCoordinate fractal = screen.toPixel(settingsFW).toFractal(defaultView);
    FractalCoordinate dzx({1, 0});
    FractalCoordinate dzy({0, 1});
    FractalCoordinate offset(fractal);
    
    glPointSize(10);
    glEnable(GL_POINT_SMOOTH);
    
    glBegin(GL_POINTS);

    glVertex2f(
        2 * fractal.toPixel(defaultView).x / (float)viewFW.sizeX - 1,
        2 * fractal.toPixel(defaultView).y / (float)viewFW.sizeY - 1
    );

    for (int i = 0; i < 20000; i++) {
        dzx = 2 * complex_mul(fractal, dzx) + FractalCoordinate({1, 0});
        dzy = 2 * complex_mul(fractal, dzy) + FractalCoordinate({0, 1});
        fractal = complex_square(fractal) + offset;

        glColor3f(1, 0, 1);
        glVertex2f(
            2 * fractal.toPixel(defaultView).toScreen(settingsFW).x / (float)viewFW.sizeX - 1,
            2 * fractal.toPixel(defaultView).toScreen(settingsFW).y / (float)viewFW.sizeY - 1
        );

        glColor3f(1, 0, 0);
        glVertex2f(
            2 * (fractal + 0.1 * dzx).toPixel(defaultView).toScreen(settingsFW).x / (float)viewFW.sizeX - 1,
            2 * (fractal + 0.1 * dzx).toPixel(defaultView).toScreen(settingsFW).y / (float)viewFW.sizeY - 1
        );

        glColor3f(0, 1, 1);
        glVertex2f(
            2 * (fractal + 0.1 * dzy).toPixel(defaultView).toScreen(settingsFW).x / (float)viewFW.sizeX - 1,
            2 * (fractal + 0.1 * dzy).toPixel(defaultView).toScreen(settingsFW).y / (float)viewFW.sizeY - 1
        );

        if ((fractal.x * fractal.x + fractal.y * fractal.y) > 16) {
            break;
        }
        
    }

    glEnd();
}

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
        settingsFW.width,
        settingsFW.height,
        0,
        GL_RGB,
        GL_UNSIGNED_INT,
        &(pixelsFW[0])
    );

    glPushMatrix();
    glScalef(settingsFW.zoom, settingsFW.zoom, 1.);
    glTranslatef(-settingsFW.centerX, -settingsFW.centerY, 0.);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0, -1.0);
        glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0, -1.0);
        glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0,  1.0);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0,  1.0);
    glEnd();

    glDisable (GL_TEXTURE_2D);

    glPointSize(2);
    glColor3f(1, 1, 1);
    glEnable(GL_POINT_SMOOTH);
    
    glBegin(GL_POINTS);

    if (settingsFW.showParticles) {
        showParticles();
    }

    if (mouseFW.state == GLUT_DOWN && !selecting) {
        drawPath();
    }

    glEnd();

    glPopMatrix();

    if (settingsFW.grid) {
        drawGrid();
    }

    if (mouseFW.state == GLUT_DOWN && selecting) {
        drawBox();
    }

    glFlush();
    glutSwapBuffers();
}

void updateView(float scale, float centerX, float centerY, float theta) {
    fprintf(stderr, "\n\n\n\n\n\nSetting region to:\n");
    fprintf(stderr, "scale = %.3f\n", scale);
    fprintf(stderr, "center = (%.3f, %.3f)\n", centerX, centerY);
    fprintf(stderr, "theta = %.3f\n", theta);

    viewStackFW.push(ViewSettings(viewFW));

    viewFW.scaleX = scale / viewFW.scaleY * viewFW.scaleX;
    viewFW.scaleY = scale;
    
    viewFW.centerX = centerX;
    viewFW.centerY = centerY;

    viewFW.theta = theta;
    viewFW.cosTheta = cos(theta);
    viewFW.sinTheta = sin(theta);

    opencl->setKernelArg("mandelStep", 7, sizeof(ViewSettings), (void*)&viewFW);

    opencl->step("resetCount");
    opencl->step("initParticles");
}

void selectRegion() {
    if (mouseFW.state != GLUT_DOWN) {
        return;
    }

    ScreenCoordinate downP({mouseFW.xDown, mouseFW.yDown});
    ScreenCoordinate upP({mouseFW.x, mouseFW.y});

    FractalCoordinate downF = downP.toPixel(settingsFW).toFractal(viewFW);
    FractalCoordinate upF   = upP.toPixel(settingsFW).toFractal(viewFW);

    updateView(
        sqrt(pow(upF.x - downF.x, 2) + pow(upF.y - downF.y, 2)),
        downF.x, downF.y,
        atan2(upF.x - downF.x, upF.y - downF.y)
    );
}

void keyPressedFW(unsigned char key, int x, int y) {
    switch (key) {
        case 'a':
            selectRegion();
            break;
        case 'e':
            glutSetWindow(windowIdFW);
            glutPostRedisplay();
            break;
        
        case 'g':
            settingsFW.grid = ! settingsFW.grid;
            break;
        case 'd':
            settingsFW.showDiff = ! settingsFW.showDiff;
            break;
        case 'b':
            selecting = ! selecting;
            break;
        case 'r':
            settingsFW.zoom = 1.;
            settingsFW.centerX = 0.;
            settingsFW.centerY = 0.;
            break;
        case 'z':
            if (!viewStackFW.empty()) {
                viewFW = viewStackFW.top();
                viewStackFW.pop();
                opencl->setKernelArg("mandelStep", 7, sizeof(ViewSettings), (void*)&viewFW);
                opencl->step("resetCount");
                opencl->step("initParticles");
            }
            break;

        case 'w':
            settingsFW.zoom *= 1.5;
            break;
        case 's':
            settingsFW.zoom /= 1.5;
            break;
        case 'p':
            settingsFW.showParticles = !settingsFW.showParticles;
            break;

        case 'q':
            exit(0);
            break;
        case 'R':
            opencl->step("resetCount");
        case 'i':
            opencl->step("initParticles");
            break;
        default:
            break;
    }
}

void specialKeyPressedFW(int key, int x, int y) {
    switch (key) {
        case GLUT_KEY_RIGHT:
            updateView(viewFW.scaleY, viewFW.centerX + 0.1 * viewFW.scaleY, viewFW.centerY, viewFW.theta);
            break;
        case GLUT_KEY_LEFT:
            updateView(viewFW.scaleY, viewFW.centerX - 0.1 * viewFW.scaleY, viewFW.centerY, viewFW.theta);
            break;
        case GLUT_KEY_UP:
            updateView(viewFW.scaleY, viewFW.centerX, viewFW.centerY + 0.1 * viewFW.scaleY, viewFW.theta);
            break;
        case GLUT_KEY_DOWN:
            updateView(viewFW.scaleY, viewFW.centerX, viewFW.centerY - 0.1 * viewFW.scaleY, viewFW.theta);
            break;
        default:
            break;
    }
}

void translateCamera(ScreenCoordinate coords) {
    settingsFW.centerX += 2. / settingsFW.zoom * (coords.x / (float)settingsFW.windowW - 0.5);
    settingsFW.centerY += 2. / settingsFW.zoom * (0.5 - coords.y / (float)settingsFW.windowH);
}

void mousePressedFW(int button, int state, int x, int y) {
    if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            translateCamera((ScreenCoordinate){x, y});
        }

        return;
    }

    mouseFW.state = state;

    if (state == GLUT_DOWN) {
        mouseFW.xDown = x;
        mouseFW.yDown = y;
    }
}

void mouseMovedFW(int x, int y) {
    mouseFW.x = x;
    mouseFW.y = y;
}

void onReshapeFW(int w, int h) {
    settingsFW.windowW = w;
    settingsFW.windowH = h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
}

void createFractalWindow(char *name, uint32_t width, uint32_t height) {
    settingsFW.width = width;
    settingsFW.height = height;

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(width, height);
    windowIdFW = glutCreateWindow(name);

    pixelsFW = (uint32_t *)malloc(3 * width * height * sizeof(uint32_t));
    particles = (Particle *)malloc(config->particle_count * sizeof(Particle));

    for (int i = 0; i < 3 * width * height; i++) {
        pixelsFW[i] = 0;
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
