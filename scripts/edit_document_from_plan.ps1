<#
.SYNOPSIS
Edits an existing DOCX by applying a JSON edit plan.

.DESCRIPTION
Runs a user-facing document-edit workflow:
1. read an edit plan JSON,
2. apply supported operations through featherdoc_cli or focused OOXML edits,
3. write the edited DOCX and a summary JSON.

The supported operations intentionally reuse stable CLI primitives where
possible. Focused OOXML edits are used for document-edit features that Word
stores inside paragraphs/runs, such as table-cell horizontal alignment and
visible text style overrides.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$EditPlan,
    [Parameter(Mandatory = $true)]
    [string]$OutputDocx,
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[edit-document-from-plan] $Message"
}

function Resolve-OptionalEditPath {
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
            Write-Output -NoEnumerate $Object[$Name]
            return
        }

        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    Write-Output -NoEnumerate $property.Value
}

function Test-ObjectPropertyExists {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $false
    }

    if ($Object -is [System.Collections.IDictionary]) {
        return $Object.Contains($Name)
    }

    return $null -ne $Object.PSObject.Properties[$Name]
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

function Get-RequiredObjectPropertyValue {
    param(
        $Object,
        [string]$Name,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyValue -Object $Object -Name $Name
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "$Label must provide '$Name'."
    }

    return $value
}

function Get-EditPlanOperations {
    param($Plan)

    if ($Plan -is [System.Collections.IEnumerable] -and
        -not ($Plan -is [string]) -and
        -not ($Plan -is [System.Collections.IDictionary]) -and
        -not ($Plan -is [System.Management.Automation.PSCustomObject])) {
        return @($Plan | Where-Object { $null -ne $_ })
    }

    $operations = Get-OptionalObjectPropertyObject -Object $Plan -Name "operations"
    if ($null -eq $operations) {
        throw "Edit plan must be an array or provide an 'operations' array."
    }
    if ($operations -is [string]) {
        throw "Edit plan 'operations' must be an array."
    }

    return @($operations | Where-Object { $null -ne $_ })
}

function Get-StringArrayProperty {
    param(
        $Object,
        [string]$Name,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        throw "$Label must provide '$Name'."
    }
    if ($value -is [string]) {
        return @([string]$value)
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | ForEach-Object { [string]$_ })
    }

    throw "$Label '$Name' must be an array of strings."
}

function Get-TableRowsProperty {
    param(
        $Object,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name "rows"
    if ($null -eq $value) {
        throw "$Label must provide 'rows'."
    }
    if ($value -is [string]) {
        throw "$Label 'rows' must be an array of row arrays."
    }

    $rows = New-Object 'System.Collections.Generic.List[object]'
    foreach ($row in @($value)) {
        if ($row -is [string]) {
            $rows.Add(@([string]$row)) | Out-Null
            continue
        }
        if ($row -is [System.Collections.IEnumerable]) {
            $rows.Add(@($row | ForEach-Object { [string]$_ })) | Out-Null
            continue
        }
        throw "$Label rows must contain arrays of cell texts."
    }

    return ,@($rows.ToArray())
}

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

function Add-TemplatePartSelectionArguments {
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

function Add-AppendFieldPreserveFormattingArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $hasNoPreserveFormatting = Test-ObjectPropertyExists `
        -Object $Operation `
        -Name "no_preserve_formatting"
    $hasPreserveFormatting = Test-ObjectPropertyExists `
        -Object $Operation `
        -Name "preserve_formatting"

    if (-not $hasNoPreserveFormatting -and -not $hasPreserveFormatting) {
        return
    }

    $noPreserveFormatting = $false
    if ($hasNoPreserveFormatting) {
        $noPreserveFormatting = Get-OptionalBooleanPropertyValue `
            -Object $Operation `
            -Name "no_preserve_formatting" `
            -DefaultValue $false
    }

    if ($hasPreserveFormatting) {
        $preserveFormatting = Get-OptionalBooleanPropertyValue `
            -Object $Operation `
            -Name "preserve_formatting" `
            -DefaultValue $true
        if ($hasNoPreserveFormatting -and ($noPreserveFormatting -eq $preserveFormatting)) {
            throw "$Label cannot combine contradictory 'no_preserve_formatting' and 'preserve_formatting' values."
        }
        if (-not $preserveFormatting) {
            $noPreserveFormatting = $true
        }
    }

    if ($noPreserveFormatting) {
        $Arguments.Add("--no-preserve-formatting") | Out-Null
    }
}

function Add-AppendFieldArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [switch]$AllowResultText,
        [switch]$AllowDateFormat,
        [switch]$AllowPreserveFormatting,
        [switch]$AllowTableOfContentsOptions,
        [switch]$AllowFieldState
    )

    Add-AppendFieldPartSelectionArguments `
        -Arguments $Arguments `
        -Operation $Operation

    if ($AllowDateFormat) {
        $null = Add-OptionalCliValueArgument `
            -Arguments $Arguments `
            -Operation $Operation `
            -Names @("format", "date_format") `
            -Flag "--format"
    }

    if ($AllowTableOfContentsOptions) {
        $null = Add-OptionalCliValueArgument `
            -Arguments $Arguments `
            -Operation $Operation `
            -Names @("min_outline_level", "toc_min_outline_level") `
            -Flag "--min-outline-level"
        $null = Add-OptionalCliValueArgument `
            -Arguments $Arguments `
            -Operation $Operation `
            -Names @("max_outline_level", "toc_max_outline_level") `
            -Flag "--max-outline-level"

        $hasNoHyperlinks = Test-ObjectPropertyExists -Object $Operation -Name "no_hyperlinks"
        $hasHyperlinks = Test-ObjectPropertyExists -Object $Operation -Name "hyperlinks"
        $noHyperlinks = $false
        if ($hasNoHyperlinks) {
            $noHyperlinks = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "no_hyperlinks" `
                -DefaultValue $false
        }
        if ($hasHyperlinks) {
            $hyperlinks = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "hyperlinks" `
                -DefaultValue $true
            if ($hasNoHyperlinks -and ($noHyperlinks -eq $hyperlinks)) {
                throw "$Label cannot combine contradictory 'no_hyperlinks' and 'hyperlinks' values."
            }
            if (-not $hyperlinks) {
                $noHyperlinks = $true
            }
        }
        if ($noHyperlinks) {
            $Arguments.Add("--no-hyperlinks") | Out-Null
        }

        $showWebPageNumbers = Get-OptionalBooleanPropertyValue `
            -Object $Operation `
            -Name "show_page_numbers_in_web_layout" `
            -DefaultValue $false
        if ($showWebPageNumbers) {
            $Arguments.Add("--show-page-numbers-in-web-layout") | Out-Null
        }
        if (Test-ObjectPropertyExists -Object $Operation -Name "hide_page_numbers_in_web_layout") {
            $hideWebPageNumbers = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "hide_page_numbers_in_web_layout" `
                -DefaultValue $true
            if (-not $hideWebPageNumbers) {
                $Arguments.Add("--show-page-numbers-in-web-layout") | Out-Null
            }
        }

        $noOutlineLevels = Get-OptionalBooleanPropertyValue `
            -Object $Operation `
            -Name "no_outline_levels" `
            -DefaultValue $false
        if ($noOutlineLevels) {
            $Arguments.Add("--no-outline-levels") | Out-Null
        }
        if (Test-ObjectPropertyExists -Object $Operation -Name "use_outline_levels") {
            $useOutlineLevels = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "use_outline_levels" `
                -DefaultValue $true
            if (-not $useOutlineLevels) {
                $Arguments.Add("--no-outline-levels") | Out-Null
            }
        }
    }

    if ($AllowResultText) {
        Add-AppendFieldResultTextArgument `
            -Arguments $Arguments `
            -Operation $Operation
    }

    if ($AllowFieldState) {
        Add-AppendFieldStateArguments `
            -Arguments $Arguments `
            -Operation $Operation
    }

    if ($AllowPreserveFormatting) {
        Add-AppendFieldPreserveFormattingArgument `
            -Arguments $Arguments `
            -Operation $Operation `
            -Label $Label
    }
}

function Add-AppendFieldPartSelectionArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation
    )

    $part = Get-OptionalObjectPropertyValue -Object $Operation -Name "part"
    if ([string]::IsNullOrWhiteSpace($part)) {
        $part = "body"
    }

    $Arguments.Add("--part") | Out-Null
    $Arguments.Add($part) | Out-Null

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

function Add-AppendFieldResultTextArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string[]]$Names = @("result_text", "text")
    )

    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names $Names `
        -Flag "--result-text"
}

function Add-AppendFieldStateArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation
    )

    $null = Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("dirty") `
        -Flag "--dirty"
    $null = Add-OptionalCliFlagArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("locked") `
        -Flag "--locked"
}

function Get-ParagraphIndexValue {
    param(
        $Operation,
        [string]$Label
    )

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("paragraph_index", "paragraph") `
        -Label $Label
}

function Get-RunIndexValue {
    param(
        $Operation,
        [string]$Label
    )

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("run_index", "run") `
        -Label $Label
}

function Get-SectionIndexValue {
    param(
        $Operation,
        [string]$Label
    )

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("section_index", "section") `
        -Label $Label
}

function Get-StyleIdValue {
    param(
        $Operation,
        [string]$Label
    )

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("style_id", "style") `
        -Label $Label
}

function Add-EnsureStyleCatalogArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $styleName = Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("name", "style_name", "display_name") `
        -Label $Label
    $Arguments.Add("--name") | Out-Null
    $Arguments.Add($styleName) | Out-Null

    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("based_on", "based_on_style", "based_on_style_id") `
        -Flag "--based-on"
    $null = Add-OptionalCliBooleanValueArgumentByNames `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("custom", "is_custom") `
        -Flag "--custom"
    $null = Add-OptionalCliBooleanValueArgumentByNames `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("semi_hidden", "is_semi_hidden") `
        -Flag "--semi-hidden"
    $null = Add-OptionalCliBooleanValueArgumentByNames `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("unhide_when_used", "is_unhide_when_used") `
        -Flag "--unhide-when-used"
    $null = Add-OptionalCliBooleanValueArgumentByNames `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("quick_format", "is_quick_format") `
        -Flag "--quick-format"
}

function Add-EnsureStyleRunArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation
    )

    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("run_font_family", "font_family", "font") `
        -Flag "--run-font-family"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("run_east_asia_font_family", "east_asia_font_family", "east_asia_font") `
        -Flag "--run-east-asia-font-family"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("run_language", "language", "lang") `
        -Flag "--run-language"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("run_east_asia_language", "east_asia_language", "east_asia_lang") `
        -Flag "--run-east-asia-language"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("run_bidi_language", "bidi_language", "bidi_lang") `
        -Flag "--run-bidi-language"
    $null = Add-OptionalCliBooleanValueArgumentByNames `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("run_rtl", "rtl") `
        -Flag "--run-rtl"
}

function Add-EnsureParagraphStyleArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    Add-EnsureStyleCatalogArguments -Arguments $Arguments -Operation $Operation -Label $Label
    Add-EnsureStyleRunArguments -Arguments $Arguments -Operation $Operation
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("next_style", "next_style_id", "next") `
        -Flag "--next-style"
    $null = Add-OptionalCliBooleanValueArgumentByNames `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("paragraph_bidi") `
        -Flag "--paragraph-bidi"
    $null = Add-OptionalCliValueArgument `
        -Arguments $Arguments `
        -Operation $Operation `
        -Names @("outline_level", "level") `
        -Flag "--outline-level"
}

function Add-EnsureCharacterStyleArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    Add-EnsureStyleCatalogArguments -Arguments $Arguments -Operation $Operation -Label $Label
    Add-EnsureStyleRunArguments -Arguments $Arguments -Operation $Operation
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

