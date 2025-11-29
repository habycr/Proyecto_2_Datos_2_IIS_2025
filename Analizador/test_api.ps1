# test_api.ps1
# Encoding: UTF-8

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Testing Complexity Analyzer API" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Test 1: Health
Write-Host ""
Write-Host "[TEST 1] Health Check..." -ForegroundColor Yellow
try {
    $health = Invoke-RestMethod -Uri http://localhost:8080/api/health
    Write-Host "SUCCESS - Status: $($health.status)" -ForegroundColor Green
    Write-Host "Service: $($health.service)" -ForegroundColor Green
    Write-Host "Version: $($health.version)" -ForegroundColor Green
} catch {
    Write-Host "FAILED: $($_.Exception.Message)" -ForegroundColor Red
    exit
}

# Test 2: Test Endpoint
Write-Host ""
Write-Host "[TEST 2] Test Endpoint (Static Analysis)..." -ForegroundColor Yellow
try {
    $test = Invoke-RestMethod -Uri http://localhost:8080/api/test
    Write-Host "SUCCESS" -ForegroundColor Green
    Write-Host "Complexity: $($test.complexity)" -ForegroundColor Cyan
    Write-Host "Algorithm: $($test.algorithmType)" -ForegroundColor Cyan
    Write-Host "Nested Loops: $($test.details.nestedLoops)" -ForegroundColor Cyan
    Write-Host "Recursive: $($test.details.isRecursive)" -ForegroundColor Cyan
    Write-Host "Explanation: $($test.explanation)" -ForegroundColor White
} catch {
    Write-Host "FAILED: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 3: Analyze Custom Code
Write-Host ""
Write-Host "[TEST 3] Analyze Custom Code..." -ForegroundColor Yellow

$testCode = @"
void linearSearch(int n) {
    vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}
"@

$requestBody = @{
    code = $testCode
    problemName = "Linear Search"
} | ConvertTo-Json

try {
    $analyze = Invoke-RestMethod -Uri http://localhost:8080/api/analyze -Method Post -Body $requestBody -ContentType "application/json"
    Write-Host "SUCCESS" -ForegroundColor Green
    Write-Host "Complexity: $($analyze.complexity)" -ForegroundColor Cyan
    Write-Host "Algorithm: $($analyze.algorithmType)" -ForegroundColor Cyan
    Write-Host "Explanation: $($analyze.explanation)" -ForegroundColor White

    if ($analyze.suggestions) {
        Write-Host "Suggestions:" -ForegroundColor Yellow
        foreach ($suggestion in $analyze.suggestions) {
            Write-Host "  - $suggestion" -ForegroundColor White
        }
    }
} catch {
    Write-Host "FAILED: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  All tests completed!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan