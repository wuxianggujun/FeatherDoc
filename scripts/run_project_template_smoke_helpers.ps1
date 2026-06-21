function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-smoke] $Message"
}

function Resolve-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Convert-ToSafeFileName {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "entry"
    }

    $sanitized = $Value -replace '[\\/:*?"<>|]', '-' -replace '\s+', '-'
    $sanitized = $sanitized.Trim('-')
    if ([string]::IsNullOrWhiteSpace($sanitized)) {
        return "entry"
    }

    return $sanitized.ToLowerInvariant()
}

function Resolve-OptionalManifestPropertyValue {
    param(
        $Entry,
        [string]$Name
    )

    $property = $Entry.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-ManifestArrayProperty {
    param(
        $Entry,
        [string]$Name
    )

    $property = $Entry.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        Write-Output -NoEnumerate @()
        return
    }

    $values = @($property.Value | Where-Object { $null -ne $_ })
    Write-Output -NoEnumerate $values
}

function Get-OptionalObjectPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name) -and $null -ne $Object[$Name]) {
            return [string]$Object[$Name]
        }

        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
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

function Normalize-ProjectTemplateSmokeVisualVerdict {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "undecided"
    }

    switch ($Value.ToLowerInvariant()) {
        "pass" { return "pass" }
        "fail" { return "fail" }
        "undetermined" { return "undetermined" }
        "undecided" { return "undecided" }
        "pending_manual_review" { return "pending_manual_review" }
        "skipped" { return "skipped" }
        default { return "undecided" }
    }
}

function Get-ProjectTemplateSmokeVisualReviewState {
    param(
        [string]$ReviewStatus,
        [string]$ReviewVerdict
    )

    $normalizedStatus = if ([string]::IsNullOrWhiteSpace($ReviewStatus)) {
        "missing"
    } else {
        $ReviewStatus
    }
    $normalizedVerdict = Normalize-ProjectTemplateSmokeVisualVerdict -Value $ReviewVerdict

    $failed = ($normalizedStatus -eq "failed" -or $normalizedVerdict -eq "fail")
    $pending = (-not $failed) -and (
        $normalizedStatus -eq "missing" -or
        $normalizedStatus -eq "pending_review" -or
        $normalizedVerdict -eq "undecided" -or
        $normalizedVerdict -eq "pending_manual_review"
    )
    $undetermined = (-not $failed) -and (-not $pending) -and $normalizedVerdict -eq "undetermined"
    $passed = (-not $failed) -and (-not $pending) -and (-not $undetermined) -and $normalizedVerdict -eq "pass"
    $skipped = (-not $failed) -and (-not $pending) -and (-not $undetermined) -and (-not $passed) -and (
        $normalizedStatus -eq "skipped" -or
        $normalizedVerdict -eq "skipped"
    )

    return [pscustomobject]@{
        review_status = $normalizedStatus
        review_verdict = $normalizedVerdict
        failed = $failed
        manual_review_pending = $pending
        review_undetermined = $undetermined
        review_passed = $passed
        skipped = $skipped
    }
}

function Get-ProjectTemplateSmokeEntryStatus {
    param(
        [bool]$EntryPassed,
        [bool]$ManualReviewPending,
        [bool]$VisualReviewUndetermined
    )

    if (-not $EntryPassed) {
        return "failed"
    }
    if ($ManualReviewPending) {
        return "passed_with_pending_visual_review"
    }
    if ($VisualReviewUndetermined) {
        return "passed_with_visual_review_undetermined"
    }

    return "passed"
}

function Get-ProjectTemplateSmokeOverallStatus {
    param(
        [bool]$SummaryPassed,
        [int]$ManualReviewPendingCount,
        [int]$VisualReviewUndeterminedCount
    )

    if (-not $SummaryPassed) {
        return "failed"
    }
    if ($ManualReviewPendingCount -gt 0) {
        return "passed_with_pending_visual_review"
    }
    if ($VisualReviewUndeterminedCount -gt 0) {
        return "passed_with_visual_review_undetermined"
    }

    return "passed"
}

function Get-ProjectTemplateSmokeVisualVerdict {
    param([object[]]$VisualSmokeResults)

    if ($VisualSmokeResults.Count -eq 0) {
        return "not_applicable"
    }

    $states = @(
        foreach ($visualSmokeResult in $VisualSmokeResults) {
            Get-ProjectTemplateSmokeVisualReviewState `
                -ReviewStatus (Get-OptionalObjectPropertyValue -Object $visualSmokeResult -Name "review_status") `
                -ReviewVerdict (Get-OptionalObjectPropertyValue -Object $visualSmokeResult -Name "review_verdict")
        }
    )

    if (@($states | Where-Object { $_.failed }).Count -gt 0) {
        return "fail"
    }
    if (@($states | Where-Object { $_.manual_review_pending }).Count -gt 0) {
        return "pending_manual_review"
    }
    if (@($states | Where-Object { $_.review_undetermined }).Count -gt 0) {
        return "undetermined"
    }
    if (@($states | Where-Object { $_.review_passed }).Count -gt 0) {
        return "pass"
    }
    if (@($states | Where-Object { $_.skipped }).Count -eq $states.Count) {
        return "skipped"
    }

    return "pending_manual_review"
}

function Resolve-ManifestPathValue {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    return Resolve-TemplateSchemaPath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing:$AllowMissing
}

