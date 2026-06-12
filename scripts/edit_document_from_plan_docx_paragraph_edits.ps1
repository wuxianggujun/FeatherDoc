function Normalize-ParagraphSpacingLineRule {
    param(
        [string]$LineRule,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($LineRule)) {
        return ""
    }

    $normalized = $LineRule.Trim().ToLowerInvariant()
    switch ($normalized) {
        "auto" { return "auto" }
        "exact" { return "exact" }
        "atleast" { return "atLeast" }
        "at_least" { return "atLeast" }
        "at-least" { return "atLeast" }
        default { throw "$Label line_rule must be auto, exact, or at_least." }
    }
}

function Set-ParagraphSpacingNode {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Paragraph,
        [string]$BeforeTwips,
        [string]$AfterTwips,
        [string]$LineTwips,
        [string]$LineRule,
        [bool]$Clear
    )

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $paragraphProperties = $Paragraph.SelectSingleNode("w:pPr", $NamespaceManager)
    if ($null -eq $paragraphProperties) {
        if ($Clear) {
            return
        }

        $paragraphProperties = $Document.CreateElement("w", "pPr", $wordNamespace)
        if ($Paragraph.HasChildNodes) {
            $Paragraph.InsertBefore($paragraphProperties, $Paragraph.FirstChild) | Out-Null
        } else {
            $Paragraph.AppendChild($paragraphProperties) | Out-Null
        }
    }

    $spacingNode = $paragraphProperties.SelectSingleNode("w:spacing", $NamespaceManager)
    if ($Clear) {
        if ($null -ne $spacingNode) {
            $paragraphProperties.RemoveChild($spacingNode) | Out-Null
        }
        if ($paragraphProperties.ChildNodes.Count -eq 0 -and
            $paragraphProperties.Attributes.Count -eq 0) {
            $Paragraph.RemoveChild($paragraphProperties) | Out-Null
        }
        return
    }

    if ($null -eq $spacingNode) {
        $spacingNode = $Document.CreateElement("w", "spacing", $wordNamespace)
        $paragraphProperties.AppendChild($spacingNode) | Out-Null
    }

    if (-not [string]::IsNullOrWhiteSpace($BeforeTwips)) {
        Set-WordAttribute -Document $Document -Element $spacingNode -Name "before" -Value $BeforeTwips
    }
    if (-not [string]::IsNullOrWhiteSpace($AfterTwips)) {
        Set-WordAttribute -Document $Document -Element $spacingNode -Name "after" -Value $AfterTwips
    }
    if (-not [string]::IsNullOrWhiteSpace($LineTwips)) {
        Set-WordAttribute -Document $Document -Element $spacingNode -Name "line" -Value $LineTwips
    }
    if (-not [string]::IsNullOrWhiteSpace($LineRule)) {
        Set-WordAttribute -Document $Document -Element $spacingNode -Name "lineRule" -Value $LineRule
    }
}

function Set-DocxBodyParagraphSpacing {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$ParagraphIndex,
        [string]$TextContains,
        [string]$BeforeTwips,
        [string]$AfterTwips,
        [string]$LineTwips,
        [string]$LineRule,
        [bool]$Clear
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
        $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
        $null = $namespaceManager.AddNamespace("w", $wordNamespace)

        $targets = @(Get-BodyParagraphAlignmentTargets `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -ParagraphIndex $ParagraphIndex `
                -TextContains $TextContains)

        foreach ($paragraph in $targets) {
            Set-ParagraphSpacingNode `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -Paragraph $paragraph `
                -BeforeTwips $BeforeTwips `
                -AfterTwips $AfterTwips `
                -LineTwips $LineTwips `
                -LineRule $LineRule `
                -Clear $Clear
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            paragraph_count = $targets.Count
            paragraph_index = $ParagraphIndex
            text_contains = $TextContains
            before_twips = $BeforeTwips
            after_twips = $AfterTwips
            line_twips = $LineTwips
            line_rule = $LineRule
            clear = $Clear
        }
    } finally {
        $archive.Dispose()
    }
}

function New-DirectParagraphHorizontalAlignmentOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label,
        [bool]$Clear
    )

    $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_index"
    if ([string]::IsNullOrWhiteSpace($paragraphIndex)) {
        $paragraphIndex = Get-OptionalObjectPropertyValue -Object $Operation -Name "index"
    }

    $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "text_contains"
    if ([string]::IsNullOrWhiteSpace($textContains)) {
        $textContains = Get-OptionalObjectPropertyValue -Object $Operation -Name "paragraph_text_contains"
    }

    $alignment = ""
    if (-not $Clear) {
        $alignment = Normalize-TableCellHorizontalAlignment `
            -Alignment (Get-OperationAlignmentProperty `
                -Operation $Operation `
                -Label $Label `
                -FallbackName "horizontal_alignment") `
            -Label $Label
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = if ($Clear) { "clear-paragraph-horizontal-alignment" } else { "set-paragraph-horizontal-alignment" }
        DirectOperation = $true
        DirectOperationKind = "body-paragraph-horizontal-alignment"
        ParagraphIndex = $paragraphIndex
        TextContains = $textContains
        HorizontalAlignment = $alignment
        ClearHorizontalAlignment = $Clear
    }
}

function New-DirectParagraphSpacingOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label,
        [bool]$Clear
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

    $beforeTwips = ""
    $afterTwips = ""
    $lineTwips = ""
    $lineRule = ""
    if (-not $Clear) {
        $beforeTwips = Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("before_twips", "space_before_twips", "spacing_before_twips")
        $afterTwips = Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("after_twips", "space_after_twips", "spacing_after_twips")
        $lineTwips = Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("line_twips", "line_spacing_twips", "spacing_line_twips")
        $lineRule = Normalize-ParagraphSpacingLineRule `
            -LineRule (Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("line_rule", "line_spacing_rule", "rule")) `
            -Label $Label

        if ([string]::IsNullOrWhiteSpace($beforeTwips) -and
            [string]::IsNullOrWhiteSpace($afterTwips) -and
            [string]::IsNullOrWhiteSpace($lineTwips) -and
            [string]::IsNullOrWhiteSpace($lineRule)) {
            throw "$Label must provide at least one paragraph spacing property."
        }
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = if ($Clear) { "clear-paragraph-spacing" } else { "set-paragraph-spacing" }
        DirectOperation = $true
        DirectOperationKind = "body-paragraph-spacing"
        ParagraphIndex = $paragraphIndex
        TextContains = $textContains
        BeforeTwips = $beforeTwips
        AfterTwips = $afterTwips
        LineTwips = $lineTwips
        LineRule = $lineRule
        ClearSpacing = $Clear
    }
}


function New-DirectTableColumnWidthOperation {
    param(
        $Operation,
        [string]$NormalizedOp,
        [string]$Label,
        [bool]$Clear
    )

    $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
    $columnIndex = Get-FirstObjectPropertyValue `
        -Object $Operation `
        -Names @("column_index", "col_index", "column") `
        -Label $Label

    $widthTwips = ""
    if (-not $Clear) {
        $widthTwips = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("width_twips", "column_width_twips", "width") `
            -Label $Label
        $widthValue = [int]$widthTwips
        if ($widthValue -le 0) {
            throw "$Label width_twips must be greater than zero."
        }
        $widthTwips = [string]$widthValue
    }

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @()
        Command = if ($Clear) { "clear-table-column-width" } else { "set-table-column-width" }
        DirectOperation = $true
        DirectOperationKind = "body-table-column-width"
        TableIndex = $tableIndex
        ColumnIndex = $columnIndex
        WidthTwips = $widthTwips
        ClearColumnWidth = $Clear
    }
}

function Get-OptionalBooleanPropertyValue {
    param(
        $Object,
        [string]$Name,
        [bool]$DefaultValue
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return $DefaultValue
    }
    if ($value -is [bool]) {
        return $value
    }

    $text = ([string]$value).Trim().ToLowerInvariant()
    switch ($text) {
        "true" { return $true }
        "1" { return $true }
        "yes" { return $true }
        "false" { return $false }
        "0" { return $false }
        "no" { return $false }
        default { throw "Property '$Name' must be a boolean value." }
    }
}

function Get-FirstObjectPropertyValue {
    param(
        $Object,
        [string[]]$Names,
        [string]$Label
    )

    foreach ($name in $Names) {
        $value = Get-OptionalObjectPropertyValue -Object $Object -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    throw "$Label must provide one of: $($Names -join ', ')."
}

function Get-FirstOptionalObjectPropertyValue {
    param(
        $Object,
        [string[]]$Names
    )

    foreach ($name in $Names) {
        $value = Get-OptionalObjectPropertyValue -Object $Object -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    return ""
}

function ConvertTo-NativeCliQuotedArgument {
    param([string]$Value)

    return $Value.Replace('"', '\"')
}

function Add-RevisionAuthoringArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [switch]$RequireText,
        [switch]$AllowText,
        [switch]$AllowExpectedText
    )

    if ($RequireText -or $AllowText) {
        $text = if ($RequireText) {
            Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("text", "revision_text") `
                -Label $Label
        } else {
            Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("text", "revision_text")
        }
        if (-not [string]::IsNullOrWhiteSpace($text)) {
            $Arguments.Add("--text") | Out-Null
            $Arguments.Add($text) | Out-Null
        }
    } elseif ((Test-ObjectPropertyExists -Object $Operation -Name "text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "revision_text")) {
        throw "$Label does not accept 'text' or 'revision_text'."
    }

    $author = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("author", "revision_author")
    if (-not [string]::IsNullOrWhiteSpace($author)) {
        $Arguments.Add("--author") | Out-Null
        $Arguments.Add($author) | Out-Null
    }

    $date = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("date", "revision_date")
    if (-not [string]::IsNullOrWhiteSpace($date)) {
        $Arguments.Add("--date") | Out-Null
        $Arguments.Add($date) | Out-Null
    }

    if ($AllowExpectedText) {
        $expectedText = Get-FirstOptionalObjectPropertyValue `
            -Object $Operation `
            -Names @("expected_text", "expected")
        if (-not [string]::IsNullOrWhiteSpace($expectedText)) {
            $Arguments.Add("--expected-text") | Out-Null
            $Arguments.Add($expectedText) | Out-Null
        }
    } elseif ((Test-ObjectPropertyExists -Object $Operation -Name "expected_text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "expected")) {
        throw "$Label does not accept 'expected_text' or 'expected'."
    }
}

function Add-RevisionMetadataArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label
    )

    $optionCount = 0
    $author = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("author", "revision_author")
    $clearAuthor = Test-ObjectPropertyExists -Object $Operation -Name "clear_author"
    if (-not [string]::IsNullOrWhiteSpace($author)) {
        if ($clearAuthor -and (Get-OptionalBooleanPropertyValue -Object $Operation -Name "clear_author" -DefaultValue $false)) {
            throw "$Label cannot combine 'author' and 'clear_author'."
        }
        $Arguments.Add("--author") | Out-Null
        $Arguments.Add($author) | Out-Null
        $optionCount += 1
    } elseif ($clearAuthor -and
        (Get-OptionalBooleanPropertyValue -Object $Operation -Name "clear_author" -DefaultValue $false)) {
        $Arguments.Add("--clear-author") | Out-Null
        $optionCount += 1
    }

    $date = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("date", "revision_date")
    $clearDate = Test-ObjectPropertyExists -Object $Operation -Name "clear_date"
    if (-not [string]::IsNullOrWhiteSpace($date)) {
        if ($clearDate -and (Get-OptionalBooleanPropertyValue -Object $Operation -Name "clear_date" -DefaultValue $false)) {
            throw "$Label cannot combine 'date' and 'clear_date'."
        }
        $Arguments.Add("--date") | Out-Null
        $Arguments.Add($date) | Out-Null
        $optionCount += 1
    } elseif ($clearDate -and
        (Get-OptionalBooleanPropertyValue -Object $Operation -Name "clear_date" -DefaultValue $false)) {
        $Arguments.Add("--clear-date") | Out-Null
        $optionCount += 1
    }

    if ($optionCount -eq 0) {
        throw "$Label must provide at least one revision metadata option."
    }
}

