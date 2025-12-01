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
    int maxDepth = 0;
    int n = code.size();

    for (int i = 0; i < n; i++) {
        // Detectar "for" o "while" seguido de espacio o "("
        bool isLoop =
            (i + 3 <= n && code.compare(i, 3, "for") == 0 &&
             (i + 3 == n || isspace(code[i+3]) || code[i+3]=='(')) ||
            (i + 5 <= n && code.compare(i, 5, "while") == 0 &&
             (i + 5 == n || isspace(code[i+5]) || code[i+5]=='('));

        if (!isLoop)
            continue;

        // Calcular profundidad desde este loop
        int depth = calculateDepth(i);
        maxDepth = max(maxDepth, depth);
    }

    return maxDepth;
}

int ComplexityAnalyzer::calculateDepth(int loopStart) {
    int n = code.size();

    // Encontrar '(' del loop
    size_t p = code.find('(', loopStart);
    if (p == string::npos || p - loopStart > 8)
        return 1;

    // Encontrar ')' que cierra la condición
    int parenDepth = 1;
    size_t closeParen = p + 1;
    while (closeParen < n && parenDepth > 0) {
        if (code[closeParen] == '(') parenDepth++;
        else if (code[closeParen] == ')') parenDepth--;
        closeParen++;
    }

    if (parenDepth != 0)
        return 1;

    closeParen--; // Retroceder al ')'

    // Saltar espacios después del ')'
    size_t bodyStart = closeParen + 1;
    while (bodyStart < n && isspace(code[bodyStart]))
        bodyStart++;

    if (bodyStart >= n)
        return 1;

    int maxInnerDepth = 0;

    // Caso 1: Loop con llaves { }
    if (code[bodyStart] == '{') {
        int braceDepth = 1;
        size_t j = bodyStart + 1;

        while (j < n && braceDepth > 0) {
            if (code[j] == '{') {
                braceDepth++;
            }
            else if (code[j] == '}') {
                braceDepth--;
                if (braceDepth == 0) break;
            }

            // Detectar loop interno
            bool innerLoop =
                (j + 3 <= n && code.compare(j, 3, "for") == 0 &&
                 (j + 3 == n || isspace(code[j+3]) || code[j+3]=='(')) ||
                (j + 5 <= n && code.compare(j, 5, "while") == 0 &&
                 (j + 5 == n || isspace(code[j+5]) || code[j+5]=='('));

            if (innerLoop) {
                int innerDepth = calculateDepth(j);
                maxInnerDepth = max(maxInnerDepth, innerDepth);
            }

            j++;
        }
    }
    // Caso 2: Loop sin llaves (una sola statement)
    else {
        // El cuerpo es una sola statement
        // Buscar si esa statement es otro loop
        size_t j = bodyStart;

        // Detectar si es un loop
        bool innerLoop =
            (j + 3 <= n && code.compare(j, 3, "for") == 0 &&
             (j + 3 == n || isspace(code[j+3]) || code[j+3]=='(')) ||
            (j + 5 <= n && code.compare(j, 5, "while") == 0 &&
             (j + 5 == n || isspace(code[j+5]) || code[j+5]=='('));

        if (innerLoop) {
            maxInnerDepth = calculateDepth(j);
        }
    }

    return 1 + maxInnerDepth;
}

bool ComplexityAnalyzer::detectRecursion() {
    if (functionName.empty()) return false;

    // Encontrar la definición de la función
    size_t defPos = code.find(functionName + "(");
    if (defPos == string::npos) return false;

    // Buscar llave que abre el cuerpo
    size_t braceOpen = code.find('{', defPos);
    if (braceOpen == string::npos) return false;

    // Buscar llave que cierra el cuerpo
    int depth = 1;
    size_t i = braceOpen + 1;

    for (; i < code.size() && depth > 0; i++) {
        if (code[i] == '{') depth++;
        else if (code[i] == '}') depth--;
    }

    if (depth != 0) return false;

    size_t braceClose = i - 1;

    // Extraer cuerpo
    string body = code.substr(braceOpen + 1, braceClose - braceOpen - 1);

    // Buscar llamada recursiva real
    regex callRegex("\\b" + functionName + "\\s*\\(");

    return regex_search(body, callRegex);
}

