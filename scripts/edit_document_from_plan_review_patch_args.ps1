function Add-TemplateAppendRowOptions {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $cellCount = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("cell_count", "column_count", "columns")
    if (-not [string]::IsNullOrWhiteSpace($cellCount)) {
        $cellCountValue = [int]$cellCount
        if ($cellCountValue -le 0) {
            throw "$Label cell_count must be greater than zero."
        }
        $Arguments.Add("--cell-count") | Out-Null
        $Arguments.Add([string]$cellCountValue) | Out-Null
    }
}

function New-EditOperationTextFile {
    param(
        [string]$TemporaryRoot,
        [int]$OperationIndex,
        [int]$ValueIndex,
        [string]$Text
    )

    $textDirectory = Join-Path $TemporaryRoot "text-values"
    New-Item -ItemType Directory -Path $textDirectory -Force | Out-Null
    $textPath = Join-Path $textDirectory ("operation-{0}-value-{1}.txt" -f $OperationIndex, $ValueIndex)
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($textPath, $Text, $utf8NoBom)

    return $textPath
}

function New-EditOperationJsonFile {
    param(
        [string]$TemporaryRoot,
        [int]$OperationIndex,
        [int]$ValueIndex,
        $Value
    )

    $jsonDirectory = Join-Path $TemporaryRoot "json-values"
    New-Item -ItemType Directory -Path $jsonDirectory -Force | Out-Null
    $jsonPath = Join-Path $jsonDirectory ("operation-{0}-value-{1}.json" -f $OperationIndex, $ValueIndex)
    $jsonText = $Value | ConvertTo-Json -Depth 64 -Compress
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($jsonPath, $jsonText, $utf8NoBom)

    return $jsonPath
}

function Add-ReviewMutationPlanProperty {
    param(
        [System.Collections.Specialized.OrderedDictionary]$Target,
        $Operation,
        [string[]]$Names,
        [string]$OutputName
    )

    foreach ($name in $Names) {
        if (-not (Test-ObjectPropertyExists -Object $Operation -Name $name)) {
            continue
        }

        $Target[$OutputName] = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        return
    }
}

