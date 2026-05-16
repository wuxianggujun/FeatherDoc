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
$expectedSupersededReviewTasksReportDisplayPath = ".\" + `
    ($supersededReviewTasksReportPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/') -replace '/', '\')

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
        source_report_count = 4
        release_blocker_count = 4
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
            }
        )
        action_item_count = 4
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
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
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
                source_json_display = ".\output\content-control-data-binding-governance\summary.json"
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
Assert-Contains -Path $handoffPath -ExpectedText 'action=review_style_numbering_audit' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'action=sync_or_fill_bound_content_control' -Label 'release_handoff.md'
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
Assert-Contains -Path $handoffPath -ExpectedText $expectedSupersededReviewTasksReportDisplayPath -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_latest_page_number_fields_review_task.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'find_superseded_review_tasks.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $bodyPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\visual-validation' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Release blockers：1' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'project_template_smoke.schema_approval' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Smoke verdict：pass' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Fixed-grid verdict：undetermined' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Section page setup verdict：pass' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Page number fields verdict：pending_manual_review' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Template table CLI selector verdict：pass' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'Word visual gate 细分结论：smoke=`pass`，fixed-grid=`undetermined`，section page setup=`pass`，page number fields=`pending_manual_review`，Template table CLI selector=`pass`。' -Label 'release_summary.zh-CN.md'
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
Assert-Contains -Path $guidePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\project-template-onboarding-governance\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_schema=featherdoc.schema_patch_confidence_calibration_report.v1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'source_json_display: .\output\schema-patch-confidence-calibration\summary.json' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Item schema-review-invalid' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the project template schema approval history trend report' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release blockers: 1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'document_skeleton.style_numbering_issues' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'document_skeleton.exemplar_catalog_missing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'content_control_data_binding.bound_placeholder' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'custom_xml_sync_evidence_missing' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.project_template_onboarding_governance_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'schema_patch_confidence_calibration.unscored_candidates' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'source_schema=featherdoc.schema_patch_confidence_calibration_report.v1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Stop here until `release_blockers` is empty' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Use action `fix_schema_patch_approval_result`: update blocked `schema_patch_approval_result.json` record(s)' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Run `sync_project_template_schema_approval.ps1` after updating approval records' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Do not approve for public release when `release_blocker_count` is non-zero' -Label 'REVIEWER_CHECKLIST.md'
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
Assert-Contains -Path $startHerePath -ExpectedText 'project_template_onboarding.schema_approval' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'schema_patch_confidence_calibration.pending_schema_approvals' -Label 'START_HERE.md'
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
