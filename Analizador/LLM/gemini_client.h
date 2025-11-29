#pragma once
#include <string>
#include <vector>
#include <cstddef>

using namespace std;

class GeminiClient {
private:
    string apiKey;
    string apiUrl;

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);
public:
    GeminiClient(const string& key);

    vector<string> generateSuggestions(
        const string& code,
        const string& complexity,
        const string& algorithmType,
        const string& consoleOutput
        );
};