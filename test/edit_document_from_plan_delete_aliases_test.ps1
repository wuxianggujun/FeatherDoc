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

function New-DeleteAliasFixtureDocx {
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
    $documentRelationshipsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rHyperlink1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink"
                Target="https://example.com/delete-alias"
                TargetMode="External"/>
</Relationships>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
            xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math">
  <w:body>
    <w:p><w:r><w:t>Before delete aliases</w:t></w:r></w:p>
    <w:p>
      <w:bookmarkStart w:id="1" w:name="delete_alias_block"/>
      <w:r><w:t>Delete alias bookmark block</w:t></w:r>
      <w:bookmarkEnd w:id="1"/>
    </w:p>
    <w:p>
      <w:hyperlink r:id="rHyperlink1" w:history="1">
        <w:r><w:t>Delete alias link</w:t></w:r>
      </w:hyperlink>
    </w:p>
    <w:p>
      <m:oMath>
        <m:r><m:t>d=1</m:t></m:r>
      </m:oMath>
    </w:p>
    <w:p><w:r><w:t>After delete aliases</w:t></w:r></w:p>
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
            Add-ZipTextEntry -Archive $archive -EntryName "word/_rels/document.xml.rels" -Content $documentRelationshipsXml
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
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_delete_aliases"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sourceDocx = Join-Path $resolvedWorkingDir "delete_aliases.source.docx"
$editPlanPath = Join-Path $resolvedWorkingDir "delete_aliases.edit_plan.json"
$outputDocx = Join-Path $resolvedWorkingDir "delete_aliases.edited.docx"
$summaryPath = Join-Path $resolvedWorkingDir "delete_aliases.summary.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null
New-DeleteAliasFixtureDocx -Path $sourceDocx

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "delete_bookmark_block",
      "bookmark": "delete_alias_block"
    },
    {
      "op": "delete_hyperlink",
      "index": 0
    },
    {
      "op": "delete_omml",
      "index": 0
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
    throw "edit_document_from_plan.ps1 failed for delete alias operations."
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $outputDocx -EntryName "word/document.xml"

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Delete-alias summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 3 `
    -Message "Delete-alias summary should record three operations."
Assert-Equal -Actual $summary.operations[0].op -Expected "delete_bookmark_block" `
    -Message "Bookmark delete alias operation should be recorded."
Assert-Equal -Actual $summary.operations[0].command -Expected "remove-bookmark-block" `
    -Message "Bookmark delete alias should use the CLI remove-bookmark-block command."
Assert-Equal -Actual $summary.operations[1].op -Expected "delete_hyperlink" `
    -Message "Hyperlink delete alias operation should be recorded."
Assert-Equal -Actual $summary.operations[1].command -Expected "remove-hyperlink" `
    -Message "Hyperlink delete alias should use the CLI remove-hyperlink command."
Assert-Equal -Actual $summary.operations[2].op -Expected "delete_omml" `
    -Message "OMML delete alias operation should be recorded."
Assert-Equal -Actual $summary.operations[2].command -Expected "remove-omml" `
    -Message "OMML delete alias should use the CLI remove-omml command."
Assert-ContainsText -Text $documentXml -ExpectedText "Before delete aliases" -Label "Delete-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "After delete aliases" -Label "Delete-alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "Delete alias link" -Label "Delete-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "Delete alias bookmark block" -Label "Delete-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "delete_alias_block" -Label "Delete-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "w:hyperlink" -Label "Delete-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "<m:oMath" -Label "Delete-alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "d=1" -Label "Delete-alias document.xml"

Write-Host "Edit-plan delete aliases passed."
