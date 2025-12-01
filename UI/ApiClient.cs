using System;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;

namespace UI.Services
{
    /// Cliente HTTP genérico para enviar peticiones al backend.
    /// Lo heredan clientes más específicos:
    ///  - ProblemApiClient
    ///  - AnalyzerApiClient
    ///
    /// Contiene utilidades comunes para:
    ///  - Serialización/deserialización JSON
    ///  - Manejo de errores HTTP
    ///  - Construcción de requests
    public class ApiClient
    {
        protected readonly HttpClient _httpClient;

        public ApiClient(string baseUrl)
        {
            _httpClient = new HttpClient
            {
                BaseAddress = new Uri(baseUrl),
                Timeout = TimeSpan.FromSeconds(30)
            };
        }

        // Envía una petición GET a una ruta relativa.
        // Devuelve el contenido como string.
        protected async Task<string?> GetStringAsync(string url)
        {
            try
            {
                var response = await _httpClient.GetAsync(url);

                if (!response.IsSuccessStatusCode)
                    return null;

                return await response.Content.ReadAsStringAsync();
            }
            catch
            {
                return null;
            }
        }

        // Envía una petición POST con contenido JSON serializando automáticamente el objeto.
        protected async Task<string?> PostJsonAsync(string url, object body)
        {
            try
            {
                var json = JsonSerializer.Serialize(body);
                var content = new StringContent(json, Encoding.UTF8, "application/json");

                var response = await _httpClient.PostAsync(url, content);

                if (!response.IsSuccessStatusCode)
                    return null;

                return await response.Content.ReadAsStringAsync();
            }
            catch
            {
                return null;
            }
        }

        // Envía una petición DELETE.
        protected async Task<bool> DeleteAsync(string url)
        {
            try
            {
                var response = await _httpClient.DeleteAsync(url);
                return response.IsSuccessStatusCode;
            }
            catch
            {
                return false;
            }
        }
    }
}
