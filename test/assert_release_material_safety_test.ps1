param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [string[]]$CasePattern = @(),
    [int]$ShardIndex = 0,
    [int]$ShardCount = 1
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\assert_release_material_safety_test"
}

if ($ShardCount -lt 1) {
    throw "ShardCount must be at least 1."
}
if ($ShardIndex -lt 0 -or $ShardIndex -ge $ShardCount) {
    throw "ShardIndex must be between 0 and ShardCount - 1."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$auditScriptPath = Join-Path $resolvedRepoRoot "scripts\assert_release_material_safety.ps1"

$contentControlInputDocx = "samples/invoice.docx"
$contentControlInputDocxDisplay = ".\samples\invoice.docx"
$contentControlTemplateName = "invoice-template"
$contentControlSchemaTarget = "invoice"
$contentControlTargetMode = "resolved-section-targets"

function New-TestContentControlRepairContract {
    return [ordered]@{
        id = "content_control_data_binding.bound_placeholder"
        action = "sync_or_fill_bound_content_control"
        source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
        source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
        input_docx = $script:contentControlInputDocx
        input_docx_display = $script:contentControlInputDocxDisplay
        template_name = $script:contentControlTemplateName
        schema_target = $script:contentControlSchemaTarget
        target_mode = $script:contentControlTargetMode
        repair_strategy = "sync_bound_content_control"
        repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
        command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        repair_action_classes = @("release_blocking", "auto_repair_candidate", "manual_confirmation_required")
    }
}

$passDir = Join-Path $resolvedWorkingDir "pass"
$failDir = Join-Path $resolvedWorkingDir "fail"
New-Item -ItemType Directory -Path $passDir -Force | Out-Null
New-Item -ItemType Directory -Path $failDir -Force | Out-Null

$materialSafetyCasePatterns = @($CasePattern | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
$materialSafetyCaseFilterEnabled = $materialSafetyCasePatterns.Count -gt 0
$materialSafetyPassDir = [System.IO.Path]::GetFullPath($passDir).TrimEnd('\', '/')
$materialSafetyFailDir = [System.IO.Path]::GetFullPath($failDir).TrimEnd('\', '/')
$materialSafetyAuditRunCount = 0
$materialSafetyAuditSkipCount = 0
$materialSafetySelectedAuditCaseCount = 0

# Optional filters keep targeted slices of this large regression runnable under
# bounded CI/test runners. The default path still executes every audit case.
function Get-MaterialSafetyAuditCaseLabels {
    param([string[]]$Path)

    $labels = [System.Collections.Generic.List[string]]::new()
    foreach ($item in @($Path)) {
        if ([string]::IsNullOrWhiteSpace($item)) {
            continue
        }

        $rawPath = [string]$item
        [void]$labels.Add($rawPath)

        try {
            $fullPath = [System.IO.Path]::GetFullPath($rawPath)
        } catch {
            continue
        }

        [void]$labels.Add($fullPath)
        foreach ($root in @($script:materialSafetyPassDir, $script:materialSafetyFailDir, $script:resolvedWorkingDir, $script:resolvedRepoRoot)) {
            if ([string]::IsNullOrWhiteSpace($root)) {
                continue
            }

            $normalizedRoot = [System.IO.Path]::GetFullPath($root).TrimEnd('\', '/')
            if ($fullPath.StartsWith($normalizedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
                $relativePath = $fullPath.Substring($normalizedRoot.Length).TrimStart('\', '/')
                if (-not [string]::IsNullOrWhiteSpace($relativePath)) {
                    [void]$labels.Add($relativePath)
                }
            }
        }
    }

    return @($labels.ToArray())
}

function Test-MaterialSafetyAuditCaseSelected {
    param([string[]]$Path)

    if (-not $script:materialSafetyCaseFilterEnabled) {
        return $true
    }

    if ($script:materialSafetyCasePatterns.Count -gt 0) {
        $labelText = (Get-MaterialSafetyAuditCaseLabels -Path $Path) -join [Environment]::NewLine
        $selectedByPattern = $false
        foreach ($pattern in $script:materialSafetyCasePatterns) {
            if ($labelText -match $pattern) {
                $selectedByPattern = $true
                break
            }
        }

        if (-not $selectedByPattern) {
            return $false
        }
    }

    return $true
}

function Test-MaterialSafetyAuditCaseExpectsFailure {
    param([string[]]$Path)

    foreach ($item in @($Path)) {
        if ([string]::IsNullOrWhiteSpace($item)) {
            continue
        }

        try {
            $fullPath = [System.IO.Path]::GetFullPath([string]$item)
        } catch {
            continue
        }

        if ($fullPath.StartsWith($script:materialSafetyFailDir, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Invoke-MaterialSafetyAuditCase {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Path
    )

    if (-not (Test-MaterialSafetyAuditCaseSelected -Path $Path)) {
        $script:materialSafetyAuditSkipCount++
        if (Test-MaterialSafetyAuditCaseExpectsFailure -Path $Path) {
            throw "Skipped filtered release material safety negative case."
        }

        return
    }

    $script:materialSafetySelectedAuditCaseCount++
    if ($script:ShardCount -gt 1 -and (($script:materialSafetySelectedAuditCaseCount - 1) % $script:ShardCount) -ne $script:ShardIndex) {
        $script:materialSafetyAuditSkipCount++
        if (Test-MaterialSafetyAuditCaseExpectsFailure -Path $Path) {
            throw "Skipped filtered release material safety negative case."
        }

        return
    }

    $script:materialSafetyAuditRunCount++
    & $script:auditScriptPath -Path $Path
}

$auditScript = "Invoke-MaterialSafetyAuditCase"

$passFile = Join-Path $passDir "release_body.zh-CN.md"
Set-Content -LiteralPath $passFile -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷璇存槑

- Release notes are ready for publishing.
- Evidence path: .\output\release-candidate-checks\report\release_handoff.md
"@

& $auditScript -Path $passFile

$passCliReferenceFile = Join-Path $passDir "cli_reference.md"
Set-Content -LiteralPath $passCliReferenceFile -Encoding UTF8 -Value @"
# CLI reference

- The template render wrappers expose ``-DraftPlanOutput``, ``-PatchedPlanOutput``, and ``-PatchPlanOutput`` options.
- These are stable option names for render-plan artifacts, not release-note draft markers.
"@

& $auditScript -Path $passCliReferenceFile

$passSummaryPath = Join-Path $passDir "summary.json"
$passManifestPath = Join-Path $passDir "release_assets_manifest.json"
$governanceMetrics = @(
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
            catalog_coverage_percent = 1
            baseline_coverage_percent = 100
            coverage_score = 1
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
            metadata_only_fields = @("leftFromText", "rightFromText", "topFromText outside paragraph anchoring", "tblOverlap")
            review_required_fields = @("full Word-compatible floating table text wrapping", "table overlap avoidance and collision resolution")
            penalty_summary = @(
                [ordered]@{ factor = "needs_review_documents"; count = 0; penalty = 0 },
                [ordered]@{ factor = "floating_table_plans_pending"; count = 0; penalty = 0 }
            )
        }
    }
)
$governanceMetricCount = $governanceMetrics.Count
$projectTemplateDeliveryReadinessContract = [ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    source_schema = "featherdoc.project_template_delivery_readiness_report.v1"
    status = "ready"
    release_ready = $true
    latest_schema_approval_gate_status = "passed"
    schema_approval_status_summary = @(
        [ordered]@{
            status = "approved"
            count = 4
        }
    )
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
    schema_history_blocked_run_count = 0
    schema_history_pending_run_count = 0
    schema_history_passed_run_count = 3
    template_count = 4
    ready_template_count = 4
    blocked_template_count = 0
    release_blocker_count = 0
    action_item_count = 0
    warning_count = 0
    source_report_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
    source_json_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
}
$projectTemplateOnboardingGovernanceContract = [ordered]@{
    schema = "featherdoc.project_template_onboarding_governance_report.v1"
    source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
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
    blocked_entry_count = 0
    pending_review_entry_count = 0
    not_evaluated_entry_count = 0
    approved_entry_count = 3
    not_required_entry_count = 1
    release_blocker_count = 0
    action_item_count = 0
    manual_review_recommendation_count = 1
    source_report_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
    source_json_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
}
$pdfVisualGateSummaryJson = ".\output\pdf-visual-release-gate-current\report\summary.json"
$pdfVisualGateEvidence = [ordered]@{
    summary_json = $pdfVisualGateSummaryJson
    status = "loaded"
    full_visual_gate_status = "pass"
    verdict = "pass"
    aggregate_contact_sheet = ".\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png"
    cjk_manifest_count = "43"
    cjk_copy_search_count = "43"
    cjk_missing_text_count = "0"
    visual_baseline_manifest_count = "42"
    visual_baseline_count = "44"
    pdf_cli_export_log = ".\output\pdf-visual-release-gate-current\report\pdf-cli-export-test.log"
    pdf_regression_log = ".\output\pdf-visual-release-gate-current\report\pdf-regression-test.log"
    cjk_copy_search_log_dir = ".\output\pdf-visual-release-gate-current\report\cjk-copy-search"
    unicode_font_log = ".\output\pdf-visual-release-gate-current\report\unicode-font.log"
    error = ""
}
$manifestSignoffEntrypoints = [ordered]@{
    status = "declared"
    release_assets_manifest = ".\output\release-assets\v1.6.4\release_assets_manifest.json"
    required_entrypoint_count = 3
    entrypoints = @(
        [ordered]@{
            id = "start_here"
            path_display = ".\output\release-candidate-checks\START_HERE.md"
            required = $true
        },
        [ordered]@{
            id = "artifact_guide"
            path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
            required = $true
        },
        [ordered]@{
            id = "reviewer_checklist"
            path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
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
        "source_report_display",
        "source_json_display"
    )
    checklist_marker = "reviewer_manifest_scoped_project_template_trace"
}
$projectTemplateReadinessChecklistEntrypoints = [ordered]@{
    status = "declared"
    checklist_label = "Project template release readiness checklist"
    checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
    required_entrypoint_count = 3
    entrypoints = @(
        [ordered]@{
            id = "start_here"
            path_display = ".\output\release-candidate-checks\START_HERE.md"
            required = $true
        },
        [ordered]@{
            id = "artifact_guide"
            path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
            required = $true
        },
        [ordered]@{
            id = "reviewer_checklist"
            path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
            required = $true
        }
    )
    checklist_marker = "release_entry_project_template_readiness_checklist_trace"
}
$releaseNoteBundle = [ordered]@{
    status = "declared"
    output_root = ".\output\release-candidate-checks"
    report_dir = ".\output\release-candidate-checks\report"
    entrypoint_count = 6
    required_entrypoint_count = 6
    entrypoint_ids = @(
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "release_handoff",
        "release_body_zh_cn",
        "release_summary_zh_cn"
    )
    entrypoints = @(
        [ordered]@{
            id = "start_here"
            path = ".\output\release-candidate-checks\START_HERE.md"
            path_display = ".\output\release-candidate-checks\START_HERE.md"
            location = "summary_root"
            required = $true
        },
        [ordered]@{
            id = "artifact_guide"
            path = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
            path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
            location = "report"
            required = $true
        },
        [ordered]@{
            id = "reviewer_checklist"
            path = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
            path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
            location = "report"
            required = $true
        },
        [ordered]@{
            id = "release_handoff"
            path = ".\output\release-candidate-checks\report\release_handoff.md"
            path_display = ".\output\release-candidate-checks\report\release_handoff.md"
            location = "report"
            required = $true
        },
        [ordered]@{
            id = "release_body_zh_cn"
            path = ".\output\release-candidate-checks\report\release_body.zh-CN.md"
            path_display = ".\output\release-candidate-checks\report\release_body.zh-CN.md"
            location = "report"
            required = $true
        },
        [ordered]@{
            id = "release_summary_zh_cn"
            path = ".\output\release-candidate-checks\report\release_summary.zh-CN.md"
            path_display = ".\output\release-candidate-checks\report\release_summary.zh-CN.md"
            location = "report"
            required = $true
        }
    )
}
$releaseEntryProjectTemplateChecklistMaterialSafetyAudit = [ordered]@{
    status = "passed"
    audit_script = ".\scripts\assert_release_material_safety.ps1"
    audited_entrypoint_count = 3
    audited_entrypoints = @("start_here", "artifact_guide", "reviewer_checklist")
    compact_evidence_label = "Project-template readiness checklist handoff evidence"
    compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
    compact_evidence_source_schema = "featherdoc.release_candidate_summary"
    checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
    checklist_marker = "release_entry_project_template_readiness_checklist_trace"
    material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
}
$wordVisualStandardReviewMetadata = @(
    [ordered]@{
        task_key = "smoke"
        review_task_key = "document"
        label = "Word visual smoke"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:10:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\document\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\document\report\final_review.md"
    },
    [ordered]@{
        task_key = "fixed_grid"
        review_task_key = "fixed_grid"
        label = "Fixed-grid merge/unmerge"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:20:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md"
    },
    [ordered]@{
        task_key = "section_page_setup"
        review_task_key = "section_page_setup"
        label = "Section page setup"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:30:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md"
    },
    [ordered]@{
        task_key = "page_number_fields"
        review_task_key = "page_number_fields"
        label = "Page number fields"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:40:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md"
    }
)

function New-NumberingCatalogRealCorpusConfidenceMirror {
    param($GovernanceMetrics)

    return [ordered]@{
        id = "numbering_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "numbering_catalog_governance"
        source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        source_report = $GovernanceMetrics[1].source_report
        source_report_display = $GovernanceMetrics[1].source_report_display
        source_json = $GovernanceMetrics[1].source_json
        source_json_display = $GovernanceMetrics[1].source_json_display
        score = 56
        level = "low"
        details = $GovernanceMetrics[1].details
    }
}

function New-TableLayoutDeliveryQualityMirror {
    param($GovernanceMetrics)

    return [ordered]@{
        id = "table_layout_delivery_governance.delivery_quality"
        metric = "delivery_quality"
        report_id = "table_layout_delivery_governance"
        source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        source_report = $GovernanceMetrics[2].source_report
        source_report_display = $GovernanceMetrics[2].source_report_display
        source_json = $GovernanceMetrics[2].source_json
        source_json_display = $GovernanceMetrics[2].source_json_display
        score = 100
        level = "release_ready"
        details = $GovernanceMetrics[2].details
    }
}

. (Join-Path $PSScriptRoot "assert_release_material_safety_manifest_cases.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_entry_cases.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_release_blocker_cases.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_release_governance_cases.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_manifest_contract_cases.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_release_summary_body_cases.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_handoff_cases.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_final_review_cases.ps1")
