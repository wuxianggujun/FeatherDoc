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

    $normalizedDisplay = ([System.IO.Path]::GetFullPath($Path)) -replace '/', '\'
    foreach ($anchor in @(
            "\output\",
            "\release-assets\",
            "\release-assets-ci\",
            "\build-msvc-install\",
            "\build-msvc-install"
        )) {
        $index = $normalizedDisplay.LastIndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
        if ($index -ge 0) {
            return ".\" + $normalizedDisplay.Substring($index + 1)
        }
    }

    $repoDisplay = Convert-TestPathToRepoRelativeDisplay -Path $Path -RepoRoot $RepoRoot
    $normalizedPath = [System.IO.Path]::GetFullPath($Path)
    if ($repoDisplay -ne $normalizedPath) {
        return $repoDisplay
    }

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
        $entrypoint["path_display"] = Convert-TestEvidencePathToPublicDisplay -Path ([string]$entrypoint.path) -RepoRoot $RepoRoot
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
- Content-control repair: content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks-ci\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks-ci\summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 fixed_layout_table_count=1 autofit_layout_table_count=1 unspecified_layout_table_count=1 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

foreach ($filePath in @($releaseBodyPath, $releaseSummaryPath)) {
    Set-Content -LiteralPath $filePath -Encoding UTF8 -Value @"
# Release Material

- Repo path: $resolvedRepoRoot\output\release-candidate-checks-ci\report
- Visual gate: skipped
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks-ci\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required
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
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 fixed_layout_table_count=1 autofit_layout_table_count=1 unspecified_layout_table_count=1 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@
}

foreach ($filePath in @($artifactGuidePath, $reviewerChecklistPath)) {
    Set-Content -LiteralPath $filePath -Encoding UTF8 -Value @"
# Release Material

- Repo path: $resolvedRepoRoot\output\release-candidate-checks-ci\report
- Visual gate: skipped
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks-ci\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks-ci\summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 fixed_layout_table_count=1 autofit_layout_table_count=1 unspecified_layout_table_count=1 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@
}

Set-Content -LiteralPath $releaseHandoffPath -Encoding UTF8 -Value @"
# Release Material

- Repo path: $resolvedRepoRoot\output\release-candidate-checks-ci\report
- Visual gate: skipped
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder action=sync_or_fill_bound_content_control source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks-ci\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json repair_action_classes=release_blocking,auto_repair_candidate,manual_confirmation_required
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
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 fixed_layout_table_count=1 autofit_layout_table_count=1 unspecified_layout_table_count=1 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution penalty_summary=floating_table_plans_pending(count=0, penalty=0)
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
            repair_action_classes = @("release_blocking", "auto_repair_candidate", "manual_confirmation_required")
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
    business_document_type_summary = @(
        [ordered]@{
            document_type = "invoice"
            count = 1
        },
        [ordered]@{
            document_type = "policy"
            count = 1
        }
    )
    corpus_role_summary = @(
        [ordered]@{
            corpus_role = "planned-business-template"
            count = 1
        },
        [ordered]@{
            corpus_role = "registered-business-template"
            count = 1
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
    onboarding_governance_next_action = [ordered]@{
        action = "publish_project_template"
        status = "ready"
        blocker_id = ""
        reason = "Project template delivery readiness is release-ready."
    }
    onboarding_governance_next_action_summary = @(
        [ordered]@{
            action = "publish_project_template"
            status = "ready"
            blocker_id = ""
            reason = "Project template delivery readiness is release-ready."
        }
    )
    onboarding_governance_next_action_group_count = 1
    requires_reviewer_action = $false
    reviewer_action_summary = "none"
    reviewer_action_reason = "latest_review_state=approved; no reviewer action required"
    reviewer_actions = @()
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
    business_document_type_summary = @(
        [ordered]@{
            document_type = "invoice"
            count = 1
        },
        [ordered]@{
            document_type = "policy"
            count = 1
        }
    )
    corpus_role_summary = @(
        [ordered]@{
            corpus_role = "planned-business-template"
            count = 1
        },
        [ordered]@{
            corpus_role = "registered-business-template"
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
    next_action = [ordered]@{
        action = "publish_project_template"
        status = "ready"
        blocker_id = ""
        reason = "Project template onboarding governance is release-ready."
    }
    next_action_summary = @(
        [ordered]@{
            action = "publish_project_template"
            status = "ready"
            blocker_id = ""
            reason = "Project template onboarding governance is release-ready."
        }
    )
    next_action_group_count = 1
    requires_reviewer_action = $false
    reviewer_action_summary = "none"
    reviewer_action_reason = "latest_review_state=approved; no reviewer action required"
    reviewer_actions = @()
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
                fixed_layout_table_count = 1
                autofit_layout_table_count = 1
                unspecified_layout_table_count = 1
                command_failure_count = 0
                unresolved_item_count = 0
                pdf_floating_table_supported_geometry_count = 4
                pdf_floating_table_metadata_only_count = 5
                pdf_floating_table_tracked_geometry_count = 9
                pdf_floating_table_supported_geometry_percent = 44
                metadata_only_fields = @("leftFromText", "rightFromText", "topFromText outside paragraph anchoring", "tblOverlap")
                review_required_fields = @("full Word-compatible floating table text wrapping", "table overlap avoidance and collision resolution")
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
                path_display = Convert-TestEvidencePathToPublicDisplay -Path $startHerePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "artifact_guide"
                path = $artifactGuidePath
                path_display = Convert-TestEvidencePathToPublicDisplay -Path $artifactGuidePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "reviewer_checklist"
                path = $reviewerChecklistPath
                path_display = Convert-TestEvidencePathToPublicDisplay -Path $reviewerChecklistPath -RepoRoot $resolvedRepoRoot
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
            "onboarding_governance_next_action",
            "onboarding_governance_next_action_summary",
            "onboarding_governance_next_action_group_count",
            "next_action",
            "next_action_summary",
            "next_action_group_count",
            "requires_reviewer_action",
            "reviewer_action_summary",
            "reviewer_action_reason",
            "reviewer_actions",
            "business_document_type_summary",
            "corpus_role_summary",
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
                path_display = Convert-TestEvidencePathToPublicDisplay -Path $startHerePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "artifact_guide"
                path = $artifactGuidePath
                path_display = Convert-TestEvidencePathToPublicDisplay -Path $artifactGuidePath -RepoRoot $resolvedRepoRoot
                required = $true
            },
            [ordered]@{
                id = "reviewer_checklist"
                path = $reviewerChecklistPath
                path_display = Convert-TestEvidencePathToPublicDisplay -Path $reviewerChecklistPath -RepoRoot $resolvedRepoRoot
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
