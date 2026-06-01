param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.IO.Compression.FileSystem

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

function Assert-LineContainsAll {
    param(
        [string]$Path,
        [string[]]$ExpectedFragments,
        [string]$Label
    )

    $lines = Get-Content -LiteralPath $Path
    foreach ($line in $lines) {
        $normalizedLine = $line -replace '/', '\'
        $matchesAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($normalizedLine -notmatch [regex]::Escape($normalizedFragment)) {
                $matchesAllFragments = $false
                break
            }
        }

        if ($matchesAllFragments) {
            return
        }
    }

    throw ("{0} does not contain one line with all expected fragments: {1}" -f $Label, ($ExpectedFragments -join ", "))
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

function Convert-TestComparableValue {
    param($Value)

    if ($null -eq $Value) {
        return ""
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$Value
}

function Convert-TestComparablePathValue {
    param($Value)

    return (Convert-TestComparableValue -Value $Value) -replace '/', '\'
}

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
$smokeTaskDir = Join-Path $gateOutputDir "review-tasks\document"
$fixedGridTaskDir = Join-Path $gateOutputDir "review-tasks\fixed-grid"
$sectionPageSetupTaskDir = Join-Path $gateOutputDir "review-tasks\section-page-setup"
$pageNumberFieldsTaskDir = Join-Path $gateOutputDir "review-tasks\page-number-fields"
$pdfGateOutputDir = Join-Path $resolvedWorkingDir "output\pdf-visual-release-gate"
$pdfGateReportDir = Join-Path $pdfGateOutputDir "report"
$pdfGateCopySearchDir = Join-Path $pdfGateReportDir "cjk-copy-search"
$pdfGateUnicodeReportDir = Join-Path $pdfGateOutputDir "unicode-font\report"
$outputRoot = Join-Path $resolvedWorkingDir "release-assets"

New-Item -ItemType Directory -Path (Join-Path $installPrefix "share\FeatherDoc") -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $smokeEvidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $fixedGridAggregateDir -Force | Out-Null
New-Item -ItemType Directory -Path $sectionPageSetupAggregateDir -Force | Out-Null
foreach ($reviewTaskDir in @($smokeTaskDir, $fixedGridTaskDir, $sectionPageSetupTaskDir, $pageNumberFieldsTaskDir)) {
    New-Item -ItemType Directory -Path (Join-Path $reviewTaskDir "report") -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $reviewTaskDir "report\review_result.json") -Encoding UTF8 -Value '{"review_verdict":"pass"}'
    Set-Content -LiteralPath (Join-Path $reviewTaskDir "report\final_review.md") -Encoding UTF8 -Value "# Final Review"
}
New-Item -ItemType Directory -Path $pdfGateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $pdfGateCopySearchDir -Force | Out-Null
New-Item -ItemType Directory -Path $pdfGateUnicodeReportDir -Force | Out-Null

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
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$pdfGateSummaryPath = Join-Path $pdfGateReportDir "summary.json"
$pdfGateAggregateContactSheetPath = Join-Path $pdfGateReportDir "aggregate-contact-sheet.png"
$pdfGateSummaryPathDisplay = ".\output\pdf-visual-release-gate\report\summary.json"
$pdfGateAggregateContactSheetPathDisplay = ".\output\pdf-visual-release-gate\report\aggregate-contact-sheet.png"
$pdfGateCliExportLogPath = Join-Path $pdfGateReportDir "pdf-cli-export-test.log"
$pdfGateRegressionLogPath = Join-Path $pdfGateReportDir "pdf-regression-test.log"
$pdfGateUnicodeLogPath = Join-Path $pdfGateReportDir "unicode-font.log"
$pdfGateUnicodeContactSheetPath = Join-Path $pdfGateUnicodeReportDir "full-contact-sheet.png"
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
$summaryPath = Join-Path $reportDir "summary.json"
$installedReadmePath = Join-Path $installPrefix "share\FeatherDoc\README.md"
$installedChangelogPath = Join-Path $installPrefix "share\FeatherDoc\CHANGELOG.md"
$projectTemplateChecklistPackagedAuditEvidenceLine = "Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, audited_entrypoints=start_here, artifact_guide, reviewer_checklist, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json"

Set-Content -LiteralPath $startHerePath -Encoding UTF8 -Value @"
# START_HERE

- Evidence root: $resolvedRepoRoot\output\release-candidate-checks\report
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
- $projectTemplateChecklistPackagedAuditEvidenceLine
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. penalty_summary=floating_table_plans_pending(count=0, penalty=0)
- PDF visual gate summary: $pdfGateSummaryPathDisplay
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 2
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 3
- PDF visual gate aggregate contact sheet: $pdfGateAggregateContactSheetPathDisplay
- PDF release readiness checklist: docs/pdf_release_readiness_checklist_zh.rst
"@

Set-Content -LiteralPath $releaseHandoffPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4

- Handoff path: $resolvedRepoRoot\output\release-candidate-checks\report\release_handoff.md
- Governance metrics: numbering_catalog_governance.real_corpus_confidence=low, table_layout_delivery_governance.delivery_quality=release_ready
- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

Set-Content -LiteralPath $releaseGovernanceHandoffPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
- ``project_template_onboarding.schema_approval``: action=``review_schema_update_candidate`` source_schema=``featherdoc.project_template_onboarding_governance_report.v1``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
  - project_template_onboarding_governance_contract:
    - source_schema: ``featherdoc.project_template_onboarding_governance_report.v1``
    - status: ``ready``
    - release_ready: ``True``
    - schema_approval_status_summary: ``approved``
    - source_report_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
    - source_json_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``

## Governance Metrics

- numbering_catalog_governance.real_corpus_confidence: report=numbering_catalog_governance metric=real_corpus_confidence level=low score=56 source_schema=featherdoc.numbering_catalog_governance_report.v1
- table_layout_delivery_governance.delivery_quality: report=table_layout_delivery_governance metric=delivery_quality level=release_ready score=100 source_schema=featherdoc.table_layout_delivery_governance_report.v1

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``$pdfGateSummaryPathDisplay``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``$pdfGateAggregateContactSheetPathDisplay``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``2``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``3``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``$pdfBoundedCtestSubsetsText``
    - pdf_bounded_ctest_summary_json_display: ``$pdfBoundedCtestSummaryJsonDisplayText``
