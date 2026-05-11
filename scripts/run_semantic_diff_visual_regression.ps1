param(
    [string]$BuildDir = "build-semantic-diff-visual",
    [string]$OutputDir = "output/semantic-diff-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = "Confirm the before/after DOCX pair renders readable changed paragraphs, review markers, content control text, table values, and style-linked numbering after Word PDF export."
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Write-Step {
    param([string]$Message)
    Write-Host "[semantic-diff-visual] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$InputPath)
    if ([IO.Path]::IsPathRooted($InputPath)) {
        return [IO.Path]::GetFullPath($InputPath)
    }
    return [IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}

function Find-BuildExecutable {
    param([string]$BuildRoot, [string]$TargetName)
    $candidates = @(
        Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } |
            Sort-Object LastWriteTime -Descending
    )
    if ($candidates.Count -eq 0) {
        throw "Could not find built executable for target '$TargetName' under $BuildRoot."
    }
    return $candidates[0].FullName
}

function Get-DocxEntryText {
    param([string]$DocxPath, [string]$EntryName)
    $archive = [IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entry = $archive.GetEntry($EntryName)
        if ($null -eq $entry) { throw "Missing $EntryName" }
        $stream = $entry.Open()
        try {
            $reader = [IO.StreamReader]::new($stream, [Text.Encoding]::UTF8)
            try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
        } finally { $stream.Dispose() }
    } finally { $archive.Dispose() }
}

function Assert-GeneratedDocxEvidence {
    param([string]$LeftDocx, [string]$RightDocx)
    foreach ($docx in @($LeftDocx, $RightDocx)) {
        if (-not (Test-Path $docx)) { throw "Generated DOCX was not created: $docx" }
    }
    $leftXml = Get-DocxEntryText $LeftDocx "word/document.xml"
    $rightXml = Get-DocxEntryText $RightDocx "word/document.xml"
    $leftHeaderXml = Get-DocxEntryText $LeftDocx "word/header1.xml"
    $rightHeaderXml = Get-DocxEntryText $RightDocx "word/header1.xml"
    $leftFooterXml = Get-DocxEntryText $LeftDocx "word/footer1.xml"
    $rightFooterXml = Get-DocxEntryText $RightDocx "word/footer1.xml"
    $leftStylesXml = Get-DocxEntryText $LeftDocx "word/styles.xml"
    $rightStylesXml = Get-DocxEntryText $RightDocx "word/styles.xml"
    $leftNumberingXml = Get-DocxEntryText $LeftDocx "word/numbering.xml"
    $rightNumberingXml = Get-DocxEntryText $RightDocx "word/numbering.xml"
    $leftFootnotesXml = Get-DocxEntryText $LeftDocx "word/footnotes.xml"
    $rightFootnotesXml = Get-DocxEntryText $RightDocx "word/footnotes.xml"
    $leftEndnotesXml = Get-DocxEntryText $LeftDocx "word/endnotes.xml"
    $rightEndnotesXml = Get-DocxEntryText $RightDocx "word/endnotes.xml"
    $leftCommentsXml = Get-DocxEntryText $LeftDocx "word/comments.xml"
    $rightCommentsXml = Get-DocxEntryText $RightDocx "word/comments.xml"
    foreach ($text in @("Semantic diff visual - BEFORE", "Status: draft", "Ada Lovelace", "`$120", "Review footnote marker", "Review comment anchor", "Review revision anchor text")) {
        if (-not $leftXml.Contains($text)) { throw "Missing left semantic evidence: $text" }
    }
    foreach ($text in @("Semantic diff visual - AFTER", "Status: approved", "Grace Hopper", "`$150", "Review endnote marker", "Review comment anchor", "Review revision anchor text")) {
        if (-not $rightXml.Contains($text)) { throw "Missing right semantic evidence: $text" }
    }
    if (-not $leftHeaderXml.Contains("Header BEFORE")) { throw "Missing left header semantic evidence." }
    if (-not $rightHeaderXml.Contains("Header AFTER")) { throw "Missing right header semantic evidence." }
    if (-not $leftFooterXml.Contains("Footer BEFORE")) { throw "Missing left footer semantic evidence." }
    if (-not $rightFooterXml.Contains("Footer AFTER")) { throw "Missing right footer semantic evidence." }
    foreach ($text in @("Semantic Diff Heading Draft", "SemanticDiffHeading", "Heading1")) {
        if (-not $leftStylesXml.Contains($text)) { throw "Missing left styles semantic evidence: $text" }
    }
    foreach ($text in @("Semantic Diff Heading Approved", "SemanticDiffHeading", "Title")) {
        if (-not $rightStylesXml.Contains($text)) { throw "Missing right styles semantic evidence: $text" }
    }
    foreach ($text in @("SemanticDiffDraftOutline", 'w:numFmt w:val="decimal"', 'w:start w:val="1"', 'w:lvlText w:val="%1."')) {
        if (-not $leftNumberingXml.Contains($text)) { throw "Missing left numbering semantic evidence: $text" }
    }
    foreach ($text in @("SemanticDiffApprovedOutline", 'w:numFmt w:val="bullet"', 'w:start w:val="3"', 'w:lvlText w:val="o"')) {
        if (-not $rightNumberingXml.Contains($text)) { throw "Missing right numbering semantic evidence: $text" }
    }
    if (-not $leftFootnotesXml.Contains("Semantic original footnote")) { throw "Missing left footnote semantic evidence." }
    if (-not $rightFootnotesXml.Contains("Semantic approved footnote")) { throw "Missing right footnote semantic evidence." }
    if (-not $leftEndnotesXml.Contains("Semantic original endnote")) { throw "Missing left endnote semantic evidence." }
    if (-not $rightEndnotesXml.Contains("Semantic approved endnote")) { throw "Missing right endnote semantic evidence." }
    if (-not $leftCommentsXml.Contains("Semantic original comment")) { throw "Missing left comment semantic evidence." }
    if (-not $rightCommentsXml.Contains("Semantic approved comment")) { throw "Missing right comment semantic evidence." }
}

function Assert-DiffJsonEvidence {
    param([string]$JsonPath)
    if (-not (Test-Path $JsonPath)) { throw "Semantic diff JSON was not created: $JsonPath" }
    $json = Get-Content -Raw -Path $JsonPath
    foreach ($text in @('"command":"semantic-diff"', '"different":true', '"change_count":15', '"align_sequences_by_content":true', '"added_count":1', '"paragraph_changes"', '"table_changes"', '"content_control_changes"', '"style_changes"', '"numbering_changes"', '"footnote_changes"', '"endnote_changes"', '"comment_changes"', '"revision_changes"', '"section_changes"', '"template_parts"', '"template_part_results"', '"part":"header"', '"part":"footer"', '"part":"section-header"', '"part":"section-footer"', '"section_index":0', '"reference_kind":"default"', '"left_resolved_from_section_index":0', '"right_resolved_from_section_index":0', '"entry_name":"word/header1.xml"', '"entry_name":"word/footer1.xml"', '"field":"style"', '"field":"numbering"', '"field":"footnote"', '"field":"endnote"', '"field":"comment"', '"field":"revision"', '"field_path":"name"', '"field_path":"based_on"', '"field_path":"levels.kind"', '"field_path":"author"', '"field_changes"', '"field_path":"page_setup.orientation"', '"field_path":"text"', 'Semantic Diff Heading Approved', 'SemanticDiffApprovedOutline', 'kind=bullet', 'orientation=landscape', 'page_number_start=5', 'Inserted approval note', 'Semantic approved footnote', 'Semantic approved endnote', 'Semantic approved comment', 'Grace Hopper', 'Header AFTER', 'Footer AFTER')) {
        if (-not $json.Contains($text)) { throw "Missing semantic diff JSON evidence: $text" }
    }
}
$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath $repoRoot $BuildDir
$resolvedOutputDir = Resolve-RepoPath $repoRoot $OutputDir
$targetName = "featherdoc_sample_semantic_diff_visual"
$leftDocxPath = Join-Path $resolvedOutputDir "semantic_diff_left.docx"
$rightDocxPath = Join-Path $resolvedOutputDir "semantic_diff_right.docx"
$diffJsonPath = Join-Path $resolvedOutputDir "semantic_diff.json"
$leftSmokeOutputDir = Join-Path $resolvedOutputDir "word-smoke-left"
$rightSmokeOutputDir = Join-Path $resolvedOutputDir "word-smoke-right"
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

if (-not $SkipBuild) {
    Write-Step "Configuring sample build directory $resolvedBuildDir"
    & cmake -S $repoRoot -B $resolvedBuildDir -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
    Write-Step "Building $targetName and featherdoc_cli"
    & cmake --build $resolvedBuildDir --target $targetName featherdoc_cli --config Debug
    if ($LASTEXITCODE -ne 0) { throw "Sample or CLI build failed." }
}

$sampleExecutable = Find-BuildExecutable $resolvedBuildDir $targetName
$cliExecutable = Find-BuildExecutable $resolvedBuildDir "featherdoc_cli"

Write-Step "Generating before/after DOCX pair via $sampleExecutable"
& $sampleExecutable $resolvedOutputDir
if ($LASTEXITCODE -ne 0) { throw "Semantic diff sample failed." }

Write-Step "Checking generated DOCX XML evidence"
Assert-GeneratedDocxEvidence $leftDocxPath $rightDocxPath

Write-Step "Running semantic-diff JSON check"
& $cliExecutable semantic-diff $leftDocxPath $rightDocxPath --json | Set-Content -Encoding UTF8 -Path $diffJsonPath
if ($LASTEXITCODE -ne 0) { throw "semantic-diff CLI failed." }
Assert-DiffJsonEvidence $diffJsonPath

if (-not $SkipVisual) {
    Write-Step "Running Word visual smoke for left DOCX"
    & $wordSmokeScript -InputDocx $leftDocxPath -OutputDir $leftSmokeOutputDir -SkipBuild -Dpi $Dpi -KeepWordOpen:$KeepWordOpen.IsPresent -VisibleWord:$VisibleWord.IsPresent -ReviewVerdict $ReviewVerdict -ReviewNote $ReviewNote
    if ($LASTEXITCODE -ne 0) { throw "Word visual smoke failed for left DOCX." }

    Write-Step "Running Word visual smoke for right DOCX"
    & $wordSmokeScript -InputDocx $rightDocxPath -OutputDir $rightSmokeOutputDir -SkipBuild -Dpi $Dpi -KeepWordOpen:$KeepWordOpen.IsPresent -VisibleWord:$VisibleWord.IsPresent -ReviewVerdict $ReviewVerdict -ReviewNote $ReviewNote
    if ($LASTEXITCODE -ne 0) { throw "Word visual smoke failed for right DOCX." }
}

Write-Step "Completed semantic diff visual regression"
Write-Host "Left DOCX: $leftDocxPath"
Write-Host "Right DOCX: $rightDocxPath"
Write-Host "Diff JSON: $diffJsonPath"
Write-Host "Left visual output: $leftSmokeOutputDir"
Write-Host "Right visual output: $rightSmokeOutputDir"
