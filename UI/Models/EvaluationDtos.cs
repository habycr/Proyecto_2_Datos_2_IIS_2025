using System.Text.Json.Serialization;

/// DTO que representa la respuesta completa enviada por el Motor de Evaluación
/// después de compilar y ejecutar una solución.
/// Coincide exactamente con el JSON emitido por el servidor C++.

public class EvaluationResponse
{
    [JsonPropertyName("submission_id")]
    public string SubmissionId { get; set; }

    [JsonPropertyName("overall_status")]
    public string OverallStatus { get; set; } // Accepted, WrongAnswer, RuntimeError, etc.

    [JsonPropertyName("compile_log")]
    public string CompileLog { get; set; } // Log completo de compilación (stderr)

    [JsonPropertyName("max_time_ms")]
    public int MaxTimeMs { get; set; } // Tiempo máximo entre todos los tests

    [JsonPropertyName("max_memory_kb")]
    public int MaxMemoryKb { get; set; } // Memoria máxima usada

    [JsonPropertyName("tests")]
    public TestResultDto[] Tests { get; set; } // Lista detallada por test
}


/// DTO de un resultado específico de un test.
public class TestResultDto
{
    [JsonPropertyName("id")]
    public string Id { get; set; }

    [JsonPropertyName("status")]
    public string Status { get; set; } // Accepted, WrongAnswer, etc.

    [JsonPropertyName("time_ms")]
    public int TimeMs { get; set; } // Tiempo por test individual

    [JsonPropertyName("memory_kb")]
    public int MemoryKb { get; set; } // Memoria por test individual

    [JsonPropertyName("runtime_log")]
    public string RuntimeLog { get; set; } // stderr de la ejecución
}
