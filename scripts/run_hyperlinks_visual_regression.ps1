param(
    [string]$BuildDir = "build-hyperlinks-visual",
    [string]$OutputDir = "output/hyperlinks-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = "Confirm both external hyperlink display texts are visible after Word PDF export and raw URLs are not printed in the page body."
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Write-Step { param([string]$Message) Write-Host "[hyperlinks-visual] $Message" }
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
function Assert-GeneratedDocxEvidence {
    param([string]$DocxPath)
    if (-not (Test-Path $DocxPath)) { throw "Generated DOCX was not created: $DocxPath" }

    $documentXml = Get-DocxEntryText -DocxPath $DocxPath -EntryName "word/document.xml"
    $relationshipsXml = Get-DocxEntryText -DocxPath $DocxPath -EntryName "word/_rels/document.xml.rels"

    $requiredText = @(
        "External hyperlink typed API visual regression.",
        "OpenAI external hyperlink",
        "FeatherDoc documentation hyperlink"
    )
    foreach ($text in $requiredText) {
        if (-not $documentXml.Contains($text)) { throw "Generated DOCX is missing expected text: $text" }
    }

    if (([regex]::Matches($documentXml, "<w:hyperlink")).Count -ne 2) {
        throw "Generated DOCX should contain exactly two w:hyperlink nodes."
    }
    if ($documentXml.Contains("https://openai.com") -or $documentXml.Contains("https://example.com/featherdoc/docs")) {
        throw "Generated DOCX body should not print raw hyperlink targets."
    }
    if (([regex]::Matches($relationshipsXml, "/relationships/hyperlink")).Count -ne 2) {
        throw "Generated DOCX should contain exactly two hyperlink relationships."
    }
    foreach ($text in @('Target="https://openai.com"', 'Target="https://example.com/featherdoc/docs"', 'TargetMode="External"')) {
        if (-not $relationshipsXml.Contains($text)) { throw "Generated DOCX is missing relationship evidence: $text" }
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$targetName = "featherdoc_sample_hyperlinks_visual"
$docxPath = Join-Path $resolvedOutputDir "hyperlinks_visual.docx"
$wordSmokeOutputDir = Join-Path $resolvedOutputDir "word-smoke"
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

if (-not $SkipBuild) {
    Write-Step "Configuring sample build directory $resolvedBuildDir"
    & cmake -S $repoRoot -B $resolvedBuildDir -DBUILD_SAMPLES=ON -DBUILD_CLI=OFF -DBUILD_TESTING=OFF
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
    Write-Step "Building $targetName"
    & cmake --build $resolvedBuildDir --target $targetName --config Debug
    if ($LASTEXITCODE -ne 0) { throw "Sample build failed." }
}

$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName $targetName
Write-Step "Generating DOCX via $sampleExecutable"
& $sampleExecutable $docxPath
if ($LASTEXITCODE -ne 0) { throw "Hyperlinks visual sample failed." }

Write-Step "Checking generated DOCX package evidence"
Assert-GeneratedDocxEvidence -DocxPath $docxPath

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
}

Write-Step "Completed hyperlinks visual regression"
Write-Host "DOCX: $docxPath"
Write-Host "Visual output: $wordSmokeOutputDir"
