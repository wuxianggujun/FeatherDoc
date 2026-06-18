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

function Add-ProjectTemplateReviewerActionContractViolations {
    param(
        [string]$File,
        $Contract,
        $Violations,
        [string]$Label,
        [string]$ContractName
    )

    $requiresReviewerAction = Get-JsonPropertyValue -Object $Contract -Name "requires_reviewer_action"
    if ($null -eq $requiresReviewerAction) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$ContractName.requires_reviewer_action is missing."
    } elseif (-not (Test-StringValueInSet -Value $requiresReviewerAction -AllowedValues @("true", "false"))) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$ContractName.requires_reviewer_action must be true or false."
    }

    $reviewerActionSummary = Get-JsonPropertyValue -Object $Contract -Name "reviewer_action_summary"
    if ([string]::IsNullOrWhiteSpace([string]$reviewerActionSummary)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$ContractName.reviewer_action_summary is missing."
    }

    $reviewerActionReason = Get-JsonPropertyValue -Object $Contract -Name "reviewer_action_reason"
    if ([string]::IsNullOrWhiteSpace([string]$reviewerActionReason)) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$ContractName.reviewer_action_reason is missing."
    }

    $reviewerActionsProperty = $null
    if ($null -ne $Contract) {
        $reviewerActionsProperty = $Contract.PSObject.Properties["reviewer_actions"]
    }
    if ($null -eq $reviewerActionsProperty) {
        Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$ContractName.reviewer_actions is missing."
    }
    $reviewerActions = @(Get-JsonArray -Object $Contract -Name "reviewer_actions" |
        ForEach-Object { [string]$_ } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })

    $requiresReviewerActionIsTrue = $false
    if ($requiresReviewerAction -is [bool]) {
        $requiresReviewerActionIsTrue = $requiresReviewerAction
    } elseif ($null -ne $requiresReviewerAction) {
        $requiresReviewerActionIsTrue = ([string]$requiresReviewerAction).Trim().ToLowerInvariant() -eq "true"
    }

    if ($requiresReviewerActionIsTrue) {
        if ([string]::Equals([string]$reviewerActionSummary, "none", [System.StringComparison]::OrdinalIgnoreCase)) {
            Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$ContractName.reviewer_action_summary must name an action when requires_reviewer_action is true."
        }
        if ($reviewerActions.Count -eq 0) {
            Add-AuditViolation -Violations $Violations -File $File -Label $Label -Text "$ContractName.reviewer_actions must list at least one action when requires_reviewer_action is true."
        }
    }
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
        return ([string]$Matches["value"]).Trim().Trim('`').TrimEnd(",", ";")
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

function Get-MarkdownListBlockFieldValuesFromText {
    param(
        [string]$Block,
        [string]$FieldName
    )

    if ([string]::IsNullOrEmpty($Block)) {
        return @()
    }

    $values = [System.Collections.Generic.List[string]]::new()
    $escapedFieldName = [regex]::Escape($FieldName)
    $fieldPattern = '(?<![A-Za-z0-9_])`*' + $escapedFieldName + '`*\s*[:=]\s*(?<value>.+?)(?=\s+[A-Za-z_][A-Za-z0-9_]*\s*[:=]|$)'
    foreach ($line in ($Block -split "\r?\n")) {
        if ($line -match $fieldPattern) {
            $value = $Matches["value"].Trim()
            $value = $value -replace '^[`\s]+|[`\s,;]+$', ''
            [void]$values.Add($value)
        }
    }

    return @($values)
}

function Test-MarkdownAnyListBlockContainsAllAndFieldValuesInSet {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$Needles,
        [string]$FieldName,
        [string[]]$AllowedValues
    )

    foreach ($block in @(Get-MarkdownListBlockTexts -Text $Text -Anchor $Anchor)) {
        $containsAllNeedles = $true
        foreach ($needle in $Needles) {
            if ([string]::IsNullOrWhiteSpace($needle) -or -not $block.Contains($needle)) {
                $containsAllNeedles = $false
                break
            }
        }

        if (-not $containsAllNeedles) {
            continue
        }

        $values = @(Get-MarkdownListBlockFieldValuesFromText -Block $block -FieldName $FieldName)
        if ($values.Count -eq 0) {
            continue
        }

        $valuesAreAllowed = $true
        foreach ($value in $values) {
            if (-not (Test-StringValueInSet -Value $value -AllowedValues $AllowedValues)) {
                $valuesAreAllowed = $false
                break
            }
        }

        if ($valuesAreAllowed) {
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

    return @(Get-MarkdownListBlockFieldValuesFromText -Block $block -FieldName $FieldName)
}

function Test-MarkdownListBlockFieldValuesIdentify {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$FieldName,
        [string[]]$Needles,
        [string[]]$ForbiddenNeedles = @()
    )

    foreach ($block in @(Get-MarkdownListBlockTexts -Text $Text -Anchor $Anchor)) {
        $values = @(Get-MarkdownListBlockFieldValuesFromText -Block $block -FieldName $FieldName)
        if ($values.Count -eq 0) {
            continue
        }

        $valuesIdentifyNeedles = $true
        foreach ($value in $values) {
            if (-not (Test-TextContainsAny -Text ([string]$value) -Needles $Needles)) {
                $valuesIdentifyNeedles = $false
                break
            }

            if ($ForbiddenNeedles.Count -gt 0 -and (Test-TextContainsAny -Text ([string]$value) -Needles $ForbiddenNeedles)) {
                $valuesIdentifyNeedles = $false
                break
            }
        }

        if ($valuesIdentifyNeedles) {
            return $true
        }
    }

    return $false
}

function Test-MarkdownListBlockFieldValuesInSet {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$FieldName,
        [string[]]$AllowedValues
    )

    $allowed = @{}
    foreach ($allowedValue in @($AllowedValues)) {
        if (-not [string]::IsNullOrWhiteSpace($allowedValue)) {
            $allowed[[string]$allowedValue.ToLowerInvariant()] = $true
        }
    }

    foreach ($block in @(Get-MarkdownListBlockTexts -Text $Text -Anchor $Anchor)) {
        $values = @(Get-MarkdownListBlockFieldValuesFromText -Block $block -FieldName $FieldName)
        if ($values.Count -eq 0) {
            continue
        }

        $valuesAreAllowed = $true
        foreach ($value in $values) {
            $normalized = ([string]$value).Trim().ToLowerInvariant()
            if (-not $allowed.ContainsKey($normalized)) {
                $valuesAreAllowed = $false
                break
            }
        }

        if ($valuesAreAllowed) {
            return $true
        }
    }

    return $false
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

function Test-MarkdownSectionListRunContainsAll {
    param(
        [string]$Text,
        [string]$Heading,
        [string]$Anchor,
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
        if (Test-MarkdownListRunContainsAll -Text $section -Anchor $Anchor -Needles $Needles) {
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
