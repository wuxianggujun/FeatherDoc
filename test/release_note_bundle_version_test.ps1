param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Join-Path $PSScriptRoot "..")
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $resolvedRepoRoot "output\test\release-note-bundle-version"
}

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw ("{0} does not contain expected text '{1}': {2}" -f $Label, $ExpectedText, $Path)
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
        $matchesAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            if ($line -notmatch [regex]::Escape($fragment)) {
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

function Assert-MarkdownListBlockContainsAll {
    param(
        [string]$Path,
        [string]$Anchor,
        [string[]]$ExpectedFragments,
        [string]$Label
    )

    $lines = Get-Content -LiteralPath $Path
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if ($lines[$lineIndex] -notmatch [regex]::Escape($Anchor)) {
            continue
        }
        if ($lines[$lineIndex] -notmatch '^\s*-\s*') {
            continue
        }

        $blockLines = @($lines[$lineIndex])
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            $nextLine = $lines[$nextLineIndex]
            if ([string]::IsNullOrWhiteSpace($nextLine) -or
                $nextLine -match '^#{1,6}\s' -or
                ($nextLine -match '^\s*-\s*' -and $nextLine -notmatch '^\s{2,}-\s*')) {
                break
            }

            $blockLines += $nextLine
        }

        $block = $blockLines -join "`n"
        $matchesAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            if ($block -notmatch [regex]::Escape($fragment)) {
                $matchesAllFragments = $false
                break
            }
        }

        if ($matchesAllFragments) {
            return
        }
    }

    throw ("{0} does not contain one Markdown list block anchored by '{1}' with all expected fragments: {2}" -f $Label, $Anchor, ($ExpectedFragments -join ", "))
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
        throw ("{0} unexpectedly contains '{1}': {2}" -f $Label, $UnexpectedText, $Path)
    }
}

function Assert-ContentControlGovernanceTrace {
    param(
        [string]$Path,
        [string]$Label
    )

    $expectedFragments = @(
        'content_control_data_binding.bound_placeholder',
        'custom_xml_sync_evidence_missing',
        'source_schema=featherdoc.content_control_data_binding_governance_report.v1',
        'source_report_display: .\output\content-control-data-binding-governance\summary.json',
        'source_json_display: .\output\content-control-data-binding\inspect-content-controls.json',
        'input_docx_display: .\samples\invoice.docx',
        'input_docx: samples/invoice.docx',
        'template_name=invoice-template',
        'schema_target: invoice',
        'target_mode: resolved-section-targets',
        'repair_strategy: sync_bound_content_control',
        'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.',
        $contentControlCommandTemplateMarker,
        'review_duplicate_content_control_binding',
        'repair_strategy: deduplicate_or_confirm_shared_binding',
        'repair_hint: Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths.',
        $contentControlDuplicateActionCommandTemplateMarker,
        'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1'
    )

    foreach ($expectedFragment in $expectedFragments) {
        Assert-Contains -Path $Path -ExpectedText $expectedFragment -Label $Label
    }
}

function New-NumberingGovernanceMetricFixture {
    return [ordered]@{
        id = "numbering_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "numbering_catalog_governance"
        source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        source_report_display = ".\output\numbering-catalog-governance\summary.json"
        source_json_display = ".\output\numbering-catalog-governance\summary.json"
        score = 56
        level = "low"
        details = [ordered]@{
            score = 56
            level = "low"
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
                [ordered]@{ factor = "style_numbering_issues"; count = 4; penalty = 20 }
            )
        }
    }
}

function New-TableLayoutDeliveryMetricFixture {
    return [ordered]@{
        id = "table_layout_delivery_governance.delivery_quality"
        metric = "delivery_quality"
        report_id = "table_layout_delivery_governance"
        source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        source_report_display = ".\output\table-layout-delivery-governance\summary.json"
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
            penalty_summary = @(
                [ordered]@{ factor = "floating_table_plans_pending"; count = 0; penalty = 0 }
            )
        }
    }
}

$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$reportDir = Join-Path $resolvedWorkingDir "report"
$installDir = Join-Path $resolvedWorkingDir "install"
$gateReportDir = Join-Path $resolvedWorkingDir "word-visual-release-gate\report"
$schemaApprovalHistoryJsonPath = Join-Path $reportDir "project_template_schema_approval_history.json"
$schemaApprovalHistoryMarkdownPath = Join-Path $reportDir "project_template_schema_approval_history.md"
$taskOutputRoot = Join-Path $resolvedWorkingDir "tasks"
$sectionPageSetupTaskDir = Join-Path $resolvedWorkingDir "tasks\section-page-setup"
$pageNumberFieldsTaskDir = Join-Path $resolvedWorkingDir "tasks\page-number-fields"
$curatedBundleId = "template-table-cli-selector"
$curatedBundleLabel = "Template table CLI selector"
$curatedBundleTaskDir = Join-Path $resolvedWorkingDir "tasks\$curatedBundleId"
$supersededReviewTasksReportPath = Join-Path $taskOutputRoot "superseded_review_tasks.json"
$expectedSupersededReviewTasksReportDisplayPath = if ($supersededReviewTasksReportPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    ".\" + ($supersededReviewTasksReportPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/') -replace '/', '\')
} else {
    [System.IO.Path]::GetFullPath($supersededReviewTasksReportPath)
}
$contentControlCommandTemplateMarker = "command_template: featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
$contentControlDuplicateActionCommandTemplateMarker = "command_template: featherdoc_cli inspect-content-controls <input.docx> --json"

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $installDir -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $taskOutputRoot -Force | Out-Null
New-Item -ItemType Directory -Path $sectionPageSetupTaskDir -Force | Out-Null
New-Item -ItemType Directory -Path $pageNumberFieldsTaskDir -Force | Out-Null
New-Item -ItemType Directory -Path $curatedBundleTaskDir -Force | Out-Null

