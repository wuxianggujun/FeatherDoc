param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

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

function Convert-JsonEscapedText {
    param([string]$EscapedText)

    return ConvertFrom-Json -InputObject ('"' + $EscapedText + '"')
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

function Read-DocxEntryTextsMatching {
    param(
        [string]$DocxPath,
        [string]$EntryNamePattern
    )

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $texts = New-Object System.Collections.Generic.List[string]
        foreach ($entry in $archive.Entries) {
            if ($entry.FullName -notmatch $EntryNamePattern) {
                continue
            }

            $reader = New-Object System.IO.StreamReader($entry.Open())
            try {
                $texts.Add($reader.ReadToEnd()) | Out-Null
            } finally {
                $reader.Dispose()
            }
        }

        if ($texts.Count -eq 0) {
            throw "No entries matching '$EntryNamePattern' were found in $DocxPath."
        }

        return [string]::Join("`n", $texts.ToArray())
    } finally {
        $archive.Dispose()
    }
}

function Assert-DocxEntryExists {
    param(
        [string]$DocxPath,
        [string]$EntryName,
        [string]$Message
    )

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        if ($null -eq $archive.GetEntry($EntryName)) {
            throw $Message
        }
    } finally {
        $archive.Dispose()
    }
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

function New-ContentControlFixtureDocx {
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
  <Default Extension="png" ContentType="image/png"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
</Types>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:w14="http://schemas.microsoft.com/office/word/2010/wordml">
  <w:body>
    <w:p>
      <w:r><w:t>Order: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Order Number"/>
          <w:tag w:val="order_no"/>
          <w:id w:val="43"/>
          <w:showingPlcHdr/>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Summary"/>
        <w:tag w:val="summary"/>
        <w:id w:val="44"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Summary placeholder</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Item</w:t></w:r></w:p></w:tc>
        <w:tc><w:p><w:r><w:t>Status</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Line Items"/>
          <w:tag w:val="line_items"/>
          <w:id w:val="45"/>
        </w:sdtPr>
        <w:sdtContent>
          <w:tr>
            <w:tc><w:p><w:r><w:t>Template item</w:t></w:r></w:p></w:tc>
            <w:tc><w:p><w:r><w:t>Template status</w:t></w:r></w:p></w:tc>
          </w:tr>
        </w:sdtContent>
      </w:sdt>
    </w:tbl>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Metrics"/>
        <w:tag w:val="metrics"/>
        <w:id w:val="46"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Metrics placeholder</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:p>
      <w:r><w:t>Approved: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Approved"/>
          <w:tag w:val="approved"/>
          <w:id w:val="47"/>
          <w:lock w:val="sdtContentLocked"/>
          <w14:checkbox><w14:checked w14:val="1"/></w14:checkbox>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>checked</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:p>
      <w:r><w:t>Status: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Status"/>
          <w:tag w:val="status"/>
          <w:id w:val="48"/>
          <w:dropDownList>
            <w:listItem w:displayText="Draft" w:value="draft"/>
            <w:listItem w:displayText="Approved" w:value="approved"/>
          </w:dropDownList>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>Approved</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Due Date"/>
        <w:tag w:val="due_date"/>
        <w:id w:val="49"/>
        <w:date>
          <w:dateFormat w:val="yyyy-MM-dd"/>
          <w:lid w:val="en-US"/>
        </w:date>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>2026-05-01</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Logo"/>
        <w:tag w:val="logo"/>
        <w:id w:val="50"/>
      </w:sdtPr>
      <w:sdtContent>
        <w:p><w:r><w:t>Logo placeholder</w:t></w:r></w:p>
      </w:sdtContent>
    </w:sdt>
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

function New-BookmarkRichFixtureDocx {
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
    <w:p><w:r><w:t>before rich bookmarks</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="0" w:name="metrics_table"/>
      <w:r><w:t>metrics table placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="0"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="optional_note"/>
      <w:r><w:t>optional note placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="2" w:name="inline_logo"/>
      <w:r><w:t>inline image placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="2"/>
    </w:p>
    <w:p>
      <w:bookmarkStart w:id="3" w:name="floating_logo"/>
      <w:r><w:t>floating image placeholder</w:t></w:r>
      <w:bookmarkEnd w:id="3"/>
    </w:p>
    <w:p><w:bookmarkStart w:id="4" w:name="keep_block"/></w:p>
    <w:p><w:r><w:t>Keep me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="4"/></w:p>
    <w:p><w:bookmarkStart w:id="5" w:name="hide_block"/></w:p>
    <w:tbl>
      <w:tr>
        <w:tc><w:p><w:r><w:t>Secret Cell</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:p><w:r><w:t>Hide me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="5"/></w:p>
    <w:p><w:bookmarkStart w:id="6" w:name="hide_batch_block"/></w:p>
    <w:p><w:r><w:t>Batch hide me</w:t></w:r></w:p>
    <w:p><w:bookmarkEnd w:id="6"/></w:p>
    <w:p><w:r><w:t>after rich bookmarks</w:t></w:r></w:p>
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

function New-LinkFormulaFixtureDocx {
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
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math">
  <w:body>
    <w:p><w:r><w:t>Link and formula source</w:t></w:r></w:p>
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

function New-RunRevisionFixtureDocx {
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
    <w:p>
      <w:r><w:t>Left </w:t></w:r>
      <w:r><w:t>Middle</w:t></w:r>
      <w:r><w:t> Right</w:t></w:r>
    </w:p>
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

function New-ReviewFixtureDocx {
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
  <Override PartName="/word/comments.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.comments+xml"/>
</Types>
'@
    $documentRelationshipsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rComments"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/comments"
                Target="comments.xml"/>
</Relationships>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p>
      <w:commentRangeStart w:id="4"/>
      <w:r><w:t>Commented text</w:t></w:r>
      <w:commentRangeEnd w:id="4"/>
      <w:r><w:commentReference w:id="4"/></w:r>
    </w:p>
    <w:p>
      <w:ins w:id="5" w:author="Ada" w:date="2026-04-01T00:00:00Z">
        <w:r><w:t>Inserted review text</w:t></w:r>
      </w:ins>
    </w:p>
    <w:p>
      <w:del w:id="6" w:author="Ada" w:date="2026-04-02T00:00:00Z">
        <w:r><w:delText>Deleted review text</w:delText></w:r>
      </w:del>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
'@
    $commentsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:comments xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:comment w:id="4" w:author="Reviewer" w:initials="RV" w:date="2026-04-03T00:00:00Z">
    <w:p><w:r><w:t>Reviewer note</w:t></w:r></w:p>
  </w:comment>
</w:comments>
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
            Add-ZipTextEntry -Archive $archive -EntryName "word/comments.xml" -Content $commentsXml
        } finally {
            $archive.Dispose()
        }
    } finally {
        $fileStream.Dispose()
    }
}

function New-SectionPartsFixtureDocx {
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
  <Override PartName="/word/header2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.header+xml"/>
  <Override PartName="/word/footer1.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
  <Override PartName="/word/footer2.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.footer+xml"/>
</Types>
'@
    $documentRelationshipsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rHeader1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header1.xml"/>
  <Relationship Id="rHeader2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/header"
                Target="header2.xml"/>
  <Relationship Id="rFooter1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer1.xml"/>
  <Relationship Id="rFooter2"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/footer"
                Target="footer2.xml"/>
</Relationships>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <w:body>
    <w:p>
      <w:r><w:t>section 0 body</w:t></w:r>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rHeader1"/>
          <w:footerReference w:type="default" r:id="rFooter1"/>
        </w:sectPr>
      </w:pPr>
    </w:p>
    <w:p>
      <w:r><w:t>section 1 body</w:t></w:r>
      <w:pPr>
        <w:sectPr>
          <w:headerReference w:type="default" r:id="rHeader2"/>
          <w:footerReference w:type="first" r:id="rFooter2"/>
        </w:sectPr>
      </w:pPr>
    </w:p>
    <w:p><w:r><w:t>section 2 body</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
'@
    $header1Xml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 0 header</w:t></w:r></w:p>
</w:hdr>
'@
    $header2Xml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:hdr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 header</w:t></w:r></w:p>
</w:hdr>
'@
    $footer1Xml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 0 footer</w:t></w:r></w:p>
</w:ftr>
'@
    $footer2Xml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:ftr xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:p><w:r><w:t>section 1 first footer</w:t></w:r></w:p>
</w:ftr>
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
            Add-ZipTextEntry -Archive $archive -EntryName "word/header1.xml" -Content $header1Xml
            Add-ZipTextEntry -Archive $archive -EntryName "word/header2.xml" -Content $header2Xml
            Add-ZipTextEntry -Archive $archive -EntryName "word/footer1.xml" -Content $footer1Xml
            Add-ZipTextEntry -Archive $archive -EntryName "word/footer2.xml" -Content $footer2Xml
        } finally {
            $archive.Dispose()
        }
    } finally {
        $fileStream.Dispose()
    }
}

function New-WordNamespaceManager {
    param([System.Xml.XmlDocument]$Document)

    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($Document.NameTable)
    $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")
    Write-Output -NoEnumerate $namespaceManager
}

