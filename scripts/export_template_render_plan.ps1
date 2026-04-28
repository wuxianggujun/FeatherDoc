<#
.SYNOPSIS
Exports a render-plan draft from a DOCX template.

.DESCRIPTION
Builds or reuses `featherdoc_cli`, inspects body/header/footer template parts,
classifies bookmarks by kind, and writes a JSON render-plan draft that can be
edited and then fed back into `render_template_document.ps1`. By default,
header/footer bookmarks target loaded part indexes; use `-TargetMode
resolved-section-targets` when the render plan should target section-specific
header/footer references instead.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\export_template_render_plan.ps1 `
    -InputDocx .\samples\chinese_invoice_template.docx `
    -OutputPlan .\output\rendered\invoice.render-plan.json `
    -SummaryJson .\output\rendered\invoice.render-plan.summary.json `
    -BuildDir build-codex-clang-compat `
    -SkipBuild
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [string]$OutputPlan = "",
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [ValidateSet("loaded-parts", "resolved-section-targets")]
    [string]$TargetMode = "loaded-parts",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[export-template-render-plan] $Message"
}

function Resolve-OptionalExportPath {
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
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Convert-CliOutputToJsonObject {
    param(
        [string[]]$Lines,
        [string]$Command
    )

    $jsonLine = $Lines |
        Where-Object {
            $trimmed = $_.TrimStart()
            $trimmed.StartsWith("{") -or $trimmed.StartsWith("[")
        } |
        Select-Object -Last 1

    if ([string]::IsNullOrWhiteSpace([string]$jsonLine)) {
        throw "Command '$Command' did not emit a JSON payload."
    }

    return $jsonLine | ConvertFrom-Json
}

function Invoke-TemplateSchemaJsonCommand {
    param(
        [string]$ExecutablePath,
        [string]$Command,
        [string[]]$Arguments
    )

    $result = Invoke-TemplateSchemaCli -ExecutablePath $ExecutablePath -Arguments $Arguments
    if ($result.ExitCode -ne 0) {
        $joined = if ($result.Output.Count -gt 0) {
            $result.Output -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "$Command failed with exit code $($result.ExitCode). Output:`n$joined"
    }

    return [pscustomobject]@{
        Raw = $result
        Json = Convert-CliOutputToJsonObject -Lines $result.Output -Command $Command
    }
}

function New-SelectionObject {
    param(
        [string]$Part,
        [Nullable[int]]$Index,
        [Nullable[int]]$Section,
        [string]$Kind = ""
    )

    $selection = [ordered]@{
        part = $Part
    }

    if ($null -ne $Index) {
        $selection.index = [int]$Index
    }
    if ($null -ne $Section) {
        $selection.section = [int]$Section
    }
    if (-not [string]::IsNullOrWhiteSpace($Kind)) {
        $selection.kind = $Kind
    }

    return $selection
}

function New-RenderPlanEntry {
    param(
        $Selection,
        [string]$BookmarkName
    )

    $entry = [ordered]@{
        bookmark_name = $BookmarkName
        part = [string]$Selection.part
    }

    $indexValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "index"
    if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
        $entry.index = [int]$indexValue
    }

    $sectionValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "section"
    if (-not [string]::IsNullOrWhiteSpace($sectionValue)) {
        $entry.section = [int]$sectionValue
    }

    $kindValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "kind"
    if (-not [string]::IsNullOrWhiteSpace($kindValue)) {
        $entry.kind = $kindValue
    }

    return $entry
}

function Add-UnsupportedBookmark {
    param(
        [System.Collections.Generic.List[object]]$UnsupportedBookmarks,
        $Selection,
        $Bookmark,
        [string]$Reason
    )

    $entry = New-RenderPlanEntry -Selection $Selection -BookmarkName ([string]$Bookmark.bookmark_name)
    $entry.kind = [string]$Bookmark.kind
    $entry.occurrence_count = [int]$Bookmark.occurrence_count
    $entry.reason = $Reason
    $UnsupportedBookmarks.Add([pscustomobject]$entry) | Out-Null
}

