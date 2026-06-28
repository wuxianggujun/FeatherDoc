param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

. (Join-Path $resolvedRepoRoot "scripts\release_blocker_metadata_helpers.ps1")

$registeredActions = @(Get-ReleaseBlockerRegisteredActions)
Assert-True -Condition ($registeredActions.Count -gt 0) `
    -Message "Release blocker action registry should not be empty."

$uniqueActions = @($registeredActions | Sort-Object -Unique)
Assert-True -Condition ($uniqueActions.Count -eq $registeredActions.Count) `
    -Message "Release blocker action registry should not contain duplicate actions."

$ordinalIgnoreCaseActions = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
foreach ($action in $registeredActions) {
    Assert-True -Condition (-not [string]::IsNullOrWhiteSpace($action)) `
        -Message "Release blocker action registry should not contain blank actions."
    Assert-True -Condition ($action -eq $action.Trim()) `
        -Message "Release blocker action registry should not contain leading or trailing whitespace: '$action'"
    Assert-True -Condition ($action -match '^[A-Za-z0-9_]+$') `
        -Message "Release blocker action registry should use stable ASCII token characters only: $action"
    Assert-True -Condition ($ordinalIgnoreCaseActions.Add($action)) `
        -Message "Release blocker action registry should not contain case-insensitive duplicate actions: $action"
    Assert-True -Condition (Test-ReleaseBlockerActionRegistered -Action $action) `
        -Message "Registered release blocker action should resolve as registered: $action"

    $blocker = [pscustomobject]@{
        id = "test.$action"
        source = "release_blocker_action_registry_test"
        severity = "warning"
        status = "blocked"
        action = $action
        summary_json = Join-Path $resolvedWorkingDir "$action.summary.json"
        history_markdown = Join-Path $resolvedWorkingDir "$action.history.md"
    }

    $guidanceLines = @(Get-ReleaseBlockerActionGuidanceLines `
            -Blocker $blocker `
            -RepoRoot $resolvedRepoRoot `
            -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json"))
    Assert-True -Condition ($guidanceLines.Count -gt 0) `
        -Message "Registered release blocker action should produce a checklist runbook: $action"

    $guidanceText = $guidanceLines -join "`n"
    Assert-ContainsText -Text $guidanceText -ExpectedText $action `
        -Message "Registered release blocker action runbook should mention its action token: $action"
    Assert-True -Condition ($guidanceText -notmatch [regex]::Escape("Unregistered release blocker action")) `
        -Message "Registered release blocker action should not use the unregistered fallback: $action"
    Assert-True -Condition ($guidanceText -notmatch [regex]::Escape("does not have a checklist runbook yet")) `
        -Message "Registered release blocker action should not use the missing-runbook fallback: $action"
}

$canonicalRegisteredAction = $registeredActions[0]
$caseVariantAction = $canonicalRegisteredAction.ToUpperInvariant()
Assert-True -Condition (Test-ReleaseBlockerActionRegistered -Action $caseVariantAction) `
    -Message "Release blocker action registry lookup should be case-insensitive for registered actions."

$paddedRegisteredAction = " $canonicalRegisteredAction "
Assert-True -Condition (-not (Test-ReleaseBlockerActionRegistered -Action $paddedRegisteredAction)) `
    -Message "Release blocker action registry lookup should not trim malformed action tokens."

$paddedGuidance = @(Get-ReleaseBlockerActionGuidanceLines -Blocker ([pscustomobject]@{ action = $paddedRegisteredAction })) -join "`n"
Assert-ContainsText -Text $paddedGuidance -ExpectedText ("Unregistered release blocker action ``{0}``" -f $paddedRegisteredAction) `
    -Message "Malformed registered-looking action should render the unregistered fallback instead of being normalized."

$unknownAction = "custom_unregistered_action"
Assert-True -Condition (-not (Test-ReleaseBlockerActionRegistered -Action $unknownAction)) `
    -Message "Unknown release blocker action should not resolve as registered."

$unknownGuidance = @(Get-ReleaseBlockerActionGuidanceLines -Blocker ([pscustomobject]@{ action = $unknownAction })) -join "`n"
Assert-ContainsText -Text $unknownGuidance -ExpectedText ("Unregistered release blocker action ``{0}``" -f $unknownAction) `
    -Message "Unknown release blocker action should render the unregistered runbook fallback."

$pdfPreflightBlocker = [pscustomobject]@{
    id = "pdf_visual_release_gate_preflight.build_outputs_missing"
    source = "pdf_visual_release_gate_preflight"
    severity = "error"
    status = "blocked"
    action = "prepare_pdf_visual_release_gate_build_outputs"
    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
    source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
    command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly"
    issue_keys = @("cmake_cache_exists", "ctest_manifest_exists", "pdf_build_options_enabled")
    output_gap_count = 3
    missing_output_count = 87
    pdf_dependency_inputs = [pscustomobject]@{
        status = "not_ready"
        selected_pdfium_provider = "unresolved"
        pdfio_ready = $false
        pdfium_ready = $false
        missing_input_count = 3
        missing_inputs = @(
            "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h",
            "PDFium source header: C:\repo\tmp\pdfium-workspace\pdfium\public\fpdfview.h",
            "PDFium input: provide prebuilt library/include inputs or a source checkout with public/fpdfview.h."
        )
    }
    readiness_action_evidence = @(
        [pscustomobject]@{
            action = "provide_pdf_dependency_input"
            issue_key = "pdf_dependency_inputs_ready"
            item = "PDFio source header"
            source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
        },
        [pscustomobject]@{
            action = "enable_pdf_build_option"
            issue_key = "pdf_build_options_enabled"
            item = "FEATHERDOC_BUILD_PDF_IMPORT"
            source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
        }
    )
    build_dir_auto_candidates = @(
        [pscustomobject]@{
            relative_path = "build"
            exists = $true
            cmake_cache_exists = $true
            ctest_manifest_exists = $true
            pdf_build_options_enabled = $false
            looks_reusable = $false
            pdf_build_options = @(
                [pscustomobject]@{
                    name = "FEATHERDOC_BUILD_PDF"
                    present = $true
                    value = "OFF"
                    enabled = $false
                },
                [pscustomobject]@{
                    name = "FEATHERDOC_BUILD_PDF_IMPORT"
                    present = $true
                    value = "OFF"
                    enabled = $false
                }
            )
        },
        [pscustomobject]@{
            relative_path = "out\build"
            exists = $false
            cmake_cache_exists = $false
            ctest_manifest_exists = $false
            pdf_build_options_enabled = $false
            looks_reusable = $false
            pdf_build_options = @(
                [pscustomobject]@{
                    name = "FEATHERDOC_BUILD_PDF"
                    present = $false
                    value = ""
                    enabled = $false
                },
                [pscustomobject]@{
                    name = "FEATHERDOC_BUILD_PDF_IMPORT"
                    present = $false
                    value = ""
                    enabled = $false
                }
            )
        }
    )
    blocking_summary = [pscustomobject]@{
        required_check_count = 12
        blocking_check_count = 7
        missing_cli_pdf_count = 2
        visual_baseline_sample_count = 42
        missing_visual_baseline_pdf_count = 42
        cjk_text_layer_sample_count = 43
        missing_cjk_text_layer_pdf_count = 43
        build_dir_entry_count = 1
        ctest_required_pattern_count = 4
        memory_guard_blocked = $false
        memory_guard_skipped = $false
        free_memory_mb = 1140
        min_free_memory_mb = 2048
        pdf_build_options_enabled = $false
        disabled_pdf_build_options = @("FEATHERDOC_BUILD_PDF", "FEATHERDOC_BUILD_PDF_IMPORT")
        missing_pdf_build_options = @()
        pdf_dependency_inputs_status = "not_ready"
        selected_pdfium_provider = "unresolved"
        pdfio_dependency_ready = $false
        pdfium_dependency_ready = $false
        pdf_dependency_missing_input_count = 3
    }
}
$pdfPreflightGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $pdfPreflightBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "prepare_pdf_visual_release_gate_build_outputs" `
    -Message "PDF preflight build-output blocker should render its fixed action runbook."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "run_pdf_visual_release_gate.ps1" `
    -Message "PDF preflight build-output blocker should point at the PDF visual gate wrapper."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "-PreflightOnly" `
    -Message "PDF preflight build-output blocker should keep the first reviewer step lightweight."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "not release-ready evidence" `
    -Message "PDF preflight build-output blocker should state that preflight-only is not release-ready evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "cmake_cache_exists" `
    -Message "PDF preflight build-output blocker should explain the missing CMakeCache preflight issue."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "CMakeCache.txt" `
    -Message "PDF preflight build-output blocker should require a reusable CMake build directory."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "pdf_build_options_enabled" `
    -Message "PDF preflight build-output blocker should explain disabled PDF build options."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "-DFEATHERDOC_BUILD_PDF=ON" `
    -Message "PDF preflight build-output blocker should include the PDF writer build option."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "-DFEATHERDOC_BUILD_PDF_IMPORT=ON" `
    -Message "PDF preflight build-output blocker should include the PDF import build option."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing CLI PDFs=2" `
    -Message "PDF preflight build-output blocker should include the missing CLI PDF count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing visual baseline PDFs=42" `
    -Message "PDF preflight build-output blocker should include the missing visual baseline count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing CJK text-layer PDFs=43" `
    -Message "PDF preflight build-output blocker should include the missing CJK text-layer count."

$schemaCalibrationBlocker = [pscustomobject]@{
    id = "schema_patch_confidence_calibration.pending_schema_approvals"
    source = "schema_patch_confidence_calibration"
    severity = "error"
    status = "blocked"
    action = "resolve_pending_schema_approvals"
    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
    source_report_display = ".\output\schema-patch-confidence-calibration\schema_patch_confidence_calibration.md"
    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
    command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1 -ReportRoot .\output\schema-patch-confidence-calibration"
}
$schemaCalibrationGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $schemaCalibrationBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $schemaCalibrationGuidance -ExpectedText "resolve_pending_schema_approvals" `
    -Message "Schema patch confidence calibration blocker should render its fixed action runbook."
Assert-ContainsText -Text $schemaCalibrationGuidance -ExpectedText "schema patch approval outcome" `
    -Message "Schema patch confidence calibration runbook should explain pending approval remediation."
Assert-ContainsText -Text $schemaCalibrationGuidance -ExpectedText ".\output\schema-patch-confidence-calibration\schema_patch_confidence_calibration.md" `
    -Message "Schema patch confidence calibration runbook should point at the report."
Assert-ContainsText -Text $schemaCalibrationGuidance -ExpectedText ".\output\schema-patch-confidence-calibration\summary.json" `
    -Message "Schema patch confidence calibration runbook should point at source JSON evidence."
Assert-ContainsText -Text $schemaCalibrationGuidance -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
    -Message "Schema patch confidence calibration runbook should point at the calibration writer."
Assert-ContainsText -Text $schemaCalibrationGuidance -ExpectedText "regenerate the release note bundle" `
    -Message "Schema patch confidence calibration runbook should tell reviewers to refresh release materials."

$schemaCalibrationSourceMetadataWarning = [pscustomobject]@{
    id = "schema_patch_confidence_calibration.missing_business_template_source_metadata"
    source = "schema_patch_confidence_calibration"
    severity = "warning"
    status = "ready"
    action = "add_business_template_source_metadata"
    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
    source_report_display = ".\output\schema-patch-confidence-calibration\schema_patch_confidence_calibration.md"
    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
    open_command = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
}
$schemaCalibrationSourceMetadataGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $schemaCalibrationSourceMetadataWarning `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $schemaCalibrationSourceMetadataGuidance -ExpectedText "add_business_template_source_metadata" `
    -Message "Schema patch confidence calibration source metadata warning should render its fixed action runbook."
Assert-ContainsText -Text $schemaCalibrationSourceMetadataGuidance -ExpectedText "project_id, template_name, and source summary metadata" `
    -Message "Schema patch confidence calibration source metadata runbook should explain missing source identity remediation."
Assert-ContainsText -Text $schemaCalibrationSourceMetadataGuidance -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
    -Message "Schema patch confidence calibration source metadata runbook should point at the calibration writer."

$schemaCalibrationDocumentTypeWarning = [pscustomobject]@{
    id = "schema_patch_confidence_calibration.missing_business_document_type_metadata"
    source = "schema_patch_confidence_calibration"
    severity = "warning"
    status = "ready"
    action = "add_business_template_document_type_metadata"
    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
    source_report_display = ".\output\schema-patch-confidence-calibration\schema_patch_confidence_calibration.md"
    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
    open_command = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
}
$schemaCalibrationDocumentTypeGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $schemaCalibrationDocumentTypeWarning `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $schemaCalibrationDocumentTypeGuidance -ExpectedText "add_business_template_document_type_metadata" `
    -Message "Schema patch confidence calibration document type warning should render its fixed action runbook."
Assert-ContainsText -Text $schemaCalibrationDocumentTypeGuidance -ExpectedText "business_document_type metadata" `
    -Message "Schema patch confidence calibration document type runbook should explain missing business document type remediation."
Assert-ContainsText -Text $schemaCalibrationDocumentTypeGuidance -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
    -Message "Schema patch confidence calibration document type runbook should point at the calibration writer."

$schemaCalibrationCorpusRoleWarning = [pscustomobject]@{
    id = "schema_patch_confidence_calibration.missing_business_template_corpus_role_metadata"
    source = "schema_patch_confidence_calibration"
    severity = "warning"
    status = "ready"
    action = "add_business_template_corpus_role_metadata"
    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
    source_report_display = ".\output\schema-patch-confidence-calibration\schema_patch_confidence_calibration.md"
    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
    open_command = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
}
$schemaCalibrationCorpusRoleGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $schemaCalibrationCorpusRoleWarning `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $schemaCalibrationCorpusRoleGuidance -ExpectedText "add_business_template_corpus_role_metadata" `
    -Message "Schema patch confidence calibration corpus role warning should render its fixed action runbook."
Assert-ContainsText -Text $schemaCalibrationCorpusRoleGuidance -ExpectedText "corpus_role metadata" `
    -Message "Schema patch confidence calibration corpus role runbook should explain missing corpus role remediation."
Assert-ContainsText -Text $schemaCalibrationCorpusRoleGuidance -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
    -Message "Schema patch confidence calibration corpus role runbook should point at the calibration writer."

$schemaCalibrationCorpusMismatchWarning = [pscustomobject]@{
    id = "schema_patch_confidence_calibration.mismatched_business_template_corpus_metadata"
    source = "schema_patch_confidence_calibration"
    severity = "warning"
    status = "ready"
    action = "align_business_template_corpus_metadata"
    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
    source_report_display = ".\output\schema-patch-confidence-calibration\schema_patch_confidence_calibration.md"
    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
    open_command = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
}
$schemaCalibrationCorpusMismatchGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $schemaCalibrationCorpusMismatchWarning `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $schemaCalibrationCorpusMismatchGuidance -ExpectedText "align_business_template_corpus_metadata" `
    -Message "Schema patch confidence calibration corpus metadata mismatch warning should render its fixed action runbook."
Assert-ContainsText -Text $schemaCalibrationCorpusMismatchGuidance -ExpectedText "source corpus entry" `
    -Message "Schema patch confidence calibration corpus metadata mismatch runbook should explain source entry alignment."
Assert-ContainsText -Text $schemaCalibrationCorpusMismatchGuidance -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
    -Message "Schema patch confidence calibration corpus metadata mismatch runbook should point at the calibration writer."

$contentControlBlocker = [pscustomobject]@{
    id = "content_control_data_binding.bound_placeholder"
    source = "content_control_data_binding_governance"
    severity = "error"
    status = "placeholder_visible"
    action = "sync_or_fill_bound_content_control"
    message = "A data-bound content control is still showing placeholder text."
    source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
    source_report_display = ".\output\content-control-data-binding-governance\report\summary.json"
    source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
    command_template = "featherdoc_cli sync-content-controls-from-custom-xml .\samples\invoice.docx --output .\output\invoice.synced.docx --json"
    repair_strategy = "sync_bound_content_control"
    repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
    input_docx = "samples/invoice.docx"
    input_docx_display = ".\samples\invoice.docx"
    template_name = "invoice-template"
    schema_target = "invoice"
    target_mode = "resolved-section-targets"
    part_entry_name = "word/document.xml"
    content_control_index = 2
    tag = "invoice.dueDate"
    alias = "Due Date"
    store_item_id = "{55555555-5555-5555-5555-555555555555}"
    xpath = "/invoice/dueDate"
}
$contentControlGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $contentControlBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "sync_or_fill_bound_content_control" `
    -Message "Content-control data-binding blocker should render its fixed action runbook."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "placeholder text" `
    -Message "Content-control data-binding runbook should explain placeholder remediation."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText ".\output\content-control-data-binding-governance\report\summary.json" `
    -Message "Content-control data-binding runbook should point at the governance report."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText ".\output\content-control-data-binding\inspect-content-controls.json" `
    -Message "Content-control data-binding runbook should point at source JSON evidence."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "input_docx_display=.\samples\invoice.docx" `
    -Message "Content-control data-binding runbook should keep input DOCX provenance."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "template_name=invoice-template" `
    -Message "Content-control data-binding runbook should keep template provenance."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "schema_target=invoice" `
    -Message "Content-control data-binding runbook should keep schema target provenance."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "tag=invoice.dueDate" `
    -Message "Content-control data-binding runbook should identify the content-control target."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "sync-content-controls-from-custom-xml" `
    -Message "Content-control data-binding runbook should point at the sync command."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "build_content_control_data_binding_governance_report.ps1" `
    -Message "Content-control data-binding runbook should point at the governance report writer."
Assert-ContainsText -Text $contentControlGuidance -ExpectedText "release note bundle" `
    -Message "Content-control data-binding runbook should tell reviewers to refresh release materials."

$contentControlChecklistGuidance = @(Get-ReleaseGovernanceChecklistGuidanceLines `
        -Item $contentControlBlocker `
        -ItemKind "blocker" `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $contentControlChecklistGuidance -ExpectedText "release governance blocker" `
    -Message "Content-control data-binding checklist guidance should identify blocker context."
Assert-ContainsText -Text $contentControlChecklistGuidance -ExpectedText "sync_or_fill_bound_content_control" `
    -Message "Content-control data-binding checklist guidance should render its fixed action runbook."
Assert-ContainsText -Text $contentControlChecklistGuidance -ExpectedText "input_docx_display=.\samples\invoice.docx" `
    -Message "Content-control data-binding checklist guidance should keep input DOCX provenance."
Assert-ContainsText -Text $contentControlChecklistGuidance -ExpectedText "sync-content-controls-from-custom-xml" `
    -Message "Content-control data-binding checklist guidance should point at the sync command."

$projectTemplateBlocker = [pscustomobject]@{
    id = "project_template_onboarding.schema_approval"
    source = "project_template_onboarding_governance"
    severity = "error"
    status = "pending_review"
    action = "review_schema_update_candidate"
    message = "Project-template schema update candidate needs review before release."
    source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
    source_report_display = ".\output\project-template-onboarding-governance\governance\report.md"
    source_json_display = ".\output\project-template-onboarding-governance\summary.json"
    command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1 -ProjectSummaryJson .\output\project-template-smoke\summary.json"
    project_id = "starter-docs"
    template_name = "starter-template"
    input_docx = "samples/starter-template.docx"
    input_docx_display = ".\samples\starter-template.docx"
    schema_target = "schemas/starter-template.schema.json"
    target_mode = "resolved-section-targets"
    candidate_type = "schema_patch"
    schema_approval_status = "pending_review"
    gate_status = "pending"
    onboarding_governance_status = "blocked"
    onboarding_governance_release_ready = "false"
    onboarding_governance_source_report_display = ".\output\project-template-onboarding-governance\governance\report.md"
    onboarding_governance_source_json_display = ".\output\project-template-onboarding-governance\summary.json"
    onboarding_governance_schema_approval_status_summary = @(
        [pscustomobject]@{
            status = "pending_review"
            count = 1
        }
    )
}
$projectTemplateGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $projectTemplateBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "review_schema_update_candidate" `
    -Message "Project-template schema approval blocker should render its fixed action runbook."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "project-template" `
    -Message "Project-template schema approval runbook should identify the governance domain."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText ".\output\project-template-onboarding-governance\governance\report.md" `
    -Message "Project-template schema approval runbook should point at source_report_display."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText ".\output\project-template-onboarding-governance\summary.json" `
    -Message "Project-template schema approval runbook should point at source_json_display."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "template_name=starter-template" `
    -Message "Project-template schema approval runbook should keep template provenance."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "input_docx_display=.\samples\starter-template.docx" `
    -Message "Project-template schema approval runbook should keep input DOCX provenance."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "schema_target=schemas/starter-template.schema.json" `
    -Message "Project-template schema approval runbook should keep schema target provenance."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "pending_review=1" `
    -Message "Project-template schema approval runbook should summarize schema approval status."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "sync_project_template_schema_approval.ps1" `
    -Message "Project-template schema approval runbook should point at schema approval sync."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "build_project_template_delivery_readiness_report.ps1" `
    -Message "Project-template schema approval runbook should tell reviewers to rebuild delivery readiness."
Assert-ContainsText -Text $projectTemplateGuidance -ExpectedText "release note bundle" `
    -Message "Project-template schema approval runbook should tell reviewers to refresh release materials."
Assert-True -Condition ($projectTemplateGuidance -notmatch [regex]::Escape("Registered release blocker action")) `
    -Message "Project-template registered action should not use the missing-runbook fallback."

$projectTemplateChecklistGuidance = @(Get-ReleaseGovernanceChecklistGuidanceLines `
        -Item $projectTemplateBlocker `
        -ItemKind "blocker" `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $projectTemplateChecklistGuidance -ExpectedText "release governance blocker" `
    -Message "Project-template checklist guidance should identify blocker context."
Assert-ContainsText -Text $projectTemplateChecklistGuidance -ExpectedText "review_schema_update_candidate" `
    -Message "Project-template checklist guidance should render its fixed action runbook."
Assert-ContainsText -Text $projectTemplateChecklistGuidance -ExpectedText "template_name=starter-template" `
    -Message "Project-template checklist guidance should keep template provenance."
Assert-ContainsText -Text $projectTemplateChecklistGuidance -ExpectedText "sync_project_template_schema_approval.ps1" `
    -Message "Project-template checklist guidance should point at schema approval sync."
Assert-ContainsText -Text $projectTemplateChecklistGuidance -ExpectedText "build_project_template_onboarding_governance_report.ps1" `
    -Message "Project-template checklist guidance should tell reviewers when to rebuild onboarding governance."

$numberingGovernanceBlocker = [pscustomobject]@{
    id = "numbering_catalog_governance.real_corpus_alignment_gap"
    source = "numbering_catalog_governance"
    severity = "error"
    status = "real_corpus_alignment_gap"
    action = "review_numbering_catalog_real_corpus_alignment"
    message = "Numbering catalog exemplars and baseline entries do not align on document identity."
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
    source_report_display = ".\output\numbering-catalog-governance\summary.json"
    source_json_display = ".\output\numbering-catalog-governance\summary.json"
    scope = "numbering_catalog_governance"
    matched_document_count = 1
    unmatched_catalog_document_count = 1
    unmatched_baseline_document_count = 1
    catalog_coverage_percent = 50
    baseline_coverage_percent = 50
    coverage_score = 50
    catalog_document_keys = @("contract", "invoice")
    baseline_document_keys = @("invoice", "obsolete")
    matched_document_keys = @("invoice")
}
$numberingGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $numberingGovernanceBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $numberingGuidance -ExpectedText "review_numbering_catalog_real_corpus_alignment" `
    -Message "Numbering catalog alignment blocker should render its fixed action runbook."
Assert-ContainsText -Text $numberingGuidance -ExpectedText "numbering governance" `
    -Message "Numbering catalog runbook should identify the governance domain."
Assert-ContainsText -Text $numberingGuidance -ExpectedText ".\output\numbering-catalog-governance\summary.json" `
    -Message "Numbering catalog runbook should point at source evidence."
Assert-ContainsText -Text $numberingGuidance -ExpectedText "scope=numbering_catalog_governance" `
    -Message "Numbering catalog runbook should keep scope provenance."
Assert-ContainsText -Text $numberingGuidance -ExpectedText "matched_document_count=1" `
    -Message "Numbering catalog runbook should keep matched document metrics."
Assert-ContainsText -Text $numberingGuidance -ExpectedText "unmatched_catalog_document_count=1" `
    -Message "Numbering catalog runbook should keep unmatched catalog metrics."
Assert-ContainsText -Text $numberingGuidance -ExpectedText "catalog document keys: contract,invoice" `
    -Message "Numbering catalog runbook should list catalog document keys."
Assert-ContainsText -Text $numberingGuidance -ExpectedText "baseline document keys: invoice,obsolete" `
    -Message "Numbering catalog runbook should list baseline document keys."
Assert-ContainsText -Text $numberingGuidance -ExpectedText "build_numbering_catalog_governance_report.ps1" `
    -Message "Numbering catalog runbook should tell reviewers to rebuild governance evidence."
Assert-ContainsText -Text $numberingGuidance -ExpectedText "release note bundle" `
    -Message "Numbering catalog runbook should tell reviewers to refresh release materials."
Assert-True -Condition ($numberingGuidance -notmatch [regex]::Escape("Registered release blocker action")) `
    -Message "Numbering catalog registered action should not use the missing-runbook fallback."

$numberingChecklistGuidance = @(Get-ReleaseGovernanceChecklistGuidanceLines `
        -Item $numberingGovernanceBlocker `
        -ItemKind "blocker" `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $numberingChecklistGuidance -ExpectedText "release governance blocker" `
    -Message "Numbering catalog checklist guidance should identify blocker context."
Assert-ContainsText -Text $numberingChecklistGuidance -ExpectedText "review_numbering_catalog_real_corpus_alignment" `
    -Message "Numbering catalog checklist guidance should render its fixed action runbook."
Assert-ContainsText -Text $numberingChecklistGuidance -ExpectedText "matched_document_count=1" `
    -Message "Numbering catalog checklist guidance should keep real-corpus metrics."
Assert-ContainsText -Text $numberingChecklistGuidance -ExpectedText "build_document_skeleton_governance_rollup_report.ps1" `
    -Message "Numbering catalog checklist guidance should mention skeleton rollup refresh."

$tableLayoutGovernanceBlocker = [pscustomobject]@{
    id = "table_layout_delivery.safe_tblLook_fixes_pending"
    source = "table_layout_delivery_governance"
    severity = "error"
    status = "needs_review"
    action = "apply_safe_tblLook_fixes_then_visual_regression"
    message = "Safe tblLook-only fixes are available and need visual evidence refresh."
    source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
    source_report_display = ".\output\table-layout-delivery-governance\summary.json"
    source_json_display = ".\output\table-layout-delivery-governance\summary.json"
    scope = "table_layout_delivery"
    document_name = "contract.docx"
    table_style_issue_count = 3
    automatic_tblLook_fix_count = 2
    manual_table_style_fix_count = 1
    table_position_review_count = 1
    unresolved_item_count = 10
    pdf_floating_table_layout_boundary = "stable_pdf_geometry_subset_not_full_word_wrapping"
    command_template = "featherdoc_cli apply-table-style-quality-fixes <input.docx> --look-only --output <reviewed.docx> --json"
}
$tableLayoutGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $tableLayoutGovernanceBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "apply_safe_tblLook_fixes_then_visual_regression" `
    -Message "Table layout delivery blocker should render its fixed action runbook."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "table layout delivery governance" `
    -Message "Table layout delivery runbook should identify the governance domain."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText ".\output\table-layout-delivery-governance\summary.json" `
    -Message "Table layout delivery runbook should point at source evidence."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "scope=table_layout_delivery" `
    -Message "Table layout delivery runbook should keep scope provenance."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "document_name=contract.docx" `
    -Message "Table layout delivery runbook should keep document provenance."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "automatic_tblLook_fix_count=2" `
    -Message "Table layout delivery runbook should keep safe tblLook fix metrics."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "pdf_floating_table_layout_boundary=stable_pdf_geometry_subset_not_full_word_wrapping" `
    -Message "Table layout delivery runbook should keep PDF floating table boundary."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "apply-table-style-quality-fixes <input.docx> --look-only" `
    -Message "Table layout delivery runbook should point at safe tblLook repair command."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "run_table_style_quality_visual_regression.ps1" `
    -Message "Table layout delivery runbook should require visual regression evidence refresh."
Assert-ContainsText -Text $tableLayoutGuidance -ExpectedText "release note bundle" `
    -Message "Table layout delivery runbook should tell reviewers to refresh release materials."
Assert-True -Condition ($tableLayoutGuidance -notmatch [regex]::Escape("Registered release blocker action")) `
    -Message "Table layout delivery registered action should not use the missing-runbook fallback."

$tableLayoutChecklistGuidance = @(Get-ReleaseGovernanceChecklistGuidanceLines `
        -Item $tableLayoutGovernanceBlocker `
        -ItemKind "blocker" `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $tableLayoutChecklistGuidance -ExpectedText "release governance blocker" `
    -Message "Table layout delivery checklist guidance should identify blocker context."
Assert-ContainsText -Text $tableLayoutChecklistGuidance -ExpectedText "apply_safe_tblLook_fixes_then_visual_regression" `
    -Message "Table layout delivery checklist guidance should render its fixed action runbook."
Assert-ContainsText -Text $tableLayoutChecklistGuidance -ExpectedText "Table layout delivery metrics" `
    -Message "Table layout delivery checklist guidance should keep quality metrics."
Assert-ContainsText -Text $tableLayoutChecklistGuidance -ExpectedText "build_table_layout_delivery_governance_report.ps1" `
    -Message "Table layout delivery checklist guidance should mention governance report rebuild."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "output gap checks=3" `
    -Message "PDF preflight build-output blocker should include the output gap group count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing outputs=87" `
    -Message "PDF preflight build-output blocker should include the missing output total."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "required checks=12" `
    -Message "PDF preflight build-output blocker should include the memory-aware required check count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDF build options: enabled=false" `
    -Message "PDF preflight build-output blocker should summarize PDF build option readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "disabled=FEATHERDOC_BUILD_PDF,FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "PDF preflight build-output blocker should list disabled PDF build options."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDF preflight build auto candidates" `
    -Message "PDF preflight build-output blocker should summarize auto build candidate readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "build(exists=true, CMakeCache=true, CTest=true, PDF options=false, reusable=false" `
    -Message "PDF preflight build-output blocker should show the build candidate is rejected by PDF options."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "FEATHERDOC_BUILD_PDF(present=true,value=OFF,enabled=false)" `
    -Message "PDF preflight build-output blocker should show the writer option snapshot for build candidate."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT(present=true,value=OFF,enabled=false)" `
    -Message "PDF preflight build-output blocker should show the import option snapshot for build candidate."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDF dependency inputs: status=not_ready" `
    -Message "PDF preflight build-output blocker should summarize PDF dependency input readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "selected PDFium provider=unresolved" `
    -Message "PDF preflight build-output blocker should show the selected PDFium provider."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDFio ready=false" `
    -Message "PDF preflight build-output blocker should show PDFio readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDFium ready=false" `
    -Message "PDF preflight build-output blocker should show PDFium readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing inputs=3" `
    -Message "PDF preflight build-output blocker should show the missing dependency input count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDFium source header" `
    -Message "PDF preflight build-output blocker should show dependency missing input preview."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "Readiness action evidence" `
    -Message "PDF preflight build-output blocker should summarize readiness action evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "provide_pdf_dependency_input/pdf_dependency_inputs_ready -> PDFio source header" `
    -Message "PDF preflight build-output blocker should show dependency input readiness evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "enable_pdf_build_option/pdf_build_options_enabled -> FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "PDF preflight build-output blocker should show disabled build option readiness evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "source JSON: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json" `
    -Message "PDF preflight build-output blocker should keep readiness evidence traceable to source JSON."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "memory guard blocked=false" `
    -Message "PDF preflight build-output blocker should include the memory guard blocked state."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "memory guard skipped=false" `
    -Message "PDF preflight build-output blocker should include the memory guard skipped state."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "free memory MB=1140" `
    -Message "PDF preflight build-output blocker should include the observed free memory."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "minimum free memory MB=2048" `
    -Message "PDF preflight build-output blocker should include the configured minimum free memory."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "source preflight JSON" `
    -Message "PDF preflight build-output blocker should point reviewers at source_json_display evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "release note bundle" `
    -Message "PDF preflight build-output blocker should tell reviewers to refresh release materials."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "clean up only task-owned PDF gate processes" `
    -Message "PDF preflight build-output blocker should remind reviewers to clean up only task-owned resources."

$pdfPreflightChecklistGuidance = @(Get-ReleaseGovernanceChecklistGuidanceLines `
        -Item $pdfPreflightBlocker `
        -ItemKind "action item" `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing CLI PDFs=2" `
    -Message "PDF preflight checklist guidance should include the missing CLI PDF count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing visual baseline PDFs=42" `
    -Message "PDF preflight checklist guidance should include the missing visual baseline count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing CJK text-layer PDFs=43" `
    -Message "PDF preflight checklist guidance should include the missing CJK text-layer count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "output gap checks=3" `
    -Message "PDF preflight checklist guidance should include the output gap group count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing outputs=87" `
    -Message "PDF preflight checklist guidance should include the missing output total."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "memory guard blocked=false" `
    -Message "PDF preflight checklist guidance should include the memory guard blocked state."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "PDF preflight build auto candidates" `
    -Message "PDF preflight checklist guidance should summarize auto build candidate readiness."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT(present=true,value=OFF,enabled=false)" `
    -Message "PDF preflight checklist guidance should show candidate PDF option snapshots."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "PDF dependency inputs: status=not_ready" `
    -Message "PDF preflight checklist guidance should summarize PDF dependency input readiness."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing inputs=3" `
    -Message "PDF preflight checklist guidance should show missing dependency input count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "Readiness action evidence" `
    -Message "PDF preflight checklist guidance should summarize readiness action evidence."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "provide_pdf_dependency_input/pdf_dependency_inputs_ready -> PDFio source header" `
    -Message "PDF preflight checklist guidance should show dependency input readiness evidence."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "not release-ready evidence" `
    -Message "PDF preflight checklist guidance should state that preflight-only is not release-ready evidence."

$controlledVisualSmokeWarning = [pscustomobject]@{
    id = "pdf_controlled_visual_smoke.unavailable_or_failed"
    source = "pdf_controlled_visual_smoke"
    severity = "warning"
    status = "fail"
    action = "rerun_pdf_controlled_visual_smoke_check"
    message = "Controlled PDF visual smoke evidence was provided but is not passing."
    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
    source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\controlled-visual-smoke-failed.json"
    error_message = "Controlled smoke page was blank."
}
$controlledVisualSmokeGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $controlledVisualSmokeWarning `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "rerun_pdf_controlled_visual_smoke_check" `
    -Message "Controlled PDF visual smoke warning should render its fixed action runbook."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "check_pdf_controlled_visual_smoke.ps1" `
    -Message "Controlled PDF visual smoke runbook should point at the smoke check wrapper."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "controlled-visual-smoke-failed.json" `
    -Message "Controlled PDF visual smoke runbook should keep source JSON evidence visible."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "read-only" `
    -Message "Controlled PDF visual smoke runbook should state the check is read-only."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "does not run CMake, CTest, Word" `
    -Message "Controlled PDF visual smoke runbook should keep heavyweight runtime boundaries explicit."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "not release-ready evidence" `
    -Message "Controlled PDF visual smoke runbook should state the evidence boundary."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "write_pdf_visual_release_gate_preflight_governance_report.ps1" `
    -Message "Controlled PDF visual smoke runbook should tell reviewers to rebuild governance evidence."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "release note bundle" `
    -Message "Controlled PDF visual smoke runbook should tell reviewers to refresh release materials."
Assert-True -Condition ($controlledVisualSmokeGuidance -notmatch [regex]::Escape("Unregistered release blocker action")) `
    -Message "Controlled PDF visual smoke registered action should not use the unregistered fallback."

$controlledVisualSmokeChecklistGuidance = @(Get-ReleaseGovernanceWarningActionGuidanceLines `
        -Warning $controlledVisualSmokeWarning `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $controlledVisualSmokeChecklistGuidance -ExpectedText "release governance warning" `
    -Message "Controlled PDF visual smoke checklist guidance should identify warning context."
Assert-ContainsText -Text $controlledVisualSmokeChecklistGuidance -ExpectedText "check_pdf_controlled_visual_smoke.ps1" `
    -Message "Controlled PDF visual smoke checklist guidance should point at the smoke check wrapper."
Assert-ContainsText -Text $controlledVisualSmokeChecklistGuidance -ExpectedText "controlled-visual-smoke-failed.json" `
    -Message "Controlled PDF visual smoke checklist guidance should keep source JSON visible."
Assert-ContainsText -Text $controlledVisualSmokeChecklistGuidance -ExpectedText "not release-ready evidence" `
    -Message "Controlled PDF visual smoke checklist guidance should state the evidence boundary."

$docxFunctionalSmokeBlocker = [pscustomobject]@{
    id = "docx_functional_smoke.word_visual_smoke_reused_evidence"
    source = "docx_functional_smoke_readiness"
    severity = "error"
    status = "open"
    action = "restore_docx_functional_smoke_evidence"
    message = "Persisted Word visual smoke contact sheets and page PNGs must exist and be non-empty."
    source_schema = "featherdoc.docx_functional_smoke_readiness.v1"
    source_report_display = ".\output\docx-functional-smoke-readiness-current\summary.json"
    source_json_display = ".\output\docx-functional-smoke-readiness-current\summary.json"
}
$docxFunctionalSmokeGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $docxFunctionalSmokeBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $docxFunctionalSmokeGuidance -ExpectedText "restore_docx_functional_smoke_evidence" `
    -Message "DOCX functional smoke blocker should render its fixed action runbook."
Assert-ContainsText -Text $docxFunctionalSmokeGuidance -ExpectedText "check_docx_functional_smoke_readiness.ps1" `
    -Message "DOCX functional smoke runbook should point at the readiness check wrapper."
Assert-ContainsText -Text $docxFunctionalSmokeGuidance -ExpectedText "docx-functional-smoke-readiness-current\summary.json" `
    -Message "DOCX functional smoke runbook should keep source JSON evidence visible."
Assert-ContainsText -Text $docxFunctionalSmokeGuidance -ExpectedText "read-only" `
    -Message "DOCX functional smoke runbook should state the check is read-only."
Assert-ContainsText -Text $docxFunctionalSmokeGuidance -ExpectedText "does not run CMake, CTest, Word" `
    -Message "DOCX functional smoke runbook should keep heavyweight runtime boundaries explicit."
Assert-ContainsText -Text $docxFunctionalSmokeGuidance -ExpectedText "not a fresh Word COM render" `
    -Message "DOCX functional smoke runbook should state the reused-evidence boundary."
Assert-ContainsText -Text $docxFunctionalSmokeGuidance -ExpectedText "release governance pipeline and handoff evidence" `
    -Message "DOCX functional smoke runbook should tell reviewers to rebuild governance evidence."
Assert-ContainsText -Text $docxFunctionalSmokeGuidance -ExpectedText "release note bundle" `
    -Message "DOCX functional smoke runbook should tell reviewers to refresh release materials."
Assert-True -Condition ($docxFunctionalSmokeGuidance -notmatch [regex]::Escape("Unregistered release blocker action")) `
    -Message "DOCX functional smoke registered action should not use the unregistered fallback."

$docxFunctionalSmokeChecklistGuidance = @(Get-ReleaseGovernanceBlockerActionGuidanceLines `
        -Blocker $docxFunctionalSmokeBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $docxFunctionalSmokeChecklistGuidance -ExpectedText "release governance blocker" `
    -Message "DOCX functional smoke checklist guidance should identify blocker context."
Assert-ContainsText -Text $docxFunctionalSmokeChecklistGuidance -ExpectedText "check_docx_functional_smoke_readiness.ps1" `
    -Message "DOCX functional smoke checklist guidance should point at the readiness check wrapper."
Assert-ContainsText -Text $docxFunctionalSmokeChecklistGuidance -ExpectedText "not a fresh Word COM render" `
    -Message "DOCX functional smoke checklist guidance should state the reused-evidence boundary."

$pdfPreflightUnavailableBlocker = [pscustomobject]@{
    id = "pdf_visual_release_gate_preflight.summary_unavailable"
    source = "pdf_visual_release_gate_preflight"
    severity = "error"
    status = "blocked"
    action = "rerun_pdf_visual_release_gate_preflight"
    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
    command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
    readiness_action_evidence = @(
        [pscustomobject]@{
            action = "rerun_pdf_visual_release_gate_preflight"
            issue_key = "summary_unavailable"
            item = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
            source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
        }
    )
}
$pdfPreflightUnavailableGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $pdfPreflightUnavailableBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "rerun_pdf_visual_release_gate_preflight" `
    -Message "PDF preflight summary-unavailable blocker should render its fixed action runbook."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "check_pdf_visual_release_gate_preflight.ps1" `
    -Message "PDF preflight summary-unavailable blocker should point at lightweight preflight regeneration."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "write_pdf_visual_release_gate_preflight_governance_report.ps1" `
    -Message "PDF preflight summary-unavailable blocker should tell reviewers to rebuild governance evidence."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "Readiness action evidence" `
    -Message "PDF preflight summary-unavailable blocker should summarize readiness action evidence."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "rerun_pdf_visual_release_gate_preflight/summary_unavailable -> .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json" `
    -Message "PDF preflight summary-unavailable blocker should keep its rerun readiness evidence visible."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "not release-ready evidence" `
    -Message "PDF preflight summary-unavailable blocker should state that preflight summary is not release-ready evidence."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "release note bundle" `
    -Message "PDF preflight summary-unavailable blocker should tell reviewers to refresh release materials."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "clean up only task-owned PDF gate processes" `
    -Message "PDF preflight summary-unavailable blocker should remind reviewers to clean up only task-owned resources."

Write-Host "Release blocker action registry self-check passed."
