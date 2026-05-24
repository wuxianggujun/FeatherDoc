Set-StrictMode -Version Latest

function Convert-VisualReviewTimestamp {
    param($Value)

    if ($null -eq $Value) {
        return ""
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$Value
}

function Get-OptionalPropertyRawValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-OptionalPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-OptionalPropertyRawValue -Object $Object -Name $Name
    if ($null -eq $value) {
        return ""
    }

    return [string]$value
}

function Get-OptionalPropertyObject {
    param(
        $Object,
        [string]$Name
    )

    return Get-OptionalPropertyRawValue -Object $Object -Name $Name
}

function Get-SupersededReviewTasksReportPath {
    param(
        $Summary,
        $VisualGateSummary
    )

    $reportPath = Get-OptionalPropertyValue -Object $VisualGateSummary -Name "superseded_review_tasks_report"
    if (-not [string]::IsNullOrWhiteSpace($reportPath)) {
        return $reportPath
    }

    return Get-OptionalPropertyValue -Object $Summary -Name "superseded_review_tasks_report"
}

function Get-SupersededReviewTaskCount {
    param([string]$ReportPath)

    if ([string]::IsNullOrWhiteSpace($ReportPath) -or -not (Test-Path -LiteralPath $ReportPath)) {
        return ""
    }

    try {
        $report = Get-Content -Raw -LiteralPath $ReportPath | ConvertFrom-Json
    } catch {
        return ""
    }

    return Get-OptionalPropertyValue -Object $report -Name "superseded_task_count"
}

function Get-VisualTaskVerdict {
    param(
        $VisualGateSummary,
        $GateSummary,
        [string]$TaskKey
    )

    $summaryVerdict = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_verdict" -f $TaskKey)
    if (-not [string]::IsNullOrWhiteSpace($summaryVerdict)) {
        return $summaryVerdict
    }

    $gateFlow = Get-OptionalPropertyObject -Object $GateSummary -Name $TaskKey
    $gateFlowVerdict = Get-OptionalPropertyValue -Object $gateFlow -Name "review_verdict"
    if (-not [string]::IsNullOrWhiteSpace($gateFlowVerdict)) {
        return $gateFlowVerdict
    }

    return ""
}

function Get-VisualTaskReviewStatus {
    param(
        $VisualGateSummary,
        $GateSummary,
        [string]$TaskKey
    )

    $summaryStatus = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_review_status" -f $TaskKey)
    if (-not [string]::IsNullOrWhiteSpace($summaryStatus)) {
        return $summaryStatus
    }

    $gateFlow = Get-OptionalPropertyObject -Object $GateSummary -Name $TaskKey
    return Get-OptionalPropertyValue -Object $gateFlow -Name "review_status"
}

function Get-VisualTaskReviewNote {
    param(
        $VisualGateSummary,
        $GateSummary,
        [string]$TaskKey
    )

    $summaryNote = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_review_note" -f $TaskKey)
    if (-not [string]::IsNullOrWhiteSpace($summaryNote)) {
        return $summaryNote
    }

    $gateFlow = Get-OptionalPropertyObject -Object $GateSummary -Name $TaskKey
    return Get-OptionalPropertyValue -Object $gateFlow -Name "review_note"
}

function Get-VisualTaskReviewedAt {
    param(
        $VisualGateSummary,
        $GateSummary,
        [string]$TaskKey
    )

    $summaryReviewedAt = Get-OptionalPropertyRawValue -Object $VisualGateSummary -Name ("{0}_reviewed_at" -f $TaskKey)
    $summaryReviewedAt = Convert-VisualReviewTimestamp -Value $summaryReviewedAt
    if (-not [string]::IsNullOrWhiteSpace($summaryReviewedAt)) {
        return $summaryReviewedAt
    }

    $gateFlow = Get-OptionalPropertyObject -Object $GateSummary -Name $TaskKey
    $gateReviewedAt = Get-OptionalPropertyRawValue -Object $gateFlow -Name "reviewed_at"
    return Convert-VisualReviewTimestamp -Value $gateReviewedAt
}

