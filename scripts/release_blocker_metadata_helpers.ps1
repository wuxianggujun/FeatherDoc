function Get-ReleaseBlockerPropertyObject {
    param(
        [AllowNull()]$Object,
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

function Get-ReleaseBlockerPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-ReleaseBlockerPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return ""
    }

    return [string]$value
}

function Get-ReleaseBlockerIntPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name,
        [int]$DefaultValue = 0
    )

    $value = Get-ReleaseBlockerPropertyObject -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }

    try {
        return [int]$value
    } catch {
        return $DefaultValue
    }
}

function Get-ReleaseBlockerBoolPropertyDisplayValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-ReleaseBlockerPropertyObject -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return ""
    }

    if ($value -is [bool]) {
        return ([string]$value).ToLowerInvariant()
    }

    $text = [string]$value
    if ([string]::Equals($text, "true", [System.StringComparison]::OrdinalIgnoreCase)) {
        return "true"
    }
    if ([string]::Equals($text, "false", [System.StringComparison]::OrdinalIgnoreCase)) {
        return "false"
    }

    return $text
}

function Get-ReleaseBlockerArrayProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-ReleaseBlockerPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }
    if ($value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($value)) {
            return @()
        }

        return @($value)
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Get-ReleaseBlockers {
    param([AllowNull()]$Summary)

    return @(Get-ReleaseBlockerArrayProperty -Object $Summary -Name "release_blockers")
}

function Get-ReleaseBlockerDeclaredCount {
    param(
        [AllowNull()]$Summary,
        [string]$Context = "release summary"
    )

    $countObject = Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_count"
    if ($null -eq $countObject) {
        return $null
    }

    $countValue = [string]$countObject
    if ([string]::IsNullOrWhiteSpace($countValue)) {
        return $null
    }

    try {
        $declaredCount = [int]$countValue
    } catch {
        throw "release_blocker_count must be an integer when provided in ${Context}; actual value: $countValue"
    }

    if ($declaredCount -lt 0) {
        throw "release_blocker_count must not be negative in ${Context}; actual value: $declaredCount"
    }

    return $declaredCount
}

function Assert-ReleaseBlockerCountConsistency {
    param(
        [AllowNull()]$Summary,
        [string]$Context = "release summary"
    )

    $declaredCount = Get-ReleaseBlockerDeclaredCount -Summary $Summary -Context $Context
    if ($null -eq $declaredCount) {
        return
    }

    $actualCount = @(Get-ReleaseBlockers -Summary $Summary).Count
    if ($declaredCount -ne $actualCount) {
        throw "release_blocker_count mismatch in ${Context}: declared $declaredCount but release_blockers contains $actualCount item(s)."
    }
}

function Assert-ReleaseBlockerMetadataQuality {
    param(
        [AllowNull()]$Summary,
        [string]$Context = "release summary"
    )

    Assert-ReleaseBlockerCountConsistency -Summary $Summary -Context $Context

    $requiredFields = @("id", "source", "status", "severity", "action")
    $seenIds = New-Object 'System.Collections.Generic.HashSet[string]' ([System.StringComparer]::OrdinalIgnoreCase)
    $releaseBlockers = @(Get-ReleaseBlockers -Summary $Summary)
    for ($blockerIndex = 0; $blockerIndex -lt $releaseBlockers.Count; $blockerIndex++) {
        $releaseBlocker = $releaseBlockers[$blockerIndex]
        foreach ($fieldName in $requiredFields) {
            $fieldValue = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name $fieldName
            if ([string]::IsNullOrWhiteSpace($fieldValue)) {
                throw "release_blockers[$blockerIndex].$fieldName must not be empty in ${Context}."
            }
        }

        $blockerId = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name "id"
        if (-not $seenIds.Add($blockerId)) {
            throw "duplicate release blocker id '$blockerId' in ${Context}."
        }
    }
}

function Assert-ReleaseGovernanceDeclaredCountConsistency {
    param(
        [AllowNull()]$Container,
        [string]$ContainerName,
        [string]$CountProperty,
        [string]$ItemsProperty,
        [string]$Context
    )

    $countObject = Get-ReleaseBlockerPropertyObject -Object $Container -Name $CountProperty
    if ($null -eq $countObject -or [string]::IsNullOrWhiteSpace([string]$countObject)) {
        return
    }

    try {
        $declaredCount = [int]([string]$countObject)
    } catch {
        throw "$ContainerName.$CountProperty must be an integer when provided in ${Context}; actual value: $countObject"
    }

    if ($declaredCount -lt 0) {
        throw "$ContainerName.$CountProperty must not be negative in ${Context}; actual value: $declaredCount"
    }

    $actualCount = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name $ItemsProperty).Count
    if ($declaredCount -ne $actualCount) {
        throw "$ContainerName.$CountProperty mismatch in ${Context}: declared $declaredCount but $ContainerName.$ItemsProperty contains $actualCount item(s)."
    }
}

function Assert-ReleaseGovernanceReviewerMetadataItemQuality {
    param(
        [AllowNull()]$Item,
        [string]$ItemPath,
        [string[]]$RequiredFields,
        [string]$Context
    )

    foreach ($fieldName in @($RequiredFields)) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if ([string]::IsNullOrWhiteSpace($fieldValue)) {
            throw "$ItemPath.$fieldName must not be empty in ${Context}."
        }
    }
}

function Assert-ReleaseGovernanceReviewerMetadataCollectionQuality {
    param(
        [AllowNull()]$Container,
        [string]$ContainerName,
        [string]$Context
    )

    if ($null -eq $Container) {
        return
    }

    Assert-ReleaseGovernanceDeclaredCountConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -CountProperty "release_blocker_count" `
        -ItemsProperty "release_blockers" `
        -Context $Context
    Assert-ReleaseGovernanceDeclaredCountConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -CountProperty "warning_count" `
        -ItemsProperty "warnings" `
        -Context $Context
    Assert-ReleaseGovernanceDeclaredCountConsistency `
        -Container $Container `
        -ContainerName $ContainerName `
        -CountProperty "action_item_count" `
        -ItemsProperty "action_items" `
        -Context $Context

    $blockerRequiredFields = @("id", "action", "message", "source_schema", "source_report_display", "source_json_display")
    $warningRequiredFields = @("id", "action", "message", "source_schema", "source_report_display", "source_json_display")
    $actionItemRequiredFields = @("id", "action", "open_command", "source_schema", "source_report_display", "source_json_display")

    $releaseBlockers = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name "release_blockers")
    for ($itemIndex = 0; $itemIndex -lt $releaseBlockers.Count; $itemIndex++) {
        Assert-ReleaseGovernanceReviewerMetadataItemQuality `
            -Item $releaseBlockers[$itemIndex] `
            -ItemPath "$ContainerName.release_blockers[$itemIndex]" `
            -RequiredFields $blockerRequiredFields `
            -Context $Context
    }

    $warnings = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name "warnings")
    for ($itemIndex = 0; $itemIndex -lt $warnings.Count; $itemIndex++) {
        Assert-ReleaseGovernanceReviewerMetadataItemQuality `
            -Item $warnings[$itemIndex] `
            -ItemPath "$ContainerName.warnings[$itemIndex]" `
            -RequiredFields $warningRequiredFields `
            -Context $Context
    }

    $actionItems = @(Get-ReleaseBlockerArrayProperty -Object $Container -Name "action_items")
    for ($itemIndex = 0; $itemIndex -lt $actionItems.Count; $itemIndex++) {
        Assert-ReleaseGovernanceReviewerMetadataItemQuality `
            -Item $actionItems[$itemIndex] `
            -ItemPath "$ContainerName.action_items[$itemIndex]" `
            -RequiredFields $actionItemRequiredFields `
            -Context $Context
    }
}

function Assert-ReleaseGovernanceReviewerMetadataQuality {
    param(
        [AllowNull()]$Summary,
        [string]$Context = "release summary"
    )

    Assert-ReleaseGovernanceReviewerMetadataCollectionQuality `
        -Container (Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup") `
        -ContainerName "release_blocker_rollup" `
        -Context $Context
    Assert-ReleaseGovernanceReviewerMetadataCollectionQuality `
        -Container (Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff") `
        -ContainerName "release_governance_handoff" `
        -Context $Context

    $stages = @(Get-ReleaseBlockerArrayProperty -Object $Summary -Name "stages")
    for ($stageIndex = 0; $stageIndex -lt $stages.Count; $stageIndex++) {
        $stageId = Get-ReleaseBlockerDisplayValue `
            -Value (Get-ReleaseBlockerPropertyValue -Object $stages[$stageIndex] -Name "id") `
            -Fallback $stageIndex
        Assert-ReleaseGovernanceReviewerMetadataCollectionQuality `
            -Container $stages[$stageIndex] `
            -ContainerName "stages[$($stageIndex):$stageId]" `
            -Context $Context
    }
}

function Get-ReleaseBlockerCount {
    param([AllowNull()]$Summary)

    $declaredCount = Get-ReleaseBlockerDeclaredCount -Summary $Summary
    if ($null -ne $declaredCount) {
        Assert-ReleaseBlockerCountConsistency -Summary $Summary
        return $declaredCount
    }

    return @(Get-ReleaseBlockers -Summary $Summary).Count
}

function Get-ReleaseBlockerDisplayValue {
    param(
        [AllowNull()]$Value,
        [string]$Fallback = "(not available)"
    )

    if ($null -eq $Value -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        return $Fallback
    }

    return [string]$Value
}

function Get-ReleaseBlockerDisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    try {
        $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
            $Path
        } elseif (-not [string]::IsNullOrWhiteSpace($RepoRoot)) {
            Join-Path $RepoRoot $Path
        } else {
            $Path
        }
        $resolvedPath = [System.IO.Path]::GetFullPath($candidate)
        if (-not [string]::IsNullOrWhiteSpace($RepoRoot)) {
            $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
            if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
                $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
                if ([string]::IsNullOrWhiteSpace($relative)) {
                    return "."
                }

                return ".\" + ($relative -replace '/', '\')
            }
        }

        return $resolvedPath
    } catch {
        return $Path
    }
}

function Get-ReleaseBlockerPublicDisplayPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    $displayPath = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $Path
    if ([string]::IsNullOrWhiteSpace($displayPath) -or
        [string]::Equals($displayPath, "(not available)", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $displayPath
    }

    if ($displayPath.StartsWith(".\") -or
        $displayPath.StartsWith("./") -or
        -not [System.IO.Path]::IsPathRooted($displayPath)) {
        return $displayPath
    }

    return Split-Path -Leaf $displayPath
}

function Convert-ReleaseBlockerLocalPathsForPublicText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }

    return [regex]::Replace(
        $Text,
        '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|;,)]+|(?<!\w)/(?:Users|home)/[^\s"''`<>|;,)]+',
        '<local-path>'
    )
}

function Join-ReleaseBlockerValues {
    param([object[]]$Values)

    $normalizedValues = @($Values |
        Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
        ForEach-Object { [string]$_ } |
        Sort-Object -Unique)
    if ($normalizedValues.Count -eq 0) {
        return "(none)"
    }

    return ($normalizedValues -join ",")
}

function Get-ReleaseBlockerSummaryText {
    param([AllowNull()]$Blocker)

    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "id") -Fallback "(unknown)"
    $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "status")
    $severity = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "severity")
    $source = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source")
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action")
    $blockedItemCount = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "blocked_item_count") -Fallback "0"
    $issueKeys = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $Blocker -Name "issue_keys")

    return "{0}: status={1} severity={2} source={3} action={4} blocked_items={5} issues={6}" -f `
        $id, $status, $severity, $source, $action, $blockedItemCount, $issueKeys
}

function Get-ReleaseBlockerItemSummaryText {
    param([AllowNull()]$Item)

    $name = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "name") -Fallback "(unknown item)"
    $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "status")
    $decision = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "decision")
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "action")
    $issueKeys = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "compliance_issues")

    return "{0}: status={1} decision={2} action={3} issues={4}" -f `
        $name, $status, $decision, $action, $issueKeys
}

function Add-ReleaseBlockerActionGuidanceLine {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Text
    )

    if (-not [string]::IsNullOrWhiteSpace($Text)) {
        [void]$Lines.Add($Text)
    }
}

function Get-PdfVisualPreflightBlockingSummaryLine {
    param([AllowNull()]$Item)

    $blockingSummary = Get-ReleaseBlockerPropertyObject -Object $Item -Name "blocking_summary"
    if ($null -eq $blockingSummary) {
        return ""
    }

    $requiredCheckCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "required_check_count"
    $blockingCheckCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "blocking_check_count"
    $missingCliPdfCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "missing_cli_pdf_count"
    $visualBaselineSampleCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "visual_baseline_sample_count"
    $missingVisualBaselinePdfCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "missing_visual_baseline_pdf_count"
    $cjkTextLayerSampleCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "cjk_text_layer_sample_count"
    $missingCjkTextLayerPdfCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "missing_cjk_text_layer_pdf_count"
    $buildDirEntryCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "build_dir_entry_count"
    $ctestRequiredPatternCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "ctest_required_pattern_count"
    $outputGapCount = Get-ReleaseBlockerIntPropertyValue -Object $Item -Name "output_gap_count"
    $missingOutputCount = Get-ReleaseBlockerIntPropertyValue -Object $Item -Name "missing_output_count"
    $memoryGuardBlocked = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "memory_guard_blocked"
    $memoryGuardSkipped = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "memory_guard_skipped"
    $freeMemoryMb = Get-ReleaseBlockerPropertyValue -Object $blockingSummary -Name "free_memory_mb"
    $minFreeMemoryMb = Get-ReleaseBlockerPropertyValue -Object $blockingSummary -Name "min_free_memory_mb"
    $pdfBuildOptionsEnabled = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "pdf_build_options_enabled"
    $disabledPdfBuildOptions = @(Get-ReleaseBlockerArrayProperty -Object $blockingSummary -Name "disabled_pdf_build_options" | ForEach-Object { [string]$_ })
    $missingPdfBuildOptions = @(Get-ReleaseBlockerArrayProperty -Object $blockingSummary -Name "missing_pdf_build_options" | ForEach-Object { [string]$_ })

    if (($requiredCheckCount + $blockingCheckCount + $missingCliPdfCount +
            $visualBaselineSampleCount + $missingVisualBaselinePdfCount +
            $cjkTextLayerSampleCount + $missingCjkTextLayerPdfCount +
            $buildDirEntryCount + $ctestRequiredPatternCount +
            $outputGapCount + $missingOutputCount) -le 0 -and
        [string]::IsNullOrWhiteSpace($memoryGuardBlocked) -and
        [string]::IsNullOrWhiteSpace($memoryGuardSkipped) -and
        [string]::IsNullOrWhiteSpace($freeMemoryMb) -and
        [string]::IsNullOrWhiteSpace($minFreeMemoryMb) -and
        [string]::IsNullOrWhiteSpace($pdfBuildOptionsEnabled) -and
        $disabledPdfBuildOptions.Count -eq 0 -and
        $missingPdfBuildOptions.Count -eq 0) {
        return ""
    }

    $summaryLine = ("PDF preflight blocker summary: required checks={0}, blocking checks={1}, missing CLI PDFs={2}, visual baseline samples={3}, missing visual baseline PDFs={4}, CJK text-layer samples={5}, missing CJK text-layer PDFs={6}, build dir entries={7}, CTest required patterns={8}, output gap checks={9}, missing outputs={10}." -f `
        $requiredCheckCount,
        $blockingCheckCount,
        $missingCliPdfCount,
        $visualBaselineSampleCount,
        $missingVisualBaselinePdfCount,
        $cjkTextLayerSampleCount,
        $missingCjkTextLayerPdfCount,
        $buildDirEntryCount,
        $ctestRequiredPatternCount,
        $outputGapCount,
        $missingOutputCount)
    $memoryParts = @()
    if (-not [string]::IsNullOrWhiteSpace($memoryGuardBlocked)) {
        $memoryParts += "memory guard blocked=$memoryGuardBlocked"
    }
    if (-not [string]::IsNullOrWhiteSpace($memoryGuardSkipped)) {
        $memoryParts += "memory guard skipped=$memoryGuardSkipped"
    }
    if (-not [string]::IsNullOrWhiteSpace($freeMemoryMb)) {
        $memoryParts += "free memory MB=$freeMemoryMb"
    }
    if (-not [string]::IsNullOrWhiteSpace($minFreeMemoryMb)) {
        $memoryParts += "minimum free memory MB=$minFreeMemoryMb"
    }
    if ($memoryParts.Count -gt 0) {
        $summaryLine += " Memory guard: $($memoryParts -join ', ')."
    }
    $pdfBuildOptionParts = @()
    if (-not [string]::IsNullOrWhiteSpace($pdfBuildOptionsEnabled)) {
        $pdfBuildOptionParts += "enabled=$pdfBuildOptionsEnabled"
    }
    if ($disabledPdfBuildOptions.Count -gt 0) {
        $pdfBuildOptionParts += "disabled=$($disabledPdfBuildOptions -join ',')"
    }
    if ($missingPdfBuildOptions.Count -gt 0) {
        $pdfBuildOptionParts += "missing=$($missingPdfBuildOptions -join ',')"
    }
    if ($pdfBuildOptionParts.Count -gt 0) {
        $summaryLine += " PDF build options: $($pdfBuildOptionParts -join ', ')."
    }

    return $summaryLine
}

function Get-PdfVisualPreflightBuildCandidateSummaryLine {
    param([AllowNull()]$Item)

    $candidates = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "build_dir_auto_candidates")
    if ($candidates.Count -eq 0) {
        return ""
    }

    $candidateParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($candidate in $candidates) {
        $relativePath = Get-ReleaseBlockerPropertyValue -Object $candidate -Name "relative_path"
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            $relativePath = Get-ReleaseBlockerPropertyValue -Object $candidate -Name "path"
        }
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            $relativePath = "(unknown)"
        }

        $stateParts = @()
        foreach ($stateName in @(
                [ordered]@{ Label = "exists"; Name = "exists" },
                [ordered]@{ Label = "CMakeCache"; Name = "cmake_cache_exists" },
                [ordered]@{ Label = "CTest"; Name = "ctest_manifest_exists" },
                [ordered]@{ Label = "PDF options"; Name = "pdf_build_options_enabled" },
                [ordered]@{ Label = "reusable"; Name = "looks_reusable" }
            )) {
            $value = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $candidate -Name $stateName.Name
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                $stateParts += "$($stateName.Label)=$value"
            }
        }

        $pdfOptionParts = @()
        foreach ($option in @(Get-ReleaseBlockerArrayProperty -Object $candidate -Name "pdf_build_options")) {
            $name = Get-ReleaseBlockerPropertyValue -Object $option -Name "name"
            if ([string]::IsNullOrWhiteSpace($name)) {
                continue
            }

            $present = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $option -Name "present"
            $value = Get-ReleaseBlockerPropertyValue -Object $option -Name "value"
            $enabled = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $option -Name "enabled"
            $optionStateParts = @()
            if (-not [string]::IsNullOrWhiteSpace($present)) {
                $optionStateParts += "present=$present"
            }
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                $optionStateParts += "value=$value"
            }
            if (-not [string]::IsNullOrWhiteSpace($enabled)) {
                $optionStateParts += "enabled=$enabled"
            }
            if ($optionStateParts.Count -gt 0) {
                $pdfOptionParts += "$name($($optionStateParts -join ','))"
            } else {
                $pdfOptionParts += $name
            }
        }

        if ($pdfOptionParts.Count -gt 0) {
            $stateParts += "options=$($pdfOptionParts -join ';')"
        }

        if ($stateParts.Count -gt 0) {
            $candidateParts.Add("$relativePath($($stateParts -join ', '))") | Out-Null
        } else {
            $candidateParts.Add($relativePath) | Out-Null
        }
    }

    return "PDF preflight build auto candidates: $($candidateParts.ToArray() -join '; ')."
}

function Get-PdfVisualPreflightDependencyInputSummaryLine {
    param([AllowNull()]$Item)

    $dependencyInputs = Get-ReleaseBlockerPropertyObject -Object $Item -Name "pdf_dependency_inputs"
    $blockingSummary = Get-ReleaseBlockerPropertyObject -Object $Item -Name "blocking_summary"
    $status = Get-ReleaseBlockerPropertyValue -Object $dependencyInputs -Name "status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        $status = Get-ReleaseBlockerPropertyValue -Object $blockingSummary -Name "pdf_dependency_inputs_status"
    }
    $selectedPdfiumProvider = Get-ReleaseBlockerPropertyValue -Object $dependencyInputs -Name "selected_pdfium_provider"
    if ([string]::IsNullOrWhiteSpace($selectedPdfiumProvider)) {
        $selectedPdfiumProvider = Get-ReleaseBlockerPropertyValue -Object $blockingSummary -Name "selected_pdfium_provider"
    }
    $pdfioReady = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $dependencyInputs -Name "pdfio_ready"
    if ([string]::IsNullOrWhiteSpace($pdfioReady)) {
        $pdfioReady = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "pdfio_dependency_ready"
    }
    $pdfiumReady = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $dependencyInputs -Name "pdfium_ready"
    if ([string]::IsNullOrWhiteSpace($pdfiumReady)) {
        $pdfiumReady = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "pdfium_dependency_ready"
    }

    $missingInputCount = Get-ReleaseBlockerIntPropertyValue -Object $dependencyInputs -Name "missing_input_count" -DefaultValue -1
    if ($missingInputCount -lt 0) {
        $missingInputCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "pdf_dependency_missing_input_count" -DefaultValue -1
    }
    $missingInputs = @(Get-ReleaseBlockerArrayProperty -Object $dependencyInputs -Name "missing_inputs" | ForEach-Object { [string]$_ })
    if ($missingInputs.Count -eq 0) {
        $missingInputs = @(Get-ReleaseBlockerArrayProperty -Object $blockingSummary -Name "pdf_dependency_missing_inputs_preview" | ForEach-Object { [string]$_ })
    }

    $parts = @()
    if (-not [string]::IsNullOrWhiteSpace($status)) {
        $parts += "status=$status"
    }
    if (-not [string]::IsNullOrWhiteSpace($selectedPdfiumProvider)) {
        $parts += "selected PDFium provider=$selectedPdfiumProvider"
    }
    if (-not [string]::IsNullOrWhiteSpace($pdfioReady)) {
        $parts += "PDFio ready=$pdfioReady"
    }
    if (-not [string]::IsNullOrWhiteSpace($pdfiumReady)) {
        $parts += "PDFium ready=$pdfiumReady"
    }
    if ($missingInputCount -ge 0) {
        $parts += "missing inputs=$missingInputCount"
    }
    if ($missingInputs.Count -gt 0) {
        $publicMissingInputs = @(
            $missingInputs |
                Select-Object -First 5 |
                ForEach-Object { Convert-ReleaseBlockerLocalPathsForPublicText -Text ([string]$_) }
        )
        $parts += "missing input preview=$($publicMissingInputs -join '; ')"
    }

    if ($parts.Count -eq 0) {
        return ""
    }

    return "PDF dependency inputs: $($parts -join ', ')."
}

function Get-PdfVisualPreflightReadinessActionEvidenceLine {
    param([AllowNull()]$Item)

    $evidenceItems = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "readiness_action_evidence")
    if ($evidenceItems.Count -eq 0) {
        return ""
    }

    $evidenceParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($evidence in $evidenceItems) {
        $action = Get-ReleaseBlockerDisplayValue `
            -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "action") `
            -Fallback "(unknown action)"
        $issueKey = Get-ReleaseBlockerDisplayValue `
            -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "issue_key") `
            -Fallback "(unknown issue)"
        $item = Convert-ReleaseBlockerLocalPathsForPublicText `
            -Text (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "item")

        $part = "$action/$issueKey"
        if (-not [string]::IsNullOrWhiteSpace($item)) {
            $part += " -> $item"
        }

        [void]$evidenceParts.Add($part)
    }

    $summaryLine = "Readiness action evidence: $($evidenceParts.ToArray() -join '; ')."
    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        foreach ($evidence in $evidenceItems) {
            $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $evidence -Name "source_json_display"
            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                break
            }
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $summaryLine += " source JSON: $sourceJsonDisplay"
    }

    return $summaryLine
}

