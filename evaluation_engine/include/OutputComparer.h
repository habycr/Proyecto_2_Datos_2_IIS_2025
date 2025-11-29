#pragma once

#include <filesystem>
#include <string>

namespace engine {

    class OutputComparer {
    public:
        // Compara dos archivos de texto con tolerancia:
        // - ignora \r
        // - quita espacios en blanco al final de cada línea
        // - ignora líneas vacías al final
        static bool areEqual(
            const std::filesystem::path& outputFile,
            const std::filesystem::path& expectedFile);
    };

} // namespace engine
