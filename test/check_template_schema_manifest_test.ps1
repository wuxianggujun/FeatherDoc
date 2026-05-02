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

function Invoke-TemplateSchemaManifestScript {
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

function Convert-ToPortableRelativePath {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $resolvedBasePath = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $resolvedBasePath.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $resolvedBasePath += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolvedTargetPath = [System.IO.Path]::GetFullPath($TargetPath)

    $baseUri = [System.Uri]$resolvedBasePath
    $targetUri = [System.Uri]$resolvedTargetPath
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)
    return ([System.Uri]::UnescapeDataString($relativeUri.ToString()) -replace '\\', '/')
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_template_schema_manifest.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$matchingSchema = Join-Path $resolvedWorkingDir "matching.template-schema.json"
$driftSchema = Join-Path $resolvedWorkingDir "drift.template-schema.json"
$passManifest = Join-Path $resolvedWorkingDir "pass.manifest.json"
$failManifest = Join-Path $resolvedWorkingDir "fail.manifest.json"
$passOutputDir = Join-Path $resolvedWorkingDir "pass-output"
$failOutputDir = Join-Path $resolvedWorkingDir "fail-output"
$passReviewDir = Join-Path $resolvedWorkingDir "pass-reviews"
$failReviewDir = Join-Path $resolvedWorkingDir "fail-reviews"

Set-Content -LiteralPath $matchingSchema -Encoding UTF8 -Value `
    '{"targets":[{"part":"body","slots":[{"bookmark":"customer_name","kind":"text"},{"bookmark":"invoice_number","kind":"text"},{"bookmark":"issue_date","kind":"text"},{"bookmark":"line_item_row","kind":"table_rows"},{"bookmark":"note_lines","kind":"block"}]}]}'
Set-Content -LiteralPath $driftSchema -Encoding UTF8 -Value `
    '{"targets":[{"part":"body","slots":[{"bookmark":"customer_name","kind":"text"}]}]}'

$sampleDocxRelative = "samples/chinese_invoice_template.docx"
$matchingSchemaRelative = Convert-ToPortableRelativePath -BasePath $resolvedRepoRoot -TargetPath $matchingSchema
$driftSchemaRelative = Convert-ToPortableRelativePath -BasePath $resolvedRepoRoot -TargetPath $driftSchema

$passManifestObject = [ordered]@{
    entries = @(
        [ordered]@{
            name = "matching"
            input_docx = $sampleDocxRelative
            schema_file = $matchingSchemaRelative
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
            schema_file = $matchingSchemaRelative
        },
        [ordered]@{
            name = "drift"
            input_docx = $sampleDocxRelative
            schema_file = $driftSchemaRelative
        }
    )
}
($failManifestObject | ConvertTo-Json -Depth 5) |
    Set-Content -LiteralPath $failManifest -Encoding UTF8

$commonArguments = @(
    "-BuildDir", $resolvedBuildDir,
    "-SkipBuild"
)

$passResult = Invoke-TemplateSchemaManifestScript -Arguments (
    $commonArguments + @(
        "-ManifestPath", $passManifest,
        "-OutputDir", $passOutputDir,
        "-ReviewJsonOutputDir", $passReviewDir
    )
)
Assert-Equal -Actual $passResult.ExitCode -Expected 0 `
    -Message "Clean template schema manifest should pass. Output: $($passResult.Text)"
Assert-ContainsText -Text $passResult.Text -ExpectedText "schema_patch_review_output_path:" `
    -Message "Passing manifest should expose review JSON paths."

$passSummaryPath = Join-Path $passOutputDir "summary.json"
Assert-True -Condition (Test-Path -LiteralPath $passSummaryPath) `
    -Message "Passing manifest should write summary.json."
$passSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$passSummary.entry_count) -Expected 1 `
    -Message "Passing manifest should include one entry."
Assert-Equal -Actual ([bool]$passSummary.passed) -Expected $true `
    -Message "Passing manifest should set passed=true."
Assert-Equal -Actual ([string]$passSummary.review_json_output_dir) -Expected $passReviewDir `
    -Message "Passing manifest should record the review output directory."
Assert-True -Condition (Test-Path -LiteralPath $passSummary.entries[0].schema_patch_review_output_path) `
    -Message "Passing manifest should write an entry review JSON file."
$passReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $passSummary.entries[0].schema_patch_review_output_path | ConvertFrom-Json
Assert-Equal -Actual ([bool]$passReview.changed) -Expected $false `
    -Message "Passing manifest review should be unchanged."

$failResult = Invoke-TemplateSchemaManifestScript -Arguments (
    $commonArguments + @(
        "-ManifestPath", $failManifest,
        "-OutputDir", $failOutputDir,
        "-ReviewJsonOutputDir", $failReviewDir
    )
)
Assert-Equal -Actual $failResult.ExitCode -Expected 1 `
    -Message "Drifting template schema manifest should fail. Output: $($failResult.Text)"
Assert-ContainsText -Text $failResult.Text -ExpectedText "matches: False" `
    -Message "Failing manifest should expose the drifting entry output."

$failSummaryPath = Join-Path $failOutputDir "summary.json"
Assert-True -Condition (Test-Path -LiteralPath $failSummaryPath) `
    -Message "Failing manifest should still write summary.json."
$failSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$failSummary.entry_count) -Expected 2 `
    -Message "Failing manifest should include all entries."
Assert-Equal -Actual ([int]$failSummary.drift_count) -Expected 1 `
    -Message "Failing manifest should report one drift."
Assert-Equal -Actual ([bool]$failSummary.passed) -Expected $false `
    -Message "Failing manifest should set passed=false."
Assert-True -Condition (Test-Path -LiteralPath $failSummary.entries[1].schema_patch_review_output_path) `
    -Message "Failing manifest should write a drift review JSON file."
$driftReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $failSummary.entries[1].schema_patch_review_output_path | ConvertFrom-Json
Assert-Equal -Actual ([bool]$driftReview.changed) -Expected $true `
    -Message "Failing manifest drift review should be changed."
Assert-Equal -Actual ([int]$driftReview.patch.upsert_slot_count) -Expected 4 `
    -Message "Failing manifest drift review should report upsert slots."

Write-Host "Template schema manifest review JSON regression passed."
