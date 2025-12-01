#pragma once

#include <filesystem>
#include <string>

namespace engine {

    // ============================================================================
    // OutputComparer
    //
    // Utilidad para comparar archivos output.txt y expected.txt.
    // Implementa comparación "tolerante":
    //  - ignora \r
    //  - remueve espacios en blanco al final de cada línea
    //  - ignora líneas vacías al final
    //
    // Esto evita falsos WA por diferencias triviales de formato.
    // ============================================================================
    class OutputComparer {
    public:
        static bool areEqual(
            const std::filesystem::path& outputFile,
            const std::filesystem::path& expectedFile);
    };

} // namespace engine
