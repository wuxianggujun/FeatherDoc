param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$script:TextFileCache = @{}
$script:LineFileCache = @{}

function Get-CachedFileText {
    param([string]$Path)

    $cacheKey = [System.IO.Path]::GetFullPath($Path)
    if (-not $script:TextFileCache.ContainsKey($cacheKey)) {
        $script:TextFileCache[$cacheKey] = Get-Content -Raw -LiteralPath $Path
    }

    return $script:TextFileCache[$cacheKey]
}

function Get-CachedFileLines {
    param([string]$Path)

    $cacheKey = [System.IO.Path]::GetFullPath($Path)
    if (-not $script:LineFileCache.ContainsKey($cacheKey)) {
        $script:LineFileCache[$cacheKey] = Get-Content -LiteralPath $Path
    }

    return $script:LineFileCache[$cacheKey]
}

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-CachedFileText -Path $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

function Assert-LineContainsAll {
    param(
        [string]$Path,
        [string[]]$Fragments,
        [string]$Label
    )

    $lines = Get-CachedFileLines -Path $Path
    foreach ($line in $lines) {
        $normalizedLine = $line -replace '/', '\'
        $lineMatches = $true
        foreach ($fragment in $Fragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($normalizedLine -notmatch [regex]::Escape($normalizedFragment)) {
                $lineMatches = $false
                break
            }
        }
        if ($lineMatches) {
            return
        }
    }

    throw "$Label does not contain a line with all expected fragments: $($Fragments -join ', ')"
}

function Assert-MarkdownListRunContainsAll {
    param(
        [string]$Path,
        [string]$Anchor,
        [string[]]$Fragments,
        [string]$Label
    )

    $lines = Get-CachedFileLines -Path $Path
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if ($lines[$lineIndex] -notmatch [regex]::Escape($Anchor)) {
            continue
        }
        if ($lines[$lineIndex] -notmatch '^\s*-\s*') {
            continue
        }

        $runStart = $lineIndex
        while ($runStart -gt 0 -and $lines[$runStart - 1] -match '^\s*-\s*') {
            $runStart--
        }

        $runEnd = $lineIndex
        while ($runEnd + 1 -lt $lines.Count -and $lines[$runEnd + 1] -match '^\s*-\s*') {
            $runEnd++
        }

        $run = ($lines[$runStart..$runEnd]) -join "`n"
        $runMatches = $true
        foreach ($fragment in $Fragments) {
            if ($run -notmatch [regex]::Escape($fragment)) {
                $runMatches = $false
                break
            }
        }

        if ($runMatches) {
            return
        }
    }

    throw "$Label does not contain a Markdown list run anchored by '$Anchor' with all expected fragments: $($Fragments -join ', ')"
}

