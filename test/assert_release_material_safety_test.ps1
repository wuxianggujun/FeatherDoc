param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\assert_release_material_safety_test"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$auditScript = Join-Path $resolvedRepoRoot "scripts\assert_release_material_safety.ps1"

$contentControlInputDocx = "samples/invoice.docx"
$contentControlInputDocxDisplay = ".\samples\invoice.docx"
$contentControlTemplateName = "invoice-template"
$contentControlSchemaTarget = "invoice"
$contentControlTargetMode = "resolved-section-targets"

function New-TestContentControlRepairContract {
    return [ordered]@{
        id = "content_control_data_binding.bound_placeholder"
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
    }
}

$passDir = Join-Path $resolvedWorkingDir "pass"
$failDir = Join-Path $resolvedWorkingDir "fail"
New-Item -ItemType Directory -Path $passDir -Force | Out-Null
New-Item -ItemType Directory -Path $failDir -Force | Out-Null

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
        "schema_approval_status_summary",
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

function New-NumberingCatalogRealCorpusConfidenceMirror {
    param($GovernanceMetrics)

    return [ordered]@{
        id = "numbering_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "numbering_catalog_governance"
        source_schema = "featherdoc.numbering_catalog_governance_report.v1"
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
        score = 100
        level = "release_ready"
        details = $GovernanceMetrics[2].details
    }
}

$passSummary = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    release_handoff = ".\output\release-candidate-checks\report\release_handoff.md"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
}
($passSummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $passSummaryPath -Encoding UTF8

$passManifest = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    visual_gate_status = "completed"
    pdf_visual_gate_status = "loaded"
    pdf_visual_gate_summary_json = $pdfVisualGateSummaryJson
    pdf_visual_gate_output_dir = ".\output\pdf-visual-release-gate-current"
    pdf_visual_gate_evidence_included = $true
    pdf_visual_gate_evidence = $pdfVisualGateEvidence
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        (New-TestContentControlRepairContract)
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
    manifest_signoff_entrypoints = $manifestSignoffEntrypoints
    project_template_readiness_checklist_entrypoints = $projectTemplateReadinessChecklistEntrypoints
    release_entry_project_template_readiness_checklist_material_safety_audit = $releaseEntryProjectTemplateChecklistMaterialSafetyAudit
}
($passManifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $passManifestPath -Encoding UTF8

& $auditScript -Path @($passSummaryPath, $passManifestPath)

$badManifestMissingProjectTemplateChecklistEntrypointsDir = Join-Path $failDir "manifest-missing-project-template-readiness-checklist-entrypoints"
$badManifestMissingProjectTemplateChecklistEntrypointsPath = Join-Path $badManifestMissingProjectTemplateChecklistEntrypointsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateChecklistEntrypointsDir -Force | Out-Null
$badManifestMissingProjectTemplateChecklistEntrypoints = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingProjectTemplateChecklistEntrypoints.PSObject.Properties.Remove("project_template_readiness_checklist_entrypoints")
($badManifestMissingProjectTemplateChecklistEntrypoints | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingProjectTemplateChecklistEntrypointsPath -Encoding UTF8

$missingProjectTemplateChecklistEntrypointsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateChecklistEntrypointsPath
} catch {
    $missingProjectTemplateChecklistEntrypointsFailedAsExpected = $true
}

if (-not $missingProjectTemplateChecklistEntrypointsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing project_template_readiness_checklist_entrypoints."
}

$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditDir = Join-Path $failDir "manifest-missing-project-template-checklist-material-safety-audit"
$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditPath = Join-Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditDir -Force | Out-Null
$badManifestMissingProjectTemplateChecklistMaterialSafetyAudit = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingProjectTemplateChecklistMaterialSafetyAudit.PSObject.Properties.Remove("release_entry_project_template_readiness_checklist_material_safety_audit")
($badManifestMissingProjectTemplateChecklistMaterialSafetyAudit | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditPath -Encoding UTF8

$missingProjectTemplateChecklistMaterialSafetyAuditFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditPath
} catch {
    $missingProjectTemplateChecklistMaterialSafetyAuditFailedAsExpected = $true
}

if (-not $missingProjectTemplateChecklistMaterialSafetyAuditFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing release_entry_project_template_readiness_checklist_material_safety_audit."
}

$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir = Join-Path $failDir "manifest-missing-project-template-checklist-material-safety-audit-source-schema"
$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath = Join-Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir -Force | Out-Null
$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchema.release_entry_project_template_readiness_checklist_material_safety_audit.PSObject.Properties.Remove("compact_evidence_source_schema")
($badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath -Encoding UTF8

$missingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath
} catch {
    $missingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected = $true
}

if (-not $missingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing release_entry_project_template_readiness_checklist_material_safety_audit.compact_evidence_source_schema."
}

$badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir = Join-Path $failDir "manifest-wrong-project-template-checklist-material-safety-audit-source-schema"
$badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath = Join-Path $badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir -Force | Out-Null
$badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchema.release_entry_project_template_readiness_checklist_material_safety_audit.compact_evidence_source_schema = "featherdoc.release_blocker_rollup_report.v1"
($badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath -Encoding UTF8

$wrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath
} catch {
    $wrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected = $true
}

if (-not $wrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with release_entry_project_template_readiness_checklist_material_safety_audit.compact_evidence_source_schema pointing at the wrong schema."
}

$badManifestMissingSignoffDir = Join-Path $failDir "manifest-missing-signoff-entrypoints"
$badManifestMissingSignoffPath = Join-Path $badManifestMissingSignoffDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingSignoffDir -Force | Out-Null
$badManifestMissingSignoff = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingSignoff.PSObject.Properties.Remove("manifest_signoff_entrypoints")
($badManifestMissingSignoff | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingSignoffPath -Encoding UTF8

$missingSignoffFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingSignoffPath
} catch {
    $missingSignoffFailedAsExpected = $true
}

if (-not $missingSignoffFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing manifest_signoff_entrypoints."
}

$badManifestMissingSignoffReviewerDir = Join-Path $failDir "manifest-missing-signoff-reviewer-checklist"
$badManifestMissingSignoffReviewerPath = Join-Path $badManifestMissingSignoffReviewerDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingSignoffReviewerDir -Force | Out-Null
$badManifestMissingSignoffReviewer = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingSignoffReviewer.manifest_signoff_entrypoints.entrypoints = @(
    $badManifestMissingSignoffReviewer.manifest_signoff_entrypoints.entrypoints |
        Where-Object { [string]$_.id -ne "reviewer_checklist" }
)
($badManifestMissingSignoffReviewer | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingSignoffReviewerPath -Encoding UTF8

$missingSignoffReviewerFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingSignoffReviewerPath
} catch {
    $missingSignoffReviewerFailedAsExpected = $true
}

if (-not $missingSignoffReviewerFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing reviewer_checklist signoff entrypoint."
}

$badManifestMissingSignoffFieldDir = Join-Path $failDir "manifest-missing-signoff-source-json-field"
$badManifestMissingSignoffFieldPath = Join-Path $badManifestMissingSignoffFieldDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingSignoffFieldDir -Force | Out-Null
$badManifestMissingSignoffField = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingSignoffField.manifest_signoff_entrypoints.required_fields = @(
    $badManifestMissingSignoffField.manifest_signoff_entrypoints.required_fields |
        Where-Object { [string]$_ -ne "source_json_display" }
)
($badManifestMissingSignoffField | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingSignoffFieldPath -Encoding UTF8

$missingSignoffFieldFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingSignoffFieldPath
} catch {
    $missingSignoffFieldFailedAsExpected = $true
}

if (-not $missingSignoffFieldFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing source_json_display signoff field."
}

$badManifestMissingPdfEvidenceDir = Join-Path $failDir "manifest-missing-pdf-visual-evidence"
$badManifestMissingPdfEvidencePath = Join-Path $badManifestMissingPdfEvidenceDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingPdfEvidenceDir -Force | Out-Null
$badManifestMissingPdfEvidence = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingPdfEvidence.PSObject.Properties.Remove("pdf_visual_gate_evidence")
($badManifestMissingPdfEvidence | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingPdfEvidencePath -Encoding UTF8

$missingPdfEvidenceFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingPdfEvidencePath
} catch {
    $missingPdfEvidenceFailedAsExpected = $true
}

if (-not $missingPdfEvidenceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without PDF visual gate evidence."
}

$badManifestMissingPdfVerdictDir = Join-Path $failDir "manifest-missing-pdf-visual-verdict"
$badManifestMissingPdfVerdictPath = Join-Path $badManifestMissingPdfVerdictDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingPdfVerdictDir -Force | Out-Null
$badManifestMissingPdfVerdict = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingPdfVerdict.pdf_visual_gate_evidence.PSObject.Properties.Remove("verdict")
($badManifestMissingPdfVerdict | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingPdfVerdictPath -Encoding UTF8

$missingPdfVerdictFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingPdfVerdictPath
} catch {
    $missingPdfVerdictFailedAsExpected = $true
}

if (-not $missingPdfVerdictFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without PDF visual gate verdict."
}

$badManifestInvalidPdfVerdictDir = Join-Path $failDir "manifest-invalid-pdf-visual-verdict"
$badManifestInvalidPdfVerdictPath = Join-Path $badManifestInvalidPdfVerdictDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestInvalidPdfVerdictDir -Force | Out-Null
$badManifestInvalidPdfVerdict = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestInvalidPdfVerdict.pdf_visual_gate_evidence.verdict = "blocked"
($badManifestInvalidPdfVerdict | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestInvalidPdfVerdictPath -Encoding UTF8

$invalidPdfVerdictFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestInvalidPdfVerdictPath
} catch {
    $invalidPdfVerdictFailedAsExpected = $true
}

if (-not $invalidPdfVerdictFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid PDF visual gate verdict."
}

$badManifestMismatchedFullPdfStatusDir = Join-Path $failDir "manifest-mismatched-full-pdf-visual-status"
$badManifestMismatchedFullPdfStatusPath = Join-Path $badManifestMismatchedFullPdfStatusDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMismatchedFullPdfStatusDir -Force | Out-Null
$badManifestMismatchedFullPdfStatus = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMismatchedFullPdfStatus.pdf_visual_gate_evidence.full_visual_gate_status = "fail"
($badManifestMismatchedFullPdfStatus | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMismatchedFullPdfStatusPath -Encoding UTF8

$mismatchedFullPdfStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMismatchedFullPdfStatusPath
} catch {
    $mismatchedFullPdfStatusFailedAsExpected = $true
}

if (-not $mismatchedFullPdfStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with PDF full visual status diverging from verdict."
}

$badManifestMismatchedPdfSummaryDir = Join-Path $failDir "manifest-mismatched-pdf-visual-summary"
$badManifestMismatchedPdfSummaryPath = Join-Path $badManifestMismatchedPdfSummaryDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMismatchedPdfSummaryDir -Force | Out-Null
$badManifestMismatchedPdfSummary = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMismatchedPdfSummary.pdf_visual_gate_evidence.summary_json = ".\output\pdf-visual-release-gate-current\report\other-summary.json"
($badManifestMismatchedPdfSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMismatchedPdfSummaryPath -Encoding UTF8

$mismatchedPdfSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMismatchedPdfSummaryPath
} catch {
    $mismatchedPdfSummaryFailedAsExpected = $true
}

if (-not $mismatchedPdfSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched PDF visual gate summary path."
}

$badManifestZeroPdfCountsDir = Join-Path $failDir "manifest-zero-pdf-visual-counts"
$badManifestZeroPdfCountsPath = Join-Path $badManifestZeroPdfCountsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestZeroPdfCountsDir -Force | Out-Null
$badManifestZeroPdfCounts = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestZeroPdfCounts.pdf_visual_gate_evidence.cjk_copy_search_count = "0"
$badManifestZeroPdfCounts.pdf_visual_gate_evidence.visual_baseline_count = "0"
($badManifestZeroPdfCounts | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestZeroPdfCountsPath -Encoding UTF8

$zeroPdfCountsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestZeroPdfCountsPath
} catch {
    $zeroPdfCountsFailedAsExpected = $true
}

if (-not $zeroPdfCountsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with zero PDF visual gate evidence counts."
}

$badManifestLowPdfManifestCountsDir = Join-Path $failDir "manifest-low-pdf-visual-manifest-counts"
$badManifestLowPdfManifestCountsPath = Join-Path $badManifestLowPdfManifestCountsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestLowPdfManifestCountsDir -Force | Out-Null
$badManifestLowPdfManifestCounts = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestLowPdfManifestCounts.pdf_visual_gate_evidence.cjk_manifest_count = "42"
$badManifestLowPdfManifestCounts.pdf_visual_gate_evidence.visual_baseline_manifest_count = "41"
($badManifestLowPdfManifestCounts | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestLowPdfManifestCountsPath -Encoding UTF8

$lowPdfManifestCountsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestLowPdfManifestCountsPath
} catch {
    $lowPdfManifestCountsFailedAsExpected = $true
}

if (-not $lowPdfManifestCountsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with low PDF visual manifest sample counts."
}

$badManifestPassingPdfMissingTextDir = Join-Path $failDir "manifest-passing-pdf-visual-missing-cjk-text"
$badManifestPassingPdfMissingTextPath = Join-Path $badManifestPassingPdfMissingTextDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestPassingPdfMissingTextDir -Force | Out-Null
$badManifestPassingPdfMissingText = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestPassingPdfMissingText.pdf_visual_gate_evidence.cjk_missing_text_count = "1"
($badManifestPassingPdfMissingText | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestPassingPdfMissingTextPath -Encoding UTF8

$passingPdfMissingTextFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestPassingPdfMissingTextPath
} catch {
    $passingPdfMissingTextFailedAsExpected = $true
}

if (-not $passingPdfMissingTextFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with verdict=pass and missing CJK text."
}

$badManifestMissingMetricDetailsDir = Join-Path $failDir "manifest-missing-governance-metric-details"
$badManifestMissingMetricDetailsPath = Join-Path $badManifestMissingMetricDetailsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingMetricDetailsDir -Force | Out-Null
$badManifestMissingMetricDetails = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingMetricDetails.numbering_catalog_real_corpus_confidence = [ordered]@{
    id = "numbering_catalog_governance.real_corpus_confidence"
    metric = "real_corpus_confidence"
    report_id = "numbering_catalog_governance"
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
    score = 56
    level = "low"
}
($badManifestMissingMetricDetails | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingMetricDetailsPath -Encoding UTF8

$missingManifestMetricDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingMetricDetailsPath
} catch {
    $missingManifestMetricDetailsFailedAsExpected = $true
}

if (-not $missingManifestMetricDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without numbering governance metric details."
}

$badManifestMismatchedMetricDetailsDir = Join-Path $failDir "manifest-mismatched-governance-metric-details"
$badManifestMismatchedMetricDetailsPath = Join-Path $badManifestMismatchedMetricDetailsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMismatchedMetricDetailsDir -Force | Out-Null
$badManifestMismatchedMetricDetails = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMismatchedMetricDetails.table_layout_delivery_quality = [ordered]@{
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
        unresolved_item_count = 1
        penalty_summary = @(
            [ordered]@{ factor = "needs_review_documents"; count = 0; penalty = 0 },
            [ordered]@{ factor = "floating_table_plans_pending"; count = 0; penalty = 0 }
        )
    }
}
($badManifestMismatchedMetricDetails | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMismatchedMetricDetailsPath -Encoding UTF8

$mismatchedManifestMetricDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMismatchedMetricDetailsPath
} catch {
    $mismatchedManifestMetricDetailsFailedAsExpected = $true
}

if (-not $mismatchedManifestMetricDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched table layout delivery details."
}

$passContentControlSummaryPath = Join-Path $passDir "content_control_data_binding_governance_summary.json"
$passContentControlSummary = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "blocker"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            input_docx = $contentControlInputDocx
            input_docx_display = $contentControlInputDocxDisplay
            template_name = $contentControlTemplateName
            schema_target = $contentControlSchemaTarget
            target_mode = $contentControlTargetMode
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
}
($passContentControlSummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $passContentControlSummaryPath -Encoding UTF8

& $auditScript -Path $passContentControlSummaryPath

$passEntryGovernanceTracePath = Join-Path $passDir "START_HERE.md"
Set-Content -LiteralPath $passEntryGovernanceTracePath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Content-control provenance: input_docx=samples/invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Content-control contract: source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Project template release readiness checklist: docs/project_template_release_readiness_checklist_zh.rst
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence level=low score=56 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

& $auditScript -Path $passEntryGovernanceTracePath

$passEntryProjectTemplateChecklistHandoffEvidenceDir = Join-Path $passDir "entry-project-template-checklist-handoff-evidence"
$passEntryProjectTemplateChecklistHandoffEvidencePath = Join-Path $passEntryProjectTemplateChecklistHandoffEvidenceDir "START_HERE.md"
New-Item -ItemType Directory -Path $passEntryProjectTemplateChecklistHandoffEvidenceDir -Force | Out-Null
Set-Content -LiteralPath $passEntryProjectTemplateChecklistHandoffEvidencePath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

& $auditScript -Path $passEntryProjectTemplateChecklistHandoffEvidencePath

$badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaDir = Join-Path $failDir "entry-project-template-checklist-handoff-evidence-missing-source-schema"
$badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaPath = Join-Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_report=.\output\release-candidate-checks\summary.json
"@

$badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaPath
} catch {
    $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistHandoffEvidenceMissingSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with project-template checklist handoff evidence missing source_schema."
}

$badEntryProjectTemplateChecklistHandoffEvidenceSplitDir = Join-Path $failDir "entry-project-template-checklist-handoff-evidence-split"
$badEntryProjectTemplateChecklistHandoffEvidenceSplitPath = Join-Path $badEntryProjectTemplateChecklistHandoffEvidenceSplitDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistHandoffEvidenceSplitDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistHandoffEvidenceSplitPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json

## Detached notes

- marker=release_entry_project_template_readiness_checklist_trace
"@

$badEntryProjectTemplateChecklistHandoffEvidenceSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistHandoffEvidenceSplitPath
} catch {
    $badEntryProjectTemplateChecklistHandoffEvidenceSplitFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistHandoffEvidenceSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with project-template checklist handoff marker supplied only by detached notes."
}

$badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsDir = Join-Path $failDir "entry-project-template-checklist-handoff-evidence-missing-paths"
$badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsPath = Join-Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

$badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsPath
} catch {
    $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistHandoffEvidenceMissingPathsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with project-template checklist handoff evidence missing entrypoint path_display values."
}

$badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceDir = Join-Path $failDir "entry-project-template-checklist-handoff-evidence-wrong-source"
$badEntryProjectTemplateChecklistHandoffEvidenceWrongSourcePath = Join-Path $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourcePath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist handoff evidence: project_template_readiness_checklist_entrypoints_source_reports=1, status=declared, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, required_entrypoint_count=3, entrypoints=start_here, artifact_guide, reviewer_checklist, entrypoint_paths=start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md; artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md; reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md, marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json
"@

$badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourcePath
} catch {
    $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistHandoffEvidenceWrongSourceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with project-template checklist handoff source_report pointing at the wrong evidence source."
}

$passEntryProjectTemplateChecklistPackagedAuditEvidenceDir = Join-Path $passDir "entry-project-template-checklist-packaged-audit-evidence"
$passEntryProjectTemplateChecklistPackagedAuditEvidencePath = Join-Path $passEntryProjectTemplateChecklistPackagedAuditEvidenceDir "START_HERE.md"
New-Item -ItemType Directory -Path $passEntryProjectTemplateChecklistPackagedAuditEvidenceDir -Force | Out-Null
Set-Content -LiteralPath $passEntryProjectTemplateChecklistPackagedAuditEvidencePath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, audited_entrypoints=start_here, artifact_guide, reviewer_checklist, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json
"@

& $auditScript -Path $passEntryProjectTemplateChecklistPackagedAuditEvidencePath

$badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaDir = Join-Path $failDir "entry-project-template-checklist-packaged-audit-evidence-missing-source-schema"
$badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaPath = Join-Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, audited_entrypoints=start_here, artifact_guide, reviewer_checklist, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace, source_report=.\output\release-blocker-rollup\summary.json
"@

$badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaPath
} catch {
    $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistPackagedAuditEvidenceMissingSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with packaged checklist audit evidence missing source_schema."
}

$badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitDir = Join-Path $failDir "entry-project-template-checklist-packaged-audit-evidence-split"
$badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitPath = Join-Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-blocker-rollup\summary.json

## Detached packaged audit notes

- audited_entrypoints=start_here, artifact_guide, reviewer_checklist
- material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace
"@

$badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitPath
} catch {
    $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistPackagedAuditEvidenceSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with packaged checklist audit entrypoints and material-safety marker supplied only by detached notes."
}

$badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceDir = Join-Path $failDir "entry-project-template-checklist-packaged-audit-evidence-wrong-source"
$badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourcePath = Join-Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourcePath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist packaged audit evidence: release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1, status=passed, audit_script=.\scripts\assert_release_material_safety.ps1, audited_entrypoint_count=3, audited_entrypoints=start_here, artifact_guide, reviewer_checklist, compact_evidence_label=Project-template readiness checklist handoff evidence, compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports, compact_evidence_source_schema=featherdoc.release_candidate_summary, checklist_path=docs/project_template_release_readiness_checklist_zh.rst, checklist_marker=release_entry_project_template_readiness_checklist_trace, material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace, source_schema=featherdoc.release_candidate_summary, source_report=.\output\release-candidate-checks\summary.json
"@

$badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourcePath
} catch {
    $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateChecklistPackagedAuditEvidenceWrongSourceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with packaged checklist audit source_report pointing at the wrong evidence source."
}

$badStartHereProjectTemplateMissingChecklistDir = Join-Path $failDir "start-here-project-template-readiness-checklist-missing"
$badStartHereProjectTemplateMissingChecklistPath = Join-Path $badStartHereProjectTemplateMissingChecklistDir "START_HERE.md"
New-Item -ItemType Directory -Path $badStartHereProjectTemplateMissingChecklistDir -Force | Out-Null
Set-Content -LiteralPath $badStartHereProjectTemplateMissingChecklistPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badStartHereProjectTemplateMissingChecklistFailedAsExpected = $false
try {
    & $auditScript -Path $badStartHereProjectTemplateMissingChecklistPath
} catch {
    $badStartHereProjectTemplateMissingChecklistFailedAsExpected = $true
}

if (-not $badStartHereProjectTemplateMissingChecklistFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md without the fixed project-template release readiness checklist."
}

$passReleaseSummaryTracePath = Join-Path $passDir "release_summary.zh-CN.md"
Set-Content -LiteralPath $passReleaseSummaryTracePath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

& $auditScript -Path $passReleaseSummaryTracePath

$passReleaseSummaryPdfTraceDir = Join-Path $passDir "release-summary-pdf-visual-gate-trace"
$passReleaseSummaryPdfTracePath = Join-Path $passReleaseSummaryPdfTraceDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $passReleaseSummaryPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseSummaryPdfTracePath -Encoding UTF8 -Value @"
# Release summary

- PDF visual gate：verdict=pass summary=.\output\pdf-visual-release-gate-current\report\summary.json aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png cjk_manifest_count=43 cjk_copy_search_count=43 visual_baseline_manifest_count=42 visual_baseline_count=44
"@

& $auditScript -Path $passReleaseSummaryPdfTracePath

$passStartHerePdfTraceDir = Join-Path $passDir "start-here-pdf-visual-gate-trace"
$passStartHerePdfTracePath = Join-Path $passStartHerePdfTraceDir "START_HERE.md"
New-Item -ItemType Directory -Path $passStartHerePdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passStartHerePdfTracePath -Encoding UTF8 -Value @"
# START_HERE

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF release readiness checklist: docs/pdf_release_readiness_checklist_zh.rst
"@

& $auditScript -Path $passStartHerePdfTracePath

$passReviewerChecklistPdfTraceDir = Join-Path $passDir "reviewer-checklist-pdf-visual-gate-trace"
$passReviewerChecklistPdfTracePath = Join-Path $passReviewerChecklistPdfTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $passReviewerChecklistPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReviewerChecklistPdfTracePath -Encoding UTF8 -Value @"
# Reviewer Checklist

- Confirm PDF visual gate summary .\output\pdf-visual-release-gate-current\report\summary.json with 43 CJK copy/search samples and 44 visual baselines before release.
- Confirm PDF visual gate aggregate contact sheet .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png before release.
- Confirm the fixed PDF release readiness checklist has been reviewed before publishing: docs/pdf_release_readiness_checklist_zh.rst.
"@

& $auditScript -Path $passReviewerChecklistPdfTracePath

$passReviewerChecklistFinalizePdfTraceDir = Join-Path $passDir "reviewer-checklist-pdf-visual-finalize-trace"
$passReviewerChecklistFinalizePdfTracePath = Join-Path $passReviewerChecklistFinalizePdfTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $passReviewerChecklistFinalizePdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReviewerChecklistFinalizePdfTracePath -Encoding UTF8 -Value @"
# Reviewer Checklist

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- [ ] Confirm the PDF visual gate finalize evidence is signed off: verdict ``pass``, summary .\output\pdf-visual-release-gate-current\report\summary.json, aggregate contact sheet .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png, CJK manifest samples ``43``, CJK copy/search samples ``43``, missing text ``0``, visual baseline manifest samples ``42``, visual baselines ``44``.
- [ ] Confirm the fixed PDF release readiness checklist has been reviewed before publishing: docs/pdf_release_readiness_checklist_zh.rst.
"@

& $auditScript -Path $passReviewerChecklistFinalizePdfTracePath

$badStartHerePdfMissingChecklistDir = Join-Path $failDir "start-here-pdf-readiness-checklist-missing"
$badStartHerePdfMissingChecklistPath = Join-Path $badStartHerePdfMissingChecklistDir "START_HERE.md"
New-Item -ItemType Directory -Path $badStartHerePdfMissingChecklistDir -Force | Out-Null
Set-Content -LiteralPath $badStartHerePdfMissingChecklistPath -Encoding UTF8 -Value @"
# START_HERE

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badStartHerePdfMissingChecklistFailedAsExpected = $false
try {
    & $auditScript -Path $badStartHerePdfMissingChecklistPath
} catch {
    $badStartHerePdfMissingChecklistFailedAsExpected = $true
}

if (-not $badStartHerePdfMissingChecklistFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md without the fixed PDF release readiness checklist."
}

$passReleaseBodyTraceDir = Join-Path $passDir "release-body-project-template-trace"
$passReleaseBodyTracePath = Join-Path $passReleaseBodyTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $passReleaseBodyTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseBodyTracePath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

& $auditScript -Path $passReleaseBodyTracePath

$passReleaseBodyPdfTraceDir = Join-Path $passDir "release-body-pdf-visual-gate-trace"
$passReleaseBodyPdfTracePath = Join-Path $passReleaseBodyPdfTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $passReleaseBodyPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseBodyPdfTracePath -Encoding UTF8 -Value @"
# Release body

## Validation

- PDF visual gate summary：.\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status：loaded
- PDF visual gate verdict：pass
- PDF visual gate aggregate contact sheet：.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF CJK manifest samples：43
- PDF CJK copy/search samples：43
- PDF visual baseline manifest samples：42
- PDF visual baselines：44
"@

& $auditScript -Path $passReleaseBodyPdfTracePath

$passReleaseHandoffTraceDir = Join-Path $passDir "release-handoff-project-template-trace"
$passReleaseHandoffTracePath = Join-Path $passReleaseHandoffTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $passReleaseHandoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseHandoffTracePath -Encoding UTF8 -Value @"
# Release handoff

- Project-template readiness checklist entrypoints evidence source reports: 1
  - source_report: .\output\release-candidate-checks\summary.json schema=featherdoc.release_candidate_summary
    - project_template_readiness_checklist_entrypoints_status: declared
    - project_template_readiness_checklist_entrypoints_checklist_label: Project template release readiness checklist
    - project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist
    - project_template_readiness_checklist_entrypoints_checklist_marker: release_entry_project_template_readiness_checklist_trace
- Release-entry project-template readiness checklist material-safety audit source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: passed
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: .\scripts\assert_release_material_safety.ps1
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: 3
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: Project-template readiness checklist handoff evidence
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: project_template_readiness_checklist_entrypoints_source_reports
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: release_entry_project_template_readiness_checklist_trace
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace
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

& $auditScript -Path $passReleaseHandoffTracePath

$badReleaseMetadataProjectTemplateChecklistWrongSchemaDir = Join-Path $failDir "release-metadata-project-template-checklist-wrong-schema"
$badReleaseMetadataProjectTemplateChecklistWrongSchemaPath = Join-Path $badReleaseMetadataProjectTemplateChecklistWrongSchemaDir "START_HERE.md"
New-Item -ItemType Directory -Path $badReleaseMetadataProjectTemplateChecklistWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseMetadataProjectTemplateChecklistWrongSchemaPath -Encoding UTF8 -Value @"
# START_HERE

- Project-template readiness checklist entrypoints evidence source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_blocker_rollup_report.v1
    - project_template_readiness_checklist_entrypoints_status: declared
    - project_template_readiness_checklist_entrypoints_checklist_label: Project template release readiness checklist
    - project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist
    - project_template_readiness_checklist_entrypoints_checklist_marker: release_entry_project_template_readiness_checklist_trace
"@

$badReleaseMetadataProjectTemplateChecklistWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseMetadataProjectTemplateChecklistWrongSchemaPath
} catch {
    $badReleaseMetadataProjectTemplateChecklistWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseMetadataProjectTemplateChecklistWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release metadata with project-template checklist entrypoints source_report using a non-release-candidate schema."
}

$badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir = Join-Path $failDir "release-metadata-project-template-checklist-material-safety-audit-wrong-schema"
$badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath = Join-Path $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath -Encoding UTF8 -Value @"
# Release handoff

- Release-entry project-template readiness checklist material-safety audit source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_blocker_rollup_report.v1
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: passed
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: .\scripts\assert_release_material_safety.ps1
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: 3
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: Project-template readiness checklist handoff evidence
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: project_template_readiness_checklist_entrypoints_source_reports
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: release_entry_project_template_readiness_checklist_trace
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace
"@

$badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath
} catch {
    $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseMetadataProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release metadata with release-entry project-template checklist material-safety audit source_report using a non-release-candidate schema."
}

$passReleaseHandoffPdfTraceDir = Join-Path $passDir "release-handoff-pdf-visual-gate-trace"
$passReleaseHandoffPdfTracePath = Join-Path $passReleaseHandoffPdfTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $passReleaseHandoffPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseHandoffPdfTracePath -Encoding UTF8 -Value @"
# Release handoff

## PDF visual gate evidence

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: ready
- PDF visual gate verdict: pass
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

& $auditScript -Path $passReleaseHandoffPdfTracePath

$passReleaseGovernanceHandoffTraceDir = Join-Path $passDir "release-governance-handoff-project-template-trace"
$passReleaseGovernanceHandoffTracePath = Join-Path $passReleaseGovernanceHandoffTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

& $auditScript -Path $passReleaseGovernanceHandoffTracePath

$passReleaseGovernanceHandoffOnboardingTraceDir = Join-Path $passDir "release-governance-handoff-project-template-onboarding-trace"
$passReleaseGovernanceHandoffOnboardingTracePath = Join-Path $passReleaseGovernanceHandoffOnboardingTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffOnboardingTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffOnboardingTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

& $auditScript -Path $passReleaseGovernanceHandoffOnboardingTracePath

$passReleaseGovernanceHandoffPdfTraceDir = Join-Path $passDir "release-governance-handoff-pdf-visual-gate-trace"
$passReleaseGovernanceHandoffPdfTracePath = Join-Path $passReleaseGovernanceHandoffPdfTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffPdfTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``
"@

& $auditScript -Path $passReleaseGovernanceHandoffPdfTracePath

$badReleaseGovernanceHandoffPdfBoundedSplitDir = Join-Path $failDir "release-governance-handoff-pdf-bounded-split"
$badReleaseGovernanceHandoffPdfBoundedSplitPath = Join-Path $badReleaseGovernanceHandoffPdfBoundedSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfBoundedSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfBoundedSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``

- Detached bounded CTest evidence:
  - pdf_bounded_ctest_summary_count: ``7``
  - pdf_bounded_ctest_pass_count: ``7``
  - pdf_bounded_ctest_skipped_test_count: ``0``
  - pdf_bounded_ctest_selected_test_count: ``70``
  - pdf_bounded_ctest_subsets: ``smoke-import, regression-business-samples``
  - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-regression-business-samples-current\summary.json``
"@

$badReleaseGovernanceHandoffPdfBoundedSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfBoundedSplitPath
} catch {
    $badReleaseGovernanceHandoffPdfBoundedSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfBoundedSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with bounded PDF CTest evidence split outside source_report block."
}

$passReleaseGovernanceHandoffPdfAttemptTraceDir = Join-Path $passDir "release-governance-handoff-pdf-attempt-trace"
$passReleaseGovernanceHandoffPdfAttemptTracePath = Join-Path $passReleaseGovernanceHandoffPdfAttemptTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffPdfAttemptTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffPdfAttemptTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``
    - pdf_visual_gate_attempt_status: ``partial``
    - pdf_visual_gate_attempt_verdict: ``not_complete``
    - pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``
    - pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``
    - pdf_visual_gate_attempt_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\attempt-summary.json``
    - pdf_visual_gate_attempt_stage_count: ``6``
    - pdf_visual_gate_attempt_passed_stage_count: ``4``
    - pdf_visual_gate_attempt_failed_stage_count: ``0``
    - pdf_visual_gate_attempt_incomplete_stage_count: ``2``
    - pdf_visual_gate_attempt_pdf_cli_export_status: ``pass``
    - pdf_visual_gate_attempt_pdf_regression_status: ``pass``
    - pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``
    - pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``
    - pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``
    - pdf_visual_gate_attempt_unicode_font_status: ``pass``
    - pdf_visual_gate_attempt_cjk_copy_search_status: ``pass``
    - pdf_visual_gate_attempt_cjk_copy_search_count: ``43``
    - pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``0``
    - pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``
    - pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``
    - pdf_visual_gate_attempt_expected_visual_render_count: ``44``
    - pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``stale``
    - pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
"@

& $auditScript -Path $passReleaseGovernanceHandoffPdfAttemptTracePath

$badReleaseGovernanceHandoffPdfAttemptSplitDir = Join-Path $failDir "release-governance-handoff-pdf-attempt-split"
$badReleaseGovernanceHandoffPdfAttemptSplitPath = Join-Path $badReleaseGovernanceHandoffPdfAttemptSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfAttemptSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfAttemptSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - full_visual_gate_status: ``pass``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
    - pdf_bounded_ctest_summary_count: ``7``
    - pdf_bounded_ctest_pass_count: ``7``
    - pdf_bounded_ctest_skipped_test_count: ``0``
    - pdf_bounded_ctest_selected_test_count: ``70``
    - pdf_bounded_ctest_subsets: ``smoke-import, contract-static, cjk-flow-static, regression-basic-text, regression-styled-document, regression-business-samples, regression-table-layout``
    - pdf_bounded_ctest_summary_json_display: ``.\build\pdf-ctest-bounded-subset-current\summary.json, .\build\pdf-ctest-bounded-contract-static-current\summary.json, .\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json, .\build\pdf-ctest-bounded-regression-basic-text-current\summary.json, .\build\pdf-ctest-bounded-regression-styled-document-current\summary.json, .\build\pdf-ctest-bounded-regression-business-samples-current\summary.json, .\build\pdf-ctest-bounded-regression-table-layout-current\summary.json``

- Detached PDF visual gate attempt evidence:
  - pdf_visual_gate_attempt_status: ``partial``
  - pdf_visual_gate_attempt_verdict: ``not_complete``
  - pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``
  - pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``
  - pdf_visual_gate_attempt_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\attempt-summary.json``
  - pdf_visual_gate_attempt_stage_count: ``6``
  - pdf_visual_gate_attempt_passed_stage_count: ``4``
  - pdf_visual_gate_attempt_failed_stage_count: ``0``
  - pdf_visual_gate_attempt_incomplete_stage_count: ``2``
  - pdf_visual_gate_attempt_pdf_cli_export_status: ``pass``
  - pdf_visual_gate_attempt_pdf_regression_status: ``pass``
  - pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``
  - pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``
  - pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``
  - pdf_visual_gate_attempt_unicode_font_status: ``pass``
  - pdf_visual_gate_attempt_cjk_copy_search_status: ``pass``
  - pdf_visual_gate_attempt_cjk_copy_search_count: ``43``
  - pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``0``
  - pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``
  - pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``
  - pdf_visual_gate_attempt_expected_visual_render_count: ``44``
  - pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``stale``
  - pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
"@

$badReleaseGovernanceHandoffPdfAttemptSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfAttemptSplitPath
} catch {
    $badReleaseGovernanceHandoffPdfAttemptSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfAttemptSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with PDF visual gate attempt evidence split outside source_report block."
}

$passReleaseGovernanceHandoffProjectTemplateChecklistTraceDir = Join-Path $passDir "release-governance-handoff-project-template-checklist-source-report-trace"
$passReleaseGovernanceHandoffProjectTemplateChecklistTracePath = Join-Path $passReleaseGovernanceHandoffProjectTemplateChecklistTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffProjectTemplateChecklistTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffProjectTemplateChecklistTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Project-template readiness checklist entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - project_template_readiness_checklist_entrypoints_status: ``declared``
    - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
    - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
    - project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
"@

& $auditScript -Path $passReleaseGovernanceHandoffProjectTemplateChecklistTracePath

$badReleaseGovernanceHandoffProjectTemplateChecklistSplitDir = Join-Path $failDir "release-governance-handoff-project-template-checklist-split"
$badReleaseGovernanceHandoffProjectTemplateChecklistSplitPath = Join-Path $badReleaseGovernanceHandoffProjectTemplateChecklistSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffProjectTemplateChecklistSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffProjectTemplateChecklistSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Project-template readiness checklist entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - project_template_readiness_checklist_entrypoints_status: ``declared``
    - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
    - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``

## Detached checklist notes

- project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
- project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
"@

$badReleaseGovernanceHandoffProjectTemplateChecklistSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffProjectTemplateChecklistSplitPath
} catch {
    $badReleaseGovernanceHandoffProjectTemplateChecklistSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffProjectTemplateChecklistSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with project-template checklist entrypoints split outside source_report block."
}

$badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaDir = Join-Path $failDir "release-governance-handoff-project-template-checklist-wrong-schema"
$badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaPath = Join-Path $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Project-template readiness checklist entrypoints evidence source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_blocker_rollup_report.v1``
    - project_template_readiness_checklist_entrypoints_status: ``declared``
    - project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``
    - project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``3``
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``
    - project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
"@

$badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaPath
} catch {
    $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffProjectTemplateChecklistWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with project-template checklist entrypoints source_report using a non-release-candidate schema."
}

$passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTraceDir = Join-Path $passDir "release-governance-handoff-project-template-checklist-material-safety-audit-trace"
$passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTracePath = Join-Path $passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTraceDir -Force | Out-Null
Set-Content -LiteralPath $passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Release-entry project-template readiness checklist material-safety audit source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_candidate_summary``
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

& $auditScript -Path $passReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditTracePath

$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitDir = Join-Path $failDir "release-governance-handoff-project-template-checklist-material-safety-audit-split"
$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitPath = Join-Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Release-entry project-template readiness checklist material-safety audit source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_candidate_summary``
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``

## Detached release-entry checklist material-safety audit notes

- release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
- release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
- release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitPath
} catch {
    $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with release-entry project-template checklist material-safety audit evidence split outside source_report block."
}

$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir = Join-Path $failDir "release-governance-handoff-project-template-checklist-material-safety-audit-wrong-schema"
$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath = Join-Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- Release-entry project-template readiness checklist material-safety audit source reports: ``1``
  - source_report: ``.\output\release-blocker-rollup\summary.json`` schema=``featherdoc.release_blocker_rollup_report.v1``
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``
"@

$badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath
} catch {
    $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release governance handoff with release-entry project-template checklist material-safety audit source_report using a non-release-candidate schema."
}

$passFinalReviewTraceDir = Join-Path $passDir "final-review-project-template-trace"
$passFinalReviewTracePath = Join-Path $passFinalReviewTraceDir "final_review.md"
New-Item -ItemType Directory -Path $passFinalReviewTraceDir -Force | Out-Null
Set-Content -LiteralPath $passFinalReviewTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

- Project-template readiness checklist entrypoints evidence source reports: 1
  - source_report: .\output\release-candidate-checks\summary.json schema=featherdoc.release_candidate_summary
    - project_template_readiness_checklist_entrypoints_status: declared
    - project_template_readiness_checklist_entrypoints_checklist_label: Project template release readiness checklist
    - project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist
    - project_template_readiness_checklist_entrypoints_checklist_marker: release_entry_project_template_readiness_checklist_trace
- Release-entry project-template readiness checklist material-safety audit source reports: 1
  - source_report: .\output\release-candidate-checks\summary.json schema=featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: passed
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: .\scripts\assert_release_material_safety.ps1
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: 3
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: Project-template readiness checklist handoff evidence
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: project_template_readiness_checklist_entrypoints_source_reports
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: release_entry_project_template_readiness_checklist_trace
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace

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
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

& $auditScript -Path $passFinalReviewTracePath

$badFinalReviewProjectTemplateChecklistWrongSchemaDir = Join-Path $failDir "final-review-project-template-checklist-wrong-schema"
$badFinalReviewProjectTemplateChecklistWrongSchemaPath = Join-Path $badFinalReviewProjectTemplateChecklistWrongSchemaDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewProjectTemplateChecklistWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewProjectTemplateChecklistWrongSchemaPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

- Project-template readiness checklist entrypoints evidence source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_blocker_rollup_report.v1
    - project_template_readiness_checklist_entrypoints_status: declared
    - project_template_readiness_checklist_entrypoints_checklist_label: Project template release readiness checklist
    - project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3
    - project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist
    - project_template_readiness_checklist_entrypoints_checklist_marker: release_entry_project_template_readiness_checklist_trace
"@

$badFinalReviewProjectTemplateChecklistWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewProjectTemplateChecklistWrongSchemaPath
} catch {
    $badFinalReviewProjectTemplateChecklistWrongSchemaFailedAsExpected = $true
}

if (-not $badFinalReviewProjectTemplateChecklistWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final review with project-template checklist entrypoints source_report using a non-release-candidate schema."
}

$badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir = Join-Path $failDir "final-review-project-template-checklist-material-safety-audit-wrong-schema"
$badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath = Join-Path $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

- Release-entry project-template readiness checklist material-safety audit source reports: 1
  - source_report: .\output\release-blocker-rollup\summary.json schema=featherdoc.release_blocker_rollup_report.v1
    - release_entry_project_template_readiness_checklist_material_safety_audit_status: passed
    - release_entry_project_template_readiness_checklist_material_safety_audit_script: .\scripts\assert_release_material_safety.ps1
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: 3
    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: Project-template readiness checklist handoff evidence
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: project_template_readiness_checklist_entrypoints_source_reports
    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: docs/project_template_release_readiness_checklist_zh.rst
    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: release_entry_project_template_readiness_checklist_trace
    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace
"@

$badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaPath
} catch {
    $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected = $true
}

if (-not $badFinalReviewProjectTemplateChecklistMaterialSafetyAuditWrongSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final review with release-entry project-template checklist material-safety audit source_report using a non-release-candidate schema."
}

$badEntryMissingGovernanceMetricDetailsDir = Join-Path $failDir "entry-missing-governance-metric-details"
$badEntryMissingGovernanceMetricDetailsPath = Join-Path $badEntryMissingGovernanceMetricDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingGovernanceMetricDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingGovernanceMetricDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence low 56 source_schema=featherdoc.numbering_catalog_governance_report.v1
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0
"@

$missingEntryGovernanceMetricDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingGovernanceMetricDetailsPath
} catch {
    $missingEntryGovernanceMetricDetailsFailedAsExpected = $true
}

if (-not $missingEntryGovernanceMetricDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without governance metric detail markers."
}

$badEntryDetachedNumberingDetailsDir = Join-Path $failDir "entry-numbering-details-supplied-by-detached-notes"
$badEntryDetachedNumberingDetailsPath = Join-Path $badEntryDetachedNumberingDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryDetachedNumberingDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryDetachedNumberingDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence level=low score=56
- Detached governance details are intentionally outside this metric.
- Detached details: catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
"@

$detachedEntryNumberingDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryDetachedNumberingDetailsPath
} catch {
    $detachedEntryNumberingDetailsFailedAsExpected = $true
}

if (-not $detachedEntryNumberingDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with numbering details supplied only by detached notes."
}

$badEntryDetachedTableLayoutDetailsDir = Join-Path $failDir "entry-table-layout-details-supplied-by-detached-notes"
$badEntryDetachedTableLayoutDetailsPath = Join-Path $badEntryDetachedTableLayoutDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryDetachedTableLayoutDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryDetachedTableLayoutDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready
- Detached governance details are intentionally outside this metric.
- Detached details: table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

$detachedEntryTableLayoutDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryDetachedTableLayoutDetailsPath
} catch {
    $detachedEntryTableLayoutDetailsFailedAsExpected = $true
}

if (-not $detachedEntryTableLayoutDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with table-layout details supplied only by detached notes."
}

$badEntryMissingRepairDetailsDir = Join-Path $failDir "entry-missing-content-control-repair-details"
$badEntryMissingRepairDetailsPath = Join-Path $badEntryMissingRepairDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingRepairDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingRepairDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$missingEntryContentControlRepairDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingRepairDetailsPath
} catch {
    $missingEntryContentControlRepairDetailsFailedAsExpected = $true
}

if (-not $missingEntryContentControlRepairDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without full content-control repair workflow details."
}

$badEntryDetachedContentControlDetailsDir = Join-Path $failDir "entry-content-control-details-supplied-by-detached-notes"
$badEntryDetachedContentControlDetailsPath = Join-Path $badEntryDetachedContentControlDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryDetachedContentControlDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryDetachedContentControlDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Detached governance details are intentionally outside this entry.
- Detached details: source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json input_docx=samples/invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
"@

$detachedEntryContentControlDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryDetachedContentControlDetailsPath
} catch {
    $detachedEntryContentControlDetailsFailedAsExpected = $true
}

if (-not $detachedEntryContentControlDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with content-control repair details supplied only by detached notes."
}

$badEntryMissingProjectTemplateContractsDir = Join-Path $failDir "entry-missing-project-template-contracts"
$badEntryMissingProjectTemplateContractsPath = Join-Path $badEntryMissingProjectTemplateContractsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingProjectTemplateContractsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingProjectTemplateContractsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness release_ready
- Project template onboarding: project_template_onboarding.schema_approval approved
"@

$missingEntryProjectTemplateContractsFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingProjectTemplateContractsPath
} catch {
    $missingEntryProjectTemplateContractsFailedAsExpected = $true
}

if (-not $missingEntryProjectTemplateContractsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document without full project template contract details."
}

$badEntryMissingReadinessSchemaSummaryDir = Join-Path $failDir "entry-missing-readiness-schema-summary"
$badEntryMissingReadinessSchemaSummaryPath = Join-Path $badEntryMissingReadinessSchemaSummaryDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingReadinessSchemaSummaryDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingReadinessSchemaSummaryPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Content-control provenance: input_docx=samples/invoice.docx template_name=invoice-template schema_target=invoice target_mode=resolved-section-targets
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$missingEntryReadinessSchemaSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingReadinessSchemaSummaryPath
} catch {
    $missingEntryReadinessSchemaSummaryFailedAsExpected = $true
}

if (-not $missingEntryReadinessSchemaSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document where only onboarding carried schema_approval_status_summary."
}

$badEntryProjectTemplateReadinessSplitDir = Join-Path $failDir "entry-project-template-readiness-split-block"
$badEntryProjectTemplateReadinessSplitPath = Join-Path $badEntryProjectTemplateReadinessSplitDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateReadinessSplitDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateReadinessSplitPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Detached readiness note: project_template_delivery_readiness source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$projectTemplateReadinessSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateReadinessSplitPath
} catch {
    $projectTemplateReadinessSplitFailedAsExpected = $true
}

if (-not $projectTemplateReadinessSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with project-template readiness details split across detached notes."
}

$badEntryProjectTemplateOnboardingSplitDir = Join-Path $failDir "entry-project-template-onboarding-split-block"
$badEntryProjectTemplateOnboardingSplitPath = Join-Path $badEntryProjectTemplateOnboardingSplitDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateOnboardingSplitDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateOnboardingSplitPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Detached onboarding note: project_template_onboarding.schema_approval source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$projectTemplateOnboardingSplitFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateOnboardingSplitPath
} catch {
    $projectTemplateOnboardingSplitFailedAsExpected = $true
}

if (-not $projectTemplateOnboardingSplitFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with project-template onboarding details split across detached notes."
}

$badEntryMissingReadinessForOnboardingDir = Join-Path $failDir "entry-missing-readiness-for-onboarding"
$badEntryMissingReadinessForOnboardingPath = Join-Path $badEntryMissingReadinessForOnboardingDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingReadinessForOnboardingDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingReadinessForOnboardingPath -Encoding UTF8 -Value @"
# START_HERE

- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$missingEntryReadinessForOnboardingFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryMissingReadinessForOnboardingPath
} catch {
    $missingEntryReadinessForOnboardingFailedAsExpected = $true
}

if (-not $missingEntryReadinessForOnboardingFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document with onboarding governance but without delivery readiness contract."
}

$badEntryProjectTemplateOnboardingSourceJsonDir = Join-Path $failDir "entry-onboarding-source-json-supplied-by-readiness"
$badEntryProjectTemplateOnboardingSourceJsonPath = Join-Path $badEntryProjectTemplateOnboardingSourceJsonDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateOnboardingSourceJsonDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateOnboardingSourceJsonPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$missingEntryProjectTemplateOnboardingSourceJsonFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateOnboardingSourceJsonPath
} catch {
    $missingEntryProjectTemplateOnboardingSourceJsonFailedAsExpected = $true
}

if (-not $missingEntryProjectTemplateOnboardingSourceJsonFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed entry document where readiness source_json_display satisfied onboarding."
}

$badDraftFile = Join-Path $failDir "bad_draft.md"
Set-Content -LiteralPath $badDraftFile -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷璇存槑鑽夌

- This file is still drafting and should not be public.
"@

$badPathFile = Join-Path $failDir "bad_path.md"
Set-Content -LiteralPath $badPathFile -Encoding UTF8 -Value @"
# FeatherDoc v1.6.4 鍙戝竷璇存槑

- Local path leak: C:\Users\wuxianggujun\CodeSpace\CMakeProjects\FeatherDoc\output\release-candidate-checks\report\release_body.zh-CN.md
"@

$failedAsExpected = $false
try {
    & $auditScript -Path @($badDraftFile, $badPathFile)
} catch {
    $failedAsExpected = $true
}

if (-not $failedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed forbidden release materials."
}

$badSummaryPath = Join-Path $failDir "summary.json"
$badSummary = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    release_handoff = ".\output\release-candidate-checks\report\release_handoff.md"
}
($badSummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badSummaryPath -Encoding UTF8

$missingSummaryMetricsFailedAsExpected = $false
try {
    & $auditScript -Path $badSummaryPath
} catch {
    $missingSummaryMetricsFailedAsExpected = $true
}

if (-not $missingSummaryMetricsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release summary without governance_metrics."
}

$badSummaryWrongRealCorpusContractDir = Join-Path $failDir "summary-wrong-real-corpus-contract"
$badSummaryWrongRealCorpusContractPath = Join-Path $badSummaryWrongRealCorpusContractDir "summary.json"
New-Item -ItemType Directory -Path $badSummaryWrongRealCorpusContractDir -Force | Out-Null
$badSummaryWrongRealCorpusContract = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    release_handoff = ".\output\release-candidate-checks\report\release_handoff.md"
    governance_metric_count = 2
    governance_metrics = @(
        [ordered]@{
            id = "style_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "style_catalog_governance"
            source_schema = "featherdoc.style_catalog_governance_report.v1"
            score = 12
            level = "experimental"
        },
        [ordered]@{
            id = "table_layout_delivery_governance.delivery_quality"
            metric = "delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            score = 100
            level = "release_ready"
        }
    )
}
($badSummaryWrongRealCorpusContract | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badSummaryWrongRealCorpusContractPath -Encoding UTF8

$wrongSummaryRealCorpusContractFailedAsExpected = $false
try {
    & $auditScript -Path $badSummaryWrongRealCorpusContractPath
} catch {
    $wrongSummaryRealCorpusContractFailedAsExpected = $true
}

if (-not $wrongSummaryRealCorpusContractFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release summary without numbering_catalog_governance.real_corpus_confidence."
}

$badManifestPath = Join-Path $failDir "release_assets_manifest.json"
$badManifest = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = 1
    governance_metrics = @(
        [ordered]@{
            id = "numbering_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            score = 56
            level = "low"
        }
    )
    content_control_repair_contract_count = 0
    content_control_repair_contracts = @()
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestPath -Encoding UTF8

$missingManifestMetricFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestPath
} catch {
    $missingManifestMetricFailedAsExpected = $true
}

if (-not $missingManifestMetricFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without delivery_quality."
}

$badManifestMissingContentControlContractDir = Join-Path $failDir "manifest-missing-content-control-contract"
$badManifestMissingContentControlContractPath = Join-Path $badManifestMissingContentControlContractDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingContentControlContractDir -Force | Out-Null
$badManifestMissingContentControlContract = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestMissingContentControlContract | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingContentControlContractPath -Encoding UTF8

$missingManifestContentControlContractFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingContentControlContractPath
} catch {
    $missingManifestContentControlContractFailedAsExpected = $true
}

if (-not $missingManifestContentControlContractFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without content_control_repair_contracts."
}

$badManifestContentControlContractCountDir = Join-Path $failDir "manifest-bad-content-control-contract-count"
$badManifestContentControlContractCountPath = Join-Path $badManifestContentControlContractCountDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestContentControlContractCountDir -Force | Out-Null
$badManifestContentControlContractCount = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 2
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestContentControlContractCount | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestContentControlContractCountPath -Encoding UTF8

$badManifestContentControlContractCountFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestContentControlContractCountPath
} catch {
    $badManifestContentControlContractCountFailedAsExpected = $true
}

if (-not $badManifestContentControlContractCountFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched content_control_repair_contract_count."
}

$badManifestContentControlContractSourceSchemaDir = Join-Path $failDir "manifest-content-control-contract-missing-source-schema"
$badManifestContentControlContractSourceSchemaPath = Join-Path $badManifestContentControlContractSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestContentControlContractSourceSchemaDir -Force | Out-Null
$badManifestContentControlContractSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestContentControlContractSourceSchema.content_control_repair_contracts[0].PSObject.Properties.Remove("source_schema")
($badManifestContentControlContractSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestContentControlContractSourceSchemaPath -Encoding UTF8

$badManifestContentControlContractSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestContentControlContractSourceSchemaPath
} catch {
    $badManifestContentControlContractSourceSchemaFailedAsExpected = $true
}

if (-not $badManifestContentControlContractSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with content-control repair contract missing source_schema."
}

$badManifestContentControlContractSourceJsonDir = Join-Path $failDir "manifest-content-control-contract-missing-source-json"
$badManifestContentControlContractSourceJsonPath = Join-Path $badManifestContentControlContractSourceJsonDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestContentControlContractSourceJsonDir -Force | Out-Null
$badManifestContentControlContractSourceJson = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestContentControlContractSourceJson.content_control_repair_contracts[0].PSObject.Properties.Remove("source_json_display")
($badManifestContentControlContractSourceJson | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestContentControlContractSourceJsonPath -Encoding UTF8

$badManifestContentControlContractSourceJsonFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestContentControlContractSourceJsonPath
} catch {
    $badManifestContentControlContractSourceJsonFailedAsExpected = $true
}

if (-not $badManifestContentControlContractSourceJsonFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with content-control repair contract missing source_json_display."
}

$badManifestProjectTemplateReadinessSourceSchemaDir = Join-Path $failDir "manifest-project-template-readiness-missing-source-schema"
$badManifestProjectTemplateReadinessSourceSchemaPath = Join-Path $badManifestProjectTemplateReadinessSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSourceSchemaDir -Force | Out-Null
$badManifestProjectTemplateReadinessSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSourceSchema.project_template_delivery_readiness_contract.PSObject.Properties.Remove("source_schema")
($badManifestProjectTemplateReadinessSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSourceSchemaPath -Encoding UTF8

$badManifestProjectTemplateReadinessSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSourceSchemaPath
} catch {
    $badManifestProjectTemplateReadinessSourceSchemaFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness contract missing source_schema."
}

$badManifestProjectTemplateReadinessSourceJsonDir = Join-Path $failDir "manifest-project-template-readiness-missing-source-json"
$badManifestProjectTemplateReadinessSourceJsonPath = Join-Path $badManifestProjectTemplateReadinessSourceJsonDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSourceJsonDir -Force | Out-Null
$badManifestProjectTemplateReadinessSourceJson = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSourceJson.project_template_delivery_readiness_contract.PSObject.Properties.Remove("source_json_display")
($badManifestProjectTemplateReadinessSourceJson | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSourceJsonPath -Encoding UTF8

$badManifestProjectTemplateReadinessSourceJsonFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSourceJsonPath
} catch {
    $badManifestProjectTemplateReadinessSourceJsonFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSourceJsonFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness contract missing source_json_display."
}

$badManifestProjectTemplateReadinessSourceReportDir = Join-Path $failDir "manifest-project-template-readiness-missing-source-report"
$badManifestProjectTemplateReadinessSourceReportPath = Join-Path $badManifestProjectTemplateReadinessSourceReportDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSourceReportDir -Force | Out-Null
$badManifestProjectTemplateReadinessSourceReport = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSourceReport.project_template_delivery_readiness_contract.PSObject.Properties.Remove("source_report_display")
($badManifestProjectTemplateReadinessSourceReport | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSourceReportPath -Encoding UTF8

$badManifestProjectTemplateReadinessSourceReportFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSourceReportPath
} catch {
    $badManifestProjectTemplateReadinessSourceReportFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSourceReportFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness contract missing source_report_display."
}

