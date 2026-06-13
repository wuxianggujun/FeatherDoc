function Invoke-EditPlanBookmarkMediaOperationArguments {
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
        "fill_bookmarks" {
            $arguments.Add("fill-bookmarks") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-FillBookmarkBindings `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -OperationIndex $OperationIndex `
                -TemporaryRoot $TemporaryRoot
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "replace_bookmark_text" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $textPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1 `
                -Text $text
            $arguments.Add("fill-bookmarks") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--set-file") | Out-Null
            $arguments.Add($bookmark) | Out-Null
            $arguments.Add($textPath) | Out-Null
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "replace_bookmark_paragraphs" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $paragraphs = Get-StringArrayProperty -Object $Operation -Name "paragraphs" -Label $Label
            $arguments.Add("replace-bookmark-paragraphs") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            $valueIndex = 1
            foreach ($paragraph in $paragraphs) {
                $paragraphPath = New-EditOperationTextFile `
                    -TemporaryRoot $TemporaryRoot `
                    -OperationIndex $OperationIndex `
                    -ValueIndex $valueIndex `
                    -Text $paragraph
                $arguments.Add("--paragraph-file") | Out-Null
                $arguments.Add($paragraphPath) | Out-Null
                $valueIndex += 1
            }
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "replace_bookmark_table_rows" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $rows = Get-TableRowsProperty -Object $Operation -Label $Label
            $arguments.Add("replace-bookmark-table-rows") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            if ($rows.Count -eq 0) {
                $arguments.Add("--empty") | Out-Null
            } else {
                $valueIndex = 1
                foreach ($row in $rows) {
                    $cells = @($row)
                    if ($cells.Count -eq 0) {
                        throw "$Label table rows must contain at least one cell text."
                    }
                    $rowPath = New-EditOperationTextFile `
                        -TemporaryRoot $TemporaryRoot `
                        -OperationIndex $OperationIndex `
                        -ValueIndex $valueIndex `
                        -Text ([string]$cells[0])
                    $arguments.Add("--row-file") | Out-Null
                    $arguments.Add($rowPath) | Out-Null
                    $valueIndex += 1
                    for ($index = 1; $index -lt $cells.Count; ++$index) {
                        $cellPath = New-EditOperationTextFile `
                            -TemporaryRoot $TemporaryRoot `
                            -OperationIndex $OperationIndex `
                            -ValueIndex $valueIndex `
                            -Text ([string]$cells[$index])
                        $arguments.Add("--cell-file") | Out-Null
                        $arguments.Add($cellPath) | Out-Null
                        $valueIndex += 1
                    }
                }
            }
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "replace_bookmark_table" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $arguments.Add("replace-bookmark-table") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            Add-ContentControlTableRowFileArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "remove_bookmark_block" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $arguments.Add("remove-bookmark-block") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "delete_bookmark_block" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $arguments.Add("remove-bookmark-block") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "set_bookmark_block_visibility" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $visible = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "visible" `
                -DefaultValue $true
            $arguments.Add("set-bookmark-block-visibility") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            $arguments.Add("--visible") | Out-Null
            $arguments.Add($(if ($visible) { "true" } else { "false" })) | Out-Null
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "apply_bookmark_block_visibility" {
            $arguments.Add("apply-bookmark-block-visibility") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-BookmarkVisibilityBindings -Arguments $arguments -Operation $Operation -Label $Label
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "replace_bookmark_image" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $imagePath = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("image_path", "image", "path", "file") `
                -Label $Label
            $arguments.Add("replace-bookmark-image") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            $arguments.Add($imagePath) | Out-Null
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
            Add-BookmarkImageOptions -Arguments $arguments -Operation $Operation
        }
        "replace_bookmark_floating_image" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $imagePath = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("image_path", "image", "path", "file") `
                -Label $Label
            $arguments.Add("replace-bookmark-floating-image") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            $arguments.Add($imagePath) | Out-Null
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
            Add-BookmarkImageOptions -Arguments $arguments -Operation $Operation -Floating
        }
        "append_image" {
            $imagePath = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("image_path", "image", "path", "file") `
                -Label $Label
            $arguments.Add("append-image") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($imagePath) | Out-Null
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
            $floating = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "floating" `
                -DefaultValue $false
            if ($floating) {
                $arguments.Add("--floating") | Out-Null
            }
            Add-BookmarkImageOptions -Arguments $arguments -Operation $Operation -Floating:$floating
        }
        "append_floating_image" {
            $imagePath = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("image_path", "image", "path", "file") `
                -Label $Label
            $arguments.Add("append-image") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($imagePath) | Out-Null
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
            $arguments.Add("--floating") | Out-Null
            Add-BookmarkImageOptions -Arguments $arguments -Operation $Operation -Floating
        }
        "replace_image" {
            $imagePath = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("image_path", "replacement_image_path", "replacement_path", "path", "file") `
                -Label $Label
            $arguments.Add("replace-image") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($imagePath) | Out-Null
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
            Add-ImageSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
        }
        "remove_image" {
            $arguments.Add("remove-image") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
            Add-ImageSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
        }
        "append_hyperlink" {
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $target = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("target", "url", "href") `
                -Label $Label
            $arguments.Add("append-hyperlink") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--text") | Out-Null
            $arguments.Add($text) | Out-Null
            $arguments.Add("--target") | Out-Null
            $arguments.Add($target) | Out-Null
        }
        "replace_hyperlink" {
            $hyperlinkIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("hyperlink_index", "index") `
                -Label $Label
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $target = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("target", "url", "href") `
                -Label $Label
            $arguments.Add("replace-hyperlink") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($hyperlinkIndex) | Out-Null
            $arguments.Add("--text") | Out-Null
            $arguments.Add($text) | Out-Null
            $arguments.Add("--target") | Out-Null
            $arguments.Add($target) | Out-Null
        }
        "remove_hyperlink" {
            $hyperlinkIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("hyperlink_index", "index") `
                -Label $Label
            $arguments.Add("remove-hyperlink") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($hyperlinkIndex) | Out-Null
        }
        "delete_hyperlink" {
            $hyperlinkIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("hyperlink_index", "index") `
                -Label $Label
            $arguments.Add("remove-hyperlink") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($hyperlinkIndex) | Out-Null
        }
        "append_omml" {
            $xml = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("xml", "omml", "omml_xml") `
                -Label $Label
            $arguments.Add("append-omml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--xml") | Out-Null
            $arguments.Add($xml) | Out-Null
        }
        "replace_omml" {
            $formulaIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("formula_index", "omml_index", "index") `
                -Label $Label
            $xml = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("xml", "omml", "omml_xml") `
                -Label $Label
            $arguments.Add("replace-omml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($formulaIndex) | Out-Null
            $arguments.Add("--xml") | Out-Null
            $arguments.Add($xml) | Out-Null
        }
        "remove_omml" {
            $formulaIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("formula_index", "omml_index", "index") `
                -Label $Label
            $arguments.Add("remove-omml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($formulaIndex) | Out-Null
        }
        default {
            return $false
        }
    }

    return $true
}