function Add-TemplateAppendRowOptions {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $cellCount = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("cell_count", "column_count", "columns")
    if (-not [string]::IsNullOrWhiteSpace($cellCount)) {
        $cellCountValue = [int]$cellCount
        if ($cellCountValue -le 0) {
            throw "$Label cell_count must be greater than zero."
        }
        $Arguments.Add("--cell-count") | Out-Null
        $Arguments.Add([string]$cellCountValue) | Out-Null
    }
}

function New-EditOperationTextFile {
    param(
        [string]$TemporaryRoot,
        [int]$OperationIndex,
        [int]$ValueIndex,
        [string]$Text
    )

    $textDirectory = Join-Path $TemporaryRoot "text-values"
    New-Item -ItemType Directory -Path $textDirectory -Force | Out-Null
    $textPath = Join-Path $textDirectory ("operation-{0}-value-{1}.txt" -f $OperationIndex, $ValueIndex)
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($textPath, $Text, $utf8NoBom)

    return $textPath
}

function New-EditOperationJsonFile {
    param(
        [string]$TemporaryRoot,
        [int]$OperationIndex,
        [int]$ValueIndex,
        $Value
    )

    $jsonDirectory = Join-Path $TemporaryRoot "json-values"
    New-Item -ItemType Directory -Path $jsonDirectory -Force | Out-Null
    $jsonPath = Join-Path $jsonDirectory ("operation-{0}-value-{1}.json" -f $OperationIndex, $ValueIndex)
    $jsonText = $Value | ConvertTo-Json -Depth 64 -Compress
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($jsonPath, $jsonText, $utf8NoBom)

    return $jsonPath
}

function Add-ReviewMutationPlanProperty {
    param(
        [System.Collections.Specialized.OrderedDictionary]$Target,
        $Operation,
        [string[]]$Names,
        [string]$OutputName
    )

    foreach ($name in $Names) {
        if (-not (Test-ObjectPropertyExists -Object $Operation -Name $name)) {
            continue
        }

        $Target[$OutputName] = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        return
    }
}

function New-ReviewMutationPlanOperationObject {
    param(
        $Operation,
        [string]$NormalizedOp
    )

    $reviewOperation = [ordered]@{
        kind = $NormalizedOp
    }

    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("comment_index", "index") `
        -OutputName "comment_index"
    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("expected_text", "expected_anchor_text") `
        -OutputName "expected_text"
    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("expected_comment_text", "expected_comment_body") `
        -OutputName "expected_comment_text"
    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("expected_resolved") `
        -OutputName "expected_resolved"
    Add-ReviewMutationPlanProperty `
        -Target $reviewOperation `
        -Operation $Operation `
        -Names @("expected_parent_index") `
        -OutputName "expected_parent_index"

    switch ($NormalizedOp) {
        "set_comment_resolved" {
            Add-ReviewMutationPlanProperty `
                -Target $reviewOperation `
                -Operation $Operation `
                -Names @("resolved") `
                -OutputName "resolved"
        }
        "set_comment_metadata" {
            foreach ($property in @("author", "initials", "date", "clear_author", "clear_initials", "clear_date")) {
                Add-ReviewMutationPlanProperty `
                    -Target $reviewOperation `
                    -Operation $Operation `
                    -Names @($property) `
                    -OutputName $property
            }
        }
        "replace_comment" {
            Add-ReviewMutationPlanProperty `
                -Target $reviewOperation `
                -Operation $Operation `
                -Names @("comment_text", "text", "replacement_text", "comment_body") `
                -OutputName "comment_text"
        }
        "remove_comment" {
        }
        default {
            throw "Unsupported review mutation operation '$NormalizedOp'."
        }
    }

    return [pscustomobject]$reviewOperation
}

function Add-ReviewMutationPlanArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$NormalizedOp,
        [string]$InputPath,
        [string]$TemporaryRoot,
        [int]$OperationIndex
    )

    $reviewOperation = New-ReviewMutationPlanOperationObject `
        -Operation $Operation `
        -NormalizedOp $NormalizedOp
    $reviewPlan = [pscustomobject]@{
        operations = @($reviewOperation)
    }
    $reviewPlanPath = New-EditOperationJsonFile `
        -TemporaryRoot $TemporaryRoot `
        -OperationIndex $OperationIndex `
        -ValueIndex 1 `
        -Value $reviewPlan

    $Arguments.Add("apply-review-mutation-plan") | Out-Null
    $Arguments.Add($InputPath) | Out-Null
    $Arguments.Add("--plan-file") | Out-Null
    $Arguments.Add($reviewPlanPath) | Out-Null
}

function Get-TemplateTableJsonPatchObject {
    param(
        $Operation,
        [string]$Label
    )

    foreach ($name in @("patch", "json_patch", "table_patch")) {
        $patch = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $patch) {
            return $patch
        }
    }

    $patchObject = [ordered]@{}
    $mode = Get-OptionalObjectPropertyValue -Object $Operation -Name "mode"
    if (-not [string]::IsNullOrWhiteSpace($mode)) {
        $patchObject["mode"] = $mode
    }

    foreach ($name in @("start_row", "start_row_index", "start_cell", "start_cell_index")) {
        $value = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $value) {
            $patchObject[$name] = $value
        }
    }

    $rows = Get-OptionalObjectPropertyObject -Object $Operation -Name "rows"
    if ($null -ne $rows) {
        $patchObject["rows"] = $rows
    }

    if ($patchObject.Count -eq 0) {
        throw "$Label must provide 'patch' or JSON patch members such as 'start_row' and 'rows'."
    }

    return [pscustomobject]$patchObject
}

function Get-TemplateTableJsonBatchPatchObject {
    param(
        $Operation,
        [string]$Label
    )

    foreach ($name in @("patch", "json_patch", "batch_patch")) {
        $patch = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $patch) {
            return $patch
        }
    }

    foreach ($name in @("table_operations", "patches", "table_patches", "operations")) {
        $operations = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $operations) {
            return [pscustomobject]@{
                operations = $operations
            }
        }
    }

    throw "$Label must provide 'patch' or a table patch operation array."
}

function Add-TemplateTableJsonPatchFileArgument {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [string]$TemporaryRoot,
        [int]$OperationIndex,
        [int]$ValueIndex,
        [switch]$Batch
    )

    $patchFile = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("patch_file", "patch_path", "json_patch_file")
    if ([string]::IsNullOrWhiteSpace($patchFile)) {
        if ($Batch) {
            $patch = Get-TemplateTableJsonBatchPatchObject -Operation $Operation -Label $Label
        } else {
            $patch = Get-TemplateTableJsonPatchObject -Operation $Operation -Label $Label
        }
        $patchFile = New-EditOperationJsonFile `
            -TemporaryRoot $TemporaryRoot `
            -OperationIndex $OperationIndex `
            -ValueIndex $ValueIndex `
            -Value $patch
    }

    $Arguments.Add("--patch-file") | Out-Null
    $Arguments.Add($patchFile) | Out-Null
}

function Get-BookmarkNameForOperation {
    param(
        $Operation,
        [string]$Label
    )

    $bookmark = Get-OptionalObjectPropertyValue -Object $Operation -Name "bookmark"
    if ([string]::IsNullOrWhiteSpace($bookmark)) {
        $bookmark = Get-OptionalObjectPropertyValue -Object $Operation -Name "bookmark_name"
    }
    if ([string]::IsNullOrWhiteSpace($bookmark)) {
        throw "$Label must provide 'bookmark' or 'bookmark_name'."
    }

    return $bookmark
}

function Get-OperationAlignmentProperty {
    param(
        $Operation,
        [string]$Label,
        [string]$FallbackName = ""
    )

    $alignment = Get-OptionalObjectPropertyValue -Object $Operation -Name "alignment"
    if ([string]::IsNullOrWhiteSpace($alignment) -and
        -not [string]::IsNullOrWhiteSpace($FallbackName)) {
        $alignment = Get-OptionalObjectPropertyValue -Object $Operation -Name $FallbackName
    }
    if ([string]::IsNullOrWhiteSpace($alignment)) {
        throw "$Label must provide 'alignment'."
    }

    return $alignment.Trim()
}

function Normalize-TableCellHorizontalAlignment {
    param(
        [string]$Alignment,
        [string]$Label
    )

    switch ($Alignment.Trim().ToLowerInvariant()) {
        "left" { return "left" }
        "start" { return "left" }
        "center" { return "center" }
        "centre" { return "center" }
        "right" { return "right" }
        "end" { return "right" }
        "both" { return "both" }
        "justify" { return "both" }
        default {
            throw "$Label has unsupported horizontal alignment '$Alignment'. Use left, center, right, or both."
        }
    }
}

function Get-OperationRowHeightRule {
    param($Operation)

    $rule = Get-OptionalObjectPropertyValue -Object $Operation -Name "rule"
    if ([string]::IsNullOrWhiteSpace($rule)) {
        $rule = Get-OptionalObjectPropertyValue -Object $Operation -Name "height_rule"
    }
    if ([string]::IsNullOrWhiteSpace($rule)) {
        return "exact"
    }

    return $rule.Trim()
}

function Read-DocxZipEntryText {
    param(
        [System.IO.Compression.ZipArchive]$Archive,
        [string]$EntryName
    )

    $entry = $Archive.GetEntry($EntryName)
    if ($null -eq $entry) {
        throw "Entry '$EntryName' was not found."
    }

    $reader = New-Object System.IO.StreamReader($entry.Open())
    try {
        return $reader.ReadToEnd()
    } finally {
        $reader.Dispose()
    }
}

function Write-DocxZipEntryText {
    param(
        [System.IO.Compression.ZipArchive]$Archive,
        [string]$EntryName,
        [string]$Text
    )

    $existingEntry = $Archive.GetEntry($EntryName)
    if ($null -ne $existingEntry) {
        $existingEntry.Delete()
    }

    $entry = $Archive.CreateEntry($EntryName)
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    $writer = New-Object System.IO.StreamWriter($entry.Open(), $utf8NoBom)
    try {
        $writer.Write($Text)
    } finally {
        $writer.Dispose()
    }
}

function Set-ParagraphHorizontalAlignmentNode {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Paragraph,
        [string]$Alignment,
        [bool]$Clear
    )

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $paragraphProperties = $Paragraph.SelectSingleNode("w:pPr", $NamespaceManager)
    if ($null -eq $paragraphProperties) {
        if ($Clear) {
            return
        }

        $paragraphProperties = $Document.CreateElement("w", "pPr", $wordNamespace)
        if ($Paragraph.HasChildNodes) {
            $Paragraph.InsertBefore($paragraphProperties, $Paragraph.FirstChild) | Out-Null
        } else {
            $Paragraph.AppendChild($paragraphProperties) | Out-Null
        }
    }

    $alignmentNode = $paragraphProperties.SelectSingleNode("w:jc", $NamespaceManager)
    if ($Clear) {
        if ($null -ne $alignmentNode) {
            $paragraphProperties.RemoveChild($alignmentNode) | Out-Null
        }
        if ($paragraphProperties.ChildNodes.Count -eq 0 -and
            $paragraphProperties.Attributes.Count -eq 0) {
            $Paragraph.RemoveChild($paragraphProperties) | Out-Null
        }
        return
    }

    if ($null -eq $alignmentNode) {
        $alignmentNode = $Document.CreateElement("w", "jc", $wordNamespace)
        $paragraphProperties.AppendChild($alignmentNode) | Out-Null
    }

    $valueAttribute = $Document.CreateAttribute("w", "val", $wordNamespace)
    $valueAttribute.Value = $Alignment
    $alignmentNode.Attributes.SetNamedItem($valueAttribute) | Out-Null
}

