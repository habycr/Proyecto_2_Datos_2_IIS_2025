#include "Models.h"
#include "EvaluationService.h"

#include <filesystem>
#include <iostream>

using namespace engine;

int main() {
    try {
        std::cout << "== Demo motor de evaluacion (EvaluationService) ==\n";

        std::filesystem::path baseDir =
            std::filesystem::current_path() / "eval_workdir";

        std::cout << "Directorio base: " << baseDir.string() << "\n";

        // Construimos una SubmissionRequest de ejemplo
        SubmissionRequest req;
        req.submissionId = "sub-demo-001";
        req.problemId    = "sum-two-ints";
        req.language     = "cpp";
        req.timeLimitMs  = 2000;
        req.memoryLimitKb = 262144;

        // Código del usuario
        req.sourceCode =
            "#include <bits/stdc++.h>\n"
            "int main(){\n"
            "    std::ios::sync_with_stdio(false);\n"
            "    std::cin.tie(nullptr);\n"
            "    long long a,b;\n"
            "    if(!(std::cin >> a >> b)) return 0;\n"
            "    std::cout << (a + b) << '\\n';\n"
            "    return 0;\n"
            "}\n";

        // Test 1: 3 + 5 = 8
        TestCase tc1;
        tc1.id = "1";
        tc1.input = "3 5\n";
        tc1.expectedOutput = "8\n";

        // Test 2: -2 + 10 = 8
        TestCase tc2;
        tc2.id = "2";
        tc2.input = "-2 10\n";
        tc2.expectedOutput = "8\n";

        req.testCases = {tc1, tc2};

        EvaluationService service(baseDir, "codecoach-cpp:latest");

        std::cout << "Lanzando evaluacion...\n";

        EvaluationResult res = service.evaluate(req);

        std::cout << "== Resultado general ==\n";
        std::cout << "Submission: " << res.submissionId << "\n";
        std::cout << "Max time (ms): " << res.maxTimeMs << "\n";
        std::cout << "Estado global: ";

        switch (res.overallStatus) {
            case OverallStatus::Accepted:           std::cout << "Accepted\n"; break;
            case OverallStatus::CompilationError:   std::cout << "CompilationError\n"; break;
            case OverallStatus::PartialAccepted:    std::cout << "PartialAccepted\n"; break;
            case OverallStatus::InternalError:      std::cout << "InternalError\n"; break;
        }

        if (!res.compileLog.empty()) {
            std::cout << "== compile.log ==\n" << res.compileLog << "\n";
        }

        std::cout << "== Detalle por test ==\n";
        for (const auto& t : res.tests) {
            std::cout << "Test " << t.testId << " -> ";
            switch (t.status) {
                case TestStatus::Accepted:           std::cout << "Accepted"; break;
                case TestStatus::WrongAnswer:        std::cout << "WrongAnswer"; break;
                case TestStatus::TimeLimitExceeded:  std::cout << "TimeLimitExceeded"; break;
                case TestStatus::RuntimeError:       std::cout << "RuntimeError"; break;
                case TestStatus::InternalError:      std::cout << "InternalError"; break;
            }
            std::cout << " (" << t.timeMs << " ms)\n";

            if (!t.runtimeLog.empty()) {
                std::cout << "  runtime.log:\n" << t.runtimeLog << "\n";
            }
        }

    } catch (const std::exception& ex) {
        std::cerr << "Excepcion en demo: " << ex.what() << "\n";
    }

    return 0;
}
