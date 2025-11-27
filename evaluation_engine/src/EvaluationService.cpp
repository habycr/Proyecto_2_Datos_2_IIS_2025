#include "EvaluationService.h"

#include "SubmissionFilesystem.h"
#include "DockerRunner.h"
#include "OutputComparer.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>   // solo para logs en demo
#include <cstdint>
#include <system_error>

namespace engine {

EvaluationService::EvaluationService(std::filesystem::path baseDir,
                                     std::string dockerImage)
    : baseDir_(std::move(baseDir)),
      dockerImage_(std::move(dockerImage))
{}

EvaluationResult EvaluationService::evaluate(const SubmissionRequest& request)
{
    EvaluationResult result;
    result.submissionId = request.submissionId;

    try {
        // 1. Crear directorio y archivos
        auto submissionDir =
            SubmissionFilesystem::createSubmissionDir(baseDir_, request.submissionId);

        SubmissionFilesystem::writeSourceFile(
            submissionDir, "main.cpp", request.sourceCode);

        SubmissionFilesystem::writeTestFiles(
            submissionDir, request.testCases);

        // 2. Compilar
        DockerRunner runner(dockerImage_);

        auto comp = runner.compile(submissionDir, "main.cpp");
        std::ifstream compLog(comp.logFilePath);
        if (compLog) {
            result.compileLog.assign(
                (std::istreambuf_iterator<char>(compLog)),
                std::istreambuf_iterator<char>());
        }

        if (comp.exitCode != 0) {
            result.overallStatus = OverallStatus::CompilationError;
            return result;
        }

        // 3. Ejecutar tests uno por uno
        int maxTimeMs = 0;

        for (const auto& tc : request.testCases) {
            TestResult tr;
            tr.testId = tc.id;

            std::string inputFile   = "input_"   + tc.id + ".txt";
            std::string outputFile  = "output_"  + tc.id + ".txt";
            std::string runtimeFile = "runtime_" + tc.id + ".log";

            // Preparar límites de ejecución para este test
            RunLimits limits;
            // Al menos 1 segundo, aunque el request tenga menos de 1000 ms
            limits.timeLimitSeconds = std::max(1, request.timeLimitMs / 1000);

            // memoryLimitKb -> MB (con mínimo razonable de 16 MB)
            if (request.memoryLimitKb > 0) {
                limits.memoryLimitMb = std::max(16, request.memoryLimitKb / 1024);
            } else {
                limits.memoryLimitMb = 256; // valor por defecto
            }

            limits.cpuLimit  = 1.0; // 1 CPU lógico
            limits.pidsLimit = 64;  // anti fork-bomb

            auto start = std::chrono::steady_clock::now();

            auto runRes = runner.runSingleTest(
                submissionDir,
                inputFile,
                outputFile,
                runtimeFile,
                limits.timeLimitSeconds,
                limits);

            auto end = std::chrono::steady_clock::now();
            tr.timeMs = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

            if (tr.timeMs > maxTimeMs) {
                maxTimeMs = tr.timeMs;
            }

            // Leer runtime log (por si hubo errores)
            std::ifstream rt(runRes.runtimeLogPath);
            if (rt) {
                tr.runtimeLog.assign(
                    (std::istreambuf_iterator<char>(rt)),
                    std::istreambuf_iterator<char>());
            }

            // Clasificación del resultado
            if (runRes.timedOut) {
                tr.status = TestStatus::TimeLimitExceeded;
            } else if (runRes.exitCode != 0) {
                tr.status = TestStatus::RuntimeError;
            } else {
                // 3.1. Límite de salida: p.ej. 1 MB
                constexpr std::uintmax_t MAX_OUTPUT_BYTES = 1 * 1024 * 1024;

                std::error_code ecSize;
                auto outputPath = submissionDir / outputFile;
                auto outSize = std::filesystem::file_size(outputPath, ecSize);

                if (!ecSize && outSize > MAX_OUTPUT_BYTES) {
                    tr.status = TestStatus::RuntimeError;
                    tr.runtimeLog +=
                        "\n[Output limit exceeded: " + std::to_string(outSize) + " bytes]\n";
                } else {
                    // Si no hay expected_output, es ejecución simple (caso /run)
                    if (tc.expectedOutput.empty()) {
                        tr.status = TestStatus::Accepted;
                    } else {
                        // Comparar salida con expected_output (caso /submissions normal)
                        bool ok = OutputComparer::areEqual(
                            submissionDir / outputFile,
                            submissionDir / ("expected_" + tc.id + ".txt"));

                        tr.status = ok ? TestStatus::Accepted : TestStatus::WrongAnswer;
                    }
                }
            }

            result.tests.push_back(std::move(tr));
        }

        result.maxTimeMs = maxTimeMs;

        // 4. Determinar overallStatus
        bool allAccepted = true;
        bool anyAccepted = false;

        for (const auto& t : result.tests) {
            if (t.status != TestStatus::Accepted) {
                allAccepted = false;
            } else {
                anyAccepted = true;
            }
        }

        if (allAccepted) {
            result.overallStatus = OverallStatus::Accepted;
        } else if (anyAccepted) {
            result.overallStatus = OverallStatus::PartialAccepted;
        } else {
            result.overallStatus = OverallStatus::PartialAccepted; // o InternalError si quieres ser más estricto
        }

    } catch (const std::exception& ex) {
        result.overallStatus = OverallStatus::InternalError;
        result.compileLog += "\n[INTERNAL ERROR] ";
        result.compileLog += ex.what();
    }

    return result;
}

} // namespace engine
