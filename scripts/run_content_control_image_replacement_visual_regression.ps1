param(
    [string]$BuildDir = "build-content-control-image-visual",
    [string]$OutputDir = "output/content-control-image-replacement-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = "Confirm the block image, inline badge image, and table-cell image are visible after Word PDF export; placeholder text must be absent."
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Write-Step { param([string]$Message) Write-Host "[content-control-image-visual] $Message" }
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
function Assert-DocxEntryExists {
    param([string]$DocxPath, [string]$EntryName)
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        if ($archive.GetEntry($EntryName) -eq $null) { throw "DOCX entry is missing: $EntryName" }
    } finally { $archive.Dispose() }
}
function Assert-GeneratedDocxEvidence {
    param([string]$DocxPath)
    if (-not (Test-Path $DocxPath)) { throw "Generated DOCX was not created: $DocxPath" }

    $documentXml = Get-DocxEntryText -DocxPath $DocxPath -EntryName "word/document.xml"
    $relationshipsXml = Get-DocxEntryText -DocxPath $DocxPath -EntryName "word/_rels/document.xml.rels"
    $contentTypes = Get-DocxEntryText -DocxPath $DocxPath -EntryName "[Content_Types].xml"

    $requiredText = @(
        "Content Control image replacement visual regression.",
        "Block image content control below:",
        "Inline badge replacement:",
        "should stay on this line.",
        "Table-cell image content control below:"
    )
    foreach ($text in $requiredText) {
        if (-not $documentXml.Contains($text)) { throw "Generated DOCX is missing expected text: $text" }
    }

    $forbiddenText = @(
        "Hero image placeholder should disappear.",
        "Inline badge placeholder",
        "Cell image placeholder should disappear.",
        "w:showingPlcHdr"
    )
    foreach ($text in $forbiddenText) {
        if ($documentXml.Contains($text)) { throw "Generated DOCX still contains placeholder evidence: $text" }
    }

    if (([regex]::Matches($documentXml, "<wp:inline")).Count -ne 3) {
        throw "Generated DOCX should contain exactly three inline drawing images."
    }
    if (-not $relationshipsXml.Contains("/relationships/image")) {
        throw "Generated DOCX is missing image relationships."
    }
    if (-not $contentTypes.Contains('Extension="png"') -or -not $contentTypes.Contains('ContentType="image/png"')) {
        throw "Generated DOCX is missing PNG content type evidence."
    }

    Assert-DocxEntryExists -DocxPath $DocxPath -EntryName "word/media/image1.png"
    Assert-DocxEntryExists -DocxPath $DocxPath -EntryName "word/media/image2.png"
    Assert-DocxEntryExists -DocxPath $DocxPath -EntryName "word/media/image3.png"
}
function New-ImageAssets {
    param([string]$OutputDir)
    New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
    $python = @"
import sys
from pathlib import Path
from PIL import Image, ImageDraw
out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
image = Image.new("RGB", (240, 120), (21, 101, 192))
draw = ImageDraw.Draw(image)
draw.rectangle((8, 8, 232, 112), outline=(255, 255, 255), width=4)
draw.rectangle((18, 18, 222, 102), outline=(255, 193, 7), width=3)
draw.text((58, 52), "CC image", fill=(255, 255, 255))
image.save(out / "content_control_image.png")
"@
    $assetScript = Join-Path $OutputDir "create_assets.py"
    Set-Content -LiteralPath $assetScript -Encoding UTF8 -Value $python
    & python $assetScript $OutputDir
    if ($LASTEXITCODE -ne 0) { throw "Failed to generate PNG image asset." }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$assetDir = Join-Path $resolvedOutputDir "assets"
$targetName = "featherdoc_sample_content_control_image_replacement_visual"
$docxPath = Join-Path $resolvedOutputDir "content_control_image_replacement_visual.docx"
$wordSmokeOutputDir = Join-Path $resolvedOutputDir "word-smoke"
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

Write-Step "Generating PNG image asset"
New-ImageAssets -OutputDir $assetDir

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
& $sampleExecutable $docxPath (Join-Path $assetDir "content_control_image.png")
if ($LASTEXITCODE -ne 0) { throw "Content Control image replacement sample failed." }

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

Write-Step "Completed Content Control image replacement visual regression"
Write-Host "DOCX: $docxPath"
Write-Host "Visual output: $wordSmokeOutputDir"
