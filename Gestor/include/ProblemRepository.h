#pragma once

#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>

#include <string>
#include <vector>
#include <optional>

// Representa un caso de prueba de un problema
struct TestCase {
    std::string input;
    std::string expected_output;
};

// Representa un problema de CodeCoach
struct Problem {
    std::string problem_id;           // ID legible (ej: "sum-two-numbers")
    std::string title;
    std::string description;
    std::string difficulty;           // "Fácil", "Medio", "Difícil"
    std::vector<std::string> tags;    // ["intro", "math"]
    std::vector<TestCase> test_cases; // Casos de prueba
    std::string code_stub;            // Código base opcional
};

// Repositorio que encapsula el acceso a MongoDB
class ProblemRepository {
public:
    explicit ProblemRepository(mongocxx::database db);

    // Crea un nuevo documento en la colección "problems"
    void insert_problem(const Problem& p);

    // Devuelve todos los problemas
    std::vector<Problem> get_all();

    // Busca por problem_id (clave lógica)
    std::optional<Problem> get_by_id(const std::string& id);

    // Lista problemas filtrados por dificultad
    std::vector<Problem> get_by_difficulty(const std::string& difficulty);

    // Actualiza un problema existente (match por problem_id)
    bool update_problem(const Problem& p);

    // Elimina un problema (match por problem_id)
    bool delete_problem(const std::string& id);

private:
    mongocxx::collection collection_;
};