function New-ReviewMutationPlanOperationObject {
    param(
        $Operation,
        [string]$NormalizedOp
    )

    $reviewOperation = [ordered]@{
        kind = $NormalizedOp
    }

    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("comment_index", "index") `
        -OutputName "comment_index"
    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("expected_text", "expected_anchor_text") `
        -OutputName "expected_text"
    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("expected_comment_text", "expected_comment_body") `
        -OutputName "expected_comment_text"
    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("expected_resolved") `
        -OutputName "expected_resolved"
    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("expected_parent_index") `
        -OutputName "expected_parent_index"

    switch ($NormalizedOp) {
        "set_comment_resolved" {
            Add-ReviewMutationPlanProperty `
                -Target $reviewOperation `
                -Operation $Operation `
                -Names @("resolved") `
                -OutputName "resolved"
        }
        "set_comment_metadata" {
            foreach ($property in @("author", "initials", "date", "clear_author", "clear_initials", "clear_date")) {
                Add-ReviewMutationPlanProperty `
                    -Target $reviewOperation `
                    -Operation $Operation `
                    -Names @($property) `
                    -OutputName $property
            }
        }
        "replace_comment" {
            Add-ReviewMutationPlanProperty `
                -Target $reviewOperation `
                -Operation $Operation `
                -Names @("comment_text", "text", "replacement_text", "comment_body") `
                -OutputName "comment_text"
        }
        "remove_comment" {
        }
        default {
            throw "Unsupported review mutation operation '$NormalizedOp'."
        }
    }

    return [pscustomobject]$reviewOperation
}

function Add-ReviewMutationPlanArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$NormalizedOp,
        [string]$InputPath,
        [string]$TemporaryRoot,
        [int]$OperationIndex
    )

    $reviewOperation = New-ReviewMutationPlanOperationObject `
        -Operation $Operation `
        -NormalizedOp $NormalizedOp
    $reviewPlan = [pscustomobject]@{
        operations = @($reviewOperation)
    }
    $reviewPlanPath = New-EditOperationJsonFile `
        -TemporaryRoot $TemporaryRoot `
        -OperationIndex $OperationIndex `
        -ValueIndex 1 `
        -Value $reviewPlan

    $Arguments.Add("apply-review-mutation-plan") | Out-Null
    $Arguments.Add($InputPath) | Out-Null
    $Arguments.Add("--plan-file") | Out-Null
    $Arguments.Add($reviewPlanPath) | Out-Null
}

function Get-InlineReviewMutationPlanObject {
    param(
        $Operation,
        [string]$Label
    )

    $reviewPlan = $null
    foreach ($name in @("review_plan", "review_mutation_plan", "plan")) {
        $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $value) {
            $reviewPlan = $value
            break
        }
    }

    $reviewOperations = $null
    foreach ($name in @("review_operations", "operations")) {
        $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $value) {
            $reviewOperations = $value
            break
        }
    }

    if (($null -ne $reviewPlan) -and ($null -ne $reviewOperations)) {
        throw "$Label cannot combine an inline review mutation plan with inline review operations."
    }

    if ($null -ne $reviewPlan) {
        $operations = Get-OptionalObjectPropertyObject -Object $reviewPlan -Name "operations"
        if ($null -eq $operations) {
            throw "$Label inline review mutation plan must provide an 'operations' array."
        }
        if ($operations -is [string]) {
            throw "$Label inline review mutation plan 'operations' must be an array."
        }

        return $reviewPlan
    }

    if ($null -ne $reviewOperations) {
        if ($reviewOperations -is [string]) {
            throw "$Label inline review mutation operations must be an array."
        }

        return [pscustomobject]@{
            operations = @($reviewOperations)
        }
    }

    return $null
}

function Add-ApplyReviewMutationPlanArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [string]$InputPath,
        [string]$TemporaryRoot,
        [int]$OperationIndex
    )

    $planFile = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @(
            "plan_file",
            "plan_path",
            "review_plan_file",
            "review_plan_path",
            "review_mutation_plan_file",
            "review_mutation_plan_path"
        )
    $inlinePlan = Get-InlineReviewMutationPlanObject -Operation $Operation -Label $Label

    if ((-not [string]::IsNullOrWhiteSpace($planFile)) -and ($null -ne $inlinePlan)) {
        throw "$Label cannot combine 'plan_file' with an inline review mutation plan."
    }

    if ([string]::IsNullOrWhiteSpace($planFile) -and ($null -eq $inlinePlan)) {
        throw "$Label must provide 'plan_file', 'review_plan', or 'operations'."
    }

    $reviewPlanPath = $planFile
    if ($null -ne $inlinePlan) {
        $reviewPlanPath = New-EditOperationJsonFile `
            -TemporaryRoot $TemporaryRoot `
            -OperationIndex $OperationIndex `
            -ValueIndex 1 `
            -Value $inlinePlan
    }

    $Arguments.Add("apply-review-mutation-plan") | Out-Null
    $Arguments.Add($InputPath) | Out-Null
    $Arguments.Add("--plan-file") | Out-Null
    $Arguments.Add($reviewPlanPath) | Out-Null
}

function Get-InlineTablePositionPlanObject {
    param(
        $Operation,
        [string]$Label
    )

    foreach ($name in @("table_position_plan", "plan")) {
        $plan = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -eq $plan) {
            continue
        }
        if ($plan -is [string]) {
            throw "$Label inline table position plan '$name' must be a JSON object."
        }

        return $plan
    }

    return $null
}

function Add-ApplyTablePositionPlanArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [string]$InputPath,
        [string]$TemporaryRoot,
        [int]$OperationIndex
    )

    $dryRun = Get-OptionalBooleanPropertyValue `
        -Object $Operation `
        -Name "dry_run" `
        -DefaultValue $false
    if ($dryRun) {
        throw "$Label cannot use dry_run because edit-plan operations must produce an output DOCX."
    }

    $planFile = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @(
            "plan_file",
            "plan_path",
            "table_position_plan_file",
            "table_position_plan_path"
        )
    $inlinePlan = Get-InlineTablePositionPlanObject -Operation $Operation -Label $Label

    if ((-not [string]::IsNullOrWhiteSpace($planFile)) -and ($null -ne $inlinePlan)) {
        throw "$Label cannot combine 'plan_file' with an inline table position plan."
    }

    if ([string]::IsNullOrWhiteSpace($planFile) -and ($null -eq $inlinePlan)) {
        throw "$Label must provide 'plan_file' or 'table_position_plan'."
    }

    $planObject = $inlinePlan
    if (-not [string]::IsNullOrWhiteSpace($planFile)) {
        $planObject = ConvertFrom-EditPlanJson -Json (
            Get-Content -Raw -Encoding UTF8 -LiteralPath $planFile
        )
    }
    if ($null -eq $planObject -or $planObject -is [string]) {
        throw "$Label table position plan must be a JSON object."
    }
    Set-ObjectPropertyValue -Object $planObject -Name "input_path" -Value $InputPath

    $tablePositionPlanPath = New-EditOperationJsonFile `
        -TemporaryRoot $TemporaryRoot `
        -OperationIndex $OperationIndex `
        -ValueIndex 1 `
        -Value $planObject

    $Arguments.Add("apply-table-position-plan") | Out-Null
    $Arguments.Add($tablePositionPlanPath) | Out-Null
}