function Add-RenderPlanEntriesFromInspection {
    param(
        $Inspection,
        $Selection,
        [System.Collections.Generic.List[object]]$BookmarkTextEntries,
        [System.Collections.Generic.List[object]]$BookmarkParagraphEntries,
        [System.Collections.Generic.List[object]]$BookmarkTableRowEntries,
        [System.Collections.Generic.List[object]]$BookmarkVisibilityEntries,
        [System.Collections.Generic.List[object]]$UnsupportedBookmarks
    )

    foreach ($bookmark in (Get-OptionalObjectArrayProperty -Object $Inspection -Name "bookmarks")) {
        $bookmarkName = [string](Get-OptionalObjectPropertyValue -Object $bookmark -Name "bookmark_name")
        $bookmarkKind = [string](Get-OptionalObjectPropertyValue -Object $bookmark -Name "kind")
        if ([string]::IsNullOrWhiteSpace($bookmarkName)) {
            continue
        }

        switch ($bookmarkKind) {
            "text" {
                $entry = New-RenderPlanEntry -Selection $Selection -BookmarkName $bookmarkName
                $entry.text = "TODO: $bookmarkName"
                $BookmarkTextEntries.Add([pscustomobject]$entry) | Out-Null
            }
            "block" {
                $entry = New-RenderPlanEntry -Selection $Selection -BookmarkName $bookmarkName
                $entry.paragraphs = @("TODO: $bookmarkName")
                $BookmarkParagraphEntries.Add([pscustomobject]$entry) | Out-Null
            }
            "table_rows" {
                $entry = New-RenderPlanEntry -Selection $Selection -BookmarkName $bookmarkName
                $entry.rows = @()
                $BookmarkTableRowEntries.Add([pscustomobject]$entry) | Out-Null
            }
            "block_range" {
                $entry = New-RenderPlanEntry -Selection $Selection -BookmarkName $bookmarkName
                $entry.visible = $true
                $BookmarkVisibilityEntries.Add([pscustomobject]$entry) | Out-Null
            }
            default {
                Add-UnsupportedBookmark `
                    -UnsupportedBookmarks $UnsupportedBookmarks `
                    -Selection $Selection `
                    -Bookmark $bookmark `
                    -Reason "bookmark kind cannot be exported as a render plan entry"
            }
        }
    }
}

function Build-RenderPlanExportSummary {
    param(
        [string]$Status,
        [string]$InputDocx,
        [string]$OutputPlan,
        [string]$SummaryJsonPath,
        [string]$CliPath,
        [string]$ResolvedBuildDir,
        [string]$TargetMode,
        [object[]]$InspectedParts,
        [object[]]$UnsupportedBookmarks,
        [object]$Plan
    )

    return [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        output_plan = $OutputPlan
        summary_json = $SummaryJsonPath
        cli_path = $CliPath
        build_dir = $ResolvedBuildDir
        target_mode = $TargetMode
        plan_counts = [ordered]@{
            bookmark_text = @($Plan.bookmark_text).Count
            bookmark_paragraphs = @($Plan.bookmark_paragraphs).Count
            bookmark_table_rows = @($Plan.bookmark_table_rows).Count
            bookmark_block_visibility = @($Plan.bookmark_block_visibility).Count
        }
        inspected_part_count = $InspectedParts.Count
        inspected_parts = $InspectedParts
        unsupported_bookmark_count = $UnsupportedBookmarks.Count
        unsupported_bookmarks = $UnsupportedBookmarks
    }
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedOutputPlan = if ([string]::IsNullOrWhiteSpace($OutputPlan)) {
    $inputDirectory = [System.IO.Path]::GetDirectoryName($resolvedInputDocx)
    $inputStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedInputDocx)
    Join-Path $inputDirectory ($inputStem + ".render-plan.json")
} else {
    Resolve-OptionalExportPath -RepoRoot $repoRoot -InputPath $OutputPlan
}
$resolvedSummaryJson = Resolve-OptionalExportPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalExportPath -RepoRoot $repoRoot -InputPath $BuildDir
}
$schemaPath = Join-Path $repoRoot "samples\template_render_plan.schema.json"
$outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedOutputPlan)
if ([string]::IsNullOrWhiteSpace($outputDirectory)) {
    $outputDirectory = $repoRoot
}
$schemaReference = Get-RelativePathString -BasePath $outputDirectory -TargetPath $schemaPath
if (-not ($schemaReference.StartsWith("./") -or $schemaReference.StartsWith("../"))) {
    $schemaReference = "./" + $schemaReference
}

