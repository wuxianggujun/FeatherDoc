<#
.SYNOPSIS
Exports an editable render-data mapping draft from a render plan.

.DESCRIPTION
Reads a template render-plan JSON file and writes a render-data mapping draft.
The generated mapping preserves bookmark target selectors and uses bookmark
names as editable `source` paths. Table-row entries infer `column_1`,
`column_2`, ... paths when example rows are present in the render plan.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputPlan,
    [string]$OutputMapping = "",
    [string]$SummaryJson = "",
    [string]$SourceRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[export-render-data-mapping-draft] $Message"
}

function Resolve-OptionalDraftPath {
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

function Get-RelativePathString {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $resolvedBasePath = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $resolvedBasePath.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedBasePath.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedBasePath += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = New-Object System.Uri($resolvedBasePath)
    $targetUri = New-Object System.Uri([System.IO.Path]::GetFullPath($TargetPath))
    return [System.Uri]::UnescapeDataString(
        $baseUri.MakeRelativeUri($targetUri).ToString()
    ).Replace('\', '/')
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
            return $Object[$Name]
        }

        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
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

function Get-ObjectArrayProperty {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }

    if ($value -is [string]) {
        throw "$Name must be an array."
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $true })
    }

    return @($value)
}

function Get-BookmarkName {
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

function Convert-BookmarkNameToSourceSegment {
    param([string]$BookmarkName)

    $segment = $BookmarkName.Trim() -replace '\s+', '_' -replace '\.', '_'
    if ([string]::IsNullOrWhiteSpace($segment)) {
        return "field"
    }

    return $segment
}

function Join-SourcePath {
    param(
        [string]$Root,
        [string]$Segment
    )

    $sourceSegment = Convert-BookmarkNameToSourceSegment -BookmarkName $Segment
    if ([string]::IsNullOrWhiteSpace($Root)) {
        return $sourceSegment
    }

    return ($Root.Trim().TrimEnd('.') + "." + $sourceSegment)
}

function Copy-TargetSelector {
    param(
        $SourceEntry,
        $TargetEntry
    )

    foreach ($propertyName in @("part", "index", "section", "kind")) {
        $value = Get-OptionalObjectPropertyObject -Object $SourceEntry -Name $propertyName
        if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
            continue
        }

        if ($propertyName -eq "index" -or $propertyName -eq "section") {
            $TargetEntry[$propertyName] = [int]$value
        } else {
            $TargetEntry[$propertyName] = [string]$value
        }
    }
}

function Convert-SelectorToSummaryObject {
    param($Entry)

    $selector = [ordered]@{}
    foreach ($propertyName in @("part", "index", "section", "kind")) {
        $value = Get-OptionalObjectPropertyObject -Object $Entry -Name $propertyName
        if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
            continue
        }

        if ($propertyName -eq "index" -or $propertyName -eq "section") {
            $selector[$propertyName] = [int]$value
        } else {
            $selector[$propertyName] = [string]$value
        }
    }

    return $selector
}

function Get-RowCellCount {
    param($Row)

    if ($null -eq $Row) {
        return 0
    }
    if ($Row -is [string]) {
        return 1
    }
    if ($Row -is [System.Collections.IEnumerable]) {
        return @($Row | Where-Object { $true }).Count
    }

    return 1
}

function Get-InferredTableColumns {
    param($Entry)

    $rows = Get-ObjectArrayProperty -Object $Entry -Name "rows"
    $maxCellCount = 0
    foreach ($row in $rows) {
        $cellCount = Get-RowCellCount -Row $row
        if ($cellCount -gt $maxCellCount) {
            $maxCellCount = $cellCount
        }
    }

    if ($maxCellCount -le 0) {
        return @()
    }

    $columns = New-Object 'System.Collections.Generic.List[string]'
    for ($index = 1; $index -le $maxCellCount; $index++) {
        $columns.Add("column_$index") | Out-Null
    }

    return @($columns.ToArray())
}

function New-MappingEntry {
    param(
        $SourceEntry,
        [string]$Category,
        [string]$SourceRoot,
        [string[]]$Columns
    )

    $bookmarkName = Get-BookmarkName -Item $SourceEntry -Category $Category
    $entry = [ordered]@{
        bookmark_name = $bookmarkName
        source = Join-SourcePath -Root $SourceRoot -Segment $bookmarkName
    }

    Copy-TargetSelector -SourceEntry $SourceEntry -TargetEntry $entry

    $columnValues = @()
    if ($null -ne $Columns) {
        $columnValues = @($Columns)
    }

    if ($Category -eq "bookmark_table_rows" -and $columnValues.Count -gt 0) {
        $entry.columns = $columnValues
    }

    return $entry
}

function New-EntrySummary {
    param(
        [string]$Category,
        [int]$Index,
        $MappingEntry,
        [string[]]$Columns
    )

    $selector = Convert-SelectorToSummaryObject -Entry $MappingEntry
    $columnValues = @()
    if ($null -ne $Columns) {
        $columnValues = @($Columns)
    }

    return [ordered]@{
        category = $Category
        entry_index = $Index
        bookmark_name = [string]$MappingEntry.bookmark_name
        source = [string]$MappingEntry.source
        selector = $selector
        inferred_column_count = $columnValues.Count
        columns = $columnValues
    }
}