- Word visual standard review metadata source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - word_visual_standard_review_metadata_count: ``4``
    - word_visual_standard_review_task_keys: ``smoke, fixed_grid, section_page_setup, page_number_fields``
    - word_visual_standard_review_status_summary: ``reviewed=4``
    - word_visual_standard_review_verdict_summary: ``pass=4``
    - ``smoke``: review_task_key=``document`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``
      - label: ``Word visual smoke``
      - reviewed_at: ``2026-04-12T12:10:00``
      - review_result_path: ``.\output\word-visual-release-gate\review-tasks\document\report\review_result.json``
      - final_review_path: ``.\output\word-visual-release-gate\review-tasks\document\report\final_review.md``
    - ``fixed_grid``: review_task_key=``fixed_grid`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``
    - ``section_page_setup``: review_task_key=``section_page_setup`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``
    - ``page_number_fields``: review_task_key=``page_number_fields`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``
"@

Set-Content -LiteralPath $releaseBodyPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷璇存槑

- 璇佹嵁鐩綍锛?resolvedRepoRoot\output\word-visual-release-gate\report
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷鎽樿

- 鎽樿璺緞锛?resolvedRepoRoot\output\release-candidate-checks\report\release_summary.zh-CN.md
- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

Set-Content -LiteralPath $finalReviewPath -Encoding UTF8 -Value @"
# Release Candidate Checks

- Final review root: $resolvedRepoRoot\output\release-candidate-checks\report

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

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 3 visual baselines, 2 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: $pdfGateSummaryPathDisplay
- PDF visual gate contact sheet: $pdfGateAggregateContactSheetPathDisplay
"@

Set-Content -LiteralPath $artifactGuidePath -Encoding UTF8 -Value @"
# Artifact Guide

- Report root: $resolvedRepoRoot\output\release-candidate-checks\report
- Governance metric: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Governance metric: table_layout_delivery_governance.delivery_quality release_ready 100 source_schema=featherdoc.table_layout_delivery_governance_report.v1 table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. penalty_summary=floating_table_plans_pending(count=0, penalty=0)
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding governance: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
- $projectTemplateChecklistPackagedAuditEvidenceLine
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. penalty_summary=floating_table_plans_pending(count=0, penalty=0)
- Content-control provenance: input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- PDF visual gate summary: $pdfGateSummaryPathDisplay
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 2
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 3
- PDF visual gate aggregate contact sheet: $pdfGateAggregateContactSheetPathDisplay
- PDF release readiness checklist: docs/pdf_release_readiness_checklist_zh.rst
"@

Set-Content -LiteralPath $reviewerChecklistPath -Encoding UTF8 -Value @"
# Reviewer Checklist

