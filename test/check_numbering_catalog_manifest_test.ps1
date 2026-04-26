param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
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

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Invoke-NumberingCatalogManifestScript {
    param([string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    $output = @(& $powerShellPath -NoProfile -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_numbering_catalog_manifest.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$baselineCatalog = Join-Path $resolvedWorkingDir "matching.numbering-catalog.json"
$driftCatalog = Join-Path $resolvedWorkingDir "drift.numbering-catalog.json"
$dirtyCatalog = Join-Path $resolvedWorkingDir "dirty.numbering-catalog.json"
$passManifest = Join-Path $resolvedWorkingDir "pass.manifest.json"
$failManifest = Join-Path $resolvedWorkingDir "fail.manifest.json"
$passOutputDir = Join-Path $resolvedWorkingDir "pass-output"
$failOutputDir = Join-Path $resolvedWorkingDir "fail-output"

Set-Content -LiteralPath $baselineCatalog -Encoding UTF8 -Value `
    '{"definition_count":0,"instance_count":0,"definitions":[]}'
Set-Content -LiteralPath $driftCatalog -Encoding UTF8 -Value `
    '{"definitions":[{"name":"UnexpectedOutline","levels":[{"level":0,"kind":"decimal","start":1,"text_pattern":"%1."}],"instances":[]}]}'
Set-Content -LiteralPath $dirtyCatalog -Encoding UTF8 -Value `
    '{"definitions":[{"name":"","levels":[],"instances":[]}]}'

$sampleDocxRelative = "test/my_test.docx"
$baselineCatalogRelative = [System.IO.Path]::GetRelativePath($resolvedRepoRoot, $baselineCatalog).Replace('\\', '/')
$driftCatalogRelative = [System.IO.Path]::GetRelativePath($resolvedRepoRoot, $driftCatalog).Replace('\\', '/')
$dirtyCatalogRelative = [System.IO.Path]::GetRelativePath($resolvedRepoRoot, $dirtyCatalog).Replace('\\', '/')

$passManifestObject = [ordered]@{
    entries = @(
        [ordered]@{
            name = "matching"
            input_docx = $sampleDocxRelative
            catalog_file = $baselineCatalogRelative
        }
    )
}
($passManifestObject | ConvertTo-Json -Depth 5) |
    Set-Content -LiteralPath $passManifest -Encoding UTF8

$failManifestObject = [ordered]@{
    entries = @(
        [ordered]@{
            name = "matching"
            input_docx = $sampleDocxRelative
            catalog_file = $baselineCatalogRelative
        },
        [ordered]@{
            name = "drift"
            input_docx = $sampleDocxRelative
            catalog_file = $driftCatalogRelative
        },
        [ordered]@{
            name = "dirty"
            input_docx = $sampleDocxRelative
            catalog_file = $dirtyCatalogRelative
        }
    )
}
($failManifestObject | ConvertTo-Json -Depth 5) |
    Set-Content -LiteralPath $failManifest -Encoding UTF8

$commonArguments = @(
    "-BuildDir", $resolvedBuildDir,
    "-SkipBuild"
)

$passResult = Invoke-NumberingCatalogManifestScript -Arguments (
    $commonArguments + @(
        "-ManifestPath", $passManifest,
        "-OutputDir", $passOutputDir
    )
)
Assert-Equal -Actual $passResult.ExitCode -Expected 0 `
    -Message "Clean numbering catalog manifest should pass. Output: $($passResult.Text)"
Assert-ContainsText -Text $passResult.Text -ExpectedText "Manifest summary:" `
    -Message "Passing manifest should report the summary path."

$passSummaryPath = Join-Path $passOutputDir "summary.json"
Assert-True -Condition (Test-Path -LiteralPath $passSummaryPath) `
    -Message "Passing manifest should write summary.json."
$passSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$passSummary.entry_count) -Expected 1 `
    -Message "Passing manifest should include one entry."
Assert-Equal -Actual ([int]$passSummary.drift_count) -Expected 0 `
    -Message "Passing manifest should not report drift."
Assert-Equal -Actual ([bool]$passSummary.passed) -Expected $true `
    -Message "Passing manifest should set passed=true."
Assert-True -Condition (Test-Path -LiteralPath $passSummary.entries[0].generated_output_path) `
    -Message "Passing manifest should write a generated catalog output."

$failResult = Invoke-NumberingCatalogManifestScript -Arguments (
    $commonArguments + @(
        "-ManifestPath", $failManifest,
        "-OutputDir", $failOutputDir
    )
)
Assert-Equal -Actual $failResult.ExitCode -Expected 1 `
    -Message "Drifting or dirty numbering catalog manifest should fail. Output: $($failResult.Text)"
Assert-ContainsText -Text $failResult.Text -ExpectedText "matches: False" `
    -Message "Failing manifest should expose the drifting entry output."
Assert-ContainsText -Text $failResult.Text -ExpectedText "catalog_lint_clean: False" `
    -Message "Failing manifest should expose the dirty entry output."

$failSummaryPath = Join-Path $failOutputDir "summary.json"
Assert-True -Condition (Test-Path -LiteralPath $failSummaryPath) `
    -Message "Failing manifest should still write summary.json."
$failSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$failSummary.entry_count) -Expected 3 `
    -Message "Failing manifest should include all entries."
Assert-Equal -Actual ([int]$failSummary.drift_count) -Expected 2 `
    -Message "Failing manifest should report drift for drift and dirty entries."
Assert-Equal -Actual ([int]$failSummary.dirty_baseline_count) -Expected 1 `
    -Message "Failing manifest should report one dirty baseline."
Assert-Equal -Actual ([bool]$failSummary.passed) -Expected $false `
    -Message "Failing manifest should set passed=false."
Assert-Equal -Actual ([string]$failSummary.entries[1].name) -Expected "drift" `
    -Message "Failing manifest should preserve entry ordering."
Assert-Equal -Actual ([int]$failSummary.entries[1].removed_definition_count) -Expected 1 `
    -Message "Drift entry should report the removed baseline definition."
Assert-Equal -Actual ([int]$failSummary.entries[2].catalog_lint_issue_count) -Expected 2 `
    -Message "Dirty entry should report baseline lint issues."

Write-Host "Numbering catalog manifest check regression passed."