function Set-DocxTableCellHorizontalAlignment {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$TableIndex,
        [string]$RowIndex,
        [string]$CellIndex,
        [string]$Alignment,
        [bool]$Clear
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
        $null = $namespaceManager.AddNamespace("w", $wordNamespace)

        $tables = @($document.SelectNodes("/w:document/w:body/w:tbl", $namespaceManager))
        $tableIndexValue = [int]$TableIndex
        $rowIndexValue = [int]$RowIndex
        $cellIndexValue = [int]$CellIndex
        if ($tableIndexValue -lt 0 -or $tableIndexValue -ge $tables.Count) {
            throw "table_index '$TableIndex' is out of range."
        }

        $rows = @($tables[$tableIndexValue].SelectNodes("w:tr", $namespaceManager))
        if ($rowIndexValue -lt 0 -or $rowIndexValue -ge $rows.Count) {
            throw "row_index '$RowIndex' is out of range."
        }

        $cells = @($rows[$rowIndexValue].SelectNodes("w:tc", $namespaceManager))
        if ($cellIndexValue -lt 0 -or $cellIndexValue -ge $cells.Count) {
            throw "cell_index '$CellIndex' is out of range."
        }

        $paragraphs = @($cells[$cellIndexValue].SelectNodes("w:p", $namespaceManager))
        if ($paragraphs.Count -eq 0) {
            throw "target table cell does not contain direct paragraphs."
        }

        foreach ($paragraph in $paragraphs) {
            Set-ParagraphHorizontalAlignmentNode `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -Paragraph $paragraph `
                -Alignment $Alignment `
                -Clear $Clear
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            table_index = $tableIndexValue
            row_index = $rowIndexValue
            cell_index = $cellIndexValue
            paragraph_count = $paragraphs.Count
        }
    } finally {
        $archive.Dispose()
    }
}


function Get-OrCreateTableCellProperties {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Cell
    )

    $cellProperties = $Cell.SelectSingleNode("w:tcPr", $NamespaceManager)
    if ($null -ne $cellProperties) {
        return $cellProperties
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $cellProperties = $Document.CreateElement("w", "tcPr", $wordNamespace)
    if ($Cell.HasChildNodes) {
        $Cell.InsertBefore($cellProperties, $Cell.FirstChild) | Out-Null
    } else {
        $Cell.AppendChild($cellProperties) | Out-Null
    }
    return $cellProperties
}

function Get-OrCreateTableGrid {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Table
    )

    $tableGrid = $Table.SelectSingleNode("w:tblGrid", $NamespaceManager)
    if ($null -ne $tableGrid) {
        return $tableGrid
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $tableGrid = $Document.CreateElement("w", "tblGrid", $wordNamespace)
    $tableProperties = $Table.SelectSingleNode("w:tblPr", $NamespaceManager)
    if ($null -ne $tableProperties) {
        $Table.InsertAfter($tableGrid, $tableProperties) | Out-Null
    } elseif ($Table.HasChildNodes) {
        $Table.InsertBefore($tableGrid, $Table.FirstChild) | Out-Null
    } else {
        $Table.AppendChild($tableGrid) | Out-Null
    }
    return $tableGrid
}

function Set-TableColumnWidthNode {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Table,
        [int]$ColumnIndex,
        [string]$WidthTwips,
        [bool]$Clear
    )

    if ($ColumnIndex -lt 0) {
        throw "column_index '$ColumnIndex' is out of range."
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $targetCellCount = 0
    $tableGrid = if ($Clear) {
        $Table.SelectSingleNode("w:tblGrid", $NamespaceManager)
    } else {
        Get-OrCreateTableGrid -Document $Document -NamespaceManager $NamespaceManager -Table $Table
    }

    if ($null -ne $tableGrid) {
        $gridColumns = @($tableGrid.SelectNodes("w:gridCol", $NamespaceManager))
        if ($Clear) {
            if ($ColumnIndex -lt $gridColumns.Count) {
                $gridColumns[$ColumnIndex].RemoveAttribute("w", $wordNamespace)
            }
        } else {
            while ($gridColumns.Count -le $ColumnIndex) {
                $gridColumn = $Document.CreateElement("w", "gridCol", $wordNamespace)
                $tableGrid.AppendChild($gridColumn) | Out-Null
                $gridColumns = @($tableGrid.SelectNodes("w:gridCol", $NamespaceManager))
            }
            Set-WordAttribute -Document $Document -Element $gridColumns[$ColumnIndex] -Name "w" -Value $WidthTwips
        }
    }

    $rows = @($Table.SelectNodes("w:tr", $NamespaceManager))
    foreach ($row in $rows) {
        $cells = @($row.SelectNodes("w:tc", $NamespaceManager))
        if ($ColumnIndex -ge $cells.Count) {
            continue
        }

        $targetCellCount += 1
        $cell = $cells[$ColumnIndex]
        $cellProperties = if ($Clear) {
            $cell.SelectSingleNode("w:tcPr", $NamespaceManager)
        } else {
            Get-OrCreateTableCellProperties -Document $Document -NamespaceManager $NamespaceManager -Cell $cell
        }
        if ($null -eq $cellProperties) {
            continue
        }

        $cellWidthNode = $cellProperties.SelectSingleNode("w:tcW", $NamespaceManager)
        if ($Clear) {
            if ($null -ne $cellWidthNode) {
                $cellProperties.RemoveChild($cellWidthNode) | Out-Null
            }
            if ($cellProperties.ChildNodes.Count -eq 0 -and
                $cellProperties.Attributes.Count -eq 0) {
                $cell.RemoveChild($cellProperties) | Out-Null
            }
            continue
        }

        if ($null -eq $cellWidthNode) {
            $cellWidthNode = $Document.CreateElement("w", "tcW", $wordNamespace)
            if ($cellProperties.HasChildNodes) {
                $cellProperties.InsertBefore($cellWidthNode, $cellProperties.FirstChild) | Out-Null
            } else {
                $cellProperties.AppendChild($cellWidthNode) | Out-Null
            }
        }
        Set-WordAttribute -Document $Document -Element $cellWidthNode -Name "w" -Value $WidthTwips
        Set-WordAttribute -Document $Document -Element $cellWidthNode -Name "type" -Value "dxa"
    }

    if ($targetCellCount -eq 0) {
        throw "column_index '$ColumnIndex' did not match any table cells."
    }

    return [ordered]@{
        row_count = $rows.Count
        target_cell_count = $targetCellCount
    }
}

function Set-DocxTableColumnWidth {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$TableIndex,
        [string]$ColumnIndex,
        [string]$WidthTwips,
        [bool]$Clear
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    if (-not $Clear) {
        $widthValue = [int]$WidthTwips
        if ($widthValue -le 0) {
            throw "width_twips must be greater than zero."
        }
        $WidthTwips = [string]$widthValue
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
        $null = $namespaceManager.AddNamespace("w", $wordNamespace)

        $tables = @($document.SelectNodes("/w:document/w:body/w:tbl", $namespaceManager))
        $tableIndexValue = [int]$TableIndex
        if ($tableIndexValue -lt 0 -or $tableIndexValue -ge $tables.Count) {
            throw "table_index '$TableIndex' is out of range."
        }
        $columnIndexValue = [int]$ColumnIndex

        $mutationResult = Set-TableColumnWidthNode `
            -Document $document `
            -NamespaceManager $namespaceManager `
            -Table $tables[$tableIndexValue] `
            -ColumnIndex $columnIndexValue `
            -WidthTwips $WidthTwips `
            -Clear $Clear

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            table_index = $tableIndexValue
            column_index = $columnIndexValue
            width_twips = $WidthTwips
            clear = $Clear
            row_count = $mutationResult.row_count
            target_cell_count = $mutationResult.target_cell_count
        }
    } finally {
        $archive.Dispose()
    }
}

function Get-ParagraphTextContent {
    param(
        [System.Xml.XmlElement]$Paragraph,
        [System.Xml.XmlNamespaceManager]$NamespaceManager
    )

    $builder = New-Object System.Text.StringBuilder
    foreach ($textNode in @($Paragraph.SelectNodes(".//w:t", $NamespaceManager))) {
        [void]$builder.Append($textNode.InnerText)
    }

    return $builder.ToString()
}

function Get-BodyParagraphAlignmentTargets {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$ParagraphIndex,
        [string]$TextContains
    )

    $paragraphs = @($Document.SelectNodes("/w:document/w:body/w:p", $NamespaceManager))
    if (-not [string]::IsNullOrWhiteSpace($ParagraphIndex)) {
        $paragraphIndexValue = [int]$ParagraphIndex
        if ($paragraphIndexValue -lt 0 -or $paragraphIndexValue -ge $paragraphs.Count) {
            throw "paragraph_index '$ParagraphIndex' is out of range."
        }

        return @($paragraphs[$paragraphIndexValue])
    }

    if (-not [string]::IsNullOrWhiteSpace($TextContains)) {
        $matches = @($paragraphs | Where-Object {
                (Get-ParagraphTextContent -Paragraph $_ -NamespaceManager $NamespaceManager).Contains($TextContains)
            })
        if ($matches.Count -eq 0) {
            throw "No body paragraph contains '$TextContains'."
        }

        return $matches
    }

    throw "Paragraph alignment operation must provide 'paragraph_index' or 'text_contains'."
}

function Set-DocxBodyParagraphHorizontalAlignment {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$ParagraphIndex,
        [string]$TextContains,
        [string]$Alignment,
        [bool]$Clear
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
        $null = $namespaceManager.AddNamespace("w", $wordNamespace)

        $targets = @(Get-BodyParagraphAlignmentTargets `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -ParagraphIndex $ParagraphIndex `
                -TextContains $TextContains)

        foreach ($paragraph in $targets) {
            Set-ParagraphHorizontalAlignmentNode `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -Paragraph $paragraph `
                -Alignment $Alignment `
                -Clear $Clear
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            paragraph_count = $targets.Count
            paragraph_index = $ParagraphIndex
            text_contains = $TextContains
        }
    } finally {
        $archive.Dispose()
    }
}

function Normalize-ParagraphSpacingLineRule {
    param(
        [string]$LineRule,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($LineRule)) {
        return ""
    }

    $normalized = $LineRule.Trim().ToLowerInvariant()
    switch ($normalized) {
        "auto" { return "auto" }
        "exact" { return "exact" }
        "atleast" { return "atLeast" }
        "at_least" { return "atLeast" }
        "at-least" { return "atLeast" }
        default { throw "$Label line_rule must be auto, exact, or at_least." }
    }
}

function Set-ParagraphSpacingNode {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Paragraph,
        [string]$BeforeTwips,
        [string]$AfterTwips,
        [string]$LineTwips,
        [string]$LineRule,
        [bool]$Clear
    )

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $paragraphProperties = $Paragraph.SelectSingleNode("w:pPr", $NamespaceManager)
    if ($null -eq $paragraphProperties) {
        if ($Clear) {
            return
        }

        $paragraphProperties = $Document.CreateElement("w", "pPr", $wordNamespace)
        if ($Paragraph.HasChildNodes) {
            $Paragraph.InsertBefore($paragraphProperties, $Paragraph.FirstChild) | Out-Null
        } else {
            $Paragraph.AppendChild($paragraphProperties) | Out-Null
        }
    }

    $spacingNode = $paragraphProperties.SelectSingleNode("w:spacing", $NamespaceManager)
    if ($Clear) {
        if ($null -ne $spacingNode) {
            $paragraphProperties.RemoveChild($spacingNode) | Out-Null
        }
        if ($paragraphProperties.ChildNodes.Count -eq 0 -and
            $paragraphProperties.Attributes.Count -eq 0) {
            $Paragraph.RemoveChild($paragraphProperties) | Out-Null
        }
        return
    }

    if ($null -eq $spacingNode) {
        $spacingNode = $Document.CreateElement("w", "spacing", $wordNamespace)
        $paragraphProperties.AppendChild($spacingNode) | Out-Null
    }

    if (-not [string]::IsNullOrWhiteSpace($BeforeTwips)) {
        Set-WordAttribute -Document $Document -Element $spacingNode -Name "before" -Value $BeforeTwips
    }
    if (-not [string]::IsNullOrWhiteSpace($AfterTwips)) {
        Set-WordAttribute -Document $Document -Element $spacingNode -Name "after" -Value $AfterTwips
    }
    if (-not [string]::IsNullOrWhiteSpace($LineTwips)) {
        Set-WordAttribute -Document $Document -Element $spacingNode -Name "line" -Value $LineTwips
    }
    if (-not [string]::IsNullOrWhiteSpace($LineRule)) {
        Set-WordAttribute -Document $Document -Element $spacingNode -Name "lineRule" -Value $LineRule
    }
}

