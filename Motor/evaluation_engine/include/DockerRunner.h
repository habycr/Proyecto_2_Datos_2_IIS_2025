#pragma once

#include <filesystem>
#include <string>

namespace engine {

    // Resultado de la compilación dentro del contenedor Docker.
    // Se almacena:
    // - exitCode: código devuelto por el compilador (0 = éxito)
    // - logFilePath: ruta local (host) al archivo compile.log generado
    struct CompileResult {
        int exitCode{0};
        std::string logFilePath;
    };

    // Resultado de la ejecución de un solo test.
    // - exitCode: código de retorno del programa
    // - timedOut: true si excedió el límite de tiempo
    // - runtimeLogPath: ruta al log generado (stderr / info)
    // - outputPath: salida real generada por el programa para el test
    struct RunResult {
        int exitCode{0};
        bool timedOut{false};
        std::string runtimeLogPath;
        std::string outputPath;
    };

    // Límites de seguridad/recursos para la ejecución dentro de Docker.
    struct RunLimits {
        int timeLimitSeconds{2};   // límite de tiempo por test
        int memoryLimitMb{256};    // límite de memoria
        double cpuLimit{1.0};      // CPUs asignadas (1.0 = una CPU completa)
        int pidsLimit{64};         // límite de procesos (evita fork-bombs)
    };

    // ============================================================================
    // DockerRunner
    //
    // Clase encargada de:
    //  - ejecutar compilación dentro del contenedor
    //  - ejecutar un test individual con límites
    //  - montar volúmenes para compartir archivos host <-> contenedor
    // ============================================================================
    class DockerRunner {
    public:
        explicit DockerRunner(std::string imageName);

        // Compila el archivo fuente dentro del contenedor Docker.
        // submissionDir: carpeta donde está submission.cpp
        // sourceFileName: nombre del archivo del usuario (ej: "solution.cpp")
        CompileResult compile(
            const std::filesystem::path& submissionDir,
            const std::string& sourceFileName) const;

        // Ejecuta un test:
        // - inputFileName: input_1.txt
        // - outputFileName: output_1.txt (salida generada)
        // - runtimeLogName: stderr/log
        // - timeLimitSeconds: límite individual para el test
        RunResult runSingleTest(
            const std::filesystem::path& submissionDir,
            const std::string& inputFileName,
            const std::string& outputFileName,
            const std::string& runtimeLogName,
            int timeLimitSeconds,
            const RunLimits& limits = RunLimits{}) const;

    private:
        std::string imageName_;  // nombre de la imagen Docker usada

        // Construye el argumento `-v /host:/container` para montar el volumen.
        std::string buildVolumeArgument(
            const std::filesystem::path& submissionDir) const;
    };

} // namespace engine
