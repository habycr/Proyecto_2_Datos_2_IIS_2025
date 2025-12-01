#pragma once

#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>

#include <string>
#include <vector>
#include <optional>

// ============================================================================
// Estructuras de datos utilizadas por el Gestor de Problemas
// ============================================================================

// Representa un caso de prueba asociado a un problema.
// Cada caso contiene:
// - input:      la entrada que se enviará al programa del usuario.
// - expected_output: la salida esperada para validar la solución.
struct TestCase {
    std::string input;
    std::string expected_output;
};

// Representa un problema almacenado en MongoDB.
// Este objeto se construye tanto desde JSON (UI) como desde documentos BSON.
struct Problem {
    std::string problem_id;           // ID lógico (ej: "sum-two-numbers"), usado como primary key.
    std::string title;                // Título del problema mostrado en la UI.
    std::string description;          // Enunciado completo.
    std::string difficulty;           // Nivel de dificultad ("Fácil", "Medio", "Difícil").
    std::vector<std::string> tags;    // Lista de etiquetas (ej: ["math", "loops"]).
    std::vector<TestCase> test_cases; // Casos de prueba para el juez.
    std::string code_stub;            // Código base inicial mostrado al usuario.
};

// ============================================================================
// Repositorio que encapsula TODA la comunicación con MongoDB.
// Contiene métodos CRUD para administrar problemas.
// ============================================================================
class ProblemRepository {
public:
    // Recibe la instancia de base de datos (db["problems"]) desde el main.
    explicit ProblemRepository(mongocxx::database db);

    // Inserta un nuevo documento en la colección "problems".
    void insert_problem(const Problem& p);

    // Obtiene todos los problemas de la colección.
    std::vector<Problem> get_all();

    // Busca un problema por su identificador lógico (problem_id).
    std::optional<Problem> get_by_id(const std::string& id);

    // Filtra problemas por nivel de dificultad.
    std::vector<Problem> get_by_difficulty(const std::string& difficulty);

    // Actualiza un problema existente (match por problem_id).
    bool update_problem(const Problem& p);

    // Elimina un problema de la colección (match por problem_id).
    bool delete_problem(const std::string& id);

private:
    // Colección MongoDB "problems".
    mongocxx::collection collection_;
};
