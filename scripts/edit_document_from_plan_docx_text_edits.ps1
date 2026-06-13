function Remove-DocxBodyParagraphs {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$ParagraphIndex,
        [string]$TextContains
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")

        $targets = @(Get-BodyParagraphAlignmentTargets `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -ParagraphIndex $ParagraphIndex `
                -TextContains $TextContains)

        foreach ($paragraph in @($targets)) {
            $parent = $paragraph.ParentNode
            if ($null -ne $parent) {
                $parent.RemoveChild($paragraph) | Out-Null
            }
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            paragraph_count = $targets.Count
            paragraph_index = $ParagraphIndex
            text_contains = $TextContains
        }
    } finally {
        $archive.Dispose()
    }
}

function New-DirectParagraphDeletionOperation {
    param(
        $Operation,
        [string]$NormalizedOp
    )

    $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_index"
    if ([string]::IsNullOrWhiteSpace($paragraphIndex)) {
        $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "index"
    }

    $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "text_contains"
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_text_contains"
    }
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "contains"
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = "delete-paragraph"
        DirectOperation = $true
        DirectOperationKind = "body-paragraph-deletion"
        ParagraphIndex = $paragraphIndex
        TextContains = $textContains
    }
}

function Set-ParagraphPlainTextContent {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Paragraph,
        [string]$Text
    )

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $xmlNamespace = "http://www.w3.org/XML/1998/namespace"
    $paragraphProperties = $Paragraph.SelectSingleNode("w:pPr", $NamespaceManager)
    $firstRunProperties = $Paragraph.SelectSingleNode("w:r/w:rPr", $NamespaceManager)

    foreach ($child in @($Paragraph.ChildNodes)) {
        if ($null -ne $paragraphProperties -and [object]::ReferenceEquals($child, $paragraphProperties)) {
            continue
        }
        $Paragraph.RemoveChild($child) | Out-Null
    }

    $run = $Document.CreateElement("w", "r", $wordNamespace)
    if ($null -ne $firstRunProperties) {
        $run.AppendChild($firstRunProperties.CloneNode($true)) | Out-Null
    }

    $textNode = $Document.CreateElement("w", "t", $wordNamespace)
    if ($Text.Length -gt 0 -and
        ($Text[0] -match '\s' -or $Text[$Text.Length - 1] -match '\s' -or $Text.Contains("  "))) {
        $spaceAttribute = $Document.CreateAttribute("xml", "space", $xmlNamespace)
        $spaceAttribute.Value = "preserve"
        $textNode.Attributes.SetNamedItem($spaceAttribute) | Out-Null
    }
    $textNode.InnerText = $Text
    $run.AppendChild($textNode) | Out-Null
    $Paragraph.AppendChild($run) | Out-Null
}

function Set-DocxTextReplacement {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$FindText,
        [string]$ReplacementText,
        [bool]$MatchCase
    )

    if ([string]::IsNullOrEmpty($FindText)) {
        throw "Text replacement must provide a non-empty find text."
    }

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")

        $regexOptions = [System.Text.RegularExpressions.RegexOptions]::CultureInvariant
        if (-not $MatchCase) {
            $regexOptions = $regexOptions -bor [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
        }
        $pattern = [System.Text.RegularExpressions.Regex]::Escape($FindText)
        $changedParagraphCount = 0
        $replacementCount = 0

        foreach ($paragraph in @($document.SelectNodes("//w:p", $namespaceManager))) {
            $paragraphText = Get-ParagraphTextContent `
                -Paragraph $paragraph `
                -NamespaceManager $namespaceManager
            $matches = [System.Text.RegularExpressions.Regex]::Matches($paragraphText, $pattern, $regexOptions)
            if ($matches.Count -eq 0) {
                continue
            }

            $newText = [System.Text.RegularExpressions.Regex]::Replace(
                $paragraphText,
                $pattern,
                [System.Text.RegularExpressions.MatchEvaluator]{ param($match) $ReplacementText },
                $regexOptions
            )
            Set-ParagraphPlainTextContent `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -Paragraph $paragraph `
                -Text $newText
            $changedParagraphCount += 1
            $replacementCount += $matches.Count
        }

        if ($replacementCount -eq 0) {
            throw "Text '$FindText' was not found in word/document.xml."
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            find_text = $FindText
            replacement_text = $ReplacementText
            match_case = $MatchCase
            paragraph_count = $changedParagraphCount
            replacement_count = $replacementCount
        }
    } finally {
        $archive.Dispose()
    }
}