function Build-DraftSummary {
    param(
        [string]$Status,
        [string]$InputPlan,
        [string]$OutputMapping,
        [string]$SummaryJson,
        [string]$SourceRoot,
        $MappingCounts,
        [int]$InferredTableColumnCount,
        $Entries,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_plan = $InputPlan
        output_mapping = $OutputMapping
        summary_json = $SummaryJson
        source_root = $SourceRoot
        mapping_counts = $MappingCounts
        mapping_entry_count = (
            [int]$MappingCounts.bookmark_text +
            [int]$MappingCounts.bookmark_paragraphs +
            [int]$MappingCounts.bookmark_table_rows +
            [int]$MappingCounts.bookmark_block_visibility
        )
        inferred_table_column_count = $InferredTableColumnCount
        entries = @($Entries)
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputPlan = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputPlan
$resolvedOutputMapping = if ([string]::IsNullOrWhiteSpace($OutputMapping)) {
    $planDirectory = [System.IO.Path]::GetDirectoryName($resolvedInputPlan)
    if ([string]::IsNullOrWhiteSpace($planDirectory)) {
        $planDirectory = $repoRoot
    }

    $planStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedInputPlan)
    if ($planStem.EndsWith(".render-plan")) {
        $planStem = $planStem.Substring(0, $planStem.Length - ".render-plan".Length)
    }

    Join-Path $planDirectory ($planStem + ".render_data_mapping.draft.json")
} else {
    Resolve-OptionalDraftPath -RepoRoot $repoRoot -InputPath $OutputMapping
}
$resolvedSummaryJson = Resolve-OptionalDraftPath -RepoRoot $repoRoot -InputPath $SummaryJson
$schemaPath = Join-Path $repoRoot "samples\template_render_data_mapping.schema.json"
$outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedOutputMapping)
if ([string]::IsNullOrWhiteSpace($outputDirectory)) {
    $outputDirectory = $repoRoot
}
$schemaReference = Get-RelativePathString -BasePath $outputDirectory -TargetPath $schemaPath
if (-not ($schemaReference.StartsWith("./") -or $schemaReference.StartsWith("../"))) {
    $schemaReference = "./" + $schemaReference
}

$mappingObject = [ordered]@{
    '$schema' = $schemaReference
    bookmark_text = @()
    bookmark_paragraphs = @()
    bookmark_table_rows = @()
    bookmark_block_visibility = @()
}
$mappingCounts = [ordered]@{
    bookmark_text = 0
    bookmark_paragraphs = 0
    bookmark_table_rows = 0
    bookmark_block_visibility = 0
}
$entrySummaries = New-Object System.Collections.ArrayList
$inferredTableColumnCount = 0
$status = "failed"
$failureMessage = ""

Ensure-PathParent -Path $resolvedOutputMapping
Ensure-PathParent -Path $resolvedSummaryJson

try {
    Write-Step "Loading render plan from $resolvedInputPlan"
    $planObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedInputPlan | ConvertFrom-Json

    foreach ($category in @("bookmark_text", "bookmark_paragraphs", "bookmark_table_rows", "bookmark_block_visibility")) {
        $draftEntries = New-Object 'System.Collections.Generic.List[object]'
        $sourceEntries = @(Get-ObjectArrayProperty -Object $planObject -Name $category)

        for ($entryIndex = 0; $entryIndex -lt $sourceEntries.Count; $entryIndex++) {
            $sourceEntry = $sourceEntries[$entryIndex]
            $columns = $null
            if ($category -eq "bookmark_table_rows") {
                $columns = @(Get-InferredTableColumns -Entry $sourceEntry)
            }

            $draftEntry = New-MappingEntry `
                -SourceEntry $sourceEntry `
                -Category $category `
                -SourceRoot $SourceRoot `
                -Columns $columns
            $draftEntries.Add([pscustomobject]$draftEntry) | Out-Null
            $entrySummaries.Add((New-EntrySummary `
                    -Category $category `
                    -Index $entryIndex `
                    -MappingEntry ([pscustomobject]$draftEntry) `
                    -Columns $columns)) | Out-Null

            if ($null -ne $columns) {
                $inferredTableColumnCount += @($columns).Count
            }
        }

        $mappingObject[$category] = @($draftEntries.ToArray())
        $mappingCounts[$category] = @($draftEntries.ToArray()).Count
    }

    $entryCount = (
        [int]$mappingCounts.bookmark_text +
        [int]$mappingCounts.bookmark_paragraphs +
        [int]$mappingCounts.bookmark_table_rows +
        [int]$mappingCounts.bookmark_block_visibility
    )
    if ($entryCount -eq 0) {
        throw "Render plan does not contain any supported mapping entries."
    }

    Write-Step "Writing render-data mapping draft to $resolvedOutputMapping"
    ($mappingObject | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedOutputMapping -Encoding UTF8
    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    throw
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-DraftSummary `
            -Status $status `
            -InputPlan $resolvedInputPlan `
            -OutputMapping $resolvedOutputMapping `
            -SummaryJson $resolvedSummaryJson `
            -SourceRoot $SourceRoot `
            -MappingCounts $mappingCounts `
            -InferredTableColumnCount $inferredTableColumnCount `
            -Entries $entrySummaries `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }
}

Write-Step "Render-data mapping draft written to $resolvedOutputMapping"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