function Get-TemplateTableJsonPatchObject {
    param(
        $Operation,
        [string]$Label
    )

    foreach ($name in @("patch", "json_patch", "table_patch")) {
        $patch = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $patch) {
            return $patch
        }
    }

    $patchObject = [ordered]@{}
    $mode = Get-OptionalObjectPropertyValue -Object $Operation -Name "mode"
    if (-not [string]::IsNullOrWhiteSpace($mode)) {
        $patchObject["mode"] = $mode
    }

    foreach ($name in @("start_row", "start_row_index", "start_cell", "start_cell_index")) {
        $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $value) {
            $patchObject[$name] = $value
        }
    }

    $rows = Get-OptionalObjectPropertyObject -Object $Operation -Name "rows"
    if ($null -ne $rows) {
        $patchObject["rows"] = $rows
    }

    if ($patchObject.Count -eq 0) {
        throw "$Label must provide 'patch' or JSON patch members such as 'start_row' and 'rows'."
    }

    return [pscustomobject]$patchObject
}

function Get-TemplateTableJsonBatchPatchObject {
    param(
        $Operation,
        [string]$Label
    )

    foreach ($name in @("patch", "json_patch", "batch_patch")) {
        $patch = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $patch) {
            return $patch
        }
    }

    foreach ($name in @("table_operations", "patches", "table_patches", "operations")) {
        $operations = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $operations) {
            return [pscustomobject]@{
                operations = $operations
            }
        }
    }

    throw "$Label must provide 'patch' or a table patch operation array."
}

function Add-TemplateTableJsonPatchFileArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [string]$TemporaryRoot,
        [int]$OperationIndex,
        [int]$ValueIndex,
        [switch]$Batch
    )

    $patchFile = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("patch_file", "patch_path", "json_patch_file")
    if ([string]::IsNullOrWhiteSpace($patchFile)) {
        if ($Batch) {
            $patch = Get-TemplateTableJsonBatchPatchObject -Operation $Operation -Label $Label
        } else {
            $patch = Get-TemplateTableJsonPatchObject -Operation $Operation -Label $Label
        }
        $patchFile = New-EditOperationJsonFile `
            -TemporaryRoot $TemporaryRoot `
            -OperationIndex $OperationIndex `
            -ValueIndex $ValueIndex `
            -Value $patch
    }

    $Arguments.Add("--patch-file") | Out-Null
    $Arguments.Add($patchFile) | Out-Null
}

function Get-BookmarkNameForOperation {
    param(
        $Operation,
        [string]$Label
    )

    $bookmark = Get-OptionalObjectPropertyValue -Object $Operation -Name "bookmark"
    if ([string]::IsNullOrWhiteSpace($bookmark)) {
        $bookmark = Get-OptionalObjectPropertyValue -Object $Operation -Name "bookmark_name"
    }
    if ([string]::IsNullOrWhiteSpace($bookmark)) {
        throw "$Label must provide 'bookmark' or 'bookmark_name'."
    }

    return $bookmark
}

function Get-OperationAlignmentProperty {
    param(
        $Operation,
        [string]$Label,
        [string]$FallbackName = ""
    )

    $alignment = Get-OptionalObjectPropertyValue -Object $Operation -Name "alignment"
    if ([string]::IsNullOrWhiteSpace($alignment) -and
        -not [string]::IsNullOrWhiteSpace($FallbackName)) {
        $alignment = Get-OptionalObjectPropertyValue -Object $Operation -Name $FallbackName
    }
    if ([string]::IsNullOrWhiteSpace($alignment)) {
        throw "$Label must provide 'alignment'."
    }

    return $alignment.Trim()
}

function Normalize-TableCellHorizontalAlignment {
    param(
        [string]$Alignment,
        [string]$Label
    )

    switch ($Alignment.Trim().ToLowerInvariant()) {
        "left" { return "left" }
        "start" { return "left" }
        "center" { return "center" }
        "centre" { return "center" }
        "right" { return "right" }
        "end" { return "right" }
        "both" { return "both" }
        "justify" { return "both" }
        default {
            throw "$Label has unsupported horizontal alignment '$Alignment'. Use left, center, right, or both."
        }
    }
}
