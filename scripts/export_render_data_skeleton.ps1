<#
.SYNOPSIS
Exports a business-data JSON skeleton from a render-data mapping file.

.DESCRIPTION
Reads a render-data mapping document and writes a JSON skeleton that follows
every mapping `source` path. The generated data is intended as an editable
starting point before passing the file into
`convert_render_data_to_patch_plan.ps1` or
`render_template_document_from_data.ps1`.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$MappingPath,
    [string]$OutputData = "",
    [string]$SummaryJson = "",
    [int]$DefaultParagraphCount = 1,
    [int]$DefaultTableRowCount = 1,
    [int]$DefaultTableCellCount = 1
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[export-render-data-skeleton] $Message"
}

function Resolve-OptionalSkeletonPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    return Resolve-TemplateSchemaPath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing
}

function Ensure-PathParent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
}

function Get-OptionalObjectPropertyObject {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            $value = $Object[$Name]
            if ($value -is [System.Collections.IEnumerable] -and
                -not ($value -is [string]) -and
                -not ($value -is [System.Collections.IDictionary]) -and
                -not ($value -is [System.Management.Automation.PSCustomObject])) {
                Write-Output -NoEnumerate $value
                return
            }

            return $value
        }

        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    $value = $property.Value
    if ($value -is [System.Collections.IEnumerable] -and
        -not ($value -is [string]) -and
        -not ($value -is [System.Collections.IDictionary]) -and
        -not ($value -is [System.Management.Automation.PSCustomObject])) {
        Write-Output -NoEnumerate $value
        return
    }

    return $value
}

function Get-OptionalObjectPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return ""
    }

    return [string]$value
}

function Get-OptionalObjectArrayProperty {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }

    if ($value -is [string]) {
        return @($value)
    }

    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $true })
    }

    return @($value)
}

function Test-IsDictionaryLike {
    param($Value)

    return $Value -is [System.Collections.IDictionary] -or
        $Value -is [System.Management.Automation.PSCustomObject]
}

function Convert-PathToTraversalSteps {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        throw "$Label must not be empty."
    }

    $steps = New-Object 'System.Collections.Generic.List[object]'
    foreach ($segment in ($Path -split '\.')) {
        if ([string]::IsNullOrWhiteSpace($segment)) {
            throw "$Label contains an empty segment."
        }

        $remaining = $segment
        while ($remaining.Length -gt 0) {
            if ($remaining.StartsWith("[")) {
                $match = [regex]::Match($remaining, '^\[(\d+)\](.*)$')
                if (-not $match.Success) {
                    throw "$Label has an invalid array index segment '$remaining'."
                }

                $steps.Add([pscustomobject]@{
                        kind = "index"
                        value = [int]$match.Groups[1].Value
                    }) | Out-Null
                $remaining = $match.Groups[2].Value
                continue
            }

            $match = [regex]::Match($remaining, '^([^\[\]]+)(.*)$')
            if (-not $match.Success) {
                throw "$Label has an invalid property segment '$remaining'."
            }

            $steps.Add([pscustomobject]@{
                    kind = "property"
                    value = $match.Groups[1].Value
                }) | Out-Null
            $remaining = $match.Groups[2].Value
        }
    }

    Write-Output -NoEnumerate @($steps.ToArray())
    return
}

function Get-MappingBookmarkName {
    param(
        $Item,
        [string]$Category
    )

    $bookmarkName = Get-OptionalObjectPropertyValue -Object $Item -Name "bookmark_name"
    if ([string]::IsNullOrWhiteSpace($bookmarkName)) {
        $bookmarkName = Get-OptionalObjectPropertyValue -Object $Item -Name "bookmark"
    }

    if ([string]::IsNullOrWhiteSpace($bookmarkName)) {
        throw "$Category entries must provide 'bookmark_name' or 'bookmark'."
    }

    return $bookmarkName
}

