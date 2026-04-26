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

function Invoke-NumberingCatalogCheckScript {
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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_numbering_catalog_baseline.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "test\my_test.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$baselineCatalog = Join-Path $resolvedWorkingDir "matching.numbering-catalog.json"
$driftCatalog = Join-Path $resolvedWorkingDir "drift.numbering-catalog.json"
$dirtyCatalog = Join-Path $resolvedWorkingDir "dirty.numbering-catalog.json"
$generatedCatalog = Join-Path $resolvedWorkingDir "generated.numbering-catalog.json"

Set-Content -LiteralPath $baselineCatalog -Encoding UTF8 -Value `
    '{"definition_count":0,"instance_count":0,"definitions":[]}'
Set-Content -LiteralPath $driftCatalog -Encoding UTF8 -Value `
    '{"definitions":[{"name":"UnexpectedOutline","levels":[{"level":0,"kind":"decimal","start":1,"text_pattern":"%1."}],"instances":[]}]}'
Set-Content -LiteralPath $dirtyCatalog -Encoding UTF8 -Value `
    '{"definitions":[{"name":"","levels":[],"instances":[]}]}'

$commonArguments = @(
    "-InputDocx", $sampleDocx,
    "-BuildDir", $resolvedBuildDir,
    "-SkipBuild"
)

$matchResult = Invoke-NumberingCatalogCheckScript -Arguments (
    $commonArguments + @(
        "-CatalogFile", $baselineCatalog,
        "-GeneratedCatalogOutput", $generatedCatalog
    )
)
Assert-Equal -Actual $matchResult.ExitCode -Expected 0 `
    -Message "Matching numbering catalog baseline should pass. Output: $($matchResult.Text)"
Assert-ContainsText -Text $matchResult.Text -ExpectedText "catalog_lint_clean: True" `
    -Message "Matching run should report a clean catalog lint."
Assert-ContainsText -Text $matchResult.Text -ExpectedText "matches: True" `
    -Message "Matching run should report matches=True."
Assert-ContainsText -Text $matchResult.Text -ExpectedText "changed_definition_count: 0" `
    -Message "Matching run should report no changed definitions."
Assert-True -Condition (Test-Path -LiteralPath $generatedCatalog) `
    -Message "Matching run should write the generated numbering catalog."

$generated = Get-Content -Raw -Encoding UTF8 -LiteralPath $generatedCatalog | ConvertFrom-Json
Assert-Equal -Actual ([int]$generated.definition_count) -Expected 0 `
    -Message "Generated catalog for my_test.docx should be empty."

$driftResult = Invoke-NumberingCatalogCheckScript -Arguments (
    $commonArguments + @("-CatalogFile", $driftCatalog)
)
Assert-Equal -Actual $driftResult.ExitCode -Expected 1 `
    -Message "Drifting numbering catalog baseline should fail. Output: $($driftResult.Text)"
Assert-ContainsText -Text $driftResult.Text -ExpectedText "matches: False" `
    -Message "Drift run should report matches=False."
Assert-ContainsText -Text $driftResult.Text -ExpectedText "removed_definition_count: 1" `
    -Message "Drift run should report the removed baseline definition."

$dirtyResult = Invoke-NumberingCatalogCheckScript -Arguments (
    $commonArguments + @("-CatalogFile", $dirtyCatalog)
)
Assert-Equal -Actual $dirtyResult.ExitCode -Expected 1 `
    -Message "Dirty numbering catalog baseline should fail. Output: $($dirtyResult.Text)"
Assert-ContainsText -Text $dirtyResult.Text -ExpectedText "catalog_lint_clean: False" `
    -Message "Dirty run should report a lint failure."
Assert-ContainsText -Text $dirtyResult.Text -ExpectedText '"issue":"empty_definition_name"' `
    -Message "Dirty run should include the empty definition name lint issue."
Assert-ContainsText -Text $dirtyResult.Text -ExpectedText '"issue":"empty_levels"' `
    -Message "Dirty run should include the empty levels lint issue."

Write-Host "Numbering catalog baseline check regression passed."
