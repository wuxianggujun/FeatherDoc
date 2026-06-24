function Add-ReleaseBodyPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_body.zh-cn.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "PDF visual gate summary",
        "PDF visual gate verdict",
        "PDF visual gate aggregate contact sheet"
    ))) {
        return
    }

    $label = "release body PDF visual gate trace"
    $evidenceNeedles = @(
        "PDF visual gate summary",
        "PDF visual gate evidence status",
        "PDF visual gate verdict",
        "PDF visual gate aggregate contact sheet",
        "aggregate-contact-sheet.png",
        "PDF CJK manifest samples",
        "PDF CJK copy/search samples",
        "PDF visual baseline manifest samples",
        "PDF visual baselines"
    )
    foreach ($needle in $evidenceNeedles) {
        if (-not $Content.Contains($needle)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body lost PDF visual gate trace marker '$needle'."
        }
    }

    if (-not (Test-MarkdownListRunContainsAll -Text $Content -Anchor "PDF visual gate summary" -Needles $evidenceNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release body must keep PDF visual gate status, verdict, counts, and contact-sheet evidence in the same Markdown list run."
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict", "pass")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict", "fail"))
    )) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release body must keep a pass/fail PDF visual gate verdict on the PDF visual gate verdict line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate summary", "summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release body must keep the PDF visual gate summary path on the PDF visual gate summary line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate aggregate contact sheet", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release body must keep the PDF visual gate aggregate contact-sheet path on the PDF visual gate aggregate contact sheet line."
    }
}

function Add-ReleaseHandoffProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_handoff.md") {
        return
    }

    $label = "release handoff project template governance trace"

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_delivery_readiness:", "project_template_delivery_readiness_contract")) {
        $readinessAnchor = "project_template_delivery_readiness:"
        $readinessBlockNeedles = @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "source_schema: featherdoc.project_template_delivery_readiness_report.v1",
            "status:",
            "release_ready:",
            "latest_schema_approval_gate_status:",
            "schema_approval_status_summary:",
            "source_report_display:",
            "source_json_display:"
        )
        if (-not (Test-ReleaseHandoffProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $readinessAnchor -Needles $readinessBlockNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff must keep project template readiness schema, contract, status, and source displays in the same list block."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $readinessAnchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template readiness status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $readinessAnchor `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template readiness release_ready must be true or false."
        }

        foreach ($fieldName in @("source_report_display", "source_json_display")) {
            if (-not (Test-MarkdownListBlockFieldValuesIdentify `
                -Text $Content `
                -Anchor $readinessAnchor `
                -FieldName $fieldName `
                -Needles $ProjectTemplateDeliveryReadinessSourceNeedles `
                -ForbiddenNeedles $ProjectTemplateOnboardingGovernanceSourceNeedles)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release handoff project template readiness $fieldName must identify the delivery readiness evidence source."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_onboarding.schema_approval", "project_template_onboarding_governance_contract")) {
        $onboardingAnchor = "project_template_onboarding.schema_approval"
        $onboardingBlockNeedles = @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance_contract",
            "source_schema: featherdoc.project_template_onboarding_governance_report.v1",
            "status:",
            "release_ready:",
            "schema_approval_status_summary:",
            "source_report_display:",
            "source_json_display:"
        )
        if (-not (Test-ReleaseHandoffProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $onboardingAnchor -Needles $onboardingBlockNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff must keep project template onboarding schema approval, contract, and source displays in the same list block."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "source_report_display" `
            -Needles $ProjectTemplateGovernanceReportSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template onboarding source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "source_json_display" `
            -Needles $ProjectTemplateOnboardingGovernanceSourceNeedles `
            -ForbiddenNeedles $ProjectTemplateDeliveryReadinessSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template onboarding source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template onboarding release_ready must be true or false."
        }
    }
}

function Add-ReleaseHandoffPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "PDF visual gate summary:",
        "PDF visual gate verdict:",
        "PDF visual gate aggregate contact sheet:"
    ))) {
        return
    }

    $label = "release handoff PDF visual gate trace"
    $evidenceNeedles = @(
        "PDF visual gate summary:",
        "PDF visual gate evidence status:",
        "PDF visual gate verdict:",
        "PDF visual gate aggregate contact sheet:",
        "aggregate-contact-sheet.png",
        "PDF CJK manifest samples:",
        "PDF CJK copy/search samples:",
        "PDF visual baseline manifest samples:",
        "PDF visual baselines:"
    )
    foreach ($needle in $evidenceNeedles) {
        if (-not $Content.Contains($needle)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff lost PDF visual gate trace marker '$needle'."
        }
    }

    if (-not (Test-MarkdownListRunContainsAll -Text $Content -Anchor "PDF visual gate summary:" -Needles $evidenceNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release handoff must keep PDF visual gate status, verdict, counts, and contact-sheet evidence in the same Markdown list run."
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict:", "pass")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict:", "fail"))
    )) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release handoff must keep a pass/fail PDF visual gate verdict on the PDF visual gate verdict line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate summary:", "summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release handoff must keep the PDF visual gate summary path on the PDF visual gate summary line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate aggregate contact sheet:", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release handoff must keep the PDF visual gate contact-sheet path on the PDF visual gate aggregate contact sheet line."
    }
}

function Add-ReleaseGovernanceHandoffProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    $label = "release governance handoff project template trace"

    if (Test-TextContainsAny -Text $Content -Needles @(
        '`project_template_delivery_readiness`:',
        "featherdoc.project_template_delivery_readiness_report.v1",
        "latest_schema_approval_gate_status"
    )) {
        $readinessAnchor = '`project_template_delivery_readiness`:'
        $readinessBlockNeedles = @(
            "project_template_delivery_readiness",
            "featherdoc.project_template_delivery_readiness_report.v1",
            "status=",
            "ready=",
            "latest_schema_approval_gate_status",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )
        if (-not (Test-ReleaseGovernanceHandoffProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $readinessAnchor -Needles $readinessBlockNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff must keep project template readiness schema, status, and source displays in the same report-status block."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $readinessAnchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template readiness status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $readinessAnchor `
            -FieldName "ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template readiness ready must be true or false."
        }

        foreach ($fieldName in @("source_report_display", "source_json_display")) {
            if (-not (Test-MarkdownListBlockFieldValuesIdentify `
                -Text $Content `
                -Anchor $readinessAnchor `
                -FieldName $fieldName `
                -Needles $ProjectTemplateDeliveryReadinessSourceNeedles `
                -ForbiddenNeedles $ProjectTemplateOnboardingGovernanceSourceNeedles)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release governance handoff project template readiness $fieldName must identify the delivery readiness evidence source."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @(
        "project_template_onboarding.schema_approval",
        "featherdoc.project_template_onboarding_governance_report.v1"
    )) {
        $onboardingAnchor = "project_template_onboarding.schema_approval"
        $onboardingBlockNeedles = @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance_contract",
            "featherdoc.project_template_onboarding_governance_report.v1",
            "status:",
            "release_ready:",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )
        if (-not (Test-ReleaseGovernanceHandoffProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $onboardingAnchor -Needles $onboardingBlockNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff must keep project template onboarding schema approval, contract, and source displays in the same blocker block."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "source_report_display" `
            -Needles $ProjectTemplateGovernanceReportSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template onboarding source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "source_json_display" `
            -Needles $ProjectTemplateOnboardingGovernanceSourceNeedles `
            -ForbiddenNeedles $ProjectTemplateDeliveryReadinessSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template onboarding source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template onboarding release_ready must be true or false."
        }
    }
}

function Add-ReleaseGovernanceHandoffPdfVisualGateTraceViolations {
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
        "PDF visual gate evidence source reports",
        "pdf_visual_gate_verdict",
        "pdf_visual_gate_summary_json_display"
    ))) {
        return
    }

    $label = "release governance handoff PDF visual gate trace"
    if (-not $Content.Contains("PDF visual gate evidence source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff lost PDF visual gate trace marker 'PDF visual gate evidence source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "pdf_visual_gate_status:",
        "full_visual_gate_status:",
        "pdf_visual_gate_verdict:",
        "pdf_visual_gate_finalizable:",
        "pdf_visual_gate_summary_json_display:",
        "pdf_visual_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_gate_cjk_manifest_count:",
        "pdf_visual_gate_cjk_copy_search_count:",
        "pdf_visual_gate_visual_baseline_manifest_count:",
        "pdf_visual_gate_visual_baseline_count:",
        "pdf_bounded_ctest_summary_count:",
        "pdf_bounded_ctest_pass_count:",
        "pdf_bounded_ctest_skipped_test_count:",
        "pdf_bounded_ctest_selected_test_count:",
        "pdf_bounded_ctest_subsets:",
        "pdf_bounded_ctest_summary_json_display:"
    )
    if (-not (Test-MarkdownListBlockContainsAll -Text $Content -Anchor "source_report:" -Needles $sourceReportBlockNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff must keep PDF visual gate source report, full visual status, verdict, counts, contact-sheet path, and bounded CTest auxiliary evidence in the same source_report block."
    }
}

function Add-ReleaseGovernanceHandoffPdfVisualGateAttemptTraceViolations {
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
        "pdf_visual_gate_attempt_status",
        "pdf_visual_gate_attempt_summary_json_display",
        "bounded_attempt_auxiliary_only"
    ))) {
        return
    }

    $label = "release governance handoff PDF visual gate attempt trace"
    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "pdf_visual_gate_attempt_status:",
        "pdf_visual_gate_attempt_verdict:",
        "pdf_visual_gate_attempt_full_visual_gate_status:",
        "pdf_visual_gate_attempt_evidence_scope:",
        "bounded_attempt_auxiliary_only",
        "pdf_visual_gate_attempt_summary_json_display:",
        "attempt-summary.json",
        "pdf_visual_gate_attempt_stage_count:",
        "pdf_visual_gate_attempt_passed_stage_count:",
        "pdf_visual_gate_attempt_failed_stage_count:",
        "pdf_visual_gate_attempt_incomplete_stage_count:",
        "pdf_visual_gate_attempt_pdf_cli_export_status:",
        "pdf_visual_gate_attempt_pdf_regression_status:",
        "pdf_visual_gate_attempt_pdf_regression_selected_test_count:",
        "pdf_visual_gate_attempt_pdf_regression_failed_test_count:",
        "pdf_visual_gate_attempt_pdf_regression_skipped_test_count:",
        "pdf_visual_gate_attempt_unicode_font_status:",
        "pdf_visual_gate_attempt_cjk_copy_search_status:",
        "pdf_visual_gate_attempt_cjk_copy_search_count:",
        "pdf_visual_gate_attempt_cjk_copy_search_missing_text_count:",
        "pdf_visual_gate_attempt_visual_baseline_render_status:",
        "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count:",
        "pdf_visual_gate_attempt_expected_visual_render_count:",
        "pdf_visual_gate_attempt_visual_baseline_missing_pdf_count:",
        "pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes:",
        "pdf_visual_gate_attempt_visual_baseline_png_page_count:",
        "pdf_visual_gate_attempt_visual_baseline_missing_png_page_count:",
        "pdf_visual_gate_attempt_visual_baseline_png_total_bytes:",
        "pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count:",
        "pdf_visual_gate_attempt_aggregate_contact_sheet_status:",
        "pdf_visual_gate_attempt_aggregate_contact_sheet_display:"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll -Text $Content -Anchor "source_report:" -Needles $sourceReportBlockNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff must keep PDF visual gate bounded attempt status, verdict, auxiliary scope, CTest counts, CJK evidence, render progress, artifact completeness, and contact-sheet status in the same source_report block."
    }
}

