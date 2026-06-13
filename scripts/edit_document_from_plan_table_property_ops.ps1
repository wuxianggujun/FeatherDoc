function Invoke-EditPlanTablePropertyOperationArguments {
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
        "set_table_position" {
            $tableIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("table_index", "target") `
                -Label $Label
            $arguments.Add("set-table-position") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null

            $additionalTables = Get-OptionalObjectPropertyObject -Object $Operation -Name "additional_table_indices"
            if ($null -eq $additionalTables) {
                $additionalTables = Get-OptionalObjectPropertyObject -Object $Operation -Name "additional_tables"
            }
            if ($null -ne $additionalTables) {
                if ($additionalTables -is [string]) {
                    $additionalTables = @($additionalTables)
                }
                foreach ($additionalTable in @($additionalTables)) {
                    $arguments.Add("--table") | Out-Null
                    $arguments.Add([string]$additionalTable) | Out-Null
                }
            }

            $preset = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("preset", "position_preset")
            if (-not [string]::IsNullOrWhiteSpace($preset)) {
                $arguments.Add("--preset") | Out-Null
                $arguments.Add($preset) | Out-Null
            } else {
                $horizontalReference = Get-FirstObjectPropertyValue `
                    -Object $Operation `
                    -Names @("horizontal_reference", "horizontal_anchor") `
                    -Label $Label
                $horizontalOffset = Get-FirstObjectPropertyValue `
                    -Object $Operation `
                    -Names @("horizontal_offset", "horizontal_offset_twips") `
                    -Label $Label
                $verticalReference = Get-FirstObjectPropertyValue `
                    -Object $Operation `
                    -Names @("vertical_reference", "vertical_anchor") `
                    -Label $Label
                $verticalOffset = Get-FirstObjectPropertyValue `
                    -Object $Operation `
                    -Names @("vertical_offset", "vertical_offset_twips") `
                    -Label $Label
                $arguments.Add("--horizontal-reference") | Out-Null
                $arguments.Add($horizontalReference) | Out-Null
                $arguments.Add("--horizontal-offset") | Out-Null
                $arguments.Add($horizontalOffset) | Out-Null
                $arguments.Add("--vertical-reference") | Out-Null
                $arguments.Add($verticalReference) | Out-Null
                $arguments.Add("--vertical-offset") | Out-Null
                $arguments.Add($verticalOffset) | Out-Null
            }

            $positionOptions = @(
                [pscustomobject]@{ Name = "horizontal_offset"; Flag = "--horizontal-offset" },
                [pscustomobject]@{ Name = "horizontal_offset_twips"; Flag = "--horizontal-offset" },
                [pscustomobject]@{ Name = "horizontal_spec"; Flag = "--horizontal-spec" },
                [pscustomobject]@{ Name = "horizontal_position_spec"; Flag = "--horizontal-spec" },
                [pscustomobject]@{ Name = "vertical_offset"; Flag = "--vertical-offset" },
                [pscustomobject]@{ Name = "vertical_offset_twips"; Flag = "--vertical-offset" },
                [pscustomobject]@{ Name = "vertical_spec"; Flag = "--vertical-spec" },
                [pscustomobject]@{ Name = "vertical_position_spec"; Flag = "--vertical-spec" },
                [pscustomobject]@{ Name = "left_from_text"; Flag = "--left-from-text" },
                [pscustomobject]@{ Name = "left_from_text_twips"; Flag = "--left-from-text" },
                [pscustomobject]@{ Name = "right_from_text"; Flag = "--right-from-text" },
                [pscustomobject]@{ Name = "right_from_text_twips"; Flag = "--right-from-text" },
                [pscustomobject]@{ Name = "top_from_text"; Flag = "--top-from-text" },
                [pscustomobject]@{ Name = "top_from_text_twips"; Flag = "--top-from-text" },
                [pscustomobject]@{ Name = "bottom_from_text"; Flag = "--bottom-from-text" },
                [pscustomobject]@{ Name = "bottom_from_text_twips"; Flag = "--bottom-from-text" },
                [pscustomobject]@{ Name = "overlap"; Flag = "--overlap" }
            )
            $emittedPositionFlags = New-Object 'System.Collections.Generic.HashSet[string]'
            if ([string]::IsNullOrWhiteSpace($preset)) {
                $emittedPositionFlags.Add("--horizontal-offset") | Out-Null
                $emittedPositionFlags.Add("--vertical-offset") | Out-Null
            }
            foreach ($positionOption in $positionOptions) {
                if ($emittedPositionFlags.Contains($positionOption.Flag)) {
                    continue
                }
                $value = Get-OptionalObjectPropertyValue -Object $Operation -Name $positionOption.Name
                if ([string]::IsNullOrWhiteSpace($value)) {
                    continue
                }
                $arguments.Add($positionOption.Flag) | Out-Null
                $arguments.Add($value) | Out-Null
                $emittedPositionFlags.Add($positionOption.Flag) | Out-Null
            }
        }
        "clear_table_position" {
            $tableIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("table_index", "target") `
                -Label $Label
            $arguments.Add("clear-table-position") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null

            $additionalTables = Get-OptionalObjectPropertyObject -Object $Operation -Name "additional_table_indices"
            if ($null -eq $additionalTables) {
                $additionalTables = Get-OptionalObjectPropertyObject -Object $Operation -Name "additional_tables"
            }
            if ($null -ne $additionalTables) {
                if ($additionalTables -is [string]) {
                    $additionalTables = @($additionalTables)
                }
                foreach ($additionalTable in @($additionalTables)) {
                    $arguments.Add("--table") | Out-Null
                    $arguments.Add([string]$additionalTable) | Out-Null
                }
            }
        }
        "apply_table_position_plan" {
            Add-ApplyTablePositionPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        "remove_table" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("remove-table") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "delete_table" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("remove-table") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "set_table_style_id" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $styleId = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("style_id", "table_style_id", "style") `
                -Label $Label
            if ([string]::IsNullOrWhiteSpace($styleId)) {
                throw "$Label style_id must not be empty."
            }
            $arguments.Add("set-table-style-id") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($styleId) | Out-Null
        }
        "clear_table_style_id" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("clear-table-style-id") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "set_table_style_look" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("set-table-style-look") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null

            $styleLookFlags = @(
                [pscustomobject]@{ Name = "first_row"; Flag = "--first-row" },
                [pscustomobject]@{ Name = "last_row"; Flag = "--last-row" },
                [pscustomobject]@{ Name = "first_column"; Flag = "--first-column" },
                [pscustomobject]@{ Name = "last_column"; Flag = "--last-column" },
                [pscustomobject]@{ Name = "banded_rows"; Flag = "--banded-rows" },
                [pscustomobject]@{ Name = "banded_columns"; Flag = "--banded-columns" }
            )
            $providedFlagCount = 0
            foreach ($styleLookFlag in $styleLookFlags) {
                $value = Get-OptionalObjectPropertyValue -Object $Operation -Name $styleLookFlag.Name
                if ([string]::IsNullOrWhiteSpace($value)) {
                    continue
                }

                $arguments.Add($styleLookFlag.Flag) | Out-Null
                $arguments.Add(([string]$value).Trim().ToLowerInvariant()) | Out-Null
                $providedFlagCount += 1
            }
            if ($providedFlagCount -eq 0) {
                throw "$Label must provide at least one table style-look flag."
            }
        }
        "clear_table_style_look" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("clear-table-style-look") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "set_table_width" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $widthTwips = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("width_twips", "table_width_twips", "width") `
                -Label $Label
            $widthValue = [int]$widthTwips
            if ($widthValue -le 0) {
                throw "$Label width_twips must be greater than zero."
            }
            $arguments.Add("set-table-width") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add([string]$widthValue) | Out-Null
        }
        "clear_table_width" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("clear-table-width") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "set_table_layout_mode" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $layoutMode = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("layout_mode", "mode") `
                -Label $Label
            if ($layoutMode -ne "autofit" -and $layoutMode -ne "fixed") {
                throw "$Label layout_mode must be 'autofit' or 'fixed'."
            }
            $arguments.Add("set-table-layout-mode") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($layoutMode) | Out-Null
        }
        "clear_table_layout_mode" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("clear-table-layout-mode") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "set_table_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $alignment = Get-OperationAlignmentProperty `
                -Operation $Operation `
                -Label $Label `
                -FallbackName "table_alignment"
            $alignment = $alignment.Trim().ToLowerInvariant()
            if ($alignment -ne "left" -and $alignment -ne "center" -and $alignment -ne "right") {
                throw "$Label alignment must be 'left', 'center', or 'right'."
            }
            $arguments.Add("set-table-alignment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($alignment) | Out-Null
        }
        "clear_table_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("clear-table-alignment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "set_table_indent" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $indentTwips = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("indent_twips", "table_indent_twips", "indent") `
                -Label $Label
            $indentValue = [int]$indentTwips
            if ($indentValue -lt 0) {
                throw "$Label indent_twips must not be negative."
            }
            $arguments.Add("set-table-indent") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add([string]$indentValue) | Out-Null
        }
        "clear_table_indent" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("clear-table-indent") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "set_table_cell_spacing" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $spacingTwips = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("cell_spacing_twips", "spacing_twips", "spacing") `
                -Label $Label
            $spacingValue = [int]$spacingTwips
            if ($spacingValue -lt 0) {
                throw "$Label cell_spacing_twips must not be negative."
            }
            $arguments.Add("set-table-cell-spacing") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add([string]$spacingValue) | Out-Null
        }
        "clear_table_cell_spacing" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("clear-table-cell-spacing") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "set_table_default_cell_margin" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "margin_edge") `
                -Label $Label
            $marginTwips = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("margin_twips", "default_cell_margin_twips", "cell_margin_twips", "margin") `
                -Label $Label
            $marginValue = [int]$marginTwips
            if ($marginValue -lt 0) {
                throw "$Label margin_twips must not be negative."
            }
            $arguments.Add("set-table-default-cell-margin") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
            $arguments.Add([string]$marginValue) | Out-Null
        }
        "clear_table_default_cell_margin" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "margin_edge") `
                -Label $Label
            $arguments.Add("clear-table-default-cell-margin") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
        }
        "set_table_border" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "border_edge") `
                -Label $Label
            $style = Get-OptionalObjectPropertyValue -Object $Operation -Name "style"
            if ([string]::IsNullOrWhiteSpace($style)) {
                $style = "single"
            }
            $arguments.Add("set-table-border") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
            $arguments.Add("--style") | Out-Null
            $arguments.Add($style) | Out-Null
            $size = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("size", "width", "thickness")
            if (-not [string]::IsNullOrWhiteSpace($size)) {
                $arguments.Add("--size") | Out-Null
                $arguments.Add($size) | Out-Null
            }
            $color = Get-OptionalObjectPropertyValue -Object $Operation -Name "color"
            if (-not [string]::IsNullOrWhiteSpace($color)) {
                $arguments.Add("--color") | Out-Null
                $arguments.Add($color) | Out-Null
            }
            $space = Get-OptionalObjectPropertyValue -Object $Operation -Name "space"
            if (-not [string]::IsNullOrWhiteSpace($space)) {
                $arguments.Add("--space") | Out-Null
                $arguments.Add($space) | Out-Null
            }
        }
        "clear_table_border" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "border_edge") `
                -Label $Label
            $arguments.Add("clear-table-border") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
        }
        "set_table_cell_text" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $textPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1 `
                -Text $text
            $arguments.Add("set-table-cell-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--text-file") | Out-Null
            $arguments.Add($textPath) | Out-Null
        }
        "set_table_cell_fill" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $color = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("color", "fill_color", "background_color", "bg_color")
            if ([string]::IsNullOrWhiteSpace($color)) {
                throw "$Label must provide one of: color, fill_color, background_color, bg_color."
            }
            $arguments.Add("set-table-cell-fill") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($color) | Out-Null
        }
        "clear_table_cell_fill" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("clear-table-cell-fill") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "set_table_cell_width" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $widthTwips = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("width_twips", "cell_width_twips", "width", "twips") `
                -Label $Label
            $widthValue = [int]$widthTwips
            if ($widthValue -le 0) {
                throw "$Label width_twips must be greater than zero."
            }
            $arguments.Add("set-table-cell-width") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add([string]$widthValue) | Out-Null
        }
        "clear_table_cell_width" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("clear-table-cell-width") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "set_table_cell_margin" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "margin_edge") `
                -Label $Label
            $marginTwips = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("margin_twips", "twips", "margin") `
                -Label $Label
            $marginValue = [int]$marginTwips
            if ($marginValue -lt 0) {
                throw "$Label margin_twips must not be negative."
            }
            $arguments.Add("set-table-cell-margin") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
            $arguments.Add([string]$marginValue) | Out-Null
        }
        "clear_table_cell_margin" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "margin_edge") `
                -Label $Label
            $arguments.Add("clear-table-cell-margin") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
        }
        "set_table_cell_border" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "border_edge") `
                -Label $Label
            $style = Get-OptionalObjectPropertyValue -Object $Operation -Name "style"
            if ([string]::IsNullOrWhiteSpace($style)) {
                $style = "single"
            }
            $arguments.Add("set-table-cell-border") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
            $arguments.Add("--style") | Out-Null
            $arguments.Add($style) | Out-Null
            $size = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("size", "width", "thickness")
            if (-not [string]::IsNullOrWhiteSpace($size)) {
                $arguments.Add("--size") | Out-Null
                $arguments.Add($size) | Out-Null
            }
            $color = Get-OptionalObjectPropertyValue -Object $Operation -Name "color"
            if (-not [string]::IsNullOrWhiteSpace($color)) {
                $arguments.Add("--color") | Out-Null
                $arguments.Add($color) | Out-Null
            }
            $space = Get-OptionalObjectPropertyValue -Object $Operation -Name "space"
            if (-not [string]::IsNullOrWhiteSpace($space)) {
                $arguments.Add("--space") | Out-Null
                $arguments.Add($space) | Out-Null
            }
        }
        "clear_table_cell_border" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "border_edge") `
                -Label $Label
            $arguments.Add("clear-table-cell-border") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
        }
        "set_table_cell_vertical_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $alignment = Get-OperationAlignmentProperty `
                -Operation $Operation `
                -Label $Label `
                -FallbackName "vertical_alignment"
            $arguments.Add("set-table-cell-vertical-alignment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($alignment) | Out-Null
        }
        "clear_table_cell_vertical_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("clear-table-cell-vertical-alignment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "set_table_cell_text_direction" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "text_direction") `
                -Label $Label
            $arguments.Add("set-table-cell-text-direction") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($direction) | Out-Null
        }
        "clear_table_cell_text_direction" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("clear-table-cell-text-direction") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "set_table_row_height" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $heightTwips = Get-RequiredObjectPropertyValue -Object $Operation -Name "height_twips" -Label $Label
            $heightRule = Get-OperationRowHeightRule -Operation $Operation
            $arguments.Add("set-table-row-height") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($heightTwips) | Out-Null
            $arguments.Add($heightRule) | Out-Null
        }
        "clear_table_row_height" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("clear-table-row-height") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "set_table_row_cant_split" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("set-table-row-cant-split") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "clear_table_row_cant_split" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("clear-table-row-cant-split") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "set_table_row_repeat_header" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("set-table-row-repeat-header") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "clear_table_row_repeat_header" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("clear-table-row-repeat-header") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "set_table_cell_horizontal_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $alignment = Normalize-TableCellHorizontalAlignment `
                -Alignment (Get-OperationAlignmentProperty `
                    -Operation $Operation `
                    -Label $Label `
                    -FallbackName "horizontal_alignment") `
                -Label $Label
            return [pscustomobject]@{
                Operation = $normalizedOp
                Arguments = @()
                Command = "set-table-cell-horizontal-alignment"
                DirectOperation = $true
                DirectOperationKind = "table-cell-horizontal-alignment"
                TableIndex = $tableIndex
                RowIndex = $rowIndex
                CellIndex = $cellIndex
                HorizontalAlignment = $alignment
                ClearHorizontalAlignment = $false
            }
        }
        "clear_table_cell_horizontal_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            return [pscustomobject]@{
                Operation = $normalizedOp
                Arguments = @()
                Command = "clear-table-cell-horizontal-alignment"
                DirectOperation = $true
                DirectOperationKind = "table-cell-horizontal-alignment"
                TableIndex = $tableIndex
                RowIndex = $rowIndex
                CellIndex = $cellIndex
                HorizontalAlignment = ""
                ClearHorizontalAlignment = $true
            }
        }
        default {
            return $false
        }
    }

    return $true
}
