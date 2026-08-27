function Add-CatalogPatchPlanMarkdownLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object]$Item,
        [string]$Indent = "  "
    )

    $plan = Get-JsonProperty -Object $Item -Name "catalog_patch_plan"
    $planId = Get-JsonString -Object $Item -Name "catalog_patch_plan_id"
    if ($null -eq $plan -and [string]::IsNullOrWhiteSpace($planId)) {
        return
    }
    if ([string]::IsNullOrWhiteSpace($planId) -and $null -ne $plan) {
        $planId = Get-JsonString -Object $plan -Name "id"
    }
    if (-not [string]::IsNullOrWhiteSpace($planId)) {
        $Lines.Add("${Indent}- catalog_patch_plan_id: ``$planId``") | Out-Null
    }
    if ($null -eq $plan) {
        return
    }

    $Lines.Add("${Indent}- catalog_patch_plan:") | Out-Null
    foreach ($fieldName in @(
            "schema",
            "document_key",
            "status",
            "safe_to_apply",
            "automatic_patch_available",
            "patch_apply_supported",
            "manual_review_required",
            "requires_authoritative_catalog_selection"
        )) {
        $fieldValue = Get-JsonProperty -Object $plan -Name $fieldName
        if ($null -eq $fieldValue) {
            continue
        }
        $fieldDisplay = [string]$fieldValue
        if (-not [string]::IsNullOrWhiteSpace($fieldDisplay)) {
            $Lines.Add("${Indent}  - ${fieldName}: ``$fieldDisplay``") | Out-Null
        }
    }

    $candidateCatalogs = @(Get-JsonArray -Object $plan -Name "candidate_catalog_displays")
    if ($candidateCatalogs.Count -eq 0) {
        $candidateCatalogs = @(Get-JsonArray -Object $plan -Name "candidate_catalog_paths")
    }
    $candidateCatalogParts = @(
        $candidateCatalogs |
            Where-Object { $null -ne $_ -and -not [string]::IsNullOrWhiteSpace([string]$_) } |
            ForEach-Object { "``$([string]$_)``" }
    )
    $candidateCatalogDisplay = if ($candidateCatalogParts.Count -eq 0) { "(none)" } else { $candidateCatalogParts -join ", " }
    $Lines.Add("${Indent}  - candidate_catalogs: $candidateCatalogDisplay") | Out-Null

    foreach ($arrayFieldName in @(
            "reviewer_inputs",
            "supported_patch_operations"
        )) {
        $values = @(
            Get-JsonArray -Object $plan -Name $arrayFieldName |
                Where-Object { $null -ne $_ -and -not [string]::IsNullOrWhiteSpace([string]$_) } |
                ForEach-Object { "``$([string]$_)``" }
        )
        if ($values.Count -gt 0) {
            $Lines.Add("${Indent}  - ${arrayFieldName}: $($values -join ', ')") | Out-Null
        }
    }

    $unsupportedChanges = @(
        Get-JsonArray -Object $plan -Name "unsupported_automatic_changes" |
            ForEach-Object {
                $changeKind = Get-JsonString -Object $_ -Name "change_kind"
                if ([string]::IsNullOrWhiteSpace($changeKind)) {
                    $changeKind = [string]$_
                }
                if (-not [string]::IsNullOrWhiteSpace($changeKind)) {
                    "``$changeKind``"
                }
            }
    )
    $unsupportedDisplay = if ($unsupportedChanges.Count -eq 0) { "(none)" } else { $unsupportedChanges -join ", " }
    $Lines.Add("${Indent}  - unsupported_automatic_changes: $unsupportedDisplay") | Out-Null

    $patchCountParts = @(
        foreach ($operationName in @("upsert_levels", "upsert_overrides", "remove_overrides")) {
            $operationCount = Get-JsonString -Object (Get-JsonProperty -Object $plan -Name "patch_counts") -Name $operationName
            if (-not [string]::IsNullOrWhiteSpace($operationCount)) {
                "${operationName}=``$operationCount``"
            }
        }
    )
    if ($patchCountParts.Count -gt 0) {
        $Lines.Add("${Indent}  - patch_counts: $($patchCountParts -join ', ')") | Out-Null
    }

    foreach ($commandName in @(
            "review_command",
            "patch_command_template",
            "lint_command_template",
            "verification_command_template"
        )) {
        $commandValue = Get-JsonString -Object $plan -Name $commandName
        if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
            $Lines.Add("${Indent}  - ${commandName}: ``$commandValue``") | Out-Null
        }
    }

    $requiredSteps = @(Get-JsonArray -Object $plan -Name "required_steps")
    if ($requiredSteps.Count -gt 0) {
        $Lines.Add("${Indent}  - required_steps:") | Out-Null
        foreach ($step in $requiredSteps) {
            $sequence = Get-JsonString -Object $step -Name "sequence"
            $action = Get-JsonString -Object $step -Name "action"
            $required = Get-JsonString -Object $step -Name "required"
            $description = Get-JsonString -Object $step -Name "description"
            $stepParts = New-Object 'System.Collections.Generic.List[string]'
            if (-not [string]::IsNullOrWhiteSpace($sequence)) {
                $stepParts.Add("sequence=``$sequence``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($action)) {
                $stepParts.Add("action=``$action``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($required)) {
                $stepParts.Add("required=``$required``") | Out-Null
            }
            $stepDisplay = if ($stepParts.Count -gt 0) { $stepParts -join " " } else { "step" }
            if (-not [string]::IsNullOrWhiteSpace($description)) {
                $stepDisplay += ": $description"
            }
            $Lines.Add("${Indent}    - $stepDisplay") | Out-Null
        }
    }
}
