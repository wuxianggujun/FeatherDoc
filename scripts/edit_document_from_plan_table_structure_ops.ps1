function Invoke-EditPlanTableStructureOperationArguments {
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
        "merge_table_cells" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $count = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("count", "merge_count")
            $arguments.Add("merge-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace($count)) {
                $arguments.Add("--count") | Out-Null
                $arguments.Add($count) | Out-Null
            }
        }
        "merge_table_cell" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $count = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("count", "merge_count")
            $arguments.Add("merge-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace($count)) {
                $arguments.Add("--count") | Out-Null
                $arguments.Add($count) | Out-Null
            }
        }
        "unmerge_table_cells" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $arguments.Add("unmerge-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
        }
        "unmerge_table_cell" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $arguments.Add("unmerge-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
        }
        "set_table_column_width" {
            return New-DirectTableColumnWidthOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "set_table_col_width" {
            return New-DirectTableColumnWidthOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "clear_table_column_width" {
            return New-DirectTableColumnWidthOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "clear_table_col_width" {
            return New-DirectTableColumnWidthOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "append_table_row" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("append-table-row") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null

            $cellCount = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("cell_count", "column_count", "columns")
            if (-not [string]::IsNullOrWhiteSpace($cellCount)) {
                $cellCountValue = [int]$cellCount
                if ($cellCountValue -le 0) {
                    throw "$Label cell_count must be greater than zero."
                }
                $arguments.Add("--cell-count") | Out-Null
                $arguments.Add([string]$cellCountValue) | Out-Null
            }
        }
        "insert_table_row_before" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("insert-table-row-before") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "insert_table_row_after" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("insert-table-row-after") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "remove_table_row" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("remove-table-row") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "delete_table_row" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("remove-table-row") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "append_template_table_row" {
            $arguments.Add("append-template-table-row") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateAppendRowOptions -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "insert_template_table_row_before" {
            $arguments.Add("insert-template-table-row-before") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "insert_template_table_row_after" {
            $arguments.Add("insert-template-table-row-after") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "remove_template_table_row" {
            $arguments.Add("remove-template-table-row") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "delete_template_table_row" {
            $arguments.Add("remove-template-table-row") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "insert_template_table_column_before" {
            $arguments.Add("insert-template-table-column-before") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "insert_template_table_column_after" {
            $arguments.Add("insert-template-table-column-after") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "remove_template_table_column" {
            $arguments.Add("remove-template-table-column") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "delete_template_table_column" {
            $arguments.Add("remove-template-table-column") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "set_template_table_cell_text" {
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $textPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1 `
                -Text $text
            $arguments.Add("set-template-table-cell-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableCellTextTargetArguments -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--text-file") | Out-Null
            $arguments.Add($textPath) | Out-Null
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "set_template_table_row_texts" {
            $arguments.Add("set-template-table-row-texts") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowsTextArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "set_template_table_rows_texts" {
            $arguments.Add("set-template-table-row-texts") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowsTextArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "set_template_table_cell_block_texts" {
            $arguments.Add("set-template-table-cell-block-texts") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowsTextArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "merge_template_table_cells" {
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $count = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("count", "merge_count")
            $arguments.Add("merge-template-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace($count)) {
                $arguments.Add("--count") | Out-Null
                $arguments.Add($count) | Out-Null
            }
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "merge_template_table_cell" {
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $count = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("count", "merge_count")
            $arguments.Add("merge-template-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace($count)) {
                $arguments.Add("--count") | Out-Null
                $arguments.Add($count) | Out-Null
            }
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "unmerge_template_table_cells" {
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $arguments.Add("unmerge-template-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "unmerge_template_table_cell" {
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $arguments.Add("unmerge-template-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableRowIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableCellIndexArgument -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "set_template_table_from_json" {
            $arguments.Add("set-template-table-from-json") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableJsonPatchFileArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "patch_template_table_from_json" {
            $arguments.Add("set-template-table-from-json") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-TemplateTableJsonPatchFileArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "set_template_tables_from_json" {
            $arguments.Add("set-template-tables-from-json") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableJsonPatchFileArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1 `
                -Batch
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "patch_template_tables_from_json" {
            $arguments.Add("set-template-tables-from-json") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-TemplateTableJsonPatchFileArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1 `
                -Batch
            Add-TemplatePartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "insert_table_column_before" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("insert-table-column-before") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "insert_table_column_after" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("insert-table-column-after") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "remove_table_column" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("remove-table-column") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "delete_table_column" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("remove-table-column") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "insert_table_before" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowCount = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("row_count", "rows") `
                -Label $Label
            $columnCount = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("column_count", "columns", "cell_count") `
                -Label $Label
            $rowCountValue = [int]$rowCount
            $columnCountValue = [int]$columnCount
            if ($rowCountValue -le 0) {
                throw "$Label row_count must be greater than zero."
            }
            if ($columnCountValue -le 0) {
                throw "$Label column_count must be greater than zero."
            }
            $arguments.Add("insert-table-before") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add([string]$rowCountValue) | Out-Null
            $arguments.Add([string]$columnCountValue) | Out-Null
        }
        "insert_table_after" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowCount = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("row_count", "rows") `
                -Label $Label
            $columnCount = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("column_count", "columns", "cell_count") `
                -Label $Label
            $rowCountValue = [int]$rowCount
            $columnCountValue = [int]$columnCount
            if ($rowCountValue -le 0) {
                throw "$Label row_count must be greater than zero."
            }
            if ($columnCountValue -le 0) {
                throw "$Label column_count must be greater than zero."
            }
            $arguments.Add("insert-table-after") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add([string]$rowCountValue) | Out-Null
            $arguments.Add([string]$columnCountValue) | Out-Null
        }
        "insert_table_like_before" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("insert-table-like-before") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "insert_table_like_after" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $arguments.Add("insert-table-like-after") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
        }
        "insert_paragraph_after_table" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $text = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text", "paragraph_text") `
                -Label $Label
            $textPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1 `
                -Text $text
            $arguments.Add("insert-paragraph-after-table") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add("--text-file") | Out-Null
            $arguments.Add($textPath) | Out-Null
        }
        default {
            return $false
        }
    }

    return $true
}
