#include "analizador.h"
#include <fstream>
#include <regex>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <map>
#include "../LLM/gemini_client.h"

#ifdef _WIN32
#include <Windows.h>
#endif

// Constructor: guarda el código, la API key de Gemini y trata de identificar el nombre de la función
// principal a analizar.
ComplexityAnalyzer::ComplexityAnalyzer(const string& sourceCode, const string& apiKey)
    : code(sourceCode), geminiApiKey(apiKey) {
    functionName = extractFunctionName ();
}

// Intenta extraer el nombre de la función principal usando varios patrones heurísticos.
string ComplexityAnalyzer::extractFunctionName() {
    // Patrón 1: Función que recibe un int n, típico de análisis de complejidad.
    regex patron1(R"(\w+\s+(\w+)\s*\(\s*int\s+n\s*\))");
    smatch coincidencias;

    if (regex_search(code, coincidencias, patron1)) {
        cout << "[ANALYZER] Found function: " << coincidencias[1].str() << "(int n)" << endl;
        return coincidencias[1].str();
    }

    // Patrón 2: Función main()
    regex patron2(R"(int\s+main\s*\()");
    if (regex_search(code, coincidencias, patron2)) {
        cout << "[ANALYZER] Found main() function" << endl;
        return "main";
    }

    // Patrón 3: Cualquier otra función con parámetros arbitrarios.
    regex patron3(R"(\w+\s+(\w+)\s*\([^)]*\))");
    if (regex_search(code, coincidencias, patron3)) {
        cout << "[ANALYZER] Found function: " << coincidencias[1].str() << endl;
        return coincidencias[1].str();
    }

    // Si no encuentra función, se asume que es un fragmento de código genérico.
    cout << "[ANALYZER] No function found, analyzing code structure" << endl;
    return "code_snippet";
}

// Cuenta la profundidad máxima de loops anidados (for/while) de forma aproximada.
int ComplexityAnalyzer::countNestedLoops() {
    int maxNivel = 0;
    int nivelActual = 0;
    bool dentroDeLoop = false;

    // Recorre el código buscando palabras clave de loops y llaves que abren/cerran bloques.
    for (size_t i = 0; i < code.length(); i++) {
        // Detectar inicio de loop
        if (i + 3 < code.length()) {
            string sub = code.substr(i, 3);
            if (sub == "for" || sub == "whi") {  // for o while (se comprueba solo las primeras letras)
                dentroDeLoop = true;
            }
        }

        if (code[i] == '{' && dentroDeLoop) {
            // Al abrir una llave dentro de un loop, aumenta el nivel de anidamiento.
            nivelActual++;
            if (nivelActual > maxNivel) {
                maxNivel = nivelActual;
            }
        } else if (code[i] == '}') {
            // Al cerrar una llave, se reduce el nivel actual (si es mayor a 0).
            if (nivelActual > 0) {
                nivelActual--;
            }
        }
    }

    // Si no se detectaron loops anidados, se revisa si al menos hay loops simples.
    if (maxNivel == 0) {
        size_t pos = 0;
        while ((pos = code.find("for", pos)) != string::npos) {
            maxNivel = 1;
            pos += 3;
        }
        pos = 0;
        while ((pos = code.find("while", pos)) != string::npos) {
            maxNivel = max(maxNivel, 1);
            pos += 5;
        }
    }

    return maxNivel;
}

// Busca patrones sencillos que sugieren complejidad logarítmica, como i *= 2.
bool ComplexityAnalyzer::detectLogarithmic() {
    regex logRegex(R"(\w+\s*[*/]=\s*2)");
    return regex_search(code, logRegex);
}

