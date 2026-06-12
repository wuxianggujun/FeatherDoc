function Add-ManifestSignoffEntrypointsEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary,
        [string]$RepoRoot
    )

    $signoff = Get-JsonProperty -Object $Summary -Name "manifest_signoff_entrypoints"
    if ($null -eq $signoff) {
        return
    }

    $releaseAssetsManifest = Get-JsonString -Object $signoff -Name "release_assets_manifest"
    $releaseAssetsManifestDisplay = ""
    if (-not [string]::IsNullOrWhiteSpace($releaseAssetsManifest)) {
        try {
            $resolvedManifest = Resolve-RepoPath -RepoRoot $RepoRoot -Path $releaseAssetsManifest -AllowMissing
            $releaseAssetsManifestDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $resolvedManifest
        } catch {
            $normalizedManifest = $releaseAssetsManifest -replace '/', '\'
            $looksRooted = $normalizedManifest.StartsWith('\\') -or $normalizedManifest -match '^[A-Za-z]:\\'
            $releaseAssetsManifestDisplay = if ($normalizedManifest.StartsWith('.\') -or $looksRooted) {
                $normalizedManifest
            } else {
                ".\$normalizedManifest"
            }
        }
    }

    $entrypointEvidence = @(
        foreach ($entrypoint in @(Get-JsonArray -Object $signoff -Name "entrypoints")) {
            $id = Get-JsonString -Object $entrypoint -Name "id"
            if ([string]::IsNullOrWhiteSpace($id)) {
                continue
            }

            [ordered]@{
                id = $id
                required = Get-JsonBool -Object $entrypoint -Name "required"
                path_display = Get-JsonString -Object $entrypoint -Name "path_display"
            }
        }
    )

    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_status" `
        -Value (Get-JsonString -Object $signoff -Name "status")
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_release_assets_manifest" `
        -Value $releaseAssetsManifest
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_release_assets_manifest_display" `
        -Value $releaseAssetsManifestDisplay
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_required_entrypoint_count" `
        -Value (Get-FirstJsonProperty -Object $signoff -Names @("required_entrypoint_count"))
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_entrypoint_ids" `
        -Value @($entrypointEvidence | ForEach-Object { [string]$_.id })
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_entrypoints" `
        -Value @($entrypointEvidence)
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_required_contracts" `
        -Value @(Get-JsonArray -Object $signoff -Name "required_contracts")
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_required_fields" `
        -Value @(Get-JsonArray -Object $signoff -Name "required_fields")
    Set-OptionalSourceReportField -Target $Target -Name "manifest_signoff_entrypoints_checklist_marker" `
        -Value (Get-JsonString -Object $signoff -Name "checklist_marker")
}

function Add-ProjectTemplateReadinessChecklistEntrypointsEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary
    )

    $entrypointsContract = Get-JsonProperty -Object $Summary -Name "project_template_readiness_checklist_entrypoints"
    if ($null -eq $entrypointsContract) {
        return
    }

    $entrypointEvidence = @(
        foreach ($entrypoint in @(Get-JsonArray -Object $entrypointsContract -Name "entrypoints")) {
            $id = Get-JsonString -Object $entrypoint -Name "id"
            if ([string]::IsNullOrWhiteSpace($id)) {
                continue
            }

            [ordered]@{
                id = $id
                required = Get-JsonBool -Object $entrypoint -Name "required"
                path_display = Get-JsonString -Object $entrypoint -Name "path_display"
            }
        }
    )

    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_status" `
        -Value (Get-JsonString -Object $entrypointsContract -Name "status")
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_checklist_label" `
        -Value (Get-JsonString -Object $entrypointsContract -Name "checklist_label")
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_checklist_path" `
        -Value (Get-JsonString -Object $entrypointsContract -Name "checklist_path")
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_required_entrypoint_count" `
        -Value (Get-FirstJsonProperty -Object $entrypointsContract -Names @("required_entrypoint_count"))
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_entrypoint_ids" `
        -Value @($entrypointEvidence | ForEach-Object { [string]$_.id })
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_entrypoints" `
        -Value @($entrypointEvidence)
    Set-OptionalSourceReportField -Target $Target -Name "project_template_readiness_checklist_entrypoints_checklist_marker" `
        -Value (Get-JsonString -Object $entrypointsContract -Name "checklist_marker")
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary
    )

    $audit = Get-JsonProperty -Object $Summary -Name "release_entry_project_template_readiness_checklist_material_safety_audit"
    if ($null -eq $audit) {
        return
    }

    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_status" `
        -Value (Get-JsonString -Object $audit -Name "status")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_script" `
        -Value (Get-JsonString -Object $audit -Name "audit_script")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count" `
        -Value (Get-FirstJsonProperty -Object $audit -Names @("audited_entrypoint_count"))
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints" `
        -Value @(Get-JsonArray -Object $audit -Name "audited_entrypoints" | ForEach-Object { [string]$_ })
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label" `
        -Value (Get-JsonString -Object $audit -Name "compact_evidence_label")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field" `
        -Value (Get-JsonString -Object $audit -Name "compact_evidence_field")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema" `
        -Value (Get-JsonString -Object $audit -Name "compact_evidence_source_schema")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path" `
        -Value (Get-JsonString -Object $audit -Name "checklist_path")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker" `
        -Value (Get-JsonString -Object $audit -Name "checklist_marker")
    Set-OptionalSourceReportField -Target $Target -Name "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker" `
        -Value (Get-JsonString -Object $audit -Name "material_safety_marker")
}

function Get-WordVisualStandardReviewMetadataSummaryText {
    param(
        [object[]]$Items,
        [string]$PropertyName
    )

    $values = @(
        foreach ($item in @($Items)) {
            $value = Get-JsonString -Object $item -Name $PropertyName
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                $value
            }
        }
    )

    if ($values.Count -eq 0) {
        return ""
    }

    return @(
        $values |
            Group-Object |
            Sort-Object -Property @{ Expression = "Count"; Descending = $true }, @{ Expression = "Name"; Ascending = $true } |
            ForEach-Object { "$($_.Name)=$($_.Count)" }
    ) -join ", "
}

function Get-WordVisualStandardReviewMetadataEntries {
    param($Summary)

    return @(
        foreach ($entry in @(Get-JsonArray -Object $Summary -Name "word_visual_standard_review_metadata")) {
            $taskKey = Get-JsonString -Object $entry -Name "task_key"
            if ([string]::IsNullOrWhiteSpace($taskKey)) {
                continue
            }

            [ordered]@{
                task_key = $taskKey
                review_task_key = Get-JsonString -Object $entry -Name "review_task_key"
                label = Get-JsonString -Object $entry -Name "label"
                verdict = Get-JsonString -Object $entry -Name "verdict"
                review_status = Get-JsonString -Object $entry -Name "review_status"
                reviewed_at = Get-JsonString -Object $entry -Name "reviewed_at"
                review_method = Get-JsonString -Object $entry -Name "review_method"
                review_result_path = Get-JsonString -Object $entry -Name "review_result_path"
                final_review_path = Get-JsonString -Object $entry -Name "final_review_path"
            }
        }
    )
}

function Add-WordVisualStandardReviewMetadataEvidenceFields {
    param(
        [System.Collections.IDictionary]$Target,
        $Summary
    )

    $metadata = @(Get-WordVisualStandardReviewMetadataEntries -Summary $Summary)
    $count = Get-FirstJsonProperty -Object $Summary -Names @("word_visual_standard_review_metadata_count")
    if ($null -eq $count -and $metadata.Count -eq 0) {
        return
    }
    if ($null -eq $count) {
        $count = $metadata.Count
    }

    Set-OptionalSourceReportField -Target $Target -Name "word_visual_standard_review_metadata_count" `
        -Value $count
    Set-OptionalSourceReportField -Target $Target -Name "word_visual_standard_review_task_keys" `
        -Value @($metadata | ForEach-Object { [string]$_.task_key })
    Set-OptionalSourceReportField -Target $Target -Name "word_visual_standard_review_status_summary" `
        -Value (Get-WordVisualStandardReviewMetadataSummaryText -Items $metadata -PropertyName "review_status")
    Set-OptionalSourceReportField -Target $Target -Name "word_visual_standard_review_verdict_summary" `
        -Value (Get-WordVisualStandardReviewMetadataSummaryText -Items $metadata -PropertyName "verdict")
    Set-OptionalSourceReportField -Target $Target -Name "word_visual_standard_review_metadata" `
        -Value @($metadata)
}
