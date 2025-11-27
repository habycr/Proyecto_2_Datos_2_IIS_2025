#pragma once

#include <filesystem>
#include <string>

namespace engine {

    struct CompileResult {
        int exitCode{0};
        std::string logFilePath;  // ruta a compile.log dentro del host
    };

    struct RunResult {
        int exitCode{0};
        bool timedOut{false};
        std::string runtimeLogPath; // runtime_<id>.log
        std::string outputPath;     // output_<id>.txt
    };

    // Límites de ejecución dentro del contenedor Docker
    struct RunLimits {
        int timeLimitSeconds{2};  // tiempo máximo por test en segundos
        int memoryLimitMb{256};   // memoria máxima en MB
        double cpuLimit{1.0};     // número de CPUs (ej. 1.0 = 1 CPU)
        int pidsLimit{64};        // límite de procesos (anti fork-bomb)
    };

    class DockerRunner {
    public:
        explicit DockerRunner(std::string imageName);

        CompileResult compile(
            const std::filesystem::path& submissionDir,
            const std::string& sourceFileName) const;

        RunResult runSingleTest(
            const std::filesystem::path& submissionDir,
            const std::string& inputFileName,   // "input_1.txt"
            const std::string& outputFileName,  // "output_1.txt"
            const std::string& runtimeLogName,  // "runtime_1.log"
            int timeLimitSeconds,
            const RunLimits& limits = RunLimits{}) const;

    private:
        std::string imageName_;

        std::string buildVolumeArgument(
            const std::filesystem::path& submissionDir) const;
    };

} // namespace engine
