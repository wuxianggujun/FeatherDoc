<#
.SYNOPSIS
Audits release-facing FeatherDoc materials for draft wording and local path leaks.

.DESCRIPTION
Scans one or more files or directories and fails if release-facing text files
contain draft/草稿 wording or machine-local absolute paths. Directories are
scanned recursively but only text-like files are considered.

.PARAMETER Path
One or more files or directories to scan.

.PARAMETER AdditionalForbiddenPattern
Optional extra regex patterns to audit in addition to the built-in rules.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\assert_release_material_safety.ps1 `
    -Path .\output\release-candidate-checks\START_HERE.md, .\output\release-candidate-checks\report
#>
param(
    [Parameter(Mandatory = $true)]
    [string[]]$Path,
    [string[]]$AdditionalForbiddenPattern = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[assert-release-material-safety] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Test-TextLikeFile {
    param([System.IO.FileInfo]$File)

    $textExtensions = @(
        ".md",
        ".txt",
        ".json",
        ".xml",
        ".yml",
        ".yaml",
        ".csv",
        ".log",
        ".rst"
    )

    return $textExtensions -contains $File.Extension.ToLowerInvariant()
}

function Get-ScanFiles {
    param(
        [string]$RepoRoot,
        [string[]]$InputPaths
    )

    $files = [System.Collections.Generic.List[string]]::new()
    foreach ($inputPath in $InputPaths) {
        $resolvedPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $inputPath
        if (-not (Test-Path -LiteralPath $resolvedPath)) {
            throw "Audit path does not exist: $resolvedPath"
        }

        $item = Get-Item -LiteralPath $resolvedPath
        if ($item.PSIsContainer) {
            foreach ($file in Get-ChildItem -LiteralPath $resolvedPath -Recurse -File) {
                if (Test-TextLikeFile -File $file) {
                    [void]$files.Add($file.FullName)
                }
            }
        } else {
            [void]$files.Add($item.FullName)
        }
    }

    return @($files | Sort-Object -Unique)
}

function New-Rule {
    param(
        [string]$Label,
        [string]$Pattern
    )

    return [ordered]@{
        label = $Label
        pattern = $Pattern
    }
}

function Get-JsonPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-JsonArray {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-JsonPropertyValue -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }

    if ($value -is [string]) {
        return @($value)
    }

    if ($value -is [System.Collections.IDictionary]) {
        return @($value)
    }

    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Get-JsonObjectNodes {
    param($Value)

    if ($null -eq $Value) {
        return
    }

    if ($Value -is [string]) {
        return
    }

    if ($Value -is [System.Collections.IDictionary]) {
        Write-Output $Value
        foreach ($key in $Value.Keys) {
            Get-JsonObjectNodes -Value $Value[$key]
        }

        return
    }

    if ($Value -is [pscustomobject]) {
        Write-Output $Value
        foreach ($property in $Value.PSObject.Properties) {
            Get-JsonObjectNodes -Value $property.Value
        }

        return
    }

    if ($Value -is [System.Collections.IEnumerable]) {
        foreach ($item in $Value) {
            Get-JsonObjectNodes -Value $item
        }
    }
}

function Add-AuditViolation {
    param(
        $Violations,
        [string]$File,
        [string]$Label,
        [string]$Text
    )

    [void]$Violations.Add([ordered]@{
        file = $File
        line = 1
        label = $Label
        text = $Text
    })
}

function Test-TextContainsAny {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    foreach ($needle in $Needles) {
        if (-not [string]::IsNullOrWhiteSpace($needle) -and $Text.Contains($needle)) {
            return $true
        }
    }

    return $false
}

function Test-StringValueInSet {
    param(
        $Value,
        [string[]]$AllowedValues
    )

    if ($null -eq $Value) {
        return $false
    }

    $normalized = ([string]$Value).Trim().Trim('`').ToLowerInvariant()
    if ([string]::IsNullOrWhiteSpace($normalized)) {
        return $false
    }

    $allowed = @{}
    foreach ($allowedValue in @($AllowedValues)) {
        if (-not [string]::IsNullOrWhiteSpace($allowedValue)) {
            $allowed[[string]$allowedValue.ToLowerInvariant()] = $true
        }
    }

    return $allowed.ContainsKey($normalized)
}

function Add-SourceDisplayIdentityViolations {
    param(
        [string]$File,
        $Violations,
        [string]$Label,
        [string]$FieldName,
        $Value,
        [string[]]$Needles,
        [string]$EvidenceDescription
    )

    if ([string]::IsNullOrWhiteSpace([string]$Value)) {
        return
    }

    if (-not (Test-TextContainsAny -Text ([string]$Value) -Needles $Needles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $Label `
            -Text "$FieldName must identify the $EvidenceDescription evidence source."
    }
}

function Get-TextLineContainingAll {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    foreach ($line in ($Text -split "\r?\n")) {
        $containsAllNeedles = $true
        foreach ($needle in $Needles) {
            if ([string]::IsNullOrWhiteSpace($needle) -or -not $line.Contains($needle)) {
                $containsAllNeedles = $false
                break
            }
        }

        if ($containsAllNeedles) {
            return $line
        }
    }

    return $null
}

function Test-TextLineContainsAll {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    $line = Get-TextLineContainingAll -Text $Text -Needles $Needles
    if ($null -ne $line) {
        return $true
    }

    return $false
}

function Test-ReleaseNoteProjectTemplateTraceLineContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    return Test-TextLineContainsAll -Text $Text -Needles (@($Anchor) + $Needles)
}

function Get-ReleaseNoteProjectTemplateTraceFieldValue {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$FieldName
    )

    $fieldPrefix = "$FieldName="
    $line = Get-TextLineContainingAll -Text $Text -Needles @($Anchor, $fieldPrefix)
    if ($null -eq $line) {
        return $null
    }

    $escapedFieldPrefix = [regex]::Escape($fieldPrefix)
    if ($line -match "(?:^|\s)$escapedFieldPrefix(?<value>\S+)") {
        return ([string]$Matches["value"]).Trim().Trim('`')
    }

    return $null
}

function Test-ReleaseNoteProjectTemplateTraceFieldIdentifies {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$FieldName,
        [string[]]$Needles
    )

    $fieldValue = Get-ReleaseNoteProjectTemplateTraceFieldValue `
        -Text $Text `
        -Anchor $Anchor `
        -FieldName $FieldName
    if ([string]::IsNullOrWhiteSpace([string]$fieldValue)) {
        return $false
    }

    return Test-TextContainsAny -Text ([string]$fieldValue) -Needles $Needles
}

function Test-ReleaseNoteProjectTemplateTraceFieldValueInSet {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$FieldName,
        [string[]]$AllowedValues
    )

    $fieldValue = Get-ReleaseNoteProjectTemplateTraceFieldValue `
        -Text $Text `
        -Anchor $Anchor `
        -FieldName $FieldName
    return (Test-StringValueInSet -Value $fieldValue -AllowedValues $AllowedValues)
}

function Test-MarkdownListBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    $block = Get-MarkdownListBlockText -Text $Text -Anchor $Anchor
    if ($null -eq $block) {
        return $false
    }

    $containsAllNeedles = $true
    foreach ($needle in $Needles) {
        if ([string]::IsNullOrWhiteSpace($needle) -or -not $block.Contains($needle)) {
            $containsAllNeedles = $false
            break
        }
    }

    return $containsAllNeedles
}

function Get-MarkdownListBlockText {
    param(
        [string]$Text,
        [string]$Anchor
    )

    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if (-not $lines[$lineIndex].Contains($Anchor)) {
            continue
        }

        $blockLines = [System.Collections.Generic.List[string]]::new()
        $blockLines.Add($lines[$lineIndex])
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            $nextLine = $lines[$nextLineIndex]
            if (-not [string]::IsNullOrWhiteSpace($nextLine) -and $nextLine -match '^\S') {
                break
            }
            $blockLines.Add($nextLine)
        }

        return ($blockLines -join "`n")
    }

    return $null
}

function Get-MarkdownListBlockTexts {
    param(
        [string]$Text,
        [string]$Anchor
    )

    $blocks = [System.Collections.Generic.List[string]]::new()
    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if (-not $lines[$lineIndex].Contains($Anchor)) {
            continue
        }

        $blockLines = [System.Collections.Generic.List[string]]::new()
        $blockLines.Add($lines[$lineIndex])
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            $nextLine = $lines[$nextLineIndex]
            if (-not [string]::IsNullOrWhiteSpace($nextLine) -and $nextLine -match '^\S') {
                break
            }
            $blockLines.Add($nextLine)
        }

        $blocks.Add(($blockLines -join "`n"))
    }

    return $blocks.ToArray()
}

function Test-MarkdownAnyListBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    foreach ($block in @(Get-MarkdownListBlockTexts -Text $Text -Anchor $Anchor)) {
        $containsAllNeedles = $true
        foreach ($needle in $Needles) {
            if ([string]::IsNullOrWhiteSpace($needle) -or -not $block.Contains($needle)) {
                $containsAllNeedles = $false
                break
            }
        }

        if ($containsAllNeedles) {
            return $true
        }
    }

    return $false
}

function Get-MarkdownListBlockFieldValues {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$FieldName
    )

    $block = Get-MarkdownListBlockText -Text $Text -Anchor $Anchor
    if ($null -eq $block) {
        return @()
    }

    $values = [System.Collections.Generic.List[string]]::new()
    $escapedFieldName = [regex]::Escape($FieldName)
    $fieldPattern = '(?<![A-Za-z0-9_])`*' + $escapedFieldName + '`*\s*[:=]\s*(?<value>.+?)(?=\s+[A-Za-z_][A-Za-z0-9_]*\s*[:=]|$)'
    foreach ($line in ($block -split "\r?\n")) {
        if ($line -match $fieldPattern) {
            $value = $Matches["value"].Trim()
            $value = $value -replace '^[`\s]+|[`\s]+$', ''
            [void]$values.Add($value)
        }
    }

    return @($values)
}

function Test-MarkdownListBlockFieldValuesIdentify {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$FieldName,
        [string[]]$Needles,
        [string[]]$ForbiddenNeedles = @()
    )

    $values = @(Get-MarkdownListBlockFieldValues -Text $Text -Anchor $Anchor -FieldName $FieldName)
    if ($values.Count -eq 0) {
        return $false
    }

    foreach ($value in $values) {
        if (-not (Test-TextContainsAny -Text ([string]$value) -Needles $Needles)) {
            return $false
        }

        if ($ForbiddenNeedles.Count -gt 0 -and (Test-TextContainsAny -Text ([string]$value) -Needles $ForbiddenNeedles)) {
            return $false
        }
    }

    return $true
}

function Test-MarkdownListBlockFieldValuesInSet {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$FieldName,
        [string[]]$AllowedValues
    )

    $values = @(Get-MarkdownListBlockFieldValues -Text $Text -Anchor $Anchor -FieldName $FieldName)
    if ($values.Count -eq 0) {
        return $false
    }

    $allowed = @{}
    foreach ($allowedValue in @($AllowedValues)) {
        if (-not [string]::IsNullOrWhiteSpace($allowedValue)) {
            $allowed[[string]$allowedValue.ToLowerInvariant()] = $true
        }
    }

    foreach ($value in $values) {
        $normalized = ([string]$value).Trim().ToLowerInvariant()
        if (-not $allowed.ContainsKey($normalized)) {
            return $false
        }
    }

    return $true
}

