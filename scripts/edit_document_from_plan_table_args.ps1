function Add-RepeatableCliValueArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string[]]$Names,
        [string]$Flag,
        [string]$Label
    )

    foreach ($name in $Names) {
        $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -eq $value) {
            continue
        }

        foreach ($item in @($value)) {
            $itemText = [string]$item
            if ([string]::IsNullOrWhiteSpace($itemText)) {
                throw "$Label '$name' values must not be empty."
            }
            $Arguments.Add($Flag) | Out-Null
            $Arguments.Add($itemText) | Out-Null
        }
        return
    }
}

function Add-EnsureTableStyleArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    Add-EnsureStyleCatalogArguments -Arguments $Arguments -Operation $Operation -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_fill", "fills") -Flag "--style-fill" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_text_color", "text_colors") -Flag "--style-text-color" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_bold", "bold") -Flag "--style-bold" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_italic", "italic") -Flag "--style-italic" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_font_size", "font_size") -Flag "--style-font-size" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_font_family", "font_family") -Flag "--style-font-family" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_east_asia_font_family", "east_asia_font_family") -Flag "--style-east-asia-font-family" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_cell_vertical_alignment", "cell_vertical_alignment") -Flag "--style-cell-vertical-alignment" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_cell_text_direction", "cell_text_direction") -Flag "--style-cell-text-direction" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_paragraph_alignment", "paragraph_alignment") -Flag "--style-paragraph-alignment" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_paragraph_spacing_before", "paragraph_spacing_before") -Flag "--style-paragraph-spacing-before" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_paragraph_spacing_after", "paragraph_spacing_after") -Flag "--style-paragraph-spacing-after" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_paragraph_line_spacing", "paragraph_line_spacing") -Flag "--style-paragraph-line-spacing" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_margin", "margins") -Flag "--style-margin" -Label $Label
    Add-RepeatableCliValueArgument -Arguments $Arguments -Operation $Operation -Names @("style_border", "borders") -Flag "--style-border" -Label $Label
}

function Add-StyleLinkArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $links = $null
    foreach ($name in @("style_links", "links")) {
        $links = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $links) {
            break
        }
    }

    if ($null -eq $links) {
        throw "$Label must provide 'style_links' or 'links'."
    }

    if ($links -is [string]) {
        if ([string]::IsNullOrWhiteSpace([string]$links)) {
            throw "$Label style-link strings must not be empty."
        }
        $Arguments.Add("--style-link") | Out-Null
        $Arguments.Add([string]$links) | Out-Null
        return 1
    }

    $count = 0
    foreach ($link in @($links)) {
        if ($link -is [string]) {
            if ([string]::IsNullOrWhiteSpace([string]$link)) {
                throw "$Label style-link strings must not be empty."
            }
            $Arguments.Add("--style-link") | Out-Null
            $Arguments.Add([string]$link) | Out-Null
            $count += 1
            continue
        }

        $styleId = Get-FirstObjectPropertyValue `
            -Object $link `
            -Names @("style_id", "style") `
            -Label $Label
        $level = Get-FirstObjectPropertyValue `
            -Object $link `
            -Names @("level", "style_level") `
            -Label $Label
        $Arguments.Add("--style-link") | Out-Null
        $Arguments.Add("$styleId`:$level") | Out-Null
        $count += 1
    }

    if ($count -eq 0) {
        throw "$Label must provide at least one style link."
    }

    return $count
}

function Add-RunPropertyMutationArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $count = 0
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("font_family", "font") `
        -Flag "--font-family"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("east_asia_font_family", "east_asia_font") `
        -Flag "--east-asia-font-family"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("language", "lang") `
        -Flag "--language"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("east_asia_language", "east_asia_lang") `
        -Flag "--east-asia-language"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("bidi_language", "bidi_lang") `
        -Flag "--bidi-language"
    $count += Add-OptionalCliBooleanValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Name "rtl" `
        -Flag "--rtl"
    $count += Add-OptionalCliBooleanValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Name "paragraph_bidi" `
        -Flag "--paragraph-bidi"

    if ($count -eq 0) {
        throw "$Label must provide at least one run-property mutation field."
    }

    return $count
}

function Add-RunPropertyClearArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $count = 0
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_font_family", "font_family") `
        -Flag "--font-family"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_east_asia_font_family", "east_asia_font_family") `
        -Flag "--east-asia-font-family"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_primary_language", "primary_language") `
        -Flag "--primary-language"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_language", "language") `
        -Flag "--language"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_east_asia_language", "east_asia_language") `
        -Flag "--east-asia-language"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_bidi_language", "bidi_language") `
        -Flag "--bidi-language"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_rtl", "rtl") `
        -Flag "--rtl"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_paragraph_bidi", "paragraph_bidi") `
        -Flag "--paragraph-bidi"

    if ($count -eq 0) {
        throw "$Label must provide at least one run-property clear field."
    }

    return $count
}

function Add-ParagraphStylePropertyMutationArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $count = 0
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("based_on", "based_on_style", "based_on_style_id") `
        -Flag "--based-on"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("next_style", "next_style_id", "next") `
        -Flag "--next-style"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("outline_level", "level") `
        -Flag "--outline-level"

    if ($count -eq 0) {
        throw "$Label must provide at least one paragraph-style mutation field."
    }

    return $count
}

function Add-ParagraphStylePropertyClearArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $count = 0
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_based_on", "based_on") `
        -Flag "--based-on"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_next_style", "next_style", "next") `
        -Flag "--next-style"
    $count += Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("clear_outline_level", "outline_level") `
        -Flag "--outline-level"

    if ($count -eq 0) {
        throw "$Label must provide at least one paragraph-style clear field."
    }

    return $count
}

function Add-NumberingDefinitionLevelArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $levels = $null
    foreach ($name in @("numbering_levels", "levels", "definition_levels")) {
        $levels = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $levels) {
            break
        }
    }

    if ($null -eq $levels) {
        throw "$Label must provide 'numbering_levels', 'levels', or 'definition_levels'."
    }
    if ($levels -is [string]) {
        if ([string]::IsNullOrWhiteSpace([string]$levels)) {
            throw "$Label numbering level definition strings must not be empty."
        }
        $Arguments.Add("--numbering-level") | Out-Null
        $Arguments.Add([string]$levels) | Out-Null
        return 1
    }

    $count = 0
    foreach ($level in @($levels)) {
        if ($level -is [string]) {
            if ([string]::IsNullOrWhiteSpace([string]$level)) {
                throw "$Label numbering level definition strings must not be empty."
            }
            $Arguments.Add("--numbering-level") | Out-Null
            $Arguments.Add([string]$level) | Out-Null
            $count += 1
            continue
        }

        $levelIndex = Get-FirstObjectPropertyValue `
            -Object $level `
            -Names @("level", "index") `
            -Label $Label
        $kind = Get-FirstObjectPropertyValue `
            -Object $level `
            -Names @("kind", "list_kind") `
            -Label $Label
        $start = Get-FirstOptionalObjectPropertyValue `
            -Object $level `
            -Names @("start")
        if ([string]::IsNullOrWhiteSpace($start)) {
            $start = "1"
        }
        $textPattern = Get-FirstObjectPropertyValue `
            -Object $level `
            -Names @("text_pattern", "pattern") `
            -Label $Label

        $Arguments.Add("--numbering-level") | Out-Null
        $Arguments.Add("$levelIndex`:$kind`:$start`:$textPattern") | Out-Null
        $count += 1
    }

    if ($count -eq 0) {
        throw "$Label must provide at least one numbering level definition."
    }

    return $count
}

