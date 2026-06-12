function Invoke-EditPlanStyleOperationArguments {
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
        "set_text_style" {
            return New-DirectTextStyleOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "set_paragraph_text_style" {
            return New-DirectTextStyleOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "set_text_format" {
            return New-DirectTextStyleOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "delete_paragraph" {
            return New-DirectParagraphDeletionOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp
        }
        "delete_paragraph_contains" {
            return New-DirectParagraphDeletionOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp
        }
        "remove_paragraph" {
            return New-DirectParagraphDeletionOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp
        }
        "replace_text" {
            return New-DirectTextReplacementOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "replace_document_text" {
            return New-DirectTextReplacementOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "set_paragraph_horizontal_alignment" {
            return New-DirectParagraphHorizontalAlignmentOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "set_paragraph_alignment" {
            return New-DirectParagraphHorizontalAlignmentOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "clear_paragraph_horizontal_alignment" {
            return New-DirectParagraphHorizontalAlignmentOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "clear_paragraph_alignment" {
            return New-DirectParagraphHorizontalAlignmentOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "set_paragraph_spacing" {
            return New-DirectParagraphSpacingOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "set_paragraph_line_spacing" {
            return New-DirectParagraphSpacingOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "clear_paragraph_spacing" {
            return New-DirectParagraphSpacingOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "set_default_run_properties" {
            $arguments.Add("set-default-run-properties") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $null = Add-RunPropertyMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "clear_default_run_properties" {
            $arguments.Add("clear-default-run-properties") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $null = Add-RunPropertyClearArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "set_paragraph_style_properties" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("set-paragraph-style-properties") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            $null = Add-ParagraphStylePropertyMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "clear_paragraph_style_properties" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("clear-paragraph-style-properties") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            $null = Add-ParagraphStylePropertyClearArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "set_style_run_properties" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("set-style-run-properties") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            $null = Add-RunPropertyMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "clear_style_run_properties" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("clear-style-run-properties") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            $null = Add-RunPropertyClearArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "set_paragraph_style_numbering" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $definitionName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("definition_name", "name") `
                -Label $Label
            $arguments.Add("set-paragraph-style-numbering") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            $arguments.Add("--definition-name") | Out-Null
            $arguments.Add($definitionName) | Out-Null
            $null = Add-NumberingDefinitionLevelArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("style_level", "level") `
                -Flag "--style-level"
        }
        "clear_paragraph_style_numbering" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("clear-paragraph-style-numbering") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
        }
        "ensure_paragraph_style" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("ensure-paragraph-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            Add-EnsureParagraphStyleArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "ensure_character_style" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("ensure-character-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            Add-EnsureCharacterStyleArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "materialize_style_run_properties" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("materialize-style-run-properties") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
        }
        "rebase_character_style_based_on" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $basedOn = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("based_on", "based_on_style", "based_on_style_id", "target_based_on", "target_style", "target_style_id") `
                -Label $Label
            $arguments.Add("rebase-character-style-based-on") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            $arguments.Add($basedOn) | Out-Null
        }
        "rebase_paragraph_style_based_on" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $basedOn = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("based_on", "based_on_style", "based_on_style_id", "target_based_on", "target_style", "target_style_id") `
                -Label $Label
            $arguments.Add("rebase-paragraph-style-based-on") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            $arguments.Add($basedOn) | Out-Null
        }
        "rename_style" {
            $oldStyleId = Get-OldStyleIdValue -Operation $Operation -Label $Label
            $newStyleId = Get-NewStyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("rename-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($oldStyleId) | Out-Null
            $arguments.Add($newStyleId) | Out-Null
        }
        "merge_style" {
            $sourceStyleId = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("source_style_id", "source_style", "old_style_id", "old_style", "from_style_id", "from_style", "style_id", "style") `
                -Label $Label
            $targetStyleId = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("target_style_id", "target_style", "new_style_id", "new_style", "to_style_id", "to_style", "replacement_style_id", "replacement_style") `
                -Label $Label
            $arguments.Add("merge-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceStyleId) | Out-Null
            $arguments.Add($targetStyleId) | Out-Null
        }
        "apply_style_refactor" {
            $arguments.Add("apply-style-refactor") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-StyleRefactorApplyArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "restore_style_merge" {
            $arguments.Add("restore-style-merge") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-StyleMergeRestoreSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "restore_style_refactor" {
            $arguments.Add("restore-style-merge") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-StyleMergeRestoreSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "prune_unused_styles" {
            $arguments.Add("prune-unused-styles") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "apply_table_style_quality_fixes" {
            $arguments.Add("apply-table-style-quality-fixes") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--look-only") | Out-Null
        }
        "repair_table_style_look" {
            $arguments.Add("repair-table-style-look") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--apply") | Out-Null
        }
        "repair_style_numbering" {
            $arguments.Add("repair-style-numbering") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--apply") | Out-Null
            $catalogFile = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("catalog_file", "catalog_path", "numbering_catalog_file", "numbering_catalog_path")
            if (-not [string]::IsNullOrWhiteSpace($catalogFile)) {
                $arguments.Add("--catalog-file") | Out-Null
                $arguments.Add($catalogFile) | Out-Null
            }
        }
        "import_numbering_catalog" {
            $catalogFile = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("catalog_file", "catalog_path", "numbering_catalog_file", "numbering_catalog_path") `
                -Label $Label
            $arguments.Add("import-numbering-catalog") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--catalog-file") | Out-Null
            $arguments.Add($catalogFile) | Out-Null
        }
        "ensure_table_style" {
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("ensure-table-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleId) | Out-Null
            Add-EnsureTableStyleArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "ensure_numbering_definition" {
            $definitionName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("definition_name", "name") `
                -Label $Label
            $arguments.Add("ensure-numbering-definition") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--definition-name") | Out-Null
            $arguments.Add($definitionName) | Out-Null
            $null = Add-NumberingDefinitionLevelArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "ensure_style_linked_numbering" {
            $definitionName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("definition_name", "name") `
                -Label $Label
            $arguments.Add("ensure-style-linked-numbering") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--definition-name") | Out-Null
            $arguments.Add($definitionName) | Out-Null
            $null = Add-NumberingDefinitionLevelArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
            $null = Add-StyleLinkArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "set_paragraph_style" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("set-paragraph-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($styleId) | Out-Null
        }
        "clear_paragraph_style" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $arguments.Add("clear-paragraph-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
        }
        "set_run_style" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $runIndex = Get-RunIndexValue -Operation $Operation -Label $Label
            $styleId = Get-StyleIdValue -Operation $Operation -Label $Label
            $arguments.Add("set-run-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            $arguments.Add($styleId) | Out-Null
        }
        "clear_run_style" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $runIndex = Get-RunIndexValue -Operation $Operation -Label $Label
            $arguments.Add("clear-run-style") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
        }
        "set_run_font_family" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $runIndex = Get-RunIndexValue -Operation $Operation -Label $Label
            $fontFamily = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("font_family", "font") `
                -Label $Label
            $arguments.Add("set-run-font-family") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            $arguments.Add($fontFamily) | Out-Null
        }
        "clear_run_font_family" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $runIndex = Get-RunIndexValue -Operation $Operation -Label $Label
            $arguments.Add("clear-run-font-family") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
        }
        "set_run_language" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $runIndex = Get-RunIndexValue -Operation $Operation -Label $Label
            $language = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("language", "lang") `
                -Label $Label
            $arguments.Add("set-run-language") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            $arguments.Add($language) | Out-Null
        }
        "clear_run_language" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $runIndex = Get-RunIndexValue -Operation $Operation -Label $Label
            $arguments.Add("clear-run-language") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
        }
        "set_paragraph_numbering" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $definition = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("definition", "definition_id", "numbering_definition", "numbering_definition_id", "abstract_num_id") `
                -Label $Label
            $arguments.Add("set-paragraph-numbering") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add("--definition") | Out-Null
            $arguments.Add($definition) | Out-Null
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("level", "numbering_level", "list_level") `
                -Flag "--level"
        }
        "set_paragraph_list" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $listKind = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("kind", "list_kind", "list_type") `
                -Label $Label
            $arguments.Add("set-paragraph-list") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add("--kind") | Out-Null
            $arguments.Add($listKind) | Out-Null
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("level", "list_level") `
                -Flag "--level"
        }
        "restart_paragraph_list" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $listKind = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("kind", "list_kind", "list_type") `
                -Label $Label
            $arguments.Add("restart-paragraph-list") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add("--kind") | Out-Null
            $arguments.Add($listKind) | Out-Null
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("level", "list_level") `
                -Flag "--level"
        }
        "clear_paragraph_list" {
            $paragraphIndex = Get-ParagraphIndexValue -Operation $Operation -Label $Label
            $arguments.Add("clear-paragraph-list") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
        }
        default {
            return $false
        }
    }

    return $true
}
