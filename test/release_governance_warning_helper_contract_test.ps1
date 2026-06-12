param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "release_governance_warning_helper_contract_test_helpers.ps1")

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$helpersPath = Join-Path $resolvedRepoRoot "scripts\release_blocker_metadata_helpers.ps1"
Assert-ScriptParses -Path $helpersPath
. $helpersPath


Assert-ReleaseGovernanceWarningNormalizationContracts

$sectionLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceWarningsMarkdownSection `
    -Lines $sectionLines `
    -Summary ([pscustomobject]@{
        release_blocker_rollup = [pscustomobject]@{
            warning_count = 1
            warnings = @($warningWithStyleMergeCount)
        }
        release_governance_handoff = [pscustomobject]@{
            warning_count = 1
            warnings = @($warningWithoutStyleMergeCount)
            release_blocker_rollup = [pscustomobject]@{
                warning_count = 1
                warnings = @($warningWithStyleMergeCount)
            }
        }
    })
$sectionMarkdown = $sectionLines -join "`n"
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "## Release Governance Warnings" `
    -Message "Markdown section should include the top-level warning heading."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "### Release blocker rollup warnings" `
    -Message "Markdown section should include the release blocker subsection."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "### Release governance handoff warnings" `
    -Message "Markdown section should include the handoff subsection."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "### Release governance handoff nested rollup warnings" `
    -Message "Markdown section should include the nested handoff rollup subsection."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "Numbering catalog report is missing source metadata." `
    -Message "Markdown section should render plain warning messages."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText 'source_report: `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Markdown section should render warning source report displays."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText 'style_merge_suggestion_count: `2`' `
    -Message "Markdown section should render optional style merge counts."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText 'style_merge_suggestion_pending_count: `2`' `
    -Message "Markdown section should render optional pending style merge counts."

$blockerSectionLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceBlockersMarkdownSection `
    -Lines $blockerSectionLines `
    -Summary ([pscustomobject]@{
        release_blocker_rollup = [pscustomobject]@{
            release_blocker_count = 1
            release_blockers = @($styleNumberingBlocker)
        }
        release_governance_handoff = [pscustomobject]@{
            release_blocker_count = 1
            release_blockers = @($styleMergeBlocker)
            release_blocker_rollup = [pscustomobject]@{
                release_blocker_count = 1
                release_blockers = @($styleNumberingBlocker)
            }
        }
    })
$blockerSectionMarkdown = $blockerSectionLines -join "`n"
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText "## Release Governance Blockers" `
    -Message "Markdown section should include the top-level governance blocker heading."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText "### Release blocker rollup blockers" `
    -Message "Markdown section should include the release blocker rollup subsection."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText "### Release governance handoff blockers" `
    -Message "Markdown section should include the handoff blocker subsection."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText "### Release governance handoff nested rollup blockers" `
    -Message "Markdown section should include the nested handoff rollup blocker subsection."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText 'action: `review_style_numbering_audit`' `
    -Message "Markdown section should render rollup blocker actions."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText 'source_report_display: `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Markdown section should render blocker source report displays."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText 'source_report: `output/numbering-catalog-governance/summary.json`' `
    -Message "Markdown section should render blocker raw source reports."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText 'source_json: `output/numbering-catalog-governance/style-numbering-audit.json`' `
    -Message "Markdown section should render blocker raw source JSON paths."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText 'id: `style_merge.restore_audit_issues`' `
    -Message "Markdown section should render handoff blocker ids when composite ids are absent."

$blockerChecklistItems = @(Get-ReleaseGovernanceBlockerChecklistItems -Summary ([pscustomobject]@{
            release_blocker_rollup = [pscustomobject]@{
                release_blocker_count = 1
                release_blockers = @($styleNumberingBlocker)
            }
            release_governance_handoff = [pscustomobject]@{
                release_blocker_count = 1
                release_blockers = @($styleMergeBlocker)
                release_blocker_rollup = [pscustomobject]@{
                    release_blocker_count = 1
                    release_blockers = @($styleNumberingBlocker)
                }
            }
        }))
Assert-Equal -Actual $blockerChecklistItems.Count -Expected 3 `
    -Message "Checklist items should preserve blockers from every release governance source."
$blockerChecklistText = Get-ReleaseGovernanceBlockerChecklistText -BlockerItem $blockerChecklistItems[0]
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'Resolve release governance blocker `numbering_catalog_governance.style_numbering_issues`' `
    -Message "Blocker checklist text should include blocker id."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'action `review_style_numbering_audit`' `
    -Message "Blocker checklist text should include blocker action."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'source_schema `featherdoc.numbering_catalog_governance_report.v1`' `
    -Message "Blocker checklist text should include source schema."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'composite_id `source0.blocker0.numbering_catalog_governance.style_numbering_issues`' `
    -Message "Blocker checklist text should include rollup composite id."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'project_id `project-finance`' `
    -Message "Blocker checklist text should include project id."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'template_name `invoice-template`' `
    -Message "Blocker checklist text should include template name."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'candidate_type `rename`' `
    -Message "Blocker checklist text should include candidate type."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'source_report `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Blocker checklist text should include source report display."

