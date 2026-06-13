function Assert-ParagraphState {
    param(
        [string]$JsonPath,
        [int]$ExpectedIndex,
        [string]$ExpectedText,
        [AllowNull()][string]$ExpectedStyleId,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $paragraph = $payload.paragraph
    if ($null -eq $paragraph) {
        throw "$Label expected paragraph payload."
    }
    if ([int]$paragraph.index -ne $ExpectedIndex) {
        throw "$Label expected paragraph index $ExpectedIndex, got $($paragraph.index)."
    }
    if ([int]$paragraph.section_index -ne 0) {
        throw "$Label expected section_index 0, got $($paragraph.section_index)."
    }
    if ([string]$paragraph.text -ne $ExpectedText) {
        throw "$Label expected text '$ExpectedText', got '$($paragraph.text)'."
    }
    if ([bool]$paragraph.section_break) {
        throw "$Label expected section_break=false."
    }
    if ($null -ne $paragraph.numbering) {
        throw "$Label expected direct paragraph numbering metadata to stay null."
    }

    if ($null -eq $ExpectedStyleId) {
        if ($null -ne $paragraph.style_id) {
            throw "$Label expected null style_id, got '$($paragraph.style_id)'."
        }
    } elseif ([string]$paragraph.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($paragraph.style_id)'."
    }
}

function Assert-RunState {
    param(
        [string]$JsonPath,
        [int]$ExpectedParagraphIndex,
        [int]$ExpectedRunIndex,
        [string]$ExpectedText,
        [AllowNull()][string]$ExpectedStyleId,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $run = $payload.run
    if ($null -eq $run) {
        throw "$Label expected run payload."
    }
    if ([int]$payload.paragraph_index -ne $ExpectedParagraphIndex) {
        throw "$Label expected paragraph_index $ExpectedParagraphIndex, got $($payload.paragraph_index)."
    }
    if ([int]$run.index -ne $ExpectedRunIndex) {
        throw "$Label expected run index $ExpectedRunIndex, got $($run.index)."
    }
    if ([string]$run.text -ne $ExpectedText) {
        throw "$Label expected run text '$ExpectedText', got '$($run.text)'."
    }

    if ($null -eq $ExpectedStyleId) {
        if ($null -ne $run.style_id) {
            throw "$Label expected null run style_id, got '$($run.style_id)'."
        }
    } elseif ([string]$run.style_id -ne $ExpectedStyleId) {
        throw "$Label expected run style_id '$ExpectedStyleId', got '$($run.style_id)'."
    }
}

function Assert-StyleCatalogState {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [AllowNull()][string]$ExpectedBasedOn,
        [string]$ExpectedKind,
        [string]$ExpectedType,
        [bool]$ExpectedCustom,
        [bool]$ExpectedSemiHidden,
        [bool]$ExpectedUnhideWhenUsed,
        [bool]$ExpectedQuickFormat,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $style = $payload.style
    if ($null -eq $style) {
        throw "$Label expected style payload."
    }
    if ([string]$style.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($style.style_id)'."
    }
    if ([string]$style.name -ne $ExpectedName) {
        throw "$Label expected style name '$ExpectedName', got '$($style.name)'."
    }

    if ($null -eq $ExpectedBasedOn) {
        if ($null -ne $style.based_on) {
            throw "$Label expected based_on=null, got '$($style.based_on)'."
        }
    } elseif ([string]$style.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected based_on '$ExpectedBasedOn', got '$($style.based_on)'."
    }

    if ([string]$style.kind -ne $ExpectedKind) {
        throw "$Label expected kind '$ExpectedKind', got '$($style.kind)'."
    }
    if ([string]$style.type -ne $ExpectedType) {
        throw "$Label expected type '$ExpectedType', got '$($style.type)'."
    }
    if ([bool]$style.is_custom -ne $ExpectedCustom) {
        throw "$Label expected is_custom=$ExpectedCustom, got $($style.is_custom)."
    }
    if ([bool]$style.is_semi_hidden -ne $ExpectedSemiHidden) {
        throw "$Label expected is_semi_hidden=$ExpectedSemiHidden, got $($style.is_semi_hidden)."
    }
    if ([bool]$style.is_unhide_when_used -ne $ExpectedUnhideWhenUsed) {
        throw "$Label expected is_unhide_when_used=$ExpectedUnhideWhenUsed, got $($style.is_unhide_when_used)."
    }
    if ([bool]$style.is_quick_format -ne $ExpectedQuickFormat) {
        throw "$Label expected is_quick_format=$ExpectedQuickFormat, got $($style.is_quick_format)."
    }
    if ($null -ne $style.numbering) {
        throw "$Label expected numbering=null."
    }
}

function Assert-StyleInheritanceState {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedType,
        [string]$ExpectedKind,
        [AllowNull()][string]$ExpectedBasedOn,
        [string[]]$ExpectedInheritanceChain,
        [AllowNull()][string]$ExpectedFontFamilyValue,
        [AllowNull()][string]$ExpectedFontFamilySource,
        [AllowNull()][string]$ExpectedEastAsiaFontFamilyValue,
        [AllowNull()][string]$ExpectedEastAsiaFontFamilySource,
        [AllowNull()][string]$ExpectedLanguageValue,
        [AllowNull()][string]$ExpectedLanguageSource,
        [AllowNull()][string]$ExpectedEastAsiaLanguageValue,
        [AllowNull()][string]$ExpectedEastAsiaLanguageSource,
        [AllowNull()][string]$ExpectedBidiLanguageValue,
        [AllowNull()][string]$ExpectedBidiLanguageSource,
        [AllowNull()][object]$ExpectedRtlValue,
        [AllowNull()][string]$ExpectedRtlSource,
        [AllowNull()][object]$ExpectedParagraphBidiValue,
        [AllowNull()][string]$ExpectedParagraphBidiSource,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($payload.style_id)'."
    }
    if ([string]$payload.type -ne $ExpectedType) {
        throw "$Label expected type '$ExpectedType', got '$($payload.type)'."
    }
    if ([string]$payload.kind -ne $ExpectedKind) {
        throw "$Label expected kind '$ExpectedKind', got '$($payload.kind)'."
    }

    if ($null -eq $ExpectedBasedOn) {
        if ($null -ne $payload.based_on) {
            throw "$Label expected based_on=null, got '$($payload.based_on)'."
        }
    } elseif ([string]$payload.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected based_on '$ExpectedBasedOn', got '$($payload.based_on)'."
    }

    $actualChain = @($payload.inheritance_chain)
    if ($actualChain.Count -ne $ExpectedInheritanceChain.Count) {
        throw "$Label expected inheritance_chain count $($ExpectedInheritanceChain.Count), got $($actualChain.Count)."
    }
    for ($index = 0; $index -lt $ExpectedInheritanceChain.Count; $index++) {
        if ([string]$actualChain[$index] -ne [string]$ExpectedInheritanceChain[$index]) {
            throw "$Label expected inheritance_chain[$index] '$($ExpectedInheritanceChain[$index])', got '$($actualChain[$index])'."
        }
    }

    $resolved = $payload.resolved_properties
    if ($null -eq $resolved) {
        throw "$Label expected resolved_properties payload."
    }

    $assertStringProperty = {
        param(
            [object]$Property,
            [AllowNull()][string]$ExpectedValue,
            [AllowNull()][string]$ExpectedSource,
            [string]$PropertyLabel
        )

        if ($null -eq $ExpectedValue) {
            if ($null -ne $Property.value) {
                throw "$Label expected $PropertyLabel.value=null, got '$($Property.value)'."
            }
        } elseif ([string]$Property.value -ne $ExpectedValue) {
            throw "$Label expected $PropertyLabel.value '$ExpectedValue', got '$($Property.value)'."
        }

        if ($null -eq $ExpectedSource) {
            if ($null -ne $Property.source_style_id) {
                throw "$Label expected $PropertyLabel.source_style_id=null, got '$($Property.source_style_id)'."
            }
        } elseif ([string]$Property.source_style_id -ne $ExpectedSource) {
            throw "$Label expected $PropertyLabel.source_style_id '$ExpectedSource', got '$($Property.source_style_id)'."
        }
    }

    $assertBoolProperty = {
        param(
            [object]$Property,
            [AllowNull()][object]$ExpectedValue,
            [AllowNull()][string]$ExpectedSource,
            [string]$PropertyLabel
        )

        if ($null -eq $ExpectedValue) {
            if ($null -ne $Property.value) {
                throw "$Label expected $PropertyLabel.value=null, got '$($Property.value)'."
            }
        } elseif ([bool]$Property.value -ne $ExpectedValue) {
            throw "$Label expected $PropertyLabel.value=$ExpectedValue, got $($Property.value)."
        }

        if ($null -eq $ExpectedSource) {
            if ($null -ne $Property.source_style_id) {
                throw "$Label expected $PropertyLabel.source_style_id=null, got '$($Property.source_style_id)'."
            }
        } elseif ([string]$Property.source_style_id -ne $ExpectedSource) {
            throw "$Label expected $PropertyLabel.source_style_id '$ExpectedSource', got '$($Property.source_style_id)'."
        }
    }

    & $assertStringProperty $resolved.font_family $ExpectedFontFamilyValue $ExpectedFontFamilySource "font_family"
    & $assertStringProperty $resolved.east_asia_font_family $ExpectedEastAsiaFontFamilyValue $ExpectedEastAsiaFontFamilySource "east_asia_font_family"
    & $assertStringProperty $resolved.language $ExpectedLanguageValue $ExpectedLanguageSource "language"
    & $assertStringProperty $resolved.east_asia_language $ExpectedEastAsiaLanguageValue $ExpectedEastAsiaLanguageSource "east_asia_language"
    & $assertStringProperty $resolved.bidi_language $ExpectedBidiLanguageValue $ExpectedBidiLanguageSource "bidi_language"
    & $assertBoolProperty $resolved.rtl $ExpectedRtlValue $ExpectedRtlSource "rtl"
    & $assertBoolProperty $resolved.paragraph_bidi $ExpectedParagraphBidiValue $ExpectedParagraphBidiSource "paragraph_bidi"
}

function Assert-EnsureParagraphStyleResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [string]$ExpectedBasedOn,
        [bool]$ExpectedUnhideWhenUsed,
        [bool]$ExpectedQuickFormat,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "ensure-paragraph-style") {
        throw "$Label expected command ensure-paragraph-style, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }

    $style = $payload.style
    if ($null -eq $style) {
        throw "$Label expected nested style payload."
    }
    if ([string]$style.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($style.style_id)'."
    }
    if ([string]$style.name -ne $ExpectedName) {
        throw "$Label expected style name '$ExpectedName', got '$($style.name)'."
    }
    if ([string]$style.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected based_on '$ExpectedBasedOn', got '$($style.based_on)'."
    }
    if ([bool]$style.is_unhide_when_used -ne $ExpectedUnhideWhenUsed) {
        throw "$Label expected is_unhide_when_used=$ExpectedUnhideWhenUsed, got $($style.is_unhide_when_used)."
    }
    if ([bool]$style.is_quick_format -ne $ExpectedQuickFormat) {
        throw "$Label expected is_quick_format=$ExpectedQuickFormat, got $($style.is_quick_format)."
    }
}