$badManifestProjectTemplateReadinessSourceIdentityDir = Join-Path $failDir "manifest-project-template-readiness-wrong-source-identity"
$badManifestProjectTemplateReadinessSourceIdentityPath = Join-Path $badManifestProjectTemplateReadinessSourceIdentityDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSourceIdentityDir -Force | Out-Null
$badManifestProjectTemplateReadinessSourceIdentity = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSourceIdentity.project_template_delivery_readiness_contract.source_report_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
$badManifestProjectTemplateReadinessSourceIdentity.project_template_delivery_readiness_contract.source_json_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
($badManifestProjectTemplateReadinessSourceIdentity | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSourceIdentityPath -Encoding UTF8

$badManifestProjectTemplateReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSourceIdentityPath
} catch {
    $badManifestProjectTemplateReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness source displays pointing at onboarding governance evidence."
}

$badManifestProjectTemplateReadinessSchemaSummaryDir = Join-Path $failDir "manifest-project-template-readiness-missing-schema-summary"
$badManifestProjectTemplateReadinessSchemaSummaryPath = Join-Path $badManifestProjectTemplateReadinessSchemaSummaryDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSchemaSummaryDir -Force | Out-Null
$badManifestProjectTemplateReadinessSchemaSummary = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSchemaSummary.project_template_delivery_readiness_contract.PSObject.Properties.Remove("schema_approval_status_summary")
($badManifestProjectTemplateReadinessSchemaSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSchemaSummaryPath -Encoding UTF8

$badManifestProjectTemplateReadinessSchemaSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSchemaSummaryPath
} catch {
    $badManifestProjectTemplateReadinessSchemaSummaryFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSchemaSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness contract missing schema_approval_status_summary."
}

$badManifestProjectTemplateReadinessStatusDir = Join-Path $failDir "manifest-project-template-readiness-status-release-ready-mismatch"
$badManifestProjectTemplateReadinessStatusPath = Join-Path $badManifestProjectTemplateReadinessStatusDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessStatusDir -Force | Out-Null
$badManifestProjectTemplateReadinessStatus = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessStatus.project_template_delivery_readiness_contract.status = "blocked"
($badManifestProjectTemplateReadinessStatus | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessStatusPath -Encoding UTF8

$badManifestProjectTemplateReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessStatusPath
} catch {
    $badManifestProjectTemplateReadinessStatusFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with release_ready=true but project template readiness status not ready."
}

$badManifestProjectTemplateReadinessInvalidStatusDir = Join-Path $failDir "manifest-project-template-readiness-invalid-status"
$badManifestProjectTemplateReadinessInvalidStatusPath = Join-Path $badManifestProjectTemplateReadinessInvalidStatusDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessInvalidStatusDir -Force | Out-Null
$badManifestProjectTemplateReadinessInvalidStatus = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessInvalidStatus.project_template_delivery_readiness_contract.status = "ready-ish"
($badManifestProjectTemplateReadinessInvalidStatus | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessInvalidStatusPath -Encoding UTF8

$badManifestProjectTemplateReadinessInvalidStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessInvalidStatusPath
} catch {
    $badManifestProjectTemplateReadinessInvalidStatusFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessInvalidStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid project template readiness status."
}

$badManifestProjectTemplateReadinessReleaseReadyDir = Join-Path $failDir "manifest-project-template-readiness-release-ready-status-mismatch"
$badManifestProjectTemplateReadinessReleaseReadyPath = Join-Path $badManifestProjectTemplateReadinessReleaseReadyDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessReleaseReadyDir -Force | Out-Null
$badManifestProjectTemplateReadinessReleaseReady = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessReleaseReady.project_template_delivery_readiness_contract.release_ready = $false
$badManifestProjectTemplateReadinessReleaseReady.project_template_delivery_readiness_contract.release_blocker_count = 1
($badManifestProjectTemplateReadinessReleaseReady | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessReleaseReadyPath -Encoding UTF8

$badManifestProjectTemplateReadinessReleaseReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessReleaseReadyPath
} catch {
    $badManifestProjectTemplateReadinessReleaseReadyFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessReleaseReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness status=ready but release_ready=false."
}

$badManifestProjectTemplateReadinessInvalidReleaseReadyDir = Join-Path $failDir "manifest-project-template-readiness-invalid-release-ready"
$badManifestProjectTemplateReadinessInvalidReleaseReadyPath = Join-Path $badManifestProjectTemplateReadinessInvalidReleaseReadyDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessInvalidReleaseReadyDir -Force | Out-Null
$badManifestProjectTemplateReadinessInvalidReleaseReady = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessInvalidReleaseReady.project_template_delivery_readiness_contract.release_ready = "maybe"
($badManifestProjectTemplateReadinessInvalidReleaseReady | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessInvalidReleaseReadyPath -Encoding UTF8

$badManifestProjectTemplateReadinessInvalidReleaseReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessInvalidReleaseReadyPath
} catch {
    $badManifestProjectTemplateReadinessInvalidReleaseReadyFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessInvalidReleaseReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid project template readiness release_ready."
}

$badManifestProjectTemplateOnboardingSourceSchemaDir = Join-Path $failDir "manifest-project-template-onboarding-missing-source-schema"
$badManifestProjectTemplateOnboardingSourceSchemaPath = Join-Path $badManifestProjectTemplateOnboardingSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSourceSchemaDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSourceSchema.project_template_onboarding_governance_contract.PSObject.Properties.Remove("source_schema")
($badManifestProjectTemplateOnboardingSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSourceSchemaPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSourceSchemaPath
} catch {
    $badManifestProjectTemplateOnboardingSourceSchemaFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding contract missing source_schema."
}

$badManifestProjectTemplateOnboardingSourceJsonDir = Join-Path $failDir "manifest-project-template-onboarding-missing-source-json"
$badManifestProjectTemplateOnboardingSourceJsonPath = Join-Path $badManifestProjectTemplateOnboardingSourceJsonDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSourceJsonDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSourceJson = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSourceJson.project_template_onboarding_governance_contract.PSObject.Properties.Remove("source_json_display")
($badManifestProjectTemplateOnboardingSourceJson | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSourceJsonPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSourceJsonFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSourceJsonPath
} catch {
    $badManifestProjectTemplateOnboardingSourceJsonFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSourceJsonFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding contract missing source_json_display."
}

$badManifestProjectTemplateOnboardingSourceReportDir = Join-Path $failDir "manifest-project-template-onboarding-missing-source-report"
$badManifestProjectTemplateOnboardingSourceReportPath = Join-Path $badManifestProjectTemplateOnboardingSourceReportDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSourceReportDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSourceReport = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSourceReport.project_template_onboarding_governance_contract.PSObject.Properties.Remove("source_report_display")
($badManifestProjectTemplateOnboardingSourceReport | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSourceReportPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSourceReportFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSourceReportPath
} catch {
    $badManifestProjectTemplateOnboardingSourceReportFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSourceReportFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding contract missing source_report_display."
}

$badManifestProjectTemplateOnboardingSourceIdentityDir = Join-Path $failDir "manifest-project-template-onboarding-wrong-source-identity"
$badManifestProjectTemplateOnboardingSourceIdentityPath = Join-Path $badManifestProjectTemplateOnboardingSourceIdentityDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSourceIdentityDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSourceIdentity = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSourceIdentity.project_template_onboarding_governance_contract.source_report_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
$badManifestProjectTemplateOnboardingSourceIdentity.project_template_onboarding_governance_contract.source_json_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
($badManifestProjectTemplateOnboardingSourceIdentity | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSourceIdentityPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSourceIdentityPath
} catch {
    $badManifestProjectTemplateOnboardingSourceIdentityFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding source displays pointing at delivery readiness evidence."
}

$badManifestProjectTemplateOnboardingSchemaSummaryDir = Join-Path $failDir "manifest-project-template-onboarding-empty-schema-summary"
$badManifestProjectTemplateOnboardingSchemaSummaryPath = Join-Path $badManifestProjectTemplateOnboardingSchemaSummaryDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSchemaSummaryDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSchemaSummary = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSchemaSummary.project_template_onboarding_governance_contract.schema_approval_status_summary = @()
($badManifestProjectTemplateOnboardingSchemaSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSchemaSummaryPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSchemaSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSchemaSummaryPath
} catch {
    $badManifestProjectTemplateOnboardingSchemaSummaryFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSchemaSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with empty project template onboarding schema_approval_status_summary."
}

$badManifestProjectTemplateOnboardingInvalidStatusDir = Join-Path $failDir "manifest-project-template-onboarding-invalid-status"
$badManifestProjectTemplateOnboardingInvalidStatusPath = Join-Path $badManifestProjectTemplateOnboardingInvalidStatusDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingInvalidStatusDir -Force | Out-Null
$badManifestProjectTemplateOnboardingInvalidStatus = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingInvalidStatus.project_template_onboarding_governance_contract.status = "ready-ish"
($badManifestProjectTemplateOnboardingInvalidStatus | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingInvalidStatusPath -Encoding UTF8

$badManifestProjectTemplateOnboardingInvalidStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingInvalidStatusPath
} catch {
    $badManifestProjectTemplateOnboardingInvalidStatusFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingInvalidStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid project template onboarding status."
}

$badManifestProjectTemplateOnboardingReleaseReadyDir = Join-Path $failDir "manifest-project-template-onboarding-release-ready-status-mismatch"
$badManifestProjectTemplateOnboardingReleaseReadyPath = Join-Path $badManifestProjectTemplateOnboardingReleaseReadyDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingReleaseReadyDir -Force | Out-Null
$badManifestProjectTemplateOnboardingReleaseReady = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingReleaseReady.project_template_onboarding_governance_contract.release_ready = $false
$badManifestProjectTemplateOnboardingReleaseReady.project_template_onboarding_governance_contract.release_blocker_count = 1
($badManifestProjectTemplateOnboardingReleaseReady | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingReleaseReadyPath -Encoding UTF8

$badManifestProjectTemplateOnboardingReleaseReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingReleaseReadyPath
} catch {
    $badManifestProjectTemplateOnboardingReleaseReadyFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingReleaseReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding status=ready but release_ready=false."
}

$badManifestProjectTemplateOnboardingInvalidReleaseReadyDir = Join-Path $failDir "manifest-project-template-onboarding-invalid-release-ready"
$badManifestProjectTemplateOnboardingInvalidReleaseReadyPath = Join-Path $badManifestProjectTemplateOnboardingInvalidReleaseReadyDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingInvalidReleaseReadyDir -Force | Out-Null
$badManifestProjectTemplateOnboardingInvalidReleaseReady = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingInvalidReleaseReady.project_template_onboarding_governance_contract.release_ready = "maybe"
($badManifestProjectTemplateOnboardingInvalidReleaseReady | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingInvalidReleaseReadyPath -Encoding UTF8

$badManifestProjectTemplateOnboardingInvalidReleaseReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingInvalidReleaseReadyPath
} catch {
    $badManifestProjectTemplateOnboardingInvalidReleaseReadyFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingInvalidReleaseReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid project template onboarding release_ready."
}

$badManifestMissingNumberingConfidenceDir = Join-Path $failDir "manifest-missing-numbering-confidence"
$badManifestMissingNumberingConfidencePath = Join-Path $badManifestMissingNumberingConfidenceDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingNumberingConfidenceDir -Force | Out-Null
$badManifestMissingNumberingConfidence = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestMissingNumberingConfidence | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingNumberingConfidencePath -Encoding UTF8

$missingManifestNumberingConfidenceFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingNumberingConfidencePath
} catch {
    $missingManifestNumberingConfidenceFailedAsExpected = $true
}

if (-not $missingManifestNumberingConfidenceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without numbering_catalog_real_corpus_confidence."
}

$badManifestNumberingConfidenceMismatchDir = Join-Path $failDir "manifest-bad-numbering-confidence"
$badManifestNumberingConfidenceMismatchPath = Join-Path $badManifestNumberingConfidenceMismatchDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestNumberingConfidenceMismatchDir -Force | Out-Null
$badManifestNumberingConfidenceMismatch = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = [ordered]@{
        id = "numbering_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "numbering_catalog_governance"
        source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        score = 55
        level = "low"
        details = $governanceMetrics[1].details
    }
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestNumberingConfidenceMismatch | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestNumberingConfidenceMismatchPath -Encoding UTF8

$badManifestNumberingConfidenceMismatchFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestNumberingConfidenceMismatchPath
} catch {
    $badManifestNumberingConfidenceMismatchFailedAsExpected = $true
}

if (-not $badManifestNumberingConfidenceMismatchFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched numbering_catalog_real_corpus_confidence."
}

$badManifestStyleAsNumberingConfidenceDir = Join-Path $failDir "manifest-style-as-numbering-confidence"
$badManifestStyleAsNumberingConfidencePath = Join-Path $badManifestStyleAsNumberingConfidenceDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestStyleAsNumberingConfidenceDir -Force | Out-Null
$badManifestStyleAsNumberingConfidence = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = [ordered]@{
        id = "style_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "style_catalog_governance"
        source_schema = "featherdoc.style_catalog_governance_report.v1"
        score = 12
        level = "experimental"
        details = $governanceMetrics[1].details
    }
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestStyleAsNumberingConfidence | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestStyleAsNumberingConfidencePath -Encoding UTF8

$badManifestStyleAsNumberingConfidenceFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestStyleAsNumberingConfidencePath
} catch {
    $badManifestStyleAsNumberingConfidenceFailedAsExpected = $true
}

if (-not $badManifestStyleAsNumberingConfidenceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with style_catalog real corpus confidence mirrored as numbering."
}

$badManifestMissingTableLayoutQualityDir = Join-Path $failDir "manifest-missing-table-layout-quality"
$badManifestMissingTableLayoutQualityPath = Join-Path $badManifestMissingTableLayoutQualityDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingTableLayoutQualityDir -Force | Out-Null
$badManifestMissingTableLayoutQuality = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestMissingTableLayoutQuality | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingTableLayoutQualityPath -Encoding UTF8

$missingManifestTableLayoutQualityFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingTableLayoutQualityPath
} catch {
    $missingManifestTableLayoutQualityFailedAsExpected = $true
}

if (-not $missingManifestTableLayoutQualityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without table_layout_delivery_quality."
}

$badManifestTableLayoutQualityMismatchDir = Join-Path $failDir "manifest-bad-table-layout-quality"
$badManifestTableLayoutQualityMismatchPath = Join-Path $badManifestTableLayoutQualityMismatchDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestTableLayoutQualityMismatchDir -Force | Out-Null
$badManifestTableLayoutQualityMismatch = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = [ordered]@{
        id = "table_layout_delivery_governance.delivery_quality"
        metric = "delivery_quality"
        report_id = "table_layout_delivery_governance"
        source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        score = 90
        level = "release_ready"
        details = $governanceMetrics[2].details
    }
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestTableLayoutQualityMismatch | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestTableLayoutQualityMismatchPath -Encoding UTF8

$badManifestTableLayoutQualityMismatchFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestTableLayoutQualityMismatchPath
} catch {
    $badManifestTableLayoutQualityMismatchFailedAsExpected = $true
}

