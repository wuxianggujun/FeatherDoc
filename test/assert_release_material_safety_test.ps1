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
    aggregate_contact_sheet = ".\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png"
    cjk_copy_search_count = "43"
    cjk_missing_text_count = "0"
    visual_baseline_count = "44"
    pdf_cli_export_log = ".\output\pdf-visual-release-gate-current\report\pdf-cli-export-test.log"
    pdf_regression_log = ".\output\pdf-visual-release-gate-current\report\pdf-regression-test.log"
    cjk_copy_search_log_dir = ".\output\pdf-visual-release-gate-current\report\cjk-copy-search"
    unicode_font_log = ".\output\pdf-visual-release-gate-current\report\unicode-font.log"
    error = ""
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
($passManifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $passManifestPath -Encoding UTF8

& $auditScript -Path @($passSummaryPath, $passManifestPath)

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
- Content-control contract: source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control repair_hint=Rerun Custom XML sync or explicitly fill the bound content control before release. command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
- Numbering real corpus confidence: numbering_catalog_governance.real_corpus_confidence level=low score=56 catalog_coverage_percent=100 baseline_coverage_percent=100 coverage_score=100 matched_document_count=2 unmatched_catalog_document_count=0 unmatched_baseline_document_count=0 alignment_gap_count=0 catalog_document_keys=contract.docx,invoice.docx baseline_document_keys=contract.docx,invoice.docx matched_document_keys=contract.docx,invoice.docx penalty_summary=style_numbering_issues(count=4, penalty=20)
- Table layout delivery: table_layout_delivery_governance.delivery_quality release_ready table_style_issue_count=0 automatic_tblLook_fix_count=0 manual_table_style_fix_count=0 table_position_automatic_count=0 table_position_review_count=0 command_failure_count=0 ready_document_percent=100 unresolved_item_count=0 penalty_summary=floating_table_plans_pending(count=0, penalty=0)
"@

& $auditScript -Path $passEntryGovernanceTracePath

$badEntryMissingGovernanceMetricDetailsDir = Join-Path $failDir "entry-missing-governance-metric-details"
$badEntryMissingGovernanceMetricDetailsPath = Join-Path $badEntryMissingGovernanceMetricDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingGovernanceMetricDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingGovernanceMetricDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder source_schema=featherdoc.content_control_data_binding_governance_report.v1 source_json_display=.\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json repair_strategy=sync_bound_content_control command_template=featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
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

$badEntryMissingRepairDetailsDir = Join-Path $failDir "entry-missing-content-control-repair-details"
$badEntryMissingRepairDetailsPath = Join-Path $badEntryMissingRepairDetailsDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryMissingRepairDetailsDir -Force | Out-Null
Set-Content -LiteralPath $badEntryMissingRepairDetailsPath -Encoding UTF8 -Value @"
# START_HERE

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
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


