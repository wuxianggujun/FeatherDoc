param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Convert-TestPathToRepoRelativeDisplay {
    param(
        [string]$Path,
        [string]$RepoRoot
    )

    $normalizedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $normalizedPath = [System.IO.Path]::GetFullPath($Path)
    if ($normalizedPath.StartsWith($normalizedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relativePath = $normalizedPath.Substring($normalizedRepoRoot.Length).TrimStart('\', '/')
        return ".\" + ($relativePath -replace '/', '\')
    }

    return $normalizedPath
}

function Convert-TestEvidencePathToPublicDisplay {
    param(
        [string]$Path,
        [string]$RepoRoot
    )

    $repoDisplay = Convert-TestPathToRepoRelativeDisplay -Path $Path -RepoRoot $RepoRoot
    $normalizedPath = [System.IO.Path]::GetFullPath($Path)
    if ($repoDisplay -ne $normalizedPath) {
        return $repoDisplay
    }

    $normalizedDisplay = $normalizedPath -replace '/', '\'
    foreach ($anchor in @("\output\", "\release-assets\", "\release-assets-ci\")) {
        $index = $normalizedDisplay.IndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
        if ($index -ge 0) {
            return ".\" + $normalizedDisplay.Substring($index + 1)
        }
    }

    return $normalizedPath
}

function New-TestReleaseNoteBundleContract {
    param(
        [string]$SummaryOutputDir,
        [string]$ReportDir,
        [string]$StartHerePath,
        [string]$ArtifactGuidePath,
        [string]$ReviewerChecklistPath,
        [string]$ReleaseHandoffPath,
        [string]$ReleaseBodyPath,
        [string]$ReleaseSummaryPath,
        [string]$RepoRoot
    )

    $entrypoints = @(
        [ordered]@{ id = "start_here"; path = $StartHerePath; location = "summary_root" },
        [ordered]@{ id = "artifact_guide"; path = $ArtifactGuidePath; location = "report" },
        [ordered]@{ id = "reviewer_checklist"; path = $ReviewerChecklistPath; location = "report" },
        [ordered]@{ id = "release_handoff"; path = $ReleaseHandoffPath; location = "report" },
        [ordered]@{ id = "release_body_zh_cn"; path = $ReleaseBodyPath; location = "report" },
        [ordered]@{ id = "release_summary_zh_cn"; path = $ReleaseSummaryPath; location = "report" }
    )

    foreach ($entrypoint in $entrypoints) {
        $entrypoint["path_display"] = Convert-TestPathToRepoRelativeDisplay -Path ([string]$entrypoint.path) -RepoRoot $RepoRoot
        $entrypoint["required"] = $true
    }

    return [ordered]@{
        status = "declared"
        output_root = $SummaryOutputDir
        report_dir = $ReportDir
        entrypoint_count = @($entrypoints).Count
        required_entrypoint_count = @($entrypoints).Count
        entrypoint_ids = @($entrypoints | ForEach-Object { [string]$_["id"] })
        entrypoints = @($entrypoints)
    }
}

function Convert-TestComparablePathValue {
    param($Value)

    if ($null -eq $Value) {
        return ""
    }

    return ([string]$Value) -replace '/', '\'
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
$installPrefix = Join-Path $resolvedWorkingDir "build-msvc-install"
$summaryOutputDir = Join-Path $resolvedWorkingDir "output\release-candidate-checks-ci"
$reportDir = Join-Path $summaryOutputDir "report"
$outputRoot = Join-Path $resolvedWorkingDir "release-assets-ci"

New-Item -ItemType Directory -Path (Join-Path $installPrefix "share\FeatherDoc") -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

$startHerePath = Join-Path $summaryOutputDir "START_HERE.md"
$releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
$releaseGovernanceHandoffPath = Join-Path $reportDir "release_governance_handoff.md"
$releaseBodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$releaseSummaryPath = Join-Path $reportDir "release_summary.zh-CN.md"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$contentControlSummaryPath = Join-Path $reportDir "content_control_data_binding_governance_summary.json"
$projectTemplateDeliveryReadinessSummaryPath = Join-Path $reportDir "project_template_delivery_readiness_summary.json"
$projectTemplateOnboardingGovernanceSummaryPath = Join-Path $reportDir "project_template_onboarding_governance_summary.json"
$summaryPath = Join-Path $reportDir "summary.json"

Set-Content -LiteralPath $startHerePath -Encoding UTF8 -Value @"
# START_HERE

- CI summary root: $resolvedRepoRoot\output\release-candidate-checks-ci\report
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks-ci\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks-ci\summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

foreach ($filePath in @($releaseBodyPath, $releaseSummaryPath)) {
    Set-Content -LiteralPath $filePath -Encoding UTF8 -Value @"
# Release Material

- Repo path: $resolvedRepoRoot\output\release-candidate-checks-ci\report
- Visual gate: skipped
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks-ci\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@
}

foreach ($filePath in @($artifactGuidePath, $reviewerChecklistPath)) {
    Set-Content -LiteralPath $filePath -Encoding UTF8 -Value @"
# Release Material

- Repo path: $resolvedRepoRoot\output\release-candidate-checks-ci\report
- Visual gate: skipped
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks-ci\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks-ci\summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@
}

Set-Content -LiteralPath $releaseHandoffPath -Encoding UTF8 -Value @"
# Release Material

- Repo path: $resolvedRepoRoot\output\release-candidate-checks-ci\report
- Visual gate: skipped
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks-ci\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

Set-Content -LiteralPath $finalReviewPath -Encoding UTF8 -Value @"
# Release Candidate Checks

- Final review root: $resolvedRepoRoot\output\release-candidate-checks-ci\report

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready
  - readiness_release_ready: True
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

Set-Content -LiteralPath $releaseGovernanceHandoffPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
- ``project_template_onboarding.schema_approval``: action=``review_schema_update_candidate`` source_schema=``featherdoc.project_template_onboarding_governance_report.v1``
  - source_report_display: ``.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json``
  - source_json_display: ``.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json``
  - project_template_onboarding_governance_contract:
    - source_schema: ``featherdoc.project_template_onboarding_governance_report.v1``
    - status: ``ready``
    - release_ready: ``True``
    - schema_approval_status_summary: ``approved``
    - source_report_display: ``.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json``
    - source_json_display: ``.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json``

## Governance Metrics

- numbering_catalog_governance.real_corpus_confidence: report=numbering_catalog_governance metric=real_corpus_confidence level=low score=56 source_schema=featherdoc.numbering_catalog_governance_report.v1
- table_layout_delivery_governance.delivery_quality: report=table_layout_delivery_governance metric=delivery_quality level=release_ready score=100 source_schema=featherdoc.table_layout_delivery_governance_report.v1
"@

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
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            input_docx = "samples/invoice.docx"
            input_docx_display = ".\samples\invoice.docx"
            template_name = "invoice-template"
            schema_target = "invoice"
            target_mode = "resolved-section-targets"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
}
($contentControlSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $contentControlSummaryPath -Encoding UTF8

$projectTemplateDeliveryReadinessSummary = [ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    status = "ready"
    release_ready = $true
    latest_schema_approval_gate_status = "passed"
    schema_approval_status_summary = @(
        [ordered]@{
            status = "approved"
            count = 4
        }
    )
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
    install_dir = $installPrefix
    gate_output_dir = (Join-Path $resolvedWorkingDir "output\word-visual-release-gate")
    release_version = "1.6.4"
    execution_status = "pass"
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
            source_report = ".\output\numbering-catalog-governance\summary.json"
            source_report_display = ".\output\numbering-catalog-governance\summary.json"
            source_json = ".\output\numbering-catalog-governance\summary.json"
            source_json_display = ".\output\numbering-catalog-governance\summary.json"
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
            source_report = ".\output\table-layout-delivery-governance\summary.json"
            source_report_display = ".\output\table-layout-delivery-governance\summary.json"
            source_json = ".\output\table-layout-delivery-governance\summary.json"
            source_json_display = ".\output\table-layout-delivery-governance\summary.json"
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
                pdf_floating_table_supported_geometry_count = 4
                pdf_floating_table_metadata_only_count = 5
                pdf_floating_table_tracked_geometry_count = 9
                pdf_floating_table_supported_geometry_percent = 44
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
    start_here = $startHerePath
    release_handoff = $releaseHandoffPath
    release_governance_handoff = $releaseGovernanceHandoffPath
    release_body_zh_cn = $releaseBodyPath
    release_summary_zh_cn = $releaseSummaryPath
    final_review = $finalReviewPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    release_note_bundle = New-TestReleaseNoteBundleContract `
        -SummaryOutputDir $summaryOutputDir `
        -ReportDir $reportDir `
        -StartHerePath $startHerePath `
        -ArtifactGuidePath $artifactGuidePath `
        -ReviewerChecklistPath $reviewerChecklistPath `
        -ReleaseHandoffPath $releaseHandoffPath `
        -ReleaseBodyPath $releaseBodyPath `
        -ReleaseSummaryPath $releaseSummaryPath `
        -RepoRoot $resolvedRepoRoot
    manifest_signoff_entrypoints = [ordered]@{
        status = "declared"
        release_assets_manifest = (Join-Path $outputRoot "v1.6.4\release_assets_manifest.json")
        required_entrypoint_count = 3
        entrypoints = @(
            [ordered]@{
                id = "start_here"
                path = $startHerePath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $startHerePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "artifact_guide"
                path = $artifactGuidePath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $artifactGuidePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "reviewer_checklist"
                path = $reviewerChecklistPath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $reviewerChecklistPath -RepoRoot $resolvedRepoRoot
                required = $true
            }
        )
        required_contracts = @(
            "project_template_delivery_readiness_contract",
            "project_template_onboarding_governance_contract"
        )
        required_fields = @(
            "status",
            "release_ready",
            "release_blocker_count",
            "warning_count",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )
        checklist_marker = "reviewer_manifest_scoped_project_template_trace"
    }
    project_template_readiness_checklist_entrypoints = [ordered]@{
        status = "declared"
        checklist_label = "Project template release readiness checklist"
        checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
        required_entrypoint_count = 3
        entrypoints = @(
            [ordered]@{
                id = "start_here"
                path = $startHerePath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $startHerePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "artifact_guide"
                path = $artifactGuidePath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $artifactGuidePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "reviewer_checklist"
                path = $reviewerChecklistPath
                path_display = Convert-TestPathToRepoRelativeDisplay -Path $reviewerChecklistPath -RepoRoot $resolvedRepoRoot
                required = $true
            }
        )
        checklist_marker = "release_entry_project_template_readiness_checklist_trace"
    }
    readme_gallery = [ordered]@{
        status = "visual_gate_skipped"
        assets_dir = (Join-Path $resolvedRepoRoot "docs\assets\readme")
    }
    steps = [ordered]@{
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installPrefix
        }
        visual_gate = [ordered]@{
            status = "skipped"
        }
    }
}
($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$packageScript = Join-Path $resolvedRepoRoot "scripts\package_release_assets.ps1"
& $packageScript `
    -SummaryJson $summaryPath `
    -OutputRoot $outputRoot `
    -AllowIncomplete `
    -KeepStaging

$stagingRoot = Join-Path $outputRoot "v1.6.4\staging"
$placeholderPath = Join-Path $stagingRoot "word-visual-release-gate\README.md"
$stagedStartHerePath = Join-Path $stagingRoot "release-candidate-checks\START_HERE.md"
$stagedSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\summary.json"
$stagedGovernanceHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_governance_handoff.md"
$stagedFinalReviewPath = Join-Path $stagingRoot "release-candidate-checks\report\final_review.md"
$stagedArtifactGuidePath = Join-Path $stagingRoot "release-candidate-checks\report\ARTIFACT_GUIDE.md"
$stagedReviewerChecklistPath = Join-Path $stagingRoot "release-candidate-checks\report\REVIEWER_CHECKLIST.md"
$manifestPath = Join-Path $outputRoot "v1.6.4\release_assets_manifest.json"
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json

Assert-NotContains -Path $manifestPath -UnexpectedText $resolvedRepoRoot -Label 'release_assets_manifest.json'
Assert-NotContains -Path $manifestPath -UnexpectedText '<windows-absolute-path>' -Label 'release_assets_manifest.json'
Assert-NotContains -Path $manifestPath -UnexpectedText '<unix-absolute-path>' -Label 'release_assets_manifest.json'

if (-not (Test-Path -LiteralPath $placeholderPath)) {
    throw "Expected incomplete visual-gate placeholder note was not created."
}

$placeholderContent = Get-Content -Raw -LiteralPath $placeholderPath
if ($placeholderContent -notmatch 'visual_gate=skipped') {
    throw "Incomplete visual-gate placeholder note does not mention skipped visual gate."
}

if ($manifest.visual_gate_status -ne "skipped") {
    throw "Release assets manifest did not record visual_gate_status=skipped."
}

if ([bool]$manifest.visual_gate_evidence_included) {
    throw "Release assets manifest incorrectly reported visual gate evidence as included."
}

if ($manifest.governance_metric_count -ne 3) {
    throw "Release assets manifest did not preserve governance_metric_count=3."
}

$manifestReleaseNoteBundle = $manifest.release_note_bundle
if ($null -eq $manifestReleaseNoteBundle) {
    throw "Release assets manifest lost release_note_bundle in AllowIncomplete mode."
}
if ([string]$manifestReleaseNoteBundle.status -ne "declared") {
    throw "Release assets manifest lost release_note_bundle status in AllowIncomplete mode."
}
if ([int]$manifestReleaseNoteBundle.entrypoint_count -ne 6) {
    throw "Release assets manifest lost release_note_bundle entrypoint count in AllowIncomplete mode."
}
if ([int]$manifestReleaseNoteBundle.required_entrypoint_count -ne 6) {
    throw "Release assets manifest lost release_note_bundle required entrypoint count in AllowIncomplete mode."
}
$manifestReleaseNoteBundleEntrypointsById = @{}
foreach ($entrypoint in @($manifestReleaseNoteBundle.entrypoints)) {
    $manifestReleaseNoteBundleEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks-ci\START_HERE.md"; location = "summary_root" },
        [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md"; location = "report" },
        [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md"; location = "report" },
        [ordered]@{ id = "release_handoff"; path = $releaseHandoffPath; path_display = ".\output\release-candidate-checks-ci\report\release_handoff.md"; location = "report" },
        [ordered]@{ id = "release_body_zh_cn"; path = $releaseBodyPath; path_display = ".\output\release-candidate-checks-ci\report\release_body.zh-CN.md"; location = "report" },
        [ordered]@{ id = "release_summary_zh_cn"; path = $releaseSummaryPath; path_display = ".\output\release-candidate-checks-ci\report\release_summary.zh-CN.md"; location = "report" }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not (@($manifestReleaseNoteBundle.entrypoint_ids | ForEach-Object { [string]$_ }) -contains $entrypointId)) {
        throw "Release assets manifest lost release_note_bundle entrypoint id '$entrypointId' in AllowIncomplete mode."
    }
    if (-not $manifestReleaseNoteBundleEntrypointsById.ContainsKey($entrypointId)) {
        throw "Release assets manifest lost release_note_bundle entrypoint '$entrypointId' in AllowIncomplete mode."
    }

    $entrypoint = $manifestReleaseNoteBundleEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "Release assets manifest lost required=true for release_note_bundle entrypoint '$entrypointId' in AllowIncomplete mode."
    }
    if ([string]$entrypoint.location -ne [string]$entrypointExpectation.location) {
        throw "Release assets manifest lost release_note_bundle entrypoint '$entrypointId' location in AllowIncomplete mode."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "Release assets manifest did not public-sanitize release_note_bundle entrypoint '$entrypointId' path in AllowIncomplete mode."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "Release assets manifest lost release_note_bundle entrypoint '$entrypointId' path_display in AllowIncomplete mode. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
}

$manifestSignoffEntrypoints = $manifest.manifest_signoff_entrypoints
if ($null -eq $manifestSignoffEntrypoints) {
    throw "Release assets manifest lost manifest_signoff_entrypoints in AllowIncomplete mode."
}
if ([string]$manifestSignoffEntrypoints.status -ne "declared") {
    throw "Release assets manifest lost manifest signoff status in AllowIncomplete mode."
}
if ([int]$manifestSignoffEntrypoints.required_entrypoint_count -ne 3) {
    throw "Release assets manifest lost manifest signoff required entrypoint count in AllowIncomplete mode."
}
$expectedManifestDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $manifestPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$manifestSignoffEntrypoints.release_assets_manifest -ne $expectedManifestDisplay) {
    throw "Release assets manifest lost packaged manifest signoff path in AllowIncomplete mode."
}
foreach ($requiredContract in @(
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract"
    )) {
    if (-not (@($manifestSignoffEntrypoints.required_contracts | ForEach-Object { [string]$_ }) -contains $requiredContract)) {
        throw "Release assets manifest lost manifest signoff required contract '$requiredContract' in AllowIncomplete mode."
    }
}
foreach ($requiredField in @(
        "status",
        "release_ready",
        "release_blocker_count",
        "warning_count",
        "schema_approval_status_summary",
        "source_report_display",
        "source_json_display"
    )) {
    if (-not (@($manifestSignoffEntrypoints.required_fields | ForEach-Object { [string]$_ }) -contains $requiredField)) {
        throw "Release assets manifest lost manifest signoff required field '$requiredField' in AllowIncomplete mode."
    }
}
if ([string]$manifestSignoffEntrypoints.checklist_marker -ne "reviewer_manifest_scoped_project_template_trace") {
    throw "Release assets manifest lost manifest signoff checklist marker in AllowIncomplete mode."
}

$manifestProjectTemplateChecklistEntrypoints = $manifest.project_template_readiness_checklist_entrypoints
if ($null -eq $manifestProjectTemplateChecklistEntrypoints) {
    throw "Release assets manifest lost project_template_readiness_checklist_entrypoints in AllowIncomplete mode."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.status -ne "declared") {
    throw "Release assets manifest lost project-template readiness checklist entrypoints status in AllowIncomplete mode."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_label -ne "Project template release readiness checklist") {
    throw "Release assets manifest lost project-template readiness checklist label in AllowIncomplete mode."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst") {
    throw "Release assets manifest lost project-template readiness checklist path in AllowIncomplete mode."
}
if ([int]$manifestProjectTemplateChecklistEntrypoints.required_entrypoint_count -ne 3) {
    throw "Release assets manifest lost project-template readiness checklist required entrypoint count in AllowIncomplete mode."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace") {
    throw "Release assets manifest lost project-template readiness checklist marker in AllowIncomplete mode."
}

$manifestProjectTemplateChecklistMaterialSafetyAudit = $manifest.release_entry_project_template_readiness_checklist_material_safety_audit
if ($null -eq $manifestProjectTemplateChecklistMaterialSafetyAudit) {
    throw "Release assets manifest lost release_entry_project_template_readiness_checklist_material_safety_audit in AllowIncomplete mode."
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.status -ne "passed") {
    throw "Release assets manifest lost project-template checklist material-safety audit pass status in AllowIncomplete mode."
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.audit_script -ne ".\scripts\assert_release_material_safety.ps1") {
    throw "Release assets manifest lost project-template checklist material-safety audit script in AllowIncomplete mode."
}
if ([int]$manifestProjectTemplateChecklistMaterialSafetyAudit.audited_entrypoint_count -ne 3) {
    throw "Release assets manifest lost project-template checklist material-safety audited entrypoint count in AllowIncomplete mode."
}
foreach ($entrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
    if (-not (@($manifestProjectTemplateChecklistMaterialSafetyAudit.audited_entrypoints | ForEach-Object { [string]$_ }) -contains $entrypointId)) {
        throw "Release assets manifest lost project-template checklist material-safety audited entrypoint '$entrypointId' in AllowIncomplete mode."
    }
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_label -ne "Project-template readiness checklist handoff evidence" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_field -ne "project_template_readiness_checklist_entrypoints_source_reports" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_source_schema -ne "featherdoc.release_candidate_summary" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.material_safety_marker -ne "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace") {
    throw "Release assets manifest lost project-template checklist material-safety audit identity in AllowIncomplete mode."
}

$manifestSignoffEntrypointsById = @{}
foreach ($entrypoint in @($manifestSignoffEntrypoints.entrypoints)) {
    $manifestSignoffEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks-ci\START_HERE.md" },
        [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md" },
        [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md" }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not $manifestSignoffEntrypointsById.ContainsKey($entrypointId)) {
        throw "Release assets manifest lost manifest signoff entrypoint '$entrypointId' in AllowIncomplete mode."
    }

    $entrypoint = $manifestSignoffEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "Release assets manifest lost required=true for manifest signoff entrypoint '$entrypointId' in AllowIncomplete mode."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "Release assets manifest did not public-sanitize manifest signoff entrypoint '$entrypointId' path in AllowIncomplete mode."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "Release assets manifest lost manifest signoff entrypoint '$entrypointId' path_display in AllowIncomplete mode. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
}

$manifestProjectTemplateChecklistEntrypointsById = @{}
foreach ($entrypoint in @($manifestProjectTemplateChecklistEntrypoints.entrypoints)) {
    $manifestProjectTemplateChecklistEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks-ci\START_HERE.md" },
        [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md" },
        [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md" }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not $manifestProjectTemplateChecklistEntrypointsById.ContainsKey($entrypointId)) {
        throw "Release assets manifest lost project-template readiness checklist entrypoint '$entrypointId' in AllowIncomplete mode."
    }

    $entrypoint = $manifestProjectTemplateChecklistEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "Release assets manifest lost required=true for project-template readiness checklist entrypoint '$entrypointId' in AllowIncomplete mode."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "Release assets manifest did not public-sanitize project-template readiness checklist entrypoint '$entrypointId' path in AllowIncomplete mode."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "Release assets manifest lost project-template readiness checklist entrypoint '$entrypointId' path_display in AllowIncomplete mode. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
}

if ($manifest.governance_metrics.Count -ne 3) {
    throw "Release assets manifest did not preserve all governance metrics."
}

if (-not (@($manifest.governance_metrics | ForEach-Object { $_.metric }) -contains "real_corpus_confidence")) {
    throw "Release assets manifest lost real_corpus_confidence governance metric."
}

if (-not (@($manifest.governance_metrics | ForEach-Object { $_.metric }) -contains "delivery_quality")) {
    throw "Release assets manifest lost delivery_quality governance metric."
}

$manifestStyleConfidence = @($manifest.governance_metrics | Where-Object {
    [string]$_.id -eq "style_catalog_governance.real_corpus_confidence"
}) | Select-Object -First 1
if ($null -eq $manifestStyleConfidence) {
    throw "Release assets manifest lost style_catalog_governance.real_corpus_confidence governance metric."
}

$manifestNumberingConfidence = $manifest.numbering_catalog_real_corpus_confidence
if ($null -eq $manifestNumberingConfidence) {
    throw "Release assets manifest lost numbering_catalog_real_corpus_confidence in AllowIncomplete mode."
}
if ([string]$manifestNumberingConfidence.id -ne "numbering_catalog_governance.real_corpus_confidence") {
    throw "Release assets manifest used the wrong real corpus confidence id for numbering."
}
if ([string]$manifestNumberingConfidence.metric -ne "real_corpus_confidence") {
    throw "Release assets manifest lost numbering real corpus confidence metric."
}
if ([string]$manifestNumberingConfidence.report_id -ne "numbering_catalog_governance") {
    throw "Release assets manifest used the wrong real corpus confidence report_id for numbering."
}
if ([string]$manifestNumberingConfidence.source_schema -ne "featherdoc.numbering_catalog_governance_report.v1") {
    throw "Release assets manifest used the wrong real corpus confidence source_schema for numbering."
}
if ([string]$manifestNumberingConfidence.source_report -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "Release assets manifest lost numbering real corpus confidence source_report in AllowIncomplete mode."
}
if ([string]$manifestNumberingConfidence.source_report_display -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "Release assets manifest lost numbering real corpus confidence source_report_display in AllowIncomplete mode."
}
if ([string]$manifestNumberingConfidence.source_json -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "Release assets manifest lost numbering real corpus confidence source_json in AllowIncomplete mode."
}
if ([string]$manifestNumberingConfidence.source_json_display -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "Release assets manifest lost numbering real corpus confidence source_json_display in AllowIncomplete mode."
}
if ([int]$manifestNumberingConfidence.score -ne 56) {
    throw "Release assets manifest lost numbering real corpus confidence score."
}
if ([string]$manifestNumberingConfidence.level -ne "low") {
    throw "Release assets manifest lost numbering real corpus confidence level."
}
if ($null -eq $manifestNumberingConfidence.details) {
    throw "Release assets manifest lost numbering real corpus confidence details in AllowIncomplete mode."
}
if ([int]$manifestNumberingConfidence.details.document_count -ne 2) {
    throw "Release assets manifest used the wrong real corpus confidence details for numbering in AllowIncomplete mode."
}
if ([int]$manifestNumberingConfidence.details.matched_document_count -ne 2) {
    throw "Release assets manifest lost numbering real corpus confidence matched document count in AllowIncomplete mode."
}
if ([int]$manifestNumberingConfidence.details.alignment_gap_count -ne 0) {
    throw "Release assets manifest lost numbering real corpus confidence alignment gap count in AllowIncomplete mode."
}
if (-not (@($manifestNumberingConfidence.details.matched_document_keys) -contains "contract.docx")) {
    throw "Release assets manifest lost numbering real corpus confidence matched document keys in AllowIncomplete mode."
}
$manifestNumberingPenaltyFactors = @($manifestNumberingConfidence.details.penalty_summary |
    ForEach-Object { [string]$_.factor })
if ($manifestNumberingPenaltyFactors -contains "style_catalog_only") {
    throw "Release assets manifest mirrored style catalog confidence details into numbering confidence in AllowIncomplete mode."
}
if (-not ($manifestNumberingPenaltyFactors -contains "style_numbering_issues")) {
    throw "Release assets manifest lost numbering real corpus confidence penalty summary in AllowIncomplete mode."
}

$manifestTableLayoutDeliveryQuality = $manifest.table_layout_delivery_quality
if ($null -eq $manifestTableLayoutDeliveryQuality) {
    throw "Release assets manifest lost table_layout_delivery_quality in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.id -ne "table_layout_delivery_governance.delivery_quality") {
    throw "Release assets manifest used the wrong table layout delivery quality id in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.metric -ne "delivery_quality") {
    throw "Release assets manifest lost table layout delivery quality metric in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.report_id -ne "table_layout_delivery_governance") {
    throw "Release assets manifest lost table layout delivery quality report_id in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_schema -ne "featherdoc.table_layout_delivery_governance_report.v1") {
    throw "Release assets manifest used the wrong table layout delivery quality source_schema in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_report -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "Release assets manifest lost table layout delivery quality source_report in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_report_display -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "Release assets manifest lost table layout delivery quality source_report_display in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_json -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "Release assets manifest lost table layout delivery quality source_json in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_json_display -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "Release assets manifest lost table layout delivery quality source_json_display in AllowIncomplete mode."
}
if ([int]$manifestTableLayoutDeliveryQuality.score -ne 100) {
    throw "Release assets manifest lost table layout delivery quality score in AllowIncomplete mode."
}
if ([string]$manifestTableLayoutDeliveryQuality.level -ne "release_ready") {
    throw "Release assets manifest lost table layout delivery quality level in AllowIncomplete mode."
}
if ($null -eq $manifestTableLayoutDeliveryQuality.details) {
    throw "Release assets manifest lost table layout delivery quality details in AllowIncomplete mode."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.document_count -ne 3) {
    throw "Release assets manifest lost table layout delivery quality document_count in AllowIncomplete mode."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.unresolved_item_count -ne 0) {
    throw "Release assets manifest lost table layout delivery quality unresolved_item_count in AllowIncomplete mode."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.pdf_floating_table_supported_geometry_percent -ne 44) {
    throw "Release assets manifest lost table layout delivery quality PDF floating table support percentage in AllowIncomplete mode."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.pdf_floating_table_tracked_geometry_count -ne 9) {
    throw "Release assets manifest lost table layout delivery quality PDF floating table tracked geometry count in AllowIncomplete mode."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.table_position_automatic_count -ne 0) {
    throw "Release assets manifest lost table layout delivery quality table_position_automatic_count in AllowIncomplete mode."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.table_position_review_count -ne 0) {
    throw "Release assets manifest lost table layout delivery quality table_position_review_count in AllowIncomplete mode."
}
if (-not (@($manifestTableLayoutDeliveryQuality.details.penalty_summary |
        ForEach-Object { [string]$_.factor }) -contains "floating_table_plans_pending")) {
    throw "Release assets manifest lost table layout delivery quality penalty summary in AllowIncomplete mode."
}

$contentControlContract = @($manifest.content_control_repair_contracts | Where-Object {
    [string]$_.id -eq "content_control_data_binding.bound_placeholder"
}) | Select-Object -First 1
if ($null -eq $contentControlContract) {
    throw "Release assets manifest lost content_control_data_binding.bound_placeholder repair contract in AllowIncomplete mode."
}
if ([string]$contentControlContract.source_schema -ne "featherdoc.content_control_data_binding_governance_report.v1") {
    throw "Release assets manifest lost content-control repair contract source_schema in AllowIncomplete mode."
}
$expectedContentControlSummaryDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $contentControlSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$contentControlContract.source_json_display -ne $expectedContentControlSummaryDisplay) {
    throw "Release assets manifest lost content-control repair contract source_json_display in AllowIncomplete mode."
}
if ([string]$contentControlContract.input_docx -ne "samples/invoice.docx") {
    throw "Release assets manifest lost content-control input_docx provenance in AllowIncomplete mode."
}
if ([string]$contentControlContract.input_docx_display -ne ".\samples\invoice.docx") {
    throw "Release assets manifest lost content-control input_docx_display provenance in AllowIncomplete mode."
}
if ([string]$contentControlContract.template_name -ne "invoice-template") {
    throw "Release assets manifest lost content-control template_name provenance in AllowIncomplete mode."
}
if ([string]$contentControlContract.schema_target -ne "invoice") {
    throw "Release assets manifest lost content-control schema_target provenance in AllowIncomplete mode."
}
if ([string]$contentControlContract.target_mode -ne "resolved-section-targets") {
    throw "Release assets manifest lost content-control target_mode provenance in AllowIncomplete mode."
}
if ([string]$contentControlContract.repair_strategy -ne "sync_bound_content_control") {
    throw "Release assets manifest lost sync_bound_content_control repair strategy in AllowIncomplete mode."
}
if ([string]$contentControlContract.repair_hint -ne "Rerun Custom XML sync or explicitly fill the bound content control before release.") {
    throw "Release assets manifest lost content-control repair_hint in AllowIncomplete mode."
}
if ([string]$contentControlContract.command_template -notmatch "sync-content-controls-from-custom-xml") {
    throw "Release assets manifest lost sync-content-controls-from-custom-xml command template in AllowIncomplete mode."
}

$projectTemplateDeliveryReadinessContract = $manifest.project_template_delivery_readiness_contract
if ($null -eq $projectTemplateDeliveryReadinessContract) {
    throw "Release assets manifest lost project_template_delivery_readiness_contract in AllowIncomplete mode."
}
$expectedProjectTemplateDeliveryReadinessDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $projectTemplateDeliveryReadinessSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$projectTemplateDeliveryReadinessContract.schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
    throw "Release assets manifest lost project template delivery readiness schema in AllowIncomplete mode."
}
if ([string]$projectTemplateDeliveryReadinessContract.source_schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
    throw "Release assets manifest lost project template delivery readiness source_schema in AllowIncomplete mode."
}
if ([string]$projectTemplateDeliveryReadinessContract.source_report_display -ne $expectedProjectTemplateDeliveryReadinessDisplay) {
    throw "Release assets manifest lost project template delivery readiness source_report_display in AllowIncomplete mode."
}
if ([string]$projectTemplateDeliveryReadinessContract.source_json_display -ne $expectedProjectTemplateDeliveryReadinessDisplay) {
    throw "Release assets manifest lost project template delivery readiness source_json_display in AllowIncomplete mode."
}
$projectTemplateDeliveryReadinessSchemaSummary = @($projectTemplateDeliveryReadinessContract.schema_approval_status_summary)
if ($projectTemplateDeliveryReadinessSchemaSummary.Count -ne 1 -or
    [string]$projectTemplateDeliveryReadinessSchemaSummary[0].status -ne "approved" -or
    [int]$projectTemplateDeliveryReadinessSchemaSummary[0].count -ne 4) {
    throw "Release assets manifest lost project template delivery readiness schema_approval_status_summary in AllowIncomplete mode."
}

$projectTemplateOnboardingGovernanceContract = $manifest.project_template_onboarding_governance_contract
if ($null -eq $projectTemplateOnboardingGovernanceContract) {
    throw "Release assets manifest lost project_template_onboarding_governance_contract in AllowIncomplete mode."
}
$expectedProjectTemplateOnboardingGovernanceDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $projectTemplateOnboardingGovernanceSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$projectTemplateOnboardingGovernanceContract.schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
    throw "Release assets manifest lost project template onboarding governance schema in AllowIncomplete mode."
}
if ([string]$projectTemplateOnboardingGovernanceContract.source_schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
    throw "Release assets manifest lost project template onboarding governance source_schema in AllowIncomplete mode."
}
if ([string]$projectTemplateOnboardingGovernanceContract.status -ne "ready") {
    throw "Release assets manifest lost project template onboarding governance status in AllowIncomplete mode."
}
if (-not [bool]$projectTemplateOnboardingGovernanceContract.release_ready) {
    throw "Release assets manifest lost project template onboarding governance release_ready=true in AllowIncomplete mode."
}
if ([string]$projectTemplateOnboardingGovernanceContract.source_report_display -ne $expectedProjectTemplateOnboardingGovernanceDisplay) {
    throw "Release assets manifest lost project template onboarding governance source_report_display in AllowIncomplete mode."
}
if ([string]$projectTemplateOnboardingGovernanceContract.source_json_display -ne $expectedProjectTemplateOnboardingGovernanceDisplay) {
    throw "Release assets manifest lost project template onboarding governance source_json_display in AllowIncomplete mode."
}

$stagedSummaryContent = Get-Content -Raw -LiteralPath $stagedSummaryPath
foreach ($expectedText in @("governance_metrics", "real_corpus_confidence", "delivery_quality", "content_control_data_binding_governance", "project_template_delivery_readiness", "project_template_onboarding_governance")) {
    if ($stagedSummaryContent -notmatch [regex]::Escape($expectedText)) {
        throw "Staged allow-incomplete summary lost governance metric text '$expectedText'."
    }
}

$stagedGovernanceHandoffContent = Get-Content -Raw -LiteralPath $stagedGovernanceHandoffPath
foreach ($expectedText in @(
    "Report Status",
    "project_template_delivery_readiness",
    "featherdoc.project_template_delivery_readiness_report.v1",
    "latest_schema_approval_gate_status",
    "schema_approval_status_summary",
    "source_report_display",
    "source_json_display",
    ".\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
    "project_template_onboarding.schema_approval",
    "project_template_onboarding_governance_contract",
    "featherdoc.project_template_onboarding_governance_report.v1",
    'status: `ready`',
    'release_ready: `True`',
    ".\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
    "Governance Metrics",
    "real_corpus_confidence",
    "numbering_catalog_governance.real_corpus_confidence",
    "featherdoc.numbering_catalog_governance_report.v1",
    "delivery_quality",
    "table_layout_delivery_governance.delivery_quality"
)) {
    if ($stagedGovernanceHandoffContent -notmatch [regex]::Escape($expectedText)) {
        throw "Staged allow-incomplete governance handoff lost '$expectedText'."
    }
}

$stagedFinalReviewContent = Get-Content -Raw -LiteralPath $stagedFinalReviewPath
foreach ($expectedText in @(
    "Release governance handoff details",
    "project_template_delivery_readiness",
    "project_template_onboarding.schema_approval",
    "featherdoc.project_template_onboarding_governance_report.v1",
    "project_template_onboarding_governance_contract",
    "status: ready",
    "release_ready: True",
    "schema_approval_status_summary: approved",
    "source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json",
    "source_json_display: .\output\project-template-onboarding-governance\summary.json",
    "readiness_status: ready",
    "readiness_release_ready: True"
)) {
    if ($stagedFinalReviewContent -notmatch [regex]::Escape($expectedText)) {
        throw "Staged allow-incomplete final_review.md lost '$expectedText'."
    }
}

$stagedStartHereContent = Get-Content -Raw -LiteralPath $stagedStartHerePath
foreach ($expectedText in @(
    "content_control_data_binding.bound_placeholder",
    "featherdoc.content_control_data_binding_governance_report.v1",
    "source_json_display",
    "input_docx=samples/invoice.docx",
    "template_name=invoice-template",
    "schema_target=invoice",
    "target_mode=resolved-section-targets",
    "repair_strategy",
    "repair_hint",
    "sync_bound_content_control",
    "command_template",
    "sync-content-controls-from-custom-xml",
    "project_template_delivery_readiness",
    "project_template_delivery_readiness_contract",
    "featherdoc.project_template_delivery_readiness_report.v1",
    "latest_schema_approval_gate_status",
    "schema_approval_status_summary=approved=4",
    "source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
    "source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
    "project_template_onboarding.schema_approval",
    "project_template_onboarding_governance_contract",
    "featherdoc.project_template_onboarding_governance_report.v1",
    "status=ready",
    "release_ready=True",
    "schema_approval_status_summary",
    "source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
    "source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
    "Project-template readiness checklist handoff evidence",
    "project_template_readiness_checklist_entrypoints_source_reports=1",
    "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
    "required_entrypoint_count=3",
    "entrypoints=start_here, artifact_guide, reviewer_checklist",
    "entrypoint_paths=",
    "start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md",
    "artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md",
    "reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md",
    "marker=release_entry_project_template_readiness_checklist_trace",
    "source_schema=featherdoc.release_candidate_summary",
    "source_report=.\output\release-candidate-checks-ci\summary.json",
    "numbering_catalog_governance.real_corpus_confidence",
    "table_layout_delivery_governance.delivery_quality",
    "pdf_floating_table_support_coverage",
    "pdf_floating_table_reviewer_focus",
    "metadata-only tblpPr",
    "catalog_coverage_percent=100",
    "matched_document_count=2",
    "alignment_gap_count=0",
    "matched_document_keys=contract.docx,invoice.docx",
    "style_numbering_issues(count=4, penalty=20)",
    "unresolved_item_count=0",
    "floating_table_plans_pending(count=0, penalty=0)"
)) {
    if ($stagedStartHereContent -notmatch [regex]::Escape($expectedText)) {
        throw "Staged allow-incomplete START_HERE lost '$expectedText'."
    }
}

$stagedArtifactGuideContent = Get-Content -Raw -LiteralPath $stagedArtifactGuidePath
foreach ($expectedText in @(
    "content_control_data_binding.bound_placeholder",
    "featherdoc.content_control_data_binding_governance_report.v1",
    "source_json_display",
    "input_docx=samples/invoice.docx",
    "template_name=invoice-template",
    "schema_target=invoice",
    "target_mode=resolved-section-targets",
    "repair_strategy",
    "repair_hint",
    "sync_bound_content_control",
    "command_template",
    "sync-content-controls-from-custom-xml",
    "project_template_delivery_readiness",
    "project_template_delivery_readiness_contract",
    "featherdoc.project_template_delivery_readiness_report.v1",
    "latest_schema_approval_gate_status",
    "schema_approval_status_summary=approved=4",
    "source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
    "source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
    "project_template_onboarding.schema_approval",
    "project_template_onboarding_governance_contract",
    "featherdoc.project_template_onboarding_governance_report.v1",
    "status=ready",
    "release_ready=True",
    "schema_approval_status_summary",
    "source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
    "source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
    "Project-template readiness checklist handoff evidence",
    "project_template_readiness_checklist_entrypoints_source_reports=1",
    "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
    "required_entrypoint_count=3",
    "entrypoints=start_here, artifact_guide, reviewer_checklist",
    "entrypoint_paths=",
    "start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md",
    "artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md",
    "reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md",
    "marker=release_entry_project_template_readiness_checklist_trace",
    "source_schema=featherdoc.release_candidate_summary",
    "source_report=.\output\release-candidate-checks-ci\summary.json",
    "numbering_catalog_governance.real_corpus_confidence",
    "featherdoc.numbering_catalog_governance_report.v1",
    "table_layout_delivery_governance.delivery_quality",
    "pdf_floating_table_support_coverage",
    "pdf_floating_table_reviewer_focus",
    "metadata-only tblpPr",
    "catalog_coverage_percent=100",
    "matched_document_count=2",
    "alignment_gap_count=0",
    "matched_document_keys=contract.docx,invoice.docx",
    "style_numbering_issues(count=4, penalty=20)",
    "unresolved_item_count=0",
    "floating_table_plans_pending(count=0, penalty=0)"
)) {
    if ($stagedArtifactGuideContent -notmatch [regex]::Escape($expectedText)) {
        throw "Staged allow-incomplete ARTIFACT_GUIDE lost '$expectedText'."
    }
}

$stagedReviewerChecklistContent = Get-Content -Raw -LiteralPath $stagedReviewerChecklistPath
foreach ($expectedText in @(
    "content_control_data_binding.bound_placeholder",
    "featherdoc.content_control_data_binding_governance_report.v1",
    "source_json_display",
    "input_docx=samples/invoice.docx",
    "template_name=invoice-template",
    "schema_target=invoice",
    "target_mode=resolved-section-targets",
    "repair_strategy",
    "repair_hint",
    "sync_bound_content_control",
    "command_template",
    "sync-content-controls-from-custom-xml",
    "project_template_delivery_readiness",
    "project_template_delivery_readiness_contract",
    "featherdoc.project_template_delivery_readiness_report.v1",
    "latest_schema_approval_gate_status",
    "schema_approval_status_summary=approved=4",
    "source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
    "source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
    "project_template_onboarding.schema_approval",
    "project_template_onboarding_governance_contract",
    "featherdoc.project_template_onboarding_governance_report.v1",
    "status=ready",
    "release_ready=True",
    "schema_approval_status_summary",
    "source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
    "source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
    "Project-template readiness checklist handoff evidence",
    "project_template_readiness_checklist_entrypoints_source_reports=1",
    "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
    "required_entrypoint_count=3",
    "entrypoints=start_here, artifact_guide, reviewer_checklist",
    "entrypoint_paths=",
    "start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md",
    "artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md",
    "reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md",
    "marker=release_entry_project_template_readiness_checklist_trace",
    "source_schema=featherdoc.release_candidate_summary",
    "source_report=.\output\release-candidate-checks-ci\summary.json",
    "numbering_catalog_governance.real_corpus_confidence",
    "table_layout_delivery_governance.delivery_quality",
    "pdf_floating_table_support_coverage",
    "pdf_floating_table_reviewer_focus",
    "metadata-only tblpPr",
    "catalog_coverage_percent=100",
    "matched_document_count=2",
    "alignment_gap_count=0",
    "matched_document_keys=contract.docx,invoice.docx",
    "style_numbering_issues(count=4, penalty=20)",
    "unresolved_item_count=0",
    "floating_table_plans_pending(count=0, penalty=0)"
)) {
    if ($stagedReviewerChecklistContent -notmatch [regex]::Escape($expectedText)) {
        throw "Staged allow-incomplete REVIEWER_CHECKLIST lost '$expectedText'."
    }
}

Write-Host "Package release assets allow-incomplete regression passed."

