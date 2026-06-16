function Get-StyleXmlState {
    param(
        [string]$DocxPath,
        [string]$StyleId
    )

    $stylesXml = Read-DocxEntryText -DocxPath $DocxPath -EntryName "word/styles.xml"
    $xmlDocument = New-Object System.Xml.XmlDocument
    $xmlDocument.LoadXml($stylesXml)

    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($xmlDocument.NameTable)
    $namespaceManager.AddNamespace("w", $script:WordMlNamespace)

    $styleNode = $xmlDocument.SelectSingleNode(
        "/w:styles/w:style[@w:styleId='$StyleId']",
        $namespaceManager)
    if ($null -eq $styleNode) {
        throw "Style '$StyleId' was not found in word/styles.xml for $DocxPath."
    }

    $nameNode = $styleNode.SelectSingleNode("w:name", $namespaceManager)
    $basedOnNode = $styleNode.SelectSingleNode("w:basedOn", $namespaceManager)
    $nextNode = $styleNode.SelectSingleNode("w:next", $namespaceManager)
    $qFormatNode = $styleNode.SelectSingleNode("w:qFormat", $namespaceManager)
    $semiHiddenNode = $styleNode.SelectSingleNode("w:semiHidden", $namespaceManager)
    $unhideWhenUsedNode = $styleNode.SelectSingleNode("w:unhideWhenUsed", $namespaceManager)
    $paragraphBidiNode = $styleNode.SelectSingleNode("w:pPr/w:bidi", $namespaceManager)
    $outlineLevelNode = $styleNode.SelectSingleNode("w:pPr/w:outlineLvl", $namespaceManager)
    $fontAsciiNode = $styleNode.SelectSingleNode("w:rPr/w:rFonts", $namespaceManager)
    $langNode = $styleNode.SelectSingleNode("w:rPr/w:lang", $namespaceManager)
    $rtlNode = $styleNode.SelectSingleNode("w:rPr/w:rtl", $namespaceManager)

    $outlineLevel = $null
    if ($null -ne $outlineLevelNode) {
        $outlineLevel = [int]$outlineLevelNode.GetAttribute("val", $script:WordMlNamespace)
    }

    $fontAscii = $null
    if ($null -ne $fontAsciiNode) {
        $value = $fontAsciiNode.GetAttribute("ascii", $script:WordMlNamespace)
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $fontAscii = $value
        }
    }

    $langValue = $null
    $langBidi = $null
    if ($null -ne $langNode) {
        $value = $langNode.GetAttribute("val", $script:WordMlNamespace)
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $langValue = $value
        }
        $bidiValue = $langNode.GetAttribute("bidi", $script:WordMlNamespace)
        if (-not [string]::IsNullOrWhiteSpace($bidiValue)) {
            $langBidi = $bidiValue
        }
    }

    $rtlValue = $null
    if ($null -ne $rtlNode) {
        $value = $rtlNode.GetAttribute("val", $script:WordMlNamespace)
        if ([string]::IsNullOrWhiteSpace($value)) {
            $rtlValue = "1"
        } else {
            $rtlValue = $value
        }
    }

    return [pscustomobject][ordered]@{
        style_id = $StyleId
        type = $styleNode.GetAttribute("type", $script:WordMlNamespace)
        name = if ($null -ne $nameNode) { $nameNode.GetAttribute("val", $script:WordMlNamespace) } else { $null }
        based_on = if ($null -ne $basedOnNode) { $basedOnNode.GetAttribute("val", $script:WordMlNamespace) } else { $null }
        next_style = if ($null -ne $nextNode) { $nextNode.GetAttribute("val", $script:WordMlNamespace) } else { $null }
        has_qformat = ($null -ne $qFormatNode)
        has_semi_hidden = ($null -ne $semiHiddenNode)
        has_unhide_when_used = ($null -ne $unhideWhenUsedNode)
        has_paragraph_bidi = ($null -ne $paragraphBidiNode)
        outline_level = $outlineLevel
        font_ascii = $fontAscii
        lang_val = $langValue
        lang_bidi = $langBidi
        rtl_val = $rtlValue
    }
}

