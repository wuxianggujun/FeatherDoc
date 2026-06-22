function Add-ReleaseEntryDocumentGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    $governanceMarkers = @(
        "content_control_data_binding.bound_placeholder",
        "project_template_delivery_readiness",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance",
        "table_layout_delivery_governance.delivery_quality",
        "real_corpus_confidence",
        "delivery_quality",
        "Release Governance Rollup Details",
        "Release Governance Handoff Details"
    )
    if (-not (Test-TextContainsAny -Text $Content -Needles $governanceMarkers)) {
        return
    }

    $label = "release entry governance trace"

    if ($Content.Contains("content_control_data_binding.bound_placeholder")) {
        $contentControlAnchor = "content_control_data_binding.bound_placeholder"
        $contentControlNeedles = @(
            "featherdoc.content_control_data_binding_governance_report.v1",
            "action=sync_or_fill_bound_content_control",
            "source_json_display",
            "input_docx",
            "template_name",
            "schema_target",
            "target_mode",
            "repair_strategy",
            "repair_hint",
            "Rerun Custom XML sync",
            "sync_bound_content_control",
            "command_template",
            "sync-content-controls-from-custom-xml",
            "repair_action_classes",
            "release_blocking",
            "auto_repair_candidate",
            "manual_confirmation_required"
        )
        $contentControlBlockNeedles = @($contentControlAnchor) + $contentControlNeedles
        if (-not (Test-ReleaseEntryContentControlTraceBlockContainsAll -Text $Content -Needles $contentControlBlockNeedles)) {
            $foundMissingNeedle = $false
            foreach ($needle in $contentControlNeedles) {
                if (-not (Test-ReleaseEntryContentControlTraceBlockContainsAll -Text $Content -Needles @($contentControlAnchor, $needle))) {
                    $foundMissingNeedle = $true
                    Add-AuditViolation `
                        -Violations $Violations `
                        -File $File `
                        -Label $label `
                        -Text "Entry document mentions content_control_data_binding.bound_placeholder without required repair workflow marker '$needle'."
                }
            }

            if (-not $foundMissingNeedle) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Entry document must keep content_control_data_binding.bound_placeholder repair workflow markers in the same Markdown list block or adjacent content-control entry block."
            }
        }
    }

    $mentionsProjectTemplateGovernance = Test-TextContainsAny -Text $Content -Needles @(
        "project_template_delivery_readiness",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance"
    )
    if ($mentionsProjectTemplateGovernance) {
        if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
            "Project template release readiness checklist",
            "docs/project_template_release_readiness_checklist_zh.rst"
        ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document must point reviewers at docs/project_template_release_readiness_checklist_zh.rst when project-template governance evidence is present."
        }
    }

    $hasProjectTemplateGovernanceEvidence = Test-TextContainsAny -Text $Content -Needles @(
        "featherdoc.project_template_delivery_readiness_report.v1",
        "project_template_onboarding.schema_approval",
        "featherdoc.project_template_onboarding_governance_report.v1"
    )
    if ($hasProjectTemplateGovernanceEvidence) {

        Add-ReleaseEntryProjectTemplateDetailViolations `
            -File $File `
            -Content $Content `
            -Violations $Violations `
            -Label $label `
            -Anchor "project_template_delivery_readiness" `
            -Description "project_template_delivery_readiness" `
            -Needles @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "featherdoc.project_template_delivery_readiness_report.v1",
            "status:",
            "release_ready:",
            "latest_schema_approval_gate_status",
            "source_report_display",
            "source_json_display"
        )

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor "project_template_delivery_readiness" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document project template delivery readiness status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor "project_template_delivery_readiness" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document project template delivery readiness release_ready must be true or false."
        }

        if (-not (Test-ReleaseEntryProjectTemplateTraceBlockContainsAll -Text $Content -Anchor "project_template_delivery_readiness" -Needles @(
            "project_template_delivery_readiness",
            "schema_approval_status_summary="
        ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document lost project template delivery readiness schema approval summary marker 'schema_approval_status_summary='."
        }

        Add-ReleaseEntryProjectTemplateDetailViolations `
            -File $File `
            -Content $Content `
            -Violations $Violations `
            -Label $label `
            -Anchor "project_template_onboarding.schema_approval" `
            -Description "project_template_onboarding.schema_approval" `
            -Needles @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance_contract",
            "featherdoc.project_template_onboarding_governance_report.v1",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )
    }

    if (Test-TextContainsAny -Text $Content -Needles @("delivery_quality", "table_layout_delivery_governance")) {
        if (-not $Content.Contains("table_layout_delivery_governance.delivery_quality")) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document mentions delivery_quality without table_layout_delivery_governance.delivery_quality."
        }
    }

    if ($Content.Contains("numbering_catalog_governance.real_corpus_confidence")) {
        Add-ReleaseEntryGovernanceMetricDetailViolations `
            -File $File `
            -Content $Content `
            -Violations $Violations `
            -Label $label `
            -Anchor "numbering_catalog_governance.real_corpus_confidence" `
            -Description "numbering_catalog_governance.real_corpus_confidence" `
            -Needles @(
            "catalog_coverage_percent",
            "baseline_coverage_percent",
            "coverage_score",
            "matched_document_count",
            "unmatched_catalog_document_count",
            "unmatched_baseline_document_count",
            "alignment_gap_count",
            "catalog_document_keys",
            "baseline_document_keys",
            "matched_document_keys",
            "penalty_summary",
            "style_numbering_issues"
        )
    }

    if ($Content.Contains("table_layout_delivery_governance.delivery_quality")) {
        Add-ReleaseEntryGovernanceMetricDetailViolations `
            -File $File `
            -Content $Content `
            -Violations $Violations `
            -Label $label `
            -Anchor "table_layout_delivery_governance.delivery_quality" `
            -Description "table_layout_delivery_governance.delivery_quality" `
            -Needles @(
            "table_style_issue_count",
            "automatic_tblLook_fix_count",
            "manual_table_style_fix_count",
            "table_position_automatic_count",
            "table_position_review_count",
            "fixed_layout_table_count",
            "autofit_layout_table_count",
            "unspecified_layout_table_count",
            "command_failure_count",
            "ready_document_percent",
            "unresolved_item_count",
            "pdf_floating_table_support_coverage",
            "pdf_floating_table_reviewer_focus",
            "metadata_only_fields",
            "review_required_fields",
            "metadata-only tblpPr",
            "penalty_summary",
            "floating_table_plans_pending"
        )
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistEntrypointsEvidenceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist handoff evidence:",
        "project_template_readiness_checklist_entrypoints_source_reports="
    ))) {
        return
    }

    $label = "release entry project template readiness checklist handoff evidence trace"
    if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
        "Project-template readiness checklist handoff evidence",
        "project_template_readiness_checklist_entrypoints_source_reports=",
        "status=",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "required_entrypoint_count=3",
        "entrypoints=",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "entrypoint_paths=",
        "START_HERE.md",
        "ARTIFACT_GUIDE.md",
        "REVIEWER_CHECKLIST.md",
        "marker=release_entry_project_template_readiness_checklist_trace",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report="
    ))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry must keep project-template readiness checklist handoff evidence count, status, checklist path, required entrypoint count, entrypoint ids, entrypoint paths, marker, source schema, and source report on the same compact evidence line."
    }

    if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "Project-template readiness checklist handoff evidence" `
                -FieldName "source_report" `
                -Needles @("release-candidate-checks", "release_candidate_summary"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry project-template checklist handoff source_report must identify the release-candidate summary evidence source."
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist packaged audit evidence",
        "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports="
    ))) {
        return
    }

    $label = "release entry project template readiness checklist packaged audit evidence trace"
    if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
        "Project-template readiness checklist packaged audit evidence",
        "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=",
        "status=passed",
        "audit_script=",
        "assert_release_material_safety.ps1",
        "audited_entrypoint_count=3",
        "audited_entrypoints=",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "compact_evidence_label=Project-template readiness checklist handoff evidence",
        "compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports",
        "compact_evidence_source_schema=featherdoc.release_candidate_summary",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "checklist_marker=release_entry_project_template_readiness_checklist_trace",
        "material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
        "source_report="
    ))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry must keep packaged project-template readiness checklist material-safety audit count, status, audit script, audited entrypoints, compact evidence identity, compact evidence source schema, checklist path, checklist marker, material-safety marker, source schema, and source report on the same compact evidence line."
    }

    if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
                -Text $Content `
                -Anchor "Project-template readiness checklist packaged audit evidence" `
                -FieldName "source_schema" `
                -AllowedValues @("featherdoc.release_candidate_summary"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry must keep packaged project-template readiness checklist material-safety audit source_schema as an explicit compact evidence field."
    }

    if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "Project-template readiness checklist packaged audit evidence" `
                -FieldName "source_report" `
                -Needles @("release-blocker-rollup", "release_blocker_rollup"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry project-template packaged audit source_report must identify the release-blocker rollup evidence source."
    }
}

function Add-ReleaseEntryProjectTemplateWorkflowDashboardTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    if (-not $Content.Contains("Project template workflow dashboard")) {
        return
    }

    $label = "release entry project template workflow dashboard trace"
    $requiredLineNeedleSets = @(
        @("Project template workflow dashboard status"),
        @("Project template workflow dashboard release ready"),
        @("Project template workflow dashboard counts", "reports", "blockers", "warnings"),
        @("Project template workflow dashboard summary"),
        @("Project template workflow dashboard report"),
        @("Project template workflow dashboard next action"),
        @("Project template workflow dashboard next action groups")
    )

    foreach ($needles in $requiredLineNeedleSets) {
        if (-not (Test-TextLineContainsAll -Text $Content -Needles $needles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text ("Release entry must keep project-template workflow dashboard marker line with: {0}." -f (@($needles) -join ", "))
        }
    }

    $hasZeroActionGroups = Test-TextLineContainsAll -Text $Content -Needles @(
        "Project template workflow dashboard next action groups:",
        "0"
    )
    if (-not $hasZeroActionGroups -and -not (Test-TextLineContainsAll -Text $Content -Needles @(
            "Project template workflow dashboard action group:",
            "source=",
            "action=",
            "blocker=",
            "entries="
        ))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry must keep non-zero project-template workflow dashboard action groups with source, action, blocker, and entries markers."
    }

    if ($leafName -eq "reviewer_checklist.md") {
        $requiresStopCondition = Test-TextContainsAny -Text $Content -Needles @(
            "Project template workflow dashboard status: blocked",
            "Project template workflow dashboard status: failed",
            "Project template workflow dashboard status: missing_summary",
            "Project template workflow dashboard status: missing_input",
            "Project template workflow dashboard status: schema_mismatch",
            "Project template workflow dashboard release ready: False"
        )
        if ($requiresStopCondition -and -not (Test-TextLineContainsAll -Text $Content -Needles @(
                "Stop here until the project template workflow dashboard is release-ready",
                "status",
                "release_ready",
                "blockers",
                "warnings"
            ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Reviewer checklist must keep the project-template workflow dashboard stop condition when the dashboard is blocked, failed, missing, or not release-ready."
        }
    }
}

function Add-ReleaseEntryWordVisualStandardReviewMetadataEvidenceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md", "final_review.md", "release_handoff.md")) {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Word visual standard review metadata evidence",
        "word_visual_standard_review_metadata_source_reports="
    ))) {
        return
    }

    $label = "release entry Word visual standard review metadata evidence trace"
    if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
        "Word visual standard review metadata evidence",
        "word_visual_standard_review_metadata_source_reports=",
        "metadata_count=4",
        "task_keys=smoke, fixed_grid, section_page_setup, page_number_fields",
        "status_summary=reviewed=4",
        "verdict_summary=pass=4",
        "task_reviews=",
        "smoke:review_task_key=document",
        "fixed_grid:review_task_key=fixed_grid",
        "section_page_setup:review_task_key=section_page_setup",
        "page_number_fields:review_task_key=page_number_fields",
        "review_result_path=",
        "final_review_path=",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report="
    ))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry must keep Word visual standard review metadata count, task keys, status and verdict summaries, task review mapping, review/final paths, source schema, and source report on the same compact evidence line."
    }

    $evidenceLine = Get-TextLineContainingAll -Text $Content -Needles @(
        "Word visual standard review metadata evidence",
        "word_visual_standard_review_metadata_source_reports="
    )
    if ($null -ne $evidenceLine -and $evidenceLine.Contains("review_note")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry Word visual standard review metadata compact evidence must not expose review_note."
    }

    if ($Content.Contains("Word visual standard review metadata evidence") -and
        $Content.Contains("review_note")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry Word visual standard review metadata evidence must keep review_note out of release entry documents."
    }

    if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
                -Text $Content `
                -Anchor "Word visual standard review metadata evidence" `
                -FieldName "source_schema" `
                -AllowedValues @("featherdoc.release_candidate_summary"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry Word visual standard review metadata evidence must keep source_schema as featherdoc.release_candidate_summary."
    }

    if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "Word visual standard review metadata evidence" `
                -FieldName "source_report" `
                -Needles @("release-candidate-checks", "release_candidate_summary"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry Word visual standard review metadata source_report must identify the release-candidate summary evidence source."
    }
}