function New-DirectTextReplacementOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label
    )

    $findText = Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("find", "search", "old_text") `
        -Label $Label
    $replacementText = Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("replace", "replacement", "new_text", "text") `
        -Label $Label
    $matchCase = Get-OptionalBooleanPropertyValue `
        -Object $Operation `
        -Name "match_case" `
        -DefaultValue $true

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = "replace-text"
        DirectOperation = $true
        DirectOperationKind = "text-replacement"
        FindText = $findText
        ReplacementText = $replacementText
        MatchCase = $matchCase
    }
}


function Get-ParagraphTextStyleTargets {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$TableIndex,
        [string]$RowIndex,
        [string]$CellIndex,
        [string]$ParagraphIndex,
        [string]$TextContains
    )

    $hasTableLocator = @(@($TableIndex, $RowIndex, $CellIndex) | Where-Object {
            -not [string]::IsNullOrWhiteSpace($_)
        })
    if ($hasTableLocator.Count -gt 0) {
        if ([string]::IsNullOrWhiteSpace($TableIndex) -or
            [string]::IsNullOrWhiteSpace($RowIndex) -or
            [string]::IsNullOrWhiteSpace($CellIndex)) {
            throw "Text style table-cell targeting must provide table_index, row_index, and cell_index together."
        }

        $tableIndexValue = [int]$TableIndex
        $rowIndexValue = [int]$RowIndex
        $cellIndexValue = [int]$CellIndex
        $tables = @($Document.SelectNodes("/w:document/w:body/w:tbl", $NamespaceManager))
        if ($tableIndexValue -lt 0 -or $tableIndexValue -ge $tables.Count) {
            throw "table_index '$TableIndex' is out of range."
        }
        $rows = @($tables[$tableIndexValue].SelectNodes("w:tr", $NamespaceManager))
        if ($rowIndexValue -lt 0 -or $rowIndexValue -ge $rows.Count) {
            throw "row_index '$RowIndex' is out of range."
        }
        $cells = @($rows[$rowIndexValue].SelectNodes("w:tc", $NamespaceManager))
        if ($cellIndexValue -lt 0 -or $cellIndexValue -ge $cells.Count) {
            throw "cell_index '$CellIndex' is out of range."
        }

        $paragraphs = @($cells[$cellIndexValue].SelectNodes("w:p", $NamespaceManager))
        if ($paragraphs.Count -eq 0) {
            throw "target table cell does not contain direct paragraphs."
        }
        if (-not [string]::IsNullOrWhiteSpace($ParagraphIndex)) {
            $paragraphIndexValue = [int]$ParagraphIndex
            if ($paragraphIndexValue -lt 0 -or $paragraphIndexValue -ge $paragraphs.Count) {
                throw "paragraph_index '$ParagraphIndex' is out of range in target table cell."
            }
            $paragraphs = @($paragraphs[$paragraphIndexValue])
        }
        if (-not [string]::IsNullOrWhiteSpace($TextContains)) {
            $paragraphs = @($paragraphs | Where-Object {
                    (Get-ParagraphTextContent -Paragraph $_ -NamespaceManager $NamespaceManager).Contains($TextContains)
                })
            if ($paragraphs.Count -eq 0) {
                throw "No target table-cell paragraph contains '$TextContains'."
            }
        }
        return $paragraphs
    }

    if (-not [string]::IsNullOrWhiteSpace($ParagraphIndex)) {
        $paragraphs = @($Document.SelectNodes("/w:document/w:body/w:p", $NamespaceManager))
        $paragraphIndexValue = [int]$ParagraphIndex
        if ($paragraphIndexValue -lt 0 -or $paragraphIndexValue -ge $paragraphs.Count) {
            throw "paragraph_index '$ParagraphIndex' is out of range."
        }
        return @($paragraphs[$paragraphIndexValue])
    }

    if (-not [string]::IsNullOrWhiteSpace($TextContains)) {
        $matches = @($Document.SelectNodes("//w:p", $NamespaceManager) | Where-Object {
                (Get-ParagraphTextContent -Paragraph $_ -NamespaceManager $NamespaceManager).Contains($TextContains)
            })
        if ($matches.Count -eq 0) {
            throw "No paragraph contains '$TextContains'."
        }
        return $matches
    }

    throw "Text style operation must provide 'text_contains', 'paragraph_index', or table cell indexes."
}

function Get-OrCreateChildElement {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlElement]$Parent,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$LocalName
    )

    $existing = $Parent.SelectSingleNode("w:$LocalName", $NamespaceManager)
    if ($null -ne $existing) {
        return $existing
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $child = $Document.CreateElement("w", $LocalName, $wordNamespace)
    $Parent.AppendChild($child) | Out-Null
    return $child
}

function Get-OrCreateRunProperties {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Run
    )

    $runProperties = $Run.SelectSingleNode("w:rPr", $NamespaceManager)
    if ($null -ne $runProperties) {
        return $runProperties
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $runProperties = $Document.CreateElement("w", "rPr", $wordNamespace)
    if ($Run.HasChildNodes) {
        $Run.InsertBefore($runProperties, $Run.FirstChild) | Out-Null
    } else {
        $Run.AppendChild($runProperties) | Out-Null
    }
    return $runProperties
}

function Set-WordAttribute {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlElement]$Element,
        [string]$Name,
        [string]$Value
    )

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $attribute = $Document.CreateAttribute("w", $Name, $wordNamespace)
    $attribute.Value = $Value
    $Element.Attributes.SetNamedItem($attribute) | Out-Null
}

function Normalize-TextStyleColor {
    param(
        [string]$Color,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Color)) {
        return ""
    }

    $normalizedColor = $Color.Trim()
    if ($normalizedColor.StartsWith("#")) {
        $normalizedColor = $normalizedColor.Substring(1)
    }
    if ($normalizedColor -eq "auto") {
        return $normalizedColor
    }
    if ($normalizedColor -notmatch "^[0-9A-Fa-f]{6}$") {
        throw "$Label text color must be 'auto' or a 6-digit RGB hex value."
    }

    return $normalizedColor.ToUpperInvariant()
}

function Convert-TextStyleFontSizeToHalfPoints {
    param(
        [string]$FontSize,
        [string]$FontSizeHalfPoints,
        [string]$Label
    )

    if (-not [string]::IsNullOrWhiteSpace($FontSizeHalfPoints)) {
        $halfPoints = [int]$FontSizeHalfPoints
        if ($halfPoints -le 0) {
            throw "$Label font_size_half_points must be greater than zero."
        }
        return [string]$halfPoints
    }

    if ([string]::IsNullOrWhiteSpace($FontSize)) {
        return ""
    }

    $culture = [System.Globalization.CultureInfo]::InvariantCulture
    $points = [double]::Parse($FontSize, $culture)
    if ($points -le 0) {
        throw "$Label font_size_points must be greater than zero."
    }

    return [string]([int][System.Math]::Round(
            $points * 2,
            [System.MidpointRounding]::AwayFromZero))
}

function Set-RunTextStyle {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Run,
        [bool]$BoldSpecified,
        [bool]$Bold,
        [string]$FontFamily,
        [string]$EastAsiaFontFamily,
        [string]$Language,
        [string]$EastAsiaLanguage,
        [string]$Color,
        [string]$FontSizeHalfPoints
    )

    $runProperties = Get-OrCreateRunProperties `
        -Document $Document `
        -NamespaceManager $NamespaceManager `
        -Run $Run

    if ($BoldSpecified) {
        $boldNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "b"
        $boldValue = if ($Bold) { "1" } else { "0" }
        Set-WordAttribute `
            -Document $Document `
            -Element $boldNode `
            -Name "val" `
            -Value $boldValue
    }

    if (-not [string]::IsNullOrWhiteSpace($FontFamily) -or
        -not [string]::IsNullOrWhiteSpace($EastAsiaFontFamily)) {
        $fontsNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "rFonts"
        if (-not [string]::IsNullOrWhiteSpace($FontFamily)) {
            Set-WordAttribute -Document $Document -Element $fontsNode -Name "ascii" -Value $FontFamily
            Set-WordAttribute -Document $Document -Element $fontsNode -Name "hAnsi" -Value $FontFamily
        }
        if (-not [string]::IsNullOrWhiteSpace($EastAsiaFontFamily)) {
            Set-WordAttribute -Document $Document -Element $fontsNode -Name "eastAsia" -Value $EastAsiaFontFamily
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($Language) -or
        -not [string]::IsNullOrWhiteSpace($EastAsiaLanguage)) {
        $languageNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "lang"
        if (-not [string]::IsNullOrWhiteSpace($Language)) {
            Set-WordAttribute -Document $Document -Element $languageNode -Name "val" -Value $Language
        }
        if (-not [string]::IsNullOrWhiteSpace($EastAsiaLanguage)) {
            Set-WordAttribute -Document $Document -Element $languageNode -Name "eastAsia" -Value $EastAsiaLanguage
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($Color)) {
        $colorNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "color"
        Set-WordAttribute -Document $Document -Element $colorNode -Name "val" -Value $Color
    }

    if (-not [string]::IsNullOrWhiteSpace($FontSizeHalfPoints)) {
        $sizeNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "sz"
        Set-WordAttribute -Document $Document -Element $sizeNode -Name "val" -Value $FontSizeHalfPoints

        $complexScriptSizeNode = Get-OrCreateChildElement `
            -Document $Document `
            -Parent $runProperties `
            -NamespaceManager $NamespaceManager `
            -LocalName "szCs"
        Set-WordAttribute -Document $Document -Element $complexScriptSizeNode -Name "val" -Value $FontSizeHalfPoints
    }
}

function Set-DocxParagraphTextStyle {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$TableIndex,
        [string]$RowIndex,
        [string]$CellIndex,
        [string]$ParagraphIndex,
        [string]$TextContains,
        [bool]$BoldSpecified,
        [bool]$Bold,
        [string]$FontFamily,
        [string]$EastAsiaFontFamily,
        [string]$Language,
        [string]$EastAsiaLanguage,
        [string]$Color,
        [string]$FontSizeHalfPoints
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::Open($outputFullPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $documentXml = Read-DocxZipEntryText -Archive $archive -EntryName "word/document.xml"
        $document = New-Object System.Xml.XmlDocument
        $document.PreserveWhitespace = $true
        $document.LoadXml($documentXml)

        $namespaceManager = New-Object System.Xml.XmlNamespaceManager($document.NameTable)
        $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")

        $targets = @(Get-ParagraphTextStyleTargets `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -TableIndex $TableIndex `
                -RowIndex $RowIndex `
                -CellIndex $CellIndex `
                -ParagraphIndex $ParagraphIndex `
                -TextContains $TextContains)

        $runCount = 0
        foreach ($paragraph in $targets) {
            foreach ($run in @($paragraph.SelectNodes("w:r", $namespaceManager))) {
                Set-RunTextStyle `
                    -Document $document `
                    -NamespaceManager $namespaceManager `
                    -Run $run `
                    -BoldSpecified $BoldSpecified `
                    -Bold $Bold `
                    -FontFamily $FontFamily `
                    -EastAsiaFontFamily $EastAsiaFontFamily `
                    -Language $Language `
                    -EastAsiaLanguage $EastAsiaLanguage `
                    -Color $Color `
                    -FontSizeHalfPoints $FontSizeHalfPoints
                $runCount += 1
            }
        }

        if ($runCount -eq 0) {
            throw "Target paragraph does not contain runs to style."
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            paragraph_count = $targets.Count
            run_count = $runCount
            table_index = $TableIndex
            row_index = $RowIndex
            cell_index = $CellIndex
            paragraph_index = $ParagraphIndex
            text_contains = $TextContains
            color = $Color
            font_size_half_points = $FontSizeHalfPoints
        }
    } finally {
        $archive.Dispose()
    }
}

