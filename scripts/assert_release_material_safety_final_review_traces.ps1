function Add-ReleaseGovernanceHandoffManifestSignoffEntrypointsTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Manifest signoff entrypoints evidence source reports",
        "manifest_signoff_entrypoints_status",
        "reviewer_manifest_scoped_project_template_trace"
    ))) {
        return
    }

    $label = "release governance handoff manifest signoff entrypoints trace"
    if (-not $Content.Contains("Manifest signoff entrypoints evidence source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff lost manifest signoff entrypoints source-report trace marker 'Manifest signoff entrypoints evidence source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "featherdoc.release_candidate_summary",
        "manifest_signoff_entrypoints_status:",
        "declared",
        "manifest_signoff_entrypoints_release_assets_manifest_display:",
        "release_assets_manifest.json",
        "manifest_signoff_entrypoints_required_entrypoint_count:",
        "3",
        "manifest_signoff_entrypoints_entrypoint_ids:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "manifest_signoff_entrypoints_required_contracts:",
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract",
        "manifest_signoff_entrypoints_required_fields:",
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
        "business_document_type_summary",
        "corpus_role_summary",
        "source_report_display",
        "source_json_display",
        "manifest_signoff_entrypoints_checklist_marker:",
        "reviewer_manifest_scoped_project_template_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAllAndFieldValuesInSet `
        -Text $Content `
        -Anchor "source_report:" `
        -Needles $sourceReportBlockNeedles `
        -FieldName "schema" `
        -AllowedValues @("featherdoc.release_candidate_summary"))) {
        $traceMarker = "manifest_signoff_entrypoints_handoff_material_safety_trace"
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff must keep manifest signoff status, release assets manifest path, required entrypoints, required contracts, required fields, fixed marker, and release-candidate source identity in the same source_report block. Fixed marker: $traceMarker."
    }
}

function Add-ReleaseBlockerRollupManifestSignoffEntrypointsTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_blocker_rollup.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "manifest_signoff_entrypoints_status",
        "manifest_signoff_entrypoints_release_assets_manifest_display",
        "reviewer_manifest_scoped_project_template_trace"
    ))) {
        return
    }

    $label = "release blocker rollup manifest signoff entrypoints trace"
    if (-not $Content.Contains("Source Report Contracts")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup lost Source Report Contracts while carrying manifest signoff entrypoints evidence."
    }

    $sourceReportBlockNeedles = @(
        "featherdoc.release_candidate_summary",
        "manifest_signoff_entrypoints_status:",
        "declared",
        "manifest_signoff_entrypoints_release_assets_manifest_display:",
        "release_assets_manifest.json",
        "manifest_signoff_entrypoints_required_entrypoint_count:",
        "3",
        "manifest_signoff_entrypoints_entrypoint_ids:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "manifest_signoff_entrypoints_required_contracts:",
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract",
        "manifest_signoff_entrypoints_required_fields:",
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
        "business_document_type_summary",
        "corpus_role_summary",
        "source_report_display",
        "source_json_display",
        "manifest_signoff_entrypoints_checklist_marker:",
        "reviewer_manifest_scoped_project_template_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll `
        -Text $Content `
        -Anchor "featherdoc.release_candidate_summary" `
        -Needles $sourceReportBlockNeedles)) {
        $traceMarker = "manifest_signoff_entrypoints_rollup_material_safety_trace"
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup must keep manifest signoff status, release assets manifest path, required entrypoints, required contracts, required fields, fixed marker, and release-candidate source identity in the same Source Report Contracts block. Fixed marker: $traceMarker."
    }
}

