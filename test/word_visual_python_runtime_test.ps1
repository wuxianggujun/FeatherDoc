param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot,

    [Parameter(Mandatory = $true)]
    [string]$WorkingDir
)

$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Write-FakePythonCommand {
    param(
        [string]$Path,
        [string]$Marker
    )

    $content = @"
param([Parameter(ValueFromRemainingArguments = `$true)][string[]]`$Arguments)
if (`$Arguments.Count -eq 1 -and `$Arguments[0] -eq '--version') {
    Write-Output 'Python $Marker'
    return
}
throw 'Unexpected fake Python arguments.'
"@
    Set-Content -LiteralPath $Path -Value $content -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$runtimeScript = Join-Path $resolvedRepoRoot "scripts\python_runtime.ps1"
Assert-True -Condition (Test-Path -LiteralPath $runtimeScript -PathType Leaf) `
    -Message "Shared Word visual Python runtime helper is missing: $runtimeScript"

if (Test-Path -LiteralPath $WorkingDir) {
    Remove-Item -LiteralPath $WorkingDir -Recurse -Force
}
New-Item -ItemType Directory -Path $WorkingDir -Force | Out-Null
$resolvedWorkingDir = (Resolve-Path -LiteralPath $WorkingDir).Path

. $runtimeScript

$originalPath = $env:PATH
$originalRequestedPython = $env:FEATHERDOC_PYTHON_EXECUTABLE
try {
    $env:PATH = $resolvedWorkingDir
    $env:FEATHERDOC_PYTHON_EXECUTABLE = $null

    $pyOnlyPath = Join-Path $resolvedWorkingDir "py.ps1"
    Write-FakePythonCommand -Path $pyOnlyPath -Marker "3.13.0-py-only"
    $resolvedPyOnly = Resolve-FeatherDocPythonExecutable
    Assert-True -Condition ($resolvedPyOnly -eq $pyOnlyPath) `
        -Message "The shared resolver did not accept a validated py launcher when python/python3 were unavailable."

    $invalidPythonPath = Join-Path $resolvedWorkingDir "python.ps1"
    Set-Content -LiteralPath $invalidPythonPath -Value "Write-Output 'not a Python interpreter'" -Encoding UTF8
    $resolvedPastInvalidPython = Resolve-FeatherDocPythonExecutable
    Assert-True -Condition ($resolvedPastInvalidPython -eq $pyOnlyPath) `
        -Message "The shared resolver accepted a non-Python command without validating --version."
    Remove-Item -LiteralPath $invalidPythonPath -Force

    $python3Path = Join-Path $resolvedWorkingDir "python3.ps1"
    Write-FakePythonCommand -Path $python3Path -Marker "3.12.0-python3"
    $resolvedPython3 = Resolve-FeatherDocPythonExecutable
    Assert-True -Condition ($resolvedPython3 -eq $python3Path) `
        -Message "The shared resolver did not prefer python3 over py."

    $pythonPath = Join-Path $resolvedWorkingDir "python.ps1"
    Write-FakePythonCommand -Path $pythonPath -Marker "3.11.0-python"
    $resolvedPython = Resolve-FeatherDocPythonExecutable
    Assert-True -Condition ($resolvedPython -eq $pythonPath) `
        -Message "The shared resolver did not prefer python over python3 and py."

    $env:FEATHERDOC_PYTHON_EXECUTABLE = $pyOnlyPath
    $resolvedExplicit = Resolve-FeatherDocPythonExecutable
    Assert-True -Condition ($resolvedExplicit -eq $pyOnlyPath) `
        -Message "FEATHERDOC_PYTHON_EXECUTABLE did not override PATH candidates."

    $env:FEATHERDOC_PYTHON_EXECUTABLE = Join-Path $resolvedWorkingDir "missing-python.exe"
    $invalidExplicitFailed = $false
    try {
        $null = Resolve-FeatherDocPythonExecutable
    } catch {
        $invalidExplicitFailed = $_.Exception.Message -match "FEATHERDOC_PYTHON_EXECUTABLE"
    }
    Assert-True -Condition $invalidExplicitFailed `
        -Message "An invalid FEATHERDOC_PYTHON_EXECUTABLE must fail with actionable context instead of silently falling back."
} finally {
    $env:PATH = $originalPath
    $env:FEATHERDOC_PYTHON_EXECUTABLE = $originalRequestedPython
}

$wordVisualScripts = Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "scripts") -Filter "*.ps1" -File |
    Where-Object {
        $_.Name -notlike "run_pdf_*" -and
        $_.Name -ne "python_runtime.ps1" -and
        (Select-String -LiteralPath $_.FullName -Pattern "Resolve-FeatherDocPythonExecutable|function Get-BasePython" -Quiet)
    }

Assert-True -Condition ($wordVisualScripts.Count -eq 49) `
    -Message "Expected 49 migrated Word visual scripts, found $($wordVisualScripts.Count)."

foreach ($script in $wordVisualScripts) {
    $source = Get-Content -LiteralPath $script.FullName -Raw -Encoding UTF8
    Assert-True -Condition ($source -notmatch "function\s+Get-BasePython") `
        -Message "Legacy Get-BasePython remains in $($script.Name)."
    Assert-True -Condition ($source -match [regex]::Escape('python_runtime.ps1')) `
        -Message "Shared Python runtime helper is not imported by $($script.Name)."
    Assert-True -Condition ($source -match "Resolve-FeatherDocPythonExecutable") `
        -Message "Shared Python resolver is not used by $($script.Name)."
}

Write-Host "Word visual Python runtime tests passed."
