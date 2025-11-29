// Analizador/env_loader.cpp
#include "env_loader.h"
#include <fstream>
#include <sstream>
#include <iostream>

EnvLoader::EnvLoader(const string& filename) {
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "[ADVERTENCIA] " << filename << " no encontrado" << endl;
        return;
    }

    string line;
    while (getline(file, line)) {
        // Ignorar líneas vacías y comentarios
        if (line.empty() || line[0] == '#') continue;

        // Buscar el '='
        size_t pos = line.find('=');
        if (pos == string::npos) continue;

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);

        // Trim espacios
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Remover comillas si existen
        if (!value.empty() && (value.front() == '"' || value.front() == '\'')) {
            value = value.substr(1, value.length() - 2);
        }

        envVars[key] = value;
    }

    file.close();

    if (!envVars.empty()) {
        cout << "[INFO] Environment variables loaded from .env" << endl;
    }
}

bool EnvLoader::load() {
    return !envVars.empty();
}

string EnvLoader::get(const string& key, const string& defaultValue) {
    auto it = envVars.find(key);
    if (it != envVars.end()) {
        return it->second;
    }
    return defaultValue;
}