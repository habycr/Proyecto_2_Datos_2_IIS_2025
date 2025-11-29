#pragma once

#include "Models.h"
#include <filesystem>

namespace engine {

    class SubmissionFilesystem {
    public:
        static std::filesystem::path createSubmissionDir(
            const std::filesystem::path& baseDir,
            const std::string& submissionId);

        static void writeSourceFile(
            const std::filesystem::path& submissionDir,
            const std::string& sourceFileName,
            const std::string& sourceCode);

        static void writeTestFiles(
            const std::filesystem::path& submissionDir,
            const std::vector<TestCase>& testCases);
    };

} // namespace engine
