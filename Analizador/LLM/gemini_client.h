#pragma once
#include <string>
#include <vector>
#include <cstddef>

using namespace std;

// Cliente ligero para consumir la API de Gemini y obtener sugerencias de optimización.
class GeminiClient {
private:
    string apiKey;   // API key de Gemini
    string apiUrl;   // URL base del endpoint a utilizar

    // Callback estático usado por libcurl para ir acumulando la respuesta HTTP.
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

public:
    // Construye el cliente con la API key ya configurada.
    GeminiClient(const string& key);

    // Envía el código y metadatos de análisis a Gemini y devuelve una lista de sugerencias.
    vector<string> generateSuggestions(
        const string& code,           // Código fuente original a analizar
        const string& complexity,     // Complejidad detectada (Big-O)
        const string& algorithmType,  // Tipo de algoritmo identificado
        const string& consoleOutput   // Salida de consola del programa, si se dispone
        );
};