function Add-ReleaseMetadataProjectTemplateReadinessChecklistEntrypointsTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md", "release_handoff.md")) {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist entrypoints evidence source reports",
        "project_template_readiness_checklist_entrypoints_status",
        "project_template_readiness_checklist_entrypoints_checklist_path"
    ))) {
        return
    }

    $label = "release metadata project template readiness checklist entrypoints trace"
    if (-not $Content.Contains("Project-template readiness checklist entrypoints evidence source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release metadata lost project-template readiness checklist source-report trace marker 'Project-template readiness checklist entrypoints evidence source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "featherdoc.release_candidate_summary",
        "project_template_readiness_checklist_entrypoints_status:",
        "project_template_readiness_checklist_entrypoints_checklist_label:",
        "Project template release readiness checklist",
        "project_template_readiness_checklist_entrypoints_checklist_path:",
        "docs/project_template_release_readiness_checklist_zh.rst",
        "project_template_readiness_checklist_entrypoints_required_entrypoint_count:",
        "project_template_readiness_checklist_entrypoints_entrypoint_ids:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "project_template_readiness_checklist_entrypoints_checklist_marker:",
        "release_entry_project_template_readiness_checklist_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAllAndFieldValuesInSet `
        -Text $Content `
        -Anchor "source_report:" `
        -Needles $sourceReportBlockNeedles `
        -FieldName "schema" `
        -AllowedValues @("featherdoc.release_candidate_summary"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release metadata must keep project-template readiness checklist release-candidate source identity, status, checklist path, required entrypoint ids, and fixed checklist marker in the same source_report block."
    }
}

function Add-ReleaseMetadataProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md", "release_handoff.md")) {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Release-entry project-template readiness checklist material-safety audit source reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
    ))) {
        return
    }

    $label = "release metadata project template readiness checklist material-safety audit trace"
    if (-not $Content.Contains("Release-entry project-template readiness checklist material-safety audit source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release metadata lost release-entry project-template readiness checklist material-safety audit source-report trace marker 'Release-entry project-template readiness checklist material-safety audit source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "featherdoc.release_candidate_summary",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status:",
        "passed",
        "release_entry_project_template_readiness_checklist_material_safety_audit_script:",
        "assert_release_material_safety.ps1",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count:",
        "3",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label:",
        "Project-template readiness checklist handoff evidence",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field:",
        "project_template_readiness_checklist_entrypoints_source_reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema:",
        "featherdoc.release_candidate_summary",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path:",
        "docs/project_template_release_readiness_checklist_zh.rst",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker:",
        "release_entry_project_template_readiness_checklist_trace",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker:",
        "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAllAndFieldValuesInSet `
        -Text $Content `
        -Anchor "source_report:" `
        -Needles $sourceReportBlockNeedles `
        -FieldName "schema" `
        -AllowedValues @("featherdoc.release_candidate_summary"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release metadata must keep release-entry project-template readiness checklist material-safety audit release-candidate source identity, status, audit script, audited entrypoints, compact evidence identity, compact evidence source schema, checklist path, checklist marker, and material-safety marker in the same source_report block."
    }
}
