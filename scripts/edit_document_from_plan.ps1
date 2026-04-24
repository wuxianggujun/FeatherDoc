<#
.SYNOPSIS
Edits an existing DOCX by applying a JSON edit plan.

.DESCRIPTION
Runs a user-facing document-edit workflow:
1. read an edit plan JSON,
2. apply supported operations through featherdoc_cli or focused OOXML edits,
3. write the edited DOCX and a summary JSON.

The supported operations intentionally reuse stable CLI primitives where
possible. Focused OOXML edits are used for document-edit features that Word
stores inside paragraphs/runs, such as table-cell horizontal alignment and
visible text style overrides.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$EditPlan,
    [Parameter(Mandatory = $true)]
    [string]$OutputDocx,
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[edit-document-from-plan] $Message"
}

function Resolve-OptionalEditPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    return Resolve-TemplateSchemaPath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing
}

function Ensure-PathParent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
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

function Get-OptionalObjectPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return ""
    }

    return [string]$value
}

function Get-RequiredObjectPropertyValue {
    param(
        $Object,
        [string]$Name,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyValue -Object $Object -Name $Name
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "$Label must provide '$Name'."
    }

    return $value
}

function Get-EditPlanOperations {
    param($Plan)

    if ($Plan -is [System.Collections.IEnumerable] -and
        -not ($Plan -is [string]) -and
        -not ($Plan -is [System.Collections.IDictionary]) -and
        -not ($Plan -is [System.Management.Automation.PSCustomObject])) {
        return @($Plan | Where-Object { $null -ne $_ })
    }

    $operations = Get-OptionalObjectPropertyObject -Object $Plan -Name "operations"
    if ($null -eq $operations) {
        throw "Edit plan must be an array or provide an 'operations' array."
    }
    if ($operations -is [string]) {
        throw "Edit plan 'operations' must be an array."
    }

    return @($operations | Where-Object { $null -ne $_ })
}

function Get-StringArrayProperty {
    param(
        $Object,
        [string]$Name,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        throw "$Label must provide '$Name'."
    }
    if ($value -is [string]) {
        return @([string]$value)
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | ForEach-Object { [string]$_ })
    }

    throw "$Label '$Name' must be an array of strings."
}

