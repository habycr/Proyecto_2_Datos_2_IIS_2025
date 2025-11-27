#pragma once

#include <string>
#include <vector>

namespace engine {

    struct TestCase {
        std::string id;
        std::string input;
        std::string expectedOutput;
    };

    enum class TestStatus {
        Accepted,
        WrongAnswer,
        TimeLimitExceeded,
        RuntimeError,
        InternalError
    };

    struct TestResult {
        std::string testId;
        TestStatus status{TestStatus::InternalError};
        int timeMs{0};
        std::string runtimeLog;
    };

    enum class OverallStatus {
        Accepted,
        CompilationError,
        PartialAccepted,
        InternalError
    };

    struct SubmissionRequest {
        std::string submissionId;
        std::string problemId;
        std::string language;   // "cpp" por ahora
        std::string sourceCode;
        int timeLimitMs{2000};
        int memoryLimitKb{262144};
        std::vector<TestCase> testCases;
    };

    struct EvaluationResult {
        std::string submissionId;
        OverallStatus overallStatus{OverallStatus::InternalError};
        std::string compileLog;
        std::vector<TestResult> tests;
        int maxTimeMs{0};
    };

} // namespace engine
