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
    unsigned int thresholds[5] = {250, 500, 1000, 2000, 4000};

    Config(char *filename);
    void printValues();

private:
    void processLine(std::string line);
    void setValue(std::string name, char *value);
    void printSetting(std::string name, Setting setting);

    const std::map<std::string, Setting> typeMap = {
        {"particle_count", {'i', (void *)&particle_count}},
        {"threshold_count", {'i', (void *)&threshold_count}},
        {"threshold0", {'i', (void *)&(thresholds[0])}},
        {"threshold1", {'i', (void *)&(thresholds[1])}},
        {"threshold2", {'i', (void *)&(thresholds[2])}},
        {"threshold3", {'i', (void *)&(thresholds[3])}},
        {"threshold4", {'i', (void *)&(thresholds[4])}},
    };
};

#endif
