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

function New-CustomXmlContentControlFixtureDocx {
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
    <w:sdt>
      <w:sdtPr>
        <w:alias w:val="Due Date"/>
        <w:tag w:val="due_date"/>
        <w:dataBinding w:storeItemID="{66666666-6666-6666-6666-666666666666}" w:xpath="/invoice/dueDate"/>
      </w:sdtPr>
      <w:sdtContent><w:p><w:r><w:t>Pending date</w:t></w:r></w:p></w:sdtContent>
    </w:sdt>
  </w:body>
</w:document>
'@
    $customXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<invoice><dueDate>2026-08-20</dueDate></invoice>
'@
    $itemPropsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ds:datastoreItem ds:itemID="{66666666-6666-6666-6666-666666666666}"
                  xmlns:ds="http://schemas.openxmlformats.org/officeDocument/2006/customXml">
  <ds:schemaRefs/>
</ds:datastoreItem>
'@
    $itemRelationshipsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/customXmlProps"
                Target="itemProps1.xml"/>
</Relationships>
'@

    $fileStream = [System.IO.File]::Open($Path, [System.IO.FileMode]::Create)
    try {
        $archive = New-Object System.IO.Compression.ZipArchive($fileStream, [System.IO.Compression.ZipArchiveMode]::Create)
        try {
            Add-ZipTextEntry -Archive $archive -EntryName "_rels/.rels" -Content $relationshipsXml
            Add-ZipTextEntry -Archive $archive -EntryName "[Content_Types].xml" -Content $contentTypesXml
            Add-ZipTextEntry -Archive $archive -EntryName "word/document.xml" -Content $documentXml
            Add-ZipTextEntry -Archive $archive -EntryName "customXml/item1.xml" -Content $customXml
            Add-ZipTextEntry -Archive $archive -EntryName "customXml/itemProps1.xml" -Content $itemPropsXml
            Add-ZipTextEntry -Archive $archive -EntryName "customXml/_rels/item1.xml.rels" -Content $itemRelationshipsXml
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
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_content_control_sync"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sourceDocx = Join-Path $resolvedWorkingDir "custom_xml_content_control.source.docx"
$editPlanPath = Join-Path $resolvedWorkingDir "custom_xml_content_control.edit_plan.json"
$outputDocx = Join-Path $resolvedWorkingDir "custom_xml_content_control.edited.docx"
$summaryPath = Join-Path $resolvedWorkingDir "custom_xml_content_control.summary.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null
New-CustomXmlContentControlFixtureDocx -Path $sourceDocx

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "sync_content_controls_from_custom_xml"
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
    throw "edit_document_from_plan.ps1 failed for the custom XML content-control sync operation."
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $outputDocx -EntryName "word/document.xml"

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Custom XML content-control sync summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 1 `
    -Message "Custom XML content-control sync summary should record one operation."
Assert-Equal -Actual $summary.operations[0].op -Expected "sync_content_controls_from_custom_xml" `
    -Message "Custom XML content-control sync operation should be recorded."
Assert-Equal -Actual $summary.operations[0].command -Expected "sync-content-controls-from-custom-xml" `
    -Message "Custom XML content-control sync operation should use the CLI sync command."
Assert-ContainsText -Text $documentXml -ExpectedText "<w:t>2026-08-20</w:t>" -Label "Custom XML content-control document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "Pending date" -Label "Custom XML content-control document.xml"

Write-Host "Edit-plan content-control Custom XML sync passed."
