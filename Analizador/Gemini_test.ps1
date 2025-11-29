# test_gemini.ps1
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Testing Gemini API Connection" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

try {
    $result = Invoke-RestMethod -Uri http://localhost:8081/api/test-gemini

    Write-Host "`nStatus: " -NoNewline
    if ($result.success) {
        Write-Host "SUCCESS" -ForegroundColor Green
    } else {
        Write-Host "FAILED" -ForegroundColor Red
        Write-Host "Error: $($result.error)" -ForegroundColor Red
        exit
    }

    Write-Host "`nGemini Connected: " -NoNewline
    if ($result.geminiConnected) {
        Write-Host "YES" -ForegroundColor Green
    } else {
        Write-Host "NO" -ForegroundColor Yellow
    }

    Write-Host "Model: $($result.model)" -ForegroundColor Cyan
    Write-Host "Complexity: $($result.complexity)" -ForegroundColor Cyan
    Write-Host "Algorithm: $($result.algorithmType)" -ForegroundColor Cyan
    Write-Host "Suggestions Source: $($result.suggestionsSource)" -ForegroundColor Cyan
    Write-Host "Suggestions Count: $($result.suggestionsCount)" -ForegroundColor Cyan

    Write-Host "`nSuggestions:" -ForegroundColor Yellow
    foreach ($suggestion in $result.suggestions) {
        Write-Host "  • $suggestion" -ForegroundColor White
    }

} catch {
    Write-Host "`nERROR: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`n========================================" -ForegroundColor Cyan