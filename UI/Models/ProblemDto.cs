// Modelos utilizados para comunicar la UI con el Gestor de Problemas (C++).

using System.Collections.Generic;

namespace UI.Models
{
    /// Representa un caso de prueba tal como lo almacena el Gestor.
    /// input → stdin
    /// expected_output → salida esperada exacta.
    public class TestCaseDto
    {
        public string input { get; set; } = string.Empty;
        public string expected_output { get; set; } = string.Empty;
    }

    /// Modelo completo de un problema.
    /// Debe coincidir exactamente con el JSON recibido del backend C++.
    public class ProblemDto
    {
        public string problem_id { get; set; } = string.Empty;
        public string title { get; set; } = string.Empty;
        public string description { get; set; } = string.Empty;
        public string difficulty { get; set; } = string.Empty; // Fácil, Medio, Difícil
        public List<string> tags { get; set; } = new();
        public List<TestCaseDto> test_cases { get; set; } = new();
        public string code_stub { get; set; } = string.Empty;
    }

    /// Respuesta recibida al consultar GET /problems.
    /// Contiene la lista completa de problemas.
    public class ProblemListResponse
    {
        public List<ProblemDto> problems { get; set; } = new();
    }

    /// Respuesta estándar del Gestor a POST/PUT/DELETE.
    /// Contiene mensaje y el ID afectado.
    public class ApiResponse
    {
        public string message { get; set; } = string.Empty;
        public string? problem_id { get; set; }
    }

    /// Estructura para respuestas de error del Gestor.
    /// Devuelta típicamente con status 400/404/500.
    public class ErrorResponse
    {
        public string error { get; set; } = string.Empty;
    }
}