function Add-SectionPageSetupArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $count = 0

    $orientation = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("orientation", "page_orientation")
    if (-not [string]::IsNullOrWhiteSpace($orientation)) {
        $Arguments.Add("--orientation") | Out-Null
        $Arguments.Add($orientation) | Out-Null
        $count += 1
    }

    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("width", "page_width", "width_twips", "page_width_twips") `
        -Flag "--width"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("height", "page_height", "height_twips", "page_height_twips") `
        -Flag "--height"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("margin_top", "top_margin", "margin_top_twips", "top_margin_twips") `
        -Flag "--margin-top"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("margin_bottom", "bottom_margin", "margin_bottom_twips", "bottom_margin_twips") `
        -Flag "--margin-bottom"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("margin_left", "left_margin", "margin_left_twips", "left_margin_twips") `
        -Flag "--margin-left"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("margin_right", "right_margin", "margin_right_twips", "right_margin_twips") `
        -Flag "--margin-right"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("margin_header", "header_margin", "margin_header_twips", "header_margin_twips") `
        -Flag "--margin-header"
    $count += Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("margin_footer", "footer_margin", "margin_footer_twips", "footer_margin_twips") `
        -Flag "--margin-footer"

    $pageNumberStart = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("page_number_start", "start_page_number")
    $clearPageNumberStart = $false
    $hasClearPageNumberStart = $false
    foreach ($name in @("clear_page_number_start", "clear_start_page_number")) {
        if (Test-ObjectPropertyExists -Object $Operation -Name $name) {
            $clearPageNumberStart = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name $name `
                -DefaultValue $false
            $hasClearPageNumberStart = $true
            break
        }
    }

    if ($hasClearPageNumberStart -and
        $clearPageNumberStart -and
        -not [string]::IsNullOrWhiteSpace($pageNumberStart)) {
        throw "$Label cannot combine 'page_number_start' with 'clear_page_number_start'."
    }

    if (-not [string]::IsNullOrWhiteSpace($pageNumberStart)) {
        $Arguments.Add("--page-number-start") | Out-Null
        $Arguments.Add($pageNumberStart) | Out-Null
        $count += 1
    } elseif ($hasClearPageNumberStart -and $clearPageNumberStart) {
        $Arguments.Add("--clear-page-number-start") | Out-Null
        $count += 1
    }

    if ($count -eq 0) {
        throw "$Label must provide at least one page-setup mutation field."
    }

    return $count
}

function Get-SectionPartReferenceKind {
    param(
        $Operation
    )

    return Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("kind", "reference_kind", "section_part_kind", "header_footer_kind")
}

function Add-SectionPartKindArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation
    )

    $kind = Get-SectionPartReferenceKind -Operation $Operation
    if (-not [string]::IsNullOrWhiteSpace($kind)) {
        $Arguments.Add("--kind") | Out-Null
        $Arguments.Add($kind) | Out-Null
        return 1
    }

    return 0
}

function Get-SectionPartIndexValue {
    param(
        $Operation,
        [string]$Label,
        [string]$Family
    )

    $names = if ($Family -eq "header") {
        @("header_index", "part_index", "index")
    } else {
        @("footer_index", "part_index", "index")
    }

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names $names `
        -Label $Label
}

function Get-SourceIndexValue {
    param(
        $Operation,
        [string]$Label
    )

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("source_index", "source", "from_index", "from") `
        -Label $Label
}

function Get-TargetIndexValue {
    param(
        $Operation,
        [string]$Label
    )

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("target_index", "target", "to_index", "to", "destination_index", "destination") `
        -Label $Label
}

function Add-SectionTextArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [string]$TemporaryRoot,
        [int]$OperationIndex
    )

    $text = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("text", "content")
    $textFile = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("text_file", "text_path", "content_file", "content_path")

    if (-not [string]::IsNullOrWhiteSpace($text) -and
        -not [string]::IsNullOrWhiteSpace($textFile)) {
        throw "$Label cannot combine 'text' with 'text_file'."
    }

    if (-not [string]::IsNullOrWhiteSpace($textFile)) {
        $Arguments.Add("--text-file") | Out-Null
        $Arguments.Add($textFile) | Out-Null
        return
    }

    if ([string]::IsNullOrWhiteSpace($text)) {
        throw "$Label must provide 'text' or 'text_file'."
    }

    $textPath = New-EditOperationTextFile `
        -TemporaryRoot $TemporaryRoot `
        -OperationIndex $OperationIndex `
        -ValueIndex 1 `
        -Text $text
    $Arguments.Add("--text-file") | Out-Null
    $Arguments.Add($textPath) | Out-Null
}

