#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <iostream>

#include "ProblemRepository.h"

int main() {
    try {
        mongocxx::instance inst{};  // Inicializa driver MongoDB (una sola vez)
        mongocxx::client client{mongocxx::uri{"mongodb://localhost:27017"}};

        auto db = client["codecoach"];  // Nombre de tu base de datos
        ProblemRepository repo{db};

        // Crear problema de ejemplo
        Problem p;
        p.problem_id  = "sum-two-numbers";
        p.title       = "Suma de dos números";
        p.description = "Dado dos enteros a y b, imprima su suma.";
        p.difficulty  = "Fácil";
        p.code_stub   = "int solve(int a, int b) { return a + b; }";
        p.tags        = {"intro", "math"};
        p.test_cases = {
            {"2 3", "5"},
            {"10 20", "30"}
        };

        repo.insert_problem(p);
        std::cout << "[OK] Problema insertado.\n";

        auto all = repo.get_all();
        std::cout << "[INFO] Problemas en BD: " << all.size() << "\n";

        auto found = repo.get_by_id("sum-two-numbers");
        if (found) {
            std::cout << "[OK] Encontrado: " << found->title
                      << " (" << found->difficulty << ")\n";
        } else {
            std::cout << "[WARN] No se encontró sum-two-numbers\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Excepción: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