function Test-MarkdownListRunContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if (-not $lines[$lineIndex].Contains($Anchor)) {
            continue
        }
        if ($lines[$lineIndex] -notmatch '^\s*-\s*') {
            continue
        }

        $runStart = $lineIndex
        while ($runStart -gt 0 -and $lines[$runStart - 1] -match '^\s*-\s*') {
            $runStart--
        }

        $runEnd = $lineIndex
        while ($runEnd + 1 -lt $lines.Count -and $lines[$runEnd + 1] -match '^\s*-\s*') {
            $runEnd++
        }

        $run = ($lines[$runStart..$runEnd]) -join "`n"
        $containsAllNeedles = $true
        foreach ($needle in $Needles) {
            if ([string]::IsNullOrWhiteSpace($needle) -or -not $run.Contains($needle)) {
                $containsAllNeedles = $false
                break
            }
        }

        if ($containsAllNeedles) {
            return $true
        }
    }

    return $false
}

function Test-MarkdownSectionContainsAll {
    param(
        [string]$Text,
        [string]$Heading,
        [string[]]$Needles
    )

    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        $line = $lines[$lineIndex].Trim()
        if ($line -ne $Heading) {
            continue
        }
        if ($line -notmatch '^(#+)\s+') {
            continue
        }

        $headingLevel = $Matches[1].Length
        $sectionEnd = $lines.Count - 1
        for ($nextLineIndex = $lineIndex + 1; $nextLineIndex -lt $lines.Count; $nextLineIndex++) {
            if ($lines[$nextLineIndex] -match '^(#+)\s+' -and $Matches[1].Length -le $headingLevel) {
                $sectionEnd = $nextLineIndex - 1
                break
            }
        }

        $section = ($lines[$lineIndex..$sectionEnd]) -join "`n"
        $containsAllNeedles = $true
        foreach ($needle in $Needles) {
            if ([string]::IsNullOrWhiteSpace($needle) -or -not $section.Contains($needle)) {
                $containsAllNeedles = $false
                break
            }
        }

        if ($containsAllNeedles) {
            return $true
        }
    }

    return $false
}

function Test-ReleaseEntryContentControlBulletRunContainsAll {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    $anchor = "content_control_data_binding.bound_placeholder"
    $contentControlBulletPattern = "(?i)(content-control|content_control_data_binding)"
    $lines = $Text -split "\r?\n"
    for ($lineIndex = 0; $lineIndex -lt $lines.Count; $lineIndex++) {
        if (-not $lines[$lineIndex].Contains($anchor)) {
            continue
        }
        if ($lines[$lineIndex] -notmatch '^\s*-\s*') {
            continue
        }

        $blockStart = $lineIndex
        while (
            $blockStart -gt 0 -and
            $lines[$blockStart - 1] -match '^\s*-\s*' -and
            $lines[$blockStart - 1] -match $contentControlBulletPattern
        ) {
            $blockStart--
        }

        $blockEnd = $lineIndex
        while (
            $blockEnd + 1 -lt $lines.Count -and
            $lines[$blockEnd + 1] -match '^\s*-\s*' -and
            $lines[$blockEnd + 1] -match $contentControlBulletPattern
        ) {
            $blockEnd++
        }

        $block = ($lines[$blockStart..$blockEnd]) -join "`n"
        $containsAllNeedles = $true
        foreach ($needle in $Needles) {
            if ([string]::IsNullOrWhiteSpace($needle) -or -not $block.Contains($needle)) {
                $containsAllNeedles = $false
                break
            }
        }

        if ($containsAllNeedles) {
            return $true
        }
    }

    return $false
}

function Test-ReleaseEntryContentControlTraceBlockContainsAll {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    $anchor = "content_control_data_binding.bound_placeholder"
    return (
        (Test-MarkdownListBlockContainsAll -Text $Text -Anchor $anchor -Needles $Needles) -or
        (Test-ReleaseEntryContentControlBulletRunContainsAll -Text $Text -Needles $Needles)
    )
}

function Test-ReleaseEntryGovernanceMetricTraceBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    return (
        (Test-TextLineContainsAll -Text $Text -Needles $Needles) -or
        (Test-MarkdownListBlockContainsAll -Text $Text -Anchor $Anchor -Needles $Needles)
    )
}

function Add-ReleaseEntryGovernanceMetricDetailViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations,
        [string]$Label,
        [string]$Anchor,
        [string]$Description,
        [string[]]$Needles
    )

    $blockNeedles = @($Anchor) + $Needles
    if (Test-ReleaseEntryGovernanceMetricTraceBlockContainsAll -Text $Content -Anchor $Anchor -Needles $blockNeedles) {
        return
    }

    $foundMissingNeedle = $false
    foreach ($needle in $Needles) {
        if (-not (Test-ReleaseEntryGovernanceMetricTraceBlockContainsAll -Text $Content -Anchor $Anchor -Needles @($Anchor, $needle))) {
            $foundMissingNeedle = $true
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $Label `
                -Text "Entry document mentions $Description without required detail marker '$needle'."
        }
    }

    if (-not $foundMissingNeedle) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $Label `
            -Text "Entry document must keep $Description detail markers in the same line or Markdown list block."
    }
}

function Add-ReleaseEntryProjectTemplateDetailViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations,
        [string]$Label,
        [string]$Anchor,
        [string]$Description,
        [string[]]$Needles
    )

    $blockNeedles = @($Anchor) + $Needles
    if (Test-MarkdownListBlockContainsAll -Text $Content -Anchor $Anchor -Needles $blockNeedles) {
        return
    }

    $foundMissingNeedle = $false
    foreach ($needle in $Needles) {
        if (-not (Test-MarkdownListBlockContainsAll -Text $Content -Anchor $Anchor -Needles @($Anchor, $needle))) {
            $foundMissingNeedle = $true
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $Label `
                -Text "Entry document mentions $Description without required block marker '$needle'."
        }
    }

    if (-not $foundMissingNeedle) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $Label `
            -Text "Entry document must keep $Description detail markers in the same Markdown list block."
    }
}

function Test-ReleaseEntryProjectTemplateTraceBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    return (Test-MarkdownListBlockContainsAll -Text $Text -Anchor $Anchor -Needles $Needles)
}

function Test-ReleaseGovernanceHandoffProjectTemplateTraceBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    return Test-MarkdownListBlockContainsAll -Text $Text -Anchor $Anchor -Needles (@($Anchor) + $Needles)
}

function Test-ReleaseHandoffProjectTemplateTraceBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    return Test-MarkdownListBlockContainsAll -Text $Text -Anchor $Anchor -Needles (@($Anchor) + $Needles)
}

function Test-FinalReviewProjectTemplateTraceBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles
    )

    return Test-MarkdownListBlockContainsAll -Text $Text -Anchor $Anchor -Needles (@($Anchor) + $Needles)
}

$ProjectTemplateDeliveryReadinessSourceNeedles = @("project_template_delivery_readiness", "project-template-delivery-readiness")
$ProjectTemplateOnboardingGovernanceSourceNeedles = @("project_template_onboarding_governance", "project-template-onboarding-governance")
$ProjectTemplateGovernanceReportSourceNeedles = @(
    "project_template_delivery_readiness",
    "project-template-delivery-readiness",
    "project_template_onboarding_governance",
    "project-template-onboarding-governance"
)
$ProjectTemplateReadinessStatusValues = @("ready", "blocked", "failed", "pending_review", "needs_review")
$ProjectTemplateBooleanValues = @("true", "false")

function Add-ReleaseEntryDocumentGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    $governanceMarkers = @(
        "content_control_data_binding.bound_placeholder",
        "project_template_delivery_readiness",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance",
        "table_layout_delivery_governance.delivery_quality",
        "real_corpus_confidence",
        "delivery_quality",
        "Release Governance Rollup Details",
        "Release Governance Handoff Details"
    )
    if (-not (Test-TextContainsAny -Text $Content -Needles $governanceMarkers)) {
        return
    }

    $label = "release entry governance trace"

    if ($Content.Contains("content_control_data_binding.bound_placeholder")) {
        $contentControlAnchor = "content_control_data_binding.bound_placeholder"
        $contentControlNeedles = @(
            "featherdoc.content_control_data_binding_governance_report.v1",
            "source_json_display",
            "input_docx",
            "template_name",
            "schema_target",
            "target_mode",
            "repair_strategy",
            "repair_hint",
            "Rerun Custom XML sync",
            "sync_bound_content_control",
            "command_template",
            "sync-content-controls-from-custom-xml"
        )
        $contentControlBlockNeedles = @($contentControlAnchor) + $contentControlNeedles
        if (-not (Test-ReleaseEntryContentControlTraceBlockContainsAll -Text $Content -Needles $contentControlBlockNeedles)) {
            $foundMissingNeedle = $false
            foreach ($needle in $contentControlNeedles) {
                if (-not (Test-ReleaseEntryContentControlTraceBlockContainsAll -Text $Content -Needles @($contentControlAnchor, $needle))) {
                    $foundMissingNeedle = $true
                    Add-AuditViolation `
                        -Violations $Violations `
                        -File $File `
                        -Label $label `
                        -Text "Entry document mentions content_control_data_binding.bound_placeholder without required repair workflow marker '$needle'."
                }
            }

            if (-not $foundMissingNeedle) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Entry document must keep content_control_data_binding.bound_placeholder repair workflow markers in the same Markdown list block or adjacent content-control entry block."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_delivery_readiness", "project_template_onboarding.schema_approval", "project_template_onboarding_governance")) {
        if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
            "Project template release readiness checklist",
            "docs/project_template_release_readiness_checklist_zh.rst"
        ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document must point reviewers at docs/project_template_release_readiness_checklist_zh.rst when project-template governance evidence is present."
        }

        Add-ReleaseEntryProjectTemplateDetailViolations `
            -File $File `
            -Content $Content `
            -Violations $Violations `
            -Label $label `
            -Anchor "project_template_delivery_readiness" `
            -Description "project_template_delivery_readiness" `
            -Needles @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "featherdoc.project_template_delivery_readiness_report.v1",
            "status:",
            "release_ready:",
            "latest_schema_approval_gate_status",
            "source_report_display",
            "source_json_display"
        )

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor "project_template_delivery_readiness" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document project template delivery readiness status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor "project_template_delivery_readiness" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document project template delivery readiness release_ready must be true or false."
        }

        if (-not (Test-ReleaseEntryProjectTemplateTraceBlockContainsAll -Text $Content -Anchor "project_template_delivery_readiness" -Needles @(
            "project_template_delivery_readiness",
            "schema_approval_status_summary="
        ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document lost project template delivery readiness schema approval summary marker 'schema_approval_status_summary='."
        }

        Add-ReleaseEntryProjectTemplateDetailViolations `
            -File $File `
            -Content $Content `
            -Violations $Violations `
            -Label $label `
            -Anchor "project_template_onboarding.schema_approval" `
            -Description "project_template_onboarding.schema_approval" `
            -Needles @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance_contract",
            "featherdoc.project_template_onboarding_governance_report.v1",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )
    }

    if (Test-TextContainsAny -Text $Content -Needles @("delivery_quality", "table_layout_delivery_governance")) {
        if (-not $Content.Contains("table_layout_delivery_governance.delivery_quality")) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document mentions delivery_quality without table_layout_delivery_governance.delivery_quality."
        }
    }

    if ($Content.Contains("numbering_catalog_governance.real_corpus_confidence")) {
        Add-ReleaseEntryGovernanceMetricDetailViolations `
            -File $File `
            -Content $Content `
            -Violations $Violations `
            -Label $label `
            -Anchor "numbering_catalog_governance.real_corpus_confidence" `
            -Description "numbering_catalog_governance.real_corpus_confidence" `
            -Needles @(
            "catalog_coverage_percent",
            "baseline_coverage_percent",
            "coverage_score",
            "matched_document_count",
            "unmatched_catalog_document_count",
            "unmatched_baseline_document_count",
            "alignment_gap_count",
            "catalog_document_keys",
            "baseline_document_keys",
            "matched_document_keys",
            "penalty_summary",
            "style_numbering_issues"
        )
    }

    if ($Content.Contains("table_layout_delivery_governance.delivery_quality")) {
        Add-ReleaseEntryGovernanceMetricDetailViolations `
            -File $File `
            -Content $Content `
            -Violations $Violations `
            -Label $label `
            -Anchor "table_layout_delivery_governance.delivery_quality" `
            -Description "table_layout_delivery_governance.delivery_quality" `
            -Needles @(
            "table_style_issue_count",
            "automatic_tblLook_fix_count",
            "manual_table_style_fix_count",
            "table_position_automatic_count",
            "table_position_review_count",
            "command_failure_count",
            "ready_document_percent",
            "unresolved_item_count",
            "penalty_summary",
            "floating_table_plans_pending"
        )
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistEntrypointsEvidenceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist handoff evidence:",
        "project_template_readiness_checklist_entrypoints_source_reports="
    ))) {
        return
    }

    $label = "release entry project template readiness checklist handoff evidence trace"
    if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
        "Project-template readiness checklist handoff evidence",
        "project_template_readiness_checklist_entrypoints_source_reports=",
        "status=",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "entrypoints=",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "marker=release_entry_project_template_readiness_checklist_trace",
        "source_report="
    ))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry must keep project-template readiness checklist handoff evidence count, status, checklist path, required entrypoints, marker, and source report on the same compact evidence line."
    }

    if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "Project-template readiness checklist handoff evidence" `
                -FieldName "source_report" `
                -Needles @("release-candidate-checks", "release_candidate_summary"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry project-template checklist handoff source_report must identify the release-candidate summary evidence source."
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist packaged audit evidence",
        "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports="
    ))) {
        return
    }

    $label = "release entry project template readiness checklist packaged audit evidence trace"
    if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
        "Project-template readiness checklist packaged audit evidence",
        "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=",
        "status=passed",
        "audit_script=",
        "assert_release_material_safety.ps1",
        "audited_entrypoint_count=3",
        "audited_entrypoints=",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "compact_evidence_label=Project-template readiness checklist handoff evidence",
        "compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "checklist_marker=release_entry_project_template_readiness_checklist_trace",
        "material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
        "source_report="
    ))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry must keep packaged project-template readiness checklist material-safety audit count, status, audit script, audited entrypoints, compact evidence identity, checklist path, checklist marker, material-safety marker, and source report on the same compact evidence line."
    }

    if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "Project-template readiness checklist packaged audit evidence" `
                -FieldName "source_report" `
                -Needles @("release-blocker-rollup", "release_blocker_rollup"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release entry project-template packaged audit source_report must identify the release-blocker rollup evidence source."
    }
}

