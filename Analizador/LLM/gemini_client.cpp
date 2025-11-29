#include "gemini_client.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
GeminiClient::GeminiClient(const string& key) :
    apiKey(key) {
    apiUrl = "https://generativelanguage.googleapis.com/v1/models/gemini-2.0-flash-lite:generateContent"; // URL para generar
}
size_t GeminiClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

vector<string> GeminiClient::generateSuggestions(
    const string& code,
    const string& complexity,
    const string& algorithmType,
    const string& consoleOutput
) {
    vector<string> suggestions;

    if (apiKey.empty()) {
        suggestions.push_back("Gemini API key not configured");
        return suggestions;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        suggestions.push_back("Failed to initialize CURL");
        return suggestions;
    }

    try {
        // Construir el prompt
        stringstream prompt;
        prompt << "Eres un asiste experto en optimización de codigo para C++ .\n\n";
        prompt << "Codigo a analizar:\n```cpp\n" << code << "\n```\n\n";
        prompt << "Analisis de complejidad:\n";
        prompt << "- Complejidad detectada: " << complexity << "\n";
        prompt << "- Tipo de algoritmo: " << algorithmType << "\n\n";

        if (!consoleOutput.empty()) {
            prompt << "Salida en consola:\n" << consoleOutput << "\n\n";
        }

        prompt << "TAREA:\n";
        prompt << "Proporciona  3 sugerencias especificas y practicas para mejorar este codigo.\n";
        prompt << "Enfocate en:\n";
        prompt << "1. Mejoras en la eficiencia del algoritmo\n";
        prompt << "2. Mejoras en las estructuras de dtaos empleadas\n";
        prompt << "3. Tecnicas de optimización del codigo\n";
        prompt << "Formato: Devuelve SOLO uns array de strings JSON, donde cada string es una sugerencia.\n";
        prompt << "Ejemplo: [\"Usar un mapa hash en lugar de busqueda lineal\", \"Considerar using std::sort\"]\n";
        prompt << "Las sugerencias deben de ser concisas (max 100 caracteres cada una).";

        // Construir JSON payload
        json payload = {
            {"contents", json::array({
                {
                    {"parts", json::array({
                        {{"text", prompt.str()}}
                    })}
                }
            })},
            {"generationConfig", {
                {"temperature", 0.7},
                {"topK", 40},
                {"topP", 0.95},
                {"maxOutputTokens", 1024}
            }}
        };

        string payloadStr = payload.dump();

        // Configurar CURL
        string response;
        string url = apiUrl + "?key=" + apiKey;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        // Ejecutar request
        CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            suggestions.push_back("Network error: " + string(curl_easy_strerror(res)));
            curl_easy_cleanup(curl);
            return suggestions;
        }

        // Parsear respuesta
        try {
            json responseJson = json::parse(response);

            if (responseJson.contains("candidates") &&
                !responseJson["candidates"].empty() &&
                responseJson["candidates"][0].contains("content")) {

                string text = responseJson["candidates"][0]["content"]["parts"][0]["text"];

                // Intentar parsear como JSON array
                try {
                    // Buscar el array JSON en la respuesta
                    size_t start = text.find('[');
                    size_t end = text.rfind(']');

                    if (start != string::npos && end != string::npos && end > start) {
                        string jsonArrayStr = text.substr(start, end - start + 1);
                        json suggestionsJson = json::parse(jsonArrayStr);

                        for (const auto& suggestion : suggestionsJson) {
                            if (suggestion.is_string()) {
                                suggestions.push_back(suggestion.get<string>());
                            }
                        }
                    } else {
                        // Si no hay JSON array, usar el texto completo dividido por líneas
                        stringstream ss(text);
                        string line;
                        while (getline(ss, line)) {
                            if (!line.empty() && line.length() > 5) {
                                // Limpiar números de lista, guiones, etc.
                                size_t pos = line.find_first_not_of("0123456789.-* \t");
                                if (pos != string::npos) {
                                    suggestions.push_back(line.substr(pos));
                                }
                            }
                        }
                    }
                } catch (...) {
                    // Si falla el parsing, usar el texto completo
                    suggestions.push_back(text);
                }
            } else if (responseJson.contains("error")) {
                suggestions.push_back("API Error: " + responseJson["error"]["message"].get<string>());
            } else {
                suggestions.push_back("Unexpected API response format");
            }
        } catch (const json::exception& e) {
            suggestions.push_back("JSON parse error: " + string(e.what()));
        }

        curl_easy_cleanup(curl);

    } catch (const exception& e) {
        suggestions.push_back("Exception: " + string(e.what()));
        curl_easy_cleanup(curl);
    }

    // Si no se generaron sugerencias, dar una por defecto
    if (suggestions.empty()) {
        suggestions.push_back("Considerar revisar la complejidad del algoritmo y las estructuras de datos utilizadas.");
    }

    return suggestions;
}