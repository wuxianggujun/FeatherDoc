param(
    [string]$BuildDir = "build-generic-fields-visual",
    [string]$OutputDir = "output/generic-fields-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = "Confirm TOC, REF, PAGEREF, STYLEREF, DOCPROPERTY, DATE, HYPERLINK, SEQ, caption, XE, INDEX, and nested complex field placeholders are readable, and updateFields settings evidence is present after Word PDF export."
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Write-Step { param([string]$Message) Write-Host "[generic-fields-visual] $Message" }
function Resolve-RepoRoot { return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path }
function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$InputPath)
    if ([System.IO.Path]::IsPathRooted($InputPath)) { return [System.IO.Path]::GetFullPath($InputPath) }
    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}
function Find-BuildExecutable {
    param([string]$BuildRoot, [string]$TargetName)
    $candidates = @(Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } |
        Sort-Object LastWriteTime -Descending)
    if ($candidates.Count -eq 0) { throw "Could not find built executable for target '$TargetName' under $BuildRoot." }
    return $candidates[0].FullName
}
function Get-DocxEntryText {
    param([string]$DocxPath, [string]$EntryName)
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entry = $archive.GetEntry($EntryName)
        if ($entry -eq $null) { throw "DOCX entry is missing: $EntryName" }
        $stream = $entry.Open()
        try {
            $reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::UTF8)
            try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
        } finally { $stream.Dispose() }
    } finally { $archive.Dispose() }
}
function Set-DocxEntryText {
    param([string]$DocxPath, [string]$EntryName, [string]$Text)
    $archive = [System.IO.Compression.ZipFile]::Open($DocxPath, [System.IO.Compression.ZipArchiveMode]::Update)
    try {
        $entry = $archive.GetEntry($EntryName)
        if ($entry -eq $null) { throw "DOCX entry is missing: $EntryName" }
        $entry.Delete()
        $newEntry = $archive.CreateEntry($EntryName, [System.IO.Compression.CompressionLevel]::Optimal)
        $stream = $newEntry.Open()
        try {
            $writer = New-Object System.IO.StreamWriter($stream, (New-Object System.Text.UTF8Encoding($false)))
            try { $writer.Write($Text) } finally { $writer.Dispose() }
        } finally { $stream.Dispose() }
    } finally { $archive.Dispose() }
}
function New-SemanticDiffRightDocx {
    param([string]$LeftDocxPath, [string]$RightDocxPath)
    Copy-Item -LiteralPath $LeftDocxPath -Destination $RightDocxPath -Force
    $documentXml = Get-DocxEntryText -DocxPath $RightDocxPath -EntryName "word/document.xml"
    $replacements = [ordered]@{
        'TOC \o &quot;1-2&quot; \h \z \u' = 'TOC \o &quot;1-3&quot; \h \z \u'
        'Referenced target heading' = 'Reviewed target heading'
        'SEQ Figure \* ARABIC \r 1 \* MERGEFORMAT' = 'SEQ Figure \* ARABIC \r 2 \* MERGEFORMAT'
        'Figure 1' = 'Figure 10'
    }
    foreach ($key in $replacements.Keys) {
        if (-not $documentXml.Contains($key)) { throw "Right DOCX mutation source text missing: $key" }
        $documentXml = $documentXml.Replace($key, $replacements[$key])
    }
    Set-DocxEntryText -DocxPath $RightDocxPath -EntryName "word/document.xml" -Text $documentXml
}
function Assert-SemanticDiffFieldEvidence {
    param([string]$JsonPath)
    if (-not (Test-Path $JsonPath)) { throw "Semantic diff JSON was not created: $JsonPath" }
    $json = Get-Content -LiteralPath $JsonPath -Raw -Encoding UTF8
    $requiredText = @(
        '"command":"semantic-diff"',
        '"different":true',
        '"fields":{"left_count":',
        '"field_changes"',
        '"field":"field"',
        '"field_path":"instruction"',
        '"field_path":"result_text"',
        'kind=table_of_contents',
        'kind=reference',
        'kind=sequence',
        'Reviewed target heading',
        'Figure 10',
        '"template_part_results"',
        '"part":"body"'
    )
    foreach ($text in $requiredText) {
        if (-not $json.Contains($text)) { throw "Semantic diff JSON is missing expected field evidence: $text" }
    }
}
function Assert-GeneratedDocxEvidence {
    param([string]$DocxPath)
    if (-not (Test-Path $DocxPath)) { throw "Generated DOCX was not created: $DocxPath" }
    $documentXml = Get-DocxEntryText -DocxPath $DocxPath -EntryName "word/document.xml"
    $settingsXml = Get-DocxEntryText -DocxPath $DocxPath -EntryName "word/settings.xml"
    $requiredSettingsText = @(
        'w:updateFields',
        'w:val="1"'
    )
    foreach ($text in $requiredSettingsText) {
        if (-not $settingsXml.Contains($text)) { throw "Generated DOCX is missing expected settings evidence: $text" }
    }

    $requiredText = @(
        "TOC \o &quot;1-2&quot; \h \z \u",
        "REF target_heading \h \* MERGEFORMAT",
        "PAGEREF target_heading \h \p \* MERGEFORMAT",
        "STYLEREF &quot;Heading 1&quot; \n \p \* MERGEFORMAT",
        "DOCPROPERTY Title \* MERGEFORMAT",
        "DATE \@ &quot;yyyy-MM-dd&quot; \* MERGEFORMAT",
        "HYPERLINK &quot;https://example.com/report&quot; \l &quot;target_heading&quot; \o &quot;Open target heading&quot; \* MERGEFORMAT",
        'w:dirty="true"',
        'w:fldLock="true"',
        "SEQ Figure \* ARABIC \r 1 \* MERGEFORMAT",
        "SEQ Figure \* ARABIC \* MERGEFORMAT",
        "SEQ Figure \* ARABIC \r 3 \* MERGEFORMAT",
        "XE &quot;FeatherDoc:Fields&quot; \r target_heading \t &quot;See typed fields&quot; \b",
        "INDEX \c &quot;2&quot; \* MERGEFORMAT",
        "TOC field placeholder - update fields in Word",
        "Referenced target heading",
        "Page reference placeholder",
        "Style reference placeholder",
        "Document title placeholder",
        "2026-05-01",
        "Open report",
        "Figure 1",
        "Figure 2",
        "Generated caption placeholder",
        "Index field placeholder",
        'w:fldCharType="begin"',
        'w:fldCharType="separate"',
        'w:fldCharType="end"',
        "MERGEFIELD CustomerName",
        "Nested field matched"
    )
    foreach ($text in $requiredText) {
        if (-not $documentXml.Contains($text)) { throw "Generated DOCX is missing expected field evidence: $text" }
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$targetName = "featherdoc_sample_generic_fields_visual"
$docxPath = Join-Path $resolvedOutputDir "generic_fields_visual.docx"
$rightDocxPath = Join-Path $resolvedOutputDir "generic_fields_visual_right.docx"
$semanticDiffJsonPath = Join-Path $resolvedOutputDir "generic_fields_semantic_diff.json"
$wordSmokeOutputDir = Join-Path $resolvedOutputDir "word-smoke"
$rightWordSmokeOutputDir = Join-Path $resolvedOutputDir "word-smoke-right"
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

if (-not $SkipBuild) {
    Write-Step "Configuring sample build directory $resolvedBuildDir"
    & cmake -S $repoRoot -B $resolvedBuildDir -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
    Write-Step "Building $targetName and featherdoc_cli"
    & cmake --build $resolvedBuildDir --target $targetName featherdoc_cli --config Debug
    if ($LASTEXITCODE -ne 0) { throw "Sample build failed." }
}

$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName $targetName
$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
Write-Step "Generating DOCX via $sampleExecutable"
& $sampleExecutable $docxPath
if ($LASTEXITCODE -ne 0) { throw "Generic fields sample failed." }

Write-Step "Checking generated DOCX XML evidence"
Assert-GeneratedDocxEvidence -DocxPath $docxPath

Write-Step "Generating semantic diff field mutation DOCX"
New-SemanticDiffRightDocx -LeftDocxPath $docxPath -RightDocxPath $rightDocxPath

Write-Step "Running semantic-diff field JSON check"
& $cliExecutable semantic-diff $docxPath $rightDocxPath --json --no-paragraphs --no-tables --no-images --no-content-controls --no-sections | Set-Content -Encoding UTF8 -Path $semanticDiffJsonPath
if ($LASTEXITCODE -ne 0) { throw "semantic-diff CLI failed for generic fields." }
Assert-SemanticDiffFieldEvidence -JsonPath $semanticDiffJsonPath

if (-not $SkipVisual) {
    Write-Step "Running Word visual smoke for generated DOCX"
    & $wordSmokeScript `
        -InputDocx $docxPath `
        -OutputDir $wordSmokeOutputDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen.IsPresent `
        -VisibleWord:$VisibleWord.IsPresent `
        -ReviewVerdict $ReviewVerdict `
        -ReviewNote $ReviewNote
    if ($LASTEXITCODE -ne 0) { throw "Word visual smoke failed for generated DOCX." }

    Write-Step "Running Word visual smoke for semantic diff right DOCX"
    & $wordSmokeScript `
        -InputDocx $rightDocxPath `
        -OutputDir $rightWordSmokeOutputDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen.IsPresent `
        -VisibleWord:$VisibleWord.IsPresent `
        -ReviewVerdict $ReviewVerdict `
        -ReviewNote "Confirm the semantic-diff right-side TOC, REF, and SEQ field mutations remain readable after Word PDF export."
    if ($LASTEXITCODE -ne 0) { throw "Word visual smoke failed for semantic diff right DOCX." }
}

Write-Step "Completed generic field visual regression"
Write-Host "DOCX: $docxPath"
Write-Host "Right DOCX: $rightDocxPath"
Write-Host "Semantic diff JSON: $semanticDiffJsonPath"
Write-Host "Visual output: $wordSmokeOutputDir"
Write-Host "Right visual output: $rightWordSmokeOutputDir"