function Resolve-ManifestInputDocxPath {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        $Entry
    )

    $repoRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx"
    $buildRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx_build_relative"

    if (-not [string]::IsNullOrWhiteSpace($repoRelativePath) -and
        -not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        throw "Manifest entries must not set both 'input_docx' and 'input_docx_build_relative'."
    }

    if (-not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
            throw "Entry '$([string]$Entry.name)' uses 'input_docx_build_relative' but no -BuildDir was provided."
        }

        return Resolve-TemplateSchemaPath -RepoRoot $ResolvedBuildDir -InputPath $buildRelativePath -AllowMissing
    }

    if ([string]::IsNullOrWhiteSpace($repoRelativePath)) {
        throw "Entry '$([string]$Entry.name)' must set either 'input_docx' or 'input_docx_build_relative'."
    }

    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $repoRelativePath -AllowMissing
}

function Resolve-PrepareArgumentPath {
    param(
        [string]$RepoRoot,
        [string]$ResolvedInputDocx,
        $Entry
    )

    $prepareArgument = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "prepare_argument"
    if ([string]::IsNullOrWhiteSpace($prepareArgument)) {
        return $ResolvedInputDocx
    }

    return Resolve-ManifestPathValue -RepoRoot $RepoRoot -InputPath $prepareArgument -AllowMissing
}

function Ensure-PathParent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $extension = [System.IO.Path]::GetExtension($Path)
    if ([string]::IsNullOrWhiteSpace($extension)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
}

function Convert-SlotSpecToCliString {
    param($SlotSpec)

    if ($SlotSpec -is [string]) {
        return $SlotSpec
    }

    $name = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "bookmark"
    if ([string]::IsNullOrWhiteSpace($name)) {
        $name = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "name"
    }
    $kind = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "kind"

    if ([string]::IsNullOrWhiteSpace($name) -or [string]::IsNullOrWhiteSpace($kind)) {
        throw "Structured slot objects must provide 'bookmark'/'name' and 'kind'."
    }

    $segments = New-Object 'System.Collections.Generic.List[string]'
    $segments.Add($name)
    $segments.Add($kind)

    $requiredProperty = $SlotSpec.PSObject.Properties["required"]
    if ($null -ne $requiredProperty -and $null -ne $requiredProperty.Value) {
        $segments.Add((if ([bool]$requiredProperty.Value) { "required" } else { "optional" }))
    } else {
        $optionalProperty = $SlotSpec.PSObject.Properties["optional"]
        if ($null -ne $optionalProperty -and $null -ne $optionalProperty.Value) {
            $segments.Add((if ([bool]$optionalProperty.Value) { "optional" } else { "required" }))
        }
    }

    foreach ($occurrenceName in @("count", "min", "max")) {
        $property = $SlotSpec.PSObject.Properties[$occurrenceName]
        if ($null -ne $property -and $null -ne $property.Value -and
            -not [string]::IsNullOrWhiteSpace([string]$property.Value)) {
            $segments.Add("$occurrenceName=$([string]$property.Value)")
        }
    }

    return ($segments -join ":")
}

function Add-PartSelectionArguments {
    param(
        [string[]]$Arguments,
        [string]$PartSwitch,
        $Selection
    )

    $part = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "part"
    if ([string]::IsNullOrWhiteSpace($part)) {
        throw "Every selection block must provide a non-empty 'part'."
    }

    $result = @($Arguments + @($PartSwitch, $part))

    $index = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "index"
    if (-not [string]::IsNullOrWhiteSpace($index)) {
        $result += @("--index", $index)
    }

    $section = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "section"
    if (-not [string]::IsNullOrWhiteSpace($section)) {
        $result += @("--section", $section)
    }

    $kind = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "kind"
    if (-not [string]::IsNullOrWhiteSpace($kind)) {
        $result += @("--kind", $kind)
    }

    return $result
}

function Write-CommandOutput {
    param(
        [string]$OutputPath,
        [string[]]$Lines
    )

    if ([string]::IsNullOrWhiteSpace($OutputPath)) {
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($OutputPath)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $Lines | Set-Content -LiteralPath $OutputPath -Encoding UTF8
}

function Invoke-CliJsonCommand {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureLabel
    )

    $commandResult = Invoke-TemplateSchemaCli -ExecutablePath $ExecutablePath -Arguments $Arguments
    Write-CommandOutput -OutputPath $OutputPath -Lines $commandResult.Output

    if ($commandResult.ExitCode -ne 0) {
        $lines = if ($commandResult.Output.Count -gt 0) {
            ($commandResult.Output -join [System.Environment]::NewLine)
        } else {
            "(no output)"
        }
        throw "$FailureLabel failed with exit code $($commandResult.ExitCode). Output:`n$lines"
    }

    $jsonText = $commandResult.Text.Trim()
    if ([string]::IsNullOrWhiteSpace($jsonText)) {
        throw "$FailureLabel did not emit JSON output."
    }

    try {
        $parsed = $jsonText | ConvertFrom-Json
    } catch {
        throw "$FailureLabel emitted invalid JSON: $jsonText"
    }

    return [pscustomobject]@{
        ExitCode = $commandResult.ExitCode
        Text = $jsonText
        Output = $commandResult.Output
        Json = $parsed
    }
}

function Get-OptionalTemplateSchemaCommandJsonObject {
    param(
        [string[]]$Lines,
        [string]$Command
    )

    $pattern = '^\{"command":"' + [regex]::Escape($Command) + '",'
    $jsonLine = $Lines |
        Where-Object { $_ -match $pattern } |
        Select-Object -Last 1

    if ([string]::IsNullOrWhiteSpace([string]$jsonLine)) {
        return $null
    }

    return $jsonLine | ConvertFrom-Json
}

function Read-JsonFileIfPresent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not [System.IO.File]::Exists($Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}