function Add-ReleaseBlockerRollupProjectTemplateReadinessChecklistEntrypointsTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_blocker_rollup.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "project_template_readiness_checklist_entrypoints_status",
        "project_template_readiness_checklist_entrypoints_checklist_path",
        "release_entry_project_template_readiness_checklist_trace"
    ))) {
        return
    }

    $label = "release blocker rollup project template readiness checklist entrypoints trace"
    if (-not $Content.Contains("Source Report Contracts")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup lost Source Report Contracts while carrying project-template readiness checklist entrypoints evidence."
    }

    $sourceReportBlockNeedles = @(
        "featherdoc.release_candidate_summary",
        "project_template_readiness_checklist_entrypoints_status:",
        "declared",
        "project_template_readiness_checklist_entrypoints_checklist_label:",
        "Project template release readiness checklist",
        "project_template_readiness_checklist_entrypoints_checklist_path:",
        "docs/project_template_release_readiness_checklist_zh.rst",
        "project_template_readiness_checklist_entrypoints_required_entrypoint_count:",
        "3",
        "project_template_readiness_checklist_entrypoints_entrypoint_ids:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "project_template_readiness_checklist_entrypoints_checklist_marker:",
        "release_entry_project_template_readiness_checklist_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll `
        -Text $Content `
        -Anchor "featherdoc.release_candidate_summary" `
        -Needles $sourceReportBlockNeedles)) {
        $traceMarker = "project_template_readiness_checklist_entrypoints_rollup_material_safety_trace"
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup must keep project-template readiness checklist status, checklist identity, required entrypoint count, entrypoint ids, fixed checklist marker, and release-candidate source identity in the same Source Report Contracts block. Fixed marker: $traceMarker."
    }
}

function Add-ReleaseBlockerRollupProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_blocker_rollup.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "release_entry_project_template_readiness_checklist_material_safety_audit_status",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker",
        "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    ))) {
        return
    }

    $label = "release blocker rollup project template readiness checklist material-safety audit trace"
    if (-not $Content.Contains("Source Report Contracts")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup lost Source Report Contracts while carrying release-entry project-template readiness checklist material-safety audit evidence."
    }

    $sourceReportBlockNeedles = @(
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
    if (-not (Test-MarkdownAnyListBlockContainsAll `
        -Text $Content `
        -Anchor "featherdoc.release_candidate_summary" `
        -Needles $sourceReportBlockNeedles)) {
        $traceMarker = "project_template_readiness_checklist_entrypoints_packaged_audit_rollup_material_safety_trace"
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup must keep release-entry project-template readiness checklist material-safety audit status, script, audited entrypoints, compact evidence identity, compact source schema, checklist path, checklist marker, material-safety marker, and release-candidate source identity in the same Source Report Contracts block. Fixed marker: $traceMarker."
    }
}

function Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistEntrypointsTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist entrypoints evidence source reports",
        "project_template_readiness_checklist_entrypoints_status",
        "project_template_readiness_checklist_entrypoints_checklist_path"
    ))) {
        return
    }

    $label = "release governance handoff project template readiness checklist entrypoints trace"
    if (-not $Content.Contains("Project-template readiness checklist entrypoints evidence source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff lost project-template readiness checklist source-report trace marker 'Project-template readiness checklist entrypoints evidence source reports:'."
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
            -Text "Release governance handoff must keep project-template readiness checklist release-candidate source identity, status, checklist path, required entrypoint ids, and fixed checklist marker in the same source_report block."
    }
}

function Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Release-entry project-template readiness checklist material-safety audit source reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
    ))) {
        return
    }

    $label = "release governance handoff project template readiness checklist material-safety audit trace"
    if (-not $Content.Contains("Release-entry project-template readiness checklist material-safety audit source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff lost release-entry project-template readiness checklist material-safety audit source-report trace marker 'Release-entry project-template readiness checklist material-safety audit source reports:'."
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
            -Text "Release governance handoff must keep release-entry project-template readiness checklist material-safety audit release-candidate source identity, status, audit script, audited entrypoints, compact evidence identity, compact evidence source schema, checklist path, checklist marker, and material-safety marker in the same source_report block."
    }
}

function Add-FinalReviewProjectTemplateReadinessChecklistEntrypointsTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist entrypoints evidence source reports",
        "project_template_readiness_checklist_entrypoints_status",
        "project_template_readiness_checklist_entrypoints_checklist_path"
    ))) {
        return
    }

    $label = "final review project template readiness checklist entrypoints trace"
    if (-not $Content.Contains("Project-template readiness checklist entrypoints evidence source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review lost project-template readiness checklist source-report trace marker 'Project-template readiness checklist entrypoints evidence source reports:'."
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
            -Text "Final review must keep project-template readiness checklist release-candidate source identity, status, checklist path, required entrypoint ids, and fixed checklist marker in the same source_report block."
    }
}

function Add-FinalReviewProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Release-entry project-template readiness checklist material-safety audit source reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
    ))) {
        return
    }

    $label = "final review project template readiness checklist material-safety audit trace"
    if (-not $Content.Contains("Release-entry project-template readiness checklist material-safety audit source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review lost release-entry project-template readiness checklist material-safety audit source-report trace marker 'Release-entry project-template readiness checklist material-safety audit source reports:'."
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
            -Text "Final review must keep release-entry project-template readiness checklist material-safety audit release-candidate source identity, status, audit script, audited entrypoints, compact evidence identity, compact evidence source schema, checklist path, checklist marker, and material-safety marker in the same source_report block."
    }
}

function Add-FinalReviewProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Release governance handoff details",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract"
    ))) {
        return
    }

    $label = "final review project template governance trace"
    if (-not $Content.Contains("Release governance handoff details")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review lost project template governance trace marker 'Release governance handoff details'."
    }

    $anchor = "project_template_delivery_readiness / project_template_onboarding.schema_approval"
    $blockNeedles = @(
        "featherdoc.project_template_onboarding_governance_report.v1",
        "project_template_onboarding_governance_contract",
        "schema_approval_status_summary",
        "status:",
        "release_ready:",
        "source_report_display:",
        "project-template-delivery-readiness",
        "source_json_display:",
        "project-template-onboarding-governance",
        "readiness_status:",
        "readiness_release_ready:"
    )

    if (Test-FinalReviewProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $anchor -Needles $blockNeedles) {
        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "source_report_display" `
            -Needles $ProjectTemplateGovernanceReportSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "source_json_display" `
            -Needles $ProjectTemplateOnboardingGovernanceSourceNeedles `
            -ForbiddenNeedles $ProjectTemplateDeliveryReadinessSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "readiness_status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template readiness_status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "readiness_release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template readiness_release_ready must be true or false."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template onboarding release_ready must be true or false."
        }

        return
    }

    $foundMissingNeedle = $false
    foreach ($needle in $blockNeedles) {
        if (-not (Test-FinalReviewProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $anchor -Needles @($needle))) {
            $foundMissingNeedle = $true
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review lost project template governance trace marker '$needle' inside the handoff blocker block."
        }
    }

    if (-not $foundMissingNeedle) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep project template governance trace markers in the same handoff blocker block."
    }
}

function Add-FinalReviewPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate summary:", "summary.json")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate contact sheet:", "aggregate-contact-sheet.png"))
    )) {
        return
    }

    $label = "final review PDF visual gate trace"
    $stepStatusNeedles = @(
        "PDF visual gate:",
        "PDF visual gate verdict:",
        "PDF visual gate counts:",
        "PDF visual gate manifest counts:",
        "PDF visual gate finalizable:"
    )
    foreach ($needle in $stepStatusNeedles) {
        if (-not $Content.Contains($needle)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review lost PDF visual gate trace marker '$needle'."
        }
    }

    if (-not (Test-MarkdownSectionListRunContainsAll -Text $Content -Heading "## Step status" -Anchor "PDF visual gate:" -Needles $stepStatusNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep PDF visual gate status, verdict, counts, and finalizable evidence in the same step-status Markdown list run."
    }

    $keyOutputNeedles = @(
        "PDF visual gate summary:",
        "summary.json",
        "PDF visual gate contact sheet:",
        "aggregate-contact-sheet.png"
    )
    if (-not (Test-MarkdownSectionContainsAll -Text $Content -Heading "## Key outputs" -Needles $keyOutputNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep PDF visual gate summary and contact-sheet evidence inside the Key outputs section."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate summary:", "summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep the PDF visual gate summary path on the PDF visual gate summary line."
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict:", "pass")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict:", "fail"))
    )) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep a pass/fail PDF visual gate verdict on the PDF visual gate verdict line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate contact sheet:", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep the PDF visual gate contact-sheet path on the PDF visual gate contact sheet line."
    }
}

