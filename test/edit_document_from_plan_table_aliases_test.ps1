param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText'."
    }
}

function Assert-NotContainsText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Label
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText'."
    }
}

function Assert-NoDocxXPath {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$XPath,
        [string]$Message
    )

    if ($null -ne $Document.SelectSingleNode($XPath, $NamespaceManager)) {
        throw "$Message XPath='$XPath'."
    }
}

function New-WordNamespaceManager {
    param([System.Xml.XmlDocument]$Document)

    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($Document.NameTable)
    $null = $namespaceManager.AddNamespace(
        "w",
        "http://schemas.openxmlformats.org/wordprocessingml/2006/main")
    Write-Output -NoEnumerate $namespaceManager
}

function Add-ZipTextEntry {
    param(
        [System.IO.Compression.ZipArchive]$Archive,
        [string]$EntryName,
        [string]$Content
    )

    $entry = $Archive.CreateEntry($EntryName)
    $stream = $entry.Open()
    try {
        $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
        $writer = New-Object System.IO.StreamWriter($stream, $utf8NoBom)
        try {
            $writer.Write($Content)
        } finally {
            $writer.Dispose()
        }
    } finally {
        $stream.Dispose()
    }
}

function Read-DocxEntryText {
    param(
        [string]$DocxPath,
        [string]$EntryName
    )

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entry = $archive.GetEntry($EntryName)
        if ($null -eq $entry) {
            throw "Entry '$EntryName' was not found in $DocxPath."
        }

        $reader = New-Object System.IO.StreamReader($entry.Open())
        try {
            return $reader.ReadToEnd()
        } finally {
            $reader.Dispose()
        }
    } finally {
        $archive.Dispose()
    }
}

function New-TableAliasFixtureDocx {
    param([string]$Path)

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }

    $relationshipsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>
'@
    $contentTypesXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels"
           ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Before table aliases</w:t></w:r></w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="TableGrid"/>
        <w:tblW w:w="7200" w:type="dxa"/>
        <w:jc w:val="center"/>
        <w:tblInd w:w="360" w:type="dxa"/>
        <w:tblLayout w:type="fixed"/>
        <w:tblCellSpacing w:w="120" w:type="dxa"/>
        <w:tblCellMar>
          <w:left w:w="240" w:type="dxa"/>
        </w:tblCellMar>
        <w:tblBorders>
          <w:insideH w:val="dashed" w:sz="8" w:space="2" w:color="00AA00"/>
        </w:tblBorders>
        <w:tblLook w:val="0340" w:firstRow="0" w:lastRow="1" w:firstColumn="0" w:lastColumn="1" w:noHBand="1" w:noVBand="0"/>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="1800"/>
        <w:gridCol w:w="3600"/>
      </w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>style-a1</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="3600" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>style-b1</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>style-a2</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="3600" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>style-b2</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:tbl>
      <w:tblGrid><w:gridCol w:w="2400"/><w:gridCol w:w="2400"/></w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>delete-row-keep-top-a</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>delete-row-keep-top-b</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>delete-row-target-a</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>delete-row-target-b</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>delete-row-keep-bottom-a</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>delete-row-keep-bottom-b</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:tbl>
      <w:tblGrid><w:gridCol w:w="1600"/><w:gridCol w:w="1600"/><w:gridCol w:w="1600"/></w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>col-keep-left-1</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>delete-col-target-1</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>col-keep-right-1</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>col-keep-left-2</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>delete-col-target-2</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>col-keep-right-2</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:tbl>
      <w:tblGrid><w:gridCol w:w="2400"/></w:tblGrid>
      <w:tr><w:tc><w:p><w:r><w:t>delete-table-target</w:t></w:r></w:p></w:tc></w:tr>
    </w:tbl>
    <w:p><w:r><w:t>After table aliases</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