function New-DirectTextStyleOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label
    )

    $boldObject = Get-OptionalObjectPropertyObject -Object $Operation -Name "bold"
    $boldSpecified = $null -ne $boldObject
    $bold = if ($boldSpecified) {
        Get-OptionalBooleanPropertyValue -Object $Operation -Name "bold" -DefaultValue $false
    } else {
        $false
    }
    $fontFamily = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("font_family", "font", "latin_font_family")
    $eastAsiaFontFamily = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @(
            "east_asia_font_family",
            "east_asia_font",
            "chinese_font_family",
            "cjk_font_family")
    $language = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("language", "lang", "primary_language")
    $eastAsiaLanguage = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @(
            "east_asia_language",
            "east_asia_lang",
            "chinese_language",
            "cjk_language")
    $color = Normalize-TextStyleColor `
        -Color (Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("color", "text_color", "font_color")) `
        -Label $Label
    $fontSizeHalfPoints = Convert-TextStyleFontSizeToHalfPoints `
        -FontSize (Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("font_size_points", "font_size", "size_points", "font_size_pt")) `
        -FontSizeHalfPoints (Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("font_size_half_points", "size_half_points")) `
        -Label $Label
    if ((-not $boldSpecified) -and
        [string]::IsNullOrWhiteSpace($fontFamily) -and
        [string]::IsNullOrWhiteSpace($eastAsiaFontFamily) -and
        [string]::IsNullOrWhiteSpace($language) -and
        [string]::IsNullOrWhiteSpace($eastAsiaLanguage) -and
        [string]::IsNullOrWhiteSpace($color) -and
        [string]::IsNullOrWhiteSpace($fontSizeHalfPoints)) {
        throw "$Label must provide at least one text style property."
    }

    $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_index"
    if ([string]::IsNullOrWhiteSpace($paragraphIndex)) {
        $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "index"
    }
    $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "text_contains"
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "contains"
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = "set-text-style"
        DirectOperation = $true
        DirectOperationKind = "text-style"
        TableIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "table_index"
        RowIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "row_index"
        CellIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "cell_index"
        ParagraphIndex = $paragraphIndex
        TextContains = $textContains
        BoldSpecified = $boldSpecified
        Bold = $bold
        FontFamily = $fontFamily
        EastAsiaFontFamily = $eastAsiaFontFamily
        Language = $language
        EastAsiaLanguage = $eastAsiaLanguage
        Color = $color
        FontSizeHalfPoints = $fontSizeHalfPoints
    }
}