function Get-VisualTaskReviewMethod {
    param(
        $VisualGateSummary,
        $GateSummary,
        [string]$TaskKey
    )

    $summaryReviewMethod = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_review_method" -f $TaskKey)
    if (-not [string]::IsNullOrWhiteSpace($summaryReviewMethod)) {
        return $summaryReviewMethod
    }

    $gateFlow = Get-OptionalPropertyObject -Object $GateSummary -Name $TaskKey
    return Get-OptionalPropertyValue -Object $gateFlow -Name "review_method"
}

function Get-CompleteVisualReviewTaskSummary {
    param($Summary)

    if ($null -eq $Summary) {
        return $null
    }

    $totalCount = Get-OptionalPropertyValue -Object $Summary -Name "total_count"
    $standardCount = Get-OptionalPropertyValue -Object $Summary -Name "standard_count"
    $curatedCount = Get-OptionalPropertyValue -Object $Summary -Name "curated_count"
    if ([string]::IsNullOrWhiteSpace($totalCount) -or
        [string]::IsNullOrWhiteSpace($standardCount) -or
        [string]::IsNullOrWhiteSpace($curatedCount)) {
        return $null
    }

    return [pscustomobject]@{
        total_count = $totalCount
        standard_count = $standardCount
        curated_count = $curatedCount
    }
}

