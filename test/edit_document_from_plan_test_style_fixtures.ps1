function New-RangeFixtureDocx {
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
    <w:p><w:r><w:t>Alpha </w:t></w:r><w:r><w:t>Beta</w:t></w:r><w:r><w:t> Gamma</w:t></w:r></w:p>
    <w:p><w:r><w:t>Middle </w:t></w:r><w:r><w:t>Text</w:t></w:r></w:p>
    <w:p><w:r><w:t>Gamma</w:t></w:r><w:r><w:t>Delta</w:t></w:r></w:p>
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

function New-StyleListFixtureDocx {
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
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
  <Override PartName="/word/numbering.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.numbering+xml"/>
</Types>
'@
    $documentRelationshipsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
  <Relationship Id="rId2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/numbering"
                Target="numbering.xml"/>
</Relationships>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:r><w:t>Paragraph style set</w:t></w:r>
    </w:p>
    <w:p>
      <w:pPr><w:pStyle w:val="LegacyBody"/></w:pPr>
      <w:r><w:t>Paragraph style clear</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Run style set</w:t></w:r>
      <w:r><w:rPr><w:rStyle w:val="LegacyStrong"/></w:rPr><w:t>Run style clear</w:t></w:r>
      <w:r><w:t>Font family set</w:t></w:r>
      <w:r><w:rPr><w:rFonts w:ascii="Arial" w:hAnsi="Arial"/></w:rPr><w:t>Font family clear</w:t></w:r>
      <w:r><w:t>Language set</w:t></w:r>
      <w:r><w:rPr><w:lang w:val="fr-FR"/></w:rPr><w:t>Language clear</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>List set</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>List restart</w:t></w:r>
    </w:p>
    <w:p>
      <w:r><w:t>Numbering set</w:t></w:r>
    </w:p>
    <w:p>
      <w:pPr>
        <w:numPr>
          <w:ilvl w:val="0"/>
          <w:numId w:val="8"/>
        </w:numPr>
      </w:pPr>
      <w:r><w:t>List clear</w:t></w:r>
    </w:p>
    <w:sectPr>
      <w:pgSz w:w="12240" w:h="15840"/>
      <w:pgMar w:top="1440" w:right="1440" w:bottom="1440" w:left="1440" w:header="720" w:footer="720" w:gutter="0"/>
    </w:sectPr>
  </w:body>
</w:document>
'@
    $stylesXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:docDefaults>
    <w:rPrDefault>
      <w:rPr>
        <w:rFonts w:ascii="Calibri" w:hAnsi="Calibri" w:eastAsia="SimSun"/>
        <w:lang w:val="en-GB" w:eastAsia="zh-TW" w:bidi="fa-IR"/>
        <w:rtl w:val="1"/>
      </w:rPr>
    </w:rPrDefault>
    <w:pPrDefault>
      <w:pPr>
        <w:bidi w:val="1"/>
      </w:pPr>
    </w:pPrDefault>
  </w:docDefaults>
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="Heading1">
    <w:name w:val="Heading 1"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="LegacyBody">
    <w:name w:val="Legacy Body"/>
    <w:basedOn w:val="Normal"/>
    <w:next w:val="Heading1"/>
    <w:pPr>
      <w:outlineLvl w:val="3"/>
      <w:numPr>
        <w:ilvl w:val="0"/>
        <w:numId w:val="8"/>
      </w:numPr>
      <w:bidi w:val="1"/>
    </w:pPr>
    <w:rPr>
      <w:rFonts w:ascii="Legacy Sans" w:hAnsi="Legacy Sans" w:eastAsia="SimSun"/>
      <w:lang w:val="fr-FR" w:eastAsia="ja-JP" w:bidi="he-IL"/>
      <w:rtl w:val="1"/>
    </w:rPr>
  </w:style>
  <w:style w:type="character" w:styleId="Strong">
    <w:name w:val="Strong"/>
  </w:style>
  <w:style w:type="character" w:styleId="LegacyStrong">
    <w:name w:val="Legacy Strong"/>
  </w:style>
</w:styles>
'@
    $numberingXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:numbering xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:abstractNum w:abstractNumId="1">
    <w:multiLevelType w:val="singleLevel"/>
    <w:lvl w:ilvl="0">
      <w:start w:val="1"/>
      <w:numFmt w:val="decimal"/>
      <w:lvlText w:val="%1."/>
    </w:lvl>
  </w:abstractNum>
  <w:num w:numId="7">
    <w:abstractNumId w:val="1"/>
  </w:num>
  <w:num w:numId="8">
    <w:abstractNumId w:val="1"/>
  </w:num>
</w:numbering>
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
            Add-ZipTextEntry -Archive $archive -EntryName "word/_rels/document.xml.rels" -Content $documentRelationshipsXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/styles.xml" -Content $stylesXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/numbering.xml" -Content $numberingXml
        } finally {
            $archive.Dispose()
        }
    } finally {
        $fileStream.Dispose()
    }
}

function New-StyleRefactorFixtureDocx {
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
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>
'@
    $documentRelationshipsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles"
                Target="styles.xml"/>
</Relationships>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:pPr><w:pStyle w:val="LegacyBody"/></w:pPr>
      <w:r><w:t>Legacy styled paragraph</w:t></w:r>
    </w:p>
    <w:p>
      <w:pPr><w:pStyle w:val="MergeSource"/></w:pPr>
      <w:r><w:t>Merge styled paragraph</w:t></w:r>
    </w:p>
    <w:p>
      <w:pPr><w:pStyle w:val="BulkOld"/></w:pPr>
      <w:r><w:t>Bulk renamed paragraph</w:t></w:r>
    </w:p>
    <w:p>
      <w:pPr><w:pStyle w:val="BulkMergeSource"/></w:pPr>
      <w:r><w:t>Bulk merged paragraph</w:t></w:r>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
'@
    $stylesXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="LegacyBody">
    <w:name w:val="Legacy Body"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="MergeSource">
    <w:name w:val="Merge Source"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="MergeTarget">
    <w:name w:val="Merge Target"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="BulkOld">
    <w:name w:val="Bulk Old"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="BulkMergeSource">
    <w:name w:val="Bulk Merge Source"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:styleId="BulkMergeTarget">
    <w:name w:val="Bulk Merge Target"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="paragraph" w:customStyle="1" w:styleId="UnusedBody">
    <w:name w:val="Unused Body"/>
    <w:basedOn w:val="Normal"/>
  </w:style>
  <w:style w:type="character" w:customStyle="1" w:styleId="UnusedCharacter">
    <w:name w:val="Unused Character"/>
  </w:style>
</w:styles>
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
            Add-ZipTextEntry -Archive $archive -EntryName "word/_rels/document.xml.rels" -Content $documentRelationshipsXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/styles.xml" -Content $stylesXml
        } finally {
            $archive.Dispose()
        }
    } finally {
        $fileStream.Dispose()
    }
}
