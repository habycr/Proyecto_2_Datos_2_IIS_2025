
// Analizador/main.cpp
#define CROW_MAIN


#include <crow.h>
#include "Analyzer/analizador.h"
#include "Env/env_loader.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    crow::SimpleApp app;

    // Cargar variables de entorno desde .env
    EnvLoader env(".env");
    string geminiKey = env.get("GEMINI_API_KEY", "");

    cout << "========================================" << endl;
    cout << "   CodeCoach Complexity Analyzer API   " << endl;
    cout << "========================================" << endl;

    if (!geminiKey.empty()) {
        cout << "[INFO] Gemini API: Enabled" << endl;
        cout << "[INFO] API Key: " << geminiKey.substr(0, 10) << "..." << endl;
    } else {
        cout << "[WARN] Gemini API: Disabled" << endl;
        cout << "[INFO] To enable: Set GEMINI_API_KEY in .env file" << endl;
    }

    cout << "========================================" << endl;

    // Endpoint: Analizar complejidad
    CROW_ROUTE(app, "/api/analyze").methods(crow::HTTPMethod::Post)
    ([geminiKey](const crow::request& req) {
        crow::json::wvalue response;

        try {
            auto body = crow::json::load(req.body);

            if (!body) {
                response["success"] = false;
                response["error"] = "Invalid JSON format";
                crow::response res(400, response);
                res.add_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            string code = body["code"].s();
            string problemName = "";
            string consoleOutput = "";

            try {
                if (body.has("problemName")) {
                    problemName = body["problemName"].s();
                }
                if (body.has("consoleOutput")) {
                    consoleOutput = body["consoleOutput"].s();
                }
            } catch (...) {}

            cout << "[INFO] Analyzing code submission..." << endl;
            if (!problemName.empty()) {
                cout << "[INFO] Problem: " << problemName << endl;
            }

            // Analizar código con Gemini API key
            ComplexityAnalyzer analyzer(code, geminiKey);
            AnalysisResult result = analyzer.analyze();

            // Construir respuesta
            response["success"] = result.success;
            response["complexity"] = result.complexity;
            response["algorithmType"] = result.algorithmType;
            response["details"]["nestedLoops"] = result.nestedLoops;
            response["details"]["isRecursive"] = result.isRecursive;
            response["details"]["averageRatio"] = result.averageRatio;

            for (size_t i = 0; i < result.executionTimes.size(); i++) {
                response["details"]["executionTimes"][i] = result.executionTimes[i];
            }

            response["explanation"] = result.explanation;

            for (size_t i = 0; i < result.suggestions.size(); i++) {
                response["suggestions"][i] = result.suggestions[i];
            }

            if (!result.success) {
                response["error"] = result.errorMessage;
            }

            cout << "[SUCCESS] Analysis completed: " << result.complexity << endl;

        } catch (const exception& e) {
            response["success"] = false;
            response["error"] = string("Exception: ") + e.what();
            cerr << "[ERROR] " << e.what() << endl;
        }

        crow::response res(response);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Content-Type", "application/json; charset=utf-8");
        return res;
    });

    // Health check
    CROW_ROUTE(app, "/api/health").methods(crow::HTTPMethod::Get)
    ([]() {
        crow::json::wvalue health;
        health["status"] = "OK";
        health["timestamp"] = time(nullptr);
        health["version"] = "1.0.0";
        health["service"] = "CodeCoach Complexity Analyzer";

        crow::response res(health);
        res.add_header("Access-Control-Allow-Origin", "*");
        return res;
    });

    // Test endpoint
    CROW_ROUTE(app, "/api/test")
    ([](const crow::request&) {
        crow::json::wvalue response;

        try {
            cout << "[TEST] Running static analysis test..." << endl;

            response["success"] = true;
            response["complexity"] = "O(n²)";
            response["algorithmType"] = "Bubble Sort (Test)";
            response["details"]["nestedLoops"] = 2;
            response["details"]["isRecursive"] = false;
            response["details"]["averageRatio"] = 0.0;
            response["explanation"] = "Static analysis: Detected 2 nested loops.";
            response["suggestions"][0] = "Consider using O(n log n) algorithms";
            response["note"] = "Static analysis only";

            cout << "[TEST] Test completed" << endl;

        } catch (const exception& e) {
            response["success"] = false;
            response["error"] = string("Test error: ") + e.what();
        }

        crow::response res(response);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Content-Type", "application/json; charset=utf-8");
        return res;
    });

// ========== NUEVO: Test de conexión con Gemini ==========
CROW_ROUTE(app, "/api/test-gemini")
([geminiKey](const crow::request&) {
    crow::json::wvalue response;

    try {
        cout << "[TEST-GEMINI] Testing Gemini API connection..." << endl;

        if (geminiKey.empty()) {
            response["success"] = false;
            response["error"] = "Gemini API key not configured";
            response["message"] = "Set GEMINI_API_KEY in .env file";

            crow::response res(response);
            res.add_header("Access-Control-Allow-Origin", "*");
            return res;
        }

        // Código de prueba simple
        string testCode = R"(
void linearSearch(int n) {
    vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}
        )";

        // Crear analizador con API key
        ComplexityAnalyzer analyzer(testCode, geminiKey);
        AnalysisResult result = analyzer.analyze();

        if (result.success) {
            response["success"] = true;
            response["geminiConnected"] = !result.suggestions.empty();
            response["model"] = "gemini-2.0-flash-lite";
            response["complexity"] = result.complexity;
            response["algorithmType"] = result.algorithmType;

            // Verificar si las sugerencias vienen de Gemini o son estáticas
            bool isGeminiResponse = false;
            for (const auto& suggestion : result.suggestions) {
                // Las sugerencias de Gemini suelen ser más largas y específicas
                if (suggestion.length() > 50) {
                    isGeminiResponse = true;
                    break;
                }
            }

            response["suggestionsSource"] = isGeminiResponse ? "Gemini AI" : "Static fallback";
            response["suggestionsCount"] = static_cast<int>(result.suggestions.size());

            for (size_t i = 0; i < result.suggestions.size(); i++) {
                response["suggestions"][i] = result.suggestions[i];
            }

            cout << "[TEST-GEMINI] ✓ Connection successful" << endl;
            cout << "[TEST-GEMINI] Model: gemini-2.0-flash-lite" << endl;
            cout << "[TEST-GEMINI] Suggestions source: " << (isGeminiResponse ? "Gemini AI" : "Static") << endl;

        } else {
            response["success"] = false;
            response["error"] = result.errorMessage;
        }

    } catch (const exception& e) {
        response["success"] = false;
        response["error"] = string("Exception: ") + e.what();
        cerr << "[TEST-GEMINI] ✗ Error: " << e.what() << endl;
    }

    crow::response res(response);
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Content-Type", "application/json; charset=utf-8");
    return res;
});

    // CORS
    CROW_ROUTE(app, "/api/<path>").methods(crow::HTTPMethod::Options)
    ([](const crow::request&, string) {
        crow::response res(200);
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type");
        return res;
    });

    cout << "Server started at http://localhost:8080" << endl;
    cout << "========================================" << endl;

    app.port(8081)
        .multithreaded()
        .run();

    return 0;
}