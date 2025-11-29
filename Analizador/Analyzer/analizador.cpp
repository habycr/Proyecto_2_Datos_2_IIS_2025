//
// Created by dylan on 22/11/25.
//
// AlgorithmComplexityAnalyzer/analyzer.cpp
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

ComplexityAnalyzer::ComplexityAnalyzer(const string& sourceCode, const string& apiKey)
    : code(sourceCode), geminiApiKey(apiKey) {
    functionName = extractFunctionName ();
}

string ComplexityAnalyzer::extractFunctionName() {
    regex funcRegex(R"(\b(void|int|long|double|float|bool)\s+(\w+)\s*\([^)]*int\s+n[^)]*\))");
    smatch match;

    if (regex_search(code, match, funcRegex)) {
        return match[2];
    }
    return "";
}

int ComplexityAnalyzer::countNestedLoops() {
    int maxDepth = 0;
    int currentDepth = 0;

    for (size_t i = 0; i < code.length(); i++) {
        // Detectar 'for'
        if (i + 3 <= code.length() && code.substr(i, 3) == "for") {
            if (i > 0 && isalnum(code[i-1])) continue;
            if (i + 3 < code.length() && isalnum(code[i+3])) continue;

            currentDepth++;
            maxDepth = max(maxDepth, currentDepth);
        }
        // Detectar 'while'
        else if (i + 5 <= code.length() && code.substr(i, 5) == "while") {
            if (i > 0 && isalnum(code[i-1])) continue;
            if (i + 5 < code.length() && isalnum(code[i+5])) continue;

            currentDepth++;
            maxDepth = max(maxDepth, currentDepth);
        }

        // Detectar cierre de bloque
        if (code[i] == '}') {
            if (currentDepth > 0) currentDepth--;
        }
    }

    return maxDepth;
}

bool ComplexityAnalyzer::detectRecursion() {
    if (functionName.empty()) return false;

    regex callRegex(functionName + R"(\s*\()");

    size_t defPos = code.find(functionName + "(");
    if (defPos == string::npos) return false;

    string functionBody = code.substr(defPos);

    smatch match;
    int calls = 0;
    auto searchStart = functionBody.cbegin();

    while (regex_search(searchStart, functionBody.cend(), match, callRegex)) {
        calls++;
        searchStart = match.suffix().first;
        if (calls > 1) return true;
    }

    return calls > 1;
}

bool ComplexityAnalyzer::detectLogarithmic() {
    regex logRegex(R"(\w+\s*[*/]=\s*2)");
    return regex_search(code, logRegex);
}

string ComplexityAnalyzer::identifyAlgorithmType() {
    string codeLower = code;
    transform(codeLower.begin(), codeLower.end(), codeLower.begin(), ::tolower);

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

    for (const auto& [algorithm, keywords] : patterns) {
        int score = 0;
        for (const auto& keyword : keywords) {
            if (codeLower.find(keyword) != string::npos) {
                score++;
            }
        }
        scores[algorithm] = score;
    }

    auto maxIt = max_element(scores.begin(), scores.end(),
        [](const pair<string, int>& a, const pair<string, int>& b) {
            return a.second < b.second;
        });

    if (maxIt != scores.end() && maxIt->second >= 2) {
        return maxIt->first;
    }

    return "Custom Algorithm";
}

bool ComplexityAnalyzer::compileAndRun(int n, double& time) {
    ofstream file("temp_analysis.cpp");

    file << "#include <iostream>\n";
    file << "#include <vector>\n";
    file << "#include <algorithm>\n";
    file << "#include <chrono>\n";
    file << "#include <queue>\n";
    file << "#include <cstdlib>\n";
    file << "#include <ctime>\n";
    file << "using namespace std;\n";
    file << "using namespace chrono;\n\n";

    file << code << "\n\n";

    file << "int main() {\n";
    file << "    srand(time(nullptr));\n";
    file << "    auto start = high_resolution_clock::now();\n";
    file << "    " << functionName << "(" << n << ");\n";
    file << "    auto end = high_resolution_clock::now();\n";
    file << "    auto duration = duration_cast<microseconds>(end - start).count();\n";
    file << "    cout << duration << endl;\n";
    file << "    return 0;\n";
    file << "}\n";

    file.close();

    // Compilar con cl.exe (Visual Studio) o g++
    int result = system("cl /EHsc /std:c++17 /O2 temp_analysis.cpp >nul 2>&1");

    if (result != 0) {
        result = system("g++ -std=c++17 -O2 temp_analysis.cpp -o temp_analysis.exe 2>nul");

        if (result != 0) {
            return false;
        }
    }

    // Ejecutar
#ifdef _WIN32
    FILE* pipe = _popen("temp_analysis.exe", "r");
#else
    FILE* pipe = popen("./temp_analysis", "r");
#endif

    if (!pipe) {
        return false;
    }

    char buffer[128];
    string resultStr;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        resultStr += buffer;
    }