function Add-ReleaseGovernanceHandoffPdfVisualSegmentedGateTraceViolations {
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
        "pdf_visual_segmented_gate_status",
        "pdf_visual_segmented_gate_summary_json_display",
        "segmented_visual_gate_auxiliary_only"
    ))) {
        return
    }

    $label = "release governance handoff PDF visual segmented gate trace"
    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "featherdoc.release_candidate_summary",
        "pdf_visual_segmented_gate_status:",
        "pdf_visual_segmented_gate_verdict:",
        "pdf_visual_segmented_gate_full_visual_gate_status:",
        "pdf_visual_segmented_gate_evidence_scope:",
        "segmented_visual_gate_auxiliary_only",
        "pdf_visual_segmented_gate_boundary:",
        "segmented_summary_does_not_replace_full_visual_gate_verdict",
        "pdf_visual_segmented_gate_summary_json_display:",
        "segmented-summary.json",
        "pdf_visual_segmented_gate_slice_summary_count:",
        "pdf_visual_segmented_gate_slice_pass_count:",
        "pdf_visual_segmented_gate_slice_failed_count:",
        "pdf_visual_segmented_gate_covered_baseline_count:",
        "pdf_visual_segmented_gate_expected_visual_render_count:",
        "pdf_visual_segmented_gate_attempt_stage_count:",
        "pdf_visual_segmented_gate_attempt_passed_stage_count:",
        "pdf_visual_segmented_gate_visual_baseline_render_status:",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_status:",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes:",
        "pdf_visual_segmented_gate_aggregate_rebuild_status:",
        "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count:"
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
            -Text "Release governance handoff must keep PDF visual segmented gate status, verdict, auxiliary scope, summary path, slice coverage, contact-sheet status, and rebuild evidence in the same release-candidate source_report block."
    }
}

