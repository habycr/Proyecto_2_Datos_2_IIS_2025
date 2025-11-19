// Archivo: Gestor/src/GestorREST.cpp
// Servidor REST del Gestor de Problemas (CodeCoach)

#include "ProblemRepository.h"

#include <crow.h>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include <algorithm>
#include <optional>
#include <random>
#include <string>
#include <vector>

// =================== Helpers JSON ===================

// Convierte un Problem a JSON (Crow). Si summary = true, omite description, code_stub y test_cases.
static crow::json::wvalue problem_to_json(const Problem& p, bool summary = false) {
    crow::json::wvalue json;

    json["problem_id"] = p.problem_id;
    json["title"]      = p.title;
    json["difficulty"] = p.difficulty;

    if (!summary) {
        json["description"] = p.description;
        json["code_stub"]   = p.code_stub;

        // tags
        for (std::size_t i = 0; i < p.tags.size(); ++i) {
            json["tags"][i] = p.tags[i];
        }

        // test_cases
        for (std::size_t i = 0; i < p.test_cases.size(); ++i) {
            json["test_cases"][i]["input"]           = p.test_cases[i].input;
            json["test_cases"][i]["expected_output"] = p.test_cases[i].expected_output;
        }
    } else {
        // En modo resumen, igual devolvemos tags porque son útiles para filtrar en UI
        for (std::size_t i = 0; i < p.tags.size(); ++i) {
            json["tags"][i] = p.tags[i];
        }
    }

    return json;
}

// Convierte un JSON (Crow) a Problem.
// - require_id = true en POST: el body debe traer problem_id.
// - require_id = false en PUT: el id viene en la ruta, no en el JSON.
static std::optional<Problem> json_to_problem(const crow::json::rvalue& body,
                                              bool require_id,
                                              std::string& error_out) {
    Problem p;

    using crow::json::type;

    // Helper para validar un campo string
    auto get_string = [&](const char* name, bool required, std::string& dst) -> bool {
        if (!body.has(name)) {
            if (required) {
                error_out = std::string("Falta el campo obligatorio '") + name + "'";
                return false;
            }
            return true; // no requerido
        }
        if (body[name].t() != type::String) {
            error_out = std::string("El campo '") + name + "' debe ser string";
            return false;
        }
        dst = std::string(body[name].s());
        return true;
    };

    if (require_id) {
        if (!get_string("problem_id", true, p.problem_id)) return std::nullopt;
    }

    if (!get_string("title",       true, p.title))       return std::nullopt;
    if (!get_string("description", true, p.description)) return std::nullopt;
    if (!get_string("difficulty",  true, p.difficulty))  return std::nullopt;
    if (!get_string("code_stub",   true, p.code_stub))   return std::nullopt;

    // tags (lista de strings)
    if (!body.has("tags") || body["tags"].t() != type::List) {
        error_out = "El campo 'tags' es obligatorio y debe ser una lista";
        return std::nullopt;
    }
    for (std::size_t i = 0; i < body["tags"].size(); ++i) {
        if (body["tags"][i].t() != type::String) {
            error_out = "Todos los elementos de 'tags' deben ser strings";
            return std::nullopt;
        }
        p.tags.emplace_back(body["tags"][i].s());
    }

    // test_cases (lista de objetos { input, expected_output })
    if (!body.has("test_cases") || body["test_cases"].t() != type::List) {
        error_out = "El campo 'test_cases' es obligatorio y debe ser una lista";
        return std::nullopt;
    }

    for (std::size_t i = 0; i < body["test_cases"].size(); ++i) {
        const auto& tc_json = body["test_cases"][i];

        if (tc_json.t() != type::Object) {
            error_out = "Cada elemento de 'test_cases' debe ser un objeto";
            return std::nullopt;
        }

        if (!tc_json.has("input") ||
            tc_json["input"].t() != type::String ||
            !tc_json.has("expected_output") ||
            tc_json["expected_output"].t() != type::String) {
            error_out = "Cada test_case debe tener 'input' y 'expected_output' como strings";
            return std::nullopt;
        }

        TestCase tc;
        tc.input           = std::string(tc_json["input"].s());
        tc.expected_output = std::string(tc_json["expected_output"].s());
        p.test_cases.push_back(std::move(tc));
    }

    return p;
}

// Crea una respuesta HTTP con JSON y código de estado
static crow::response make_json_response(int status, const crow::json::wvalue& body) {
    crow::response res;
    res.code = status;
    res.set_header("Content-Type", "application/json");
    res.body = body.dump();
    return res;
}

// Crea una respuesta de error JSON simple
static crow::response make_error_response(int status, const std::string& message) {
    crow::json::wvalue body;
    body["error"] = message;
    return make_json_response(status, body);
}

// =================== main ===================

