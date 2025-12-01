#pragma once

#include <string>
#include <vector>

namespace engine {

    // Un test recibido desde el GestorREST.
    struct TestCase {
        std::string id;             // id lógico del test ("1", "2"...)
        std::string input;          // input completo para stdin
        std::string expectedOutput; // output esperado
    };

    // Estados de un test tras la ejecución del motor.
    enum class TestStatus {
        Accepted,
        WrongAnswer,
        TimeLimitExceeded,
        RuntimeError,
        InternalError
    };

    // Resultado detallado de un único test.
    struct TestResult {
        std::string testId;
        TestStatus status{TestStatus::InternalError};
        int timeMs{0};       // tiempo medido
        int memoryKb{0};     // memoria máxima utilizada
        std::string runtimeLog; // stderr o info adicional
    };

    // Estado global de una submission.
    enum class OverallStatus {
        Accepted,          // todos los tests ok
        CompilationError,  // falló la compilación
        PartialAccepted,   // algunos tests fallaron
        InternalError
    };

    // Request enviado por el GestorREST al motor.
    struct SubmissionRequest {
        std::string submissionId;
        std::string problemId;
        std::string language;       // "cpp"
        std::string sourceCode;
        int timeLimitMs{2000};
        int memoryLimitKb{262144};  // 256 MB
        std::vector<TestCase> testCases;
    };

    // Respuesta final del motor, enviada a la UI.
    struct EvaluationResult {
        std::string submissionId;
        OverallStatus overallStatus{OverallStatus::InternalError};
        std::string compileLog;
        std::vector<TestResult> tests;
        int maxTimeMs{0};    // máximo entre todos los tests
        int maxMemoryKb{0};  // máximo entre todos los tests
    };

} // namespace engine
