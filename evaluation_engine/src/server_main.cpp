#include "EvaluationService.h"
#include "Models.h"

#include <crow.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;
using namespace engine;

int main() {
    crow::SimpleApp app;

    std::filesystem::path baseDir =
        std::filesystem::current_path() / "eval_workdir";

    EvaluationService service(baseDir, "codecoach-cpp:latest");

    CROW_ROUTE(app, "/evaluate").methods(crow::HTTPMethod::Post)
    ([&service](const crow::request& req){
        try {
            json body = json::parse(req.body);

            // Parsear SubmissionRequest desde JSON
            SubmissionRequest sr;
            sr.submissionId = body.at("submission_id").get<std::string>();
            sr.problemId    = body.at("problem_id").get<std::string>();
            sr.language     = body.at("language").get<std::string>();
            sr.sourceCode   = body.at("source_code").get<std::string>();
            sr.timeLimitMs  = body.value("time_limit_ms", 2000);

            for (auto& tc : body.at("test_cases")) {
                TestCase t;
                t.id = tc.at("id").get<std::string>();
                t.input = tc.at("input").get<std::string>();
                t.expectedOutput = tc.at("expected_output").get<std::string>();
                sr.testCases.push_back(t);
            }

            EvaluationResult er = service.evaluate(sr);

            // Convertir EvaluationResult → JSON
            json result;
            result["submission_id"] = er.submissionId;
            result["overall_status"] =
                (er.overallStatus == OverallStatus::Accepted) ? "Accepted" :
                (er.overallStatus == OverallStatus::CompilationError) ? "CompilationError" :
                (er.overallStatus == OverallStatus::PartialAccepted) ? "PartialAccepted" :
                "InternalError";

            result["compile_log"] = er.compileLog;
            result["max_time_ms"] = er.maxTimeMs;
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