function Add-ReleaseEntryPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -notin @("start_here.md", "artifact_guide.md", "reviewer_checklist.md")) {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "PDF visual gate summary",
        "PDF visual gate aggregate contact sheet"
    ))) {
        return
    }

    $label = "release entry PDF visual gate trace"
    if (-not (Test-TextLineContainsAll -Text $Content -Needles @(
        "PDF release readiness checklist",
        "docs/pdf_release_readiness_checklist_zh.rst"
    ))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Entry document must point reviewers at docs/pdf_release_readiness_checklist_zh.rst when PDF visual gate evidence is present."
    }

    if ($leafName -in @("start_here.md", "artifact_guide.md")) {
        $evidenceNeedles = @(
            "PDF visual gate summary:",
            "summary.json",
            "PDF CJK manifest samples:",
            "PDF CJK copy/search samples:",
            "PDF visual baseline manifest samples:",
            "PDF visual baselines:",
            "PDF visual gate aggregate contact sheet:",
            "aggregate-contact-sheet.png"
        )

        if (-not (Test-MarkdownListRunContainsAll -Text $Content -Anchor "PDF visual gate summary:" -Needles $evidenceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Entry document must keep PDF visual gate summary, counts, and contact-sheet evidence in the same Markdown list run."
        }

        return
    }

    $finalizeEvidenceNeedles = @(
        "Confirm the PDF visual gate finalize evidence",
        'verdict `',
        "summary ",
        "summary.json",
        "aggregate contact sheet",
        "aggregate-contact-sheet.png",
        "CJK manifest samples",
        "CJK copy/search samples",
        "missing text",
        "visual baseline manifest samples",
        "visual baselines"
    )
    if (Test-TextLineContainsAll -Text $Content -Needles $finalizeEvidenceNeedles) {
        return
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("Confirm PDF visual gate summary", "summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Reviewer checklist must keep the PDF visual gate summary path on the summary confirmation line."
    }

    foreach ($needle in @(
        "CJK copy/search samples",
        "visual baselines"
    )) {
        if (-not (Test-TextLineContainsAll -Text $Content -Needles @("Confirm PDF visual gate summary", $needle))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Reviewer checklist must keep PDF visual gate marker '$needle' on the summary confirmation line."
        }
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("Confirm PDF visual gate aggregate contact sheet", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Reviewer checklist must keep the PDF visual gate contact-sheet path on the contact-sheet confirmation line."
    }
}

function Add-ReleaseSummaryProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_summary.zh-cn.md") {
        return
    }

    $label = "release summary project template governance trace"

    if ($Content.Contains("project-template readiness governance contract")) {
        foreach ($needle in @(
            "project-template readiness governance contract",
            "status=",
            "release_ready=",
            "latest_schema_approval_gate_status=",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceLineContainsAll -Text $Content -Anchor "project-template readiness governance contract" -Needles @($needle))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release summary lost project template readiness trace marker '$needle'."
            }
        }

        foreach ($fieldName in @("source_report_display", "source_json_display")) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "project-template readiness governance contract" `
                -FieldName $fieldName `
                -Needles @("project_template_delivery_readiness", "project-template-delivery-readiness"))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release summary project template readiness $fieldName must identify the delivery readiness evidence source."
            }
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "project-template readiness governance contract" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template readiness status must be a recognized readiness state."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "project-template readiness governance contract" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template readiness release_ready must be true or false."
        }
    }

    if ($Content.Contains("project-template onboarding governance contract")) {
        foreach ($needle in @(
            "project-template onboarding governance contract",
            "status=",
            "release_ready=",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceLineContainsAll -Text $Content -Anchor "project-template onboarding governance contract" -Needles @($needle))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release summary lost project template onboarding trace marker '$needle'."
            }
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
            -Text $Content `
            -Anchor "project-template onboarding governance contract" `
            -FieldName "source_report_display" `
            -Needles @(
                "project_template_delivery_readiness",
                "project-template-delivery-readiness",
                "project_template_onboarding_governance",
                "project-template-onboarding-governance"
            ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template onboarding source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
            -Text $Content `
            -Anchor "project-template onboarding governance contract" `
            -FieldName "source_json_display" `
            -Needles @("project_template_onboarding_governance", "project-template-onboarding-governance"))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template onboarding source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "project-template onboarding governance contract" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "project-template onboarding governance contract" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary project template onboarding release_ready must be true or false."
        }
    }
}

function Add-ReleaseSummaryPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_summary.zh-cn.md") {
        return
    }

    if (-not $Content.Contains("PDF visual gate")) {
        return
    }

    $label = "release summary PDF visual gate trace"
    foreach ($needle in @(
        "PDF visual gate",
        "verdict=",
        "summary=",
        "summary.json",
        "aggregate-contact-sheet.png",
        "cjk_manifest_count=",
        "cjk_copy_search_count=",
        "visual_baseline_manifest_count=",
        "visual_baseline_count="
    )) {
        if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate", $needle))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release summary must keep PDF visual gate marker '$needle' on the PDF visual gate summary line."
        }
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate", "verdict=pass")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate", "verdict=fail"))
    )) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release summary must keep a pass/fail PDF visual gate verdict on the PDF visual gate summary line."
    }
}

function Add-ReleaseBodyProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_body.zh-cn.md") {
        return
    }

    $label = "release body project template governance trace"

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_delivery_readiness", "project_template_delivery_readiness_contract")) {
        foreach ($needle in @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "source_schema=featherdoc.project_template_delivery_readiness_report.v1",
            "status=",
            "release_ready=",
            "latest_schema_approval_gate_status=",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceLineContainsAll -Text $Content -Anchor "Project template readiness:" -Needles @($needle))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release body lost project template readiness trace marker '$needle'."
            }
        }

        foreach ($fieldName in @("source_report_display", "source_json_display")) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
                -Text $Content `
                -Anchor "Project template readiness:" `
                -FieldName $fieldName `
                -Needles @("project_template_delivery_readiness", "project-template-delivery-readiness"))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release body project template readiness $fieldName must identify the delivery readiness evidence source."
            }
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "Project template readiness:" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template readiness status must be a recognized readiness state."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "Project template readiness:" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template readiness release_ready must be true or false."
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_onboarding.schema_approval", "project_template_onboarding_governance_contract")) {
        foreach ($needle in @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance",
            "project_template_onboarding_governance_contract",
            "source_schema=featherdoc.project_template_onboarding_governance_report.v1",
            "status=",
            "release_ready=",
            "schema_approval_status_summary=",
            "source_report_display=",
            "source_json_display="
        )) {
            if (-not (Test-ReleaseNoteProjectTemplateTraceLineContainsAll -Text $Content -Anchor "Project template onboarding:" -Needles @($needle))) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release body lost project template onboarding trace marker '$needle'."
            }
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
            -Text $Content `
            -Anchor "Project template onboarding:" `
            -FieldName "source_report_display" `
            -Needles @(
                "project_template_delivery_readiness",
                "project-template-delivery-readiness",
                "project_template_onboarding_governance",
                "project-template-onboarding-governance"
            ))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template onboarding source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldIdentifies `
            -Text $Content `
            -Anchor "Project template onboarding:" `
            -FieldName "source_json_display" `
            -Needles @("project_template_onboarding_governance", "project-template-onboarding-governance"))) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template onboarding source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "Project template onboarding:" `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-ReleaseNoteProjectTemplateTraceFieldValueInSet `
            -Text $Content `
            -Anchor "Project template onboarding:" `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body project template onboarding release_ready must be true or false."
        }
    }
}

function Add-ReleaseBodyPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_body.zh-cn.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "PDF visual gate summary",
        "PDF visual gate verdict",
        "PDF visual gate aggregate contact sheet"
    ))) {
        return
    }

    $label = "release body PDF visual gate trace"
    $evidenceNeedles = @(
        "PDF visual gate summary",
        "PDF visual gate evidence status",
        "PDF visual gate verdict",
        "PDF visual gate aggregate contact sheet",
        "aggregate-contact-sheet.png",
        "PDF CJK manifest samples",
        "PDF CJK copy/search samples",
        "PDF visual baseline manifest samples",
        "PDF visual baselines"
    )
    foreach ($needle in $evidenceNeedles) {
        if (-not $Content.Contains($needle)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release body lost PDF visual gate trace marker '$needle'."
        }
    }

    if (-not (Test-MarkdownListRunContainsAll -Text $Content -Anchor "PDF visual gate summary" -Needles $evidenceNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release body must keep PDF visual gate status, verdict, counts, and contact-sheet evidence in the same Markdown list run."
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict", "pass")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict", "fail"))
    )) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release body must keep a pass/fail PDF visual gate verdict on the PDF visual gate verdict line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate summary", "summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release body must keep the PDF visual gate summary path on the PDF visual gate summary line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate aggregate contact sheet", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release body must keep the PDF visual gate aggregate contact-sheet path on the PDF visual gate aggregate contact sheet line."
    }
}

