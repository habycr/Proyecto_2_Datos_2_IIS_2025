#pragma once

#include "Models.h"
#include <filesystem>
#include <string>

namespace engine {

    // ========================================================================
    // EvaluationService
    //
    // Servicio principal del motor de evaluación.
    // Coordina:
    //  - creación de directorio de la submission
    //  - escritura de archivos (código, inputs)
    //  - compilación con DockerRunner
    //  - ejecución de todos los test cases
    //  - armado del EvaluationResult final
    // ========================================================================
    class EvaluationService {
    public:
        // baseDir: carpeta donde se crearán las submissions
        // dockerImage: imagen Docker usada para compilar y ejecutar
        EvaluationService(std::filesystem::path baseDir,
                          std::string dockerImage);

        // Ejecuta toda una submission:
        // - compila
        // - corre todos los test cases
        // - arma el resultado global
        EvaluationResult evaluate(const SubmissionRequest& request);

    private:
        std::filesystem::path baseDir_; // carpeta base para submissions
        std::string dockerImage_;       // imagen Docker seleccionada
    };

} // namespace engine