function Add-ReleaseBlockerRollupPdfVisualGateTraceViolations {
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
        "pdf_visual_gate_verdict",
        "pdf_visual_gate_summary_json_display",
        "pdf_bounded_ctest_summary_count"
    ))) {
        return
    }

    $label = "release blocker rollup PDF visual gate trace"
    if (-not $Content.Contains("Source Report Contracts")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup lost Source Report Contracts while carrying PDF visual gate evidence."
    }

    $sourceReportBlockNeedles = @(
        "featherdoc.release_candidate_summary",
        "pdf_visual_gate_status:",
        "full_visual_gate_status:",
        "pdf_visual_gate_verdict:",
        "pdf_visual_gate_finalizable:",
        "pdf_visual_gate_summary_json_display:",
        "summary.json",
        "pdf_visual_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_gate_cjk_manifest_count:",
        "pdf_visual_gate_cjk_copy_search_count:",
        "pdf_visual_gate_cjk_missing_text_count:",
        "pdf_visual_gate_visual_baseline_manifest_count:",
        "pdf_visual_gate_visual_baseline_count:",
        "pdf_bounded_ctest_summary_count:",
        "pdf_bounded_ctest_pass_count:",
        "pdf_bounded_ctest_skipped_test_count:",
        "pdf_bounded_ctest_selected_test_count:",
        "pdf_bounded_ctest_subsets:",
        "pdf_bounded_ctest_summary_json_display:"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll `
        -Text $Content `
        -Anchor "featherdoc.release_candidate_summary" `
        -Needles $sourceReportBlockNeedles)) {
        $traceMarker = "pdf_visual_gate_rollup_material_safety_trace"
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup must keep PDF visual gate status, full visual status, verdict, summary/contact-sheet paths, CJK/baseline counts, and bounded CTest evidence in the same release-candidate Source Report Contracts block. Fixed marker: $traceMarker."
    }
}

function Add-ReleaseBlockerRollupPdfVisualGateAttemptTraceViolations {
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
        "pdf_visual_gate_attempt_status",
        "pdf_visual_gate_attempt_summary_json_display",
        "bounded_attempt_auxiliary_only"
    ))) {
        return
    }

    $label = "release blocker rollup PDF visual gate attempt trace"
    $sourceReportBlockNeedles = @(
        "featherdoc.release_candidate_summary",
        "pdf_visual_gate_attempt_status:",
        "pdf_visual_gate_attempt_verdict:",
        "pdf_visual_gate_attempt_full_visual_gate_status:",
        "pdf_visual_gate_attempt_evidence_scope:",
        "bounded_attempt_auxiliary_only",
        "pdf_visual_gate_attempt_summary_json_display:",
        "attempt-summary.json",
        "pdf_visual_gate_attempt_stage_count:",
        "pdf_visual_gate_attempt_passed_stage_count:",
        "pdf_visual_gate_attempt_failed_stage_count:",
        "pdf_visual_gate_attempt_incomplete_stage_count:",
        "pdf_visual_gate_attempt_outer_guard_status:",
        "timed_out",
        "pdf_visual_gate_attempt_outer_guard_timed_out:",
        "True",
        "pdf_visual_gate_attempt_outer_guard_timeout_seconds:",
        "60",
        "pdf_visual_gate_attempt_pdf_regression_selected_test_count:",
        "pdf_visual_gate_attempt_pdf_regression_failed_test_count:",
        "pdf_visual_gate_attempt_pdf_regression_skipped_test_count:",
        "pdf_visual_gate_attempt_cjk_copy_search_count:",
        "pdf_visual_gate_attempt_cjk_copy_search_missing_text_count:",
        "pdf_visual_gate_attempt_visual_baseline_render_status:",
        "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count:",
        "pdf_visual_gate_attempt_expected_visual_render_count:",
        "pdf_visual_gate_attempt_visual_baseline_missing_pdf_count:",
        "pdf_visual_gate_attempt_visual_baseline_pdf_total_bytes:",
        "pdf_visual_gate_attempt_visual_baseline_png_page_count:",
        "pdf_visual_gate_attempt_visual_baseline_missing_png_page_count:",
        "pdf_visual_gate_attempt_visual_baseline_png_total_bytes:",
        "pdf_visual_gate_attempt_visual_baseline_unreadable_png_dimension_count:",
        "pdf_visual_gate_attempt_aggregate_contact_sheet_status:",
        "pdf_visual_gate_attempt_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll `
        -Text $Content `
        -Anchor "featherdoc.release_candidate_summary" `
        -Needles $sourceReportBlockNeedles)) {
        $traceMarker = "pdf_visual_gate_attempt_rollup_material_safety_trace"
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup must keep PDF visual gate bounded attempt status, verdict, auxiliary scope, outer guard status, regression counts, CJK evidence, render progress, artifact completeness, and contact-sheet evidence in the same release-candidate Source Report Contracts block. Fixed marker: $traceMarker."
    }
}

