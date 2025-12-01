#include "OutputComparer.h"

#include <fstream>
#include <algorithm>
#include <vector>

namespace engine {

    namespace {

        // Quita espacios, tabs, \r y \n al final de la línea.
        inline void trimRight(std::string& s) {
            while (!s.empty() &&
                   (s.back() == ' ' || s.back() == '\t' ||
                    s.back() == '\r' || s.back() == '\n')) {
                s.pop_back();
            }
        }

        // Lee un archivo línea por línea, normalizando:
        // - elimina \r
        // - elimina espacios al final
        // - elimina líneas vacías finales
        std::vector<std::string> readNormalizedLines(const std::filesystem::path& path) {
            std::ifstream in(path);
            std::vector<std::string> lines;

            if (!in) {
                return lines; // si no existe, se consideran no iguales
            }

            std::string line;
            while (std::getline(in, line)) {
                trimRight(line);
                lines.push_back(std::move(line));
            }

            // Eliminar líneas vacías al final
            while (!lines.empty() && lines.back().empty()) {
                lines.pop_back();
            }

            return lines;
        }
    } // namespace interno

    // ========================================================================
    // areEqual
    // Compara línea por línea dos archivos normalizados.
    // ========================================================================
    bool OutputComparer::areEqual(
        const std::filesystem::path& outputFile,
        const std::filesystem::path& expectedFile)
    {
        auto outLines = readNormalizedLines(outputFile);
        auto expLines = readNormalizedLines(expectedFile);

        if (outLines.size() != expLines.size()) {
            return false;
        }

        for (std::size_t i = 0; i < outLines.size(); ++i) {
            if (outLines[i] != expLines[i]) {
                return false;
            }
        }

        return true;
    }

} // namespace engine
