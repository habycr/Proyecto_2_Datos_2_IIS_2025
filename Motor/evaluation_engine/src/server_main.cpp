#include "EvaluationService.h"
#include "Models.h"

#include <crow.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;
using namespace engine;

// ============================================================================
// Servidor REST del motor de evaluación
//
// Expone POST /evaluate
// y delega tod0 el procesamiento en EvaluationService
// ============================================================================
int main() {
    crow::SimpleApp app;

    // Carpeta donde se crearán las submissions
    std::filesystem::path baseDir =
        std::filesystem::current_path() / "eval_workdir";

    // Servicio principal del motor
    EvaluationService service(baseDir, "codecoach-cpp:latest");

    // ------------------------------------------------------------------------
    // POST /evaluate
    //
    // Recibe:
    // {
    //   "submission_id": "...",
    //   "problem_id": "...",
    //   "language": "cpp",
    //   "source_code": "...",
    //   "time_limit_ms": 2000,
    //   "test_cases": [...]
    // }
    //
    // Y devuelve el JSON con los resultados detallados.
    // ------------------------------------------------------------------------
    CROW_ROUTE(app, "/evaluate").methods(crow::HTTPMethod::Post)
    ([&service](const crow::request& req){
        try {
            json body = json::parse(req.body);

            // Parsear SubmissionRequest
            SubmissionRequest sr;
            sr.submissionId = body.at("submission_id").get<std::string>();
            sr.problemId    = body.at("problem_id").get<std::string>();
            sr.language     = body.at("language").get<std::string>();
            sr.sourceCode   = body.at("source_code").get<std::string>();
            sr.timeLimitMs  = body.value("time_limit_ms", 2000);

            // test_cases (lista)
            for (auto& tc : body.at("test_cases")) {
                TestCase t;
                t.id = tc.at("id").get<std::string>();
                t.input = tc.at("input").get<std::string>();
                t.expectedOutput = tc.at("expected_output").get<std::string>();
                sr.testCases.push_back(t);
            }

            // Ejecutar evaluación
            EvaluationResult er = service.evaluate(sr);

            // Convertir a JSON
            json result;
            result["submission_id"] = er.submissionId;
            result["overall_status"] =
                (er.overallStatus == OverallStatus::Accepted) ? "Accepted" :
                (er.overallStatus == OverallStatus::CompilationError) ? "CompilationError" :
                (er.overallStatus == OverallStatus::PartialAccepted) ? "PartialAccepted" :
                "InternalError";

            result["compile_log"]  = er.compileLog;
            result["max_time_ms"]  = er.maxTimeMs;
            result["max_memory_kb"] = er.maxMemoryKb;

            json testArray = json::array();

            for (auto& t : er.tests){
                json jt;
                jt["id"] = t.testId;
                jt["time_ms"] = t.timeMs;
                jt["memory_kb"] = t.memoryKb;

                jt["status"] =
                    (t.status == TestStatus::Accepted) ? "Accepted" :
                    (t.status == TestStatus::WrongAnswer) ? "WrongAnswer" :
                    (t.status == TestStatus::RuntimeError) ? "RuntimeError" :
                    (t.status == TestStatus::TimeLimitExceeded) ? "TimeLimitExceeded" :
                    "InternalError";

                jt["runtime_log"] = t.runtimeLog;
                testArray.push_back(jt);
            }

            result["tests"] = testArray;

            return crow::response(200, result.dump());

        } catch (const std::exception& ex) {
            return crow::response(500, std::string("Error: ") + ex.what());
        }
    });

    std::cout << "Evaluation Engine escuchando en http://localhost:8090 ...\n";
    app.port(8090).multithreaded().run();
}
