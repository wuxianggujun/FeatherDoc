param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Invoke-PowerShellScript {
    param([string]$ScriptPath, [string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellPath = (Get-Command powershell.exe -ErrorAction Stop).Source
    }

    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    throw "WorkingDir is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_pdf_dependency_inputs.ps1"
$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "PDF dependency input check has parse errors."
}

$missingSummaryPath = Join-Path $resolvedWorkingDir "missing-summary.json"
$missingResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PdfioSourceDir", (Join-Path $resolvedWorkingDir "missing-pdfio"),
    "-PdfiumProvider", "source",
    "-PdfiumSourceDir", (Join-Path $resolvedWorkingDir "missing-pdfium"),
    "-OutputJson", $missingSummaryPath
)
Assert-Equal -Actual $missingResult.ExitCode -Expected 0 `
    -Message "Missing dependency input check should report not_ready without failing."
$missingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $missingSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$missingSummary.schema) `
    -Expected "featherdoc.pdf_dependency_inputs_check.v1" `
    -Message "Dependency input check should expose a stable schema."
Assert-Equal -Actual ([string]$missingSummary.status) -Expected "not_ready" `
    -Message "Missing dependency input check should be not_ready."
Assert-Equal -Actual ([bool]$missingSummary.pdfio_ready) -Expected $false `
    -Message "Missing PDFio input should be reported."
Assert-Equal -Actual ([bool]$missingSummary.pdfium_ready) -Expected $false `
    -Message "Missing PDFium input should be reported."
Assert-True -Condition ([int]$missingSummary.missing_input_count -ge 2) `
    -Message "Missing dependency input check should count missing inputs."

$strictResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PdfioSourceDir", (Join-Path $resolvedWorkingDir "missing-pdfio"),
    "-PdfiumProvider", "source",
    "-PdfiumSourceDir", (Join-Path $resolvedWorkingDir "missing-pdfium"),
    "-Strict"
)
Assert-True -Condition ($strictResult.ExitCode -ne 0) `
    -Message "Strict missing dependency input check should fail."

$sourceRoot = Join-Path $resolvedWorkingDir "source-ready"
$pdfioSource = Join-Path $sourceRoot "pdfio-src"
$pdfiumSource = Join-Path $sourceRoot "pdfium"
New-Item -ItemType Directory -Path $pdfioSource -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $pdfiumSource "public") -Force | Out-Null
Set-Content -LiteralPath (Join-Path $pdfioSource "pdfio.h") -Encoding UTF8 -Value "/* fake pdfio */"
Set-Content -LiteralPath (Join-Path $pdfiumSource "public\fpdfview.h") -Encoding UTF8 -Value "/* fake pdfium */"
$sourceSummaryPath = Join-Path $resolvedWorkingDir "source-summary.json"
$sourceResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PdfioSourceDir", $pdfioSource,
    "-PdfiumProvider", "source",
    "-PdfiumSourceDir", $pdfiumSource,
    "-OutputJson", $sourceSummaryPath,
    "-Strict"
)
Assert-Equal -Actual $sourceResult.ExitCode -Expected 0 `
    -Message "Source dependency input check should pass."
$sourceSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $sourceSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$sourceSummary.status) -Expected "ready" `
    -Message "Source dependency input check should be ready."
Assert-Equal -Actual ([string]$sourceSummary.selected_pdfium_provider) -Expected "source" `
    -Message "Source dependency input check should select source."
Assert-Equal -Actual ([int]$sourceSummary.missing_input_count) -Expected 0 `
    -Message "Source dependency input check should have zero missing inputs."

$prebuiltRoot = Join-Path $resolvedWorkingDir "prebuilt-ready"
$prebuiltInclude = Join-Path $prebuiltRoot "include"
$prebuiltLibDir = Join-Path $prebuiltRoot "lib"
$prebuiltBinDir = Join-Path $prebuiltRoot "bin"
New-Item -ItemType Directory -Path $prebuiltInclude -Force | Out-Null
New-Item -ItemType Directory -Path $prebuiltLibDir -Force | Out-Null
New-Item -ItemType Directory -Path $prebuiltBinDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $prebuiltInclude "fpdfview.h") -Encoding UTF8 -Value "/* fake pdfium */"
Set-Content -LiteralPath (Join-Path $prebuiltLibDir "pdfium.dll.lib") -Encoding UTF8 -Value "fake import lib"
Set-Content -LiteralPath (Join-Path $prebuiltLibDir "pdfium.lib") -Encoding UTF8 -Value "fallback lib"
Set-Content -LiteralPath (Join-Path $prebuiltBinDir "pdfium.dll") -Encoding UTF8 -Value "fake runtime"
$prebuiltSummaryPath = Join-Path $resolvedWorkingDir "prebuilt-summary.json"
$prebuiltResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PdfioSourceDir", $pdfioSource,
    "-PdfiumProvider", "prebuilt",
    "-PdfiumPrebuiltRoot", $prebuiltRoot,
    "-OutputJson", $prebuiltSummaryPath,
    "-Strict"
)
Assert-Equal -Actual $prebuiltResult.ExitCode -Expected 0 `
    -Message "Prebuilt dependency input check should pass."
$prebuiltSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $prebuiltSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$prebuiltSummary.status) -Expected "ready" `
    -Message "Prebuilt dependency input check should be ready."
