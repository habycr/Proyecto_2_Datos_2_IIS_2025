#include "SubmissionFilesystem.h"

#include <fstream>
#include <stdexcept>

namespace engine {

    std::filesystem::path SubmissionFilesystem::createSubmissionDir(
        const std::filesystem::path& baseDir,
        const std::string& submissionId)
    {
        std::filesystem::path dir = baseDir / submissionId;
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            throw std::runtime_error(
                "No se pudo crear el directorio de la submission: " +
                dir.string() + " - " + ec.message());
        }
        return dir;
    }

    void SubmissionFilesystem::writeSourceFile(
        const std::filesystem::path& submissionDir,
        const std::string& sourceFileName,
        const std::string& sourceCode)
    {
        std::filesystem::path filePath = submissionDir / sourceFileName;
        std::ofstream out(filePath);
        if (!out) {
            throw std::runtime_error(
                "No se pudo abrir el archivo de código fuente: " + filePath.string());
        }
        out << sourceCode;
    }

    void SubmissionFilesystem::writeTestFiles(
        const std::filesystem::path& submissionDir,
        const std::vector<TestCase>& testCases)
    {
        for (const auto& tc : testCases) {
            std::filesystem::path inputPath    = submissionDir / ("input_" + tc.id + ".txt");
            std::filesystem::path expectedPath = submissionDir / ("expected_" + tc.id + ".txt");

            {
                std::ofstream in(inputPath);
                if (!in) {
                    throw std::runtime_error(
                        "No se pudo crear archivo de input: " + inputPath.string());
                }
                in << tc.input;
            }

            {
                std::ofstream exp(expectedPath);
                if (!exp) {
                    throw std::runtime_error(
                        "No se pudo crear archivo expected: " + expectedPath.string());
                }
                exp << tc.expectedOutput;
            }
        }
    }

} // namespace engine