function Get-ReleaseBlockerRegisteredActions {
    return @(
        "complete_visual_manual_review",
        "fix_schema_patch_approval_result",
        "prepare_pdf_visual_release_gate_build_outputs",
        "rerun_pdf_visual_release_gate_preflight"
    )
}

function Test-ReleaseBlockerActionRegistered {
    param([string]$Action)

    if ([string]::IsNullOrWhiteSpace($Action)) {
        return $false
    }

    foreach ($registeredAction in @(Get-ReleaseBlockerRegisteredActions)) {
        if ([string]::Equals($registeredAction, $Action, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Get-ReleaseBlockerActionGuidanceLines {
    param(
        [AllowNull()]$Blocker,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $action = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action"
    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    if (-not (Test-ReleaseBlockerActionRegistered -Action $action)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Unregistered release blocker action `{0}`: add a fixed checklist runbook in `release_blocker_metadata_helpers.ps1`; until then, follow the blocker summary, update the owning evidence, rerun the relevant release checks, and regenerate the release note bundle before public release.' -f $action)
        return $guidanceLines.ToArray()
    }

    switch ($action) {
        "fix_schema_patch_approval_result" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `fix_schema_patch_approval_result`: update blocked `schema_patch_approval_result.json` record(s), provide reviewer and reviewed_at for non-pending decisions, then refresh release metadata.'

            $historyMarkdown = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "history_markdown"
            if (-not [string]::IsNullOrWhiteSpace($historyMarkdown)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Open the schema approval history markdown before editing approval records: {0}" -f (Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $historyMarkdown))
            }

            $summaryJson = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "summary_json"
            $displaySummaryJson = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $summaryJson
            $displayReleaseSummaryJson = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Run `sync_project_template_schema_approval.ps1` after updating approval records, using project template summary `{0}` and release summary `{1}`, then regenerate the release note bundle.' -f $displaySummaryJson, $displayReleaseSummaryJson)
            break
        }
        "complete_visual_manual_review" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `complete_visual_manual_review`: open the referenced visual review task, record the reviewer verdict and reviewed_at, then resync the visual verdict metadata before public release.'
            break
        }
        "prepare_pdf_visual_release_gate_build_outputs" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `prepare_pdf_visual_release_gate_build_outputs`: prepare or reuse the PDF visual release gate build outputs before attempting the full PDF visual gate.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightBlockingSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightBuildCandidateSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightDependencyInputSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $Blocker)
            $issueKeys = @(Get-ReleaseBlockerArrayProperty -Object $Blocker -Name "issue_keys" | ForEach-Object { [string]$_ })
            if ($issueKeys -contains "cmake_cache_exists") {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Preflight issue `cmake_cache_exists` means the selected build directory is not a reusable CMake build; prepare or point at a build directory containing `CMakeCache.txt` before checking CTest registration or PDF outputs.'
            }
            if ($issueKeys -contains "pdf_build_options_enabled") {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Preflight issue `pdf_build_options_enabled` means the selected CMake cache does not enable the full PDF writer/import build; reconfigure with `-DFEATHERDOC_BUILD_PDF=ON` and `-DFEATHERDOC_BUILD_PDF_IMPORT=ON` plus the required PDFio/PDFium inputs before generating visual gate outputs.'
            }

            $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_report_display"
            if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
                $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_report")
            }
            if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Open the PDF preflight governance report first: {0}" -f $sourceReportDisplay)
            }

            $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_json_display"
            if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_json")
            }
            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Open the source preflight JSON before changing build outputs: {0}" -f $sourceJsonDisplay)
            }

            $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "command_template"
            if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
                $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly'
            }
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Run the lightweight preflight command first: `{0}`' -f $commandTemplate)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'The `-PreflightOnly` command only verifies PDF visual gate prerequisites; it is not release-ready evidence until the full PDF visual release gate passes.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Only after preflight is ready and workstation resources allow it, run the full PDF visual release gate, rebuild the release blocker rollup, and regenerate the release note bundle.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'After each PDF preflight or gate attempt, clean up only task-owned PDF gate processes and transient outputs after capturing the required evidence; do not terminate unrelated external build, Office, browser, node, or PowerShell processes.'
            break
        }
        "rerun_pdf_visual_release_gate_preflight" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `rerun_pdf_visual_release_gate_preflight`: regenerate the PDF visual release gate preflight summary, then rebuild the PDF preflight governance report.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightBlockingSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightBuildCandidateSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightDependencyInputSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $Blocker)

            $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_json_display"
            if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_json")
            }
            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Inspect the missing or unreadable preflight JSON path before rerunning: {0}" -f $sourceJsonDisplay)
            }

            $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "command_template"
            if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
                $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json'
            }
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Run the lightweight preflight regeneration command: `{0}`' -f $commandTemplate)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'A regenerated PDF preflight summary only refreshes prerequisite evidence; it is not release-ready evidence until the full PDF visual release gate passes.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'After the preflight summary is readable, rerun `write_pdf_visual_release_gate_preflight_governance_report.ps1`, rebuild the release blocker rollup, and regenerate the release note bundle.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'After each PDF preflight or gate attempt, clean up only task-owned PDF gate processes and transient outputs after capturing the required evidence; do not terminate unrelated external build, Office, browser, node, or PowerShell processes.'
            break
        }
        default {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Registered release blocker action `{0}` does not have a checklist runbook yet; add one in `release_blocker_metadata_helpers.ps1` before relying on this action for handoff.' -f $action)
        }
    }

    return $guidanceLines.ToArray()
}

