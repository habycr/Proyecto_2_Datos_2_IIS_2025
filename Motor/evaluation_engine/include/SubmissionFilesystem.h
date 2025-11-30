#pragma once

#include "Models.h"
#include <filesystem>

namespace engine {

    // ============================================================================
    // SubmissionFilesystem
    //
    // Se encarga de manejar el sistema de archivos de una submission:
    //  - crear directorio único
    //  - escribir archivo fuente del usuario
    //  - escribir archivos input_*.txt y expected_*.txt
    //
    // Es la parte que prepara tod0 antes de llamar al DockerRunner.
    // ============================================================================
    class SubmissionFilesystem {
    public:

        // Crea la carpeta:
        //   baseDir/submissionId/
        static std::filesystem::path createSubmissionDir(
            const std::filesystem::path& baseDir,
            const std::string& submissionId);

        // Escribe el archivo fuente (ej: main.cpp)
        static void writeSourceFile(
            const std::filesystem::path& submissionDir,
            const std::string& sourceFileName,
            const std::string& sourceCode);

        // Escribe input_#.txt y expected_#.txt por cada TestCase.
        static void writeTestFiles(
            const std::filesystem::path& submissionDir,
            const std::vector<TestCase>& testCases);
    };

} // namespace engine