- Review root: $resolvedRepoRoot\output\word-visual-release-gate\report
- Confirm governance_metrics before publishing release assets.
- Confirm numbering_catalog_governance.real_corpus_confidence catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20) before publishing release assets.
- Confirm content-control provenance input_docx=samples/invoice.docx input_docx_display=.\samples\invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets before release.
- Check content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json before release.
- Confirm project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json before release.
- Check project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json before release.
- Confirm Project template release readiness checklist docs/project_template_release_readiness_checklist_zh.rst before release.
- Confirm release governance handoff carries project-template readiness checklist entrypoint evidence: Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json.
- Confirm release governance handoff carries project-template readiness checklist packaged audit evidence: $projectTemplateChecklistPackagedAuditEvidenceLine.
- Confirm table_layout_delivery_governance.delivery_quality table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 pdf_floating_table_support_coverage=4/9 supported (44 percent) pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release. penalty_summary=floating_table_plans_pending(count=0, penalty=0) before release.
- Confirm PDF visual gate summary $pdfGateSummaryPathDisplay with 2 CJK copy/search samples and 3 visual baselines before release.
- Confirm PDF visual gate aggregate contact sheet $pdfGateAggregateContactSheetPathDisplay before release.
- Confirm PDF release readiness checklist docs/pdf_release_readiness_checklist_zh.rst before release.
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
        review_verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:10:00"
        review_method = "operator_supplied"
        task = [ordered]@{
            task_dir = $smokeTaskDir
        }
    }
    fixed_grid = [ordered]@{
        aggregate_evidence_dir = $fixedGridAggregateDir
        review_verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:20:00"
        review_method = "operator_supplied"
        task = [ordered]@{
            task_dir = $fixedGridTaskDir
        }
    }
    section_page_setup = [ordered]@{
        review_verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:30:00"
        review_method = "operator_supplied"
        task = [ordered]@{
            task_dir = $sectionPageSetupTaskDir
        }
    }
    page_number_fields = [ordered]@{
        review_verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:40:00"
        review_method = "operator_supplied"
        task = [ordered]@{
            task_dir = $pageNumberFieldsTaskDir
        }
    }
}
($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

[System.IO.File]::WriteAllBytes($pdfGateAggregateContactSheetPath, [byte[]](0x89, 0x50, 0x4E, 0x47))
Set-Content -LiteralPath $pdfGateCliExportLogPath -Encoding UTF8 -Value "PDF CLI export passed from $resolvedRepoRoot"
Set-Content -LiteralPath $pdfGateRegressionLogPath -Encoding UTF8 -Value "PDF regression passed from $resolvedRepoRoot"
Set-Content -LiteralPath $pdfGateUnicodeLogPath -Encoding UTF8 -Value "Unicode font smoke passed from $resolvedRepoRoot"
[System.IO.File]::WriteAllBytes($pdfGateUnicodeContactSheetPath, [byte[]](0x89, 0x50, 0x4E, 0x47))

$pdfVisualGateSummary = [ordered]@{
    generated_at = "2026-04-12T12:00:00"
    verdict = "pass"
    repo_root = $resolvedRepoRoot
    output_dir = $pdfGateOutputDir
    aggregate_contact_sheet = $pdfGateAggregateContactSheetPath
    cjk_manifest_count = 43
    visual_baseline_manifest_count = 42
    logs = [ordered]@{
        pdf_cli_export = $pdfGateCliExportLogPath
        pdf_regression = $pdfGateRegressionLogPath
        cjk_copy_search = $pdfGateCopySearchDir
        unicode_font = $pdfGateUnicodeLogPath
    }
    cjk_copy_search = @(
        [ordered]@{
            sample_id = "cjk-text"
            pdf_path = Join-Path $resolvedWorkingDir "build\cjk-text.pdf"
            summary_path = Join-Path $pdfGateCopySearchDir "cjk-text-summary.json"
            text_output_path = Join-Path $pdfGateCopySearchDir "cjk-text.txt"
            expected_text = @("中文文本路径回归样本")
            matched_text = @("中文文本路径回归样本")
            missing_text = @()
            page_count = 1
        },
        [ordered]@{
            sample_id = "contract-cjk-style"
            pdf_path = Join-Path $resolvedWorkingDir "build\contract-cjk-style.pdf"
            summary_path = Join-Path $pdfGateCopySearchDir "contract-cjk-style-summary.json"
            text_output_path = Join-Path $pdfGateCopySearchDir "contract-cjk-style.txt"
            expected_text = @("SERVICE AGREEMENT", "中文合同样本")
            matched_text = @("SERVICE AGREEMENT", "中文合同样本")
            missing_text = @()
            page_count = 1
        }
    )
    baselines = @(
        [ordered]@{ sample_id = "pdf-basic"; baseline_pdf = Join-Path $resolvedWorkingDir "baseline\pdf-basic.pdf" },
        [ordered]@{ sample_id = "pdf-cjk"; baseline_pdf = Join-Path $resolvedWorkingDir "baseline\pdf-cjk.pdf" },
        [ordered]@{ sample_id = "pdf-table"; baseline_pdf = Join-Path $resolvedWorkingDir "baseline\pdf-table.pdf" }
    )
}
($pdfVisualGateSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $pdfGateSummaryPath -Encoding UTF8

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
            input_docx = "samples/invoice.docx"
            input_docx_display = ".\samples\invoice.docx"
            template_name = "invoice-template"
            schema_target = "invoice"
            target_mode = "resolved-section-targets"
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
    release_handoff = $releaseHandoffPath
    release_body_zh_cn = $releaseBodyPath
    release_summary_zh_cn = $releaseSummaryPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    start_here = $startHerePath
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
    pdf_visual_gate_summary_json = $pdfGateSummaryPath
    pdf_visual_gate = [ordered]@{
        requested = $true
        status = "available"
        summary_json = $pdfGateSummaryPath
    }
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
            smoke_review_result_path = (Join-Path $smokeTaskDir "report\review_result.json")
            smoke_final_review_path = (Join-Path $smokeTaskDir "report\final_review.md")
            fixed_grid_review_result_path = (Join-Path $fixedGridTaskDir "report\review_result.json")
            fixed_grid_final_review_path = (Join-Path $fixedGridTaskDir "report\final_review.md")
            section_page_setup_review_result_path = (Join-Path $sectionPageSetupTaskDir "report\review_result.json")
            section_page_setup_final_review_path = (Join-Path $sectionPageSetupTaskDir "report\final_review.md")
            page_number_fields_review_result_path = (Join-Path $pageNumberFieldsTaskDir "report\review_result.json")
            page_number_fields_final_review_path = (Join-Path $pageNumberFieldsTaskDir "report\final_review.md")
        }
        pdf_visual_gate = [ordered]@{
            requested = $true
            status = "available"
            summary_json = $pdfGateSummaryPath
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

$packageScript = Join-Path $resolvedRepoRoot "scripts\package_release_assets.ps1"
& $packageScript `
    -SummaryJson $summaryPath `
    -OutputRoot $outputRoot `
    -KeepStaging

$stagingRoot = Join-Path $outputRoot "v1.6.4\staging"
$stagedSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\summary.json"
$stagedGateSummaryPath = Join-Path $stagingRoot "word-visual-release-gate\report\gate_summary.json"
$stagedPdfGateSummaryPath = Join-Path $stagingRoot "pdf-visual-release-gate\report\summary.json"
$stagedPdfGateAggregateContactSheetPath = Join-Path $stagingRoot "pdf-visual-release-gate\report\aggregate-contact-sheet.png"
$stagedPdfGateUnicodeContactSheetPath = Join-Path $stagingRoot "pdf-visual-release-gate\unicode-font\report\full-contact-sheet.png"
$stagedHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_handoff.md"
$stagedGovernanceHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_governance_handoff.md"
$stagedReleaseBodyPath = Join-Path $stagingRoot "release-candidate-checks\report\release_body.zh-CN.md"
$stagedReleaseSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\release_summary.zh-CN.md"
$stagedFinalReviewPath = Join-Path $stagingRoot "release-candidate-checks\report\final_review.md"
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
$expectedRelativePdfGateSummary = ".\$relativeWorkingDir\output\pdf-visual-release-gate\report\summary.json"
$expectedRelativePdfGateRoot = ".\$relativeWorkingDir\output\pdf-visual-release-gate"
$expectedSmokeReviewResultPath = Convert-TestEvidencePathToPublicDisplay `
    -Path (Join-Path $smokeTaskDir "report\review_result.json") `
    -RepoRoot $resolvedRepoRoot
$expectedSmokeFinalReviewPath = Convert-TestEvidencePathToPublicDisplay `
    -Path (Join-Path $smokeTaskDir "report\final_review.md") `
    -RepoRoot $resolvedRepoRoot
$stagedSummary = Get-Content -Raw -LiteralPath $stagedSummaryPath | ConvertFrom-Json
$stagedGateSummary = Get-Content -Raw -LiteralPath $stagedGateSummaryPath | ConvertFrom-Json
$stagedPdfGateSummary = Get-Content -Raw -LiteralPath $stagedPdfGateSummaryPath | ConvertFrom-Json
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json

Assert-NotContains -Path $stagedSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged summary.json'
Assert-NotContains -Path $stagedGateSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged gate_summary.json'
Assert-NotContains -Path $stagedPdfGateSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged PDF visual gate summary.json'
Assert-NotContains -Path $manifestPath -UnexpectedText $resolvedRepoRoot -Label 'release_assets_manifest.json'
Assert-NotContains -Path $manifestPath -UnexpectedText '<windows-absolute-path>' -Label 'release_assets_manifest.json'
Assert-NotContains -Path $manifestPath -UnexpectedText '<unix-absolute-path>' -Label 'release_assets_manifest.json'
Assert-NotContains -Path $stagedHandoffPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_handoff.md'
Assert-NotContains -Path $stagedGovernanceHandoffPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_governance_handoff.md'
Assert-NotContains -Path $stagedReleaseBodyPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_body.zh-CN.md'
Assert-NotContains -Path $stagedReleaseSummaryPath -UnexpectedText $resolvedRepoRoot -Label 'staged release_summary.zh-CN.md'
Assert-NotContains -Path $stagedFinalReviewPath -UnexpectedText $resolvedRepoRoot -Label 'staged final_review.md'
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
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'source_report_display' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'source_json_display' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'latest_schema_approval_gate_status' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'schema_approval_status_summary' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText '.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'status: `ready`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'release_ready: `True`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText '.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'PDF visual gate evidence source reports: `1`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'full_visual_gate_status: `pass`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_verdict: `pass`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_finalizable: `True`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_summary_json_display' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_aggregate_contact_sheet_display' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_cjk_manifest_count: `43`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_cjk_copy_search_count: `2`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_visual_baseline_manifest_count: `42`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_visual_gate_visual_baseline_count: `3`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_bounded_ctest_summary_count: `7`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_bounded_ctest_pass_count: `7`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_bounded_ctest_skipped_test_count: `0`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'pdf_bounded_ctest_selected_test_count: `70`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText "pdf_bounded_ctest_subsets: ``$pdfBoundedCtestSubsetsText``" -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText "pdf_bounded_ctest_summary_json_display: ``$pdfBoundedCtestSummaryJsonDisplayText``" -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'Word visual standard review metadata source reports: `1`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'word_visual_standard_review_metadata_count: `4`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'word_visual_standard_review_task_keys: `smoke, fixed_grid, section_page_setup, page_number_fields`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'word_visual_standard_review_status_summary: `reviewed=4`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'word_visual_standard_review_verdict_summary: `pass=4`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText '`smoke`: review_task_key=`document` verdict=`pass` review_status=`reviewed` review_method=`operator_supplied`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'reviewed_at: `2026-04-12T12:10:00`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'review_result_path: `.\output\word-visual-release-gate\review-tasks\document\report\review_result.json`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedGovernanceHandoffPath -ExpectedText 'final_review_path: `.\output\word-visual-release-gate\review-tasks\document\report\final_review.md`' -Label 'staged release_governance_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'project_template_delivery_readiness_contract:' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_schema: featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'latest_schema_approval_gate_status: passed' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'schema_approval_status_summary: approved=4' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'project_template_onboarding_governance_contract:' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_schema: featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'status: ready' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'release_ready: True' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'schema_approval_status_summary: approved' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedHandoffPath -ExpectedText 'source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json' -Label 'staged release_handoff.md'
Assert-Contains -Path $stagedReleaseBodyPath -ExpectedText 'project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_body.zh-CN.md'
Assert-Contains -Path $stagedReleaseBodyPath -ExpectedText 'project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json' -Label 'staged release_body.zh-CN.md'
Assert-Contains -Path $stagedReleaseSummaryPath -ExpectedText 'project-template readiness governance contract' -Label 'staged release_summary.zh-CN.md'
Assert-Contains -Path $stagedReleaseSummaryPath -ExpectedText 'status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json' -Label 'staged release_summary.zh-CN.md'
Assert-Contains -Path $stagedReleaseSummaryPath -ExpectedText 'project-template onboarding governance contract' -Label 'staged release_summary.zh-CN.md'
Assert-Contains -Path $stagedReleaseSummaryPath -ExpectedText 'status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json' -Label 'staged release_summary.zh-CN.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'Release governance handoff details' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'featherdoc.project_template_onboarding_governance_report.v1' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'project_template_onboarding_governance_contract' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'status: ready' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'release_ready: True' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'schema_approval_status_summary: approved' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'readiness_status: ready' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'readiness_release_ready: True' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate: loaded' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate verdict: pass' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate counts: 3 visual baselines, 2 CJK copy/search' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate finalizable: True' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate summary:' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'PDF visual gate contact sheet:' -Label 'staged final_review.md'
Assert-Contains -Path $stagedFinalReviewPath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged final_review.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'source_report_display' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'source_json_display' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'input_docx=samples/invoice.docx' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'template_name=invoice-template' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'schema_target=invoice' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'target_mode=resolved-section-targets' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'repair_strategy' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'repair_hint' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'sync_bound_content_control' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'command_template' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_delivery_readiness' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'status: ready release_ready: True' -Label 'staged START_HERE.md'
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
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF visual gate summary:' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF CJK manifest samples: 43' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF CJK copy/search samples: 2' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF visual baseline manifest samples: 42' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'PDF visual baselines: 3' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedStartHerePath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged START_HERE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'source_report_display' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'source_json_display' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'input_docx=samples/invoice.docx' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'template_name=invoice-template' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'schema_target=invoice' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'target_mode=resolved-section-targets' -Label 'staged ARTIFACT_GUIDE.md'
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
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'status: ready release_ready: True' -Label 'staged ARTIFACT_GUIDE.md'
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
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF visual gate summary:' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF CJK manifest samples: 43' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF CJK copy/search samples: 2' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF visual baseline manifest samples: 42' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'PDF visual baselines: 3' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedArtifactGuidePath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged ARTIFACT_GUIDE.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'featherdoc.content_control_data_binding_governance_report.v1' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'source_report_display' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'source_json_display' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'input_docx=samples/invoice.docx' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'template_name=invoice-template' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'schema_target=invoice' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'target_mode=resolved-section-targets' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'repair_strategy' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'repair_hint' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'command_template' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_delivery_readiness' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'project_template_delivery_readiness_contract' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'featherdoc.project_template_delivery_readiness_report.v1' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'status: ready release_ready: True' -Label 'staged REVIEWER_CHECKLIST.md'
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
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'Confirm PDF visual gate summary' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText '2 CJK copy/search samples' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText '3 visual baselines' -Label 'staged REVIEWER_CHECKLIST.md'
Assert-Contains -Path $stagedReviewerChecklistPath -ExpectedText 'aggregate-contact-sheet.png' -Label 'staged REVIEWER_CHECKLIST.md'
foreach ($entrypointMaterial in @(
        @{ Path = $stagedStartHerePath; Label = 'staged START_HERE.md' },
        @{ Path = $stagedArtifactGuidePath; Label = 'staged ARTIFACT_GUIDE.md' },
        @{ Path = $stagedReviewerChecklistPath; Label = 'staged REVIEWER_CHECKLIST.md' }
    )) {
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'pdf_floating_table_support_coverage' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'pdf_floating_table_reviewer_focus' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'metadata-only tblpPr' -Label $entrypointMaterial.Label
}
foreach ($entrypointMaterial in @(
        @{ Path = $stagedStartHerePath; Label = 'staged START_HERE.md' },
        @{ Path = $stagedArtifactGuidePath; Label = 'staged ARTIFACT_GUIDE.md' },
        @{ Path = $stagedReviewerChecklistPath; Label = 'staged REVIEWER_CHECKLIST.md' }
    )) {
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'Project-template readiness checklist handoff evidence' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'project_template_readiness_checklist_entrypoints_source_reports=1' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'checklist_path=docs/project_template_release_readiness_checklist_zh.rst' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'required_entrypoint_count=3' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'entrypoints=start_here, artifact_guide, reviewer_checklist' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'entrypoint_paths=' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'marker=release_entry_project_template_readiness_checklist_trace' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'source_schema=featherdoc.release_candidate_summary' -Label $entrypointMaterial.Label
    Assert-Contains -Path $entrypointMaterial.Path -ExpectedText 'source_report=.\output\release-candidate-checks\summary.json' -Label $entrypointMaterial.Label
    Assert-LineContainsAll -Path $entrypointMaterial.Path -ExpectedFragments @(
        'Project-template readiness checklist packaged audit evidence',
        'release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1',
        'status=passed',
        'audit_script=.\scripts\assert_release_material_safety.ps1',
        'audited_entrypoint_count=3',
        'audited_entrypoints=start_here, artifact_guide, reviewer_checklist',
        'compact_evidence_label=Project-template readiness checklist handoff evidence',
        'compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports',
        'compact_evidence_source_schema=featherdoc.release_candidate_summary',
        'checklist_path=docs/project_template_release_readiness_checklist_zh.rst',
        'checklist_marker=release_entry_project_template_readiness_checklist_trace',
        'material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace',
        'source_schema=featherdoc.release_candidate_summary',
        'source_report=.\output\release-blocker-rollup\summary.json'
    ) -Label $entrypointMaterial.Label
}
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'sync_bound_content_control' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'command_template' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'sync-content-controls-from-custom-xml' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'samples/invoice.docx' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'invoice-template' -Label 'staged content-control summary'
Assert-Contains -Path $stagedContentControlSummaryPath -ExpectedText 'resolved-section-targets' -Label 'staged content-control summary'
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
if ($stagedSummary.pdf_visual_gate_summary_json -ne $expectedRelativePdfGateSummary) {
    throw "staged summary.json did not rewrite pdf_visual_gate_summary_json to the expected relative path."
}
if ($stagedPdfGateSummary.output_dir -ne ".\$relativeWorkingDir\output\pdf-visual-release-gate") {
    throw "staged PDF visual gate summary.json did not rewrite output_dir to the expected relative path."
}
if ($stagedPdfGateSummary.cjk_copy_search.Count -ne 2) {
    throw "staged PDF visual gate summary.json lost CJK copy/search entries."
}
if ($stagedPdfGateSummary.baselines.Count -ne 3) {
    throw "staged PDF visual gate summary.json lost visual baseline entries."
}
foreach ($pdfEvidencePath in @($stagedPdfGateAggregateContactSheetPath, $stagedPdfGateUnicodeContactSheetPath)) {
    if (-not (Test-Path -LiteralPath $pdfEvidencePath)) {
        throw "Expected staged PDF visual gate evidence was not created: $pdfEvidencePath"
    }
}
if ([string]$manifest.pdf_visual_gate_status -ne "loaded") {
    throw "release_assets_manifest.json did not record pdf_visual_gate_status=loaded."
}
if (-not [bool]$manifest.pdf_visual_gate_evidence_included) {
    throw "release_assets_manifest.json did not record PDF visual gate evidence as included."
}
if ([string]$manifest.pdf_visual_gate_evidence.summary_json -ne $expectedRelativePdfGateSummary) {
    throw "release_assets_manifest.json did not preserve the PDF visual gate summary display path."
}
if ([string]$manifest.pdf_visual_gate_evidence.verdict -ne "pass") {
    throw "release_assets_manifest.json lost the PDF visual gate verdict."
}
if ([string]$manifest.pdf_visual_gate_evidence.full_visual_gate_status -ne "pass") {
    throw "release_assets_manifest.json lost the full PDF visual gate status."
}
if ([string]$manifest.pdf_visual_gate_evidence.aggregate_contact_sheet -notmatch "aggregate-contact-sheet.png") {
    throw "release_assets_manifest.json lost the PDF visual gate aggregate contact sheet."
}
if ([string]$manifest.pdf_visual_gate_evidence.cjk_manifest_count -ne "43") {
    throw "release_assets_manifest.json lost the PDF CJK manifest sample count."
}
if ([string]$manifest.pdf_visual_gate_evidence.cjk_copy_search_count -ne "2") {
    throw "release_assets_manifest.json lost the PDF CJK copy/search sample count."
}
if ([string]$manifest.pdf_visual_gate_evidence.cjk_missing_text_count -ne "0") {
    throw "release_assets_manifest.json lost the PDF CJK missing text count."
}
if ([string]$manifest.pdf_visual_gate_evidence.visual_baseline_count -ne "3") {
    throw "release_assets_manifest.json lost the PDF visual baseline count."
}
if ([string]$manifest.pdf_visual_gate_evidence.visual_baseline_manifest_count -ne "42") {
    throw "release_assets_manifest.json lost the PDF visual baseline manifest sample count."
}
if (-not [bool]$manifest.pdf_bounded_ctest_evidence_included) {
    throw "release_assets_manifest.json did not record PDF bounded CTest evidence as included."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.status -ne "pass") {
    throw "release_assets_manifest.json lost the bounded PDF CTest pass status."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.summary_count -ne "7") {
    throw "release_assets_manifest.json lost the bounded PDF CTest summary_count."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.pass_count -ne "7") {
    throw "release_assets_manifest.json lost the bounded PDF CTest pass_count."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.selected_test_count -ne "70") {
    throw "release_assets_manifest.json lost the bounded PDF CTest selected_test_count."
}
if ([string]$manifest.pdf_bounded_ctest_evidence.skipped_test_count -ne "0") {
    throw "release_assets_manifest.json lost the bounded PDF CTest skipped_test_count."
}
$manifestPdfBoundedCtestSubsets = @($manifest.pdf_bounded_ctest_evidence.subsets | ForEach-Object { [string]$_ })
foreach ($expectedSubset in $pdfBoundedCtestSubsets) {
    if (-not ($manifestPdfBoundedCtestSubsets -contains $expectedSubset)) {
        throw "release_assets_manifest.json lost bounded PDF CTest subset '$expectedSubset'."
    }
}
$manifestPdfBoundedCtestSummaryJsonDisplay = @($manifest.pdf_bounded_ctest_evidence.summary_json_display | ForEach-Object { [string]$_ })
foreach ($expectedDisplay in $pdfBoundedCtestSummaryJsonDisplay) {
    if (-not ($manifestPdfBoundedCtestSummaryJsonDisplay -contains $expectedDisplay)) {
        throw "release_assets_manifest.json lost bounded PDF CTest summary display '$expectedDisplay'."
    }
}
$wordVisualStandardReviewMetadata = @($manifest.word_visual_standard_review_metadata)
if ([int]$manifest.word_visual_standard_review_metadata_count -ne 4) {
    throw "release_assets_manifest.json did not record four standard Word visual review metadata entries."
}
if ($wordVisualStandardReviewMetadata.Count -ne 4) {
    throw "release_assets_manifest.json lost standard Word visual review metadata entries."
}
$wordVisualStandardReviewMetadataByTask = @{}
foreach ($entry in $wordVisualStandardReviewMetadata) {
    $wordVisualStandardReviewMetadataByTask[[string]$entry.task_key] = $entry
    if ($entry.PSObject.Properties["review_note"]) {
        throw "release_assets_manifest.json must not expose standard Word visual review notes."
    }
    foreach ($pathField in @("review_result_path", "final_review_path")) {
        $pathValue = [string]$entry.$pathField
        if ([string]::IsNullOrWhiteSpace($pathValue)) {
            throw "release_assets_manifest.json lost standard Word visual review $pathField for '$($entry.task_key)'."
        }
        if ($pathValue.Contains($resolvedRepoRoot)) {
            throw "release_assets_manifest.json did not public-sanitize standard Word visual review $pathField for '$($entry.task_key)'."
        }
    }
}
foreach ($expectedTaskKey in @("smoke", "fixed_grid", "section_page_setup", "page_number_fields")) {
    if (-not $wordVisualStandardReviewMetadataByTask.ContainsKey($expectedTaskKey)) {
        throw "release_assets_manifest.json lost standard Word visual review metadata task '$expectedTaskKey'."
    }
}
$manifestSmokeReviewMetadata = $wordVisualStandardReviewMetadataByTask["smoke"]
if ([string]$manifestSmokeReviewMetadata.review_task_key -ne "document") {
    throw "release_assets_manifest.json did not preserve the smoke review task key alias."
}
if ((Convert-TestComparableValue -Value $manifestSmokeReviewMetadata.verdict) -ne "pass" -or
    (Convert-TestComparableValue -Value $manifestSmokeReviewMetadata.review_status) -ne "reviewed" -or
    (Convert-TestComparableValue -Value $manifestSmokeReviewMetadata.reviewed_at) -ne "2026-04-12T12:10:00" -or
    (Convert-TestComparableValue -Value $manifestSmokeReviewMetadata.review_method) -ne "operator_supplied") {
    throw "release_assets_manifest.json lost the smoke standard Word visual review status metadata."
}
if ([string]$manifestSmokeReviewMetadata.review_result_path -ne $expectedSmokeReviewResultPath) {
    throw "release_assets_manifest.json lost the smoke standard Word visual review result path."
}
if ([string]$manifestSmokeReviewMetadata.final_review_path -ne $expectedSmokeFinalReviewPath) {
    throw "release_assets_manifest.json lost the smoke standard Word visual final review path."
}
if ([string]$manifest.workspace -ne ".") {
    throw "release_assets_manifest.json did not rewrite workspace to a public relative path."
}
if ([string]$manifest.summary_json -ne ".\$relativeWorkingDir\output\release-candidate-checks\report\summary.json") {
    throw "release_assets_manifest.json did not rewrite summary_json to the expected relative path."
}
if ([string]$manifest.pdf_visual_gate_summary_json -ne $expectedRelativePdfGateSummary) {
    throw "release_assets_manifest.json did not rewrite pdf_visual_gate_summary_json to the expected relative path."
}
if ([string]$manifest.pdf_visual_gate_output_dir -ne $expectedRelativePdfGateRoot) {
    throw "release_assets_manifest.json did not rewrite pdf_visual_gate_output_dir to the expected relative path."
}
if ($manifest.governance_metric_count -ne 3) {
    throw "release_assets_manifest.json did not preserve governance_metric_count=3."
}

$manifestReleaseNoteBundle = $manifest.release_note_bundle
if ($null -eq $manifestReleaseNoteBundle) {
    throw "release_assets_manifest.json lost release_note_bundle."
}
if ([string]$manifestReleaseNoteBundle.status -ne "declared") {
    throw "release_assets_manifest.json lost release_note_bundle status."
}
if ([int]$manifestReleaseNoteBundle.entrypoint_count -ne 6) {
    throw "release_assets_manifest.json lost release_note_bundle entrypoint count."
}
if ([int]$manifestReleaseNoteBundle.required_entrypoint_count -ne 6) {
    throw "release_assets_manifest.json lost release_note_bundle required entrypoint count."
}
$manifestReleaseNoteBundleEntrypointsById = @{}
foreach ($entrypoint in @($manifestReleaseNoteBundle.entrypoints)) {
    $manifestReleaseNoteBundleEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks\START_HERE.md"; location = "summary_root" },
        [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"; location = "report" },
        [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"; location = "report" },
        [ordered]@{ id = "release_handoff"; path = $releaseHandoffPath; path_display = ".\output\release-candidate-checks\report\release_handoff.md"; location = "report" },
        [ordered]@{ id = "release_body_zh_cn"; path = $releaseBodyPath; path_display = ".\output\release-candidate-checks\report\release_body.zh-CN.md"; location = "report" },
        [ordered]@{ id = "release_summary_zh_cn"; path = $releaseSummaryPath; path_display = ".\output\release-candidate-checks\report\release_summary.zh-CN.md"; location = "report" }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not (@($manifestReleaseNoteBundle.entrypoint_ids | ForEach-Object { [string]$_ }) -contains $entrypointId)) {
        throw "release_assets_manifest.json lost release_note_bundle entrypoint id '$entrypointId'."
    }
    if (-not $manifestReleaseNoteBundleEntrypointsById.ContainsKey($entrypointId)) {
        throw "release_assets_manifest.json lost release_note_bundle entrypoint '$entrypointId'."
    }

    $entrypoint = $manifestReleaseNoteBundleEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "release_assets_manifest.json lost required=true for release_note_bundle entrypoint '$entrypointId'."
    }
    if ([string]$entrypoint.location -ne [string]$entrypointExpectation.location) {
        throw "release_assets_manifest.json lost release_note_bundle entrypoint '$entrypointId' location."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "release_assets_manifest.json did not public-sanitize release_note_bundle entrypoint '$entrypointId' path."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "release_assets_manifest.json lost release_note_bundle entrypoint '$entrypointId' path_display. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
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
if ([string]$manifestNumberingConfidence.source_report -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_report."
}
if ([string]$manifestNumberingConfidence.source_report_display -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_report_display."
}
if ([string]$manifestNumberingConfidence.source_json -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_json."
}
if ([string]$manifestNumberingConfidence.source_json_display -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_json_display."
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
if ([string]$manifestTableLayoutDeliveryQuality.source_report -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "release_assets_manifest.json lost table layout delivery quality source_report."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_report_display -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "release_assets_manifest.json lost table layout delivery quality source_report_display."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_json -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "release_assets_manifest.json lost table layout delivery quality source_json."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_json_display -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "release_assets_manifest.json lost table layout delivery quality source_json_display."
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
if ([int]$manifestTableLayoutDeliveryQuality.details.pdf_floating_table_supported_geometry_percent -ne 44) {
    throw "release_assets_manifest.json lost table layout delivery quality PDF floating table support percentage."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.pdf_floating_table_tracked_geometry_count -ne 9) {
    throw "release_assets_manifest.json lost table layout delivery quality PDF floating table tracked geometry count."
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
if ([string]$manifestContentControlContract.input_docx -ne "samples/invoice.docx") {
    throw "release_assets_manifest.json lost content-control input_docx provenance."
}
if ([string]$manifestContentControlContract.input_docx_display -ne ".\samples\invoice.docx") {
    throw "release_assets_manifest.json lost content-control input_docx_display provenance."
}
if ([string]$manifestContentControlContract.template_name -ne "invoice-template") {
    throw "release_assets_manifest.json lost content-control template_name provenance."
}
if ([string]$manifestContentControlContract.schema_target -ne "invoice") {
    throw "release_assets_manifest.json lost content-control schema_target provenance."
}
if ([string]$manifestContentControlContract.target_mode -ne "resolved-section-targets") {
    throw "release_assets_manifest.json lost content-control target_mode provenance."
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
if ([string]$manifestProjectTemplateReadiness.source_schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
    throw "release_assets_manifest.json lost project template delivery readiness source_schema."
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
if (@($manifestProjectTemplateReadiness.schema_approval_status_summary).Count -ne 1) {
    throw "release_assets_manifest.json lost project template delivery readiness schema_approval_status_summary."
}
if ([string]$manifestProjectTemplateReadiness.schema_approval_status_summary[0].status -ne "approved" -or
    [int]$manifestProjectTemplateReadiness.schema_approval_status_summary[0].count -ne 4) {
    throw "release_assets_manifest.json lost project template delivery readiness schema_approval_status_summary values."
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
if ([int]$manifestProjectTemplateReadiness.warning_count -ne 0) {
    throw "release_assets_manifest.json lost project template delivery readiness warning_count."
}
$expectedProjectTemplateDeliveryReadinessDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $projectTemplateDeliveryReadinessSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$manifestProjectTemplateReadiness.source_report_display -ne $expectedProjectTemplateDeliveryReadinessDisplay) {
    throw "release_assets_manifest.json lost project template delivery readiness source_report_display."
}
if ([string]$manifestProjectTemplateReadiness.source_json_display -ne $expectedProjectTemplateDeliveryReadinessDisplay) {
    throw "release_assets_manifest.json lost project template delivery readiness source_json_display."
}
$manifestProjectTemplateOnboarding = $manifest.project_template_onboarding_governance_contract
if ($null -eq $manifestProjectTemplateOnboarding) {
    throw "release_assets_manifest.json lost project_template_onboarding_governance_contract."
}
if ([string]$manifestProjectTemplateOnboarding.schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
    throw "release_assets_manifest.json lost project template onboarding governance schema."
}
if ([string]$manifestProjectTemplateOnboarding.source_schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
    throw "release_assets_manifest.json lost project template onboarding governance source_schema."
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
$expectedProjectTemplateOnboardingGovernanceDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $projectTemplateOnboardingGovernanceSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$manifestProjectTemplateOnboarding.source_report_display -ne $expectedProjectTemplateOnboardingGovernanceDisplay) {
    throw "release_assets_manifest.json lost project template onboarding governance source_report_display."
}
if ([string]$manifestProjectTemplateOnboarding.source_json_display -ne $expectedProjectTemplateOnboardingGovernanceDisplay) {
    throw "release_assets_manifest.json lost project template onboarding governance source_json_display."
}

$warningOnlyProjectTemplateDeliveryReadinessSummaryPath = Join-Path $reportDir "project_template_delivery_readiness_warning_only_summary.json"
$warningOnlyProjectTemplateDeliveryReadinessSummary = $projectTemplateDeliveryReadinessSummary | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$warningOnlyProjectTemplateDeliveryReadinessSummary.status = "needs_review"
$warningOnlyProjectTemplateDeliveryReadinessSummary.release_ready = $false
$warningOnlyProjectTemplateDeliveryReadinessSummary.template_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.ready_template_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.blocked_template_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.release_blocker_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.action_item_count = 0
$warningOnlyProjectTemplateDeliveryReadinessSummary.warning_count = 1
($warningOnlyProjectTemplateDeliveryReadinessSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $warningOnlyProjectTemplateDeliveryReadinessSummaryPath -Encoding UTF8

$warningOnlySummaryPath = Join-Path $reportDir "summary.warning-only-project-template-readiness.json"
$warningOnlySummary = $summary | ConvertTo-Json -Depth 20 | ConvertFrom-Json
$warningOnlySummary.project_template_delivery_readiness = $warningOnlyProjectTemplateDeliveryReadinessSummaryPath
($warningOnlySummary | ConvertTo-Json -Depth 20) | Set-Content -LiteralPath $warningOnlySummaryPath -Encoding UTF8

$warningOnlyOutputRoot = Join-Path $resolvedWorkingDir "output-release-warning-only"
& $packageScript `
    -SummaryJson $warningOnlySummaryPath `
    -OutputRoot $warningOnlyOutputRoot `
    -KeepStaging

$warningOnlyManifestPath = Join-Path $warningOnlyOutputRoot "v1.6.4\release_assets_manifest.json"
$warningOnlyManifest = Get-Content -Raw -Encoding UTF8 -LiteralPath $warningOnlyManifestPath | ConvertFrom-Json
$warningOnlyManifestReadiness = $warningOnlyManifest.project_template_delivery_readiness_contract
if ([string]$warningOnlyManifestReadiness.status -ne "needs_review") {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness status."
}
if ([bool]$warningOnlyManifestReadiness.release_ready) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness release_ready=false."
}
if ([int]$warningOnlyManifestReadiness.release_blocker_count -ne 0) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness release_blocker_count."
}
if ([int]$warningOnlyManifestReadiness.warning_count -ne 1) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness warning_count."
}
$expectedWarningOnlyProjectTemplateDeliveryReadinessDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $warningOnlyProjectTemplateDeliveryReadinessSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$warningOnlyManifestReadiness.source_report_display -ne $expectedWarningOnlyProjectTemplateDeliveryReadinessDisplay) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness source_report_display."
}
if ([string]$warningOnlyManifestReadiness.source_json_display -ne $expectedWarningOnlyProjectTemplateDeliveryReadinessDisplay) {
    throw "release_assets_manifest.json lost warning-only project template delivery readiness source_json_display."
}

