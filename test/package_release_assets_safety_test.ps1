param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$UnexpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if (-not [string]::IsNullOrWhiteSpace($UnexpectedText) -and
        $content -match [regex]::Escape($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText': $Path"
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$workingDirParent = Split-Path -Parent $resolvedWorkingDir
if (-not [string]::IsNullOrWhiteSpace($workingDirParent)) {
    New-Item -ItemType Directory -Path $workingDirParent -Force | Out-Null
}
if (Test-Path -LiteralPath $resolvedWorkingDir) {
    Remove-Item -LiteralPath $resolvedWorkingDir -Recurse -Force
}
$null = New-Item -ItemType Directory -Path $resolvedWorkingDir -Force
$relativeWorkingDir = $resolvedWorkingDir.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
$installPrefix = Join-Path $resolvedWorkingDir "build-msvc-install"
$summaryOutputDir = Join-Path $resolvedWorkingDir "output\release-candidate-checks"
$reportDir = Join-Path $summaryOutputDir "report"
$gateOutputDir = Join-Path $resolvedWorkingDir "output\word-visual-release-gate"
$gateReportDir = Join-Path $gateOutputDir "report"
$smokeEvidenceDir = Join-Path $gateOutputDir "smoke\evidence"
$fixedGridAggregateDir = Join-Path $gateOutputDir "fixed-grid\aggregate-evidence"
$sectionPageSetupAggregateDir = Join-Path $gateOutputDir "section-page-setup\aggregate-evidence"
$outputRoot = Join-Path $resolvedWorkingDir "release-assets"

New-Item -ItemType Directory -Path (Join-Path $installPrefix "share\FeatherDoc") -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $smokeEvidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $fixedGridAggregateDir -Force | Out-Null
New-Item -ItemType Directory -Path $sectionPageSetupAggregateDir -Force | Out-Null

$startHerePath = Join-Path $summaryOutputDir "START_HERE.md"
$releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
$releaseGovernanceHandoffPath = Join-Path $reportDir "release_governance_handoff.md"
$releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
$artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$contentControlSummaryPath = Join-Path $reportDir "content_control_data_binding_governance_summary.json"
$projectTemplateDeliveryReadinessSummaryPath = Join-Path $reportDir "project_template_delivery_readiness_summary.json"
$projectTemplateOnboardingGovernanceSummaryPath = Join-Path $reportDir "project_template_onboarding_governance_summary.json"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$summaryPath = Join-Path $reportDir "summary.json"
$installedReadmePath = Join-Path $installPrefix "share\FeatherDoc\README.md"
$installedChangelogPath = Join-Path $installPrefix "share\FeatherDoc\CHANGELOG.md"

Set-Content -LiteralPath $startHerePath -Encoding UTF8 -Value @"
# START_HERE

- Evidence root: $resolvedRepoRoot\output\release-candidate-checks\report
- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

Set-Content -LiteralPath $releaseHandoffPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4

- Handoff path: $resolvedRepoRoot\output\release-candidate-checks\report\release_handoff.md
- Governance metrics: numbering_catalog_governance.real_corpus_confidence=low, table_layout_delivery_governance.delivery_quality=release_ready
"@

Set-Content -LiteralPath $releaseGovernanceHandoffPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Governance Metrics

- numbering_catalog_governance.real_corpus_confidence: report=numbering_catalog_governance metric=real_corpus_confidence level=low score=56 source_schema=featherdoc.numbering_catalog_governance_report.v1
- table_layout_delivery_governance.delivery_quality: report=table_layout_delivery_governance metric=delivery_quality level=release_ready score=100 source_schema=featherdoc.table_layout_delivery_governance_report.v1
"@

Set-Content -LiteralPath $releaseBodyPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷璇存槑

- 璇佹嵁鐩綍锛?resolvedRepoRoot\output\word-visual-release-gate\report
"@

Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷鎽樿

- 鎽樿璺緞锛?resolvedRepoRoot\output\release-candidate-checks\report\release_summary.zh-CN.md
"@

Set-Content -LiteralPath $artifactGuidePath -Encoding UTF8 -Value @"
# Artifact Guide

- Report root: $resolvedRepoRoot\output\release-candidate-checks\report
- Governance metric: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Governance metric: table_layout_delivery_governance.delivery_quality release_ready 100 source_schema=featherdoc.table_layout_delivery_governance_report.v1 table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 penalty_summary=floating_table_plans_pending(count=0, penalty=0)
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding governance: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 penalty_summary=floating_table_plans_pending(count=0, penalty=0)
- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
"@

Set-Content -LiteralPath $reviewerChecklistPath -Encoding UTF8 -Value @"
# Reviewer Checklist

- Review root: $resolvedRepoRoot\output\word-visual-release-gate\report
- Confirm governance_metrics before publishing release assets.
- Confirm numbering_catalog_governance.real_corpus_confidence catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20) before publishing release assets.
- Check content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json before release.
- Confirm project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json before release.
- Check project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json before release.
- Confirm table_layout_delivery_governance.delivery_quality table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 penalty_summary=floating_table_plans_pending(count=0, penalty=0) before release.
"@

Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8 -Value @"
# Gate Final Review

- Local gate report: $resolvedRepoRoot\output\word-visual-release-gate\report
"@

Set-Content -LiteralPath $installedReadmePath -Encoding UTF8 -Value @'
# README

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 `
    -DocxPath C:\path\to\target.docx `
    -Mode review-only
```
'@

Set-Content -LiteralPath $installedChangelogPath -Encoding UTF8 -Value @'
# Changelog

- Removed remaining public-facing "draft" wording from release docs.
- The release-note drafting helper still keeps the old draft boilerplate.
- `-Publish` flips `draft=false` after final signoff.
- Normalize `C:\Users\someone\workspace` before public release.
'@

Set-Content -LiteralPath (Join-Path $smokeEvidenceDir "README.md") -Encoding UTF8 -Value @"
# Smoke Evidence

- Local smoke evidence: $resolvedRepoRoot\output\word-visual-release-gate\smoke\evidence
"@

Set-Content -LiteralPath (Join-Path $fixedGridAggregateDir "README.md") -Encoding UTF8 -Value @"
# Fixed Grid Evidence

- Local aggregate evidence: $resolvedRepoRoot\output\word-visual-release-gate\fixed-grid\aggregate-evidence
"@

Set-Content -LiteralPath (Join-Path $sectionPageSetupAggregateDir "README.md") -Encoding UTF8 -Value @"
# Section Page Setup Evidence

- Local aggregate evidence: $resolvedRepoRoot\output\word-visual-release-gate\section-page-setup\aggregate-evidence
"@

$gateSummary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    gate_output_dir = $gateOutputDir
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pass"
    smoke = [ordered]@{
        evidence_dir = $smokeEvidenceDir
    }
    fixed_grid = [ordered]@{
        aggregate_evidence_dir = $fixedGridAggregateDir
    }
}
($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

$contentControlSummary = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    status = "blocked"
    release_ready = $false
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "error"
            status = "placeholder_visible"
            action = "sync_or_fill_bound_content_control"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
        }
    )
}
($contentControlSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $contentControlSummaryPath -Encoding UTF8

$projectTemplateDeliveryReadinessSummary = [ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    status = "ready"
    release_ready = $true
    latest_schema_approval_gate_status = "passed"
    schema_history_blocked_run_count = 0
    schema_history_pending_run_count = 0
    schema_history_passed_run_count = 3
    template_count = 4
    ready_template_count = 4
    blocked_template_count = 0
    release_blocker_count = 0
    action_item_count = 0
    warning_count = 0
}
($projectTemplateDeliveryReadinessSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $projectTemplateDeliveryReadinessSummaryPath -Encoding UTF8

$projectTemplateOnboardingGovernanceSummary = [ordered]@{
    schema = "featherdoc.project_template_onboarding_governance_report.v1"
    status = "ready"
    release_ready = $true
    source_file_count = 3
    source_failure_count = 0
    entry_count = 4
    schema_approval_status_summary = @(
        [ordered]@{
            status = "approved"
            count = 3
        },
        [ordered]@{
            status = "not_required"
            count = 1
        }
    )
    blocked_entry_count = 0
    pending_review_entry_count = 0
    not_evaluated_entry_count = 0
    approved_entry_count = 3
    not_required_entry_count = 1
    release_blocker_count = 0
    action_item_count = 0
    manual_review_recommendation_count = 1
}
($projectTemplateOnboardingGovernanceSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $projectTemplateOnboardingGovernanceSummaryPath -Encoding UTF8

$summary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    workspace = $resolvedRepoRoot
    build_dir = (Join-Path $resolvedWorkingDir "build-msvc-nmake")
    install_dir = $installPrefix
    consumer_build_dir = (Join-Path $resolvedWorkingDir "build-msvc-install-consumer")
    gate_output_dir = $gateOutputDir
    task_output_root = (Join-Path $resolvedWorkingDir "output\word-visual-smoke\tasks-release-gate")
    release_version = "1.6.4"
    execution_status = "pass"
    visual_verdict = "pass"
    governance_metric_count = 3
    governance_metrics = @(
        [ordered]@{
            id = "style_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "style_catalog_governance"
            source_schema = "featherdoc.style_catalog_governance_report.v1"
            score = 12
            level = "experimental"
            details = [ordered]@{
                score = 12
                level = "experimental"
                document_count = 99
                catalog_exemplar_count = 1
                baseline_entry_count = 1
                penalty_summary = @(
                    [ordered]@{ factor = "style_catalog_only"; count = 99; penalty = 88 }
                )
            }
        },
        [ordered]@{
            id = "numbering_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            score = 56
            level = "low"
            details = [ordered]@{
                score = 56
                level = "low"
                document_count = 2
                catalog_exemplar_count = 2
                baseline_entry_count = 2
                catalog_coverage_percent = 100
                baseline_coverage_percent = 100
                coverage_score = 100
                matched_document_count = 2
                unmatched_catalog_document_count = 0
                unmatched_baseline_document_count = 0
                alignment_gap_count = 0
                catalog_document_keys = @("contract.docx", "invoice.docx")
                baseline_document_keys = @("contract.docx", "invoice.docx")
                matched_document_keys = @("contract.docx", "invoice.docx")
                penalty_summary = @(
                    [ordered]@{ factor = "style_numbering_issues"; count = 4; penalty = 20 },
                    [ordered]@{ factor = "catalog_drift_or_dirty_baseline"; count = 2; penalty = 20 },
                    [ordered]@{ factor = "command_failures"; count = 1; penalty = 20 }
                )
            }
        },
        [ordered]@{
            id = "table_layout_delivery_governance.delivery_quality"
            metric = "delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            score = 100
            level = "release_ready"
            details = [ordered]@{
                score = 100
                level = "release_ready"
                document_count = 3
                ready_document_count = 3
                ready_document_percent = 100
                needs_review_document_count = 0
                failed_document_count = 0
                table_style_issue_count = 0
                automatic_tblLook_fix_count = 0
                manual_table_style_fix_count = 0
                table_position_automatic_count = 0
                table_position_review_count = 0
                command_failure_count = 0
                unresolved_item_count = 0
                penalty_summary = @(
                    [ordered]@{ factor = "needs_review_documents"; count = 0; penalty = 0 },
                    [ordered]@{ factor = "floating_table_plans_pending"; count = 0; penalty = 0 }
                )
            }
        }
    )
    content_control_data_binding_governance = $contentControlSummaryPath
    project_template_delivery_readiness = $projectTemplateDeliveryReadinessSummaryPath
    project_template_onboarding_governance = $projectTemplateOnboardingGovernanceSummaryPath
    release_handoff = $releaseHandoffPath
    release_body_zh_cn = $releaseBodyPath
    release_summary_zh_cn = $releaseSummaryPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    start_here = $startHerePath
    readme_gallery = [ordered]@{
        status = "completed"
        assets_dir = (Join-Path $resolvedRepoRoot "docs\assets\readme")
    }
    steps = [ordered]@{
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installPrefix
            consumer_document = (Join-Path $resolvedWorkingDir "build-msvc-install-consumer\featherdoc-install-smoke.docx")
        }
        visual_gate = [ordered]@{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = $gateFinalReviewPath
            visual_verdict = "pass"
        }
    }
}
($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$packageScript = Join-Path $resolvedRepoRoot "scripts\package_release_assets.ps1"
& $packageScript `
    -SummaryJson $summaryPath `
    -OutputRoot $outputRoot `
    -KeepStaging

$stagingRoot = Join-Path $outputRoot "v1.6.4\staging"
$stagedSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\summary.json"
$stagedGateSummaryPath = Join-Path $stagingRoot "word-visual-release-gate\report\gate_summary.json"
$stagedHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_handoff.md"
$stagedGovernanceHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_governance_handoff.md"
$stagedStartHerePath = Join-Path $stagingRoot "release-candidate-checks\START_HERE.md"
$stagedArtifactGuidePath = Join-Path $stagingRoot "release-candidate-checks\report\ARTIFACT_GUIDE.md"
$stagedReviewerChecklistPath = Join-Path $stagingRoot "release-candidate-checks\report\REVIEWER_CHECKLIST.md"
$stagedContentControlSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\content_control_data_binding_governance_summary.json"
$stagedProjectTemplateDeliveryReadinessSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\project_template_delivery_readiness_summary.json"
$stagedProjectTemplateOnboardingGovernanceSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\project_template_onboarding_governance_summary.json"
$stagedInstalledReadmePath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\README.md"
$stagedInstalledChangelogPath = Join-Path $stagingRoot "build-msvc-install\share\FeatherDoc\CHANGELOG.md"
$manifestPath = Join-Path $outputRoot "v1.6.4\release_assets_manifest.json"
$installZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-msvc-install.zip"
$galleryZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-visual-validation-gallery.zip"
$evidenceZipPath = Join-Path $outputRoot "v1.6.4\FeatherDoc-v1.6.4-release-evidence.zip"
$expectedRelativeHandoff = ".\$relativeWorkingDir\output\release-candidate-checks\report\release_handoff.md"
$expectedRelativeGateReport = ".\$relativeWorkingDir\output\word-visual-release-gate\report"
$stagedSummary = Get-Content -Raw -LiteralPath $stagedSummaryPath | ConvertFrom-Json
$stagedGateSummary = Get-Content -Raw -LiteralPath $stagedGateSummaryPath | ConvertFrom-Json
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json

Assert-NotContains -Path $stagedSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged summary.json'
Assert-NotContains -Path $stagedGateSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged gate_summary.json'
Assert-NotContains -Path $stagedHandoffPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_handoff.md'
Assert-NotContains -Path $stagedGovernanceHandoffPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_governance_handoff.md'
Assert-NotContains -Path $stagedInstalledReadmePath -UnexpectedText 'C:\path\to\target.docx' -Label 'staged installed README.md'
Assert-NotContains -Path $stagedInstalledChangelogPath -UnexpectedText 'draft' -Label 'staged installed CHANGELOG.md'
Assert-NotContains -Path $stagedInstalledChangelogPath -UnexpectedText 'C:\Users\someone\workspace' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedInstalledReadmePath -ExpectedText '<windows-absolute-path>' -Label 'staged installed README.md'
Assert-Contains -Path $stagedInstalledChangelogPath -ExpectedText 'preview' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedInstalledChangelogPath -ExpectedText '<windows-absolute-path>' -Label 'staged installed CHANGELOG.md'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'governance_metrics' -Label 'staged summary.json'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'real_corpus_confidence' -Label 'staged summary.json'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'delivery_quality' -Label 'staged summary.json'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged summary.json'
Assert-Contains -Path $stagedSummaryPath -ExpectedText 'project_template_onboarding_governance' -Label 'staged summary.json'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'Governance Metrics' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'real_corpus_confidence' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'featherdoc.numbering_catalog_governance_report.v1' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'delivery_quality' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'source_json_display' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'repair_strategy' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'repair_hint' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'sync_bound_content_control' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'command_template' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_delivery_readiness' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'schema_approval_status_summary' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'catalog_coverage_percent=100' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'matched_document_count=2' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'alignment_gap_count=0' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'matched_document_keys=contract.docx,invoice.docx' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'style_numbering_issues(count=4, penalty=20)' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'unresolved_item_count=0' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'floating_table_plans_pending(count=0, penalty=0)' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'source_json_display' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'repair_strategy' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'repair_hint' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'sync_bound_content_control' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'real_corpus_confidence' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.numbering_catalog_governance_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'delivery_quality' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_delivery_readiness' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_onboarding_governance' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'schema_approval_status_summary' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'catalog_coverage_percent=100' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'matched_document_count=2' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'alignment_gap_count=0' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'matched_document_keys=contract.docx,invoice.docx' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'style_numbering_issues(count=4, penalty=20)' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'unresolved_item_count=0' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'floating_table_plans_pending(count=0, penalty=0)' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'command_template' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'source_json_display' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'repair_strategy' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'repair_hint' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'command_template' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'schema_approval_status_summary' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'catalog_coverage_percent=100' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'matched_document_count=2' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'alignment_gap_count=0' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'matched_document_keys=contract.docx,invoice.docx' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'style_numbering_issues(count=4, penalty=20)' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'unresolved_item_count=0' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'floating_table_plans_pending(count=0, penalty=0)' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'sync_bound_content_control' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'command_template' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged content-control summary'
Assert-Contains -Path $stagedProjectTemplateDeliveryReadinessSummaryPath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged project-template readiness summary'
Assert-Contains -Path $stagedProjectTemplateDeliveryReadinessSummaryPath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged project-template readiness summary'
Assert-Contains -Path $stagedProjectTemplateOnboardingGovernanceSummaryPath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged project-template onboarding governance summary'
Assert-Contains -Path $stagedProjectTemplateOnboardingGovernanceSummaryPath -ExpectedText 'schema_approval_status_summary' -Label 'staged project-template onboarding governance summary'
if ($stagedSummary.release_handoff -ne $expectedRelativeHandoff) {
    throw "staged summary.json did not rewrite release_handoff to the expected relative path."
}
if ($stagedGateSummary.report_dir -ne $expectedRelativeGateReport) {
    throw "staged gate_summary.json did not rewrite report_dir to the expected relative path."
}
if ($manifest.governance_metric_count -ne 3) {
    throw "release_assets_manifest.json did not preserve governance_metric_count=3."
}
if ($manifest.governance_metrics.Count -ne 3) {
    throw "release_assets_manifest.json did not preserve all governance metrics."
}
if (-not (@($manifest.governance_metrics | ForEach-Object { $_.metric }) -contains "real_corpus_confidence")) {
    throw "release_assets_manifest.json lost real_corpus_confidence governance metric."
}
if (-not (@($manifest.governance_metrics | ForEach-Object { $_.metric }) -contains "delivery_quality")) {
    throw "release_assets_manifest.json lost delivery_quality governance metric."
}
$manifestNumberingConfidence = $manifest.numbering_catalog_real_corpus_confidence
if ($null -eq $manifestNumberingConfidence) {
    throw "release_assets_manifest.json lost numbering_catalog_real_corpus_confidence."
}
if ([string]$manifestNumberingConfidence.id -ne "numbering_catalog_governance.real_corpus_confidence") {
    throw "release_assets_manifest.json lost numbering real corpus confidence id."
}
if ([string]$manifestNumberingConfidence.metric -ne "real_corpus_confidence") {
    throw "release_assets_manifest.json lost numbering real corpus confidence metric."
}
if ([string]$manifestNumberingConfidence.report_id -ne "numbering_catalog_governance") {
    throw "release_assets_manifest.json lost numbering real corpus confidence report_id."
}
if ([string]$manifestNumberingConfidence.source_schema -ne "featherdoc.numbering_catalog_governance_report.v1") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_schema."
}
if ([int]$manifestNumberingConfidence.score -ne 56) {
    throw "release_assets_manifest.json lost numbering real corpus confidence score."
}
if ([string]$manifestNumberingConfidence.level -ne "low") {
    throw "release_assets_manifest.json lost numbering real corpus confidence level."
}
if ($null -eq $manifestNumberingConfidence.details) {
    throw "release_assets_manifest.json lost numbering real corpus confidence details."
}
if ([int]$manifestNumberingConfidence.details.document_count -ne 2) {
    throw "release_assets_manifest.json used the wrong real corpus confidence details for numbering."
}
if ([int]$manifestNumberingConfidence.details.catalog_coverage_percent -ne 100) {
    throw "release_assets_manifest.json lost numbering real corpus confidence catalog coverage."
}
if ([int]$manifestNumberingConfidence.details.matched_document_count -ne 2) {
    throw "release_assets_manifest.json lost numbering real corpus confidence matched document count."
}
if ([int]$manifestNumberingConfidence.details.alignment_gap_count -ne 0) {
    throw "release_assets_manifest.json lost numbering real corpus confidence alignment gap count."
}
if (-not (@($manifestNumberingConfidence.details.matched_document_keys) -contains "contract.docx")) {
    throw "release_assets_manifest.json lost numbering real corpus confidence matched document keys."
}
$manifestNumberingPenaltyFactors = @($manifestNumberingConfidence.details.penalty_summary |
    ForEach-Object { [string]$_.factor })