function Add-FinalReviewPdfVisualGateAttemptTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "attempt-summary.json",
        "bounded_attempt_auxiliary_only"
    ))) {
        return
    }

    $label = "final review PDF visual gate attempt trace"
    $stepStatusNeedles = @(
        "PDF visual gate attempt:",
        "PDF visual gate attempt verdict:",
        "PDF visual gate attempt full status:",
        "PDF visual gate attempt stages:",
        "PDF visual gate attempt pdf_regression:",
        "PDF visual gate attempt render:"
    )
    if (-not (Test-MarkdownSectionListRunContainsAll -Text $Content -Heading "## Step status" -Anchor "PDF visual gate attempt:" -Needles $stepStatusNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep PDF visual gate attempt status, verdict, full status, stage counts, regression counts, and render progress in the same step-status Markdown list run."
    }

    $keyOutputNeedles = @(
        "PDF visual gate attempt summary:",
        "attempt-summary.json",
        "PDF visual gate attempt contact sheet:",
        "aggregate-contact-sheet.png"
    )
    if (-not (Test-MarkdownSectionContainsAll -Text $Content -Heading "## Key outputs" -Needles $keyOutputNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep PDF visual gate attempt summary and contact-sheet evidence inside the Key outputs section."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate attempt summary:", "attempt-summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep the PDF visual gate attempt summary path on the PDF visual gate attempt summary line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate attempt contact sheet:", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep the PDF visual gate attempt contact-sheet path on the PDF visual gate attempt contact sheet line."
    }
}

function Add-FinalReviewPdfVisualSegmentedGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "segmented-summary.json",
        "segmented_visual_gate_auxiliary_only"
    ))) {
        return
    }

    $label = "final review PDF visual segmented gate trace"
    $stepStatusNeedles = @(
        "PDF visual segmented gate:",
        "PDF visual segmented gate verdict:",
        "PDF visual segmented gate full status:",
        "PDF visual segmented gate scope:",
        "segmented_visual_gate_auxiliary_only",
        "PDF visual segmented gate slices:",
        "PDF visual segmented gate coverage:"
    )
    if (-not (Test-MarkdownSectionListRunContainsAll -Text $Content -Heading "## Step status" -Anchor "PDF visual segmented gate:" -Needles $stepStatusNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep PDF visual segmented gate status, verdict, full status, auxiliary scope, slice counts, and coverage in the same step-status Markdown list run."
    }

    $keyOutputNeedles = @(
        "PDF visual segmented gate summary:",
        "segmented-summary.json",
        "PDF visual segmented gate contact sheet:",
        "aggregate-contact-sheet.png"
    )
    if (-not (Test-MarkdownSectionContainsAll -Text $Content -Heading "## Key outputs" -Needles $keyOutputNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep PDF visual segmented gate summary and contact-sheet evidence inside the Key outputs section."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual segmented gate summary:", "segmented-summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep the PDF visual segmented gate summary path on the PDF visual segmented gate summary line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual segmented gate contact sheet:", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep the PDF visual segmented gate contact-sheet path on the PDF visual segmented gate contact sheet line."
    }
}