$summaryPath = Join-Path $reportDir "summary.json"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"

$gateSummary = [ordered]@{
    generated_at = "2026-04-11T12:00:00"
    workspace = $resolvedRepoRoot
    report_dir = $gateReportDir
    visual_verdict = "pending_manual_review"
    review_tasks = [ordered]@{
        section_page_setup = [ordered]@{
            task_dir = $sectionPageSetupTaskDir
        }
        page_number_fields = [ordered]@{
            task_dir = $pageNumberFieldsTaskDir
        }
        curated_visual_regressions = @(
            [ordered]@{
                id = $curatedBundleId
                label = $curatedBundleLabel
                task = [ordered]@{
                    task_dir = $curatedBundleTaskDir
                }
            }
        )
    }
    manual_review = [ordered]@{
        tasks = [ordered]@{
            section_page_setup = [ordered]@{
                verdict = "pass"
            }
            page_number_fields = [ordered]@{
                verdict = "pending_manual_review"
            }
            curated_visual_regressions = @(
                [ordered]@{
                    id = $curatedBundleId
                    label = "curated:$curatedBundleId"
                    display_label = $curatedBundleLabel
                    verdict = "pass"
                    task_dir = $curatedBundleTaskDir
                }
            )
        }
    }
    section_page_setup = [ordered]@{
        review_verdict = "pass"
    }
    page_number_fields = [ordered]@{
        review_verdict = "pending_manual_review"
    }
    curated_visual_regressions = @(
        [ordered]@{
            id = $curatedBundleId
            label = $curatedBundleLabel
            review_verdict = "pass"
            status = "completed"
            task = [ordered]@{
                task_dir = $curatedBundleTaskDir
            }
        }
    )
}
($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8
Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8 -Value "# Gate Final Review"
(@{
    generated_at = "2026-04-11T12:00:00"
    task_output_root = $taskOutputRoot
    report_path = $supersededReviewTasksReportPath
    group_count = 2
    superseded_task_count = 0
    groups = @()
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $supersededReviewTasksReportPath -Encoding UTF8

$summary = [ordered]@{
    generated_at = "2026-04-11T12:00:00"
    workspace = $resolvedRepoRoot
    execution_status = "pass"
    release_version = "1.6.0"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            source = "project_template_smoke"
            severity = "error"
            status = "blocked"
            message = "Project template smoke schema approval gate blocked."
            gate_status = "blocked"
            compliance_issue_count = 2
            invalid_result_count = 1
            blocked_item_count = 1
            issue_keys = @("missing_reviewer", "missing_reviewed_at")
            summary_json = Join-Path $resolvedWorkingDir "project-template-smoke-summary.json"
            history_json = $schemaApprovalHistoryJsonPath
            history_markdown = $schemaApprovalHistoryMarkdownPath
            action = "fix_schema_patch_approval_result"
            items = @(
                [ordered]@{
                    name = "schema-review-invalid"
                    status = "invalid_result"
                    decision = "approved"
                    action = "fix_schema_patch_approval_result"
                    compliance_issue_count = 2
                    compliance_issues = @("missing_reviewer", "missing_reviewed_at")
                    approval_result = Join-Path $resolvedWorkingDir "schema_patch_approval_result.json"
                }
            )
        }
    )
    release_blocker_rollup = [ordered]@{
        requested = $true
        status = "blocked"
        source_report_count = 6
        source_failure_count = 1
        source_reports = @(
            [ordered]@{
                schema = "featherdoc.table_layout_delivery_governance_report.v1"
                status = "failed"
                release_ready = $false
                path = ".\output\table-layout-delivery-governance\summary.json"
                path_display = ".\output\table-layout-delivery-governance\summary.json"
                source_json = ".\output\table-layout-delivery-governance\unreadable-summary.json"
                source_json_display = ".\output\table-layout-delivery-governance\unreadable-summary.json"
                source_failure_count = 1
                error = "Unexpected token while reading table layout governance summary."
                build_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1"
            },
            [ordered]@{
                schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                status = "blocked"
                release_ready = $false
                path = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                path_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                source_failure_count = 0
                preflight_ready = $false
                full_visual_gate_required = $true
                full_visual_gate_status = "not_run_by_preflight_governance"
                controlled_visual_smoke_available = $true
                controlled_visual_smoke_status = "pass"
                controlled_visual_smoke_passed = $true
                controlled_visual_smoke_case_count = 2
                controlled_visual_smoke_json = ".\output\pdf-controlled-visual-smoke-20260520\controlled-visual-smoke-check-latest.json"
                controlled_visual_smoke_json_display = ".\output\pdf-controlled-visual-smoke-20260520\controlled-visual-smoke-check-latest.json"
            },
            [ordered]@{
                schema = "featherdoc.project_template_delivery_readiness_report.v1"
                status = "blocked"
                release_ready = $false
                path = ".\output\project-template-delivery-readiness\summary.json"
                path_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-delivery-readiness\summary.json"
                source_failure_count = 0
                latest_schema_approval_gate_status = "pending_review"
                schema_approval_status_summary = "pending_review"
                schema_history_blocked_run_count = 0
                schema_history_pending_run_count = 1
                schema_history_passed_run_count = 2
                template_count = 3
                ready_template_count = 2
                blocked_template_count = 1
                release_blocker_count = 1
                action_item_count = 1
                warning_count = 0
                build_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1"
            }
        )
        release_blocker_count = 5
        release_blockers = @(
            [ordered]@{
                id = "document_skeleton.style_numbering_issues"
                source = "document_skeleton_governance_rollup"
                severity = "error"
                status = "needs_review"
                action = "review_style_numbering_audit"
                message = "Document skeleton rollup found style numbering issues."
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance\contract\style-numbering-audit.json"
            },
            [ordered]@{
                id = "content_control_data_binding.bound_placeholder"
                source = "content_control_data_binding_governance"
                severity = "error"
                status = "placeholder_visible"
                action = "sync_or_fill_bound_content_control"
                message = "Bound content control still shows placeholder text."
                source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
                source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                input_docx = "samples/invoice.docx"
                input_docx_display = ".\samples\invoice.docx"
                template_name = "invoice-template"
                schema_target = "invoice"
                target_mode = "resolved-section-targets"
                repair_strategy = "sync_bound_content_control"
                repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
                command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
            },
            [ordered]@{
                id = "project_template_onboarding.schema_approval"
                source = "project_template_delivery_readiness"
                severity = "error"
                status = "pending_review"
                action = "review_schema_update_candidate"
                message = "Project template onboarding schema approval is pending."
                source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            },
            [ordered]@{
                id = "schema_patch_confidence_calibration.pending_schema_approvals"
                source = "schema_patch_confidence_calibration"
                severity = "error"
                status = "pending_review"
                action = "resolve_pending_schema_approvals"
                message = "Schema patch confidence calibration still contains pending approval outcome(s)."
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            },
            [ordered]@{
                id = "pdf_visual_release_gate_preflight.build_outputs_missing"
                source = "pdf_visual_release_gate_preflight"
                severity = "error"
                status = "blocked"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                message = "PDF visual release gate preflight is not ready; blocking checks: build_dir_exists, cmake_cache_exists, ctest_manifest_exists, pdf_build_options_enabled."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                repair_strategy = "reuse_or_prepare_pdf_visual_release_gate_build_outputs"
                repair_hint = "Prepare a reusable PDF build directory with CMakeCache.txt and enable FEATHERDOC_BUILD_PDF / FEATHERDOC_BUILD_PDF_IMPORT before running the full PDF visual release gate."
                command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly"
                issue_keys = @("build_dir_exists", "cmake_cache_exists", "ctest_manifest_exists", "pdf_build_options_enabled")
                blocking_summary = [ordered]@{
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
                }
                readiness_action_evidence_count = 2
                readiness_action_evidence = @(
                    [ordered]@{
                        id = "pdf_visual_release_gate_preflight.pdf_dependency_inputs_ready.pdfio_source_header"
                        action = "provide_pdf_dependency_input"
                        issue_key = "pdf_dependency_inputs_ready"
                        item = "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h"
                        source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                        source_report = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                        source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                    },
                    [ordered]@{
                        id = "pdf_visual_release_gate_preflight.pdf_build_options_enabled.FEATHERDOC_BUILD_PDF_IMPORT"
                        action = "enable_pdf_build_option"
                        issue_key = "pdf_build_options_enabled"
                        item = "FEATHERDOC_BUILD_PDF_IMPORT"
                        source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                        source_report = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                        source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                    }
                )
            }
        )
        action_item_count = 5
        action_items = @(
            [ordered]@{
                id = "open_document_skeleton_rollup"
                action = "open_document_skeleton_rollup"
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1 -InputRoot .\output\document-skeleton-governance"
            },
            [ordered]@{
                id = "review_duplicate_content_control_binding"
                action = "review_duplicate_content_control_binding"
                source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
                source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                input_docx = "samples/invoice.docx"
                input_docx_display = ".\samples\invoice.docx"
                template_name = "invoice-template"
                schema_target = "invoice"
                target_mode = "resolved-section-targets"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
                repair_strategy = "deduplicate_or_confirm_shared_binding"
                repair_hint = "Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths."
                command_template = "featherdoc_cli inspect-content-controls <input.docx> --json"
            },
            [ordered]@{
                id = "review_invoice_schema"
                action = "review_schema_update_candidate"
                source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1"
            },
            [ordered]@{
                id = "resolve_pending_schema_approvals"
                action = "resolve_pending_schema_approvals"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
            },
            [ordered]@{
                id = "prepare_pdf_visual_release_gate_build_outputs"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                open_command = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly"
                command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly"
                issue_keys = @("build_dir_exists", "cmake_cache_exists", "ctest_manifest_exists", "pdf_build_options_enabled")
                blocking_summary = [ordered]@{
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
                }
                readiness_action_evidence_count = 2
                readiness_action_evidence = @(
                    [ordered]@{
                        id = "pdf_visual_release_gate_preflight.pdf_dependency_inputs_ready.pdfio_source_header"
                        action = "provide_pdf_dependency_input"
                        issue_key = "pdf_dependency_inputs_ready"
                        item = "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h"
                        source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                        source_report = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                        source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                    },
                    [ordered]@{
                        id = "pdf_visual_release_gate_preflight.pdf_build_options_enabled.FEATHERDOC_BUILD_PDF_IMPORT"
                        action = "enable_pdf_build_option"
                        issue_key = "pdf_build_options_enabled"
                        item = "FEATHERDOC_BUILD_PDF_IMPORT"
                        source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                        source_report = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
                        source_json = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                        source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
                    }
                )
            }
        )
        warning_count = 3
        warnings = @(
            [ordered]@{
                id = "document_skeleton.exemplar_catalog_missing"
                action = "open_document_skeleton_rollup"
                message = "One exemplar catalog path is missing."
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
            },
            [ordered]@{
                id = "custom_xml_sync_evidence_missing"
                action = "run_content_control_custom_xml_sync"
                message = "Data-bound content controls were inspected, but no Custom XML sync result was provided."
                source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
                source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                input_docx = "samples/invoice.docx"
                input_docx_display = ".\samples\invoice.docx"
                template_name = "invoice-template"
                schema_target = "invoice"
                target_mode = "resolved-section-targets"
            },
            [ordered]@{
                id = "schema_patch_confidence_calibration.unscored_candidates"
                action = "add_explicit_confidence_metadata"
                message = "Some schema patch candidates do not carry explicit confidence metadata."
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            }
        )
        governance_metric_count = 2
        governance_metrics = @(
            (New-NumberingGovernanceMetricFixture),
            (New-TableLayoutDeliveryMetricFixture)
        )
    }
    release_governance_handoff = [ordered]@{
        requested = $true
        status = "blocked"
        expected_report_count = 5
        loaded_report_count = 5
        missing_report_count = 0
        failed_report_count = 1
        reports = @(
            [ordered]@{
                id = "project_template_delivery_readiness"
                schema = "featherdoc.project_template_delivery_readiness_report.v1"
                status = "failed"
                release_ready = $false
                expected_summary = ".\output\project-template-delivery-readiness\summary.json"
                expected_summary_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-delivery-readiness\summary.json"
                source_failure_count = 1
                latest_schema_approval_gate_status = "pending_review"
                schema_approval_status_summary = "pending_review"
                schema_history_blocked_run_count = 0
                schema_history_pending_run_count = 1
                schema_history_passed_run_count = 2
                template_count = 3
                ready_template_count = 2
                blocked_template_count = 1
                release_blocker_count = 1
                action_item_count = 1
                warning_count = 0
                error = "Failed to parse project template delivery readiness summary."
                build_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1"
            }
        )
        release_blocker_count = 2
        release_blockers = @(
            [ordered]@{
                report_id = "project_template_delivery_readiness"
                id = "project_template_onboarding.schema_approval"
                severity = "error"
                status = "pending_review"
                action = "review_schema_update_candidate"
                message = "Project template onboarding schema approval is pending."
                source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
            },
            [ordered]@{
                report_id = "schema_patch_confidence_calibration"
                id = "schema_patch_confidence_calibration.pending_schema_approvals"
                severity = "error"
                status = "pending_review"
                action = "resolve_pending_schema_approvals"
                message = "Schema patch confidence calibration still contains pending approval outcome(s)."
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            }
        )
        action_item_count = 2
        action_items = @(
            [ordered]@{
                report_id = "project_template_delivery_readiness"
                id = "review_invoice_schema"
                action = "review_schema_update_candidate"
                source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1"
            },
            [ordered]@{
                report_id = "schema_patch_confidence_calibration"
                id = "resolve_pending_schema_approvals"
                action = "resolve_pending_schema_approvals"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
            }
        )
        warning_count = 1
        warnings = @(
            [ordered]@{
                report_id = "schema_patch_confidence_calibration"
                id = "schema_patch_confidence_calibration.unscored_candidates"
                action = "add_explicit_confidence_metadata"
                message = "Some schema patch candidates do not carry explicit confidence metadata."
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            }
        )
        governance_metric_count = 2
        governance_metrics = @(
            (New-NumberingGovernanceMetricFixture),
            (New-TableLayoutDeliveryMetricFixture)
        )
    }
    task_output_root = $taskOutputRoot
    superseded_review_tasks_report = $supersededReviewTasksReportPath
    install_dir = $installDir
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
            smoke_verdict = "pass"
            fixed_grid_verdict = "undetermined"
            curated_visual_regressions = @(
                [ordered]@{
                    id = $curatedBundleId
                    label = $curatedBundleLabel
                    review_verdict = "pass"
                    task_dir = $curatedBundleTaskDir
                }
            )
        }
        project_template_smoke = [ordered]@{
            status = "completed"
            schema_patch_approval_history_json = $schemaApprovalHistoryJsonPath
            schema_patch_approval_history_markdown = $schemaApprovalHistoryMarkdownPath
        }
    }
    project_template_smoke = [ordered]@{
        requested = $true
        manifest_path = Join-Path $resolvedWorkingDir "project-template-smoke.manifest.json"
        summary_json = Join-Path $resolvedWorkingDir "project-template-smoke-summary.json"
        output_dir = Join-Path $resolvedWorkingDir "project-template-smoke"
        candidate_discovery_json = Join-Path $resolvedWorkingDir "project-template-smoke-candidates.json"
        schema_patch_approval_gate_status = "passed"
        schema_patch_approval_history_json = $schemaApprovalHistoryJsonPath
        schema_patch_approval_history_markdown = $schemaApprovalHistoryMarkdownPath
    }
}
($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$bundleScript = Join-Path $resolvedRepoRoot "scripts\write_release_note_bundle.ps1"
& $bundleScript -SummaryJson $summaryPath -SkipMaterialSafetyAudit

$handoffPath = Join-Path $reportDir "release_handoff.md"
$bodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$shortPath = Join-Path $reportDir "release_summary.zh-CN.md"

Assert-Contains -Path $handoffPath -ExpectedText 'Project version: 1.6.0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'publish_github_release.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Smoke verdict: pass' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Section page setup verdict: pass' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Page number fields verdict: pending_manual_review' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Template table CLI selector review task' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_latest_word_review_task.ps1 -SourceKind template-table-cli-selector-visual-regression-bundle -PrintPrompt' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Superseded review tasks: 0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Release blockers: 1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'project_template_smoke.schema_approval' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'missing_reviewer' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Release Governance Rollup Details' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.table_layout_delivery_governance_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\table-layout-delivery-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'matched_document_count=4' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'catalog_coverage_percent=100' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText "style_numbering_issues(count=4, penalty=20)" -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\table-layout-delivery-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'ready_document_percent=100' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'table_style_issue_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'automatic_tblLook_fix_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'manual_table_style_fix_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'table_position_automatic_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'table_position_review_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'command_failure_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'unresolved_item_count=0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText "floating_table_plans_pending(count=0, penalty=0)" -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=review_style_numbering_audit' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=sync_or_fill_bound_content_control' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'repair_strategy: sync_bound_content_control' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText $contentControlCommandTemplateMarker -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=review_schema_update_candidate' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=resolve_pending_schema_approvals' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.document_skeleton_governance_rollup_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.content_control_data_binding_governance_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_schema=featherdoc.schema_patch_confidence_calibration_report.v1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\document-skeleton-governance-rollup\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\project-template-delivery-readiness\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\document-skeleton-governance\contract\style-numbering-audit.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\content-control-data-binding\inspect-content-controls.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'document_skeleton.exemplar_catalog_missing' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'custom_xml_sync_evidence_missing' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'schema_patch_confidence_calibration.unscored_candidates' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_document_skeleton_rollup' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Release Governance Handoff Details' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Handoff Warnings' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'schema_patch_confidence_calibration.unscored_candidates' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_report_display: .\output\project-template-delivery-readiness\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText $expectedSupersededReviewTasksReportDisplayPath -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_latest_page_number_fields_review_task.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'find_superseded_review_tasks.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $bodyPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\visual-validation' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Release blockers: 1' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'project_template_smoke.schema_approval' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Project template governance contracts' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'project_template_delivery_readiness project_template_delivery_readiness_contract' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source_schema=featherdoc.project_template_delivery_readiness_report.v1' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'latest_schema_approval_gate_status=pending_review' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'latest_schema_approval_gate_status=pending_review schema_approval_status_summary=pending_review schema_history_blocked_run_count=0 schema_history_pending_run_count=1 schema_history_passed_run_count=2 template_count=3 ready_template_count=2 blocked_template_count=1 release_blocker_count=1 action_item_count=1 warning_count=0 source_failure_count=0 source_report_display=.\output\project-template-delivery-readiness\summary.json source_json_display=.\output\project-template-delivery-readiness\summary.json' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source_json_display=.\output\project-template-delivery-readiness\summary.json' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'schema_approval_status_summary=pending_review' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=pending_review source_report_display=.\output\project-template-delivery-readiness\summary.json source_json_display=.\output\project-template-onboarding-governance\summary.json' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source_json_display=.\output\project-template-onboarding-governance\summary.json' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Readiness action evidence:' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'provide_pdf_dependency_input/pdf_dependency_inputs_ready -> PDFio source header' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'enable_pdf_build_option/pdf_build_options_enabled -> FEATHERDOC_BUILD_PDF_IMPORT' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'source JSON: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Smoke verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Fixed-grid verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Section page setup verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Page number fields verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Template table CLI selector verdict' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'smoke=`pass`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'fixed-grid=`undetermined`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'section page setup=`pass`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'page number fields=`pending_manual_review`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'Template table CLI selector=`pass`' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'project-template readiness governance contract' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'status=blocked release_ready=False latest_schema_approval_gate_status=pending_review schema_approval_status_summary=pending_review source_report_display=.\output\project-template-delivery-readiness\summary.json source_json_display=.\output\project-template-delivery-readiness\summary.json' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'project-template onboarding governance contract' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'schema_approval_status_summary=pending_review source_report_display=.\output\project-template-delivery-readiness\summary.json source_json_display=.\output\project-template-onboarding-governance\summary.json' -Label 'release_summary.zh-CN.md'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template readiness governance contract',
    'status=blocked',
    'release_ready=False',
    'latest_schema_approval_gate_status=pending_review',
    'schema_approval_status_summary=pending_review',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-delivery-readiness\summary.json'
) -Label 'release_summary.zh-CN.md'
Assert-LineContainsAll -Path $shortPath -ExpectedFragments @(
    'project-template onboarding governance contract',
    'schema_approval_status_summary=pending_review',
    'source_report_display=.\output\project-template-delivery-readiness\summary.json',
    'source_json_display=.\output\project-template-onboarding-governance\summary.json'
) -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $installDir -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $resolvedWorkingDir -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText 'draft' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText '草稿' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText '发布说明草稿' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText '请在发布前补齐' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText '这份文件由 `write_release_body_zh.ps1` 自动生成' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText 'draft' -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText '草稿' -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText '这份文件由 `write_release_body_zh.ps1` 自动生成' -Label 'release_summary.zh-CN.md'

$guidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$checklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$startHerePath = Join-Path (Split-Path -Parent $reportDir) "START_HERE.md"
$releaseGovernanceReportIssueDocuments = @(
    [pscustomobject]@{ Path = $handoffPath; Label = "release_handoff.md" },
    [pscustomobject]@{ Path = $guidePath; Label = "ARTIFACT_GUIDE.md" },
    [pscustomobject]@{ Path = $checklistPath; Label = "REVIEWER_CHECKLIST.md" },
    [pscustomobject]@{ Path = $startHerePath; Label = "START_HERE.md" }
)
foreach ($document in $releaseGovernanceReportIssueDocuments) {
    Assert-Contains -Path $document.Path -ExpectedText 'Rollup Source Report Contracts' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'featherdoc.pdf_visual_release_gate_preflight_governance_report.v1: status=blocked ready=False' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'preflight_ready: False' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'full_visual_gate_status: not_run_by_preflight_governance' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'controlled_visual_smoke_status: pass' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'controlled_visual_smoke_json_display: .\output\pdf-controlled-visual-smoke-20260520\controlled-visual-smoke-check-latest.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'Source failures: 1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_failure_count: 1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'readiness_action_evidence_count: 2' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'readiness_action_evidence:' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'provide_pdf_dependency_input' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'pdf_dependency_inputs_ready' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'PDFio source header' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'enable_pdf_build_option' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'FEATHERDOC_BUILD_PDF_IMPORT' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'project_template_onboarding_governance_contract:' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'schema_approval_status_summary: pending_review' -Label $document.Label
    Assert-MarkdownListBlockContainsAll -Path $document.Path -Anchor 'featherdoc.project_template_delivery_readiness_report.v1: status=blocked' -ExpectedFragments @(
        'source_report_display: .\output\project-template-delivery-readiness\summary.json',
        'source_json_display: .\output\project-template-delivery-readiness\summary.json',
        'project_template_delivery_readiness_contract:',
        'source_schema: featherdoc.project_template_delivery_readiness_report.v1',
        'status: blocked',
        'release_ready: False',
        'schema_approval_status_summary: pending_review'
    ) -Label $document.Label
    Assert-MarkdownListBlockContainsAll -Path $document.Path -Anchor 'project_template_onboarding.schema_approval: action=review_schema_update_candidate' -ExpectedFragments @(
        'source_schema=featherdoc.project_template_onboarding_governance_report.v1',
        'source_report_display: .\output\project-template-delivery-readiness\summary.json',
        'source_json_display: .\output\project-template-onboarding-governance\summary.json',
        'project_template_onboarding_governance_contract:',
        'schema_approval_status_summary: pending_review'
    ) -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'catalog_document_keys: contract-template,invoice-template,report-template,long-doc-template' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'baseline_document_keys: contract-template,invoice-template,report-template,long-doc-template' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'matched_document_keys: contract-template,invoice-template,report-template,long-doc-template' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report: .\output\pdf-visual-release-gate-preflight-governance\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json_display: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'Rollup Source Report Issues' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report: .\output\table-layout-delivery-governance\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json: .\output\table-layout-delivery-governance\unreadable-summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'error: Unexpected token while reading table layout governance summary.' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'build: pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'Handoff Report Issues' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report: .\output\project-template-delivery-readiness\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json: .\output\project-template-delivery-readiness\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'project_template_delivery_readiness_contract:' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_schema: featherdoc.project_template_delivery_readiness_report.v1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'latest_schema_approval_gate_status: pending_review' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'schema_approval_status_summary: pending_review' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'template_count: 3' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'release_blocker_count: 1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_report_display: .\output\project-template-delivery-readiness\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'source_json_display: .\output\project-template-delivery-readiness\summary.json' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'error: Failed to parse project template delivery readiness summary.' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText 'build: pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1' -Label $document.Label
    Assert-Contains -Path $document.Path -ExpectedText '<local-path>' -Label $document.Label
    Assert-NotContains -Path $document.Path -UnexpectedText 'C:\repo\tmp\pdfio-src\pdfio.h' -Label $document.Label
}
Assert-Contains -Path $bodyPath -ExpectedText '<local-path>' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText 'C:\repo\tmp\pdfio-src\pdfio.h' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $guidePath -ExpectedText 'publish_github_release.ps1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'publish_github_release.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'publish_github_release.ps1' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'release-refresh.yml' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-refresh.yml' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'release-refresh.yml' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'release-publish.yml' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-publish.yml' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'release-publish.yml' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Release Refresh' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release Publish' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $guidePath -ExpectedText 'Run workflow' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Actions' -Label 'START_HERE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-refresh-output' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-publish-output' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $guidePath -ExpectedText 'Smoke verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Section page setup verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Page number fields review task' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Template table CLI selector review task' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Project template schema approval history JSON' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'project_template_schema_approval_history.md' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Release blockers: 1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.document_skeleton_governance_rollup_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.content_control_data_binding_governance_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'repair_strategy: sync_bound_content_control' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText $contentControlCommandTemplateMarker -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.table_layout_delivery_governance_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\table-layout-delivery-governance\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'matched_document_count=4' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'catalog_coverage_percent=100' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText "style_numbering_issues(count=4, penalty=20)" -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'ready_document_percent=100' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'table_position_automatic_count=0' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'table_position_review_count=0' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'unresolved_item_count=0' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText "floating_table_plans_pending(count=0, penalty=0)" -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.schema_patch_confidence_calibration_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Item schema-review-invalid' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the project template schema approval history trend report' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blockers: 1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'document_skeleton.exemplar_catalog_missing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'repair_strategy: sync_bound_content_control' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText $contentControlCommandTemplateMarker -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'custom_xml_sync_evidence_missing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.table_layout_delivery_governance_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'matched_document_count=4' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'catalog_coverage_percent=100' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText "style_numbering_issues(count=4, penalty=20)" -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_json_display: .\output\table-layout-delivery-governance\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'ready_document_percent=100' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'table_position_automatic_count=0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'table_position_review_count=0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'unresolved_item_count=0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText "floating_table_plans_pending(count=0, penalty=0)" -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'schema_patch_confidence_calibration.unscored_candidates' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.schema_patch_confidence_calibration_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release Governance Handoff Details' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Handoff Action Items' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_visual_release_gate_preflight.build_outputs_missing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.pdf_visual_release_gate_preflight_governance_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_report_display: .\output\pdf-visual-release-gate-preflight-governance\summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_json_display: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'prepare_pdf_visual_release_gate_build_outputs' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Readiness action evidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'provide_pdf_dependency_input/pdf_dependency_inputs_ready -> PDFio source header' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'enable_pdf_build_option/pdf_build_options_enabled -> FEATHERDOC_BUILD_PDF_IMPORT' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source JSON: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'cmake_cache_exists' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'CMakeCache.txt' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'pdf_build_options_enabled' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText '-DFEATHERDOC_BUILD_PDF=ON' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText '-DFEATHERDOC_BUILD_PDF_IMPORT=ON' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'missing CLI PDFs=2' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'missing visual baseline PDFs=42' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'missing CJK text-layer PDFs=43' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'memory guard blocked=false' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'memory guard skipped=false' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'free memory MB=1140' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'minimum free memory MB=2048' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'PDF build options: enabled=false' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'disabled=FEATHERDOC_BUILD_PDF,FEATHERDOC_BUILD_PDF_IMPORT' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'not release-ready evidence' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Only after preflight is ready and workstation resources allow it' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release note bundle' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Stop here until' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'fix_schema_patch_approval_result' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'sync_project_template_schema_approval.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Resolve release governance blocker' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blocker rollup blockers' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open source report' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'while handling release governance blocker' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open source JSON' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review ' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'repair_hint' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release governance handoff blockers' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review release governance action item' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'build_dir_exists,cmake_cache_exists,ctest_manifest_exists,pdf_build_options_enabled' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blocker rollup action items' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Run ' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'for release governance action item' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'build_document_skeleton_governance_rollup_report.ps1 -InputRoot .\output\document-skeleton-governance' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release governance handoff action items' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'write_schema_patch_confidence_calibration_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Review release governance warning' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blocker rollup warnings' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release governance handoff warnings' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Do not approve for public release when release governance blocker counts are non-zero in the final rollup, governance handoff, or nested handoff rollup.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Do not approve for public release when' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'project_template_schema_approval_history.md' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Superseded review tasks: 0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm `superseded_review_tasks.json` reports zero stale task directories' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Smoke verdict: pass' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm the Word visual smoke verdict is signed off as `pass` in the gate summary.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm the fixed-grid visual verdict is signed off as `undetermined` in the gate summary.' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Page number fields verdict: pending_manual_review' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the Template table CLI selector review task if the release touches this curated visual bundle' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the page number fields review task if the release touches page numbers' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Release blockers: 1' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'project_template_smoke.schema_approval' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'repair_strategy: sync_bound_content_control' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'repair_hint: Rerun Custom XML sync or explicitly fill the bound content control before release.' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText $contentControlCommandTemplateMarker -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'table_layout_delivery_governance.delivery_quality' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'numbering_catalog_governance.real_corpus_confidence' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'matched_document_count=4' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'catalog_coverage_percent=100' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText "style_numbering_issues(count=4, penalty=20)" -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\table-layout-delivery-governance\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'ready_document_percent=100' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'table_position_automatic_count=0' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'table_position_review_count=0' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'unresolved_item_count=0' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText "floating_table_plans_pending(count=0, penalty=0)" -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\document-skeleton-governance\contract\style-numbering-audit.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\content-control-data-binding\inspect-content-controls.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Visual verdict: pending_manual_review' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Smoke verdict: pass' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Section page setup review task' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Template table CLI selector review task' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'open_latest_word_review_task.ps1 -SourceKind template-table-cli-selector-visual-regression-bundle -PrintPrompt' -Label 'START_HERE.md'

