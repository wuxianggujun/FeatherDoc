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
$docxPath = Join-Path $documentRoot "table_visual_smoke.docx"
$pdfPath = Join-Path $documentRoot "table_visual_smoke.pdf"
$summaryPath = Join-Path $reportDir "summary.json"
$reviewChecklistPath = Join-Path $reportDir "review_checklist.md"
$sourceReviewResultPath = Join-Path $reportDir "review_result.json"
$sourceFinalReviewPath = Join-Path $reportDir "final_review.md"
$contactSheetPath = Join-Path $evidenceDir "contact_sheet.png"
$page01Path = Join-Path $pagesDir "page-01.png"
$page02Path = Join-Path $pagesDir "page-02.png"

New-Item -ItemType Directory -Path $pagesDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $taskRoot -Force | Out-Null

Set-Content -LiteralPath $docxPath -Encoding UTF8 -Value "docx"
Set-Content -LiteralPath $pdfPath -Encoding UTF8 -Value "pdf"
Set-Content -LiteralPath $contactSheetPath -Encoding UTF8 -Value "png"
Set-Content -LiteralPath $page01Path -Encoding UTF8 -Value "png"
Set-Content -LiteralPath $page02Path -Encoding UTF8 -Value "png"
(@{
    page_count = 2
    pages = @($page01Path, $page02Path)
} | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Set-Content -LiteralPath $reviewChecklistPath -Encoding UTF8 -Value "# checklist"
Set-Content -LiteralPath $sourceReviewResultPath -Encoding UTF8 -Value "{`"status`":`"pending_review`"}"
Set-Content -LiteralPath $sourceFinalReviewPath -Encoding UTF8 -Value "# final"

& $prepareScript `
    -DocxPath $docxPath `
    -TaskOutputRoot $taskRoot `
    -Mode review-only

$latestPointerPath = Join-Path $taskRoot "latest_document_task.json"
Assert-True -Condition (Test-Path -LiteralPath $latestPointerPath) `
    -Message "Latest document task pointer was not created."

$pointer = Get-Content -Raw -LiteralPath $latestPointerPath | ConvertFrom-Json
$taskDir = $pointer.task_dir
$manifestPath = Join-Path $taskDir "task_manifest.json"
$promptPath = Join-Path $taskDir "task_prompt.md"
$copiedPdfPath = Join-Path $taskDir "table_visual_smoke.pdf"
$copiedContactSheetPath = Join-Path $taskDir "evidence\contact_sheet.png"
$copiedPage01Path = Join-Path $taskDir "evidence\pages\page-01.png"
$copiedSummaryPath = Join-Path $taskDir "report\summary.json"
$copiedReviewChecklistPath = Join-Path $taskDir "report\review_checklist.md"
$seedReviewResultPath = Join-Path $taskDir "report\review_result.json"
$copiedSourceReviewResultPath = Join-Path $taskDir "report\source_visual_review_result.json"
$copiedSourceFinalReviewPath = Join-Path $taskDir "report\source_visual_final_review.md"

foreach ($path in @(
        $taskDir,
        $manifestPath,
        $promptPath,
        $copiedPdfPath,
        $copiedContactSheetPath,
        $copiedPage01Path,
        $copiedSummaryPath,
        $copiedReviewChecklistPath,
        $seedReviewResultPath,
        $copiedSourceReviewResultPath,
        $copiedSourceFinalReviewPath
    )) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected prepared document task artifact is missing: $path"
}

$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
Assert-True -Condition ($manifest.source.kind -eq "document") `
    -Message "Prepared task manifest recorded an unexpected source kind."
Assert-True -Condition ($null -ne $manifest.document_visual_artifacts) `
    -Message "Prepared task manifest did not include document_visual_artifacts metadata."
Assert-True -Condition ([System.IO.Path]::GetFullPath($manifest.document_visual_artifacts.copied_contact_sheet_path) -eq [System.IO.Path]::GetFullPath($copiedContactSheetPath)) `
    -Message "Prepared task manifest recorded an unexpected copied contact sheet path."
Assert-True -Condition ([System.IO.Path]::GetFullPath($manifest.document_visual_artifacts.copied_summary_path) -eq [System.IO.Path]::GetFullPath($copiedSummaryPath)) `
    -Message "Prepared task manifest recorded an unexpected copied summary path."

Assert-Contains -Path $promptPath -ExpectedText "优先复用这些现成证据" -Label "task_prompt.md"

Write-Host "Prepare document review task regression passed."
