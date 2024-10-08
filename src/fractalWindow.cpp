#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <chrono>
#include <stack>

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLFW/glfw3.h>

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl2.h"

#include "../implot/implot.h"

#include "coordinates.hpp"
#include "fractalWindow.hpp"
#include "lodepng.hpp"
#include "opencl.hpp"
#include "plots.hpp"

using namespace std;

GLFWwindow *windowFW;
uint32_t *pixelsFW;
Particle *particles;

WindowSettings settingsFW;
MouseState mouseFW;
ViewSettings viewFW, defaultView;
stack<ViewSettings> viewStackFW;
bool selecting = true;
bool readParticles = false;

const size_t particleHistBins = 50;

void showParticles() {
    if (!readParticles) {
        opencl->readBuffer("particles", particles);
        readParticles = true;
    }

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

    ImGui::SeparatorText("Threshold Counts");

    for (int i = 0; i < config->threshold_count; i++) {
        ImGui::Text("Threshold %d: %d", config->thresholds[i], maximumCounts[i]);
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
        stepCount = 0;
    }
}

void plotParticleIterCounts() {
    if (!readParticles) {
        opencl->readBuffer("particles", particles);
        readParticles = true;
    }

    double bins[particleHistBins + 1];
    double counts[particleHistBins];

    double delta = log(config->thresholds[config->threshold_count - 1] + 1) / (particleHistBins - 1);
    for (size_t i = 0; i < particleHistBins; i++) {
        bins[i+1] = exp(i * delta);
        counts[i] = 0;
    }

    for (size_t i = 0; i < config->particle_count; i++) {
        double c = particles[i].bestIter;
        if (c == 0) {
            counts[0]++;
            continue;
        }

        size_t j = fmin(log(c) / delta, particleHistBins - 2);
        counts[j + 1]++;
    }

    if (ImPlot::BeginPlot("Particle Itercounts")) {
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        ImPlot::PlotHistogram("", bins, counts, particleHistBins);
        ImPlot::EndPlot();
    }
}

void plotParticleScores() {
    if (!readParticles) {
        opencl->readBuffer("particles", particles);
        readParticles = true;
    }

    double maxScore = 0;
    for (size_t i = 0; i < config->particle_count; i++) {
        if (particles[i].prevScore > maxScore) {
            maxScore = particles[i].prevScore;
        }
    }

    double bins[particleHistBins + 1];
    double counts[particleHistBins];

    double delta = log(maxScore) / (particleHistBins - 1);
    for (size_t i = 0; i < particleHistBins; i++) {
        bins[i+1] = exp(i * delta);
        counts[i] = 0.001;
    }

    for (size_t i = 0; i < config->particle_count; i++) {
        double c = particles[i].prevScore;
        if (c == 0) {
            counts[0]++;
            continue;
        }

        size_t j = fmin(log(c) / delta, particleHistBins - 2);
        counts[j + 1]++;
    }

    if (ImPlot::BeginPlot("Particle Scores")) {
        ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
        ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_Log10);
        ImPlot::PlotHistogram("", bins, counts, particleHistBins);
        ImPlot::EndPlot();
    }
}

void displayFW() {
    // --------------------------- RESET ---------------------------
    glfwMakeContextCurrent(windowFW);

    ImGuiIO& io = ImGui::GetIO();

    glClearColor( 0, 0, 0, 1 );
    glColor3f(1, 1, 1);
    glClear( GL_COLOR_BUFFER_BIT );

    readParticles = false;

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

    if (mouseFW.state == GLFW_PRESS && !selecting) {
        drawPath();
    }

    glEnd();

    glPopMatrix();

    if (settingsFW.grid) {
        drawGrid();
    }

    if (mouseFW.state == GLFW_PRESS && selecting) {
        drawBox();
    }

    // --------------------------- IMGUI ---------------------------

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(320, 0));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 320, 0));

    ImGui::Begin("Info");
    ImGui::PushItemWidth(140);

    showInfo();
    showControls();

    if (ImGui::TreeNode("Plots")) {
        plotParticleIterCounts();
        plotParticleScores();
        ImGui::TreePop();
    }

    ImGui::End();

    // --------------------------- DRAW ---------------------------
    
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glFlush();
    glfwSwapBuffers(windowFW);


}