function Add-ReleaseHandoffProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_handoff.md") {
        return
    }

    $label = "release handoff project template governance trace"

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_delivery_readiness:", "project_template_delivery_readiness_contract")) {
        $readinessAnchor = "project_template_delivery_readiness:"
        $readinessBlockNeedles = @(
            "project_template_delivery_readiness",
            "project_template_delivery_readiness_contract",
            "source_schema: featherdoc.project_template_delivery_readiness_report.v1",
            "status:",
            "release_ready:",
            "latest_schema_approval_gate_status:",
            "schema_approval_status_summary:",
            "source_report_display:",
            "source_json_display:"
        )
        if (-not (Test-ReleaseHandoffProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $readinessAnchor -Needles $readinessBlockNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff must keep project template readiness schema, contract, status, and source displays in the same list block."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $readinessAnchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template readiness status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $readinessAnchor `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template readiness release_ready must be true or false."
        }

        foreach ($fieldName in @("source_report_display", "source_json_display")) {
            if (-not (Test-MarkdownListBlockFieldValuesIdentify `
                -Text $Content `
                -Anchor $readinessAnchor `
                -FieldName $fieldName `
                -Needles $ProjectTemplateDeliveryReadinessSourceNeedles `
                -ForbiddenNeedles $ProjectTemplateOnboardingGovernanceSourceNeedles)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release handoff project template readiness $fieldName must identify the delivery readiness evidence source."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @("project_template_onboarding.schema_approval", "project_template_onboarding_governance_contract")) {
        $onboardingAnchor = "project_template_onboarding.schema_approval"
        $onboardingBlockNeedles = @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance_contract",
            "source_schema: featherdoc.project_template_onboarding_governance_report.v1",
            "status:",
            "release_ready:",
            "schema_approval_status_summary:",
            "source_report_display:",
            "source_json_display:"
        )
        if (-not (Test-ReleaseHandoffProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $onboardingAnchor -Needles $onboardingBlockNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff must keep project template onboarding schema approval, contract, and source displays in the same list block."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "source_report_display" `
            -Needles $ProjectTemplateGovernanceReportSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template onboarding source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "source_json_display" `
            -Needles $ProjectTemplateOnboardingGovernanceSourceNeedles `
            -ForbiddenNeedles $ProjectTemplateDeliveryReadinessSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template onboarding source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff project template onboarding release_ready must be true or false."
        }
    }
}

function Add-ReleaseHandoffPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "PDF visual gate summary:",
        "PDF visual gate verdict:",
        "PDF visual gate aggregate contact sheet:"
    ))) {
        return
    }

    $label = "release handoff PDF visual gate trace"
    $evidenceNeedles = @(
        "PDF visual gate summary:",
        "PDF visual gate evidence status:",
        "PDF visual gate verdict:",
        "PDF visual gate aggregate contact sheet:",
        "aggregate-contact-sheet.png",
        "PDF CJK manifest samples:",
        "PDF CJK copy/search samples:",
        "PDF visual baseline manifest samples:",
        "PDF visual baselines:"
    )
    foreach ($needle in $evidenceNeedles) {
        if (-not $Content.Contains($needle)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release handoff lost PDF visual gate trace marker '$needle'."
        }
    }

    if (-not (Test-MarkdownListRunContainsAll -Text $Content -Anchor "PDF visual gate summary:" -Needles $evidenceNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release handoff must keep PDF visual gate status, verdict, counts, and contact-sheet evidence in the same Markdown list run."
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict:", "pass")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict:", "fail"))
    )) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release handoff must keep a pass/fail PDF visual gate verdict on the PDF visual gate verdict line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate summary:", "summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release handoff must keep the PDF visual gate summary path on the PDF visual gate summary line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate aggregate contact sheet:", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release handoff must keep the PDF visual gate contact-sheet path on the PDF visual gate aggregate contact sheet line."
    }
}

function Add-ReleaseGovernanceHandoffProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    $label = "release governance handoff project template trace"

    if (Test-TextContainsAny -Text $Content -Needles @(
        "project_template_delivery_readiness",
        "featherdoc.project_template_delivery_readiness_report.v1",
        "latest_schema_approval_gate_status"
    )) {
        $readinessAnchor = '`project_template_delivery_readiness`:'
        $readinessBlockNeedles = @(
            "project_template_delivery_readiness",
            "featherdoc.project_template_delivery_readiness_report.v1",
            "status=",
            "ready=",
            "latest_schema_approval_gate_status",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )
        if (-not (Test-ReleaseGovernanceHandoffProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $readinessAnchor -Needles $readinessBlockNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff must keep project template readiness schema, status, and source displays in the same report-status block."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $readinessAnchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template readiness status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $readinessAnchor `
            -FieldName "ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template readiness ready must be true or false."
        }

        foreach ($fieldName in @("source_report_display", "source_json_display")) {
            if (-not (Test-MarkdownListBlockFieldValuesIdentify `
                -Text $Content `
                -Anchor $readinessAnchor `
                -FieldName $fieldName `
                -Needles $ProjectTemplateDeliveryReadinessSourceNeedles `
                -ForbiddenNeedles $ProjectTemplateOnboardingGovernanceSourceNeedles)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "Release governance handoff project template readiness $fieldName must identify the delivery readiness evidence source."
            }
        }
    }

    if (Test-TextContainsAny -Text $Content -Needles @(
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract",
        "featherdoc.project_template_onboarding_governance_report.v1"
    )) {
        $onboardingAnchor = "project_template_onboarding.schema_approval"
        $onboardingBlockNeedles = @(
            "project_template_onboarding.schema_approval",
            "project_template_onboarding_governance_contract",
            "featherdoc.project_template_onboarding_governance_report.v1",
            "status:",
            "release_ready:",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )
        if (-not (Test-ReleaseGovernanceHandoffProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $onboardingAnchor -Needles $onboardingBlockNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff must keep project template onboarding schema approval, contract, and source displays in the same blocker block."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "source_report_display" `
            -Needles $ProjectTemplateGovernanceReportSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template onboarding source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "source_json_display" `
            -Needles $ProjectTemplateOnboardingGovernanceSourceNeedles `
            -ForbiddenNeedles $ProjectTemplateDeliveryReadinessSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template onboarding source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $onboardingAnchor `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Release governance handoff project template onboarding release_ready must be true or false."
        }
    }
}

function Add-ReleaseGovernanceHandoffPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "PDF visual gate evidence source reports",
        "pdf_visual_gate_verdict",
        "pdf_visual_gate_summary_json_display"
    ))) {
        return
    }

    $label = "release governance handoff PDF visual gate trace"
    if (-not $Content.Contains("PDF visual gate evidence source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff lost PDF visual gate trace marker 'PDF visual gate evidence source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "pdf_visual_gate_status:",
        "full_visual_gate_status:",
        "pdf_visual_gate_verdict:",
        "pdf_visual_gate_finalizable:",
        "pdf_visual_gate_summary_json_display:",
        "pdf_visual_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_gate_cjk_manifest_count:",
        "pdf_visual_gate_cjk_copy_search_count:",
        "pdf_visual_gate_visual_baseline_manifest_count:",
        "pdf_visual_gate_visual_baseline_count:",
        "pdf_bounded_ctest_summary_count:",
        "pdf_bounded_ctest_pass_count:",
        "pdf_bounded_ctest_skipped_test_count:",
        "pdf_bounded_ctest_selected_test_count:",
        "pdf_bounded_ctest_subsets:",
        "pdf_bounded_ctest_summary_json_display:"
    )
    if (-not (Test-MarkdownListBlockContainsAll -Text $Content -Anchor "source_report:" -Needles $sourceReportBlockNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff must keep PDF visual gate source report, full visual status, verdict, counts, contact-sheet path, and bounded CTest auxiliary evidence in the same source_report block."
    }
}

function Add-ReleaseGovernanceHandoffPdfVisualGateAttemptTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "pdf_visual_gate_attempt_status",
        "pdf_visual_gate_attempt_summary_json_display",
        "bounded_attempt_auxiliary_only"
    ))) {
        return
    }

    $label = "release governance handoff PDF visual gate attempt trace"
    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "pdf_visual_gate_attempt_status:",
        "pdf_visual_gate_attempt_verdict:",
        "pdf_visual_gate_attempt_full_visual_gate_status:",
        "pdf_visual_gate_attempt_evidence_scope:",
        "bounded_attempt_auxiliary_only",
        "pdf_visual_gate_attempt_summary_json_display:",
        "attempt-summary.json",
        "pdf_visual_gate_attempt_stage_count:",
        "pdf_visual_gate_attempt_passed_stage_count:",
        "pdf_visual_gate_attempt_failed_stage_count:",
        "pdf_visual_gate_attempt_incomplete_stage_count:",
        "pdf_visual_gate_attempt_pdf_cli_export_status:",
        "pdf_visual_gate_attempt_pdf_regression_status:",
        "pdf_visual_gate_attempt_pdf_regression_selected_test_count:",
        "pdf_visual_gate_attempt_pdf_regression_failed_test_count:",
        "pdf_visual_gate_attempt_pdf_regression_skipped_test_count:",
        "pdf_visual_gate_attempt_unicode_font_status:",
        "pdf_visual_gate_attempt_cjk_copy_search_status:",
        "pdf_visual_gate_attempt_cjk_copy_search_count:",
        "pdf_visual_gate_attempt_cjk_copy_search_missing_text_count:",
        "pdf_visual_gate_attempt_visual_baseline_render_status:",
        "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count:",
        "pdf_visual_gate_attempt_expected_visual_render_count:",
        "pdf_visual_gate_attempt_aggregate_contact_sheet_status:",
        "pdf_visual_gate_attempt_aggregate_contact_sheet_display:"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll -Text $Content -Anchor "source_report:" -Needles $sourceReportBlockNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff must keep PDF visual gate bounded attempt status, verdict, auxiliary scope, CTest counts, CJK evidence, render progress, and contact-sheet status in the same source_report block."
    }
}

function Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistEntrypointsTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist entrypoints evidence source reports",
        "project_template_readiness_checklist_entrypoints_status",
        "project_template_readiness_checklist_entrypoints_checklist_path"
    ))) {
        return
    }

    $label = "release governance handoff project template readiness checklist entrypoints trace"
    if (-not $Content.Contains("Project-template readiness checklist entrypoints evidence source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff lost project-template readiness checklist source-report trace marker 'Project-template readiness checklist entrypoints evidence source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "featherdoc.release_candidate_summary",
        "project_template_readiness_checklist_entrypoints_status:",
        "project_template_readiness_checklist_entrypoints_checklist_label:",
        "Project template release readiness checklist",
        "project_template_readiness_checklist_entrypoints_checklist_path:",
        "docs/project_template_release_readiness_checklist_zh.rst",
        "project_template_readiness_checklist_entrypoints_required_entrypoint_count:",
        "project_template_readiness_checklist_entrypoints_entrypoint_ids:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "project_template_readiness_checklist_entrypoints_checklist_marker:",
        "release_entry_project_template_readiness_checklist_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll -Text $Content -Anchor "source_report:" -Needles $sourceReportBlockNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff must keep project-template readiness checklist release-candidate source identity, status, checklist path, required entrypoint ids, and fixed checklist marker in the same source_report block."
    }
}

function Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_governance_handoff.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Release-entry project-template readiness checklist material-safety audit source reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
    ))) {
        return
    }

    $label = "release governance handoff project template readiness checklist material-safety audit trace"
    if (-not $Content.Contains("Release-entry project-template readiness checklist material-safety audit source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff lost release-entry project-template readiness checklist material-safety audit source-report trace marker 'Release-entry project-template readiness checklist material-safety audit source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "featherdoc.release_candidate_summary",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status:",
        "passed",
        "release_entry_project_template_readiness_checklist_material_safety_audit_script:",
        "assert_release_material_safety.ps1",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count:",
        "3",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label:",
        "Project-template readiness checklist handoff evidence",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field:",
        "project_template_readiness_checklist_entrypoints_source_reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path:",
        "docs/project_template_release_readiness_checklist_zh.rst",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker:",
        "release_entry_project_template_readiness_checklist_trace",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker:",
        "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll -Text $Content -Anchor "source_report:" -Needles $sourceReportBlockNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Release governance handoff must keep release-entry project-template readiness checklist material-safety audit release-candidate source identity, status, audit script, audited entrypoints, compact evidence identity, checklist path, checklist marker, and material-safety marker in the same source_report block."
    }
}

function Add-FinalReviewProjectTemplateReadinessChecklistEntrypointsTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Project-template readiness checklist entrypoints evidence source reports",
        "project_template_readiness_checklist_entrypoints_status",
        "project_template_readiness_checklist_entrypoints_checklist_path"
    ))) {
        return
    }

    $label = "final review project template readiness checklist entrypoints trace"
    if (-not $Content.Contains("Project-template readiness checklist entrypoints evidence source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review lost project-template readiness checklist source-report trace marker 'Project-template readiness checklist entrypoints evidence source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "featherdoc.release_candidate_summary",
        "project_template_readiness_checklist_entrypoints_status:",
        "project_template_readiness_checklist_entrypoints_checklist_label:",
        "Project template release readiness checklist",
        "project_template_readiness_checklist_entrypoints_checklist_path:",
        "docs/project_template_release_readiness_checklist_zh.rst",
        "project_template_readiness_checklist_entrypoints_required_entrypoint_count:",
        "project_template_readiness_checklist_entrypoints_entrypoint_ids:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "project_template_readiness_checklist_entrypoints_checklist_marker:",
        "release_entry_project_template_readiness_checklist_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll -Text $Content -Anchor "source_report:" -Needles $sourceReportBlockNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep project-template readiness checklist release-candidate source identity, status, checklist path, required entrypoint ids, and fixed checklist marker in the same source_report block."
    }
}

function Add-FinalReviewProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Release-entry project-template readiness checklist material-safety audit source reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
    ))) {
        return
    }

    $label = "final review project template readiness checklist material-safety audit trace"
    if (-not $Content.Contains("Release-entry project-template readiness checklist material-safety audit source reports:")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review lost release-entry project-template readiness checklist material-safety audit source-report trace marker 'Release-entry project-template readiness checklist material-safety audit source reports:'."
    }

    $sourceReportBlockNeedles = @(
        "source_report:",
        "schema=",
        "featherdoc.release_candidate_summary",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status:",
        "passed",
        "release_entry_project_template_readiness_checklist_material_safety_audit_script:",
        "assert_release_material_safety.ps1",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count:",
        "3",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints:",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label:",
        "Project-template readiness checklist handoff evidence",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field:",
        "project_template_readiness_checklist_entrypoints_source_reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path:",
        "docs/project_template_release_readiness_checklist_zh.rst",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker:",
        "release_entry_project_template_readiness_checklist_trace",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker:",
        "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    )
    if (-not (Test-MarkdownAnyListBlockContainsAll -Text $Content -Anchor "source_report:" -Needles $sourceReportBlockNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep release-entry project-template readiness checklist material-safety audit release-candidate source identity, status, audit script, audited entrypoints, compact evidence identity, checklist path, checklist marker, and material-safety marker in the same source_report block."
    }
}

function Add-FinalReviewProjectTemplateGovernanceTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "Release governance handoff details",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract"
    ))) {
        return
    }

    $label = "final review project template governance trace"
    if (-not $Content.Contains("Release governance handoff details")) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review lost project template governance trace marker 'Release governance handoff details'."
    }

    $anchor = "project_template_delivery_readiness / project_template_onboarding.schema_approval"
    $blockNeedles = @(
        "featherdoc.project_template_onboarding_governance_report.v1",
        "project_template_onboarding_governance_contract",
        "schema_approval_status_summary",
        "status:",
        "release_ready:",
        "source_report_display:",
        "project-template-delivery-readiness",
        "source_json_display:",
        "project-template-onboarding-governance",
        "readiness_status:",
        "readiness_release_ready:"
    )

    if (Test-FinalReviewProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $anchor -Needles $blockNeedles) {
        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "source_report_display" `
            -Needles $ProjectTemplateGovernanceReportSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template source_report_display must identify a project-template governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesIdentify `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "source_json_display" `
            -Needles $ProjectTemplateOnboardingGovernanceSourceNeedles `
            -ForbiddenNeedles $ProjectTemplateDeliveryReadinessSourceNeedles)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template source_json_display must identify the onboarding governance evidence source."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "readiness_status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template readiness_status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "readiness_release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template readiness_release_ready must be true or false."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "status" `
            -AllowedValues $ProjectTemplateReadinessStatusValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template onboarding status must be a recognized readiness state."
        }

        if (-not (Test-MarkdownListBlockFieldValuesInSet `
            -Text $Content `
            -Anchor $anchor `
            -FieldName "release_ready" `
            -AllowedValues $ProjectTemplateBooleanValues)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review project template onboarding release_ready must be true or false."
        }

        return
    }

    $foundMissingNeedle = $false
    foreach ($needle in $blockNeedles) {
        if (-not (Test-FinalReviewProjectTemplateTraceBlockContainsAll -Text $Content -Anchor $anchor -Needles @($needle))) {
            $foundMissingNeedle = $true
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review lost project template governance trace marker '$needle' inside the handoff blocker block."
        }
    }

    if (-not $foundMissingNeedle) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep project template governance trace markers in the same handoff blocker block."
    }
}

function Add-FinalReviewPdfVisualGateTraceViolations {
    param(
        [string]$File,
        [string]$Content,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "final_review.md") {
        return
    }

    if (-not (Test-TextContainsAny -Text $Content -Needles @(
        "PDF visual gate verdict",
        "PDF visual gate counts",
        "PDF visual gate summary"
    ))) {
        return
    }

    $label = "final review PDF visual gate trace"
    $stepStatusNeedles = @(
        "PDF visual gate:",
        "PDF visual gate verdict:",
        "PDF visual gate counts:",
        "PDF visual gate manifest counts:",
        "PDF visual gate finalizable:"
    )
    foreach ($needle in $stepStatusNeedles) {
        if (-not $Content.Contains($needle)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "Final review lost PDF visual gate trace marker '$needle'."
        }
    }

    if (-not (Test-MarkdownListRunContainsAll -Text $Content -Anchor "PDF visual gate:" -Needles $stepStatusNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep PDF visual gate status, verdict, counts, and finalizable evidence in the same step-status Markdown list run."
    }

    $keyOutputNeedles = @(
        "PDF visual gate summary:",
        "summary.json",
        "PDF visual gate contact sheet:",
        "aggregate-contact-sheet.png"
    )
    if (-not (Test-MarkdownSectionContainsAll -Text $Content -Heading "## Key outputs" -Needles $keyOutputNeedles)) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep PDF visual gate summary and contact-sheet evidence inside the Key outputs section."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate summary:", "summary.json"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep the PDF visual gate summary path on the PDF visual gate summary line."
    }

    if (-not (
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict:", "pass")) -or
        (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate verdict:", "fail"))
    )) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep a pass/fail PDF visual gate verdict on the PDF visual gate verdict line."
    }

    if (-not (Test-TextLineContainsAll -Text $Content -Needles @("PDF visual gate contact sheet:", "aggregate-contact-sheet.png"))) {
        Add-AuditViolation `
            -Violations $Violations `
            -File $File `
            -Label $label `
            -Text "Final review must keep the PDF visual gate contact-sheet path on the PDF visual gate contact sheet line."
    }
}

function Add-ContentControlRepairContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $contentControlBlockerId = "content_control_data_binding.bound_placeholder"
    $expectedRepairStrategy = "sync_bound_content_control"
    $expectedRepairHintMarker = "Rerun Custom XML sync"
    $expectedCommand = "sync-content-controls-from-custom-xml"
    $label = "content-control repair contract"
    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()

    if ($leafName -eq "release_assets_manifest.json") {
        $contractsValue = Get-JsonPropertyValue -Object $Json -Name "content_control_repair_contracts"
        if ($null -eq $contractsValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing content_control_repair_contracts."
        } else {
            $contracts = @($contractsValue)
            $countValue = Get-JsonPropertyValue -Object $Json -Name "content_control_repair_contract_count"
            if ($null -eq $countValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing content_control_repair_contract_count."
            } else {
                try {
                    $declaredCount = [int]$countValue
                    if ($declaredCount -ne $contracts.Count) {
                        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "content_control_repair_contract_count does not match content_control_repair_contracts length."
                    }
                } catch {
                    Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "content_control_repair_contract_count must be an integer."
                }
            }
        }
    }

    $contentControlBlockers = @(Get-JsonObjectNodes -Value $Json | Where-Object {
        ([string](Get-JsonPropertyValue -Object $_ -Name "id")) -eq $contentControlBlockerId
    })

    if ($contentControlBlockers.Count -eq 0) {
        return
    }

    foreach ($blocker in $contentControlBlockers) {
        $sourceSchema = [string](Get-JsonPropertyValue -Object $blocker -Name "source_schema")
        if ($sourceSchema -ne "featherdoc.content_control_data_binding_governance_report.v1") {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must carry source_schema=featherdoc.content_control_data_binding_governance_report.v1."
        }

        $sourceJsonDisplay = [string](Get-JsonPropertyValue -Object $blocker -Name "source_json_display")
        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must carry source_json_display."
        }

        foreach ($fieldName in @("input_docx", "template_name", "schema_target", "target_mode")) {
            $fieldValue = [string](Get-JsonPropertyValue -Object $blocker -Name $fieldName)
            if ([string]::IsNullOrWhiteSpace($fieldValue)) {
                Add-AuditViolation `
                    -Violations $Violations `
                    -File $File `
                    -Label $label `
                    -Text "content_control_data_binding.bound_placeholder must carry $fieldName provenance."
            }
        }

        $repairStrategy = [string](Get-JsonPropertyValue -Object $blocker -Name "repair_strategy")
        if ($repairStrategy -ne $expectedRepairStrategy) {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must use repair_strategy=$expectedRepairStrategy."
        }

        $repairHint = [string](Get-JsonPropertyValue -Object $blocker -Name "repair_hint")
        if ([string]::IsNullOrWhiteSpace($repairHint) -or $repairHint -notlike "*$expectedRepairHintMarker*") {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder must carry repair_hint containing $expectedRepairHintMarker."
        }

        $commandTemplate = [string](Get-JsonPropertyValue -Object $blocker -Name "command_template")
        if ([string]::IsNullOrWhiteSpace($commandTemplate) -or $commandTemplate -notlike "*$expectedCommand*") {
            Add-AuditViolation `
                -Violations $Violations `
                -File $File `
                -Label $label `
                -Text "content_control_data_binding.bound_placeholder command_template must include $expectedCommand."
        }
    }
}