'@

    $fileStream = [System.IO.File]::Open(
        $Path,
        [System.IO.FileMode]::Create,
        [System.IO.FileAccess]::ReadWrite,
        [System.IO.FileShare]::None)
    try {
        $archive = New-Object System.IO.Compression.ZipArchive($fileStream, [System.IO.Compression.ZipArchiveMode]::Create)
        try {
            Add-ZipTextEntry -Archive $archive -EntryName "_rels/.rels" -Content $relationshipsXml
            Add-ZipTextEntry -Archive $archive -EntryName "[Content_Types].xml" -Content $contentTypesXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/document.xml" -Content $documentXml
        } finally {
            $archive.Dispose()
        }
    } finally {
        $fileStream.Dispose()
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build-msvc-nmake"
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_table_aliases"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sourceDocx = Join-Path $resolvedWorkingDir "table_aliases.source.docx"
$editPlanPath = Join-Path $resolvedWorkingDir "table_aliases.edit_plan.json"
$outputDocx = Join-Path $resolvedWorkingDir "table_aliases.edited.docx"
$summaryPath = Join-Path $resolvedWorkingDir "table_aliases.summary.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null
New-TableAliasFixtureDocx -Path $sourceDocx

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_col_width",
      "table_index": 0,
      "col_index": 1,
      "width": 2800
    },
    {
      "op": "clear_table_col_width",
      "table_index": 0,
      "column": 1
    },
    {
      "op": "clear_table_width",
      "table_index": 0
    },
    {
      "op": "clear_table_alignment",
      "table_index": 0
    },
    {
      "op": "clear_table_indent",
      "table_index": 0
    },
    {
      "op": "clear_table_layout_mode",
      "table_index": 0
    },
    {
      "op": "clear_table_style_id",
      "table_index": 0
    },
    {
      "op": "clear_table_style_look",
      "table_index": 0
    },
    {
      "op": "clear_table_cell_spacing",
      "table_index": 0
    },
    {
      "op": "clear_table_default_cell_margin",
      "table_index": 0,
      "margin_edge": "left"
    },
    {
      "op": "clear_table_border",
      "table_index": 0,
      "border_edge": "inside_horizontal"
    },
    {
      "op": "delete_table_row",
      "table_index": 1,
      "row_index": 1
    },
    {
      "op": "delete_table_column",
      "table_index": 2,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "delete_table",
      "table_index": 3
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sourceDocx `
    -EditPlan $editPlanPath `
    -OutputDocx $outputDocx `
    -SummaryJson $summaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for table alias operations."
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $outputDocx -EntryName "word/document.xml"
$document = New-Object System.Xml.XmlDocument
$document.PreserveWhitespace = $true
$document.LoadXml($documentXml)
$namespaceManager = New-WordNamespaceManager -Document $document

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Table-alias summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 14 `
    -Message "Table-alias summary should record fourteen operations."

$expectedCommands = @(
    "set-table-column-width",
    "clear-table-column-width",
    "clear-table-width",
    "clear-table-alignment",
    "clear-table-indent",
    "clear-table-layout-mode",
    "clear-table-style-id",
    "clear-table-style-look",
    "clear-table-cell-spacing",
    "clear-table-default-cell-margin",
    "clear-table-border",
    "remove-table-row",
    "remove-table-column",
    "remove-table"
)

for ($index = 0; $index -lt $expectedCommands.Count; ++$index) {
    Assert-Equal -Actual $summary.operations[$index].command -Expected $expectedCommands[$index] `
        -Message "Table-alias operation $index used the wrong command."
}

Assert-Equal -Actual $summary.operations[0].op -Expected "set_table_col_width" `
    -Message "Column-width set alias should be recorded."
Assert-Equal -Actual $summary.operations[1].op -Expected "clear_table_col_width" `
    -Message "Column-width clear alias should be recorded."
Assert-Equal -Actual $summary.operations[11].op -Expected "delete_table_row" `
    -Message "Row delete alias should be recorded."
Assert-Equal -Actual $summary.operations[12].op -Expected "delete_table_column" `
    -Message "Column delete alias should be recorded."
Assert-Equal -Actual $summary.operations[13].op -Expected "delete_table" `
    -Message "Table delete alias should be recorded."

Assert-Equal -Actual $document.SelectNodes("//w:tbl", $namespaceManager).Count -Expected 3 `
    -Message "Table delete alias should remove the final body table."
Assert-ContainsText -Text $documentXml -ExpectedText "Before table aliases" -Label "Table-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "After table aliases" -Label "Table-alias document.xml"

Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblGrid/w:gridCol[2][@w:w]" `
    -Message "Column-width clear alias should remove the grid column width."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tr/w:tc[2]/w:tcPr/w:tcW" `
    -Message "Column-width clear alias should remove direct cell widths."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblW" `
    -Message "Clear-table-width alias should remove table width."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:jc" `
    -Message "Clear-table-alignment alias should remove table alignment."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblInd" `
    -Message "Clear-table-indent alias should remove table indent."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblLayout" `
    -Message "Clear-table-layout-mode alias should remove layout mode."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblStyle" `
    -Message "Clear-table-style-id alias should remove table style id."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblLook" `
    -Message "Clear-table-style-look alias should remove style look flags."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblCellSpacing" `
    -Message "Clear-table-cell-spacing alias should remove table cell spacing."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblCellMar/w:left" `
    -Message "Clear-table-default-cell-margin alias should remove the left margin."
Assert-NoDocxXPath -Document $document -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblBorders/w:insideH" `
    -Message "Clear-table-border alias should remove the inside horizontal border."

Assert-Equal -Actual $document.SelectNodes("//w:tbl[2]/w:tr", $namespaceManager).Count -Expected 2 `
    -Message "Row delete alias should remove exactly one row."
Assert-ContainsText -Text $documentXml -ExpectedText "delete-row-keep-top-a" -Label "Table-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "delete-row-keep-bottom-a" -Label "Table-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "delete-row-target-a" -Label "Table-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "delete-row-target-b" -Label "Table-alias document.xml"

Assert-Equal -Actual $document.SelectNodes("//w:tbl[3]/w:tr[1]/w:tc", $namespaceManager).Count -Expected 2 `
    -Message "Column delete alias should leave two cells in the first row."
Assert-Equal -Actual $document.SelectNodes("//w:tbl[3]/w:tr[2]/w:tc", $namespaceManager).Count -Expected 2 `
    -Message "Column delete alias should leave two cells in the second row."
Assert-ContainsText -Text $documentXml -ExpectedText "col-keep-left-1" -Label "Table-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "col-keep-right-2" -Label "Table-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "delete-col-target-1" -Label "Table-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "delete-col-target-2" -Label "Table-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "delete-table-target" -Label "Table-alias document.xml"

Write-Host "Edit-plan table aliases passed."