function Get-TableRowsProperty {
    param(
        $Object,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name "rows"
    if ($null -eq $value) {
        throw "$Label must provide 'rows'."
    }
    if ($value -is [string]) {
        throw "$Label 'rows' must be an array of row arrays."
    }

    $rows = New-Object 'System.Collections.Generic.List[object]'
    foreach ($row in @($value)) {
        if ($row -is [string]) {
            $rows.Add(@([string]$row)) | Out-Null
            continue
        }
        if ($row -is [System.Collections.IEnumerable]) {
            $rows.Add(@($row | ForEach-Object { [string]$_ })) | Out-Null
            continue
        }
        throw "$Label rows must contain arrays of cell texts."
    }

    return @($rows.ToArray())
}

function Add-BookmarkSelectorArguments {
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

function New-EditOperationTextFile {
    param(
        [string]$TemporaryRoot,
        [int]$OperationIndex,
        [int]$ValueIndex,
        [string]$Text
    )

    $textDirectory = Join-Path $TemporaryRoot "text-values"
    New-Item -ItemType Directory -Path $textDirectory -Force | Out-Null
    $textPath = Join-Path $textDirectory ("operation-{0}-value-{1}.txt" -f $OperationIndex, $ValueIndex)
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($textPath, $Text, $utf8NoBom)

    return $textPath
}

function Get-BookmarkNameForOperation {
    param(
        $Operation,
        [string]$Label
    )

    $bookmark = Get-OptionalObjectPropertyValue -Object $Operation -Name "bookmark"
    if ([string]::IsNullOrWhiteSpace($bookmark)) {
        $bookmark = Get-OptionalObjectPropertyValue -Object $Operation -Name "bookmark_name"
    }
    if ([string]::IsNullOrWhiteSpace($bookmark)) {
        throw "$Label must provide 'bookmark' or 'bookmark_name'."
    }

    return $bookmark
}

function Get-OperationAlignmentProperty {
    param(
        $Operation,
        [string]$Label,
        [string]$FallbackName = ""
    )

    $alignment = Get-OptionalObjectPropertyValue -Object $Operation -Name "alignment"
    if ([string]::IsNullOrWhiteSpace($alignment) -and
        -not [string]::IsNullOrWhiteSpace($FallbackName)) {
        $alignment = Get-OptionalObjectPropertyValue -Object $Operation -Name $FallbackName
    }
    if ([string]::IsNullOrWhiteSpace($alignment)) {
        throw "$Label must provide 'alignment'."
    }

    return $alignment.Trim()
}

function Normalize-TableCellHorizontalAlignment {
    param(
        [string]$Alignment,
        [string]$Label
    )

    switch ($Alignment.Trim().ToLowerInvariant()) {
        "left" { return "left" }
        "start" { return "left" }
        "center" { return "center" }
        "centre" { return "center" }
        "right" { return "right" }
        "end" { return "right" }
        "both" { return "both" }
        "justify" { return "both" }
        default {
            throw "$Label has unsupported horizontal alignment '$Alignment'. Use left, center, right, or both."
        }
    }
}

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

function New-OperationArguments {
    param(
        $Operation,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Label,
        [int]$OperationIndex,
        [string]$TemporaryRoot
    )

    $op = (Get-RequiredObjectPropertyValue -Object $Operation -Name "op" -Label $Label).Trim()
    $normalizedOp = $op -replace '-', '_'
    $arguments = New-Object 'System.Collections.Generic.List[string]'

    switch ($normalizedOp) {
        "replace_bookmark_text" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $textPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1 `
                -Text $text
            $arguments.Add("fill-bookmarks") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add("--set-file") | Out-Null
            $arguments.Add($bookmark) | Out-Null
            $arguments.Add($textPath) | Out-Null
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "replace_bookmark_paragraphs" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $paragraphs = Get-StringArrayProperty -Object $Operation -Name "paragraphs" -Label $Label
            $arguments.Add("replace-bookmark-paragraphs") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            $valueIndex = 1
            foreach ($paragraph in $paragraphs) {
                $paragraphPath = New-EditOperationTextFile `
                    -TemporaryRoot $TemporaryRoot `
                    -OperationIndex $OperationIndex `
                    -ValueIndex $valueIndex `
                    -Text $paragraph
                $arguments.Add("--paragraph-file") | Out-Null
                $arguments.Add($paragraphPath) | Out-Null
                $valueIndex += 1
            }
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "replace_bookmark_table_rows" {
            $bookmark = Get-BookmarkNameForOperation -Operation $Operation -Label $Label
            $rows = Get-TableRowsProperty -Object $Operation -Label $Label
            $arguments.Add("replace-bookmark-table-rows") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($bookmark) | Out-Null
            if ($rows.Count -eq 0) {
                $arguments.Add("--empty") | Out-Null
            } else {
                $valueIndex = 1
                foreach ($row in $rows) {
                    $cells = @($row)
                    if ($cells.Count -eq 0) {
                        throw "$Label table rows must contain at least one cell text."
                    }
                    $rowPath = New-EditOperationTextFile `
                        -TemporaryRoot $TemporaryRoot `
                        -OperationIndex $OperationIndex `
                        -ValueIndex $valueIndex `
                        -Text ([string]$cells[0])
                    $arguments.Add("--row-file") | Out-Null
                    $arguments.Add($rowPath) | Out-Null
                    $valueIndex += 1
                    for ($index = 1; $index -lt $cells.Count; ++$index) {
                        $cellPath = New-EditOperationTextFile `
                            -TemporaryRoot $TemporaryRoot `
                            -OperationIndex $OperationIndex `
                            -ValueIndex $valueIndex `
                            -Text ([string]$cells[$index])
                        $arguments.Add("--cell-file") | Out-Null
                        $arguments.Add($cellPath) | Out-Null
                        $valueIndex += 1
                    }
                }
            }
            Add-BookmarkSelectorArguments -Arguments $arguments -Operation $Operation
        }
        "merge_table_cells" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $count = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("count", "merge_count")
            $arguments.Add("merge-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace($count)) {
                $arguments.Add("--count") | Out-Null
                $arguments.Add($count) | Out-Null
            }
        }
        "merge_table_cell" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $count = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("count", "merge_count")
            $arguments.Add("merge-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
            if (-not [string]::IsNullOrWhiteSpace($count)) {
                $arguments.Add("--count") | Out-Null
                $arguments.Add($count) | Out-Null
            }
        }
        "unmerge_table_cells" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $arguments.Add("unmerge-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
        }
        "unmerge_table_cell" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $direction = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("direction", "merge_direction") `
                -Label $Label
            $arguments.Add("unmerge-table-cells") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--direction") | Out-Null
            $arguments.Add($direction) | Out-Null
        }
        "set_table_column_width" {
            return New-DirectTableColumnWidthOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "set_table_col_width" {
            return New-DirectTableColumnWidthOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "clear_table_column_width" {
            return New-DirectTableColumnWidthOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "clear_table_col_width" {
            return New-DirectTableColumnWidthOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "set_table_cell_text" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $text = Get-RequiredObjectPropertyValue -Object $Operation -Name "text" -Label $Label
            $textPath = New-EditOperationTextFile `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex `
                -ValueIndex 1 `
                -Text $text
            $arguments.Add("set-table-cell-text") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add("--text-file") | Out-Null
            $arguments.Add($textPath) | Out-Null
        }
        "set_table_cell_fill" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $color = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("color", "fill_color", "background_color", "bg_color")
            if ([string]::IsNullOrWhiteSpace($color)) {
                throw "$Label must provide one of: color, fill_color, background_color, bg_color."
            }
            $arguments.Add("set-table-cell-fill") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($color) | Out-Null
        }
        "clear_table_cell_fill" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("clear-table-cell-fill") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "set_table_cell_border" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "border_edge") `
                -Label $Label
            $style = Get-OptionalObjectPropertyValue -Object $Operation -Name "style"
            if ([string]::IsNullOrWhiteSpace($style)) {
                $style = "single"
            }
            $arguments.Add("set-table-cell-border") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
            $arguments.Add("--style") | Out-Null
            $arguments.Add($style) | Out-Null
            $size = Get-FirstOptionalObjectPropertyValue `
                -Object $Operation `
                -Names @("size", "width", "thickness")
            if (-not [string]::IsNullOrWhiteSpace($size)) {
                $arguments.Add("--size") | Out-Null
                $arguments.Add($size) | Out-Null
            }
            $color = Get-OptionalObjectPropertyValue -Object $Operation -Name "color"
            if (-not [string]::IsNullOrWhiteSpace($color)) {
                $arguments.Add("--color") | Out-Null
                $arguments.Add($color) | Out-Null
            }
            $space = Get-OptionalObjectPropertyValue -Object $Operation -Name "space"
            if (-not [string]::IsNullOrWhiteSpace($space)) {
                $arguments.Add("--space") | Out-Null
                $arguments.Add($space) | Out-Null
            }
        }
        "clear_table_cell_border" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $edge = Get-FirstObjectPropertyValue `
                -Object $Operation `
                -Names @("edge", "border_edge") `
                -Label $Label
            $arguments.Add("clear-table-cell-border") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($edge) | Out-Null
        }
        "set_table_cell_vertical_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $alignment = Get-OperationAlignmentProperty `
                -Operation $Operation `
                -Label $Label `
                -FallbackName "vertical_alignment"
            $arguments.Add("set-table-cell-vertical-alignment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
            $arguments.Add($alignment) | Out-Null
        }
        "clear_table_cell_vertical_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $arguments.Add("clear-table-cell-vertical-alignment") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($cellIndex) | Out-Null
        }
        "set_table_row_height" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $heightTwips = Get-RequiredObjectPropertyValue -Object $Operation -Name "height_twips" -Label $Label
            $heightRule = Get-OperationRowHeightRule -Operation $Operation
            $arguments.Add("set-table-row-height") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
            $arguments.Add($heightTwips) | Out-Null
            $arguments.Add($heightRule) | Out-Null
        }
        "clear_table_row_height" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $arguments.Add("clear-table-row-height") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($tableIndex) | Out-Null
            $arguments.Add($rowIndex) | Out-Null
        }
        "set_table_cell_horizontal_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            $alignment = Normalize-TableCellHorizontalAlignment `
                -Alignment (Get-OperationAlignmentProperty `
                    -Operation $Operation `
                    -Label $Label `
                    -FallbackName "horizontal_alignment") `
                -Label $Label
            return [pscustomobject]@{
                Operation = $normalizedOp
                Arguments = @()
                Command = "set-table-cell-horizontal-alignment"
                DirectOperation = $true
                DirectOperationKind = "table-cell-horizontal-alignment"
                TableIndex = $tableIndex
                RowIndex = $rowIndex
                CellIndex = $cellIndex
                HorizontalAlignment = $alignment
                ClearHorizontalAlignment = $false
            }
        }
        "clear_table_cell_horizontal_alignment" {
            $tableIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "table_index" -Label $Label
            $rowIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "row_index" -Label $Label
            $cellIndex = Get-RequiredObjectPropertyValue -Object $Operation -Name "cell_index" -Label $Label
            return [pscustomobject]@{
                Operation = $normalizedOp
                Arguments = @()
                Command = "clear-table-cell-horizontal-alignment"
                DirectOperation = $true
                DirectOperationKind = "table-cell-horizontal-alignment"
                TableIndex = $tableIndex
                RowIndex = $rowIndex
                CellIndex = $cellIndex
                HorizontalAlignment = ""
                ClearHorizontalAlignment = $true
            }
        }
        "set_text_style" {
            return New-DirectTextStyleOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "set_paragraph_text_style" {
            return New-DirectTextStyleOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "set_text_format" {
            return New-DirectTextStyleOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "delete_paragraph" {
            return New-DirectParagraphDeletionOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp
        }
        "delete_paragraph_contains" {
            return New-DirectParagraphDeletionOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp
        }
        "remove_paragraph" {
            return New-DirectParagraphDeletionOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp
        }
        "replace_text" {
            return New-DirectTextReplacementOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "replace_document_text" {
            return New-DirectTextReplacementOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label
        }
        "set_paragraph_horizontal_alignment" {
            return New-DirectParagraphHorizontalAlignmentOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "set_paragraph_alignment" {
            return New-DirectParagraphHorizontalAlignmentOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "clear_paragraph_horizontal_alignment" {
            return New-DirectParagraphHorizontalAlignmentOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "clear_paragraph_alignment" {
            return New-DirectParagraphHorizontalAlignmentOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        "set_paragraph_spacing" {
            return New-DirectParagraphSpacingOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "set_paragraph_line_spacing" {
            return New-DirectParagraphSpacingOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $false
        }
        "clear_paragraph_spacing" {
            return New-DirectParagraphSpacingOperation `
                -Operation $Operation `
                -NormalizedOp $normalizedOp `
                -Label $Label `
                -Clear $true
        }
        default {
            throw "$Label has unsupported op '$op'."
        }
    }

    $arguments.Add("--output") | Out-Null
    $arguments.Add($OutputPath) | Out-Null
    $arguments.Add("--json") | Out-Null

    return [pscustomobject]@{
        Operation = $normalizedOp
        Arguments = @($arguments.ToArray())
        Command = [string]$arguments[0]
        DirectOperation = $false
    }
}

function New-OperationRecord {
    param(
        [int]$Index,
        [string]$Op,
        [string]$Command,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Status,
        [int]$ExitCode,
        [string[]]$Output,
        [string]$ErrorMessage
    )

    $record = [ordered]@{
        index = $Index
        op = $Op
        command = $Command
        input_docx = $InputPath
        output_docx = $OutputPath
        status = $Status
        exit_code = $ExitCode
        output = @($Output)
    }
    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $record.error = $ErrorMessage
    }

    return $record
}

function Build-EditSummary {
    param(
        [string]$Status,
        [string]$InputDocx,
        [string]$EditPlan,
        [string]$OutputDocx,
        [string]$SummaryJson,
        [string]$CliPath,
        [string]$BuildDir,
        [string]$Generator,
        [bool]$SkipBuild,
        [object[]]$Operations,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        edit_plan = $EditPlan
        output_docx = $OutputDocx
        summary_json = $SummaryJson
        cli_path = $CliPath
        build_dir = $BuildDir
        generator = $Generator
        skip_build = $SkipBuild
        operation_count = @($Operations).Count
        operations = @($Operations)
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedEditPlan = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $EditPlan
$resolvedOutputDocx = Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $OutputDocx
$resolvedSummaryJson = Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $BuildDir
}

Ensure-PathParent -Path $resolvedOutputDocx
Ensure-PathParent -Path $resolvedSummaryJson

$planObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedEditPlan | ConvertFrom-Json
$operations = Get-EditPlanOperations -Plan $planObject
if ($operations.Count -eq 0) {
    throw "Edit plan must contain at least one operation."
}

$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "featherdoc-edit-document-" + [System.Guid]::NewGuid().ToString("N")
)
$operationRecords = New-Object 'System.Collections.Generic.List[object]'
$status = "failed"
$failureMessage = ""
$cliPath = ""

New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null

try {
    Write-Step "Resolving featherdoc_cli"
    $cliPath = Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $resolvedBuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild

    $currentInput = $resolvedInputDocx
    for ($index = 0; $index -lt $operations.Count; ++$index) {
        $operationIndex = $index + 1
        $isLast = ($operationIndex -eq $operations.Count)
        $stepOutput = if ($isLast) {
            $resolvedOutputDocx
        } else {
            Join-Path $temporaryRoot ("edit-step-{0}.docx" -f $operationIndex)
        }

        $label = "operation[$operationIndex]"
        $operationInfo = New-OperationArguments `
            -Operation $operations[$index] `
            -InputPath $currentInput `
            -OutputPath $stepOutput `
            -Label $label `
            -OperationIndex $operationIndex `
            -TemporaryRoot $temporaryRoot

        Write-Step "Applying $($operationInfo.Operation)"
        if ($operationInfo.DirectOperation) {
            try {
                $directOperationKind = Get-OptionalObjectPropertyValue `
                    -Object $operationInfo `
                    -Name "DirectOperationKind"
                switch ($directOperationKind) {
                    "table-cell-horizontal-alignment" {
                        $directResult = Set-DocxTableCellHorizontalAlignment `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -RowIndex $operationInfo.RowIndex `
                            -CellIndex $operationInfo.CellIndex `
                            -Alignment $operationInfo.HorizontalAlignment `
                            -Clear $operationInfo.ClearHorizontalAlignment
                    }
                    "body-table-column-width" {
                        $directResult = Set-DocxTableColumnWidth `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -ColumnIndex $operationInfo.ColumnIndex `
                            -WidthTwips $operationInfo.WidthTwips `
                            -Clear $operationInfo.ClearColumnWidth
                    }
                    "text-style" {
                        $directResult = Set-DocxParagraphTextStyle `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -RowIndex $operationInfo.RowIndex `
                            -CellIndex $operationInfo.CellIndex `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -BoldSpecified $operationInfo.BoldSpecified `
                            -Bold $operationInfo.Bold `
                            -FontFamily $operationInfo.FontFamily `
                            -EastAsiaFontFamily $operationInfo.EastAsiaFontFamily `
                            -Language $operationInfo.Language `
                            -EastAsiaLanguage $operationInfo.EastAsiaLanguage `
                            -Color $operationInfo.Color `
                            -FontSizeHalfPoints $operationInfo.FontSizeHalfPoints
                    }
                    "body-paragraph-deletion" {
                        $directResult = Remove-DocxBodyParagraphs `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains
                    }
                    "text-replacement" {
                        $directResult = Set-DocxTextReplacement `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -FindText $operationInfo.FindText `
                            -ReplacementText $operationInfo.ReplacementText `
                            -MatchCase $operationInfo.MatchCase
                    }
                    "body-paragraph-horizontal-alignment" {
                        $directResult = Set-DocxBodyParagraphHorizontalAlignment `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -Alignment $operationInfo.HorizontalAlignment `
                            -Clear $operationInfo.ClearHorizontalAlignment
                    }
                    "body-paragraph-spacing" {
                        $directResult = Set-DocxBodyParagraphSpacing `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -BeforeTwips $operationInfo.BeforeTwips `
                            -AfterTwips $operationInfo.AfterTwips `
                            -LineTwips $operationInfo.LineTwips `
                            -LineRule $operationInfo.LineRule `
                            -Clear $operationInfo.ClearSpacing
                    }
                    default {
                        throw "Unsupported direct operation kind '$directOperationKind'."
                    }
                }
                $result = [pscustomobject]@{
                    ExitCode = 0
                    Text = ""
                    Output = @(($directResult | ConvertTo-Json -Compress))
                }
            } catch {
                $result = [pscustomobject]@{
                    ExitCode = 1
                    Text = $_.Exception.Message
                    Output = @($_.Exception.Message)
                }
            }
        } else {
            $result = Invoke-TemplateSchemaCli `
                -ExecutablePath $cliPath `
                -Arguments $operationInfo.Arguments
        }

        if ($result.ExitCode -ne 0) {
            $message = if ([string]::IsNullOrWhiteSpace($result.Text)) {
                "$label failed."
            } else {
                "$label failed: $($result.Text)"
            }
            $operationRecords.Add([pscustomobject](New-OperationRecord `
                    -Index $operationIndex `
                    -Op $operationInfo.Operation `
                    -Command $operationInfo.Command `
                    -InputPath $currentInput `
                    -OutputPath $stepOutput `
                    -Status "failed" `
                    -ExitCode $result.ExitCode `
                    -Output $result.Output `
                    -ErrorMessage $message)) | Out-Null
            throw $message
        }
        if (-not (Test-Path -LiteralPath $stepOutput)) {
            $message = "$label did not produce '$stepOutput'."
            $operationRecords.Add([pscustomobject](New-OperationRecord `
                    -Index $operationIndex `
                    -Op $operationInfo.Operation `
                    -Command $operationInfo.Command `
                    -InputPath $currentInput `
                    -OutputPath $stepOutput `
                    -Status "failed" `
                    -ExitCode $result.ExitCode `
                    -Output $result.Output `
                    -ErrorMessage $message)) | Out-Null
            throw $message
        }

        $operationRecords.Add([pscustomobject](New-OperationRecord `
                -Index $operationIndex `
                -Op $operationInfo.Operation `
                -Command $operationInfo.Command `
                -InputPath $currentInput `
                -OutputPath $stepOutput `
                -Status "completed" `
                -ExitCode $result.ExitCode `
                -Output $result.Output `
                -ErrorMessage "")) | Out-Null
        $currentInput = $stepOutput
    }

    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    Write-Error $failureMessage
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-EditSummary `
            -Status $status `
            -InputDocx $resolvedInputDocx `
            -EditPlan $resolvedEditPlan `
            -OutputDocx $resolvedOutputDocx `
            -SummaryJson $resolvedSummaryJson `
            -CliPath $cliPath `
            -BuildDir $resolvedBuildDir `
            -Generator $Generator `
            -SkipBuild ([bool]$SkipBuild) `
            -Operations @($operationRecords.ToArray()) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }

    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

if ($status -ne "completed") {
    exit 1
}

Write-Step "Edited DOCX: $resolvedOutputDocx"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