// Intenta identificar el tipo de algoritmo por palabras clave típicas en el código.
string ComplexityAnalyzer::identifyAlgorithmType() {
    string codeLower = code;
    // Normaliza el código a minúsculas para facilitar las búsquedas.
    transform(codeLower.begin(), codeLower.end(), codeLower.begin(), ::tolower);

    // Mapa de tipo de algoritmo -> palabras clave que lo caracterizan.
    map<string, vector<string>> patterns = {
        {"Linear Search", {"for", "arr[i]", "==", "return"}},
        {"Binary Search", {"while", "low", "high", "mid"}},
        {"Bubble Sort", {"for", "for", "swap"}},
        {"Insertion Sort", {"for", "while", "insert"}},
        {"Selection Sort", {"for", "for", "min"}},
        {"Merge Sort", {"merge", "mid", "recursiv"}},
        {"Quick Sort", {"pivot", "partition"}},
        {"Fibonacci", {"fib", "recursiv"}},
        {"Factorial", {"factorial", "recursiv"}},
        {"Matrix Multiplication", {"for", "for", "for", "matrix"}},
    };

    map<string, int> scores;

    // Recorre cada patrón y suma "puntos" según las coincidencias.
    for (const auto& [algorithm, keywords] : patterns) {
        int score = 0;
        for (const auto& keyword : keywords) {
            if (codeLower.find(keyword) != string::npos) {
                score++;
            }
        }
        // Si se cumple un umbral mínimo, se devuelve directamente ese algoritmo.
        if (score >= 1) {  // Umbral reducido a 1 para detección más flexible
            return algorithm;
        }

    }

    // Si no se devolvió antes, se busca el mejor puntaje entre todos.
    auto maxIt = max_element(scores.begin(), scores.end(),
        [](const pair<string, int>& a, const pair<string, int>& b) {
            return a.second < b.second;
        });

    if (maxIt != scores.end() && maxIt->second >= 2) {
        return maxIt->first;
    }

    // Si el código contiene main pero no coincide con ningún algoritmo conocido,
    // se clasifica como programa principal o simple I/O.
    if (code.find("main") != string::npos) {
        if (code.find("cin") != string::npos || code.find("cout") != string::npos) {
            return "Simple I/O Program";
        }
        return "Main Program";
    }

    // Caso por defecto.
    return "Custom Algorithm";
}

// Detecta si la función principal se llama a sí misma dentro de su cuerpo (recursión directa).
bool ComplexityAnalyzer::detectRecursion() {
    if (functionName.empty()) return false;

    // Regex que busca llamadas a la función por nombre seguida de '('.
    regex callRegex(functionName + R"(\s*\()");

    // Ubica la definición de la función.
    size_t defPos = code.find(functionName + "(");
    if (defPos == string::npos) return false;

    // Solo se analiza el código a partir de la definición.
    string functionBody = code.substr(defPos);

    smatch match;
    int calls = 0;
    auto searchStart = functionBody.cbegin();

    // Cuenta cuántas veces aparece una llamada a la función en su propio cuerpo.
    while (regex_search(searchStart, functionBody.cend(), match, callRegex)) {
        calls++;
        searchStart = match.suffix().first;
        if (calls > 1) return true;   // Se considera recursivo si hay más de una llamada.
    }

    return calls>1;
}

// Determina la complejidad usando tiempos empíricos si existen; si no, usa análisis estático.
string ComplexityAnalyzer::determineComplexity(const vector<double>& times) {
    if (times.size() < 2) {
        // Fallback a análisis estático si no hay suficientes datos empíricos.

        if (detectRecursion()) {
            // Heurística simple: si es recursivo y no hay más info, se asume exponencial.
            return "O(2^n)";
        }

        int loops = countNestedLoops();
        switch (loops) {
            case 0: return "O(1)"; // Sin loops: tiempo constante aproximado.
            case 1: return detectLogarithmic() ? "O(log n)" : "O(n)";
            case 2: return "O(n²)";
            case 3: return "O(n³)";
            default: return "O(n^" + to_string(loops) + ")";
        }
    }

    // Si hay tiempos de ejecución, se calcula una razón promedio entre ellos.
    double avgRatio = 0;
    for (size_t i = 1; i < times.size(); i++) {
        if (times[i-1] > 0) {
            avgRatio += times[i] / times[i-1];
        }
    }
    avgRatio /= (times.size() - 1);

    // Se compara la razón promedio contra rangos típicos de crecimiento.
    if (avgRatio < 1.5) return "O(log n)";
    else if (avgRatio <= 2.3) return "O(n)";
    else if (avgRatio <= 2.5) return "O(n log n)";
    else if (avgRatio <= 4.5) return "O(n²)";
    else if (avgRatio <= 9.0) return "O(n³)";
    else return "O(2^n)";
}

// Construye una explicación en texto de la complejidad, usando mensajes predefinidos.
string ComplexityAnalyzer::generateExplanation(const string& complexity, const string& type) {
    map<string, string> explanations = {
        {"O(1)", "Constant complexity - execution time does not depend on input size."},
        {"O(log n)", "Logarithmic complexity - grows slowly as input increases."},
        {"O(n)", "Linear complexity - grows proportionally to input size."},
        {"O(n log n)", "Linearithmic complexity - efficient sorting algorithms."},
        {"O(n²)", "Quadratic complexity - two nested loops."},
        {"O(n³)", "Cubic complexity - three nested loops."},
        {"O(2^n)", "Exponential complexity - doubles with each additional element."}
    };

    string explanation = explanations[complexity];
    explanation += " Detected algorithm: " + type + ".";

    return explanation;
}