void updateView(float scale, float centerX, float centerY, float theta) {
    fprintf(stderr, "\n\n\n\n\n\nSetting region to:\n");
    fprintf(stderr, "scale = %.5f\n", scale);
    fprintf(stderr, "center_x = %.5f\ncenter_y = %.5f\n", centerX, centerY);
    fprintf(stderr, "theta = %.4f\n", theta);

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

    prevMax = 0;
    stepCount = 0;
}

void selectRegion() {
    if (mouseFW.state != GLFW_PRESS) {
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

    const auto p1 = std::chrono::system_clock::now();
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();

    sprintf(filename, "images/%lld_%s_%d_%d_%.6f_%.6f_%.6f_%.6f.png", 
        seconds, getMandelName().c_str(), settingsFW.width, settingsFW.height,
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

void keyPressedFW(GLFWwindow* window, unsigned int key) {
    switch (key) {
        case 'a':
            selectRegion();
            iterCount = 0;
            stepCount = 0;
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
                stepCount = 0;
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
            stepCount = 0;
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

void specialKeyPressedFW(GLFWwindow* window, int key, int scancode, int action, int mod) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mod);
    if (io.WantCaptureKeyboard) {
        return;
    }

    switch (key) {
        case GLFW_KEY_RIGHT:
            updateView(viewFW.scaleY, 
                viewFW.centerX + 0.1 * viewFW.scaleY * cos(viewFW.theta), 
                viewFW.centerY - 0.1 * viewFW.scaleY * sin(viewFW.theta), viewFW.theta);
            break;
        case GLFW_KEY_LEFT:
            updateView(viewFW.scaleY, 
                viewFW.centerX - 0.1 * viewFW.scaleY * cos(viewFW.theta), 
                viewFW.centerY + 0.1 * viewFW.scaleY * sin(viewFW.theta), viewFW.theta);
            break;
        case GLFW_KEY_UP:
            updateView(viewFW.scaleY, 
                viewFW.centerX + 0.1 * viewFW.scaleY * sin(viewFW.theta), 
                viewFW.centerY + 0.1 * viewFW.scaleY * cos(viewFW.theta), viewFW.theta);
            break;
        case GLFW_KEY_DOWN:
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

void mousePressedFW(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    if (!io.WantCaptureMouse) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_RIGHT) {
            translateCamera((ScreenCoordinate){x, y});
        }

        mouseFW.state = action;

        if (action == GLFW_PRESS) {
            mouseFW.xDown = x;
            mouseFW.yDown = y;
        }

        return;
    }
}

void mouseMovedFW(GLFWwindow* window, double x, double y) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);

    if (!io.WantCaptureMouse) {
        mouseFW.x = x;
        mouseFW.y = y;
    }
}

void onReshapeFW(GLFWwindow* window, int w, int h) {
    // ImGui_ImplGlfw(w, h);

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

    pixelsFW = (uint32_t *)malloc(3 * width * height * sizeof(uint32_t));
    particles = (Particle *)malloc(config->particle_count * sizeof(Particle));

    for (int i = 0; i < 3 * width * height; i++) {
        pixelsFW[i] = 0;
    }

    windowFW = glfwCreateWindow(width, height, name, NULL, NULL);
    if (windowFW == nullptr) {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(windowFW);
    glfwSwapInterval(1); // Enable vsync

    // Install imgui callbacks
    glfwSetWindowFocusCallback(windowFW, ImGui_ImplGlfw_WindowFocusCallback);
    glfwSetCursorEnterCallback(windowFW, ImGui_ImplGlfw_CursorEnterCallback);
    glfwSetScrollCallback(windowFW, ImGui_ImplGlfw_ScrollCallback);
    glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);

    glfwSetCharCallback(windowFW, keyPressedFW);
    glfwSetKeyCallback(windowFW, specialKeyPressedFW);
    glfwSetCursorPosCallback(windowFW, mouseMovedFW);
    glfwSetMouseButtonCallback(windowFW, mousePressedFW);
    glfwSetWindowSizeCallback(windowFW, onReshapeFW);

    glfwGetWindowSize(windowFW, &(settingsFW.windowW), &(settingsFW.windowH));

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(windowFW, false);
    ImGui_ImplOpenGL2_Init();
}

void destroyFractalWindow() {
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    free(pixelsFW);
    glfwDestroyWindow(windowFW);
}