function Add-GovernanceMetricDetailsViolations {
    param(
        [string]$File,
        [string]$Label,
        [string]$MetricName,
        [string]$PropertyName,
        $SourceMetric,
        $ManifestMetric,
        [string[]]$RequiredFields,
        $Violations
    )

    $sourceDetails = Get-JsonPropertyValue -Object $SourceMetric -Name "details"
    if ($null -eq $sourceDetails) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "Governance metric $MetricName is missing details."
        return
    }

    $manifestDetails = Get-JsonPropertyValue -Object $ManifestMetric -Name "details"
    if ($null -eq $manifestDetails) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName is missing details."
        return
    }

    foreach ($fieldName in @($RequiredFields)) {
        $sourceValue = Get-JsonPropertyValue -Object $sourceDetails -Name $fieldName
        $manifestValue = Get-JsonPropertyValue -Object $manifestDetails -Name $fieldName

        if ($null -eq $manifestValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName.details is missing $fieldName."
            continue
        }

        if ([string]$manifestValue -ne [string]$sourceValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName.details.$fieldName does not match the source governance metric."
        }
    }

    $sourcePenaltySummary = @(Get-JsonArray -Object $sourceDetails -Name "penalty_summary")
    $manifestPenaltySummary = @(Get-JsonArray -Object $manifestDetails -Name "penalty_summary")
    if ($sourcePenaltySummary.Count -ne $manifestPenaltySummary.Count) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName.details.penalty_summary does not match the source governance metric."
        return
    }

    for ($index = 0; $index -lt $sourcePenaltySummary.Count; $index++) {
        $sourcePenalty = $sourcePenaltySummary[$index]
        $manifestPenalty = $manifestPenaltySummary[$index]
        foreach ($fieldName in @("factor", "count", "penalty")) {
            $sourceValue = Get-JsonPropertyValue -Object $sourcePenalty -Name $fieldName
            $manifestValue = Get-JsonPropertyValue -Object $manifestPenalty -Name $fieldName
            if ($null -eq $manifestValue -or [string]$manifestValue -ne [string]$sourceValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$PropertyName.details.penalty_summary[$index].$fieldName does not match the source governance metric."
            }
        }
    }
}

function Test-ReleaseGovernanceContractTarget {
    param(
        [string]$File,
        $Json
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -eq "release_assets_manifest.json") {
        return $true
    }

    if ($leafName -ne "summary.json") {
        return $false
    }

    $releaseVersion = Get-JsonPropertyValue -Object $Json -Name "release_version"
    $executionStatus = Get-JsonPropertyValue -Object $Json -Name "execution_status"
    if ([string]::IsNullOrWhiteSpace([string]$releaseVersion) -or
        [string]::IsNullOrWhiteSpace([string]$executionStatus)) {
        return $false
    }

    foreach ($marker in @("release_handoff", "artifact_guide", "reviewer_checklist", "governance_metrics")) {
        if ($null -ne (Get-JsonPropertyValue -Object $Json -Name $marker)) {
            return $true
        }
    }

    return $false
}

function Get-GovernanceMetricByContract {
    param(
        $Metrics,
        [string]$Metric,
        [string]$Id,
        [string]$ReportId,
        [string]$SourceSchema
    )

    return @($Metrics | Where-Object {
        ([string](Get-JsonPropertyValue -Object $_ -Name "metric")) -eq $Metric -and
        ([string](Get-JsonPropertyValue -Object $_ -Name "id")) -eq $Id -and
        ([string](Get-JsonPropertyValue -Object $_ -Name "report_id")) -eq $ReportId -and
        ([string](Get-JsonPropertyValue -Object $_ -Name "source_schema")) -eq $SourceSchema
    }) | Select-Object -First 1
}

function Add-GovernanceMetricContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    if (-not (Test-ReleaseGovernanceContractTarget -File $File -Json $Json)) {
        return
    }

    $label = "governance metrics contract"
    $metricsValue = Get-JsonPropertyValue -Object $Json -Name "governance_metrics"
    if ($null -eq $metricsValue) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing governance_metrics."
        return
    }

    $metrics = @($metricsValue)
    if ($metrics.Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "governance_metrics must not be empty."
        return
    }

    $countValue = Get-JsonPropertyValue -Object $Json -Name "governance_metric_count"
    if ($null -eq $countValue) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing governance_metric_count."
    } else {
        try {
            $declaredCount = [int]$countValue
            if ($declaredCount -ne $metrics.Count) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "governance_metric_count does not match governance_metrics length."
            }
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "governance_metric_count must be an integer."
        }
    }

    $requiredMetricContracts = @(
        [ordered]@{
            metric = "real_corpus_confidence"
            id = "numbering_catalog_governance.real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        },
        [ordered]@{
            metric = "delivery_quality"
            id = "table_layout_delivery_governance.delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        }
    )

    foreach ($requiredMetric in $requiredMetricContracts) {
        $metricName = [string]$requiredMetric.metric
        $metric = Get-GovernanceMetricByContract `
            -Metrics $metrics `
            -Metric $metricName `
            -Id ([string]$requiredMetric.id) `
            -ReportId ([string]$requiredMetric.report_id) `
            -SourceSchema ([string]$requiredMetric.source_schema)

        if ($null -eq $metric) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing required governance metric: $($requiredMetric.id)."
            continue
        }

        $level = Get-JsonPropertyValue -Object $metric -Name "level"
        if ([string]::IsNullOrWhiteSpace([string]$level)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Governance metric $metricName is missing level."
        }

        $score = Get-JsonPropertyValue -Object $metric -Name "score"
        if ($null -eq $score) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Governance metric $metricName is missing score."
        }
    }

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $manifestMetricMirrors = @(
        [ordered]@{
            metric = "real_corpus_confidence"
            id = "numbering_catalog_governance.real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            property = "numbering_catalog_real_corpus_confidence"
        },
        [ordered]@{
            metric = "delivery_quality"
            id = "table_layout_delivery_governance.delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            property = "table_layout_delivery_quality"
        }
    )

    foreach ($mirror in $manifestMetricMirrors) {
        $metricName = [string]$mirror.metric
        $propertyName = [string]$mirror.property
        $sourceMetric = Get-GovernanceMetricByContract `
            -Metrics $metrics `
            -Metric $metricName `
            -Id ([string]$mirror.id) `
            -ReportId ([string]$mirror.report_id) `
            -SourceSchema ([string]$mirror.source_schema)

        if ($null -eq $sourceMetric) {
            continue
        }

        $manifestMetric = Get-JsonPropertyValue -Object $Json -Name $propertyName
        if ($null -eq $manifestMetric) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing $propertyName."
            continue
        }

        foreach ($fieldName in @("id", "metric", "report_id", "source_schema", "level", "score")) {
            $manifestValue = Get-JsonPropertyValue -Object $manifestMetric -Name $fieldName
            $metricValue = Get-JsonPropertyValue -Object $sourceMetric -Name $fieldName
            if ($null -eq $manifestValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "$propertyName is missing $fieldName."
                continue
            }

            if ([string]$manifestValue -ne [string]$metricValue) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "$propertyName.$fieldName does not match $metricName governance metric."
            }
        }

        if ($metricName -eq "real_corpus_confidence") {
            Add-GovernanceMetricDetailsViolations `
                -File $File `
                -Label $label `
                -MetricName $metricName `
                -PropertyName $propertyName `
                -SourceMetric $sourceMetric `
                -ManifestMetric $manifestMetric `
                -RequiredFields @(
                    "score",
                    "level",
                    "document_count",
                    "catalog_exemplar_count",
                    "baseline_entry_count",
                    "catalog_coverage_percent",
                    "baseline_coverage_percent",
                    "coverage_score",
                    "matched_document_count",
                    "unmatched_catalog_document_count",
                    "unmatched_baseline_document_count",
                    "alignment_gap_count",
                    "catalog_document_keys",
                    "baseline_document_keys",
                    "matched_document_keys"
                ) `
                -Violations $Violations
        } elseif ($metricName -eq "delivery_quality") {
            Add-GovernanceMetricDetailsViolations `
                -File $File `
                -Label $label `
                -MetricName $metricName `
                -PropertyName $propertyName `
                -SourceMetric $sourceMetric `
                -ManifestMetric $manifestMetric `
                -RequiredFields @(
                    "score",
                    "level",
                    "document_count",
                    "ready_document_count",
                    "ready_document_percent",
                    "needs_review_document_count",
                    "failed_document_count",
                    "table_style_issue_count",
                    "automatic_tblLook_fix_count",
                    "manual_table_style_fix_count",
                    "table_position_automatic_count",
                    "table_position_review_count",
                    "command_failure_count",
                    "unresolved_item_count"
                ) `
                -Violations $Violations
        }
    }
}

function Add-ProjectTemplateDeliveryReadinessContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "project template delivery readiness contract"
    $contract = Get-JsonPropertyValue -Object $Json -Name "project_template_delivery_readiness_contract"
    if ($null -eq $contract) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing project_template_delivery_readiness_contract."
        return
    }

    $schema = Get-JsonPropertyValue -Object $contract -Name "schema"
    if ([string]$schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.schema is invalid."
    }

    $sourceSchema = Get-JsonPropertyValue -Object $contract -Name "source_schema"
    if ([string]$sourceSchema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.source_schema is invalid."
    }

    $status = Get-JsonPropertyValue -Object $contract -Name "status"
    if ([string]::IsNullOrWhiteSpace([string]$status)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.status is missing."
    }
    if (-not (Test-StringValueInSet -Value $status -AllowedValues $ProjectTemplateReadinessStatusValues)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.status must be a recognized readiness state."
    }

    $latestGateStatus = Get-JsonPropertyValue -Object $contract -Name "latest_schema_approval_gate_status"
    if ([string]::IsNullOrWhiteSpace([string]$latestGateStatus)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.latest_schema_approval_gate_status is missing."
    }

    $statusSummary = Get-JsonPropertyValue -Object $contract -Name "schema_approval_status_summary"
    if ($null -eq $statusSummary -or
        ($statusSummary -is [string] -and [string]::IsNullOrWhiteSpace($statusSummary)) -or
        @($statusSummary).Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.schema_approval_status_summary is missing."
    }

    $sourceJsonDisplay = Get-JsonPropertyValue -Object $contract -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceJsonDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.source_json_display is missing."
    }

    $sourceReportDisplay = Get-JsonPropertyValue -Object $contract -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceReportDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.source_report_display is missing."
    }

    $deliveryReadinessSourceNeedles = @("project_template_delivery_readiness", "project-template-delivery-readiness")
    Add-SourceDisplayIdentityViolations `
        -File $File `
        -Violations $Violations `
        -Label $label `
        -FieldName "project_template_delivery_readiness_contract.source_json_display" `
        -Value $sourceJsonDisplay `
        -Needles $deliveryReadinessSourceNeedles `
        -EvidenceDescription "project template delivery readiness"
    Add-SourceDisplayIdentityViolations `
        -File $File `
        -Violations $Violations `
        -Label $label `
        -FieldName "project_template_delivery_readiness_contract.source_report_display" `
        -Value $sourceReportDisplay `
        -Needles $deliveryReadinessSourceNeedles `
        -EvidenceDescription "project template delivery readiness"

    $releaseReady = Get-JsonPropertyValue -Object $contract -Name "release_ready"
    if ($null -eq $releaseReady) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.release_ready is missing."
    }
    if (-not (Test-StringValueInSet -Value $releaseReady -AllowedValues $ProjectTemplateBooleanValues)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.release_ready must be true or false."
    }

    $integerValues = @{}
    foreach ($fieldName in @(
        "schema_history_blocked_run_count",
        "schema_history_pending_run_count",
        "schema_history_passed_run_count",
        "template_count",
        "ready_template_count",
        "blocked_template_count",
        "release_blocker_count",
        "action_item_count",
        "warning_count"
    )) {
        $fieldValue = Get-JsonPropertyValue -Object $contract -Name $fieldName
        if ($null -eq $fieldValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.$fieldName is missing."
            continue
        }

        try {
            $integerValues[$fieldName] = [int]$fieldValue
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.$fieldName must be an integer."
        }
    }

    $releaseReadyIsTrue = $false
    $releaseReadyIsFalse = $false
    if ($releaseReady -is [bool]) {
        $releaseReadyIsTrue = $releaseReady
        $releaseReadyIsFalse = -not $releaseReady
    } elseif ($null -ne $releaseReady) {
        $normalizedReleaseReady = ([string]$releaseReady).Trim().ToLowerInvariant()
        $releaseReadyIsTrue = ($normalizedReleaseReady -eq "true")
        $releaseReadyIsFalse = ($normalizedReleaseReady -eq "false")
    }

    if ($releaseReadyIsTrue -and [string]$status -ne "ready") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.status must be ready when release_ready is true."
    }

    if ($releaseReadyIsFalse -and [string]$status -eq "ready") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.status must not be ready when release_ready is false."
    }

    if ($releaseReadyIsFalse -and $integerValues.ContainsKey("release_blocker_count") -and $integerValues["release_blocker_count"] -le 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_delivery_readiness_contract.release_blocker_count must be greater than 0 when release_ready is false."
    }
}