function Get-OptionalPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-OptionalPropertyObject {
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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
. (Join-Path $resolvedRepoRoot "scripts\release_visual_metadata_helpers.ps1")

$missingReviewTaskSummaryLine = Get-VisualReviewTaskSummaryLine `
    -VisualGateSummary ([pscustomobject]@{}) `
    -GateSummary ([pscustomobject]@{})
if ($missingReviewTaskSummaryLine -ne "") {
    throw "Missing review_task_summary unexpectedly rendered '$missingReviewTaskSummaryLine'."
}

$emptyReleaseReviewTaskSummaryLine = Get-VisualReviewTaskSummaryLine `
    -VisualGateSummary ([pscustomobject]@{ review_task_summary = [pscustomobject]@{ total_count = "" } }) `
    -GateSummary ([pscustomobject]@{})
if ($emptyReleaseReviewTaskSummaryLine -ne "") {
    throw "Incomplete release review_task_summary unexpectedly rendered '$emptyReleaseReviewTaskSummaryLine'."
}

$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$reportDir = Join-Path $resolvedWorkingDir "report"
$installDir = Join-Path $resolvedWorkingDir "install"
$gateReportDir = Join-Path $resolvedWorkingDir "word-visual-release-gate\report"
$taskOutputRoot = Join-Path $resolvedWorkingDir "tasks"
$smokeTaskDir = Join-Path $taskOutputRoot "smoke"
$fixedGridTaskDir = Join-Path $taskOutputRoot "fixed-grid"
$sectionPageSetupTaskDir = Join-Path $taskOutputRoot "section-page-setup"
$pageNumberFieldsTaskDir = Join-Path $taskOutputRoot "page-number-fields"
$curatedBundleId = "curated-review-verdict-metadata"
$curatedBundleLabel = "Curated review verdict metadata"
$curatedBundleTaskDir = Join-Path $taskOutputRoot $curatedBundleId
$supersededReviewTasksReportPath = Join-Path $taskOutputRoot "superseded_review_tasks.json"
$pdfVisualGateReportDir = Join-Path $resolvedWorkingDir "pdf-visual-gate\report"
$pdfVisualGateSummaryPath = Join-Path $pdfVisualGateReportDir "summary.json"
$pdfVisualGateAggregateContactSheetPath = Join-Path $pdfVisualGateReportDir "aggregate-contact-sheet.png"
$pdfVisualGateCliExportLogPath = Join-Path $pdfVisualGateReportDir "pdf-cli-export-test.log"
$pdfVisualGateRegressionLogPath = Join-Path $pdfVisualGateReportDir "pdf-regression-test.log"
$pdfVisualGateCopySearchLogDir = Join-Path $pdfVisualGateReportDir "cjk-copy-search"
$pdfVisualGateUnicodeFontLogPath = Join-Path $pdfVisualGateReportDir "unicode-font.log"
$pdfBoundedCtestSubsets = @(
    "smoke-import",
    "contract-static",
    "cjk-flow-static",
    "regression-basic-text",
    "regression-styled-document",
    "regression-business-samples",
    "regression-table-layout"
)
$pdfBoundedCtestSummaryJsonDisplay = @(
    ".\build\pdf-ctest-bounded-subset-current\summary.json",
    ".\build\pdf-ctest-bounded-contract-static-current\summary.json",
    ".\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json",
    ".\build\pdf-ctest-bounded-regression-basic-text-current\summary.json",
    ".\build\pdf-ctest-bounded-regression-styled-document-current\summary.json",
    ".\build\pdf-ctest-bounded-regression-business-samples-current\summary.json",
    ".\build\pdf-ctest-bounded-regression-table-layout-current\summary.json"
)
$pdfBoundedCtestSubsetsText = $pdfBoundedCtestSubsets -join ', '
$pdfBoundedCtestSummaryJsonDisplayText = $pdfBoundedCtestSummaryJsonDisplay -join ', '

foreach ($path in @(
        $reportDir,
        $installDir,
        $gateReportDir,
        $taskOutputRoot,
        $smokeTaskDir,
        $fixedGridTaskDir,
        $sectionPageSetupTaskDir,
        $pageNumberFieldsTaskDir,
        $curatedBundleTaskDir,
        $pdfVisualGateReportDir,
        $pdfVisualGateCopySearchLogDir
    )) {
    New-Item -ItemType Directory -Path $path -Force | Out-Null
}

$summaryPath = Join-Path $reportDir "summary.json"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$governanceMetricSourceJson = "output/numbering-catalog-governance/summary.json"
$governanceMetricSourceDisplay = ".\output\numbering-catalog-governance\summary.json"
$governanceMetric = [ordered]@{
    id = "numbering_catalog_governance.real_corpus_confidence"
    metric = "real_corpus_confidence"
    report_id = "numbering_catalog_governance"
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
    source_report = $governanceMetricSourceJson
    source_report_display = $governanceMetricSourceDisplay
    source_json = $governanceMetricSourceJson
    source_json_display = $governanceMetricSourceDisplay
    level = "ready"
    score = 0.96
    details = [ordered]@{
        score = 96
        level = "ready"
        matched_document_count = 4
        unmatched_catalog_document_count = 0
        unmatched_baseline_document_count = 0
        alignment_gap_count = 0
        catalog_coverage_percent = 100
        baseline_coverage_percent = 100
        coverage_score = 100
        catalog_document_keys = @("contract-template", "invoice-template", "report-template", "long-doc-template")
        baseline_document_keys = @("contract-template", "invoice-template", "report-template", "long-doc-template")
        matched_document_keys = @("contract-template", "invoice-template", "report-template", "long-doc-template")
        penalty_summary = @(
            [ordered]@{ factor = "style_numbering_issues"; count = 1; penalty = 4 }
        )
    }
}

$gateSummary = [ordered]@{
    generated_at = "2026-04-28T12:00:00"
    workspace = $resolvedRepoRoot
    report_dir = $gateReportDir
    visual_verdict = "pending_manual_review"
    smoke = [ordered]@{
        status = "completed"
        task = [ordered]@{ task_dir = $smokeTaskDir }
        review_status = "reviewed"
        review_verdict = "pass"
        reviewed_at = "2026-04-28T12:30:00"
        review_method = "operator_supplied"
        review_note = "smoke contact sheet reviewed"
    }
    fixed_grid = [ordered]@{
        status = "completed"
        task = [ordered]@{ task_dir = $fixedGridTaskDir }
        review_status = "reviewed"
        review_verdict = "fail"
        reviewed_at = "2026-04-28T12:32:00"
        review_method = "operator_supplied"
        review_note = "fixed-grid mismatch documented"
    }
    section_page_setup = [ordered]@{
        status = "completed"
        task = [ordered]@{ task_dir = $sectionPageSetupTaskDir }
        review_status = "reviewed"
        review_verdict = "undetermined"
        reviewed_at = "2026-04-28T12:33:00"
        review_method = "operator_supplied"
        review_note = "section setup needs reviewer decision"
    }
    page_number_fields = [ordered]@{
        status = "completed"
        task = [ordered]@{ task_dir = $pageNumberFieldsTaskDir }
        review_status = "reviewed"
        review_verdict = "pending_manual_review"
        reviewed_at = "2026-04-28T12:34:00"
        review_method = "operator_supplied"
        review_note = "page number fields awaiting reviewer"
    }
    review_task_summary = [ordered]@{
        total_count = 5
        standard_count = 4
        curated_count = 1
    }
    review_tasks = [ordered]@{
        curated_visual_regressions = @(
            [ordered]@{
                id = $curatedBundleId
                label = $curatedBundleLabel
                task = [ordered]@{ task_dir = $curatedBundleTaskDir }
                review_status = "reviewed"
                review_verdict = "pass"
                reviewed_at = "2026-04-28T12:35:00"
                review_method = "operator_supplied"
                review_note = "curated visual evidence checked"
            }
        )
    }
    curated_visual_regressions = @(
        [ordered]@{
            id = $curatedBundleId
            label = $curatedBundleLabel
            status = "completed"
            task = [ordered]@{ task_dir = $curatedBundleTaskDir }
            review_status = "reviewed"
            review_verdict = "pass"
            reviewed_at = "2026-04-28T12:35:00"
            review_method = "operator_supplied"
            review_note = "curated visual evidence checked"
        }
    )
}
($gateSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8
Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8 -Value "# Gate Final Review"
$pdfVisualGateSummary = [ordered]@{
    generated_at = "2026-05-23T12:00:00"
    verdict = "pass"
    aggregate_contact_sheet = $pdfVisualGateAggregateContactSheetPath
    cjk_manifest_count = 43
    visual_baseline_manifest_count = 42
    logs = [ordered]@{
        pdf_cli_export = $pdfVisualGateCliExportLogPath
        pdf_regression = $pdfVisualGateRegressionLogPath
        cjk_copy_search = $pdfVisualGateCopySearchLogDir
        unicode_font = $pdfVisualGateUnicodeFontLogPath
    }
    cjk_copy_search = @(
        [ordered]@{
            sample_id = "cjk-text"
            missing_text = @()
        },
        [ordered]@{
            sample_id = "cjk-mixed"
            missing_text = @()
        }
    )
    baselines = @(
        [ordered]@{ sample = "styled-text" },
        [ordered]@{ sample = "cjk-text" },
        [ordered]@{ sample = "unicode-font" }
    )
}
($pdfVisualGateSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $pdfVisualGateSummaryPath -Encoding UTF8
(@{
    generated_at = "2026-04-28T12:00:00"
    task_output_root = $taskOutputRoot
    report_path = $supersededReviewTasksReportPath
    group_count = 0
    superseded_task_count = 0
    groups = @()
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $supersededReviewTasksReportPath -Encoding UTF8

$summary = [ordered]@{
    generated_at = "2026-04-28T12:00:00"
    workspace = $resolvedRepoRoot
    execution_status = "pass"
    release_version = "1.6.0"
    task_output_root = $taskOutputRoot
    superseded_review_tasks_report = $supersededReviewTasksReportPath
    install_dir = $installDir
    release_blocker_rollup = [ordered]@{
        requested = $true
        status = "ready"
        source_report_count = 1
        source_failure_count = 0
        release_blocker_count = 0
        warning_count = 0
        action_item_count = 0
        release_blockers = @()
        warnings = @()
        action_items = @()
        governance_metrics = @($governanceMetric)
    }
    release_governance_handoff = [ordered]@{
        requested = $true
        status = "ready"
        expected_report_count = 1
        loaded_report_count = 1
        missing_report_count = 0
        failed_report_count = 0
        release_blocker_count = 0
        warning_count = 0
        action_item_count = 0
        release_blockers = @()
        warnings = @()
        action_items = @()
        governance_metrics = @($governanceMetric)
    }
    steps = [ordered]@{
        configure = [ordered]@{ status = "completed" }
        build = [ordered]@{ status = "completed" }
        tests = [ordered]@{ status = "completed" }
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installDir
            consumer_document = (Join-Path $resolvedWorkingDir "consumer\install-smoke.docx")
        }
        visual_gate = [ordered]@{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = $gateFinalReviewPath
            superseded_review_tasks_report = $supersededReviewTasksReportPath
            review_task_summary = [ordered]@{
                total_count = ""
            }
            smoke_reviewed_at = "2026-04-28T12:31:00"
            smoke_review_method = "release_summary_override"
        }
        pdf_visual_gate = [ordered]@{
            status = "completed"
            summary_json = $pdfVisualGateSummaryPath
        }
        pdf_bounded_ctest = [ordered]@{
            status = "pass"
            summary_count = 7
            pass_count = 7
            skipped_test_count = 0
            selected_test_count = 70
            subsets = $pdfBoundedCtestSubsets
            summary_json_display = $pdfBoundedCtestSummaryJsonDisplay
        }
    }
}
($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$bundleScript = Join-Path $resolvedRepoRoot "scripts\write_release_note_bundle.ps1"
& $bundleScript -SummaryJson $summaryPath -SkipMaterialSafetyAudit

$handoffPath = Join-Path $reportDir "release_handoff.md"
$bodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$shortPath = Join-Path $reportDir "release_summary.zh-CN.md"
$guidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$checklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$startHerePath = Join-Path (Split-Path -Parent $reportDir) "START_HERE.md"

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $assertion.Path -ExpectedText "Review task count: 5 total (4 standard, 1 curated)" -Label $assertion.Label
}

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF visual gate summary:" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "pdf-visual-gate\report\summary.json" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF visual gate evidence status: loaded" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF visual gate verdict: pass" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "aggregate-contact-sheet.png" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF CJK manifest samples: 43" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF CJK copy/search samples: 2" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF CJK missing text count: 0" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF visual baseline manifest samples: 42" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF visual baselines: 3" -Label $assertion.Label
}

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $assertion.Path -ExpectedText "PDF bounded CTest auxiliary evidence" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "status=pass" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "summaries=7" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "pass=7" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "selected_tests=70" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "skipped_tests=0" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "regression-business-samples" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "pdf-ctest-bounded-regression-business-samples-current\summary.json" -Label $assertion.Label
}

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-MarkdownListRunContainsAll -Path $assertion.Path -Anchor "PDF bounded CTest auxiliary evidence" -Fragments @(
        "PDF bounded CTest auxiliary evidence",
        "status=pass",
        "summaries=7",
        "pass=7",
        "selected_tests=70",
        "skipped_tests=0",
        "PDF bounded CTest auxiliary subsets",
        "regression-business-samples",
        "PDF bounded CTest auxiliary summaries",
        "pdf-ctest-bounded-regression-business-samples-current\summary.json"
    ) -Label $assertion.Label
}

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-MarkdownListRunContainsAll -Path $assertion.Path -Anchor "PDF visual gate summary:" -Fragments @(
        "PDF visual gate summary:",
        "pdf-visual-gate\report\summary.json",
        "PDF visual gate evidence status: loaded",
        "PDF visual gate verdict: pass",
        "PDF visual gate aggregate contact sheet",
        "aggregate-contact-sheet.png",
        "PDF CJK manifest samples: 43",
        "PDF CJK copy/search samples: 2",
        "PDF CJK missing text count: 0",
        "PDF visual baseline manifest samples: 42",
        "PDF visual baselines: 3"
    ) -Label $assertion.Label
}