function Assert-DocxXPath {
    param(
        [System.Xml.XmlDocument]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$XPath,
        [string]$Message
    )

    if ($null -eq $Document.SelectSingleNode($XPath, $NamespaceManager)) {
        throw $Message
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$editPlanPath = Join-Path $resolvedWorkingDir "invoice.edit_plan.json"
$editedDocx = Join-Path $resolvedWorkingDir "invoice.edited.docx"
$summaryPath = Join-Path $resolvedWorkingDir "invoice.edit.summary.json"

$expectedCustomerName = Convert-JsonEscapedText "\u4e0a\u6d77\u7fbd\u6587\u6863\u79d1\u6280\u6709\u9650\u516c\u53f8"
$expectedInvoiceNumber = Convert-JsonEscapedText "\u7f16\u8f91\u8ba1\u5212-2026-0424"
$expectedIssueDate = Convert-JsonEscapedText "2026\u5e744\u670824\u65e5"
$expectedNotePrefix = Convert-JsonEscapedText "\u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX"
$expectedTableItem = Convert-JsonEscapedText "\u8868\u683c\u4fee\u6539"
$expectedReplacementText = Convert-JsonEscapedText "JSON \u53ef\u590d\u7528\u5e76\u53ef\u5ba1\u9605"
$unexpectedReplacementText = Convert-JsonEscapedText "JSON \u53ef\u4ee5\u590d\u7528"
$deletedParagraphText = Convert-JsonEscapedText "\u4e34\u65f6\u8bf4\u660e\u4f1a\u88ab\u5220\u9664"

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "replace_bookmark_text",
      "bookmark": "customer_name",
      "text": "\u4e0a\u6d77\u7fbd\u6587\u6863\u79d1\u6280\u6709\u9650\u516c\u53f8"
    },
    {
      "op": "replace_bookmark_text",
      "bookmark": "invoice_number",
      "text": "\u7f16\u8f91\u8ba1\u5212-2026-0424"
    },
    {
      "op": "replace_bookmark_text",
      "bookmark": "issue_date",
      "text": "2026\u5e744\u670824\u65e5"
    },
    {
      "op": "replace_bookmark_paragraphs",
      "bookmark": "note_lines",
      "paragraphs": [
        "1. \u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX\u3002",
        "2. \u4fee\u6539\u8ba1\u5212 JSON \u53ef\u4ee5\u590d\u7528\u3002",
        "3. \u4e34\u65f6\u8bf4\u660e\u4f1a\u88ab\u5220\u9664\u3002"
      ]
    },
    {
      "op": "replace_bookmark_table_rows",
      "bookmark": "line_item_row",
      "rows": [
        [
          "\u6587\u672c\u66ff\u6362",
          "\u6309\u4e66\u7b7e\u66ff\u6362\u6b63\u6587\u4e2d\u7684\u4e1a\u52a1\u5b57\u6bb5",
          "1,200.00"
        ],
        [
          "\u8868\u683c\u4fee\u6539",
          "\u6309\u4e66\u7b7e\u66ff\u6362\u6a21\u677f\u8868\u683c\u884c",
          "2,800.00"
        ]
      ]
    },
    {
      "op": "set_table_cell_text",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "text": "4,000.00"
    },
    {
      "op": "set_table_cell_fill",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "background_color": "FFF2CC"
    },
    {
      "op": "set_table_cell_border",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "edge": "right",
      "style": "single",
      "thickness": 16,
      "color": "FF0000"
    },
    {
      "op": "set_table_row_height",
      "table_index": 0,
      "row_index": 3,
      "height_twips": 720,
      "rule": "exact"
    },
    {
      "op": "set_table_cell_vertical_alignment",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "alignment": "center"
    },
    {
      "op": "set_table_cell_horizontal_alignment",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 2,
      "alignment": "right"
    },
    {
      "op": "replace_text",
      "find": "JSON \u53ef\u4ee5\u590d\u7528",
      "replace": "JSON \u53ef\u590d\u7528\u5e76\u53ef\u5ba1\u9605"
    },
    {
      "op": "set_text_style",
      "text_contains": "JSON \u53ef\u590d\u7528\u5e76\u53ef\u5ba1\u9605",
      "bold": true,
      "font_family": "Segoe UI",
      "east_asia_font_family": "Microsoft YaHei",
      "language": "en-US",
      "east_asia_language": "zh-CN",
      "color": "C00000",
      "font_size_points": 14
    },
    {
      "op": "delete_paragraph_contains",
      "text_contains": "\u4e34\u65f6\u8bf4\u660e"
    },
    {
      "op": "set_paragraph_horizontal_alignment",
      "text_contains": "\u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX",
      "alignment": "center"
    },
    {
      "op": "set_paragraph_spacing",
      "text_contains": "\u76f4\u63a5\u7f16\u8f91\u5df2\u6709 DOCX",
      "before_twips": 120,
      "after_twips": 240,
      "line_twips": 360,
      "line_rule": "exact"
    },
    {
      "op": "set_table_column_width",
      "table_index": 0,
      "column_index": 1,
      "width_twips": 5200
    },
    {
      "op": "set_table_style_id",
      "table_index": 0,
      "style_id": "TableGrid"
    },
    {
      "op": "set_table_style_look",
      "table_index": 0,
      "first_row": false,
      "last_row": true,
      "first_column": false,
      "last_column": true,
      "banded_rows": false,
      "banded_columns": true
    },
    {
      "op": "set_table_width",
      "table_index": 0,
      "width_twips": 7200
    },
    {
      "op": "set_table_layout_mode",
      "table_index": 0,
      "layout_mode": "fixed"
    },
    {
      "op": "set_table_alignment",
      "table_index": 0,
      "alignment": "center"
    },
    {
      "op": "set_table_indent",
      "table_index": 0,
      "indent_twips": 360
    },
    {
      "op": "set_table_cell_spacing",
      "table_index": 0,
      "cell_spacing_twips": 180
    },
    {
      "op": "set_table_default_cell_margin",
      "table_index": 0,
      "edge": "left",
      "margin_twips": 240
    },
    {
      "op": "set_table_border",
      "table_index": 0,
      "edge": "inside_horizontal",
      "style": "dashed",
      "size": 8,
      "color": "00AA00",
      "space": 2
    },
    {
      "op": "merge_table_cells",
      "table_index": 0,
      "row_index": 3,
      "cell_index": 0,
      "direction": "right"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $editPlanPath `
    -OutputDocx $editedDocx `
    -SummaryJson $summaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $editedDocx) `
    -Message "Edited DOCX was not created."
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Edit summary JSON was not created."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $editedDocx -EntryName "word/document.xml"
$document = New-Object System.Xml.XmlDocument
$document.PreserveWhitespace = $true
$document.LoadXml($documentXml)
$namespaceManager = New-WordNamespaceManager -Document $document

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Edit summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 27 `
    -Message "Edit summary should record twenty-seven operations."
Assert-Equal -Actual $summary.operations[0].op -Expected "replace_bookmark_text" `
    -Message "First operation should be bookmark text replacement."
Assert-Equal -Actual $summary.operations[3].op -Expected "replace_bookmark_paragraphs" `
    -Message "Fourth operation should be bookmark paragraph replacement."
Assert-Equal -Actual $summary.operations[4].op -Expected "replace_bookmark_table_rows" `
    -Message "Fifth operation should be bookmark table-row replacement."
Assert-Equal -Actual $summary.operations[4].status -Expected "completed" `
    -Message "Table-row edit operation did not complete."
Assert-Equal -Actual $summary.operations[5].op -Expected "set_table_cell_text" `
    -Message "Sixth operation should be table-cell text replacement."
Assert-Equal -Actual $summary.operations[5].status -Expected "completed" `
    -Message "Table-cell edit operation did not complete."
Assert-Equal -Actual $summary.operations[6].op -Expected "set_table_cell_fill" `
    -Message "Seventh operation should set table-cell fill color."
Assert-Equal -Actual $summary.operations[6].command -Expected "set-table-cell-fill" `
    -Message "Table-cell fill should use the CLI fill command."
Assert-Equal -Actual $summary.operations[7].op -Expected "set_table_cell_border" `
    -Message "Eighth operation should set table-cell border."
Assert-Equal -Actual $summary.operations[7].command -Expected "set-table-cell-border" `
    -Message "Table-cell border should use the CLI border command."
Assert-Equal -Actual $summary.operations[8].op -Expected "set_table_row_height" `
    -Message "Ninth operation should be table-row height replacement."
Assert-Equal -Actual $summary.operations[9].op -Expected "set_table_cell_vertical_alignment" `
    -Message "Tenth operation should be table-cell vertical alignment."
Assert-Equal -Actual $summary.operations[10].op -Expected "set_table_cell_horizontal_alignment" `
    -Message "Eleventh operation should be table-cell horizontal alignment."
Assert-Equal -Actual $summary.operations[10].command -Expected "set-table-cell-horizontal-alignment" `
    -Message "Horizontal alignment should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[11].op -Expected "replace_text" `
    -Message "Twelfth operation should be ordinary text replacement."
Assert-Equal -Actual $summary.operations[11].command -Expected "replace-text" `
    -Message "Ordinary text replacement should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[12].op -Expected "set_text_style" `
    -Message "Thirteenth operation should style matching text."
Assert-Equal -Actual $summary.operations[12].command -Expected "set-text-style" `
    -Message "Text style should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[13].op -Expected "delete_paragraph_contains" `
    -Message "Fourteenth operation should delete a paragraph by content."
Assert-Equal -Actual $summary.operations[13].command -Expected "delete-paragraph" `
    -Message "Paragraph deletion should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[14].op -Expected "set_paragraph_horizontal_alignment" `
    -Message "Fifteenth operation should be paragraph horizontal alignment."
Assert-Equal -Actual $summary.operations[14].command -Expected "set-paragraph-horizontal-alignment" `
    -Message "Paragraph horizontal alignment should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[15].op -Expected "set_paragraph_spacing" `
    -Message "Sixteenth operation should be paragraph spacing."
Assert-Equal -Actual $summary.operations[15].command -Expected "set-paragraph-spacing" `
    -Message "Paragraph spacing should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[16].op -Expected "set_table_column_width" `
    -Message "Seventeenth operation should be table-column width."
Assert-Equal -Actual $summary.operations[16].command -Expected "set-table-column-width" `
    -Message "Table-column width should use the direct OOXML command record."
Assert-Equal -Actual $summary.operations[17].op -Expected "set_table_style_id" `
    -Message "Eighteenth operation should be table style id."
Assert-Equal -Actual $summary.operations[17].command -Expected "set-table-style-id" `
    -Message "Table style id should use the CLI table-style-id command."
Assert-Equal -Actual $summary.operations[18].op -Expected "set_table_style_look" `
    -Message "Nineteenth operation should be table style look."
Assert-Equal -Actual $summary.operations[18].command -Expected "set-table-style-look" `
    -Message "Table style look should use the CLI table-style-look command."
Assert-Equal -Actual $summary.operations[19].op -Expected "set_table_width" `
    -Message "Twentieth operation should be table width."
Assert-Equal -Actual $summary.operations[19].command -Expected "set-table-width" `
    -Message "Table width should use the CLI table-width command."
Assert-Equal -Actual $summary.operations[20].op -Expected "set_table_layout_mode" `
    -Message "Twenty-first operation should be table layout mode."
Assert-Equal -Actual $summary.operations[20].command -Expected "set-table-layout-mode" `
    -Message "Table layout mode should use the CLI layout-mode command."
Assert-Equal -Actual $summary.operations[21].op -Expected "set_table_alignment" `
    -Message "Twenty-second operation should be table alignment."
Assert-Equal -Actual $summary.operations[21].command -Expected "set-table-alignment" `
    -Message "Table alignment should use the CLI table-alignment command."
Assert-Equal -Actual $summary.operations[22].op -Expected "set_table_indent" `
    -Message "Twenty-third operation should be table indent."
Assert-Equal -Actual $summary.operations[22].command -Expected "set-table-indent" `
    -Message "Table indent should use the CLI table-indent command."
Assert-Equal -Actual $summary.operations[23].op -Expected "set_table_cell_spacing" `
    -Message "Twenty-fourth operation should be table cell spacing."
Assert-Equal -Actual $summary.operations[23].command -Expected "set-table-cell-spacing" `
    -Message "Table cell spacing should use the CLI cell-spacing command."
Assert-Equal -Actual $summary.operations[24].op -Expected "set_table_default_cell_margin" `
    -Message "Twenty-fifth operation should be table default cell margin."
Assert-Equal -Actual $summary.operations[24].command -Expected "set-table-default-cell-margin" `
    -Message "Table default cell margin should use the CLI table-default-cell-margin command."
Assert-Equal -Actual $summary.operations[25].op -Expected "set_table_border" `
    -Message "Twenty-sixth operation should be table border."
Assert-Equal -Actual $summary.operations[25].command -Expected "set-table-border" `
    -Message "Table border should use the CLI table-border command."
Assert-Equal -Actual $summary.operations[26].op -Expected "merge_table_cells" `
    -Message "Twenty-seventh operation should merge table cells."
Assert-Equal -Actual $summary.operations[26].command -Expected "merge-table-cells" `
    -Message "Table-cell merge should use the CLI merge command."

Assert-ContainsText -Text $documentXml -ExpectedText $expectedCustomerName -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedInvoiceNumber -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedIssueDate -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedNotePrefix -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedTableItem -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "4,000.00" -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText $expectedReplacementText -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:trHeight w:val="720" w:hRule="exact"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:vAlign w:val="center"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:jc w:val="right"' -Label "Edited document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:jc w:val="center"' -Label "Edited document.xml"
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tcPr/w:shd[@w:fill='FFF2CC']" `
    -Message "Edited document.xml should contain the table-cell fill color."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tcPr/w:tcBorders/w:right[@w:val='single' and @w:sz='16' and @w:color='FF0000']" `
    -Message "Edited document.xml should contain the right table-cell border."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:b[@w:val='1']" `
    -Message "Edited document.xml should contain bold text style."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:rFonts[@w:ascii='Segoe UI' and @w:hAnsi='Segoe UI' and @w:eastAsia='Microsoft YaHei']" `
    -Message "Edited document.xml should contain Latin and Chinese font settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:lang[@w:val='en-US' and @w:eastAsia='zh-CN']" `
    -Message "Edited document.xml should contain Latin and Chinese language settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:color[@w:val='C00000']" `
    -Message "Edited document.xml should contain text color settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:sz[@w:val='28']" `
    -Message "Edited document.xml should contain primary font-size settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:rPr/w:szCs[@w:val='28']" `
    -Message "Edited document.xml should contain complex-script font-size settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:pPr/w:spacing[@w:before='120' and @w:after='240' and @w:line='360' and @w:lineRule='exact']" `
    -Message "Edited document.xml should contain paragraph spacing settings."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblGrid/w:gridCol[2][@w:w='5200']" `
    -Message "Edited document.xml should contain the updated table-column grid width."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblStyle[@w:val='TableGrid']" `
    -Message "Edited document.xml should contain the table style id."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblLook[@w:val='0340' and @w:firstRow='0' and @w:lastRow='1' and @w:firstColumn='0' and @w:lastColumn='1' and @w:noHBand='1' and @w:noVBand='0']" `
    -Message "Edited document.xml should contain the table style-look flags."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tr/w:tc[2]/w:tcPr/w:tcW[@w:w='5200' and @w:type='dxa']" `
    -Message "Edited document.xml should contain table-cell widths for the updated column."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblW[@w:w='7200' and @w:type='dxa']" `
    -Message "Edited document.xml should contain the updated table width."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblLayout[@w:type='fixed']" `
    -Message "Edited document.xml should contain fixed table layout mode."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:jc[@w:val='center']" `
    -Message "Edited document.xml should contain centered table alignment."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblInd[@w:w='360' and @w:type='dxa']" `
    -Message "Edited document.xml should contain the table indent."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblCellSpacing[@w:w='180' and @w:type='dxa']" `
    -Message "Edited document.xml should contain table cell spacing."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblCellMar/w:left[@w:w='240' and @w:type='dxa']" `
    -Message "Edited document.xml should contain the table default cell margin."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblBorders/w:insideH[@w:val='dashed' and @w:sz='8' and @w:color='00AA00' and @w:space='2']" `
    -Message "Edited document.xml should contain the table inside horizontal border."
Assert-DocxXPath `
    -Document $document `
    -NamespaceManager $namespaceManager `
    -XPath "//w:tbl[1]/w:tr[4]/w:tc[1]/w:tcPr/w:gridSpan[@w:val='2']" `
    -Message "Edited document.xml should contain the horizontal table-cell merge."
Assert-NotContainsText -Text $documentXml -UnexpectedText "12,800.00" -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText $unexpectedReplacementText -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText $deletedParagraphText -Label "Edited document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "TODO:" -Label "Edited document.xml"

$contentControlSourceDocx = Join-Path $resolvedWorkingDir "content_control.source.docx"
$contentControlPlanPath = Join-Path $resolvedWorkingDir "content_control.edit_plan.json"
$contentControlEditedDocx = Join-Path $resolvedWorkingDir "content_control.edited.docx"
$contentControlSummaryPath = Join-Path $resolvedWorkingDir "content_control.edit.summary.json"
$contentControlImagePath = Join-Path $resolvedWorkingDir "content_control.logo.png"

New-ContentControlFixtureDocx -Path $contentControlSourceDocx
[System.IO.File]::WriteAllBytes(
    $contentControlImagePath,
    [System.Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"))
$escapedContentControlImagePath = $contentControlImagePath.Replace("\", "\\")

Set-Content -LiteralPath $contentControlPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "replace_content_control_text",
      "content_control_tag": "order_no",
      "text": "INV-002"
    },
    {
      "op": "replace_content_control_paragraphs",
      "content_control_tag": "summary",
      "paragraphs": [
        "Summary paragraph one",
        "Summary paragraph two"
      ]
    },
    {
      "op": "replace_content_control_table_rows",
      "content_control_tag": "line_items",
      "rows": [
        ["SKU-1", "Ready"],
        ["SKU-2", "Queued"]
      ]
    },
    {
      "op": "replace_content_control_table",
      "content_control_alias": "Metrics",
      "rows": [
        ["Metric", "Value"],
        ["Quality", "Pass"]
      ]
    },
    {
      "op": "set_content_control_form_state",
      "content_control_tag": "approved",
      "checked": false,
      "clear_lock": true
    },
    {
      "op": "set_content_control_form_state",
      "content_control_alias": "Status",
      "selected_item": "draft",
      "lock": "sdtLocked"
    },
    {
      "op": "set_content_control_form_state",
      "content_control_tag": "due_date",
      "date_text": "2026-06-01",
      "date_format": "yyyy/MM/dd",
      "date_locale": "zh-CN"
    },
    {
      "op": "replace_content_control_image",
      "content_control_tag": "logo",
      "image_path": "$escapedContentControlImagePath",
      "width": 24,
      "height": 24
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $contentControlSourceDocx `
    -EditPlan $contentControlPlanPath `
    -OutputDocx $contentControlEditedDocx `
    -SummaryJson $contentControlSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the content-control edit plan."
}

$contentControlSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $contentControlSummaryPath | ConvertFrom-Json
$contentControlXml = Read-DocxEntryText -DocxPath $contentControlEditedDocx -EntryName "word/document.xml"
$contentControlDocument = New-Object System.Xml.XmlDocument
$contentControlDocument.PreserveWhitespace = $true
$contentControlDocument.LoadXml($contentControlXml)
$contentControlNamespaceManager = New-WordNamespaceManager -Document $contentControlDocument

Assert-Equal -Actual $contentControlSummary.status -Expected "completed" `
    -Message "Content-control summary did not report status=completed."
Assert-Equal -Actual $contentControlSummary.operation_count -Expected 8 `
    -Message "Content-control summary should record eight operations."
Assert-Equal -Actual $contentControlSummary.operations[0].command -Expected "replace-content-control-text" `
    -Message "Content-control text edit should use the CLI text command."
Assert-Equal -Actual $contentControlSummary.operations[1].command -Expected "replace-content-control-paragraphs" `
    -Message "Content-control paragraph edit should use the CLI paragraph command."
Assert-Equal -Actual $contentControlSummary.operations[2].command -Expected "replace-content-control-table-rows" `
    -Message "Content-control table-row edit should use the CLI table-row command."
Assert-Equal -Actual $contentControlSummary.operations[3].command -Expected "replace-content-control-table" `
    -Message "Content-control table edit should use the CLI table command."
Assert-Equal -Actual $contentControlSummary.operations[4].command -Expected "set-content-control-form-state" `
    -Message "Content-control checkbox edit should use the CLI form-state command."
Assert-Equal -Actual $contentControlSummary.operations[7].command -Expected "replace-content-control-image" `
    -Message "Content-control image edit should use the CLI image command."
Assert-ContainsText -Text $contentControlXml -ExpectedText "INV-002" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Summary paragraph one" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Summary paragraph two" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "SKU-2" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Queued" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Metric" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "Quality" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText 'w14:checked w14:val="0"' -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText '<w:t>Draft</w:t>' -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText "2026-06-01" -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText 'w:dateFormat w:val="yyyy/MM/dd"' -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText 'w:lid w:val="zh-CN"' -Label "Content-control document.xml"
Assert-ContainsText -Text $contentControlXml -ExpectedText 'w:lock w:val="sdtLocked"' -Label "Content-control document.xml"
Assert-DocxXPath `
    -Document $contentControlDocument `
    -NamespaceManager $contentControlNamespaceManager `
    -XPath "//w:drawing" `
    -Message "Content-control image replacement should add a drawing."
Assert-DocxEntryExists `
    -DocxPath $contentControlEditedDocx `
    -EntryName "word/media/image1.png" `
    -Message "Content-control image replacement should add image media."
Assert-NotContainsText -Text $contentControlXml -UnexpectedText "INV-001" -Label "Content-control document.xml"
Assert-NotContainsText -Text $contentControlXml -UnexpectedText "Summary placeholder" -Label "Content-control document.xml"
Assert-NotContainsText -Text $contentControlXml -UnexpectedText "Metrics placeholder" -Label "Content-control document.xml"
Assert-NotContainsText -Text $contentControlXml -UnexpectedText "Logo placeholder" -Label "Content-control document.xml"

$bookmarkRichSourceDocx = Join-Path $resolvedWorkingDir "bookmark_rich.source.docx"
$bookmarkRichPlanPath = Join-Path $resolvedWorkingDir "bookmark_rich.edit_plan.json"
$bookmarkRichEditedDocx = Join-Path $resolvedWorkingDir "bookmark_rich.edited.docx"
$bookmarkRichSummaryPath = Join-Path $resolvedWorkingDir "bookmark_rich.edit.summary.json"
$bookmarkRichImagePath = Join-Path $resolvedWorkingDir "bookmark_rich.logo.png"

New-BookmarkRichFixtureDocx -Path $bookmarkRichSourceDocx
[System.IO.File]::WriteAllBytes(
    $bookmarkRichImagePath,
    [System.Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"))
$escapedBookmarkRichImagePath = $bookmarkRichImagePath.Replace("\", "\\")

Set-Content -LiteralPath $bookmarkRichPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "replace_bookmark_table",
      "bookmark": "metrics_table",
      "rows": [
        ["Metric", "Value"],
        ["Quality", "Pass"]
      ]
    },
    {
      "op": "remove_bookmark_block",
      "bookmark": "optional_note"
    },
    {
      "op": "replace_bookmark_image",
      "bookmark": "inline_logo",
      "image_path": "$escapedBookmarkRichImagePath",
      "width": 24,
      "height": 24
    },
    {
      "op": "replace_bookmark_floating_image",
      "bookmark": "floating_logo",
      "image_path": "$escapedBookmarkRichImagePath",
      "width": 32,
      "height": 32,
      "horizontal_reference": "margin",
      "horizontal_offset": 18,
      "vertical_reference": "paragraph",
      "vertical_offset": 24,
      "behind_text": false,
      "allow_overlap": false,
      "z_order": 7,
      "wrap_mode": "square",
      "wrap_distance_left": 2,
      "wrap_distance_right": 2,
      "wrap_distance_top": 1,
      "wrap_distance_bottom": 1,
      "crop_left": 0,
      "crop_top": 0,
      "crop_right": 0,
      "crop_bottom": 0
    },
    {
      "op": "set_bookmark_block_visibility",
      "bookmark": "hide_block",
      "visible": false
    },
    {
      "op": "apply_bookmark_block_visibility",
      "show": "keep_block",
      "hide": "hide_batch_block"
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $bookmarkRichSourceDocx `
    -EditPlan $bookmarkRichPlanPath `
    -OutputDocx $bookmarkRichEditedDocx `
    -SummaryJson $bookmarkRichSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the bookmark rich edit plan."
}

$bookmarkRichSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $bookmarkRichSummaryPath | ConvertFrom-Json
$bookmarkRichXml = Read-DocxEntryText -DocxPath $bookmarkRichEditedDocx -EntryName "word/document.xml"
$bookmarkRichDocument = New-Object System.Xml.XmlDocument
$bookmarkRichDocument.PreserveWhitespace = $true
$bookmarkRichDocument.LoadXml($bookmarkRichXml)
$bookmarkRichNamespaceManager = New-WordNamespaceManager -Document $bookmarkRichDocument

Assert-Equal -Actual $bookmarkRichSummary.status -Expected "completed" `
    -Message "Bookmark-rich summary did not report status=completed."
Assert-Equal -Actual $bookmarkRichSummary.operation_count -Expected 6 `
    -Message "Bookmark-rich summary should record six operations."
Assert-Equal -Actual $bookmarkRichSummary.operations[0].command -Expected "replace-bookmark-table" `
    -Message "Bookmark table edit should use the CLI replace-bookmark-table command."
Assert-Equal -Actual $bookmarkRichSummary.operations[1].command -Expected "remove-bookmark-block" `
    -Message "Bookmark block removal should use the CLI remove-bookmark-block command."
Assert-Equal -Actual $bookmarkRichSummary.operations[2].command -Expected "replace-bookmark-image" `
    -Message "Bookmark image edit should use the CLI replace-bookmark-image command."
Assert-Equal -Actual $bookmarkRichSummary.operations[3].command -Expected "replace-bookmark-floating-image" `
    -Message "Bookmark floating-image edit should use the CLI replace-bookmark-floating-image command."
Assert-Equal -Actual $bookmarkRichSummary.operations[4].command -Expected "set-bookmark-block-visibility" `
    -Message "Bookmark block visibility edit should use the CLI set-bookmark-block-visibility command."
Assert-Equal -Actual $bookmarkRichSummary.operations[5].command -Expected "apply-bookmark-block-visibility" `
    -Message "Bookmark block visibility batch edit should use the CLI apply-bookmark-block-visibility command."
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "Metric" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "Quality" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "Keep me" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "wp:inline" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText "wp:anchor" -Label "Bookmark-rich document.xml"
Assert-ContainsText -Text $bookmarkRichXml -ExpectedText 'relativeHeight="7"' -Label "Bookmark-rich document.xml"
Assert-DocxXPath `
    -Document $bookmarkRichDocument `
    -NamespaceManager $bookmarkRichNamespaceManager `
    -XPath "//w:tbl[1]//w:t[.='Pass']" `
    -Message "Bookmark table replacement should write the generated table cells."
Assert-DocxEntryExists `
    -DocxPath $bookmarkRichEditedDocx `
    -EntryName "word/media/image1.png" `
    -Message "Bookmark image replacement should add the first image media."
Assert-DocxEntryExists `
    -DocxPath $bookmarkRichEditedDocx `
    -EntryName "word/media/image2.png" `
    -Message "Bookmark floating-image replacement should add the second image media."
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "metrics table placeholder" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "optional note placeholder" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "inline image placeholder" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "floating image placeholder" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "Secret Cell" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "Hide me" -Label "Bookmark-rich document.xml"
Assert-NotContainsText -Text $bookmarkRichXml -UnexpectedText "Batch hide me" -Label "Bookmark-rich document.xml"

$genericImageSourceDocx = Join-Path $resolvedWorkingDir "generic_image.source.docx"
$genericImagePlanPath = Join-Path $resolvedWorkingDir "generic_image.edit_plan.json"
$genericImageEditedDocx = Join-Path $resolvedWorkingDir "generic_image.edited.docx"
$genericImageSummaryPath = Join-Path $resolvedWorkingDir "generic_image.edit.summary.json"
$genericImageRemovePlanPath = Join-Path $resolvedWorkingDir "generic_image.remove_plan.json"
$genericImageRemovedDocx = Join-Path $resolvedWorkingDir "generic_image.removed.docx"
$genericImageRemoveSummaryPath = Join-Path $resolvedWorkingDir "generic_image.remove.summary.json"
$genericImagePath = Join-Path $resolvedWorkingDir "generic_image.initial.png"
$genericImageReplacementPath = Join-Path $resolvedWorkingDir "generic_image.replacement.gif"

New-BookmarkRichFixtureDocx -Path $genericImageSourceDocx
[System.IO.File]::WriteAllBytes(
    $genericImagePath,
    [System.Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"))
[System.IO.File]::WriteAllBytes(
    $genericImageReplacementPath,
    [System.Convert]::FromBase64String("R0lGODlhAQABAIABAP///wAAACwAAAAAAQABAAACAkQBADs="))
$escapedGenericImagePath = $genericImagePath.Replace("\", "\\")
$escapedGenericImageReplacementPath = $genericImageReplacementPath.Replace("\", "\\")

Set-Content -LiteralPath $genericImagePlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "append_image",
      "image_path": "$escapedGenericImagePath",
      "width": 16,
      "height": 12
    },
    {
      "op": "replace_image",
      "image_path": "$escapedGenericImageReplacementPath",
      "image_index": 0
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $genericImageSourceDocx `
    -EditPlan $genericImagePlanPath `
    -OutputDocx $genericImageEditedDocx `
    -SummaryJson $genericImageSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the generic image append/replace plan."
}

$genericImageSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $genericImageSummaryPath | ConvertFrom-Json
$genericImageXml = Read-DocxEntryText -DocxPath $genericImageEditedDocx -EntryName "word/document.xml"
$genericImageContentTypesXml = Read-DocxEntryText -DocxPath $genericImageEditedDocx -EntryName "[Content_Types].xml"
$genericImageRelsXml = Read-DocxEntryText -DocxPath $genericImageEditedDocx -EntryName "word/_rels/document.xml.rels"

Assert-Equal -Actual $genericImageSummary.status -Expected "completed" `
    -Message "Generic-image summary did not report status=completed."
Assert-Equal -Actual $genericImageSummary.operation_count -Expected 2 `
    -Message "Generic-image summary should record two operations."
Assert-Equal -Actual $genericImageSummary.operations[0].command -Expected "append-image" `
    -Message "Generic append_image should use the CLI append-image command."
Assert-Equal -Actual $genericImageSummary.operations[1].command -Expected "replace-image" `
    -Message "Generic replace_image should use the CLI replace-image command."
Assert-ContainsText -Text $genericImageXml -ExpectedText "wp:inline" -Label "Generic-image document.xml"
Assert-ContainsText -Text $genericImageContentTypesXml -ExpectedText "image/gif" -Label "Generic-image [Content_Types].xml"
Assert-ContainsText -Text $genericImageRelsXml -ExpectedText "media/image1.gif" -Label "Generic-image document.xml.rels"
Assert-DocxEntryExists `
    -DocxPath $genericImageEditedDocx `
    -EntryName "word/media/image1.gif" `
    -Message "Generic replace_image should replace the appended media with GIF data."

Set-Content -LiteralPath $genericImageRemovePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_image",
      "image_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $genericImageEditedDocx `
    -EditPlan $genericImageRemovePlanPath `
    -OutputDocx $genericImageRemovedDocx `
    -SummaryJson $genericImageRemoveSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the generic remove_image plan."
}

$genericImageRemoveSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $genericImageRemoveSummaryPath | ConvertFrom-Json
$genericImageRemovedXml = Read-DocxEntryText -DocxPath $genericImageRemovedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $genericImageRemoveSummary.status -Expected "completed" `
    -Message "Generic-image remove summary did not report status=completed."
Assert-Equal -Actual $genericImageRemoveSummary.operation_count -Expected 1 `
    -Message "Generic-image remove summary should record one operation."
Assert-Equal -Actual $genericImageRemoveSummary.operations[0].command -Expected "remove-image" `
    -Message "Generic remove_image should use the CLI remove-image command."
Assert-NotContainsText -Text $genericImageRemovedXml -UnexpectedText "w:drawing" -Label "Generic-image removed document.xml"

$fieldSourceDocx = Join-Path $resolvedWorkingDir "fields.source.docx"
$fieldPlanPath = Join-Path $resolvedWorkingDir "fields.edit_plan.json"
$fieldEditedDocx = Join-Path $resolvedWorkingDir "fields.edited.docx"
$fieldSummaryPath = Join-Path $resolvedWorkingDir "fields.edit.summary.json"

New-BookmarkRichFixtureDocx -Path $fieldSourceDocx

Set-Content -LiteralPath $fieldPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_page_number_field",
      "dirty": true,
      "locked": true
    },
    {
      "op": "append_total_pages_field",
      "part": "body"
    },
    {
      "op": "append_table_of_contents_field",
      "min_outline_level": 1,
      "max_outline_level": 2,
      "no_hyperlinks": true,
      "show_page_numbers_in_web_layout": true,
      "no_outline_levels": true,
      "result_text": "TOC placeholder"
    },
    {
      "op": "append_document_property_field",
      "property_name": "Title",
      "result_text": "FeatherDoc",
      "preserve_formatting": false
    },
    {
      "op": "append_date_field",
      "part": "body",
      "date_format": "yyyy-MM-dd",
      "text": "2026-05-13",
      "dirty": true
    },
    {
      "op": "set_update_fields_on_open",
      "enabled": true
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $fieldSourceDocx `
    -EditPlan $fieldPlanPath `
    -OutputDocx $fieldEditedDocx `
    -SummaryJson $fieldSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the fields edit plan."
}

$fieldSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $fieldSummaryPath | ConvertFrom-Json
$fieldXml = Read-DocxEntryText -DocxPath $fieldEditedDocx -EntryName "word/document.xml"
$fieldSettingsXml = Read-DocxEntryText -DocxPath $fieldEditedDocx -EntryName "word/settings.xml"

Assert-Equal -Actual $fieldSummary.status -Expected "completed" `
    -Message "Fields summary did not report status=completed."
Assert-Equal -Actual $fieldSummary.operation_count -Expected 6 `
    -Message "Fields summary should record six operations."
Assert-Equal -Actual $fieldSummary.operations[0].command -Expected "append-page-number-field" `
    -Message "Append-page-number-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[1].command -Expected "append-total-pages-field" `
    -Message "Append-total-pages-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[2].command -Expected "append-table-of-contents-field" `
    -Message "Append-table-of-contents-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[3].command -Expected "append-document-property-field" `
    -Message "Append-document-property-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[4].command -Expected "append-date-field" `
    -Message "Append-date-field should use the CLI field command."
Assert-Equal -Actual $fieldSummary.operations[5].command -Expected "set-update-fields-on-open" `
    -Message "Set-update-fields-on-open should use the CLI update-fields command."
Assert-ContainsText -Text $fieldXml -ExpectedText 'w:instr=" PAGE "' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'w:instr=" NUMPAGES "' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'TOC \o &quot;1-2&quot;' -Label "Fields document.xml"
Assert-NotContainsText -Text $fieldXml -UnexpectedText 'TOC \o &quot;1-2&quot; \h' -Label "Fields document.xml"
Assert-NotContainsText -Text $fieldXml -UnexpectedText 'TOC \o &quot;1-2&quot; \z' -Label "Fields document.xml"
Assert-NotContainsText -Text $fieldXml -UnexpectedText 'TOC \o &quot;1-2&quot; \u' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText "TOC placeholder" -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText "DOCPROPERTY Title" -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText "FeatherDoc" -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'DATE \@ &quot;yyyy-MM-dd&quot;' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText "2026-05-13" -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'w:dirty="true"' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldXml -ExpectedText 'w:fldLock="true"' -Label "Fields document.xml"
Assert-NotContainsText -Text $fieldXml -UnexpectedText 'DOCPROPERTY Title \* MERGEFORMAT' -Label "Fields document.xml"
Assert-ContainsText -Text $fieldSettingsXml -ExpectedText "<w:updateFields" -Label "Fields settings.xml"
Assert-ContainsText -Text $fieldSettingsXml -ExpectedText 'w:val="1"' -Label "Fields settings.xml"

$fieldClearPlanPath = Join-Path $resolvedWorkingDir "fields.clear_update_fields_plan.json"
$fieldClearedDocx = Join-Path $resolvedWorkingDir "fields.update_fields_cleared.docx"
$fieldClearSummaryPath = Join-Path $resolvedWorkingDir "fields.clear_update_fields.summary.json"

Set-Content -LiteralPath $fieldClearPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_update_fields_on_open"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $fieldEditedDocx `
    -EditPlan $fieldClearPlanPath `
    -OutputDocx $fieldClearedDocx `
    -SummaryJson $fieldClearSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the clear_update_fields_on_open plan."
}

$fieldClearSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $fieldClearSummaryPath | ConvertFrom-Json
$fieldClearedSettingsXml = Read-DocxEntryText -DocxPath $fieldClearedDocx -EntryName "word/settings.xml"

Assert-Equal -Actual $fieldClearSummary.status -Expected "completed" `
    -Message "Fields-clear summary did not report status=completed."
Assert-Equal -Actual $fieldClearSummary.operation_count -Expected 1 `
    -Message "Fields-clear summary should record one operation."
Assert-Equal -Actual $fieldClearSummary.operations[0].command -Expected "set-update-fields-on-open" `
    -Message "Clear-update-fields-on-open should use the CLI update-fields command."
Assert-NotContainsText -Text $fieldClearedSettingsXml -UnexpectedText "<w:updateFields" -Label "Fields cleared settings.xml"

$advancedFieldSourceDocx = Join-Path $resolvedWorkingDir "advanced_fields.source.docx"
$advancedFieldPlanPath = Join-Path $resolvedWorkingDir "advanced_fields.edit_plan.json"
$advancedFieldEditedDocx = Join-Path $resolvedWorkingDir "advanced_fields.edited.docx"
$advancedFieldSummaryPath = Join-Path $resolvedWorkingDir "advanced_fields.edit.summary.json"

New-BookmarkRichFixtureDocx -Path $advancedFieldSourceDocx
Set-Content -LiteralPath $advancedFieldPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_page_reference_field",
      "bookmark_name": "metrics_table",
      "result_text": "Plan page reference",
      "no_hyperlink": true,
      "relative_position": true,
      "preserve_formatting": false
    },
    {
      "op": "append_style_reference_field",
      "style_name": "Heading 1",
      "result_text": "Plan style reference",
      "paragraph_number": true,
      "relative_position": true,
      "preserve_formatting": false
    },
    {
      "op": "append_hyperlink_field",
      "target": "https://example.com/report",
      "anchor": "metrics_table",
      "tooltip": "Open report",
      "result_text": "Plan hyperlink field",
      "preserve_formatting": false,
      "dirty": true,
      "locked": true
    },
    {
      "op": "append_caption",
      "label": "Figure",
      "text": "Plan caption body.",
      "number_result_text": "VII",
      "number_format": "ROMAN",
      "restart": 3,
      "separator": " - ",
      "preserve_formatting": false
    },
    {
      "op": "append_index_entry_field",
      "entry_text": "Plan entry",
      "subentry": "Sub entry",
      "bookmark": "metrics_table",
      "cross_reference": "See also",
      "bold_page_number": true,
      "italic_page_number": true
    },
    {
      "op": "append_index_field",
      "columns": 2,
      "result_text": "Plan index field",
      "preserve_formatting": false
    },
    {
      "op": "append_complex_field",
      "instruction": "AUTHOR",
      "result_text": "Ada Lovelace",
      "dirty": true
    },
    {
      "op": "append_complex_field",
      "instruction_before": "IF ",
      "nested_instruction": "PAGE",
      "nested_result_text": "1",
      "instruction_after": " = 1 one many",
      "result_text": "one"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $advancedFieldSourceDocx `
    -EditPlan $advancedFieldPlanPath `
    -OutputDocx $advancedFieldEditedDocx `
    -SummaryJson $advancedFieldSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the advanced fields edit plan."
}

$advancedFieldSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $advancedFieldSummaryPath | ConvertFrom-Json
$advancedFieldXml = Read-DocxEntryText -DocxPath $advancedFieldEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $advancedFieldSummary.status -Expected "completed" `
    -Message "Advanced-fields summary did not report status=completed."
Assert-Equal -Actual $advancedFieldSummary.operation_count -Expected 8 `
    -Message "Advanced-fields summary should record eight operations."
Assert-Equal -Actual $advancedFieldSummary.operations[0].command -Expected "append-page-reference-field" `
    -Message "Append-page-reference-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[1].command -Expected "append-style-reference-field" `
    -Message "Append-style-reference-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[2].command -Expected "append-hyperlink-field" `
    -Message "Append-hyperlink-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[3].command -Expected "append-caption" `
    -Message "Append-caption should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[4].command -Expected "append-index-entry-field" `
    -Message "Append-index-entry-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[5].command -Expected "append-index-field" `
    -Message "Append-index-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[6].command -Expected "append-complex-field" `
    -Message "Append-complex-field should use the CLI field command."
Assert-Equal -Actual $advancedFieldSummary.operations[7].command -Expected "append-complex-field" `
    -Message "Nested append-complex-field should use the CLI field command."
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'PAGEREF metrics_table \p' -Label "Advanced-fields document.xml"
Assert-NotContainsText -Text $advancedFieldXml -UnexpectedText 'PAGEREF metrics_table \h' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'STYLEREF &quot;Heading 1&quot; \n \p' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'HYPERLINK &quot;https://example.com/report&quot; \l &quot;metrics_table&quot; \o &quot;Open report&quot;' -Label "Advanced-fields document.xml"
Assert-NotContainsText -Text $advancedFieldXml -UnexpectedText 'HYPERLINK &quot;https://example.com/report&quot; \l &quot;metrics_table&quot; \o &quot;Open report&quot; \* MERGEFORMAT' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'SEQ Figure \* ROMAN \r 3' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "Plan caption body." -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "VII" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'XE &quot;Plan entry:Sub entry&quot; \r metrics_table \t &quot;See also&quot; \b \i' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'INDEX \c &quot;2&quot;' -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "Plan index field" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "AUTHOR" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "Ada Lovelace" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "PAGE" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText "one" -Label "Advanced-fields document.xml"
Assert-ContainsText -Text $advancedFieldXml -ExpectedText 'w:fldLock="true"' -Label "Advanced-fields document.xml"

$styleListSourceDocx = Join-Path $resolvedWorkingDir "style_list.source.docx"
$styleListPlanPath = Join-Path $resolvedWorkingDir "style_list.edit_plan.json"
$styleListEditedDocx = Join-Path $resolvedWorkingDir "style_list.edited.docx"
$styleListSummaryPath = Join-Path $resolvedWorkingDir "style_list.edit.summary.json"

New-StyleListFixtureDocx -Path $styleListSourceDocx
Set-Content -LiteralPath $styleListPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_paragraph_style",
      "paragraph_index": 0,
      "style_id": "Heading1"
    },
    {
      "op": "clear_paragraph_style",
      "paragraph_index": 1
    },
    {
      "op": "set_run_style",
      "paragraph_index": 2,
      "run_index": 0,
      "style_id": "Strong"
    },
    {
      "op": "clear_run_style",
      "paragraph_index": 2,
      "run_index": 1
    },
    {
      "op": "set_run_font_family",
      "paragraph_index": 2,
      "run_index": 2,
      "font_family": "Consolas"
    },
    {
      "op": "clear_run_font_family",
      "paragraph_index": 2,
      "run_index": 3
    },
    {
      "op": "set_run_language",
      "paragraph_index": 2,
      "run_index": 4,
      "language": "en-US"
    },
    {
      "op": "clear_run_language",
      "paragraph_index": 2,
      "run_index": 5
    },
    {
      "op": "set_paragraph_list",
      "paragraph_index": 3,
      "kind": "bullet",
      "level": 1
    },
    {
      "op": "restart_paragraph_list",
      "paragraph_index": 4,
      "kind": "decimal",
      "level": 0
    },
    {
      "op": "set_paragraph_numbering",
      "paragraph_index": 5,
      "definition": 1,
      "level": 0
    },
    {
      "op": "clear_paragraph_list",
      "paragraph_index": 6
    },
    {
      "op": "set_section_page_setup",
      "section_index": 0,
      "orientation": "landscape",
      "width": 15840,
      "height": 12240,
      "margin_top": 1200,
      "margin_bottom": 1260,
      "margin_left": 1800,
      "margin_right": 1860,
      "margin_header": 720,
      "margin_footer": 760,
      "page_number_start": 5
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleListSourceDocx `
    -EditPlan $styleListPlanPath `
    -OutputDocx $styleListEditedDocx `
    -SummaryJson $styleListSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style/list/page-setup edit plan."
}

$styleListSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleListSummaryPath | ConvertFrom-Json
$styleListXml = Read-DocxEntryText -DocxPath $styleListEditedDocx -EntryName "word/document.xml"
$styleListDocument = New-Object System.Xml.XmlDocument
$styleListDocument.PreserveWhitespace = $true
$styleListDocument.LoadXml($styleListXml)
$styleListNamespaceManager = New-WordNamespaceManager -Document $styleListDocument
$wordNamespace = $styleListNamespaceManager.LookupNamespace("w")

Assert-Equal -Actual $styleListSummary.status -Expected "completed" `
    -Message "Style/list/page-setup summary did not report status=completed."
Assert-Equal -Actual $styleListSummary.operation_count -Expected 13 `
    -Message "Style/list/page-setup summary should record thirteen operations."
Assert-Equal -Actual $styleListSummary.operations[0].command -Expected "set-paragraph-style" `
    -Message "Set-paragraph-style should use the CLI paragraph-style command."
Assert-Equal -Actual $styleListSummary.operations[1].command -Expected "clear-paragraph-style" `
    -Message "Clear-paragraph-style should use the CLI paragraph-style command."
Assert-Equal -Actual $styleListSummary.operations[2].command -Expected "set-run-style" `
    -Message "Set-run-style should use the CLI run-style command."
Assert-Equal -Actual $styleListSummary.operations[3].command -Expected "clear-run-style" `
    -Message "Clear-run-style should use the CLI run-style command."
Assert-Equal -Actual $styleListSummary.operations[4].command -Expected "set-run-font-family" `
    -Message "Set-run-font-family should use the CLI font-family command."
Assert-Equal -Actual $styleListSummary.operations[5].command -Expected "clear-run-font-family" `
    -Message "Clear-run-font-family should use the CLI font-family command."
Assert-Equal -Actual $styleListSummary.operations[6].command -Expected "set-run-language" `
    -Message "Set-run-language should use the CLI run-language command."
Assert-Equal -Actual $styleListSummary.operations[7].command -Expected "clear-run-language" `
    -Message "Clear-run-language should use the CLI run-language command."
Assert-Equal -Actual $styleListSummary.operations[8].command -Expected "set-paragraph-list" `
    -Message "Set-paragraph-list should use the CLI list command."
Assert-Equal -Actual $styleListSummary.operations[9].command -Expected "restart-paragraph-list" `
    -Message "Restart-paragraph-list should use the CLI list command."
Assert-Equal -Actual $styleListSummary.operations[10].command -Expected "set-paragraph-numbering" `
    -Message "Set-paragraph-numbering should use the CLI numbering command."
Assert-Equal -Actual $styleListSummary.operations[11].command -Expected "clear-paragraph-list" `
    -Message "Clear-paragraph-list should use the CLI list command."
Assert-Equal -Actual $styleListSummary.operations[12].command -Expected "set-section-page-setup" `
    -Message "Set-section-page-setup should use the CLI page-setup command."

$setParagraphStyleNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[1]/w:pPr/w:pStyle", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setParagraphStyleNode) `
    -Message "Set-paragraph-style should create a paragraph style reference."
Assert-Equal -Actual $setParagraphStyleNode.GetAttribute("val", $wordNamespace) -Expected "Heading1" `
    -Message "Set-paragraph-style should set the requested style id."
$clearedParagraphStyleNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[2]/w:pPr/w:pStyle", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedParagraphStyleNode) `
    -Message "Clear-paragraph-style should remove the paragraph style reference."

$setRunStyleNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[1]/w:rPr/w:rStyle", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setRunStyleNode) `
    -Message "Set-run-style should create a run style reference."
Assert-Equal -Actual $setRunStyleNode.GetAttribute("val", $wordNamespace) -Expected "Strong" `
    -Message "Set-run-style should set the requested style id."
$clearedRunStyleNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[2]/w:rPr/w:rStyle", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedRunStyleNode) `
    -Message "Clear-run-style should remove the existing run style reference."

$setRunFontsNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[3]/w:rPr/w:rFonts", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setRunFontsNode) `
    -Message "Set-run-font-family should create a run font-family override."
Assert-Equal -Actual $setRunFontsNode.GetAttribute("ascii", $wordNamespace) -Expected "Consolas" `
    -Message "Set-run-font-family should update the ASCII font family."
Assert-Equal -Actual $setRunFontsNode.GetAttribute("hAnsi", $wordNamespace) -Expected "Consolas" `
    -Message "Set-run-font-family should update the hAnsi font family."
$clearedRunFontsNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[4]/w:rPr/w:rFonts", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedRunFontsNode) `
    -Message "Clear-run-font-family should remove the run font-family override."

$setRunLanguageNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[5]/w:rPr/w:lang", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setRunLanguageNode) `
    -Message "Set-run-language should create a run language override."
Assert-Equal -Actual $setRunLanguageNode.GetAttribute("val", $wordNamespace) -Expected "en-US" `
    -Message "Set-run-language should update the requested language."
$clearedRunLanguageNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[3]/w:r[6]/w:rPr/w:lang", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedRunLanguageNode) `
    -Message "Clear-run-language should remove the run language override."

$setListLevelNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[4]/w:pPr/w:numPr/w:ilvl", $styleListNamespaceManager)
$setListNumIdNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[4]/w:pPr/w:numPr/w:numId", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setListLevelNode -and $null -ne $setListNumIdNode) `
    -Message "Set-paragraph-list should add numbering metadata to the target paragraph."
Assert-Equal -Actual $setListLevelNode.GetAttribute("val", $wordNamespace) -Expected "1" `
    -Message "Set-paragraph-list should preserve the requested list level."

$restartListLevelNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[5]/w:pPr/w:numPr/w:ilvl", $styleListNamespaceManager)
$restartListNumIdNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[5]/w:pPr/w:numPr/w:numId", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $restartListLevelNode -and $null -ne $restartListNumIdNode) `
    -Message "Restart-paragraph-list should add numbering metadata to the target paragraph."
Assert-Equal -Actual $restartListLevelNode.GetAttribute("val", $wordNamespace) -Expected "0" `
    -Message "Restart-paragraph-list should preserve the requested list level."
Assert-True -Condition ($restartListNumIdNode.GetAttribute("val", $wordNamespace) -ne $setListNumIdNode.GetAttribute("val", $wordNamespace)) `
    -Message "Restart-paragraph-list should allocate a fresh numbering instance."

$setParagraphNumberingLevelNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[6]/w:pPr/w:numPr/w:ilvl", $styleListNamespaceManager)
$setParagraphNumberingNumIdNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[6]/w:pPr/w:numPr/w:numId", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $setParagraphNumberingLevelNode -and $null -ne $setParagraphNumberingNumIdNode) `
    -Message "Set-paragraph-numbering should add numbering metadata to the target paragraph."
Assert-Equal -Actual $setParagraphNumberingLevelNode.GetAttribute("val", $wordNamespace) -Expected "0" `
    -Message "Set-paragraph-numbering should preserve the requested numbering level."
Assert-Equal -Actual $setParagraphNumberingNumIdNode.GetAttribute("val", $wordNamespace) -Expected "7" `
    -Message "Set-paragraph-numbering should reference the requested numbering definition."

$clearedParagraphListNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:p[7]/w:pPr/w:numPr", $styleListNamespaceManager)
Assert-True -Condition ($null -eq $clearedParagraphListNode) `
    -Message "Clear-paragraph-list should remove numbering metadata from the target paragraph."

$sectionPageSizeNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:sectPr/w:pgSz", $styleListNamespaceManager)
$sectionMarginsNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:sectPr/w:pgMar", $styleListNamespaceManager)
$sectionPageNumberNode = $styleListDocument.SelectSingleNode("/w:document/w:body/w:sectPr/w:pgNumType", $styleListNamespaceManager)
Assert-True -Condition ($null -ne $sectionPageSizeNode -and $null -ne $sectionMarginsNode -and $null -ne $sectionPageNumberNode) `
    -Message "Set-section-page-setup should update the section page-setup nodes."
Assert-Equal -Actual $sectionPageSizeNode.GetAttribute("orient", $wordNamespace) -Expected "landscape" `
    -Message "Set-section-page-setup should set the requested orientation."
Assert-Equal -Actual $sectionPageSizeNode.GetAttribute("w", $wordNamespace) -Expected "15840" `
    -Message "Set-section-page-setup should set the requested page width."
Assert-Equal -Actual $sectionPageSizeNode.GetAttribute("h", $wordNamespace) -Expected "12240" `
    -Message "Set-section-page-setup should set the requested page height."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("top", $wordNamespace) -Expected "1200" `
    -Message "Set-section-page-setup should set the top margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("bottom", $wordNamespace) -Expected "1260" `
    -Message "Set-section-page-setup should set the bottom margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("left", $wordNamespace) -Expected "1800" `
    -Message "Set-section-page-setup should set the left margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("right", $wordNamespace) -Expected "1860" `
    -Message "Set-section-page-setup should set the right margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("header", $wordNamespace) -Expected "720" `
    -Message "Set-section-page-setup should set the header margin."
Assert-Equal -Actual $sectionMarginsNode.GetAttribute("footer", $wordNamespace) -Expected "760" `
    -Message "Set-section-page-setup should set the footer margin."
Assert-Equal -Actual $sectionPageNumberNode.GetAttribute("start", $wordNamespace) -Expected "5" `
    -Message "Set-section-page-setup should set the starting page number."

$styleListClearPageNumberPlanPath = Join-Path $resolvedWorkingDir "style_list.clear_page_number_start.edit_plan.json"
$styleListClearPageNumberDocx = Join-Path $resolvedWorkingDir "style_list.clear_page_number_start.edited.docx"
$styleListClearPageNumberSummaryPath = Join-Path $resolvedWorkingDir "style_list.clear_page_number_start.summary.json"

Set-Content -LiteralPath $styleListClearPageNumberPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_section_page_setup",
      "section_index": 0,
      "clear_page_number_start": true
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleListEditedDocx `
    -EditPlan $styleListClearPageNumberPlanPath `
    -OutputDocx $styleListClearPageNumberDocx `
    -SummaryJson $styleListClearPageNumberSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the clear page-number-start plan."
}

$styleListClearPageNumberSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleListClearPageNumberSummaryPath | ConvertFrom-Json
$styleListClearPageNumberXml = Read-DocxEntryText -DocxPath $styleListClearPageNumberDocx -EntryName "word/document.xml"
$styleListClearPageNumberDocument = New-Object System.Xml.XmlDocument
$styleListClearPageNumberDocument.PreserveWhitespace = $true
$styleListClearPageNumberDocument.LoadXml($styleListClearPageNumberXml)
$styleListClearPageNumberNamespaceManager = New-WordNamespaceManager -Document $styleListClearPageNumberDocument
$clearedSectionPageNumberNode = $styleListClearPageNumberDocument.SelectSingleNode("/w:document/w:body/w:sectPr/w:pgNumType", $styleListClearPageNumberNamespaceManager)

Assert-Equal -Actual $styleListClearPageNumberSummary.status -Expected "completed" `
    -Message "Clear-page-number-start summary did not report status=completed."
Assert-Equal -Actual $styleListClearPageNumberSummary.operation_count -Expected 1 `
    -Message "Clear-page-number-start summary should record one operation."
Assert-Equal -Actual $styleListClearPageNumberSummary.operations[0].command -Expected "set-section-page-setup" `
    -Message "Clear-page-number-start should use the CLI page-setup command."
Assert-True -Condition ($null -eq $clearedSectionPageNumberNode) `
    -Message "Clear-page-number-start should remove the section page-number-start node."

$sectionPartsSourceDocx = Join-Path $resolvedWorkingDir "section_parts.source.docx"
$sectionPartsPlanPath = Join-Path $resolvedWorkingDir "section_parts.edit_plan.json"
$sectionPartsEditedDocx = Join-Path $resolvedWorkingDir "section_parts.edited.docx"
$sectionPartsSummaryPath = Join-Path $resolvedWorkingDir "section_parts.edit.summary.json"

New-SectionPartsFixtureDocx -Path $sectionPartsSourceDocx
Set-Content -LiteralPath $sectionPartsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_section",
      "section_index": 1,
      "no_inherit": true
    },
    {
      "op": "copy_section_layout",
      "source_index": 2,
      "target_index": 1
    },
    {
      "op": "move_section",
      "source_index": 3,
      "target_index": 0
    },
    {
      "op": "set_section_header",
      "section_index": 2,
      "kind": "even",
      "text": "Plan even header"
    },
    {
      "op": "set_section_footer",
      "section_index": 0,
      "text": "Plan footer from plan"
    },
    {
      "op": "assign_section_header",
      "section_index": 1,
      "header_index": 0
    },
    {
      "op": "assign_section_footer",
      "section_index": 1,
      "footer_index": 0,
      "kind": "first"
    },
    {
      "op": "delete_section",
      "section_index": 3
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sectionPartsSourceDocx `
    -EditPlan $sectionPartsPlanPath `
    -OutputDocx $sectionPartsEditedDocx `
    -SummaryJson $sectionPartsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the section/header/footer edit plan."
}

$sectionPartsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $sectionPartsSummaryPath | ConvertFrom-Json
$sectionPartsDocumentXml = Read-DocxEntryText -DocxPath $sectionPartsEditedDocx -EntryName "word/document.xml"
$sectionPartsHeaderXml = Read-DocxEntryTextsMatching -DocxPath $sectionPartsEditedDocx -EntryNamePattern '^word/header[0-9]+\.xml$'
$sectionPartsFooterXml = Read-DocxEntryTextsMatching -DocxPath $sectionPartsEditedDocx -EntryNamePattern '^word/footer[0-9]+\.xml$'

Assert-Equal -Actual $sectionPartsSummary.status -Expected "completed" `
    -Message "Section/header/footer summary did not report status=completed."
Assert-Equal -Actual $sectionPartsSummary.operation_count -Expected 8 `
    -Message "Section/header/footer summary should record eight operations."
Assert-Equal -Actual $sectionPartsSummary.operations[0].command -Expected "insert-section" `
    -Message "insert_section should use the CLI insert-section command."
Assert-Equal -Actual $sectionPartsSummary.operations[1].command -Expected "copy-section-layout" `
    -Message "copy_section_layout should use the CLI copy-section-layout command."
Assert-Equal -Actual $sectionPartsSummary.operations[2].command -Expected "move-section" `
    -Message "move_section should use the CLI move-section command."
Assert-Equal -Actual $sectionPartsSummary.operations[3].command -Expected "set-section-header" `
    -Message "set_section_header should use the CLI set-section-header command."
Assert-Equal -Actual $sectionPartsSummary.operations[4].command -Expected "set-section-footer" `
    -Message "set_section_footer should use the CLI set-section-footer command."
Assert-Equal -Actual $sectionPartsSummary.operations[5].command -Expected "assign-section-header" `
    -Message "assign_section_header should use the CLI assign-section-header command."
Assert-Equal -Actual $sectionPartsSummary.operations[6].command -Expected "assign-section-footer" `
    -Message "assign_section_footer should use the CLI assign-section-footer command."
Assert-Equal -Actual $sectionPartsSummary.operations[7].command -Expected "remove-section" `
    -Message "delete_section should use the CLI remove-section command."
Assert-ContainsText -Text $sectionPartsDocumentXml -ExpectedText "section 2 body" -Label "Section/header/footer document.xml"
Assert-ContainsText -Text $sectionPartsHeaderXml -ExpectedText "Plan even header" -Label "Section/header/footer header parts"
Assert-ContainsText -Text $sectionPartsFooterXml -ExpectedText "Plan footer from plan" -Label "Section/header/footer footer parts"

$sectionReferenceRemoveSourceDocx = Join-Path $resolvedWorkingDir "section_reference_remove.source.docx"
$sectionReferenceRemovePlanPath = Join-Path $resolvedWorkingDir "section_reference_remove.edit_plan.json"
$sectionReferenceRemoveEditedDocx = Join-Path $resolvedWorkingDir "section_reference_remove.edited.docx"
$sectionReferenceRemoveSummaryPath = Join-Path $resolvedWorkingDir "section_reference_remove.edit.summary.json"

New-SectionPartsFixtureDocx -Path $sectionReferenceRemoveSourceDocx
Set-Content -LiteralPath $sectionReferenceRemovePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_section_header",
      "section_index": 0
    },
    {
      "op": "remove_section_footer",
      "section_index": 1,
      "kind": "first"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sectionReferenceRemoveSourceDocx `
    -EditPlan $sectionReferenceRemovePlanPath `
    -OutputDocx $sectionReferenceRemoveEditedDocx `
    -SummaryJson $sectionReferenceRemoveSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the section header/footer reference removal edit plan."
}

$sectionReferenceRemoveSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $sectionReferenceRemoveSummaryPath | ConvertFrom-Json
$sectionReferenceRemoveDocumentXml = Read-DocxEntryText -DocxPath $sectionReferenceRemoveEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $sectionReferenceRemoveSummary.status -Expected "completed" `
    -Message "Section header/footer reference removal summary did not report status=completed."
Assert-Equal -Actual $sectionReferenceRemoveSummary.operation_count -Expected 2 `
    -Message "Section header/footer reference removal summary should record two operations."
Assert-Equal -Actual $sectionReferenceRemoveSummary.operations[0].command -Expected "remove-section-header" `
    -Message "remove_section_header should use the CLI remove-section-header command."
Assert-Equal -Actual $sectionReferenceRemoveSummary.operations[1].command -Expected "remove-section-footer" `
    -Message "remove_section_footer should use the CLI remove-section-footer command."
Assert-ContainsText -Text $sectionReferenceRemoveDocumentXml -ExpectedText "section 2 body" -Label "Section header/footer reference removal document.xml"

$sectionPartReorderSourceDocx = Join-Path $resolvedWorkingDir "section_part_reorder.source.docx"
$sectionPartReorderPlanPath = Join-Path $resolvedWorkingDir "section_part_reorder.edit_plan.json"
$sectionPartReorderEditedDocx = Join-Path $resolvedWorkingDir "section_part_reorder.edited.docx"
$sectionPartReorderSummaryPath = Join-Path $resolvedWorkingDir "section_part_reorder.edit.summary.json"

New-SectionPartsFixtureDocx -Path $sectionPartReorderSourceDocx
Set-Content -LiteralPath $sectionPartReorderPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "move_header_part",
      "source_index": 1,
      "target_index": 0
    },
    {
      "op": "move_footer_part",
      "source_index": 1,
      "target_index": 0
    },
    {
      "op": "remove_header_part",
      "header_index": 1
    },
    {
      "op": "remove_footer_part",
      "footer_index": 1
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sectionPartReorderSourceDocx `
    -EditPlan $sectionPartReorderPlanPath `
    -OutputDocx $sectionPartReorderEditedDocx `
    -SummaryJson $sectionPartReorderSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the header/footer part reorder edit plan."
}

$sectionPartReorderSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $sectionPartReorderSummaryPath | ConvertFrom-Json
$sectionPartReorderDocumentXml = Read-DocxEntryText -DocxPath $sectionPartReorderEditedDocx -EntryName "word/document.xml"
$sectionPartReorderHeaderXml = Read-DocxEntryTextsMatching -DocxPath $sectionPartReorderEditedDocx -EntryNamePattern '^word/header[0-9]+\.xml$'
$sectionPartReorderFooterXml = Read-DocxEntryTextsMatching -DocxPath $sectionPartReorderEditedDocx -EntryNamePattern '^word/footer[0-9]+\.xml$'

Assert-Equal -Actual $sectionPartReorderSummary.status -Expected "completed" `
    -Message "Header/footer part reorder summary did not report status=completed."
Assert-Equal -Actual $sectionPartReorderSummary.operation_count -Expected 4 `
    -Message "Header/footer part reorder summary should record four operations."
Assert-Equal -Actual $sectionPartReorderSummary.operations[0].command -Expected "move-header-part" `
    -Message "move_header_part should use the CLI move-header-part command."
Assert-Equal -Actual $sectionPartReorderSummary.operations[1].command -Expected "move-footer-part" `
    -Message "move_footer_part should use the CLI move-footer-part command."
Assert-Equal -Actual $sectionPartReorderSummary.operations[2].command -Expected "remove-header-part" `
    -Message "remove_header_part should use the CLI remove-header-part command."
Assert-Equal -Actual $sectionPartReorderSummary.operations[3].command -Expected "remove-footer-part" `
    -Message "remove_footer_part should use the CLI remove-footer-part command."
Assert-ContainsText -Text $sectionPartReorderDocumentXml -ExpectedText "section 2 body" -Label "Header/footer part reorder document.xml"
Assert-ContainsText -Text $sectionPartReorderHeaderXml -ExpectedText "section 1 header" -Label "Header/footer part reorder header parts"
Assert-ContainsText -Text $sectionPartReorderFooterXml -ExpectedText "section 1 first footer" -Label "Header/footer part reorder footer parts"

$styleMetadataSourceDocx = Join-Path $resolvedWorkingDir "style_metadata.source.docx"
$styleMetadataPlanPath = Join-Path $resolvedWorkingDir "style_metadata.edit_plan.json"
$styleMetadataEditedDocx = Join-Path $resolvedWorkingDir "style_metadata.edited.docx"
$styleMetadataSummaryPath = Join-Path $resolvedWorkingDir "style_metadata.edit.summary.json"
$styleMetadataClearDefaultsPlanPath = Join-Path $resolvedWorkingDir "style_metadata.clear_defaults.edit_plan.json"
$styleMetadataDefaultsClearedDocx = Join-Path $resolvedWorkingDir "style_metadata.clear_defaults.edited.docx"
$styleMetadataClearDefaultsSummaryPath = Join-Path $resolvedWorkingDir "style_metadata.clear_defaults.summary.json"
$styleDefinitionPlanPath = Join-Path $resolvedWorkingDir "style_definition.edit_plan.json"
$styleDefinitionEditedDocx = Join-Path $resolvedWorkingDir "style_definition.edited.docx"
$styleDefinitionSummaryPath = Join-Path $resolvedWorkingDir "style_definition.edit.summary.json"

