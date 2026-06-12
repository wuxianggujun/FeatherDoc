function Add-SummaryGroup {
    param([object[]]$Items, [string]$PropertyName, [string]$OutputName)

    $countsByName = @{}
    foreach ($item in @($Items)) {
        $name = Get-JsonString -Object $item -Name $PropertyName
        if (-not $countsByName.ContainsKey($name)) {
            $countsByName[$name] = 0
        }

        $countsByName[$name] = [int]$countsByName[$name] + 1
    }

    return @(
        foreach ($groupName in @($countsByName.Keys |
            Sort-Object -Property @{ Expression = { [int]$countsByName[$_] }; Descending = $true }, @{ Expression = { [string]$_ }; Ascending = $true })) {
            $summary = [ordered]@{}
            $summary[$OutputName] = [string]$groupName
            $summary["count"] = [int]$countsByName[$groupName]
            $summary
        }
    )
}

function Get-SummaryGroupMarkdownText {
    param(
        [object[]]$Items,
        [string]$PropertyName
    )

    $parts = @(
        foreach ($item in @($Items)) {
            $name = Get-JsonString -Object $item -Name $PropertyName
            if ([string]::IsNullOrWhiteSpace($name)) {
                $name = "(empty)"
            }
            $count = Get-JsonInt -Object $item -Name "count"
            "$name=$count"
        }
    )
    if ($parts.Count -eq 0) {
        return "(none)"
    }

    return ($parts -join ", ")
}

function Add-TraceabilityMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Item
    )

    foreach ($fieldName in @(
            "source_report",
            "source_json",
            "origin_source_report",
            "origin_source_report_display")) {
        $fieldValue = Get-JsonString -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            $Lines.Add("  - ${fieldName}: ``$fieldValue``") | Out-Null
        }
    }
}

function Add-ReadinessActionEvidenceMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Item
    )

    $evidenceItems = @(Get-JsonArray -Object $Item -Name "readiness_action_evidence")
    $evidenceCount = Get-JsonString -Object $Item -Name "readiness_action_evidence_count"
    if ([string]::IsNullOrWhiteSpace($evidenceCount)) {
        if ($evidenceItems.Count -eq 0) {
            return
        }

        $evidenceCount = [string]$evidenceItems.Count
    }

    $Lines.Add("  - readiness_action_evidence_count: ``$evidenceCount``") | Out-Null
    if ($evidenceItems.Count -eq 0) {
        return
    }

    $Lines.Add("  - readiness_action_evidence:") | Out-Null
    foreach ($evidence in $evidenceItems) {
        $id = Get-JsonString -Object $evidence -Name "id" -DefaultValue "(unknown evidence)"
        $action = Get-JsonString -Object $evidence -Name "action"
        $issueKey = Get-JsonString -Object $evidence -Name "issue_key"
        $item = Get-JsonString -Object $evidence -Name "item"
        $Lines.Add("    - ``${id}``: action=``$action`` issue_key=``$issueKey`` item=``$item``") | Out-Null
        foreach ($fieldName in @("source_schema", "source_report", "source_report_display", "source_json", "source_json_display")) {
            $fieldValue = Get-JsonString -Object $evidence -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                $Lines.Add("      - ${fieldName}: ``$fieldValue``") | Out-Null
            }
        }
    }
}

function Add-ManifestSignoffEntrypointsMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Report
    )

    $status = Get-JsonString -Object $Report -Name "manifest_signoff_entrypoints_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "manifest_signoff_entrypoints_status",
            "manifest_signoff_entrypoints_release_assets_manifest",
            "manifest_signoff_entrypoints_release_assets_manifest_display",
            "manifest_signoff_entrypoints_required_entrypoint_count",
            "manifest_signoff_entrypoints_entrypoint_ids",
            "manifest_signoff_entrypoints_required_contracts",
            "manifest_signoff_entrypoints_required_fields",
            "manifest_signoff_entrypoints_checklist_marker"
        )) {
        $fieldValue = Get-JsonProperty -Object $Report -Name $fieldName
        $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
            @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
        } else {
            [string]$fieldValue
        }
        if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
        }
    }

    $entrypoints = @(Get-JsonArray -Object $Report -Name "manifest_signoff_entrypoints_entrypoints")
    if ($entrypoints.Count -eq 0) {
        return
    }

    $Lines.Add("  - manifest_signoff_entrypoints:") | Out-Null
    foreach ($entrypoint in $entrypoints) {
        $id = Get-JsonString -Object $entrypoint -Name "id" -DefaultValue "(unknown entrypoint)"
        $required = Get-JsonProperty -Object $entrypoint -Name "required"
        $pathDisplay = Get-JsonString -Object $entrypoint -Name "path_display"
        $Lines.Add("    - ``${id}``: required=``$required`` path_display=``$pathDisplay``") | Out-Null
    }
}

