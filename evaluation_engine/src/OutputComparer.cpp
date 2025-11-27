#include "OutputComparer.h"

#include <fstream>
#include <algorithm>
#include <vector>

namespace engine {

    namespace {
        inline void trimRight(std::string& s) {
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t' ||
                                  s.back() == '\r' || s.back() == '\n')) {
                s.pop_back();
                                  }
        }

        std::vector<std::string> readNormalizedLines(const std::filesystem::path& path) {
            std::ifstream in(path);
            std::vector<std::string> lines;

            if (!in) {
                // Si no se puede leer, devolvemos vector vacío;
                // el que llame decidirá que no son iguales.
                return lines;
            }

            std::string line;
            while (std::getline(in, line)) {
                // getline quita '\n' pero deja '\r' si venía de Windows "\r\n"
                trimRight(line);
                lines.push_back(std::move(line));
            }

            // Eliminar líneas vacías al final
            while (!lines.empty() && lines.back().empty()) {
                lines.pop_back();
            }

            return lines;
        }
    } // namespace

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