if (-not $badManifestTableLayoutQualityMismatchFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched table_layout_delivery_quality."
}

$badManifestMissingProjectTemplateReadinessDir = Join-Path $failDir "manifest-missing-project-template-readiness"
$badManifestMissingProjectTemplateReadinessPath = Join-Path $badManifestMissingProjectTemplateReadinessDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateReadinessDir -Force | Out-Null
$badManifestMissingProjectTemplateReadiness = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestMissingProjectTemplateReadiness | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingProjectTemplateReadinessPath -Encoding UTF8

$badManifestMissingProjectTemplateReadinessFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateReadinessPath
} catch {
    $badManifestMissingProjectTemplateReadinessFailedAsExpected = $true
}

if (-not $badManifestMissingProjectTemplateReadinessFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without project_template_delivery_readiness_contract."
}

$badManifestBlockedProjectTemplateReadinessDir = Join-Path $failDir "manifest-bad-project-template-readiness"
$badManifestBlockedProjectTemplateReadinessPath = Join-Path $badManifestBlockedProjectTemplateReadinessDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestBlockedProjectTemplateReadinessDir -Force | Out-Null
$badManifestBlockedProjectTemplateReadiness = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = [ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        source_schema = "featherdoc.project_template_delivery_readiness_report.v1"
        status = "blocked"
        release_ready = $false
        latest_schema_approval_gate_status = "blocked"
        schema_approval_status_summary = @(
            [ordered]@{
                status = "blocked"
                count = 1
            }
        )
        schema_history_blocked_run_count = 1
        schema_history_pending_run_count = 0
        schema_history_passed_run_count = 2
        template_count = 4
        ready_template_count = 3
        blocked_template_count = 1
        release_blocker_count = 0
        action_item_count = 1
        warning_count = 0
        source_report_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
        source_json_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
    }
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestBlockedProjectTemplateReadiness | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestBlockedProjectTemplateReadinessPath -Encoding UTF8

$badManifestBlockedProjectTemplateReadinessFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestBlockedProjectTemplateReadinessPath
} catch {
    $badManifestBlockedProjectTemplateReadinessFailedAsExpected = $true
}

if (-not $badManifestBlockedProjectTemplateReadinessFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with release_ready=false and release_blocker_count=0."
}

$badManifestMissingProjectTemplateOnboardingDir = Join-Path $failDir "manifest-missing-project-template-onboarding"
$badManifestMissingProjectTemplateOnboardingPath = Join-Path $badManifestMissingProjectTemplateOnboardingDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateOnboardingDir -Force | Out-Null
$badManifestMissingProjectTemplateOnboarding = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = [ordered]@{
        id = "numbering_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "numbering_catalog_governance"
        source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        score = 56
        level = "low"
        details = $governanceMetrics[1].details
    }
    table_layout_delivery_quality = [ordered]@{
        id = "table_layout_delivery_governance.delivery_quality"
        metric = "delivery_quality"
        report_id = "table_layout_delivery_governance"
        source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        score = 100
        level = "release_ready"
        details = $governanceMetrics[2].details
    }
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
}
($badManifestMissingProjectTemplateOnboarding | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingProjectTemplateOnboardingPath -Encoding UTF8

$badManifestMissingProjectTemplateOnboardingFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateOnboardingPath
} catch {
    $badManifestMissingProjectTemplateOnboardingFailedAsExpected = $true
}

if (-not $badManifestMissingProjectTemplateOnboardingFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without project_template_onboarding_governance_contract."
}

$badManifestProjectTemplateOnboardingCountDir = Join-Path $failDir "manifest-bad-project-template-onboarding-count"
$badManifestProjectTemplateOnboardingCountPath = Join-Path $badManifestProjectTemplateOnboardingCountDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingCountDir -Force | Out-Null
$badManifestProjectTemplateOnboardingCount = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = [ordered]@{
        id = "numbering_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "numbering_catalog_governance"
        source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        score = 56
        level = "low"
        details = $governanceMetrics[1].details
    }
    table_layout_delivery_quality = [ordered]@{
        id = "table_layout_delivery_governance.delivery_quality"
        metric = "delivery_quality"
        report_id = "table_layout_delivery_governance"
        source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        score = 100
        level = "release_ready"
        details = $governanceMetrics[2].details
    }
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = [ordered]@{
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
                count = 2
            }
        )
        blocked_entry_count = 0
        pending_review_entry_count = 0
        not_evaluated_entry_count = 0
        approved_entry_count = 2
        not_required_entry_count = 1
        release_blocker_count = 0
        action_item_count = 0
        manual_review_recommendation_count = 1
        source_report_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
        source_json_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
    }
}
($badManifestProjectTemplateOnboardingCount | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingCountPath -Encoding UTF8

$badManifestProjectTemplateOnboardingCountFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingCountPath
} catch {
    $badManifestProjectTemplateOnboardingCountFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingCountFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched project_template_onboarding_governance entry counts."
}

$badContentControlStrategyPath = Join-Path $failDir "content_control_missing_repair_strategy.json"
$badContentControlStrategy = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "blocker"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
}
($badContentControlStrategy | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badContentControlStrategyPath -Encoding UTF8

$missingContentControlRepairStrategyFailedAsExpected = $false
try {
    & $auditScript -Path $badContentControlStrategyPath
} catch {
    $missingContentControlRepairStrategyFailedAsExpected = $true
}

if (-not $missingContentControlRepairStrategyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed content-control blocker without repair_strategy."
}

$badContentControlHintPath = Join-Path $failDir "content_control_bad_repair_hint.json"
$badContentControlHint = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "blocker"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Review the content control manually before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
}
($badContentControlHint | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badContentControlHintPath -Encoding UTF8

$badContentControlHintFailedAsExpected = $false
try {
    & $auditScript -Path $badContentControlHintPath
} catch {
    $badContentControlHintFailedAsExpected = $true
}

if (-not $badContentControlHintFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed content-control blocker without Custom XML sync repair_hint."
}

$badContentControlCommandPath = Join-Path $failDir "content_control_bad_command_template.json"
$badContentControlCommand = [ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "blocker"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli inspect-content-controls <input.docx> --json"
        }
    )
}
($badContentControlCommand | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badContentControlCommandPath -Encoding UTF8

$badContentControlCommandFailedAsExpected = $false
try {
    & $auditScript -Path $badContentControlCommandPath
} catch {
    $badContentControlCommandFailedAsExpected = $true
}

if (-not $badContentControlCommandFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed content-control blocker without sync command_template."
}

$badReleaseSummaryTracePath = Join-Path $failDir "release_summary.zh-CN.md"
Set-Content -LiteralPath $badReleaseSummaryTracePath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json

## Detached notes

- source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseSummaryTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryTracePath
} catch {
    $badReleaseSummaryTraceFailedAsExpected = $true
}

if (-not $badReleaseSummaryTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with project-template source_json_display outside the readiness summary line."
}

$badReleaseSummaryOnboardingTraceDir = Join-Path $failDir "release-summary-onboarding-missing-project-template-source-json"
$badReleaseSummaryOnboardingTracePath = Join-Path $badReleaseSummaryOnboardingTraceDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryOnboardingTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryOnboardingTracePath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseSummaryOnboardingTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryOnboardingTracePath
} catch {
    $badReleaseSummaryOnboardingTraceFailedAsExpected = $true
}

if (-not $badReleaseSummaryOnboardingTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with onboarding source_json_display supplied only by readiness."
}

$badReleaseSummaryReadinessSourceIdentityDir = Join-Path $failDir "release-summary-readiness-source-identity-impersonated"
$badReleaseSummaryReadinessSourceIdentityPath = Join-Path $badReleaseSummaryReadinessSourceIdentityDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryReadinessSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryReadinessSourceIdentityPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseSummaryReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryReadinessSourceIdentityPath
} catch {
    $badReleaseSummaryReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseSummaryReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with readiness source displays pointing at onboarding governance evidence."
}

$badReleaseSummaryInvalidReadinessStatusDir = Join-Path $failDir "release-summary-invalid-project-template-readiness-status"
$badReleaseSummaryInvalidReadinessStatusPath = Join-Path $badReleaseSummaryInvalidReadinessStatusDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready-ish release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseSummaryInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryInvalidReadinessStatusPath
} catch {
    $badReleaseSummaryInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badReleaseSummaryInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with an invalid project-template readiness status."
}

$badReleaseSummaryInvalidReadinessReadyDir = Join-Path $failDir "release-summary-invalid-project-template-readiness-release-ready"
$badReleaseSummaryInvalidReadinessReadyPath = Join-Path $badReleaseSummaryInvalidReadinessReadyDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=maybe latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseSummaryInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryInvalidReadinessReadyPath
} catch {
    $badReleaseSummaryInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badReleaseSummaryInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with an invalid project-template readiness release_ready."
}

$badReleaseSummaryInvalidOnboardingStatusDir = Join-Path $failDir "release-summary-invalid-project-template-onboarding-status"
$badReleaseSummaryInvalidOnboardingStatusPath = Join-Path $badReleaseSummaryInvalidOnboardingStatusDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready-ish release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseSummaryInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryInvalidOnboardingStatusPath
} catch {
    $badReleaseSummaryInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badReleaseSummaryInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with an invalid project-template onboarding status."
}

$badReleaseSummaryInvalidOnboardingReadyDir = Join-Path $failDir "release-summary-invalid-project-template-onboarding-release-ready"
$badReleaseSummaryInvalidOnboardingReadyPath = Join-Path $badReleaseSummaryInvalidOnboardingReadyDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=maybe schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseSummaryInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryInvalidOnboardingReadyPath
} catch {
    $badReleaseSummaryInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badReleaseSummaryInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with an invalid project-template onboarding release_ready."
}

$badReleaseSummaryOnboardingSourceJsonIdentityDir = Join-Path $failDir "release-summary-onboarding-source-json-identity-impersonated"
$badReleaseSummaryOnboardingSourceJsonIdentityPath = Join-Path $badReleaseSummaryOnboardingSourceJsonIdentityDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryOnboardingSourceJsonIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryOnboardingSourceJsonIdentityPath -Encoding UTF8 -Value @"
# Release summary

- project-template readiness governance contract: status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project-template onboarding governance contract: status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseSummaryOnboardingSourceJsonIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryOnboardingSourceJsonIdentityPath
} catch {
    $badReleaseSummaryOnboardingSourceJsonIdentityFailedAsExpected = $true
}

if (-not $badReleaseSummaryOnboardingSourceJsonIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with onboarding source_json_display pointing at delivery readiness evidence."
}

$badReleaseSummaryPdfSplitTraceDir = Join-Path $failDir "release-summary-split-pdf-visual-contact-sheet"
$badReleaseSummaryPdfSplitTracePath = Join-Path $badReleaseSummaryPdfSplitTraceDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryPdfSplitTracePath -Encoding UTF8 -Value @"
# Release summary

- PDF visual gate：verdict=pass summary=.\output\pdf-visual-release-gate-current\report\summary.json cjk_manifest_count=43 cjk_copy_search_count=43 visual_baseline_manifest_count=42 visual_baseline_count=44

## Detached notes

- aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReleaseSummaryPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryPdfSplitTracePath
} catch {
    $badReleaseSummaryPdfSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseSummaryPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with a PDF visual contact-sheet path outside the PDF visual gate summary line."
}

$badReleaseSummaryPdfVerdictTraceDir = Join-Path $failDir "release-summary-invalid-pdf-visual-verdict"
$badReleaseSummaryPdfVerdictTracePath = Join-Path $badReleaseSummaryPdfVerdictTraceDir "release_summary.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseSummaryPdfVerdictTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseSummaryPdfVerdictTracePath -Encoding UTF8 -Value @"
# Release summary

- PDF visual gate：verdict=blocked summary=.\output\pdf-visual-release-gate-current\report\summary.json aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png cjk_manifest_count=43 cjk_copy_search_count=43 visual_baseline_manifest_count=42 visual_baseline_count=44
"@

$badReleaseSummaryPdfVerdictTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseSummaryPdfVerdictTracePath
} catch {
    $badReleaseSummaryPdfVerdictTraceFailedAsExpected = $true
}

if (-not $badReleaseSummaryPdfVerdictTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_summary.zh-CN.md with a non-pass/fail PDF visual gate verdict."
}

$badStartHerePdfDetachedCountTraceDir = Join-Path $failDir "start-here-pdf-visual-count-supplied-by-detached-notes"
$badStartHerePdfDetachedCountTracePath = Join-Path $badStartHerePdfDetachedCountTraceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badStartHerePdfDetachedCountTraceDir -Force | Out-Null
Set-Content -LiteralPath $badStartHerePdfDetachedCountTracePath -Encoding UTF8 -Value @"
# START_HERE

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png

## Detached notes

- PDF visual baseline manifest samples: 42
"@

$badStartHerePdfDetachedCountTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badStartHerePdfDetachedCountTracePath
} catch {
    $badStartHerePdfDetachedCountTraceFailedAsExpected = $true
}

if (-not $badStartHerePdfDetachedCountTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with PDF visual counts supplied only by detached notes."
}

$badArtifactGuidePdfDetachedCountTraceDir = Join-Path $failDir "artifact-guide-pdf-visual-count-supplied-by-detached-notes"
$badArtifactGuidePdfDetachedCountTracePath = Join-Path $badArtifactGuidePdfDetachedCountTraceDir "ARTIFACT_GUIDE.md"
New-Item -ItemType Directory -Path $badArtifactGuidePdfDetachedCountTraceDir -Force | Out-Null
Set-Content -LiteralPath $badArtifactGuidePdfDetachedCountTracePath -Encoding UTF8 -Value @"
# Artifact Guide

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44

## Detached notes

- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badArtifactGuidePdfDetachedCountTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badArtifactGuidePdfDetachedCountTracePath
} catch {
    $badArtifactGuidePdfDetachedCountTraceFailedAsExpected = $true
}

if (-not $badArtifactGuidePdfDetachedCountTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed ARTIFACT_GUIDE.md with PDF visual contact sheet supplied only by detached notes."
}

$badReviewerChecklistPdfDetachedContactTraceDir = Join-Path $failDir "reviewer-checklist-pdf-visual-contact-sheet-supplied-by-detached-notes"
$badReviewerChecklistPdfDetachedContactTracePath = Join-Path $badReviewerChecklistPdfDetachedContactTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $badReviewerChecklistPdfDetachedContactTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReviewerChecklistPdfDetachedContactTracePath -Encoding UTF8 -Value @"
# Reviewer Checklist

- Confirm PDF visual gate summary .\output\pdf-visual-release-gate-current\report\summary.json with 43 CJK copy/search samples and 44 visual baselines before release.
- Confirm PDF visual gate aggregate contact sheet before release.

