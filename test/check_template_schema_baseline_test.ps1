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

function Invoke-TemplateSchemaBaselineScript {
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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_template_schema_baseline.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$matchingSchema = Join-Path $resolvedWorkingDir "matching.template-schema.json"
$driftSchema = Join-Path $resolvedWorkingDir "drift.template-schema.json"
$generatedSchema = Join-Path $resolvedWorkingDir "matching.generated.template-schema.json"
$matchingReview = Join-Path $resolvedWorkingDir "matching.schema.review.json"
$driftReview = Join-Path $resolvedWorkingDir "drift.schema.review.json"

Set-Content -LiteralPath $matchingSchema -Encoding UTF8 -Value `
    '{"targets":[{"part":"body","slots":[{"bookmark":"customer_name","kind":"text"},{"bookmark":"invoice_number","kind":"text"},{"bookmark":"issue_date","kind":"text"},{"bookmark":"line_item_row","kind":"table_rows"},{"bookmark":"note_lines","kind":"block"}]}]}'
Set-Content -LiteralPath $driftSchema -Encoding UTF8 -Value `
    '{"targets":[{"part":"body","slots":[{"bookmark":"customer_name","kind":"text"}]}]}'

$commonArguments = @(
    "-InputDocx", $sampleDocx,
    "-BuildDir", $resolvedBuildDir,
    "-SkipBuild"
)

$matchResult = Invoke-TemplateSchemaBaselineScript -Arguments (
    $commonArguments + @(
        "-SchemaFile", $matchingSchema,
        "-GeneratedSchemaOutput", $generatedSchema,
        "-ReviewJsonOutput", $matchingReview
    )
)
Assert-Equal -Actual $matchResult.ExitCode -Expected 0 `
    -Message "Matching template schema baseline should pass. Output: $($matchResult.Text)"
Assert-ContainsText -Text $matchResult.Text -ExpectedText "schema_lint_clean: True" `
    -Message "Matching run should report a clean schema lint."
Assert-ContainsText -Text $matchResult.Text -ExpectedText "matches: True" `
    -Message "Matching run should report matches=True."
Assert-ContainsText -Text $matchResult.Text -ExpectedText "schema_patch_review_output_path: $matchingReview" `
    -Message "Matching run should report the review JSON path."
Assert-True -Condition (Test-Path -LiteralPath $generatedSchema) `
    -Message "Matching run should write the generated schema."
Assert-True -Condition (Test-Path -LiteralPath $matchingReview) `
    -Message "Matching run should write the schema patch review JSON."

$matchingReviewJson = Get-Content -Raw -Encoding UTF8 -LiteralPath $matchingReview | ConvertFrom-Json
Assert-Equal -Actual ([string]$matchingReviewJson.schema) `
    -Expected "featherdoc.template_schema_patch_review.v1" `
    -Message "Matching review should use the stable schema id."
Assert-Equal -Actual ([int]$matchingReviewJson.baseline_slot_count) -Expected 5 `
    -Message "Matching review should count baseline slots."
Assert-Equal -Actual ([int]$matchingReviewJson.generated_slot_count) -Expected 5 `
    -Message "Matching review should count generated slots."
Assert-Equal -Actual ([bool]$matchingReviewJson.changed) -Expected $false `
    -Message "Matching review should be unchanged."

$driftResult = Invoke-TemplateSchemaBaselineScript -Arguments (
    $commonArguments + @(
        "-SchemaFile", $driftSchema,
        "-ReviewJsonOutput", $driftReview
    )
)
Assert-Equal -Actual $driftResult.ExitCode -Expected 1 `
    -Message "Drifting template schema baseline should fail. Output: $($driftResult.Text)"
Assert-ContainsText -Text $driftResult.Text -ExpectedText "matches: False" `
    -Message "Drift run should report matches=False."
Assert-ContainsText -Text $driftResult.Text -ExpectedText "schema_patch_review_output_path: $driftReview" `
    -Message "Drift run should report the review JSON path."
Assert-True -Condition (Test-Path -LiteralPath $driftReview) `
    -Message "Drift run should write the schema patch review JSON."

$driftReviewJson = Get-Content -Raw -Encoding UTF8 -LiteralPath $driftReview | ConvertFrom-Json
Assert-Equal -Actual ([int]$driftReviewJson.baseline_slot_count) -Expected 1 `
    -Message "Drift review should count baseline slots."
Assert-Equal -Actual ([int]$driftReviewJson.generated_slot_count) -Expected 5 `
    -Message "Drift review should count generated slots."
Assert-Equal -Actual ([int]$driftReviewJson.patch.upsert_slot_count) -Expected 4 `
    -Message "Drift review should report generated upsert slots."
Assert-Equal -Actual ([int]$driftReviewJson.preview.inserted_slots) -Expected 4 `
    -Message "Drift review should preview inserted slots."
Assert-Equal -Actual ([bool]$driftReviewJson.changed) -Expected $true `
    -Message "Drift review should be changed."

Write-Host "Template schema baseline review JSON regression passed."