function Get-MappingSourcePath {
    param(
        $Item,
        [string]$Category
    )

    $sourcePath = Get-OptionalObjectPropertyValue -Object $Item -Name "source"
    if ([string]::IsNullOrWhiteSpace($sourcePath)) {
        throw "$Category entries must provide a non-empty 'source' path."
    }

    return $sourcePath
}

function Convert-ToMutableArrayList {
    param($Value)

    if ($Value -is [System.Collections.ArrayList]) {
        return ,$Value
    }

    $items = New-Object System.Collections.ArrayList
    if ($null -eq $Value) {
        return ,$items
    }

    foreach ($item in @($Value)) {
        $items.Add($item) | Out-Null
    }

    return ,$items
}

function New-ContainerForNextStep {
    param([string]$NextKind)

    if ($NextKind -eq "index") {
        return (New-Object System.Collections.ArrayList)
    }

    return [ordered]@{}
}

function Test-IsArrayLike {
    param($Value)

    return $Value -is [System.Collections.IEnumerable] -and
        -not ($Value -is [string]) -and
        -not (Test-IsDictionaryLike -Value $Value)
}

function Copy-ValueForJson {
    param($Value)

    if ($null -eq $Value) {
        return $null
    }
    if ($Value -is [System.Collections.IDictionary]) {
        $copy = [ordered]@{}
        foreach ($propertyName in $Value.Keys) {
            $copiedChild = Copy-ValueForJson -Value $Value[$propertyName]
            $copy[$propertyName] = $copiedChild
        }
        return $copy
    }
    if ($Value -is [System.Management.Automation.PSCustomObject]) {
        $copy = [ordered]@{}
        foreach ($property in $Value.PSObject.Properties) {
            $copiedChild = Copy-ValueForJson -Value $property.Value
            $copy[[string]$property.Name] = $copiedChild
        }
        return $copy
    }
    if (Test-IsArrayLike -Value $Value) {
        $items = New-Object 'System.Collections.Generic.List[object]'
        foreach ($item in @($Value)) {
            $copiedItem = Copy-ValueForJson -Value $item
            $items.Add($copiedItem) | Out-Null
        }
        Write-Output -NoEnumerate @($items.ToArray())
        return
    }

    return $Value
}

function Merge-LeafValue {
    param(
        $ExistingValue,
        $DesiredValue,
        [string]$Label
    )

    if ($null -eq $ExistingValue) {
        $copiedValue = Copy-ValueForJson -Value $DesiredValue
        if (Test-IsArrayLike -Value $copiedValue) {
            Write-Output -NoEnumerate $copiedValue
            return
        }
        return $copiedValue
    }

    $existingJson = (Copy-ValueForJson -Value $ExistingValue) | ConvertTo-Json -Depth 24 -Compress
    $desiredJson = (Copy-ValueForJson -Value $DesiredValue) | ConvertTo-Json -Depth 24 -Compress
    if ($existingJson -ne $desiredJson) {
        throw "$Label conflicts with an existing generated value."
    }

    if (Test-IsArrayLike -Value $ExistingValue) {
        Write-Output -NoEnumerate $ExistingValue
        return
    }

    return $ExistingValue
}