$blockerGuidance = (Get-ReleaseGovernanceBlockerActionGuidanceLines `
        -Blocker $styleNumberingBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "summary.json")) -join "`n"
Assert-ContainsText -Text $blockerGuidance -ExpectedText 'Open source report `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Blocker guidance should point reviewers at the source report."
Assert-ContainsText -Text $blockerGuidance -ExpectedText 'blocker action `review_style_numbering_audit`' `
    -Message "Blocker guidance should include the blocker action."

$actionSectionLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceActionItemsMarkdownSection `
    -Lines $actionSectionLines `
    -Summary ([pscustomobject]@{
        release_blocker_rollup = [pscustomobject]@{
            action_item_count = 1
            action_items = @($restoreAuditActionItem)
        }
        release_governance_handoff = [pscustomobject]@{
            action_item_count = 1
            action_items = @($restoreAuditActionItem)
            release_blocker_rollup = [pscustomobject]@{
                action_item_count = 1
                action_items = @($restoreAuditActionItem)
            }
        }
    })
$actionSectionMarkdown = $actionSectionLines -join "`n"
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "## Release Governance Action Items" `
    -Message "Markdown section should include the top-level action item heading."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "### Release blocker rollup action items" `
    -Message "Markdown section should include the release blocker action subsection."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "### Release governance handoff action items" `
    -Message "Markdown section should include the handoff action subsection."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "### Release governance handoff nested rollup action items" `
    -Message "Markdown section should include the nested handoff action subsection."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Markdown section should render restore audit helper commands."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "repair_strategy: review_style_merge_restore_audit" `
    -Message "Markdown section should render action item repair strategy."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "command_template: featherdoc_cli restore-style-merge <input.docx> --rollback-plan <rollback.json> --dry-run --json" `
    -Message "Markdown section should render action item command template."

$rollupDetailLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceRollupMarkdownSection `
    -Lines $rollupDetailLines `
    -Summary ([pscustomobject]@{
        release_blocker_rollup = [pscustomobject]@{
            requested = $true
            status = "blocked"
            source_report_count = 5
            source_failure_count = 1
            governance_metrics = @($numberingGovernanceMetric)
            release_blockers = @($styleNumberingBlocker)
            blocker_source_schema_summary = @(
                [pscustomobject]@{
                    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
                    count = 1
                }
            )
            warnings = @($warningWithStyleMergeCount)
            warning_source_schema_summary = @(
                [pscustomobject]@{
                    source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                    count = 1
                }
            )
            action_items = @($restoreAuditActionItem)
            action_item_source_schema_summary = @(
                [pscustomobject]@{
                    source_schema = "featherdoc.style_merge_restore_audit.v1"
                    count = 1
                }
            )
            informational_action_item_source_schema_summary = @()
        }
    }) `
    -RepoRoot $resolvedRepoRoot
$rollupDetailMarkdown = $rollupDetailLines -join "`n"
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText "- Source reports: 5" `
    -Message "Rollup detail Markdown should include source report counts."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText "- Source failures: 1" `
    -Message "Rollup detail Markdown should include source failure counts."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText "- source_failure_count: 1" `
    -Message "Rollup detail Markdown should expose a machine-readable source failure count."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText "- Blocker source schemas: featherdoc.numbering_catalog_governance_report.v1=1" `
    -Message "Rollup detail Markdown should render blocker source schema summary."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText "- Action item source schemas: featherdoc.style_merge_restore_audit.v1=1" `
    -Message "Rollup detail Markdown should render action item source schema summary."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText "- Informational action item source schemas: (none)" `
    -Message "Rollup detail Markdown should render empty informational action source schema summary."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText "- Warning source schemas: featherdoc.document_skeleton_governance_rollup_report.v1=1" `
    -Message "Rollup detail Markdown should render warning source schema summary."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText 'source_report: output/numbering-catalog-governance/summary.json' `
    -Message "Rollup metric Markdown should render raw source report paths."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText 'source_json: output/numbering-catalog-governance/coverage.json' `
    -Message "Rollup metric Markdown should render raw source JSON paths."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText 'source_report_display: .\output\numbering-catalog-governance\summary.json' `
    -Message "Rollup metric Markdown should render source report display paths."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\coverage.json' `
    -Message "Rollup metric Markdown should render source JSON display paths."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText 'details: matched_document_count=1' `
    -Message "Rollup metric Markdown should render alignment detail counts."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText 'catalog_document_keys: contract,invoice' `
    -Message "Rollup metric Markdown should render catalog document keys."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText 'baseline_document_keys: invoice,obsolete' `
    -Message "Rollup metric Markdown should render baseline document keys."
Assert-ContainsText -Text $rollupDetailMarkdown -ExpectedText 'matched_document_keys: invoice' `
    -Message "Rollup metric Markdown should render matched document keys."

$wordVisualStandardReviewMetadataSourceReport = [pscustomobject]@{
    schema = "featherdoc.release_candidate_summary"
    path_display = ".\output\release-candidate-checks\summary.json"
    word_visual_standard_review_metadata_count = 4
    word_visual_standard_review_task_keys = @("smoke", "fixed_grid", "section_page_setup", "page_number_fields")
    word_visual_standard_review_status_summary = "reviewed=4"
    word_visual_standard_review_verdict_summary = "pass=4"
    word_visual_standard_review_metadata = @(
        [pscustomobject]@{
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
        [pscustomobject]@{
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
        [pscustomobject]@{
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
        [pscustomobject]@{
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
}

$handoffDetailLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceHandoffMarkdownSection `
    -Lines $handoffDetailLines `
    -Summary ([pscustomobject]@{
        release_governance_handoff = [pscustomobject]@{
            requested = $true
            status = "blocked"
            loaded_report_count = 4
            expected_report_count = 5
            missing_report_count = 0
            failed_report_count = 1
            governance_metrics = @($numberingGovernanceMetric)
            release_blockers = @($styleMergeBlocker)
            warnings = @($warningWithoutStyleMergeCount)
            action_items = @($restoreAuditActionItem)
            manifest_signoff_entrypoints_source_report_count = 1
            manifest_signoff_entrypoints_source_reports = @(
                [pscustomobject]@{
                    schema = "featherdoc.release_candidate_summary"
                    path_display = ".\output\release-candidate-checks\report\summary.json"
                    manifest_signoff_entrypoints_status = "declared"
                    manifest_signoff_entrypoints_release_assets_manifest = "output\release-assets\v1.6.4\release_assets_manifest.json"
                    manifest_signoff_entrypoints_release_assets_manifest_display = ".\output\release-assets\v1.6.4\release_assets_manifest.json"
                    manifest_signoff_entrypoints_required_entrypoint_count = 3
                    manifest_signoff_entrypoints_entrypoint_ids = @("start_here", "artifact_guide", "reviewer_checklist")
                    manifest_signoff_entrypoints_required_contracts = @(
                        "project_template_delivery_readiness_contract",
                        "project_template_onboarding_governance_contract"
                    )
                    manifest_signoff_entrypoints_required_fields = @(
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
                        "source_report_display",
                        "source_json_display"
                    )
                    manifest_signoff_entrypoints_checklist_marker = "reviewer_manifest_scoped_project_template_trace"
                }
            )
            project_template_readiness_checklist_entrypoints_source_report_count = 1
            project_template_readiness_checklist_entrypoints_source_reports = @(
                [pscustomobject]@{
                    schema = "featherdoc.release_candidate_summary"
                    path_display = ".\output\release-candidate-checks\report\summary.json"
                    project_template_readiness_checklist_entrypoints_status = "declared"
                    project_template_readiness_checklist_entrypoints_checklist_label = "Project template release readiness checklist"
                    project_template_readiness_checklist_entrypoints_checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
                    project_template_readiness_checklist_entrypoints_required_entrypoint_count = 3
                    project_template_readiness_checklist_entrypoints_entrypoint_ids = @("start_here", "artifact_guide", "reviewer_checklist")
                    project_template_readiness_checklist_entrypoints_entrypoints = @(
                        [pscustomobject]@{
                            id = "start_here"
                            required = $true
                            path_display = ".\output\release-candidate-checks\START_HERE.md"
                        },
                        [pscustomobject]@{
                            id = "artifact_guide"
                            required = $true
                            path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                        },
                        [pscustomobject]@{
                            id = "reviewer_checklist"
                            required = $true
                            path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
                        }
                    )
                    project_template_readiness_checklist_entrypoints_checklist_marker = "release_entry_project_template_readiness_checklist_trace"
                }
            )
            release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 1
            release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @(
                [pscustomobject]@{
                    schema = "featherdoc.release_candidate_summary"
                    path_display = ".\output\release-candidate-checks\report\summary.json"
                    release_entry_project_template_readiness_checklist_material_safety_audit_status = "passed"
                    release_entry_project_template_readiness_checklist_material_safety_audit_script = ".\scripts\assert_release_material_safety.ps1"
                    release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count = 3
                    release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints = @("start_here", "artifact_guide", "reviewer_checklist")
                    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label = "Project-template readiness checklist handoff evidence"
                    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
                    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema = "featherdoc.release_candidate_summary"
                    release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
                    release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker = "release_entry_project_template_readiness_checklist_trace"
                    release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
                }
            )
            word_visual_standard_review_metadata_source_report_count = 1
            word_visual_standard_review_metadata_source_reports = @($wordVisualStandardReviewMetadataSourceReport)
        }
    }) `
    -RepoRoot $resolvedRepoRoot
$handoffDetailMarkdown = $handoffDetailLines -join "`n"
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "- Reports loaded: 4 / 5" `
    -Message "Handoff detail Markdown should include report load counts."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "- Missing reports: 0" `
    -Message "Handoff detail Markdown should include missing report counts."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "- Failed reports: 1" `
    -Message "Handoff detail Markdown should include failed report counts."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText 'source_report: output/numbering-catalog-governance/summary.json' `
    -Message "Handoff metric Markdown should render raw source report paths."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText 'source_json: output/numbering-catalog-governance/coverage.json' `
    -Message "Handoff metric Markdown should render raw source JSON paths."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText 'source_report_display: .\output\numbering-catalog-governance\summary.json' `
    -Message "Handoff metric Markdown should render source report display paths."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText 'source_json_display: .\output\numbering-catalog-governance\coverage.json' `
    -Message "Handoff metric Markdown should render source JSON display paths."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "Manifest signoff entrypoints evidence source reports: 1" `
    -Message "Handoff detail Markdown should render manifest signoff evidence count."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "manifest_signoff_entrypoints_status: declared" `
    -Message "Handoff detail Markdown should render manifest signoff status."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "manifest_signoff_entrypoints_release_assets_manifest_display: .\output\release-assets\v1.6.4\release_assets_manifest.json" `
    -Message "Handoff detail Markdown should render release assets manifest display path."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "manifest_signoff_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist" `
    -Message "Handoff detail Markdown should render manifest signoff entrypoint ids."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "manifest_signoff_entrypoints_required_fields: status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, source_report_display, source_json_display" `
    -Message "Handoff detail Markdown should render the full manifest signoff required field contract."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "reviewer_manifest_scoped_project_template_trace" `
    -Message "Handoff detail Markdown should render manifest signoff checklist marker."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "Project-template readiness checklist entrypoints evidence source reports: 1" `
    -Message "Handoff detail Markdown should render project-template readiness checklist evidence count."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "project_template_readiness_checklist_entrypoints_status: declared" `
    -Message "Handoff detail Markdown should render project-template readiness checklist status."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "project_template_readiness_checklist_entrypoints_checklist_path: docs/project_template_release_readiness_checklist_zh.rst" `
    -Message "Handoff detail Markdown should render project-template readiness checklist path."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "project_template_readiness_checklist_entrypoints_required_entrypoint_count: 3" `
    -Message "Handoff detail Markdown should render project-template readiness checklist required entrypoint count."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "project_template_readiness_checklist_entrypoints_entrypoint_ids: start_here, artifact_guide, reviewer_checklist" `
    -Message "Handoff detail Markdown should render project-template readiness checklist entrypoint ids."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "start_here: required=True path_display=.\output\release-candidate-checks\START_HERE.md" `
    -Message "Handoff detail Markdown should render project-template readiness checklist START_HERE path."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "artifact_guide: required=True path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md" `
    -Message "Handoff detail Markdown should render project-template readiness checklist artifact guide path."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "reviewer_checklist: required=True path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md" `
    -Message "Handoff detail Markdown should render project-template readiness checklist reviewer checklist path."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "release_entry_project_template_readiness_checklist_trace" `
    -Message "Handoff detail Markdown should render project-template readiness checklist marker."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "Release-entry project-template readiness checklist material-safety audit source reports: 1" `
    -Message "Handoff detail Markdown should render release-entry checklist material-safety audit evidence count."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_status: passed" `
    -Message "Handoff detail Markdown should render release-entry checklist material-safety audit status."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist" `
    -Message "Handoff detail Markdown should render release-entry checklist material-safety audited entrypoints."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary" `
    -Message "Handoff detail Markdown should render release-entry checklist material-safety compact evidence source schema."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
    -Message "Handoff detail Markdown should render release-entry checklist material-safety marker."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "Word visual standard review metadata source reports: 1" `
    -Message "Handoff detail Markdown should render Word visual metadata evidence count."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "word_visual_standard_review_metadata_count: 4" `
    -Message "Handoff detail Markdown should render Word visual metadata count."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "word_visual_standard_review_task_keys: smoke, fixed_grid, section_page_setup, page_number_fields" `
    -Message "Handoff detail Markdown should render Word visual standard task keys."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "word_visual_standard_review_status_summary: reviewed=4" `
    -Message "Handoff detail Markdown should render Word visual review status summary."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "word_visual_standard_review_verdict_summary: pass=4" `
    -Message "Handoff detail Markdown should render Word visual review verdict summary."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "smoke: review_task_key=document label=Word visual smoke verdict=pass review_status=reviewed review_method=operator_supplied" `
    -Message "Handoff detail Markdown should render Word visual smoke review metadata."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "fixed_grid: review_task_key=fixed_grid label=Fixed-grid merge/unmerge verdict=pass review_status=reviewed review_method=operator_supplied" `
    -Message "Handoff detail Markdown should render fixed-grid review metadata."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "review_result.json" `
    -Message "Handoff detail Markdown should render Word visual review result paths."
Assert-ContainsText -Text $handoffDetailMarkdown -ExpectedText "final_review.md" `
    -Message "Handoff detail Markdown should render Word visual final review paths."
Assert-DoesNotContainText -Text $handoffDetailMarkdown -UnexpectedText "review_note" `
    -Message "Handoff detail Markdown should not expose Word visual review notes."

$projectTemplateChecklistHandoffEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine -Summary ([pscustomobject]@{
        release_governance_handoff = [pscustomobject]@{
            project_template_readiness_checklist_entrypoints_source_report_count = 1
            project_template_readiness_checklist_entrypoints_source_reports = @(
                [pscustomobject]@{
                    schema = "featherdoc.release_candidate_summary"
                    path_display = ".\output\release-candidate-checks\summary.json"
                    project_template_readiness_checklist_entrypoints_status = "declared"
                    project_template_readiness_checklist_entrypoints_checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
                    project_template_readiness_checklist_entrypoints_required_entrypoint_count = 3
                    project_template_readiness_checklist_entrypoints_entrypoint_ids = @("start_here", "artifact_guide", "reviewer_checklist")
                    project_template_readiness_checklist_entrypoints_entrypoints = @(
                        [pscustomobject]@{
                            id = "start_here"
                            required = $true
                            path_display = ".\output\release-candidate-checks\START_HERE.md"
                        },
                        [pscustomobject]@{
                            id = "artifact_guide"
                            required = $true
                            path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                        },
                        [pscustomobject]@{
                            id = "reviewer_checklist"
                            required = $true
                            path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
                        }
                    )
                    project_template_readiness_checklist_entrypoints_checklist_marker = "release_entry_project_template_readiness_checklist_trace"
                }
            )
        }
    })
foreach ($expectedText in @(
        "Project-template readiness checklist handoff evidence",
        "project_template_readiness_checklist_entrypoints_source_reports=1",
        "required_entrypoint_count=3",
        "entrypoints=start_here, artifact_guide, reviewer_checklist",
        "entrypoint_paths=",
        "start_here:required=True:path_display=.\output\release-candidate-checks\START_HERE.md",
        "artifact_guide:required=True:path_display=.\output\release-candidate-checks\report\ARTIFACT_GUIDE.md",
        "reviewer_checklist:required=True:path_display=.\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md",
        "marker=release_entry_project_template_readiness_checklist_trace",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report=.\output\release-candidate-checks\summary.json"
    )) {
    Assert-ContainsText -Text $projectTemplateChecklistHandoffEvidenceLine -ExpectedText $expectedText `
        -Message "Compact project-template checklist handoff evidence line should include '$expectedText'."
}

$sourceFirstProjectTemplateChecklistSourceReport = [pscustomobject]@{
    schema = "featherdoc.release_candidate_summary"
    path_display = ".\output\release-candidate-checks-source\summary.json"
    project_template_readiness_checklist_entrypoints_status = "source-only"
    project_template_readiness_checklist_entrypoints_required_entrypoint_count = 0
}
$sourceFirstProjectTemplateChecklistFinalReport = [pscustomobject]@{
    schema = "featherdoc.release_candidate_summary"
    path_display = ".\output\release-candidate-checks\report\summary.json"
    project_template_readiness_checklist_entrypoints_status = "declared"
    project_template_readiness_checklist_entrypoints_checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
    project_template_readiness_checklist_entrypoints_required_entrypoint_count = 3
    project_template_readiness_checklist_entrypoints_entrypoint_ids = @("start_here", "artifact_guide", "reviewer_checklist")
    project_template_readiness_checklist_entrypoints_entrypoints = @(
        [pscustomobject]@{
            id = "start_here"
            required = $true
            path_display = ".\output\release-candidate-checks\START_HERE.md"
        },
        [pscustomobject]@{
            id = "artifact_guide"
            required = $true
            path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
        },
        [pscustomobject]@{
            id = "reviewer_checklist"
            required = $true
            path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
        }
    )
    project_template_readiness_checklist_entrypoints_checklist_marker = "release_entry_project_template_readiness_checklist_trace"
}
$sourceFirstPreferredReleaseCandidateReport = Select-ReleaseGovernancePreferredReleaseCandidateSourceReport -Reports @(
    $sourceFirstProjectTemplateChecklistSourceReport,
    $sourceFirstProjectTemplateChecklistFinalReport
)
Assert-Equal -Actual ([string]$sourceFirstPreferredReleaseCandidateReport.path_display) `
    -Expected ".\output\release-candidate-checks\report\summary.json" `
    -Message "Preferred release candidate source report should skip source-only summaries when final report is present."

$sourceFirstProjectTemplateChecklistHandoffEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine -Summary ([pscustomobject]@{
        release_governance_handoff = [pscustomobject]@{
            project_template_readiness_checklist_entrypoints_source_report_count = 2
            project_template_readiness_checklist_entrypoints_source_reports = @(
                $sourceFirstProjectTemplateChecklistSourceReport,
                $sourceFirstProjectTemplateChecklistFinalReport
            )
        }
    })
Assert-ContainsText -Text $sourceFirstProjectTemplateChecklistHandoffEvidenceLine `
    -ExpectedText "source_report=.\output\release-candidate-checks\report\summary.json" `
    -Message "Compact project-template checklist handoff evidence line should prefer the final release candidate report."
Assert-DoesNotContainText -Text $sourceFirstProjectTemplateChecklistHandoffEvidenceLine `
    -UnexpectedText "source_report=.\output\release-candidate-checks-source\summary.json" `
    -Message "Compact project-template checklist handoff evidence line should not select source-only report paths."
Assert-ContainsText -Text $sourceFirstProjectTemplateChecklistHandoffEvidenceLine -ExpectedText "status=declared" `
    -Message "Compact project-template checklist handoff evidence line should use metadata from the final report."

$projectTemplateChecklistMaterialSafetyAuditEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine -Summary ([pscustomobject]@{
        release_governance_handoff = [pscustomobject]@{
            release_blocker_rollup = [pscustomobject]@{
                report_markdown_display = ".\output\release-governance-handoff\release-blocker-rollup\release_blocker_rollup.md"
            }
            release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 1
            release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @(
                [pscustomobject]@{
                    schema = "featherdoc.release_candidate_summary"
                    path_display = ".\output\release-candidate-checks\report\summary.json"
                    release_entry_project_template_readiness_checklist_material_safety_audit_status = "passed"
                    release_entry_project_template_readiness_checklist_material_safety_audit_script = ".\scripts\assert_release_material_safety.ps1"
                    release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count = 3
                    release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints = @("start_here", "artifact_guide", "reviewer_checklist")
                    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label = "Project-template readiness checklist handoff evidence"
                    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
                    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema = "featherdoc.release_candidate_summary"
                    release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
                    release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker = "release_entry_project_template_readiness_checklist_trace"
                    release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
                }
            )
        }
    })
foreach ($expectedText in @(
        "Project-template readiness checklist packaged audit evidence",
        "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=1",
        "audited_entrypoints=start_here, artifact_guide, reviewer_checklist",
        "compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports",
        "compact_evidence_source_schema=featherdoc.release_candidate_summary",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report=.\output\release-governance-handoff\release-blocker-rollup\release_blocker_rollup.md"
    )) {
    Assert-ContainsText -Text $projectTemplateChecklistMaterialSafetyAuditEvidenceLine -ExpectedText $expectedText `
        -Message "Compact project-template checklist packaged audit evidence line should include '$expectedText'."
}

$sourceFirstProjectTemplateChecklistMaterialAuditSourceReport = [pscustomobject]@{
    schema = "featherdoc.release_candidate_summary"
    path_display = ".\output\release-candidate-checks-source\summary.json"
    release_entry_project_template_readiness_checklist_material_safety_audit_status = "source-only"
    release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count = 0
}
$sourceFirstProjectTemplateChecklistMaterialAuditFinalReport = [pscustomobject]@{
    schema = "featherdoc.release_candidate_summary"
    path_display = ".\output\release-candidate-checks\report\summary.json"
    release_entry_project_template_readiness_checklist_material_safety_audit_status = "passed"
    release_entry_project_template_readiness_checklist_material_safety_audit_script = ".\scripts\assert_release_material_safety.ps1"
    release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count = 3
    release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints = @("start_here", "artifact_guide", "reviewer_checklist")
    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label = "Project-template readiness checklist handoff evidence"
    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
    release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema = "featherdoc.release_candidate_summary"
    release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
    release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker = "release_entry_project_template_readiness_checklist_trace"
    release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
}
$sourceFirstProjectTemplateChecklistMaterialAuditEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine -Summary ([pscustomobject]@{
        release_governance_handoff = [pscustomobject]@{
            release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 2
            release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @(
                $sourceFirstProjectTemplateChecklistMaterialAuditSourceReport,
                $sourceFirstProjectTemplateChecklistMaterialAuditFinalReport
            )
        }
    })
Assert-ContainsText -Text $sourceFirstProjectTemplateChecklistMaterialAuditEvidenceLine `
    -ExpectedText "source_report=.\output\release-candidate-checks\report\summary.json" `
    -Message "Compact project-template checklist packaged audit evidence line should prefer the final release candidate report."
Assert-DoesNotContainText -Text $sourceFirstProjectTemplateChecklistMaterialAuditEvidenceLine `
    -UnexpectedText "source_report=.\output\release-candidate-checks-source\summary.json" `
    -Message "Compact project-template checklist packaged audit evidence line should not select source-only report paths."
Assert-ContainsText -Text $sourceFirstProjectTemplateChecklistMaterialAuditEvidenceLine -ExpectedText "status=passed" `
    -Message "Compact project-template checklist packaged audit evidence line should use metadata from the final report."

$wordVisualMetadataEvidenceLine = Get-ReleaseGovernanceWordVisualStandardReviewMetadataEvidenceLine -Summary ([pscustomobject]@{
        release_governance_handoff = [pscustomobject]@{
            word_visual_standard_review_metadata_source_report_count = 1
            word_visual_standard_review_metadata_source_reports = @($wordVisualStandardReviewMetadataSourceReport)
        }
    })
foreach ($expectedText in @(
        "Word visual standard review metadata evidence",
        "word_visual_standard_review_metadata_source_reports=1",
        "metadata_count=4",
        "task_keys=smoke, fixed_grid, section_page_setup, page_number_fields",
        "status_summary=reviewed=4",
        "verdict_summary=pass=4",
        "task_reviews=",
        "smoke:review_task_key=document:verdict=pass:review_status=reviewed:review_method=operator_supplied",
        "fixed_grid:review_task_key=fixed_grid:verdict=pass:review_status=reviewed:review_method=operator_supplied",
        "section_page_setup:review_task_key=section_page_setup:verdict=pass:review_status=reviewed:review_method=operator_supplied",
        "page_number_fields:review_task_key=page_number_fields:verdict=pass:review_status=reviewed:review_method=operator_supplied",
        "review_result_path=.\output\word-visual-release-gate\review-tasks\document\report\review_result.json",
        "final_review_path=.\output\word-visual-release-gate\review-tasks\document\report\final_review.md",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report=.\output\release-candidate-checks\summary.json"
    )) {
    Assert-ContainsText -Text $wordVisualMetadataEvidenceLine -ExpectedText $expectedText `
        -Message "Compact Word visual standard review metadata evidence line should include '$expectedText'."
}
Assert-DoesNotContainText -Text $wordVisualMetadataEvidenceLine -UnexpectedText "review_note" `
    -Message "Compact Word visual standard review metadata evidence line should not expose review notes."

$sourceFirstWordVisualMetadataSourceReport = [pscustomobject]@{
    schema = "featherdoc.release_candidate_summary"
    path_display = ".\output\release-candidate-checks-source\summary.json"
    word_visual_standard_review_metadata_count = 0
    word_visual_standard_review_task_keys = @("source_only")
    word_visual_standard_review_status_summary = "reviewed=0"
    word_visual_standard_review_verdict_summary = "pass=0"
}
$sourceFirstWordVisualMetadataFinalReport = [pscustomobject]@{
    schema = "featherdoc.release_candidate_summary"
    path_display = ".\output\release-candidate-checks\report\summary.json"
    word_visual_standard_review_metadata_count = 4
    word_visual_standard_review_task_keys = @("smoke", "fixed_grid", "section_page_setup", "page_number_fields")
    word_visual_standard_review_status_summary = "reviewed=4"
    word_visual_standard_review_verdict_summary = "pass=4"
    word_visual_standard_review_metadata = $wordVisualStandardReviewMetadataSourceReport.word_visual_standard_review_metadata
}
$sourceFirstWordVisualMetadataEvidenceLine = Get-ReleaseGovernanceWordVisualStandardReviewMetadataEvidenceLine -Summary ([pscustomobject]@{
        release_governance_handoff = [pscustomobject]@{
            word_visual_standard_review_metadata_source_report_count = 2
            word_visual_standard_review_metadata_source_reports = @(
                $sourceFirstWordVisualMetadataSourceReport,
                $sourceFirstWordVisualMetadataFinalReport
            )
        }
    })
Assert-ContainsText -Text $sourceFirstWordVisualMetadataEvidenceLine `
    -ExpectedText "source_report=.\output\release-candidate-checks\report\summary.json" `
    -Message "Compact Word visual metadata evidence line should prefer the final release candidate report."
Assert-DoesNotContainText -Text $sourceFirstWordVisualMetadataEvidenceLine `
    -UnexpectedText "source_report=.\output\release-candidate-checks-source\summary.json" `
    -Message "Compact Word visual metadata evidence line should not select source-only report paths."
Assert-ContainsText -Text $sourceFirstWordVisualMetadataEvidenceLine -ExpectedText "metadata_count=4" `
    -Message "Compact Word visual metadata evidence line should use metadata from the final report."

$actionChecklistItems = @(Get-ReleaseGovernanceActionItemChecklistItems -Summary ([pscustomobject]@{
            release_blocker_rollup = [pscustomobject]@{
                action_item_count = 1
                action_items = @($restoreAuditActionItem)
            }
            release_governance_handoff = [pscustomobject]@{
                action_item_count = 1
                action_items = @($restoreAuditActionItem)
                release_blocker_rollup = [pscustomobject]@{
                    action_item_count = 1
                    action_items = @($restoreAuditActionItem)
                }
            }
        }))
Assert-Equal -Actual $actionChecklistItems.Count -Expected 3 `
    -Message "Checklist items should preserve action items from every release governance source."
$actionChecklistText = Get-ReleaseGovernanceActionItemChecklistText -ActionItem $actionChecklistItems[0]
Assert-ContainsText -Text $actionChecklistText -ExpectedText 'Review release governance action item `review_style_merge_restore_audit`' `
    -Message "Action item checklist text should include action item id."
Assert-ContainsText -Text $actionChecklistText -ExpectedText 'action `review_style_merge_restore_audit`' `
    -Message "Action item checklist text should include action."
Assert-ContainsText -Text $actionChecklistText -ExpectedText 'source_schema `featherdoc.style_merge_restore_audit.v1`' `
    -Message "Action item checklist text should include source schema."
Assert-ContainsText -Text $actionChecklistText -ExpectedText 'issue_keys `style_merge_restore_audit_pending,word_visual_review_pending`' `
    -Message "Action item checklist text should include issue keys."
Assert-ContainsText -Text $actionChecklistText -ExpectedText 'project_id `project-finance`' `
    -Message "Action item checklist text should include project id."
Assert-ContainsText -Text $actionChecklistText -ExpectedText 'template_name `invoice-template`' `
    -Message "Action item checklist text should include template name."
Assert-ContainsText -Text $actionChecklistText -ExpectedText 'candidate_type `rename`' `
    -Message "Action item checklist text should include candidate type."
Assert-ContainsText -Text $actionChecklistText -ExpectedText 'source_report `.\output\document-skeleton-governance\style-merge.restore-audit.summary.json`' `
    -Message "Action item checklist text should include source report display."

$actionGuidance = (Get-ReleaseGovernanceActionItemActionGuidanceLines `
        -ActionItem $restoreAuditActionItem `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "summary.json")) -join "`n"
Assert-ContainsText -Text $actionGuidance -ExpectedText 'Open source report `.\output\document-skeleton-governance\style-merge.restore-audit.summary.json`' `
    -Message "Action item guidance should point reviewers at the source report."
Assert-ContainsText -Text $actionGuidance -ExpectedText 'Run `command` for release governance action item `review_style_merge_restore_audit`' `
    -Message "Action item guidance should include the primary command."
Assert-ContainsText -Text $actionGuidance -ExpectedText 'prepare_word_review_task.ps1 -DocxPath output/document-skeleton-governance/merged-styles.docx' `
    -Message "Action item guidance should preserve the review task command."
Assert-ContainsText -Text $actionGuidance -ExpectedText 'Run `open_command` for release governance action item `review_style_merge_restore_audit`' `
    -Message "Action item guidance should include open-latest command."
Assert-ContainsText -Text $actionGuidance -ExpectedText 'Run `audit_command` for release governance action item `review_style_merge_restore_audit`' `
    -Message "Action item guidance should include audit command."

$checklistItems = @(Get-ReleaseGovernanceWarningChecklistItems -Summary ([pscustomobject]@{
            release_blocker_rollup = [pscustomobject]@{
                warning_count = 1
                warnings = @($warningWithStyleMergeCount)
            }
            release_governance_handoff = [pscustomobject]@{
                warning_count = 1
                warnings = @($warningWithoutStyleMergeCount)
                release_blocker_rollup = [pscustomobject]@{
                    warning_count = 1
                    warnings = @($warningWithStyleMergeCount)
                }
            }
        }))
Assert-Equal -Actual $checklistItems.Count -Expected 3 `
    -Message "Checklist items should preserve warnings from every release governance source."
$checklistText = Get-ReleaseGovernanceWarningChecklistText -WarningItem $checklistItems[0]
Assert-ContainsText -Text $checklistText -ExpectedText 'Review release governance warning `document_skeleton.style_merge_suggestions_pending`' `
    -Message "Checklist text should include warning id."
Assert-ContainsText -Text $checklistText -ExpectedText 'action `review_style_merge_suggestions`' `
    -Message "Checklist text should include warning action."
Assert-ContainsText -Text $checklistText -ExpectedText 'source_schema `featherdoc.document_skeleton_governance_rollup_report.v1`' `
    -Message "Checklist text should include warning source schema."
Assert-ContainsText -Text $checklistText -ExpectedText 'project_id `project-finance`' `
    -Message "Warning checklist text should include project id."
Assert-ContainsText -Text $checklistText -ExpectedText 'template_name `invoice-template`' `
    -Message "Warning checklist text should include template name."
Assert-ContainsText -Text $checklistText -ExpectedText 'candidate_type `rename`' `
    -Message "Warning checklist text should include candidate type."

$styleMergeGuidance = (Get-ReleaseGovernanceWarningActionGuidanceLines `
        -Warning $warningWithStyleMergeCount `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "summary.json")) -join "`n"
Assert-ContainsText -Text $styleMergeGuidance -ExpectedText 'Use action `review_style_merge_suggestions`' `
    -Message "Style-merge warning guidance should mention its action."
Assert-ContainsText -Text $styleMergeGuidance -ExpectedText 'Current pending style merge suggestion count is `2`' `
    -Message "Style-merge warning guidance should preserve pending suggestion count."

$plainGuidance = (Get-ReleaseGovernanceWarningActionGuidanceLines `
        -Warning $warningWithoutStyleMergeCount `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "summary.json")) -join "`n"
Assert-ContainsText -Text $plainGuidance -ExpectedText 'Follow warning action `rebuild_numbering_catalog`' `
    -Message "Generic warning guidance should include the warning action."
Assert-ContainsText -Text $plainGuidance -ExpectedText 'source_schema `featherdoc.numbering_catalog_governance_report.v1`' `
    -Message "Generic warning guidance should include the warning source schema."
Assert-ContainsText -Text $plainGuidance -ExpectedText 'Open source report `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Generic warning guidance should include the warning source report."

Write-Host "Release governance warning helper regression passed."
