// Archivo: Models/ProblemDto.cs
// Modelos que representan el JSON que envía/recibe el Gestor de Problemas (C++).

using System.Collections.Generic;

namespace UI.Models
{
    /// <summary>
    /// Representa un caso de prueba de un problema.
    /// Coincide exactamente con el JSON del Gestor.
    /// </summary>
    public class TestCaseDto
    {
        public string input { get; set; } = string.Empty;
        public string expected_output { get; set; } = string.Empty;
    }

    /// <summary>
    /// Representa un problema completo de CodeCoach.
    /// Los nombres de las propiedades coinciden con el JSON.
    /// </summary>
    public class ProblemDto
    {
        public string problem_id { get; set; } = string.Empty;
        public string title { get; set; } = string.Empty;
        public string description { get; set; } = string.Empty;
        public string difficulty { get; set; } = string.Empty;
        public List<string> tags { get; set; } = new();
        public List<TestCaseDto> test_cases { get; set; } = new();
        public string code_stub { get; set; } = string.Empty;
    }

    /// <summary>
    /// Respuesta del Gestor para GET /problems.
    /// </summary>
    public class ProblemListResponse
    {
        public List<ProblemDto> problems { get; set; } = new();
    }

    /// <summary>
    /// Respuesta genérica del Gestor para POST/PUT/DELETE.
    /// </summary>
    public class ApiResponse
    {
        public string message { get; set; } = string.Empty;
        public string? problem_id { get; set; }
    }

    /// <summary>
    /// Respuesta de error del Gestor.
    /// </summary>
    public class ErrorResponse
    {
        public string error { get; set; } = string.Empty;
    }
}
