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

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Actual=[$Actual] Expected=[$Expected]"
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

. (Join-Path $resolvedRepoRoot "scripts\word_visual_review_report.ps1")

$generatedAt = "2026-04-28T12:00:00"
$pendingResult = New-WordVisualReviewResult `
    -DocumentPath "document.docx" `
    -PdfPath "document.pdf" `
    -EvidenceDir "evidence" `
    -ReportDir "report" `
    -RepairDir "repair" `
    -GeneratedAt $generatedAt `
    -ReviewVerdict "undecided" `
    -PageCount 2 `
    -SummaryPath "report\summary.json" `
    -ChecklistPath "report\review_checklist.md" `
    -ContactSheetPath "evidence\contact_sheet.png" `
    -PageImages @("evidence\pages\page-01.png", "evidence\pages\page-02.png") `
    -Notes @("needs screenshot review")

Assert-Equal -Actual $pendingResult.status -Expected "pending_review" `
    -Message "Default review result should stay pending."
Assert-Equal -Actual $pendingResult.verdict -Expected "undecided" `
    -Message "Default review result should preserve the legacy undecided verdict."
Assert-True -Condition (-not ($pendingResult.PSObject.Properties.Name -contains "reviewed_at")) `
    -Message "Pending review result should not record reviewed_at."

$passResult = New-WordVisualReviewResult `
    -DocumentPath "document.docx" `
    -PdfPath "document.pdf" `
    -EvidenceDir "evidence" `
    -ReportDir "report" `
    -RepairDir "repair" `
    -GeneratedAt $generatedAt `
    -ReviewVerdict "pass" `
    -PageCount 1 `
    -SummaryPath "report\summary.json" `
    -ChecklistPath "report\review_checklist.md" `
    -ContactSheetPath "evidence\contact_sheet.png" `
    -PageImages @("evidence\pages\page-01.png") `
    -Notes @("checked contact sheet") `
    -ReviewNote "visual evidence checked"

Assert-Equal -Actual $passResult.status -Expected "reviewed" `
    -Message "Pass verdict should mark the review as reviewed."
Assert-Equal -Actual $passResult.verdict -Expected "pass" `
    -Message "Pass verdict should be recorded for verdict sync."
Assert-Equal -Actual $passResult.reviewed_at -Expected $generatedAt `
    -Message "Reviewed result should carry reviewed_at metadata."
Assert-Equal -Actual $passResult.review_method -Expected "operator_supplied" `
    -Message "Reviewed result should identify the review method."
Assert-Equal -Actual $passResult.review_note -Expected "visual evidence checked" `
    -Message "Reviewed result should carry the supplied review note."
Assert-ContainsText -Text (($passResult.notes) -join "`n") `
    -ExpectedText "visual evidence checked" `
    -Message "Review note should also be included in notes for readers."

$passMarkdown = New-WordVisualFinalReviewMarkdown `
    -DocumentPath "document.docx" `
    -PdfPath "document.pdf" `
    -EvidenceDir "evidence" `
    -ReportDir "report" `
    -RepairDir "repair" `
    -GeneratedAt $generatedAt `
    -ReviewVerdict "pass" `
    -ReviewNote "visual evidence checked"

Assert-ContainsText -Text $passMarkdown -ExpectedText "- Current status: reviewed" `
    -Message "Final review should show reviewed status for pass verdicts."
Assert-ContainsText -Text $passMarkdown -ExpectedText "- Verdict: pass" `
    -Message "Final review should show pass verdict."
Assert-ContainsText -Text $passMarkdown -ExpectedText "- Notes: visual evidence checked" `
    -Message "Final review should show the supplied review note."

$runScript = Get-Content -Raw -LiteralPath (Join-Path $resolvedRepoRoot "scripts\run_word_visual_smoke.ps1")
Assert-ContainsText -Text $runScript -ExpectedText "[string]`$ReviewVerdict" `
    -Message "run_word_visual_smoke.ps1 should expose ReviewVerdict."
Assert-ContainsText -Text $runScript -ExpectedText "New-WordVisualReviewResult" `
    -Message "run_word_visual_smoke.ps1 should use the shared review result helper."

Write-Host "Word visual review report regression passed."
