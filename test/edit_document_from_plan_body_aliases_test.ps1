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

function New-BodyAliasFixtureDocx {
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
    <w:p><w:r><w:t>Replace alias old text</w:t></w:r></w:p>
    <w:p><w:r><w:t>Format alias target</w:t></w:r></w:p>
    <w:p><w:r><w:t>Paragraph style alias target</w:t></w:r></w:p>
    <w:p><w:r><w:t>Alignment alias target</w:t></w:r></w:p>
    <w:p><w:pPr><w:jc w:val="left"/></w:pPr><w:r><w:t>Horizontal clear alias target</w:t></w:r></w:p>
    <w:p><w:pPr><w:jc w:val="right"/></w:pPr><w:r><w:t>Clear alignment alias target</w:t></w:r></w:p>
    <w:p><w:r><w:t>Line spacing alias target</w:t></w:r></w:p>
    <w:p><w:pPr><w:spacing w:before="120" w:after="240" w:line="360" w:lineRule="exact"/></w:pPr><w:r><w:t>Clear spacing alias target</w:t></w:r></w:p>
    <w:p><w:r><w:t>Delete paragraph alias target</w:t></w:r></w:p>
    <w:p><w:r><w:t>Remove paragraph alias target</w:t></w:r></w:p>
    <w:p><w:r><w:t>Keep paragraph alias target</w:t></w:r></w:p>
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
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_body_aliases"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sourceDocx = Join-Path $resolvedWorkingDir "body_aliases.source.docx"
$editPlanPath = Join-Path $resolvedWorkingDir "body_aliases.edit_plan.json"
$outputDocx = Join-Path $resolvedWorkingDir "body_aliases.edited.docx"
$summaryPath = Join-Path $resolvedWorkingDir "body_aliases.summary.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null
New-BodyAliasFixtureDocx -Path $sourceDocx

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "replace_document_text",
      "find": "Replace alias old text",
      "replacement": "Replace alias new text"
    },
    {
      "op": "set_text_format",
      "contains": "Format alias target",
      "bold": true,
      "font": "Aptos",
      "east_asia_font": "SimSun",
      "font_color": "#336699",
      "font_size": 11
    },
    {
      "op": "set_paragraph_text_style",
      "text_contains": "Paragraph style alias target",
      "bold": false,
      "font_family": "Courier New",
      "font_size_half_points": 24
    },
    {
      "op": "set_paragraph_alignment",
      "text_contains": "Alignment alias target",
      "alignment": "center"
    },
    {
      "op": "clear_paragraph_alignment",
      "text_contains": "Clear alignment alias target"
    },
    {
      "op": "clear_paragraph_horizontal_alignment",
      "text_contains": "Horizontal clear alias target"
    },
    {
      "op": "set_paragraph_line_spacing",
      "text_contains": "Line spacing alias target",
      "line_twips": 480,
      "line_rule": "at_least"
    },
    {
      "op": "clear_paragraph_spacing",
      "text_contains": "Clear spacing alias target"
    },
    {
      "op": "delete_paragraph",
      "text_contains": "Delete paragraph alias target"
    },
    {
      "op": "remove_paragraph",
      "text_contains": "Remove paragraph alias target"
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
    throw "edit_document_from_plan.ps1 failed for body alias operations."
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $outputDocx -EntryName "word/document.xml"

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Body-alias summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 10 `
    -Message "Body-alias summary should record ten operations."
Assert-Equal -Actual $summary.operations[0].op -Expected "replace_document_text" `
    -Message "Text replacement alias should be recorded."
Assert-Equal -Actual $summary.operations[0].command -Expected "replace-text" `
    -Message "Text replacement alias should use the direct replace-text command."
Assert-Equal -Actual $summary.operations[1].op -Expected "set_text_format" `
    -Message "Text format alias should be recorded."
Assert-Equal -Actual $summary.operations[1].command -Expected "set-text-style" `
    -Message "Text format alias should use the direct text-style command."
Assert-Equal -Actual $summary.operations[2].op -Expected "set_paragraph_text_style" `
    -Message "Paragraph text-style alias should be recorded."
Assert-Equal -Actual $summary.operations[2].command -Expected "set-text-style" `
    -Message "Paragraph text-style alias should use the direct text-style command."
Assert-Equal -Actual $summary.operations[3].command -Expected "set-paragraph-horizontal-alignment" `
    -Message "Paragraph alignment alias should use the direct alignment command."
Assert-Equal -Actual $summary.operations[4].command -Expected "clear-paragraph-horizontal-alignment" `
    -Message "Paragraph clear-alignment alias should use the direct clear-alignment command."
Assert-Equal -Actual $summary.operations[5].op -Expected "clear_paragraph_horizontal_alignment" `
    -Message "Paragraph horizontal clear-alignment alias should be recorded."
Assert-Equal -Actual $summary.operations[5].command -Expected "clear-paragraph-horizontal-alignment" `
    -Message "Paragraph horizontal clear-alignment alias should use the direct clear-alignment command."
Assert-Equal -Actual $summary.operations[6].op -Expected "set_paragraph_line_spacing" `
    -Message "Paragraph line-spacing alias should be recorded."
Assert-Equal -Actual $summary.operations[6].command -Expected "set-paragraph-spacing" `
    -Message "Paragraph line-spacing alias should use the direct paragraph-spacing command."
Assert-Equal -Actual $summary.operations[7].op -Expected "clear_paragraph_spacing" `
    -Message "Paragraph clear-spacing alias should be recorded."
Assert-Equal -Actual $summary.operations[7].command -Expected "clear-paragraph-spacing" `
    -Message "Paragraph clear-spacing alias should use the direct clear-spacing command."
Assert-Equal -Actual $summary.operations[8].command -Expected "delete-paragraph" `
    -Message "Delete paragraph alias should use the direct delete-paragraph command."
Assert-Equal -Actual $summary.operations[9].command -Expected "delete-paragraph" `
    -Message "Remove paragraph alias should use the direct delete-paragraph command."

Assert-ContainsText -Text $documentXml -ExpectedText "Replace alias new text" -Label "Body-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "Replace alias old text" -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:b w:val="1"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:rFonts w:ascii="Aptos" w:hAnsi="Aptos" w:eastAsia="SimSun"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:color w:val="336699"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:sz w:val="22"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:b w:val="0"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:rFonts w:ascii="Courier New" w:hAnsi="Courier New"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:sz w:val="24"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:jc w:val="center"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "Clear alignment alias target" -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "Horizontal clear alias target" -Label "Body-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText 'w:jc w:val="right"' -Label "Body-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText 'w:jc w:val="left"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "Line spacing alias target" -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'w:spacing w:line="480" w:lineRule="atLeast"' -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "Clear spacing alias target" -Label "Body-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText 'w:spacing w:before="120" w:after="240" w:line="360" w:lineRule="exact"' -Label "Body-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "Delete paragraph alias target" -Label "Body-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "Remove paragraph alias target" -Label "Body-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "Keep paragraph alias target" -Label "Body-alias document.xml"

Write-Host "Edit-plan body aliases passed."
