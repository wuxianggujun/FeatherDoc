param(
    [string]$BuildDir = "build-table-style-quality-visual-nmake",
    [string]$OutputDir = "output/table-style-quality-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = ""
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Write-Step { param([string]$Message) Write-Host "[table-style-quality-visual] $Message" }
function Resolve-RepoRoot { return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path }
function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$InputPath)
    if ([System.IO.Path]::IsPathRooted($InputPath)) { return [System.IO.Path]::GetFullPath($InputPath) }
    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}
function Get-VcvarsPath {
    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )
    foreach ($path in $candidates) { if (Test-Path $path) { return $path } }
    throw "Could not locate vcvars64.bat for MSVC command-line builds."
}
function Invoke-MsvcCommand {
    param([string]$VcvarsPath, [string]$CommandText)
    & cmd.exe /c "call `"$VcvarsPath`" && $CommandText"
    if ($LASTEXITCODE -ne 0) { throw "MSVC command failed: $CommandText" }
}
function Find-BuildExecutable {
    param([string]$BuildRoot, [string]$TargetName)
    $candidates = @(Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } |
        Sort-Object LastWriteTime -Descending)
    if ($candidates.Count -eq 0) { throw "Could not find built executable for target '$TargetName' under $BuildRoot." }
    return $candidates[0].FullName
}
function Resolve-BuildSearchRoot {
    param([string]$RepoRoot, [string]$PreferredBuildRoot, [string[]]$TargetNames)
    $candidateRoots = @()
    if (Test-Path $PreferredBuildRoot) { $candidateRoots += (Resolve-Path $PreferredBuildRoot).Path }
    $candidateRoots += @(Get-ChildItem -Path $RepoRoot -Directory -Filter "build*" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -ExpandProperty FullName)
    foreach ($root in ($candidateRoots | Select-Object -Unique)) {
        $hasAllTargets = $true
        foreach ($targetName in $TargetNames) {
            $target = Get-ChildItem -Path $root -Recurse -File -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -ieq "$targetName.exe" -or $_.Name -ieq $targetName } |
                Select-Object -First 1
            if (-not $target) { $hasAllTargets = $false; break }
        }
        if ($hasAllTargets) { return $root }
    }
    throw "Could not locate a build directory containing targets: $($TargetNames -join ', ')."
}
function Get-RenderPython {
    param([string]$RepoRoot)
    $venvPython = Join-Path $RepoRoot ".venv-word-visual-smoke\Scripts\python.exe"
    if (Test-Path $venvPython) { return $venvPython }
    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) { return $python.Source }
    throw "Python was not found in PATH."
}
function Invoke-CommandCapture {
    param([string]$ExecutablePath, [string[]]$Arguments, [string]$OutputPath, [string]$FailureMessage)
    $commandOutput = @(& $ExecutablePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) { $lines | Set-Content -Path $OutputPath -Encoding UTF8 }
    foreach ($line in $lines) { Write-Host $line }
    if ($exitCode -ne 0) { throw $FailureMessage }
}
function Add-ZipTextEntry {
    param([System.IO.Compression.ZipArchive]$Archive, [string]$EntryName, [string]$Content)
    $entry = $Archive.CreateEntry($EntryName)
    $stream = $entry.Open()
    $writer = New-Object System.IO.StreamWriter($stream, (New-Object System.Text.UTF8Encoding($false)))
    try { $writer.Write($Content) } finally { $writer.Dispose(); $stream.Dispose() }
}
function New-TableStyleQualityFixtureDocx {
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
    <w:p><w:r><w:t>Table style quality look-only visual validation</w:t></w:r></w:p>
    <w:tbl>
      <w:tblPr>
        <w:tblStyle w:val="QualityLookTable"/>
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
    <w:p><w:r><w:t>Expected after apply: first row gets dark fill and bold white text.</w:t></w:r></w:p>
    <w:sectPr/>
  </w:body>
</w:document>
'@
    $stylesXml = @'
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal"><w:name w:val="Normal"/></w:style>
  <w:style w:type="table" w:styleId="QualityLookTable">
    <w:name w:val="Quality Look Table"/>
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
function Read-JsonFile { param([string]$Path) return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json }
function Assert-TableStyleQualityResult {
    param([object]$BeforeAudit, [object]$BeforePlan, [object]$ApplyResult, [object]$AfterAudit)
    if ([int]$BeforeAudit.issue_count -ne 1) { throw "Expected before audit to report one issue, got $($BeforeAudit.issue_count)." }
    if ([int]$BeforeAudit.style_look_issue_count -ne 1) { throw "Expected before audit to report one style look issue, got $($BeforeAudit.style_look_issue_count)." }
    if ([int]$BeforePlan.automatic_fix_count -ne 1) { throw "Expected before plan to expose one automatic fix, got $($BeforePlan.automatic_fix_count)." }
    if ([int]$BeforePlan.manual_fix_count -ne 0) { throw "Expected before plan to expose zero manual fixes, got $($BeforePlan.manual_fix_count)." }
    if (-not [bool]$ApplyResult.ok) { throw "Expected apply-table-style-quality-fixes to return ok=true." }
    if ([int]$ApplyResult.changed_table_count -ne 1) { throw "Expected one changed table, got $($ApplyResult.changed_table_count)." }
    if ([int]$ApplyResult.after_issue_count -ne 0) { throw "Expected apply result after_issue_count=0, got $($ApplyResult.after_issue_count)." }
    if ([int]$AfterAudit.issue_count -ne 0) { throw "Expected after audit to report zero issues, got $($AfterAudit.issue_count)." }
}
function Invoke-WordSmoke {
    param([string]$ScriptPath, [string]$BuildDir, [string]$DocxPath, [string]$OutputDir, [int]$Dpi, [bool]$KeepWordOpen, [bool]$VisibleWord, [string]$ReviewVerdict, [string]$ReviewNote)
    & $ScriptPath -BuildDir $BuildDir -InputDocx $DocxPath -OutputDir $OutputDir -SkipBuild -Dpi $Dpi -KeepWordOpen:$KeepWordOpen -VisibleWord:$VisibleWord -ReviewVerdict $ReviewVerdict -ReviewNote $ReviewNote
    if ($LASTEXITCODE -ne 0) { throw "Word visual smoke failed for $DocxPath." }
}
function Get-RenderedPagePath {
    param([string]$VisualOutputDir, [int]$PageNumber, [string]$Label)
    $pagePath = Join-Path $VisualOutputDir ("evidence\pages\page-{0:D2}.png" -f $PageNumber)
    if (-not (Test-Path $pagePath)) { throw "Missing rendered page $PageNumber for $Label under $VisualOutputDir." }
    return $pagePath
}
function Build-ContactSheet {
    param([string]$Python, [string]$ScriptPath, [string]$OutputPath, [string[]]$Images, [string[]]$Labels)
    & $Python $ScriptPath --output $OutputPath --columns 2 --thumb-width 540 --images $Images --labels $Labels
    if ($LASTEXITCODE -ne 0) { throw "Failed to build contact sheet at $OutputPath." }
}
function Write-ReviewMarkdown {
    param([string]$Path, [string]$SummaryPath, [string]$BeforeDocxPath, [string]$AfterDocxPath, [AllowNull()][string]$BeforeAfterContactSheetPath, [AllowNull()][string]$PixelSummaryPath, [bool]$VisualEnabled)
    $lines = @(
        "# Table style quality visual regression review", "",
        "- Summary JSON: $SummaryPath",
        "- Before DOCX: $BeforeDocxPath",
        "- After DOCX: $AfterDocxPath",
        "- Visual enabled: $VisualEnabled", "",
        "## Expected cues", "",
        "- Before: the first table row has no dark blue fill because tblLook firstRow is disabled.",
        "- After: the first table row has dark blue fill and bold white header text.",
        "- Body row text and table borders remain readable after the look-only repair.", "",
        "## Review steps", "",
        "- Inspect before/after DOCX and audit JSON files together.",
        "- If visual output is enabled, inspect before_after_contact_sheet.png first.",
        "- Confirm pixel summary reports more dark-blue pixels after repair than before."
    )
    if ($BeforeAfterContactSheetPath) { $lines += "- Before/after contact sheet: $BeforeAfterContactSheetPath" }
    if ($PixelSummaryPath) { $lines += "- Pixel summary: $PixelSummaryPath" }
    $lines | Set-Content -Path $Path -Encoding UTF8
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$pixelSummaryScript = Join-Path $repoRoot "scripts\summarize_table_style_quality_pixels.py"
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_CLI=ON -DBUILD_TESTING=OFF"
    Write-Step "Building featherdoc_cli"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli"
}
$buildSearchRoot = Resolve-BuildSearchRoot -RepoRoot $repoRoot -PreferredBuildRoot $resolvedBuildDir -TargetNames @("featherdoc_cli")
if ($buildSearchRoot -ne $resolvedBuildDir) { Write-Step "Using existing build directory $buildSearchRoot" }
$cliExecutable = Find-BuildExecutable -BuildRoot $buildSearchRoot -TargetName "featherdoc_cli"

$beforeDocxPath = Join-Path $resolvedOutputDir "quality-look-before.docx"
$afterDocxPath = Join-Path $resolvedOutputDir "quality-look-after.docx"
$beforeAuditJsonPath = Join-Path $resolvedOutputDir "before-audit-table-style-quality.json"
$beforePlanJsonPath = Join-Path $resolvedOutputDir "before-plan-table-style-quality-fixes.json"
$applyResultJsonPath = Join-Path $resolvedOutputDir "apply-table-style-quality-fixes.json"
$afterAuditJsonPath = Join-Path $resolvedOutputDir "after-audit-table-style-quality.json"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$summaryMarkdownPath = Join-Path $resolvedOutputDir "summary.md"
$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"

Write-Step "Generating look-only table style fixture DOCX"
New-TableStyleQualityFixtureDocx -DocxPath $beforeDocxPath
Write-Step "Auditing table style quality before repair"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("audit-table-style-quality", $beforeDocxPath, "--json") -OutputPath $beforeAuditJsonPath -FailureMessage "audit-table-style-quality failed before repair."
Write-Step "Planning table style quality fixes"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("plan-table-style-quality-fixes", $beforeDocxPath, "--json") -OutputPath $beforePlanJsonPath -FailureMessage "plan-table-style-quality-fixes failed before repair."
Write-Step "Applying look-only table style quality fixes"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("apply-table-style-quality-fixes", $beforeDocxPath, "--look-only", "--output", $afterDocxPath, "--json") -OutputPath $applyResultJsonPath -FailureMessage "apply-table-style-quality-fixes failed."
Write-Step "Auditing table style quality after repair"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("audit-table-style-quality", $afterDocxPath, "--json") -OutputPath $afterAuditJsonPath -FailureMessage "audit-table-style-quality failed after repair."

$beforeAudit = Read-JsonFile -Path $beforeAuditJsonPath
$beforePlan = Read-JsonFile -Path $beforePlanJsonPath
$applyResult = Read-JsonFile -Path $applyResultJsonPath
$afterAudit = Read-JsonFile -Path $afterAuditJsonPath
Assert-TableStyleQualityResult -BeforeAudit $beforeAudit -BeforePlan $beforePlan -ApplyResult $applyResult -AfterAudit $afterAudit

$visualArtifacts = $null
$beforeAfterContactSheetPath = $null
$pixelSummaryJsonPath = $null
$pixelSummaryTextPath = $null
if (-not $SkipVisual) {
    $beforeVisualDir = Join-Path $resolvedOutputDir "before-render"
    $afterVisualDir = Join-Path $resolvedOutputDir "after-render"
    $beforeAfterContactSheetPath = Join-Path $resolvedOutputDir "before_after_contact_sheet.png"
    New-Item -ItemType Directory -Path $aggregateEvidenceDir -Force | Out-Null
    $pixelSummaryJsonPath = Join-Path $resolvedOutputDir "pixel-summary.json"
    $pixelSummaryTextPath = Join-Path $resolvedOutputDir "pixel-summary.txt"
    Write-Step "Rendering before DOCX with Word"
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $beforeDocxPath -OutputDir $beforeVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent -ReviewVerdict $ReviewVerdict -ReviewNote $ReviewNote
    Write-Step "Rendering after DOCX with Word"
    Invoke-WordSmoke -ScriptPath $wordSmokeScript -BuildDir $BuildDir -DocxPath $afterDocxPath -OutputDir $afterVisualDir -Dpi $Dpi -KeepWordOpen $KeepWordOpen.IsPresent -VisibleWord $VisibleWord.IsPresent -ReviewVerdict $ReviewVerdict -ReviewNote $ReviewNote
    $beforePagePath = Get-RenderedPagePath -VisualOutputDir $beforeVisualDir -PageNumber 1 -Label "before"
    $afterPagePath = Get-RenderedPagePath -VisualOutputDir $afterVisualDir -PageNumber 1 -Label "after"
    $renderPython = Get-RenderPython -RepoRoot $repoRoot
    Write-Step "Building before/after contact sheet"
    Build-ContactSheet -Python $renderPython -ScriptPath $contactSheetScript -OutputPath $beforeAfterContactSheetPath -Images @($beforePagePath, $afterPagePath) -Labels @("before_tblLook_disabled", "after_look_only_apply")
    Copy-Item -LiteralPath $beforeAfterContactSheetPath -Destination $aggregateContactSheetPath -Force
    Write-Step "Computing pixel summary"
    & $renderPython $pixelSummaryScript --before $beforePagePath --after $afterPagePath --output-json $pixelSummaryJsonPath --output-text $pixelSummaryTextPath
    if ($LASTEXITCODE -ne 0) { throw "Failed to compute table style quality pixel summary." }
    $pixelSummary = Read-JsonFile -Path $pixelSummaryJsonPath
    if ([int64]$pixelSummary.after_dark_blue_pixels -le [int64]$pixelSummary.before_dark_blue_pixels) { throw "Expected after rendering to contain more dark-blue header pixels than before." }
    $visualArtifacts = [ordered]@{
        before_visual_dir = $beforeVisualDir; after_visual_dir = $afterVisualDir
        before_page = $beforePagePath; after_page = $afterPagePath
        before_contact_sheet = Join-Path $beforeVisualDir "evidence\contact_sheet.png"
        after_contact_sheet = Join-Path $afterVisualDir "evidence\contact_sheet.png"
        before_after_contact_sheet = $beforeAfterContactSheetPath
        aggregate_evidence_dir = $aggregateEvidenceDir
        aggregate_contact_sheet = $aggregateContactSheetPath
        pixel_summary_json = $pixelSummaryJsonPath; pixel_summary_text = $pixelSummaryTextPath
        pixel_summary = $pixelSummary
    }
}
$expectedVisualCues = @(
    "Before repair, tblLook disables firstRow so the header row does not receive the dark blue first-row style.",
    "After look-only repair, the first row renders with dark blue fill and bold white text.",
    "The body row remains plain and readable, confirming the repair only changed style routing flags."
)
$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    requested_build_dir = $resolvedBuildDir
    build_dir = $buildSearchRoot
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    cli_executable = $cliExecutable
    before_docx = $beforeDocxPath
    after_docx = $afterDocxPath
    before_audit_json = $beforeAuditJsonPath
    before_plan_json = $beforePlanJsonPath
    apply_result_json = $applyResultJsonPath
    after_audit_json = $afterAuditJsonPath
    before_issue_count = [int]$beforeAudit.issue_count
    after_issue_count = [int]$afterAudit.issue_count
    before_automatic_fix_count = [int]$beforePlan.automatic_fix_count
    before_manual_fix_count = [int]$beforePlan.manual_fix_count
    changed_table_count = [int]$applyResult.changed_table_count
    expected_visual_cues = $expectedVisualCues
    visual_artifacts = $visualArtifacts
    aggregate_evidence = if ($SkipVisual) { $null } else { [ordered]@{
        root = $aggregateEvidenceDir
        contact_sheet = $aggregateContactSheetPath
        before_after_contact_sheet = $beforeAfterContactSheetPath
        pixel_summary_json = $pixelSummaryJsonPath
        pixel_summary_text = $pixelSummaryTextPath
    } }
}
($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8
$reviewManifest = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = (-not $SkipVisual.IsPresent)
    summary_json = $summaryPath
    review_checklist = $reviewChecklistPath
    final_review = $finalReviewPath
    aggregate_evidence = if ($SkipVisual) { $null } else { [ordered]@{
        root = $aggregateEvidenceDir
        contact_sheet = $aggregateContactSheetPath
        before_after_contact_sheet = $beforeAfterContactSheetPath
        pixel_summary_json = $pixelSummaryJsonPath
        pixel_summary_text = $pixelSummaryTextPath
    } }
    cases = @([ordered]@{
        id = "look-only-tblLook-repair"
        before_docx = $beforeDocxPath
        after_docx = $afterDocxPath
        before_audit_json = $beforeAuditJsonPath
        before_plan_json = $beforePlanJsonPath
        apply_result_json = $applyResultJsonPath
        after_audit_json = $afterAuditJsonPath
        expected_visual_cues = $expectedVisualCues
        visual_artifacts = $visualArtifacts
    })
}
($reviewManifest | ConvertTo-Json -Depth 10) | Set-Content -Path $reviewManifestPath -Encoding UTF8
Write-ReviewMarkdown -Path $reviewChecklistPath -SummaryPath $summaryPath -BeforeDocxPath $beforeDocxPath -AfterDocxPath $afterDocxPath -BeforeAfterContactSheetPath $beforeAfterContactSheetPath -PixelSummaryPath $pixelSummaryTextPath -VisualEnabled (-not $SkipVisual.IsPresent)
$summaryMarkdown = @(
    "# Table style quality visual regression summary", "",
    "- Before issues: $($summary.before_issue_count)",
    "- After issues: $($summary.after_issue_count)",
    "- Automatic fixes before apply: $($summary.before_automatic_fix_count)",
    "- Manual fixes before apply: $($summary.before_manual_fix_count)",
    "- Changed tables: $($summary.changed_table_count)",
    "- Visual enabled: $($summary.visual_enabled)",
    "- Summary JSON: $summaryPath",
    "- Review manifest: $reviewManifestPath"
)
if ($beforeAfterContactSheetPath) { $summaryMarkdown += "- Before/after contact sheet: $beforeAfterContactSheetPath" }
if ($pixelSummaryTextPath) { $summaryMarkdown += "- Pixel summary: $pixelSummaryTextPath" }
$summaryMarkdown | Set-Content -Path $summaryMarkdownPath -Encoding UTF8
$finalReviewMarkdown = @(
    "# Table style quality visual final review", "",
    "## Verdict", "",
    "- Current verdict: $ReviewVerdict",
    "- Review note: $ReviewNote",
    "- Visual evidence generated: $(-not $SkipVisual.IsPresent)", "",
    "## Evidence", "",
    "- Summary JSON: $summaryPath",
    "- Review checklist: $reviewChecklistPath",
    "- Before DOCX: $beforeDocxPath",
    "- After DOCX: $afterDocxPath"
)
if ($beforeAfterContactSheetPath) { $finalReviewMarkdown += "- Before/after contact sheet: $beforeAfterContactSheetPath" }
$finalReviewMarkdown | Set-Content -Path $finalReviewPath -Encoding UTF8
Write-Step "Completed table style quality visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
if ($beforeAfterContactSheetPath) { Write-Host "Before/after contact sheet: $beforeAfterContactSheetPath" }
