function Add-BookmarkSelectorArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation
    )

    $part = Get-OptionalObjectPropertyValue -Object $Operation -Name "part"
    if (-not [string]::IsNullOrWhiteSpace($part)) {
        $Arguments.Add("--part") | Out-Null
        $Arguments.Add($part) | Out-Null
    }

    $partIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "part_index"
    if ([string]::IsNullOrWhiteSpace($partIndex)) {
        $partIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "index"
    }
    if (-not [string]::IsNullOrWhiteSpace($partIndex)) {
        $Arguments.Add("--index") | Out-Null
        $Arguments.Add($partIndex) | Out-Null
    }

    $section = Get-OptionalObjectPropertyValue -Object $Operation -Name "section"
    if (-not [string]::IsNullOrWhiteSpace($section)) {
        $Arguments.Add("--section") | Out-Null
        $Arguments.Add($section) | Out-Null
    }

    $kind = Get-OptionalObjectPropertyValue -Object $Operation -Name "kind"
    if (-not [string]::IsNullOrWhiteSpace($kind)) {
        $Arguments.Add("--kind") | Out-Null
        $Arguments.Add($kind) | Out-Null
    }
}

function Get-ContentControlSelector {
    param(
        $Operation,
        [string]$Label
    )

    $tag = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("tag", "content_control_tag")
    $alias = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("alias", "content_control_alias")

    if ([string]::IsNullOrWhiteSpace($tag) -and [string]::IsNullOrWhiteSpace($alias)) {
        throw "$Label must provide 'tag'/'content_control_tag' or 'alias'/'content_control_alias'."
    }
    if (-not [string]::IsNullOrWhiteSpace($tag) -and -not [string]::IsNullOrWhiteSpace($alias)) {
        throw "$Label cannot combine content-control tag and alias selectors."
    }

    if (-not [string]::IsNullOrWhiteSpace($tag)) {
        return [pscustomobject]@{
            Flag = "--tag"
            Value = $tag
        }
    }

    return [pscustomobject]@{
        Flag = "--alias"
        Value = $alias
    }
}

function Add-ContentControlSelectorArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $selector = Get-ContentControlSelector -Operation $Operation -Label $Label
    $Arguments.Add($selector.Flag) | Out-Null
    $Arguments.Add($selector.Value) | Out-Null
}

function Add-ContentControlPartSelectionArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation
    )

    Add-TemplatePartSelectionArguments -Arguments $Arguments -Operation $Operation
}

function Add-ContentControlParagraphFileArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [string]$TemporaryRoot,
        [int]$OperationIndex
    )

    $paragraphs = Get-StringArrayProperty -Object $Operation -Name "paragraphs" -Label $Label
    $valueIndex = 1
    foreach ($paragraph in $paragraphs) {
        $paragraphPath = New-EditOperationTextFile `
            -TemporaryRoot $TemporaryRoot `
            -OperationIndex $OperationIndex `
            -ValueIndex $valueIndex `
            -Text $paragraph
        $Arguments.Add("--paragraph-file") | Out-Null
        $Arguments.Add($paragraphPath) | Out-Null
        $valueIndex += 1
    }
}

