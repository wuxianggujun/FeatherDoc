param(
    [string]$RepoRoot,
    [string]$PythonExecutable
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-ProjectVersion {
    param(
        [string]$CMakeListsPath
    )

    $content = Get-Content -Raw -LiteralPath $CMakeListsPath
    $match = [regex]::Match(
        $content,
        'project\([\s\S]*?\bVERSION\s+([0-9]+\.[0-9]+\.[0-9]+)'
    )
    if (-not $match.Success) {
        throw "Unable to determine project version from $CMakeListsPath"
    }

    return $match.Groups[1].Value
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedPython = (Resolve-Path $PythonExecutable).Path
$cmakeListsPath = Join-Path $resolvedRepoRoot "CMakeLists.txt"
$docsConfPath = Join-Path $resolvedRepoRoot "docs\conf.py"

$expectedVersion = Get-ProjectVersion -CMakeListsPath $cmakeListsPath

$env:FEATHERDOC_DOCS_CONF_PATH = $docsConfPath
try {
    $pythonOutput = & $resolvedPython -c "import os, runpy; cfg = runpy.run_path(os.environ['FEATHERDOC_DOCS_CONF_PATH']); print(cfg['version']); print(cfg['release'])"
}
finally {
    Remove-Item Env:FEATHERDOC_DOCS_CONF_PATH -ErrorAction SilentlyContinue
}

if ($LASTEXITCODE -ne 0) {
    throw "Python failed while evaluating docs/conf.py"
}

$lines = @($pythonOutput | Where-Object { $_ -ne $null })
if ($lines.Count -lt 2) {
    throw "Expected docs/conf.py to print version and release, got: $lines"
}

$docsVersion = $lines[0].Trim()
$docsRelease = $lines[1].Trim()

if ($docsVersion -ne $expectedVersion) {
    throw "docs/conf.py version mismatch: expected '$expectedVersion' but got '$docsVersion'"
}

if ($docsRelease -ne $expectedVersion) {
    throw "docs/conf.py release mismatch: expected '$expectedVersion' but got '$docsRelease'"
}

Write-Host "docs/conf.py version sync passed."
