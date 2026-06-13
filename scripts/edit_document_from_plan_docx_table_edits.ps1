function Get-OperationRowHeightRule {
    param($Operation)

    $rule = Get-OptionalObjectPropertyValue -Object $Operation -Name "rule"
    if ([string]::IsNullOrWhiteSpace($rule)) {
        $rule = Get-OptionalObjectPropertyValue -Object $Operation -Name "height_rule"
    }
    if ([string]::IsNullOrWhiteSpace($rule)) {
        return "exact"
    }

    return $rule.Trim()
}

function Read-DocxZipEntryText {
    param(
        [System.IO.Compression.ZipArchive]$Archive,
        [string]$EntryName
    )

    $entry = $Archive.GetEntry($EntryName)
    if ($null -eq $entry) {
        throw "Entry '$EntryName' was not found."
    }

    $reader = New-Object System.IO.StreamReader($entry.Open())
    try {
        return $reader.ReadToEnd()
    } finally {
        $reader.Dispose()
    }
}

function Write-DocxZipEntryText {
    param(
        [System.IO.Compression.ZipArchive]$Archive,
        [string]$EntryName,
        [string]$Text
    )

    $existingEntry = $Archive.GetEntry($EntryName)
    if ($null -ne $existingEntry) {
        $existingEntry.Delete()
    }

    $entry = $Archive.CreateEntry($EntryName)
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    $writer = New-Object System.IO.StreamWriter($entry.Open(), $utf8NoBom)
    try {
        $writer.Write($Text)
    } finally {
        $writer.Dispose()
    }
}

function Set-ParagraphHorizontalAlignmentNode {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Paragraph,
        [string]$Alignment,
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

    $alignmentNode = $paragraphProperties.SelectSingleNode("w:jc", $NamespaceManager)
    if ($Clear) {
        if ($null -ne $alignmentNode) {
            $paragraphProperties.RemoveChild($alignmentNode) | Out-Null
        }
        if ($paragraphProperties.ChildNodes.Count -eq 0 -and
            $paragraphProperties.Attributes.Count -eq 0) {
            $Paragraph.RemoveChild($paragraphProperties) | Out-Null
        }
        return
    }

    if ($null -eq $alignmentNode) {
        $alignmentNode = $Document.CreateElement("w", "jc", $wordNamespace)
        $paragraphProperties.AppendChild($alignmentNode) | Out-Null
    }

    $valueAttribute = $Document.CreateAttribute("w", "val", $wordNamespace)
    $valueAttribute.Value = $Alignment
    $alignmentNode.Attributes.SetNamedItem($valueAttribute) | Out-Null
}

