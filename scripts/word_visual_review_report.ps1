function Normalize-WordVisualReviewVerdict {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "undecided"
    }

    switch ($Value.ToLowerInvariant()) {
        "pass" { return "pass" }
        "fail" { return "fail" }
        "undetermined" { return "undetermined" }
        "pending_manual_review" { return "pending_manual_review" }
        "undecided" { return "undecided" }
        default { throw "Unsupported Word visual review verdict: $Value" }
    }
}

function Get-WordVisualReviewStatus {
    param([string]$Verdict)

    $normalizedVerdict = Normalize-WordVisualReviewVerdict -Value $Verdict
    if ($normalizedVerdict -eq "pass" -or
        $normalizedVerdict -eq "fail" -or
        $normalizedVerdict -eq "undetermined") {
        return "reviewed"
    }

    return "pending_review"
}

function Get-WordVisualReviewDisplayVerdict {
    param([string]$Verdict)

    $normalizedVerdict = Normalize-WordVisualReviewVerdict -Value $Verdict
    if ($normalizedVerdict -eq "undecided") {
        return "pending_manual_review"
    }

    return $normalizedVerdict
}

function New-WordVisualReviewResult {
    param(
        [string]$DocumentPath,
        [string]$PdfPath,
        [string]$EvidenceDir,
        [string]$ReportDir,
        [string]$RepairDir,
        [string]$GeneratedAt,
        [string]$ReviewVerdict,
        [int]$PageCount,
        [string]$SummaryPath,
        [string]$ChecklistPath,
        [string]$ContactSheetPath,
        [string[]]$PageImages,
        [string[]]$Notes,
        [string]$ReviewNote = ""
    )

    $normalizedVerdict = Normalize-WordVisualReviewVerdict -Value $ReviewVerdict
    $status = Get-WordVisualReviewStatus -Verdict $normalizedVerdict
    $reviewNotes = @()
    if ($null -ne $Notes) {
        $reviewNotes = @($Notes)
    }
    if (-not [string]::IsNullOrWhiteSpace($ReviewNote)) {
        $reviewNotes += $ReviewNote
    }

    $result = [ordered]@{
        document_path = $DocumentPath
        pdf_path = $PdfPath
        evidence_dir = $EvidenceDir
        report_dir = $ReportDir
        repair_dir = $RepairDir
        generated_at = $GeneratedAt
        status = $status
        verdict = $normalizedVerdict
        page_count = $PageCount
        evidence = [ordered]@{
            summary_json = $SummaryPath
            checklist = $ChecklistPath
            contact_sheet = $ContactSheetPath
            page_images = @($PageImages)
        }
        findings = @()
        notes = $reviewNotes
    }

    if ($status -eq "reviewed") {
        $result.reviewed_at = $GeneratedAt
        $result.review_method = "operator_supplied"
    }
    if (-not [string]::IsNullOrWhiteSpace($ReviewNote)) {
        $result.review_note = $ReviewNote
    }

    return [pscustomobject]$result
}

function New-WordVisualFinalReviewMarkdown {
    param(
        [string]$DocumentPath,
        [string]$PdfPath,
        [string]$EvidenceDir,
        [string]$ReportDir,
        [string]$RepairDir,
        [string]$GeneratedAt,
        [string]$ReviewVerdict,
        [string]$ReviewNote = ""
    )

    $normalizedVerdict = Normalize-WordVisualReviewVerdict -Value $ReviewVerdict
    $status = Get-WordVisualReviewStatus -Verdict $normalizedVerdict
    $displayVerdict = Get-WordVisualReviewDisplayVerdict -Verdict $normalizedVerdict
    $summaryVerdict = if ($status -eq "reviewed") { $displayVerdict } else { "" }
    $summaryNotes = if ([string]::IsNullOrWhiteSpace($ReviewNote)) { "" } else { $ReviewNote }

    return @"
# Word Visual Review

- Document: $DocumentPath
- PDF: $PdfPath
- Evidence directory: $EvidenceDir
- Report directory: $ReportDir
- Repair directory: $RepairDir
- Generated at: $GeneratedAt
- Current status: $status
- Verdict: $displayVerdict

## Summary

- Verdict: $summaryVerdict
- Notes: $summaryNotes

## Findings

- Page:
  Element type:
  Description:
  Severity:
  Screenshot evidence:
  Confidence:

## Repair Decision

- Enter repair loop:
- Suggested fix:
- Current best candidate:
- Notes:
"@
}
