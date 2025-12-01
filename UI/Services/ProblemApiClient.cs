// Cliente HTTP para hablar con el Gestor de Problemas en C++ (Crow + MongoDB).

using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Net.Http.Json;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using UI.Models;

namespace UI.Services
{
    /// Cliente HTTP para comunicarse con el Gestor de Problemas (REST API en C++).
    /// Maneja las operaciones CRUD de problemas.
    public class ProblemApiClient
    {
        private readonly HttpClient _httpClient;

        /// <param name="gestorBaseUrl">URL base del Gestor, ej: "http://localhost:8080"</param>
        public ProblemApiClient(string gestorBaseUrl = "http://localhost:8080")
        {
            // Aseguramos que no haya doble slash en las rutas
            gestorBaseUrl = gestorBaseUrl.TrimEnd('/');

            _httpClient = new HttpClient
            {
                BaseAddress = new Uri(gestorBaseUrl),
                Timeout = TimeSpan.FromSeconds(30)
            };
        }

        // ==================== GET /health ====================
        public async Task<bool> IsHealthyAsync()
        {
            try
            {
                var response = await _httpClient.GetAsync("/health");
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }

        // ==================== GET /problems ====================
        /// Obtiene la lista resumida de todos los problemas.
        public async Task<List<ProblemDto>> GetAllProblemsAsync()
        {
            try
            {
                var response = await _httpClient.GetAsync("/problems");

                if (!response.IsSuccessStatusCode)
                {
                    Console.WriteLine($"[ERROR] GET /problems falló: {response.StatusCode}");
                    return new List<ProblemDto>();
                }

                var result = await response.Content.ReadFromJsonAsync<ProblemListResponse>();
                return result?.problems ?? new List<ProblemDto>();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ERROR] Excepción en GetAllProblemsAsync: {ex.Message}");
                return new List<ProblemDto>();
            }
        }

        // ==================== GET /problems/{id} ====================
        /// Obtiene el detalle completo de un problema.
        public async Task<ProblemDto?> GetProblemAsync(string problemId)
        {
            try
            {
                var response = await _httpClient.GetAsync($"/problems/{problemId}");

                if (response.StatusCode == System.Net.HttpStatusCode.NotFound)
                {
                    Console.WriteLine($"[INFO] Problema '{problemId}' no encontrado (404).");
                    return null;
                }

                if (!response.IsSuccessStatusCode)
                {
                    Console.WriteLine($"[ERROR] GET /problems/{problemId} falló: {response.StatusCode}");
                    return null;
                }

                return await response.Content.ReadFromJsonAsync<ProblemDto>();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ERROR] Excepción en GetProblemAsync: {ex.Message}");
                return null;
            }
        }