function Set-GeneratedValueAtPath {
    param(
        $CurrentNode,
        [object[]]$Steps,
        [int]$Offset,
        $LeafValue,
        [string]$Label
    )

    $step = $Steps[$Offset]
    $isLeaf = $Offset -eq ($Steps.Count - 1)
    $nextKind = if ($isLeaf) { "" } else { [string]$Steps[$Offset + 1].kind }

    if ($step.kind -eq "property") {
        if ($null -eq $CurrentNode) {
            $CurrentNode = [ordered]@{}
        }
        if (-not (Test-IsDictionaryLike -Value $CurrentNode)) {
            throw "$Label conflicts with an existing non-object container."
        }

        $propertyName = [string]$step.value
        $existingChild = Get-OptionalObjectPropertyObject -Object $CurrentNode -Name $propertyName
        if ($isLeaf) {
            $mergedLeafValue = Merge-LeafValue `
                -ExistingValue $existingChild `
                -DesiredValue $LeafValue `
                -Label $Label
            $CurrentNode[$propertyName] = $mergedLeafValue
            return $CurrentNode
        }

        $updatedChild = Set-GeneratedValueAtPath `
            -CurrentNode $existingChild `
            -Steps $Steps `
            -Offset ($Offset + 1) `
            -LeafValue $LeafValue `
            -Label $Label
        $CurrentNode[$propertyName] = $updatedChild
        return $CurrentNode
    }

    if ($null -eq $CurrentNode) {
        $CurrentNode = New-Object System.Collections.ArrayList
    } elseif (-not (Test-IsArrayLike -Value $CurrentNode)) {
        throw "$Label conflicts with an existing non-array container."
    }

    $mutableArray = Convert-ToMutableArrayList -Value $CurrentNode
    if ($mutableArray -is [object[]] -and @($mutableArray).Count -eq 1 -and
        @($mutableArray)[0] -is [System.Collections.ArrayList]) {
        $mutableArray = $mutableArray[0]
    }
    $indexValue = [int]$step.value
    while ($mutableArray.Count -le $indexValue) {
        $mutableArray.Add($null) | Out-Null
    }

    $existingItem = $mutableArray[$indexValue]
    if ($isLeaf) {
        $mergedLeafValue = Merge-LeafValue `
            -ExistingValue $existingItem `
            -DesiredValue $LeafValue `
            -Label $Label
        $mutableArray[$indexValue] = $mergedLeafValue
        Write-Output -NoEnumerate $mutableArray
        return
    }

    $updatedItem = Set-GeneratedValueAtPath `
        -CurrentNode $existingItem `
        -Steps $Steps `
        -Offset ($Offset + 1) `
        -LeafValue $LeafValue `
        -Label $Label
    $mutableArray[$indexValue] = $updatedItem
    Write-Output -NoEnumerate $mutableArray
    return
}

function New-TextPlaceholder {
    param([string]$BookmarkName)

    return "TODO: $BookmarkName"
}

function New-ParagraphPlaceholderArray {
    param(
        [string]$BookmarkName,
        [int]$ParagraphCount
    )

    $items = New-Object System.Collections.ArrayList
    for ($index = 0; $index -lt $ParagraphCount; $index++) {
        if ($ParagraphCount -eq 1) {
            $items.Add("TODO: $BookmarkName") | Out-Null
        } else {
            $items.Add("TODO: $BookmarkName[$index]") | Out-Null
        }
    }

    Write-Output -NoEnumerate $items
    return
}

function New-TableRowPlaceholderArray {
    param(
        [string]$BookmarkName,
        [string[]]$Columns,
        [int]$RowCount,
        [int]$DefaultCellCount
    )

    $rows = New-Object System.Collections.ArrayList
    $columnValues = New-Object 'System.Collections.Generic.List[string]'
    if ($null -ne $Columns) {
        foreach ($columnValue in $Columns) {
            if (-not [string]::IsNullOrWhiteSpace([string]$columnValue)) {
                $columnValues.Add([string]$columnValue) | Out-Null
            }
        }
    }
    for ($rowIndex = 0; $rowIndex -lt $RowCount; $rowIndex++) {
        if ($columnValues.Count -gt 0) {
            $rowObject = $null
            for ($columnIndex = 0; $columnIndex -lt $columnValues.Count; $columnIndex++) {
                $columnPath = [string]$columnValues[$columnIndex]
                $columnSteps = Convert-PathToTraversalSteps `
                    -Path $columnPath `
                    -Label "bookmark_table_rows '$BookmarkName' column '$columnPath'"
                $columnValue = "TODO: $BookmarkName.row[$rowIndex].$columnPath"
                $rowObject = Set-GeneratedValueAtPath `
                    -CurrentNode $rowObject `
                    -Steps $columnSteps `
                    -Offset 0 `
                    -LeafValue $columnValue `
                    -Label "bookmark_table_rows '$BookmarkName' row[$rowIndex] column '$columnPath'"
            }
            $rows.Add((Copy-ValueForJson -Value $rowObject)) | Out-Null
            continue
        }

        $rowCells = New-Object System.Collections.ArrayList
        for ($cellIndex = 0; $cellIndex -lt $DefaultCellCount; $cellIndex++) {
            $rowCells.Add("TODO: $BookmarkName.row[$rowIndex].cell[$cellIndex]") | Out-Null
        }
        $rows.Add($rowCells) | Out-Null
    }

    Write-Output -NoEnumerate $rows
    return
}

function New-EntrySummary {
    param(
        [string]$Category,
        [int]$EntryIndex,
        [string]$BookmarkName,
        [string]$SourcePath,
        [string]$ValueKind,
        [int]$ParagraphCount,
        [int]$RowCount,
        [int]$ColumnCount
    )

    return [ordered]@{
        category = $Category
        entry_index = $EntryIndex
        bookmark_name = $BookmarkName
        source = $SourcePath
        value_kind = $ValueKind
        paragraph_count = $ParagraphCount
        row_count = $RowCount
        column_count = $ColumnCount
    }
}

function Get-JsonIndent {
    param([int]$Level)

    return ("  " * $Level)
}

function Convert-ScalarToJsonText {
    param($Value)

    if ($null -eq $Value) {
        return "null"
    }
    if ($Value -is [bool]) {
        if ([bool]$Value) {
            return "true"
        }

        return "false"
    }

    $stringValue = [string]$Value
    $stringValue = $stringValue.Replace('\', '\\')
    $stringValue = $stringValue.Replace('"', '\"')
    $stringValue = $stringValue.Replace("`r", '\r')
    $stringValue = $stringValue.Replace("`n", '\n')
    $stringValue = $stringValue.Replace("`t", '\t')
    return '"' + $stringValue + '"'
}

function Convert-GeneratedValueToJsonText {
    param(
        $Value,
        [int]$IndentLevel = 0
    )

    if ($null -eq $Value) {
        return "null"
    }

    if (Test-IsArrayLike -Value $Value) {
        $items = @($Value)
        if ($items.Count -eq 0) {
            return "[]"
        }

        $lines = New-Object 'System.Collections.Generic.List[string]'
        $lines.Add("[") | Out-Null
        foreach ($item in $items) {
            $itemJson = Convert-GeneratedValueToJsonText `
                -Value $item `
                -IndentLevel ($IndentLevel + 1)
            $lines.Add((Get-JsonIndent -Level ($IndentLevel + 1)) + $itemJson + ",") | Out-Null
        }
        $lines[$lines.Count - 1] = $lines[$lines.Count - 1].TrimEnd(',')
        $lines.Add((Get-JsonIndent -Level $IndentLevel) + "]") | Out-Null
        return ($lines -join [Environment]::NewLine)
    }

    if ($Value -is [System.Collections.IDictionary]) {
        $propertyNames = @($Value.Keys)
        if ($propertyNames.Count -eq 0) {
            return "{}"
        }

        $lines = New-Object 'System.Collections.Generic.List[string]'
        $lines.Add("{") | Out-Null
        foreach ($propertyName in $propertyNames) {
            $propertyJson = Convert-GeneratedValueToJsonText `
                -Value $Value[$propertyName] `
                -IndentLevel ($IndentLevel + 1)
            $lines.Add(
                (Get-JsonIndent -Level ($IndentLevel + 1)) +
                '"' + [string]$propertyName + '": ' + $propertyJson + ","
            ) | Out-Null
        }
        $lines[$lines.Count - 1] = $lines[$lines.Count - 1].TrimEnd(',')
        $lines.Add((Get-JsonIndent -Level $IndentLevel) + "}") | Out-Null
        return ($lines -join [Environment]::NewLine)
    }

    if ($Value -is [System.Management.Automation.PSCustomObject]) {
        $propertyNames = @($Value.PSObject.Properties | ForEach-Object { [string]$_.Name })
        if ($propertyNames.Count -eq 0) {
            return "{}"
        }

        $lines = New-Object 'System.Collections.Generic.List[string]'
        $lines.Add("{") | Out-Null
        foreach ($propertyName in $propertyNames) {
            $propertyJson = Convert-GeneratedValueToJsonText `
                -Value ($Value.PSObject.Properties[$propertyName].Value) `
                -IndentLevel ($IndentLevel + 1)
            $lines.Add(
                (Get-JsonIndent -Level ($IndentLevel + 1)) +
                '"' + $propertyName + '": ' + $propertyJson + ","
            ) | Out-Null
        }
        $lines[$lines.Count - 1] = $lines[$lines.Count - 1].TrimEnd(',')
        $lines.Add((Get-JsonIndent -Level $IndentLevel) + "}") | Out-Null
        return ($lines -join [Environment]::NewLine)
    }

    return Convert-ScalarToJsonText -Value $Value
}

function Build-SkeletonSummary {
    param(
        [string]$Status,
        [string]$MappingPath,
        [string]$OutputData,
        [string]$SummaryJsonPath,
        [int]$DefaultParagraphCount,
        [int]$DefaultTableRowCount,
        [int]$DefaultTableCellCount,
        $MappingCounts,
        [int]$SourceCount,
        [int]$ConflictCount,
        $Entries,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        mapping_path = $MappingPath
        output_data = $OutputData
        summary_json = $SummaryJsonPath
        default_paragraph_count = $DefaultParagraphCount
        default_table_row_count = $DefaultTableRowCount
        default_table_cell_count = $DefaultTableCellCount
        mapping_counts = $MappingCounts
        mapping_entry_count = (
            [int]$MappingCounts.bookmark_text +
            [int]$MappingCounts.bookmark_paragraphs +
            [int]$MappingCounts.bookmark_table_rows +
            [int]$MappingCounts.bookmark_block_visibility
        )
        source_count = $SourceCount
        conflict_count = $ConflictCount
        entries = @($Entries)
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedMappingPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $MappingPath
$resolvedOutputData = if ([string]::IsNullOrWhiteSpace($OutputData)) {
    $mappingDirectory = [System.IO.Path]::GetDirectoryName($resolvedMappingPath)
    if ([string]::IsNullOrWhiteSpace($mappingDirectory)) {
        $mappingDirectory = $repoRoot
    }

    $mappingStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedMappingPath)
    if ($mappingStem.EndsWith(".render_data_mapping")) {
        $mappingStem = $mappingStem.Substring(0, $mappingStem.Length - ".render_data_mapping".Length)
    }

    Join-Path $mappingDirectory ($mappingStem + ".render_data.skeleton.json")
} else {
    Resolve-OptionalSkeletonPath -RepoRoot $repoRoot -InputPath $OutputData
}
$resolvedSummaryJson = Resolve-OptionalSkeletonPath -RepoRoot $repoRoot -InputPath $SummaryJson

if ($DefaultParagraphCount -lt 1) {
    throw "DefaultParagraphCount must be at least 1."
}
if ($DefaultTableRowCount -lt 1) {
    throw "DefaultTableRowCount must be at least 1."
}
if ($DefaultTableCellCount -lt 1) {
    throw "DefaultTableCellCount must be at least 1."
}

$mappingCounts = [ordered]@{
    bookmark_text = 0
    bookmark_paragraphs = 0
    bookmark_table_rows = 0
    bookmark_block_visibility = 0
}
$entrySummaries = New-Object System.Collections.ArrayList
$sourceCount = 0
$conflictCount = 0
$status = "failed"
$failureMessage = ""
$generatedData = $null

Ensure-PathParent -Path $resolvedOutputData
Ensure-PathParent -Path $resolvedSummaryJson

try {
    Write-Step "Loading render-data mapping from $resolvedMappingPath"
    $mappingObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedMappingPath | ConvertFrom-Json

    foreach ($category in @("bookmark_text", "bookmark_paragraphs", "bookmark_table_rows", "bookmark_block_visibility")) {
        $entries = @(Get-OptionalObjectArrayProperty -Object $mappingObject -Name $category)
        $mappingCounts[$category] = $entries.Count

        for ($entryIndex = 0; $entryIndex -lt $entries.Count; $entryIndex++) {
            $mappingEntry = $entries[$entryIndex]
            $bookmarkName = Get-MappingBookmarkName -Item $mappingEntry -Category $category
            $sourcePath = Get-MappingSourcePath -Item $mappingEntry -Category $category
            $steps = Convert-PathToTraversalSteps `
                -Path $sourcePath `
                -Label "$category '$bookmarkName' source"

            $paragraphCount = 0
            $rowCount = 0
            $columnCount = 0
            $valueKind = ""
            $leafValue = $null

            switch ($category) {
                "bookmark_text" {
                    $valueKind = "scalar"
                    $leafValue = New-TextPlaceholder -BookmarkName $bookmarkName
                }
                "bookmark_paragraphs" {
                    $valueKind = "array"
                    $paragraphCount = $DefaultParagraphCount
                    $leafValue = New-ParagraphPlaceholderArray `
                        -BookmarkName $bookmarkName `
                        -ParagraphCount $DefaultParagraphCount
                }
                "bookmark_table_rows" {
                    $valueKind = "array"
                    $columns = @(
                        Get-OptionalObjectArrayProperty -Object $mappingEntry -Name "columns" |
                            ForEach-Object {
                                $columnPath = [string]$_
                                if ([string]::IsNullOrWhiteSpace($columnPath)) {
                                    throw "bookmark_table_rows '$bookmarkName' contains an empty column path."
                                }
                                $columnPath
                            }
                    )
                    $rowCount = $DefaultTableRowCount
                    $columnCount = $columns.Count
                    $leafValue = New-TableRowPlaceholderArray `
                        -BookmarkName $bookmarkName `
                        -Columns $columns `
                        -RowCount $DefaultTableRowCount `
                        -DefaultCellCount $DefaultTableCellCount
                }
                "bookmark_block_visibility" {
                    $valueKind = "boolean"
                    $leafValue = $true
                }
            }

            try {
                $generatedData = Set-GeneratedValueAtPath `
                    -CurrentNode $generatedData `
                    -Steps $steps `
                    -Offset 0 `
                    -LeafValue $leafValue `
                    -Label "$category '$bookmarkName' source '$sourcePath'"
            } catch {
                $conflictCount++
                throw
            }

            $sourceCount++
            $entrySummaries.Add((New-EntrySummary `
                    -Category $category `
                    -EntryIndex $entryIndex `
                    -BookmarkName $bookmarkName `
                    -SourcePath $sourcePath `
                    -ValueKind $valueKind `
                    -ParagraphCount $paragraphCount `
                    -RowCount $rowCount `
                    -ColumnCount $columnCount)) | Out-Null
        }
    }

    if ($null -eq $generatedData) {
        throw "Mapping file does not contain any supported source entries."
    }

    Write-Step "Writing render-data skeleton to $resolvedOutputData"
    $jsonText = Convert-GeneratedValueToJsonText -Value $generatedData
    $jsonText | Set-Content -LiteralPath $resolvedOutputData -Encoding UTF8
    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    throw
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-SkeletonSummary `
            -Status $status `
            -MappingPath $resolvedMappingPath `
            -OutputData $resolvedOutputData `
            -SummaryJsonPath $resolvedSummaryJson `
            -DefaultParagraphCount $DefaultParagraphCount `
            -DefaultTableRowCount $DefaultTableRowCount `
            -DefaultTableCellCount $DefaultTableCellCount `
            -MappingCounts $mappingCounts `
            -SourceCount $sourceCount `
            -ConflictCount $conflictCount `
            -Entries $entrySummaries `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }
}

Write-Step "Render-data skeleton written to $resolvedOutputData"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