function Add-ProjectTemplateReadinessChecklistEntrypointsMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Report
    )

    $status = Get-JsonString -Object $Report -Name "project_template_readiness_checklist_entrypoints_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "project_template_readiness_checklist_entrypoints_status",
            "project_template_readiness_checklist_entrypoints_checklist_label",
            "project_template_readiness_checklist_entrypoints_checklist_path",
            "project_template_readiness_checklist_entrypoints_required_entrypoint_count",
            "project_template_readiness_checklist_entrypoints_entrypoint_ids",
            "project_template_readiness_checklist_entrypoints_checklist_marker"
        )) {
        $fieldValue = Get-JsonProperty -Object $Report -Name $fieldName
        $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
            @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
        } else {
            [string]$fieldValue
        }
        if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
        }
    }

    $entrypoints = @(Get-JsonArray -Object $Report -Name "project_template_readiness_checklist_entrypoints_entrypoints")
    if ($entrypoints.Count -eq 0) {
        return
    }

    $Lines.Add("  - project_template_readiness_checklist_entrypoints:") | Out-Null
    foreach ($entrypoint in $entrypoints) {
        $id = Get-JsonString -Object $entrypoint -Name "id" -DefaultValue "(unknown entrypoint)"
        $required = Get-JsonProperty -Object $entrypoint -Name "required"
        $pathDisplay = Get-JsonString -Object $entrypoint -Name "path_display"
        $Lines.Add("    - ``${id}``: required=``$required`` path_display=``$pathDisplay``") | Out-Null
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Report
    )

    $status = Get-JsonString -Object $Report -Name "release_entry_project_template_readiness_checklist_material_safety_audit_status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        return
    }

    foreach ($fieldName in @(
            "release_entry_project_template_readiness_checklist_material_safety_audit_status",
            "release_entry_project_template_readiness_checklist_material_safety_audit_script",
            "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count",
            "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field",
            "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema",
            "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path",
            "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker",
            "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
        )) {
        $fieldValue = Get-JsonProperty -Object $Report -Name $fieldName
        $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
            @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
        } else {
            [string]$fieldValue
        }
        if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
        }
    }
}

function Add-WordVisualStandardReviewMetadataMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Report
    )

    $metadataCount = Get-JsonProperty -Object $Report -Name "word_visual_standard_review_metadata_count"
    $metadata = @(Get-JsonArray -Object $Report -Name "word_visual_standard_review_metadata")
    if ($null -eq $metadataCount -and $metadata.Count -eq 0) {
        return
    }

    foreach ($fieldName in @(
            "word_visual_standard_review_metadata_count",
            "word_visual_standard_review_task_keys",
            "word_visual_standard_review_status_summary",
            "word_visual_standard_review_verdict_summary"
        )) {
        $fieldValue = Get-JsonProperty -Object $Report -Name $fieldName
        $fieldDisplay = if ($fieldValue -is [System.Collections.IEnumerable] -and $fieldValue -isnot [string]) {
            @($fieldValue | ForEach-Object { [string]$_ }) -join ", "
        } else {
            [string]$fieldValue
        }
        if ($null -ne $fieldValue -and -not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("  - ${fieldName}: ``$fieldDisplay``") | Out-Null
        }
    }

    if ($metadata.Count -eq 0) {
        return
    }

    $Lines.Add("  - word_visual_standard_review_metadata:") | Out-Null
    foreach ($entry in $metadata) {
        $taskKey = Get-JsonString -Object $entry -Name "task_key" -DefaultValue "(unknown task)"
        $reviewTaskKey = Get-JsonString -Object $entry -Name "review_task_key"
        $verdict = Get-JsonString -Object $entry -Name "verdict"
        $reviewStatus = Get-JsonString -Object $entry -Name "review_status"
        $reviewMethod = Get-JsonString -Object $entry -Name "review_method"
        $Lines.Add("    - ``${taskKey}``: review_task_key=``$reviewTaskKey`` verdict=``$verdict`` review_status=``$reviewStatus`` review_method=``$reviewMethod``") | Out-Null
        foreach ($fieldName in @("label", "reviewed_at", "review_result_path", "final_review_path")) {
            $fieldValue = Get-JsonString -Object $entry -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                $Lines.Add("      - ${fieldName}: ``$fieldValue``") | Out-Null
            }
        }
    }
}
