<#
.SYNOPSIS
Applies a render-plan patch onto an existing render-plan draft.

.DESCRIPTION
Reads a base render plan plus a patch document that uses the same top-level
arrays as the render plan. Each patch entry matches one existing render-plan
entry by bookmark name plus optional part/index/section/kind selectors, then
replaces only that entry's payload. This keeps template-selection metadata in
the base plan while letting business data live in a separate JSON file.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\patch_template_render_plan.ps1 `
    -BasePlanPath .\output\rendered\invoice.render-plan.json `
    -PatchPlanPath .\samples\chinese_invoice_template.render_patch.json `
    -OutputPlan .\output\rendered\invoice.render-plan.filled.json `
    -SummaryJson .\output\rendered\invoice.render-plan.patch.summary.json `
    -RequireComplete
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$BasePlanPath,
    [Parameter(Mandatory = $true)]
    [string]$PatchPlanPath,
    [string]$OutputPlan = "",
    [string]$SummaryJson = "",
    [switch]$RequireComplete
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[patch-template-render-plan] $Message"
}

function Resolve-RenderPlanRepoRoot {
    param([string]$ScriptRoot)

    return (Resolve-Path (Join-Path $ScriptRoot "..")).Path
}

function Resolve-OptionalRenderPlanPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    if ($AllowMissing) {
        return [System.IO.Path]::GetFullPath($candidate)
    }

    return (Resolve-Path -LiteralPath $candidate).Path
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

function Convert-ToSupportedSelectorKind {
    param(
        [string]$Text,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return "default"
    }

    $supportedKinds = @("default", "first", "even")
    if ($supportedKinds -notcontains $Text) {
        throw "$Label uses unsupported kind '$Text'."
    }

    return $Text
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

            return [pscustomobject]@{
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

            return [pscustomobject]@{
                part = "section-header"
                section = Convert-ToNonNegativeInt -Text $sectionValue -Label "$Category section"
                kind = Convert-ToSupportedSelectorKind `
                    -Text $kindValue `
                    -Label "$Category entries targeting '$part'"
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
                section = Convert-ToNonNegativeInt -Text $sectionValue -Label "$Category section"
                kind = Convert-ToSupportedSelectorKind `
                    -Text $kindValue `
                    -Label "$Category entries targeting '$part'"
            }
        }
        default {
            throw "$Category entry uses unsupported part '$part'."
        }
    }
}

function Convert-ToPatchSelector {
    param(
        $Item,
        [string]$Category
    )

    $part = Get-OptionalObjectPropertyValue -Object $Item -Name "part"
    $indexValue = Get-OptionalObjectPropertyValue -Object $Item -Name "index"
    $sectionValue = Get-OptionalObjectPropertyValue -Object $Item -Name "section"
    $kindValue = Get-OptionalObjectPropertyValue -Object $Item -Name "kind"

    if ([string]::IsNullOrWhiteSpace($part)) {
        if (-not [string]::IsNullOrWhiteSpace($indexValue) -or
            -not [string]::IsNullOrWhiteSpace($sectionValue) -or
            -not [string]::IsNullOrWhiteSpace($kindValue)) {
            throw "$Category patch entries that set 'index', 'section', or 'kind' must also set 'part'."
        }

        return [pscustomobject]@{
            has_selection = $false
        }
    }

    switch ($part) {
        "body" {
            if (-not [string]::IsNullOrWhiteSpace($indexValue) -or
                -not [string]::IsNullOrWhiteSpace($sectionValue) -or
                -not [string]::IsNullOrWhiteSpace($kindValue)) {
                throw "$Category patch entries targeting 'body' must not set 'index', 'section', or 'kind'."
            }

            return [pscustomobject]@{
                has_selection = $true
                part = "body"
            }
        }
        "header" {
            if ([string]::IsNullOrWhiteSpace($indexValue)) {
                throw "$Category patch entries targeting 'header' must provide 'index'."
            }
            if (-not [string]::IsNullOrWhiteSpace($sectionValue) -or
                -not [string]::IsNullOrWhiteSpace($kindValue)) {
                throw "$Category patch entries targeting 'header' must not set 'section' or 'kind'."
            }

            return [pscustomobject]@{
                has_selection = $true
                part = "header"
                index = Convert-ToNonNegativeInt -Text $indexValue -Label "$Category patch index"
            }
        }
        "footer" {
            if ([string]::IsNullOrWhiteSpace($indexValue)) {
                throw "$Category patch entries targeting 'footer' must provide 'index'."
            }
            if (-not [string]::IsNullOrWhiteSpace($sectionValue) -or
                -not [string]::IsNullOrWhiteSpace($kindValue)) {
                throw "$Category patch entries targeting 'footer' must not set 'section' or 'kind'."
            }

            return [pscustomobject]@{
                has_selection = $true
                part = "footer"
                index = Convert-ToNonNegativeInt -Text $indexValue -Label "$Category patch index"
            }
        }
        "section-header" {
            if ([string]::IsNullOrWhiteSpace($sectionValue)) {
                throw "$Category patch entries targeting 'section-header' must provide 'section'."
            }
            if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
                throw "$Category patch entries targeting 'section-header' must not set 'index'."
            }
            if ([string]::IsNullOrWhiteSpace($kindValue)) {
                $kindValue = "default"
            }

            return [pscustomobject]@{
                has_selection = $true
                part = "section-header"
                section = Convert-ToNonNegativeInt -Text $sectionValue -Label "$Category patch section"
                kind = Convert-ToSupportedSelectorKind `
                    -Text $kindValue `
                    -Label "$Category patch entries targeting '$part'"
            }
        }
        "section-footer" {
            if ([string]::IsNullOrWhiteSpace($sectionValue)) {
                throw "$Category patch entries targeting 'section-footer' must provide 'section'."
            }
            if (-not [string]::IsNullOrWhiteSpace($indexValue)) {
                throw "$Category patch entries targeting 'section-footer' must not set 'index'."
            }
            if ([string]::IsNullOrWhiteSpace($kindValue)) {
                $kindValue = "default"
            }

            return [pscustomobject]@{
                has_selection = $true
                part = "section-footer"
                section = Convert-ToNonNegativeInt -Text $sectionValue -Label "$Category patch section"
                kind = Convert-ToSupportedSelectorKind `
                    -Text $kindValue `
                    -Label "$Category patch entries targeting '$part'"
            }
        }
        default {
            throw "$Category patch entry uses unsupported part '$part'."
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
    param(
        $Selection,
        [string]$BookmarkName
    )

    $summary = [ordered]@{
        bookmark_name = $BookmarkName
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

function SelectionMatchesPatchSelector {
    param(
        $Selection,
        $Selector
    )

    if (-not [bool]$Selector.has_selection) {
        return $true
    }

    if ([string]$Selection.part -ne [string]$Selector.part) {
        return $false
    }

    $selectorIndex = Get-OptionalObjectPropertyValue -Object $Selector -Name "index"
    if (-not [string]::IsNullOrWhiteSpace($selectorIndex)) {
        if ((Get-OptionalObjectPropertyValue -Object $Selection -Name "index") -ne $selectorIndex) {
            return $false
        }
    }

    $selectorSection = Get-OptionalObjectPropertyValue -Object $Selector -Name "section"
    if (-not [string]::IsNullOrWhiteSpace($selectorSection)) {
        if ((Get-OptionalObjectPropertyValue -Object $Selection -Name "section") -ne $selectorSection) {
            return $false
        }
    }

    $selectorKind = Get-OptionalObjectPropertyValue -Object $Selector -Name "kind"
    if (-not [string]::IsNullOrWhiteSpace($selectorKind)) {
        if ((Get-OptionalObjectPropertyValue -Object $Selection -Name "kind") -ne $selectorKind) {
            return $false
        }
    }

    return $true
}

function Get-CategoryPayloadValue {
    param(
        [string]$Category,
        $Item
    )

    switch ($Category) {
        "bookmark_text" {
            $value = Get-OptionalObjectPropertyObject -Object $Item -Name "text"
            if ($null -eq $value) {
                throw "bookmark_text entries must provide a 'text' property."
            }
            return [string]$value
        }
        "bookmark_paragraphs" {
            $paragraphs = @(
                Get-OptionalObjectArrayProperty -Object $Item -Name "paragraphs" |
                    ForEach-Object { $_.ToString() }
            )
            if ($paragraphs.Count -eq 0) {
                throw "bookmark_paragraphs entries must provide at least one paragraph."
            }
            return @($paragraphs)
        }
        "bookmark_table_rows" {
            $rows = Get-OptionalObjectArrayProperty -Object $Item -Name "rows"
            $convertedRows = New-Object 'System.Collections.Generic.List[object]'

            foreach ($row in $rows) {
                if ($row -is [string] -or -not ($row -is [System.Collections.IEnumerable])) {
                    throw "bookmark_table_rows rows must be arrays of one or more cell texts."
                }

                $cells = @($row | ForEach-Object { $_.ToString() })
                if ($cells.Count -eq 0) {
                    throw "bookmark_table_rows rows must not be empty arrays."
                }

                $convertedRows.Add($cells) | Out-Null
            }

            return @($convertedRows.ToArray())
        }
        "bookmark_block_visibility" {
            $visible = Get-OptionalObjectPropertyObject -Object $Item -Name "visible"
            if ($null -eq $visible) {
                throw "bookmark_block_visibility entries must provide a boolean 'visible' property."
            }
            return [bool]$visible
        }
        default {
            throw "Unsupported render-plan category '$Category'."
        }
    }
}

function Set-CategoryPayloadValue {
    param(
        [string]$Category,
        $Item,
        $Value
    )

    switch ($Category) {
        "bookmark_text" {
            $Item.text = [string]$Value
        }
        "bookmark_paragraphs" {
            $Item.paragraphs = @($Value)
        }
        "bookmark_table_rows" {
            $Item.rows = @($Value)
        }
        "bookmark_block_visibility" {
            $Item.visible = [bool]$Value
        }
        default {
            throw "Unsupported render-plan category '$Category'."
        }
    }
}

function Test-CategoryPlaceholder {
    param(
        [string]$Category,
        $Record
    )

    switch ($Category) {
        "bookmark_text" {
            return [string]$Record.Payload -eq ("TODO: " + $Record.BookmarkName)
        }
        "bookmark_paragraphs" {
            return @($Record.Payload).Count -eq 1 -and
                [string]$Record.Payload[0] -eq ("TODO: " + $Record.BookmarkName)
        }
        "bookmark_table_rows" {
            return @($Record.Payload).Count -eq 0
        }
        default {
            return $false
        }
    }
}

function Merge-RenderPlanCategory {
    param(
        [string]$Category,
        $BasePlan,
        $PatchPlan
    )

    $baseEntries = Get-RenderPlanArray -Plan $BasePlan -Name $Category
    $baseRecords = New-Object 'System.Collections.Generic.List[object]'

    foreach ($entry in $baseEntries) {
        $bookmarkName = Get-BookmarkName -Item $entry -Category $Category
        $selection = Convert-ToNormalizedSelection -Item $entry -Category $Category
        $payload = Get-CategoryPayloadValue -Category $Category -Item $entry
        $baseRecords.Add([pscustomobject]@{
                BookmarkName = $bookmarkName
                Selection = $selection
                Payload = $payload
                Identity = Convert-SelectionToIdentityKey -Selection $selection -BookmarkName $bookmarkName
                Entry = $entry
            }) | Out-Null
    }

    $requestedCount = 0
    $appliedCount = 0
    $matchedPatchIdentities = @{}
    $patchEntries = Get-RenderPlanArray -Plan $PatchPlan -Name $Category

    foreach ($patchEntry in $patchEntries) {
        $requestedCount += 1
        $bookmarkName = Get-BookmarkName -Item $patchEntry -Category $Category
        $selector = Convert-ToPatchSelector -Item $patchEntry -Category $Category
        $payload = Get-CategoryPayloadValue -Category $Category -Item $patchEntry

        $matches = @(
            $baseRecords | Where-Object {
                $_.BookmarkName -eq $bookmarkName -and
                (SelectionMatchesPatchSelector -Selection $_.Selection -Selector $selector)
            }
        )

        if ($matches.Count -eq 0) {
            throw "$Category patch entry '$bookmarkName' did not match any base render-plan entry."
        }
        if ($matches.Count -gt 1) {
            throw "$Category patch entry '$bookmarkName' matched multiple base render-plan entries; specify part/index/section/kind in the patch."
        }

        $match = $matches[0]
        if ($matchedPatchIdentities.ContainsKey($match.Identity)) {
            throw "$Category patch entry '$bookmarkName' targeted the same base render-plan entry more than once."
        }

        $matchedPatchIdentities[$match.Identity] = $true
        Set-CategoryPayloadValue -Category $Category -Item $match.Entry -Value $payload
        $match.Payload = $payload
        $appliedCount += 1
    }

    $remainingPlaceholders = New-Object 'System.Collections.Generic.List[object]'
    foreach ($record in $baseRecords) {
        if (Test-CategoryPlaceholder -Category $Category -Record $record) {
            $remainingPlaceholders.Add(
                [pscustomobject](Convert-SelectionToSummaryObject -Selection $record.Selection -BookmarkName $record.BookmarkName)
            ) | Out-Null
        }
    }

    return [pscustomobject]@{
        requested = $requestedCount
        applied = $appliedCount
        remaining_placeholders = @($remainingPlaceholders.ToArray())
    }
}

function Build-PatchSummary {
    param(
        [string]$Status,
        [string]$BasePlan,
        [string]$PatchPlan,
        [string]$OutputPlan,
        [string]$SummaryJsonPath,
        [bool]$RequireComplete,
        [object]$MergedPlan,
        [object]$CategoryResults,
        [string]$ErrorMessage
    )

    $remainingPlaceholderCount =
        @($CategoryResults.bookmark_text.remaining_placeholders).Count +
        @($CategoryResults.bookmark_paragraphs.remaining_placeholders).Count +
        @($CategoryResults.bookmark_table_rows.remaining_placeholders).Count

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        base_plan = $BasePlan
        patch_plan = $PatchPlan
        output_plan = $OutputPlan
        summary_json = $SummaryJsonPath
        require_complete = $RequireComplete
        plan_counts = [ordered]@{
            bookmark_text = @(Get-RenderPlanArray -Plan $MergedPlan -Name "bookmark_text").Count
            bookmark_paragraphs = @(Get-RenderPlanArray -Plan $MergedPlan -Name "bookmark_paragraphs").Count
            bookmark_table_rows = @(Get-RenderPlanArray -Plan $MergedPlan -Name "bookmark_table_rows").Count
            bookmark_block_visibility = @(Get-RenderPlanArray -Plan $MergedPlan -Name "bookmark_block_visibility").Count
        }
        patch_counts = [ordered]@{
            bookmark_text = [ordered]@{
                requested = [int]$CategoryResults.bookmark_text.requested
                applied = [int]$CategoryResults.bookmark_text.applied
            }
            bookmark_paragraphs = [ordered]@{
                requested = [int]$CategoryResults.bookmark_paragraphs.requested
                applied = [int]$CategoryResults.bookmark_paragraphs.applied
            }
            bookmark_table_rows = [ordered]@{
                requested = [int]$CategoryResults.bookmark_table_rows.requested
                applied = [int]$CategoryResults.bookmark_table_rows.applied
            }
            bookmark_block_visibility = [ordered]@{
                requested = [int]$CategoryResults.bookmark_block_visibility.requested
                applied = [int]$CategoryResults.bookmark_block_visibility.applied
            }
        }
        remaining_placeholder_count = $remainingPlaceholderCount
        remaining_placeholders = [ordered]@{
            bookmark_text = @($CategoryResults.bookmark_text.remaining_placeholders)
            bookmark_paragraphs = @($CategoryResults.bookmark_paragraphs.remaining_placeholders)
            bookmark_table_rows = @($CategoryResults.bookmark_table_rows.remaining_placeholders)
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-RenderPlanRepoRoot -ScriptRoot $PSScriptRoot
$resolvedBasePlanPath = Resolve-OptionalRenderPlanPath -RepoRoot $repoRoot -InputPath $BasePlanPath
$resolvedPatchPlanPath = Resolve-OptionalRenderPlanPath -RepoRoot $repoRoot -InputPath $PatchPlanPath
$resolvedOutputPlan = if ([string]::IsNullOrWhiteSpace($OutputPlan)) {
    $baseDirectory = [System.IO.Path]::GetDirectoryName($resolvedBasePlanPath)
    $baseStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedBasePlanPath)
    Join-Path $baseDirectory ($baseStem + ".patched.json")
} else {
    Resolve-OptionalRenderPlanPath -RepoRoot $repoRoot -InputPath $OutputPlan -AllowMissing
}
$resolvedSummaryJson = Resolve-OptionalRenderPlanPath -RepoRoot $repoRoot -InputPath $SummaryJson -AllowMissing

Ensure-PathParent -Path $resolvedOutputPlan
Ensure-PathParent -Path $resolvedSummaryJson

$basePlan = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedBasePlanPath | ConvertFrom-Json
$patchPlan = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedPatchPlanPath | ConvertFrom-Json
$status = "failed"
$failureMessage = ""
$categoryResults = [ordered]@{
    bookmark_text = [pscustomobject]@{ requested = 0; applied = 0; remaining_placeholders = @() }
    bookmark_paragraphs = [pscustomobject]@{ requested = 0; applied = 0; remaining_placeholders = @() }
    bookmark_table_rows = [pscustomobject]@{ requested = 0; applied = 0; remaining_placeholders = @() }
    bookmark_block_visibility = [pscustomobject]@{ requested = 0; applied = 0; remaining_placeholders = @() }
}

try {
    Write-Step "Applying bookmark_text patch entries"
    $categoryResults.bookmark_text = Merge-RenderPlanCategory `
        -Category "bookmark_text" `
        -BasePlan $basePlan `
        -PatchPlan $patchPlan

    Write-Step "Applying bookmark_paragraphs patch entries"
    $categoryResults.bookmark_paragraphs = Merge-RenderPlanCategory `
        -Category "bookmark_paragraphs" `
        -BasePlan $basePlan `
        -PatchPlan $patchPlan

    Write-Step "Applying bookmark_table_rows patch entries"
    $categoryResults.bookmark_table_rows = Merge-RenderPlanCategory `
        -Category "bookmark_table_rows" `
        -BasePlan $basePlan `
        -PatchPlan $patchPlan

    Write-Step "Applying bookmark_block_visibility patch entries"
    $categoryResults.bookmark_block_visibility = Merge-RenderPlanCategory `
        -Category "bookmark_block_visibility" `
        -BasePlan $basePlan `
        -PatchPlan $patchPlan

    Write-Step "Writing patched render plan to $resolvedOutputPlan"
    ($basePlan | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedOutputPlan -Encoding UTF8

    $remainingPlaceholderCount =
        @($categoryResults.bookmark_text.remaining_placeholders).Count +
        @($categoryResults.bookmark_paragraphs.remaining_placeholders).Count +
        @($categoryResults.bookmark_table_rows.remaining_placeholders).Count

    if ($RequireComplete -and $remainingPlaceholderCount -gt 0) {
        $remainingLabels = New-Object 'System.Collections.Generic.List[string]'
        foreach ($entry in @($categoryResults.bookmark_text.remaining_placeholders)) {
            $remainingLabels.Add("bookmark_text:$($entry.bookmark_name)") | Out-Null
        }
        foreach ($entry in @($categoryResults.bookmark_paragraphs.remaining_placeholders)) {
            $remainingLabels.Add("bookmark_paragraphs:$($entry.bookmark_name)") | Out-Null
        }
        foreach ($entry in @($categoryResults.bookmark_table_rows.remaining_placeholders)) {
            $remainingLabels.Add("bookmark_table_rows:$($entry.bookmark_name)") | Out-Null
        }

        throw "render plan still contains $remainingPlaceholderCount placeholder entries: $($remainingLabels -join ', ')."
    }

    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    throw
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-PatchSummary `
            -Status $status `
            -BasePlan $resolvedBasePlanPath `
            -PatchPlan $resolvedPatchPlanPath `
            -OutputPlan $resolvedOutputPlan `
            -SummaryJsonPath $resolvedSummaryJson `
            -RequireComplete ([bool]$RequireComplete) `
            -MergedPlan $basePlan `
            -CategoryResults ([pscustomobject]$categoryResults) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }
}

if ($status -ne "completed") {
    exit 1
}

Write-Step "Patched render plan: $resolvedOutputPlan"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