foreach ($fragments in @(
        @("PDF visual gate summary", "pdf-visual-gate\report\summary.json"),
        @("PDF visual gate evidence status", "loaded"),
        @("PDF visual gate verdict", "pass"),
        @("PDF visual gate aggregate contact sheet", "aggregate-contact-sheet.png"),
        @("PDF CJK manifest samples", "43"),
        @("PDF CJK copy/search samples", "2"),
        @("PDF CJK missing text count", "0"),
        @("PDF visual baseline manifest samples", "42"),
        @("PDF visual baselines", "3")
    )) {
    Assert-LineContainsAll -Path $bodyPath -Fragments $fragments -Label "release_body.zh-CN.md"
}
foreach ($fragments in @(
        @("PDF bounded CTest auxiliary evidence", "status=pass", "summaries=7", "pass=7", "selected_tests=70", "skipped_tests=0"),
        @("PDF bounded CTest auxiliary subsets", "regression-business-samples"),
        @("PDF bounded CTest auxiliary summaries", "pdf-ctest-bounded-regression-business-samples-current\summary.json"),
        @("PDF bounded CTest boundary", "full visual gate verdict")
    )) {
    Assert-LineContainsAll -Path $bodyPath -Fragments $fragments -Label "release_body.zh-CN.md"
}
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm the PDF visual gate finalize evidence is signed off: verdict `pass`' -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath -ExpectedText 'CJK manifest samples `43`' -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath -ExpectedText 'visual baseline manifest samples `42`' -Label "REVIEWER_CHECKLIST.md"
Assert-LineContainsAll -Path $checklistPath -Fragments @(
    'Confirm the PDF visual gate finalize evidence is signed off',
    'verdict `pass`',
    'summary',
    'pdf-visual-gate\report\summary.json',
    'aggregate contact sheet',
    'aggregate-contact-sheet.png',
    'CJK manifest samples `43`',
    'CJK copy/search samples `2`',
    'missing text `0`',
    'visual baseline manifest samples `42`',
    'visual baselines `3`'
) -Label "REVIEWER_CHECKLIST.md"
Assert-LineContainsAll -Path $checklistPath -Fragments @(
    'Confirm the PDF bounded CTest auxiliary evidence is recorded separately from the full visual gate',
    'status `pass`',
    'summaries `7`',
    'pass `7`',
    'selected tests `70`',
    'skipped tests `0`',
    'regression-business-samples'
) -Label "REVIEWER_CHECKLIST.md"
Assert-LineContainsAll -Path $checklistPath -Fragments @(
    'Open the bounded CTest summary list',
    'pdf-ctest-bounded-regression-business-samples-current\summary.json'
) -Label "REVIEWER_CHECKLIST.md"

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $assertion.Path -ExpectedText "Governance Metrics" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "numbering_catalog_governance.real_corpus_confidence" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "source_report: $governanceMetricSourceJson" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "source_json: $governanceMetricSourceJson" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "source_report_display: $governanceMetricSourceDisplay" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "source_json_display: $governanceMetricSourceDisplay" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "source_failure_count: 0" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Source failures: 0" -Label $assertion.Label
}

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke verdict: pass" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke reviewed at: 2026-04-28T12:31:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke review method: release_summary_override" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Smoke review note: smoke contact sheet reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid verdict: fail" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid reviewed at: 2026-04-28T12:32:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Fixed-grid review note: fixed-grid mismatch documented" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup verdict: undetermined" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup reviewed at: 2026-04-28T12:33:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Section page setup review note: section setup needs reviewer decision" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields verdict: pending_manual_review" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields reviewed at: 2026-04-28T12:34:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Page number fields review note: page number fields awaiting reviewer" -Label $assertion.Label
    foreach ($reviewPathAssertion in @(
            @{ Label = "Smoke"; Task = "smoke" },
            @{ Label = "Fixed-grid"; Task = "fixed-grid" },
            @{ Label = "Section page setup"; Task = "section-page-setup" },
            @{ Label = "Page number fields"; Task = "page-number-fields" }
        )) {
        Assert-LineContainsAll -Path $assertion.Path -Fragments @(
            "$($reviewPathAssertion.Label) review result",
            $reviewPathAssertion.Task,
            "report\review_result.json"
        ) -Label $assertion.Label
        Assert-LineContainsAll -Path $assertion.Path -Fragments @(
            "$($reviewPathAssertion.Label) final review",
            $reviewPathAssertion.Task,
            "report\final_review.md"
        ) -Label $assertion.Label
    }
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata verdict: pass" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata reviewed at: 2026-04-28T12:35:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata review note: curated visual evidence checked" -Label $assertion.Label
    Assert-LineContainsAll -Path $assertion.Path -Fragments @(
        "Curated review verdict metadata review result",
        "curated-review-verdict-metadata",
        "report\review_result.json"
    ) -Label $assertion.Label
    Assert-LineContainsAll -Path $assertion.Path -Fragments @(
        "Curated review verdict metadata final review",
        "curated-review-verdict-metadata",
        "report\final_review.md"
    ) -Label $assertion.Label
}