function Assert-EnsureCharacterStyleResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [string]$ExpectedBasedOn,
        [bool]$ExpectedQuickFormat,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "ensure-character-style") {
        throw "$Label expected command ensure-character-style, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }

    $style = $payload.style
    if ($null -eq $style) {
        throw "$Label expected nested style payload."
    }
    if ([string]$style.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($style.style_id)'."
    }
    if ([string]$style.name -ne $ExpectedName) {
        throw "$Label expected style name '$ExpectedName', got '$($style.name)'."
    }
    if ([string]$style.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected based_on '$ExpectedBasedOn', got '$($style.based_on)'."
    }
    if ([bool]$style.is_quick_format -ne $ExpectedQuickFormat) {
        throw "$Label expected is_quick_format=$ExpectedQuickFormat, got $($style.is_quick_format)."
    }
}

function Assert-EnsureTableStyleResult {
    param(
        [string]$JsonPath,
        [string]$ExpectedStyleId,
        [string]$ExpectedName,
        [string]$ExpectedBasedOn,
        [bool]$ExpectedUnhideWhenUsed,
        [bool]$ExpectedQuickFormat,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    if ([string]$payload.command -ne "ensure-table-style") {
        throw "$Label expected command ensure-table-style, got '$($payload.command)'."
    }
    if (-not [bool]$payload.ok) {
        throw "$Label expected ok=true."
    }

    $style = $payload.style
    if ($null -eq $style) {
        throw "$Label expected nested style payload."
    }
    if ([string]$style.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($style.style_id)'."
    }
    if ([string]$style.name -ne $ExpectedName) {
        throw "$Label expected style name '$ExpectedName', got '$($style.name)'."
    }
    if ([string]$style.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected based_on '$ExpectedBasedOn', got '$($style.based_on)'."
    }
    if ([bool]$style.is_unhide_when_used -ne $ExpectedUnhideWhenUsed) {
        throw "$Label expected is_unhide_when_used=$ExpectedUnhideWhenUsed, got $($style.is_unhide_when_used)."
    }
    if ([bool]$style.is_quick_format -ne $ExpectedQuickFormat) {
        throw "$Label expected is_quick_format=$ExpectedQuickFormat, got $($style.is_quick_format)."
    }
}

function Assert-TableState {
    param(
        [string]$JsonPath,
        [int]$ExpectedIndex,
        [string]$ExpectedStyleId,
        [int]$ExpectedRowCount,
        [int]$ExpectedColumnCount,
        [string]$Label
    )

    $payload = Read-JsonFile -Path $JsonPath
    $table = $payload.table
    if ($null -eq $table) {
        throw "$Label expected table payload."
    }
    if ([int]$table.index -ne $ExpectedIndex) {
        throw "$Label expected table index $ExpectedIndex, got $($table.index)."
    }
    if ([string]$table.style_id -ne $ExpectedStyleId) {
        throw "$Label expected style_id '$ExpectedStyleId', got '$($table.style_id)'."
    }
    if ([int]$table.row_count -ne $ExpectedRowCount) {
        throw "$Label expected row_count $ExpectedRowCount, got $($table.row_count)."
    }
    if ([int]$table.column_count -ne $ExpectedColumnCount) {
        throw "$Label expected column_count $ExpectedColumnCount, got $($table.column_count)."
    }
}
