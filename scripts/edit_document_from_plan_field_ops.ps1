function Invoke-EditPlanFieldOperationArguments {
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
        "append_page_number_field" {
            $arguments.Add("append-page-number-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowFieldState
        }
        "append_total_pages_field" {
            $arguments.Add("append-total-pages-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowFieldState
        }
        "append_table_of_contents_field" {
            $arguments.Add("append-table-of-contents-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowTableOfContentsOptions `
                -AllowFieldState
        }
        "append_document_property_field" {
            $propertyName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("property_name", "property", "name") `
                -Label $Label
            $arguments.Add("append-document-property-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($propertyName) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
        }
        "append_field" {
            $instruction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("instruction", "field_instruction") `
                -Label $Label
            $arguments.Add("append-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($instruction) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowFieldState
        }
        "append_date_field" {
            $arguments.Add("append-date-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowDateFormat `
                -AllowPreserveFormatting `
                -AllowFieldState
        }
        "append_reference_field" {
            $bookmarkName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("bookmark_name", "bookmark", "name") `
                -Label $Label
            $arguments.Add("append-reference-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmarkName) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState

            $hasNoHyperlink = Test-ObjectPropertyExists -Object $Operation -Name "no_hyperlink"
            $hasHyperlink = Test-ObjectPropertyExists -Object $Operation -Name "hyperlink"
            $noHyperlink = $false
            if ($hasNoHyperlink) {
                $noHyperlink = Get-OptionalBooleanPropertyValue `
                    -Object $Operation `
                    -Name "no_hyperlink" `
                    -DefaultValue $false
            }
            if ($hasHyperlink) {
                $hyperlink = Get-OptionalBooleanPropertyValue `
                    -Object $Operation `
                    -Name "hyperlink" `
                    -DefaultValue $true
                if ($hasNoHyperlink -and ($noHyperlink -eq $hyperlink)) {
                    throw "$Label cannot combine contradictory 'no_hyperlink' and 'hyperlink' values."
                }
                if (-not $hyperlink) {
                    $noHyperlink = $true
                }
            }
            if ($noHyperlink) {
                $arguments.Add("--no-hyperlink") | Out-Null
            }
        }
        "append_page_reference_field" {
            $bookmarkName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("bookmark_name", "bookmark", "name") `
                -Label $Label
            $arguments.Add("append-page-reference-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmarkName) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState

            $hasNoHyperlink = Test-ObjectPropertyExists -Object $Operation -Name "no_hyperlink"
            $hasHyperlink = Test-ObjectPropertyExists -Object $Operation -Name "hyperlink"
            $noHyperlink = $false
            if ($hasNoHyperlink) {
                $noHyperlink = Get-OptionalBooleanPropertyValue `
                    -Object $Operation `
                    -Name "no_hyperlink" `
                    -DefaultValue $false
            }
            if ($hasHyperlink) {
                $hyperlink = Get-OptionalBooleanPropertyValue `
                    -Object $Operation `
                    -Name "hyperlink" `
                    -DefaultValue $true
                if ($hasNoHyperlink -and ($noHyperlink -eq $hyperlink)) {
                    throw "$Label cannot combine contradictory 'no_hyperlink' and 'hyperlink' values."
                }
                if (-not $hyperlink) {
                    $noHyperlink = $true
                }
            }
            if ($noHyperlink) {
                $arguments.Add("--no-hyperlink") | Out-Null
            }
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("relative_position") `
                -Flag "--relative-position"
        }
        "append_style_reference_field" {
            $styleName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("style_name", "style", "name") `
                -Label $Label
            $arguments.Add("append-style-reference-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleName) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("paragraph_number") `
                -Flag "--paragraph-number"
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("relative_position") `
                -Flag "--relative-position"
        }
        "append_hyperlink_field" {
            $target = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("target", "url", "href") `
                -Label $Label
            $arguments.Add("append-hyperlink-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($target) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("anchor", "bookmark", "bookmark_name") `
                -Flag "--anchor"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("tooltip", "screen_tip") `
                -Flag "--tooltip"
        }
        "append_caption" {
            $labelName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("label", "caption_label", "name") `
                -Label $Label
            $captionText = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("caption_text", "text") `
                -Label $Label
            $arguments.Add("append-caption") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($labelName) | Out-Null
            Add-AppendFieldPartSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation
            $arguments.Add("--text") | Out-Null
            $arguments.Add($captionText) | Out-Null
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("number_result_text", "number_result") `
                -Flag "--number-result"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("number_format", "format") `
                -Flag "--number-format"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("restart", "restart_value") `
                -Flag "--restart"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("separator") `
                -Flag "--separator"
            Add-AppendFieldStateArguments `
                -Arguments $arguments `
                -Operation $Operation
            Add-AppendFieldPreserveFormattingArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "append_index_entry_field" {
            $entryText = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("entry_text", "entry", "text") `
                -Label $Label
            $arguments.Add("append-index-entry-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($entryText) | Out-Null
            Add-AppendFieldPartSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("subentry") `
                -Flag "--subentry"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("bookmark_name", "bookmark") `
                -Flag "--bookmark"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("cross_reference", "crossref") `
                -Flag "--cross-reference"
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("bold_page_number") `
                -Flag "--bold-page-number"
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("italic_page_number") `
                -Flag "--italic-page-number"
            Add-AppendFieldStateArguments `
                -Arguments $arguments `
                -Operation $Operation
        }
        "append_index_field" {
            $arguments.Add("append-index-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("columns", "column_count") `
                -Flag "--columns"
        }
        "append_sequence_field" {
            $identifier = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("identifier", "sequence_identifier", "name") `
                -Label $Label
            $arguments.Add("append-sequence-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($identifier) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("number_format", "format") `
                -Flag "--number-format"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("restart", "restart_value") `
                -Flag "--restart"
        }
        "append_complex_field" {
            $instruction = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("instruction")
            $instructionBefore = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("instruction_before")
            $nestedInstruction = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("nested_instruction")
            $nestedResultText = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("nested_result_text")
            $instructionAfter = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("instruction_after")
            $hasInstruction = -not [string]::IsNullOrWhiteSpace($instruction)
            $hasNestedInstruction = -not [string]::IsNullOrWhiteSpace($nestedInstruction)
            $hasNestedInstructionParts = `
                (-not [string]::IsNullOrWhiteSpace($instructionBefore)) -or `
                $hasNestedInstruction -or `
                (-not [string]::IsNullOrWhiteSpace($nestedResultText)) -or `
                (-not [string]::IsNullOrWhiteSpace($instructionAfter))

            if ($hasInstruction -and $hasNestedInstructionParts) {
                throw "$Label cannot combine 'instruction' with nested instruction properties."
            }
            if ((-not $hasInstruction) -and (-not $hasNestedInstruction)) {
                throw "$Label must provide 'instruction' or 'nested_instruction'."
            }
            if ($hasNestedInstruction) {
                if ([string]::IsNullOrWhiteSpace($instructionBefore) -or
                    [string]::IsNullOrWhiteSpace($instructionAfter)) {
                    throw "$Label nested instruction mode requires 'instruction_before' and 'instruction_after'."
                }
            }

            $arguments.Add("append-complex-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldPartSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation
            if ($hasInstruction) {
                $arguments.Add("--instruction") | Out-Null
                $arguments.Add($instruction) | Out-Null
            } else {
                $arguments.Add("--instruction-before") | Out-Null
                $arguments.Add($instructionBefore) | Out-Null
                $arguments.Add("--nested-instruction") | Out-Null
                $arguments.Add($nestedInstruction) | Out-Null
                if (-not [string]::IsNullOrWhiteSpace($nestedResultText)) {
                    $arguments.Add("--nested-result-text") | Out-Null
                    $arguments.Add($nestedResultText) | Out-Null
                }
                $arguments.Add("--instruction-after") | Out-Null
                $arguments.Add($instructionAfter) | Out-Null
            }
            Add-AppendFieldResultTextArgument `
                -Arguments $arguments `
                -Operation $Operation
            Add-AppendFieldStateArguments `
                -Arguments $arguments `
                -Operation $Operation
        }
        "replace_field" {
            $fieldIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("field_index", "index") `
                -Label $Label
            $instruction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("instruction", "field_instruction") `
                -Label $Label
            $arguments.Add("replace-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($fieldIndex) | Out-Null
            $arguments.Add($instruction) | Out-Null
            Add-FieldPartSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation
            Add-AppendFieldResultTextArgument `
                -Arguments $arguments `
                -Operation $Operation
        }
        "set_update_fields_on_open" {
            $enabled = Get-UpdateFieldsOnOpenEnabledValue `
                -Operation $Operation `
                -Label $Label
            $arguments.Add("set-update-fields-on-open") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($(if ($enabled) { "--enable" } else { "--disable" })) | Out-Null
        }
        "enable_update_fields_on_open" {
            $enabled = Get-UpdateFieldsOnOpenEnabledValue `
                -Operation $Operation `
                -Label $Label `
                -DefaultValue $true
            $arguments.Add("set-update-fields-on-open") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($(if ($enabled) { "--enable" } else { "--disable" })) | Out-Null
        }
        "disable_update_fields_on_open" {
            $enabled = Get-UpdateFieldsOnOpenEnabledValue `
                -Operation $Operation `
                -Label $Label `
                -DefaultValue $false
            $arguments.Add("set-update-fields-on-open") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($(if ($enabled) { "--enable" } else { "--disable" })) | Out-Null
        }
        "clear_update_fields_on_open" {
            $enabled = Get-UpdateFieldsOnOpenEnabledValue `
                -Operation $Operation `
                -Label $Label `
                -DefaultValue $false
            $arguments.Add("set-update-fields-on-open") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($(if ($enabled) { "--enable" } else { "--disable" })) | Out-Null
        }
        default {
            return $false
        }
    }

    return $true
}
