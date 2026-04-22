function Get-ProjectTemplateSmokeOptionalPropertyValue {
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

function Get-ProjectTemplateSmokeOptionalPropertyObject {
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

function Get-ProjectTemplateSmokeArrayProperty {
    param(
        $Object,
        [string]$Name
    )

    $propertyValue = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Object -Name $Name
    if ($null -eq $propertyValue) {
        return @()
    }

    if ($propertyValue -is [System.Collections.IEnumerable] -and $propertyValue -isnot [string]) {
        return @($propertyValue | Where-Object { $null -ne $_ })
    }

    return @($propertyValue)
}

function Resolve-ProjectTemplateSmokePath {
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

function Add-ProjectTemplateSmokeValidationIssue {
    param(
        [System.Collections.Generic.List[object]]$Issues,
        [string]$Path,
        [string]$Message
    )

    $Issues.Add([pscustomobject]@{
        path = $Path
        message = $Message
    }) | Out-Null
}

function Test-ProjectTemplateSmokeBooleanProperty {
    param(
        $Object,
        [string]$Name,
        [string]$Path,
        [System.Collections.Generic.List[object]]$Issues
    )

    $propertyValue = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Object -Name $Name
    if ($null -eq $propertyValue) {
        return
    }

    if ($propertyValue -is [bool]) {
        return
    }

    Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "must be a boolean"
}

function Test-ProjectTemplateSmokeNonNegativeIntegerProperty {
    param(
        $Object,
        [string]$Name,
        [string]$Path,
        [System.Collections.Generic.List[object]]$Issues
    )

    $propertyValue = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $Object -Name $Name
    if ($null -eq $propertyValue) {
        return $null
    }

    $parsedValue = 0
    if (-not [int]::TryParse([string]$propertyValue, [ref]$parsedValue) -or $parsedValue -lt 0) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "must be a non-negative integer"
        return $null
    }

    return $parsedValue
}

function Test-ProjectTemplateSmokeSlotSpec {
    param(
        $SlotSpec,
        [string]$Path,
        [System.Collections.Generic.List[object]]$Issues
    )

    if ($SlotSpec -is [string]) {
        if ([string]::IsNullOrWhiteSpace($SlotSpec) -or $SlotSpec.IndexOf(":") -lt 1) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "string slot specs must look like <bookmark>:<kind>"
        }
        return
    }

    $bookmark = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SlotSpec -Name "bookmark"
    $name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SlotSpec -Name "name"
    $kind = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $SlotSpec -Name "kind"

    if ([string]::IsNullOrWhiteSpace($bookmark) -and [string]::IsNullOrWhiteSpace($name)) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "slot objects must provide 'bookmark' or 'name'"
    }
    if (-not [string]::IsNullOrWhiteSpace($bookmark) -and -not [string]::IsNullOrWhiteSpace($name) -and $bookmark -ne $name) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "'bookmark' and 'name' must match when both are provided"
    }
    if ([string]::IsNullOrWhiteSpace($kind)) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "slot objects must provide a non-empty 'kind'"
    }

    Test-ProjectTemplateSmokeBooleanProperty -Object $SlotSpec -Name "required" -Path "$Path.required" -Issues $Issues
    Test-ProjectTemplateSmokeBooleanProperty -Object $SlotSpec -Name "optional" -Path "$Path.optional" -Issues $Issues

    $countValue = Test-ProjectTemplateSmokeNonNegativeIntegerProperty -Object $SlotSpec -Name "count" -Path "$Path.count" -Issues $Issues
    $minValue = Test-ProjectTemplateSmokeNonNegativeIntegerProperty -Object $SlotSpec -Name "min" -Path "$Path.min" -Issues $Issues
    $maxValue = Test-ProjectTemplateSmokeNonNegativeIntegerProperty -Object $SlotSpec -Name "max" -Path "$Path.max" -Issues $Issues

    if ($null -ne $countValue -and ($null -ne $minValue -or $null -ne $maxValue)) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "'count' conflicts with 'min'/'max'"
    }
    if ($null -ne $minValue -and $null -ne $maxValue -and $minValue -gt $maxValue) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "'min' must be less than or equal to 'max'"
    }
}

