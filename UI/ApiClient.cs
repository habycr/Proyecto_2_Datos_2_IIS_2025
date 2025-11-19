

using System.Net.Http;
using System.Text;
using System.Text.Json;
namespace UI
{
    class ApiClient
    {
        private readonly HttpClient http = new HttpClient();

        public string MotorEvaluacionURL { get; set; }
        public string GestorProblemasURL { get; set; }
        public string AnalizadorURL { get; set; }

        public async Task<JsonDocument> GetProbelemList()
        {
            var response = await http.GetStreamAsync($"{GestorProblemasURL}/problems"); //Esto se cambia por la URL real
            return JsonDocument.Parse(response);
        }

        public async Task<JsonDocument> GetProblemDetail()
        {
            var response = await http.GetStreamAsync($"{GestorProblemasURL}/problem/detail"); //Esto se cambia por la URL real
            return JsonDocument.Parse(response);
        }

        public async Task<JsonDocument> RunSolution(string problemId, string code)
        {
            var body = JsonSerializer.Serialize(new
            {
                problemId,
                code,
                mode = "sample"
            });
            var response = await http.PostAsync(
                $"{MotorEvaluacionURL}/run",
                new StringContent(body, System.Text.Encoding.UTF8, "application/json"));
            return JsonDocument.Parse(await response.Content.ReadAsStringAsync());
        }

        public async Task<JsonDocument> SubmitSolution(string problemId, string code, string lang)
        {
            var body = JsonSerializer.Serialize(new
            {
                problemId,
                code,
                language = lang
            });

            var response = await http.PostAsync(
                $"{AnalizadorURL}/submit",
                new StringContent(body, Encoding.UTF8, "application/json"));

            return JsonDocument.Parse(await response.Content.ReadAsStringAsync());
        }



    }
}


