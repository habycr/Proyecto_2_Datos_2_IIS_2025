#include "EvaluationService.h"

#include "SubmissionFilesystem.h"
#include "DockerRunner.h"
#include "OutputComparer.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <system_error>

#include <sstream>

namespace {

    // ============================================================================
    // extractMaxMemoryKb
    // Extrae del runtime.log la línea con:
    //   "Maximum resident set size (kbytes): N"
    // Esto lo genera /usr/bin/time -v dentro del contenedor Docker.
    // ============================================================================
    int extractMaxMemoryKb(const std::string& logText) {
        std::istringstream iss(logText);
        std::string line;
        const std::string key = "Maximum resident set size (kbytes):";
        while (std::getline(iss, line)) {
            auto pos = line.find(key);
            if (pos != std::string::npos) {
                std::string valueStr = line.substr(pos + key.size());
                try {
                    return std::stoi(valueStr);
                } catch (...) {
                    return 0;
                }
            }
        }
        return 0;
    }

} // namespace


namespace engine {

// ============================================================================
// Constructor del servicio.
// baseDir: carpeta base donde se crearán carpetas por submission
// dockerImage: imagen Docker para compilar/ejecutar
// ============================================================================
EvaluationService::EvaluationService(std::filesystem::path baseDir,
                                     std::string dockerImage)
    : baseDir_(std::move(baseDir)),
      dockerImage_(std::move(dockerImage))
{}

// ============================================================================
// evaluate
// Orquesta todo el flujo:
//
// 1) Crear carpeta submission
// 2) Escribir archivo fuente y archivos input/expected
// 3) Compilar
// 4) Ejecutar test por test
// 5) Medir tiempo/memoria
// 6) Comparar salida con expected_output
// 7) Construir EvaluationResult final
// ============================================================================
EvaluationResult EvaluationService::evaluate(const SubmissionRequest& request)
{
    EvaluationResult result;
    result.submissionId = request.submissionId;

    try {
        // -------------------------
        // 1. Crear directorio
        // -------------------------
        auto submissionDir =
            SubmissionFilesystem::createSubmissionDir(baseDir_, request.submissionId);

        // 2. Escribir código fuente
        SubmissionFilesystem::writeSourceFile(
            submissionDir, "main.cpp", request.sourceCode);

        // 3. Escribir todos los test cases
        SubmissionFilesystem::writeTestFiles(
            submissionDir, request.testCases);

        // -------------------------
        // 2. Compilar
        // -------------------------
        DockerRunner runner(dockerImage_);

        auto comp = runner.compile(submissionDir, "main.cpp");

        // Leer compile.log
        std::ifstream compLog(comp.logFilePath);
        if (compLog) {
            result.compileLog.assign(
                (std::istreambuf_iterator<char>(compLog)),
                std::istreambuf_iterator<char>());
        }

        // Si compilación falló
        if (comp.exitCode != 0) {
            result.overallStatus = OverallStatus::CompilationError;
            return result;
        }

        // -------------------------
        // 3. Ejecutar test cases
        // -------------------------
        int maxTimeMs = 0;
        int maxMemoryKb = 0;

        for (const auto& tc : request.testCases) {
            TestResult tr;
            tr.testId = tc.id;

            // Construcción de nombres
            std::string inputFile   = "input_"   + tc.id + ".txt";
            std::string outputFile  = "output_"  + tc.id + ".txt";
            std::string runtimeFile = "runtime_" + tc.id + ".log";

            // Configurar límites de ejecución
            RunLimits limits;

            // time limit → mínimo 1s
            limits.timeLimitSeconds = std::max(1, request.timeLimitMs / 1000);

            // memoria → convertir KB a MB
            if (request.memoryLimitKb > 0) {
                limits.memoryLimitMb = std::max(16, request.memoryLimitKb / 1024);
            } else {
                limits.memoryLimitMb = 256;
            }

            limits.cpuLimit  = 1.0;
            limits.pidsLimit = 64;

            // Medición de tiempo
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

            // Leer runtime log
            std::ifstream rt(runRes.runtimeLogPath);
            if (rt) {
                tr.runtimeLog.assign(
                    (std::istreambuf_iterator<char>(rt)),
                    std::istreambuf_iterator<char>());
            }

            // Extraer memoria usada
            tr.memoryKb = extractMaxMemoryKb(tr.runtimeLog);
            if (tr.memoryKb > maxMemoryKb) {
                maxMemoryKb = tr.memoryKb;
            }

            // Clasificar estado del test
            if (runRes.timedOut) {
                tr.status = TestStatus::TimeLimitExceeded;
            }
            else if (runRes.exitCode != 0) {
                tr.status = TestStatus::RuntimeError;
            }
            else {
                // Límite de tamaño de salida: 1 MB
                constexpr std::uintmax_t MAX_OUTPUT_BYTES = 1 * 1024 * 1024;

                std::error_code ecSize;
                auto outputPath = submissionDir / outputFile;
                auto outSize = std::filesystem::file_size(outputPath, ecSize);

                if (!ecSize && outSize > MAX_OUTPUT_BYTES) {
                    tr.status = TestStatus::RuntimeError;
                    tr.runtimeLog +=
                        "\n[Output limit exceeded: " + std::to_string(outSize) + " bytes]\n";
                } else {
                    // Caso /run: no se compara expected_output
                    if (tc.expectedOutput.empty()) {
                        tr.status = TestStatus::Accepted;
                    } else {
                        // Comparación tolerante
                        bool ok = OutputComparer::areEqual(
                            submissionDir / outputFile,
                            submissionDir / ("expected_" + tc.id + ".txt"));

                        tr.status = ok ? TestStatus::Accepted : TestStatus::WrongAnswer;
                    }
                }
            }

            result.tests.push_back(std::move(tr));
        }

        // Guardar máximos globales
        result.maxTimeMs = maxTimeMs;
        result.maxMemoryKb = maxMemoryKb;

        // -------------------------
        // 4. Estado global
        // -------------------------
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
        }
        else if (anyAccepted) {
            result.overallStatus = OverallStatus::PartialAccepted;
        }
        else {
            result.overallStatus = OverallStatus::PartialAccepted;
        }

    } catch (const std::exception& ex) {
        result.overallStatus = OverallStatus::InternalError;
        result.compileLog += "\n[INTERNAL ERROR] ";
        result.compileLog += ex.what();
    }

    return result;
}

} // namespace engine