Assert-Equal -Actual ([string]$prebuiltSummary.selected_pdfium_provider) -Expected "prebuilt" `
    -Message "Prebuilt dependency input check should select prebuilt."
Assert-Equal -Actual ([string]$prebuiltSummary.pdfium_prebuilt_root) `
    -Expected ([System.IO.Path]::GetFullPath($prebuiltRoot)) `
    -Message "Prebuilt dependency input check should expose the prebuilt root."
Assert-Equal -Actual ([bool]$prebuiltSummary.pdfium_prebuilt_root_exists) -Expected $true `
    -Message "Prebuilt dependency input check should report the prebuilt root as present."
Assert-Equal -Actual ([string]$prebuiltSummary.pdfium_library) `
    -Expected ([System.IO.Path]::GetFullPath((Join-Path $prebuiltLibDir "pdfium.dll.lib"))) `
    -Message "Prebuilt dependency input check should prefer pdfium.dll.lib from the root."
Assert-Equal -Actual ([string]$prebuiltSummary.pdfium_include_dir) `
    -Expected ([System.IO.Path]::GetFullPath($prebuiltInclude)) `
    -Message "Prebuilt dependency input check should derive include from the root."
Assert-Equal -Actual ([string]$prebuiltSummary.pdfium_runtime_dll) `
    -Expected ([System.IO.Path]::GetFullPath((Join-Path $prebuiltBinDir "pdfium.dll"))) `
    -Message "Prebuilt dependency input check should derive runtime DLL from the root."

$runtimeDirRoot = Join-Path $resolvedWorkingDir "prebuilt-runtime-dir"
$runtimeDirInclude = Join-Path $runtimeDirRoot "include"
$runtimeDirLibDir = Join-Path $runtimeDirRoot "lib"
$externalRuntimeDir = Join-Path $resolvedWorkingDir "external-runtime"
New-Item -ItemType Directory -Path $runtimeDirInclude -Force | Out-Null
New-Item -ItemType Directory -Path $runtimeDirLibDir -Force | Out-Null
New-Item -ItemType Directory -Path $externalRuntimeDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $runtimeDirInclude "fpdfview.h") -Encoding UTF8 -Value "/* fake pdfium */"
Set-Content -LiteralPath (Join-Path $runtimeDirLibDir "pdfium.dll.lib") -Encoding UTF8 -Value "fake import lib"
$runtimeDirSummaryPath = Join-Path $resolvedWorkingDir "prebuilt-runtime-dir-summary.json"
$runtimeDirResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PdfioSourceDir", $pdfioSource,
    "-PdfiumProvider", "prebuilt",
    "-PdfiumPrebuiltRoot", $runtimeDirRoot,
    "-PdfiumRuntimeDir", $externalRuntimeDir,
    "-OutputJson", $runtimeDirSummaryPath,
    "-Strict"
)
Assert-Equal -Actual $runtimeDirResult.ExitCode -Expected 0 `
    -Message "Prebuilt dependency input check should accept an explicit runtime directory."
$runtimeDirSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $runtimeDirSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$runtimeDirSummary.status) -Expected "ready" `
    -Message "Prebuilt dependency input check should be ready with an explicit runtime directory."
Assert-Equal -Actual ([string]$runtimeDirSummary.pdfium_runtime_dll) -Expected "" `
    -Message "Prebuilt dependency input check should not derive bin\pdfium.dll when runtime dir is explicit."
Assert-Equal -Actual ([string]$runtimeDirSummary.pdfium_runtime_dir) `
    -Expected ([System.IO.Path]::GetFullPath($externalRuntimeDir)) `
    -Message "Prebuilt dependency input check should preserve the explicit runtime directory."

Write-Host "PDF dependency input check contract passed."
