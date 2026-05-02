param(
    [string]$BuildDir = "build-extended-image-formats-visual",
    [string]$OutputDir = "output/extended-image-formats-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = "Confirm SVG, WebP, and TIFF images appear as three readable colored image blocks after Word PDF export."
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Write-Step { param([string]$Message) Write-Host "[extended-image-formats-visual] $Message" }
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
    $contentTypes = Get-DocxEntryText -DocxPath $DocxPath -EntryName "[Content_Types].xml"
    $requiredText = @(
        'ContentType="image/svg+xml"',
        'ContentType="image/webp"',
        'ContentType="image/tiff"',
        'Extension="svg"',
        'Extension="webp"',
        'Extension="tiff"'
    )
    foreach ($text in $requiredText) {
        if (-not $contentTypes.Contains($text)) { throw "Generated DOCX is missing expected content-type evidence: $text" }
    }
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
svg = "<svg xmlns='http://www.w3.org/2000/svg' width='240' height='120'><rect width='240' height='120' fill='#1E88E5'/><text x='32' y='68' font-size='28' fill='white'>SVG image</text></svg>"
(out / "visual.svg").write_text(svg, encoding="utf-8")
for name, color, label in [("visual.webp", (67, 160, 71), "WebP image"), ("visual.tiff", (142, 36, 170), "TIFF image")]:
    image = Image.new("RGB", (240, 120), color)
    draw = ImageDraw.Draw(image)
    draw.rectangle((8, 8, 232, 112), outline=(255, 255, 255), width=4)
    draw.text((42, 52), label, fill=(255, 255, 255))
    image.save(out / name)
"@
    $assetScript = Join-Path $OutputDir "create_assets.py"
    Set-Content -LiteralPath $assetScript -Encoding UTF8 -Value $python
    & python $assetScript $OutputDir
    if ($LASTEXITCODE -ne 0) { throw "Failed to generate SVG/WebP/TIFF image assets." }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$assetDir = Join-Path $resolvedOutputDir "assets"
$targetName = "featherdoc_sample_extended_image_formats_visual"
$docxPath = Join-Path $resolvedOutputDir "extended_image_formats_visual.docx"
$wordSmokeOutputDir = Join-Path $resolvedOutputDir "word-smoke"
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

Write-Step "Generating SVG/WebP/TIFF assets"
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
& $sampleExecutable `
    $docxPath `
    (Join-Path $assetDir "visual.svg") `
    (Join-Path $assetDir "visual.webp") `
    (Join-Path $assetDir "visual.tiff")
if ($LASTEXITCODE -ne 0) { throw "Extended image formats sample failed." }

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

Write-Step "Completed extended image formats visual regression"
Write-Host "DOCX: $docxPath"
Write-Host "Visual output: $wordSmokeOutputDir"