function Add-ContentControlTableRowFileArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [string]$TemporaryRoot,
        [int]$OperationIndex,
        [switch]$AllowEmptyRows
    )

    $rows = Get-TableRowsProperty -Object $Operation -Label $Label
    if ($rows.Count -eq 0) {
        if (-not $AllowEmptyRows) {
            throw "$Label rows must contain at least one row."
        }
        $Arguments.Add("--empty") | Out-Null
        return
    }

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
        $Arguments.Add("--row-file") | Out-Null
        $Arguments.Add($rowPath) | Out-Null
        $valueIndex += 1
        for ($index = 1; $index -lt $cells.Count; ++$index) {
            $cellPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex $valueIndex `
                -Text ([string]$cells[$index])
            $Arguments.Add("--cell-file") | Out-Null
            $Arguments.Add($cellPath) | Out-Null
            $valueIndex += 1
        }
    }
}

function Add-OptionalCliValueArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string[]]$Names,
        [string]$Flag
    )

    $value = Get-FirstOptionalObjectPropertyValue -Object $Operation -Names $Names
    if ([string]::IsNullOrWhiteSpace($value)) {
        return 0
    }

    $Arguments.Add($Flag) | Out-Null
    $Arguments.Add($value) | Out-Null
    return 1
}

function Add-OptionalCliBooleanValueArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Name,
        [string]$Flag
    )

    $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $Name
    if ($null -eq $value) {
        return 0
    }

    $text = if (Get-OptionalBooleanPropertyValue -Object $Operation -Name $Name -DefaultValue $false) {
        "true"
    } else {
        "false"
    }
    $Arguments.Add($Flag) | Out-Null
    $Arguments.Add($text) | Out-Null
    return 1
}

function Add-OptionalCliBooleanValueArgumentByNames {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string[]]$Names,
        [string]$Flag
    )

    foreach ($name in $Names) {
        $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -eq $value) {
            continue
        }

        $text = if (Get-OptionalBooleanPropertyValue -Object $Operation -Name $name -DefaultValue $false) {
            "true"
        } else {
            "false"
        }
        $Arguments.Add($Flag) | Out-Null
        $Arguments.Add($text) | Out-Null
        return 1
    }

    return 0
}

function Add-OptionalCliFlagArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string[]]$Names,
        [string]$Flag
    )

    foreach ($name in $Names) {
        $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -eq $value) {
            continue
        }
        if (Get-OptionalBooleanPropertyValue -Object $Operation -Name $name -DefaultValue $false) {
            $Arguments.Add($Flag) | Out-Null
            return 1
        }
        return 0
    }

    return 0
}

function Add-ContentControlFormStateArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation
    )

    $count = 0
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("lock", "content_control_lock") `
        -Flag "--lock"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_lock", "clear_content_control_lock") `
        -Flag "--clear-lock"
    $count += Add-OptionalCliBooleanValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Name "checked" `
        -Flag "--checked"
    $count += Add-OptionalCliBooleanValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Name "is_checked" `
        -Flag "--checked"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("selected_item", "selected", "selected_value") `
        -Flag "--selected-item"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("date_text", "date") `
        -Flag "--date-text"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("date_format", "format") `
        -Flag "--date-format"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("date_locale", "locale") `
        -Flag "--date-locale"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("data_binding_store_item_id", "store_item_id") `
        -Flag "--data-binding-store-item-id"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("data_binding_xpath", "xpath") `
        -Flag "--data-binding-xpath"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("data_binding_prefix_mappings", "prefix_mappings") `
        -Flag "--data-binding-prefix-mappings"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_data_binding") `
        -Flag "--clear-data-binding"

    return $count
}

function Add-BookmarkImageOptions {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [switch]$Floating
    )

    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("width", "width_px") `
        -Flag "--width"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("height", "height_px") `
        -Flag "--height"

    if (-not $Floating) {
        return
    }

    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("horizontal_reference", "horizontal_ref") `
        -Flag "--horizontal-reference"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("horizontal_offset", "horizontal_offset_px") `
        -Flag "--horizontal-offset"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("vertical_reference", "vertical_ref") `
        -Flag "--vertical-reference"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("vertical_offset", "vertical_offset_px") `
        -Flag "--vertical-offset"
    $null = Add-OptionalCliBooleanValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Name "behind_text" `
        -Flag "--behind-text"
    $null = Add-OptionalCliBooleanValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Name "allow_overlap" `
        -Flag "--allow-overlap"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("z_order", "z") `
        -Flag "--z-order"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("wrap_mode", "wrap") `
        -Flag "--wrap-mode"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("wrap_distance_left", "wrap_left") `
        -Flag "--wrap-distance-left"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("wrap_distance_right", "wrap_right") `
        -Flag "--wrap-distance-right"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("wrap_distance_top", "wrap_top") `
        -Flag "--wrap-distance-top"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("wrap_distance_bottom", "wrap_bottom") `
        -Flag "--wrap-distance-bottom"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("crop_left", "crop_left_per_mille") `
        -Flag "--crop-left"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("crop_top", "crop_top_per_mille") `
        -Flag "--crop-top"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("crop_right", "crop_right_per_mille") `
        -Flag "--crop-right"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("crop_bottom", "crop_bottom_per_mille") `
        -Flag "--crop-bottom"
}

function Add-ImageSelectorArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $selectorCount = 0
    $imageIndex = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("image_index", "image")
    if (-not [string]::IsNullOrWhiteSpace($imageIndex)) {
        $Arguments.Add("--image") | Out-Null
        $Arguments.Add($imageIndex) | Out-Null
        $selectorCount += 1
    }

    $relationshipId = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("relationship_id", "relationship", "rid")
    if (-not [string]::IsNullOrWhiteSpace($relationshipId)) {
        $Arguments.Add("--relationship-id") | Out-Null
        $Arguments.Add($relationshipId) | Out-Null
        $selectorCount += 1
    }

    $entryName = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("image_entry_name", "entry_name", "media_entry_name")
    if (-not [string]::IsNullOrWhiteSpace($entryName)) {
        $Arguments.Add("--image-entry-name") | Out-Null
        $Arguments.Add($entryName) | Out-Null
        $selectorCount += 1
    }

    if ($selectorCount -eq 0) {
        throw "$Label must provide 'image_index', 'relationship_id', or 'image_entry_name'."
    }
}