$manifestSignoffEntrypoints = $manifest.manifest_signoff_entrypoints
if ($null -eq $manifestSignoffEntrypoints) {
    throw "release_assets_manifest.json lost manifest_signoff_entrypoints."
}
if ([string]$manifestSignoffEntrypoints.status -ne "declared") {
    throw "release_assets_manifest.json lost manifest signoff status."
}
$expectedManifestDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $manifestPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$manifestSignoffEntrypoints.release_assets_manifest -ne $expectedManifestDisplay) {
    throw "release_assets_manifest.json lost manifest signoff packaged manifest path."
}
if ([int]$manifestSignoffEntrypoints.required_entrypoint_count -ne 3) {
    throw "release_assets_manifest.json lost manifest signoff required entrypoint count."
}
foreach ($requiredContract in @(
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract"
    )) {
    if (-not (@($manifestSignoffEntrypoints.required_contracts | ForEach-Object { [string]$_ }) -contains $requiredContract)) {
        throw "release_assets_manifest.json lost manifest signoff required contract '$requiredContract'."
    }
}
foreach ($requiredField in @(
        "status",
        "release_ready",
        "schema_approval_status_summary",
        "source_report_display",
        "source_json_display"
    )) {
    if (-not (@($manifestSignoffEntrypoints.required_fields | ForEach-Object { [string]$_ }) -contains $requiredField)) {
        throw "release_assets_manifest.json lost manifest signoff required field '$requiredField'."
    }
}
if ([string]$manifestSignoffEntrypoints.checklist_marker -ne "reviewer_manifest_scoped_project_template_trace") {
    throw "release_assets_manifest.json lost manifest signoff checklist marker."
}

