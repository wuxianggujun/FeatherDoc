param(
    [string]$BuildDir = "build-omml-visual",
    [string]$OutputDir = "output/omml-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = "Confirm the inserted display OMML and replaced inline OMML are visible and readable after Word PDF export."
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Write-Step { param([string]$Message) Write-Host "[omml-visual] $Message" }
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
    $requiredText = @(
        'xmlns:m="http://schemas.openxmlformats.org/officeDocument/2006/math"',
        'm:oMathPara',
        'm:oMath',
        'sum(i=1..n) i = n(n+1)/2',
        'E=mc^2',
        'm:nary',
        'm:chr m:val=',
        'm:d',
        'x+1'
    )
    foreach ($text in $requiredText) {
        if (-not $documentXml.Contains($text)) { throw "Generated DOCX is missing expected OMML evidence: $text" }
    }
    if ($documentXml.Contains("placeholder") -or $documentXml.Contains("temporary equation removed")) {
        throw "Generated DOCX still contains OMML placeholder or temporary equation text."
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$targetName = "featherdoc_sample_omml_visual"
$docxPath = Join-Path $resolvedOutputDir "omml_visual.docx"
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
if ($LASTEXITCODE -ne 0) { throw "OMML visual sample failed." }

Write-Step "Checking generated DOCX XML evidence"
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

Write-Step "Completed OMML visual regression"
Write-Host "DOCX: $docxPath"
Write-Host "Visual output: $wordSmokeOutputDir"