int main() {
    try {
        // 1. Inicializar MongoDB driver
        mongocxx::instance inst{}; // una sola vez por proceso

        // 2. Conectarse a MongoDB local
        mongocxx::client client{mongocxx::uri{"mongodb://localhost:27017"}};
        auto db = client["codecoach"];

        // 3. Crear el repositorio de problemas
        ProblemRepository repo{db};

        // 4. Inicializar la app Crow
        crow::SimpleApp app;

        // --------- Endpoint de salud ---------
        CROW_ROUTE(app, "/health")
        ([] {
            crow::json::wvalue body;
            body["status"] = "OK";
            return make_json_response(200, body);
        });

        // --------- GET /problems (lista resumida) ---------
        CROW_ROUTE(app, "/problems")
        ([&repo](const crow::request&) {
            auto problems = repo.get_all();

            crow::json::wvalue body;
            for (std::size_t i = 0; i < problems.size(); ++i) {
                body["problems"][i] = problem_to_json(problems[i], /*summary=*/true);
            }

            return make_json_response(200, body);
        });

        // --------- GET /problems/<id> (detalle completo) ---------
        CROW_ROUTE(app, "/problems/<string>")
        ([&repo](const crow::request&, const std::string& problem_id) {
            auto maybe = repo.get_by_id(problem_id);
            if (!maybe) {
                return make_error_response(404, "Problema no encontrado");
            }

            crow::json::wvalue body = problem_to_json(*maybe, /*summary=*/false);
            return make_json_response(200, body);
        });

        // --------- GET /problems/difficulty/<level> ---------
        CROW_ROUTE(app, "/problems/difficulty/<string>")
        ([&repo](const crow::request&, const std::string& level) {
            auto problems = repo.get_by_difficulty(level);

            crow::json::wvalue body;
            for (std::size_t i = 0; i < problems.size(); ++i) {
                body["problems"][i] = problem_to_json(problems[i], /*summary=*/true);
            }

            return make_json_response(200, body);
        });

        // --------- GET /problems/tags/<tag> (filtra en memoria) ---------
        CROW_ROUTE(app, "/problems/tags/<string>")
        ([&repo](const crow::request&, const std::string& tag) {
            auto all = repo.get_all();
            std::vector<Problem> filtered;

            for (const auto& p : all) {
                if (std::find(p.tags.begin(), p.tags.end(), tag) != p.tags.end()) {
                    filtered.push_back(p);
                }
            }

            crow::json::wvalue body;
            for (std::size_t i = 0; i < filtered.size(); ++i) {
                body["problems"][i] = problem_to_json(filtered[i], /*summary=*/true);
            }

            return make_json_response(200, body);
        });

        // --------- GET /problems/random ---------
        CROW_ROUTE(app, "/problems/random")
        ([&repo](const crow::request&) {
            auto all = repo.get_all();
            if (all.empty()) {
                return make_error_response(404, "No hay problemas en la base de datos");
            }

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<std::size_t> dist(0, all.size() - 1);

            const auto& chosen = all[dist(gen)];
            crow::json::wvalue body = problem_to_json(chosen, /*summary=*/false);
            return make_json_response(200, body);
        });

        // --------- POST /problems (crear) ---------
        CROW_ROUTE(app, "/problems").methods(crow::HTTPMethod::Post)
        ([&repo](const crow::request& req) {
            auto body_json = crow::json::load(req.body);
            if (!body_json) {
                return make_error_response(400, "JSON malformado");
            }

            std::string error;
            auto maybe_problem = json_to_problem(body_json, /*require_id=*/true, error);
            if (!maybe_problem) {
                return make_error_response(400, error);
            }

            Problem p = *maybe_problem;

            // Verificar si ya existe ese problem_id
            auto existing = repo.get_by_id(p.problem_id);
            if (existing) {
                return make_error_response(400, "Ya existe un problema con ese problem_id");
            }

            repo.insert_problem(p);

            crow::json::wvalue res_body;
            res_body["message"]    = "Problema creado correctamente";
            res_body["problem_id"] = p.problem_id;
            return make_json_response(201, res_body);
        });

        // --------- PUT /problems/<id> (actualizar) ---------
        CROW_ROUTE(app, "/problems/<string>").methods(crow::HTTPMethod::Put)
        ([&repo](const crow::request& req, const std::string& problem_id) {
            auto body_json = crow::json::load(req.body);
            if (!body_json) {
                return make_error_response(400, "JSON malformado");
            }

            std::string error;
            // No requerimos problem_id en el body; viene en la URL
            auto maybe_problem = json_to_problem(body_json, /*require_id=*/false, error);
            if (!maybe_problem) {
                return make_error_response(400, error);
            }

            Problem p = *maybe_problem;
            p.problem_id = problem_id; // clave lógica desde la URL

            // Intentamos actualizar
            bool updated = repo.update_problem(p);
            if (!updated) {
                return make_error_response(404, "No se encontró un problema con ese problem_id para actualizar");
            }

            crow::json::wvalue res_body;
            res_body["message"]    = "Problema actualizado correctamente";
            res_body["problem_id"] = p.problem_id;
            return make_json_response(200, res_body);
        });

        // --------- DELETE /problems/<id> (eliminar) ---------
        CROW_ROUTE(app, "/problems/<string>").methods(crow::HTTPMethod::Delete)
        ([&repo](const crow::request&, const std::string& problem_id) {
            bool deleted = repo.delete_problem(problem_id);
            if (!deleted) {
                return make_error_response(404, "No se encontró un problema con ese problem_id para eliminar");
            }

            crow::json::wvalue res_body;
            res_body["message"]    = "Problema eliminado correctamente";
            res_body["problem_id"] = problem_id;
            return make_json_response(200, res_body);
        });

        // 5. Levantar el servidor
        std::cout << "[GestorREST] Escuchando en http://localhost:8080 ..." << std::endl;
        app.port(8080).multithreaded().run();
    }
    catch (const std::exception& ex) {
        std::cerr << "[GestorREST] Excepción fatal: " << ex.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "[GestorREST] Excepción desconocida" << std::endl;
        return 1;
    }

    return 0;
}
