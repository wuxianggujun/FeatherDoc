$candidateOutputDir = Join-Path $resolvedWorkingDir "release-candidate-checks"
$candidateBuildDir = Join-Path $resolvedWorkingDir "build"
$candidateInstallDir = Join-Path $resolvedWorkingDir "install"
$candidateConsumerBuildDir = Join-Path $resolvedWorkingDir "consumer-build"
$candidateGateOutputDir = Join-Path $resolvedWorkingDir "word-visual-gate"
$candidateTaskOutputRoot = Join-Path $resolvedWorkingDir "visual-tasks"
$releaseGovernanceHandoffInputRoot = Join-Path $resolvedWorkingDir "release-governance-handoff-input"
$releaseGovernanceHandoffOutputDir = Join-Path $resolvedWorkingDir "release-governance-handoff"
$releaseGovernanceSourceDir = Join-Path $resolvedWorkingDir "release-candidate-checks-source"
$releaseGovernanceSourcePath = Join-Path $releaseGovernanceSourceDir "summary.json"
$candidateSummaryPath = Join-Path $candidateOutputDir "report\summary.json"
$candidateReuseMarkerPath = Join-Path $resolvedWorkingDir "release-candidate-visual-verdict-marker.json"
$candidateReuseKey = [ordered]@{
    status = "complete"
    script_last_write_utc = (Get-Item -LiteralPath $scriptPath).LastWriteTimeUtc.Ticks
    test_last_write_utc = (Get-Item -LiteralPath $MyInvocation.MyCommand.Path).LastWriteTimeUtc.Ticks
    import_diagnostics_contract_fields_text = (@(Get-PdfImportDiagnosticsContractFields) -join "`n")
}
New-Item -ItemType Directory -Path $releaseGovernanceHandoffInputRoot -Force | Out-Null
New-Item -ItemType Directory -Path $releaseGovernanceSourceDir -Force | Out-Null
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "numbering-catalog-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.numbering_catalog_governance_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
        real_corpus_confidence_score = 100
        real_corpus_confidence_level = "high"
        real_corpus_confidence = [ordered]@{
            document_count = 2
            catalog_exemplar_count = 2
            baseline_entry_count = 2
            matched_document_count = 2
            unmatched_catalog_document_count = 0
            unmatched_baseline_document_count = 0
            alignment_gap_count = 0
            catalog_coverage_percent = 100
            baseline_coverage_percent = 100
            coverage_score = 100
            catalog_document_keys = @("catalog.invoice", "catalog.statement")
            baseline_document_keys = @("catalog.invoice", "catalog.statement")
            matched_document_keys = @("catalog.invoice", "catalog.statement")
            penalty_summary = @(
                [ordered]@{
                    factor = "style_numbering_issues"
                    count = 0
                    penalty = 0
                }
            )
            style_numbering_issues = @()
        }
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "table-layout-delivery-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.table_layout_delivery_governance_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
        delivery_quality_score = 100
        delivery_quality_level = "high"
        delivery_quality = [ordered]@{
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
            pdf_floating_table_support_coverage = "4/9 supported (44 percent); metadata_only=5"
            pdf_floating_table_reviewer_focus = "review metadata-only tblpPr fields before approving PDF-layout-sensitive release."
            metadata_only_fields = @(
                "leftFromText",
                "rightFromText",
                "topFromText outside paragraph anchoring",
                "tblOverlap"
            )
            review_required_fields = @(
                "full Word-compatible floating table text wrapping",
                "table overlap avoidance and collision resolution"
            )
            command_failure_count = 0
            unresolved_item_count = 0
            penalty_summary = @(
                [ordered]@{
                    factor = "floating_table_plans_pending"
                    count = 0
                    penalty = 0
                }
            )
            floating_table_plans_pending = 0
        }
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "content-control-data-binding-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.content_control_data_binding_governance_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "project-template-delivery-readiness\summary.json") -Value ([ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        status = "ready"
        release_ready = $true
        latest_schema_approval_gate_status = "not_required"
        schema_approval_status_summary = @(
            [ordered]@{
                status = "not_required"
                count = 3
            }
        )
        template_count = 3
        ready_template_count = 3
        blocked_template_count = 0
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
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
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "project-template-onboarding-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = "ready"
        release_ready = $true
        schema_approval_status_summary = @(
            [ordered]@{
                status = "not_required"
                count = 3
            }
        )
        entry_count = 3
        blocked_entry_count = 0
        pending_review_entry_count = 0
        not_evaluated_entry_count = 0
        approved_entry_count = 0
        not_required_entry_count = 3
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
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
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "schema-patch-confidence-calibration\summary.json") -Value ([ordered]@{
        schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
        status = "needs_review"
        release_ready = $false
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "add_business_template_document_type_metadata"
                action = "add_business_template_document_type_metadata"
                title = "Add schema patch candidate business document type metadata"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
                project_id = "project-legal"
                template_name = "contract-template"
                missing_business_document_type_count = 1
                source_business_document_type = "contract"
                corpus_role = "registered-business-template"
                source_corpus_role = "registered-business-template"
                business_document_type_mismatch = $false
                corpus_role_mismatch = $false
                missing_corpus_role_count = 0
                mismatched_corpus_metadata_count = 0
                mismatched_business_document_type_count = 0
                mismatched_corpus_role_count = 0
                candidate_type = "rename"
                candidate_name = "contract.customer_name"
                schema_update_candidate = "customer_name"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report = "output/schema-patch-confidence-calibration/summary.json"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json = "output/schema-patch-confidence-calibration/summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            },
            [ordered]@{
                id = "add_business_template_corpus_role_metadata"
                action = "add_business_template_corpus_role_metadata"
                title = "Add schema patch candidate corpus role metadata"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
                project_id = "project-policy"
                template_name = "policy-template"
                business_document_type = "policy"
                missing_corpus_role_count = 1
                source_business_document_type = "policy"
                source_corpus_role = "planned-business-template"
                business_document_type_mismatch = $false
                corpus_role_mismatch = $false
                missing_business_document_type_count = 0
                mismatched_corpus_metadata_count = 0
                mismatched_business_document_type_count = 0
                mismatched_corpus_role_count = 0
                candidate_type = "add"
                candidate_name = "policy.effective_date"
                schema_update_candidate = "effective_date"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report = "output/schema-patch-confidence-calibration/summary.json"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json = "output/schema-patch-confidence-calibration/summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            },
            [ordered]@{
                id = "align_business_template_corpus_metadata"
                action = "align_business_template_corpus_metadata"
                title = "Align schema patch candidate corpus metadata"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
                project_id = "project-office"
                template_name = "office-notice-template"
                business_document_type = "invoice"
                source_business_document_type = "notice"
                corpus_role = "experimental-business-template"
                source_corpus_role = "registered-business-template"
                mismatched_corpus_metadata_count = 1
                mismatched_business_document_type_count = 1
                mismatched_corpus_role_count = 1
                candidate_type = "add"
                candidate_name = "notice.invoice_number"
                schema_update_candidate = "invoice_number"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report = "output/schema-patch-confidence-calibration/summary.json"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json = "output/schema-patch-confidence-calibration/summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            }
        )
        warning_count = 3
        warnings = @(
            [ordered]@{
                id = "schema_patch_confidence_calibration.missing_business_document_type_metadata"
                action = "add_business_template_document_type_metadata"
                message = "Some schema patch candidates are missing business document type metadata."
                missing_business_document_type_count = 1
                project_id = "project-legal"
                template_name = "contract-template"
                source_business_document_type = "contract"
                corpus_role = "registered-business-template"
                source_corpus_role = "registered-business-template"
                business_document_type_mismatch = $false
                corpus_role_mismatch = $false
                missing_corpus_role_count = 0
                mismatched_corpus_metadata_count = 0
                mismatched_business_document_type_count = 0
                mismatched_corpus_role_count = 0
                candidate_type = "rename"
                candidate_name = "contract.customer_name"
                schema_update_candidate = "customer_name"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report = "output/schema-patch-confidence-calibration/summary.json"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json = "output/schema-patch-confidence-calibration/summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            },
            [ordered]@{
                id = "schema_patch_confidence_calibration.missing_business_template_corpus_role_metadata"
                action = "add_business_template_corpus_role_metadata"
                message = "Some schema patch candidates are missing business template corpus role metadata."
                missing_corpus_role_count = 1
                project_id = "project-policy"
                template_name = "policy-template"
                business_document_type = "policy"
                source_business_document_type = "policy"
                source_corpus_role = "planned-business-template"
                business_document_type_mismatch = $false
                corpus_role_mismatch = $false
                missing_business_document_type_count = 0
                mismatched_corpus_metadata_count = 0
                mismatched_business_document_type_count = 0
                mismatched_corpus_role_count = 0
                candidate_type = "add"
                candidate_name = "policy.effective_date"
                schema_update_candidate = "effective_date"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report = "output/schema-patch-confidence-calibration/summary.json"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json = "output/schema-patch-confidence-calibration/summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            },
            [ordered]@{
                id = "schema_patch_confidence_calibration.mismatched_business_template_corpus_metadata"
                action = "align_business_template_corpus_metadata"
                message = "Some schema patch candidates disagree with their source business template corpus metadata."
                mismatched_corpus_metadata_count = 1
                mismatched_business_document_type_count = 1
                mismatched_corpus_role_count = 1
                business_document_type = "invoice"
                source_business_document_type = "notice"
                corpus_role = "experimental-business-template"
                source_corpus_role = "registered-business-template"
                business_document_type_mismatch = $true
                corpus_role_mismatch = $true
                project_id = "project-office"
                template_name = "office-notice-template"
                candidate_type = "add"
                candidate_name = "notice.invoice_number"
                schema_update_candidate = "invoice_number"
                source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                source_report = "output/schema-patch-confidence-calibration/summary.json"
                source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                source_json = "output/schema-patch-confidence-calibration/summary.json"
                source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
            }
        )
        source_failure_count = 0
    })
Write-TestJson -Path (Join-Path $releaseGovernanceHandoffInputRoot "docx-functional-smoke-readiness\summary.json") -Value ([ordered]@{
        schema = "featherdoc.docx_functional_smoke_readiness.v1"
        status = "pass"
        verdict = "pass"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        source_failure_count = 0
    })
([ordered]@{
        schema = "featherdoc.release_candidate_summary"
        status = "ready"
        release_ready = $true
        project_template_smoke = [ordered]@{
            status = "ready"
        }
        project_template_readiness_checklist_entrypoints = [ordered]@{
            status = "declared"
            checklist_label = "Project template release readiness checklist"
            checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
            required_entrypoint_count = 3
            entrypoints = @(
                [ordered]@{
                    id = "start_here"
                    required = $true
                    path_display = ".\output\release-candidate-checks\START_HERE.md"
                },
                [ordered]@{
                    id = "artifact_guide"
                    required = $true
                    path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                },
                [ordered]@{
                    id = "reviewer_checklist"
                    required = $true
                    path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
                }
            )
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
        }
        release_entry_project_template_readiness_checklist_material_safety_audit = [ordered]@{
            status = "passed"
            audit_script = ".\scripts\assert_release_material_safety.ps1"
            audited_entrypoint_count = 3
            audited_entrypoints = @(
                "start_here",
                "artifact_guide",
                "reviewer_checklist"
            )
            compact_evidence_label = "Project-template readiness checklist handoff evidence"
            compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
            compact_evidence_source_schema = "featherdoc.release_candidate_summary"
            checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
            material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
        }
        word_visual_standard_review_metadata_count = 4
        word_visual_standard_review_task_keys = @(
            "smoke",
            "fixed_grid",
            "section_page_setup",
            "page_number_fields"
        )
        word_visual_standard_review_status_summary = "reviewed=4"
        word_visual_standard_review_verdict_summary = "pass=4"
        word_visual_standard_review_metadata = @(
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
                review_note = "Private operator note"
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
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
    } | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $releaseGovernanceSourcePath -Encoding UTF8

$shouldRunCandidate = $true
if ($Scenario -eq "candidate_reports" -and
    (Test-Path -LiteralPath $candidateSummaryPath -PathType Leaf) -and
    (Test-Path -LiteralPath $candidateReuseMarkerPath -PathType Leaf)) {
    $reuseMarker = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReuseMarkerPath | ConvertFrom-Json
    $reuseStatusProperty = $reuseMarker.PSObject.Properties["status"]
    $reuseStatus = if ($null -ne $reuseStatusProperty) { [string]$reuseStatusProperty.Value } else { "" }
    $reuseImportDiagnosticsFieldsProperty =
        $reuseMarker.PSObject.Properties["import_diagnostics_contract_fields_text"]
    $reuseImportDiagnosticsFieldsText =
        if ($null -ne $reuseImportDiagnosticsFieldsProperty) {
            [string]$reuseImportDiagnosticsFieldsProperty.Value
        } else {
            ""
        }
    if ($reuseStatus -eq "complete" -and
        [string]$reuseMarker.script_last_write_utc -eq [string]$candidateReuseKey.script_last_write_utc -and
        [string]$reuseMarker.test_last_write_utc -eq [string]$candidateReuseKey.test_last_write_utc -and
        $reuseImportDiagnosticsFieldsText -eq
            [string]$candidateReuseKey.import_diagnostics_contract_fields_text) {
        $shouldRunCandidate = $false
    }
}

if ($shouldRunCandidate) {
    $candidateRunningKey = [ordered]@{
        status = "running"
        script_last_write_utc = $candidateReuseKey.script_last_write_utc
        test_last_write_utc = $candidateReuseKey.test_last_write_utc
        import_diagnostics_contract_fields_text =
            $candidateReuseKey.import_diagnostics_contract_fields_text
        started_at_utc = [DateTime]::UtcNow.ToString("o")
    }
    Write-TestJson -Path $candidateReuseMarkerPath -Value $candidateRunningKey

    & $scriptPath `
        -SkipConfigure `
        -SkipBuild `
        -SkipTests `
        -SkipInstallSmoke `
        -SkipVisualGate `
        -SkipReviewTasks `
        -BuildDir $candidateBuildDir `
        -InstallDir $candidateInstallDir `
        -ConsumerBuildDir $candidateConsumerBuildDir `
        -GateOutputDir $candidateGateOutputDir `
        -TaskOutputRoot $candidateTaskOutputRoot `
        -SummaryOutputDir $candidateOutputDir `
        -ReleaseGovernanceHandoff `
        -ReleaseGovernanceHandoffInputRoot $releaseGovernanceHandoffInputRoot `
        -ReleaseGovernanceHandoffInputJson $releaseGovernanceSourcePath `
        -ReleaseGovernanceHandoffOutputDir $releaseGovernanceHandoffOutputDir `
        -ReleaseGovernanceHandoffIncludeRollup `
        -PdfVisualGateSummaryJson $pdfSummaryPath `
        -PdfVisualGateAttemptSummaryJson $pdfAttemptSummaryPath `
        -PdfVisualSegmentedGateSummaryJson $pdfSegmentedSummaryPath `
        -PdfBoundedCtestSummaryJson @($boundedSmokePath, $boundedBusinessPath) `
        -PdfReleaseReadinessSummaryJson $pdfReadinessSummaryPath
    if ($LASTEXITCODE -ne 0) {
        throw "Release candidate dry run with PDF visual gate summary failed with exit code $LASTEXITCODE."
    }
    Write-TestJson -Path $candidateReuseMarkerPath -Value $candidateReuseKey
} else {
    Write-Host "Reusing release candidate visual verdict artifacts from candidate_core."
}

if (-not (Test-Path -LiteralPath $candidateSummaryPath)) {
    throw "Release candidate dry run did not write summary.json."
}

$candidateSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateSummaryPath | ConvertFrom-Json
$expectedPdfSummaryPath = ".\pdf-summary\summary.json"
$expectedPdfContactSheetPath = ".\pdf-summary\aggregate-contact-sheet.png"
$expectedPdfAttemptSummaryPath = ".\pdf-summary\attempt-summary.json"
$expectedPdfSegmentedSummaryPath = ".\pdf-summary\segmented-summary.json"
$expectedPdfReadinessSummaryPath = ".\pdf-summary\release-readiness-summary.json"
$expectedFullPdfCtestSummaryPath = ".\pdf-bounded-ctest\full-pdf-ctest-summary.json"
$expectedReleaseEntryChecklistSourceReport = ".\release-candidate-checks\report\summary.json"
$expectedReleaseEntryPackagedAuditSummaryReport = ".\release-candidate-checks-source\summary.json"
$expectedReleaseEntryPackagedAuditSourceReport = ".\release-governance-handoff\release-blocker-rollup\release_blocker_rollup.md"