function Set-DocxTableCellHorizontalAlignment {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$TableIndex,
        [string]$RowIndex,
        [string]$CellIndex,
        [string]$Alignment,
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

        $tables = @($document.SelectNodes("/w:document/w:body/w:tbl", $namespaceManager))
        $tableIndexValue = [int]$TableIndex
        $rowIndexValue = [int]$RowIndex
        $cellIndexValue = [int]$CellIndex
        if ($tableIndexValue -lt 0 -or $tableIndexValue -ge $tables.Count) {
            throw "table_index '$TableIndex' is out of range."
        }

        $rows = @($tables[$tableIndexValue].SelectNodes("w:tr", $namespaceManager))
        if ($rowIndexValue -lt 0 -or $rowIndexValue -ge $rows.Count) {
            throw "row_index '$RowIndex' is out of range."
        }

        $cells = @($rows[$rowIndexValue].SelectNodes("w:tc", $namespaceManager))
        if ($cellIndexValue -lt 0 -or $cellIndexValue -ge $cells.Count) {
            throw "cell_index '$CellIndex' is out of range."
        }

        $paragraphs = @($cells[$cellIndexValue].SelectNodes("w:p", $namespaceManager))
        if ($paragraphs.Count -eq 0) {
            throw "target table cell does not contain direct paragraphs."
        }

        foreach ($paragraph in $paragraphs) {
            Set-ParagraphHorizontalAlignmentNode `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -Paragraph $paragraph `
                -Alignment $Alignment `
                -Clear $Clear
        }

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            table_index = $tableIndexValue
            row_index = $rowIndexValue
            cell_index = $cellIndexValue
            paragraph_count = $paragraphs.Count
        }
    } finally {
        $archive.Dispose()
    }
}


function Get-OrCreateTableCellProperties {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Cell
    )

    $cellProperties = $Cell.SelectSingleNode("w:tcPr", $NamespaceManager)
    if ($null -ne $cellProperties) {
        return $cellProperties
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $cellProperties = $Document.CreateElement("w", "tcPr", $wordNamespace)
    if ($Cell.HasChildNodes) {
        $Cell.InsertBefore($cellProperties, $Cell.FirstChild) | Out-Null
    } else {
        $Cell.AppendChild($cellProperties) | Out-Null
    }
    return $cellProperties
}

function Get-OrCreateTableGrid {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Table
    )

    $tableGrid = $Table.SelectSingleNode("w:tblGrid", $NamespaceManager)
    if ($null -ne $tableGrid) {
        return $tableGrid
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $tableGrid = $Document.CreateElement("w", "tblGrid", $wordNamespace)
    $tableProperties = $Table.SelectSingleNode("w:tblPr", $NamespaceManager)
    if ($null -ne $tableProperties) {
        $Table.InsertAfter($tableGrid, $tableProperties) | Out-Null
    } elseif ($Table.HasChildNodes) {
        $Table.InsertBefore($tableGrid, $Table.FirstChild) | Out-Null
    } else {
        $Table.AppendChild($tableGrid) | Out-Null
    }
    return $tableGrid
}

function Set-TableColumnWidthNode {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [System.Xml.XmlElement]$Table,
        [int]$ColumnIndex,
        [string]$WidthTwips,
        [bool]$Clear
    )

    if ($ColumnIndex -lt 0) {
        throw "column_index '$ColumnIndex' is out of range."
    }

    $wordNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"
    $targetCellCount = 0
    $tableGrid = if ($Clear) {
        $Table.SelectSingleNode("w:tblGrid", $NamespaceManager)
    } else {
        Get-OrCreateTableGrid -Document $Document -NamespaceManager $NamespaceManager -Table $Table
    }

    if ($null -ne $tableGrid) {
        $gridColumns = @($tableGrid.SelectNodes("w:gridCol", $NamespaceManager))
        if ($Clear) {
            if ($ColumnIndex -lt $gridColumns.Count) {
                $gridColumns[$ColumnIndex].RemoveAttribute("w", $wordNamespace)
            }
        } else {
            while ($gridColumns.Count -le $ColumnIndex) {
                $gridColumn = $Document.CreateElement("w", "gridCol", $wordNamespace)
                $tableGrid.AppendChild($gridColumn) | Out-Null
                $gridColumns = @($tableGrid.SelectNodes("w:gridCol", $NamespaceManager))
            }
            Set-WordAttribute -Document $Document -Element $gridColumns[$ColumnIndex] -Name "w" -Value $WidthTwips
        }
    }

    $rows = @($Table.SelectNodes("w:tr", $NamespaceManager))
    foreach ($row in $rows) {
        $cells = @($row.SelectNodes("w:tc", $NamespaceManager))
        if ($ColumnIndex -ge $cells.Count) {
            continue
        }

        $targetCellCount += 1
        $cell = $cells[$ColumnIndex]
        $cellProperties = if ($Clear) {
            $cell.SelectSingleNode("w:tcPr", $NamespaceManager)
        } else {
            Get-OrCreateTableCellProperties -Document $Document -NamespaceManager $NamespaceManager -Cell $cell
        }
        if ($null -eq $cellProperties) {
            continue
        }

        $cellWidthNode = $cellProperties.SelectSingleNode("w:tcW", $NamespaceManager)
        if ($Clear) {
            if ($null -ne $cellWidthNode) {
                $cellProperties.RemoveChild($cellWidthNode) | Out-Null
            }
            if ($cellProperties.ChildNodes.Count -eq 0 -and
                $cellProperties.Attributes.Count -eq 0) {
                $cell.RemoveChild($cellProperties) | Out-Null
            }
            continue
        }

        if ($null -eq $cellWidthNode) {
            $cellWidthNode = $Document.CreateElement("w", "tcW", $wordNamespace)
            if ($cellProperties.HasChildNodes) {
                $cellProperties.InsertBefore($cellWidthNode, $cellProperties.FirstChild) | Out-Null
            } else {
                $cellProperties.AppendChild($cellWidthNode) | Out-Null
            }
        }
        Set-WordAttribute -Document $Document -Element $cellWidthNode -Name "w" -Value $WidthTwips
        Set-WordAttribute -Document $Document -Element $cellWidthNode -Name "type" -Value "dxa"
    }

    if ($targetCellCount -eq 0) {
        throw "column_index '$ColumnIndex' did not match any table cells."
    }

    return [ordered]@{
        row_count = $rows.Count
        target_cell_count = $targetCellCount
    }
}

function Set-DocxTableColumnWidth {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$TableIndex,
        [string]$ColumnIndex,
        [string]$WidthTwips,
        [bool]$Clear
    )

    $inputFullPath = [System.IO.Path]::GetFullPath($InputPath)
    $outputFullPath = [System.IO.Path]::GetFullPath($OutputPath)
    if ($inputFullPath -ne $outputFullPath) {
        Copy-Item -LiteralPath $inputFullPath -Destination $outputFullPath -Force
    }

    if (-not $Clear) {
        $widthValue = [int]$WidthTwips
        if ($widthValue -le 0) {
            throw "width_twips must be greater than zero."
        }
        $WidthTwips = [string]$widthValue
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

        $tables = @($document.SelectNodes("/w:document/w:body/w:tbl", $namespaceManager))
        $tableIndexValue = [int]$TableIndex
        if ($tableIndexValue -lt 0 -or $tableIndexValue -ge $tables.Count) {
            throw "table_index '$TableIndex' is out of range."
        }
        $columnIndexValue = [int]$ColumnIndex

        $mutationResult = Set-TableColumnWidthNode `
            -Document $document `
            -NamespaceManager $namespaceManager `
            -Table $tables[$tableIndexValue] `
            -ColumnIndex $columnIndexValue `
            -WidthTwips $WidthTwips `
            -Clear $Clear

        Write-DocxZipEntryText `
            -Archive $archive `
            -EntryName "word/document.xml" `
            -Text $document.OuterXml

        return [ordered]@{
            table_index = $tableIndexValue
            column_index = $columnIndexValue
            width_twips = $WidthTwips
            clear = $Clear
            row_count = $mutationResult.row_count
            target_cell_count = $mutationResult.target_cell_count
        }
    } finally {
        $archive.Dispose()
    }
}

