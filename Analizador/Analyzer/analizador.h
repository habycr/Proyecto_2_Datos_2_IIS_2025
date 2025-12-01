#pragma once
#include <string>
#include <vector>

using namespace std;

// Resultado completo de un análisis de complejidad sobre un fragmento de código.
struct AnalysisResult {
    bool success;                     // Indica si el análisis se pudo completar correctamente
    string complexity;                // Complejidad Big-O detectada (por ejemplo "O(n)", "O(n^2)", etc.)
    string algorithmType;             // Tipo de algoritmo identificado (búsqueda lineal, ordenamiento, etc.)
    int nestedLoops;                  // Máxima profundidad de loops anidados encontrada
    bool isRecursive;                 // true si se detecta recursión en la función analizada
    double averageRatio;              // Razón promedio entre tiempos de ejecución consecutivos (para análisis empírico)
    vector<double> executionTimes;    // Tiempos de ejecución medidos para distintos tamaños de entrada
    string explanation;               // Explicación en texto de la complejidad detectada
    vector<string> suggestions;       // Sugerencias de optimización generadas (local o vía Gemini)
    string errorMessage;              // Mensaje de error en caso de que success = false
    string consoleOutput;             // Salida de consola del programa probado (si se captura)
};

// Clase principal que realiza el análisis de complejidad de un código fuente C++.
class ComplexityAnalyzer {
private:
    string code;            // Código fuente completo a analizar
    string functionName;    // Nombre de la función principal encontrada en el código
    string geminiApiKey;    // API key para llamar a Gemini (opcional)

public:
    // Constructor: recibe el código fuente y opcionalmente la API key de Gemini.
    ComplexityAnalyzer(const string& sourceCode, const string& apiKey = "");

    // Punto de entrada principal: ejecuta todos los análisis y devuelve un AnalysisResult completo.
    AnalysisResult analyze();

private:
    // Intenta extraer el nombre de la función más relevante en el código (por patrones de regex).
    string extractFunctionName();

    // Cuenta el nivel máximo de loops anidados (for/while) presentes en el código.
    int countNestedLoops();

    // Detecta si la función principal se llama a sí misma (recursión).
    bool detectRecursion();

    // Intenta detectar patrones simples que sugieran comportamiento logarítmico (por ejemplo i *= 2).
    bool detectLogarithmic();

    // Clasifica el tipo de algoritmo (búsqueda lineal, ordenamiento, etc.) según palabras clave.
    string identifyAlgorithmType();

    // Determina la complejidad Big-O en función de los tiempos de ejecución medidos
    // o, si no hay tiempos, mediante análisis estático.
    string determineComplexity(const vector<double>& times);

    // Construye una explicación en texto a partir de la complejidad y tipo de algoritmo detectados.
    string generateExplanation(const string& complexity, const string& type);

    // Genera una lista de sugerencias estáticas basadas únicamente en la complejidad detectada.
    vector<string> generateSuggestions(const string& complexity);

    // Versión que genera sugerencias usando Gemini (si hay API key); si falla, usa las sugerencias estáticas.
    vector<string> generateSuggestionsWithGemini(const string& complexity, const string& consoleOutput);

};