function Get-VisualReviewTaskSummaryLine {
    param(
        $VisualGateSummary,
        $GateSummary
    )

    $summaryCandidates = @(
        (Get-OptionalPropertyObject -Object $VisualGateSummary -Name "review_task_summary"),
        (Get-OptionalPropertyObject -Object $GateSummary -Name "review_task_summary")
    )

    foreach ($summaryCandidate in $summaryCandidates) {
        $summary = Get-CompleteVisualReviewTaskSummary -Summary $summaryCandidate
        if ($null -ne $summary) {
            return "Review task count: {0} total ({1} standard, {2} curated)" -f `
                $summary.total_count,
                $summary.standard_count,
                $summary.curated_count
        }
    }

    return ""
}

function Get-PdfVisualGateSummaryPath {
    param($Summary)

    $steps = Get-OptionalPropertyObject -Object $Summary -Name "steps"
    $stepSummary = Get-OptionalPropertyObject -Object $steps -Name "pdf_visual_gate"
    $topLevelSummary = Get-OptionalPropertyObject -Object $Summary -Name "pdf_visual_gate"

    foreach ($candidate in @(
            (Get-OptionalPropertyValue -Object $stepSummary -Name "summary_json"),
            (Get-OptionalPropertyValue -Object $topLevelSummary -Name "summary_json"),
            (Get-OptionalPropertyValue -Object $Summary -Name "pdf_visual_gate_summary_json")
        )) {
        if (-not [string]::IsNullOrWhiteSpace($candidate)) {
            return $candidate
        }
    }

    return ""
}

function Get-PdfVisualGateEvidence {
    param([string]$SummaryPath)

    $evidence = [ordered]@{
        summary_json = $SummaryPath
        status = if ([string]::IsNullOrWhiteSpace($SummaryPath)) { "" } else { "missing" }
        verdict = ""
        aggregate_contact_sheet = ""
        cjk_manifest_count = ""
        cjk_copy_search_count = ""
        cjk_missing_text_count = ""
        visual_baseline_manifest_count = ""
        visual_baseline_count = ""
        pdf_cli_export_log = ""
        pdf_regression_log = ""
        cjk_copy_search_log_dir = ""
        unicode_font_log = ""
        error = ""
    }

    if ([string]::IsNullOrWhiteSpace($SummaryPath)) {
        return [pscustomobject]$evidence
    }

    if (-not (Test-Path -LiteralPath $SummaryPath)) {
        return [pscustomobject]$evidence
    }

    try {
        $summary = Get-Content -Raw -LiteralPath $SummaryPath | ConvertFrom-Json
    } catch {
        $evidence.status = "unreadable"
        $evidence.error = $_.Exception.Message
        return [pscustomobject]$evidence
    }

    $evidence.status = "loaded"
    $evidence.verdict = Get-OptionalPropertyValue -Object $summary -Name "verdict"
    $evidence.aggregate_contact_sheet = Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet"
    $evidence.cjk_manifest_count = Get-OptionalPropertyValue -Object $summary -Name "cjk_manifest_count"
    $evidence.visual_baseline_manifest_count = Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_manifest_count"

    $logs = Get-OptionalPropertyObject -Object $summary -Name "logs"
    $evidence.pdf_cli_export_log = Get-OptionalPropertyValue -Object $logs -Name "pdf_cli_export"
    $evidence.pdf_regression_log = Get-OptionalPropertyValue -Object $logs -Name "pdf_regression"
    $evidence.cjk_copy_search_log_dir = Get-OptionalPropertyValue -Object $logs -Name "cjk_copy_search"
    $evidence.unicode_font_log = Get-OptionalPropertyValue -Object $logs -Name "unicode_font"

    $copySearchEntries = @(Get-OptionalPropertyArray -Object $summary -Name "cjk_copy_search")
    $baselineEntries = @(Get-OptionalPropertyArray -Object $summary -Name "baselines")
    $missingTextCount = 0
    foreach ($entry in $copySearchEntries) {
        $missingTextCount += @(Get-OptionalPropertyArray -Object $entry -Name "missing_text").Count
    }

    $copySearchCount = if ($copySearchEntries.Count -gt 0) {
        [string]$copySearchEntries.Count
    } else {
        Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_count"
    }
    $baselineCount = if ($baselineEntries.Count -gt 0) {
        [string]$baselineEntries.Count
    } else {
        Get-OptionalPropertyValue -Object $summary -Name "baselines_count"
    }

    $evidence.cjk_copy_search_count = [string]$copySearchCount
    $evidence.cjk_missing_text_count = [string]$missingTextCount
    $evidence.visual_baseline_count = [string]$baselineCount

    return [pscustomobject]$evidence
}

function Get-OptionalPropertyArray {
    param(
        $Object,
        [string]$Name
    )

    $propertyValue = Get-OptionalPropertyObject -Object $Object -Name $Name
    if ($null -eq $propertyValue) {
        return @()
    }

    return @($propertyValue)
}

function Get-OrCreateCuratedVisualReviewEntry {
    param(
        [hashtable]$EntryMap,
        [System.Collections.Generic.List[string]]$EntryOrder,
        $Source,
        [int]$FallbackIndex
    )

    $id = Get-OptionalPropertyValue -Object $Source -Name "id"
    $displayLabel = Get-OptionalPropertyValue -Object $Source -Name "display_label"
    $label = if (-not [string]::IsNullOrWhiteSpace($displayLabel)) {
        $displayLabel
    } else {
        Get-OptionalPropertyValue -Object $Source -Name "label"
    }

    $key = if (-not [string]::IsNullOrWhiteSpace($id)) {
        $id
    } elseif (-not [string]::IsNullOrWhiteSpace($label) -and $label -notlike "curated:*") {
        $label
    } else {
        "__curated_{0}" -f $FallbackIndex
    }

    if (-not $EntryMap.ContainsKey($key)) {
        $EntryMap[$key] = [ordered]@{
            id = ""
            label = ""
            verdict = ""
            task_dir = ""
            review_result_path = ""
            final_review_path = ""
            review_status = ""
            reviewed_at = ""
            review_method = ""
            review_note = ""
        }
        [void]$EntryOrder.Add($key)
    }

    return $EntryMap[$key]
}

function Merge-CuratedVisualReviewEntry {
    param(
        $Entry,
        $Source
    )

    if ($null -eq $Source) {
        return
    }

    $id = Get-OptionalPropertyValue -Object $Source -Name "id"
    if (-not [string]::IsNullOrWhiteSpace($id)) {
        $Entry.id = $id
    }

    $displayLabel = Get-OptionalPropertyValue -Object $Source -Name "display_label"
    $label = if (-not [string]::IsNullOrWhiteSpace($displayLabel)) {
        $displayLabel
    } else {
        Get-OptionalPropertyValue -Object $Source -Name "label"
    }
    if (-not [string]::IsNullOrWhiteSpace($label) -and $label -notlike "curated:*") {
        $Entry.label = $label
    }

    $verdict = Get-OptionalPropertyValue -Object $Source -Name "review_verdict"
    if (-not [string]::IsNullOrWhiteSpace($verdict)) {
        $Entry.verdict = $verdict
    }

    $reviewStatus = Get-OptionalPropertyValue -Object $Source -Name "review_status"
    if (-not [string]::IsNullOrWhiteSpace($reviewStatus)) {
        $Entry.review_status = $reviewStatus
    }

    $reviewedAt = Get-OptionalPropertyRawValue -Object $Source -Name "reviewed_at"
    $reviewedAt = Convert-VisualReviewTimestamp -Value $reviewedAt
    if (-not [string]::IsNullOrWhiteSpace($reviewedAt)) {
        $Entry.reviewed_at = $reviewedAt
    }

    $reviewMethod = Get-OptionalPropertyValue -Object $Source -Name "review_method"
    if (-not [string]::IsNullOrWhiteSpace($reviewMethod)) {
        $Entry.review_method = $reviewMethod
    }

    $reviewNote = Get-OptionalPropertyValue -Object $Source -Name "review_note"
    if (-not [string]::IsNullOrWhiteSpace($reviewNote)) {
        $Entry.review_note = $reviewNote
    }

    $taskInfo = Get-OptionalPropertyObject -Object $Source -Name "task"
    if ($null -eq $taskInfo) {
        $taskInfo = $Source
    }

    $taskDir = Get-OptionalPropertyValue -Object $taskInfo -Name "task_dir"
    if (-not [string]::IsNullOrWhiteSpace($taskDir)) {
        $Entry.task_dir = $taskDir
    }

    $reviewResultPath = Get-OptionalPropertyValue -Object $taskInfo -Name "review_result_path"
    if (-not [string]::IsNullOrWhiteSpace($reviewResultPath)) {
        $Entry.review_result_path = $reviewResultPath
    }

    $finalReviewPath = Get-OptionalPropertyValue -Object $taskInfo -Name "final_review_path"
    if (-not [string]::IsNullOrWhiteSpace($finalReviewPath)) {
        $Entry.final_review_path = $finalReviewPath
    }
}

function Get-CuratedVisualReviewEntries {
    param(
        $VisualGateSummary,
        $GateSummary
    )

    $entryMap = @{}
    $entryOrder = New-Object 'System.Collections.Generic.List[string]'
    $fallbackIndex = 0

    $reviewTasks = Get-OptionalPropertyObject -Object $GateSummary -Name "review_tasks"
    $manualReview = Get-OptionalPropertyObject -Object $GateSummary -Name "manual_review"
    $manualTasks = Get-OptionalPropertyObject -Object $manualReview -Name "tasks"

    $sources = @(
        (Get-OptionalPropertyArray -Object $VisualGateSummary -Name "curated_visual_regressions"),
        (Get-OptionalPropertyArray -Object $reviewTasks -Name "curated_visual_regressions"),
        (Get-OptionalPropertyArray -Object $manualTasks -Name "curated_visual_regressions"),
        (Get-OptionalPropertyArray -Object $GateSummary -Name "curated_visual_regressions")
    )

    foreach ($sourceGroup in $sources) {
        foreach ($source in $sourceGroup) {
            $fallbackIndex += 1
            $entry = Get-OrCreateCuratedVisualReviewEntry `
                -EntryMap $entryMap `
                -EntryOrder $entryOrder `
                -Source $source `
                -FallbackIndex $fallbackIndex
            Merge-CuratedVisualReviewEntry -Entry $entry -Source $source
        }
    }

    $entries = @()
    foreach ($key in $entryOrder) {
        $entry = $entryMap[$key]
        if ([string]::IsNullOrWhiteSpace($entry.label)) {
            if (-not [string]::IsNullOrWhiteSpace($entry.id)) {
                $entry.label = $entry.id
            } else {
                $entry.label = "Curated visual regression bundle"
            }
        }

        $entries += [pscustomobject]$entry
    }

    return $entries
}