foreach ($fragments in @(
        @("Smoke verdict", "pass"),
        @("Smoke review status", "reviewed"),
        @("Fixed-grid verdict", "fail"),
        @("Fixed-grid review status", "reviewed"),
        @("Section page setup verdict", "undetermined"),
        @("Section page setup review status", "reviewed"),
        @("Page number fields verdict", "pending_manual_review"),
        @("Page number fields review status", "reviewed"),
        @("Curated review verdict metadata verdict", "pass"),
        @("Curated review verdict metadata review status", "reviewed")
    )) {
    Assert-LineContainsAll -Path $bodyPath -Fragments $fragments -Label "release_body.zh-CN.md"
}

$bodyContent = Get-CachedFileText -Path $bodyPath
foreach ($unexpectedNote in @(
        "smoke contact sheet reviewed",
        "fixed-grid mismatch documented",
        "section setup needs reviewer decision",
        "page number fields awaiting reviewer",
        "curated visual evidence checked"
    )) {
    if ($bodyContent -match [regex]::Escape($unexpectedNote)) {
        throw "release_body.zh-CN.md unexpectedly rendered review note '$unexpectedNote'."
    }
}

foreach ($unexpectedProvenance in @(
        "2026-04-28T12:31:00",
        "2026-04-28T12:32:00",
        "2026-04-28T12:33:00",
        "2026-04-28T12:34:00",
        "2026-04-28T12:35:00",
        "release_summary_override",
        "operator_supplied",
        "review_result_path",
        "review_result.json",
        "review result"
    )) {
    if ($bodyContent -match [regex]::Escape($unexpectedProvenance)) {
        throw "release_body.zh-CN.md unexpectedly rendered review provenance '$unexpectedProvenance'."
    }
}

