param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) { throw "$Message Missing='$ExpectedText'." }
}

function Add-ZipTextEntry {
    param([System.IO.Compression.ZipArchive]$Archive, [string]$EntryName, [string]$Content)
    $entry = $Archive.CreateEntry($EntryName)
    $stream = $entry.Open()
    $writer = New-Object System.IO.StreamWriter($stream, (New-Object System.Text.UTF8Encoding($false)))
    try { $writer.Write($Content) } finally { $writer.Dispose(); $stream.Dispose() }
}

function New-TableLayoutDeliveryFixtureDocx {
    param([string]$DocxPath)

    New-Item -ItemType Directory -Path (Split-Path -Parent $DocxPath) -Force | Out-Null
    $contentTypesXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>
'@
    $rootRelsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/>
</Relationships>
'@
    $documentRelsXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>
'@
    $documentXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:body>
    <w:p><w:r><w:t>Table layout delivery report fixture</w:t></w:r></w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="DeliveryTable"/>
        <w:tblW w:w="7200" w:type="dxa"/>
        <w:tblLook w:val="0060" w:firstRow="0" w:lastRow="0" w:firstColumn="0" w:lastColumn="0" w:noHBand="0" w:noVBand="1"/>
      </w:tblPr>
      <w:tblGrid><w:gridCol w:w="2400"/><w:gridCol w:w="2400"/><w:gridCol w:w="2400"/></w:tblGrid>
      <w:tr>
        <w:tc><w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Header A</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Header B</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Header C</w:t></w:r></w:p></w:tc>
      </w:tr>
      <w:tr>
        <w:tc><w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Row 1 A</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Row 1 B</w:t></w:r></w:p></w:tc>
        <w:tc><w:tcPr><w:tcW w:w="2400" w:type="dxa"/></w:tcPr><w:p><w:r><w:t>Row 1 C</w:t></w:r></w:p></w:tc>
      </w:tr>
    </w:tbl>
    <w:sectPr/>
  </w:body>
</w:document>
'@
    $stylesXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>
  <w:style w:type="table" w:styleId="DeliveryTable">
    <w:name w:val="Delivery Table"/>
    <w:tblPr>
      <w:tblBorders>
        <w:top w:val="single" w:sz="8" w:space="0" w:color="4F81BD"/>
        <w:left w:val="single" w:sz="8" w:space="0" w:color="4F81BD"/>
        <w:bottom w:val="single" w:sz="8" w:space="0" w:color="4F81BD"/>
        <w:right w:val="single" w:sz="8" w:space="0" w:color="4F81BD"/>
        <w:insideH w:val="single" w:sz="4" w:space="0" w:color="B8CCE4"/>
        <w:insideV w:val="single" w:sz="4" w:space="0" w:color="B8CCE4"/>
      </w:tblBorders>
    </w:tblPr>
    <w:tblStylePr w:type="firstRow">
      <w:rPr><w:b/><w:color w:val="FFFFFF"/></w:rPr>
      <w:tcPr><w:shd w:val="clear" w:color="auto" w:fill="1F4E79"/></w:tcPr>
    </w:tblStylePr>
  </w:style>
</w:styles>
'@

    $fileStream = [System.IO.File]::Open($DocxPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::ReadWrite, [System.IO.FileShare]::None)
    $archive = New-Object System.IO.Compression.ZipArchive($fileStream, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        Add-ZipTextEntry -Archive $archive -EntryName "_rels/.rels" -Content $rootRelsXml
        Add-ZipTextEntry -Archive $archive -EntryName "[Content_Types].xml" -Content $contentTypesXml
        Add-ZipTextEntry -Archive $archive -EntryName "word/document.xml" -Content $documentXml
        Add-ZipTextEntry -Archive $archive -EntryName "word/_rels/document.xml.rels" -Content $documentRelsXml
        Add-ZipTextEntry -Archive $archive -EntryName "word/styles.xml" -Content $stylesXml
    } finally {
        $archive.Dispose()
        $fileStream.Dispose()
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_table_layout_delivery_report.ps1"
$fixtureDocx = Join-Path $resolvedWorkingDir "table-layout-delivery-fixture.docx"
$outputDir = Join-Path $resolvedWorkingDir "table-layout-delivery-report"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null
New-TableLayoutDeliveryFixtureDocx -DocxPath $fixtureDocx

& $scriptPath `
    -InputDocx $fixtureDocx `
    -BuildDir $resolvedBuildDir `
    -OutputDir $outputDir `
    -PositionPreset paragraph-callout `
    -SkipBuild
if ($LASTEXITCODE -ne 0) {
    throw "Table layout delivery report script failed."
}

$summaryPath = Join-Path $outputDir "table-layout-delivery-report.json"
$markdownPath = Join-Path $outputDir "table-layout-delivery-report.md"
$positionPlanPath = Join-Path $outputDir "plan-table-position-paragraph-callout.json"
$positionReplayPlanPath = Join-Path $outputDir "table-position-paragraph-callout.replay-plan.json"
$positionDryRunPath = Join-Path $outputDir "apply-table-position-plan-dry-run.json"
$styleDefinitionPath = Join-Path $outputDir "table-style-definitions\inspect-table-style-DeliveryTable.json"

foreach ($path in @($summaryPath, $markdownPath, $positionPlanPath, $positionReplayPlanPath, $positionDryRunPath, $styleDefinitionPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) -Message "Expected artifact was not generated: $path"
}

$summary = Get-Content -Raw -LiteralPath $summaryPath | ConvertFrom-Json
$markdown = Get-Content -Raw -LiteralPath $markdownPath
$positionPlan = Get-Content -Raw -LiteralPath $positionPlanPath | ConvertFrom-Json
$positionDryRun = Get-Content -Raw -LiteralPath $positionDryRunPath | ConvertFrom-Json
$styleDefinition = Get-Content -Raw -LiteralPath $styleDefinitionPath | ConvertFrom-Json

Assert-Equal -Actual ([int]$summary.table_count) -Expected 1 `
    -Message "Report should inspect one body table."
Assert-Equal -Actual ([int]$summary.styled_table_count) -Expected 1 `
    -Message "Report should count the fixture table style."
Assert-Equal -Actual ([int]$summary.positioned_table_count) -Expected 0 `
    -Message "Fixture should start without floating table position."
Assert-Equal -Actual ([int]$summary.unpositioned_table_count) -Expected 1 `
    -Message "Fixture should report one unpositioned table."
Assert-Equal -Actual ([string]$summary.table_style_ids[0]) -Expected "DeliveryTable" `
    -Message "Report should list the table style id."
Assert-Equal -Actual ([int]$summary.table_style_quality_issue_count) -Expected 1 `
    -Message "Disabled firstRow tblLook should be a quality issue."
Assert-Equal -Actual ([int]$summary.table_style_quality_automatic_fix_count) -Expected 1 `
    -Message "Quality plan should expose one automatic tblLook fix."
Assert-Equal -Actual ([int]$summary.table_style_quality_manual_fix_count) -Expected 0 `
    -Message "Fixture should not require manual style definition fixes."
Assert-Equal -Actual ([int]$summary.table_style_look_issue_count) -Expected 1 `
    -Message "tblLook gate should expose one issue."
Assert-Equal -Actual ([int]$summary.position_automatic_count) -Expected 1 `
    -Message "Position preset should be applicable to the unpositioned table."
Assert-Equal -Actual ([int]$summary.position_review_count) -Expected 0 `
    -Message "Unpositioned fixture should not require replacement review."
Assert-Equal -Actual ([bool]$summary.position_plan_dry_run_ok) -Expected $true `
    -Message "Position replay dry-run should pass."
Assert-Equal -Actual ([bool]$positionDryRun.ok) -Expected $true `
    -Message "Position dry-run JSON should be ok."
Assert-Equal -Actual ([int]$positionPlan.automatic_count) -Expected 1 `
    -Message "Position plan JSON should report one automatic change."
Assert-Equal -Actual ([string]$styleDefinition.table_style_definition.style.style_id) -Expected "DeliveryTable" `
    -Message "Style definition artifact should inspect DeliveryTable."

$suggestionIds = @($summary.suggested_actions | ForEach-Object { [string]$_.id })
Assert-True -Condition ($suggestionIds -contains "apply-table-style-quality-look-only") `
    -Message "Report should suggest safe table style quality repair."
Assert-True -Condition ($suggestionIds -contains "repair-table-style-look") `
    -Message "Report should suggest tblLook repair."
Assert-True -Condition ($suggestionIds -contains "apply-table-position-plan") `
    -Message "Report should suggest applying the floating table preset plan."

$visualIds = @($summary.visual_regression_entries | ForEach-Object { [string]$_.id })
Assert-True -Condition ($visualIds -contains "word-visual-smoke") `
    -Message "Report should expose a Word visual smoke entry."
Assert-True -Condition ($visualIds -contains "table-style-quality-visual-regression") `
    -Message "Report should expose the curated table style visual regression entry."

Assert-ContainsText -Text $markdown -ExpectedText "Table layout delivery report" `
    -Message "Markdown should have the report title."
Assert-ContainsText -Text $markdown -ExpectedText "Apply safe tblLook quality fixes" `
    -Message "Markdown should include the quality repair suggestion."
Assert-ContainsText -Text $markdown -ExpectedText "Apply floating table position preset" `
    -Message "Markdown should include the position repair suggestion."
Assert-ContainsText -Text $markdown -ExpectedText "run_table_style_quality_visual_regression.ps1" `
    -Message "Markdown should include the visual regression entry."

$scriptText = Get-Content -Raw -LiteralPath $scriptPath
Assert-ContainsText -Text $scriptText -ExpectedText "audit-table-style-quality" `
    -Message "Delivery script should call the table style quality audit."
Assert-ContainsText -Text $scriptText -ExpectedText "check-table-style-look" `
    -Message "Delivery script should call the tblLook checker."
Assert-ContainsText -Text $scriptText -ExpectedText "plan-table-position-presets" `
    -Message "Delivery script should call the position preset planner."
Assert-ContainsText -Text $scriptText -ExpectedText "run_word_visual_smoke.ps1" `
    -Message "Delivery script should expose a visual smoke entry."

Write-Host "Table layout delivery report regression passed."
