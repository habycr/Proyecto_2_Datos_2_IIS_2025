#include "SubmissionFilesystem.h"

#include <fstream>
#include <stdexcept>

namespace engine {

    // ============================================================================
    // Crea carpeta para la submission:
    //   baseDir / submissionId
    // ============================================================================
    std::filesystem::path SubmissionFilesystem::createSubmissionDir(
        const std::filesystem::path& baseDir,
        const std::string& submissionId)
    {
        std::filesystem::path dir = baseDir / submissionId;
        std::error_code ec;

        // create_directories: crea toda la cadena si no existe
        std::filesystem::create_directories(dir, ec);

        if (ec) {
            throw std::runtime_error(
                "No se pudo crear el directorio de la submission: " +
                dir.string() + " - " + ec.message());
        }
        return dir;
    }

    // ============================================================================
    // writeSourceFile
    // Crea el archivo main.cpp (o el que se pida) con el código fuente del usuario.
    // ============================================================================
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

    // ============================================================================
    // writeTestFiles
    // Escribe input_#.txt y expected_#.txt por cada test case.
    // ============================================================================
    void SubmissionFilesystem::writeTestFiles(
        const std::filesystem::path& submissionDir,
        const std::vector<TestCase>& testCases)
    {
        for (const auto& tc : testCases) {
            std::filesystem::path inputPath    = submissionDir / ("input_" + tc.id + ".txt");
            std::filesystem::path expectedPath = submissionDir / ("expected_" + tc.id + ".txt");

            // input_#.txt
            {
                std::ofstream in(inputPath);
                if (!in) {
                    throw std::runtime_error(
                        "No se pudo crear archivo de input: " + inputPath.string());
                }
                in << tc.input;
            }

            // expected_#.txt
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