        // ==================== POST /problems ====================
        /// Crea un nuevo problema. El DTO DEBE incluir un problem_id único.
        public async Task<bool> CreateProblemAsync(ProblemDto problem)
        {
            try
            {
                var json = JsonSerializer.Serialize(problem);
                var content = new StringContent(json, Encoding.UTF8, "application/json");

                var response = await _httpClient.PostAsync("/problems", content);

                if (!response.IsSuccessStatusCode)
                {
                    var error = await response.Content.ReadAsStringAsync();
                    Console.WriteLine($"[ERROR] POST /problems falló ({response.StatusCode}): {error}");
                    return false;
                }

                var result = await response.Content.ReadFromJsonAsync<ApiResponse>();
                Console.WriteLine($"[OK] Problema '{result?.problem_id}' creado: {result?.message}");
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ERROR] Excepción en CreateProblemAsync: {ex.Message}");
                return false;
            }
        }

        // ==================== PUT /problems/{id} ====================
        /// Actualiza un problema existente. El ID va en la URL, NO en el body.
        public async Task<bool> UpdateProblemAsync(string problemId, ProblemDto problemWithoutId)
        {
            try
            {
                // Creamos una copia sin problem_id, porque el Gestor lo toma de la URL
                var bodyObject = new
                {
                    title = problemWithoutId.title,
                    description = problemWithoutId.description,
                    difficulty = problemWithoutId.difficulty,
                    code_stub = problemWithoutId.code_stub,
                    tags = problemWithoutId.tags,
                    test_cases = problemWithoutId.test_cases
                };

                var json = JsonSerializer.Serialize(bodyObject);
                var content = new StringContent(json, Encoding.UTF8, "application/json");

                var response = await _httpClient.PutAsync($"/problems/{problemId}", content);

                if (response.StatusCode == System.Net.HttpStatusCode.NotFound)
                {
                    Console.WriteLine($"[ERROR] PUT /problems/{problemId} (404 No encontrado).");
                    return false;
                }

                if (!response.IsSuccessStatusCode)
                {
                    var error = await response.Content.ReadAsStringAsync();
                    Console.WriteLine($"[ERROR] PUT /problems/{problemId} falló ({response.StatusCode}): {error}");
                    return false;
                }

                var result = await response.Content.ReadFromJsonAsync<ApiResponse>();
                Console.WriteLine($"[OK] Problema '{result?.problem_id}' actualizado: {result?.message}");
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ERROR] Excepción en UpdateProblemAsync: {ex.Message}");
                return false;
            }
        }

        // ==================== DELETE /problems/{id} ====================
        /// Elimina un problema existente.
        public async Task<bool> DeleteProblemAsync(string problemId)
        {
            try
            {
                var response = await _httpClient.DeleteAsync($"/problems/{problemId}");

                if (response.StatusCode == System.Net.HttpStatusCode.NotFound)
                {
                    Console.WriteLine($"[ERROR] DELETE /problems/{problemId} (404 No encontrado).");
                    return false;
                }

                if (!response.IsSuccessStatusCode)
                {
                    var error = await response.Content.ReadAsStringAsync();
                    Console.WriteLine($"[ERROR] DELETE /problems/{problemId} falló ({response.StatusCode}): {error}");
                    return false;
                }

                var result = await response.Content.ReadFromJsonAsync<ApiResponse>();
                Console.WriteLine($"[OK] Problema '{result?.problem_id}' eliminado: {result?.message}");
                return true;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ERROR] Excepción en DeleteProblemAsync: {ex.Message}");
                return false;
            }
        }

        // ==================== Extras opcionales ====================

        public async Task<List<ProblemDto>> GetProblemsByDifficultyAsync(string difficulty)
        {
            try
            {
                var response = await _httpClient.GetAsync($"/problems/difficulty/{difficulty}");
                if (!response.IsSuccessStatusCode)
                    return new List<ProblemDto>();

                var result = await response.Content.ReadFromJsonAsync<ProblemListResponse>();
                return result?.problems ?? new List<ProblemDto>();
            }
            catch
            {
                return new List<ProblemDto>();
            }
        }

        public async Task<List<ProblemDto>> GetProblemsByTagAsync(string tag)
        {
            try
            {
                var response = await _httpClient.GetAsync($"/problems/tags/{tag}");
                if (!response.IsSuccessStatusCode)
                    return new List<ProblemDto>();

                var result = await response.Content.ReadFromJsonAsync<ProblemListResponse>();
                return result?.problems ?? new List<ProblemDto>();
            }
            catch
            {
                return new List<ProblemDto>();
            }
        }

        public async Task<ProblemDto?> GetRandomProblemAsync()
        {
            try
            {
                var response = await _httpClient.GetAsync("/problems/random");
                if (!response.IsSuccessStatusCode)
                    return null;

                return await response.Content.ReadFromJsonAsync<ProblemDto>();
            }
            catch
            {
                return null;
            }
        }

        // ==================== POST /submissions ====================
        /// Envía una solución para que sea evaluada por el Gestor + Motor.
        public async Task<EvaluationResponse?> SubmitSolutionAsync(
            string problemId,
            string language,
            string sourceCode,
            int timeLimitMs = 2000)
        {
            try
            {
                var bodyObject = new
                {
                    problem_id = problemId,
                    language = language,
                    source_code = sourceCode,
                    time_limit_ms = timeLimitMs
                };

                var json = JsonSerializer.Serialize(bodyObject);
                var content = new StringContent(json, Encoding.UTF8, "application/json");

                var response = await _httpClient.PostAsync("/submissions", content);

                if (!response.IsSuccessStatusCode)
                {
                    var error = await response.Content.ReadAsStringAsync();
                    Console.WriteLine($"[ERROR] POST /submissions falló ({response.StatusCode}): {error}");
                    return null;
                }

                var responseText = await response.Content.ReadAsStringAsync();

                var options = new JsonSerializerOptions
                {
                    PropertyNameCaseInsensitive = true
                };

                var evalResponse = JsonSerializer.Deserialize<EvaluationResponse>(responseText, options);
                return evalResponse;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[ERROR] Excepción en SubmitSolutionAsync: {ex.Message}");
                return null;
            }
        }



    }
}
