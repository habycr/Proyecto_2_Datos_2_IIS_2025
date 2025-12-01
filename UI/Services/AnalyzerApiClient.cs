using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;

namespace UI.Services
{
    /// Contiene métricas detalladas del análisis estático y dinámico:
    /// - nestedLoops: profundidad máxima de loops
    /// - isRecursive: si detectó recursión
    /// - averageRatio: ratio de crecimiento de tiempos
    /// - executionTimes: muestras empíricas usadas para estimar complejidad
    public class AnalysisDetails
    {
        [JsonPropertyName("nestedLoops")]
        public int NestedLoops { get; set; }

        [JsonPropertyName("isRecursive")]
        public bool IsRecursive { get; set; }

        [JsonPropertyName("averageRatio")]
        public double AverageRatio { get; set; }

        [JsonPropertyName("executionTimes")]
        public List<double> ExecutionTimes { get; set; } = new List<double>();
    }

    /// Resultado global del analizador inteligente.
    /// Incluye complejidad, tipo de algoritmo, explicaciones y sugerencias.
    public class AnalysisResult
    {
        [JsonPropertyName("success")]
        public bool Success { get; set; }

        [JsonPropertyName("complexity")]
        public string Complexity { get; set; }

        [JsonPropertyName("algorithmType")]
        public string AlgorithmType { get; set; }

        [JsonPropertyName("details")]
        public AnalysisDetails Details { get; set; }

        [JsonPropertyName("explanation")]
        public string Explanation { get; set; }

        [JsonPropertyName("suggestions")]
        public List<string> Suggestions { get; set; } = new List<string>();

        [JsonPropertyName("error")]
        public string Error { get; set; }
    }

    /// Cliente HTTP para conectarse al Analizador Automático (puerto 8081).
    class AnalyzerApiClient
    {
        private readonly HttpClient _httpClient;
        private const string API_BASE_URL = "http://localhost:8081/api";

        public AnalyzerApiClient()
        {
            // Timeout alto porque realiza compilación + instrumentación
            _httpClient = new HttpClient
            {
                Timeout = TimeSpan.FromMinutes(5)
            };
        }

        /// Verifica si el servidor del analizador está vivo.
        public async Task<bool> CheckServerHealthAsync()
        {
            try
            {
                var response = await _httpClient.GetAsync($"{API_BASE_URL}/health");
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        /// Envía código al analizador para obtener complejidad, tipo de algoritmo,
        /// métricas, explicaciones y sugerencias.
        /// También acepta consoleOutput para análisis híbrido.
        public async Task<AnalysisResult> AnalyzeCodeAsync(string code, string problemName = "", string consoleOutput = "")
        {
            try
            {
                var request = new
                {
                    code = code,
                    problemName = problemName,
                    consoleOutput = consoleOutput
                };

                var json = JsonSerializer.Serialize(request);
                var content = new StringContent(json, Encoding.UTF8, "application/json");

                var response = await _httpClient.PostAsync($"{API_BASE_URL}/analyze", content);
                var responseBody = await response.Content.ReadAsStringAsync();

                if (!response.IsSuccessStatusCode)
                {
                    return new AnalysisResult
                    {
                        Success = false,
                        Error = $"HTTP Error {response.StatusCode}: {responseBody}"
                    };
                }

                var options = new JsonSerializerOptions
                {
                    PropertyNameCaseInsensitive = true
                };

                return JsonSerializer.Deserialize<AnalysisResult>(responseBody, options);
            }
            catch (Exception ex)
            {
                return new AnalysisResult
                {
                    Success = false,
                    Error = $"Error: {ex.Message}"
                };
            }
        }
    }
}
