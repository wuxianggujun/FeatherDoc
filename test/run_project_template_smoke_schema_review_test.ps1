param(
    [string]$RepoRoot,
    [string]$BuildDir,
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
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Invoke-ProjectTemplateSmokeScript {
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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_project_template_smoke.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$manifestPath = Join-Path $resolvedWorkingDir "schema-review.project-template-smoke.manifest.json"
$schemaPath = Join-Path $resolvedWorkingDir "drift.template-schema.json"
$outputDir = Join-Path $resolvedWorkingDir "smoke-output"
$generatedOutput = Join-Path $resolvedWorkingDir "generated.template-schema.json"

Set-Content -LiteralPath $schemaPath -Encoding UTF8 -Value `
    '{"targets":[{"part":"body","slots":[{"bookmark":"customer_name","kind":"text"}]}]}'

$manifest = [ordered]@{
    entries = @(
        [ordered]@{
            name = "schema-review-drift"
            input_docx = $sampleDocx
            schema_baseline = [ordered]@{
                schema_file = $schemaPath
                generated_output = $generatedOutput
                target_mode = "default"
            }
        }
    )
}
($manifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $manifestPath -Encoding UTF8

$result = Invoke-ProjectTemplateSmokeScript -Arguments @(
    "-ManifestPath", $manifestPath,
    "-BuildDir", $resolvedBuildDir,
    "-OutputDir", $outputDir,
    "-SkipBuild",
    "-SkipVisualSmoke"
)
Assert-Equal -Actual $result.ExitCode -Expected 1 `
    -Message "Drifting project template smoke should fail. Output: $($result.Text)"
Assert-ContainsText -Text $result.Text -ExpectedText "Summary JSON:" `
    -Message "Smoke run should report summary JSON."

$summaryPath = Join-Path $outputDir "summary.json"
$summaryMarkdownPath = Join-Path $outputDir "summary.md"
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Smoke run should write summary.json."
Assert-True -Condition (Test-Path -LiteralPath $summaryMarkdownPath) `
    -Message "Smoke run should write summary.md."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$summary.schema_patch_review_count) -Expected 1 `
    -Message "Summary should aggregate one schema patch review."
Assert-Equal -Actual ([int]$summary.schema_patch_review_changed_count) -Expected 1 `
    -Message "Summary should report one changed schema patch review."
Assert-Equal -Actual ([bool]$summary.schema_patch_reviews[0].changed) -Expected $true `
    -Message "Aggregated review should be changed."
Assert-Equal -Actual ([int]$summary.schema_patch_reviews[0].upsert_slot_count) -Expected 4 `
    -Message "Aggregated review should expose patch upsert count."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_pending_count) -Expected 1 `
    -Message "Summary should expose one pending schema patch approval."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_approved_count) -Expected 0 `
    -Message "Summary should expose zero approved schema patch approvals."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_rejected_count) -Expected 0 `
    -Message "Summary should expose zero rejected schema patch approvals."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_compliance_issue_count) -Expected 0 `
    -Message "Summary should expose zero schema patch approval compliance issues."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_invalid_result_count) -Expected 0 `
    -Message "Summary should expose zero invalid schema patch approval results."
Assert-Equal -Actual ([string]$summary.schema_patch_approval_gate_status) -Expected "pending" `
    -Message "Pending schema patch approval should make the approval gate pending."
Assert-Equal -Actual ([bool]$summary.schema_patch_approval_gate_blocked) -Expected $false `
    -Message "Pending approval should not block on compliance."
Assert-Equal -Actual ([bool]$summary.schema_patch_approval_items[0].required) -Expected $true `
    -Message "Approval item should require manual review."
Assert-Equal -Actual ([bool]$summary.schema_patch_approval_items[0].pending) -Expected $true `
    -Message "Approval item should be pending until a reviewer records a decision."
Assert-Equal -Actual ([string]$summary.schema_patch_approval_items[0].status) -Expected "pending_review" `
    -Message "Approval item should use the pending-review status."