function Set-DocxBodyParagraphSpacing {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$ParagraphIndex,
        [string]$TextContains,
        [string]$BeforeTwips,
        [string]$AfterTwips,
        [string]$LineTwips,
        [string]$LineRule,
        [bool]$Clear
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
        $null = $namespaceManager.AddNamespace("w", $wordNamespace)

        $targets = @(Get-BodyParagraphAlignmentTargets `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -ParagraphIndex $ParagraphIndex `
                -TextContains $TextContains)

        foreach ($paragraph in $targets) {
            Set-ParagraphSpacingNode `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -Paragraph $paragraph `
                -BeforeTwips $BeforeTwips `
                -AfterTwips $AfterTwips `
                -LineTwips $LineTwips `
                -LineRule $LineRule `
                -Clear $Clear
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            paragraph_count = $targets.Count
            paragraph_index = $ParagraphIndex
            text_contains = $TextContains
            before_twips = $BeforeTwips
            after_twips = $AfterTwips
            line_twips = $LineTwips
            line_rule = $LineRule
            clear = $Clear
        }
    } finally {
        $archive.Dispose()
    }
}

function New-DirectParagraphHorizontalAlignmentOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label,
        [bool]$Clear
    )

    $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_index"
    if ([string]::IsNullOrWhiteSpace($paragraphIndex)) {
        $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "index"
    }

    $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "text_contains"
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_text_contains"
    }

    $alignment = ""
    if (-not $Clear) {
        $alignment = Normalize-TableCellHorizontalAlignment `
            -Alignment (Get-OperationAlignmentProperty `
                -Operation $Operation `
                -Label $Label `
                -FallbackName "horizontal_alignment") `
            -Label $Label
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = if ($Clear) { "clear-paragraph-horizontal-alignment" } else { "set-paragraph-horizontal-alignment" }
        DirectOperation = $true
        DirectOperationKind = "body-paragraph-horizontal-alignment"
        ParagraphIndex = $paragraphIndex
        TextContains = $textContains
        HorizontalAlignment = $alignment
        ClearHorizontalAlignment = $Clear
    }
}

function New-DirectParagraphSpacingOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label,
        [bool]$Clear
    )

    $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_index"
    if ([string]::IsNullOrWhiteSpace($paragraphIndex)) {
        $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "index"
    }

    $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "text_contains"
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_text_contains"
    }
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "contains"
    }

    $beforeTwips = ""
    $afterTwips = ""
    $lineTwips = ""
    $lineRule = ""
    if (-not $Clear) {
        $beforeTwips = Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("before_twips", "space_before_twips", "spacing_before_twips")
        $afterTwips = Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("after_twips", "space_after_twips", "spacing_after_twips")
        $lineTwips = Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("line_twips", "line_spacing_twips", "spacing_line_twips")
        $lineRule = Normalize-ParagraphSpacingLineRule `
            -LineRule (Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("line_rule", "line_spacing_rule", "rule")) `
            -Label $Label

        if ([string]::IsNullOrWhiteSpace($beforeTwips) -and
            [string]::IsNullOrWhiteSpace($afterTwips) -and
            [string]::IsNullOrWhiteSpace($lineTwips) -and
            [string]::IsNullOrWhiteSpace($lineRule)) {
            throw "$Label must provide at least one paragraph spacing property."
        }
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = if ($Clear) { "clear-paragraph-spacing" } else { "set-paragraph-spacing" }
        DirectOperation = $true
        DirectOperationKind = "body-paragraph-spacing"
        ParagraphIndex = $paragraphIndex
        TextContains = $textContains
        BeforeTwips = $beforeTwips
        AfterTwips = $afterTwips
        LineTwips = $lineTwips
        LineRule = $lineRule
        ClearSpacing = $Clear
    }
}


function New-DirectTableColumnWidthOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label,
        [bool]$Clear
    )

    $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
    $columnIndex = Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("column_index", "col_index", "column") `
        -Label $Label

    $widthTwips = ""
    if (-not $Clear) {
        $widthTwips = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("width_twips", "column_width_twips", "width") `
            -Label $Label
        $widthValue = [int]$widthTwips
        if ($widthValue -le 0) {
            throw "$Label width_twips must be greater than zero."
        }
        $widthTwips = [string]$widthValue
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = if ($Clear) { "clear-table-column-width" } else { "set-table-column-width" }
        DirectOperation = $true
        DirectOperationKind = "body-table-column-width"
        TableIndex = $tableIndex
        ColumnIndex = $columnIndex
        WidthTwips = $widthTwips
        ClearColumnWidth = $Clear
    }
}

function Get-OptionalBooleanPropertyValue {
    param(
        $Object,
        [string]$Name,
        [bool]$DefaultValue
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return $DefaultValue
    }
    if ($value -is [bool]) {
        return $value
    }

    $text = ([string]$value).Trim().ToLowerInvariant()
    switch ($text) {
        "true" { return $true }
        "1" { return $true }
        "yes" { return $true }
        "false" { return $false }
        "0" { return $false }
        "no" { return $false }
        default { throw "Property '$Name' must be a boolean value." }
    }
}

function Get-FirstObjectPropertyValue {
    param(
        $Object,
        [string[]]$Names,
        [string]$Label
    )

    foreach ($name in $Names) {
        $value = Get-OptionalObjectPropertyValue -Object $Object -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    throw "$Label must provide one of: $($Names -join ', ')."
}

function Get-FirstOptionalObjectPropertyValue {
    param(
        $Object,
        [string[]]$Names
    )

    foreach ($name in $Names) {
        $value = Get-OptionalObjectPropertyValue -Object $Object -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    return ""
}

function ConvertTo-NativeCliQuotedArgument {
    param([string]$Value)

    return $Value.Replace('"', '\"')
}

function Add-RevisionAuthoringArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [switch]$RequireText,
        [switch]$AllowText,
        [switch]$AllowExpectedText
    )

    if ($RequireText -or $AllowText) {
        $text = if ($RequireText) {
            Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text", "revision_text") `
                -Label $Label
        } else {
            Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("text", "revision_text")
        }
        if (-not [string]::IsNullOrWhiteSpace($text)) {
            $Arguments.Add("--text") | Out-Null
            $Arguments.Add($text) | Out-Null
        }
    } elseif ((Test-ObjectPropertyExists -Object $Operation -Name "text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "revision_text")) {
        throw "$Label does not accept 'text' or 'revision_text'."
    }

    $author = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("author", "revision_author")
    if (-not [string]::IsNullOrWhiteSpace($author)) {
        $Arguments.Add("--author") | Out-Null
        $Arguments.Add($author) | Out-Null
    }

    $date = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("date", "revision_date")
    if (-not [string]::IsNullOrWhiteSpace($date)) {
        $Arguments.Add("--date") | Out-Null
        $Arguments.Add($date) | Out-Null
    }

    if ($AllowExpectedText) {
        $expectedText = Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("expected_text", "expected")
        if (-not [string]::IsNullOrWhiteSpace($expectedText)) {
            $Arguments.Add("--expected-text") | Out-Null
            $Arguments.Add($expectedText) | Out-Null
        }
    } elseif ((Test-ObjectPropertyExists -Object $Operation -Name "expected_text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "expected")) {
        throw "$Label does not accept 'expected_text' or 'expected'."
    }
}

function Add-RevisionMetadataArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $optionCount = 0
    $author = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("author", "revision_author")
    $clearAuthor = Test-ObjectPropertyExists -Object $Operation -Name "clear_author"
    if (-not [string]::IsNullOrWhiteSpace($author)) {
        if ($clearAuthor -and (Get-OptionalBooleanPropertyValue -Object $Operation -Name "clear_author" -DefaultValue $false)) {
            throw "$Label cannot combine 'author' and 'clear_author'."
        }
        $Arguments.Add("--author") | Out-Null
        $Arguments.Add($author) | Out-Null
        $optionCount += 1
    } elseif ($clearAuthor -and
        (Get-OptionalBooleanPropertyValue -Object $Operation -Name "clear_author" -DefaultValue $false)) {
        $Arguments.Add("--clear-author") | Out-Null
        $optionCount += 1
    }

    $date = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("date", "revision_date")
    $clearDate = Test-ObjectPropertyExists -Object $Operation -Name "clear_date"
    if (-not [string]::IsNullOrWhiteSpace($date)) {
        if ($clearDate -and (Get-OptionalBooleanPropertyValue -Object $Operation -Name "clear_date" -DefaultValue $false)) {
            throw "$Label cannot combine 'date' and 'clear_date'."
        }
        $Arguments.Add("--date") | Out-Null
        $Arguments.Add($date) | Out-Null
        $optionCount += 1
    } elseif ($clearDate -and
        (Get-OptionalBooleanPropertyValue -Object $Operation -Name "clear_date" -DefaultValue $false)) {
        $Arguments.Add("--clear-date") | Out-Null
        $optionCount += 1
    }

    if ($optionCount -eq 0) {
        throw "$Label must provide at least one revision metadata option."
    }
}

function Add-CommentAuthoringArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [switch]$RequireSelectedText,
        [switch]$RequireCommentText
    )

    if ($RequireSelectedText) {
        $selectedText = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("selected_text", "anchor_text") `
            -Label $Label
        $Arguments.Add("--selected-text") | Out-Null
        $Arguments.Add($selectedText) | Out-Null
    }

    if ($RequireCommentText) {
        $commentText = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("comment_text", "text", "body") `
            -Label $Label
        $Arguments.Add("--comment-text") | Out-Null
        $Arguments.Add($commentText) | Out-Null
    }

    $author = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("author", "comment_author")
    if (-not [string]::IsNullOrWhiteSpace($author)) {
        $Arguments.Add("--author") | Out-Null
        $Arguments.Add($author) | Out-Null
    }

    $initials = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("initials", "comment_initials")
    if (-not [string]::IsNullOrWhiteSpace($initials)) {
        $Arguments.Add("--initials") | Out-Null
        $Arguments.Add($initials) | Out-Null
    }

    $date = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("date", "comment_date")
    if (-not [string]::IsNullOrWhiteSpace($date)) {
        $Arguments.Add("--date") | Out-Null
        $Arguments.Add($date) | Out-Null
    }
}

function Add-ReviewNoteMutationArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [switch]$RequireReferenceText,
        [switch]$RequireNoteText
    )

    if ($RequireReferenceText) {
        $referenceText = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("reference_text", "selected_text", "anchor_text") `
            -Label $Label
        $Arguments.Add("--reference-text") | Out-Null
        $Arguments.Add($referenceText) | Out-Null
    } elseif ((Test-ObjectPropertyExists -Object $Operation -Name "reference_text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "selected_text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "anchor_text")) {
        throw "$Label does not accept 'reference_text', 'selected_text', or 'anchor_text'."
    }

    if ($RequireNoteText) {
        $noteText = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("note_text", "text", "body") `
            -Label $Label
        $Arguments.Add("--note-text") | Out-Null
        $Arguments.Add($noteText) | Out-Null
    } elseif ((Test-ObjectPropertyExists -Object $Operation -Name "note_text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "body")) {
        throw "$Label does not accept 'note_text', 'text', or 'body'."
    }
}


