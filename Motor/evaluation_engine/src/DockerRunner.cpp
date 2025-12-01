#include "DockerRunner.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace engine {

// ============================================================================
// Constructor: simplemente almacena el nombre de la imagen Docker a usar.
// ============================================================================
DockerRunner::DockerRunner(std::string imageName)
    : imageName_(std::move(imageName))
{}

// ============================================================================
// buildVolumeArgument
// Construye el argumento de volumen para Docker:
//   -v "<ruta absoluta host>:/workspace"
// Permite que el contenedor lea/escriba en la carpeta de la submission.
// ============================================================================
std::string DockerRunner::buildVolumeArgument(
    const std::filesystem::path& submissionDir) const
{
    // Convertir a ruta absoluta
    std::filesystem::path abs = std::filesystem::absolute(submissionDir);
    std::string hostPath = abs.string();

    // Reemplazar backslashes por slashes (Windows compatibilidad)
    std::replace(hostPath.begin(), hostPath.end(), '\\', '/');

    std::ostringstream oss;
    oss << "-v \"" << hostPath << ":/workspace\" ";
    return oss.str();
}

// ============================================================================
// compile
// Ejecuta dentro del contenedor Docker:
//   g++ main.cpp -O2 -std=c++20 -o main
// El stderr se redirige a compile.log dentro del host.
// ============================================================================
CompileResult DockerRunner::compile(
    const std::filesystem::path& submissionDir,
    const std::string& sourceFileName) const
{
    CompileResult result;
    std::string volumeArg = buildVolumeArgument(submissionDir);

    std::ostringstream cmd;
    cmd << "docker run --rm "
        << "--network=none "          // sin acceso a red
        << "--memory=512m "           // límite de memoria
        << "--cpus=1 "                // una CPU
        << volumeArg
        << imageName_ << " "
        << "/bin/bash -lc \"cd /workspace && "
        << "g++ " << sourceFileName
        << " -O2 -std=c++20 -o main 2> compile.log\"";

    // Ruta al log dentro del host
    result.logFilePath = (submissionDir / "compile.log").string();

    // Ejecutar comando
    int exitCode = std::system(cmd.str().c_str());
    result.exitCode = exitCode;

    return result;
}

// ============================================================================
// runSingleTest
// Ejecuta un test dentro de Docker con:
//   - timeout
//   - /usr/bin/time -v (para obtener memoria)
//   - límites (memoria, CPU, pids)
//
// El input se redirige "< input_#.txt"
// El output se escribe en "output_#.txt"
// El stderr va a runtime_#.log
// ============================================================================
RunResult DockerRunner::runSingleTest(
    const std::filesystem::path& submissionDir,
    const std::string& inputFileName,
    const std::string& outputFileName,
    const std::string& runtimeLogName,
    int timeLimitSeconds,
    const RunLimits& limits) const
{
    RunResult result;
    std::string volumeArg = buildVolumeArgument(submissionDir);

    std::ostringstream cmd;
    cmd << "docker run --rm "
        << "--network=none "
        << "--memory=" << limits.memoryLimitMb << "m "
        << "--cpus=" << limits.cpuLimit << " "
        << "--pids-limit=" << limits.pidsLimit << " "
        << volumeArg
        << imageName_ << " "
        << "/bin/bash -lc \"cd /workspace && "
        << "timeout " << timeLimitSeconds << "s "
        << "/usr/bin/time -v ./main < " << inputFileName
        << " > " << outputFileName
        << " 2> " << runtimeLogName << "\"";

    // Rutas finales de salida y log
    result.outputPath     = (submissionDir / outputFileName).string();
    result.runtimeLogPath = (submissionDir / runtimeLogName).string();

    int exitCode = std::system(cmd.str().c_str());
    result.exitCode = exitCode;

    // timeout devuelve 124 → time limit exceeded
    if (exitCode == 124) {
        result.timedOut = true;
    }

    return result;
}

} // namespace engine