function Get-ParagraphTextContent {
    param(
        [System.Xml.XmlElement]$Paragraph,
        [System.Xml.XmlNamespaceManager]$NamespaceManager
    )

    $builder = New-Object System.Text.StringBuilder
    foreach ($textNode in @($Paragraph.SelectNodes(".//w:t", $NamespaceManager))) {
        [void]$builder.Append($textNode.InnerText)
    }

    return $builder.ToString()
}

function Get-BodyParagraphAlignmentTargets {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$ParagraphIndex,
        [string]$TextContains
    )

    $paragraphs = @($Document.SelectNodes("/w:document/w:body/w:p", $NamespaceManager))
    if (-not [string]::IsNullOrWhiteSpace($ParagraphIndex)) {
        $paragraphIndexValue = [int]$ParagraphIndex
        if ($paragraphIndexValue -lt 0 -or $paragraphIndexValue -ge $paragraphs.Count) {
            throw "paragraph_index '$ParagraphIndex' is out of range."
        }

        return @($paragraphs[$paragraphIndexValue])
    }

    if (-not [string]::IsNullOrWhiteSpace($TextContains)) {
        $matches = @($paragraphs | Where-Object {
                (Get-ParagraphTextContent -Paragraph $_ -NamespaceManager $NamespaceManager).Contains($TextContains)
            })
        if ($matches.Count -eq 0) {
            throw "No body paragraph contains '$TextContains'."
        }

        return $matches
    }

    throw "Paragraph alignment operation must provide 'paragraph_index' or 'text_contains'."
}

function Set-DocxBodyParagraphHorizontalAlignment {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [string]$ParagraphIndex,
        [string]$TextContains,
        [string]$Alignment,
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
            Set-ParagraphHorizontalAlignmentNode `
                -Document $document `
                -NamespaceManager $namespaceManager `
                -Paragraph $paragraph `
                -Alignment $Alignment `
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
        }
    } finally {
        $archive.Dispose()
    }
}