foreach ($fragment in @(
        'smoke=`pass`',
        'fixed-grid=`fail`',
        'section page setup=`undetermined`',
        'page number fields=`pending_manual_review`',
        'Curated review verdict metadata=`pass`',
        'PDF visual gate',
        'verdict=pass',
        'summary=',
        'pdf-visual-gate\report\summary.json',
        'aggregate-contact-sheet.png',
        'cjk_manifest_count=43',
        'cjk_copy_search_count=2',
        'visual_baseline_manifest_count=42',
        'visual_baseline_count=3'
    )) {
    Assert-Contains -Path $shortPath -ExpectedText $fragment -Label "release_summary.zh-CN.md"
}
foreach ($fragment in @(
        'PDF bounded CTest',
        'status=pass',
        'summaries=7',
        'pass=7',
        'selected_tests=70',
        'skipped_tests=0',
        'full visual gate verdict'
    )) {
    Assert-Contains -Path $shortPath -ExpectedText $fragment -Label "release_summary.zh-CN.md"
}
Assert-LineContainsAll -Path $shortPath -Fragments @(
    'PDF visual gate',
    'verdict=pass',
    'summary=',
    'pdf-visual-gate\report\summary.json',
    'aggregate_contact_sheet=',
    'aggregate-contact-sheet.png',
    'cjk_manifest_count=43',
    'cjk_copy_search_count=2',
    'visual_baseline_manifest_count=42',
    'visual_baseline_count=3'
) -Label "release_summary.zh-CN.md"
Assert-LineContainsAll -Path $shortPath -Fragments @(
    'PDF bounded CTest',
    'status=pass',
    'summaries=7',
    'pass=7',
    'selected_tests=70',
    'skipped_tests=0',
    'full visual gate verdict'
) -Label "release_summary.zh-CN.md"

Write-Host "Release note bundle visual verdict metadata regression passed."
