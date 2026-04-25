<#
.SYNOPSIS
Converts business JSON data plus a mapping file into a render-plan patch.

.DESCRIPTION
Reads one business-data JSON document and one mapping file, resolves mapping
`source` paths, and writes a render-plan patch document that can be fed into
`patch_template_render_plan.ps1` or `render_template_document_from_patch.ps1`.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$DataPath,
    [Parameter(Mandatory = $true)]
    [string]$MappingPath,
    [string]$OutputPatch = "",
    [string]$SummaryJson = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[convert-render-data-to-patch-plan] $Message"
}

function Resolve-OptionalDataPath {
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
        $Value -is [System.Collections.IDictionary] -or
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
        -not ($current -is [System.Collections.IDictionary]) -and
        -not ($current -is [System.Management.Automation.PSCustomObject])) {
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
    if ($Value -is [System.Collections.IDictionary] -or
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

function New-RenderPlanEntryBase {
    param(
        $MappingEntry,
        [string]$Category
    )

    $entry = [ordered]@{
        bookmark_name = Get-MappingBookmarkName -Item $MappingEntry -Category $Category
    }

    foreach ($propertyName in @("part", "index", "section", "kind")) {
        $value = Get-OptionalObjectPropertyObject -Object $MappingEntry -Name $propertyName
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            if ($propertyName -eq "index" -or $propertyName -eq "section") {
                $entry[$propertyName] = [int]$value
            } else {
                $entry[$propertyName] = [string]$value
            }
        }
    }

    return $entry
}

function Convert-ResolvedValueToStringArray {
    param(
        $Value,
        [string]$Label
    )

    $items = Convert-ToObjectArray -Value $Value -Label $Label
    $converted = New-Object 'System.Collections.Generic.List[string]'
    foreach ($item in $items) {
        $converted.Add((Convert-ScalarValueToString -Value $item -Label $Label)) | Out-Null
    }

    Write-Output -NoEnumerate @($converted.ToArray())
    return
}

function Convert-ResolvedValueToTableRows {
    param(
        $Value,
        [string[]]$Columns,
        [string]$Label
    )

    $rows = Convert-ToObjectArray -Value $Value -Label $Label -AllowEmpty
    $convertedRows = New-Object 'System.Collections.Generic.List[object]'

    for ($rowIndex = 0; $rowIndex -lt $rows.Count; $rowIndex++) {
        $row = $rows[$rowIndex]
        $cells = New-Object 'System.Collections.Generic.List[string]'

        if ($Columns.Count -gt 0) {
            foreach ($columnPath in $Columns) {
                $cellValue = Resolve-PathValue `
                    -RootObject $row `
                    -Path $columnPath `
                    -Label "$Label row[$rowIndex] column '$columnPath'"
                $cells.Add((Convert-ScalarValueToString `
                        -Value $cellValue `
                        -Label "$Label row[$rowIndex] column '$columnPath'")) | Out-Null
            }
        } else {
            $rowValues = Convert-ToObjectArray `
                -Value $row `
                -Label "$Label row[$rowIndex]"
            foreach ($cellValue in $rowValues) {
                $cells.Add((Convert-ScalarValueToString `
                        -Value $cellValue `
                        -Label "$Label row[$rowIndex]")) | Out-Null
            }
        }

        if ($cells.Count -eq 0) {
            throw "$Label row[$rowIndex] must contain at least one cell."
        }

        $convertedRows.Add(@($cells.ToArray())) | Out-Null
    }

    Write-Output -NoEnumerate @($convertedRows.ToArray())
    return
}

function Build-ConversionSummary {
    param(
        [string]$Status,
        [string]$DataPath,
        [string]$MappingPath,
        [string]$OutputPatch,
        [string]$SummaryJsonPath,
        [object]$Patch,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        data_path = $DataPath
        mapping_path = $MappingPath
        output_patch = $OutputPatch
        summary_json = $SummaryJsonPath
        patch_counts = [ordered]@{
            bookmark_text = @(Get-OptionalObjectArrayProperty -Object $Patch -Name "bookmark_text").Count
            bookmark_paragraphs = @(Get-OptionalObjectArrayProperty -Object $Patch -Name "bookmark_paragraphs").Count
            bookmark_table_rows = @(Get-OptionalObjectArrayProperty -Object $Patch -Name "bookmark_table_rows").Count
            bookmark_block_visibility = @(Get-OptionalObjectArrayProperty -Object $Patch -Name "bookmark_block_visibility").Count
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedDataPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $DataPath
$resolvedMappingPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $MappingPath
$resolvedOutputPatch = if ([string]::IsNullOrWhiteSpace($OutputPatch)) {
    $dataDirectory = [System.IO.Path]::GetDirectoryName($resolvedDataPath)
    $dataStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedDataPath)
    Join-Path $dataDirectory ($dataStem + ".render_patch.generated.json")
} else {
    Resolve-OptionalDataPath -RepoRoot $repoRoot -InputPath $OutputPatch
}
$resolvedSummaryJson = Resolve-OptionalDataPath -RepoRoot $repoRoot -InputPath $SummaryJson
$schemaPath = Join-Path $repoRoot "samples\template_render_plan.schema.json"
$outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedOutputPatch)
if ([string]::IsNullOrWhiteSpace($outputDirectory)) {
    $outputDirectory = $repoRoot
}
$schemaReference = Get-RelativePathString -BasePath $outputDirectory -TargetPath $schemaPath
if (-not ($schemaReference.StartsWith("./") -or $schemaReference.StartsWith("../"))) {
    $schemaReference = "./" + $schemaReference
}

Ensure-PathParent -Path $resolvedOutputPatch
Ensure-PathParent -Path $resolvedSummaryJson

$dataObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedDataPath | ConvertFrom-Json
$mappingObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedMappingPath | ConvertFrom-Json
$patchObject = [ordered]@{
    '$schema' = $schemaReference
    bookmark_text = @()
    bookmark_paragraphs = @()
    bookmark_table_rows = @()
    bookmark_block_visibility = @()
}
$status = "failed"
$failureMessage = ""

try {
    Write-Step "Resolving bookmark_text mappings"
    $textEntries = New-Object 'System.Collections.Generic.List[object]'
    foreach ($mappingEntry in (Get-OptionalObjectArrayProperty -Object $mappingObject -Name "bookmark_text")) {
        $entry = New-RenderPlanEntryBase -MappingEntry $mappingEntry -Category "bookmark_text"
        $sourcePath = Get-MappingSourcePath -Item $mappingEntry -Category "bookmark_text"
        $resolvedValue = Resolve-PathValue `
            -RootObject $dataObject `
            -Path $sourcePath `
            -Label "bookmark_text '$($entry.bookmark_name)'"
        $entry.text = Convert-ScalarValueToString `
            -Value $resolvedValue `
            -Label "bookmark_text '$($entry.bookmark_name)'"
        $textEntries.Add([pscustomobject]$entry) | Out-Null
    }

    Write-Step "Resolving bookmark_paragraphs mappings"
    $paragraphEntries = New-Object 'System.Collections.Generic.List[object]'
    foreach ($mappingEntry in (Get-OptionalObjectArrayProperty -Object $mappingObject -Name "bookmark_paragraphs")) {
        $entry = New-RenderPlanEntryBase -MappingEntry $mappingEntry -Category "bookmark_paragraphs"
        $sourcePath = Get-MappingSourcePath -Item $mappingEntry -Category "bookmark_paragraphs"
        $resolvedValue = Resolve-PathValue `
            -RootObject $dataObject `
            -Path $sourcePath `
            -Label "bookmark_paragraphs '$($entry.bookmark_name)'"
        $entry.paragraphs = [object[]](Convert-ResolvedValueToStringArray `
                -Value $resolvedValue `
                -Label "bookmark_paragraphs '$($entry.bookmark_name)'")
        $paragraphEntries.Add([pscustomobject]$entry) | Out-Null
    }

    Write-Step "Resolving bookmark_table_rows mappings"
    $tableRowEntries = New-Object 'System.Collections.Generic.List[object]'
    foreach ($mappingEntry in (Get-OptionalObjectArrayProperty -Object $mappingObject -Name "bookmark_table_rows")) {
        $entry = New-RenderPlanEntryBase -MappingEntry $mappingEntry -Category "bookmark_table_rows"
        $sourcePath = Get-MappingSourcePath -Item $mappingEntry -Category "bookmark_table_rows"
        $resolvedValue = Resolve-PathValue `
            -RootObject $dataObject `
            -Path $sourcePath `
            -Label "bookmark_table_rows '$($entry.bookmark_name)'"
        $columns = @(
            Get-OptionalObjectArrayProperty -Object $mappingEntry -Name "columns" |
                ForEach-Object {
                    $columnPath = [string]$_
                    if ([string]::IsNullOrWhiteSpace($columnPath)) {
                        throw "bookmark_table_rows '$($entry.bookmark_name)' contains an empty column path."
                    }
                    $columnPath
                }
        )
        $entry.rows = [object[]](Convert-ResolvedValueToTableRows `
                -Value $resolvedValue `
                -Columns $columns `
                -Label "bookmark_table_rows '$($entry.bookmark_name)'")
        $tableRowEntries.Add([pscustomobject]$entry) | Out-Null
    }

    Write-Step "Resolving bookmark_block_visibility mappings"
    $visibilityEntries = New-Object 'System.Collections.Generic.List[object]'
    foreach ($mappingEntry in (Get-OptionalObjectArrayProperty -Object $mappingObject -Name "bookmark_block_visibility")) {
        $entry = New-RenderPlanEntryBase -MappingEntry $mappingEntry -Category "bookmark_block_visibility"
        $sourcePath = Get-MappingSourcePath -Item $mappingEntry -Category "bookmark_block_visibility"
        $resolvedValue = Resolve-PathValue `
            -RootObject $dataObject `
            -Path $sourcePath `
            -Label "bookmark_block_visibility '$($entry.bookmark_name)'"
        $entry.visible = Convert-ValueToBoolean `
            -Value $resolvedValue `
            -Label "bookmark_block_visibility '$($entry.bookmark_name)'"
        $visibilityEntries.Add([pscustomobject]$entry) | Out-Null
    }

    $patchObject.bookmark_text = @($textEntries.ToArray())
    $patchObject.bookmark_paragraphs = @($paragraphEntries.ToArray())
    $patchObject.bookmark_table_rows = @($tableRowEntries.ToArray())
    $patchObject.bookmark_block_visibility = @($visibilityEntries.ToArray())

    Write-Step "Writing render patch to $resolvedOutputPatch"
    ($patchObject | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedOutputPatch -Encoding UTF8
    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    throw
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-ConversionSummary `
            -Status $status `
            -DataPath $resolvedDataPath `
            -MappingPath $resolvedMappingPath `
            -OutputPatch $resolvedOutputPatch `
            -SummaryJsonPath $resolvedSummaryJson `
            -Patch ([pscustomobject]$patchObject) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }
}

Write-Step "Render patch written to $resolvedOutputPatch"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