#ifdef _WIN32
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    try {
        time = stod(resultStr) / 1000.0; // Convert to ms
        return true;
    } catch (...) {
        return false;
    }
}

vector<double> ComplexityAnalyzer::measureExecutionTimes() {
    vector<int> sizes = {100, 200, 400, 800, 1600};
    vector<double> times;

    cout << "[INFO] Measuring execution times..." << endl;

    for (int n : sizes) {
        double time;
        if (compileAndRun(n, time)) {
            times.push_back(time);
            cout << "[INFO]   n=" << n << ": " << time << " ms" << endl;
        } else {
            cout << "[WARN]   Failed at n=" << n << endl;
            break;
        }
    }

    // Cleanup
#ifdef _WIN32
    system("del temp_analysis.cpp temp_analysis.exe temp_analysis.obj 2>nul");
#else
    system("rm -f temp_analysis.cpp temp_analysis temp_analysis.o");
#endif

    return times;
}

string ComplexityAnalyzer::determineComplexity(const vector<double>& times) {
    if (times.size() < 2) {
        // Fallback to static analysis
        if (detectRecursion()) {
            return "O(2^n)";
        }

        int loops = countNestedLoops();
        switch (loops) {
            case 0: return "O(1)";
            case 1: return detectLogarithmic() ? "O(log n)" : "O(n)";
            case 2: return "O(n²)";
            case 3: return "O(n³)";
            default: return "O(n^" + to_string(loops) + ")";
        }
    }

    double avgRatio = 0;
    for (size_t i = 1; i < times.size(); i++) {
        if (times[i-1] > 0) {
            avgRatio += times[i] / times[i-1];
        }
    }
    avgRatio /= (times.size() - 1);

    if (avgRatio < 1.5) return "O(log n)";
    else if (avgRatio <= 2.3) return "O(n)";
    else if (avgRatio <= 2.5) return "O(n log n)";
    else if (avgRatio <= 4.5) return "O(n²)";
    else if (avgRatio <= 9.0) return "O(n³)";
    else return "O(2^n)";
}

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

AnalysisResult ComplexityAnalyzer::analyze() {
    AnalysisResult result;

    if (functionName.empty()) {
        result.success = false;
        result.errorMessage = "No valid function detected with 'int n' parameter";
        return result;
    }

    result.nestedLoops = countNestedLoops();
    result.isRecursive = detectRecursion();
    result.algorithmType = identifyAlgorithmType();

    result.executionTimes = measureExecutionTimes();

    if (!result.executionTimes.empty()) {
        result.averageRatio = 0;
        for (size_t i = 1; i < result.executionTimes.size(); i++) {
            if (result.executionTimes[i-1] > 0) {
                result.averageRatio += result.executionTimes[i] / result.executionTimes[i-1];
            }
        }
        result.averageRatio /= (result.executionTimes.size() - 1);
    }

    result.complexity = determineComplexity(result.executionTimes);
    result.explanation = generateExplanation(result.complexity, result.algorithmType);
    result.suggestions = generateSuggestions(result.complexity);
    result.suggestions = generateSuggestionsWithGemini(result.complexity, result.consoleOutput);


    result.success = true;
    return result;
}

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

        return gemini.generateSuggestions(code, complexity, algorithmType, consoleOutput);
    } catch (const exception& e) {
        cerr << "[WARN] Gemini API failed, using static suggestions: " << e.what() << endl;
        return generateSuggestions(complexity);
    }
}