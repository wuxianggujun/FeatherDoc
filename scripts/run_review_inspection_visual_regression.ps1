param(
    [string]$BuildDir = "build-review-inspection-visual",
    [string]$OutputDir = "output/review-inspection-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = "Confirm review fixture renders footnote/endnote markers and revision text while API XML evidence is present."
)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Write-Step { param([string]$Message) Write-Host "[review-inspection-visual] $Message" }
function Resolve-RepoRoot { return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path }
function Resolve-RepoPath { param([string]$RepoRoot,[string]$InputPath) if ([IO.Path]::IsPathRooted($InputPath)) { return [IO.Path]::GetFullPath($InputPath) } return [IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath)) }
function Find-BuildExecutable { param([string]$BuildRoot,[string]$TargetName) $c=@(Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } | Sort-Object LastWriteTime -Descending); if ($c.Count -eq 0) { throw "Could not find built executable for target '$TargetName' under $BuildRoot." } return $c[0].FullName }
function Get-DocxEntryText { param([string]$DocxPath,[string]$EntryName) $a=[IO.Compression.ZipFile]::OpenRead($DocxPath); try { $e=$a.GetEntry($EntryName); if ($null -eq $e) { throw "Missing $EntryName" }; $s=$e.Open(); try { $r=[IO.StreamReader]::new($s,[Text.Encoding]::UTF8); try { return $r.ReadToEnd() } finally { $r.Dispose() } } finally { $s.Dispose() } } finally { $a.Dispose() } }
function Assert-GeneratedDocxEvidence { param([string]$DocxPath) if (-not (Test-Path $DocxPath)) { throw "Generated DOCX was not created: $DocxPath" }; $documentXml=Get-DocxEntryText $DocxPath "word/document.xml"; $footnotesXml=Get-DocxEntryText $DocxPath "word/footnotes.xml"; $endnotesXml=Get-DocxEntryText $DocxPath "word/endnotes.xml"; $commentsXml=Get-DocxEntryText $DocxPath "word/comments.xml"; foreach ($text in @("Inserted revision text","Deleted revision text","commentReference","footnoteReference","endnoteReference")) { if (-not $documentXml.Contains($text)) { throw "Missing document review evidence: $text" } }; if (-not $footnotesXml.Contains("Visual footnote body text")) { throw "Missing footnote body evidence" }; if (-not $endnotesXml.Contains("Visual endnote body text")) { throw "Missing endnote body evidence" }; if (-not $commentsXml.Contains("Visual comment body text")) { throw "Missing comment body evidence" } }
$repoRoot=Resolve-RepoRoot
$resolvedBuildDir=Resolve-RepoPath $repoRoot $BuildDir
$resolvedOutputDir=Resolve-RepoPath $repoRoot $OutputDir
$targetName="featherdoc_sample_review_inspection_visual"
$docxPath=Join-Path $resolvedOutputDir "review_inspection_visual.docx"
$wordSmokeOutputDir=Join-Path $resolvedOutputDir "word-smoke"
$wordSmokeScript=Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
if (-not $SkipBuild) { Write-Step "Configuring sample build directory $resolvedBuildDir"; & cmake -S $repoRoot -B $resolvedBuildDir -DBUILD_SAMPLES=ON -DBUILD_CLI=OFF -DBUILD_TESTING=OFF; if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }; Write-Step "Building $targetName"; & cmake --build $resolvedBuildDir --target $targetName --config Debug; if ($LASTEXITCODE -ne 0) { throw "Sample build failed." } }
$sampleExecutable=Find-BuildExecutable $resolvedBuildDir $targetName
Write-Step "Generating DOCX via $sampleExecutable"
& $sampleExecutable $docxPath
if ($LASTEXITCODE -ne 0) { throw "Review inspection sample failed." }
Write-Step "Checking generated DOCX XML evidence"
Assert-GeneratedDocxEvidence $docxPath
if (-not $SkipVisual) { Write-Step "Running Word visual smoke for generated DOCX"; & $wordSmokeScript -InputDocx $docxPath -OutputDir $wordSmokeOutputDir -SkipBuild -Dpi $Dpi -KeepWordOpen:$KeepWordOpen.IsPresent -VisibleWord:$VisibleWord.IsPresent -ReviewVerdict $ReviewVerdict -ReviewNote $ReviewNote; if ($LASTEXITCODE -ne 0) { throw "Word visual smoke failed for generated DOCX." } }
Write-Step "Completed review inspection visual regression"
Write-Host "DOCX: $docxPath"
Write-Host "Visual output: $wordSmokeOutputDir"
