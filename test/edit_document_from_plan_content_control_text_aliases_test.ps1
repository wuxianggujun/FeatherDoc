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

function New-ContentControlTextFixtureDocx {
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
      <w:r><w:t>Order: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Order Number"/>
          <w:tag w:val="order_no"/>
          <w:id w:val="43"/>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>INV-001</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:p>
      <w:r><w:t>Status: </w:t></w:r>
      <w:sdt>
        <w:sdtPr>
          <w:alias w:val="Status"/>
          <w:tag w:val="status"/>
          <w:id w:val="44"/>
        </w:sdtPr>
        <w:sdtContent><w:r><w:t>Pending</w:t></w:r></w:sdtContent>
      </w:sdt>
    </w:p>
    <w:sectPr/>
  </w:body>
</w:document>
'@

    $fileStream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Create)
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
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_content_control_text_aliases"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sourceDocx = Join-Path $resolvedWorkingDir "content_control_text_aliases.source.docx"
$editPlanPath = Join-Path $resolvedWorkingDir "content_control_text_aliases.edit_plan.json"
$outputDocx = Join-Path $resolvedWorkingDir "content_control_text_aliases.edited.docx"
$summaryPath = Join-Path $resolvedWorkingDir "content_control_text_aliases.summary.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null
New-ContentControlTextFixtureDocx -Path $sourceDocx

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "replace_content_control_text_by_tag",
      "content_control_tag": "order_no",
      "text": "INV-ALIAS"
    },
    {
      "op": "replace_content_control_text_by_alias",
      "content_control_alias": "Status",
      "text": "Approved"
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
    throw "edit_document_from_plan.ps1 failed for content-control text alias operations."
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $outputDocx -EntryName "word/document.xml"

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Content-control text alias summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 2 `
    -Message "Content-control text alias summary should record two operations."
Assert-Equal -Actual $summary.operations[0].op -Expected "replace_content_control_text_by_tag" `
    -Message "Tag content-control text alias should be recorded."
Assert-Equal -Actual $summary.operations[0].command -Expected "replace-content-control-text" `
    -Message "Tag content-control text alias should use the CLI replace-content-control-text command."
Assert-Equal -Actual $summary.operations[1].op -Expected "replace_content_control_text_by_alias" `
    -Message "Alias content-control text alias should be recorded."
Assert-Equal -Actual $summary.operations[1].command -Expected "replace-content-control-text" `
    -Message "Alias content-control text alias should use the CLI replace-content-control-text command."
Assert-ContainsText -Text $documentXml -ExpectedText "INV-ALIAS" -Label "Content-control text alias document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "Approved" -Label "Content-control text alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "INV-001" -Label "Content-control text alias document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "Pending" -Label "Content-control text alias document.xml"

Write-Host "Edit-plan content-control text aliases passed."
