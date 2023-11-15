#ifndef CONFIG_H
#define CONFIG_H

#include <map>
#include <string>

typedef struct Setting {
    char type;
    void *pointer;
} Setting;

class Config {
public:
    unsigned int particle_count = 20000;

    unsigned int threshold_count = 3;
    unsigned int thresholds[3] = {250, 500, 1000};//, 2000, 4000};

    unsigned int width = 1080;
    unsigned int height = 720;
    unsigned int maximum_size = 32;
    unsigned int frame_steps = 100;

    float scale = 1.3;
    float center_x = -0.5;
    float center_y = 0.;
    float theta = 0.;

    bool profile = true;
    bool verbose = true;

    float alpha = 0.8;

    Config(char *filename);
    void printValues();

private:
    void processLine(std::string line);
    void setValue(std::string name, char *value);
    void printSetting(std::string name, Setting setting);

    const std::map<std::string, Setting> typeMap = {
        {"particle_count", {'i', (void *)&particle_count}},

        {"threshold_count", {'i', (void *)&threshold_count}},
        {"threshold0", {'i', (void *)thresholds}},
        {"threshold1", {'i', (void *)&(thresholds[1])}},
        {"threshold2", {'i', (void *)&(thresholds[2])}},
        // {"threshold3", {'i', (void *)&(thresholds[3])}},
        // {"threshold4", {'i', (void *)&(thresholds[4])}},

        {"width", {'i', (void *)&width}},
        {"height", {'i', (void *)&height}},

        {"scale", {'f', (void *)&scale}},
        {"center_x", {'f', (void *)&center_x}},
        {"center_y", {'f', (void *)&center_y}},
        {"theta", {'f', (void *)&theta}},
        
        {"maximum_size", {'i', (void *)&maximum_size}},
        {"frame_steps", {'i', (void *)&frame_steps}},
        {"profile", {'b', (void *)&profile}},
        {"verbose", {'b', (void *)&verbose}},
        
        {"alpha", {'f', (void *)&alpha}},
    };
};

#endif