New-StyleListFixtureDocx -Path $styleMetadataSourceDocx
Set-Content -LiteralPath $styleMetadataPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_default_run_properties",
      "font": "Aptos",
      "east_asia_font": "Microsoft YaHei",
      "lang": "en-US",
      "east_asia_lang": "zh-CN",
      "bidi_lang": "ar-SA",
      "rtl": true,
      "paragraph_bidi": true
    },
    {
      "op": "set_style_run_properties",
      "style_id": "Heading1",
      "font_family": "Aptos Display",
      "east_asia_font_family": "Microsoft YaHei",
      "language": "en-US",
      "east_asia_language": "zh-CN",
      "bidi_language": "ar-SA",
      "rtl": true,
      "paragraph_bidi": true
    },
    {
      "op": "clear_style_run_properties",
      "style": "LegacyBody",
      "clear_font_family": true,
      "east_asia_font_family": true,
      "clear_primary_language": true,
      "language": true,
      "clear_east_asia_language": true,
      "bidi_language": true,
      "clear_rtl": true,
      "paragraph_bidi": true
    },
    {
      "op": "set_paragraph_style_properties",
      "style": "Heading1",
      "based_on_style_id": "LegacyBody",
      "next_style_id": "LegacyBody",
      "level": 1
    },
    {
      "op": "clear_paragraph_style_properties",
      "style_id": "LegacyBody",
      "based_on": true,
      "clear_next_style": true,
      "clear_outline_level": true
    },
    {
      "op": "set_paragraph_style_numbering",
      "style": "Heading1",
      "name": "HeadingOutline",
      "levels": [
        {
          "level": 0,
          "kind": "decimal",
          "pattern": "%1."
        },
        {
          "level": 1,
          "kind": "decimal",
          "start": 3,
          "text_pattern": "%1.%2."
        }
      ],
      "style_level": 1
    },
    {
      "op": "clear_paragraph_style_numbering",
      "style_id": "LegacyBody"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleMetadataSourceDocx `
    -EditPlan $styleMetadataPlanPath `
    -OutputDocx $styleMetadataEditedDocx `
    -SummaryJson $styleMetadataSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style metadata edit plan."
}

$styleMetadataSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleMetadataSummaryPath | ConvertFrom-Json
$styleMetadataStylesXml = Read-DocxEntryText -DocxPath $styleMetadataEditedDocx -EntryName "word/styles.xml"
$styleMetadataStylesDocument = New-Object System.Xml.XmlDocument
$styleMetadataStylesDocument.PreserveWhitespace = $true
$styleMetadataStylesDocument.LoadXml($styleMetadataStylesXml)
$styleMetadataStylesNamespaceManager = New-WordNamespaceManager -Document $styleMetadataStylesDocument
$styleMetadataNumberingXml = Read-DocxEntryText -DocxPath $styleMetadataEditedDocx -EntryName "word/numbering.xml"
$styleMetadataNumberingDocument = New-Object System.Xml.XmlDocument
$styleMetadataNumberingDocument.PreserveWhitespace = $true
$styleMetadataNumberingDocument.LoadXml($styleMetadataNumberingXml)
$styleMetadataNumberingNamespaceManager = New-WordNamespaceManager -Document $styleMetadataNumberingDocument
$styleMetadataWordNamespace = $styleMetadataStylesNamespaceManager.LookupNamespace("w")

Assert-Equal -Actual $styleMetadataSummary.status -Expected "completed" `
    -Message "Style-metadata summary did not report status=completed."
Assert-Equal -Actual $styleMetadataSummary.operation_count -Expected 7 `
    -Message "Style-metadata summary should record seven operations."
Assert-Equal -Actual $styleMetadataSummary.operations[0].command -Expected "set-default-run-properties" `
    -Message "Set-default-run-properties should use the CLI default-run command."
Assert-Equal -Actual $styleMetadataSummary.operations[1].command -Expected "set-style-run-properties" `
    -Message "Set-style-run-properties should use the CLI style-run command."
Assert-Equal -Actual $styleMetadataSummary.operations[2].command -Expected "clear-style-run-properties" `
    -Message "Clear-style-run-properties should use the CLI style-run command."
Assert-Equal -Actual $styleMetadataSummary.operations[3].command -Expected "set-paragraph-style-properties" `
    -Message "Set-paragraph-style-properties should use the CLI paragraph-style-properties command."
Assert-Equal -Actual $styleMetadataSummary.operations[4].command -Expected "clear-paragraph-style-properties" `
    -Message "Clear-paragraph-style-properties should use the CLI paragraph-style-properties command."
Assert-Equal -Actual $styleMetadataSummary.operations[5].command -Expected "set-paragraph-style-numbering" `
    -Message "Set-paragraph-style-numbering should use the CLI style-numbering command."
Assert-Equal -Actual $styleMetadataSummary.operations[6].command -Expected "clear-paragraph-style-numbering" `
    -Message "Clear-paragraph-style-numbering should use the CLI style-numbering command."

$defaultRunFontsNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:rFonts", $styleMetadataStylesNamespaceManager)
$defaultRunLanguageNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:lang", $styleMetadataStylesNamespaceManager)
$defaultRunRtlNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:rtl", $styleMetadataStylesNamespaceManager)
$defaultParagraphBidiNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:pPrDefault/w:pPr/w:bidi", $styleMetadataStylesNamespaceManager)

Assert-True -Condition ($null -ne $defaultRunFontsNode) `
    -Message "Set-default-run-properties should keep docDefaults run font metadata."
Assert-Equal -Actual $defaultRunFontsNode.GetAttribute("ascii", $styleMetadataWordNamespace) -Expected "Aptos" `
    -Message "Set-default-run-properties should update the default ASCII font family."
Assert-Equal -Actual $defaultRunFontsNode.GetAttribute("hAnsi", $styleMetadataWordNamespace) -Expected "Aptos" `
    -Message "Set-default-run-properties should update the default hAnsi font family."
Assert-Equal -Actual $defaultRunFontsNode.GetAttribute("eastAsia", $styleMetadataWordNamespace) -Expected "Microsoft YaHei" `
    -Message "Set-default-run-properties should update the default East Asia font family."
Assert-True -Condition ($null -ne $defaultRunLanguageNode) `
    -Message "Set-default-run-properties should keep docDefaults language metadata."
Assert-Equal -Actual $defaultRunLanguageNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "en-US" `
    -Message "Set-default-run-properties should update the default primary language."
Assert-Equal -Actual $defaultRunLanguageNode.GetAttribute("eastAsia", $styleMetadataWordNamespace) -Expected "zh-CN" `
    -Message "Set-default-run-properties should update the default East Asia language."
Assert-Equal -Actual $defaultRunLanguageNode.GetAttribute("bidi", $styleMetadataWordNamespace) -Expected "ar-SA" `
    -Message "Set-default-run-properties should update the default bidi language."
Assert-True -Condition ($null -ne $defaultRunRtlNode) `
    -Message "Set-default-run-properties should keep the default RTL marker."
Assert-True -Condition ($null -ne $defaultParagraphBidiNode) `
    -Message "Set-default-run-properties should keep the default paragraph bidi marker."

$headingRunFontsNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:rPr/w:rFonts", $styleMetadataStylesNamespaceManager)
$headingRunLanguageNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:rPr/w:lang", $styleMetadataStylesNamespaceManager)
$headingRunRtlNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:rPr/w:rtl", $styleMetadataStylesNamespaceManager)
$headingParagraphBidiNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:bidi", $styleMetadataStylesNamespaceManager)
$headingBasedOnNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:basedOn", $styleMetadataStylesNamespaceManager)
$headingNextNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:next", $styleMetadataStylesNamespaceManager)
$headingOutlineNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:outlineLvl", $styleMetadataStylesNamespaceManager)
$headingNumPrNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:numPr", $styleMetadataStylesNamespaceManager)
$headingNumLevelNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:numPr/w:ilvl", $styleMetadataStylesNamespaceManager)
$headingNumIdNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:numPr/w:numId", $styleMetadataStylesNamespaceManager)

Assert-True -Condition ($null -ne $headingRunFontsNode) `
    -Message "Set-style-run-properties should create style run font metadata."
Assert-Equal -Actual $headingRunFontsNode.GetAttribute("ascii", $styleMetadataWordNamespace) -Expected "Aptos Display" `
    -Message "Set-style-run-properties should update the style ASCII font family."
Assert-Equal -Actual $headingRunFontsNode.GetAttribute("hAnsi", $styleMetadataWordNamespace) -Expected "Aptos Display" `
    -Message "Set-style-run-properties should update the style hAnsi font family."
Assert-Equal -Actual $headingRunFontsNode.GetAttribute("eastAsia", $styleMetadataWordNamespace) -Expected "Microsoft YaHei" `
    -Message "Set-style-run-properties should update the style East Asia font family."
Assert-True -Condition ($null -ne $headingRunLanguageNode) `
    -Message "Set-style-run-properties should create style language metadata."
Assert-Equal -Actual $headingRunLanguageNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "en-US" `
    -Message "Set-style-run-properties should update the style primary language."
Assert-Equal -Actual $headingRunLanguageNode.GetAttribute("eastAsia", $styleMetadataWordNamespace) -Expected "zh-CN" `
    -Message "Set-style-run-properties should update the style East Asia language."
Assert-Equal -Actual $headingRunLanguageNode.GetAttribute("bidi", $styleMetadataWordNamespace) -Expected "ar-SA" `
    -Message "Set-style-run-properties should update the style bidi language."
Assert-True -Condition ($null -ne $headingRunRtlNode) `
    -Message "Set-style-run-properties should create the style RTL marker."
Assert-True -Condition ($null -ne $headingParagraphBidiNode) `
    -Message "Set-style-run-properties should create the style paragraph bidi marker."
Assert-True -Condition ($null -ne $headingBasedOnNode) `
    -Message "Set-paragraph-style-properties should create the basedOn node."
Assert-Equal -Actual $headingBasedOnNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "LegacyBody" `
    -Message "Set-paragraph-style-properties should set the requested based-on style."
Assert-True -Condition ($null -ne $headingNextNode) `
    -Message "Set-paragraph-style-properties should create the next style node."
Assert-Equal -Actual $headingNextNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "LegacyBody" `
    -Message "Set-paragraph-style-properties should set the requested next style."
Assert-True -Condition ($null -ne $headingOutlineNode) `
    -Message "Set-paragraph-style-properties should create the outline-level node."
Assert-Equal -Actual $headingOutlineNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "1" `
    -Message "Set-paragraph-style-properties should set the requested outline level."
Assert-True -Condition ($null -ne $headingNumPrNode -and $null -ne $headingNumLevelNode -and $null -ne $headingNumIdNode) `
    -Message "Set-paragraph-style-numbering should create style numbering metadata."
Assert-Equal -Actual $headingNumLevelNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "1" `
    -Message "Set-paragraph-style-numbering should keep the requested style level."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace($headingNumIdNode.GetAttribute("val", $styleMetadataWordNamespace))) `
    -Message "Set-paragraph-style-numbering should link a numbering instance."

$headingDefinitionNode = $styleMetadataNumberingDocument.SelectSingleNode("/w:numbering/w:abstractNum[w:name[@w:val='HeadingOutline']]", $styleMetadataNumberingNamespaceManager)
Assert-True -Condition ($null -ne $headingDefinitionNode) `
    -Message "Set-paragraph-style-numbering should create the requested numbering definition."
$headingDefinitionLevelZeroNode = $headingDefinitionNode.SelectSingleNode("w:lvl[@w:ilvl='0']", $styleMetadataNumberingNamespaceManager)
$headingDefinitionLevelOneNode = $headingDefinitionNode.SelectSingleNode("w:lvl[@w:ilvl='1']", $styleMetadataNumberingNamespaceManager)
Assert-True -Condition ($null -ne $headingDefinitionLevelZeroNode -and $null -ne $headingDefinitionLevelOneNode) `
    -Message "Set-paragraph-style-numbering should create every requested numbering level."
Assert-Equal -Actual $headingDefinitionLevelZeroNode.SelectSingleNode("w:numFmt", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "decimal" `
    -Message "Set-paragraph-style-numbering should set level zero numbering kind."