function Test-ProjectTemplateSmokeSelection {
    param(
        $Selection,
        [string]$Path,
        [bool]$RequireSlots,
        [System.Collections.Generic.List[object]]$Issues
    )

    $part = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Selection -Name "part"
    $index = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Selection -Name "index"
    $section = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Selection -Name "section"
    $kind = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Selection -Name "kind"

    $allowedParts = @("body", "header", "footer", "section-header", "section-footer")
    if ($part -notin $allowedParts) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.part" -Message "must be one of: $($allowedParts -join ', ')"
    }

    if (-not [string]::IsNullOrWhiteSpace($index)) {
        $indexValue = 0
        if (-not [int]::TryParse($index, [ref]$indexValue) -or $indexValue -lt 0) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.index" -Message "must be a non-negative integer"
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($section)) {
        $sectionValue = 0
        if (-not [int]::TryParse($section, [ref]$sectionValue) -or $sectionValue -lt 0) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.section" -Message "must be a non-negative integer"
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($kind) -and $kind -notin @("default", "first", "even")) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.kind" -Message "must be one of: default, first, even"
    }

    switch ($part) {
        "body" {
            if (-not [string]::IsNullOrWhiteSpace($index)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.index" -Message "is only supported by header/footer selections"
            }
            if (-not [string]::IsNullOrWhiteSpace($section)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.section" -Message "is only supported by section-header/section-footer selections"
            }
            if (-not [string]::IsNullOrWhiteSpace($kind)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.kind" -Message "is only supported by section-header/section-footer selections"
            }
        }
        "header" {
            if (-not [string]::IsNullOrWhiteSpace($section)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.section" -Message "is only supported by section-header/section-footer selections"
            }
            if (-not [string]::IsNullOrWhiteSpace($kind)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.kind" -Message "is only supported by section-header/section-footer selections"
            }
        }
        "footer" {
            if (-not [string]::IsNullOrWhiteSpace($section)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.section" -Message "is only supported by section-header/section-footer selections"
            }
            if (-not [string]::IsNullOrWhiteSpace($kind)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.kind" -Message "is only supported by section-header/section-footer selections"
            }
        }
        "section-header" {
            if ([string]::IsNullOrWhiteSpace($section)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.section" -Message "is required for section-header selections"
            }
            if (-not [string]::IsNullOrWhiteSpace($index)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.index" -Message "is only supported by header/footer selections"
            }
        }
        "section-footer" {
            if ([string]::IsNullOrWhiteSpace($section)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.section" -Message "is required for section-footer selections"
            }
            if (-not [string]::IsNullOrWhiteSpace($index)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.index" -Message "is only supported by header/footer selections"
            }
        }
    }

    if ($RequireSlots) {
        $slots = @(Get-ProjectTemplateSmokeArrayProperty -Object $Selection -Name "slots")
        if ($slots.Count -eq 0) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$Path.slots" -Message "must contain at least one slot"
        }

        for ($slotIndex = 0; $slotIndex -lt $slots.Count; $slotIndex++) {
            Test-ProjectTemplateSmokeSlotSpec -SlotSpec $slots[$slotIndex] -Path "$Path.slots[$slotIndex]" -Issues $Issues
        }
    }
}