## Detached notes

- aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReviewerChecklistPdfDetachedContactTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReviewerChecklistPdfDetachedContactTracePath
} catch {
    $badReviewerChecklistPdfDetachedContactTraceFailedAsExpected = $true
}

if (-not $badReviewerChecklistPdfDetachedContactTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed REVIEWER_CHECKLIST.md with PDF visual contact sheet supplied only by detached notes."
}

$badReviewerChecklistPdfFinalizeTraceDir = Join-Path $failDir "reviewer-checklist-pdf-visual-finalize-line-missing-contact-sheet"
$badReviewerChecklistPdfFinalizeTracePath = Join-Path $badReviewerChecklistPdfFinalizeTraceDir "REVIEWER_CHECKLIST.md"
New-Item -ItemType Directory -Path $badReviewerChecklistPdfFinalizeTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReviewerChecklistPdfFinalizeTracePath -Encoding UTF8 -Value @"
# Reviewer Checklist

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- [ ] Confirm the PDF visual gate finalize evidence is signed off: verdict ``pass``, summary .\output\pdf-visual-release-gate-current\report\summary.json, aggregate contact sheet, CJK manifest samples ``43``, CJK copy/search samples ``43``, missing text ``0``, visual baseline manifest samples ``42``, visual baselines ``44``.
"@

$badReviewerChecklistPdfFinalizeTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReviewerChecklistPdfFinalizeTracePath
} catch {
    $badReviewerChecklistPdfFinalizeTraceFailedAsExpected = $true
}

if (-not $badReviewerChecklistPdfFinalizeTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed REVIEWER_CHECKLIST.md with PDF visual finalize evidence missing the contact-sheet path on the finalize line."
}

$badReleaseBodyTraceDir = Join-Path $failDir "release-body-missing-project-template-source-json"
$badReleaseBodyTracePath = Join-Path $badReleaseBodyTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyTracePath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json

## Detached notes

- source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseBodyTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyTracePath
} catch {
    $badReleaseBodyTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with project-template source_json_display outside the readiness summary line."
}

$badReleaseBodyOnboardingTraceDir = Join-Path $failDir "release-body-onboarding-missing-project-template-source-json"
$badReleaseBodyOnboardingTracePath = Join-Path $badReleaseBodyOnboardingTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyOnboardingTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyOnboardingTracePath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseBodyOnboardingTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyOnboardingTracePath
} catch {
    $badReleaseBodyOnboardingTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyOnboardingTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with onboarding source_json_display supplied only by readiness."
}

$badReleaseBodyReadinessSourceIdentityDir = Join-Path $failDir "release-body-readiness-source-identity-impersonated"
$badReleaseBodyReadinessSourceIdentityPath = Join-Path $badReleaseBodyReadinessSourceIdentityDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyReadinessSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyReadinessSourceIdentityPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseBodyReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyReadinessSourceIdentityPath
} catch {
    $badReleaseBodyReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseBodyReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with readiness source displays pointing at onboarding governance evidence."
}

$badReleaseBodyInvalidReadinessStatusDir = Join-Path $failDir "release-body-invalid-project-template-readiness-status"
$badReleaseBodyInvalidReadinessStatusPath = Join-Path $badReleaseBodyInvalidReadinessStatusDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready-ish release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseBodyInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyInvalidReadinessStatusPath
} catch {
    $badReleaseBodyInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badReleaseBodyInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with an invalid project-template readiness status."
}

$badReleaseBodyInvalidReadinessReadyDir = Join-Path $failDir "release-body-invalid-project-template-readiness-release-ready"
$badReleaseBodyInvalidReadinessReadyPath = Join-Path $badReleaseBodyInvalidReadinessReadyDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=maybe latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseBodyInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyInvalidReadinessReadyPath
} catch {
    $badReleaseBodyInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badReleaseBodyInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with an invalid project-template readiness release_ready."
}

$badReleaseBodyInvalidOnboardingStatusDir = Join-Path $failDir "release-body-invalid-project-template-onboarding-status"
$badReleaseBodyInvalidOnboardingStatusPath = Join-Path $badReleaseBodyInvalidOnboardingStatusDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready-ish release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseBodyInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyInvalidOnboardingStatusPath
} catch {
    $badReleaseBodyInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badReleaseBodyInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with an invalid project-template onboarding status."
}

$badReleaseBodyInvalidOnboardingReadyDir = Join-Path $failDir "release-body-invalid-project-template-onboarding-release-ready"
$badReleaseBodyInvalidOnboardingReadyPath = Join-Path $badReleaseBodyInvalidOnboardingReadyDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=maybe schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseBodyInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyInvalidOnboardingReadyPath
} catch {
    $badReleaseBodyInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badReleaseBodyInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with an invalid project-template onboarding release_ready."
}

$badReleaseBodyOnboardingSourceIdentityDir = Join-Path $failDir "release-body-onboarding-source-identity-impersonated"
$badReleaseBodyOnboardingSourceIdentityPath = Join-Path $badReleaseBodyOnboardingSourceIdentityDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyOnboardingSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyOnboardingSourceIdentityPath -Encoding UTF8 -Value @"
# Release body

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status=ready release_ready=True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 status=ready release_ready=True schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseBodyOnboardingSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyOnboardingSourceIdentityPath
} catch {
    $badReleaseBodyOnboardingSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseBodyOnboardingSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with onboarding source_json_display pointing at delivery readiness evidence."
}

$badReleaseBodyPdfSplitTraceDir = Join-Path $failDir "release-body-split-pdf-visual-contact-sheet"
$badReleaseBodyPdfSplitTracePath = Join-Path $badReleaseBodyPdfSplitTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyPdfSplitTracePath -Encoding UTF8 -Value @"
# Release body

## Validation

- PDF visual gate summary：.\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status：loaded
- PDF visual gate verdict：pass
- PDF visual gate aggregate contact sheet：
- PDF CJK manifest samples：43
- PDF CJK copy/search samples：43
- PDF visual baseline manifest samples：42
- PDF visual baselines：44

## Detached notes

- aggregate_contact_sheet=.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReleaseBodyPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyPdfSplitTracePath
} catch {
    $badReleaseBodyPdfSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with a PDF visual contact-sheet path outside the PDF visual gate aggregate contact sheet line."
}

$badReleaseBodyPdfDetachedCountTraceDir = Join-Path $failDir "release-body-pdf-visual-count-supplied-by-detached-notes"
$badReleaseBodyPdfDetachedCountTracePath = Join-Path $badReleaseBodyPdfDetachedCountTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyPdfDetachedCountTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyPdfDetachedCountTracePath -Encoding UTF8 -Value @"
# Release body

## Validation

- PDF visual gate summary：.\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status：loaded
- PDF visual gate verdict：pass
- PDF visual gate aggregate contact sheet：.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF CJK manifest samples：43
- PDF CJK copy/search samples：43
- PDF visual baselines：44

## Detached notes

- PDF visual baseline manifest samples：42
"@

$badReleaseBodyPdfDetachedCountTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyPdfDetachedCountTracePath
} catch {
    $badReleaseBodyPdfDetachedCountTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyPdfDetachedCountTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with PDF visual counts supplied only by detached notes."
}

$badReleaseBodyPdfVerdictTraceDir = Join-Path $failDir "release-body-invalid-pdf-visual-verdict"
$badReleaseBodyPdfVerdictTracePath = Join-Path $badReleaseBodyPdfVerdictTraceDir "release_body.zh-CN.md"
New-Item -ItemType Directory -Path $badReleaseBodyPdfVerdictTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseBodyPdfVerdictTracePath -Encoding UTF8 -Value @"
# Release body

## Validation

- PDF visual gate summary：.\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status：loaded
- PDF visual gate verdict：blocked
- PDF visual gate aggregate contact sheet：.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF CJK manifest samples：43
- PDF CJK copy/search samples：43
- PDF visual baseline manifest samples：42
- PDF visual baselines：44
"@

$badReleaseBodyPdfVerdictTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseBodyPdfVerdictTracePath
} catch {
    $badReleaseBodyPdfVerdictTraceFailedAsExpected = $true
}

if (-not $badReleaseBodyPdfVerdictTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_body.zh-CN.md with a non-pass/fail PDF visual gate verdict."
}

$badEntryProjectTemplateStatusTraceDir = Join-Path $failDir "entry-project-template-readiness-missing-status-release-ready"
$badEntryProjectTemplateStatusTracePath = Join-Path $badEntryProjectTemplateStatusTraceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateStatusTraceDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateStatusTracePath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badEntryProjectTemplateStatusTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateStatusTracePath
} catch {
    $badEntryProjectTemplateStatusTraceFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateStatusTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md without project-template readiness status/release_ready in the readiness block."
}

$badEntryProjectTemplateInvalidStatusDir = Join-Path $failDir "entry-project-template-readiness-invalid-status"
$badEntryProjectTemplateInvalidStatusPath = Join-Path $badEntryProjectTemplateInvalidStatusDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateInvalidStatusDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateInvalidStatusPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready-ish release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badEntryProjectTemplateInvalidStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateInvalidStatusPath
} catch {
    $badEntryProjectTemplateInvalidStatusFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateInvalidStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with an invalid project-template readiness status."
}

$badEntryProjectTemplateInvalidReadyDir = Join-Path $failDir "entry-project-template-readiness-invalid-release-ready"
$badEntryProjectTemplateInvalidReadyPath = Join-Path $badEntryProjectTemplateInvalidReadyDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateInvalidReadyDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateInvalidReadyPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: maybe latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badEntryProjectTemplateInvalidReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateInvalidReadyPath
} catch {
    $badEntryProjectTemplateInvalidReadyFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateInvalidReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with an invalid project-template readiness release_ready."
}

$badReleaseHandoffTraceDir = Join-Path $failDir "release-handoff-missing-project-template-source-json"
$badReleaseHandoffTracePath = Join-Path $badReleaseHandoffTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffTracePath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json

## Detached notes

- project_template_delivery_readiness: source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseHandoffTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffTracePath
} catch {
    $badReleaseHandoffTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with project-template source_json_display outside the readiness list block."
}

$badReleaseHandoffReadinessSplitTraceDir = Join-Path $failDir "release-handoff-readiness-source-json-supplied-by-onboarding"
$badReleaseHandoffReadinessSplitTracePath = Join-Path $badReleaseHandoffReadinessSplitTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffReadinessSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffReadinessSplitTracePath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffReadinessSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffReadinessSplitTracePath
} catch {
    $badReleaseHandoffReadinessSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffReadinessSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with readiness source_json_display supplied only by onboarding."
}

$badReleaseHandoffOnboardingSplitTraceDir = Join-Path $failDir "release-handoff-onboarding-source-json-supplied-by-readiness"
$badReleaseHandoffOnboardingSplitTracePath = Join-Path $badReleaseHandoffOnboardingSplitTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffOnboardingSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffOnboardingSplitTracePath -Encoding UTF8 -Value @"
# Release handoff

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
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json

## Detached notes

- project_template_onboarding.schema_approval source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffOnboardingSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffOnboardingSplitTracePath
} catch {
    $badReleaseHandoffOnboardingSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffOnboardingSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with onboarding source_json_display outside the onboarding list block."
}

$badReleaseHandoffReadinessSourceIdentityDir = Join-Path $failDir "release-handoff-readiness-source-identity-impersonated"
$badReleaseHandoffReadinessSourceIdentityPath = Join-Path $badReleaseHandoffReadinessSourceIdentityDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffReadinessSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffReadinessSourceIdentityPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffReadinessSourceIdentityPath
} catch {
    $badReleaseHandoffReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseHandoffReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with readiness source displays pointing at onboarding governance evidence."
}

$badReleaseHandoffOnboardingSourceJsonIdentityDir = Join-Path $failDir "release-handoff-onboarding-source-json-identity-impersonated"
$badReleaseHandoffOnboardingSourceJsonIdentityPath = Join-Path $badReleaseHandoffOnboardingSourceJsonIdentityDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffOnboardingSourceJsonIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffOnboardingSourceJsonIdentityPath -Encoding UTF8 -Value @"
# Release handoff

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
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseHandoffOnboardingSourceJsonIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffOnboardingSourceJsonIdentityPath
} catch {
    $badReleaseHandoffOnboardingSourceJsonIdentityFailedAsExpected = $true
}

if (-not $badReleaseHandoffOnboardingSourceJsonIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with onboarding source_json_display pointing at delivery readiness evidence."
}

$badReleaseHandoffInvalidOnboardingStatusDir = Join-Path $failDir "release-handoff-invalid-project-template-onboarding-status"
$badReleaseHandoffInvalidOnboardingStatusPath = Join-Path $badReleaseHandoffInvalidOnboardingStatusDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready-ish
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffInvalidOnboardingStatusPath
} catch {
    $badReleaseHandoffInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badReleaseHandoffInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with an invalid nested onboarding status value."
}

$badReleaseHandoffInvalidOnboardingReadyDir = Join-Path $failDir "release-handoff-invalid-project-template-onboarding-release-ready"
$badReleaseHandoffInvalidOnboardingReadyPath = Join-Path $badReleaseHandoffInvalidOnboardingReadyDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: maybe
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffInvalidOnboardingReadyPath
} catch {
    $badReleaseHandoffInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badReleaseHandoffInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with an invalid nested onboarding release_ready value."
}

$badReleaseHandoffInvalidReadinessStatusDir = Join-Path $failDir "release-handoff-invalid-project-template-readiness-status"
$badReleaseHandoffInvalidReadinessStatusPath = Join-Path $badReleaseHandoffInvalidReadinessStatusDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready-ish ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready-ish
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseHandoffInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffInvalidReadinessStatusPath
} catch {
    $badReleaseHandoffInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badReleaseHandoffInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with an invalid project-template readiness status value."
}

$badReleaseHandoffInvalidReadinessReadyDir = Join-Path $failDir "release-handoff-invalid-project-template-readiness-release-ready"
$badReleaseHandoffInvalidReadinessReadyPath = Join-Path $badReleaseHandoffInvalidReadinessReadyDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: maybe
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseHandoffInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffInvalidReadinessReadyPath
} catch {
    $badReleaseHandoffInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badReleaseHandoffInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with an invalid project-template readiness release_ready value."
}

$badReleaseHandoffPdfSplitTraceDir = Join-Path $failDir "release-handoff-split-pdf-visual-contact-sheet"
$badReleaseHandoffPdfSplitTracePath = Join-Path $badReleaseHandoffPdfSplitTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffPdfSplitTracePath -Encoding UTF8 -Value @"
# Release handoff

## PDF visual gate evidence

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: ready
- PDF visual gate verdict: pass
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet:

## Detached notes