Assert-Equal -Actual ([string]$summary.schema_patch_approval_items[0].decision) -Expected "pending" `
    -Message "Approval item should expose the pending decision."
Assert-Equal -Actual ([string]$summary.schema_patch_approval_items[0].action) -Expected "review_schema_update_candidate" `
    -Message "Approval item should explain the next reviewer action."
Assert-Equal -Actual ([int]$summary.schema_patch_approval_items[0].compliance_issue_count) -Expected 0 `
    -Message "Pending approval item should not require reviewer or reviewed_at yet."
Assert-Equal -Actual ([string]$summary.schema_patch_approval_items[0].schema_update_candidate) -Expected $generatedOutput `
    -Message "Approval item should point at the generated schema update candidate."
Assert-True -Condition (Test-Path -LiteralPath $summary.schema_patch_approval_items[0].approval_result) `
    -Message "Approval item should point at a persisted approval result file."

$entry = $summary.entries[0]
$schemaBaseline = $entry.checks.schema_baseline
Assert-True -Condition (Test-Path -LiteralPath $schemaBaseline.schema_patch_review_output_path) `
    -Message "Entry should write the schema patch review JSON."
Assert-Equal -Actual ([bool]$schemaBaseline.schema_patch_review.changed) -Expected $true `
    -Message "Entry summary should embed the compact review object."
Assert-Equal -Actual ([int]$schemaBaseline.schema_patch_review.inserted_slots) -Expected 4 `
    -Message "Entry compact review should expose inserted slots."
Assert-Equal -Actual ([bool]$schemaBaseline.schema_patch_approval.required) -Expected $true `
    -Message "Entry should embed the schema patch approval summary."
Assert-Equal -Actual ([string]$schemaBaseline.schema_patch_approval.status) -Expected "pending_review" `
    -Message "Entry approval should require review for drift."
Assert-Equal -Actual ([string]$schemaBaseline.schema_patch_approval.decision) -Expected "pending" `
    -Message "Entry approval should record the pending decision."
Assert-True -Condition (Test-Path -LiteralPath $schemaBaseline.schema_patch_approval.schema_update_candidate) `
    -Message "Entry approval should point to the generated schema candidate."
Assert-True -Condition (Test-Path -LiteralPath $schemaBaseline.schema_patch_approval_result_path) `
    -Message "Entry should write a persisted schema patch approval result file."
Assert-Equal -Actual ([string]$schemaBaseline.schema_patch_approval_result.schema) -Expected "featherdoc.template_schema_patch_approval_result.v1" `
    -Message "Entry approval result should use the stable schema id."
Assert-Equal -Actual ([string]$schemaBaseline.schema_patch_approval_result.decision) -Expected "pending" `
    -Message "Entry approval result should default to a pending decision."

$review = Get-Content -Raw -Encoding UTF8 -LiteralPath $schemaBaseline.schema_patch_review_output_path | ConvertFrom-Json
Assert-Equal -Actual ([string]$review.schema) -Expected "featherdoc.template_schema_patch_review.v1" `
    -Message "Review JSON should use the stable schema id."
Assert-Equal -Actual ([int]$review.generated_slot_count) -Expected 5 `
    -Message "Review JSON should include generated slot count."

$summaryMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryMarkdownPath
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch reviews: 1" `
    -Message "Markdown summary should include review count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approvals pending: 1" `
    -Message "Markdown summary should include approval count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approvals approved: 0" `
    -Message "Markdown summary should include approved approval count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval compliance issues: 0" `
    -Message "Markdown summary should include approval compliance issue count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval invalid results: 0" `
    -Message "Markdown summary should include invalid approval-result count."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval gate status: pending" `
    -Message "Markdown summary should include pending approval gate status."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema Patch Approvals" `
    -Message "Markdown summary should include approval section."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch review: changed=True" `
    -Message "Markdown summary should include entry review details."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Schema patch approval: status=pending_review" `
    -Message "Markdown summary should include entry approval details."

Write-Host "Project template smoke schema review aggregation regression passed."
