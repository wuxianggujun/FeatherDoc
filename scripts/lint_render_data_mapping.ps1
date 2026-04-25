<#
.SYNOPSIS
Lint-checks render-data mapping files before template export or document render.

.DESCRIPTION
Validates mapping entry selectors, detects duplicate targets inside one mapping
category, and can optionally resolve business-data `source` paths to produce a
summary JSON with per-entry diagnostics.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$MappingPath,
    [string]$DataPath = "",
    [string]$SummaryJson = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[lint-render-data-mapping] $Message"
}

function Resolve-OptionalLintPath {
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
                -not (Test-IsJsonObject -Value $value)) {
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
        -not (Test-IsJsonObject -Value $value)) {
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

function Test-IsJsonObject {
    param($Value)

    if ($null -eq $Value) {
        return $false
    }

    if ($Value -is [System.Collections.IDictionary] -or
        $Value -is [System.Management.Automation.PSCustomObject]) {
        return $true
    }

    return $false
}

function Get-MappingCategoryEntries {
    param(
        $MappingObject,
        [string]$Category
    )

    $value = Get-OptionalObjectPropertyObject -Object $MappingObject -Name $Category
    if ($null -eq $value) {
        return @()
    }

    if ($value -is [string]) {
        throw "$Category must be an array."
    }

    if ((Test-IsJsonObject -Value $value) -or
        -not ($value -is [System.Collections.IEnumerable])) {
        return @($value)
    }

    return @($value | Where-Object { $true })
}

function Convert-ToObjectArray {
    param(
        $Value,
        [string]$Label,
        [switch]$AllowEmpty
    )

    if ($null -eq $Value) {
        throw "$Label resolved to null."
    }
    if ($Value -is [string] -or
        (Test-IsJsonObject -Value $Value) -or
        -not ($Value -is [System.Collections.IEnumerable])) {
        throw "$Label must resolve to an array."
    }

    $items = New-Object 'System.Collections.Generic.List[object]'
    foreach ($item in $Value) {
        $items.Add($item) | Out-Null
    }

    if (-not $AllowEmpty -and $items.Count -eq 0) {
        throw "$Label must not be empty."
    }

    Write-Output -NoEnumerate @($items.ToArray())
    return
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

function Resolve-PathValue {
    param(
        $RootObject,
        [string]$Path,
        [string]$Label
    )

    $current = $RootObject
    $steps = Convert-PathToTraversalSteps -Path $Path -Label $Label

    foreach ($step in $steps) {
        if ($step.kind -eq "property") {
            $current = Get-OptionalObjectPropertyObject -Object $current -Name ([string]$step.value)
            if ($null -eq $current) {
                throw "$Label could not resolve property '$($step.value)'."
            }
            continue
        }

        $items = Convert-ToObjectArray `
            -Value $current `
            -Label "$Label property traversal" `
            -AllowEmpty
        if ($step.value -lt 0 -or $step.value -ge $items.Count) {
            throw "$Label index [$($step.value)] is out of range."
        }

        $current = $items[$step.value]
    }

    if ($current -is [System.Collections.IEnumerable] -and
        -not ($current -is [string]) -and
        -not (Test-IsJsonObject -Value $current)) {
        Write-Output -NoEnumerate $current
        return
    }

    return $current
}

function Convert-ScalarValueToString {
    param(
        $Value,
        [string]$Label
    )

    if ($null -eq $Value) {
        throw "$Label resolved to null."
    }
    if ((Test-IsJsonObject -Value $Value) -or
        ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [string]))) {
        throw "$Label must resolve to a scalar value."
    }

    return [string]$Value
}

function Convert-ValueToBoolean {
    param(
        $Value,
        [string]$Label
    )

    if ($Value -is [bool]) {
        return [bool]$Value
    }
    if ($Value -is [byte] -or $Value -is [int16] -or $Value -is [int32] -or
        $Value -is [int64] -or $Value -is [uint16] -or $Value -is [uint32] -or
        $Value -is [uint64]) {
        if ([int64]$Value -eq 0) {
            return $false
        }
        if ([int64]$Value -eq 1) {
            return $true
        }
    }

    $stringValue = Convert-ScalarValueToString -Value $Value -Label $Label
    if ($stringValue -ieq "true") {
        return $true
    }
    if ($stringValue -ieq "false") {
        return $false
    }

    throw "$Label must resolve to a boolean-compatible value."
}

function Convert-ToNonNegativeInt {
    param(
        [string]$Text,
        [string]$Label
    )

    $value = 0
    if (-not [int]::TryParse($Text, [ref]$value) -or $value -lt 0) {
        throw "$Label must be a non-negative integer."
    }

    return $value
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

function Convert-ToNormalizedSelection {
    param(
        $Item,
        [string]$Category
    )

    $supportedKinds = @("default", "first", "even")
    $part = Get-OptionalObjectPropertyValue -Object $Item -Name "part"
    if ([string]::IsNullOrWhiteSpace($part)) {
        $part = "body"
    }

    $indexValue = Get-OptionalObjectPropertyValue -Object $Item -Name "index"
    $sectionValue = Get-OptionalObjectPropertyValue -Object $Item -Name "section"
    $kindValue = Get-OptionalObjectPropertyValue -Object $Item -Name "kind"

    switch ($part) {
        "body" {
            if (-not [string]::IsNullOrWhiteSpace($indexValue) -or
                -not [string]::IsNullOrWhiteSpace($sectionValue) -or
                -not [string]::IsNullOrWhiteSpace($kindValue)) {
                throw "$Category entries targeting 'body' must not set 'index', 'section', or 'kind'."
            }

            return [ordered]@{
                part = "body"
            }
        }
        "header" {
            if ([string]::IsNullOrWhiteSpace($indexValue)) {
                throw "$Category entries targeting 'header' must provide 'index'."
            }
            if (-not [string]::IsNullOrWhiteSpace($sectionValue) -or
                -not [string]::IsNullOrWhiteSpace($kindValue)) {
                throw "$Category entries targeting 'header' must not set 'section' or 'kind'."
            }

            return [ordered]@{
                part = "header"
                index = Convert-ToNonNegativeInt -Text $indexValue -Label "$Category index"
            }
        }
        "footer" {
            if ([string]::IsNullOrWhiteSpace($indexValue)) {
                throw "$Category entries targeting 'footer' must provide 'index'."
            }
            if (-not [string]::IsNullOrWhiteSpace($sectionValue) -or
                -not [string]::IsNullOrWhiteSpace($kindValue)) {
                throw "$Category entries targeting 'footer' must not set 'section' or 'kind'."
            }

            return [ordered]@{
                part = "footer"
                index = Convert-ToNonNegativeInt -Text $indexValue -Label "$Category index"
            }
        }
        "section-header" {
            if ([string]::IsNullOrWhiteSpace($sectionValue)) {
                throw "$Category entries targeting 'section-header' must provide 'section'."
            }
            if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
                throw "$Category entries targeting 'section-header' must not set 'index'."
            }
            if ([string]::IsNullOrWhiteSpace($kindValue)) {
                $kindValue = "default"
            }
            if ($supportedKinds -notcontains $kindValue) {
                throw "$Category entries targeting 'section-header' use unsupported kind '$kindValue'."
            }

            return [ordered]@{
                part = "section-header"
                section = Convert-ToNonNegativeInt -Text $sectionValue -Label "$Category section"
                kind = $kindValue
            }
        }
        "section-footer" {
            if ([string]::IsNullOrWhiteSpace($sectionValue)) {
                throw "$Category entries targeting 'section-footer' must provide 'section'."
            }
            if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
                throw "$Category entries targeting 'section-footer' must not set 'index'."
            }
            if ([string]::IsNullOrWhiteSpace($kindValue)) {
                $kindValue = "default"
            }
            if ($supportedKinds -notcontains $kindValue) {
                throw "$Category entries targeting 'section-footer' use unsupported kind '$kindValue'."
            }

            return [ordered]@{
                part = "section-footer"
                section = Convert-ToNonNegativeInt -Text $sectionValue -Label "$Category section"
                kind = $kindValue
            }
        }
        default {
            throw "$Category entry uses unsupported part '$part'."
        }
    }
}

function Convert-SelectionToIdentityKey {
    param(
        $Selection,
        [string]$BookmarkName
    )

    $part = Get-OptionalObjectPropertyValue -Object $Selection -Name "part"
    $indexValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "index"
    $sectionValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "section"
    $kindValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "kind"

    return "bookmark=$BookmarkName|part=$part|index=$indexValue|section=$sectionValue|kind=$kindValue"
}

function Convert-SelectionToSummaryObject {
    param($Selection)

    $summary = [ordered]@{
        part = [string](Get-OptionalObjectPropertyValue -Object $Selection -Name "part")
    }

    $indexValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "index"
    if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
        $summary.index = [int]$indexValue
    }

    $sectionValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "section"
    if (-not [string]::IsNullOrWhiteSpace($sectionValue)) {
        $summary.section = [int]$sectionValue
    }

    $kindValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "kind"
    if (-not [string]::IsNullOrWhiteSpace($kindValue)) {
        $summary.kind = $kindValue
    }

    return $summary
}

function Get-EntryColumns {
    param(
        $Item,
        [string]$Category,
        [string]$BookmarkName
    )

    if ($Category -ne "bookmark_table_rows") {
        return @()
    }

    $columnsValue = Get-OptionalObjectPropertyObject -Object $Item -Name "columns"
    if ($null -eq $columnsValue) {
        return @()
    }

    if ($columnsValue -is [string]) {
        return @([string]$columnsValue)
    }

    if ((Test-IsJsonObject -Value $columnsValue) -or
        -not ($columnsValue -is [System.Collections.IEnumerable])) {
        throw "bookmark_table_rows '$BookmarkName' columns must be an array."
    }

    $columns = New-Object 'System.Collections.Generic.List[string]'
    foreach ($columnValue in $columnsValue) {
        $columnPath = [string]$columnValue
        if ([string]::IsNullOrWhiteSpace($columnPath)) {
            throw "bookmark_table_rows '$BookmarkName' contains an empty column path."
        }

        $columns.Add($columnPath) | Out-Null
    }

    if ($columns.Count -eq 0) {
        throw "bookmark_table_rows '$BookmarkName' columns must not be empty when provided."
    }

    return @($columns.ToArray())
}

function Get-ResolvedValueType {
    param($Value)

    if ($Value -is [bool]) {
        return "boolean"
    }
    if (Test-IsJsonObject -Value $Value) {
        return "object"
    }
    if ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [string])) {
        return "array"
    }

    return "scalar"
}

function New-IssueRecord {
    param(
        [string]$Scope,
        [string]$Message
    )

    return [ordered]@{
        scope = $Scope
        message = $Message
    }
}

function Add-EntryIssue {
    param(
        $Report,
        [string]$Scope,
        [string]$Message
    )

    $Report['issues'].Add((New-IssueRecord -Scope $Scope -Message $Message)) | Out-Null
}

function Get-EntryValidationLabel {
    param($Report)

    $category = [string]$Report['category']
    $bookmarkName = [string]$Report['bookmark_name']
    if (-not [string]::IsNullOrWhiteSpace($bookmarkName)) {
        return "$category '$bookmarkName'"
    }

    return "$category[$([int]$Report['entry_index'])]"
}

function Validate-EntryDataContract {
    param(
        $Report,
        $DataObject,
        [string]$SourcePath,
        [string[]]$Columns
    )

    $label = Get-EntryValidationLabel -Report $Report
    $resolvedValue = Resolve-PathValue `
        -RootObject $DataObject `
        -Path $SourcePath `
        -Label $label
    $Report['resolved_type'] = Get-ResolvedValueType -Value $resolvedValue

    switch ([string]$Report['category']) {
        "bookmark_text" {
            [void](Convert-ScalarValueToString -Value $resolvedValue -Label $label)
        }
        "bookmark_paragraphs" {
            foreach ($item in (Convert-ToObjectArray -Value $resolvedValue -Label $label)) {
                [void](Convert-ScalarValueToString -Value $item -Label $label)
            }
        }
        "bookmark_block_visibility" {
            [void](Convert-ValueToBoolean -Value $resolvedValue -Label $label)
        }
        "bookmark_table_rows" {
            $rows = Convert-ToObjectArray -Value $resolvedValue -Label $label -AllowEmpty

            if ($Columns.Count -gt 0) {
                foreach ($columnReport in $Report['column_reports']) {
                    $columnReport['status'] = "completed"
                }

                for ($rowIndex = 0; $rowIndex -lt $rows.Count; $rowIndex++) {
                    $row = $rows[$rowIndex]
                    foreach ($columnReport in $Report['column_reports']) {
                        $columnLabel = "$label row[$rowIndex] column '$([string]$columnReport['path'])'"
                        try {
                            $cellValue = Resolve-PathValue `
                                -RootObject $row `
                                -Path ([string]$columnReport['path']) `
                                -Label $columnLabel
                            [void](Convert-ScalarValueToString -Value $cellValue -Label $columnLabel)
                        } catch {
                            $columnReport['status'] = "failed"
                            $columnReport['invalid_rows'].Add($rowIndex) | Out-Null
                        }
                    }
                }

                $failedColumns = @($Report['column_reports'] | Where-Object { $_['status'] -eq "failed" })
                if ($failedColumns.Count -gt 0) {
                    $columnMessages = $failedColumns | ForEach-Object {
                        $invalidRows = @($_['invalid_rows'])
                        if ($invalidRows.Count -gt 0) {
                            "column '$([string]$_['path'])' failed for rows [$($invalidRows -join ', ')]"
                        } else {
                            "column '$([string]$_['path'])' failed"
                        }
                    }
                    throw "$label has invalid table column mappings: $($columnMessages -join '; ')."
                }

                break
            }

            for ($rowIndex = 0; $rowIndex -lt $rows.Count; $rowIndex++) {
                foreach ($cellValue in (Convert-ToObjectArray -Value $rows[$rowIndex] -Label "$label row[$rowIndex]")) {
                    [void](Convert-ScalarValueToString -Value $cellValue -Label "$label row[$rowIndex]")
                }
            }
        }
    }
}

function Build-LintSummary {
    param(
        [string]$Status,
        [string]$MappingPath,
        [string]$DataPath,
        [string]$SummaryJsonPath,
        [bool]$DataValidationPerformed,
        $MappingCounts,
        [int]$ResolvedSourceCount,
        [int]$InvalidSourceCount,
        [int]$SkippedSourceCount,
        [int]$DuplicateTargetCount,
        [int]$IssueCount,
        $GeneralIssues,
        $Entries,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        mapping_path = $MappingPath
        data_path = $DataPath
        summary_json = $SummaryJsonPath
        data_validation_performed = $DataValidationPerformed
        mapping_counts = $MappingCounts
        mapping_entry_count = (
            [int]$MappingCounts.bookmark_text +
            [int]$MappingCounts.bookmark_paragraphs +
            [int]$MappingCounts.bookmark_table_rows +
            [int]$MappingCounts.bookmark_block_visibility
        )
        resolved_source_count = $ResolvedSourceCount
        invalid_source_count = $InvalidSourceCount
        skipped_source_count = $SkippedSourceCount
        duplicate_target_count = $DuplicateTargetCount
        issue_count = $IssueCount
        general_issues = @($GeneralIssues)
        entries = @($Entries)
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedMappingPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $MappingPath
$resolvedDataPath = Resolve-OptionalLintPath -RepoRoot $repoRoot -InputPath $DataPath
$resolvedSummaryJson = Resolve-OptionalLintPath -RepoRoot $repoRoot -InputPath $SummaryJson
$hasData = -not [string]::IsNullOrWhiteSpace($resolvedDataPath)
$supportedCategories = @(
    "bookmark_text",
    "bookmark_paragraphs",
    "bookmark_table_rows",
    "bookmark_block_visibility"
)
$mappingCounts = [ordered]@{
    bookmark_text = 0
    bookmark_paragraphs = 0
    bookmark_table_rows = 0
    bookmark_block_visibility = 0
}
$entries = New-Object System.Collections.ArrayList
$generalIssues = New-Object System.Collections.ArrayList
$resolvedSourceCount = 0
$invalidSourceCount = 0
$skippedSourceCount = 0
$duplicateTargetCount = 0
$status = "failed"
$failureMessage = ""

Ensure-PathParent -Path $resolvedSummaryJson

try {
    Write-Step "Loading mapping JSON from $resolvedMappingPath"
    $mappingObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedMappingPath | ConvertFrom-Json
    if (-not (Test-IsJsonObject -Value $mappingObject)) {
        throw "Mapping JSON root must be an object."
    }

    $dataObject = $null
    if ($hasData) {
        Write-Step "Loading business data JSON from $resolvedDataPath"
        $dataObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedDataPath | ConvertFrom-Json
    }

    foreach ($propertyName in $mappingObject.PSObject.Properties.Name) {
        if ($propertyName -ne '$schema' -and $supportedCategories -notcontains $propertyName) {
            $generalIssues.Add((New-IssueRecord `
                    -Scope "mapping" `
                    -Message "Unsupported top-level mapping property '$propertyName'.")) | Out-Null
        }
    }

    $categoryEntries = @{}
    foreach ($category in $supportedCategories) {
        try {
            $items = @(Get-MappingCategoryEntries -MappingObject $mappingObject -Category $category)
        } catch {
            $items = @()
            $generalIssues.Add((New-IssueRecord `
                    -Scope "mapping" `
                    -Message $_.Exception.Message)) | Out-Null
        }

        $categoryEntries[$category] = $items
        $mappingCounts[$category] = $items.Count
    }

    $totalMappingEntries = 0
    foreach ($category in $supportedCategories) {
        $totalMappingEntries += [int]$mappingCounts[$category]
    }
    if ($totalMappingEntries -eq 0) {
        $generalIssues.Add((New-IssueRecord `
                -Scope "mapping" `
                -Message "Mapping file must contain at least one supported category.")) | Out-Null
    }

    $targetGroups = @{}
    foreach ($category in $supportedCategories) {
        $items = $categoryEntries[$category]
        for ($entryIndex = 0; $entryIndex -lt $items.Count; $entryIndex++) {
            $mappingEntry = $items[$entryIndex]
            $report = [ordered]@{
                id = "$category[$entryIndex]"
                category = $category
                entry_index = $entryIndex
                bookmark_name = ""
                source = ""
                selector = $null
                target_key = ""
                resolved_type = ""
                duplicate_target = $false
                static_status = "completed"
                data_status = if ($hasData) { "pending" } else { "skipped" }
                status = "pending"
                issues = (New-Object System.Collections.ArrayList)
                column_reports = (New-Object System.Collections.ArrayList)
            }

            $bookmarkName = ""
            $sourcePath = ""
            $selection = $null
            $columns = @()
            $skipDataValidation = $false

            try {
                $bookmarkName = Get-MappingBookmarkName -Item $mappingEntry -Category $category
                $report['bookmark_name'] = $bookmarkName
            } catch {
                Add-EntryIssue -Report $report -Scope "static" -Message $_.Exception.Message
            }

            try {
                $sourcePath = Get-MappingSourcePath -Item $mappingEntry -Category $category
                $report['source'] = $sourcePath
            } catch {
                Add-EntryIssue -Report $report -Scope "static" -Message $_.Exception.Message
                $skipDataValidation = $true
            }

            try {
                $selection = Convert-ToNormalizedSelection -Item $mappingEntry -Category $category
                $report['selector'] = Convert-SelectionToSummaryObject -Selection $selection
                if (-not [string]::IsNullOrWhiteSpace($bookmarkName)) {
                    $report['target_key'] = Convert-SelectionToIdentityKey `
                        -Selection $selection `
                        -BookmarkName $bookmarkName
                }
            } catch {
                Add-EntryIssue -Report $report -Scope "static" -Message $_.Exception.Message
            }

            try {
                $columns = @(Get-EntryColumns `
                    -Item $mappingEntry `
                    -Category $category `
                    -BookmarkName $bookmarkName)
                foreach ($columnPath in $columns) {
                    $report['column_reports'].Add([ordered]@{
                            path = $columnPath
                            status = if ($hasData) { "pending" } else { "skipped" }
                            invalid_rows = (New-Object System.Collections.ArrayList)
                        }) | Out-Null
                }
            } catch {
                Add-EntryIssue -Report $report -Scope "static" -Message $_.Exception.Message
                $skipDataValidation = $true
            }

            if (-not [string]::IsNullOrWhiteSpace([string]$report['target_key'])) {
                $groupKey = "$category|$([string]$report['target_key'])"
                if (-not $targetGroups.ContainsKey($groupKey)) {
                    $targetGroups[$groupKey] = New-Object System.Collections.ArrayList
                }
                $targetGroups[$groupKey].Add($report) | Out-Null
            }

            if ($hasData) {
                if ($skipDataValidation -or [string]::IsNullOrWhiteSpace($sourcePath)) {
                    $report['data_status'] = "skipped"
                } else {
                    try {
                        Validate-EntryDataContract `
                            -Report $report `
                            -DataObject $dataObject `
                            -SourcePath $sourcePath `
                            -Columns $columns
                        $report['data_status'] = "completed"
                    } catch {
                        Add-EntryIssue -Report $report -Scope "data" -Message $_.Exception.Message
                        $report['data_status'] = "failed"
                    }
                }
            }

            $entries.Add($report) | Out-Null
        }
    }

    foreach ($groupKey in $targetGroups.Keys) {
        $groupReports = @($targetGroups[$groupKey])
        if ($groupReports.Count -le 1) {
            continue
        }

        $duplicateTargetCount++
        foreach ($report in $groupReports) {
            $report['duplicate_target'] = $true
            Add-EntryIssue `
                -Report $report `
                -Scope "static" `
                -Message "Duplicate target within category '$([string]$report['category'])': $([string]$report['target_key'])."
        }
    }

    $issueCount = $generalIssues.Count
    foreach ($report in $entries) {
        $staticIssueCount = @($report['issues'] | Where-Object { $_['scope'] -eq "static" }).Count
        $dataIssueCount = @($report['issues'] | Where-Object { $_['scope'] -eq "data" }).Count

        $report['static_status'] = if ($staticIssueCount -gt 0) { "failed" } else { "completed" }
        if (-not $hasData) {
            $report['data_status'] = "skipped"
        } elseif ($dataIssueCount -gt 0) {
            $report['data_status'] = "failed"
        } elseif ([string]$report['data_status'] -eq "completed") {
            $report['data_status'] = "completed"
        } else {
            $report['data_status'] = "skipped"
        }

        if ([string]$report['data_status'] -eq "completed") {
            $resolvedSourceCount++
        } elseif ([string]$report['data_status'] -eq "failed") {
            $invalidSourceCount++
        } elseif ($hasData) {
            $skippedSourceCount++
        }

        $entryIssueCount = @($report['issues']).Count
        $report['status'] = if ($entryIssueCount -gt 0) { "failed" } else { "completed" }
        $issueCount += $entryIssueCount
    }

    if ($issueCount -gt 0) {
        $status = "failed"
        $failureMessage = "lint_render_data_mapping.ps1 found $issueCount issue(s)."
    } else {
        $status = "completed"
    }
} catch {
    if ([string]::IsNullOrWhiteSpace($failureMessage)) {
        $failureMessage = $_.Exception.Message
    }
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $finalIssueCount = $generalIssues.Count
        foreach ($report in $entries) {
            $finalIssueCount += @($report['issues']).Count
        }

        $summary = Build-LintSummary `
            -Status $status `
            -MappingPath $resolvedMappingPath `
            -DataPath $resolvedDataPath `
            -SummaryJsonPath $resolvedSummaryJson `
            -DataValidationPerformed $hasData `
            -MappingCounts $mappingCounts `
            -ResolvedSourceCount $resolvedSourceCount `
            -InvalidSourceCount $invalidSourceCount `
            -SkippedSourceCount $skippedSourceCount `
            -DuplicateTargetCount $duplicateTargetCount `
            -IssueCount $finalIssueCount `
            -GeneralIssues $generalIssues `
            -Entries $entries `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }
}

if ($status -ne "completed") {
    throw $failureMessage
}

Write-Step "Render-data mapping lint completed for $resolvedMappingPath"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
