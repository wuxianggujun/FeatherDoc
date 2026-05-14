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

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build-msvc-nmake"
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_numbering_catalog"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$cliPath = Join-Path $resolvedBuildDir "featherdoc_cli.exe"
$sourceWithNumberingDocx = Join-Path $resolvedWorkingDir "invoice.numbering_source.docx"
$editPlanPath = Join-Path $resolvedWorkingDir "invoice.import_numbering_catalog.edit_plan.json"
$catalogPath = Join-Path $resolvedWorkingDir "invoice.numbering_catalog.json"
$outputDocx = Join-Path $resolvedWorkingDir "invoice.import_numbering_catalog.docx"
$summaryPath = Join-Path $resolvedWorkingDir "invoice.import_numbering_catalog.summary.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

& $cliPath `
    ensure-numbering-definition `
    $sampleDocx `
    --definition-name `
    PlanImportedOutline `
    --numbering-level `
    "0:decimal:1:%1." `
    --output `
    $sourceWithNumberingDocx `
    --json | Out-Null

if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_cli failed to generate the numbering definition fixture."
}

& $cliPath `
    export-numbering-catalog `
    $sourceWithNumberingDocx `
    --output `
    $catalogPath `
    --json | Out-Null

if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_cli failed to export the numbering catalog fixture."
}

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "import_numbering_catalog",
      "catalog_file": "$($catalogPath.Replace('\', '\\'))"
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $editPlanPath `
    -OutputDocx $outputDocx `
    -SummaryJson $summaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the import_numbering_catalog operation."
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$numberingXml = Read-DocxEntryText -DocxPath $outputDocx -EntryName "word/numbering.xml"

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Import-numbering-catalog summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 1 `
    -Message "Import-numbering-catalog summary should record one operation."
Assert-Equal -Actual $summary.operations[0].op -Expected "import_numbering_catalog" `
    -Message "Import-numbering-catalog operation should be recorded."
Assert-Equal -Actual $summary.operations[0].command -Expected "import-numbering-catalog" `
    -Message "Import-numbering-catalog operation should use the CLI import-numbering-catalog command."
Assert-ContainsText -Text $numberingXml -ExpectedText 'w:name w:val="PlanImportedOutline"' -Label "Imported numbering.xml"
Assert-ContainsText -Text $numberingXml -ExpectedText 'w:lvlText w:val="%1."' -Label "Imported numbering.xml"

Write-Host "Edit-plan numbering catalog import passed."