function Test-ProjectTemplateSmokeManifest {
    param(
        [string]$RepoRoot,
        [string]$ManifestPath,
        [string]$BuildDir,
        [switch]$CheckPaths
    )

    $issues = New-Object 'System.Collections.Generic.List[object]'
    $entryReports = New-Object 'System.Collections.Generic.List[object]'
    $manifest = $null

    try {
        $manifest = Get-Content -Raw -LiteralPath $ManifestPath | ConvertFrom-Json
    } catch {
        Add-ProjectTemplateSmokeValidationIssue -Issues $issues -Path "manifest" -Message ("failed to parse JSON: " + $_.Exception.Message)
    }

    if ($null -eq $manifest) {
        return [pscustomobject]@{
            generated_at = (Get-Date).ToString("s")
            manifest_path = $ManifestPath
            build_dir = $BuildDir
            passed = $false
            error_count = $issues.Count
            entry_count = 0
            errors = $issues.ToArray()
            entries = @()
        }
    }

    $entries = @(Get-ProjectTemplateSmokeArrayProperty -Object $manifest -Name "entries")
    if ($entries.Count -eq 0) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $issues -Path "entries" -Message "must contain at least one entry"
    }

    $seenNames = @{}

    for ($entryIndex = 0; $entryIndex -lt $entries.Count; $entryIndex++) {
        $entry = $entries[$entryIndex]
        $entryPath = "entries[$entryIndex]"
        $entryIssues = New-Object 'System.Collections.Generic.List[object]'
        $configuredChecks = New-Object 'System.Collections.Generic.List[string]'

        $name = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "name"
        if ([string]::IsNullOrWhiteSpace($name)) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.name" -Message "must be a non-empty string"
        } elseif ($seenNames.ContainsKey($name)) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.name" -Message "must be unique across all entries"
        } else {
            $seenNames[$name] = $true
        }

        $inputDocx = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "input_docx"
        $inputDocxBuildRelative = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "input_docx_build_relative"
        $prepareSampleTarget = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "prepare_sample_target"
        $prepareArgument = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "prepare_argument"

        if ([string]::IsNullOrWhiteSpace($inputDocx) -and [string]::IsNullOrWhiteSpace($inputDocxBuildRelative)) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path $entryPath -Message "must provide 'input_docx' or 'input_docx_build_relative'"
        }
        if (-not [string]::IsNullOrWhiteSpace($inputDocx) -and -not [string]::IsNullOrWhiteSpace($inputDocxBuildRelative)) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path $entryPath -Message "'input_docx' conflicts with 'input_docx_build_relative'"
        }
        if (-not [string]::IsNullOrWhiteSpace($inputDocxBuildRelative) -and [string]::IsNullOrWhiteSpace($BuildDir)) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.input_docx_build_relative" -Message "requires -BuildDir when validating or running the manifest"
        }
        if (-not [string]::IsNullOrWhiteSpace($prepareArgument) -and [string]::IsNullOrWhiteSpace($prepareSampleTarget)) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.prepare_argument" -Message "requires 'prepare_sample_target'"
        }

        if ($CheckPaths) {
            if (-not [string]::IsNullOrWhiteSpace($inputDocx) -and [string]::IsNullOrWhiteSpace($prepareSampleTarget)) {
                $resolvedInputDocx = Resolve-ProjectTemplateSmokePath -RepoRoot $RepoRoot -InputPath $inputDocx -AllowMissing
                if (-not (Test-Path -LiteralPath $resolvedInputDocx)) {
                    Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.input_docx" -Message "does not exist: $resolvedInputDocx"
                }
            }

            if (-not [string]::IsNullOrWhiteSpace($inputDocxBuildRelative) -and
                -not [string]::IsNullOrWhiteSpace($BuildDir) -and
                [string]::IsNullOrWhiteSpace($prepareSampleTarget)) {
                $resolvedBuildRelativeInput = Resolve-ProjectTemplateSmokePath -RepoRoot $BuildDir -InputPath $inputDocxBuildRelative -AllowMissing
                if (-not (Test-Path -LiteralPath $resolvedBuildRelativeInput)) {
                    Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.input_docx_build_relative" -Message "does not exist: $resolvedBuildRelativeInput"
                }
            }
        }

        $templateValidations = @(Get-ProjectTemplateSmokeArrayProperty -Object $entry -Name "template_validations")
        if ($templateValidations.Count -gt 0) {
            $configuredChecks.Add("template_validations") | Out-Null
            for ($validationIndex = 0; $validationIndex -lt $templateValidations.Count; $validationIndex++) {
                Test-ProjectTemplateSmokeSelection `
                    -Selection $templateValidations[$validationIndex] `
                    -Path "$entryPath.template_validations[$validationIndex]" `
                    -RequireSlots:$true `
                    -Issues $entryIssues
            }
        }

        $schemaValidation = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "schema_validation"
        if ($null -ne $schemaValidation) {
            $configuredChecks.Add("schema_validation") | Out-Null
            $schemaFile = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaValidation -Name "schema_file"
            $targets = @(Get-ProjectTemplateSmokeArrayProperty -Object $schemaValidation -Name "targets")

            if ([string]::IsNullOrWhiteSpace($schemaFile) -and $targets.Count -eq 0) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.schema_validation" -Message "must provide 'schema_file' or at least one target"
            }
            if ($CheckPaths -and -not [string]::IsNullOrWhiteSpace($schemaFile)) {
                $resolvedSchemaFile = Resolve-ProjectTemplateSmokePath -RepoRoot $RepoRoot -InputPath $schemaFile -AllowMissing
                if (-not (Test-Path -LiteralPath $resolvedSchemaFile)) {
                    Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.schema_validation.schema_file" -Message "does not exist: $resolvedSchemaFile"
                }
            }

            for ($targetIndex = 0; $targetIndex -lt $targets.Count; $targetIndex++) {
                Test-ProjectTemplateSmokeSelection `
                    -Selection $targets[$targetIndex] `
                    -Path "$entryPath.schema_validation.targets[$targetIndex]" `
                    -RequireSlots:$true `
                    -Issues $entryIssues
            }
        }

        $schemaBaseline = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "schema_baseline"
        if ($null -ne $schemaBaseline) {
            $configuredChecks.Add("schema_baseline") | Out-Null
            $schemaFile = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "schema_file"
            $targetMode = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "target_mode"

            if ([string]::IsNullOrWhiteSpace($schemaFile)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.schema_baseline.schema_file" -Message "must be a non-empty string"
            } elseif ($CheckPaths) {
                $resolvedSchemaFile = Resolve-ProjectTemplateSmokePath -RepoRoot $RepoRoot -InputPath $schemaFile -AllowMissing
                if (-not (Test-Path -LiteralPath $resolvedSchemaFile)) {
                    Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.schema_baseline.schema_file" -Message "does not exist: $resolvedSchemaFile"
                }
            }

            if (-not [string]::IsNullOrWhiteSpace($targetMode) -and $targetMode -notin @("default", "section-targets", "resolved-section-targets")) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.schema_baseline.target_mode" -Message "must be one of: default, section-targets, resolved-section-targets"
            }
        }

        $visualSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "visual_smoke"
        if ($null -ne $visualSmoke) {
            if ($visualSmoke -is [bool]) {
                if ($visualSmoke) {
                    $configuredChecks.Add("visual_smoke") | Out-Null
                }
            } else {
                $enabledValue = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $visualSmoke -Name "enabled"
                if ($null -ne $enabledValue -and $enabledValue -isnot [bool]) {
                    Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.visual_smoke.enabled" -Message "must be a boolean"
                }
                if ($null -eq $enabledValue -or [bool]$enabledValue) {
                    $configuredChecks.Add("visual_smoke") | Out-Null
                }
            }
        }

        if ($configuredChecks.Count -eq 0) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path $entryPath -Message "does not enable any checks"
        }

        foreach ($issue in $entryIssues) {
            $issues.Add($issue) | Out-Null
        }

        $entryReports.Add([pscustomobject]@{
            name = $name
            configured_checks = $configuredChecks.ToArray()
            error_count = $entryIssues.Count
            errors = $entryIssues.ToArray()
        }) | Out-Null
    }

    return [pscustomobject]@{
        generated_at = (Get-Date).ToString("s")
        manifest_path = $ManifestPath
        build_dir = $BuildDir
        passed = ($issues.Count -eq 0)
        error_count = $issues.Count
        entry_count = $entries.Count
        errors = $issues.ToArray()
        entries = $entryReports.ToArray()
    }
}