function Remove-DocxBodyParagraphs {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$ParagraphIndex,
        [string]$TextContains
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")

        $targets = @(Get-BodyParagraphAlignmentTargets `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -ParagraphIndex $ParagraphIndex `
                -TextContains $TextContains)

        foreach ($paragraph in @($targets)) {
            $parent = $paragraph.ParentNode
            if ($null -ne $parent) {
                $parent.RemoveChild($paragraph) | Out-Null
            }
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            paragraph_count = $targets.Count
            paragraph_index = $ParagraphIndex
            text_contains = $TextContains
        }
    } finally {
        $archive.Dispose()
    }
}

function New-DirectParagraphDeletionOperation {
    param(
        $Operation,
        [string]$NormalizedOp
    )

    $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_index"
    if ([string]::IsNullOrWhiteSpace($paragraphIndex)) {
        $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "index"
    }

    $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "text_contains"
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_text_contains"
    }
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "contains"
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = "delete-paragraph"
        DirectOperation = $true
        DirectOperationKind = "body-paragraph-deletion"
        ParagraphIndex = $paragraphIndex
        TextContains = $textContains
    }
}

function Set-ParagraphPlainTextContent {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Paragraph,
        [string]$Text
    )

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $xmlNamespace = "http://www.w3.org/XML/1998/namespace"
    $paragraphProperties = $Paragraph.SelectSingleNode("w:pPr", $NamespaceManager)
    $firstRunProperties = $Paragraph.SelectSingleNode("w:r/w:rPr", $NamespaceManager)

    foreach ($child in @($Paragraph.ChildNodes)) {
        if ($null -ne $paragraphProperties -and [object]::ReferenceEquals($child, $paragraphProperties)) {
            continue
        }
        $Paragraph.RemoveChild($child) | Out-Null
    }

    $run = $Document.CreateElement("w", "r", $wordNamespace)
    if ($null -ne $firstRunProperties) {
        $run.AppendChild($firstRunProperties.CloneNode($true)) | Out-Null
    }

    $textNode = $Document.CreateElement("w", "t", $wordNamespace)
    if ($Text.Length -gt 0 -and
        ($Text[0] -match '\s' -or $Text[$Text.Length - 1] -match '\s' -or $Text.Contains("  "))) {
        $spaceAttribute = $Document.CreateAttribute("xml", "space", $xmlNamespace)
        $spaceAttribute.Value = "preserve"
        $textNode.Attributes.SetNamedItem($spaceAttribute) | Out-Null
    }
    $textNode.InnerText = $Text
    $run.AppendChild($textNode) | Out-Null
    $Paragraph.AppendChild($run) | Out-Null
}

function Set-DocxTextReplacement {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$FindText,
        [string]$ReplacementText,
        [bool]$MatchCase
    )

    if ([string]::IsNullOrEmpty($FindText)) {
        throw "Text replacement must provide a non-empty find text."
    }

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")

        $regexOptions = [System.Text.RegularExpressions.RegexOptions]::CultureInvariant
        if (-not $MatchCase) {
            $regexOptions = $regexOptions -bor [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
        }
        $pattern = [System.Text.RegularExpressions.Regex]::Escape($FindText)
        $changedParagraphCount = 0
        $replacementCount = 0

        foreach ($paragraph in @($document.SelectNodes("//w:p", $namespaceManager))) {
            $paragraphText = Get-ParagraphTextContent `
                -Paragraph $paragraph `
                -NamespaceManager $namespaceManager
            $matches = [System.Text.RegularExpressions.Regex]::Matches($paragraphText, $pattern, $regexOptions)
            if ($matches.Count -eq 0) {
                continue
            }

            $newText = [System.Text.RegularExpressions.Regex]::Replace(
                $paragraphText,
                $pattern,
                [System.Text.RegularExpressions.MatchEvaluator]{ param($match) $ReplacementText },
                $regexOptions
            )
            Set-ParagraphPlainTextContent `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -Paragraph $paragraph `
                -Text $newText
            $changedParagraphCount += 1
            $replacementCount += $matches.Count
        }

        if ($replacementCount -eq 0) {
            throw "Text '$FindText' was not found in word/document.xml."
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            find_text = $FindText
            replacement_text = $ReplacementText
            match_case = $MatchCase
            paragraph_count = $changedParagraphCount
            replacement_count = $replacementCount
        }
    } finally {
        $archive.Dispose()
    }
}

function New-DirectTextReplacementOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label
    )

    $findText = Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("find", "search", "old_text") `
        -Label $Label
    $replacementText = Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("replace", "replacement", "new_text", "text") `
        -Label $Label
    $matchCase = Get-OptionalBooleanPropertyValue `
        -Object $Operation `
        -Name "match_case" `
        -DefaultValue $true

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = "replace-text"
        DirectOperation = $true
        DirectOperationKind = "text-replacement"
        FindText = $findText
        ReplacementText = $replacementText
        MatchCase = $matchCase
    }
}


function Get-ParagraphTextStyleTargets {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$TableIndex,
        [string]$RowIndex,
        [string]$CellIndex,
        [string]$ParagraphIndex,
        [string]$TextContains
    )

    $hasTableLocator = @(@($TableIndex, $RowIndex, $CellIndex) | Where-Object {
            -not [string]::IsNullOrWhiteSpace($_)
        })
    if ($hasTableLocator.Count -gt 0) {
        if ([string]::IsNullOrWhiteSpace($TableIndex) -or
            [string]::IsNullOrWhiteSpace($RowIndex) -or
            [string]::IsNullOrWhiteSpace($CellIndex)) {
            throw "Text style table-cell targeting must provide table_index, row_index, and cell_index together."
        }

        $tableIndexValue = [int]$TableIndex
        $rowIndexValue = [int]$RowIndex
        $cellIndexValue = [int]$CellIndex
        $tables = @($Document.SelectNodes("/w:document/w:body/w:tbl", $NamespaceManager))
        if ($tableIndexValue -lt 0 -or $tableIndexValue -ge $tables.Count) {
            throw "table_index '$TableIndex' is out of range."
        }
        $rows = @($tables[$tableIndexValue].SelectNodes("w:tr", $NamespaceManager))
        if ($rowIndexValue -lt 0 -or $rowIndexValue -ge $rows.Count) {
            throw "row_index '$RowIndex' is out of range."
        }
        $cells = @($rows[$rowIndexValue].SelectNodes("w:tc", $NamespaceManager))
        if ($cellIndexValue -lt 0 -or $cellIndexValue -ge $cells.Count) {
            throw "cell_index '$CellIndex' is out of range."
        }

        $paragraphs = @($cells[$cellIndexValue].SelectNodes("w:p", $NamespaceManager))
        if ($paragraphs.Count -eq 0) {
            throw "target table cell does not contain direct paragraphs."
        }
        if (-not [string]::IsNullOrWhiteSpace($ParagraphIndex)) {
            $paragraphIndexValue = [int]$ParagraphIndex
            if ($paragraphIndexValue -lt 0 -or $paragraphIndexValue -ge $paragraphs.Count) {
                throw "paragraph_index '$ParagraphIndex' is out of range in target table cell."
            }
            $paragraphs = @($paragraphs[$paragraphIndexValue])
        }
        if (-not [string]::IsNullOrWhiteSpace($TextContains)) {
            $paragraphs = @($paragraphs | Where-Object {
                    (Get-ParagraphTextContent -Paragraph $_ -NamespaceManager $NamespaceManager).Contains($TextContains)
                })
            if ($paragraphs.Count -eq 0) {
                throw "No target table-cell paragraph contains '$TextContains'."
            }
        }
        return $paragraphs
    }

    if (-not [string]::IsNullOrWhiteSpace($ParagraphIndex)) {
        $paragraphs = @($Document.SelectNodes("/w:document/w:body/w:p", $NamespaceManager))
        $paragraphIndexValue = [int]$ParagraphIndex
        if ($paragraphIndexValue -lt 0 -or $paragraphIndexValue -ge $paragraphs.Count) {
            throw "paragraph_index '$ParagraphIndex' is out of range."
        }
        return @($paragraphs[$paragraphIndexValue])
    }

    if (-not [string]::IsNullOrWhiteSpace($TextContains)) {
        $matches = @($Document.SelectNodes("//w:p", $NamespaceManager) | Where-Object {
                (Get-ParagraphTextContent -Paragraph $_ -NamespaceManager $NamespaceManager).Contains($TextContains)
            })
        if ($matches.Count -eq 0) {
            throw "No paragraph contains '$TextContains'."
        }
        return $matches
    }

    throw "Text style operation must provide 'text_contains', 'paragraph_index', or table cell indexes."
}

function Get-OrCreateChildElement {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlElement]$Parent,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$LocalName
    )

    $existing = $Parent.SelectSingleNode("w:$LocalName", $NamespaceManager)
    if ($null -ne $existing) {
        return $existing
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $child = $Document.CreateElement("w", $LocalName, $wordNamespace)
    $Parent.AppendChild($child) | Out-Null
    return $child
}

function Get-OrCreateRunProperties {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Run
    )

    $runProperties = $Run.SelectSingleNode("w:rPr", $NamespaceManager)
    if ($null -ne $runProperties) {
        return $runProperties
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $runProperties = $Document.CreateElement("w", "rPr", $wordNamespace)
    if ($Run.HasChildNodes) {
        $Run.InsertBefore($runProperties, $Run.FirstChild) | Out-Null
    } else {
        $Run.AppendChild($runProperties) | Out-Null
    }
    return $runProperties
}

function Set-WordAttribute {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlElement]$Element,
        [string]$Name,
        [string]$Value
    )

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $attribute = $Document.CreateAttribute("w", $Name, $wordNamespace)
    $attribute.Value = $Value
    $Element.Attributes.SetNamedItem($attribute) | Out-Null
}

function Normalize-TextStyleColor {
    param(
        [string]$Color,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Color)) {
        return ""
    }

    $normalizedColor = $Color.Trim()
    if ($normalizedColor.StartsWith("#")) {
        $normalizedColor = $normalizedColor.Substring(1)
    }
    if ($normalizedColor -eq "auto") {
        return $normalizedColor
    }
    if ($normalizedColor -notmatch "^[0-9A-Fa-f]{6}$") {
        throw "$Label text color must be 'auto' or a 6-digit RGB hex value."
    }

    return $normalizedColor.ToUpperInvariant()
}

function Convert-TextStyleFontSizeToHalfPoints {
    param(
        [string]$FontSize,
        [string]$FontSizeHalfPoints,
        [string]$Label
    )

    if (-not [string]::IsNullOrWhiteSpace($FontSizeHalfPoints)) {
        $halfPoints = [int]$FontSizeHalfPoints
        if ($halfPoints -le 0) {
            throw "$Label font_size_half_points must be greater than zero."
        }
        return [string]$halfPoints
    }

    if ([string]::IsNullOrWhiteSpace($FontSize)) {
        return ""
    }

    $culture = [System.Globalization.CultureInfo]::InvariantCulture
    $points = [double]::Parse($FontSize, $culture)
    if ($points -le 0) {
        throw "$Label font_size_points must be greater than zero."
    }

    return [string]([int][System.Math]::Round(
            $points * 2,
            [System.MidpointRounding]::AwayFromZero))
}

function Set-RunTextStyle {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Run,
        [bool]$BoldSpecified,
        [bool]$Bold,
        [string]$FontFamily,
        [string]$EastAsiaFontFamily,
        [string]$Language,
        [string]$EastAsiaLanguage,
        [string]$Color,
        [string]$FontSizeHalfPoints
    )

    $runProperties = Get-OrCreateRunProperties `
        -Document $Document `
        -NamespaceManager $NamespaceManager `
        -Run $Run

    if ($BoldSpecified) {
        $boldNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "b"
        $boldValue = if ($Bold) { "1" } else { "0" }
        Set-WordAttribute `
            -Document $Document `
            -Element $boldNode `
            -Name "val" `
            -Value $boldValue
    }

    if (-not [string]::IsNullOrWhiteSpace($FontFamily) -or
        -not [string]::IsNullOrWhiteSpace($EastAsiaFontFamily)) {
        $fontsNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "rFonts"
        if (-not [string]::IsNullOrWhiteSpace($FontFamily)) {
            Set-WordAttribute -Document $Document -Element $fontsNode -Name "ascii" -Value $FontFamily
            Set-WordAttribute -Document $Document -Element $fontsNode -Name "hAnsi" -Value $FontFamily
        }
        if (-not [string]::IsNullOrWhiteSpace($EastAsiaFontFamily)) {
            Set-WordAttribute -Document $Document -Element $fontsNode -Name "eastAsia" -Value $EastAsiaFontFamily
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($Language) -or
        -not [string]::IsNullOrWhiteSpace($EastAsiaLanguage)) {
        $languageNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "lang"
        if (-not [string]::IsNullOrWhiteSpace($Language)) {
            Set-WordAttribute -Document $Document -Element $languageNode -Name "val" -Value $Language
        }
        if (-not [string]::IsNullOrWhiteSpace($EastAsiaLanguage)) {
            Set-WordAttribute -Document $Document -Element $languageNode -Name "eastAsia" -Value $EastAsiaLanguage
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($Color)) {
        $colorNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "color"
        Set-WordAttribute -Document $Document -Element $colorNode -Name "val" -Value $Color
    }

    if (-not [string]::IsNullOrWhiteSpace($FontSizeHalfPoints)) {
        $sizeNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "sz"
        Set-WordAttribute -Document $Document -Element $sizeNode -Name "val" -Value $FontSizeHalfPoints

        $complexScriptSizeNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "szCs"
        Set-WordAttribute -Document $Document -Element $complexScriptSizeNode -Name "val" -Value $FontSizeHalfPoints
    }
}

function Set-DocxParagraphTextStyle {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$TableIndex,
        [string]$RowIndex,
        [string]$CellIndex,
        [string]$ParagraphIndex,
        [string]$TextContains,
        [bool]$BoldSpecified,
        [bool]$Bold,
        [string]$FontFamily,
        [string]$EastAsiaFontFamily,
        [string]$Language,
        [string]$EastAsiaLanguage,
        [string]$Color,
        [string]$FontSizeHalfPoints
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")

        $targets = @(Get-ParagraphTextStyleTargets `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -TableIndex $TableIndex `
                -RowIndex $RowIndex `
                -CellIndex $CellIndex `
                -ParagraphIndex $ParagraphIndex `
                -TextContains $TextContains)

        $runCount = 0
        foreach ($paragraph in $targets) {
            foreach ($run in @($paragraph.SelectNodes("w:r", $namespaceManager))) {
                Set-RunTextStyle `
                    -Document $document `
                    -NamespaceManager $namespaceManager `
                    -Run $run `
                    -BoldSpecified $BoldSpecified `
                    -Bold $Bold `
                    -FontFamily $FontFamily `
                    -EastAsiaFontFamily $EastAsiaFontFamily `
                    -Language $Language `
                    -EastAsiaLanguage $EastAsiaLanguage `
                    -Color $Color `
                    -FontSizeHalfPoints $FontSizeHalfPoints
                $runCount += 1
            }
        }

        if ($runCount -eq 0) {
            throw "Target paragraph does not contain runs to style."
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            paragraph_count = $targets.Count
            run_count = $runCount
            table_index = $TableIndex
            row_index = $RowIndex
            cell_index = $CellIndex
            paragraph_index = $ParagraphIndex
            text_contains = $TextContains
            color = $Color
            font_size_half_points = $FontSizeHalfPoints
        }
    } finally {
        $archive.Dispose()
    }
}