Ensure-PathParent -Path $resolvedOutputPlan
Ensure-PathParent -Path $resolvedSummaryJson

$cliPath = ""
$status = "failed"
$plan = [ordered]@{
    '$schema' = $schemaReference
    bookmark_text = @()
    bookmark_paragraphs = @()
    bookmark_table_rows = @()
    bookmark_block_visibility = @()
}
$inspectedParts = New-Object 'System.Collections.Generic.List[object]'
$unsupportedBookmarks = New-Object 'System.Collections.Generic.List[object]'

try {
    Write-Step "Resolving featherdoc_cli"
    $cliPath = Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $BuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild

    $bookmarkTextEntries = New-Object 'System.Collections.Generic.List[object]'
    $bookmarkParagraphEntries = New-Object 'System.Collections.Generic.List[object]'
    $bookmarkTableRowEntries = New-Object 'System.Collections.Generic.List[object]'
    $bookmarkVisibilityEntries = New-Object 'System.Collections.Generic.List[object]'

    $partSelections = New-Object 'System.Collections.Generic.List[object]'
    $partSelections.Add((New-SelectionObject -Part "body" -Index $null -Section $null)) | Out-Null

    if ($TargetMode -eq "resolved-section-targets") {
        Write-Step "Inspecting section header/footer references"
        $sectionsResult = Invoke-TemplateSchemaJsonCommand `
            -ExecutablePath $cliPath `
            -Command "inspect-sections" `
            -Arguments @("inspect-sections", $resolvedInputDocx, "--json")
        foreach ($sectionLayout in (Get-OptionalObjectArrayProperty -Object $sectionsResult.Json -Name "section_layouts" |
                Sort-Object { [int](Get-OptionalObjectPropertyValue -Object $_ -Name "index") })) {
            $sectionIndex = [int](Get-OptionalObjectPropertyValue -Object $sectionLayout -Name "index")
            foreach ($family in @("header", "footer")) {
                $references = Get-OptionalObjectPropertyObject -Object $sectionLayout -Name $family
                foreach ($kind in @("default", "first", "even")) {
                    $isPresent = Get-OptionalObjectPropertyObject -Object $references -Name $kind
                    if ($null -ne $isPresent -and [bool]$isPresent) {
                        $partName = if ($family -eq "header") { "section-header" } else { "section-footer" }
                        $partSelections.Add((New-SelectionObject `
                                    -Part $partName `
                                    -Index $null `
                                    -Section $sectionIndex `
                                    -Kind $kind)) | Out-Null
                    }
                }
            }
        }
    } else {
        Write-Step "Inspecting header parts"
        $headerPartsResult = Invoke-TemplateSchemaJsonCommand `
            -ExecutablePath $cliPath `
            -Command "inspect-header-parts" `
            -Arguments @("inspect-header-parts", $resolvedInputDocx, "--json")
        foreach ($part in (Get-OptionalObjectArrayProperty -Object $headerPartsResult.Json -Name "parts" |
                Sort-Object { [int](Get-OptionalObjectPropertyValue -Object $_ -Name "index") })) {
            $headerIndex = [int](Get-OptionalObjectPropertyValue -Object $part -Name "index")
            $partSelections.Add((New-SelectionObject -Part "header" -Index $headerIndex -Section $null)) | Out-Null
        }

        Write-Step "Inspecting footer parts"
        $footerPartsResult = Invoke-TemplateSchemaJsonCommand `
            -ExecutablePath $cliPath `
            -Command "inspect-footer-parts" `
            -Arguments @("inspect-footer-parts", $resolvedInputDocx, "--json")
        foreach ($part in (Get-OptionalObjectArrayProperty -Object $footerPartsResult.Json -Name "parts" |
                Sort-Object { [int](Get-OptionalObjectPropertyValue -Object $_ -Name "index") })) {
            $footerIndex = [int](Get-OptionalObjectPropertyValue -Object $part -Name "index")
            $partSelections.Add((New-SelectionObject -Part "footer" -Index $footerIndex -Section $null)) | Out-Null
        }
    }

    foreach ($selection in $partSelections) {
        $arguments = @("inspect-bookmarks", $resolvedInputDocx, "--part", [string]$selection.part, "--json")
        $selectionSummary = [ordered]@{
            part = [string]$selection.part
        }

        $indexValue = Get-OptionalObjectPropertyValue -Object $selection -Name "index"
        if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
            $arguments += @("--index", $indexValue)
            $selectionSummary.index = [int]$indexValue
        }

        $sectionValue = Get-OptionalObjectPropertyValue -Object $selection -Name "section"
        if (-not [string]::IsNullOrWhiteSpace($sectionValue)) {
            $arguments += @("--section", $sectionValue)
            $selectionSummary.section = [int]$sectionValue
        }

        $kindValue = Get-OptionalObjectPropertyValue -Object $selection -Name "kind"
        if (-not [string]::IsNullOrWhiteSpace($kindValue)) {
            $arguments += @("--kind", $kindValue)
            $selectionSummary.kind = $kindValue
        }

        $selectionLabel = if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
            "{0}[{1}]" -f [string]$selection.part, $indexValue
        } elseif (-not [string]::IsNullOrWhiteSpace($sectionValue)) {
            "{0}[section={1},kind={2}]" -f [string]$selection.part, $sectionValue, $kindValue
        } else {
            [string]$selection.part
        }
        Write-Step ("Inspecting bookmarks in {0}" -f $selectionLabel)
        $inspectResult = Invoke-TemplateSchemaJsonCommand `
            -ExecutablePath $cliPath `
            -Command "inspect-bookmarks" `
            -Arguments $arguments

        $selectionSummary.entry_name = [string](Get-OptionalObjectPropertyValue -Object $inspectResult.Json -Name "entry_name")
        $selectionSummary.bookmark_count = [int](Get-OptionalObjectPropertyValue -Object $inspectResult.Json -Name "count")
        $inspectedParts.Add([pscustomobject]$selectionSummary) | Out-Null

        Add-RenderPlanEntriesFromInspection `
            -Inspection $inspectResult.Json `
            -Selection $selection `
            -BookmarkTextEntries $bookmarkTextEntries `
            -BookmarkParagraphEntries $bookmarkParagraphEntries `
            -BookmarkTableRowEntries $bookmarkTableRowEntries `
            -BookmarkVisibilityEntries $bookmarkVisibilityEntries `
            -UnsupportedBookmarks $unsupportedBookmarks
    }

    $plan.bookmark_text = @($bookmarkTextEntries.ToArray())
    $plan.bookmark_paragraphs = @($bookmarkParagraphEntries.ToArray())
    $plan.bookmark_table_rows = @($bookmarkTableRowEntries.ToArray())
    $plan.bookmark_block_visibility = @($bookmarkVisibilityEntries.ToArray())

    Write-Step "Writing render-plan draft to $resolvedOutputPlan"
    ($plan | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedOutputPlan -Encoding UTF8

    if ($unsupportedBookmarks.Count -gt 0) {
        Write-Warning ("Skipped {0} unsupported bookmark(s) while exporting the render-plan draft." -f $unsupportedBookmarks.Count)
    }

    $status = "completed"
} catch {
    throw
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-RenderPlanExportSummary `
            -Status $status `
            -InputDocx $resolvedInputDocx `
            -OutputPlan $resolvedOutputPlan `
            -SummaryJsonPath $resolvedSummaryJson `
            -CliPath $cliPath `
            -ResolvedBuildDir $resolvedBuildDir `
            -TargetMode $TargetMode `
            -InspectedParts @($inspectedParts.ToArray()) `
            -UnsupportedBookmarks @($unsupportedBookmarks.ToArray()) `
            -Plan ([pscustomobject]$plan)
        ($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }
}

Write-Step "Render-plan draft written to $resolvedOutputPlan"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
