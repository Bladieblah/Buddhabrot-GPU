#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <stack>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glut.h"
#include "../imgui/backends/imgui_impl_opengl2.h"

#include "../implot/implot.h"

#include "coordinates.hpp"
#include "fractalWindow.hpp"
#include "lodepng.hpp"
#include "opencl.hpp"

using namespace std;

int windowIdFW;
uint32_t *pixelsFW;
Particle *particles;

WindowSettings settingsFW;
MouseState mouseFW;
ViewSettings viewFW, defaultView;
stack<ViewSettings> viewStackFW;
bool selecting = true;

void showParticles() {
    opencl->readBuffer("particles", particles);

    for (int i = 0; i < config->particle_count; i++) {
        Particle particle = particles[i];
        PixelfCoordinate coord = ((FractalCoordinate){particle.prevOffset.s[0], particle.prevOffset.s[1]}).toPixelf(defaultView);

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

void showInfo() {
    ImGui::SeparatorText("Info");
    ImGui::Text("x = %.16f", viewFW.centerX);
    ImGui::Text("y = %.16f", viewFW.centerY);
    ImGui::Text("scale = %.3g", viewFW.scaleY); ImGui::SameLine();
    ImGui::Text("theta = %.3f", viewFW.theta);
    ImGui::Text("Frametime = %.3f", frameTime);
    ImGui::Text("Frames = %d", iterCount);
    ImGui::Text("Steps = %llu M", stepCount / 1000000LLU);

    ScreenCoordinate screen({mouseFW.x, mouseFW.y});
    FractalCoordinate fractal = screen.toPixel(settingsFW).toFractal(viewFW);

    ImGui::SeparatorText("Cursor");
    ImGui::Text("x = %.16f", fractal.x);
    ImGui::Text("y = %.16f", fractal.y);

    ImGui::Checkbox("Draw Box", &selecting);

    int counts[config->threshold_count];
    opencl->readBuffer("maximum", counts);
    ImGui::SeparatorText("Threshold Counts");

    for (int i = 0; i < config->threshold_count; i++) {
        ImGui::Text("Threshold %d: %d", config->thresholds[i], counts[i]);
    }
}

void showControls() {
    ImGui::SeparatorText("Path Type");
    bool chg = false;
    chg |= ImGui::RadioButton("Constant", &(settingsFW.pathType), PathOptions::PATH_CONSTANT);
    chg |= ImGui::RadioButton("Sqrt##path",     &(settingsFW.pathType), PathOptions::PATH_SQRT);
    chg |= ImGui::RadioButton("Linear",   &(settingsFW.pathType), PathOptions::PATH_LINEAR);
    chg |= ImGui::RadioButton("Squared##path",  &(settingsFW.pathType), PathOptions::PATH_SQUARE);

    ImGui::SeparatorText("Score Type");
    chg |= ImGui::RadioButton("None",   &(settingsFW.scoreType), ScoreOptions::SCORE_NONE);
    chg |= ImGui::RadioButton("Sqrt##score",   &(settingsFW.scoreType), ScoreOptions::SCORE_SQRT);
    chg |= ImGui::RadioButton("Squared##score", &(settingsFW.scoreType), ScoreOptions::SCORE_SQUARE);
    chg |= ImGui::RadioButton("Normed", &(settingsFW.scoreType), ScoreOptions::SCORE_NORM);
    chg |= ImGui::RadioButton("Normed Square", &(settingsFW.scoreType), ScoreOptions::SCORE_SQNORM);

    if (chg) {
        opencl->step("resetCount");
        opencl->step("initParticles");
        iterCount = 0;
    }
}

void plotParticleScores() {
    unsigned int xs[5] = {0, 1, 2, 3, 4};
    unsigned int ys[5] = {0, 1, 2, 3, 4};

    if (ImPlot::BeginPlot("Bar Plot")) {
        ImPlot::PlotBars("Particle scores", xs, ys, 5, 1.);
        // ImPlot::PlotBars("Horizontal",data,10,0.4,1,ImPlotBarsFlags_Horizontal);
        ImPlot::EndPlot();
    }
}

void displayFW() {
    // --------------------------- RESET ---------------------------
    glutSetWindow(windowIdFW);

    ImGuiIO& io = ImGui::GetIO();

    glClearColor( 0, 0, 0, 1 );
    glColor3f(1, 1, 1);
    glClear( GL_COLOR_BUFFER_BIT );

    // --------------------------- FRACTAL ---------------------------

    if (settingsFW.updateView) {
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
    }

    // --------------------------- Overlays ---------------------------

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

    // --------------------------- IMGUI ---------------------------

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(320, 0));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 320, 0));

    ImGui::Begin("Info");
    ImGui::PushItemWidth(140);

    showInfo();
    showControls();
    plotParticleScores();

    ImGui::End();

    // --------------------------- DRAW ---------------------------
    
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glFlush();
    glutSwapBuffers();


}