function Add-ReleaseBlockerMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Blockers",
        [switch]$IncludeWhenEmpty,
        [switch]$PublicArtifactPaths
    )

    Assert-ReleaseBlockerMetadataQuality -Summary $Summary -Context $Heading
    $releaseBlockerCount = Get-ReleaseBlockerCount -Summary $Summary
    $releaseBlockers = @(Get-ReleaseBlockers -Summary $Summary)
    if ($releaseBlockerCount -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Release blockers: $releaseBlockerCount")
    if ($releaseBlockerCount -le 0) {
        return
    }

    foreach ($releaseBlocker in $releaseBlockers) {
        [void]$Lines.Add("- $(Get-ReleaseBlockerSummaryText -Blocker $releaseBlocker)")
        $message = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name "message"
        if (-not [string]::IsNullOrWhiteSpace($message)) {
            [void]$Lines.Add("  - Message: $message")
        }

        foreach ($pathInfo in @(
                @{ Name = "summary_json"; Label = "Summary JSON" },
                @{ Name = "history_json"; Label = "History JSON" },
                @{ Name = "history_markdown"; Label = "History Markdown" }
            )) {
            $pathValue = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name $pathInfo.Name
            if (-not [string]::IsNullOrWhiteSpace($pathValue)) {
                $displayPath = if ($PublicArtifactPaths.IsPresent) {
                    Get-ReleaseBlockerPublicDisplayPath -RepoRoot $RepoRoot -Path $pathValue
                } else {
                    Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $pathValue
                }
                [void]$Lines.Add("  - $($pathInfo.Label): $displayPath")
            }
        }

        foreach ($item in @(Get-ReleaseBlockerArrayProperty -Object $releaseBlocker -Name "items")) {
            [void]$Lines.Add("  - Item $(Get-ReleaseBlockerItemSummaryText -Item $item)")
        }
    }
}

function Get-ReleaseGovernanceRollup {
    param([AllowNull()]$Summary)

    return Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_blocker_rollup"
}

function Get-ReleaseGovernanceHandoff {
    param([AllowNull()]$Summary)

    return Get-ReleaseBlockerPropertyObject -Object $Summary -Name "release_governance_handoff"
}

function Add-ReleaseGovernanceRollupSourceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot
    )

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath `
            -RepoRoot $RepoRoot `
            -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath `
            -RepoRoot $RepoRoot `
            -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }

    $projectId = Get-ReleaseBlockerPropertyValue -Object $Item -Name "project_id"
    $templateName = Get-ReleaseBlockerPropertyValue -Object $Item -Name "template_name"
    if (-not [string]::IsNullOrWhiteSpace($projectId) -or -not [string]::IsNullOrWhiteSpace($templateName)) {
        [void]$Lines.Add("  - project_template: project_id=$projectId template_name=$templateName")
    }

    $candidateType = Get-ReleaseBlockerPropertyValue -Object $Item -Name "candidate_type"
    if (-not [string]::IsNullOrWhiteSpace($candidateType)) {
        [void]$Lines.Add("  - candidate_type: $candidateType")
    }

    foreach ($fieldName in @("source_report", "source_json")) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            [void]$Lines.Add("  - ${fieldName}: $fieldValue")
        }
    }

    [void]$Lines.Add("  - source_report_display: $sourceReportDisplay")
    [void]$Lines.Add("  - source_json_display: $sourceJsonDisplay")
    Add-ProjectTemplateOnboardingGovernanceContractLines `
        -Lines $Lines `
        -Item $Item `
        -SourceReportDisplay $sourceReportDisplay `
        -SourceJsonDisplay $sourceJsonDisplay
    Add-ReleaseGovernanceProvenanceLines -Lines $Lines -Item $Item
}

function Add-ProjectTemplateOnboardingGovernanceContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$SourceReportDisplay,
        [string]$SourceJsonDisplay
    )

    $sourceSchema = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_schema"
    if (-not [string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
        return
    }

    $schemaApprovalSummary = Get-ReleaseBlockerPropertyValue -Object $Item -Name "schema_approval_status_summary"
    if ([string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
        $schemaApprovalSummary = Get-ReleaseBlockerPropertyValue -Object $Item -Name "status"
    }
    if ([string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
        $schemaApprovalSummary = "unknown"
    }

    [void]$Lines.Add("  - project_template_onboarding_governance_contract:")
    [void]$Lines.Add("    - source_schema: featherdoc.project_template_onboarding_governance_report.v1")
    [void]$Lines.Add("    - schema_approval_status_summary: $schemaApprovalSummary")
    if (-not [string]::IsNullOrWhiteSpace($SourceReportDisplay)) {
        [void]$Lines.Add("    - source_report_display: $SourceReportDisplay")
    }
    if (-not [string]::IsNullOrWhiteSpace($SourceJsonDisplay)) {
        [void]$Lines.Add("    - source_json_display: $SourceJsonDisplay")
    }
}

function Add-ReleaseGovernanceProvenanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    foreach ($fieldName in @("input_docx_display", "input_docx", "schema_target", "target_mode")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$Lines.Add("  - ${fieldName}: $value")
        }
    }
}

function Add-ReleaseGovernanceRepairLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    foreach ($fieldName in @("repair_strategy", "repair_hint", "command_template")) {
        $value = Convert-ReleaseBlockerLocalPathsForPublicText `
            -Text (Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName)
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$Lines.Add("  - ${fieldName}: $value")
        }
    }
}

function Add-ReleaseGovernanceReadinessActionEvidenceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item
    )

    $evidenceItems = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "readiness_action_evidence")
    $evidenceCount = Get-ReleaseBlockerPropertyValue -Object $Item -Name "readiness_action_evidence_count"
    if ([string]::IsNullOrWhiteSpace($evidenceCount)) {
        if ($evidenceItems.Count -eq 0) {
            return
        }

        $evidenceCount = [string]$evidenceItems.Count
    }

    [void]$Lines.Add("  - readiness_action_evidence_count: $evidenceCount")
    if ($evidenceItems.Count -eq 0) {
        return
    }

    [void]$Lines.Add("  - readiness_action_evidence:")
    foreach ($evidence in $evidenceItems) {
        $id = Get-ReleaseBlockerDisplayValue `
            -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "id") `
            -Fallback "(unknown evidence)"
        $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "action")
        $issueKey = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "issue_key")
        $item = Get-ReleaseBlockerDisplayValue `
            -Value (Convert-ReleaseBlockerLocalPathsForPublicText -Text (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "item"))
        [void]$Lines.Add("    - ${id}: action=$action issue_key=$issueKey item=$item")

        foreach ($fieldName in @("source_schema", "source_report", "source_report_display", "source_json", "source_json_display")) {
            $fieldValue = Convert-ReleaseBlockerLocalPathsForPublicText `
                -Text (Get-ReleaseBlockerPropertyValue -Object $evidence -Name $fieldName)
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                [void]$Lines.Add("      - ${fieldName}: $fieldValue")
            }
        }
    }
}

function Add-ReleaseGovernanceReportIssueLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object[]]$Reports,
        [string]$Heading,
        [string]$IdPropertyName,
        [string]$PathPropertyName,
        [string]$DisplayPropertyName,
        [string]$RepoRoot
    )

    $issueReports = @(
        foreach ($report in @($Reports)) {
            $status = Get-ReleaseBlockerPropertyValue -Object $report -Name "status"
            $errorText = Get-ReleaseBlockerPropertyValue -Object $report -Name "error"
            $sourceFailureCount = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_failure_count"
            if ($status -in @("failed", "missing") -or
                -not [string]::IsNullOrWhiteSpace($errorText) -or
                ($sourceFailureCount -match '^[0-9]+$' -and [int]$sourceFailureCount -gt 0)) {
                $report
            }
        }
    )
    if ($issueReports.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    foreach ($report in $issueReports) {
        $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name $IdPropertyName) -Fallback "(unknown report)"
        $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "status")
        $releaseReady = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_ready")
        $sourceFailureCount = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "source_failure_count") -Fallback "0"
        $schema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")
        $sourceReport = Get-ReleaseBlockerPropertyValue -Object $report -Name $PathPropertyName
        $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $report -Name $DisplayPropertyName
        if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $sourceReport
        }

        [void]$Lines.Add("- ${id}: status=$status ready=$releaseReady source_failures=$sourceFailureCount schema=$schema")
        if (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
            [void]$Lines.Add("  - source_report: $sourceReport")
        }
        [void]$Lines.Add("  - source_report_display: $(Get-ReleaseBlockerDisplayValue -Value $sourceReportDisplay)")

        $sourceJson = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_json"
        if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
            [void]$Lines.Add("  - source_json: $sourceJson")
        }

        $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_json_display"
        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay) -and -not [string]::IsNullOrWhiteSpace($sourceJson)) {
            $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $sourceJson
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            [void]$Lines.Add("  - source_json_display: $(Get-ReleaseBlockerDisplayValue -Value $sourceJsonDisplay)")
        }
        [void]$Lines.Add("  - source_failure_count: $sourceFailureCount")
        Add-ProjectTemplateDeliveryReadinessContractLines `
            -Lines $Lines `
            -Report $report `
            -SourceReportDisplay $sourceReportDisplay `
            -SourceJsonDisplay $sourceJsonDisplay

        $errorText = Get-ReleaseBlockerPropertyValue -Object $report -Name "error"
        if (-not [string]::IsNullOrWhiteSpace($errorText)) {
            [void]$Lines.Add("  - error: $errorText")
        }

        $buildCommand = Get-ReleaseBlockerPropertyValue -Object $report -Name "build_command"
        if (-not [string]::IsNullOrWhiteSpace($buildCommand)) {
            [void]$Lines.Add("  - build: $buildCommand")
        }
    }
}

function Test-ProjectTemplateDeliveryReadinessReport {
    param([AllowNull()]$Report)

    $id = Get-ReleaseBlockerPropertyValue -Object $Report -Name "id"
    $schema = Get-ReleaseBlockerPropertyValue -Object $Report -Name "schema"

    return (
        [string]::Equals($id, "project_template_delivery_readiness", [System.StringComparison]::OrdinalIgnoreCase) -or
        [string]::Equals($schema, "featherdoc.project_template_delivery_readiness_report.v1", [System.StringComparison]::OrdinalIgnoreCase)
    )
}

function Add-ProjectTemplateDeliveryReadinessContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Report,
        [string]$SourceReportDisplay,
        [string]$SourceJsonDisplay
    )

    if (-not (Test-ProjectTemplateDeliveryReadinessReport -Report $Report)) {
        return
    }

    $sourceSchema = Get-ReleaseBlockerPropertyValue -Object $Report -Name "schema"
    if ([string]::IsNullOrWhiteSpace($sourceSchema)) {
        $sourceSchema = "featherdoc.project_template_delivery_readiness_report.v1"
    }

    [void]$Lines.Add("  - project_template_delivery_readiness_contract:")
    [void]$Lines.Add("    - source_schema: $sourceSchema")

    foreach ($fieldName in @(
            "status",
            "release_ready",
            "latest_schema_approval_gate_status",
            "schema_history_blocked_run_count",
            "schema_history_pending_run_count",
            "schema_history_passed_run_count",
            "template_count",
            "ready_template_count",
            "blocked_template_count",
            "release_blocker_count",
            "action_item_count",
            "warning_count",
            "source_failure_count"
        )) {
        $fieldValue = Get-ReleaseBlockerPropertyValue -Object $Report -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
            [void]$Lines.Add("    - ${fieldName}: $fieldValue")
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($SourceReportDisplay)) {
        [void]$Lines.Add("    - source_report_display: $SourceReportDisplay")
    }
    if (-not [string]::IsNullOrWhiteSpace($SourceJsonDisplay)) {
        [void]$Lines.Add("    - source_json_display: $SourceJsonDisplay")
    }
}

function Add-ReleaseGovernanceSourceReportContractLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object[]]$Reports,
        [string]$Heading,
        [string]$IdPropertyName,
        [string]$PathPropertyName,
        [string]$DisplayPropertyName,
        [string]$RepoRoot
    )

    $contractReports = @($Reports | Where-Object { $null -ne $_ })
    if ($contractReports.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    foreach ($report in $contractReports) {
        $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name $IdPropertyName) -Fallback "(unknown report)"
        $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "status")
        $releaseReady = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "release_ready")
        $sourceFailureCount = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "source_failure_count") -Fallback "0"
        $schema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $report -Name "schema")
        $sourceReport = Get-ReleaseBlockerPropertyValue -Object $report -Name $PathPropertyName
        $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $report -Name $DisplayPropertyName
        if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $sourceReport
        }

        [void]$Lines.Add("- ${id}: status=$status ready=$releaseReady source_failures=$sourceFailureCount schema=$schema")
        if (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
            [void]$Lines.Add("  - source_report: $sourceReport")
        }
        [void]$Lines.Add("  - source_report_display: $(Get-ReleaseBlockerDisplayValue -Value $sourceReportDisplay)")

        $sourceJson = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_json"
        if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
            [void]$Lines.Add("  - source_json: $sourceJson")
        }

        $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $report -Name "source_json_display"
        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay) -and -not [string]::IsNullOrWhiteSpace($sourceJson)) {
            $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $sourceJson
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            [void]$Lines.Add("  - source_json_display: $(Get-ReleaseBlockerDisplayValue -Value $sourceJsonDisplay)")
        }
        [void]$Lines.Add("  - source_failure_count: $sourceFailureCount")
        Add-ProjectTemplateDeliveryReadinessContractLines `
            -Lines $Lines `
            -Report $report `
            -SourceReportDisplay $sourceReportDisplay `
            -SourceJsonDisplay $sourceJsonDisplay

        foreach ($fieldName in @(
                "preflight_ready",
                "full_visual_gate_required",
                "full_visual_gate_status",
                "controlled_visual_smoke_available",
                "controlled_visual_smoke_status",
                "controlled_visual_smoke_passed",
                "controlled_visual_smoke_case_count",
                "controlled_visual_smoke_json",
                "controlled_visual_smoke_json_display",
                "error",
                "build_command"
            )) {
            $fieldValue = Get-ReleaseBlockerPropertyValue -Object $report -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                $label = if ($fieldName -eq "build_command") { "build" } else { $fieldName }
                [void]$Lines.Add("  - ${label}: $fieldValue")
            }
        }
    }
}

function Get-ReleaseGovernanceChecklistSections {
    param([AllowNull()]$Summary)

    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    return @(
        [pscustomobject]@{
            Context = "Release blocker rollup"
            SummaryObject = Get-ReleaseGovernanceRollup -Summary $Summary
        },
        [pscustomobject]@{
            Context = "Release governance handoff"
            SummaryObject = $handoff
        },
        [pscustomobject]@{
            Context = "Release governance handoff nested rollup"
            SummaryObject = Get-ReleaseBlockerPropertyObject -Object $handoff -Name "release_blocker_rollup"
        }
    )
}

function Get-NormalizedReleaseGovernanceBlockers {
    param([AllowNull()]$Blockers)

    $normalizedBlockers = New-Object 'System.Collections.Generic.List[object]'
    foreach ($blocker in @(Get-ReleaseBlockerArrayProperty -Object ([ordered]@{ release_blockers = $Blockers }) -Name "release_blockers")) {
        if ($null -eq $blocker) {
            continue
        }

        [void]$normalizedBlockers.Add([pscustomobject][ordered]@{
                composite_id = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "composite_id")
                id = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id")
                project_id = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "project_id")
                template_name = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "template_name")
                candidate_type = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "candidate_type")
                action = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")
                status = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "status")
                severity = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "severity")
                source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema")
                source_report = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report")
                source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report_display")
                source_json = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_json")
                source_json_display = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_json_display")
                message = [string](Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message")
            })
    }

    return @($normalizedBlockers.ToArray())
}

function Get-ReleaseGovernanceBlockerCount {
    param([AllowNull()]$SummaryObject)

    $declaredCount = Get-ReleaseBlockerPropertyObject -Object $SummaryObject -Name "release_blocker_count"
    if ($null -ne $declaredCount -and -not [string]::IsNullOrWhiteSpace([string]$declaredCount)) {
        try {
            return [int][string]$declaredCount
        } catch {
            # Fall back to the materialized array when older fixtures used
            # non-integer placeholders.
        }
    }

    return @(Get-NormalizedReleaseGovernanceBlockers -Blockers (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "release_blockers")).Count
}

function Get-NormalizedReleaseGovernanceWarnings {
    param([AllowNull()]$Warnings)

    $normalizedWarnings = New-Object 'System.Collections.Generic.List[object]'
    foreach ($warning in @(Get-ReleaseBlockerArrayProperty -Object ([ordered]@{ warnings = $Warnings }) -Name "warnings")) {
        if ($null -eq $warning) {
            continue
        }

        $entry = [ordered]@{
            id = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "id")
            project_id = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "project_id")
            template_name = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "template_name")
            candidate_type = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "candidate_type")
            action = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")
            message = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "message")
            source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema")
            source_report = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_report")
            source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_report_display")
            source_json = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_json")
            source_json_display = [string](Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_json_display")
        }

        $styleMergeSuggestionCount = Get-ReleaseBlockerPropertyObject -Object $warning -Name "style_merge_suggestion_count"
        if ($null -ne $styleMergeSuggestionCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionCount)) {
            $entry.style_merge_suggestion_count = [int]$styleMergeSuggestionCount
        }
        $styleMergeSuggestionPendingCount = Get-ReleaseBlockerPropertyObject -Object $warning -Name "style_merge_suggestion_pending_count"
        if ($null -ne $styleMergeSuggestionPendingCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionPendingCount)) {
            $entry.style_merge_suggestion_pending_count = [int]$styleMergeSuggestionPendingCount
        }

        [void]$normalizedWarnings.Add([pscustomobject]$entry)
    }

    return @($normalizedWarnings.ToArray())
}

function Get-ReleaseGovernanceWarningCount {
    param([AllowNull()]$SummaryObject)

    $declaredCount = Get-ReleaseBlockerPropertyObject -Object $SummaryObject -Name "warning_count"
    if ($null -ne $declaredCount -and -not [string]::IsNullOrWhiteSpace([string]$declaredCount)) {
        try {
            return [int][string]$declaredCount
        } catch {
            # Fall back to the materialized warnings array for legacy fixtures.
        }
    }

    return @(Get-NormalizedReleaseGovernanceWarnings -Warnings (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "warnings")).Count
}

function Get-NormalizedReleaseGovernanceActionItems {
    param([AllowNull()]$ActionItems)

    $normalizedActions = New-Object 'System.Collections.Generic.List[object]'
    foreach ($item in @(Get-ReleaseBlockerArrayProperty -Object ([ordered]@{ action_items = $ActionItems }) -Name "action_items")) {
        if ($null -eq $item) {
            continue
        }

        $entry = [ordered]@{
            id = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "id")
            project_id = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "project_id")
            template_name = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "template_name")
            candidate_type = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "candidate_type")
            action = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "action")
            title = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "title")
            source_schema = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema")
            source_report = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report")
            source_report_display = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report_display")
            source_json = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_json")
            source_json_display = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "source_json_display")
            command = [string](Get-ReleaseBlockerPropertyValue -Object $item -Name "command")
        }

        foreach ($optionalCommandName in @("open_command", "audit_command", "review_command")) {
            $optionalCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name $optionalCommandName
            if (-not [string]::IsNullOrWhiteSpace($optionalCommand)) {
                $entry[$optionalCommandName] = [string]$optionalCommand
            }
        }

        foreach ($repairFieldName in @("repair_strategy", "repair_hint", "command_template")) {
            $repairValue = Get-ReleaseBlockerPropertyValue -Object $item -Name $repairFieldName
            if (-not [string]::IsNullOrWhiteSpace($repairValue)) {
                $entry[$repairFieldName] = [string]$repairValue
            }
        }

        [void]$normalizedActions.Add([pscustomobject]$entry)
    }

    return @($normalizedActions.ToArray())
}

function Get-ReleaseGovernanceActionItemCount {
    param([AllowNull()]$SummaryObject)

    $declaredCount = Get-ReleaseBlockerPropertyObject -Object $SummaryObject -Name "action_item_count"
    if ($null -ne $declaredCount -and -not [string]::IsNullOrWhiteSpace([string]$declaredCount)) {
        try {
            return [int][string]$declaredCount
        } catch {
            # Fall back to the materialized action_items array for legacy fixtures.
        }
    }

    return @(Get-NormalizedReleaseGovernanceActionItems -ActionItems (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "action_items")).Count
}

function Get-ReleaseGovernanceActionItemSummaryText {
    param([AllowNull()]$ActionItem)

    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "action")
    $title = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "title")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "source_schema")
    $summaryText = ('id: `{0}`; action: `{1}`; title: {2}; source_schema: `{3}`' -f `
            $id, $action, $title, $sourceSchema)

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; {0}: `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $summaryText += ('; source_report: `{0}`' -f $sourceReportDisplay)
    }

    foreach ($fieldName in @("source_report", "source_json")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; raw_{0}: `{1}`' -f $fieldName, $value)
        }
    }

    return $summaryText
}

function Get-ReleaseGovernanceWarningSummaryText {
    param([AllowNull()]$Warning)

    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "action")
    $message = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "message")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_schema")
    $summaryText = ('id: `{0}`; action: `{1}`; message: {2}; source_schema: `{3}`' -f `
            $id, $action, $message, $sourceSchema)

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Warning -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; {0}: `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Warning -Name "source_json_display"
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $summaryText += ('; source_report: `{0}`' -f $sourceReportDisplay)
    }

    foreach ($fieldName in @("source_report", "source_json")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Warning -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; raw_{0}: `{1}`' -f $fieldName, $value)
        }
    }

    $styleMergeSuggestionCount = Get-ReleaseBlockerPropertyObject -Object $Warning -Name "style_merge_suggestion_count"
    if ($null -ne $styleMergeSuggestionCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionCount)) {
        $summaryText += ('; style_merge_suggestion_count: `{0}`' -f [string]$styleMergeSuggestionCount)
    }
    $styleMergeSuggestionPendingCount = Get-ReleaseBlockerPropertyObject -Object $Warning -Name "style_merge_suggestion_pending_count"
    if ($null -ne $styleMergeSuggestionPendingCount -and -not [string]::IsNullOrWhiteSpace([string]$styleMergeSuggestionPendingCount)) {
        $summaryText += ('; style_merge_suggestion_pending_count: `{0}`' -f [string]$styleMergeSuggestionPendingCount)
    }

    return $summaryText
}

function Get-ReleaseGovernanceBlockerSummaryText {
    param([AllowNull()]$Blocker)

    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "id") -Fallback "(unknown)"
    $summaryText = ('id: `{0}`; action: `{1}`' -f `
            $id,
            (Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action")))

    $compositeId = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "composite_id"
    if (-not [string]::IsNullOrWhiteSpace($compositeId)) {
        $summaryText += ('; composite_id: `{0}`' -f $compositeId)
    }

    foreach ($field in @("project_id", "template_name", "candidate_type", "status", "severity", "source_schema", "source_report_display", "source_json_display", "source_report", "source_json")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name $field
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $summaryText += ('; {0}: `{1}`' -f $field, $value)
        }
    }

    $message = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "message"
    if (-not [string]::IsNullOrWhiteSpace($message)) {
        $summaryText += ('; message: {0}' -f $message)
    }

    return $summaryText
}