function Add-CommentAuthoringArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [switch]$RequireSelectedText,
        [switch]$RequireCommentText
    )

    if ($RequireSelectedText) {
        $selectedText = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("selected_text", "anchor_text") `
            -Label $Label
        $Arguments.Add("--selected-text") | Out-Null
        $Arguments.Add($selectedText) | Out-Null
    }

    if ($RequireCommentText) {
        $commentText = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("comment_text", "text", "body") `
            -Label $Label
        $Arguments.Add("--comment-text") | Out-Null
        $Arguments.Add($commentText) | Out-Null
    }

    $author = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("author", "comment_author")
    if (-not [string]::IsNullOrWhiteSpace($author)) {
        $Arguments.Add("--author") | Out-Null
        $Arguments.Add($author) | Out-Null
    }

    $initials = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("initials", "comment_initials")
    if (-not [string]::IsNullOrWhiteSpace($initials)) {
        $Arguments.Add("--initials") | Out-Null
        $Arguments.Add($initials) | Out-Null
    }

    $date = Get-FirstOptionalObjectPropertyValue `
        -Object $Operation `
        -Names @("date", "comment_date")
    if (-not [string]::IsNullOrWhiteSpace($date)) {
        $Arguments.Add("--date") | Out-Null
        $Arguments.Add($date) | Out-Null
    }
}

function Add-ReviewNoteMutationArguments {
    param(
        [System.Collections.Generic.List[string]]$Arguments,
        $Operation,
        [string]$Label,
        [switch]$RequireReferenceText,
        [switch]$RequireNoteText
    )

    if ($RequireReferenceText) {
        $referenceText = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("reference_text", "selected_text", "anchor_text") `
            -Label $Label
        $Arguments.Add("--reference-text") | Out-Null
        $Arguments.Add($referenceText) | Out-Null
    } elseif ((Test-ObjectPropertyExists -Object $Operation -Name "reference_text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "selected_text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "anchor_text")) {
        throw "$Label does not accept 'reference_text', 'selected_text', or 'anchor_text'."
    }

    if ($RequireNoteText) {
        $noteText = Get-FirstObjectPropertyValue `
            -Object $Operation `
            -Names @("note_text", "text", "body") `
            -Label $Label
        $Arguments.Add("--note-text") | Out-Null
        $Arguments.Add($noteText) | Out-Null
    } elseif ((Test-ObjectPropertyExists -Object $Operation -Name "note_text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "text") -or
        (Test-ObjectPropertyExists -Object $Operation -Name "body")) {
        throw "$Label does not accept 'note_text', 'text', or 'body'."
    }
}
