param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_docx_functional_smoke_readiness.ps1"
$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
foreach ($marker in @(
        "featherdoc.docx_functional_smoke_readiness.v1",
        "persisted_docx_functional_smoke_evidence_only",
        "docx_functional_smoke_readiness_trace",
        "word_visual_smoke.pending_manual_review",
        "docx_package_integrity",
        "word_visual_smoke_reused_evidence",
        "content_controls",
        "sections_headers_footers"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText $marker `
        -Message "DOCX functional smoke readiness script should preserve marker '$marker'."
}

$summaryPath = Join-Path $resolvedWorkingDir "docx-functional-smoke-readiness.summary.json"
& $scriptPath -RepoRoot $resolvedRepoRoot -OutputJson $summaryPath
$lastExitCodeValue = Get-Variable -Name LASTEXITCODE -ValueOnly -ErrorAction SilentlyContinue
$exitCode = if ($null -eq $lastExitCodeValue) { 0 } else { [int]$lastExitCodeValue }
if ($exitCode -ne 0) {
    throw "check_docx_functional_smoke_readiness.ps1 failed with exit code $exitCode."
}
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "DOCX functional smoke readiness summary was not written."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.docx_functional_smoke_readiness.v1" `
    -Message "Summary schema mismatch."
Assert-Equal -Actual ([string]$summary.status) -Expected "pass" `
    -Message "Summary status should pass when persisted DOCX evidence is coherent."
Assert-Equal -Actual ([string]$summary.verdict) -Expected "pass_with_warnings" `
    -Message "Summary verdict should keep pending manual visual review as a warning."
Assert-True -Condition ([bool]$summary.docx_functional_smoke_ready) `
    -Message "DOCX functional smoke should be ready."
Assert-Equal -Actual ([int]$summary.failed_check_count) -Expected 0 `
    -Message "DOCX functional smoke should not have failed checks."
Assert-True -Condition ([int]$summary.warning_count -ge 1) `
    -Message "Pending manual Word visual review should be surfaced as a warning."
Assert-Equal -Actual ([int]$summary.capability_count) -Expected 8 `
    -Message "Capability matrix count mismatch."
Assert-Equal -Actual ([int]$summary.capability_pass_count) -Expected 8 `
    -Message "Every DOCX capability row should pass."
Assert-True -Condition (@($summary.visual_smoke_reused_evidence).Count -ge 2) `
    -Message "Persisted Word visual smoke evidence should include minimal and rerun roots."

foreach ($visualEvidence in @($summary.visual_smoke_reused_evidence)) {
    Assert-Equal -Actual ([string]$visualEvidence.status) -Expected "pass" `
        -Message "Persisted Word visual smoke evidence should pass non-empty checks."
    Assert-True -Condition ([bool]$visualEvidence.contact_sheet_non_empty_visual) `
        -Message "Contact sheet should be non-empty."
    Assert-True -Condition ([bool]$visualEvidence.page_image_non_empty_visual) `
        -Message "Page PNG should be non-empty."
}

Write-Host "DOCX functional smoke readiness regression passed."
