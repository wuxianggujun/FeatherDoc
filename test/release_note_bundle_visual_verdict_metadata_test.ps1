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
        $lineMatches = $true
        foreach ($fragment in $Fragments) {
            if ($line -notmatch [regex]::Escape($fragment)) {
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

foreach ($path in @(
        $reportDir,
        $installDir,
        $gateReportDir,
        $taskOutputRoot,
        $smokeTaskDir,
        $fixedGridTaskDir,
        $sectionPageSetupTaskDir,
        $pageNumberFieldsTaskDir,
        $curatedBundleTaskDir
    )) {
    New-Item -ItemType Directory -Path $path -Force | Out-Null
}

$summaryPath = Join-Path $reportDir "summary.json"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"

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
        status = "ready_with_warnings"
        release_blocker_count = 1
        release_blockers = @(
            [ordered]@{
                composite_id = "source0.blocker0.numbering_catalog_governance.style_numbering_issues"
                id = "numbering_catalog_governance.style_numbering_issues"
                action = "review_style_numbering_audit"
                status = "blocked"
                severity = "error"
                source_schema = "featherdoc.numbering_catalog_governance_report.v1"
                source_report_display = ".\output\numbering-catalog-governance\summary.json"
                message = "numbering catalog governance reports unresolved style numbering issues"
            }
        )
        warning_count = 1
        warnings = @(
            [ordered]@{
                id = "numbering_catalog.style_merge_suggestions"
                action = "review_style_merge_plan"
                message = "numbering catalog reports pending style-merge suggestions"
                source_schema = "featherdoc.numbering_catalog_governance_report.v1"
                style_merge_suggestion_count = 3
            }
        )
    }
    release_governance_handoff = [ordered]@{
        status = "ready_with_warnings"
        release_blocker_count = 1
        release_blockers = @(
            [ordered]@{
                id = "release_governance_handoff.required_report_missing"
                action = "inspect_required_report"
                status = "blocked"
                severity = "error"
                source_schema = "featherdoc.release_governance_handoff_report.v1"
                source_report_display = ".\output\release-governance-handoff\summary.json"
                message = "required governance report is missing from the handoff bundle"
            }
        )
        warning_count = 1
        warnings = @(
            [ordered]@{
                id = "release_governance_handoff.optional_report_missing"
                action = "inspect_optional_report"
                message = "optional governance report is missing from the handoff bundle"
                source_schema = "featherdoc.release_governance_handoff_report.v1"
            }
        )
        release_blocker_rollup = [ordered]@{
            included = $true
            status = "ready_with_warnings"
            release_blocker_count = 1
            release_blockers = @(
                [ordered]@{
                    composite_id = "source2.blocker0.style_merge.restore_audit_issues"
                    id = "style_merge.restore_audit_issues"
                    action = "review_style_merge_restore_audit"
                    status = "blocked"
                    severity = "error"
                    source_schema = "featherdoc.style_merge_restore_audit.v1"
                    source_report_display = ".\output\document-skeleton-governance\style-merge.restore-audit.summary.json"
                    message = "style merge restore audit reports unresolved rollback issues"
                }
            )
            warning_count = 1
            warnings = @(
                [ordered]@{
                    id = "document_skeleton.style_merge_suggestions"
                    action = "review_style_merge_plan"
                    message = "document skeleton rollup still reports style-merge suggestions"
                    source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                    style_merge_suggestion_count = 2
                }
            )
        }
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
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata verdict: pass" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata review status: reviewed" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata reviewed at: 2026-04-28T12:35:00" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata review method: operator_supplied" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Curated review verdict metadata review note: curated visual evidence checked" -Label $assertion.Label
}

foreach ($assertion in @(
        @{ Path = $handoffPath; Label = "release_handoff.md" },
        @{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $startHerePath; Label = "START_HERE.md" }
    )) {
    Assert-Contains -Path $assertion.Path -ExpectedText "Release blocker rollup blocker_count: 1" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Release governance handoff blocker_count: 1" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Release governance handoff nested rollup blocker_count: 1" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "## Release Governance Blockers" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "### Release blocker rollup blockers" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "### Release governance handoff blockers" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "### Release governance handoff nested rollup blockers" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'release_blocker_count: `1`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'id: `numbering_catalog_governance.style_numbering_issues`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'composite_id: `source0.blocker0.numbering_catalog_governance.style_numbering_issues`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'action: `review_style_numbering_audit`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'status: `blocked`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'severity: `error`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'source_schema: `featherdoc.numbering_catalog_governance_report.v1`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'source_report_display: `.\output\numbering-catalog-governance\summary.json`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'message: numbering catalog governance reports unresolved style numbering issues' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'id: `release_governance_handoff.required_report_missing`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'action: `inspect_required_report`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'source_schema: `featherdoc.release_governance_handoff_report.v1`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'message: required governance report is missing from the handoff bundle' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'id: `style_merge.restore_audit_issues`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'composite_id: `source2.blocker0.style_merge.restore_audit_issues`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'action: `review_style_merge_restore_audit`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'source_schema: `featherdoc.style_merge_restore_audit.v1`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'source_report_display: `.\output\document-skeleton-governance\style-merge.restore-audit.summary.json`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'message: style merge restore audit reports unresolved rollback issues' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Release blocker rollup warning_count: 1" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Release governance handoff warning_count: 1" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "Release governance handoff nested rollup warning_count: 1" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "## Release Governance Warnings" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "### Release blocker rollup warnings" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "### Release governance handoff warnings" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText "### Release governance handoff nested rollup warnings" -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'warning_count: `1`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'id: `numbering_catalog.style_merge_suggestions`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'message: numbering catalog reports pending style-merge suggestions' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'action: `review_style_merge_plan`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'source_schema: `featherdoc.numbering_catalog_governance_report.v1`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'style_merge_suggestion_count: `3`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'id: `release_governance_handoff.optional_report_missing`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'action: `inspect_optional_report`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'message: optional governance report is missing from the handoff bundle' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'source_schema: `featherdoc.release_governance_handoff_report.v1`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'id: `document_skeleton.style_merge_suggestions`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'message: document skeleton rollup still reports style-merge suggestions' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'action: `review_style_merge_plan`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' -Label $assertion.Label
    Assert-Contains -Path $assertion.Path -ExpectedText 'style_merge_suggestion_count: `2`' -Label $assertion.Label
}

Assert-Contains -Path $checklistPath `
    -ExpectedText 'Resolve release governance blocker `numbering_catalog_governance.style_numbering_issues` (Release blocker rollup blockers): action `review_style_numbering_audit`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'composite_id `source0.blocker0.numbering_catalog_governance.style_numbering_issues`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'source_report `.\output\numbering-catalog-governance\summary.json`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Open source report `.\output\numbering-catalog-governance\summary.json` for blocker action `review_style_numbering_audit`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Resolve release governance blocker `release_governance_handoff.required_report_missing` (Release governance handoff blockers): action `inspect_required_report`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Open source report `.\output\release-governance-handoff\summary.json` for blocker action `inspect_required_report`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Resolve release governance blocker `style_merge.restore_audit_issues` (Release governance handoff nested rollup blockers): action `review_style_merge_restore_audit`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Open source report `.\output\document-skeleton-governance\style-merge.restore-audit.summary.json` for blocker action `review_style_merge_restore_audit`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Do not approve for public release when release governance blocker counts are non-zero in the final rollup, governance handoff, or nested handoff rollup.' `
    -Label "REVIEWER_CHECKLIST.md"

Assert-Contains -Path $checklistPath `
    -ExpectedText 'Review release governance warning `numbering_catalog.style_merge_suggestions` (Release blocker rollup warnings): action `review_style_merge_plan`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Current style merge suggestion count is `3`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Follow warning action `inspect_optional_report` for source_schema `featherdoc.release_governance_handoff_report.v1`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Review release governance warning `document_skeleton.style_merge_suggestions` (Release governance handoff nested rollup warnings): action `review_style_merge_plan`' `
    -Label "REVIEWER_CHECKLIST.md"
Assert-Contains -Path $checklistPath `
    -ExpectedText 'Current style merge suggestion count is `2`' `
    -Label "REVIEWER_CHECKLIST.md"

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
        "operator_supplied"
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
        'Curated review verdict metadata=`pass`'
    )) {
    Assert-Contains -Path $shortPath -ExpectedText $fragment -Label "release_summary.zh-CN.md"
}

Write-Host "Release note bundle visual verdict metadata regression passed."