Assert-Equal -Actual $headingDefinitionLevelZeroNode.SelectSingleNode("w:start", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "1" `
    -Message "Set-paragraph-style-numbering should default the first level start to 1."
Assert-Equal -Actual $headingDefinitionLevelZeroNode.SelectSingleNode("w:lvlText", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "%1." `
    -Message "Set-paragraph-style-numbering should set the first level text pattern."
Assert-Equal -Actual $headingDefinitionLevelOneNode.SelectSingleNode("w:start", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "3" `
    -Message "Set-paragraph-style-numbering should preserve explicit numbering starts."
Assert-Equal -Actual $headingDefinitionLevelOneNode.SelectSingleNode("w:lvlText", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "%1.%2." `
    -Message "Set-paragraph-style-numbering should set the second level text pattern."

$legacyFontsNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:rPr/w:rFonts", $styleMetadataStylesNamespaceManager)
$legacyLanguageNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:rPr/w:lang", $styleMetadataStylesNamespaceManager)
$legacyRtlNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:rPr/w:rtl", $styleMetadataStylesNamespaceManager)
$legacyParagraphBidiNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:pPr/w:bidi", $styleMetadataStylesNamespaceManager)
$legacyBasedOnNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:basedOn", $styleMetadataStylesNamespaceManager)
$legacyNextNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:next", $styleMetadataStylesNamespaceManager)
$legacyOutlineNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:pPr/w:outlineLvl", $styleMetadataStylesNamespaceManager)
$legacyNumPrNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:pPr/w:numPr", $styleMetadataStylesNamespaceManager)

Assert-True -Condition ($null -eq $legacyFontsNode -or (`
        [string]::IsNullOrWhiteSpace($legacyFontsNode.GetAttribute("ascii", $styleMetadataWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($legacyFontsNode.GetAttribute("hAnsi", $styleMetadataWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($legacyFontsNode.GetAttribute("eastAsia", $styleMetadataWordNamespace)))) `
    -Message "Clear-style-run-properties should remove LegacyBody font metadata."
Assert-True -Condition ($null -eq $legacyLanguageNode -or (`
        [string]::IsNullOrWhiteSpace($legacyLanguageNode.GetAttribute("val", $styleMetadataWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($legacyLanguageNode.GetAttribute("eastAsia", $styleMetadataWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($legacyLanguageNode.GetAttribute("bidi", $styleMetadataWordNamespace)))) `
    -Message "Clear-style-run-properties should remove LegacyBody language metadata."
Assert-True -Condition ($null -eq $legacyRtlNode) `
    -Message "Clear-style-run-properties should remove the LegacyBody RTL marker."
Assert-True -Condition ($null -eq $legacyParagraphBidiNode) `
    -Message "Clear-style-run-properties should remove the LegacyBody paragraph bidi marker."
Assert-True -Condition ($null -eq $legacyBasedOnNode) `
    -Message "Clear-paragraph-style-properties should remove LegacyBody basedOn."
Assert-True -Condition ($null -eq $legacyNextNode) `
    -Message "Clear-paragraph-style-properties should remove LegacyBody next style metadata."
Assert-True -Condition ($null -eq $legacyOutlineNode) `
    -Message "Clear-paragraph-style-properties should remove LegacyBody outline level metadata."
Assert-True -Condition ($null -eq $legacyNumPrNode) `
    -Message "Clear-paragraph-style-numbering should remove LegacyBody numbering metadata."

Set-Content -LiteralPath $styleMetadataClearDefaultsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_default_run_properties",
      "font_family": true,
      "clear_east_asia_font_family": true,
      "primary_language": true,
      "clear_language": true,
      "east_asia_language": true,
      "clear_bidi_language": true,
      "rtl": true,
      "clear_paragraph_bidi": true
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleMetadataEditedDocx `
    -EditPlan $styleMetadataClearDefaultsPlanPath `
    -OutputDocx $styleMetadataDefaultsClearedDocx `
    -SummaryJson $styleMetadataClearDefaultsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the clear default-run-properties plan."
}

$styleMetadataClearDefaultsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleMetadataClearDefaultsSummaryPath | ConvertFrom-Json
$styleMetadataDefaultsClearedStylesXml = Read-DocxEntryText -DocxPath $styleMetadataDefaultsClearedDocx -EntryName "word/styles.xml"
$styleMetadataDefaultsClearedStylesDocument = New-Object System.Xml.XmlDocument
$styleMetadataDefaultsClearedStylesDocument.PreserveWhitespace = $true
$styleMetadataDefaultsClearedStylesDocument.LoadXml($styleMetadataDefaultsClearedStylesXml)
$styleMetadataDefaultsClearedNamespaceManager = New-WordNamespaceManager -Document $styleMetadataDefaultsClearedStylesDocument
$styleMetadataDefaultsClearedWordNamespace = $styleMetadataDefaultsClearedNamespaceManager.LookupNamespace("w")
$clearedDefaultRunFontsNode = $styleMetadataDefaultsClearedStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:rFonts", $styleMetadataDefaultsClearedNamespaceManager)
$clearedDefaultRunLanguageNode = $styleMetadataDefaultsClearedStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:lang", $styleMetadataDefaultsClearedNamespaceManager)
$clearedDefaultRunRtlNode = $styleMetadataDefaultsClearedStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:rtl", $styleMetadataDefaultsClearedNamespaceManager)
$clearedDefaultParagraphBidiNode = $styleMetadataDefaultsClearedStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:pPrDefault/w:pPr/w:bidi", $styleMetadataDefaultsClearedNamespaceManager)

Assert-Equal -Actual $styleMetadataClearDefaultsSummary.status -Expected "completed" `
    -Message "Clear-default-run-properties summary did not report status=completed."
Assert-Equal -Actual $styleMetadataClearDefaultsSummary.operation_count -Expected 1 `
    -Message "Clear-default-run-properties summary should record one operation."
Assert-Equal -Actual $styleMetadataClearDefaultsSummary.operations[0].command -Expected "clear-default-run-properties" `
    -Message "Clear-default-run-properties should use the CLI default-run command."
Assert-True -Condition ($null -eq $clearedDefaultRunFontsNode -or (`
        [string]::IsNullOrWhiteSpace($clearedDefaultRunFontsNode.GetAttribute("ascii", $styleMetadataDefaultsClearedWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($clearedDefaultRunFontsNode.GetAttribute("hAnsi", $styleMetadataDefaultsClearedWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($clearedDefaultRunFontsNode.GetAttribute("eastAsia", $styleMetadataDefaultsClearedWordNamespace)))) `
    -Message "Clear-default-run-properties should remove docDefaults font metadata."
Assert-True -Condition ($null -eq $clearedDefaultRunLanguageNode -or (`
        [string]::IsNullOrWhiteSpace($clearedDefaultRunLanguageNode.GetAttribute("val", $styleMetadataDefaultsClearedWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($clearedDefaultRunLanguageNode.GetAttribute("eastAsia", $styleMetadataDefaultsClearedWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($clearedDefaultRunLanguageNode.GetAttribute("bidi", $styleMetadataDefaultsClearedWordNamespace)))) `
    -Message "Clear-default-run-properties should remove docDefaults language metadata."
Assert-True -Condition ($null -eq $clearedDefaultRunRtlNode) `
    -Message "Clear-default-run-properties should remove the docDefaults RTL marker."
Assert-True -Condition ($null -eq $clearedDefaultParagraphBidiNode) `
    -Message "Clear-default-run-properties should remove the docDefaults paragraph bidi marker."

Set-Content -LiteralPath $styleDefinitionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "ensure_paragraph_style",
      "style_id": "ReviewHeading",
      "name": "Review Heading",
      "based_on": "Heading1",
      "next_style": "LegacyBody",
      "custom": true,
      "semi_hidden": false,
      "unhide_when_used": true,
      "quick_format": true,
      "font_family": "Aptos Display",
      "east_asia_font_family": "Microsoft YaHei",
      "language": "en-US",
      "east_asia_language": "zh-CN",
      "bidi_language": "ar-SA",
      "rtl": true,
      "paragraph_bidi": true,
      "outline_level": 2
    },
    {
      "op": "ensure_character_style",
      "style": "ReviewStrong",
      "name": "Review Strong",
      "based_on_style_id": "Strong",
      "custom": true,
      "run_font_family": "Consolas",
      "run_east_asia_font_family": "Microsoft YaHei",
      "run_language": "en-US",
      "run_east_asia_language": "zh-CN",
      "run_bidi_language": "ar-SA",
      "run_rtl": true
    },
    {
      "op": "materialize_style_run_properties",
      "style_id": "ReviewHeading"
    },
    {
      "op": "rebase_character_style_based_on",
      "style_id": "ReviewStrong",
      "target_style": "LegacyStrong"
    },
    {
      "op": "rebase_paragraph_style_based_on",
      "style_id": "ReviewHeading",
      "target_based_on": "Normal"
    },
    {
      "op": "ensure_numbering_definition",
      "definition_name": "DefinitionOnly",
      "numbering_levels": [
        {
          "level": 0,
          "kind": "decimal",
          "pattern": "%1)"
        }
      ]
    },
    {
      "op": "ensure_style_linked_numbering",
      "name": "SharedHeadingOutline",
      "levels": [
        "0:decimal:1:%1.",
        {
          "level": 1,
          "kind": "decimal",
          "start": 2,
          "pattern": "%1.%2."
        }
      ],
      "style_links": [
        {
          "style": "ReviewHeading",
          "level": 0
        },
        "Heading1:1"
      ]
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleMetadataDefaultsClearedDocx `
    -EditPlan $styleDefinitionPlanPath `
    -OutputDocx $styleDefinitionEditedDocx `
    -SummaryJson $styleDefinitionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style definition edit plan."
}

$styleDefinitionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleDefinitionSummaryPath | ConvertFrom-Json
$styleDefinitionStylesXml = Read-DocxEntryText -DocxPath $styleDefinitionEditedDocx -EntryName "word/styles.xml"
$styleDefinitionStylesDocument = New-Object System.Xml.XmlDocument
$styleDefinitionStylesDocument.PreserveWhitespace = $true
$styleDefinitionStylesDocument.LoadXml($styleDefinitionStylesXml)
$styleDefinitionStylesNamespaceManager = New-WordNamespaceManager -Document $styleDefinitionStylesDocument
$styleDefinitionNumberingXml = Read-DocxEntryText -DocxPath $styleDefinitionEditedDocx -EntryName "word/numbering.xml"
$styleDefinitionNumberingDocument = New-Object System.Xml.XmlDocument
$styleDefinitionNumberingDocument.PreserveWhitespace = $true
$styleDefinitionNumberingDocument.LoadXml($styleDefinitionNumberingXml)
$styleDefinitionNumberingNamespaceManager = New-WordNamespaceManager -Document $styleDefinitionNumberingDocument
$styleDefinitionWordNamespace = $styleDefinitionStylesNamespaceManager.LookupNamespace("w")

Assert-Equal -Actual $styleDefinitionSummary.status -Expected "completed" `
    -Message "Style-definition summary did not report status=completed."
Assert-Equal -Actual $styleDefinitionSummary.operation_count -Expected 7 `
    -Message "Style-definition summary should record seven operations."
Assert-Equal -Actual $styleDefinitionSummary.operations[0].command -Expected "ensure-paragraph-style" `
    -Message "Ensure-paragraph-style should use the CLI paragraph style command."
Assert-Equal -Actual $styleDefinitionSummary.operations[1].command -Expected "ensure-character-style" `
    -Message "Ensure-character-style should use the CLI character style command."
Assert-Equal -Actual $styleDefinitionSummary.operations[2].command -Expected "materialize-style-run-properties" `
    -Message "Materialize-style-run-properties should use the CLI materialize command."
Assert-Equal -Actual $styleDefinitionSummary.operations[3].command -Expected "rebase-character-style-based-on" `
    -Message "Rebase-character-style-based-on should use the CLI rebase command."
Assert-Equal -Actual $styleDefinitionSummary.operations[4].command -Expected "rebase-paragraph-style-based-on" `
    -Message "Rebase-paragraph-style-based-on should use the CLI rebase command."
Assert-Equal -Actual $styleDefinitionSummary.operations[5].command -Expected "ensure-numbering-definition" `
    -Message "Ensure-numbering-definition should use the CLI numbering command."
Assert-Equal -Actual $styleDefinitionSummary.operations[6].command -Expected "ensure-style-linked-numbering" `
    -Message "Ensure-style-linked-numbering should use the CLI style-linked numbering command."

$reviewHeadingStyleNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']", $styleDefinitionStylesNamespaceManager)
$reviewHeadingNameNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:name", $styleDefinitionStylesNamespaceManager)
$reviewHeadingBasedOnNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:basedOn", $styleDefinitionStylesNamespaceManager)
$reviewHeadingNextNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:next", $styleDefinitionStylesNamespaceManager)
$reviewHeadingOutlineNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:pPr/w:outlineLvl", $styleDefinitionStylesNamespaceManager)
$reviewHeadingNumLevelNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:pPr/w:numPr/w:ilvl", $styleDefinitionStylesNamespaceManager)
$reviewHeadingRunFontsNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:rPr/w:rFonts", $styleDefinitionStylesNamespaceManager)
$reviewHeadingLanguageNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:rPr/w:lang", $styleDefinitionStylesNamespaceManager)

Assert-True -Condition ($null -ne $reviewHeadingStyleNode) `
    -Message "Ensure-paragraph-style should create ReviewHeading."
Assert-Equal -Actual $reviewHeadingNameNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "Review Heading" `
    -Message "Ensure-paragraph-style should set the paragraph style display name."
Assert-Equal -Actual $reviewHeadingBasedOnNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "Normal" `
    -Message "Rebase-paragraph-style-based-on should switch ReviewHeading basedOn."
Assert-Equal -Actual $reviewHeadingNextNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "LegacyBody" `
    -Message "Ensure-paragraph-style should set ReviewHeading next style."
Assert-Equal -Actual $reviewHeadingOutlineNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "2" `
    -Message "Ensure-paragraph-style should set ReviewHeading outline level."
Assert-Equal -Actual $reviewHeadingNumLevelNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "0" `
    -Message "Ensure-style-linked-numbering should link ReviewHeading at level zero."
Assert-Equal -Actual $reviewHeadingRunFontsNode.GetAttribute("ascii", $styleDefinitionWordNamespace) -Expected "Aptos Display" `
    -Message "Materialize-style-run-properties should preserve ReviewHeading run font metadata."
Assert-Equal -Actual $reviewHeadingLanguageNode.GetAttribute("bidi", $styleDefinitionWordNamespace) -Expected "ar-SA" `
    -Message "Materialize-style-run-properties should preserve ReviewHeading bidi language."

$reviewStrongStyleNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']", $styleDefinitionStylesNamespaceManager)
$reviewStrongNameNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']/w:name", $styleDefinitionStylesNamespaceManager)
$reviewStrongBasedOnNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']/w:basedOn", $styleDefinitionStylesNamespaceManager)
$reviewStrongRunFontsNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']/w:rPr/w:rFonts", $styleDefinitionStylesNamespaceManager)
$reviewStrongRunRtlNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']/w:rPr/w:rtl", $styleDefinitionStylesNamespaceManager)

Assert-True -Condition ($null -ne $reviewStrongStyleNode) `
    -Message "Ensure-character-style should create ReviewStrong."
Assert-Equal -Actual $reviewStrongNameNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "Review Strong" `
    -Message "Ensure-character-style should set the character style display name."
Assert-Equal -Actual $reviewStrongBasedOnNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "LegacyStrong" `
    -Message "Rebase-character-style-based-on should switch ReviewStrong basedOn."
Assert-Equal -Actual $reviewStrongRunFontsNode.GetAttribute("ascii", $styleDefinitionWordNamespace) -Expected "Consolas" `
    -Message "Ensure-character-style should set ReviewStrong run font metadata."
Assert-True -Condition ($null -ne $reviewStrongRunRtlNode) `
    -Message "Ensure-character-style should set ReviewStrong RTL metadata."

$definitionOnlyNode = $styleDefinitionNumberingDocument.SelectSingleNode("/w:numbering/w:abstractNum[w:name[@w:val='DefinitionOnly']]", $styleDefinitionNumberingNamespaceManager)
$sharedHeadingOutlineNode = $styleDefinitionNumberingDocument.SelectSingleNode("/w:numbering/w:abstractNum[w:name[@w:val='SharedHeadingOutline']]", $styleDefinitionNumberingNamespaceManager)
$sharedHeadingLevelOneNode = $sharedHeadingOutlineNode.SelectSingleNode("w:lvl[@w:ilvl='1']", $styleDefinitionNumberingNamespaceManager)
$heading1NumLevelNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:numPr/w:ilvl", $styleDefinitionStylesNamespaceManager)

Assert-True -Condition ($null -ne $definitionOnlyNode) `
    -Message "Ensure-numbering-definition should create DefinitionOnly."
Assert-True -Condition ($null -ne $sharedHeadingOutlineNode) `
    -Message "Ensure-style-linked-numbering should create SharedHeadingOutline."
Assert-Equal -Actual $sharedHeadingLevelOneNode.SelectSingleNode("w:start", $styleDefinitionNumberingNamespaceManager).GetAttribute("val", $styleDefinitionWordNamespace) -Expected "2" `
    -Message "Ensure-style-linked-numbering should preserve structured level starts."
Assert-Equal -Actual $heading1NumLevelNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "1" `
    -Message "Ensure-style-linked-numbering should link Heading1 at level one."

$styleRefactorSourceDocx = Join-Path $resolvedWorkingDir "style_refactor.source.docx"
$styleRefactorPlanPath = Join-Path $resolvedWorkingDir "style_refactor.edit_plan.json"
$styleRefactorEditedDocx = Join-Path $resolvedWorkingDir "style_refactor.edited.docx"
$styleRefactorSummaryPath = Join-Path $resolvedWorkingDir "style_refactor.edit.summary.json"
$styleRefactorRollbackPath = Join-Path $resolvedWorkingDir "style_refactor.rollback.json"
$styleRefactorRestorePlanPath = Join-Path $resolvedWorkingDir "style_refactor.restore_plan.json"
$styleRefactorRestoredDocx = Join-Path $resolvedWorkingDir "style_refactor.restored.docx"
$styleRefactorRestoreSummaryPath = Join-Path $resolvedWorkingDir "style_refactor.restore.summary.json"

New-StyleRefactorFixtureDocx -Path $styleRefactorSourceDocx
Set-Content -LiteralPath $styleRefactorPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "rename_style",
      "old_style_id": "LegacyBody",
      "new_style_id": "ReviewBody"
    },
    {
      "op": "merge_style",
      "source_style_id": "MergeSource",
      "target_style_id": "MergeTarget"
    },
    {
      "op": "apply_style_refactor",
      "renames": [
        {
          "old_style_id": "BulkOld",
          "new_style_id": "BulkNew"
        }
      ],
      "merges": [
        {
          "source_style_id": "BulkMergeSource",
          "target_style_id": "BulkMergeTarget"
        }
      ],
      "rollback_plan": "$($styleRefactorRollbackPath.Replace('\', '\\'))"
    },
    {
      "op": "prune_unused_styles"
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $styleRefactorSourceDocx `
    -EditPlan $styleRefactorPlanPath `
    -OutputDocx $styleRefactorEditedDocx `
    -SummaryJson $styleRefactorSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style refactor edit plan."
}

$styleRefactorSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleRefactorSummaryPath | ConvertFrom-Json
$styleRefactorDocumentXml = Read-DocxEntryText -DocxPath $styleRefactorEditedDocx -EntryName "word/document.xml"
$styleRefactorStylesXml = Read-DocxEntryText -DocxPath $styleRefactorEditedDocx -EntryName "word/styles.xml"

Assert-Equal -Actual $styleRefactorSummary.status -Expected "completed" `
    -Message "Style-refactor summary did not report status=completed."
Assert-Equal -Actual $styleRefactorSummary.operation_count -Expected 4 `
    -Message "Style-refactor summary should record four operations."
Assert-Equal -Actual $styleRefactorSummary.operations[0].command -Expected "rename-style" `
    -Message "rename_style should use the CLI rename-style command."
Assert-Equal -Actual $styleRefactorSummary.operations[1].command -Expected "merge-style" `
    -Message "merge_style should use the CLI merge-style command."
Assert-Equal -Actual $styleRefactorSummary.operations[2].command -Expected "apply-style-refactor" `
    -Message "apply_style_refactor should use the CLI apply-style-refactor command."
Assert-Equal -Actual $styleRefactorSummary.operations[3].command -Expected "prune-unused-styles" `
    -Message "prune_unused_styles should use the CLI prune-unused-styles command."
Assert-ContainsText -Text $styleRefactorDocumentXml -ExpectedText 'w:pStyle w:val="ReviewBody"' -Label "Style-refactor document.xml"
Assert-ContainsText -Text $styleRefactorDocumentXml -ExpectedText 'w:pStyle w:val="MergeTarget"' -Label "Style-refactor document.xml"
Assert-ContainsText -Text $styleRefactorDocumentXml -ExpectedText 'w:pStyle w:val="BulkNew"' -Label "Style-refactor document.xml"
Assert-ContainsText -Text $styleRefactorDocumentXml -ExpectedText 'w:pStyle w:val="BulkMergeTarget"' -Label "Style-refactor document.xml"
Assert-ContainsText -Text $styleRefactorStylesXml -ExpectedText 'w:styleId="ReviewBody"' -Label "Style-refactor styles.xml"
Assert-ContainsText -Text $styleRefactorStylesXml -ExpectedText 'w:styleId="MergeTarget"' -Label "Style-refactor styles.xml"
Assert-ContainsText -Text $styleRefactorStylesXml -ExpectedText 'w:styleId="BulkNew"' -Label "Style-refactor styles.xml"
Assert-ContainsText -Text $styleRefactorStylesXml -ExpectedText 'w:styleId="BulkMergeTarget"' -Label "Style-refactor styles.xml"
Assert-NotContainsText -Text $styleRefactorStylesXml -UnexpectedText 'w:styleId="UnusedBody"' -Label "Style-refactor styles.xml"
Assert-NotContainsText -Text $styleRefactorStylesXml -UnexpectedText 'w:styleId="UnusedCharacter"' -Label "Style-refactor styles.xml"
Assert-True -Condition (Test-Path -LiteralPath $styleRefactorRollbackPath) `
    -Message "apply_style_refactor should write the requested rollback plan."

Set-Content -LiteralPath $styleRefactorRestorePlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "restore_style_merge",
      "rollback_plan": "$($styleRefactorRollbackPath.Replace('\', '\\'))",
      "entries": [1]
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $styleRefactorEditedDocx `
    -EditPlan $styleRefactorRestorePlanPath `
    -OutputDocx $styleRefactorRestoredDocx `
    -SummaryJson $styleRefactorRestoreSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the restore style merge edit plan."
}

$styleRefactorRestoreSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleRefactorRestoreSummaryPath | ConvertFrom-Json
$styleRefactorRestoredDocumentXml = Read-DocxEntryText -DocxPath $styleRefactorRestoredDocx -EntryName "word/document.xml"
$styleRefactorRestoredStylesXml = Read-DocxEntryText -DocxPath $styleRefactorRestoredDocx -EntryName "word/styles.xml"

Assert-Equal -Actual $styleRefactorRestoreSummary.status -Expected "completed" `
    -Message "Style-refactor restore summary did not report status=completed."
Assert-Equal -Actual $styleRefactorRestoreSummary.operation_count -Expected 1 `
    -Message "Style-refactor restore summary should record one operation."
Assert-Equal -Actual $styleRefactorRestoreSummary.operations[0].command -Expected "restore-style-merge" `
    -Message "restore_style_merge should use the CLI restore-style-merge command."
Assert-ContainsText -Text $styleRefactorRestoredDocumentXml -ExpectedText 'w:pStyle w:val="BulkMergeSource"' -Label "Style-refactor restored document.xml"
Assert-ContainsText -Text $styleRefactorRestoredStylesXml -ExpectedText 'w:styleId="BulkMergeSource"' -Label "Style-refactor restored styles.xml"

$removeTablePlanPath = Join-Path $resolvedWorkingDir "invoice.remove_table_plan.json"
$tableRemovedDocx = Join-Path $resolvedWorkingDir "invoice.table_removed.docx"
$removeTableSummaryPath = Join-Path $resolvedWorkingDir "invoice.remove_table.summary.json"

Set-Content -LiteralPath $removeTablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_table",
      "table_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $removeTablePlanPath `
    -OutputDocx $tableRemovedDocx `
    -SummaryJson $removeTableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the remove_table plan."
}

$removeTableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $removeTableSummaryPath | ConvertFrom-Json
$removedTableXml = Read-DocxEntryText -DocxPath $tableRemovedDocx -EntryName "word/document.xml"
$removedTableDocument = New-Object System.Xml.XmlDocument
$removedTableDocument.PreserveWhitespace = $true
$removedTableDocument.LoadXml($removedTableXml)
$removedTableNamespaceManager = New-WordNamespaceManager -Document $removedTableDocument

Assert-Equal -Actual $removeTableSummary.status -Expected "completed" `
    -Message "Remove-table summary did not report status=completed."
Assert-Equal -Actual $removeTableSummary.operation_count -Expected 1 `
    -Message "Remove-table summary should record one operation."
Assert-Equal -Actual $removeTableSummary.operations[0].op -Expected "remove_table" `
    -Message "Remove-table operation should be recorded."
Assert-Equal -Actual $removeTableSummary.operations[0].command -Expected "remove-table" `
    -Message "Remove-table operation should use the CLI remove-table command."
Assert-True -Condition ($null -eq $removedTableDocument.SelectSingleNode("//w:tbl", $removedTableNamespaceManager)) `
    -Message "Remove-table output should not contain a body table."

$insertTablePlanPath = Join-Path $resolvedWorkingDir "invoice.insert_table_plan.json"
$tableInsertedDocx = Join-Path $resolvedWorkingDir "invoice.table_inserted.docx"
$insertTableSummaryPath = Join-Path $resolvedWorkingDir "invoice.insert_table.summary.json"

Set-Content -LiteralPath $insertTablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_table_after",
      "table_index": 0,
      "row_count": 1,
      "column_count": 2
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $insertTablePlanPath `
    -OutputDocx $tableInsertedDocx `
    -SummaryJson $insertTableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the insert_table_after plan."
}

$insertTableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $insertTableSummaryPath | ConvertFrom-Json
$insertedTableXml = Read-DocxEntryText -DocxPath $tableInsertedDocx -EntryName "word/document.xml"
$insertedTableDocument = New-Object System.Xml.XmlDocument
$insertedTableDocument.PreserveWhitespace = $true
$insertedTableDocument.LoadXml($insertedTableXml)
$insertedTableNamespaceManager = New-WordNamespaceManager -Document $insertedTableDocument

Assert-Equal -Actual $insertTableSummary.status -Expected "completed" `
    -Message "Insert-table summary did not report status=completed."
Assert-Equal -Actual $insertTableSummary.operation_count -Expected 1 `
    -Message "Insert-table summary should record one operation."
Assert-Equal -Actual $insertTableSummary.operations[0].op -Expected "insert_table_after" `
    -Message "Insert-table operation should be recorded."
Assert-Equal -Actual $insertTableSummary.operations[0].command -Expected "insert-table-after" `
    -Message "Insert-table operation should use the CLI insert-table-after command."
Assert-Equal -Actual $insertedTableDocument.SelectNodes("//w:tbl", $insertedTableNamespaceManager).Count -Expected 2 `
    -Message "Insert-table output should contain the original and inserted body tables."
Assert-DocxXPath `
    -Document $insertedTableDocument `
    -NamespaceManager $insertedTableNamespaceManager `
    -XPath "//w:tbl[2]/w:tr[1]/w:tc[2]" `
    -Message "Inserted table should contain the requested second cell."

$insertLikeTablePlanPath = Join-Path $resolvedWorkingDir "invoice.insert_table_like_plan.json"
$tableLikeInsertedDocx = Join-Path $resolvedWorkingDir "invoice.table_like_inserted.docx"
$insertLikeTableSummaryPath = Join-Path $resolvedWorkingDir "invoice.insert_table_like.summary.json"

Set-Content -LiteralPath $insertLikeTablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_table_like_after",
      "table_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $insertLikeTablePlanPath `
    -OutputDocx $tableLikeInsertedDocx `
    -SummaryJson $insertLikeTableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the insert_table_like_after plan."
}

$insertLikeTableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $insertLikeTableSummaryPath | ConvertFrom-Json
$likeInsertedTableXml = Read-DocxEntryText -DocxPath $tableLikeInsertedDocx -EntryName "word/document.xml"
$likeInsertedTableDocument = New-Object System.Xml.XmlDocument
$likeInsertedTableDocument.PreserveWhitespace = $true
$likeInsertedTableDocument.LoadXml($likeInsertedTableXml)
$likeInsertedTableNamespaceManager = New-WordNamespaceManager -Document $likeInsertedTableDocument

Assert-Equal -Actual $insertLikeTableSummary.status -Expected "completed" `
    -Message "Insert-table-like summary did not report status=completed."
Assert-Equal -Actual $insertLikeTableSummary.operation_count -Expected 1 `
    -Message "Insert-table-like summary should record one operation."
Assert-Equal -Actual $insertLikeTableSummary.operations[0].op -Expected "insert_table_like_after" `
    -Message "Insert-table-like operation should be recorded."
Assert-Equal -Actual $insertLikeTableSummary.operations[0].command -Expected "insert-table-like-after" `
    -Message "Insert-table-like operation should use the CLI insert-table-like-after command."
Assert-Equal -Actual $likeInsertedTableDocument.SelectNodes("//w:tbl", $likeInsertedTableNamespaceManager).Count -Expected 2 `
    -Message "Insert-table-like output should contain the original and cloned body tables."
Assert-DocxXPath `
    -Document $likeInsertedTableDocument `
    -NamespaceManager $likeInsertedTableNamespaceManager `
    -XPath "//w:tbl[2]/w:tr[1]/w:tc[1]" `
    -Message "Cloned table should contain a first cell."
$nonEmptyCloneTextNodes = @(
    $likeInsertedTableDocument.SelectNodes("//w:tbl[2]//w:t", $likeInsertedTableNamespaceManager) |
        Where-Object { -not [string]::IsNullOrEmpty($_.InnerText) }
)
Assert-Equal -Actual $nonEmptyCloneTextNodes.Count -Expected 0 `
    -Message "Cloned table should clear copied cell text."

$insertParagraphAfterTablePlanPath = Join-Path $resolvedWorkingDir "invoice.insert_paragraph_after_table_plan.json"
$paragraphAfterTableDocx = Join-Path $resolvedWorkingDir "invoice.paragraph_after_table.docx"
$insertParagraphAfterTableSummaryPath = Join-Path $resolvedWorkingDir "invoice.insert_paragraph_after_table.summary.json"

Set-Content -LiteralPath $insertParagraphAfterTablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_paragraph_after_table",
      "table_index": 0,
      "text": "Inserted paragraph after table."
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $insertParagraphAfterTablePlanPath `
    -OutputDocx $paragraphAfterTableDocx `
    -SummaryJson $insertParagraphAfterTableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the insert_paragraph_after_table plan."
}

$insertParagraphAfterTableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $insertParagraphAfterTableSummaryPath | ConvertFrom-Json
$paragraphAfterTableXml = Read-DocxEntryText -DocxPath $paragraphAfterTableDocx -EntryName "word/document.xml"
$paragraphAfterTableDocument = New-Object System.Xml.XmlDocument
$paragraphAfterTableDocument.PreserveWhitespace = $true
$paragraphAfterTableDocument.LoadXml($paragraphAfterTableXml)
$paragraphAfterTableNamespaceManager = New-WordNamespaceManager -Document $paragraphAfterTableDocument

Assert-Equal -Actual $insertParagraphAfterTableSummary.status -Expected "completed" `
    -Message "Insert-paragraph-after-table summary did not report status=completed."
Assert-Equal -Actual $insertParagraphAfterTableSummary.operation_count -Expected 1 `
    -Message "Insert-paragraph-after-table summary should record one operation."
Assert-Equal -Actual $insertParagraphAfterTableSummary.operations[0].op -Expected "insert_paragraph_after_table" `
    -Message "Insert-paragraph-after-table operation should be recorded."
Assert-Equal -Actual $insertParagraphAfterTableSummary.operations[0].command -Expected "insert-paragraph-after-table" `
    -Message "Insert-paragraph-after-table operation should use the CLI insert-paragraph-after-table command."
Assert-DocxXPath `
    -Document $paragraphAfterTableDocument `
    -NamespaceManager $paragraphAfterTableNamespaceManager `
    -XPath "//w:tbl[1]/following-sibling::*[1][self::w:p]//w:t[.='Inserted paragraph after table.']" `
    -Message "Inserted paragraph should immediately follow the first body table."

$setTablePositionPlanPath = Join-Path $resolvedWorkingDir "invoice.set_table_position_plan.json"
$tablePositionedDocx = Join-Path $resolvedWorkingDir "invoice.table_positioned.docx"
$setTablePositionSummaryPath = Join-Path $resolvedWorkingDir "invoice.set_table_position.summary.json"

Set-Content -LiteralPath $setTablePositionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_position",
      "table_index": 0,
      "preset": "page-corner",
      "horizontal_offset": 360,
      "bottom_from_text": 288
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $setTablePositionPlanPath `
    -OutputDocx $tablePositionedDocx `
    -SummaryJson $setTablePositionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the set_table_position plan."
}

$setTablePositionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $setTablePositionSummaryPath | ConvertFrom-Json
$tablePositionedXml = Read-DocxEntryText -DocxPath $tablePositionedDocx -EntryName "word/document.xml"
$tablePositionedDocument = New-Object System.Xml.XmlDocument
$tablePositionedDocument.PreserveWhitespace = $true
$tablePositionedDocument.LoadXml($tablePositionedXml)
$tablePositionedNamespaceManager = New-WordNamespaceManager -Document $tablePositionedDocument

Assert-Equal -Actual $setTablePositionSummary.status -Expected "completed" `
    -Message "Set-table-position summary did not report status=completed."
Assert-Equal -Actual $setTablePositionSummary.operation_count -Expected 1 `
    -Message "Set-table-position summary should record one operation."
Assert-Equal -Actual $setTablePositionSummary.operations[0].op -Expected "set_table_position" `
    -Message "Set-table-position operation should be recorded."
Assert-Equal -Actual $setTablePositionSummary.operations[0].command -Expected "set-table-position" `
    -Message "Set-table-position operation should use the CLI set-table-position command."
Assert-DocxXPath `
    -Document $tablePositionedDocument `
    -NamespaceManager $tablePositionedNamespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblpPr[@w:horzAnchor='page' and @w:tblpX='360' and @w:vertAnchor='page' and @w:bottomFromText='288' and @w:tblOverlap='never']" `
    -Message "Set-table-position output should contain the requested floating table position."

$clearTablePositionPlanPath = Join-Path $resolvedWorkingDir "invoice.clear_table_position_plan.json"
$tablePositionClearedDocx = Join-Path $resolvedWorkingDir "invoice.table_position_cleared.docx"
$clearTablePositionSummaryPath = Join-Path $resolvedWorkingDir "invoice.clear_table_position.summary.json"

Set-Content -LiteralPath $clearTablePositionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_table_position",
      "table_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tablePositionedDocx `
    -EditPlan $clearTablePositionPlanPath `
    -OutputDocx $tablePositionClearedDocx `
    -SummaryJson $clearTablePositionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the clear_table_position plan."
}

$clearTablePositionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $clearTablePositionSummaryPath | ConvertFrom-Json
$tablePositionClearedXml = Read-DocxEntryText -DocxPath $tablePositionClearedDocx -EntryName "word/document.xml"
$tablePositionClearedDocument = New-Object System.Xml.XmlDocument
$tablePositionClearedDocument.PreserveWhitespace = $true
$tablePositionClearedDocument.LoadXml($tablePositionClearedXml)
$tablePositionClearedNamespaceManager = New-WordNamespaceManager -Document $tablePositionClearedDocument

Assert-Equal -Actual $clearTablePositionSummary.status -Expected "completed" `
    -Message "Clear-table-position summary did not report status=completed."
Assert-Equal -Actual $clearTablePositionSummary.operation_count -Expected 1 `
    -Message "Clear-table-position summary should record one operation."
Assert-Equal -Actual $clearTablePositionSummary.operations[0].op -Expected "clear_table_position" `
    -Message "Clear-table-position operation should be recorded."
Assert-Equal -Actual $clearTablePositionSummary.operations[0].command -Expected "clear-table-position" `
    -Message "Clear-table-position operation should use the CLI clear-table-position command."
Assert-True -Condition ($null -eq $tablePositionClearedDocument.SelectSingleNode("//w:tbl[1]/w:tblPr/w:tblpPr", $tablePositionClearedNamespaceManager)) `
    -Message "Clear-table-position output should remove the floating table position."

$tableRowsPlanPath = Join-Path $resolvedWorkingDir "invoice.table_rows_plan.json"
$tableRowsEditedDocx = Join-Path $resolvedWorkingDir "invoice.table_rows_edited.docx"
$tableRowsSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_rows.summary.json"

Set-Content -LiteralPath $tableRowsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_table_row",
      "table_index": 0,
      "cell_count": 2
    },
    {
      "op": "insert_table_row_before",
      "table_index": 0,
      "row_index": 1
    },
    {
      "op": "insert_table_row_after",
      "table_index": 0,
      "row_index": 1
    },
    {
      "op": "remove_table_row",
      "table_index": 0,
      "row_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableRowsPlanPath `
    -OutputDocx $tableRowsEditedDocx `
    -SummaryJson $tableRowsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table row edit plan."
}

$tableRowsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableRowsSummaryPath | ConvertFrom-Json
$tableRowsEditedXml = Read-DocxEntryText -DocxPath $tableRowsEditedDocx -EntryName "word/document.xml"
$tableRowsEditedDocument = New-Object System.Xml.XmlDocument
$tableRowsEditedDocument.PreserveWhitespace = $true
$tableRowsEditedDocument.LoadXml($tableRowsEditedXml)
$tableRowsEditedNamespaceManager = New-WordNamespaceManager -Document $tableRowsEditedDocument

Assert-Equal -Actual $tableRowsSummary.status -Expected "completed" `
    -Message "Table-row summary did not report status=completed."
Assert-Equal -Actual $tableRowsSummary.operation_count -Expected 4 `
    -Message "Table-row summary should record four operations."
Assert-Equal -Actual $tableRowsSummary.operations[0].op -Expected "append_table_row" `
    -Message "Append-table-row operation should be recorded."
Assert-Equal -Actual $tableRowsSummary.operations[0].command -Expected "append-table-row" `
    -Message "Append-table-row operation should use the CLI append-table-row command."
Assert-Equal -Actual $tableRowsSummary.operations[1].command -Expected "insert-table-row-before" `
    -Message "Insert-table-row-before operation should use the CLI insert-table-row-before command."
Assert-Equal -Actual $tableRowsSummary.operations[2].command -Expected "insert-table-row-after" `
    -Message "Insert-table-row-after operation should use the CLI insert-table-row-after command."
Assert-Equal -Actual $tableRowsSummary.operations[3].command -Expected "remove-table-row" `
    -Message "Remove-table-row operation should use the CLI remove-table-row command."
Assert-Equal -Actual $tableRowsEditedDocument.SelectNodes("//w:tbl[1]/w:tr", $tableRowsEditedNamespaceManager).Count -Expected 5 `
    -Message "Table-row edit output should contain the expected number of rows."

$templateTableRowsPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_rows_plan.json"
$templateTableRowsEditedDocx = Join-Path $resolvedWorkingDir "invoice.template_table_rows_edited.docx"
$templateTableRowsSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_rows.summary.json"

Set-Content -LiteralPath $templateTableRowsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_template_table_row",
      "table_index": 0,
      "cell_count": 2
    },
    {
      "op": "insert_template_table_row_before",
      "table_index": 0,
      "row_index": 1
    },
    {
      "op": "insert_template_table_row_after",
      "table_index": 0,
      "row_index": 1
    },
    {
      "op": "remove_template_table_row",
      "table_index": 0,
      "row_index": 0
    },
    {
      "op": "delete_template_table_row",
      "table_index": 0,
      "row_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableRowsPlanPath `
    -OutputDocx $templateTableRowsEditedDocx `
    -SummaryJson $templateTableRowsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table row edit plan."
}

$templateTableRowsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableRowsSummaryPath | ConvertFrom-Json
$templateTableRowsEditedXml = Read-DocxEntryText -DocxPath $templateTableRowsEditedDocx -EntryName "word/document.xml"
$templateTableRowsEditedDocument = New-Object System.Xml.XmlDocument
$templateTableRowsEditedDocument.PreserveWhitespace = $true
$templateTableRowsEditedDocument.LoadXml($templateTableRowsEditedXml)
$templateTableRowsEditedNamespaceManager = New-WordNamespaceManager -Document $templateTableRowsEditedDocument

Assert-Equal -Actual $templateTableRowsSummary.status -Expected "completed" `
    -Message "Template-table-row summary did not report status=completed."
Assert-Equal -Actual $templateTableRowsSummary.operation_count -Expected 5 `
    -Message "Template-table-row summary should record five operations."
Assert-Equal -Actual $templateTableRowsSummary.operations[0].op -Expected "append_template_table_row" `
    -Message "Append-template-table-row operation should be recorded."
Assert-Equal -Actual $templateTableRowsSummary.operations[0].command -Expected "append-template-table-row" `
    -Message "Append-template-table-row operation should use the CLI append-template-table-row command."
Assert-Equal -Actual $templateTableRowsSummary.operations[1].command -Expected "insert-template-table-row-before" `
    -Message "Insert-template-table-row-before operation should use the CLI template row command."
Assert-Equal -Actual $templateTableRowsSummary.operations[2].command -Expected "insert-template-table-row-after" `
    -Message "Insert-template-table-row-after operation should use the CLI template row command."
Assert-Equal -Actual $templateTableRowsSummary.operations[3].command -Expected "remove-template-table-row" `
    -Message "Remove-template-table-row operation should use the CLI template row command."
Assert-Equal -Actual $templateTableRowsSummary.operations[4].command -Expected "remove-template-table-row" `
    -Message "Delete-template-table-row alias should use the CLI remove-template-table-row command."
Assert-Equal -Actual $templateTableRowsEditedDocument.SelectNodes("//w:tbl[1]/w:tr", $templateTableRowsEditedNamespaceManager).Count -Expected 4 `
    -Message "Template-table-row edit output should contain the expected number of rows."

$templateTableColumnsPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_columns_plan.json"
$templateTableColumnsEditedDocx = Join-Path $resolvedWorkingDir "invoice.template_table_columns_edited.docx"
$templateTableColumnsSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_columns.summary.json"

Set-Content -LiteralPath $templateTableColumnsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_template_table_column_before",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "insert_template_table_column_after",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "remove_template_table_column",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "delete_template_table_column",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableColumnsPlanPath `
    -OutputDocx $templateTableColumnsEditedDocx `
    -SummaryJson $templateTableColumnsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table column edit plan."
}

$templateTableColumnsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableColumnsSummaryPath | ConvertFrom-Json
$templateTableColumnsEditedXml = Read-DocxEntryText -DocxPath $templateTableColumnsEditedDocx -EntryName "word/document.xml"
$templateTableColumnsEditedDocument = New-Object System.Xml.XmlDocument
$templateTableColumnsEditedDocument.PreserveWhitespace = $true
$templateTableColumnsEditedDocument.LoadXml($templateTableColumnsEditedXml)
$templateTableColumnsEditedNamespaceManager = New-WordNamespaceManager -Document $templateTableColumnsEditedDocument

Assert-Equal -Actual $templateTableColumnsSummary.status -Expected "completed" `
    -Message "Template-table-column summary did not report status=completed."
Assert-Equal -Actual $templateTableColumnsSummary.operation_count -Expected 4 `
    -Message "Template-table-column summary should record four operations."
Assert-Equal -Actual $templateTableColumnsSummary.operations[0].op -Expected "insert_template_table_column_before" `
    -Message "Insert-template-table-column-before operation should be recorded."
Assert-Equal -Actual $templateTableColumnsSummary.operations[0].command -Expected "insert-template-table-column-before" `
    -Message "Insert-template-table-column-before operation should use the CLI template column command."
Assert-Equal -Actual $templateTableColumnsSummary.operations[1].command -Expected "insert-template-table-column-after" `
    -Message "Insert-template-table-column-after operation should use the CLI template column command."
Assert-Equal -Actual $templateTableColumnsSummary.operations[2].command -Expected "remove-template-table-column" `
    -Message "Remove-template-table-column operation should use the CLI template column command."
Assert-Equal -Actual $templateTableColumnsSummary.operations[3].command -Expected "remove-template-table-column" `
    -Message "Delete-template-table-column alias should use the CLI remove-template-table-column command."
Assert-Equal -Actual $templateTableColumnsEditedDocument.SelectNodes("//w:tbl[1]/w:tblGrid/w:gridCol", $templateTableColumnsEditedNamespaceManager).Count -Expected 3 `
    -Message "Template-table-column edit output should contain the expected grid column count."
Assert-Equal -Actual $templateTableColumnsEditedDocument.SelectNodes("//w:tbl[1]/w:tr[1]/w:tc", $templateTableColumnsEditedNamespaceManager).Count -Expected 3 `
    -Message "Template-table-column edit output should contain the expected first-row cell count."

$templateTableCellTextPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_cell_text_plan.json"
$templateTableCellTextDocx = Join-Path $resolvedWorkingDir "invoice.template_table_cell_text.docx"
$templateTableCellTextSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_cell_text.summary.json"

Set-Content -LiteralPath $templateTableCellTextPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_template_table_cell_text",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "text": "Template cell text from plan"
    },
    {
      "op": "set_template_table_cell_text",
      "table_index": 0,
      "row_index": 1,
      "grid_column": 1,
      "text": "Template grid-column text"
    },
    {
      "op": "set_template_table_cell_block_texts",
      "table_index": 0,
      "row_index": 1,
      "cell_index": 0,
      "rows": [
        ["Template block A", "Template block B"],
        ["Template block C", "Template block D"]
      ]
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableCellTextPlanPath `
    -OutputDocx $templateTableCellTextDocx `
    -SummaryJson $templateTableCellTextSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table cell text plan."
}

$templateTableCellTextSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableCellTextSummaryPath | ConvertFrom-Json
$templateTableCellTextXml = Read-DocxEntryText -DocxPath $templateTableCellTextDocx -EntryName "word/document.xml"

Assert-Equal -Actual $templateTableCellTextSummary.status -Expected "completed" `
    -Message "Template-table-cell-text summary did not report status=completed."
Assert-Equal -Actual $templateTableCellTextSummary.operation_count -Expected 3 `
    -Message "Template-table-cell-text summary should record three operations."
Assert-Equal -Actual $templateTableCellTextSummary.operations[0].command -Expected "set-template-table-cell-text" `
    -Message "Set-template-table-cell-text operation should use the CLI template cell text command."
Assert-Equal -Actual $templateTableCellTextSummary.operations[1].command -Expected "set-template-table-cell-text" `
    -Message "Set-template-table-cell-text grid-column operation should use the CLI template cell text command."
Assert-Equal -Actual $templateTableCellTextSummary.operations[2].command -Expected "set-template-table-cell-block-texts" `
    -Message "Set-template-table-cell-block-texts operation should use the CLI template cell block command."
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template cell text from plan" -Label "Template table cell text output"
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template block A" -Label "Template table cell text output"
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template block B" -Label "Template table cell text output"
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template block C" -Label "Template table cell text output"
Assert-ContainsText -Text $templateTableCellTextXml -ExpectedText "Template block D" -Label "Template table cell text output"

$templateTableRowTextsPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_row_texts_plan.json"
$templateTableRowTextsDocx = Join-Path $resolvedWorkingDir "invoice.template_table_row_texts.docx"
$templateTableRowTextsSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_row_texts.summary.json"

Set-Content -LiteralPath $templateTableRowTextsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_template_table_row_texts",
      "table_index": 0,
      "row_index": 0,
      "rows": [
        ["Template row A", "Template row B", "Template row C"]
      ]
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableRowTextsPlanPath `
    -OutputDocx $templateTableRowTextsDocx `
    -SummaryJson $templateTableRowTextsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table row texts plan."
}

$templateTableRowTextsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableRowTextsSummaryPath | ConvertFrom-Json
$templateTableRowTextsXml = Read-DocxEntryText -DocxPath $templateTableRowTextsDocx -EntryName "word/document.xml"

Assert-Equal -Actual $templateTableRowTextsSummary.status -Expected "completed" `
    -Message "Template-table-row-texts summary did not report status=completed."
Assert-Equal -Actual $templateTableRowTextsSummary.operation_count -Expected 1 `
    -Message "Template-table-row-texts summary should record one operation."
Assert-Equal -Actual $templateTableRowTextsSummary.operations[0].op -Expected "set_template_table_row_texts" `
    -Message "Set-template-table-row-texts operation should be recorded."
Assert-Equal -Actual $templateTableRowTextsSummary.operations[0].command -Expected "set-template-table-row-texts" `
    -Message "Set-template-table-row-texts operation should use the CLI template row texts command."
Assert-ContainsText -Text $templateTableRowTextsXml -ExpectedText "Template row A" -Label "Template table row texts output"
Assert-ContainsText -Text $templateTableRowTextsXml -ExpectedText "Template row B" -Label "Template table row texts output"
Assert-ContainsText -Text $templateTableRowTextsXml -ExpectedText "Template row C" -Label "Template table row texts output"

$templateTableJsonPatchPlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_json_patch_plan.json"
$templateTableJsonPatchDocx = Join-Path $resolvedWorkingDir "invoice.template_table_json_patch.docx"
$templateTableJsonPatchSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_json_patch.summary.json"

Set-Content -LiteralPath $templateTableJsonPatchPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_template_table_from_json",
      "table_index": 0,
      "patch": {
        "mode": "rows",
        "start_row": 0,
        "rows": [
          ["Template JSON A", "Template JSON B", "Template JSON C"]
        ]
      }
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableJsonPatchPlanPath `
    -OutputDocx $templateTableJsonPatchDocx `
    -SummaryJson $templateTableJsonPatchSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table JSON patch plan."
}

$templateTableJsonPatchSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableJsonPatchSummaryPath | ConvertFrom-Json
$templateTableJsonPatchXml = Read-DocxEntryText -DocxPath $templateTableJsonPatchDocx -EntryName "word/document.xml"

Assert-Equal -Actual $templateTableJsonPatchSummary.status -Expected "completed" `
    -Message "Template-table-json-patch summary did not report status=completed."
Assert-Equal -Actual $templateTableJsonPatchSummary.operation_count -Expected 1 `
    -Message "Template-table-json-patch summary should record one operation."
Assert-Equal -Actual $templateTableJsonPatchSummary.operations[0].command -Expected "set-template-table-from-json" `
    -Message "Set-template-table-from-json operation should use the CLI JSON patch command."
Assert-ContainsText -Text $templateTableJsonPatchXml -ExpectedText "Template JSON A" -Label "Template table JSON patch output"
Assert-ContainsText -Text $templateTableJsonPatchXml -ExpectedText "Template JSON B" -Label "Template table JSON patch output"
Assert-ContainsText -Text $templateTableJsonPatchXml -ExpectedText "Template JSON C" -Label "Template table JSON patch output"

$templateTablesJsonPatchPlanPath = Join-Path $resolvedWorkingDir "invoice.template_tables_json_patch_plan.json"
$templateTablesJsonPatchDocx = Join-Path $resolvedWorkingDir "invoice.template_tables_json_patch.docx"
$templateTablesJsonPatchSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_tables_json_patch.summary.json"

Set-Content -LiteralPath $templateTablesJsonPatchPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_template_tables_from_json",
      "patch": {
        "operations": [
          {
            "table_index": 0,
            "mode": "rows",
            "start_row": 0,
            "rows": [
              ["Template batch A", "Template batch B", "Template batch C"]
            ]
          },
          {
            "table_index": 0,
            "mode": "block",
            "start_row": 1,
            "start_cell": 0,
            "rows": [
              ["Template batch block A"]
            ]
          }
        ]
      }
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTablesJsonPatchPlanPath `
    -OutputDocx $templateTablesJsonPatchDocx `
    -SummaryJson $templateTablesJsonPatchSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template tables JSON patch plan."
}

$templateTablesJsonPatchSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTablesJsonPatchSummaryPath | ConvertFrom-Json
$templateTablesJsonPatchXml = Read-DocxEntryText -DocxPath $templateTablesJsonPatchDocx -EntryName "word/document.xml"

Assert-Equal -Actual $templateTablesJsonPatchSummary.status -Expected "completed" `
    -Message "Template-tables-json-patch summary did not report status=completed."
Assert-Equal -Actual $templateTablesJsonPatchSummary.operation_count -Expected 1 `
    -Message "Template-tables-json-patch summary should record one operation."
Assert-Equal -Actual $templateTablesJsonPatchSummary.operations[0].command -Expected "set-template-tables-from-json" `
    -Message "Set-template-tables-from-json operation should use the CLI batch JSON patch command."
Assert-ContainsText -Text $templateTablesJsonPatchXml -ExpectedText "Template batch A" -Label "Template tables JSON patch output"
Assert-ContainsText -Text $templateTablesJsonPatchXml -ExpectedText "Template batch B" -Label "Template tables JSON patch output"
Assert-ContainsText -Text $templateTablesJsonPatchXml -ExpectedText "Template batch C" -Label "Template tables JSON patch output"
Assert-ContainsText -Text $templateTablesJsonPatchXml -ExpectedText "Template batch block A" -Label "Template tables JSON patch output"

$templateTableMergePlanPath = Join-Path $resolvedWorkingDir "invoice.template_table_merge_plan.json"
$templateTableMergeDocx = Join-Path $resolvedWorkingDir "invoice.template_table_merge.docx"
$templateTableMergeSummaryPath = Join-Path $resolvedWorkingDir "invoice.template_table_merge.summary.json"

Set-Content -LiteralPath $templateTableMergePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "merge_template_table_cells",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "direction": "right",
      "count": 2
    },
    {
      "op": "unmerge_template_table_cells",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "direction": "right"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $templateTableMergePlanPath `
    -OutputDocx $templateTableMergeDocx `
    -SummaryJson $templateTableMergeSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the template table merge plan."
}

$templateTableMergeSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $templateTableMergeSummaryPath | ConvertFrom-Json
$templateTableMergeXml = Read-DocxEntryText -DocxPath $templateTableMergeDocx -EntryName "word/document.xml"
$templateTableMergeDocument = New-Object System.Xml.XmlDocument
$templateTableMergeDocument.PreserveWhitespace = $true
$templateTableMergeDocument.LoadXml($templateTableMergeXml)
$templateTableMergeNamespaceManager = New-WordNamespaceManager -Document $templateTableMergeDocument

Assert-Equal -Actual $templateTableMergeSummary.status -Expected "completed" `
    -Message "Template-table-merge summary did not report status=completed."
Assert-Equal -Actual $templateTableMergeSummary.operation_count -Expected 2 `
    -Message "Template-table-merge summary should record two operations."
Assert-Equal -Actual $templateTableMergeSummary.operations[0].command -Expected "merge-template-table-cells" `
    -Message "Merge-template-table-cells operation should use the CLI template merge command."
Assert-Equal -Actual $templateTableMergeSummary.operations[1].command -Expected "unmerge-template-table-cells" `
    -Message "Unmerge-template-table-cells operation should use the CLI template unmerge command."
if ($null -ne $templateTableMergeDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:gridSpan", $templateTableMergeNamespaceManager)) {
    throw "Template-table merge/unmerge output should remove the first cell gridSpan."
}
Assert-Equal -Actual $templateTableMergeDocument.SelectNodes("//w:tbl[1]/w:tr[1]/w:tc", $templateTableMergeNamespaceManager).Count -Expected 3 `
    -Message "Template-table merge/unmerge output should preserve the first-row cell count."

$tableColumnsPlanPath = Join-Path $resolvedWorkingDir "invoice.table_columns_plan.json"
$tableColumnsEditedDocx = Join-Path $resolvedWorkingDir "invoice.table_columns_edited.docx"
$tableColumnsSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_columns.summary.json"

Set-Content -LiteralPath $tableColumnsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_table_column_before",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "insert_table_column_after",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 1
    },
    {
      "op": "remove_table_column",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableColumnsPlanPath `
    -OutputDocx $tableColumnsEditedDocx `
    -SummaryJson $tableColumnsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table column edit plan."
}

$tableColumnsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableColumnsSummaryPath | ConvertFrom-Json
$tableColumnsEditedXml = Read-DocxEntryText -DocxPath $tableColumnsEditedDocx -EntryName "word/document.xml"
$tableColumnsEditedDocument = New-Object System.Xml.XmlDocument
$tableColumnsEditedDocument.PreserveWhitespace = $true
$tableColumnsEditedDocument.LoadXml($tableColumnsEditedXml)
$tableColumnsEditedNamespaceManager = New-WordNamespaceManager -Document $tableColumnsEditedDocument

Assert-Equal -Actual $tableColumnsSummary.status -Expected "completed" `
    -Message "Table-column summary did not report status=completed."
Assert-Equal -Actual $tableColumnsSummary.operation_count -Expected 3 `
    -Message "Table-column summary should record three operations."
Assert-Equal -Actual $tableColumnsSummary.operations[0].op -Expected "insert_table_column_before" `
    -Message "Insert-table-column-before operation should be recorded."
Assert-Equal -Actual $tableColumnsSummary.operations[0].command -Expected "insert-table-column-before" `
    -Message "Insert-table-column-before operation should use the CLI insert-table-column-before command."
Assert-Equal -Actual $tableColumnsSummary.operations[1].command -Expected "insert-table-column-after" `
    -Message "Insert-table-column-after operation should use the CLI insert-table-column-after command."
Assert-Equal -Actual $tableColumnsSummary.operations[2].command -Expected "remove-table-column" `
    -Message "Remove-table-column operation should use the CLI remove-table-column command."
Assert-Equal -Actual $tableColumnsEditedDocument.SelectNodes("//w:tbl[1]/w:tblGrid/w:gridCol", $tableColumnsEditedNamespaceManager).Count -Expected 4 `
    -Message "Table-column edit output should contain the expected grid column count."
Assert-Equal -Actual $tableColumnsEditedDocument.SelectNodes("//w:tbl[1]/w:tr[1]/w:tc", $tableColumnsEditedNamespaceManager).Count -Expected 4 `
    -Message "Table-column edit output should contain the expected first-row cell count."

$tableColumnWidthPlanPath = Join-Path $resolvedWorkingDir "invoice.table_column_width_plan.json"
$tableColumnWidthDocx = Join-Path $resolvedWorkingDir "invoice.table_column_width.docx"
$tableColumnWidthSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_column_width.summary.json"

Set-Content -LiteralPath $tableColumnWidthPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_column_width",
      "table_index": 0,
      "column_index": 1,
      "width_twips": 2600
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableColumnWidthPlanPath `
    -OutputDocx $tableColumnWidthDocx `
    -SummaryJson $tableColumnWidthSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table column width plan."
}

$tableColumnWidthSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableColumnWidthSummaryPath | ConvertFrom-Json
$tableColumnWidthXml = Read-DocxEntryText -DocxPath $tableColumnWidthDocx -EntryName "word/document.xml"
$tableColumnWidthDocument = New-Object System.Xml.XmlDocument
$tableColumnWidthDocument.PreserveWhitespace = $true
$tableColumnWidthDocument.LoadXml($tableColumnWidthXml)
$tableColumnWidthNamespaceManager = New-WordNamespaceManager -Document $tableColumnWidthDocument

Assert-Equal -Actual $tableColumnWidthSummary.status -Expected "completed" `
    -Message "Table-column-width summary did not report status=completed."
Assert-Equal -Actual $tableColumnWidthSummary.operation_count -Expected 1 `
    -Message "Table-column-width summary should record one operation."
Assert-Equal -Actual $tableColumnWidthSummary.operations[0].command -Expected "set-table-column-width" `
    -Message "Set-table-column-width operation should use the table-column-width command."
Assert-DocxXPath `
    -Document $tableColumnWidthDocument `
    -NamespaceManager $tableColumnWidthNamespaceManager `
    -XPath "//w:tbl[1]/w:tblGrid/w:gridCol[2][@w:w='2600']" `
    -Message "Table-column-width output should set the second grid column width."
Assert-DocxXPath `
    -Document $tableColumnWidthDocument `
    -NamespaceManager $tableColumnWidthNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[2]/w:tcPr/w:tcW[@w:w='2600' and @w:type='dxa']" `
    -Message "Table-column-width output should set the second first-row cell width."

$tableColumnWidthClearPlanPath = Join-Path $resolvedWorkingDir "invoice.table_column_width_clear_plan.json"
$tableColumnWidthClearedDocx = Join-Path $resolvedWorkingDir "invoice.table_column_width_cleared.docx"
$tableColumnWidthClearSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_column_width_cleared.summary.json"

Set-Content -LiteralPath $tableColumnWidthClearPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_table_column_width",
      "table_index": 0,
      "column_index": 1
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tableColumnWidthDocx `
    -EditPlan $tableColumnWidthClearPlanPath `
    -OutputDocx $tableColumnWidthClearedDocx `
    -SummaryJson $tableColumnWidthClearSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table column width clear plan."
}

$tableColumnWidthClearSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableColumnWidthClearSummaryPath | ConvertFrom-Json
$tableColumnWidthClearedXml = Read-DocxEntryText -DocxPath $tableColumnWidthClearedDocx -EntryName "word/document.xml"
$tableColumnWidthClearedDocument = New-Object System.Xml.XmlDocument
$tableColumnWidthClearedDocument.PreserveWhitespace = $true
$tableColumnWidthClearedDocument.LoadXml($tableColumnWidthClearedXml)
$tableColumnWidthClearedNamespaceManager = New-WordNamespaceManager -Document $tableColumnWidthClearedDocument

Assert-Equal -Actual $tableColumnWidthClearSummary.status -Expected "completed" `
    -Message "Table-column-width-clear summary did not report status=completed."
Assert-Equal -Actual $tableColumnWidthClearSummary.operation_count -Expected 1 `
    -Message "Table-column-width-clear summary should record one operation."
Assert-Equal -Actual $tableColumnWidthClearSummary.operations[0].command -Expected "clear-table-column-width" `
    -Message "Clear-table-column-width operation should use the table-column-width command."

if ($null -ne $tableColumnWidthClearedDocument.SelectSingleNode("//w:tbl[1]/w:tblGrid/w:gridCol[2][@w:w]", $tableColumnWidthClearedNamespaceManager)) {
    throw "Table-column-width clear output should remove the second grid column width."
}
if ($null -ne $tableColumnWidthClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[2]/w:tcPr/w:tcW", $tableColumnWidthClearedNamespaceManager)) {
    throw "Table-column-width clear output should remove the second first-row cell width."
}

$tableRowPropertiesPlanPath = Join-Path $resolvedWorkingDir "invoice.table_row_properties_plan.json"
$tableRowPropertiesDocx = Join-Path $resolvedWorkingDir "invoice.table_row_properties.docx"
$tableRowPropertiesSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_row_properties.summary.json"

Set-Content -LiteralPath $tableRowPropertiesPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_row_height",
      "table_index": 0,
      "row_index": 0,
      "height_twips": 720,
      "rule": "exact"
    },
    {
      "op": "set_table_row_cant_split",
      "table_index": 0,
      "row_index": 0
    },
    {
      "op": "set_table_row_repeat_header",
      "table_index": 0,
      "row_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableRowPropertiesPlanPath `
    -OutputDocx $tableRowPropertiesDocx `
    -SummaryJson $tableRowPropertiesSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table row properties plan."
}

$tableRowPropertiesSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableRowPropertiesSummaryPath | ConvertFrom-Json
$tableRowPropertiesXml = Read-DocxEntryText -DocxPath $tableRowPropertiesDocx -EntryName "word/document.xml"
$tableRowPropertiesDocument = New-Object System.Xml.XmlDocument
$tableRowPropertiesDocument.PreserveWhitespace = $true
$tableRowPropertiesDocument.LoadXml($tableRowPropertiesXml)
$tableRowPropertiesNamespaceManager = New-WordNamespaceManager -Document $tableRowPropertiesDocument

Assert-Equal -Actual $tableRowPropertiesSummary.status -Expected "completed" `
    -Message "Table-row-properties summary did not report status=completed."
Assert-Equal -Actual $tableRowPropertiesSummary.operation_count -Expected 3 `
    -Message "Table-row-properties summary should record three operations."
Assert-Equal -Actual $tableRowPropertiesSummary.operations[0].command -Expected "set-table-row-height" `
    -Message "Set-table-row-height operation should use the CLI set-table-row-height command."
Assert-Equal -Actual $tableRowPropertiesSummary.operations[1].command -Expected "set-table-row-cant-split" `
    -Message "Set-table-row-cant-split operation should use the CLI set-table-row-cant-split command."
Assert-Equal -Actual $tableRowPropertiesSummary.operations[2].command -Expected "set-table-row-repeat-header" `
    -Message "Set-table-row-repeat-header operation should use the CLI set-table-row-repeat-header command."
Assert-DocxXPath `
    -Document $tableRowPropertiesDocument `
    -NamespaceManager $tableRowPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:trPr/w:trHeight[@w:val='720' and @w:hRule='exact']" `
    -Message "Table-row-properties output should set the first row height."
Assert-DocxXPath `
    -Document $tableRowPropertiesDocument `
    -NamespaceManager $tableRowPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:trPr/w:cantSplit" `
    -Message "Table-row-properties output should set cantSplit on the first row."
Assert-DocxXPath `
    -Document $tableRowPropertiesDocument `
    -NamespaceManager $tableRowPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:trPr/w:tblHeader" `
    -Message "Table-row-properties output should set repeat-header on the first row."

$tableRowPropertiesClearPlanPath = Join-Path $resolvedWorkingDir "invoice.table_row_properties_clear_plan.json"
$tableRowPropertiesClearedDocx = Join-Path $resolvedWorkingDir "invoice.table_row_properties_cleared.docx"
$tableRowPropertiesClearSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_row_properties_cleared.summary.json"

Set-Content -LiteralPath $tableRowPropertiesClearPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_table_row_height",
      "table_index": 0,
      "row_index": 0
    },
    {
      "op": "clear_table_row_cant_split",
      "table_index": 0,
      "row_index": 0
    },
    {
      "op": "clear_table_row_repeat_header",
      "table_index": 0,
      "row_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tableRowPropertiesDocx `
    -EditPlan $tableRowPropertiesClearPlanPath `
    -OutputDocx $tableRowPropertiesClearedDocx `
    -SummaryJson $tableRowPropertiesClearSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table row properties clear plan."
}

$tableRowPropertiesClearSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableRowPropertiesClearSummaryPath | ConvertFrom-Json
$tableRowPropertiesClearedXml = Read-DocxEntryText -DocxPath $tableRowPropertiesClearedDocx -EntryName "word/document.xml"
$tableRowPropertiesClearedDocument = New-Object System.Xml.XmlDocument
$tableRowPropertiesClearedDocument.PreserveWhitespace = $true
$tableRowPropertiesClearedDocument.LoadXml($tableRowPropertiesClearedXml)
$tableRowPropertiesClearedNamespaceManager = New-WordNamespaceManager -Document $tableRowPropertiesClearedDocument

Assert-Equal -Actual $tableRowPropertiesClearSummary.status -Expected "completed" `
    -Message "Table-row-properties-clear summary did not report status=completed."
Assert-Equal -Actual $tableRowPropertiesClearSummary.operation_count -Expected 3 `
    -Message "Table-row-properties-clear summary should record three operations."
Assert-Equal -Actual $tableRowPropertiesClearSummary.operations[0].command -Expected "clear-table-row-height" `
    -Message "Clear-table-row-height operation should use the CLI clear-table-row-height command."
Assert-Equal -Actual $tableRowPropertiesClearSummary.operations[1].command -Expected "clear-table-row-cant-split" `
    -Message "Clear-table-row-cant-split operation should use the CLI clear-table-row-cant-split command."
Assert-Equal -Actual $tableRowPropertiesClearSummary.operations[2].command -Expected "clear-table-row-repeat-header" `
    -Message "Clear-table-row-repeat-header operation should use the CLI clear-table-row-repeat-header command."

if ($null -ne $tableRowPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:trPr/w:trHeight", $tableRowPropertiesClearedNamespaceManager)) {
    throw "Table-row-properties clear output should remove the first row height."
}
if ($null -ne $tableRowPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:trPr/w:cantSplit", $tableRowPropertiesClearedNamespaceManager)) {
    throw "Table-row-properties clear output should remove cantSplit from the first row."
}
if ($null -ne $tableRowPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:trPr/w:tblHeader", $tableRowPropertiesClearedNamespaceManager)) {
    throw "Table-row-properties clear output should remove repeat-header from the first row."
}

$tableCellPropertiesPlanPath = Join-Path $resolvedWorkingDir "invoice.table_cell_properties_plan.json"
$tableCellPropertiesDocx = Join-Path $resolvedWorkingDir "invoice.table_cell_properties.docx"
$tableCellPropertiesSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_cell_properties.summary.json"

Set-Content -LiteralPath $tableCellPropertiesPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_table_cell_width",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "width_twips": 2400
    },
    {
      "op": "set_table_cell_margin",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "edge": "left",
      "margin_twips": 180
    },
    {
      "op": "set_table_cell_text_direction",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "direction": "top_to_bottom_right_to_left"
    },
    {
      "op": "set_table_cell_fill",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "color": "D9EAD3"
    },
    {
      "op": "set_table_cell_border",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "edge": "right",
      "style": "single",
      "size": 12,
      "color": "0088CC"
    },
    {
      "op": "set_table_cell_vertical_alignment",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "alignment": "bottom"
    },
    {
      "op": "set_table_cell_horizontal_alignment",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "alignment": "right"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $tableCellPropertiesPlanPath `
    -OutputDocx $tableCellPropertiesDocx `
    -SummaryJson $tableCellPropertiesSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table cell properties plan."
}

$tableCellPropertiesSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableCellPropertiesSummaryPath | ConvertFrom-Json
$tableCellPropertiesXml = Read-DocxEntryText -DocxPath $tableCellPropertiesDocx -EntryName "word/document.xml"
$tableCellPropertiesDocument = New-Object System.Xml.XmlDocument
$tableCellPropertiesDocument.PreserveWhitespace = $true
$tableCellPropertiesDocument.LoadXml($tableCellPropertiesXml)
$tableCellPropertiesNamespaceManager = New-WordNamespaceManager -Document $tableCellPropertiesDocument

Assert-Equal -Actual $tableCellPropertiesSummary.status -Expected "completed" `
    -Message "Table-cell-properties summary did not report status=completed."
Assert-Equal -Actual $tableCellPropertiesSummary.operation_count -Expected 7 `
    -Message "Table-cell-properties summary should record seven operations."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[0].command -Expected "set-table-cell-width" `
    -Message "Set-table-cell-width operation should use the CLI set-table-cell-width command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[1].command -Expected "set-table-cell-margin" `
    -Message "Set-table-cell-margin operation should use the CLI set-table-cell-margin command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[2].command -Expected "set-table-cell-text-direction" `
    -Message "Set-table-cell-text-direction operation should use the CLI set-table-cell-text-direction command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[3].command -Expected "set-table-cell-fill" `
    -Message "Set-table-cell-fill operation should use the CLI set-table-cell-fill command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[4].command -Expected "set-table-cell-border" `
    -Message "Set-table-cell-border operation should use the CLI set-table-cell-border command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[5].command -Expected "set-table-cell-vertical-alignment" `
    -Message "Set-table-cell-vertical-alignment operation should use the CLI set-table-cell-vertical-alignment command."
Assert-Equal -Actual $tableCellPropertiesSummary.operations[6].command -Expected "set-table-cell-horizontal-alignment" `
    -Message "Set-table-cell-horizontal-alignment operation should use the direct table-cell horizontal alignment command."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcW[@w:w='2400' and @w:type='dxa']" `
    -Message "Table-cell-properties output should set the first cell width."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcMar/w:left[@w:w='180' and @w:type='dxa']" `
    -Message "Table-cell-properties output should set the first cell left margin."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:textDirection[@w:val='tbRl']" `
    -Message "Table-cell-properties output should set the first cell text direction."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:shd[@w:fill='D9EAD3']" `
    -Message "Table-cell-properties output should set the first cell fill."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcBorders/w:right[@w:val='single' and @w:sz='12' and @w:color='0088CC']" `
    -Message "Table-cell-properties output should set the first cell right border."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:vAlign[@w:val='bottom']" `
    -Message "Table-cell-properties output should set the first cell vertical alignment."
Assert-DocxXPath `
    -Document $tableCellPropertiesDocument `
    -NamespaceManager $tableCellPropertiesNamespaceManager `
    -XPath "//w:tbl[1]/w:tr[1]/w:tc[1]/w:p[1]/w:pPr/w:jc[@w:val='right']" `
    -Message "Table-cell-properties output should set the first cell horizontal alignment."

$tableCellPropertiesClearPlanPath = Join-Path $resolvedWorkingDir "invoice.table_cell_properties_clear_plan.json"
$tableCellPropertiesClearedDocx = Join-Path $resolvedWorkingDir "invoice.table_cell_properties_cleared.docx"
$tableCellPropertiesClearSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_cell_properties_cleared.summary.json"

Set-Content -LiteralPath $tableCellPropertiesClearPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_table_cell_width",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "clear_table_cell_margin",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "edge": "left"
    },
    {
      "op": "clear_table_cell_text_direction",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "clear_table_cell_fill",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "clear_table_cell_border",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0,
      "edge": "right"
    },
    {
      "op": "clear_table_cell_vertical_alignment",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    },
    {
      "op": "clear_table_cell_horizontal_alignment",
      "table_index": 0,
      "row_index": 0,
      "cell_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $tableCellPropertiesDocx `
    -EditPlan $tableCellPropertiesClearPlanPath `
    -OutputDocx $tableCellPropertiesClearedDocx `
    -SummaryJson $tableCellPropertiesClearSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the table cell properties clear plan."
}

$tableCellPropertiesClearSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $tableCellPropertiesClearSummaryPath | ConvertFrom-Json
$tableCellPropertiesClearedXml = Read-DocxEntryText -DocxPath $tableCellPropertiesClearedDocx -EntryName "word/document.xml"
$tableCellPropertiesClearedDocument = New-Object System.Xml.XmlDocument
$tableCellPropertiesClearedDocument.PreserveWhitespace = $true
$tableCellPropertiesClearedDocument.LoadXml($tableCellPropertiesClearedXml)
$tableCellPropertiesClearedNamespaceManager = New-WordNamespaceManager -Document $tableCellPropertiesClearedDocument

Assert-Equal -Actual $tableCellPropertiesClearSummary.status -Expected "completed" `
    -Message "Table-cell-properties-clear summary did not report status=completed."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operation_count -Expected 7 `
    -Message "Table-cell-properties-clear summary should record seven operations."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[0].command -Expected "clear-table-cell-width" `
    -Message "Clear-table-cell-width operation should use the CLI clear-table-cell-width command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[1].command -Expected "clear-table-cell-margin" `
    -Message "Clear-table-cell-margin operation should use the CLI clear-table-cell-margin command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[2].command -Expected "clear-table-cell-text-direction" `
    -Message "Clear-table-cell-text-direction operation should use the CLI clear-table-cell-text-direction command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[3].command -Expected "clear-table-cell-fill" `
    -Message "Clear-table-cell-fill operation should use the CLI clear-table-cell-fill command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[4].command -Expected "clear-table-cell-border" `
    -Message "Clear-table-cell-border operation should use the CLI clear-table-cell-border command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[5].command -Expected "clear-table-cell-vertical-alignment" `
    -Message "Clear-table-cell-vertical-alignment operation should use the CLI clear-table-cell-vertical-alignment command."