Assert-ContentControlGovernanceTrace -Path $handoffPath -Label 'release_handoff.md'
Assert-ContentControlGovernanceTrace -Path $guidePath -Label 'ARTIFACT_GUIDE.md'
Assert-ContentControlGovernanceTrace -Path $checklistPath -Label 'REVIEWER_CHECKLIST.md'
Assert-ContentControlGovernanceTrace -Path $startHerePath -Label 'START_HERE.md'

function Copy-ReleaseSummaryForNegativeCase {
    return ($summary | ConvertTo-Json -Depth 10 | ConvertFrom-Json)
}

function Assert-BundleRejectsSummary {
    param(
        [object]$CandidateSummary,
        [string]$FileName,
        [string[]]$ExpectedFragments
    )

    $candidateSummaryPath = Join-Path $reportDir $FileName
    ($CandidateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $candidateSummaryPath -Encoding UTF8

    $failed = $false
    $message = ""
    try {
        & $bundleScript -SummaryJson $candidateSummaryPath -SkipMaterialSafetyAudit
    } catch {
        $failed = $true
        $message = $_.Exception.Message
    }
    if (-not $failed) {
        throw "write_release_note_bundle.ps1 unexpectedly accepted invalid release blocker metadata: $FileName"
    }

    foreach ($expectedFragment in $ExpectedFragments) {
        if ($message -notmatch [regex]::Escape($expectedFragment)) {
            throw "write_release_note_bundle.ps1 failed with unexpected release blocker validation message: $message"
        }
    }
}

$mismatchedSummary = Copy-ReleaseSummaryForNegativeCase
$mismatchedSummary.release_blocker_count = 2
Assert-BundleRejectsSummary `
    -CandidateSummary $mismatchedSummary `
    -FileName "summary.release-blocker-count-mismatch.json" `
    -ExpectedFragments @(
        "release_blocker_count mismatch",
        "declared 2 but release_blockers contains 1 item(s)"
    )

$missingFieldSummary = Copy-ReleaseSummaryForNegativeCase
$missingFieldBlocker = @($missingFieldSummary.release_blockers)[0]
$missingFieldBlocker.action = ""
Assert-BundleRejectsSummary `
    -CandidateSummary $missingFieldSummary `
    -FileName "summary.release-blocker-missing-action.json" `
    -ExpectedFragments @("release_blockers[0].action must not be empty")

$missingRollupEvidenceSummary = Copy-ReleaseSummaryForNegativeCase
$missingRollupEvidenceBlocker = @($missingRollupEvidenceSummary.release_blocker_rollup.release_blockers)[0]
$missingRollupEvidenceBlocker.source_json_display = ""
Assert-BundleRejectsSummary `
    -CandidateSummary $missingRollupEvidenceSummary `
    -FileName "summary.release-rollup-missing-source-json-display.json" `
    -ExpectedFragments @("release_blocker_rollup.release_blockers[0].source_json_display must not be empty")

$missingHandoffCommandSummary = Copy-ReleaseSummaryForNegativeCase
$missingHandoffActionItem = @($missingHandoffCommandSummary.release_governance_handoff.action_items)[0]
$missingHandoffActionItem.open_command = ""
Assert-BundleRejectsSummary `
    -CandidateSummary $missingHandoffCommandSummary `
    -FileName "summary.release-handoff-missing-open-command.json" `
    -ExpectedFragments @("release_governance_handoff.action_items[0].open_command must not be empty")

$duplicateIdSummary = Copy-ReleaseSummaryForNegativeCase
$firstBlocker = @($duplicateIdSummary.release_blockers)[0]
$duplicateBlocker = $firstBlocker | ConvertTo-Json -Depth 10 | ConvertFrom-Json
$duplicateIdSummary.release_blockers = @($firstBlocker, $duplicateBlocker)
$duplicateIdSummary.release_blocker_count = 2
Assert-BundleRejectsSummary `
    -CandidateSummary $duplicateIdSummary `
    -FileName "summary.release-blocker-duplicate-id.json" `
    -ExpectedFragments @("duplicate release blocker id 'project_template_smoke.schema_approval'")

$unknownActionSummaryPath = Join-Path $reportDir "summary.release-blocker-unregistered-action.json"
$unknownActionSummary = Copy-ReleaseSummaryForNegativeCase
$unknownActionBlocker = @($unknownActionSummary.release_blockers)[0]
$unknownActionBlocker.id = "custom_gate.unregistered_action"
$unknownActionBlocker.source = "custom_gate"
$unknownActionBlocker.action = "investigate_custom_gate"
($unknownActionSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $unknownActionSummaryPath -Encoding UTF8
& $bundleScript -SummaryJson $unknownActionSummaryPath -SkipMaterialSafetyAudit
Assert-Contains -Path $checklistPath -ExpectedText 'Unregistered release blocker action `investigate_custom_gate`: add a fixed checklist runbook in `release_blocker_metadata_helpers.ps1`' -Label 'REVIEWER_CHECKLIST.md'

$bodyContent = Get-Content -Raw -LiteralPath $bodyPath
if ($bodyContent -match 'v1\.6\.1') {
    throw "release_body.zh-CN.md unexpectedly referenced the current development version: $bodyPath"
}

Write-Host "Release note bundle version pinning passed."
