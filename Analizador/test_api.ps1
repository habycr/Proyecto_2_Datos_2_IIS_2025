$apiUrl = "http://localhost:8081/api/analyze"
$testsDir = ".\tests"

if (-not (Test-Path $testsDir)) {
    Write-Host "Folder 'tests' not found."
    exit
}

$files = Get-ChildItem -Path $testsDir -Filter *.cpp

foreach ($file in $files) {

    Write-Host "====================================="
    Write-Host ("PRUEBA: " + $file.Name)
    Write-Host "====================================="

    # Leer como texto plano
    $codeRaw = Get-Content -Path $file.FullName -Raw

    # Escapar para JSON correctamente
    $codeEscaped = $codeRaw.Replace('\','\\').Replace('"','\"').Replace("`n","\n").Replace("`r","")

    # Construir JSON manualmente (seguro)
    $json = "{""code"": ""$codeEscaped"", ""problemName"": ""$($file.Name)""}"

    try {
        $response = Invoke-RestMethod -Uri $apiUrl -Method Post -Body $json -ContentType "application/json"

        if ($response.success -eq $true) {
            Write-Host ("Complejidad: " + $response.complexity)
            Write-Host ("Algoritmo:   " + $response.algorithmType)
            Write-Host ("Loops:       " + $response.details.nestedLoops)
            Write-Host ("Recursivo:   " + $response.details.isRecursive)
        }
        else {
            Write-Host "ERROR:"
            Write-Host $response.error
        }
    }
    catch {
        Write-Host "HTTP ERROR:"
        Write-Host $_
    }

    Write-Host ""
}

Write-Host "Pruebas completadas."


