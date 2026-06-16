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

function Add-FieldPartSelectionArguments {
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

function Get-OldStyleIdValue {
    param(
        $Operation,
        [string]$Label
    )

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("old_style_id", "old_style", "source_style_id", "source_style", "style_id", "style") `
        -Label $Label
}

function Get-NewStyleIdValue {
    param(
        $Operation,
        [string]$Label
    )

    return Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("new_style_id", "new_style", "target_style_id", "target_style", "replacement_style_id", "replacement_style") `
        -Label $Label
}

function Add-StyleRefactorPairArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [string]$PairPropertyName,
        [string]$Flag,
        [string[]]$SourceNames,
        [string[]]$TargetNames,
        [string]$SourceLabel,
        [string]$TargetLabel
    )

    $pairs = Get-OptionalObjectPropertyObject -Object $Operation -Name $PairPropertyName
    if ($null -ne $pairs) {
        if ($pairs -is [string]) {
            if ([string]::IsNullOrWhiteSpace([string]$pairs)) {
                throw "$Label '$PairPropertyName' strings must not be empty."
            }
            $Arguments.Add($Flag) | Out-Null
            $Arguments.Add([string]$pairs) | Out-Null
            return 1
        }

        $count = 0
        foreach ($pair in @($pairs)) {
            if ($pair -is [string]) {
                if ([string]::IsNullOrWhiteSpace([string]$pair)) {
                    throw "$Label '$PairPropertyName' strings must not be empty."
                }
                $Arguments.Add($Flag) | Out-Null
                $Arguments.Add([string]$pair) | Out-Null
                $count += 1
                continue
            }

            $source = Get-FirstObjectPropertyValue `
                -Object $pair `
                -Names $SourceNames `
                -Label $Label
            $target = Get-FirstObjectPropertyValue `
                -Object $pair `
                -Names $TargetNames `
                -Label $Label
            $Arguments.Add($Flag) | Out-Null
            $Arguments.Add("$source`:$target") | Out-Null
            $count += 1
        }

        return $count
    }

    $source = Get-FirstOptionalObjectPropertyValue -Object $Operation -Names $SourceNames
    $target = Get-FirstOptionalObjectPropertyValue -Object $Operation -Names $TargetNames
    if ([string]::IsNullOrWhiteSpace($source) -and [string]::IsNullOrWhiteSpace($target)) {
        return 0
    }
    if ([string]::IsNullOrWhiteSpace($source) -or [string]::IsNullOrWhiteSpace($target)) {
        throw "$Label must provide both '$SourceLabel' and '$TargetLabel'."
    }

    $Arguments.Add($Flag) | Out-Null
    $Arguments.Add("$source`:$target") | Out-Null
    return 1
}

function Add-StyleRefactorApplyArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $planFile = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("plan_file", "plan_path", "style_refactor_plan_file", "style_refactor_plan_path")
    $requestCount = 0

    if (-not [string]::IsNullOrWhiteSpace($planFile)) {
        $Arguments.Add("--plan-file") | Out-Null
        $Arguments.Add($planFile) | Out-Null
        $requestCount += 1
    }

    $requestCount += Add-StyleRefactorPairArguments `
        -Arguments $Arguments `
        -Operation $Operation `
        -Label $Label `
        -PairPropertyName "renames" `
        -Flag "--rename" `
        -SourceNames @("old_style_id", "old_style", "source_style_id", "source_style", "from_style_id", "from_style", "style_id", "style") `
        -TargetNames @("new_style_id", "new_style", "target_style_id", "target_style", "to_style_id", "to_style", "replacement_style_id", "replacement_style") `
        -SourceLabel "old_style_id" `
        -TargetLabel "new_style_id"
    $requestCount += Add-StyleRefactorPairArguments `
        -Arguments $Arguments `
        -Operation $Operation `
        -Label $Label `
        -PairPropertyName "merges" `
        -Flag "--merge" `
        -SourceNames @("source_style_id", "source_style", "old_style_id", "old_style", "from_style_id", "from_style", "style_id", "style") `
        -TargetNames @("target_style_id", "target_style", "new_style_id", "new_style", "to_style_id", "to_style", "replacement_style_id", "replacement_style") `
        -SourceLabel "source_style_id" `
        -TargetLabel "target_style_id"

    if ($requestCount -eq 0) {
        throw "$Label must provide 'plan_file', 'renames', or 'merges'."
    }

    $rollbackPlan = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("rollback_plan", "rollback_plan_file", "rollback_path", "rollback_plan_path")
    if (-not [string]::IsNullOrWhiteSpace($rollbackPlan)) {
        $Arguments.Add("--rollback-plan") | Out-Null
        $Arguments.Add($rollbackPlan) | Out-Null
    }
}

function Add-StyleMergeRestoreSelectionArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $rollbackPlan = Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("rollback_plan", "rollback_plan_file", "rollback_path", "rollback_plan_path") `
        -Label $Label
    $Arguments.Add("--rollback-plan") | Out-Null
    $Arguments.Add($rollbackPlan) | Out-Null

    foreach ($name in @("entries", "entry_indexes", "entry_index", "entry")) {
        $entries = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $entries) {
            foreach ($entry in @($entries)) {
                $entryText = [string]$entry
                if ([string]::IsNullOrWhiteSpace($entryText)) {
                    throw "$Label '$name' values must not be empty."
                }
                $Arguments.Add("--entry") | Out-Null
                $Arguments.Add($entryText) | Out-Null
            }
            break
        }
    }

    foreach ($name in @("source_style_ids", "source_styles", "source_style_id", "source_style")) {
        $sourceStyles = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $sourceStyles) {
            foreach ($styleId in @($sourceStyles)) {
                $styleText = [string]$styleId
                if ([string]::IsNullOrWhiteSpace($styleText)) {
                    throw "$Label '$name' values must not be empty."
                }
                $Arguments.Add("--source-style") | Out-Null
                $Arguments.Add($styleText) | Out-Null
            }
            break
        }
    }

    foreach ($name in @("target_style_ids", "target_styles", "target_style_id", "target_style")) {
        $targetStyles = Get-OptionalObjectPropertyObject -Object $Operation -Name $name
        if ($null -ne $targetStyles) {
            foreach ($styleId in @($targetStyles)) {
                $styleText = [string]$styleId
                if ([string]::IsNullOrWhiteSpace($styleText)) {
                    throw "$Label '$name' values must not be empty."
                }
                $Arguments.Add("--target-style") | Out-Null
                $Arguments.Add($styleText) | Out-Null
            }
            break
        }
    }

    foreach ($name in @("dry_run", "plan_only")) {
        if ((Test-ObjectPropertyExists -Object $Operation -Name $name) -and
            (Get-OptionalBooleanPropertyValue -Object $Operation -Name $name -DefaultValue $false)) {
            throw "$Label does not support '$name' because edit plans write an output DOCX for each operation."
        }
    }
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
