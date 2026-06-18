function Convert-GovernanceMetricDetailValueToComparableString {
    param($Value)

    if ($null -eq $Value) {
        return $null
    }

    if ($Value -is [string]) {
        return [string]$Value
    }

    if ($Value -is [System.Collections.IDictionary]) {
        return [string]$Value
    }

    if ($Value -is [System.Collections.IEnumerable]) {
        $separator = [string][char]31
        return (@($Value |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }) -join $separator)
    }

    return [string]$Value
}

function Add-ProjectTemplateOnboardingGovernanceContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "project template onboarding governance contract"
    $contract = Get-JsonPropertyValue -Object $Json -Name "project_template_onboarding_governance_contract"
    if ($null -eq $contract) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing project_template_onboarding_governance_contract."
        return
    }

    $schema = Get-JsonPropertyValue -Object $contract -Name "schema"
    if ([string]$schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.schema is invalid."
    }

    $sourceSchema = Get-JsonPropertyValue -Object $contract -Name "source_schema"
    if ([string]$sourceSchema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_schema is invalid."
    }

    $status = Get-JsonPropertyValue -Object $contract -Name "status"
    if ([string]::IsNullOrWhiteSpace([string]$status)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status is missing."
    }
    if (-not (Test-StringValueInSet -Value $status -AllowedValues $ProjectTemplateReadinessStatusValues)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status must be a recognized readiness state."
    }

    $sourceJsonDisplay = Get-JsonPropertyValue -Object $contract -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceJsonDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_json_display is missing."
    }

    $sourceReportDisplay = Get-JsonPropertyValue -Object $contract -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceReportDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_report_display is missing."
    }

    $onboardingGovernanceSourceNeedles = @("project_template_onboarding_governance", "project-template-onboarding-governance")
    Add-SourceDisplayIdentityViolations `
        -File $File `
        -Violations $Violations `
        -Label $label `
        -FieldName "project_template_onboarding_governance_contract.source_json_display" `
        -Value $sourceJsonDisplay `
        -Needles $onboardingGovernanceSourceNeedles `
        -EvidenceDescription "project template onboarding governance"
    Add-SourceDisplayIdentityViolations `
        -File $File `
        -Violations $Violations `
        -Label $label `
        -FieldName "project_template_onboarding_governance_contract.source_report_display" `
        -Value $sourceReportDisplay `
        -Needles $onboardingGovernanceSourceNeedles `
        -EvidenceDescription "project template onboarding governance"

    $releaseReady = Get-JsonPropertyValue -Object $contract -Name "release_ready"
    if ($null -eq $releaseReady) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.release_ready is missing."
    }
    if (-not (Test-StringValueInSet -Value $releaseReady -AllowedValues $ProjectTemplateBooleanValues)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.release_ready must be true or false."
    }

    Add-ProjectTemplateReviewerActionContractViolations `
        -File $File `
        -Contract $contract `
        -Violations $Violations `
        -Label $label `
        -ContractName "project_template_onboarding_governance_contract"

    $statusSummary = Get-JsonPropertyValue -Object $contract -Name "schema_approval_status_summary"
    if ($null -eq $statusSummary -or
        ($statusSummary -is [string] -and [string]::IsNullOrWhiteSpace($statusSummary)) -or
        @($statusSummary).Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.schema_approval_status_summary is missing."
    }

    $nextAction = Get-JsonPropertyValue -Object $contract -Name "next_action"
    if ($null -eq $nextAction -or
        ($nextAction -is [string] -and [string]::IsNullOrWhiteSpace($nextAction)) -or
        @($nextAction).Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.next_action is missing."
    }

    $nextActionSummary = Get-JsonPropertyValue -Object $contract -Name "next_action_summary"
    if ($null -eq $nextActionSummary -or
        ($nextActionSummary -is [string] -and [string]::IsNullOrWhiteSpace($nextActionSummary)) -or
        @($nextActionSummary).Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.next_action_summary is missing."
    }
    $nextActionSummaryCount = @($nextActionSummary).Count

    $integerValues = @{}
    foreach ($fieldName in @(
        "source_file_count",
        "source_failure_count",
        "entry_count",
        "next_action_group_count",
        "blocked_entry_count",
        "pending_review_entry_count",
        "not_evaluated_entry_count",
        "approved_entry_count",
        "not_required_entry_count",
        "release_blocker_count",
        "action_item_count",
        "manual_review_recommendation_count"
    )) {
        $fieldValue = Get-JsonPropertyValue -Object $contract -Name $fieldName
        if ($null -eq $fieldValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.$fieldName is missing."
            continue
        }

        try {
            $integerValues[$fieldName] = [int]$fieldValue
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.$fieldName must be an integer."
        }
    }

    if ($integerValues.ContainsKey("next_action_group_count") -and
        $nextActionSummaryCount -gt 0 -and
        $integerValues["next_action_group_count"] -lt $nextActionSummaryCount) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.next_action_group_count must cover next_action_summary."
    }

    $releaseReadyIsTrue = $false
    $releaseReadyIsFalse = $false
    if ($releaseReady -is [bool]) {
        $releaseReadyIsTrue = $releaseReady
        $releaseReadyIsFalse = -not $releaseReady
    } elseif ($null -ne $releaseReady) {
        $normalizedReleaseReady = ([string]$releaseReady).Trim().ToLowerInvariant()
        $releaseReadyIsTrue = ($normalizedReleaseReady -eq "true")
        $releaseReadyIsFalse = ($normalizedReleaseReady -eq "false")
    }

    if ($releaseReadyIsTrue -and [string]$status -ne "ready") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status must be ready when release_ready is true."
    }

    if ($releaseReadyIsFalse -and [string]$status -eq "ready") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status must not be ready when release_ready is false."
    }

    if ($integerValues.ContainsKey("entry_count")) {
        $entryStatusCountFields = @(
            "blocked_entry_count",
            "pending_review_entry_count",
            "not_evaluated_entry_count",
            "approved_entry_count",
            "not_required_entry_count"
        )
        $hasAllEntryStatusCounts = $true
        $entryStatusCountTotal = 0
        foreach ($fieldName in $entryStatusCountFields) {
            if (-not $integerValues.ContainsKey($fieldName)) {
                $hasAllEntryStatusCounts = $false
                break
            }
            $entryStatusCountTotal += $integerValues[$fieldName]
        }

        if ($hasAllEntryStatusCounts -and $entryStatusCountTotal -ne $integerValues["entry_count"]) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract entry status counts do not match entry_count."
        }
    }

    if ($releaseReadyIsFalse -and
        $integerValues.ContainsKey("release_blocker_count") -and
        $integerValues.ContainsKey("source_failure_count") -and
        (($integerValues["release_blocker_count"] + $integerValues["source_failure_count"]) -le 0)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract must have release blockers or source failures when release_ready is false."
    }
}

function Add-ManifestSignoffEntrypointsContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "manifest signoff entrypoints contract"
    $signoff = Get-JsonPropertyValue -Object $Json -Name "manifest_signoff_entrypoints"
    if ($null -eq $signoff) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing manifest_signoff_entrypoints."
        return
    }

    $status = Get-JsonPropertyValue -Object $signoff -Name "status"
    if ([string]$status -ne "declared") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.status must be declared."
    }

    $manifestPath = Get-JsonPropertyValue -Object $signoff -Name "release_assets_manifest"
    if ([string]::IsNullOrWhiteSpace([string]$manifestPath) -or
        -not ([string]$manifestPath).Contains("release_assets_manifest.json")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.release_assets_manifest must identify release_assets_manifest.json."
    }

    $requiredEntrypointCount = Get-JsonPropertyValue -Object $signoff -Name "required_entrypoint_count"
    try {
        if ([int]$requiredEntrypointCount -ne 3) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.required_entrypoint_count must be 3."
        }
    } catch {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.required_entrypoint_count must be an integer."
    }

    $entrypoints = @(Get-JsonArray -Object $signoff -Name "entrypoints")
    $entrypointsById = @{}
    foreach ($entrypoint in $entrypoints) {
        $entrypointId = [string](Get-JsonPropertyValue -Object $entrypoint -Name "id")
        if (-not [string]::IsNullOrWhiteSpace($entrypointId)) {
            $entrypointsById[$entrypointId] = $entrypoint
        }
    }

    foreach ($requiredEntrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
        if (-not $entrypointsById.ContainsKey($requiredEntrypointId)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.entrypoints is missing $requiredEntrypointId."
            continue
        }

        $entrypoint = $entrypointsById[$requiredEntrypointId]
        $entrypointRequired = Get-JsonPropertyValue -Object $entrypoint -Name "required"
        if (-not (Test-StringValueInSet -Value $entrypointRequired -AllowedValues @("true"))) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.entrypoints.$requiredEntrypointId.required must be true."
        }

        $entrypointPathDisplay = Get-JsonPropertyValue -Object $entrypoint -Name "path_display"
        if ([string]::IsNullOrWhiteSpace([string]$entrypointPathDisplay)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.entrypoints.$requiredEntrypointId.path_display is missing."
        }
    }

    $requiredContracts = @(Get-JsonArray -Object $signoff -Name "required_contracts" | ForEach-Object { [string]$_ })
    foreach ($requiredContract in @(
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract"
    )) {
        if (-not ($requiredContracts -contains $requiredContract)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.required_contracts is missing $requiredContract."
        }
    }

    $requiredFields = @(Get-JsonArray -Object $signoff -Name "required_fields" | ForEach-Object { [string]$_ })
    foreach ($requiredField in @(
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
        "source_report_display",
        "source_json_display"
    )) {
        if (-not ($requiredFields -contains $requiredField)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.required_fields is missing $requiredField."
        }
    }

    $checklistMarker = Get-JsonPropertyValue -Object $signoff -Name "checklist_marker"
    if ([string]$checklistMarker -ne "reviewer_manifest_scoped_project_template_trace") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.checklist_marker is invalid."
    }
}

function Add-ProjectTemplateReadinessChecklistEntrypointsContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "project-template readiness checklist entrypoints contract"
    $entrypointsContract = Get-JsonPropertyValue -Object $Json -Name "project_template_readiness_checklist_entrypoints"
    if ($null -eq $entrypointsContract) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing project_template_readiness_checklist_entrypoints."
        return
    }

    $status = Get-JsonPropertyValue -Object $entrypointsContract -Name "status"
    if ([string]$status -ne "declared") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.status must be declared."
    }

    $checklistLabel = Get-JsonPropertyValue -Object $entrypointsContract -Name "checklist_label"
    if ([string]$checklistLabel -ne "Project template release readiness checklist") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.checklist_label is invalid."
    }

    $checklistPath = Get-JsonPropertyValue -Object $entrypointsContract -Name "checklist_path"
    if ([string]::IsNullOrWhiteSpace([string]$checklistPath) -or
        -not ([string]$checklistPath).Contains("docs/project_template_release_readiness_checklist_zh.rst")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.checklist_path must identify docs/project_template_release_readiness_checklist_zh.rst."
    }

    $requiredEntrypointCount = Get-JsonPropertyValue -Object $entrypointsContract -Name "required_entrypoint_count"
    try {
        if ([int]$requiredEntrypointCount -ne 3) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.required_entrypoint_count must be 3."
        }
    } catch {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.required_entrypoint_count must be an integer."
    }

    $entrypoints = @(Get-JsonArray -Object $entrypointsContract -Name "entrypoints")
    $entrypointsById = @{}
    foreach ($entrypoint in $entrypoints) {
        $entrypointId = [string](Get-JsonPropertyValue -Object $entrypoint -Name "id")
        if (-not [string]::IsNullOrWhiteSpace($entrypointId)) {
            $entrypointsById[$entrypointId] = $entrypoint
        }
    }

    foreach ($requiredEntrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
        if (-not $entrypointsById.ContainsKey($requiredEntrypointId)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.entrypoints is missing $requiredEntrypointId."
            continue
        }

        $entrypoint = $entrypointsById[$requiredEntrypointId]
        $entrypointRequired = Get-JsonPropertyValue -Object $entrypoint -Name "required"
        if (-not (Test-StringValueInSet -Value $entrypointRequired -AllowedValues @("true"))) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.entrypoints.$requiredEntrypointId.required must be true."
        }

        $entrypointPathDisplay = Get-JsonPropertyValue -Object $entrypoint -Name "path_display"
        if ([string]::IsNullOrWhiteSpace([string]$entrypointPathDisplay)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.entrypoints.$requiredEntrypointId.path_display is missing."
        }
    }

    $checklistMarker = Get-JsonPropertyValue -Object $entrypointsContract -Name "checklist_marker"
    if ([string]$checklistMarker -ne "release_entry_project_template_readiness_checklist_trace") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.checklist_marker is invalid."
    }
}

function Add-ReleaseNoteBundleContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "release note bundle contract"
    $bundle = Get-JsonPropertyValue -Object $Json -Name "release_note_bundle"
    if ($null -eq $bundle) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing release_note_bundle."
        return
    }

    $status = Get-JsonPropertyValue -Object $bundle -Name "status"
    if ([string]$status -ne "declared") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.status must be declared."
    }

    foreach ($pathField in @("output_root", "report_dir")) {
        $pathValue = Get-JsonPropertyValue -Object $bundle -Name $pathField
        if ([string]::IsNullOrWhiteSpace([string]$pathValue)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.$pathField is missing."
        }
    }

    foreach ($countField in @("entrypoint_count", "required_entrypoint_count")) {
        $countValue = Get-JsonPropertyValue -Object $bundle -Name $countField
        try {
            if ([int]$countValue -ne 6) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.$countField must be 6."
            }
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.$countField must be an integer."
        }
    }

    $requiredEntrypoints = @(
        [ordered]@{ id = "start_here"; location = "summary_root"; path_fragment = "START_HERE.md" },
        [ordered]@{ id = "artifact_guide"; location = "report"; path_fragment = "report\ARTIFACT_GUIDE.md" },
        [ordered]@{ id = "reviewer_checklist"; location = "report"; path_fragment = "report\REVIEWER_CHECKLIST.md" },
        [ordered]@{ id = "release_handoff"; location = "report"; path_fragment = "report\release_handoff.md" },
        [ordered]@{ id = "release_body_zh_cn"; location = "report"; path_fragment = "report\release_body.zh-CN.md" },
        [ordered]@{ id = "release_summary_zh_cn"; location = "report"; path_fragment = "report\release_summary.zh-CN.md" }
    )

    $entrypointIds = @(Get-JsonArray -Object $bundle -Name "entrypoint_ids" | ForEach-Object { [string]$_ })
    if (@($entrypointIds).Count -ne 6) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoint_ids must contain exactly 6 entries."
    }
    foreach ($entrypointContract in $requiredEntrypoints) {
        $requiredEntrypointId = [string]$entrypointContract.id
        if (-not ($entrypointIds -contains $requiredEntrypointId)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoint_ids is missing $requiredEntrypointId."
        }
    }

    $entrypoints = @(Get-JsonArray -Object $bundle -Name "entrypoints")
    if (@($entrypoints).Count -ne 6) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoints must contain exactly 6 entries."
    }
    $entrypointsById = @{}
    foreach ($entrypoint in $entrypoints) {
        $entrypointId = [string](Get-JsonPropertyValue -Object $entrypoint -Name "id")
        if (-not [string]::IsNullOrWhiteSpace($entrypointId)) {
            $entrypointsById[$entrypointId] = $entrypoint
        }
    }

    foreach ($entrypointContract in $requiredEntrypoints) {
        $requiredEntrypointId = [string]$entrypointContract.id
        if (-not $entrypointsById.ContainsKey($requiredEntrypointId)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoints is missing $requiredEntrypointId."
            continue
        }

        $entrypoint = $entrypointsById[$requiredEntrypointId]
        $entrypointRequired = Get-JsonPropertyValue -Object $entrypoint -Name "required"
        if (-not (Test-StringValueInSet -Value $entrypointRequired -AllowedValues @("true"))) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoints.$requiredEntrypointId.required must be true."
        }

        $entrypointLocation = Get-JsonPropertyValue -Object $entrypoint -Name "location"
        if ([string]$entrypointLocation -ne [string]$entrypointContract.location) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoints.$requiredEntrypointId.location is invalid."
        }

        $entrypointPath = Get-JsonPropertyValue -Object $entrypoint -Name "path"
        if ([string]::IsNullOrWhiteSpace([string]$entrypointPath)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoints.$requiredEntrypointId.path is missing."
        }

        $entrypointPathDisplay = Get-JsonPropertyValue -Object $entrypoint -Name "path_display"
        if ([string]::IsNullOrWhiteSpace([string]$entrypointPathDisplay)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoints.$requiredEntrypointId.path_display is missing."
        } elseif (-not (([string]$entrypointPathDisplay -replace '/', '\').Contains([string]$entrypointContract.path_fragment))) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_note_bundle.entrypoints.$requiredEntrypointId.path_display does not identify $($entrypointContract.path_fragment)."
        }
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "release entry project-template readiness checklist material-safety audit contract"
    $audit = Get-JsonPropertyValue -Object $Json -Name "release_entry_project_template_readiness_checklist_material_safety_audit"
    if ($null -eq $audit) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing release_entry_project_template_readiness_checklist_material_safety_audit."
        return
    }

    if ([string](Get-JsonPropertyValue -Object $audit -Name "status") -ne "passed") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.status must be passed."
    }

    $auditScript = [string](Get-JsonPropertyValue -Object $audit -Name "audit_script")
    if ([string]::IsNullOrWhiteSpace($auditScript) -or
        -not $auditScript.Contains("scripts\assert_release_material_safety.ps1")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.audit_script must identify scripts/assert_release_material_safety.ps1."
    }

    $auditedEntrypointCount = Get-JsonPropertyValue -Object $audit -Name "audited_entrypoint_count"
    try {
        if ([int]$auditedEntrypointCount -ne 3) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.audited_entrypoint_count must be 3."
        }
    } catch {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.audited_entrypoint_count must be an integer."
    }

    $auditedEntrypoints = @(Get-JsonArray -Object $audit -Name "audited_entrypoints" | ForEach-Object { [string]$_ })
    foreach ($requiredEntrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
        if (-not ($auditedEntrypoints -contains $requiredEntrypointId)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.audited_entrypoints is missing $requiredEntrypointId."
        }
    }

    $expectedFields = [ordered]@{
        compact_evidence_label = "Project-template readiness checklist handoff evidence"
        compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
        compact_evidence_source_schema = "featherdoc.release_candidate_summary"
        checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
        checklist_marker = "release_entry_project_template_readiness_checklist_trace"
        material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    }
    foreach ($entry in $expectedFields.GetEnumerator()) {
        if ([string](Get-JsonPropertyValue -Object $audit -Name $entry.Key) -ne [string]$entry.Value) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.$($entry.Key) is invalid."
        }
    }
}

function Add-PdfVisualGateManifestContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "PDF visual gate manifest contract"
    $includedValue = Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_evidence_included"
    $status = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_status")
    $included = $false
    if ($includedValue -is [bool]) {
        $included = $includedValue
    } elseif ($null -ne $includedValue) {
        $included = ([string]$includedValue).Trim().ToLowerInvariant() -eq "true"
    }

    if (-not $included) {
        if ($status -eq "loaded") {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_status=loaded requires pdf_visual_gate_evidence_included=true."
        }
        return
    }

    if ($status -ne "loaded") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence_included=true requires pdf_visual_gate_status=loaded."
    }

    $topLevelSummary = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_summary_json")
    if ([string]::IsNullOrWhiteSpace($topLevelSummary)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_summary_json is missing."
    }

    $topLevelOutputDir = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_output_dir")
    if ([string]::IsNullOrWhiteSpace($topLevelOutputDir)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_output_dir is missing."
    }

    $evidence = Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_evidence"
    if ($null -eq $evidence) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence is missing."
        return
    }

    $evidenceStatus = [string](Get-JsonPropertyValue -Object $evidence -Name "status")
    if ($evidenceStatus -ne "loaded") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.status must be loaded."
    }

    $evidenceVerdict = ([string](Get-JsonPropertyValue -Object $evidence -Name "verdict")).Trim().ToLowerInvariant()
    if ([string]::IsNullOrWhiteSpace($evidenceVerdict)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.verdict is missing."
    } elseif ($evidenceVerdict -notin @("pass", "fail")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.verdict must be pass or fail."
    }

    $fullVisualGateStatus = ([string](Get-JsonPropertyValue -Object $evidence -Name "full_visual_gate_status")).Trim().ToLowerInvariant()
    if ([string]::IsNullOrWhiteSpace($fullVisualGateStatus)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.full_visual_gate_status is missing."
    } elseif ($fullVisualGateStatus -notin @("pass", "fail")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.full_visual_gate_status must be pass or fail."
    } elseif ($evidenceVerdict -in @("pass", "fail") -and $fullVisualGateStatus -ne $evidenceVerdict) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.full_visual_gate_status must match verdict."
    }

    $evidenceSummary = [string](Get-JsonPropertyValue -Object $evidence -Name "summary_json")
    if ([string]::IsNullOrWhiteSpace($evidenceSummary)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.summary_json is missing."
    } elseif (-not [string]::IsNullOrWhiteSpace($topLevelSummary) -and $evidenceSummary -ne $topLevelSummary) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.summary_json must match pdf_visual_gate_summary_json."
    }

    foreach ($fieldName in @(
            "aggregate_contact_sheet",
            "pdf_cli_export_log",
            "pdf_regression_log",
            "cjk_copy_search_log_dir",
            "unicode_font_log"
        )) {
        $fieldValue = [string](Get-JsonPropertyValue -Object $evidence -Name $fieldName)
        if ([string]::IsNullOrWhiteSpace($fieldValue)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName is missing."
        }
    }

    $manifestCountValues = @{}
    foreach ($countContract in @(
            @{ Name = "cjk_manifest_count"; Minimum = 43 },
            @{ Name = "cjk_copy_search_count"; Minimum = 1 },
            @{ Name = "cjk_missing_text_count"; Minimum = 0 },
            @{ Name = "visual_baseline_manifest_count"; Minimum = 42 },
            @{ Name = "visual_baseline_count"; Minimum = 1 }
        )) {
        $fieldName = $countContract.Name
        $fieldValue = Get-JsonPropertyValue -Object $evidence -Name $fieldName
        if ($null -eq $fieldValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName is missing."
            continue
        }

        try {
            $integerValue = [int]$fieldValue
            $manifestCountValues[$fieldName] = $integerValue
            if ($integerValue -lt [int]$countContract.Minimum) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName must be at least $($countContract.Minimum)."
            }
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName must be an integer."
        }
    }

    if ($evidenceVerdict -eq "pass" -and
        $manifestCountValues.ContainsKey("cjk_missing_text_count") -and
        $manifestCountValues["cjk_missing_text_count"] -ne 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.cjk_missing_text_count must be 0 when verdict is pass."
    }
}

function Add-WordVisualStandardReviewManifestContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "Word visual standard review manifest contract"
    $includedValue = Get-JsonPropertyValue -Object $Json -Name "visual_gate_evidence_included"
    $included = $false
    if ($includedValue -is [bool]) {
        $included = $includedValue
    } elseif ($null -ne $includedValue) {
        $included = ([string]$includedValue).Trim().ToLowerInvariant() -eq "true"
    }

    if (-not $included) {
        return
    }

    $metadataValue = Get-JsonPropertyValue -Object $Json -Name "word_visual_standard_review_metadata"
    if ($null -eq $metadataValue) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "word_visual_standard_review_metadata is missing."
        return
    }

    $metadata = @($metadataValue)
    $countValue = Get-JsonPropertyValue -Object $Json -Name "word_visual_standard_review_metadata_count"
    if ($null -eq $countValue) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "word_visual_standard_review_metadata_count is missing."
    } else {
        try {
            $declaredCount = [int]$countValue
            if ($declaredCount -ne 4 -or $declaredCount -ne $metadata.Count) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "word_visual_standard_review_metadata_count must match four standard review metadata entries."
            }
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "word_visual_standard_review_metadata_count must be an integer."
        }
    }

    $metadataByTask = @{}
    foreach ($entry in $metadata) {
        $taskKey = [string](Get-JsonPropertyValue -Object $entry -Name "task_key")
        if (-not [string]::IsNullOrWhiteSpace($taskKey)) {
            $metadataByTask[$taskKey] = $entry
        }
    }

    foreach ($taskContract in @(
            @{ TaskKey = "smoke"; ReviewTaskKey = "document" },
            @{ TaskKey = "fixed_grid"; ReviewTaskKey = "fixed_grid" },
            @{ TaskKey = "section_page_setup"; ReviewTaskKey = "section_page_setup" },
            @{ TaskKey = "page_number_fields"; ReviewTaskKey = "page_number_fields" }
        )) {
        $taskKey = [string]$taskContract.TaskKey
        if (-not $metadataByTask.ContainsKey($taskKey)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "word_visual_standard_review_metadata is missing task '$taskKey'."
            continue
        }

        $entry = $metadataByTask[$taskKey]
        $reviewTaskKey = [string](Get-JsonPropertyValue -Object $entry -Name "review_task_key")
        if ($reviewTaskKey -ne [string]$taskContract.ReviewTaskKey) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "word_visual_standard_review_metadata.$taskKey.review_task_key is invalid."
        }

        foreach ($fieldName in @("label", "verdict", "review_status", "reviewed_at", "review_method", "review_result_path", "final_review_path")) {
            $fieldValue = [string](Get-JsonPropertyValue -Object $entry -Name $fieldName)
            if ([string]::IsNullOrWhiteSpace($fieldValue)) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "word_visual_standard_review_metadata.$taskKey.$fieldName is missing."
            }
        }

        $reviewNote = Get-JsonPropertyValue -Object $entry -Name "review_note"
        if ($null -ne $reviewNote) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "word_visual_standard_review_metadata.$taskKey must not expose review_note."
        }
    }
}

function Add-ReleaseUploadRemoteAssetsContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $upload = Get-JsonPropertyValue -Object $Json -Name "upload"
    if ($null -eq $upload) {
        return
    }

    $uploadedValue = Get-JsonPropertyValue -Object $upload -Name "uploaded"
    $uploaded = $false
    if ($uploadedValue -is [bool]) {
        $uploaded = $uploadedValue
    } elseif ($null -ne $uploadedValue) {
        $uploaded = ([string]$uploadedValue).Trim().ToLowerInvariant() -eq "true"
    }

    if (-not $uploaded) {
        return
    }

    $label = "release upload remote assets contract"
    $releaseVersion = [string](Get-JsonPropertyValue -Object $Json -Name "release_version")
    if ([string]::IsNullOrWhiteSpace($releaseVersion)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_version is required when upload.uploaded is true."
    }

    $requestedTag = [string](Get-JsonPropertyValue -Object $upload -Name "requested_tag")
    if ([string]::IsNullOrWhiteSpace($requestedTag)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.requested_tag is required when upload.uploaded is true."
    } elseif (-not [string]::IsNullOrWhiteSpace($releaseVersion) -and $requestedTag -ne "v$releaseVersion") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.requested_tag must match release_version."
    }

    $releaseHost = ""
    $releaseUrl = [string](Get-JsonPropertyValue -Object $upload -Name "release_url")
    if ([string]::IsNullOrWhiteSpace($releaseUrl)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.release_url is required when upload.uploaded is true."
    } else {
        $releaseUri = $null
        $releaseUriIsValid = [System.Uri]::TryCreate($releaseUrl, [System.UriKind]::Absolute, [ref]$releaseUri)
        if (-not $releaseUriIsValid -or
            ($releaseUri.Scheme -ne [System.Uri]::UriSchemeHttp -and $releaseUri.Scheme -ne [System.Uri]::UriSchemeHttps)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.release_url must be an HTTP URL."
        } else {
            $decodedReleasePath = [System.Uri]::UnescapeDataString($releaseUri.AbsolutePath) -replace '\\', '/'
            $normalizedReleasePath = $decodedReleasePath.TrimEnd('/')
            $requestedReleaseTagPathSuffix = "/releases/tag/$requestedTag"
            if (-not [string]::IsNullOrWhiteSpace($requestedTag) -and
                -not $normalizedReleasePath.EndsWith($requestedReleaseTagPathSuffix, [System.StringComparison]::Ordinal)) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.release_url must identify the requested release tag."
            }
            $releaseHost = $releaseUri.Authority
        }
    }

    $remoteAssetsProperty = $upload.PSObject.Properties["remote_assets"]
    if ($null -eq $remoteAssetsProperty) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets is required when upload.uploaded is true."
        return
    }

    $remoteAssets = @(Get-JsonArray -Object $upload -Name "remote_assets")
    if ($remoteAssets.Count -ne 3) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets must contain exactly three official ZIP assets."
    }

    $expectedAssetNames = @()
    if (-not [string]::IsNullOrWhiteSpace($releaseVersion)) {
        $expectedAssetNames = @(
            "FeatherDoc-v$releaseVersion-msvc-install.zip",
            "FeatherDoc-v$releaseVersion-visual-validation-gallery.zip",
            "FeatherDoc-v$releaseVersion-release-evidence.zip"
        )
    }

    $expectedAssetNameSet = @{}
    foreach ($expectedAssetName in $expectedAssetNames) {
        $expectedAssetNameSet[$expectedAssetName] = $true
    }

    $remoteAssetsByName = @{}
    foreach ($remoteAsset in $remoteAssets) {
        $assetName = [string](Get-JsonPropertyValue -Object $remoteAsset -Name "name")
        if ([string]::IsNullOrWhiteSpace($assetName)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets entries must include name."
        } else {
            if ($remoteAssetsByName.ContainsKey($assetName)) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets contains duplicate asset '$assetName'."
            }
            $remoteAssetsByName[$assetName] = $remoteAsset

            if ($expectedAssetNameSet.Count -gt 0 -and -not $expectedAssetNameSet.ContainsKey($assetName)) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets contains unexpected asset '$assetName'."
            }
        }

        $assetUrl = [string](Get-JsonPropertyValue -Object $remoteAsset -Name "url")
        if ([string]::IsNullOrWhiteSpace($assetUrl) -or -not ($assetUrl -match '^https?://')) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.url must be an HTTP URL."
        } else {
            $assetUri = $null
            $assetUriIsValid = [System.Uri]::TryCreate($assetUrl, [System.UriKind]::Absolute, [ref]$assetUri)
            if (-not $assetUriIsValid) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.url must be a valid absolute URL."
                continue
            }

            $decodedAssetPath = [System.Uri]::UnescapeDataString($assetUri.AbsolutePath) -replace '\\', '/'
            $assetPathLeaf = ($decodedAssetPath -split '/')[-1]
            if (-not [string]::IsNullOrWhiteSpace($assetName) -and
                $assetPathLeaf -ne $assetName) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.url must identify the same asset file."
            }
            $requestedTagPathSegment = "/releases/download/$requestedTag/"
            if (-not [string]::IsNullOrWhiteSpace($requestedTag) -and
                $decodedAssetPath.IndexOf($requestedTagPathSegment, [System.StringComparison]::Ordinal) -lt 0) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.url must identify the requested release tag."
            }
            if (-not [string]::IsNullOrWhiteSpace($releaseHost) -and
                $assetUri.Authority -ne $releaseHost) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.url must use the same host as upload.release_url."
            }
        }

        $assetSize = Get-JsonPropertyValue -Object $remoteAsset -Name "size_bytes"
        if ($null -eq $assetSize) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.size_bytes is missing."
        } else {
            try {
                if ([int64]$assetSize -le 0) {
                    Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.size_bytes must be greater than zero."
                }
            } catch {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.size_bytes must be an integer."
            }
        }

        $downloadCount = Get-JsonPropertyValue -Object $remoteAsset -Name "download_count"
        if ($null -eq $downloadCount) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.download_count is missing."
        } else {
            try {
                if ([int64]$downloadCount -lt 0) {
                    Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.download_count must be zero or greater."
                }
            } catch {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets.$assetName.download_count must be an integer."
            }
        }
    }

    foreach ($expectedAssetName in $expectedAssetNames) {
        if (-not $remoteAssetsByName.ContainsKey($expectedAssetName)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "upload.remote_assets is missing official asset '$expectedAssetName'."
        }
    }
}