function Add-ProjectTemplateOnboardingGovernanceContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "project template onboarding governance contract"
    $contract = Get-JsonPropertyValue -Object $Json -Name "project_template_onboarding_governance_contract"
    if ($null -eq $contract) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing project_template_onboarding_governance_contract."
        return
    }

    $schema = Get-JsonPropertyValue -Object $contract -Name "schema"
    if ([string]$schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.schema is invalid."
    }

    $sourceSchema = Get-JsonPropertyValue -Object $contract -Name "source_schema"
    if ([string]$sourceSchema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_schema is invalid."
    }

    $status = Get-JsonPropertyValue -Object $contract -Name "status"
    if ([string]::IsNullOrWhiteSpace([string]$status)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status is missing."
    }
    if (-not (Test-StringValueInSet -Value $status -AllowedValues $ProjectTemplateReadinessStatusValues)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status must be a recognized readiness state."
    }

    $sourceJsonDisplay = Get-JsonPropertyValue -Object $contract -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceJsonDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_json_display is missing."
    }

    $sourceReportDisplay = Get-JsonPropertyValue -Object $contract -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace([string]$sourceReportDisplay)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.source_report_display is missing."
    }

    $onboardingGovernanceSourceNeedles = @("project_template_onboarding_governance", "project-template-onboarding-governance")
    Add-SourceDisplayIdentityViolations `
        -File $File `
        -Violations $Violations `
        -Label $label `
        -FieldName "project_template_onboarding_governance_contract.source_json_display" `
        -Value $sourceJsonDisplay `
        -Needles $onboardingGovernanceSourceNeedles `
        -EvidenceDescription "project template onboarding governance"
    Add-SourceDisplayIdentityViolations `
        -File $File `
        -Violations $Violations `
        -Label $label `
        -FieldName "project_template_onboarding_governance_contract.source_report_display" `
        -Value $sourceReportDisplay `
        -Needles $onboardingGovernanceSourceNeedles `
        -EvidenceDescription "project template onboarding governance"

    $releaseReady = Get-JsonPropertyValue -Object $contract -Name "release_ready"
    if ($null -eq $releaseReady) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.release_ready is missing."
    }
    if (-not (Test-StringValueInSet -Value $releaseReady -AllowedValues $ProjectTemplateBooleanValues)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.release_ready must be true or false."
    }

    $statusSummary = Get-JsonPropertyValue -Object $contract -Name "schema_approval_status_summary"
    if ($null -eq $statusSummary -or
        ($statusSummary -is [string] -and [string]::IsNullOrWhiteSpace($statusSummary)) -or
        @($statusSummary).Count -eq 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.schema_approval_status_summary is missing."
    }

    $integerValues = @{}
    foreach ($fieldName in @(
        "source_file_count",
        "source_failure_count",
        "entry_count",
        "blocked_entry_count",
        "pending_review_entry_count",
        "not_evaluated_entry_count",
        "approved_entry_count",
        "not_required_entry_count",
        "release_blocker_count",
        "action_item_count",
        "manual_review_recommendation_count"
    )) {
        $fieldValue = Get-JsonPropertyValue -Object $contract -Name $fieldName
        if ($null -eq $fieldValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.$fieldName is missing."
            continue
        }

        try {
            $integerValues[$fieldName] = [int]$fieldValue
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.$fieldName must be an integer."
        }
    }

    $releaseReadyIsTrue = $false
    $releaseReadyIsFalse = $false
    if ($releaseReady -is [bool]) {
        $releaseReadyIsTrue = $releaseReady
        $releaseReadyIsFalse = -not $releaseReady
    } elseif ($null -ne $releaseReady) {
        $normalizedReleaseReady = ([string]$releaseReady).Trim().ToLowerInvariant()
        $releaseReadyIsTrue = ($normalizedReleaseReady -eq "true")
        $releaseReadyIsFalse = ($normalizedReleaseReady -eq "false")
    }

    if ($releaseReadyIsTrue -and [string]$status -ne "ready") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status must be ready when release_ready is true."
    }

    if ($releaseReadyIsFalse -and [string]$status -eq "ready") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract.status must not be ready when release_ready is false."
    }

    if ($integerValues.ContainsKey("entry_count")) {
        $entryStatusCountFields = @(
            "blocked_entry_count",
            "pending_review_entry_count",
            "not_evaluated_entry_count",
            "approved_entry_count",
            "not_required_entry_count"
        )
        $hasAllEntryStatusCounts = $true
        $entryStatusCountTotal = 0
        foreach ($fieldName in $entryStatusCountFields) {
            if (-not $integerValues.ContainsKey($fieldName)) {
                $hasAllEntryStatusCounts = $false
                break
            }
            $entryStatusCountTotal += $integerValues[$fieldName]
        }

        if ($hasAllEntryStatusCounts -and $entryStatusCountTotal -ne $integerValues["entry_count"]) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract entry status counts do not match entry_count."
        }
    }

    if ($releaseReadyIsFalse -and
        $integerValues.ContainsKey("release_blocker_count") -and
        $integerValues.ContainsKey("source_failure_count") -and
        (($integerValues["release_blocker_count"] + $integerValues["source_failure_count"]) -le 0)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_onboarding_governance_contract must have release blockers or source failures when release_ready is false."
    }
}

function Add-ManifestSignoffEntrypointsContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "manifest signoff entrypoints contract"
    $signoff = Get-JsonPropertyValue -Object $Json -Name "manifest_signoff_entrypoints"
    if ($null -eq $signoff) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing manifest_signoff_entrypoints."
        return
    }

    $status = Get-JsonPropertyValue -Object $signoff -Name "status"
    if ([string]$status -ne "declared") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.status must be declared."
    }

    $manifestPath = Get-JsonPropertyValue -Object $signoff -Name "release_assets_manifest"
    if ([string]::IsNullOrWhiteSpace([string]$manifestPath) -or
        -not ([string]$manifestPath).Contains("release_assets_manifest.json")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.release_assets_manifest must identify release_assets_manifest.json."
    }

    $requiredEntrypointCount = Get-JsonPropertyValue -Object $signoff -Name "required_entrypoint_count"
    try {
        if ([int]$requiredEntrypointCount -ne 3) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.required_entrypoint_count must be 3."
        }
    } catch {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.required_entrypoint_count must be an integer."
    }

    $entrypoints = @(Get-JsonArray -Object $signoff -Name "entrypoints")
    $entrypointsById = @{}
    foreach ($entrypoint in $entrypoints) {
        $entrypointId = [string](Get-JsonPropertyValue -Object $entrypoint -Name "id")
        if (-not [string]::IsNullOrWhiteSpace($entrypointId)) {
            $entrypointsById[$entrypointId] = $entrypoint
        }
    }

    foreach ($requiredEntrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
        if (-not $entrypointsById.ContainsKey($requiredEntrypointId)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.entrypoints is missing $requiredEntrypointId."
            continue
        }

        $entrypoint = $entrypointsById[$requiredEntrypointId]
        $entrypointRequired = Get-JsonPropertyValue -Object $entrypoint -Name "required"
        if (-not (Test-StringValueInSet -Value $entrypointRequired -AllowedValues @("true"))) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.entrypoints.$requiredEntrypointId.required must be true."
        }

        $entrypointPathDisplay = Get-JsonPropertyValue -Object $entrypoint -Name "path_display"
        if ([string]::IsNullOrWhiteSpace([string]$entrypointPathDisplay)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.entrypoints.$requiredEntrypointId.path_display is missing."
        }
    }

    $requiredContracts = @(Get-JsonArray -Object $signoff -Name "required_contracts" | ForEach-Object { [string]$_ })
    foreach ($requiredContract in @(
        "project_template_delivery_readiness_contract",
        "project_template_onboarding_governance_contract"
    )) {
        if (-not ($requiredContracts -contains $requiredContract)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.required_contracts is missing $requiredContract."
        }
    }

    $requiredFields = @(Get-JsonArray -Object $signoff -Name "required_fields" | ForEach-Object { [string]$_ })
    foreach ($requiredField in @(
        "status",
        "release_ready",
        "schema_approval_status_summary",
        "source_report_display",
        "source_json_display"
    )) {
        if (-not ($requiredFields -contains $requiredField)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.required_fields is missing $requiredField."
        }
    }

    $checklistMarker = Get-JsonPropertyValue -Object $signoff -Name "checklist_marker"
    if ([string]$checklistMarker -ne "reviewer_manifest_scoped_project_template_trace") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "manifest_signoff_entrypoints.checklist_marker is invalid."
    }
}

