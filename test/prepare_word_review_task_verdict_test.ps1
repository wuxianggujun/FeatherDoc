param(
    [string]$RepoRoot,
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

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$documentRoot = Join-Path $resolvedWorkingDir "document-source"
$evidenceDir = Join-Path $documentRoot "evidence"
$pagesDir = Join-Path $evidenceDir "pages"
$reportDir = Join-Path $documentRoot "report"
$taskRoot = Join-Path $resolvedWorkingDir "tasks"
$prepareScript = Join-Path $resolvedRepoRoot "scripts\prepare_word_review_task.ps1"
$docxPath = Join-Path $documentRoot "reviewed.docx"
$pdfPath = Join-Path $documentRoot "reviewed.pdf"
$summaryPath = Join-Path $reportDir "summary.json"
$reviewChecklistPath = Join-Path $reportDir "review_checklist.md"
$sourceReviewResultPath = Join-Path $reportDir "review_result.json"
$sourceFinalReviewPath = Join-Path $reportDir "final_review.md"
$contactSheetPath = Join-Path $evidenceDir "contact_sheet.png"
$page01Path = Join-Path $pagesDir "page-01.png"

New-Item -ItemType Directory -Path $pagesDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $taskRoot -Force | Out-Null

Set-Content -LiteralPath $docxPath -Encoding UTF8 -Value "docx"
Set-Content -LiteralPath $pdfPath -Encoding UTF8 -Value "pdf"
Set-Content -LiteralPath $contactSheetPath -Encoding UTF8 -Value "png"
Set-Content -LiteralPath $page01Path -Encoding UTF8 -Value "png"
(@{
    page_count = 1
    pages = @($page01Path)
} | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Set-Content -LiteralPath $reviewChecklistPath -Encoding UTF8 -Value "# checklist"
Set-Content -LiteralPath $sourceReviewResultPath -Encoding UTF8 -Value "{`"status`":`"pending_review`"}"
Set-Content -LiteralPath $sourceFinalReviewPath -Encoding UTF8 -Value "# final"

& $prepareScript `
    -DocxPath $docxPath `
    -TaskOutputRoot $taskRoot `
    -Mode review-only `
    -ReviewVerdict pass `
    -ReviewNote "contact sheet reviewed"

$latestPointerPath = Join-Path $taskRoot "latest_document_task.json"
Assert-True -Condition (Test-Path -LiteralPath $latestPointerPath) `
    -Message "Latest document task pointer was not created."

$pointer = Get-Content -Raw -LiteralPath $latestPointerPath | ConvertFrom-Json
$taskDir = $pointer.task_dir
$reviewResultPath = Join-Path $taskDir "report\review_result.json"
$finalReviewPath = Join-Path $taskDir "report\final_review.md"
$reviewResult = Get-Content -Raw -LiteralPath $reviewResultPath | ConvertFrom-Json

Assert-True -Condition ($reviewResult.status -eq "reviewed") `
    -Message "Prepared review task did not mark status=reviewed."
Assert-True -Condition ($reviewResult.verdict -eq "pass") `
    -Message "Prepared review task did not record verdict=pass."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace($reviewResult.reviewed_at)) `
    -Message "Prepared review task did not record reviewed_at."
Assert-True -Condition ($reviewResult.review_method -eq "operator_supplied") `
    -Message "Prepared review task did not record review_method."
Assert-True -Condition ($reviewResult.review_note -eq "contact sheet reviewed") `
    -Message "Prepared review task did not record review_note."

Assert-Contains -Path $finalReviewPath -ExpectedText "- Verdict: pass" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "- Status: reviewed" -Label "final_review.md"
Assert-Contains -Path $finalReviewPath -ExpectedText "- Notes: contact sheet reviewed" -Label "final_review.md"

Write-Host "Prepare word review task verdict regression passed."