if ($manifestNumberingPenaltyFactors -contains "style_catalog_only") {
    throw "release_assets_manifest.json mirrored style catalog confidence details into numbering confidence."
}
if (-not ($manifestNumberingPenaltyFactors -contains "style_numbering_issues")) {
    throw "release_assets_manifest.json lost numbering real corpus confidence penalty summary."
}
$manifestTableLayoutDeliveryQuality = $manifest.table_layout_delivery_quality
if ($null -eq $manifestTableLayoutDeliveryQuality) {
    throw "release_assets_manifest.json lost table_layout_delivery_quality."
}
if ([string]$manifestTableLayoutDeliveryQuality.id -ne "table_layout_delivery_governance.delivery_quality") {
    throw "release_assets_manifest.json lost table layout delivery quality id."
}
if ([string]$manifestTableLayoutDeliveryQuality.metric -ne "delivery_quality") {
    throw "release_assets_manifest.json lost table layout delivery quality metric."
}
if ([string]$manifestTableLayoutDeliveryQuality.report_id -ne "table_layout_delivery_governance") {
    throw "release_assets_manifest.json lost table layout delivery quality report_id."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_schema -ne "featherdoc.table_layout_delivery_governance_report.v1") {
    throw "release_assets_manifest.json lost table layout delivery quality source_schema."
}
if ([int]$manifestTableLayoutDeliveryQuality.score -ne 100) {
    throw "release_assets_manifest.json lost table layout delivery quality score."
}
if ([string]$manifestTableLayoutDeliveryQuality.level -ne "release_ready") {
    throw "release_assets_manifest.json lost table layout delivery quality level."
}
if ($null -eq $manifestTableLayoutDeliveryQuality.details) {
    throw "release_assets_manifest.json lost table layout delivery quality details."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.document_count -ne 3) {
    throw "release_assets_manifest.json lost table layout delivery quality document_count."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.unresolved_item_count -ne 0) {
    throw "release_assets_manifest.json lost table layout delivery quality unresolved_item_count."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.table_position_automatic_count -ne 0) {
    throw "release_assets_manifest.json lost table layout delivery quality table_position_automatic_count."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.table_position_review_count -ne 0) {
    throw "release_assets_manifest.json lost table layout delivery quality table_position_review_count."
}
if (-not (@($manifestTableLayoutDeliveryQuality.details.penalty_summary |
        ForEach-Object { [string]$_.factor }) -contains "floating_table_plans_pending")) {
    throw "release_assets_manifest.json lost table layout delivery quality penalty summary."
}
if ($manifest.content_control_repair_contract_count -ne 1) {
    throw "release_assets_manifest.json did not preserve content_control_repair_contract_count=1."
}
$manifestContentControlContract = @($manifest.content_control_repair_contracts | Where-Object {
    [string]$_.id -eq "content_control_data_binding.bound_placeholder"
}) | Select-Object -First 1
if ($null -eq $manifestContentControlContract) {
    throw "release_assets_manifest.json lost content_control_data_binding.bound_placeholder repair contract."
}
if ([string]$manifestContentControlContract.source_schema -ne "featherdoc.content_control_data_binding_governance_report.v1") {
    throw "release_assets_manifest.json lost the content-control governance source schema."
}
if ([string]$manifestContentControlContract.repair_strategy -ne "sync_bound_content_control") {
    throw "release_assets_manifest.json lost sync_bound_content_control repair strategy."
}
if ([string]$manifestContentControlContract.repair_hint -ne "Rerun Custom XML sync or explicitly fill the bound content control before release.") {
    throw "release_assets_manifest.json lost content-control repair_hint."
}
if ([string]$manifestContentControlContract.command_template -notmatch "sync-content-controls-from-custom-xml") {
    throw "release_assets_manifest.json lost sync-content-controls-from-custom-xml command template."
}
$manifestProjectTemplateReadiness = $manifest.project_template_delivery_readiness_contract
if ($null -eq $manifestProjectTemplateReadiness) {
    throw "release_assets_manifest.json lost project_template_delivery_readiness_contract."
}
if ([string]$manifestProjectTemplateReadiness.schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
    throw "release_assets_manifest.json lost project template delivery readiness schema."
}
if ([string]$manifestProjectTemplateReadiness.status -ne "ready") {
    throw "release_assets_manifest.json lost project template delivery readiness status."
}
if (-not [bool]$manifestProjectTemplateReadiness.release_ready) {
    throw "release_assets_manifest.json lost project template delivery readiness release_ready=true."
}
if ([string]$manifestProjectTemplateReadiness.latest_schema_approval_gate_status -ne "passed") {
    throw "release_assets_manifest.json lost project template latest schema approval gate status."
}
if ([int]$manifestProjectTemplateReadiness.template_count -ne 4) {
    throw "release_assets_manifest.json lost project template delivery readiness template_count."
}
if ([int]$manifestProjectTemplateReadiness.ready_template_count -ne 4) {
    throw "release_assets_manifest.json lost project template delivery readiness ready_template_count."
}
if ([int]$manifestProjectTemplateReadiness.release_blocker_count -ne 0) {
    throw "release_assets_manifest.json lost project template delivery readiness release_blocker_count."
}
if ([string]::IsNullOrWhiteSpace([string]$manifestProjectTemplateReadiness.source_json_display)) {
    throw "release_assets_manifest.json lost project template delivery readiness source_json_display."
}
$manifestProjectTemplateOnboarding = $manifest.project_template_onboarding_governance_contract
if ($null -eq $manifestProjectTemplateOnboarding) {
    throw "release_assets_manifest.json lost project_template_onboarding_governance_contract."
}
if ([string]$manifestProjectTemplateOnboarding.schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
    throw "release_assets_manifest.json lost project template onboarding governance schema."
}
if ([string]$manifestProjectTemplateOnboarding.status -ne "ready") {
    throw "release_assets_manifest.json lost project template onboarding governance status."
}
if (-not [bool]$manifestProjectTemplateOnboarding.release_ready) {
    throw "release_assets_manifest.json lost project template onboarding governance release_ready=true."
}
if ([int]$manifestProjectTemplateOnboarding.source_file_count -ne 3) {
    throw "release_assets_manifest.json lost project template onboarding governance source_file_count."
}
if ([int]$manifestProjectTemplateOnboarding.entry_count -ne 4) {
    throw "release_assets_manifest.json lost project template onboarding governance entry_count."
}
if ([int]$manifestProjectTemplateOnboarding.approved_entry_count -ne 3) {
    throw "release_assets_manifest.json lost project template onboarding governance approved_entry_count."
}
if ([int]$manifestProjectTemplateOnboarding.not_required_entry_count -ne 1) {
    throw "release_assets_manifest.json lost project template onboarding governance not_required_entry_count."
}
if ([int]$manifestProjectTemplateOnboarding.release_blocker_count -ne 0) {
    throw "release_assets_manifest.json lost project template onboarding governance release_blocker_count."
}
if (@($manifestProjectTemplateOnboarding.schema_approval_status_summary).Count -ne 2) {
    throw "release_assets_manifest.json lost project template onboarding governance schema_approval_status_summary."
}
if ([string]::IsNullOrWhiteSpace([string]$manifestProjectTemplateOnboarding.source_json_display)) {
    throw "release_assets_manifest.json lost project template onboarding governance source_json_display."
}

foreach ($zipPath in @($installZipPath, $galleryZipPath, $evidenceZipPath)) {
    if (-not (Test-Path -LiteralPath $zipPath)) {
        throw "Expected ZIP archive was not created: $zipPath"
    }
}

Write-Host "Package release assets safety regression passed."

