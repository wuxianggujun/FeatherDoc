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

function New-WordNamespaceManager {
    param([System.Xml.XmlDocument]$Document)

    $namespaceManager = New-Object System.Xml.XmlNamespaceManager($Document.NameTable)
    $null = $namespaceManager.AddNamespace("w", "http://schemas.openxmlformats.org/wordprocessingml/2006/main")
    Write-Output -NoEnumerate $namespaceManager
}

function Read-DocxDocumentXml {
    param([string]$DocxPath)

    $xml = Read-DocxEntryText -DocxPath $DocxPath -EntryName "word/document.xml"
    $document = New-Object System.Xml.XmlDocument
    $document.PreserveWhitespace = $true
    $document.LoadXml($xml)
    return $document
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build-msvc-nmake"
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_table_position_plan"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$cliPath = Join-Path $resolvedBuildDir "featherdoc_cli.exe"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$generatedPlanPath = Join-Path $resolvedWorkingDir "invoice.table_position.generated.json"
& $cliPath `
    plan-table-position-presets `
    $sampleDocx `
    --preset `
    paragraph-callout `
    --output-plan `
    $generatedPlanPath `
    --json | Out-Null

if ($LASTEXITCODE -ne 0) {
    throw "featherdoc_cli failed to generate the table position plan fixture."
}

$planFileEditPlanPath = Join-Path $resolvedWorkingDir "invoice.table_position.plan_file.edit_plan.json"
$planFileOutputDocx = Join-Path $resolvedWorkingDir "invoice.table_position.plan_file.docx"
$planFileSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_position.plan_file.summary.json"

Set-Content -LiteralPath $planFileEditPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "apply_table_position_plan",
      "plan_file": "$($generatedPlanPath.Replace('\', '\\'))"
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $planFileEditPlanPath `
    -OutputDocx $planFileOutputDocx `
    -SummaryJson $planFileSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the apply_table_position_plan plan_file operation."
}

$planFileSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $planFileSummaryPath | ConvertFrom-Json
$planFileDocument = Read-DocxDocumentXml -DocxPath $planFileOutputDocx
$planFileNamespaceManager = New-WordNamespaceManager -Document $planFileDocument

Assert-Equal -Actual $planFileSummary.status -Expected "completed" `
    -Message "Apply-table-position-plan summary did not report status=completed."
Assert-Equal -Actual $planFileSummary.operation_count -Expected 1 `
    -Message "Apply-table-position-plan summary should record one operation."
Assert-Equal -Actual $planFileSummary.operations[0].op -Expected "apply_table_position_plan" `
    -Message "Apply-table-position-plan operation should be recorded."
Assert-Equal -Actual $planFileSummary.operations[0].command -Expected "apply-table-position-plan" `
    -Message "Apply-table-position-plan operation should use the CLI apply-table-position-plan command."
Assert-DocxXPath `
    -Document $planFileDocument `
    -NamespaceManager $planFileNamespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblpPr[@w:horzAnchor='text' and @w:tblpX='0' and @w:vertAnchor='text' and @w:tblpY='0' and @w:tblOverlap='never']" `
    -Message "Apply-table-position-plan output should contain the replayed floating table position."

$inlinePlan = Get-Content -Raw -Encoding UTF8 -LiteralPath $generatedPlanPath | ConvertFrom-Json
$inlinePlan.preset = "margin-anchor"
$inlineEditPlanPath = Join-Path $resolvedWorkingDir "invoice.table_position.inline.edit_plan.json"
$inlineOutputDocx = Join-Path $resolvedWorkingDir "invoice.table_position.inline.docx"
$inlineSummaryPath = Join-Path $resolvedWorkingDir "invoice.table_position.inline.summary.json"
$inlineEditPlan = [pscustomobject]@{
    operations = @(
        [pscustomobject]@{
            op = "apply_table_position_plan"
            table_position_plan = $inlinePlan
        }
    )
}
($inlineEditPlan | ConvertTo-Json -Depth 64) |
    Set-Content -LiteralPath $inlineEditPlanPath -Encoding UTF8

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $inlineEditPlanPath `
    -OutputDocx $inlineOutputDocx `
    -SummaryJson $inlineSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the inline apply_table_position_plan operation."
}

$inlineSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $inlineSummaryPath | ConvertFrom-Json
$inlineDocument = Read-DocxDocumentXml -DocxPath $inlineOutputDocx
$inlineNamespaceManager = New-WordNamespaceManager -Document $inlineDocument

Assert-Equal -Actual $inlineSummary.status -Expected "completed" `
    -Message "Inline apply-table-position-plan summary did not report status=completed."
Assert-Equal -Actual $inlineSummary.operations[0].command -Expected "apply-table-position-plan" `
    -Message "Inline apply-table-position-plan should use the CLI apply-table-position-plan command."
Assert-DocxXPath `
    -Document $inlineDocument `
    -NamespaceManager $inlineNamespaceManager `
    -XPath "//w:tbl[1]/w:tblPr/w:tblpPr[@w:horzAnchor='margin' and @w:tblpX='0' and @w:vertAnchor='text' and @w:tblpY='0' and @w:tblOverlap='never']" `
    -Message "Inline apply-table-position-plan output should honor the inline preset."

Write-Host "Edit-plan table position plan bridge passed."
