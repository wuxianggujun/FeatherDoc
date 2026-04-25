<#
.SYNOPSIS
Renders a DOCX template from a JSON bookmark render plan.

.DESCRIPTION
Builds or reuses `featherdoc_cli`, then applies bookmark text filling,
paragraph replacement, table-row expansion, and block visibility mutations
described in a JSON plan. The script fails when any requested bookmark binding
cannot be matched so template drift is surfaced immediately.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\render_template_document.ps1 `
    -InputDocx .\samples\chinese_invoice_template.docx `
    -PlanPath .\samples\chinese_invoice_template.render_plan.json `
    -OutputDocx .\output\rendered\invoice.docx `
    -SummaryJson .\output\rendered\invoice.render.summary.json `
    -BuildDir build-codex-clang-compat `
    -SkipBuild
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$PlanPath,
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
    Write-Host "[render-template-document] $Message"
}

function Resolve-OptionalRenderPath {
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

function New-RenderTextFile {
    param(
        [string]$TemporaryRoot,
        [string]$Category,
        [string]$BaseName,
        [AllowEmptyString()]
        [string]$Text
    )

    $script:RenderTextFileIndex += 1

    $safeCategory = $Category -replace '[^A-Za-z0-9._-]', '_'
    $safeBaseName = ($BaseName -replace '[^A-Za-z0-9._-]', '_').Trim('_')
    if ([string]::IsNullOrWhiteSpace($safeBaseName)) {
        $safeBaseName = "text"
    }

    $filePath = Join-Path $TemporaryRoot (
        "render_input_{0:000}_{1}_{2}.txt" -f $script:RenderTextFileIndex, $safeCategory, $safeBaseName
    )
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($filePath, [string]$Text, $utf8NoBom)
    return $filePath
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

function Get-RenderPlanArray {
    param(
        $Plan,
        [string]$Name
    )

    return Get-OptionalObjectArrayProperty -Object $Plan -Name $Name
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

function Convert-ToNormalizedSelection {
    param(
        $Item,
        [string]$Category
    )

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

            return [pscustomobject]@{
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

            return [pscustomobject]@{
                part = "header"
                index = [int]$indexValue
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

            return [pscustomobject]@{
                part = "footer"
                index = [int]$indexValue
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

            return [pscustomobject]@{
                part = "section-header"
                section = [int]$sectionValue
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

            return [pscustomobject]@{
                part = "section-footer"
                section = [int]$sectionValue
                kind = $kindValue
            }
        }
        default {
            throw "$Category entry uses unsupported part '$part'."
        }
    }
}

function Convert-SelectionToGroupKey {
    param($Selection)

    $part = Get-OptionalObjectPropertyValue -Object $Selection -Name "part"
    $index = Get-OptionalObjectPropertyValue -Object $Selection -Name "index"
    $section = Get-OptionalObjectPropertyValue -Object $Selection -Name "section"
    $kind = Get-OptionalObjectPropertyValue -Object $Selection -Name "kind"

    return "part=$part|index=$index|section=$section|kind=$kind"
}

function Convert-SelectionToSummaryObject {
    param($Selection)

    $summary = [ordered]@{
        part = [string]$Selection.part
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

function Add-SelectionArguments {
    param(
        [string[]]$Arguments,
        $Selection
    )

    $result = @($Arguments + @("--part", [string]$Selection.part))

    $indexValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "index"
    if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
        $result += @("--index", $indexValue)
    }

    $sectionValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "section"
    if (-not [string]::IsNullOrWhiteSpace($sectionValue)) {
        $result += @("--section", $sectionValue)
    }

    $kindValue = Get-OptionalObjectPropertyValue -Object $Selection -Name "kind"
    if (-not [string]::IsNullOrWhiteSpace($kindValue)) {
        $result += @("--kind", $kindValue)
    }

    return $result
}

function New-GroupedSelectionQueue {
    return [pscustomobject]@{
        Keys = New-Object 'System.Collections.Generic.List[string]'
        Map = @{}
    }
}

function Add-GroupedSelectionItem {
    param(
        $Queue,
        $Selection,
        $Item
    )

    $key = Convert-SelectionToGroupKey -Selection $Selection
    if (-not $Queue.Map.ContainsKey($key)) {
        $Queue.Keys.Add($key) | Out-Null
        $Queue.Map[$key] = [pscustomobject]@{
            Selection = $Selection
            Items = New-Object 'System.Collections.Generic.List[object]'
        }
    }

    $Queue.Map[$key].Items.Add($Item) | Out-Null
}

function Invoke-RenderPlanCliCommand {
    param(
        [string]$ExecutablePath,
        [string]$Command,
        [string]$InputPath,
        [string[]]$Arguments,
        [string]$OutputPath
    )

    $fullArguments = @($Command, $InputPath) + $Arguments + @("--output", $OutputPath, "--json")
    $commandResult = Invoke-TemplateSchemaCli -ExecutablePath $ExecutablePath -Arguments $fullArguments
    if ($commandResult.ExitCode -ne 0) {
        $joined = if ($commandResult.Output.Count -gt 0) {
            $commandResult.Output -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "$Command failed with exit code $($commandResult.ExitCode). Output:`n$joined"
    }

    $json = Get-TemplateSchemaCommandJsonObject `
        -Lines $commandResult.Output `
        -Command $Command

    return [pscustomobject]@{
        Arguments = $fullArguments
        Output = $commandResult.Output
        Result = $json
    }
}

function Assert-CompleteBatchResult {
    param(
        $Result,
        [string]$Command
    )

    $completeProperty = Get-OptionalObjectPropertyObject -Object $Result -Name "complete"
    if ($null -ne $completeProperty -and -not [bool]$completeProperty) {
        $missingBookmarks = @(
            Get-OptionalObjectArrayProperty -Object $Result -Name "missing_bookmarks" |
                ForEach-Object { $_.ToString() }
        )
        if ($missingBookmarks.Count -gt 0) {
            throw "$Command did not fully match the requested bookmarks: $($missingBookmarks -join ', ')."
        }

        throw "$Command reported an incomplete batch result."
    }
}

function Convert-TableRows {
    param(
        $Item,
        [string]$Category
    )

    $rows = Get-OptionalObjectArrayProperty -Object $Item -Name "rows"
    $convertedRows = New-Object 'System.Collections.Generic.List[object]'

    foreach ($row in $rows) {
        if ($row -is [string] -or -not ($row -is [System.Collections.IEnumerable])) {
            throw "$Category rows must be arrays of one or more cell texts."
        }

        $cells = @($row | ForEach-Object { $_.ToString() })
        if ($cells.Count -eq 0) {
            throw "$Category rows must not be empty arrays."
        }

        $convertedRows.Add($cells) | Out-Null
    }

    return @($convertedRows.ToArray())
}

function Build-RenderSummary {
    param(
        [string]$Status,
        [string]$InputDocx,
        [string]$PlanPath,
        [string]$OutputDocx,
        [string]$SummaryJsonPath,
        [string]$CliPath,
        [string]$ResolvedBuildDir,
        [hashtable]$PlanCounts,
        [object[]]$Steps,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        plan_path = $PlanPath
        output_docx = $OutputDocx
        summary_json = $SummaryJsonPath
        cli_path = $CliPath
        build_dir = $ResolvedBuildDir
        plan_counts = [ordered]@{
            bookmark_text = [int]$PlanCounts["bookmark_text"]
            bookmark_paragraphs = [int]$PlanCounts["bookmark_paragraphs"]
            bookmark_table_rows = [int]$PlanCounts["bookmark_table_rows"]
            bookmark_block_visibility = [int]$PlanCounts["bookmark_block_visibility"]
        }
        operation_count = $Steps.Count
        steps = $Steps
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedPlanPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $PlanPath
$resolvedOutputDocx = Resolve-OptionalRenderPath -RepoRoot $repoRoot -InputPath $OutputDocx
$resolvedSummaryJson = Resolve-OptionalRenderPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalRenderPath -RepoRoot $repoRoot -InputPath $BuildDir
}
$cliPath = ""
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("featherdoc-render-" + [System.Guid]::NewGuid().ToString("N"))
$status = "failed"
$failureMessage = ""
$stepResults = New-Object 'System.Collections.Generic.List[object]'
$script:RenderTextFileIndex = 0

Ensure-PathParent -Path $resolvedOutputDocx
Ensure-PathParent -Path $resolvedSummaryJson

$planJson = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedPlanPath | ConvertFrom-Json
$bookmarkTextEntries = @(
    foreach ($entry in (Get-RenderPlanArray -Plan $planJson -Name "bookmark_text")) {
        $selection = Convert-ToNormalizedSelection -Item $entry -Category "bookmark_text"
        [pscustomobject]@{
            bookmark_name = Get-BookmarkName -Item $entry -Category "bookmark_text"
            text = [string](Get-OptionalObjectPropertyValue -Object $entry -Name "text")
            selection = $selection
        }
    }
)
$bookmarkParagraphEntries = @(
    foreach ($entry in (Get-RenderPlanArray -Plan $planJson -Name "bookmark_paragraphs")) {
        $paragraphs = @(
            Get-OptionalObjectArrayProperty -Object $entry -Name "paragraphs" |
                ForEach-Object { $_.ToString() }
        )
        if ($paragraphs.Count -eq 0) {
            throw "bookmark_paragraphs entries must provide at least one paragraph."
        }

        [pscustomobject]@{
            bookmark_name = Get-BookmarkName -Item $entry -Category "bookmark_paragraphs"
            paragraphs = $paragraphs
            selection = Convert-ToNormalizedSelection -Item $entry -Category "bookmark_paragraphs"
        }
    }
)
$bookmarkTableRowsEntries = @(
    foreach ($entry in (Get-RenderPlanArray -Plan $planJson -Name "bookmark_table_rows")) {
        [pscustomobject]@{
            bookmark_name = Get-BookmarkName -Item $entry -Category "bookmark_table_rows"
            rows = Convert-TableRows -Item $entry -Category "bookmark_table_rows"
            selection = Convert-ToNormalizedSelection -Item $entry -Category "bookmark_table_rows"
        }
    }
)
$bookmarkVisibilityEntries = @(
    foreach ($entry in (Get-RenderPlanArray -Plan $planJson -Name "bookmark_block_visibility")) {
        $visibleProperty = Get-OptionalObjectPropertyObject -Object $entry -Name "visible"
        if ($null -eq $visibleProperty) {
            throw "bookmark_block_visibility entries must provide a boolean 'visible' property."
        }

        [pscustomobject]@{
            bookmark_name = Get-BookmarkName -Item $entry -Category "bookmark_block_visibility"
            visible = [bool]$visibleProperty
            selection = Convert-ToNormalizedSelection -Item $entry -Category "bookmark_block_visibility"
        }
    }
)

$planCounts = @{
    bookmark_text = $bookmarkTextEntries.Count
    bookmark_paragraphs = $bookmarkParagraphEntries.Count
    bookmark_table_rows = $bookmarkTableRowsEntries.Count
    bookmark_block_visibility = $bookmarkVisibilityEntries.Count
}

try {
    New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null

    Write-Step "Resolving featherdoc_cli"
    $cliPath = Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $BuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild

    $currentDocxPath = $resolvedInputDocx
    $nextStepIndex = 1

    $textQueue = New-GroupedSelectionQueue
    foreach ($entry in $bookmarkTextEntries) {
        Add-GroupedSelectionItem -Queue $textQueue -Selection $entry.selection -Item $entry
    }

    foreach ($key in $textQueue.Keys) {
        $group = $textQueue.Map[$key]
        $arguments = @()
        $bindings = New-Object 'System.Collections.Generic.List[object]'

        foreach ($binding in $group.Items) {
            $textPath = New-RenderTextFile `
                -TemporaryRoot $temporaryRoot `
                -Category "fill_bookmarks" `
                -BaseName ([string]$binding.bookmark_name) `
                -Text ([string]$binding.text)
            $arguments += @("--set-file", [string]$binding.bookmark_name, $textPath)
            $bindings.Add([ordered]@{
                    bookmark_name = [string]$binding.bookmark_name
                    text = [string]$binding.text
                }) | Out-Null
        }

        $arguments = Add-SelectionArguments -Arguments $arguments -Selection $group.Selection
        $stepOutputPath = Join-Path $temporaryRoot ("step_{0:00}_fill_bookmarks.docx" -f $nextStepIndex)
        Write-Step ("Applying fill-bookmarks to {0}" -f ((Convert-SelectionToGroupKey -Selection $group.Selection) -replace '\|', ', '))
        $command = Invoke-RenderPlanCliCommand `
            -ExecutablePath $cliPath `
            -Command "fill-bookmarks" `
            -InputPath $currentDocxPath `
            -Arguments $arguments `
            -OutputPath $stepOutputPath
        Assert-CompleteBatchResult -Result $command.Result -Command "fill-bookmarks"

        $stepResults.Add([ordered]@{
                index = $nextStepIndex
                command = "fill-bookmarks"
                input_docx = $currentDocxPath
                output_docx = $stepOutputPath
                selection = Convert-SelectionToSummaryObject -Selection $group.Selection
                request = [ordered]@{
                    bindings = @($bindings.ToArray())
                }
                result = $command.Result
            }) | Out-Null

        $currentDocxPath = $stepOutputPath
        $nextStepIndex += 1
    }

    foreach ($entry in $bookmarkParagraphEntries) {
        $arguments = @([string]$entry.bookmark_name)
        for ($paragraphIndex = 0; $paragraphIndex -lt $entry.paragraphs.Count; $paragraphIndex++) {
            $paragraphPath = New-RenderTextFile `
                -TemporaryRoot $temporaryRoot `
                -Category "bookmark_paragraphs" `
                -BaseName ("{0}_{1}" -f [string]$entry.bookmark_name, ($paragraphIndex + 1)) `
                -Text ([string]$entry.paragraphs[$paragraphIndex])
            $arguments += @("--paragraph-file", $paragraphPath)
        }
        $arguments = Add-SelectionArguments -Arguments $arguments -Selection $entry.selection

        $stepOutputPath = Join-Path $temporaryRoot ("step_{0:00}_replace_bookmark_paragraphs.docx" -f $nextStepIndex)
        Write-Step ("Replacing bookmark paragraphs for '{0}'" -f [string]$entry.bookmark_name)
        $command = Invoke-RenderPlanCliCommand `
            -ExecutablePath $cliPath `
            -Command "replace-bookmark-paragraphs" `
            -InputPath $currentDocxPath `
            -Arguments $arguments `
            -OutputPath $stepOutputPath

        $stepResults.Add([ordered]@{
                index = $nextStepIndex
                command = "replace-bookmark-paragraphs"
                input_docx = $currentDocxPath
                output_docx = $stepOutputPath
                selection = Convert-SelectionToSummaryObject -Selection $entry.selection
                request = [ordered]@{
                    bookmark_name = [string]$entry.bookmark_name
                    paragraphs = @($entry.paragraphs)
                }
                result = $command.Result
            }) | Out-Null

        $currentDocxPath = $stepOutputPath
        $nextStepIndex += 1
    }

    foreach ($entry in $bookmarkTableRowsEntries) {
        $arguments = @([string]$entry.bookmark_name)
        if ($entry.rows.Count -eq 0) {
            $arguments += "--empty"
        } else {
            for ($rowIndex = 0; $rowIndex -lt $entry.rows.Count; $rowIndex++) {
                $row = $entry.rows[$rowIndex]
                $rowPath = New-RenderTextFile `
                    -TemporaryRoot $temporaryRoot `
                    -Category "bookmark_table_rows" `
                    -BaseName ("{0}_row_{1}_cell_1" -f [string]$entry.bookmark_name, ($rowIndex + 1)) `
                    -Text ([string]$row[0])
                $arguments += @("--row-file", $rowPath)
                for ($cellIndex = 1; $cellIndex -lt $row.Count; $cellIndex++) {
                    $cellPath = New-RenderTextFile `
                        -TemporaryRoot $temporaryRoot `
                        -Category "bookmark_table_rows" `
                        -BaseName ("{0}_row_{1}_cell_{2}" -f [string]$entry.bookmark_name, ($rowIndex + 1), ($cellIndex + 1)) `
                        -Text ([string]$row[$cellIndex])
                    $arguments += @("--cell-file", $cellPath)
                }
            }
        }
        $arguments = Add-SelectionArguments -Arguments $arguments -Selection $entry.selection

        $stepOutputPath = Join-Path $temporaryRoot ("step_{0:00}_replace_bookmark_table_rows.docx" -f $nextStepIndex)
        Write-Step ("Replacing bookmark table rows for '{0}'" -f [string]$entry.bookmark_name)
        $command = Invoke-RenderPlanCliCommand `
            -ExecutablePath $cliPath `
            -Command "replace-bookmark-table-rows" `
            -InputPath $currentDocxPath `
            -Arguments $arguments `
            -OutputPath $stepOutputPath

        $stepResults.Add([ordered]@{
                index = $nextStepIndex
                command = "replace-bookmark-table-rows"
                input_docx = $currentDocxPath
                output_docx = $stepOutputPath
                selection = Convert-SelectionToSummaryObject -Selection $entry.selection
                request = [ordered]@{
                    bookmark_name = [string]$entry.bookmark_name
                    rows = @($entry.rows)
                }
                result = $command.Result
            }) | Out-Null

        $currentDocxPath = $stepOutputPath
        $nextStepIndex += 1
    }

    $visibilityQueue = New-GroupedSelectionQueue
    foreach ($entry in $bookmarkVisibilityEntries) {
        Add-GroupedSelectionItem -Queue $visibilityQueue -Selection $entry.selection -Item $entry
    }

    foreach ($key in $visibilityQueue.Keys) {
        $group = $visibilityQueue.Map[$key]
        $arguments = @()
        $bindings = New-Object 'System.Collections.Generic.List[object]'

        foreach ($binding in $group.Items) {
            if ([bool]$binding.visible) {
                $arguments += @("--show", [string]$binding.bookmark_name)
            } else {
                $arguments += @("--hide", [string]$binding.bookmark_name)
            }
            $bindings.Add([ordered]@{
                    bookmark_name = [string]$binding.bookmark_name
                    visible = [bool]$binding.visible
                }) | Out-Null
        }

        $arguments = Add-SelectionArguments -Arguments $arguments -Selection $group.Selection
        $stepOutputPath = Join-Path $temporaryRoot ("step_{0:00}_apply_bookmark_block_visibility.docx" -f $nextStepIndex)
        Write-Step ("Applying bookmark block visibility to {0}" -f ((Convert-SelectionToGroupKey -Selection $group.Selection) -replace '\|', ', '))
        $command = Invoke-RenderPlanCliCommand `
            -ExecutablePath $cliPath `
            -Command "apply-bookmark-block-visibility" `
            -InputPath $currentDocxPath `
            -Arguments $arguments `
            -OutputPath $stepOutputPath
        Assert-CompleteBatchResult -Result $command.Result -Command "apply-bookmark-block-visibility"

        $stepResults.Add([ordered]@{
                index = $nextStepIndex
                command = "apply-bookmark-block-visibility"
                input_docx = $currentDocxPath
                output_docx = $stepOutputPath
                selection = Convert-SelectionToSummaryObject -Selection $group.Selection
                request = [ordered]@{
                    bindings = @($bindings.ToArray())
                }
                result = $command.Result
            }) | Out-Null

        $currentDocxPath = $stepOutputPath
        $nextStepIndex += 1
    }

    if ($stepResults.Count -eq 0) {
        Write-Step "Render plan contained no mutations; copying input document"
    } else {
        Write-Step "Writing final rendered document"
    }

    if ([System.StringComparer]::OrdinalIgnoreCase.Equals($currentDocxPath, $resolvedOutputDocx)) {
        $null = $true
    } elseif ([System.StringComparer]::OrdinalIgnoreCase.Equals($resolvedInputDocx, $resolvedOutputDocx) -and
              $stepResults.Count -eq 0) {
        $null = $true
    } else {
        [System.IO.File]::Copy($currentDocxPath, $resolvedOutputDocx, $true)
    }

    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    throw
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-RenderSummary `
            -Status $status `
            -InputDocx $resolvedInputDocx `
            -PlanPath $resolvedPlanPath `
            -OutputDocx $resolvedOutputDocx `
            -SummaryJsonPath $resolvedSummaryJson `
            -CliPath $cliPath `
            -ResolvedBuildDir $resolvedBuildDir `
            -PlanCounts $planCounts `
            -Steps @($stepResults.ToArray()) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }

    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

if ($status -ne "completed") {
    exit 1
}

Write-Step "Rendered DOCX: $resolvedOutputDocx"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