Assert-Equal -Actual $tableCellPropertiesClearSummary.operations[6].command -Expected "clear-table-cell-horizontal-alignment" `
    -Message "Clear-table-cell-horizontal-alignment operation should use the direct table-cell horizontal alignment command."

if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcW", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell width."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcMar/w:left", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell left margin."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:textDirection", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell text direction."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:shd", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell fill."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:tcBorders/w:right", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell right border."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:tcPr/w:vAlign", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell vertical alignment."
}
if ($null -ne $tableCellPropertiesClearedDocument.SelectSingleNode("//w:tbl[1]/w:tr[1]/w:tc[1]/w:p[1]/w:pPr/w:jc", $tableCellPropertiesClearedNamespaceManager)) {
    throw "Table-cell-properties clear output should remove the first cell horizontal alignment."
}

$reviewSourceDocx = Join-Path $resolvedWorkingDir "review.source.docx"
$reviewPlanPath = Join-Path $resolvedWorkingDir "review.edit_plan.json"
$reviewEditedDocx = Join-Path $resolvedWorkingDir "review.edited.docx"
$reviewSummaryPath = Join-Path $resolvedWorkingDir "review.edit.summary.json"

New-ReviewFixtureDocx -Path $reviewSourceDocx

Set-Content -LiteralPath $reviewPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "accept_all_revisions"
    },
    {
      "op": "set_comment_resolved",
      "comment_index": 0,
      "resolved": true,
      "expected_text": "Commented text",
      "expected_comment_text": "Reviewer note",
      "expected_resolved": false
    },
    {
      "op": "set_comment_metadata",
      "comment_index": 0,
      "author": "Plan Reviewer",
      "clear_initials": true,
      "date": "2026-05-05T10:00:00Z",
      "expected_comment_text": "Reviewer note",
      "expected_resolved": true
    },
    {
      "op": "replace_comment",
      "comment_index": 0,
      "text": "Plan replaced comment.",
      "expected_text": "Commented text",
      "expected_comment_text": "Reviewer note",
      "expected_resolved": true
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $reviewSourceDocx `
    -EditPlan $reviewPlanPath `
    -OutputDocx $reviewEditedDocx `
    -SummaryJson $reviewSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the review/comment edit plan."
}

$reviewSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewSummaryPath | ConvertFrom-Json
$reviewDocumentXml = Read-DocxEntryText -DocxPath $reviewEditedDocx -EntryName "word/document.xml"
$reviewCommentsXml = Read-DocxEntryText -DocxPath $reviewEditedDocx -EntryName "word/comments.xml"
$reviewCommentsExtendedXml = Read-DocxEntryText -DocxPath $reviewEditedDocx -EntryName "word/commentsExtended.xml"

Assert-Equal -Actual $reviewSummary.status -Expected "completed" `
    -Message "Review summary did not report status=completed."
Assert-Equal -Actual $reviewSummary.operation_count -Expected 4 `
    -Message "Review summary should record four operations."
Assert-Equal -Actual $reviewSummary.operations[0].command -Expected "accept-all-revisions" `
    -Message "Accept-all-revisions should use the CLI revision cleanup command."
Assert-Equal -Actual $reviewSummary.operations[1].command -Expected "apply-review-mutation-plan" `
    -Message "Set-comment-resolved should use the review mutation plan command."
Assert-Equal -Actual $reviewSummary.operations[2].command -Expected "apply-review-mutation-plan" `
    -Message "Set-comment-metadata should use the review mutation plan command."
Assert-Equal -Actual $reviewSummary.operations[3].command -Expected "apply-review-mutation-plan" `
    -Message "Replace-comment should use the review mutation plan command."
Assert-ContainsText -Text $reviewDocumentXml -ExpectedText "Inserted review text" -Label "Review document.xml"
Assert-NotContainsText -Text $reviewDocumentXml -UnexpectedText "Deleted review text" -Label "Review document.xml"
Assert-NotContainsText -Text $reviewDocumentXml -UnexpectedText "<w:ins" -Label "Review document.xml"
Assert-NotContainsText -Text $reviewDocumentXml -UnexpectedText "<w:del" -Label "Review document.xml"
Assert-ContainsText -Text $reviewCommentsXml -ExpectedText "Plan replaced comment." -Label "Review comments.xml"
Assert-ContainsText -Text $reviewCommentsXml -ExpectedText 'w:author="Plan Reviewer"' -Label "Review comments.xml"
Assert-ContainsText -Text $reviewCommentsXml -ExpectedText 'w:date="2026-05-05T10:00:00Z"' -Label "Review comments.xml"
Assert-NotContainsText -Text $reviewCommentsXml -UnexpectedText 'w:initials=' -Label "Review comments.xml"
Assert-ContainsText -Text $reviewCommentsExtendedXml -ExpectedText 'w15:done="1"' -Label "Review commentsExtended.xml"

$reviewRemovePlanPath = Join-Path $resolvedWorkingDir "review.remove_comment_plan.json"
$reviewRemovedDocx = Join-Path $resolvedWorkingDir "review.comment_removed.docx"
$reviewRemoveSummaryPath = Join-Path $resolvedWorkingDir "review.remove_comment.summary.json"

Set-Content -LiteralPath $reviewRemovePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_comment",
      "comment_index": 0,
      "expected_text": "Commented text",
      "expected_comment_text": "Plan replaced comment.",
      "expected_resolved": false
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $reviewEditedDocx `
    -EditPlan $reviewRemovePlanPath `
    -OutputDocx $reviewRemovedDocx `
    -SummaryJson $reviewRemoveSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the remove_comment review plan."
}

$reviewRemoveSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewRemoveSummaryPath | ConvertFrom-Json
$reviewRemovedDocumentXml = Read-DocxEntryText -DocxPath $reviewRemovedDocx -EntryName "word/document.xml"
$reviewRemovedCommentsXml = Read-DocxEntryText -DocxPath $reviewRemovedDocx -EntryName "word/comments.xml"

Assert-Equal -Actual $reviewRemoveSummary.status -Expected "completed" `
    -Message "Remove-comment summary did not report status=completed."
Assert-Equal -Actual $reviewRemoveSummary.operation_count -Expected 1 `
    -Message "Remove-comment summary should record one operation."
Assert-Equal -Actual $reviewRemoveSummary.operations[0].command -Expected "apply-review-mutation-plan" `
    -Message "Remove-comment should use the review mutation plan command."
Assert-NotContainsText -Text $reviewRemovedDocumentXml -UnexpectedText "commentRangeStart" -Label "Review removed document.xml"
Assert-NotContainsText -Text $reviewRemovedDocumentXml -UnexpectedText "commentReference" -Label "Review removed document.xml"
Assert-NotContainsText -Text $reviewRemovedCommentsXml -UnexpectedText "Plan replaced comment." -Label "Review removed comments.xml"

$reviewRejectSourceDocx = Join-Path $resolvedWorkingDir "review.reject.source.docx"
$reviewRejectPlanPath = Join-Path $resolvedWorkingDir "review.reject_plan.json"
$reviewRejectedDocx = Join-Path $resolvedWorkingDir "review.rejected.docx"
$reviewRejectSummaryPath = Join-Path $resolvedWorkingDir "review.reject.summary.json"

New-ReviewFixtureDocx -Path $reviewRejectSourceDocx
Set-Content -LiteralPath $reviewRejectPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "reject_all_revisions"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $reviewRejectSourceDocx `
    -EditPlan $reviewRejectPlanPath `
    -OutputDocx $reviewRejectedDocx `
    -SummaryJson $reviewRejectSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the reject_all_revisions plan."
}

$reviewRejectSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewRejectSummaryPath | ConvertFrom-Json
$reviewRejectedDocumentXml = Read-DocxEntryText -DocxPath $reviewRejectedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $reviewRejectSummary.status -Expected "completed" `
    -Message "Reject-all-revisions summary did not report status=completed."
Assert-Equal -Actual $reviewRejectSummary.operation_count -Expected 1 `
    -Message "Reject-all-revisions summary should record one operation."
Assert-Equal -Actual $reviewRejectSummary.operations[0].command -Expected "reject-all-revisions" `
    -Message "Reject-all-revisions should use the CLI revision cleanup command."
Assert-ContainsText -Text $reviewRejectedDocumentXml -ExpectedText "Deleted review text" -Label "Rejected review document.xml"
Assert-NotContainsText -Text $reviewRejectedDocumentXml -UnexpectedText "Inserted review text" -Label "Rejected review document.xml"
Assert-NotContainsText -Text $reviewRejectedDocumentXml -UnexpectedText "<w:ins" -Label "Rejected review document.xml"
Assert-NotContainsText -Text $reviewRejectedDocumentXml -UnexpectedText "<w:del" -Label "Rejected review document.xml"

$linkFormulaSourceDocx = Join-Path $resolvedWorkingDir "link_formula.source.docx"
$linkFormulaPlanPath = Join-Path $resolvedWorkingDir "link_formula.edit_plan.json"
$linkFormulaEditedDocx = Join-Path $resolvedWorkingDir "link_formula.edited.docx"
$linkFormulaSummaryPath = Join-Path $resolvedWorkingDir "link_formula.edit.summary.json"

New-LinkFormulaFixtureDocx -Path $linkFormulaSourceDocx
Set-Content -LiteralPath $linkFormulaPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_hyperlink",
      "text": "Plan link v1",
      "url": "https://example.com/v1"
    },
    {
      "op": "replace_hyperlink",
      "index": 0,
      "text": "Plan link v2",
      "href": "https://example.com/v2"
    },
    {
      "op": "append_omml",
      "omml_xml": "<m:oMath xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\"><m:r><m:t>x=1</m:t></m:r></m:oMath>"
    },
    {
      "op": "replace_omml",
      "omml_index": 0,
      "omml": "<m:oMath xmlns:m=\"http://schemas.openxmlformats.org/officeDocument/2006/math\"><m:r><m:t>y=2</m:t></m:r></m:oMath>"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $linkFormulaSourceDocx `
    -EditPlan $linkFormulaPlanPath `
    -OutputDocx $linkFormulaEditedDocx `
    -SummaryJson $linkFormulaSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the hyperlink/OMML edit plan."
}

$linkFormulaSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $linkFormulaSummaryPath | ConvertFrom-Json
$linkFormulaDocumentXml = Read-DocxEntryText -DocxPath $linkFormulaEditedDocx -EntryName "word/document.xml"
$linkFormulaRelsXml = Read-DocxEntryText -DocxPath $linkFormulaEditedDocx -EntryName "word/_rels/document.xml.rels"

Assert-Equal -Actual $linkFormulaSummary.status -Expected "completed" `
    -Message "Hyperlink/OMML summary did not report status=completed."
Assert-Equal -Actual $linkFormulaSummary.operation_count -Expected 4 `
    -Message "Hyperlink/OMML summary should record four operations."
Assert-Equal -Actual $linkFormulaSummary.operations[0].command -Expected "append-hyperlink" `
    -Message "Append-hyperlink should use the CLI hyperlink command."
Assert-Equal -Actual $linkFormulaSummary.operations[1].command -Expected "replace-hyperlink" `
    -Message "Replace-hyperlink should use the CLI hyperlink command."
Assert-Equal -Actual $linkFormulaSummary.operations[2].command -Expected "append-omml" `
    -Message "Append-OMML should use the CLI OMML command."
Assert-Equal -Actual $linkFormulaSummary.operations[3].command -Expected "replace-omml" `
    -Message "Replace-OMML should use the CLI OMML command."
Assert-ContainsText -Text $linkFormulaDocumentXml -ExpectedText "w:hyperlink" -Label "Hyperlink/OMML document.xml"
Assert-ContainsText -Text $linkFormulaDocumentXml -ExpectedText "Plan link v2" -Label "Hyperlink/OMML document.xml"
Assert-NotContainsText -Text $linkFormulaDocumentXml -UnexpectedText "Plan link v1" -Label "Hyperlink/OMML document.xml"
Assert-ContainsText -Text $linkFormulaRelsXml -ExpectedText "https://example.com/v2" -Label "Hyperlink/OMML document.xml.rels"
Assert-NotContainsText -Text $linkFormulaRelsXml -UnexpectedText "https://example.com/v1" -Label "Hyperlink/OMML document.xml.rels"
Assert-ContainsText -Text $linkFormulaDocumentXml -ExpectedText "y=2" -Label "Hyperlink/OMML document.xml"
Assert-NotContainsText -Text $linkFormulaDocumentXml -UnexpectedText "x=1" -Label "Hyperlink/OMML document.xml"

$linkFormulaRemovePlanPath = Join-Path $resolvedWorkingDir "link_formula.remove_plan.json"
$linkFormulaRemovedDocx = Join-Path $resolvedWorkingDir "link_formula.removed.docx"
$linkFormulaRemoveSummaryPath = Join-Path $resolvedWorkingDir "link_formula.remove.summary.json"

Set-Content -LiteralPath $linkFormulaRemovePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "remove_hyperlink",
      "hyperlink_index": 0
    },
    {
      "op": "remove_omml",
      "formula_index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $linkFormulaEditedDocx `
    -EditPlan $linkFormulaRemovePlanPath `
    -OutputDocx $linkFormulaRemovedDocx `
    -SummaryJson $linkFormulaRemoveSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the remove hyperlink/OMML edit plan."
}

$linkFormulaRemoveSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $linkFormulaRemoveSummaryPath | ConvertFrom-Json
$linkFormulaRemovedDocumentXml = Read-DocxEntryText -DocxPath $linkFormulaRemovedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $linkFormulaRemoveSummary.status -Expected "completed" `
    -Message "Hyperlink/OMML remove summary did not report status=completed."
Assert-Equal -Actual $linkFormulaRemoveSummary.operation_count -Expected 2 `
    -Message "Hyperlink/OMML remove summary should record two operations."
