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

function Assert-DocxXPath {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$XPath,
        [string]$Message
    )

    if ($null -eq $Document.SelectSingleNode($XPath, $NamespaceManager)) {
        throw "$Message XPath='$XPath'."
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

function New-MinimalDocx {
    param(
        [string]$Path,
        [string]$DocumentXml
    )

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
            Add-ZipTextEntry -Archive $archive -EntryName "word/document.xml" -Content $DocumentXml
        } finally {
            $archive.Dispose()
        }
    } finally {
        $fileStream.Dispose()
    }
}

function Read-DocxDocument {
    param([string]$DocxPath)

    $documentXml = Read-DocxEntryText -DocxPath $DocxPath -EntryName "word/document.xml"
    $document = New-Object System.Xml.XmlDocument
    $document.PreserveWhitespace = $true
    $document.LoadXml($documentXml)
    return [pscustomobject]@{
        Text = $documentXml
        Xml = $document
        NamespaceManager = New-WordNamespaceManager -Document $document
    }
}

function Write-EditPlan {
    param(
        [string]$Path,
        [string]$Json
    )

    Set-Content -LiteralPath $Path -Encoding UTF8 -Value $Json
}

function Invoke-EditPlan {
    param(
        [string]$InputDocx,
        [string]$PlanPath,
        [string]$OutputDocx,
        [string]$SummaryPath
    )

    & $scriptPath `
        -InputDocx $InputDocx `
        -EditPlan $PlanPath `
        -OutputDocx $OutputDocx `
        -SummaryJson $SummaryPath `
        -BuildDir $resolvedBuildDir `
        -SkipBuild

    if ($LASTEXITCODE -ne 0) {
        throw "edit_document_from_plan.ps1 failed for $PlanPath."
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryPath | ConvertFrom-Json
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build-msvc-nmake"
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_table_structure_aliases"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$insertSourceDocx = Join-Path $resolvedWorkingDir "insert_before.source.docx"
$insertPlanPath = Join-Path $resolvedWorkingDir "insert_before.plan.json"
$insertOutputDocx = Join-Path $resolvedWorkingDir "insert_before.edited.docx"
$insertSummaryPath = Join-Path $resolvedWorkingDir "insert_before.summary.json"

New-MinimalDocx -Path $insertSourceDocx -DocumentXml @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid><w:gridCol w:w="2400"/></w:tblGrid>
      <w:tr><w:tc><w:p><w:r><w:t>insert-anchor-cell</w:t></w:r></w:p></w:tc></w:tr>
    </w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>
'@
Write-EditPlan -Path $insertPlanPath -Json @'
{
  "operations": [
    {
      "op": "insert_table_before",
      "table_index": 0,
      "rows": 1,
      "columns": 2
    }
  ]
}
'@

$insertSummary = Invoke-EditPlan `
    -InputDocx $insertSourceDocx `
    -PlanPath $insertPlanPath `
    -OutputDocx $insertOutputDocx `
    -SummaryPath $insertSummaryPath
$insertResult = Read-DocxDocument -DocxPath $insertOutputDocx

Assert-Equal -Actual $insertSummary.status -Expected "completed" `
    -Message "Insert-table-before summary did not report status=completed."
Assert-Equal -Actual $insertSummary.operation_count -Expected 1 `
    -Message "Insert-table-before summary should record one operation."
Assert-Equal -Actual $insertSummary.operations[0].op -Expected "insert_table_before" `
    -Message "Insert-table-before operation should be recorded."
Assert-Equal -Actual $insertSummary.operations[0].command -Expected "insert-table-before" `
    -Message "Insert-table-before alias should use the CLI insert command."
Assert-Equal -Actual $insertResult.Xml.SelectNodes("//w:tbl", $insertResult.NamespaceManager).Count -Expected 2 `
    -Message "Insert-table-before output should contain two tables."
Assert-DocxXPath -Document $insertResult.Xml -NamespaceManager $insertResult.NamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[2]" `
    -Message "Inserted table should contain the requested second cell."
Assert-DocxXPath -Document $insertResult.Xml -NamespaceManager $insertResult.NamespaceManager `
    -XPath "//w:tbl[2]//w:t[.='insert-anchor-cell']" `
    -Message "Original table should move after the inserted table."

$likeSourceDocx = Join-Path $resolvedWorkingDir "insert_like_before.source.docx"
$likePlanPath = Join-Path $resolvedWorkingDir "insert_like_before.plan.json"
$likeOutputDocx = Join-Path $resolvedWorkingDir "insert_like_before.edited.docx"
$likeSummaryPath = Join-Path $resolvedWorkingDir "insert_like_before.summary.json"

New-MinimalDocx -Path $likeSourceDocx -DocumentXml @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="TableGrid"/>
        <w:tblW w:w="6400" w:type="dxa"/>
      </w:tblPr>
      <w:tblGrid><w:gridCol w:w="1800"/><w:gridCol w:w="4600"/></w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>clone-source-a1</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>clone-source-b1</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:p><w:r><w:t>clone-source-a2</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>clone-source-b2</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>
'@
Write-EditPlan -Path $likePlanPath -Json @'
{
  "operations": [
    {
      "op": "insert_table_like_before",
      "table_index": 0
    }
  ]
}
'@

$likeSummary = Invoke-EditPlan `
    -InputDocx $likeSourceDocx `
    -PlanPath $likePlanPath `
    -OutputDocx $likeOutputDocx `
    -SummaryPath $likeSummaryPath
$likeResult = Read-DocxDocument -DocxPath $likeOutputDocx

Assert-Equal -Actual $likeSummary.status -Expected "completed" `
    -Message "Insert-table-like-before summary did not report status=completed."
Assert-Equal -Actual $likeSummary.operations[0].op -Expected "insert_table_like_before" `
    -Message "Insert-table-like-before operation should be recorded."
Assert-Equal -Actual $likeSummary.operations[0].command -Expected "insert-table-like-before" `
    -Message "Insert-table-like-before alias should use the CLI clone command."
Assert-Equal -Actual $likeResult.Xml.SelectNodes("//w:tbl", $likeResult.NamespaceManager).Count -Expected 2 `
    -Message "Insert-table-like-before output should contain cloned and original tables."
Assert-DocxXPath -Document $likeResult.Xml -NamespaceManager $likeResult.NamespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblStyle[@w:val='TableGrid']" `
    -Message "Cloned table should preserve the table style id."
Assert-DocxXPath -Document $likeResult.Xml -NamespaceManager $likeResult.NamespaceManager `
    -XPath "//w:tbl[1]/w:tblGrid/w:gridCol[2][@w:w='4600']" `
    -Message "Cloned table should preserve grid widths."
$nonEmptyCloneTextNodes = @(
    $likeResult.Xml.SelectNodes("//w:tbl[1]//w:t", $likeResult.NamespaceManager) |
        Where-Object { -not [string]::IsNullOrEmpty($_.InnerText) }
)
Assert-Equal -Actual $nonEmptyCloneTextNodes.Count -Expected 0 `
    -Message "Cloned table should clear copied cell text."
Assert-ContainsText -Text $likeResult.Text -ExpectedText "clone-source-a1" -Label "Insert-like output"

$mergeSourceDocx = Join-Path $resolvedWorkingDir "merge_roundtrip.source.docx"
$mergePlanPath = Join-Path $resolvedWorkingDir "merge_roundtrip.plan.json"
$mergeOutputDocx = Join-Path $resolvedWorkingDir "merge_roundtrip.edited.docx"
$mergeSummaryPath = Join-Path $resolvedWorkingDir "merge_roundtrip.summary.json"

New-MinimalDocx -Path $mergeSourceDocx -DocumentXml @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid><w:gridCol w:w="1600"/><w:gridCol w:w="1600"/><w:gridCol w:w="1600"/></w:tblGrid>
      <w:tr>
        <w:tc><w:p><w:r><w:t>merge-a</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>merge-b</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>merge-c</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>
'@
Write-EditPlan -Path $mergePlanPath -Json @'
{
  "operations": [
    {
      "op": "merge_table_cell",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "merge_direction": "right"
    },
    {
      "op": "unmerge_table_cells",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "direction": "right"
    }
  ]
}
'@

$mergeSummary = Invoke-EditPlan `
    -InputDocx $mergeSourceDocx `
    -PlanPath $mergePlanPath `
    -OutputDocx $mergeOutputDocx `
    -SummaryPath $mergeSummaryPath
$mergeResult = Read-DocxDocument -DocxPath $mergeOutputDocx

Assert-Equal -Actual $mergeSummary.status -Expected "completed" `
    -Message "Merge/unmerge summary did not report status=completed."
Assert-Equal -Actual $mergeSummary.operation_count -Expected 2 `
    -Message "Merge/unmerge summary should record two operations."
Assert-Equal -Actual $mergeSummary.operations[0].op -Expected "merge_table_cell" `
    -Message "Singular merge alias should be recorded."
Assert-Equal -Actual $mergeSummary.operations[0].command -Expected "merge-table-cells" `
    -Message "Singular merge alias should use the CLI merge command."
Assert-Equal -Actual $mergeSummary.operations[1].op -Expected "unmerge_table_cells" `
    -Message "Plural unmerge alias should be recorded."
Assert-Equal -Actual $mergeSummary.operations[1].command -Expected "unmerge-table-cells" `
    -Message "Plural unmerge alias should use the CLI unmerge command."
Assert-Equal -Actual $mergeResult.Xml.SelectNodes("//w:tbl[1]/w:tr[1]/w:tc", $mergeResult.NamespaceManager).Count -Expected 3 `
    -Message "Merge/unmerge round-trip should restore the cell count."
Assert-NoDocxXPath -Document $mergeResult.Xml -NamespaceManager $mergeResult.NamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:gridSpan" `
    -Message "Merge/unmerge round-trip should remove gridSpan."
Assert-ContainsText -Text $mergeResult.Text -ExpectedText "merge-a" -Label "Merge output"
Assert-ContainsText -Text $mergeResult.Text -ExpectedText "merge-c" -Label "Merge output"
Assert-NotContainsText -Text $mergeResult.Text -UnexpectedText "merge-b" -Label "Merge output"

$unmergeSourceDocx = Join-Path $resolvedWorkingDir "unmerge_single.source.docx"
$unmergePlanPath = Join-Path $resolvedWorkingDir "unmerge_single.plan.json"
$unmergeOutputDocx = Join-Path $resolvedWorkingDir "unmerge_single.edited.docx"
$unmergeSummaryPath = Join-Path $resolvedWorkingDir "unmerge_single.summary.json"

New-MinimalDocx -Path $unmergeSourceDocx -DocumentXml @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:tbl>
      <w:tblGrid><w:gridCol w:w="1600"/><w:gridCol w:w="1600"/><w:gridCol w:w="1600"/></w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr><w:gridSpan w:val="2"/></w:tcPr>
          <w:p><w:r><w:t>premerged-left</w:t></w:r></w:p>
        </w:tc>
        <w:tc><w:p><w:r><w:t>premerged-right</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>
'@
Write-EditPlan -Path $unmergePlanPath -Json @'
{
  "operations": [
    {
      "op": "unmerge_table_cell",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "merge_direction": "right"
    }
  ]
}
'@

$unmergeSummary = Invoke-EditPlan `
    -InputDocx $unmergeSourceDocx `
    -PlanPath $unmergePlanPath `
    -OutputDocx $unmergeOutputDocx `
    -SummaryPath $unmergeSummaryPath
$unmergeResult = Read-DocxDocument -DocxPath $unmergeOutputDocx

Assert-Equal -Actual $unmergeSummary.status -Expected "completed" `
    -Message "Singular unmerge summary did not report status=completed."
Assert-Equal -Actual $unmergeSummary.operations[0].op -Expected "unmerge_table_cell" `
    -Message "Singular unmerge alias should be recorded."
Assert-Equal -Actual $unmergeSummary.operations[0].command -Expected "unmerge-table-cells" `
    -Message "Singular unmerge alias should use the CLI unmerge command."
Assert-Equal -Actual $unmergeResult.Xml.SelectNodes("//w:tbl[1]/w:tr[1]/w:tc", $unmergeResult.NamespaceManager).Count -Expected 3 `
    -Message "Singular unmerge alias should restore the spanned cell."
Assert-NoDocxXPath -Document $unmergeResult.Xml -NamespaceManager $unmergeResult.NamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:gridSpan" `
    -Message "Singular unmerge alias should remove gridSpan."
Assert-ContainsText -Text $unmergeResult.Text -ExpectedText "premerged-left" -Label "Unmerge output"
Assert-ContainsText -Text $unmergeResult.Text -ExpectedText "premerged-right" -Label "Unmerge output"

Write-Host "Edit-plan table structure aliases passed."