// Busca patrones sencillos que sugieren complejidad logarítmica, como i *= 2.
bool ComplexityAnalyzer::detectLogarithmic() {
    regex logRegex(R"(\w+\s*[*/]=\s*2)");
    return regex_search(code, logRegex);
}

// Intenta identificar el tipo de algoritmo por palabras clave típicas en el código.
string ComplexityAnalyzer::identifyAlgorithmType() {
    string c = code;
    transform(c.begin(), c.end(), c.begin(), ::tolower);

    struct Algo {
        string name;
        vector<string> keys;
        int minScore;
    };

    vector<Algo> algos = {
        {"Fibonacci", {"fib(", "return fib"}, 1},
        {"Binary Search", {"low", "high", "mid"}, 2},
        {"Bubble Sort", {"swap", "for", "for"}, 2},
        {"Insertion Sort", {"while", "insert"}, 2},
        {"Selection Sort", {"min", "for", "for"}, 2},
        {"Merge Sort", {"merge", "mergesort"}, 1},
        {"Quick Sort", {"pivot", "partition"}, 1},
        {"Linear Search", {"for", "=="}, 2}
    };

    string best = "Custom Algorithm";
    int bestScore = 0;

    for (auto& a : algos) {
        int score = 0;
        for (auto& k : a.keys)
            if (c.find(k) != string::npos)
                score++;

        if (score >= a.minScore && score > bestScore) {
            bestScore = score;
            best = a.name;
        }
    }

    return best;
}

// Determina la complejidad usando tiempos empíricos si existen; si no, usa análisis estático.
// Determina la complejidad usando tiempos empíricos si existen; si no, usa análisis estático.
string ComplexityAnalyzer::determineComplexity(const vector<double>& times) {
     // ============================
    //  ANÁLISIS ESTÁTICO
    // ============================
    if (times.size() < 2) {

        string alg = identifyAlgorithmType();
        int loops = countNestedLoops();
        bool rec = detectRecursion();

        // ---- Recursión manual ----
        if (rec)
        {
            // Algoritmos recursivos conocidos tienen prioridad
            if (alg == "Fibonacci")                return "O(2^n)";
            if (alg == "Merge Sort" || alg == "Quick Sort") return "O(n log n)";

            // doble llamada recursiva → exponencial
            regex dbl("\\b" + functionName + R"(\s*\(.*\).*\b)" + functionName + R"(\s*\()");

            if (regex_search(code, dbl))
                return "O(2^n)";

            // recursión lineal simple
            return "O(n)";
        }

        // ---- No recursivo: PRIORIZAR loops sobre algoritmos ----

        // Si tenemos loops claros, usar eso primero
        if (loops > 0) {
            switch (loops)
            {
                case 1: return detectLogarithmic() ? "O(log n)" : "O(n)";
                case 2: return "O(n^2)";
                case 3: return "O(n^3)";
                default: return "O(n^" + to_string(loops) + ")";
            }
        }

        // Si no hay loops, verificar algoritmos conocidos
        if (alg == "Linear Search")           return "O(n)";
        if (alg == "Binary Search")           return "O(log n)";
        if (alg == "Bubble Sort" ||
            alg == "Insertion Sort" ||
            alg == "Selection Sort")          return "O(n^2)";
        if (alg == "Matrix Multiplication")    return "O(n^3)";

        // Sin loops y sin algoritmo conocido
        return "O(1)";
    }

    // ============================
    //  ANÁLISIS EMPÍRICO
    // ============================
    double avgRatio = 0;
    int count = 0;

    for (size_t i = 1; i < times.size(); i++) {
        if (times[i-1] > 0) {
            avgRatio += times[i] / times[i-1];
            count++;
        }
    }

    if (count == 0)
        return determineComplexity({});

    avgRatio /= count;

    // O(log n) crece muy lento
    if (avgRatio < 1.5) return "O(log n)";
    if (avgRatio <= 2.3) return "O(n)";
    if (avgRatio <= 3.5) return "O(n log n)";
    if (avgRatio <= 6.0) return "O(n^2)";
    if (avgRatio <= 12.0) return "O(n^3)";

    return "O(2^n)";
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

    result.isRecursive = detectRecursion();

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