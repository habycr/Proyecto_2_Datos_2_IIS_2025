#include "gemini_client.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Constructor: guarda la API key y configura la URL del modelo de Gemini a usar.
GeminiClient::GeminiClient(const string& key) :
    apiKey(key) {
    // Endpoint del modelo seleccionado (flash-lite en este caso)
    apiUrl = "https://generativelanguage.googleapis.com/v1/models/gemini-2.0-flash-lite:generateContent";
}

// Callback que usa libcurl para ir escribiendo la respuesta HTTP en un std::string.
size_t GeminiClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Llama a la API de Gemini para generar sugerencias de mejora a partir del código y su análisis.
vector<string> GeminiClient::generateSuggestions(
    const string& code,
    const string& complexity,
    const string& algorithmType,
    const string& consoleOutput
) {
    vector<string> suggestions;

    // Si no hay API key, se informa con una sugerencia básica.
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
        // ============================================================
        // 1) CONSTRUCCIÓN DEL PROMPT
        //    - Contextualizamos el rol del modelo como LLM Coach.
        //    - Dejamos claro que solo debe hablar del código del estudiante.
        //    - Definimos el FORMATO ESTRICTO de salida: arreglo JSON de objetos
        //      con campos "explicacion" y "codigo".
        // ============================================================
        stringstream prompt;
        prompt << "Actuas como el 'LLM Coach' de una plataforma de practica de algoritmos "
               << "llamada CodeCoach.\n";
        prompt << "Tu unico objetivo es ayudar al ESTUDIANTE que escribio el siguiente "
               << "codigo en C++.\n";
        prompt << "Asume que el motor de evaluacion, los tests automatizados y la propia "
               << "plataforma CodeCoach funcionan correctamente.\n";
        prompt << "NO des comentarios ni sugerencias sobre el motor, la plataforma o el "
               << "diseno general del sistema; solo sobre la solucion del estudiante.\n\n";

        prompt << "Codigo del estudiante (C++):\n```cpp\n" << code << "\n```\n\n";

        prompt << "Analisis de complejidad calculado por otra herramienta:\n";
        prompt << "- Complejidad detectada: " << complexity << "\n";
        prompt << "- Tipo de algoritmo detectado: " << algorithmType << "\n\n";

        if (!consoleOutput.empty()) {
            prompt << "Resultados del motor de evaluacion (tests y salida en consola):\n";
            prompt << consoleOutput << "\n\n";
        }

        prompt << "TAREA DEL COACH:\n";
        prompt << "Analiza SOLO la solucion del ESTUDIANTE (codigo C++ anterior) ";
        prompt << "usando la informacion de complejidad y los resultados de pruebas.\n";
        prompt << "- Asume que la plataforma y el motor de evaluacion funcionan bien.\n";
        prompt << "- No hagas comentarios sobre la plataforma, solo sobre el codigo del estudiante.\n\n";

        prompt << "Debes generar EXACTAMENTE 3 sugerencias.\n";
        prompt << "Cada sugerencia debe incluir:\n";
        prompt << "1) Una explicacion clara (2–3 oraciones) de que problema o mejora ves.\n";
        prompt << "2) Un pequeno fragmento de codigo C++ de 1 a 5 lineas que ilustre la idea.\n";
        prompt << "   El fragmento debe ser SOLO un ejemplo parcial, no la solucion completa.\n";
        prompt << "   Puedes usar '...' para omitir partes.\n";
        prompt << "3) Relacionar la sugerencia con eficiencia, estructuras de datos o correccion.\n";
        prompt << "No des nunca una solucion completa al problema.\n\n";

        // Indicamos el nuevo formato de salida: arreglo JSON de objetos
        prompt << "FORMATO DE RESPUESTA (MUY IMPORTANTE):\n";
        prompt << "Responde UNICAMENTE con un arreglo JSON de 3 objetos.\n";
        prompt << "Cada objeto debe tener exactamente estos campos:\n";
        prompt << "- \"explicacion\": string con 2–3 oraciones explicando el problema o mejora.\n";
        prompt << "- \"codigo\": string con un pequeno fragmento de C++ (1–5 lineas) que\n";
        prompt << "  ilustre la idea. No debe ser la solucion completa.\n";
        prompt << "Ejemplo de forma (no de contenido real):\n";
        prompt << "[\n";
        prompt << "  {\"explicacion\": \"Tu bucle revisa elementos repetidos...\",\n";
        prompt << "   \"codigo\": \"for (int i = 0; i < n; ++i) { ... }\"},\n";
        prompt << "  {\"explicacion\": \"Falta controlar el caso de lista vacia...\",\n";
        prompt << "   \"codigo\": \"if (vec.empty()) { return 0; }\"},\n";
        prompt << "  {\"explicacion\": \"Podrias usar unordered_map para acceso O(1)...\",\n";
        prompt << "   \"codigo\": \"unordered_map<int,int> freq;\"}\n";
        prompt << "]\n";
        prompt << "No agregues texto antes ni despues del arreglo JSON.\n";

        // ============================================================
        // 2) CREACIÓN DEL PAYLOAD JSON PARA LA API DE GEMINI
        // ============================================================
        json payload = {
            {"contents", json::array({
                {
                    {"parts", json::array({
                        {{"text", prompt.str()}}
                    })}
                }
            })},
            {"generationConfig", {
                // Puedes ajustar estos parámetros si quieres que sea más/menos creativo.
                {"temperature", 0.7},
                {"topK", 40},
                {"topP", 0.95},
                {"maxOutputTokens", 1024}
            }}
        };

        string payloadStr = payload.dump();

        // Configurar CURL: URL, headers y body JSON.
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

        // Ejecutar request HTTP.
        CURLcode res = curl_easy_perform(curl);

        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            suggestions.push_back("Network error: " + string(curl_easy_strerror(res)));
            curl_easy_cleanup(curl);
            return suggestions;
        }

        // ============================================================
        // 3) PARSEO DE LA RESPUESTA DE GEMINI
        //    Esperamos un arreglo JSON de objetos:
        //    [
        //      { "explicacion": "...", "codigo": "..." },
        //      ...
        //    ]
        //    Para no cambiar la interfaz pública, combinamos
        //    explicacion + codigo en un único string por sugerencia.
        // ============================================================
        try {
            json responseJson = json::parse(response);

            if (responseJson.contains("candidates") &&
                !responseJson["candidates"].empty() &&
                responseJson["candidates"][0].contains("content")) {

                // Texto bruto generado por el modelo.
                string text = responseJson["candidates"][0]["content"]["parts"][0]["text"];

                try {
                    // 3.1 Buscar el arreglo JSON dentro del texto (por si el modelo añade basura alrededor).
                    size_t start = text.find('[');
                    size_t end   = text.rfind(']');

                    if (start != string::npos && end != string::npos && end > start) {
                        string jsonArrayStr = text.substr(start, end - start + 1);

                        // 3.2 Parsear como arreglo de objetos {explicacion, codigo}.
                        json suggestionsJson = json::parse(jsonArrayStr);

                        for (const auto& obj : suggestionsJson) {
                            if (!obj.is_object()) continue;

                            string explicacion;
                            string codigo;

                            // Extraer campo "explicacion" si existe y es string.
                            if (obj.contains("explicacion") && obj["explicacion"].is_string()) {
                                explicacion = obj["explicacion"].get<string>();
                            }

                            // Extraer campo "codigo" si existe y es string.
                            if (obj.contains("codigo") && obj["codigo"].is_string()) {
                                codigo = obj["codigo"].get<string>();
                            }

                            // Solo agregamos sugerencias que tengan al menos algo de contenido.
                            if (!explicacion.empty() || !codigo.empty()) {
                                // Formato combinado para que la UI lo pinte en un solo bloque de texto.
                                string combined = "Explicacion:\n" + explicacion;
                                if (!codigo.empty()) {
                                    combined += "\n\nEjemplo de codigo C++:\n" + codigo;
                                }
                                suggestions.push_back(combined);
                            }
                        }
                    } else {
                        // 3.3 Fallback: no se encontró un arreglo JSON; tratamos de rescatar algo legible.
                        stringstream ss(text);
                        string line;
                        while (getline(ss, line)) {
                            if (!line.empty() && line.length() > 5) {
                                // Limpiar numeraciones (1., 2.-, etc.) y guiones.
                                size_t pos = line.find_first_not_of("0123456789.-* \t");
                                if (pos != string::npos) {
                                    suggestions.push_back(line.substr(pos));
                                }
                            }
                        }
                    }
                } catch (...) {
                    // Si falla cualquier paso del parsing del arreglo de objetos,
                    // dejamos al menos el texto completo como una sola sugerencia.
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

    // Si por alguna razón no se obtuvieron sugerencias, se devuelve una genérica.
    if (suggestions.empty()) {
        suggestions.push_back(
            "Considerar revisar la complejidad del algoritmo, los casos borde y las estructuras de datos utilizadas."
        );
    }

    return suggestions;
}