$manifestProjectTemplateChecklistEntrypoints = $manifest.project_template_readiness_checklist_entrypoints
if ($null -eq $manifestProjectTemplateChecklistEntrypoints) {
    throw "release_assets_manifest.json lost project_template_readiness_checklist_entrypoints."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.status -ne "declared") {
    throw "release_assets_manifest.json lost project-template readiness checklist entrypoints status."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_label -ne "Project template release readiness checklist") {
    throw "release_assets_manifest.json lost project-template readiness checklist label."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst") {
    throw "release_assets_manifest.json lost project-template readiness checklist path."
}
if ([int]$manifestProjectTemplateChecklistEntrypoints.required_entrypoint_count -ne 3) {
    throw "release_assets_manifest.json lost project-template readiness checklist required entrypoint count."
}
if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace") {
    throw "release_assets_manifest.json lost project-template readiness checklist marker."
}

$manifestProjectTemplateChecklistMaterialSafetyAudit = $manifest.release_entry_project_template_readiness_checklist_material_safety_audit
if ($null -eq $manifestProjectTemplateChecklistMaterialSafetyAudit) {
    throw "release_assets_manifest.json lost release_entry_project_template_readiness_checklist_material_safety_audit."
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.status -ne "passed") {
    throw "release_assets_manifest.json lost project-template checklist material-safety audit pass status."
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.audit_script -ne ".\scripts\assert_release_material_safety.ps1") {
    throw "release_assets_manifest.json lost project-template checklist material-safety audit script."
}
if ([int]$manifestProjectTemplateChecklistMaterialSafetyAudit.audited_entrypoint_count -ne 3) {
    throw "release_assets_manifest.json lost project-template checklist material-safety audited entrypoint count."
}
foreach ($entrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
    if (-not (@($manifestProjectTemplateChecklistMaterialSafetyAudit.audited_entrypoints | ForEach-Object { [string]$_ }) -contains $entrypointId)) {
        throw "release_assets_manifest.json lost project-template checklist material-safety audited entrypoint '$entrypointId'."
    }
}
if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_label -ne "Project-template readiness checklist handoff evidence" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_field -ne "project_template_readiness_checklist_entrypoints_source_reports" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_source_schema -ne "featherdoc.release_candidate_summary" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace" -or
    [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.material_safety_marker -ne "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace") {
    throw "release_assets_manifest.json lost project-template checklist material-safety audit identity."
}