Assert-Equal -Actual $linkFormulaRemoveSummary.operations[0].command -Expected "remove-hyperlink" `
    -Message "Remove-hyperlink should use the CLI hyperlink command."
Assert-Equal -Actual $linkFormulaRemoveSummary.operations[1].command -Expected "remove-omml" `
    -Message "Remove-OMML should use the CLI OMML command."
Assert-NotContainsText -Text $linkFormulaRemovedDocumentXml -UnexpectedText "w:hyperlink" -Label "Hyperlink/OMML removed document.xml"
Assert-ContainsText -Text $linkFormulaRemovedDocumentXml -ExpectedText "Plan link v2" -Label "Hyperlink/OMML removed document.xml"
Assert-NotContainsText -Text $linkFormulaRemovedDocumentXml -UnexpectedText "<m:oMath" -Label "Hyperlink/OMML removed document.xml"
Assert-NotContainsText -Text $linkFormulaRemovedDocumentXml -UnexpectedText "y=2" -Label "Hyperlink/OMML removed document.xml"

$revisionAuthoringSourceDocx = Join-Path $resolvedWorkingDir "revision_authoring.source.docx"
$revisionAuthoringPlanPath = Join-Path $resolvedWorkingDir "revision_authoring.edit_plan.json"
$revisionAuthoringEditedDocx = Join-Path $resolvedWorkingDir "revision_authoring.edited.docx"
$revisionAuthoringSummaryPath = Join-Path $resolvedWorkingDir "revision_authoring.edit.summary.json"

New-LinkFormulaFixtureDocx -Path $revisionAuthoringSourceDocx
Set-Content -LiteralPath $revisionAuthoringPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_insertion_revision",
      "text": "Plan inserted revision",
      "author": "Plan Insert Author",
      "date": "2026-05-06T09:00:00Z"
    },
    {
      "op": "append_deletion_revision",
      "text": "Plan deleted revision",
      "author": "Plan Delete Author",
      "date": "2026-05-06T10:00:00Z"
    },
    {
      "op": "set_revision_metadata",
      "revision_index": 0,
      "author": "Plan Metadata Author",
      "date": "2026-05-06T11:00:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $revisionAuthoringSourceDocx `
    -EditPlan $revisionAuthoringPlanPath `
    -OutputDocx $revisionAuthoringEditedDocx `
    -SummaryJson $revisionAuthoringSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the revision authoring edit plan."
}

$revisionAuthoringSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $revisionAuthoringSummaryPath | ConvertFrom-Json
$revisionAuthoringDocumentXml = Read-DocxEntryText -DocxPath $revisionAuthoringEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $revisionAuthoringSummary.status -Expected "completed" `
    -Message "Revision-authoring summary did not report status=completed."
Assert-Equal -Actual $revisionAuthoringSummary.operation_count -Expected 3 `
    -Message "Revision-authoring summary should record three operations."
Assert-Equal -Actual $revisionAuthoringSummary.operations[0].command -Expected "append-insertion-revision" `
    -Message "Append-insertion-revision should use the CLI revision command."
Assert-Equal -Actual $revisionAuthoringSummary.operations[1].command -Expected "append-deletion-revision" `
    -Message "Append-deletion-revision should use the CLI revision command."
Assert-Equal -Actual $revisionAuthoringSummary.operations[2].command -Expected "set-revision-metadata" `
    -Message "Set-revision-metadata should use the CLI revision command."
Assert-ContainsText -Text $revisionAuthoringDocumentXml -ExpectedText "Plan inserted revision" -Label "Revision-authoring document.xml"
Assert-ContainsText -Text $revisionAuthoringDocumentXml -ExpectedText "Plan deleted revision" -Label "Revision-authoring document.xml"
Assert-ContainsText -Text $revisionAuthoringDocumentXml -ExpectedText 'w:author="Plan Metadata Author"' -Label "Revision-authoring document.xml"
Assert-ContainsText -Text $revisionAuthoringDocumentXml -ExpectedText 'w:date="2026-05-06T11:00:00Z"' -Label "Revision-authoring document.xml"

$runRevisionSourceDocx = Join-Path $resolvedWorkingDir "run_revision.source.docx"
$runRevisionPlanPath = Join-Path $resolvedWorkingDir "run_revision.edit_plan.json"
$runRevisionEditedDocx = Join-Path $resolvedWorkingDir "run_revision.edited.docx"
$runRevisionSummaryPath = Join-Path $resolvedWorkingDir "run_revision.edit.summary.json"

New-RunRevisionFixtureDocx -Path $runRevisionSourceDocx
Set-Content -LiteralPath $runRevisionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_run_revision_after",
      "paragraph_index": 0,
      "run_index": 0,
      "text": "Plan run insertion",
      "author": "Plan Run Author",
      "date": "2026-05-06T12:00:00Z"
    },
    {
      "op": "delete_run_revision",
      "paragraph_index": 0,
      "run_index": 1,
      "author": "Plan Run Reviewer",
      "date": "2026-05-06T13:00:00Z"
    },
    {
      "op": "replace_run_revision",
      "paragraph_index": 0,
      "run_index": 1,
      "text": "Plan run replacement",
      "author": "Plan Run Editor",
      "date": "2026-05-06T14:00:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $runRevisionSourceDocx `
    -EditPlan $runRevisionPlanPath `
    -OutputDocx $runRevisionEditedDocx `
    -SummaryJson $runRevisionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the run revision edit plan."
}

$runRevisionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $runRevisionSummaryPath | ConvertFrom-Json
$runRevisionDocumentXml = Read-DocxEntryText -DocxPath $runRevisionEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $runRevisionSummary.status -Expected "completed" `
    -Message "Run-revision summary did not report status=completed."
Assert-Equal -Actual $runRevisionSummary.operation_count -Expected 3 `
    -Message "Run-revision summary should record three operations."
Assert-Equal -Actual $runRevisionSummary.operations[0].command -Expected "insert-run-revision-after" `
    -Message "Insert-run-revision-after should use the CLI run revision command."
Assert-Equal -Actual $runRevisionSummary.operations[1].command -Expected "delete-run-revision" `
    -Message "Delete-run-revision should use the CLI run revision command."
Assert-Equal -Actual $runRevisionSummary.operations[2].command -Expected "replace-run-revision" `
    -Message "Replace-run-revision should use the CLI run revision command."
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan run insertion" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan run replacement" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan Run Author" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan Run Reviewer" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "Plan Run Editor" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "<w:ins" -Label "Run-revision document.xml"
Assert-ContainsText -Text $runRevisionDocumentXml -ExpectedText "<w:del" -Label "Run-revision document.xml"

$commentAuthoringSourceDocx = Join-Path $resolvedWorkingDir "comment_authoring.source.docx"
$commentAuthoringPlanPath = Join-Path $resolvedWorkingDir "comment_authoring.edit_plan.json"
$commentAuthoringEditedDocx = Join-Path $resolvedWorkingDir "comment_authoring.edited.docx"
$commentAuthoringSummaryPath = Join-Path $resolvedWorkingDir "comment_authoring.edit.summary.json"

New-LinkFormulaFixtureDocx -Path $commentAuthoringSourceDocx
Set-Content -LiteralPath $commentAuthoringPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_comment",
      "selected_text": "Link and formula source",
      "comment_text": "Plan appended comment.",
      "author": "Plan Commenter",
      "initials": "PC",
      "date": "2026-05-06T15:00:00Z"
    },
    {
      "op": "append_comment_reply",
      "parent_comment_index": 0,
      "text": "Plan reply comment.",
      "author": "Plan Replier",
      "initials": "PR",
      "date": "2026-05-06T16:00:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $commentAuthoringSourceDocx `
    -EditPlan $commentAuthoringPlanPath `
    -OutputDocx $commentAuthoringEditedDocx `
    -SummaryJson $commentAuthoringSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the comment authoring edit plan."
}

$commentAuthoringSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $commentAuthoringSummaryPath | ConvertFrom-Json
$commentAuthoringDocumentXml = Read-DocxEntryText -DocxPath $commentAuthoringEditedDocx -EntryName "word/document.xml"
$commentAuthoringCommentsXml = Read-DocxEntryText -DocxPath $commentAuthoringEditedDocx -EntryName "word/comments.xml"

Assert-Equal -Actual $commentAuthoringSummary.status -Expected "completed" `
    -Message "Comment-authoring summary did not report status=completed."
Assert-Equal -Actual $commentAuthoringSummary.operation_count -Expected 2 `
    -Message "Comment-authoring summary should record two operations."
Assert-Equal -Actual $commentAuthoringSummary.operations[0].command -Expected "append-comment" `
    -Message "Append-comment should use the CLI comment command."
Assert-Equal -Actual $commentAuthoringSummary.operations[1].command -Expected "append-comment-reply" `
    -Message "Append-comment-reply should use the CLI comment command."
Assert-ContainsText -Text $commentAuthoringDocumentXml -ExpectedText "commentRangeStart" -Label "Comment-authoring document.xml"
Assert-ContainsText -Text $commentAuthoringDocumentXml -ExpectedText "commentReference" -Label "Comment-authoring document.xml"
Assert-ContainsText -Text $commentAuthoringCommentsXml -ExpectedText "Plan appended comment." -Label "Comment-authoring comments.xml"
Assert-ContainsText -Text $commentAuthoringCommentsXml -ExpectedText "Plan reply comment." -Label "Comment-authoring comments.xml"
Assert-ContainsText -Text $commentAuthoringCommentsXml -ExpectedText 'w:author="Plan Commenter"' -Label "Comment-authoring comments.xml"
Assert-ContainsText -Text $commentAuthoringCommentsXml -ExpectedText 'w:author="Plan Replier"' -Label "Comment-authoring comments.xml"

$paragraphRangeRevisionSourceDocx = Join-Path $resolvedWorkingDir "paragraph_range_revision.source.docx"
$paragraphRangeRevisionPlanPath = Join-Path $resolvedWorkingDir "paragraph_range_revision.edit_plan.json"
$paragraphRangeRevisionEditedDocx = Join-Path $resolvedWorkingDir "paragraph_range_revision.edited.docx"
$paragraphRangeRevisionSummaryPath = Join-Path $resolvedWorkingDir "paragraph_range_revision.edit.summary.json"

New-RunRevisionFixtureDocx -Path $paragraphRangeRevisionSourceDocx
Set-Content -LiteralPath $paragraphRangeRevisionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_paragraph_text_revision",
      "paragraph_index": 0,
      "text_offset": 5,
      "text": "Plan paragraph insertion ",
      "author": "Plan Paragraph Author",
      "date": "2026-05-06T17:00:00Z"
    },
    {
      "op": "delete_paragraph_text_revision",
      "paragraph_index": 0,
      "offset": 5,
      "length": 6,
      "expected_text": "Middle",
      "author": "Plan Paragraph Reviewer",
      "date": "2026-05-06T18:00:00Z"
    },
    {
      "op": "replace_paragraph_text_revision",
      "paragraph_index": 0,
      "offset": 5,
      "length": 6,
      "text": "Plan paragraph replacement",
      "author": "Plan Paragraph Editor",
      "date": "2026-05-06T19:00:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $paragraphRangeRevisionSourceDocx `
    -EditPlan $paragraphRangeRevisionPlanPath `
    -OutputDocx $paragraphRangeRevisionEditedDocx `
    -SummaryJson $paragraphRangeRevisionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the paragraph range revision edit plan."
}

$paragraphRangeRevisionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $paragraphRangeRevisionSummaryPath | ConvertFrom-Json
$paragraphRangeRevisionDocumentXml = Read-DocxEntryText -DocxPath $paragraphRangeRevisionEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $paragraphRangeRevisionSummary.status -Expected "completed" `
    -Message "Paragraph-range-revision summary did not report status=completed."
Assert-Equal -Actual $paragraphRangeRevisionSummary.operation_count -Expected 3 `
    -Message "Paragraph-range-revision summary should record three operations."
Assert-Equal -Actual $paragraphRangeRevisionSummary.operations[0].command -Expected "insert-paragraph-text-revision" `
    -Message "Insert-paragraph-text-revision should use the CLI paragraph revision command."
Assert-Equal -Actual $paragraphRangeRevisionSummary.operations[1].command -Expected "delete-paragraph-text-revision" `
    -Message "Delete-paragraph-text-revision should use the CLI paragraph revision command."
Assert-Equal -Actual $paragraphRangeRevisionSummary.operations[2].command -Expected "replace-paragraph-text-revision" `
    -Message "Replace-paragraph-text-revision should use the CLI paragraph revision command."
Assert-ContainsText -Text $paragraphRangeRevisionDocumentXml -ExpectedText "Plan paragraph insertion" -Label "Paragraph-range-revision document.xml"
Assert-ContainsText -Text $paragraphRangeRevisionDocumentXml -ExpectedText "Plan paragraph replacement" -Label "Paragraph-range-revision document.xml"
Assert-ContainsText -Text $paragraphRangeRevisionDocumentXml -ExpectedText "Plan Paragraph Reviewer" -Label "Paragraph-range-revision document.xml"

$textRangeRevisionSourceDocx = Join-Path $resolvedWorkingDir "text_range_revision.source.docx"
$textRangeRevisionPlanPath = Join-Path $resolvedWorkingDir "text_range_revision.edit_plan.json"
$textRangeRevisionEditedDocx = Join-Path $resolvedWorkingDir "text_range_revision.edited.docx"
$textRangeRevisionSummaryPath = Join-Path $resolvedWorkingDir "text_range_revision.edit.summary.json"

New-RangeFixtureDocx -Path $textRangeRevisionSourceDocx
Set-Content -LiteralPath $textRangeRevisionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "insert_text_range_revision",
      "paragraph_index": 0,
      "offset": 6,
      "text": "Plan text insertion ",
      "author": "Plan Text Inserter",
      "date": "2026-05-06T17:30:00Z"
    },
    {
      "op": "delete_text_range_revision",
      "start_paragraph_index": 1,
      "start_offset": 0,
      "end_paragraph_index": 1,
      "end_offset": 6,
      "expected_text": "Middle",
      "author": "Plan Text Deleter",
      "date": "2026-05-06T18:30:00Z"
    },
    {
      "op": "replace_text_range_revision",
      "start_paragraph_index": 2,
      "start_offset": 0,
      "end_paragraph_index": 2,
      "end_offset": 5,
      "text": "Plan text replacement",
      "expected_text": "Gamma",
      "author": "Plan Text Replacer",
      "date": "2026-05-06T19:30:00Z"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $textRangeRevisionSourceDocx `
    -EditPlan $textRangeRevisionPlanPath `
    -OutputDocx $textRangeRevisionEditedDocx `
    -SummaryJson $textRangeRevisionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the text range revision edit plan."
}

$textRangeRevisionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $textRangeRevisionSummaryPath | ConvertFrom-Json
$textRangeRevisionDocumentXml = Read-DocxEntryText -DocxPath $textRangeRevisionEditedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $textRangeRevisionSummary.status -Expected "completed" `
    -Message "Text-range-revision summary did not report status=completed."
Assert-Equal -Actual $textRangeRevisionSummary.operation_count -Expected 3 `
    -Message "Text-range-revision summary should record three operations."
Assert-Equal -Actual $textRangeRevisionSummary.operations[0].command -Expected "insert-text-range-revision" `
    -Message "Insert-text-range-revision should use the CLI text range revision command."
Assert-Equal -Actual $textRangeRevisionSummary.operations[1].command -Expected "delete-text-range-revision" `
    -Message "Delete-text-range-revision should use the CLI text range revision command."
Assert-Equal -Actual $textRangeRevisionSummary.operations[2].command -Expected "replace-text-range-revision" `
    -Message "Replace-text-range-revision should use the CLI text range revision command."
Assert-ContainsText -Text $textRangeRevisionDocumentXml -ExpectedText "Plan text insertion" -Label "Text-range-revision document.xml"
Assert-ContainsText -Text $textRangeRevisionDocumentXml -ExpectedText "Plan text replacement" -Label "Text-range-revision document.xml"
Assert-ContainsText -Text $textRangeRevisionDocumentXml -ExpectedText "Plan Text Deleter" -Label "Text-range-revision document.xml"

$rangeCommentSourceDocx = Join-Path $resolvedWorkingDir "range_comment.source.docx"
$rangeCommentPlanPath = Join-Path $resolvedWorkingDir "range_comment.edit_plan.json"
$rangeCommentEditedDocx = Join-Path $resolvedWorkingDir "range_comment.edited.docx"
$rangeCommentSummaryPath = Join-Path $resolvedWorkingDir "range_comment.edit.summary.json"

New-RangeFixtureDocx -Path $rangeCommentSourceDocx
Set-Content -LiteralPath $rangeCommentPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_paragraph_text_comment",
      "paragraph_index": 0,
      "offset": 0,
      "length": 3,
      "comment_text": "Plan paragraph range comment.",
      "author": "Plan Range Commenter",
      "initials": "RC",
      "date": "2026-05-06T20:00:00Z"
    },
    {
      "op": "append_text_range_comment",
      "start_paragraph_index": 0,
      "start_offset": 6,
      "end_paragraph_index": 2,
      "end_offset": 5,
      "text": "Plan cross range comment.",
      "author": "Plan Cross Commenter",
      "initials": "XC",
      "date": "2026-05-06T21:00:00Z"
    },
    {
      "op": "set_paragraph_text_comment_range",
      "comment_index": 0,
      "paragraph_index": 0,
      "offset": 6,
      "length": 4
    },
    {
      "op": "set_text_range_comment_range",
      "comment_index": 1,
      "start_paragraph_index": 1,
      "start_offset": 0,
      "end_paragraph_index": 2,
      "end_offset": 5
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $rangeCommentSourceDocx `
    -EditPlan $rangeCommentPlanPath `
    -OutputDocx $rangeCommentEditedDocx `
    -SummaryJson $rangeCommentSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the range comment edit plan."
}

$rangeCommentSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rangeCommentSummaryPath | ConvertFrom-Json
$rangeCommentDocumentXml = Read-DocxEntryText -DocxPath $rangeCommentEditedDocx -EntryName "word/document.xml"
$rangeCommentCommentsXml = Read-DocxEntryText -DocxPath $rangeCommentEditedDocx -EntryName "word/comments.xml"

Assert-Equal -Actual $rangeCommentSummary.status -Expected "completed" `
    -Message "Range-comment summary did not report status=completed."
Assert-Equal -Actual $rangeCommentSummary.operation_count -Expected 4 `
    -Message "Range-comment summary should record four operations."
Assert-Equal -Actual $rangeCommentSummary.operations[0].command -Expected "append-paragraph-text-comment" `
    -Message "Append-paragraph-text-comment should use the CLI range comment command."
Assert-Equal -Actual $rangeCommentSummary.operations[1].command -Expected "append-text-range-comment" `
    -Message "Append-text-range-comment should use the CLI range comment command."
Assert-Equal -Actual $rangeCommentSummary.operations[2].command -Expected "set-paragraph-text-comment-range" `
    -Message "Set-paragraph-text-comment-range should use the CLI range comment command."
Assert-Equal -Actual $rangeCommentSummary.operations[3].command -Expected "set-text-range-comment-range" `
    -Message "Set-text-range-comment-range should use the CLI range comment command."
Assert-ContainsText -Text $rangeCommentDocumentXml -ExpectedText "commentRangeStart" -Label "Range-comment document.xml"
Assert-ContainsText -Text $rangeCommentCommentsXml -ExpectedText "Plan paragraph range comment." -Label "Range-comment comments.xml"
Assert-ContainsText -Text $rangeCommentCommentsXml -ExpectedText "Plan cross range comment." -Label "Range-comment comments.xml"

$footnoteReviewSourceDocx = Join-Path $resolvedWorkingDir "footnote_review.source.docx"
$footnoteReviewPlanPath = Join-Path $resolvedWorkingDir "footnote_review.edit_plan.json"
$footnoteReviewEditedDocx = Join-Path $resolvedWorkingDir "footnote_review.edited.docx"
$footnoteReviewSummaryPath = Join-Path $resolvedWorkingDir "footnote_review.edit.summary.json"

New-RangeFixtureDocx -Path $footnoteReviewSourceDocx
Set-Content -LiteralPath $footnoteReviewPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_footnote",
      "reference_text": "Alpha",
      "note_text": "Plan footnote one."
    },
    {
      "op": "append_footnote",
      "anchor_text": "Middle",
      "body": "Plan footnote two."
    },
    {
      "op": "replace_footnote",
      "footnote_index": 0,
      "text": "Plan footnote replaced."
    },
    {
      "op": "remove_footnote",
      "note_index": 1
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $footnoteReviewSourceDocx `
    -EditPlan $footnoteReviewPlanPath `
    -OutputDocx $footnoteReviewEditedDocx `
    -SummaryJson $footnoteReviewSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the footnote review edit plan."
}

$footnoteReviewSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $footnoteReviewSummaryPath | ConvertFrom-Json
$footnoteReviewDocumentXml = Read-DocxEntryText -DocxPath $footnoteReviewEditedDocx -EntryName "word/document.xml"
$footnoteReviewNotesXml = Read-DocxEntryText -DocxPath $footnoteReviewEditedDocx -EntryName "word/footnotes.xml"

Assert-Equal -Actual $footnoteReviewSummary.status -Expected "completed" `
    -Message "Footnote-review summary did not report status=completed."
Assert-Equal -Actual $footnoteReviewSummary.operation_count -Expected 4 `
    -Message "Footnote-review summary should record four operations."
Assert-Equal -Actual $footnoteReviewSummary.operations[0].command -Expected "append-footnote" `
    -Message "Append-footnote should use the CLI review-note command."
Assert-Equal -Actual $footnoteReviewSummary.operations[1].command -Expected "append-footnote" `
    -Message "Second append-footnote should use the CLI review-note command."
Assert-Equal -Actual $footnoteReviewSummary.operations[2].command -Expected "replace-footnote" `
    -Message "Replace-footnote should use the CLI review-note command."
Assert-Equal -Actual $footnoteReviewSummary.operations[3].command -Expected "remove-footnote" `
    -Message "Remove-footnote should use the CLI review-note command."
Assert-ContainsText -Text $footnoteReviewDocumentXml -ExpectedText "footnoteReference" -Label "Footnote-review document.xml"
Assert-ContainsText -Text $footnoteReviewNotesXml -ExpectedText "Plan footnote replaced." -Label "Footnote-review footnotes.xml"
Assert-NotContainsText -Text $footnoteReviewNotesXml -UnexpectedText "Plan footnote two." -Label "Footnote-review footnotes.xml"

$endnoteReviewSourceDocx = Join-Path $resolvedWorkingDir "endnote_review.source.docx"
$endnoteReviewPlanPath = Join-Path $resolvedWorkingDir "endnote_review.edit_plan.json"
$endnoteReviewEditedDocx = Join-Path $resolvedWorkingDir "endnote_review.edited.docx"
$endnoteReviewSummaryPath = Join-Path $resolvedWorkingDir "endnote_review.edit.summary.json"

New-RangeFixtureDocx -Path $endnoteReviewSourceDocx
Set-Content -LiteralPath $endnoteReviewPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "append_endnote",
      "selected_text": "Beta",
      "text": "Plan endnote one."
    },
    {
      "op": "append_endnote",
      "reference_text": "Text",
      "note_text": "Plan endnote two."
    },
    {
      "op": "replace_endnote",
      "endnote_index": 1,
      "body": "Plan endnote replaced."
    },
    {
      "op": "remove_endnote",
      "index": 0
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $endnoteReviewSourceDocx `
    -EditPlan $endnoteReviewPlanPath `
    -OutputDocx $endnoteReviewEditedDocx `
    -SummaryJson $endnoteReviewSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the endnote review edit plan."
}

$endnoteReviewSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $endnoteReviewSummaryPath | ConvertFrom-Json
$endnoteReviewDocumentXml = Read-DocxEntryText -DocxPath $endnoteReviewEditedDocx -EntryName "word/document.xml"
$endnoteReviewNotesXml = Read-DocxEntryText -DocxPath $endnoteReviewEditedDocx -EntryName "word/endnotes.xml"

Assert-Equal -Actual $endnoteReviewSummary.status -Expected "completed" `
    -Message "Endnote-review summary did not report status=completed."
Assert-Equal -Actual $endnoteReviewSummary.operation_count -Expected 4 `
    -Message "Endnote-review summary should record four operations."
Assert-Equal -Actual $endnoteReviewSummary.operations[0].command -Expected "append-endnote" `
    -Message "Append-endnote should use the CLI review-note command."
Assert-Equal -Actual $endnoteReviewSummary.operations[1].command -Expected "append-endnote" `
    -Message "Second append-endnote should use the CLI review-note command."
Assert-Equal -Actual $endnoteReviewSummary.operations[2].command -Expected "replace-endnote" `
    -Message "Replace-endnote should use the CLI review-note command."
Assert-Equal -Actual $endnoteReviewSummary.operations[3].command -Expected "remove-endnote" `
    -Message "Remove-endnote should use the CLI review-note command."
Assert-ContainsText -Text $endnoteReviewDocumentXml -ExpectedText "endnoteReference" -Label "Endnote-review document.xml"
Assert-ContainsText -Text $endnoteReviewNotesXml -ExpectedText "Plan endnote replaced." -Label "Endnote-review endnotes.xml"
Assert-NotContainsText -Text $endnoteReviewNotesXml -UnexpectedText "Plan endnote one." -Label "Endnote-review endnotes.xml"

Write-Host "Edit-from-plan regression passed."
