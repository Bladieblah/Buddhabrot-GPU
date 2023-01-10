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
        
        {"maximum_size", {'i', (void *)&maximum_size}},
        {"frame_steps", {'i', (void *)&frame_steps}},
    };
};

#endif
