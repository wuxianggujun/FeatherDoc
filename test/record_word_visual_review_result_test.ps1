param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}
if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\record_word_visual_review_result_test"
}

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

function Write-TestPng {
    param([string]$Path)

    Add-Type -AssemblyName System.Drawing
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    $bitmap = [System.Drawing.Bitmap]::new(16, 16)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.Clear([System.Drawing.Color]::White)
            $graphics.FillRectangle([System.Drawing.Brushes]::Black, 2, 2, 8, 8)
        } finally {
            $graphics.Dispose()
        }
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $bitmap.Dispose()
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$evidenceDir = Join-Path $resolvedWorkingDir "evidence"
$pageDir = Join-Path $evidenceDir "pages"
$reportDir = Join-Path $resolvedWorkingDir "report"
$contactSheet = Join-Path $evidenceDir "contact_sheet.png"
$pageImage = Join-Path $pageDir "page-01.png"
$reviewResult = Join-Path $reportDir "review_result.json"
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
Write-TestPng -Path $contactSheet
Write-TestPng -Path $pageImage

([ordered]@{
        document_path = Join-Path $resolvedWorkingDir "input.docx"
        pdf_path = Join-Path $resolvedWorkingDir "input.pdf"
        evidence_dir = $evidenceDir
        report_dir = $reportDir
        repair_dir = Join-Path $resolvedWorkingDir "repair"
        generated_at = "2026-05-27T00:00:00"
        status = "pending_review"
        verdict = "pending_manual_review"
        page_count = 1
        evidence = [ordered]@{
            summary_json = Join-Path $reportDir "summary.json"
            checklist = Join-Path $reportDir "review_checklist.md"
            contact_sheet = $contactSheet
            page_images = @($pageImage)
        }
        findings = @()
        notes = @("fixture")
    } | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $reviewResult -Encoding UTF8

$scriptPath = Join-Path $resolvedRepoRoot "scripts\record_word_visual_review_result.ps1"
$output = @(& powershell -NoProfile -ExecutionPolicy Bypass -File $scriptPath `
        -ReviewResultJson $reviewResult `
        -Verdict pass `
        -Reviewer "test-reviewer" `
        -ReviewNote "Reviewed fixture evidence." `
        -RequireNonEmptyEvidence 2>&1)
if ($LASTEXITCODE -ne 0) {
    throw "record_word_visual_review_result.ps1 failed: $($output -join [System.Environment]::NewLine)"
}

$summary = ($output -join [System.Environment]::NewLine) | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.word_visual_review_result_record.v1" `
    -Message "Recorder should expose a stable schema."
Assert-Equal -Actual ([int]$summary.updated_review_count) -Expected 1 `
    -Message "Recorder should update one review result."

$review = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewResult | ConvertFrom-Json
Assert-Equal -Actual ([string]$review.status) -Expected "reviewed" `
    -Message "Review status should become reviewed."
Assert-Equal -Actual ([string]$review.verdict) -Expected "pass" `
    -Message "Review verdict should be recorded."
Assert-Equal -Actual ([string]$review.review_method) -Expected "test-reviewer" `
    -Message "Review method should preserve the reviewer label."
Assert-True -Condition ([bool]$review.visual_evidence_check.contact_sheet.non_empty_visual) `
    -Message "Recorder should sample a non-empty contact sheet."
Assert-True -Condition ([bool]$review.visual_evidence_check.page_images[0].non_empty_visual) `
    -Message "Recorder should sample a non-empty page image."

$finalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $reportDir "final_review.md")
Assert-True -Condition ($finalReview -match "Current status: reviewed") `
    -Message "Final review should be refreshed with reviewed status."
Assert-True -Condition ($finalReview -match "Verdict: pass") `
    -Message "Final review should be refreshed with pass verdict."

Write-Host "Word visual review result recorder regression passed."
