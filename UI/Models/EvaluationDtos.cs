using System.Text.Json.Serialization;

public class EvaluationResponse
{
	[JsonPropertyName("submission_id")]
	public string SubmissionId { get; set; }

	[JsonPropertyName("overall_status")]
	public string OverallStatus { get; set; }

	[JsonPropertyName("compile_log")]
	public string CompileLog { get; set; }

	[JsonPropertyName("max_time_ms")]
	public int MaxTimeMs { get; set; }

	[JsonPropertyName("tests")]
	public TestResultDto[] Tests { get; set; }
}

public class TestResultDto
{
	[JsonPropertyName("id")]
	public string Id { get; set; }

	[JsonPropertyName("status")]
	public string Status { get; set; }

	[JsonPropertyName("time_ms")]
	public int TimeMs { get; set; }

	[JsonPropertyName("runtime_log")]
	public string RuntimeLog { get; set; }
}
