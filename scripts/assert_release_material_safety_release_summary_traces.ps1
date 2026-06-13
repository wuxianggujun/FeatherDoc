function Add-ReleaseEntryPdfVisualGateTraceViolations {
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
        "PDF visual gate summary",
        "PDF visual gate aggregate contact sheet"
    ))) {
        return
    }

    $label = "release entry PDF visual gate trace"
    if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
        "PDF release readiness checklist",
        "docs/pdf_release_readiness_checklist_zh.rst"
    ))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Entry document must point reviewers at docs/pdf_release_readiness_checklist_zh.rst when PDF visual gate evidence is present."
    }

    if ($leafName -in @("start_here.md", "artifact_guide.md")) {
        $evidenceNeedles = @(
            "PDF visual gate summary:",
            "summary.json",
            "PDF CJK manifest samples:",
            "PDF CJK copy/search samples:",
            "PDF visual baseline manifest samples:",
            "PDF visual baselines:",
            "PDF visual gate aggregate contact sheet:",
            "aggregate-contact-sheet.png"
        )

        if (-not (Test-MarkdownListRunContainsAll -Text $Content -Anchor "PDF visual gate summary:" -Needles $evidenceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document must keep PDF visual gate summary, counts, and contact-sheet evidence in the same Markdown list run."
        }

        return
    }

    $finalizeEvidenceNeedles = @(
        "Confirm the PDF visual gate finalize evidence",
        'verdict `',
        "summary ",
        "summary.json",
        "aggregate contact sheet",
        "aggregate-contact-sheet.png",
        "CJK manifest samples",
        "CJK copy/search samples",
        "missing text",
        "visual baseline manifest samples",
        "visual baselines"
    )
    if (Test-TextLineContainsAll -Text $Content -Needles $finalizeEvidenceNeedles) {
        return
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("Confirm PDF visual gate summary", "summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Reviewer checklist must keep the PDF visual gate summary path on the summary confirmation line."
    }

    foreach ($needle in @(
        "CJK copy/search samples",
        "visual baselines"
    )) {
        if (-not (Test-TextLineContainsAll -Text $Content -Needles @("Confirm PDF visual gate summary", $needle))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Reviewer checklist must keep PDF visual gate marker '$needle' on the summary confirmation line."
        }
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("Confirm PDF visual gate aggregate contact sheet", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Reviewer checklist must keep the PDF visual gate contact-sheet path on the contact-sheet confirmation line."
    }
}

function Add-ReleaseSummaryProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_summary.zh-cn.md") {
        return
    }

    $label = "release summary project template governance trace"

    if ($Content.Contains("project-template readiness governance contract")) {
        foreach ($needle in @(
            "project-template readiness governance contract",
            "status=",
            "release_ready=",
            "latest_schema_approval_gate_status=",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceLineContainsAll -Text $Content -Anchor "project-template readiness governance contract" -Needles @($needle))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release summary lost project template readiness trace marker '$needle'."
            }
        }

        foreach ($fieldName in @("source_report_display", "source_json_display")) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "project-template readiness governance contract" `
                -FieldName $fieldName `
                -Needles @("project_template_delivery_readiness", "project-template-delivery-readiness"))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release summary project template readiness $fieldName must identify the delivery readiness evidence source."
            }
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "project-template readiness governance contract" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template readiness status must be a recognized readiness state."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "project-template readiness governance contract" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template readiness release_ready must be true or false."
        }
    }

    if ($Content.Contains("project-template onboarding governance contract")) {
        foreach ($needle in @(
            "project-template onboarding governance contract",
            "status=",
            "release_ready=",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceLineContainsAll -Text $Content -Anchor "project-template onboarding governance contract" -Needles @($needle))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release summary lost project template onboarding trace marker '$needle'."
            }
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
            -Text $Content `
            -Anchor "project-template onboarding governance contract" `
            -FieldName "source_report_display" `
            -Needles @(
                "project_template_delivery_readiness",
                "project-template-delivery-readiness",
                "project_template_onboarding_governance",
                "project-template-onboarding-governance"
            ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template onboarding source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
            -Text $Content `
            -Anchor "project-template onboarding governance contract" `
            -FieldName "source_json_display" `
            -Needles @("project_template_onboarding_governance", "project-template-onboarding-governance"))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template onboarding source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "project-template onboarding governance contract" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "project-template onboarding governance contract" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template onboarding release_ready must be true or false."
        }
    }
}

function Add-ReleaseSummaryPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_summary.zh-cn.md") {
        return
    }

    if (-not $Content.Contains("PDF visual gate")) {
        return
    }

    $label = "release summary PDF visual gate trace"
    foreach ($needle in @(
        "PDF visual gate",
        "verdict=",
        "summary=",
        "summary.json",
        "aggregate-contact-sheet.png",
        "cjk_manifest_count=",
        "cjk_copy_search_count=",
        "visual_baseline_manifest_count=",
        "visual_baseline_count="
    )) {
        if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate", $needle))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary must keep PDF visual gate marker '$needle' on the PDF visual gate summary line."
        }
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate", "verdict=pass")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate", "verdict=fail"))
    )) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release summary must keep a pass/fail PDF visual gate verdict on the PDF visual gate summary line."
    }
}

function Add-ReleaseBodyProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_body.zh-cn.md") {
        return
    }

    $label = "release body project template governance trace"

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_delivery_readiness", "project_template_delivery_readiness_contract")) {
        foreach ($needle in @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "source_schema=featherdoc.project_template_delivery_readiness_report.v1",
            "status=",
            "release_ready=",
            "latest_schema_approval_gate_status=",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceLineContainsAll -Text $Content -Anchor "Project template readiness:" -Needles @($needle))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release body lost project template readiness trace marker '$needle'."
            }
        }

        foreach ($fieldName in @("source_report_display", "source_json_display")) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "Project template readiness:" `
                -FieldName $fieldName `
                -Needles @("project_template_delivery_readiness", "project-template-delivery-readiness"))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release body project template readiness $fieldName must identify the delivery readiness evidence source."
            }
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "Project template readiness:" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template readiness status must be a recognized readiness state."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "Project template readiness:" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template readiness release_ready must be true or false."
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_onboarding.schema_approval", "project_template_onboarding_governance_contract")) {
        foreach ($needle in @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance",
            "project_template_onboarding_governance_contract",
            "source_schema=featherdoc.project_template_onboarding_governance_report.v1",
            "status=",
            "release_ready=",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceLineContainsAll -Text $Content -Anchor "Project template onboarding:" -Needles @($needle))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release body lost project template onboarding trace marker '$needle'."
            }
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
            -Text $Content `
            -Anchor "Project template onboarding:" `
            -FieldName "source_report_display" `
            -Needles @(
                "project_template_delivery_readiness",
                "project-template-delivery-readiness",
                "project_template_onboarding_governance",
                "project-template-onboarding-governance"
            ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template onboarding source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
            -Text $Content `
            -Anchor "Project template onboarding:" `
            -FieldName "source_json_display" `
            -Needles @("project_template_onboarding_governance", "project-template-onboarding-governance"))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template onboarding source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "Project template onboarding:" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "Project template onboarding:" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template onboarding release_ready must be true or false."
        }
    }
}