void updateView(float scale, float centerX, float centerY, float theta) {
    fprintf(stderr, "\n\n\n\n\n\nSetting region to:\n");
    fprintf(stderr, "scale = %.3f\n", scale);
    fprintf(stderr, "center_x = %.3f\ncenter_y = %.3f\n", centerX, centerY);
    fprintf(stderr, "theta = %.3f\n", theta);

    viewStackFW.push(ViewSettings(viewFW));

    viewFW.scaleX = scale / viewFW.scaleY * viewFW.scaleX;
    viewFW.scaleY = scale;
    
    viewFW.centerX = centerX;
    viewFW.centerY = centerY;

    viewFW.theta = theta;
    viewFW.cosTheta = cos(theta);
    viewFW.sinTheta = sin(theta);

    for (string name : getMandelNames()) {
        opencl->setKernelArg(name, 7, sizeof(ViewSettings), (void*)&viewFW);
    }

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

void writePng() {
    char filename[200];

    uint32_t h = settingsFW.height; 
    uint32_t w = settingsFW.width; 

    sprintf(filename, "images/%s_%d_%d_%.6f_%.6f_%.6f_%.6f.png", 
        getMandelName().c_str(), settingsFW.width, settingsFW.height,
        viewFW.theta, viewFW.centerX, viewFW.centerY, viewFW.scaleY);
    
    unsigned char *image8Bit = (unsigned char *)malloc(3 * w * h * sizeof(unsigned char));

    for (uint32_t i = 0; i < h; i++) {
        for (uint32_t j = 0; j < 3 * w; j++) {
            image8Bit[3 * w * (h - i - 1) + j] = pixelsFW[3 * w * i + j] >> ((sizeof(unsigned int) - sizeof(unsigned char)) * 8);
        }
    }
    
    unsigned error = lodepng_encode24_file(filename, image8Bit, settingsFW.width, settingsFW.height);

    if (error){
        fprintf(stderr, "Encoder error %d: %s\n", error, lodepng_error_text(error));
    }
}

void keyPressedFW(unsigned char key, int x, int y) {
    switch (key) {
        case 'a':
            selectRegion();
            iterCount = 0;
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
        case 'u':
            settingsFW.updateView = ! settingsFW.updateView;
            break;
        case 'b':
            selecting = ! selecting;
            break;
        case 'c':
            settingsFW.crossPollinate = ! settingsFW.crossPollinate;
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

                for (string name : getMandelNames()) {
                    opencl->setKernelArg(name, 7, sizeof(ViewSettings), (void*)&viewFW);
                }

                opencl->step("resetCount");
                opencl->step("initParticles");
                iterCount = 0;
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
            iterCount = 0;
        case 'i':
            opencl->step("initParticles");
            break;
        case 'W':
            writePng();
            break;
        
        case '-':
            updateView(viewFW.scaleY * 1.1, viewFW.centerX, viewFW.centerY, viewFW.theta);
            break;
        case '=':
            updateView(viewFW.scaleY / 1.1, viewFW.centerX, viewFW.centerY, viewFW.theta);
            break;
        case '_':
            updateView(viewFW.scaleY * 1.01, viewFW.centerX, viewFW.centerY, viewFW.theta);
            break;
        case '+':
            updateView(viewFW.scaleY / 1.01, viewFW.centerX, viewFW.centerY, viewFW.theta);
            break;
        
        case '[':
            updateView(viewFW.scaleY, viewFW.centerX, viewFW.centerY, viewFW.theta + 0.1);
            break;
        case ']':
            updateView(viewFW.scaleY, viewFW.centerX, viewFW.centerY, viewFW.theta - 0.1);
            break;
        case '{':
            updateView(viewFW.scaleY, viewFW.centerX, viewFW.centerY, viewFW.theta + 0.003);
            break;
        case '}':
            updateView(viewFW.scaleY, viewFW.centerX, viewFW.centerY, viewFW.theta - 0.003);
            break;
        default:
            break;
    }
}

void specialKeyPressedFW(int key, int x, int y) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGLUT_SpecialFunc(key, x, y);
    if (io.WantCaptureKeyboard) {
        return;
    }

    switch (key) {
        case GLUT_KEY_RIGHT:
            updateView(viewFW.scaleY, 
                viewFW.centerX + 0.1 * viewFW.scaleY * cos(viewFW.theta), 
                viewFW.centerY - 0.1 * viewFW.scaleY * sin(viewFW.theta), viewFW.theta);
            break;
        case GLUT_KEY_LEFT:
            updateView(viewFW.scaleY, 
                viewFW.centerX - 0.1 * viewFW.scaleY * cos(viewFW.theta), 
                viewFW.centerY + 0.1 * viewFW.scaleY * sin(viewFW.theta), viewFW.theta);
            break;
        case GLUT_KEY_UP:
            updateView(viewFW.scaleY, 
                viewFW.centerX + 0.1 * viewFW.scaleY * sin(viewFW.theta), 
                viewFW.centerY + 0.1 * viewFW.scaleY * cos(viewFW.theta), viewFW.theta);
            break;
        case GLUT_KEY_DOWN:
            updateView(viewFW.scaleY, 
                viewFW.centerX - 0.1 * viewFW.scaleY * sin(viewFW.theta), 
                viewFW.centerY - 0.1 * viewFW.scaleY * cos(viewFW.theta), viewFW.theta);
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
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGLUT_MouseFunc(button, state, x, y);

    if (!io.WantCaptureMouse) {
        if (state == GLUT_DOWN && button == GLUT_RIGHT_BUTTON) {
            translateCamera((ScreenCoordinate){x, y});
        }

        mouseFW.state = state;

        if (state == GLUT_DOWN) {
            mouseFW.xDown = x;
            mouseFW.yDown = y;
        }

        return;
    }
}

void mouseMovedFW(int x, int y) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGLUT_MotionFunc(x, y);

    if (!io.WantCaptureMouse) {
        mouseFW.x = x;
        mouseFW.y = y;
    }
}

void onReshapeFW(int w, int h) {
    ImGui_ImplGLUT_ReshapeFunc(w, h);

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

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;

    ImGui::StyleColorsDark();
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();

    glutKeyboardUpFunc(ImGui_ImplGLUT_KeyboardUpFunc);
    glutSpecialUpFunc(ImGui_ImplGLUT_SpecialUpFunc);
    
    glutKeyboardFunc(&keyPressedFW);
    glutSpecialFunc(&specialKeyPressedFW);
    glutMouseFunc(&mousePressedFW);
    glutMotionFunc(&mouseMovedFW);
    glutPassiveMotionFunc(&mouseMovedFW);
    glutReshapeFunc(&onReshapeFW);
}

void destroyFractalWindow() {
    free(pixelsFW);
}
