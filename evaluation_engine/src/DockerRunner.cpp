#include "DockerRunner.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace engine {

DockerRunner::DockerRunner(std::string imageName)
    : imageName_(std::move(imageName))
{}

std::string DockerRunner::buildVolumeArgument(
    const std::filesystem::path& submissionDir) const
{
    // Convertir a ruta absoluta y normalizar separadores para Docker
    std::filesystem::path abs = std::filesystem::absolute(submissionDir);
    std::string hostPath = abs.string();
    std::replace(hostPath.begin(), hostPath.end(), '\\', '/');

    std::ostringstream oss;
    oss << "-v \"" << hostPath << ":/workspace\" ";
    return oss.str();
}

CompileResult DockerRunner::compile(
    const std::filesystem::path& submissionDir,
    const std::string& sourceFileName) const
{
    CompileResult result;
    std::string volumeArg = buildVolumeArgument(submissionDir);

    // compile.log quedará en el host: <submissionDir>/compile.log
    std::ostringstream cmd;
    cmd << "docker run --rm "
        << "--network=none "
        << "--memory=512m "
        << "--cpus=1 "
        << volumeArg
        << imageName_ << " "
        << "/bin/bash -lc \"cd /workspace && "
        << "g++ " << sourceFileName
        << " -O2 -std=c++20 -o main 2> compile.log\"";

    result.logFilePath = (submissionDir / "compile.log").string();

    int exitCode = std::system(cmd.str().c_str());
    result.exitCode = exitCode;

    return result;
}

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
        << "./main < " << inputFileName
        << " > " << outputFileName
        << " 2> " << runtimeLogName << "\"";

    result.outputPath      = (submissionDir / outputFileName).string();
    result.runtimeLogPath  = (submissionDir / runtimeLogName).string();

    int exitCode = std::system(cmd.str().c_str());
    result.exitCode = exitCode;

    // Heurística simple: timeout(1) devuelve 124 cuando expira
    if (exitCode == 124) {
        result.timedOut = true;
    }

    return result;
}

} // namespace engine
