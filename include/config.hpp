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

    Config(char *filename);
    void printValues();

private:
    void processLine(std::string line);
    void setValue(std::string name, char *value);
    void printSetting(std::string name, Setting setting);

    const std::map<std::string, Setting> typeMap = {
        {"particle_count", {'i', (void *)&particle_count}},
    };
};

#endif
