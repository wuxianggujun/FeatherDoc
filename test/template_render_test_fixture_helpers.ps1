Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function New-BookmarkBlockVisibilityFixtureDocx {
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
  <Default Extension="xml"
           ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>before</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="0" w:name="keep_block"/></w:p>
    <w:p><w:r><w:t>Keep me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="0"/></w:p>
    <w:p><w:r><w:t>middle</w:t></w:r></w:p>
    <w:p><w:bookmarkStart w:id="1" w:name="hide_block"/></w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblW w:w="4800" w:type="dxa"/>
      </w:tblPr>
      <w:tblGrid>
        <w:gridCol w:w="4800"/>
      </w:tblGrid>
      <w:tr>
        <w:tc>
          <w:tcPr><w:tcW w:w="4800" w:type="dxa"/></w:tcPr>
          <w:p><w:r><w:t>Secret Cell</w:t></w:r></w:p>
        </w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>Hide me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="1"/></w:p>
    <w:p><w:r><w:t>after</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
'@

    $fileStream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Create, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
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

function New-PartTemplateValidationFixtureDocx {
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
  <Override PartName="/word/header1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p><w:r><w:t>Header/Footer Template Validation Visual</w:t></w:r></w:p>
    <w:p><w:r><w:t>This page keeps a valid header template and an intentionally malformed footer template.</w:t></w:r></w:p>
    <w:sectPr>
      <w:headerReference w:type="default" r:id="rId2"/>
      <w:footerReference w:type="default" r:id="rId3"/>
    </w:sectPr>
  </w:body>
</w:document>
'@
    $documentRelationshipsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rId3"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
</Relationships>
'@
    $headerXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t xml:space="preserve">Header title: </w:t></w:r>
    <w:bookmarkStart w:id="0" w:name="header_title"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="0"/>
  </w:p>
  <w:p>
    <w:bookmarkStart w:id="1" w:name="header_note"/>
    <w:r><w:t>standalone header note</w:t></w:r>
    <w:bookmarkEnd w:id="1"/>
  </w:p>
  <w:tbl>
    <w:tblGrid>
      <w:gridCol w:w="2400"/>
      <w:gridCol w:w="2400"/>
    </w:tblGrid>
    <w:tr>
      <w:tc><w:p><w:r><w:t>Name</w:t></w:r></w:p></w:tc>
      <w:tc><w:p><w:r><w:t>Qty</w:t></w:r></w:p></w:tc>
    </w:tr>
    <w:tr>
      <w:tc>
        <w:p>
          <w:bookmarkStart w:id="2" w:name="header_rows"/>
          <w:r><w:t>row placeholder</w:t></w:r>
          <w:bookmarkEnd w:id="2"/>
        </w:p>
      </w:tc>
      <w:tc><w:p><w:r><w:t>0</w:t></w:r></w:p></w:tc>
    </w:tr>
  </w:tbl>
</w:hdr>
'@
    $footerXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p>
    <w:r><w:t xml:space="preserve">Footer company A: </w:t></w:r>
    <w:bookmarkStart w:id="3" w:name="footer_company"/>
    <w:r><w:t>placeholder A</w:t></w:r>
    <w:bookmarkEnd w:id="3"/>
  </w:p>
  <w:p>
    <w:r><w:t xml:space="preserve">Footer company B: </w:t></w:r>
    <w:bookmarkStart w:id="4" w:name="footer_company"/>
    <w:r><w:t>placeholder B</w:t></w:r>
    <w:bookmarkEnd w:id="4"/>
  </w:p>
  <w:p>
    <w:r><w:t xml:space="preserve">Summary: prefix </w:t></w:r>
    <w:bookmarkStart w:id="5" w:name="footer_summary"/>
    <w:r><w:t>placeholder</w:t></w:r>
    <w:bookmarkEnd w:id="5"/>
    <w:r><w:t xml:space="preserve"> suffix</w:t></w:r>
  </w:p>
</w:ftr>
'@

    $fileStream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Create, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    try {
        $archive = New-Object System.IO.Compression.ZipArchive($fileStream, [System.IO.Compression.ZipArchiveMode]::Create)
        try {
            Add-ZipTextEntry -Archive $archive -EntryName "_rels/.rels" -Content $relationshipsXml
            Add-ZipTextEntry -Archive $archive -EntryName "[Content_Types].xml" -Content $contentTypesXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/document.xml" -Content $documentXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/_rels/document.xml.rels" -Content $documentRelationshipsXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/header1.xml" -Content $headerXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/footer1.xml" -Content $footerXml
        } finally {
            $archive.Dispose()
        }
    } finally {
        $fileStream.Dispose()
    }
}