function New-DirectTextStyleOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label
    )

    $boldObject = Get-OptionalObjectPropertyObject -Object $Operation -Name "bold"
    $boldSpecified = $null -ne $boldObject
    $bold = if ($boldSpecified) {
        Get-OptionalBooleanPropertyValue -Object $Operation -Name "bold" -DefaultValue $false
    } else {
        $false
    }
    $fontFamily = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("font_family", "font", "latin_font_family")
    $eastAsiaFontFamily = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @(
            "east_asia_font_family",
            "east_asia_font",
            "chinese_font_family",
            "cjk_font_family")
    $language = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("language", "lang", "primary_language")
    $eastAsiaLanguage = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @(
            "east_asia_language",
            "east_asia_lang",
            "chinese_language",
            "cjk_language")
    $color = Normalize-TextStyleColor `
        -Color (Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("color", "text_color", "font_color")) `
        -Label $Label
    $fontSizeHalfPoints = Convert-TextStyleFontSizeToHalfPoints `
        -FontSize (Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("font_size_points", "font_size", "size_points", "font_size_pt")) `
        -FontSizeHalfPoints (Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("font_size_half_points", "size_half_points")) `
        -Label $Label
    if ((-not $boldSpecified) -and
        [string]::IsNullOrWhiteSpace($fontFamily) -and
        [string]::IsNullOrWhiteSpace($eastAsiaFontFamily) -and
        [string]::IsNullOrWhiteSpace($language) -and
        [string]::IsNullOrWhiteSpace($eastAsiaLanguage) -and
        [string]::IsNullOrWhiteSpace($color) -and
        [string]::IsNullOrWhiteSpace($fontSizeHalfPoints)) {
        throw "$Label must provide at least one text style property."
    }

    $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_index"
    if ([string]::IsNullOrWhiteSpace($paragraphIndex)) {
        $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "index"
    }
    $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "text_contains"
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "contains"
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = "set-text-style"
        DirectOperation = $true
        DirectOperationKind = "text-style"
        TableIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "table_index"
        RowIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "row_index"
        CellIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "cell_index"
        ParagraphIndex = $paragraphIndex
        TextContains = $textContains
        BoldSpecified = $boldSpecified
        Bold = $bold
        FontFamily = $fontFamily
        EastAsiaFontFamily = $eastAsiaFontFamily
        Language = $language
        EastAsiaLanguage = $eastAsiaLanguage
        Color = $color
        FontSizeHalfPoints = $fontSizeHalfPoints
    }
}

function New-OperationArguments {
    param(
        $Operation,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Label,
        [int]$OperationIndex,
        [string]$TemporaryRoot
    )

    $op = (Get-RequiredObjectPropertyValue -Object $Operation -Name "op" -Label $Label).Trim()
    $normalizedOp = $op -replace '-', '_'
    $arguments = New-Object 'System.Collections.Generic.List[string]'

    switch ($normalizedOp) {
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
            $arguments.Add((ConvertTo-NativeCliQuotedArgument -Value $xml)) | Out-Null
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
            $arguments.Add((ConvertTo-NativeCliQuotedArgument -Value $xml)) | Out-Null
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
        "delete_omml" {
            $formulaIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("formula_index", "omml_index", "index") `
                -Label $Label
            $arguments.Add("remove-omml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($formulaIndex) | Out-Null
        }
        "append_insertion_revision" {
            $arguments.Add("append-insertion-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "append_deletion_revision" {
            $arguments.Add("append-deletion-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "insert_run_revision_after" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $runIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("run_index", "run") `
                -Label $Label
            $arguments.Add("insert-run-revision-after") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "delete_run_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $runIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("run_index", "run") `
                -Label $Label
            $arguments.Add("delete-run-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "replace_run_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $runIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("run_index", "run") `
                -Label $Label
            $arguments.Add("replace-run-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($runIndex) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "insert_paragraph_text_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $arguments.Add("insert-paragraph-text-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "delete_paragraph_text_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $textLength = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_length", "length") `
                -Label $Label
            $arguments.Add("delete-paragraph-text-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            $arguments.Add($textLength) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowExpectedText
        }
        "replace_paragraph_text_revision" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $textLength = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_length", "length") `
                -Label $Label
            $arguments.Add("replace-paragraph-text-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            $arguments.Add($textLength) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText `
                -AllowExpectedText
        }
        "insert_text_range_revision" {
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index", "paragraph") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset", "text_offset", "offset") `
                -Label $Label
            $arguments.Add("insert-text-range-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText
        }
        "delete_text_range_revision" {
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset") `
                -Label $Label
            $endParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_paragraph_index") `
                -Label $Label
            $endTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_text_offset", "end_offset") `
                -Label $Label
            $arguments.Add("delete-text-range-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            $arguments.Add($endParagraphIndex) | Out-Null
            $arguments.Add($endTextOffset) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowExpectedText
        }
        "replace_text_range_revision" {
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset") `
                -Label $Label
            $endParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_paragraph_index") `
                -Label $Label
            $endTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_text_offset", "end_offset") `
                -Label $Label
            $arguments.Add("replace-text-range-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            $arguments.Add($endParagraphIndex) | Out-Null
            $arguments.Add($endTextOffset) | Out-Null
            Add-RevisionAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireText `
                -AllowExpectedText
        }
        "accept_revision" {
            $revisionIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("revision_index", "index") `
                -Label $Label
            $arguments.Add("accept-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($revisionIndex) | Out-Null
        }
        "reject_revision" {
            $revisionIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("revision_index", "index") `
                -Label $Label
            $arguments.Add("reject-revision") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($revisionIndex) | Out-Null
        }
        "set_revision_metadata" {
            $revisionIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("revision_index", "index") `
                -Label $Label
            $arguments.Add("set-revision-metadata") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($revisionIndex) | Out-Null
            Add-RevisionMetadataArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "append_comment" {
            $arguments.Add("append-comment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-CommentAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireSelectedText `
                -RequireCommentText
        }
        "append_comment_reply" {
            $parentCommentIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("parent_comment_index", "comment_index", "parent_index", "index") `
                -Label $Label
            $arguments.Add("append-comment-reply") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($parentCommentIndex) | Out-Null
            Add-CommentAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireCommentText
        }
        "append_footnote" {
            $arguments.Add("append-footnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireReferenceText `
                -RequireNoteText
        }
        "replace_footnote" {
            $noteIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("note_index", "footnote_index", "index") `
                -Label $Label
            $arguments.Add("replace-footnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($noteIndex) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireNoteText
        }
        "remove_footnote" {
            $noteIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("note_index", "footnote_index", "index") `
                -Label $Label
            $arguments.Add("remove-footnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($noteIndex) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "append_endnote" {
            $arguments.Add("append-endnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireReferenceText `
                -RequireNoteText
        }
        "replace_endnote" {
            $noteIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("note_index", "endnote_index", "index") `
                -Label $Label
            $arguments.Add("replace-endnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($noteIndex) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireNoteText
        }
        "remove_endnote" {
            $noteIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("note_index", "endnote_index", "index") `
                -Label $Label
            $arguments.Add("remove-endnote") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($noteIndex) | Out-Null
            Add-ReviewNoteMutationArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "append_paragraph_text_comment" {
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $textLength = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_length", "length") `
                -Label $Label
            $arguments.Add("append-paragraph-text-comment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            $arguments.Add($textLength) | Out-Null
            Add-CommentAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireCommentText
        }
        "append_text_range_comment" {
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset") `
                -Label $Label
            $endParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_paragraph_index") `
                -Label $Label
            $endTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_text_offset", "end_offset") `
                -Label $Label
            $arguments.Add("append-text-range-comment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            $arguments.Add($endParagraphIndex) | Out-Null
            $arguments.Add($endTextOffset) | Out-Null
            Add-CommentAuthoringArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -RequireCommentText
        }
        "set_paragraph_text_comment_range" {
            $commentIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("comment_index", "index") `
                -Label $Label
            $paragraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("paragraph_index", "paragraph") `
                -Label $Label
            $textOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_offset", "offset") `
                -Label $Label
            $textLength = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text_length", "length") `
                -Label $Label
            $arguments.Add("set-paragraph-text-comment-range") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($commentIndex) | Out-Null
            $arguments.Add($paragraphIndex) | Out-Null
            $arguments.Add($textOffset) | Out-Null
            $arguments.Add($textLength) | Out-Null
        }
        "set_text_range_comment_range" {
            $commentIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("comment_index", "index") `
                -Label $Label
            $startParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_paragraph_index", "paragraph_index") `
                -Label $Label
            $startTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("start_text_offset", "start_offset") `
                -Label $Label
            $endParagraphIndex = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_paragraph_index") `
                -Label $Label
            $endTextOffset = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("end_text_offset", "end_offset") `
                -Label $Label
            $arguments.Add("set-text-range-comment-range") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($commentIndex) | Out-Null
            $arguments.Add($startParagraphIndex) | Out-Null
            $arguments.Add($startTextOffset) | Out-Null
            $arguments.Add($endParagraphIndex) | Out-Null
            $arguments.Add($endTextOffset) | Out-Null
        }
        "append_page_number_field" {
            $arguments.Add("append-page-number-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowFieldState
        }
        "append_total_pages_field" {
            $arguments.Add("append-total-pages-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowFieldState
        }
        "append_table_of_contents_field" {
            $arguments.Add("append-table-of-contents-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowTableOfContentsOptions `
                -AllowFieldState
        }
        "append_document_property_field" {
            $propertyName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("property_name", "property", "name") `
                -Label $Label
            $arguments.Add("append-document-property-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($propertyName) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
        }
        "append_date_field" {
            $arguments.Add("append-date-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowDateFormat `
                -AllowPreserveFormatting `
                -AllowFieldState
        }
        "append_page_reference_field" {
            $bookmarkName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("bookmark_name", "bookmark", "name") `
                -Label $Label
            $arguments.Add("append-page-reference-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmarkName) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState

            $hasNoHyperlink = Test-ObjectPropertyExists -Object $Operation -Name "no_hyperlink"
            $hasHyperlink = Test-ObjectPropertyExists -Object $Operation -Name "hyperlink"
            $noHyperlink = $false
            if ($hasNoHyperlink) {
                $noHyperlink = Get-OptionalBooleanPropertyValue `
                    -Object $Operation `
                    -Name "no_hyperlink" `
                    -DefaultValue $false
            }
            if ($hasHyperlink) {
                $hyperlink = Get-OptionalBooleanPropertyValue `
                    -Object $Operation `
                    -Name "hyperlink" `
                    -DefaultValue $true
                if ($hasNoHyperlink -and ($noHyperlink -eq $hyperlink)) {
                    throw "$Label cannot combine contradictory 'no_hyperlink' and 'hyperlink' values."
                }
                if (-not $hyperlink) {
                    $noHyperlink = $true
                }
            }
            if ($noHyperlink) {
                $arguments.Add("--no-hyperlink") | Out-Null
            }
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("relative_position") `
                -Flag "--relative-position"
        }
        "append_style_reference_field" {
            $styleName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("style_name", "style", "name") `
                -Label $Label
            $arguments.Add("append-style-reference-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($styleName) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("paragraph_number") `
                -Flag "--paragraph-number"
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("relative_position") `
                -Flag "--relative-position"
        }
        "append_hyperlink_field" {
            $target = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("target", "url", "href") `
                -Label $Label
            $arguments.Add("append-hyperlink-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($target) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("anchor", "bookmark", "bookmark_name") `
                -Flag "--anchor"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("tooltip", "screen_tip") `
                -Flag "--tooltip"
        }
        "append_caption" {
            $labelName = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("label", "caption_label", "name") `
                -Label $Label
            $captionText = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("caption_text", "text") `
                -Label $Label
            $arguments.Add("append-caption") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($labelName) | Out-Null
            Add-AppendFieldPartSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation
            $arguments.Add("--text") | Out-Null
            $arguments.Add($captionText) | Out-Null
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("number_result_text", "number_result") `
                -Flag "--number-result"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("number_format", "format") `
                -Flag "--number-format"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("restart", "restart_value") `
                -Flag "--restart"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("separator") `
                -Flag "--separator"
            Add-AppendFieldStateArguments `
                -Arguments $arguments `
                -Operation $Operation
            Add-AppendFieldPreserveFormattingArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "append_index_entry_field" {
            $entryText = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("entry_text", "entry", "text") `
                -Label $Label
            $arguments.Add("append-index-entry-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($entryText) | Out-Null
            Add-AppendFieldPartSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("subentry") `
                -Flag "--subentry"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("bookmark_name", "bookmark") `
                -Flag "--bookmark"
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("cross_reference", "crossref") `
                -Flag "--cross-reference"
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("bold_page_number") `
                -Flag "--bold-page-number"
            $null = Add-OptionalCliFlagArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("italic_page_number") `
                -Flag "--italic-page-number"
            Add-AppendFieldStateArguments `
                -Arguments $arguments `
                -Operation $Operation
        }
        "append_index_field" {
            $arguments.Add("append-index-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -AllowResultText `
                -AllowPreserveFormatting `
                -AllowFieldState
            $null = Add-OptionalCliValueArgument `
                -Arguments $arguments `
                -Operation $Operation `
                -Names @("columns", "column_count") `
                -Flag "--columns"
        }
        "append_complex_field" {
            $instruction = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("instruction")
            $instructionBefore = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("instruction_before")
            $nestedInstruction = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("nested_instruction")
            $nestedResultText = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("nested_result_text")
            $instructionAfter = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("instruction_after")
            $hasInstruction = -not [string]::IsNullOrWhiteSpace($instruction)
            $hasNestedInstruction = -not [string]::IsNullOrWhiteSpace($nestedInstruction)
            $hasNestedInstructionParts = `
                (-not [string]::IsNullOrWhiteSpace($instructionBefore)) -or `
                $hasNestedInstruction -or `
                (-not [string]::IsNullOrWhiteSpace($nestedResultText)) -or `
                (-not [string]::IsNullOrWhiteSpace($instructionAfter))

            if ($hasInstruction -and $hasNestedInstructionParts) {
                throw "$Label cannot combine 'instruction' with nested instruction properties."
            }
            if ((-not $hasInstruction) -and (-not $hasNestedInstruction)) {
                throw "$Label must provide 'instruction' or 'nested_instruction'."
            }
            if ($hasNestedInstruction) {
                if ([string]::IsNullOrWhiteSpace($instructionBefore) -or
                    [string]::IsNullOrWhiteSpace($instructionAfter)) {
                    throw "$Label nested instruction mode requires 'instruction_before' and 'instruction_after'."
                }
            }

            $arguments.Add("append-complex-field") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-AppendFieldPartSelectionArguments `
                -Arguments $arguments `
                -Operation $Operation
            if ($hasInstruction) {
                $arguments.Add("--instruction") | Out-Null
                $arguments.Add($instruction) | Out-Null
            } else {
                $arguments.Add("--instruction-before") | Out-Null
                $arguments.Add($instructionBefore) | Out-Null
                $arguments.Add("--nested-instruction") | Out-Null
                $arguments.Add($nestedInstruction) | Out-Null
                if (-not [string]::IsNullOrWhiteSpace($nestedResultText)) {
                    $arguments.Add("--nested-result-text") | Out-Null
                    $arguments.Add($nestedResultText) | Out-Null
                }
                $arguments.Add("--instruction-after") | Out-Null
                $arguments.Add($instructionAfter) | Out-Null
            }
            Add-AppendFieldResultTextArgument `
                -Arguments $arguments `
                -Operation $Operation
            Add-AppendFieldStateArguments `
                -Arguments $arguments `
                -Operation $Operation
        }
        "set_update_fields_on_open" {
            $enabled = Get-UpdateFieldsOnOpenEnabledValue `
                -Operation $Operation `
                -Label $Label
            $arguments.Add("set-update-fields-on-open") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($(if ($enabled) { "--enable" } else { "--disable" })) | Out-Null
        }
        "enable_update_fields_on_open" {
            $enabled = Get-UpdateFieldsOnOpenEnabledValue `
                -Operation $Operation `
                -Label $Label `
                -DefaultValue $true
            $arguments.Add("set-update-fields-on-open") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($(if ($enabled) { "--enable" } else { "--disable" })) | Out-Null
        }
        "disable_update_fields_on_open" {
            $enabled = Get-UpdateFieldsOnOpenEnabledValue `
                -Operation $Operation `
                -Label $Label `
                -DefaultValue $false
            $arguments.Add("set-update-fields-on-open") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($(if ($enabled) { "--enable" } else { "--disable" })) | Out-Null
        }
        "clear_update_fields_on_open" {
            $enabled = Get-UpdateFieldsOnOpenEnabledValue `
                -Operation $Operation `
                -Label $Label `
                -DefaultValue $false
            $arguments.Add("set-update-fields-on-open") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($(if ($enabled) { "--enable" } else { "--disable" })) | Out-Null
        }
        "replace_content_control_text" {
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $arguments.Add("replace-content-control-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--text") | Out-Null
            $arguments.Add($text) | Out-Null
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_text_by_tag" {
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $arguments.Add("replace-content-control-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--text") | Out-Null
            $arguments.Add($text) | Out-Null
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_text_by_alias" {
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $arguments.Add("replace-content-control-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            $arguments.Add("--text") | Out-Null
            $arguments.Add($text) | Out-Null
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_paragraphs" {
            $arguments.Add("replace-content-control-paragraphs") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-ContentControlParagraphFileArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_table" {
            $arguments.Add("replace-content-control-table") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-ContentControlTableRowFileArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_table_rows" {
            $arguments.Add("replace-content-control-table-rows") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-ContentControlTableRowFileArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -AllowEmptyRows
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "replace_content_control_image" {
            $imagePath = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("image_path", "image", "path", "file") `
                -Label $Label
            $arguments.Add("replace-content-control-image") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($imagePath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
            $width = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("width", "width_px")
            if (-not [string]::IsNullOrWhiteSpace($width)) {
                $arguments.Add("--width") | Out-Null
                $arguments.Add($width) | Out-Null
            }
            $height = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("height", "height_px")
            if (-not [string]::IsNullOrWhiteSpace($height)) {
                $arguments.Add("--height") | Out-Null
                $arguments.Add($height) | Out-Null
            }
        }
        "set_content_control_form_state" {
            $arguments.Add("set-content-control-form-state") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            Add-ContentControlSelectorArguments -Arguments $arguments -Operation $Operation -Label $Label
            $optionCount = Add-ContentControlFormStateArguments -Arguments $arguments -Operation $Operation
            if ($optionCount -eq 0) {
                throw "$Label must provide at least one content-control form-state option."
            }
            Add-ContentControlPartSelectionArguments -Arguments $arguments -Operation $Operation
        }
        "sync_content_controls_from_custom_xml" {
            $arguments.Add("sync-content-controls-from-custom-xml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "sync_content_control_from_custom_xml" {
            $arguments.Add("sync-content-controls-from-custom-xml") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "accept_all_revisions" {
            $arguments.Add("accept-all-revisions") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "reject_all_revisions" {
            $arguments.Add("reject-all-revisions") | Out-Null
            $arguments.Add($InputPath) | Out-Null
        }
        "set_comment_resolved" {
            Add-ReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        "set_comment_metadata" {
            Add-ReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        "replace_comment" {
            Add-ReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
        "remove_comment" {
            Add-ReviewMutationPlanArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -InputPath $InputPath `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
        }
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
                [pscustomobject]@{ Name = "vertical_offset"; Flag = "--vertical-offset" },
                [pscustomobject]@{ Name = "vertical_offset_twips"; Flag = "--vertical-offset" },
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
        "set_section_page_setup" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("set-section-page-setup") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $null = Add-SectionPageSetupArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "insert_section" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("insert-section") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $noInherit = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "no_inherit" `
                -DefaultValue $false
            $inheritHeaderFooter = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "inherit_header_footer" `
                -DefaultValue $true
            if ($noInherit -or (-not $inheritHeaderFooter)) {
                $arguments.Add("--no-inherit") | Out-Null
            }
        }
        "remove_section" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("remove-section") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
        }
        "delete_section" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("remove-section") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
        }
        "move_section" {
            $sourceIndex = Get-SourceIndexValue -Operation $Operation -Label $Label
            $targetIndex = Get-TargetIndexValue -Operation $Operation -Label $Label
            $arguments.Add("move-section") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceIndex) | Out-Null
            $arguments.Add($targetIndex) | Out-Null
        }
        "copy_section_layout" {
            $sourceIndex = Get-SourceIndexValue -Operation $Operation -Label $Label
            $targetIndex = Get-TargetIndexValue -Operation $Operation -Label $Label
            $arguments.Add("copy-section-layout") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceIndex) | Out-Null
            $arguments.Add($targetIndex) | Out-Null
        }
        "set_section_header" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("set-section-header") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            Add-SectionTextArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "set_section_footer" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("set-section-footer") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            Add-SectionTextArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "assign_section_header" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $partIndex = Get-SectionPartIndexValue `
                -Operation $Operation `
                -Label $Label `
                -Family "header"
            $arguments.Add("assign-section-header") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $arguments.Add($partIndex) | Out-Null
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "assign_section_footer" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $partIndex = Get-SectionPartIndexValue `
                -Operation $Operation `
                -Label $Label `
                -Family "footer"
            $arguments.Add("assign-section-footer") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $arguments.Add($partIndex) | Out-Null
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "remove_section_header" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("remove-section-header") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "remove_section_footer" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("remove-section-footer") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "remove_header_part" {
            $partIndex = Get-SectionPartIndexValue `
                -Operation $Operation `
                -Label $Label `
                -Family "header"
            $arguments.Add("remove-header-part") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($partIndex) | Out-Null
        }
        "remove_footer_part" {
            $partIndex = Get-SectionPartIndexValue `
                -Operation $Operation `
                -Label $Label `
                -Family "footer"
            $arguments.Add("remove-footer-part") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($partIndex) | Out-Null
        }
        "move_header_part" {
            $sourceIndex = Get-SourceIndexValue -Operation $Operation -Label $Label
            $targetIndex = Get-TargetIndexValue -Operation $Operation -Label $Label
            $arguments.Add("move-header-part") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceIndex) | Out-Null
            $arguments.Add($targetIndex) | Out-Null
        }
        "move_footer_part" {
            $sourceIndex = Get-SourceIndexValue -Operation $Operation -Label $Label
            $targetIndex = Get-TargetIndexValue -Operation $Operation -Label $Label
            $arguments.Add("move-footer-part") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceIndex) | Out-Null
            $arguments.Add($targetIndex) | Out-Null
        }
        default {
            throw "$Label has unsupported op '$op'."
        }
    }

    $arguments.Add("--output") | Out-Null
    $arguments.Add($OutputPath) | Out-Null
    $arguments.Add("--json") | Out-Null

    return [pscustomobject]@{
        Operation = $normalizedOp
        Arguments = @($arguments.ToArray())
        Command = [string]$arguments[0]
        DirectOperation = $false
    }
}

function New-OperationRecord {
    param(
        [int]$Index,
        [string]$Op,
        [string]$Command,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Status,
        [int]$ExitCode,
        [string[]]$Output,
        [string]$ErrorMessage
    )

    $record = [ordered]@{
        index = $Index
        op = $Op
        command = $Command
        input_docx = $InputPath
        output_docx = $OutputPath
        status = $Status
        exit_code = $ExitCode
        output = @($Output)
    }
    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $record.error = $ErrorMessage
    }

    return $record
}

function Build-EditSummary {
    param(
        [string]$Status,
        [string]$InputDocx,
        [string]$EditPlan,
        [string]$OutputDocx,
        [string]$SummaryJson,
        [string]$CliPath,
        [string]$BuildDir,
        [string]$Generator,
        [bool]$SkipBuild,
        [object[]]$Operations,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        edit_plan = $EditPlan
        output_docx = $OutputDocx
        summary_json = $SummaryJson
        cli_path = $CliPath
        build_dir = $BuildDir
        generator = $Generator
        skip_build = $SkipBuild
        operation_count = @($Operations).Count
        operations = @($Operations)
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedEditPlan = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $EditPlan
$resolvedOutputDocx = Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $OutputDocx
$resolvedSummaryJson = Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $BuildDir
}

Ensure-PathParent -Path $resolvedOutputDocx
Ensure-PathParent -Path $resolvedSummaryJson

$planObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedEditPlan | ConvertFrom-Json
$operations = @(Get-EditPlanOperations -Plan $planObject)
if ($operations.Count -eq 0) {
    throw "Edit plan must contain at least one operation."
}

$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "featherdoc-edit-document-" + [System.Guid]::NewGuid().ToString("N")
)
$operationRecords = New-Object 'System.Collections.Generic.List[object]'
$status = "failed"
$failureMessage = ""
$cliPath = ""

New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null

try {
    Write-Step "Resolving featherdoc_cli"
    $cliPath = Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $resolvedBuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild

    $currentInput = $resolvedInputDocx
    for ($index = 0; $index -lt $operations.Count; ++$index) {
        $operationIndex = $index + 1
        $isLast = ($operationIndex -eq $operations.Count)
        $stepOutput = if ($isLast) {
            $resolvedOutputDocx
        } else {
            Join-Path $temporaryRoot ("edit-step-{0}.docx" -f $operationIndex)
        }

        $label = "operation[$operationIndex]"
        $operationInfo = New-OperationArguments `
            -Operation $operations[$index] `
            -InputPath $currentInput `
            -OutputPath $stepOutput `
            -Label $label `
            -OperationIndex $operationIndex `
            -TemporaryRoot $temporaryRoot

        Write-Step "Applying $($operationInfo.Operation)"
        if ($operationInfo.DirectOperation) {
            try {
                $directOperationKind = Get-OptionalObjectPropertyValue `
                    -Object $operationInfo `
                    -Name "DirectOperationKind"
                switch ($directOperationKind) {
                    "table-cell-horizontal-alignment" {
                        $directResult = Set-DocxTableCellHorizontalAlignment `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -RowIndex $operationInfo.RowIndex `
                            -CellIndex $operationInfo.CellIndex `
                            -Alignment $operationInfo.HorizontalAlignment `
                            -Clear $operationInfo.ClearHorizontalAlignment
                    }
                    "body-table-column-width" {
                        $directResult = Set-DocxTableColumnWidth `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -ColumnIndex $operationInfo.ColumnIndex `
                            -WidthTwips $operationInfo.WidthTwips `
                            -Clear $operationInfo.ClearColumnWidth
                    }
                    "text-style" {
                        $directResult = Set-DocxParagraphTextStyle `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -RowIndex $operationInfo.RowIndex `
                            -CellIndex $operationInfo.CellIndex `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -BoldSpecified $operationInfo.BoldSpecified `
                            -Bold $operationInfo.Bold `
                            -FontFamily $operationInfo.FontFamily `
                            -EastAsiaFontFamily $operationInfo.EastAsiaFontFamily `
                            -Language $operationInfo.Language `
                            -EastAsiaLanguage $operationInfo.EastAsiaLanguage `
                            -Color $operationInfo.Color `
                            -FontSizeHalfPoints $operationInfo.FontSizeHalfPoints
                    }
                    "body-paragraph-deletion" {
                        $directResult = Remove-DocxBodyParagraphs `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains
                    }
                    "text-replacement" {
                        $directResult = Set-DocxTextReplacement `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -FindText $operationInfo.FindText `
                            -ReplacementText $operationInfo.ReplacementText `
                            -MatchCase $operationInfo.MatchCase
                    }
                    "body-paragraph-horizontal-alignment" {
                        $directResult = Set-DocxBodyParagraphHorizontalAlignment `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -Alignment $operationInfo.HorizontalAlignment `
                            -Clear $operationInfo.ClearHorizontalAlignment
                    }
                    "body-paragraph-spacing" {
                        $directResult = Set-DocxBodyParagraphSpacing `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -BeforeTwips $operationInfo.BeforeTwips `
                            -AfterTwips $operationInfo.AfterTwips `
                            -LineTwips $operationInfo.LineTwips `
                            -LineRule $operationInfo.LineRule `
                            -Clear $operationInfo.ClearSpacing
                    }
                    default {
                        throw "Unsupported direct operation kind '$directOperationKind'."
                    }
                }
                $result = [pscustomobject]@{
                    ExitCode = 0
                    Text = ""
                    Output = @(($directResult | ConvertTo-Json -Compress))
                }
            } catch {
                $result = [pscustomobject]@{
                    ExitCode = 1
                    Text = $_.Exception.Message
                    Output = @($_.Exception.Message)
                }
            }
        } else {
            $result = Invoke-TemplateSchemaCli `
                -ExecutablePath $cliPath `
                -Arguments $operationInfo.Arguments
        }

        if ($result.ExitCode -ne 0) {
            $message = if ([string]::IsNullOrWhiteSpace($result.Text)) {
                "$label failed."
            } else {
                "$label failed: $($result.Text)"
            }
            $operationRecords.Add([pscustomobject](New-OperationRecord `
                    -Index $operationIndex `
                    -Op $operationInfo.Operation `
                    -Command $operationInfo.Command `
                    -InputPath $currentInput `
                    -OutputPath $stepOutput `
                    -Status "failed" `
                    -ExitCode $result.ExitCode `
                    -Output $result.Output `
                    -ErrorMessage $message)) | Out-Null
            throw $message
        }
        if (-not (Test-Path -LiteralPath $stepOutput)) {
            $message = "$label did not produce '$stepOutput'."
            $operationRecords.Add([pscustomobject](New-OperationRecord `
                    -Index $operationIndex `
                    -Op $operationInfo.Operation `
                    -Command $operationInfo.Command `
                    -InputPath $currentInput `
                    -OutputPath $stepOutput `
                    -Status "failed" `
                    -ExitCode $result.ExitCode `
                    -Output $result.Output `
                    -ErrorMessage $message)) | Out-Null
            throw $message
        }

        $operationRecords.Add([pscustomobject](New-OperationRecord `
                -Index $operationIndex `
                -Op $operationInfo.Operation `
                -Command $operationInfo.Command `
                -InputPath $currentInput `
                -OutputPath $stepOutput `
                -Status "completed" `
                -ExitCode $result.ExitCode `
                -Output $result.Output `
                -ErrorMessage "")) | Out-Null
        $currentInput = $stepOutput
    }

    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    Write-Error $failureMessage
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-EditSummary `
            -Status $status `
            -InputDocx $resolvedInputDocx `
            -EditPlan $resolvedEditPlan `
            -OutputDocx $resolvedOutputDocx `
            -SummaryJson $resolvedSummaryJson `
            -CliPath $cliPath `
            -BuildDir $resolvedBuildDir `
            -Generator $Generator `
            -SkipBuild ([bool]$SkipBuild) `
            -Operations @($operationRecords.ToArray()) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }

    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

if ($status -ne "completed") {
    exit 1
}

Write-Step "Edited DOCX: $resolvedOutputDocx"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
