#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "config.hpp"

using namespace std;

Config::Config(char *filename) {
    ifstream configFile(filename);

    if (! configFile.is_open()) {
        return;
    }

    string line;
    while (getline(configFile, line)) {
        processLine(line);
    }
}

void Config::processLine(string line) {
    if (line.length() == 0) {
        return;
    }

    if (strchr(line.c_str(), '#') != nullptr) {
        istringstream input(line);
        getline(input, line, '#'); // Remove comment
    }

    if (line.length() == 0) {
        return;
    }

    char *variableName = (char *)malloc(1000 * sizeof(char));
    char *value = (char *)malloc(1000 * sizeof(char));

    if (sscanf(line.c_str(), "%s = %s", variableName, value)) {
        string variableString(variableName);
        if (typeMap.find(variableString) != typeMap.end()) {
            setValue(variableString, value);
        }
    }
}

void Config::setValue(string name, char *value) {
    Setting setting = typeMap.at(name);

    switch (setting.type) {
        case 'i':
            *(int *)setting.pointer = atoi(value);
            break;
        case 'f':
            *(float *)setting.pointer = atof(value);
            break;
        case 'b':
            if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                *(bool *)setting.pointer = true;
            } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                *(bool *)setting.pointer = false;
            } else {
                fprintf(stderr, "Invalid value for boolean setting %s: %s, using default.\n", name.c_str(), value);
            }
            break;

        default:
            break;
    }
}

void Config::printValues() {
    map<string, Setting>::const_iterator typeIt;

    fprintf(stderr, "Loaded settings:\n");

    for (typeIt = typeMap.begin(); typeIt != typeMap.end(); typeIt++) {
        printSetting(typeIt->first, typeIt->second);
    }

    fprintf(stderr, "\n");
}

void Config::printSetting(string name, Setting setting) {
    switch (setting.type) {
    case 'i':
        fprintf(stderr, "%s = %d\n", name.c_str(), *(int *)setting.pointer);
        break;
    case 'f':
        fprintf(stderr, "%s = %.2g\n", name.c_str(), *(float *)setting.pointer);
        break;
    case 'b':
        fprintf(stderr, "%s = %s\n", name.c_str(), *(bool *)setting.pointer ? "true" : "false");
        break;
    
    default:
        break;
    }
}
