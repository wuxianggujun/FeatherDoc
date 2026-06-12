function Invoke-EditPlanContentControlOperationArguments {
    param(
        $Operation,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Label,
        [int]$OperationIndex,
        [string]$TemporaryRoot,
        [string]$NormalizedOp,
        [System.Collections.Generic.List[string]]$Arguments
    )

    switch ($NormalizedOp) {
        "replace_content_control_text" {
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $arguments.Add("replace-content-control-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--text") | Out-Null
            $arguments.Add($text) | Out-Null
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_text_by_tag" {
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $arguments.Add("replace-content-control-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--text") | Out-Null
            $arguments.Add($text) | Out-Null
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_text_by_alias" {
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $arguments.Add("replace-content-control-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--text") | Out-Null
            $arguments.Add($text) | Out-Null
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_paragraphs" {
            $arguments.Add("replace-content-control-paragraphs") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-ContentControlParagraphFileArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_table" {
            $arguments.Add("replace-content-control-table") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-ContentControlTableRowFileArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_table_rows" {
            $arguments.Add("replace-content-control-table-rows") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-ContentControlTableRowFileArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -AllowEmptyRows
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_image" {
            $imagePath = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("image_path", "image", "path", "file") `
                -Label $Label
            $arguments.Add("replace-content-control-image") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($imagePath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
            $width = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("width", "width_px")
            if (-not [string]::IsNullOrWhiteSpace($width)) {
                $arguments.Add("--width") | Out-Null
                $arguments.Add($width) | Out-Null
            }
            $height = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("height", "height_px")
            if (-not [string]::IsNullOrWhiteSpace($height)) {
                $arguments.Add("--height") | Out-Null
                $arguments.Add($height) | Out-Null
            }
        }
        "set_content_control_form_state" {
            $arguments.Add("set-content-control-form-state") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            $optionCount = Add-ContentControlFormStateArguments -Arguments $arguments -Operation $Operation
            if ($optionCount -eq 0) {
                throw "$Label must provide at least one content-control form-state option."
            }
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "sync_content_controls_from_custom_xml" {
            $arguments.Add("sync-content-controls-from-custom-xml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "sync_content_control_from_custom_xml" {
            $arguments.Add("sync-content-controls-from-custom-xml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "accept_all_revisions" {
            $arguments.Add("accept-all-revisions") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "reject_all_revisions" {
            $arguments.Add("reject-all-revisions") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "set_comment_resolved" {
            Add-ReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        "set_comment_metadata" {
            Add-ReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        "replace_comment" {
            Add-ReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        "remove_comment" {
            Add-ReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        default {
            return $false
        }
    }

    return $true
}