// Devuelve sugerencias básicas de optimización según la complejidad detectada.
vector<string> ComplexityAnalyzer::generateSuggestions(const string& complexity) {
    vector<string> suggestions;

    if (complexity == "O(n²)") {
        suggestions.push_back("Consider O(n log n) algorithms like MergeSort or QuickSort.");
        suggestions.push_back("Use HashMaps or Sets to reduce lookups to O(1).");
    }
    else if (complexity == "O(n³)") {
        suggestions.push_back("Very high complexity. Try to eliminate unnecessary nested loops.");
        suggestions.push_back("Consider dynamic programming techniques.");
    }
    else if (complexity == "O(2^n)") {
        suggestions.push_back("Exponential complexity - not feasible for large inputs.");
        suggestions.push_back("Use memoization or dynamic programming to optimize.");
    }
    else if (complexity == "O(n log n)") {
        suggestions.push_back("Excellent! This is efficient for most cases.");
    }
    else if (complexity == "O(n)") {
        suggestions.push_back("Optimal complexity for problems requiring element traversal.");
    }
    else if (complexity == "O(log n)") {
        suggestions.push_back("Very efficient! Ideal for search operations.");
    }

    return suggestions;
}

// Orquesta el análisis completo combinando análisis estático y (en el futuro) empírico.
AnalysisResult ComplexityAnalyzer::analyze() {
    AnalysisResult result;

    // Extraer nombre de función (puede ser cualquiera ahora)
    functionName = extractFunctionName();

    cout << "[ANALYZER] Starting analysis..." << endl;
    cout << "[ANALYZER] Code length: " << code.length() << " chars" << endl;

    // Análisis estático de loops
    result.nestedLoops = countNestedLoops();

    // Identificar tipo de algoritmo
    result.algorithmType = identifyAlgorithmType();

    cout << "[ANALYZER] Nested loops: " << result.nestedLoops << endl;
    cout << "[ANALYZER] Is recursive: " << (result.isRecursive ? "yes" : "no") << endl;
    cout << "[ANALYZER] Algorithm type: " << result.algorithmType << endl;

    // Análisis empírico (por ahora vacío, se deja preparado para tiempos medidos externamente).

    if (!result.executionTimes.empty()) {
        // Calcula la razón promedio entre tiempos consecutivos si vienen llenos.
        result.averageRatio = 0;
        for (size_t i = 1; i < result.executionTimes.size(); i++) {
            if (result.executionTimes[i-1] > 0) {
                result.averageRatio += result.executionTimes[i] / result.executionTimes[i-1];
            }
        }
        result.averageRatio /= (result.executionTimes.size() - 1);
    } else {
        // Si no hay tiempos, se deja en 0.0.
        result.averageRatio = 0.0;
    }

    // Determinar complejidad teórica/empírica.
    result.complexity = determineComplexity(result.executionTimes);
    cout << "[ANALYZER] Complexity: " << result.complexity << endl;

    // Generar explicación textual.
    result.explanation = generateExplanation(result.complexity, result.algorithmType);

    // Generar sugerencias (idealmente usando Gemini si está configurado).
    cout << "[ANALYZER] Generating suggestions..." << endl;
    result.suggestions = generateSuggestionsWithGemini(result.complexity, result.consoleOutput);
    cout << "[ANALYZER] Generated " << result.suggestions.size() << " suggestions" << endl;

    result.success = true;
    return result;
}

// Genera sugerencias usando Gemini si hay API key; si no, cae en sugerencias estáticas.
vector<string> ComplexityAnalyzer::generateSuggestionsWithGemini(
    const string& complexity,
    const string& consoleOutput
) {
    if (geminiApiKey.empty()) {
        // Si no hay API key, usar sugerencias estáticas
        return generateSuggestions(complexity);
    }

    try {
        GeminiClient gemini(geminiApiKey);
        string algorithmType = identifyAlgorithmType();

        // Se delega la generación de sugerencias al cliente de Gemini.
        return gemini.generateSuggestions(code, complexity, algorithmType, consoleOutput);
    } catch (const exception& e) {
        cerr << "[WARN] Gemini API failed, using static suggestions: " << e.what() << endl;
        return generateSuggestions(complexity);
    }
}