function Get-UpdateFieldsOnOpenEnabledValue {
    param(
        $Operation,
        [string]$Label,
        [AllowNull()]$DefaultValue = $null
    )

    foreach ($name in @("enabled", "enable", "update_fields_on_open")) {
        if (Test-ObjectPropertyExists -Object $Operation -Name $name) {
            return Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name $name `
                -DefaultValue $false
        }
    }

    if ($null -ne $DefaultValue) {
        return [bool]$DefaultValue
    }

    throw "$Label must provide one of: enabled, enable, update_fields_on_open."
}

function Add-TemplateTableSelectorArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $tableIndex = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("table_index", "table")
    $bookmark = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("bookmark", "bookmark_name")
    $afterText = Get-OptionalObjectPropertyValue -Object $Operation -Name "after_text"
    $headerCells = @(Get-TemplateTableHeaderCellValues -Operation $Operation)

    $selectorCount = 0
    if (-not [string]::IsNullOrWhiteSpace($tableIndex)) { ++$selectorCount }
    if (-not [string]::IsNullOrWhiteSpace($bookmark)) { ++$selectorCount }
    if (-not [string]::IsNullOrWhiteSpace($afterText)) { ++$selectorCount }
    if ($headerCells.Count -gt 0) { ++$selectorCount }

    if ($selectorCount -ne 1) {
        throw "$Label must provide exactly one template table selector: 'table_index', 'bookmark', 'after_text', or 'header_cell(s)'."
    }

    if (-not [string]::IsNullOrWhiteSpace($tableIndex)) {
        $Arguments.Add($tableIndex) | Out-Null
        return
    }

    if (-not [string]::IsNullOrWhiteSpace($bookmark)) {
        $Arguments.Add("--bookmark") | Out-Null
        $Arguments.Add($bookmark) | Out-Null
        return
    }

    if (-not [string]::IsNullOrWhiteSpace($afterText)) {
        $Arguments.Add("--after-text") | Out-Null
        $Arguments.Add($afterText) | Out-Null
    }

    foreach ($cell in $headerCells) {
        $Arguments.Add("--header-cell") | Out-Null
        $Arguments.Add($cell) | Out-Null
    }

    $headerRow = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("header_row", "header_row_index")
    if (-not [string]::IsNullOrWhiteSpace($headerRow)) {
        if ($headerCells.Count -eq 0) {
            throw "$Label header_row requires 'header_cell' or 'header_cells'."
        }
        $Arguments.Add("--header-row") | Out-Null
        $Arguments.Add($headerRow) | Out-Null
    }

    $occurrence = Get-OptionalObjectPropertyValue -Object $Operation -Name "occurrence"
    if (-not [string]::IsNullOrWhiteSpace($occurrence)) {
        $Arguments.Add("--occurrence") | Out-Null
        $Arguments.Add($occurrence) | Out-Null
    }
}

function Add-TemplateTableRowIndexArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $rowIndex = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("row_index", "row")
    if ([string]::IsNullOrWhiteSpace($rowIndex)) {
        throw "$Label must provide 'row_index'."
    }

    $Arguments.Add($rowIndex) | Out-Null
}

function Add-TemplateTableCellIndexArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $cellIndex = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("cell_index", "cell")
    if ([string]::IsNullOrWhiteSpace($cellIndex)) {
        throw "$Label must provide 'cell_index'."
    }

    $Arguments.Add($cellIndex) | Out-Null
}

function Add-TemplateTableCellTextTargetArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    Add-TemplateTableSelectorArguments -Arguments $Arguments -Operation $Operation -Label $Label
    Add-TemplateTableRowIndexArgument -Arguments $Arguments -Operation $Operation -Label $Label

    $cellIndex = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("cell_index", "cell")
    $gridColumn = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("grid_column", "column_index", "visual_column")

    $hasCellIndex = -not [string]::IsNullOrWhiteSpace($cellIndex)
    $hasGridColumn = -not [string]::IsNullOrWhiteSpace($gridColumn)
    if ($hasCellIndex -eq $hasGridColumn) {
        throw "$Label must provide exactly one of 'cell_index' or 'grid_column'."
    }

    if ($hasCellIndex) {
        $Arguments.Add($cellIndex) | Out-Null
        return
    }

    $Arguments.Add("--grid-column") | Out-Null
    $Arguments.Add($gridColumn) | Out-Null
}

function Add-TemplateTableRowsTextArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $rows = Get-OptionalObjectPropertyObject -Object $Operation -Name "rows"
    if ($null -eq $rows) {
        throw "$Label must provide 'rows'."
    }
    if ($rows -is [string]) {
        throw "$Label 'rows' must be an array of row arrays."
    }
    if ($rows.Count -eq 0) {
        throw "$Label rows must contain at least one row."
    }

    foreach ($row in @($rows)) {
        if ($row -is [string]) {
            $cells = @([string]$row)
        } elseif ($row -is [System.Collections.IEnumerable]) {
            $cells = @($row | ForEach-Object { [string]$_ })
        } else {
            throw "$Label rows must contain arrays of cell texts."
        }
        if ($cells.Count -eq 0) {
            throw "$Label table rows must contain at least one cell text."
        }
        $Arguments.Add("--row") | Out-Null
        $Arguments.Add([string]$cells[0]) | Out-Null
        for ($index = 1; $index -lt $cells.Count; ++$index) {
            $Arguments.Add("--cell") | Out-Null
            $Arguments.Add([string]$cells[$index]) | Out-Null
        }
    }
}