function Add-BookmarkVisibilityBindings {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $bindingCount = 0
    $show = Get-OptionalObjectPropertyObject -Object $Operation -Name "show"
    if ($null -ne $show) {
        foreach ($bookmark in Get-StringArrayProperty -Object $Operation -Name "show" -Label $Label) {
            $Arguments.Add("--show") | Out-Null
            $Arguments.Add($bookmark) | Out-Null
            $bindingCount += 1
        }
    }

    $hide = Get-OptionalObjectPropertyObject -Object $Operation -Name "hide"
    if ($null -ne $hide) {
        foreach ($bookmark in Get-StringArrayProperty -Object $Operation -Name "hide" -Label $Label) {
            $Arguments.Add("--hide") | Out-Null
            $Arguments.Add($bookmark) | Out-Null
            $bindingCount += 1
        }
    }

    $bindings = Get-OptionalObjectPropertyObject -Object $Operation -Name "bindings"
    if ($null -ne $bindings) {
        foreach ($binding in @($bindings)) {
            $bookmark = Get-BookmarkNameForOperation -Operation $binding -Label $Label
            $visible = Get-OptionalBooleanPropertyValue `
                -Object $binding `
                -Name "visible" `
                -DefaultValue $true
            if ($visible) {
                $Arguments.Add("--show") | Out-Null
            } else {
                $Arguments.Add("--hide") | Out-Null
            }
            $Arguments.Add($bookmark) | Out-Null
            $bindingCount += 1
        }
    }

    if ($bindingCount -eq 0) {
        throw "$Label must provide 'show', 'hide', or 'bindings'."
    }
}

function Add-FillBookmarkBindings {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [int]$OperationIndex,
        [string]$TemporaryRoot
    )

    $bindingCount = 0
    $bindings = Get-OptionalObjectPropertyObject -Object $Operation -Name "bindings"
    if ($null -ne $bindings) {
        $valueIndex = 1
        foreach ($binding in @($bindings)) {
            $bookmark = Get-BookmarkNameForOperation -Operation $binding -Label $Label
            $text = Get-RequiredObjectPropertyValue -Object $binding -Name "text" -Label $Label
            $textPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex $valueIndex `
                -Text $text
            $Arguments.Add("--set-file") | Out-Null
            $Arguments.Add($bookmark) | Out-Null
            $Arguments.Add($textPath) | Out-Null
            $valueIndex += 1
            $bindingCount += 1
        }
    }

    $values = Get-OptionalObjectPropertyObject -Object $Operation -Name "values"
    if ($null -ne $values) {
        $valueIndex = $bindingCount + 1
        foreach ($property in @(Get-ObjectOwnProperties -Object $values -Label "$Label 'values'")) {
            $bookmark = [string]$property.Name
            if ([string]::IsNullOrWhiteSpace($bookmark)) {
                throw "$Label 'values' bookmark names must not be empty."
            }
            $text = [string]$property.Value
            $textPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex $valueIndex `
                -Text $text
            $Arguments.Add("--set-file") | Out-Null
            $Arguments.Add($bookmark) | Out-Null
            $Arguments.Add($textPath) | Out-Null
            $valueIndex += 1
            $bindingCount += 1
        }
    }

    if ($bindingCount -eq 0) {
        throw "$Label must provide 'bindings' or 'values'."
    }
}

function Get-TemplateTableHeaderCellValues {
    param($Operation)

    foreach ($name in @("header_cells", "header_cell", "header_texts")) {
        $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -eq $value) {
            continue
        }

        if ($value -is [string]) {
            if ([string]::IsNullOrWhiteSpace($value)) {
                throw "Template table selector '$name' must not be empty."
            }

            return @([string]$value)
        }

        if ($value -is [System.Collections.IEnumerable]) {
            $cells = @($value | ForEach-Object { [string]$_ } | Where-Object {
                    -not [string]::IsNullOrWhiteSpace($_)
                })
            if ($cells.Count -eq 0) {
                throw "Template table selector '$name' must contain at least one value."
            }

            return $cells
        }

        $text = [string]$value
        if ([string]::IsNullOrWhiteSpace($text)) {
            throw "Template table selector '$name' must not be empty."
        }

        return @($text)
    }

    return @()
}