function Add-ReleaseBlockerRollupPdfVisualSegmentedGateTraceViolations {
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
        "pdf_visual_segmented_gate_status",
        "pdf_visual_segmented_gate_summary_json_display",
        "segmented_visual_gate_auxiliary_only"
    ))) {
        return
    }

    $label = "release blocker rollup PDF visual segmented gate trace"
    $sourceReportBlockNeedles = @(
        "featherdoc.release_candidate_summary",
        "pdf_visual_segmented_gate_status:",
        "pdf_visual_segmented_gate_verdict:",
        "pdf_visual_segmented_gate_full_visual_gate_status:",
        "pdf_visual_segmented_gate_evidence_scope:",
        "segmented_visual_gate_auxiliary_only",
        "pdf_visual_segmented_gate_boundary:",
        "segmented_summary_does_not_replace_full_visual_gate_verdict",
        "pdf_visual_segmented_gate_summary_json_display:",
        "segmented-summary.json",
        "pdf_visual_segmented_gate_slice_summary_count:",
        "pdf_visual_segmented_gate_slice_pass_count:",
        "pdf_visual_segmented_gate_slice_failed_count:",
        "pdf_visual_segmented_gate_covered_baseline_count:",
        "pdf_visual_segmented_gate_expected_visual_render_count:",
        "pdf_visual_segmented_gate_attempt_stage_count:",
        "pdf_visual_segmented_gate_attempt_passed_stage_count:",
        "pdf_visual_segmented_gate_visual_baseline_render_status:",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_status:",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes:",
        "pdf_visual_segmented_gate_aggregate_rebuild_status:",
        "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count:"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll `
        -Text $Content `
        -Anchor "featherdoc.release_candidate_summary" `
        -Needles $sourceReportBlockNeedles)) {
        $traceMarker = "pdf_visual_segmented_gate_rollup_material_safety_trace"
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release blocker rollup must keep PDF visual segmented gate status, verdict, auxiliary scope, summary path, slice coverage, contact-sheet status, and rebuild evidence in the same release-candidate Source Report Contracts block. Fixed marker: $traceMarker."
    }
}