function Add-ReleaseGovernanceBlockerMarkdownSubsection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Heading,
        [AllowNull()]$SummaryObject,
        [switch]$IncludeWhenEmpty
    )

    $releaseBlockers = @(Get-NormalizedReleaseGovernanceBlockers -Blockers (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "release_blockers"))
    $releaseBlockerCount = Get-ReleaseGovernanceBlockerCount -SummaryObject $SummaryObject
    if ($releaseBlockerCount -le 0 -and $releaseBlockers.Count -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return $false
    }

    [void]$Lines.Add("### $Heading")
    [void]$Lines.Add("")
    [void]$Lines.Add(('- release_blocker_count: `{0}`' -f $releaseBlockerCount))
    foreach ($releaseBlocker in $releaseBlockers) {
        [void]$Lines.Add("- $(Get-ReleaseGovernanceBlockerSummaryText -Blocker $releaseBlocker)")
    }
    [void]$Lines.Add("")

    return $true
}

function Add-ReleaseGovernanceBlockersMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [switch]$IncludeWhenEmpty
    )

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    $hadSection = $false
    foreach ($sectionInfo in @(Get-ReleaseGovernanceChecklistSections -Summary $Summary)) {
        if (Add-ReleaseGovernanceBlockerMarkdownSubsection `
                -Lines $sectionLines `
                -Heading "$($sectionInfo.Context) blockers" `
                -SummaryObject $sectionInfo.SummaryObject `
                -IncludeWhenEmpty:$IncludeWhenEmpty.IsPresent) {
            $hadSection = $true
        }
    }

    if (-not $hadSection) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add("## Release Governance Blockers")
    [void]$Lines.Add("")
    foreach ($line in $sectionLines) {
        [void]$Lines.Add($line)
    }
}

function Add-ReleaseGovernanceActionItemMarkdownSubsection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Heading,
        [AllowNull()]$SummaryObject,
        [switch]$IncludeWhenEmpty
    )

    $actionItems = @(Get-NormalizedReleaseGovernanceActionItems -ActionItems (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "action_items"))
    $actionItemCount = Get-ReleaseGovernanceActionItemCount -SummaryObject $SummaryObject
    if ($actionItemCount -le 0 -and $actionItems.Count -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return $false
    }

    [void]$Lines.Add("### $Heading")
    [void]$Lines.Add("")
    [void]$Lines.Add(('- action_item_count: `{0}`' -f $actionItemCount))
    foreach ($item in $actionItems) {
        [void]$Lines.Add("- $(Get-ReleaseGovernanceActionItemSummaryText -ActionItem $item)")
        foreach ($commandName in @("command", "open_command", "audit_command", "review_command")) {
            $commandValue = Get-ReleaseBlockerPropertyValue -Object $item -Name $commandName
            if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
                [void]$Lines.Add("  - ${commandName}: ``$commandValue``")
            }
        }
        Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $item
    }
    [void]$Lines.Add("")

    return $true
}

function Add-ReleaseGovernanceActionItemsMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [switch]$IncludeWhenEmpty
    )

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    $hadSection = $false
    foreach ($sectionInfo in @(Get-ReleaseGovernanceChecklistSections -Summary $Summary)) {
        if (Add-ReleaseGovernanceActionItemMarkdownSubsection `
                -Lines $sectionLines `
                -Heading "$($sectionInfo.Context) action items" `
                -SummaryObject $sectionInfo.SummaryObject `
                -IncludeWhenEmpty:$IncludeWhenEmpty.IsPresent) {
            $hadSection = $true
        }
    }

    if (-not $hadSection) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add("## Release Governance Action Items")
    [void]$Lines.Add("")
    foreach ($line in $sectionLines) {
        [void]$Lines.Add($line)
    }
}

function Add-ReleaseGovernanceWarningMarkdownSubsection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Heading,
        [AllowNull()]$SummaryObject,
        [switch]$IncludeWhenEmpty
    )

    $warnings = @(Get-NormalizedReleaseGovernanceWarnings -Warnings (Get-ReleaseBlockerArrayProperty -Object $SummaryObject -Name "warnings"))
    $warningCount = Get-ReleaseGovernanceWarningCount -SummaryObject $SummaryObject
    if ($warningCount -le 0 -and $warnings.Count -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return $false
    }

    [void]$Lines.Add("### $Heading")
    [void]$Lines.Add("")
    [void]$Lines.Add(('- warning_count: `{0}`' -f $warningCount))
    foreach ($warning in $warnings) {
        [void]$Lines.Add("- $(Get-ReleaseGovernanceWarningSummaryText -Warning $warning)")
    }
    [void]$Lines.Add("")

    return $true
}

function Add-ReleaseGovernanceWarningsMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [switch]$IncludeWhenEmpty
    )

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    $hadSection = $false
    foreach ($sectionInfo in @(Get-ReleaseGovernanceChecklistSections -Summary $Summary)) {
        if (Add-ReleaseGovernanceWarningMarkdownSubsection `
                -Lines $sectionLines `
                -Heading "$($sectionInfo.Context) warnings" `
                -SummaryObject $sectionInfo.SummaryObject `
                -IncludeWhenEmpty:$IncludeWhenEmpty.IsPresent) {
            $hadSection = $true
        }
    }

    if (-not $hadSection) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add("## Release Governance Warnings")
    [void]$Lines.Add("")
    foreach ($line in $sectionLines) {
        [void]$Lines.Add($line)
    }
}

function Get-ReleaseGovernanceChecklistItems {
    param(
        [AllowNull()]$Summary,
        [string]$ArrayPropertyName,
        [string]$ContextSuffix
    )

    $items = New-Object 'System.Collections.Generic.List[object]'
    foreach ($sectionInfo in @(Get-ReleaseGovernanceChecklistSections -Summary $Summary)) {
        if ($null -eq $sectionInfo.SummaryObject) {
            continue
        }

        foreach ($item in @(Get-ReleaseBlockerArrayProperty -Object $sectionInfo.SummaryObject -Name $ArrayPropertyName)) {
            [void]$items.Add([pscustomobject]@{
                    context = "$($sectionInfo.Context) $ContextSuffix"
                    item = $item
                })
        }
    }

    return @($items.ToArray())
}

function Get-ReleaseGovernanceBlockerChecklistItems {
    param([AllowNull()]$Summary)

    return @(Get-ReleaseGovernanceChecklistItems `
            -Summary $Summary `
            -ArrayPropertyName "release_blockers" `
            -ContextSuffix "blockers")
}

function Get-ReleaseGovernanceActionItemChecklistItems {
    param([AllowNull()]$Summary)

    return @(Get-ReleaseGovernanceChecklistItems `
            -Summary $Summary `
            -ArrayPropertyName "action_items" `
            -ContextSuffix "action items")
}