- archived_contact_sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReleaseHandoffPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffPdfSplitTracePath
} catch {
    $badReleaseHandoffPdfSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with a PDF visual gate contact-sheet path outside the contact sheet line."
}

$badReleaseHandoffPdfDetachedCountTraceDir = Join-Path $failDir "release-handoff-pdf-visual-count-supplied-by-detached-notes"
$badReleaseHandoffPdfDetachedCountTracePath = Join-Path $badReleaseHandoffPdfDetachedCountTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffPdfDetachedCountTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffPdfDetachedCountTracePath -Encoding UTF8 -Value @"
# Release handoff

## PDF visual gate evidence

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: ready
- PDF visual gate verdict: pass
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png

## Detached notes

- PDF visual baseline manifest samples: 42
"@

$badReleaseHandoffPdfDetachedCountTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffPdfDetachedCountTracePath
} catch {
    $badReleaseHandoffPdfDetachedCountTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffPdfDetachedCountTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with PDF visual counts supplied only by detached notes."
}

$badReleaseHandoffPdfVerdictTraceDir = Join-Path $failDir "release-handoff-invalid-pdf-visual-verdict"
$badReleaseHandoffPdfVerdictTracePath = Join-Path $badReleaseHandoffPdfVerdictTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffPdfVerdictTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffPdfVerdictTracePath -Encoding UTF8 -Value @"
# Release handoff

## PDF visual gate evidence

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: ready
- PDF visual gate verdict: blocked
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReleaseHandoffPdfVerdictTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffPdfVerdictTracePath
} catch {
    $badReleaseHandoffPdfVerdictTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffPdfVerdictTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with a non-pass/fail PDF visual gate verdict."
}

$badReleaseGovernanceHandoffTraceDir = Join-Path $failDir "release-governance-handoff-missing-project-template-source-json"
$badReleaseGovernanceHandoffTracePath = Join-Path $badReleaseGovernanceHandoffTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``

## Detached notes

- source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
"@

$badReleaseGovernanceHandoffTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffTracePath
} catch {
    $badReleaseGovernanceHandoffTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with project-template source_json_display outside the readiness report-status block."
}

$badReleaseGovernanceHandoffStatusTraceDir = Join-Path $failDir "release-governance-handoff-missing-project-template-status-ready"
$badReleaseGovernanceHandoffStatusTracePath = Join-Path $badReleaseGovernanceHandoffStatusTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffStatusTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffStatusTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

$badReleaseGovernanceHandoffStatusTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffStatusTracePath
} catch {
    $badReleaseGovernanceHandoffStatusTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffStatusTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md without project-template status/ready in the readiness report-status block."
}

$badReleaseGovernanceHandoffInvalidReadinessStatusDir = Join-Path $failDir "release-governance-handoff-invalid-project-template-readiness-status"
$badReleaseGovernanceHandoffInvalidReadinessStatusPath = Join-Path $badReleaseGovernanceHandoffInvalidReadinessStatusDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready-ish`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

$badReleaseGovernanceHandoffInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffInvalidReadinessStatusPath
} catch {
    $badReleaseGovernanceHandoffInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with an invalid project-template readiness status value."
}

$badReleaseGovernanceHandoffInvalidReadinessReadyDir = Join-Path $failDir "release-governance-handoff-invalid-project-template-readiness-ready"
$badReleaseGovernanceHandoffInvalidReadinessReadyPath = Join-Path $badReleaseGovernanceHandoffInvalidReadinessReadyDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``maybe`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

$badReleaseGovernanceHandoffInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffInvalidReadinessReadyPath
} catch {
    $badReleaseGovernanceHandoffInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with an invalid project-template readiness ready value."
}

$badReleaseGovernanceHandoffOnboardingTraceDir = Join-Path $failDir "release-governance-handoff-missing-project-template-onboarding-source-json"
$badReleaseGovernanceHandoffOnboardingTracePath = Join-Path $badReleaseGovernanceHandoffOnboardingTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffOnboardingTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffOnboardingTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json

## Detached notes

- project_template_onboarding.schema_approval source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseGovernanceHandoffOnboardingTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffOnboardingTracePath
} catch {
    $badReleaseGovernanceHandoffOnboardingTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffOnboardingTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with project-template onboarding source_json_display outside the onboarding blocker block."
}

$badReleaseGovernanceHandoffReadinessSourceIdentityDir = Join-Path $failDir "release-governance-handoff-readiness-source-identity-impersonated"
$badReleaseGovernanceHandoffReadinessSourceIdentityPath = Join-Path $badReleaseGovernanceHandoffReadinessSourceIdentityDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffReadinessSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffReadinessSourceIdentityPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

$badReleaseGovernanceHandoffReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffReadinessSourceIdentityPath
} catch {
    $badReleaseGovernanceHandoffReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with readiness source displays pointing at onboarding governance evidence."
}

$badReleaseGovernanceHandoffOnboardingSourceJsonIdentityDir = Join-Path $failDir "release-governance-handoff-onboarding-source-json-identity-impersonated"
$badReleaseGovernanceHandoffOnboardingSourceJsonIdentityPath = Join-Path $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseGovernanceHandoffOnboardingSourceJsonIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityPath
} catch {
    $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with onboarding source_json_display pointing at delivery readiness evidence."
}

$badReleaseGovernanceHandoffInvalidOnboardingStatusDir = Join-Path $failDir "release-governance-handoff-invalid-project-template-onboarding-status"
$badReleaseGovernanceHandoffInvalidOnboardingStatusPath = Join-Path $badReleaseGovernanceHandoffInvalidOnboardingStatusDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready-ish
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseGovernanceHandoffInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffInvalidOnboardingStatusPath
} catch {
    $badReleaseGovernanceHandoffInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with an invalid nested onboarding status value."
}

$badReleaseGovernanceHandoffInvalidOnboardingReadyDir = Join-Path $failDir "release-governance-handoff-invalid-project-template-onboarding-release-ready"
$badReleaseGovernanceHandoffInvalidOnboardingReadyPath = Join-Path $badReleaseGovernanceHandoffInvalidOnboardingReadyDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: maybe
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseGovernanceHandoffInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffInvalidOnboardingReadyPath
} catch {
    $badReleaseGovernanceHandoffInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with an invalid nested onboarding release_ready value."
}

$badReleaseGovernanceHandoffPdfTraceDir = Join-Path $failDir "release-governance-handoff-missing-pdf-visual-contact-sheet"
$badReleaseGovernanceHandoffPdfTracePath = Join-Path $badReleaseGovernanceHandoffPdfTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
"@

$badReleaseGovernanceHandoffPdfTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfTracePath
} catch {
    $badReleaseGovernanceHandoffPdfTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md without PDF visual gate contact-sheet trace."
}

$badReleaseGovernanceHandoffPdfSplitTraceDir = Join-Path $failDir "release-governance-handoff-split-pdf-visual-contact-sheet"
$badReleaseGovernanceHandoffPdfSplitTracePath = Join-Path $badReleaseGovernanceHandoffPdfSplitTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfSplitTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ````
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``

## Detached notes

- archived_contact_sheet: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
"@

$badReleaseGovernanceHandoffPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfSplitTracePath
} catch {
    $badReleaseGovernanceHandoffPdfSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with a PDF visual gate contact-sheet path outside the source_report block."
}

$badReleaseGovernanceHandoffPdfManifestTraceDir = Join-Path $failDir "release-governance-handoff-missing-pdf-visual-manifest-count"
$badReleaseGovernanceHandoffPdfManifestTracePath = Join-Path $badReleaseGovernanceHandoffPdfManifestTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfManifestTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfManifestTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_count: ``44``
"@

$badReleaseGovernanceHandoffPdfManifestTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfManifestTracePath
} catch {
    $badReleaseGovernanceHandoffPdfManifestTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfManifestTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md without PDF visual gate manifest-count trace."
}

$badFinalReviewStatusTraceDir = Join-Path $failDir "final-review-missing-project-template-readiness-status"
$badFinalReviewStatusTracePath = Join-Path $badFinalReviewStatusTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewStatusTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewStatusTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewStatusTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewStatusTracePath
} catch {
    $badFinalReviewStatusTraceFailedAsExpected = $true
}

if (-not $badFinalReviewStatusTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md without project-template readiness_status/readiness_release_ready in the handoff blocker block."
}

$badFinalReviewInvalidReadinessStatusDir = Join-Path $failDir "final-review-invalid-project-template-readiness-status"
$badFinalReviewInvalidReadinessStatusPath = Join-Path $badFinalReviewInvalidReadinessStatusDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready-ish
  - readiness_release_ready: True
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewInvalidReadinessStatusPath
} catch {
    $badFinalReviewInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badFinalReviewInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with an invalid project-template readiness_status value."
}

$badFinalReviewInvalidReadinessReadyDir = Join-Path $failDir "final-review-invalid-project-template-readiness-release-ready"
$badFinalReviewInvalidReadinessReadyPath = Join-Path $badFinalReviewInvalidReadinessReadyDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready
  - readiness_release_ready: maybe
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewInvalidReadinessReadyPath
} catch {
    $badFinalReviewInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badFinalReviewInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with an invalid project-template readiness_release_ready value."
}

$badFinalReviewTraceDir = Join-Path $failDir "final-review-missing-project-template-source-json"
$badFinalReviewTracePath = Join-Path $badFinalReviewTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
"@

$badFinalReviewTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewTracePath
} catch {
    $badFinalReviewTraceFailedAsExpected = $true
}

if (-not $badFinalReviewTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md without project-template source_json_display."
}

$badFinalReviewSplitTraceDir = Join-Path $failDir "final-review-project-template-source-json-supplied-by-detached-notes"
$badFinalReviewSplitTracePath = Join-Path $badFinalReviewSplitTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewSplitTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json

## Detached notes

- project_template_delivery_readiness / project_template_onboarding.schema_approval: detached source_json_display: .\output\project-template-onboarding-governance\summary.json project-template-onboarding-governance
"@

$badFinalReviewSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewSplitTracePath
} catch {
    $badFinalReviewSplitTraceFailedAsExpected = $true
}

if (-not $badFinalReviewSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with project-template source_json_display supplied only by repeated-anchor detached notes."
}

$badFinalReviewSourceJsonIdentityDir = Join-Path $failDir "final-review-project-template-source-json-identity-impersonated"
$badFinalReviewSourceJsonIdentityPath = Join-Path $badFinalReviewSourceJsonIdentityDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewSourceJsonIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewSourceJsonIdentityPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
"@

$badFinalReviewSourceJsonIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewSourceJsonIdentityPath
} catch {
    $badFinalReviewSourceJsonIdentityFailedAsExpected = $true
}

if (-not $badFinalReviewSourceJsonIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with project-template source_json_display pointing at delivery readiness evidence."
}

$badFinalReviewInvalidOnboardingStatusDir = Join-Path $failDir "final-review-invalid-project-template-onboarding-status"
$badFinalReviewInvalidOnboardingStatusPath = Join-Path $badFinalReviewInvalidOnboardingStatusDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready
  - readiness_release_ready: True
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready-ish
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewInvalidOnboardingStatusPath
} catch {
    $badFinalReviewInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badFinalReviewInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with an invalid nested onboarding status value."
}

$badFinalReviewInvalidOnboardingReadyDir = Join-Path $failDir "final-review-invalid-project-template-onboarding-release-ready"
$badFinalReviewInvalidOnboardingReadyPath = Join-Path $badFinalReviewInvalidOnboardingReadyDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release Candidate Checks

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
    - release_ready: maybe
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewInvalidOnboardingReadyPath
} catch {
    $badFinalReviewInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badFinalReviewInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with an invalid nested onboarding release_ready value."
}

$badFinalReviewPdfTraceDir = Join-Path $failDir "final-review-missing-pdf-visual-contact-sheet"
$badFinalReviewPdfTracePath = Join-Path $badFinalReviewPdfTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
"@

$badFinalReviewPdfTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfTracePath
} catch {
    $badFinalReviewPdfTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md without PDF visual gate contact-sheet trace."
}

$badFinalReviewPdfSplitTraceDir = Join-Path $failDir "final-review-split-pdf-visual-contact-sheet"
$badFinalReviewPdfSplitTracePath = Join-Path $badFinalReviewPdfSplitTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfSplitTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet:

## Detached notes

- archived_contact_sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badFinalReviewPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfSplitTracePath
} catch {
    $badFinalReviewPdfSplitTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with a PDF visual gate contact-sheet path outside the contact sheet line."
}

$badFinalReviewPdfDetachedStepTraceDir = Join-Path $failDir "final-review-pdf-visual-step-status-supplied-by-detached-notes"
$badFinalReviewPdfDetachedStepTracePath = Join-Path $badFinalReviewPdfDetachedStepTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfDetachedStepTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfDetachedStepTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png

## Detached notes

- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
"@

$badFinalReviewPdfDetachedStepTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfDetachedStepTracePath
} catch {
    $badFinalReviewPdfDetachedStepTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfDetachedStepTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with PDF visual step-status markers supplied only by detached notes."
}

$badFinalReviewPdfDetachedOutputTraceDir = Join-Path $failDir "final-review-pdf-visual-key-outputs-supplied-by-detached-notes"
$badFinalReviewPdfDetachedOutputTracePath = Join-Path $badFinalReviewPdfDetachedOutputTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfDetachedOutputTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfDetachedOutputTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary:
- PDF visual gate contact sheet:

## Detached notes

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badFinalReviewPdfDetachedOutputTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfDetachedOutputTracePath
} catch {
    $badFinalReviewPdfDetachedOutputTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfDetachedOutputTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with PDF visual Key outputs evidence supplied only by detached notes."
}

$badFinalReviewPdfVerdictTraceDir = Join-Path $failDir "final-review-invalid-pdf-visual-verdict"
$badFinalReviewPdfVerdictTracePath = Join-Path $badFinalReviewPdfVerdictTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfVerdictTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfVerdictTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: blocked
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badFinalReviewPdfVerdictTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfVerdictTracePath
} catch {
    $badFinalReviewPdfVerdictTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfVerdictTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with a non-pass/fail PDF visual gate verdict."
}

$badEntryGovernanceTracePath = Join-Path $failDir "ARTIFACT_GUIDE.md"
Set-Content -LiteralPath $badEntryGovernanceTracePath -Encoding UTF8 -Value @"
# Artifact Guide

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Project template readiness: project_template_delivery_readiness release_ready
- Project template onboarding: project_template_onboarding.schema_approval approved
- Governance metric: delivery_quality release_ready 100
"@

$badEntryGovernanceTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryGovernanceTracePath
} catch {
    $badEntryGovernanceTraceFailedAsExpected = $true
}

if (-not $badEntryGovernanceTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release entry document without table_layout_delivery_governance.delivery_quality."
}

Write-Host "Release material safety audit regression passed."


