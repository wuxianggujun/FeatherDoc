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

function Test-ProjectTemplateSmokeRequiredStringProperty {
    param(
        $Object,
        [string]$Name,
        [string]$Path,
        [System.Collections.Generic.List[object]]$Issues
    )

    $propertyValue = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $Object -Name $Name
    if ([string]::IsNullOrWhiteSpace($propertyValue)) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path $Path -Message "must be a non-empty string"
    }

    return $propertyValue
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

function Test-ProjectTemplateSmokeBusinessCorpus {
    param(
        [object[]]$CorpusItems,
        [hashtable]$EntryNames,
        [System.Collections.Generic.List[object]]$Issues
    )

    $seenIds = @{}
    $allowedStatuses = @("registered", "planned")
    $allowedContracts = @("template_validations", "schema_validation", "schema_baseline", "render_data", "visual_smoke")

    for ($corpusIndex = 0; $corpusIndex -lt @($CorpusItems).Count; $corpusIndex++) {
        $item = $CorpusItems[$corpusIndex]
        $itemPath = "business_template_corpus[$corpusIndex]"

        $id = Test-ProjectTemplateSmokeRequiredStringProperty -Object $item -Name "id" -Path "$itemPath.id" -Issues $Issues
        [void](Test-ProjectTemplateSmokeRequiredStringProperty -Object $item -Name "project_id" -Path "$itemPath.project_id" -Issues $Issues)
        [void](Test-ProjectTemplateSmokeRequiredStringProperty -Object $item -Name "template_name" -Path "$itemPath.template_name" -Issues $Issues)
        [void](Test-ProjectTemplateSmokeRequiredStringProperty -Object $item -Name "document_type" -Path "$itemPath.document_type" -Issues $Issues)
        [void](Test-ProjectTemplateSmokeRequiredStringProperty -Object $item -Name "coverage_goal" -Path "$itemPath.coverage_goal" -Issues $Issues)

        if (-not [string]::IsNullOrWhiteSpace($id)) {
            if ($seenIds.ContainsKey($id)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$itemPath.id" -Message "must be unique across business_template_corpus"
            } else {
                $seenIds[$id] = $true
            }
        }

        $status = Test-ProjectTemplateSmokeRequiredStringProperty -Object $item -Name "status" -Path "$itemPath.status" -Issues $Issues
        if (-not [string]::IsNullOrWhiteSpace($status) -and $status -notin $allowedStatuses) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$itemPath.status" -Message "must be one of: registered, planned"
        }

        $contracts = @(Get-ProjectTemplateSmokeArrayProperty -Object $item -Name "smoke_contract")
        if ($contracts.Count -eq 0) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$itemPath.smoke_contract" -Message "must contain at least one smoke contract"
        }
        for ($contractIndex = 0; $contractIndex -lt $contracts.Count; $contractIndex++) {
            $contract = [string]$contracts[$contractIndex]
            if ([string]::IsNullOrWhiteSpace($contract)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$itemPath.smoke_contract[$contractIndex]" -Message "must be a non-empty string"
            } elseif ($contract -notin $allowedContracts) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$itemPath.smoke_contract[$contractIndex]" -Message "must be one of: $($allowedContracts -join ', ')"
            }
        }

        $sourceEntry = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $item -Name "source_entry"
        if ($status -eq "registered") {
            if ([string]::IsNullOrWhiteSpace($sourceEntry)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$itemPath.source_entry" -Message "is required when status is registered"
            } elseif (-not $EntryNames.ContainsKey($sourceEntry)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$itemPath.source_entry" -Message "must reference an existing manifest entry"
            }
        } elseif (-not [string]::IsNullOrWhiteSpace($sourceEntry) -and -not $EntryNames.ContainsKey($sourceEntry)) {
            Add-ProjectTemplateSmokeValidationIssue -Issues $Issues -Path "$itemPath.source_entry" -Message "must reference an existing manifest entry when provided"
        }
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
            business_template_corpus_count = 0
            registered_business_template_corpus_count = 0
            planned_business_template_corpus_count = 0
            errors = $issues.ToArray()
            entries = @()
        }
    }

    $entries = @(Get-ProjectTemplateSmokeArrayProperty -Object $manifest -Name "entries")
    $businessTemplateCorpus = @(Get-ProjectTemplateSmokeArrayProperty -Object $manifest -Name "business_template_corpus")
    if ($entries.Count -eq 0) {
        Add-ProjectTemplateSmokeValidationIssue -Issues $issues -Path "entries" -Message "must contain at least one entry"
    }

    $entryNames = @{}
    foreach ($entry in $entries) {
        $entryName = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $entry -Name "name"
        if (-not [string]::IsNullOrWhiteSpace($entryName) -and -not $entryNames.ContainsKey($entryName)) {
            $entryNames[$entryName] = $true
        }
    }

    Test-ProjectTemplateSmokeBusinessCorpus -CorpusItems $businessTemplateCorpus -EntryNames $entryNames -Issues $issues

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
            $repairedOutput = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $schemaBaseline -Name "repaired_output"

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

            if (-not [string]::IsNullOrWhiteSpace($repairedOutput)) {
                [void](Resolve-ProjectTemplateSmokePath -RepoRoot $RepoRoot -InputPath $repairedOutput -AllowMissing)
            }
        }

        $renderDataSmoke = Get-ProjectTemplateSmokeOptionalPropertyObject -Object $entry -Name "render_data_smoke"
        if ($null -ne $renderDataSmoke) {
            $configuredChecks.Add("render_data") | Out-Null

            $dataPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $renderDataSmoke -Name "data_path"
            $mappingPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $renderDataSmoke -Name "mapping_path"
            $exportTargetMode = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $renderDataSmoke -Name "export_target_mode"

            if ([string]::IsNullOrWhiteSpace($dataPath)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.render_data_smoke.data_path" -Message "must be a non-empty string"
            } elseif ($CheckPaths) {
                $resolvedDataPath = Resolve-ProjectTemplateSmokePath -RepoRoot $RepoRoot -InputPath $dataPath -AllowMissing
                if (-not (Test-Path -LiteralPath $resolvedDataPath)) {
                    Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.render_data_smoke.data_path" -Message "does not exist: $resolvedDataPath"
                }
            }

            if ([string]::IsNullOrWhiteSpace($mappingPath)) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.render_data_smoke.mapping_path" -Message "must be a non-empty string"
            } elseif ($CheckPaths) {
                $resolvedMappingPath = Resolve-ProjectTemplateSmokePath -RepoRoot $RepoRoot -InputPath $mappingPath -AllowMissing
                if (-not (Test-Path -LiteralPath $resolvedMappingPath)) {
                    Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.render_data_smoke.mapping_path" -Message "does not exist: $resolvedMappingPath"
                }
            }

            if (-not [string]::IsNullOrWhiteSpace($exportTargetMode) -and
                $exportTargetMode -notin @("loaded-parts", "resolved-section-targets")) {
                Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.render_data_smoke.export_target_mode" -Message "must be one of: loaded-parts, resolved-section-targets"
            }

            foreach ($outputProperty in @("output_docx", "summary_json", "patch_plan_output", "draft_plan_output", "patched_plan_output")) {
                $outputPath = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $renderDataSmoke -Name $outputProperty
                if (-not [string]::IsNullOrWhiteSpace($outputPath)) {
                    [void](Resolve-ProjectTemplateSmokePath -RepoRoot $RepoRoot -InputPath $outputPath -AllowMissing)
                }
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
                $inputValue = Get-ProjectTemplateSmokeOptionalPropertyValue -Object $visualSmoke -Name "input"
                if (-not [string]::IsNullOrWhiteSpace($inputValue) -and
                    $inputValue -notin @("template", "rendered_docx")) {
                    Add-ProjectTemplateSmokeValidationIssue -Issues $entryIssues -Path "$entryPath.visual_smoke.input" -Message "must be one of: template, rendered_docx"
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
        business_template_corpus_count = $businessTemplateCorpus.Count
        registered_business_template_corpus_count = @($businessTemplateCorpus | Where-Object {
                (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $_ -Name "status") -eq "registered"
            }).Count
        planned_business_template_corpus_count = @($businessTemplateCorpus | Where-Object {
                (Get-ProjectTemplateSmokeOptionalPropertyValue -Object $_ -Name "status") -eq "planned"
            }).Count
        errors = $issues.ToArray()
        entries = $entryReports.ToArray()
    }
}