function Assert-StyleXmlState {
    param(
        [object]$State,
        [string]$ExpectedType,
        [string]$ExpectedName,
        [AllowNull()][string]$ExpectedBasedOn,
        [AllowNull()][string]$ExpectedNextStyle,
        [bool]$ExpectedQFormat,
        [bool]$ExpectedSemiHidden,
        [bool]$ExpectedUnhideWhenUsed,
        [bool]$ExpectedParagraphBidi,
        [AllowNull()][int]$ExpectedOutlineLevel,
        [AllowNull()][string]$ExpectedFontAscii,
        [AllowNull()][string]$ExpectedLangValue,
        [AllowNull()][string]$ExpectedLangBidi,
        [AllowNull()][string]$ExpectedRtlValue,
        [string]$Label
    )

    if ([string]$State.type -ne $ExpectedType) {
        throw "$Label expected XML type '$ExpectedType', got '$($State.type)'."
    }
    if ([string]$State.name -ne $ExpectedName) {
        throw "$Label expected XML name '$ExpectedName', got '$($State.name)'."
    }

    if ($null -eq $ExpectedBasedOn) {
        if ($null -ne $State.based_on) {
            throw "$Label expected XML based_on=null, got '$($State.based_on)'."
        }
    } elseif ([string]$State.based_on -ne $ExpectedBasedOn) {
        throw "$Label expected XML based_on '$ExpectedBasedOn', got '$($State.based_on)'."
    }

    if ($null -eq $ExpectedNextStyle) {
        if ($null -ne $State.next_style) {
            throw "$Label expected XML next_style=null, got '$($State.next_style)'."
        }
    } elseif ([string]$State.next_style -ne $ExpectedNextStyle) {
        throw "$Label expected XML next_style '$ExpectedNextStyle', got '$($State.next_style)'."
    }

    if ([bool]$State.has_qformat -ne $ExpectedQFormat) {
        throw "$Label expected XML has_qformat=$ExpectedQFormat, got $($State.has_qformat)."
    }
    if ([bool]$State.has_semi_hidden -ne $ExpectedSemiHidden) {
        throw "$Label expected XML has_semi_hidden=$ExpectedSemiHidden, got $($State.has_semi_hidden)."
    }
    if ([bool]$State.has_unhide_when_used -ne $ExpectedUnhideWhenUsed) {
        throw "$Label expected XML has_unhide_when_used=$ExpectedUnhideWhenUsed, got $($State.has_unhide_when_used)."
    }
    if ([bool]$State.has_paragraph_bidi -ne $ExpectedParagraphBidi) {
        throw "$Label expected XML has_paragraph_bidi=$ExpectedParagraphBidi, got $($State.has_paragraph_bidi)."
    }

    if ($null -eq $ExpectedOutlineLevel) {
        if ($null -ne $State.outline_level) {
            throw "$Label expected XML outline_level=null, got '$($State.outline_level)'."
        }
    } elseif ([int]$State.outline_level -ne $ExpectedOutlineLevel) {
        throw "$Label expected XML outline_level=$ExpectedOutlineLevel, got '$($State.outline_level)'."
    }

    if ($null -eq $ExpectedFontAscii) {
        if ($null -ne $State.font_ascii) {
            throw "$Label expected XML font_ascii=null, got '$($State.font_ascii)'."
        }
    } elseif ([string]$State.font_ascii -ne $ExpectedFontAscii) {
        throw "$Label expected XML font_ascii '$ExpectedFontAscii', got '$($State.font_ascii)'."
    }

    if ($null -eq $ExpectedLangValue) {
        if ($null -ne $State.lang_val) {
            throw "$Label expected XML lang_val=null, got '$($State.lang_val)'."
        }
    } elseif ([string]$State.lang_val -ne $ExpectedLangValue) {
        throw "$Label expected XML lang_val '$ExpectedLangValue', got '$($State.lang_val)'."
    }

    if ($null -eq $ExpectedLangBidi) {
        if ($null -ne $State.lang_bidi) {
            throw "$Label expected XML lang_bidi=null, got '$($State.lang_bidi)'."
        }
    } elseif ([string]$State.lang_bidi -ne $ExpectedLangBidi) {
        throw "$Label expected XML lang_bidi '$ExpectedLangBidi', got '$($State.lang_bidi)'."
    }

    if ($null -eq $ExpectedRtlValue) {
        if ($null -ne $State.rtl_val) {
            throw "$Label expected XML rtl_val=null, got '$($State.rtl_val)'."
        }
    } elseif ([string]$State.rtl_val -ne $ExpectedRtlValue) {
        throw "$Label expected XML rtl_val '$ExpectedRtlValue', got '$($State.rtl_val)'."
    }
}