function Get-ReleaseGovernanceWarningChecklistItems {
    param([AllowNull()]$Summary)

    return @(Get-ReleaseGovernanceChecklistItems `
            -Summary $Summary `
            -ArrayPropertyName "warnings" `
            -ContextSuffix "warnings")
}

function Get-ReleaseGovernanceChecklistSourceDisplay {
    param(
        [AllowNull()]$Item,
        [string]$RepoRoot,
        [string]$DisplayPropertyName,
        [string]$PathPropertyName
    )

    $displayValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $DisplayPropertyName
    if (-not [string]::IsNullOrWhiteSpace($displayValue)) {
        return $displayValue
    }

    $pathValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $PathPropertyName
    if ([string]::IsNullOrWhiteSpace($pathValue)) {
        return ""
    }

    return Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $pathValue
}

function Get-ReleaseGovernanceBlockerChecklistText {
    param([AllowNull()]$BlockerItem)

    $context = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $BlockerItem -Name "context")
    $blocker = Get-ReleaseBlockerPropertyObject -Object $BlockerItem -Name "item"
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")
    $status = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "status")
    $severity = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "severity")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema")
    $message = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message")
    $text = 'Resolve release governance blocker `{0}` ({1}): action `{2}`, status `{3}`, severity `{4}`, source_schema `{5}`, message: {6}' -f `
        $id, $context, $action, $status, $severity, $sourceSchema, $message

    $reportId = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "report_id"
    if (-not [string]::IsNullOrWhiteSpace($reportId)) {
        $text += ('; report_id `{0}`' -f $reportId)
    }

    $compositeId = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "composite_id"
    if (-not [string]::IsNullOrWhiteSpace($compositeId)) {
        $text += ('; composite_id `{0}`' -f $compositeId)
    }

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $blocker -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $text += ('; {0} `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    return $text
}

function Get-ReleaseGovernanceActionItemChecklistText {
    param([AllowNull()]$ActionItem)

    $context = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $ActionItem -Name "context")
    $item = Get-ReleaseBlockerPropertyObject -Object $ActionItem -Name "item"
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema")
    $title = Get-ReleaseBlockerPropertyValue -Object $item -Name "title"
    $text = 'Review release governance action item `{0}` ({1}): action `{2}`, source_schema `{3}`' -f `
        $id, $context, $action, $sourceSchema

    if (-not [string]::IsNullOrWhiteSpace($title)) {
        $text += ('; title: {0}' -f $title)
    }

    $issueKeys = Join-ReleaseBlockerValues -Values @(Get-ReleaseBlockerArrayProperty -Object $item -Name "issue_keys")
    if ($issueKeys -ne "(none)") {
        $text += ('; issue_keys `{0}`' -f $issueKeys)
    }

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $text += ('; {0} `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $item -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    return $text
}

function Get-ReleaseGovernanceWarningChecklistText {
    param([AllowNull()]$WarningItem)

    $context = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $WarningItem -Name "context")
    $warning = Get-ReleaseBlockerPropertyObject -Object $WarningItem -Name "item"
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema")
    $message = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "message")
    $text = 'Review release governance warning `{0}` ({1}): action `{2}`, source_schema `{3}`, message: {4}' -f `
        $id, $context, $action, $sourceSchema, $message

    foreach ($fieldName in @("project_id", "template_name", "candidate_type")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $warning -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $text += ('; {0} `{1}`' -f $fieldName, $value)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $text += ('; source_report `{0}`' -f $sourceReportDisplay)
    }

    return $text
}

function Get-ReleaseGovernanceChecklistGuidanceLines {
    param(
        [AllowNull()]$Item,
        [string]$ItemKind,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerPropertyValue -Object $Item -Name "action"
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_schema")
    $sourceReportDisplay = Get-ReleaseGovernanceChecklistSourceDisplay `
        -Item $Item `
        -RepoRoot $RepoRoot `
        -DisplayPropertyName "source_report_display" `
        -PathPropertyName "source_report"
    $sourceJsonDisplay = Get-ReleaseGovernanceChecklistSourceDisplay `
        -Item $Item `
        -RepoRoot $RepoRoot `
        -DisplayPropertyName "source_json_display" `
        -PathPropertyName "source_json"
    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson

    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Open source report `{0}` while handling release governance {1} `{2}`.' -f $sourceReportDisplay, $ItemKind, $id)
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay) -and
        -not [string]::Equals($sourceJsonDisplay, $sourceReportDisplay, [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Open source JSON `{0}` while handling release governance {1} `{2}`.' -f $sourceJsonDisplay, $ItemKind, $id)
    }

    if ([string]::Equals($action, "prepare_pdf_visual_release_gate_build_outputs", [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Use action `{0}` for release governance {1} `{2}`: prepare or reuse the PDF visual release gate build outputs before attempting the full PDF visual gate.' -f $action, $ItemKind, $id)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightBlockingSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightBuildCandidateSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightDependencyInputSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $Item)
        $issueKeys = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "issue_keys" | ForEach-Object { [string]$_ })
        if ($issueKeys -contains "cmake_cache_exists") {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text 'Preflight issue `cmake_cache_exists` means the selected build directory is not a reusable CMake build; prepare or point at a build directory containing `CMakeCache.txt` before checking CTest registration or PDF outputs.'
        }
        if ($issueKeys -contains "pdf_build_options_enabled") {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text 'Preflight issue `pdf_build_options_enabled` means the selected CMake cache does not enable the full PDF writer/import build; reconfigure with `-DFEATHERDOC_BUILD_PDF=ON` and `-DFEATHERDOC_BUILD_PDF_IMPORT=ON` plus the required PDFio/PDFium inputs before generating visual gate outputs.'
        }
        $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
        if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
            $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly'
        }
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Run the lightweight PDF preflight command first: `{0}`' -f $commandTemplate)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text 'The `-PreflightOnly` command only verifies PDF visual gate prerequisites; it is not release-ready evidence until the full PDF visual release gate passes.'
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Only after preflight is ready and workstation resources allow it, run the full PDF visual release gate, rerun release governance checks, and regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text 'After each PDF preflight or gate attempt, clean up only task-owned PDF gate processes and transient outputs after capturing the required evidence; do not terminate unrelated external build, Office, browser, node, or PowerShell processes.'
    } elseif ([string]::Equals($action, "rerun_pdf_visual_release_gate_preflight", [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Use action `{0}` for release governance {1} `{2}`: regenerate the PDF visual release gate preflight summary, then rebuild the PDF preflight governance report.' -f $action, $ItemKind, $id)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightBlockingSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightBuildCandidateSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightDependencyInputSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $Item)
        $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
        if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
            $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json'
        }
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Run the lightweight PDF preflight regeneration command: `{0}`' -f $commandTemplate)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text 'A regenerated PDF preflight summary only refreshes prerequisite evidence; it is not release-ready evidence until the full PDF visual release gate passes.'
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('After the preflight summary is readable, rerun `write_pdf_visual_release_gate_preflight_governance_report.ps1`, rerun release governance checks, and regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text 'After each PDF preflight or gate attempt, clean up only task-owned PDF gate processes and transient outputs after capturing the required evidence; do not terminate unrelated external build, Office, browser, node, or PowerShell processes.'
    }

    $hadCommand = $false
    foreach ($commandName in @("command", "open_command", "audit_command", "review_command")) {
        $commandValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $commandName
        if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
            $hadCommand = $true
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Run `{0}` for release governance {1} `{2}`: `{3}`' -f $commandName, $ItemKind, $id, $commandValue)
        }
    }

    foreach ($repairField in @("repair_strategy", "repair_hint", "command_template")) {
        $repairValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $repairField
        if (-not [string]::IsNullOrWhiteSpace($repairValue)) {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Review `{0}` for release governance {1} `{2}`: {3}' -f $repairField, $ItemKind, $id, $repairValue)
        }
    }

    if (-not $hadCommand) {
        if ([string]::IsNullOrWhiteSpace($action)) {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Action is missing for release governance {0} `{1}` from source_schema `{2}`; update the owning governance report, rerun release governance checks, and regenerate the release note bundle from `{3}` before publishing.' -f $ItemKind, $id, $sourceSchema, $releaseSummaryDisplay)
        } else {
            if ([string]::Equals($ItemKind, "warning", [System.StringComparison]::OrdinalIgnoreCase) -and
                [string]::Equals($action, "review_style_merge_suggestions", [System.StringComparison]::OrdinalIgnoreCase)) {
                Add-ReleaseBlockerActionGuidanceLine `
                    -Lines $guidanceLines `
                    -Text ('Use action `{0}` for release governance warning `{1}` before publishing.' -f $action, $id)
                $pendingCount = Get-ReleaseBlockerPropertyObject -Object $Item -Name "style_merge_suggestion_pending_count"
                if ($null -ne $pendingCount -and -not [string]::IsNullOrWhiteSpace([string]$pendingCount)) {
                    Add-ReleaseBlockerActionGuidanceLine `
                        -Lines $guidanceLines `
                        -Text ('Current pending style merge suggestion count is `{0}`.' -f [string]$pendingCount)
                }
            }

            $actionLabel = if ([string]::Equals($ItemKind, "warning", [System.StringComparison]::OrdinalIgnoreCase)) {
                "warning action"
            } elseif ([string]::Equals($ItemKind, "blocker", [System.StringComparison]::OrdinalIgnoreCase)) {
                "blocker action"
            } else {
                "action"
            }
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Follow {0} `{1}` for release governance {2} `{3}` from source_schema `{4}`: update the owning governance evidence, rerun release governance checks, and regenerate the release note bundle from `{5}` before publishing.' -f $actionLabel, $action, $ItemKind, $id, $sourceSchema, $releaseSummaryDisplay)
        }
    }

    return $guidanceLines.ToArray()
}

function Get-ReleaseGovernanceBlockerActionGuidanceLines {
    param(
        [AllowNull()]$Blocker,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $Blocker `
            -ItemKind "blocker" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Get-ReleaseGovernanceActionItemActionGuidanceLines {
    param(
        [AllowNull()]$ActionItem,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $ActionItem `
            -ItemKind "action item" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Get-ReleaseGovernanceWarningActionGuidanceLines {
    param(
        [AllowNull()]$Warning,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $Warning `
            -ItemKind "warning" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Add-ReleaseGovernanceMetricsMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$Heading = "### Governance Metrics"
    )

    $metrics = @(Get-ReleaseBlockerArrayProperty -Object $Summary -Name "governance_metrics")
    if ($metrics.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    foreach ($metric in $metrics) {
        $reportId = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "report_id")
        $metricName = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "metric")
        $metricId = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "id")
        $level = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "level")
        $score = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "score")
        $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_schema")
        $sourceReport = Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_report"
        $sourceJson = Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_json"
        $sourceReportDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_report_display")
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_json_display")

        [void]$Lines.Add("- ${metricId}: report=$reportId metric=$metricName level=$level score=$score source_schema=$sourceSchema")
        if (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
            [void]$Lines.Add("  - source_report: $sourceReport")
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
            [void]$Lines.Add("  - source_json: $sourceJson")
        }
        [void]$Lines.Add("  - source_report_display: $sourceReportDisplay")
        [void]$Lines.Add("  - source_json_display: $sourceJsonDisplay")
        Add-ReleaseGovernanceMetricDetailLines -Lines $Lines -Metric $metric
    }
}

function Add-ReleaseGovernanceMetricDetailLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Metric
    )

    $details = Get-ReleaseBlockerPropertyObject -Object $Metric -Name "details"
    if ($null -eq $details) {
        return
    }

    $detailFields = @(
        "document_count",
        "catalog_exemplar_count",
        "baseline_entry_count",
        "matched_document_count",
        "unmatched_catalog_document_count",
        "unmatched_baseline_document_count",
        "alignment_gap_count",
        "catalog_coverage_percent",
        "baseline_coverage_percent",
        "coverage_score",
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
    )
    $detailParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in $detailFields) {
        $value = Get-ReleaseBlockerPropertyObject -Object $details -Name $fieldName
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            [void]$detailParts.Add("$fieldName=$value")
        }
    }
    if ($detailParts.Count -gt 0) {
        [void]$Lines.Add("  - details: $($detailParts -join ', ')")
    }

    foreach ($fieldName in @("catalog_document_keys", "baseline_document_keys", "matched_document_keys")) {
        $values = @(
            Get-ReleaseBlockerArrayProperty -Object $details -Name $fieldName |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        if ($values.Count -gt 0) {
            [void]$Lines.Add("  - ${fieldName}: $($values -join ',')")
        }
    }

    $penaltyParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($penalty in @(Get-ReleaseBlockerArrayProperty -Object $details -Name "penalty_summary")) {
        $factor = Get-ReleaseBlockerPropertyValue -Object $penalty -Name "factor"
        if ([string]::IsNullOrWhiteSpace($factor)) {
            continue
        }

        $count = Get-ReleaseBlockerPropertyObject -Object $penalty -Name "count"
        $penaltyValue = Get-ReleaseBlockerPropertyObject -Object $penalty -Name "penalty"
        [void]$penaltyParts.Add("$factor(count=$count, penalty=$penaltyValue)")
    }
    if ($penaltyParts.Count -gt 0) {
        [void]$Lines.Add("  - penalty_summary: $($penaltyParts -join '; ')")
    }
}

function Add-ReleaseGovernanceRollupMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Governance Rollup Details"
    )

    $rollup = Get-ReleaseGovernanceRollup -Summary $Summary
    if ($null -eq $rollup) {
        return
    }

    $requested = Get-ReleaseBlockerPropertyValue -Object $rollup -Name "requested"
    $status = Get-ReleaseBlockerPropertyValue -Object $rollup -Name "status"
    $releaseBlockers = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "release_blockers")
    $warnings = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "warnings")
    $actionItems = @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "action_items")
    if ($requested -eq "False" -and
        $status -eq "not_requested" -and
        $releaseBlockers.Count -eq 0 -and
        $warnings.Count -eq 0 -and
        $actionItems.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Status: $(Get-ReleaseBlockerDisplayValue -Value $status)")
    [void]$Lines.Add("- Source reports: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $rollup -Name "source_report_count") -Fallback "0")")
    [void]$Lines.Add("- Source failures: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $rollup -Name "source_failure_count") -Fallback "0")")
    [void]$Lines.Add("- source_failure_count: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $rollup -Name "source_failure_count") -Fallback "0")")
    [void]$Lines.Add("- Blockers: $($releaseBlockers.Count)")
    [void]$Lines.Add("- Warnings: $($warnings.Count)")
    [void]$Lines.Add("- Action items: $($actionItems.Count)")

    Add-ReleaseGovernanceMetricsMarkdownSection -Lines $Lines -Summary $rollup

    Add-ReleaseGovernanceSourceReportContractLines `
        -Lines $Lines `
        -Reports @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "source_reports") `
        -Heading "### Rollup Source Report Contracts" `
        -IdPropertyName "schema" `
        -PathPropertyName "path" `
        -DisplayPropertyName "path_display" `
        -RepoRoot $RepoRoot

    Add-ReleaseGovernanceReportIssueLines `
        -Lines $Lines `
        -Reports @(Get-ReleaseBlockerArrayProperty -Object $rollup -Name "source_reports") `
        -Heading "### Rollup Source Report Issues" `
        -IdPropertyName "schema" `
        -PathPropertyName "path" `
        -DisplayPropertyName "path_display" `
        -RepoRoot $RepoRoot

    if ($releaseBlockers.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Blockers")
        [void]$Lines.Add("")
        foreach ($blocker in $releaseBlockers) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown blocker)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $blocker -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $blocker
            Add-ReleaseGovernanceReadinessActionEvidenceLines -Lines $Lines -Item $blocker
        }
    }

    if ($warnings.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Warnings")
        [void]$Lines.Add("")
        foreach ($warning in $warnings) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown warning)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $warning -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $warning -RepoRoot $RepoRoot
        }
    }

    if ($actionItems.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Rollup Action Items")
        [void]$Lines.Add("")
        foreach ($item in $actionItems) {
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown action item)"): action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema"))")
            $openCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name "open_command"
            if (-not [string]::IsNullOrWhiteSpace($openCommand)) {
                [void]$Lines.Add("  - open_command: $openCommand")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $item -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $item
            Add-ReleaseGovernanceReadinessActionEvidenceLines -Lines $Lines -Item $item
        }
    }
}

function Add-ReleaseGovernanceHandoffMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Governance Handoff Details"
    )

    $handoff = Get-ReleaseGovernanceHandoff -Summary $Summary
    if ($null -eq $handoff) {
        return
    }

    $requested = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "requested"
    $status = Get-ReleaseBlockerPropertyValue -Object $handoff -Name "status"
    $releaseBlockers = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "release_blockers")
    $warnings = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "warnings")
    $actionItems = @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "action_items")
    if ($requested -eq "False" -and
        $status -eq "not_requested" -and
        $releaseBlockers.Count -eq 0 -and
        $warnings.Count -eq 0 -and
        $actionItems.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Status: $(Get-ReleaseBlockerDisplayValue -Value $status)")
    [void]$Lines.Add("- Reports loaded: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "loaded_report_count") -Fallback "0") / $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "expected_report_count") -Fallback "0")")
    [void]$Lines.Add("- Missing reports: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "missing_report_count") -Fallback "0")")
    [void]$Lines.Add("- Failed reports: $(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $handoff -Name "failed_report_count") -Fallback "0")")
    [void]$Lines.Add("- Blockers: $($releaseBlockers.Count)")
    [void]$Lines.Add("- Warnings: $($warnings.Count)")
    [void]$Lines.Add("- Action items: $($actionItems.Count)")

    Add-ReleaseGovernanceMetricsMarkdownSection -Lines $Lines -Summary $handoff

    Add-ReleaseGovernanceReportIssueLines `
        -Lines $Lines `
        -Reports @(Get-ReleaseBlockerArrayProperty -Object $handoff -Name "reports") `
        -Heading "### Handoff Report Issues" `
        -IdPropertyName "id" `
        -PathPropertyName "expected_summary" `
        -DisplayPropertyName "expected_summary_display" `
        -RepoRoot $RepoRoot

    if ($releaseBlockers.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Blockers")
        [void]$Lines.Add("")
        foreach ($blocker in $releaseBlockers) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "id") -Fallback "(unknown blocker)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $blocker -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $blocker -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $blocker -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $blocker
            Add-ReleaseGovernanceReadinessActionEvidenceLines -Lines $Lines -Item $blocker
        }
    }

    if ($warnings.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Warnings")
        [void]$Lines.Add("")
        foreach ($warning in $warnings) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $warning -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "id") -Fallback "(unknown warning)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $warning -Name "source_schema"))")
            $message = Get-ReleaseBlockerPropertyValue -Object $warning -Name "message"
            if (-not [string]::IsNullOrWhiteSpace($message)) {
                [void]$Lines.Add("  - message: $message")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $warning -RepoRoot $RepoRoot
        }
    }

    if ($actionItems.Count -gt 0) {
        [void]$Lines.Add("")
        [void]$Lines.Add("### Handoff Action Items")
        [void]$Lines.Add("")
        foreach ($item in $actionItems) {
            $reportId = Get-ReleaseBlockerPropertyValue -Object $item -Name "report_id"
            $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "id") -Fallback "(unknown action item)"
            [void]$Lines.Add("- $(Get-ReleaseBlockerDisplayValue -Value $reportId -Fallback "(unknown report)") / ${id}: action=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "action")) source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $item -Name "source_schema"))")
            $openCommand = Get-ReleaseBlockerPropertyValue -Object $item -Name "open_command"
            if (-not [string]::IsNullOrWhiteSpace($openCommand)) {
                [void]$Lines.Add("  - open_command: $openCommand")
            }
            Add-ReleaseGovernanceRollupSourceLines -Lines $Lines -Item $item -RepoRoot $RepoRoot
            Add-ReleaseGovernanceRepairLines -Lines $Lines -Item $item
            Add-ReleaseGovernanceReadinessActionEvidenceLines -Lines $Lines -Item $item
        }
    }
}