$manifestSignoffEntrypointsById = @{}
foreach ($entrypoint in @($manifestSignoffEntrypoints.entrypoints)) {
    $manifestSignoffEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{
            id = "start_here"
            path = $startHerePath
            path_display = ".\output\release-candidate-checks\START_HERE.md"
        },
        [ordered]@{
            id = "artifact_guide"
            path = $artifactGuidePath
            path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
        },
        [ordered]@{
            id = "reviewer_checklist"
            path = $reviewerChecklistPath
            path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
        }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not $manifestSignoffEntrypointsById.ContainsKey($entrypointId)) {
        throw "release_assets_manifest.json lost manifest signoff entrypoint '$entrypointId'."
    }

    $entrypoint = $manifestSignoffEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "release_assets_manifest.json lost required=true for manifest signoff entrypoint '$entrypointId'."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "release_assets_manifest.json did not public-sanitize manifest signoff entrypoint '$entrypointId' path."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "release_assets_manifest.json lost manifest signoff entrypoint '$entrypointId' path_display. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
}

$manifestProjectTemplateChecklistEntrypointsById = @{}
foreach ($entrypoint in @($manifestProjectTemplateChecklistEntrypoints.entrypoints)) {
    $manifestProjectTemplateChecklistEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks\START_HERE.md" },
        [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md" },
        [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md" }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not $manifestProjectTemplateChecklistEntrypointsById.ContainsKey($entrypointId)) {
        throw "release_assets_manifest.json lost project-template readiness checklist entrypoint '$entrypointId'."
    }

    $entrypoint = $manifestProjectTemplateChecklistEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "release_assets_manifest.json lost required=true for project-template readiness checklist entrypoint '$entrypointId'."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "release_assets_manifest.json did not public-sanitize project-template readiness checklist entrypoint '$entrypointId' path."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "release_assets_manifest.json lost project-template readiness checklist entrypoint '$entrypointId' path_display. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
}

foreach ($zipPath in @($installZipPath, $galleryZipPath, $evidenceZipPath)) {
    if (-not (Test-Path -LiteralPath $zipPath)) {
        throw "Expected ZIP archive was not created: $zipPath"
    }
}

$evidenceZip = [System.IO.Compression.ZipFile]::OpenRead($evidenceZipPath)
try {
    $evidenceZipEntries = @($evidenceZip.Entries | ForEach-Object { $_.FullName -replace '\\', '/' })
    foreach ($expectedEntry in @(
            "release-candidate-checks/report/final_review.md",
            "pdf-visual-release-gate/report/summary.json",
            "pdf-visual-release-gate/report/aggregate-contact-sheet.png",
            "pdf-visual-release-gate/unicode-font/report/full-contact-sheet.png"
        )) {
        if (-not ($evidenceZipEntries -contains $expectedEntry)) {
            throw "Release evidence ZIP did not include expected PDF visual gate entry '$expectedEntry'."
        }
    }
} finally {
    $evidenceZip.Dispose()
}

Write-Host "Package release assets safety regression passed."

