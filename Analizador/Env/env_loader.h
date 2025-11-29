
#pragma once
#include <string>
#include <map>

using namespace std;

class EnvLoader {
private:
    map<string, string> envVars;

public:
    EnvLoader(const string& filename = ".env");
    bool load();
    string get(const string& key, const string& defaultValue = "");
};