function Add-ProjectTemplateReadinessChecklistEntrypointsContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "project-template readiness checklist entrypoints contract"
    $entrypointsContract = Get-JsonPropertyValue -Object $Json -Name "project_template_readiness_checklist_entrypoints"
    if ($null -eq $entrypointsContract) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing project_template_readiness_checklist_entrypoints."
        return
    }

    $status = Get-JsonPropertyValue -Object $entrypointsContract -Name "status"
    if ([string]$status -ne "declared") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.status must be declared."
    }

    $checklistLabel = Get-JsonPropertyValue -Object $entrypointsContract -Name "checklist_label"
    if ([string]$checklistLabel -ne "Project template release readiness checklist") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.checklist_label is invalid."
    }

    $checklistPath = Get-JsonPropertyValue -Object $entrypointsContract -Name "checklist_path"
    if ([string]::IsNullOrWhiteSpace([string]$checklistPath) -or
        -not ([string]$checklistPath).Contains("docs/project_template_release_readiness_checklist_zh.rst")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.checklist_path must identify docs/project_template_release_readiness_checklist_zh.rst."
    }

    $requiredEntrypointCount = Get-JsonPropertyValue -Object $entrypointsContract -Name "required_entrypoint_count"
    try {
        if ([int]$requiredEntrypointCount -ne 3) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.required_entrypoint_count must be 3."
        }
    } catch {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.required_entrypoint_count must be an integer."
    }

    $entrypoints = @(Get-JsonArray -Object $entrypointsContract -Name "entrypoints")
    $entrypointsById = @{}
    foreach ($entrypoint in $entrypoints) {
        $entrypointId = [string](Get-JsonPropertyValue -Object $entrypoint -Name "id")
        if (-not [string]::IsNullOrWhiteSpace($entrypointId)) {
            $entrypointsById[$entrypointId] = $entrypoint
        }
    }

    foreach ($requiredEntrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
        if (-not $entrypointsById.ContainsKey($requiredEntrypointId)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.entrypoints is missing $requiredEntrypointId."
            continue
        }

        $entrypoint = $entrypointsById[$requiredEntrypointId]
        $entrypointRequired = Get-JsonPropertyValue -Object $entrypoint -Name "required"
        if (-not (Test-StringValueInSet -Value $entrypointRequired -AllowedValues @("true"))) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.entrypoints.$requiredEntrypointId.required must be true."
        }

        $entrypointPathDisplay = Get-JsonPropertyValue -Object $entrypoint -Name "path_display"
        if ([string]::IsNullOrWhiteSpace([string]$entrypointPathDisplay)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.entrypoints.$requiredEntrypointId.path_display is missing."
        }
    }

    $checklistMarker = Get-JsonPropertyValue -Object $entrypointsContract -Name "checklist_marker"
    if ([string]$checklistMarker -ne "release_entry_project_template_readiness_checklist_trace") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "project_template_readiness_checklist_entrypoints.checklist_marker is invalid."
    }
}

function Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "release entry project-template readiness checklist material-safety audit contract"
    $audit = Get-JsonPropertyValue -Object $Json -Name "release_entry_project_template_readiness_checklist_material_safety_audit"
    if ($null -eq $audit) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "Missing release_entry_project_template_readiness_checklist_material_safety_audit."
        return
    }

    if ([string](Get-JsonPropertyValue -Object $audit -Name "status") -ne "passed") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.status must be passed."
    }

    $auditScript = [string](Get-JsonPropertyValue -Object $audit -Name "audit_script")
    if ([string]::IsNullOrWhiteSpace($auditScript) -or
        -not $auditScript.Contains("scripts\assert_release_material_safety.ps1")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.audit_script must identify scripts/assert_release_material_safety.ps1."
    }

    $auditedEntrypointCount = Get-JsonPropertyValue -Object $audit -Name "audited_entrypoint_count"
    try {
        if ([int]$auditedEntrypointCount -ne 3) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.audited_entrypoint_count must be 3."
        }
    } catch {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.audited_entrypoint_count must be an integer."
    }

    $auditedEntrypoints = @(Get-JsonArray -Object $audit -Name "audited_entrypoints" | ForEach-Object { [string]$_ })
    foreach ($requiredEntrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
        if (-not ($auditedEntrypoints -contains $requiredEntrypointId)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.audited_entrypoints is missing $requiredEntrypointId."
        }
    }

    $expectedFields = [ordered]@{
        compact_evidence_label = "Project-template readiness checklist handoff evidence"
        compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
        checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
        checklist_marker = "release_entry_project_template_readiness_checklist_trace"
        material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    }
    foreach ($entry in $expectedFields.GetEnumerator()) {
        if ([string](Get-JsonPropertyValue -Object $audit -Name $entry.Key) -ne [string]$entry.Value) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "release_entry_project_template_readiness_checklist_material_safety_audit.$($entry.Key) is invalid."
        }
    }
}

function Add-PdfVisualGateManifestContractViolations {
    param(
        [string]$File,
        $Json,
        $Violations
    )

    $leafName = (Split-Path -Leaf $File).ToLowerInvariant()
    if ($leafName -ne "release_assets_manifest.json") {
        return
    }

    $label = "PDF visual gate manifest contract"
    $includedValue = Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_evidence_included"
    $status = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_status")
    $included = $false
    if ($includedValue -is [bool]) {
        $included = $includedValue
    } elseif ($null -ne $includedValue) {
        $included = ([string]$includedValue).Trim().ToLowerInvariant() -eq "true"
    }

    if (-not $included) {
        if ($status -eq "loaded") {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_status=loaded requires pdf_visual_gate_evidence_included=true."
        }
        return
    }

    if ($status -ne "loaded") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence_included=true requires pdf_visual_gate_status=loaded."
    }

    $topLevelSummary = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_summary_json")
    if ([string]::IsNullOrWhiteSpace($topLevelSummary)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_summary_json is missing."
    }

    $topLevelOutputDir = [string](Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_output_dir")
    if ([string]::IsNullOrWhiteSpace($topLevelOutputDir)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_output_dir is missing."
    }

    $evidence = Get-JsonPropertyValue -Object $Json -Name "pdf_visual_gate_evidence"
    if ($null -eq $evidence) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence is missing."
        return
    }

    $evidenceStatus = [string](Get-JsonPropertyValue -Object $evidence -Name "status")
    if ($evidenceStatus -ne "loaded") {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.status must be loaded."
    }

    $evidenceVerdict = ([string](Get-JsonPropertyValue -Object $evidence -Name "verdict")).Trim().ToLowerInvariant()
    if ([string]::IsNullOrWhiteSpace($evidenceVerdict)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.verdict is missing."
    } elseif ($evidenceVerdict -notin @("pass", "fail")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.verdict must be pass or fail."
    }

    $fullVisualGateStatus = ([string](Get-JsonPropertyValue -Object $evidence -Name "full_visual_gate_status")).Trim().ToLowerInvariant()
    if ([string]::IsNullOrWhiteSpace($fullVisualGateStatus)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.full_visual_gate_status is missing."
    } elseif ($fullVisualGateStatus -notin @("pass", "fail")) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.full_visual_gate_status must be pass or fail."
    } elseif ($evidenceVerdict -in @("pass", "fail") -and $fullVisualGateStatus -ne $evidenceVerdict) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.full_visual_gate_status must match verdict."
    }

    $evidenceSummary = [string](Get-JsonPropertyValue -Object $evidence -Name "summary_json")
    if ([string]::IsNullOrWhiteSpace($evidenceSummary)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.summary_json is missing."
    } elseif (-not [string]::IsNullOrWhiteSpace($topLevelSummary) -and $evidenceSummary -ne $topLevelSummary) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.summary_json must match pdf_visual_gate_summary_json."
    }

    foreach ($fieldName in @(
            "aggregate_contact_sheet",
            "pdf_cli_export_log",
            "pdf_regression_log",
            "cjk_copy_search_log_dir",
            "unicode_font_log"
        )) {
        $fieldValue = [string](Get-JsonPropertyValue -Object $evidence -Name $fieldName)
        if ([string]::IsNullOrWhiteSpace($fieldValue)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName is missing."
        }
    }

    $manifestCountValues = @{}
    foreach ($countContract in @(
            @{ Name = "cjk_manifest_count"; Minimum = 43 },
            @{ Name = "cjk_copy_search_count"; Minimum = 1 },
            @{ Name = "cjk_missing_text_count"; Minimum = 0 },
            @{ Name = "visual_baseline_manifest_count"; Minimum = 42 },
            @{ Name = "visual_baseline_count"; Minimum = 1 }
        )) {
        $fieldName = $countContract.Name
        $fieldValue = Get-JsonPropertyValue -Object $evidence -Name $fieldName
        if ($null -eq $fieldValue) {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName is missing."
            continue
        }

        try {
            $integerValue = [int]$fieldValue
            $manifestCountValues[$fieldName] = $integerValue
            if ($integerValue -lt [int]$countContract.Minimum) {
                Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName must be at least $($countContract.Minimum)."
            }
        } catch {
            Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.$fieldName must be an integer."
        }
    }

    if ($evidenceVerdict -eq "pass" -and
        $manifestCountValues.ContainsKey("cjk_missing_text_count") -and
        $manifestCountValues["cjk_missing_text_count"] -ne 0) {
        Add-AuditViolation -Violations $Violations -File $File -Label $label -Text "pdf_visual_gate_evidence.cjk_missing_text_count must be 0 when verdict is pass."
    }
}

$repoRoot = Resolve-RepoRoot
$scanFiles = @(Get-ScanFiles -RepoRoot $repoRoot -InputPaths $Path)
if ($scanFiles.Count -eq 0) {
    Write-Step "No text-like files matched the requested paths."
    exit 0
}

$rules = @(
    (New-Rule `
        -Label "draft wording" `
        -Pattern '(?i)发布说明草稿|请在发布前补齐|草稿|release body draft|release-note drafts|release drafts|public release drafts|draft release notes|still drafting')
    (New-Rule `
        -Label "local absolute path" `
        -Pattern '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+|(?<!\w)/(?:Users|home)/[^\s"''`<>|]+')
)

foreach ($extraPattern in $AdditionalForbiddenPattern) {
    if (-not [string]::IsNullOrWhiteSpace($extraPattern)) {
        $rules += New-Rule -Label "custom forbidden pattern" -Pattern $extraPattern
    }
}

$violations = [System.Collections.Generic.List[object]]::new()
foreach ($file in $scanFiles) {
    foreach ($rule in $rules) {
        $matches = Select-String -LiteralPath $file -Pattern $rule.pattern -AllMatches
        foreach ($match in $matches) {
            [void]$violations.Add([ordered]@{
                file = $file
                line = $match.LineNumber
                label = $rule.label
                text = $match.Line.Trim()
            })
        }
    }

    if ([System.IO.Path]::GetExtension($file).ToLowerInvariant() -eq ".md") {
        $content = Get-Content -Raw -LiteralPath $file
        Add-ReleaseEntryDocumentGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseEntryProjectTemplateReadinessChecklistEntrypointsEvidenceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseEntryPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseSummaryProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseSummaryPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBodyProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBodyPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseHandoffProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseHandoffPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffPdfVisualGateAttemptTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistEntrypointsTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewProjectTemplateReadinessChecklistEntrypointsTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
    }

    if ([System.IO.Path]::GetExtension($file).ToLowerInvariant() -eq ".json") {
        $leafName = (Split-Path -Leaf $file).ToLowerInvariant()
        try {
            $json = Get-Content -Raw -LiteralPath $file | ConvertFrom-Json
            Add-GovernanceMetricContractViolations -File $file -Json $json -Violations $violations
            Add-ContentControlRepairContractViolations -File $file -Json $json -Violations $violations
            Add-ProjectTemplateDeliveryReadinessContractViolations -File $file -Json $json -Violations $violations
            Add-ProjectTemplateOnboardingGovernanceContractViolations -File $file -Json $json -Violations $violations
            Add-ManifestSignoffEntrypointsContractViolations -File $file -Json $json -Violations $violations
            Add-ProjectTemplateReadinessChecklistEntrypointsContractViolations -File $file -Json $json -Violations $violations
            Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditContractViolations -File $file -Json $json -Violations $violations
            Add-PdfVisualGateManifestContractViolations -File $file -Json $json -Violations $violations
        } catch {
            if ($leafName -eq "summary.json" -or $leafName -eq "release_assets_manifest.json") {
                Add-AuditViolation `
                    -Violations $violations `
                    -File $file `
                    -Label "governance metrics contract" `
                    -Text "Release governance JSON could not be parsed."
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Step "Detected forbidden release material content:"
    foreach ($violation in $violations) {
        Write-Host ("- [{0}] {1}:{2}" -f $violation.label, $violation.file, $violation.line)
        Write-Host ("  {0}" -f $violation.text)
    }

    throw "Release material safety audit failed with $($violations.Count) violation(s)."
}

Write-Step ("Audit passed for {0} file(s)." -f $scanFiles.Count)
