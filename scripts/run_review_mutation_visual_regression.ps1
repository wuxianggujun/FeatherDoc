param(
    [string]$BuildDir = "build-review-mutation-visual",
    [string]$OutputDir = "output/review-mutation-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisual,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord,
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "pending_manual_review",
    [string]$ReviewNote = "Confirm typed review mutation APIs render run, paragraph range, cross-paragraph range revisions, footnote, endnote, comment marker, hyperlink, and OMML formulas."
)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Write-Step { param([string]$Message) Write-Host "[review-mutation-visual] $Message" }
function Resolve-RepoRoot { return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path }
function Resolve-RepoPath { param([string]$RepoRoot,[string]$InputPath) if ([IO.Path]::IsPathRooted($InputPath)) { return [IO.Path]::GetFullPath($InputPath) } return [IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath)) }
function Find-BuildExecutable { param([string]$BuildRoot,[string]$TargetName) $c=@(Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } | Sort-Object LastWriteTime -Descending); if ($c.Count -eq 0) { throw "Could not find built executable for target '$TargetName' under $BuildRoot." } return $c[0].FullName }
function Get-DocxEntryText { param([string]$DocxPath,[string]$EntryName) $a=[IO.Compression.ZipFile]::OpenRead($DocxPath); try { $e=$a.GetEntry($EntryName); if ($null -eq $e) { throw "Missing $EntryName" }; $s=$e.Open(); try { $r=[IO.StreamReader]::new($s,[Text.Encoding]::UTF8); try { return $r.ReadToEnd() } finally { $r.Dispose() } } finally { $s.Dispose() } } finally { $a.Dispose() } }
function Assert-GeneratedDocxEvidence { param([string]$DocxPath)
    if (-not (Test-Path $DocxPath)) { throw "Generated DOCX was not created: $DocxPath" }
    $documentXml=Get-DocxEntryText $DocxPath "word/document.xml"
    $footnotesXml=Get-DocxEntryText $DocxPath "word/footnotes.xml"
    $endnotesXml=Get-DocxEntryText $DocxPath "word/endnotes.xml"
    $commentsXml=Get-DocxEntryText $DocxPath "word/comments.xml"
    $relationshipsXml=Get-DocxEntryText $DocxPath "word/_rels/document.xml.rels"
    $contentTypesXml=Get-DocxEntryText $DocxPath "[Content_Types].xml"
    foreach ($text in @("Accepted insertion from revision API","Rejected deletion kept by API","Authored insertion revision accepted by typed API","Authored deletion revision rejected by typed API","Run revision original visible text","accepted in-place replacement revision","accepted paragraph range replacement","accepted cross paragraph range replacement","In-place ","commented","Typed API evidence details below","footnoteReference","endnoteReference","commentReference","Typed hyperlink","Removed hyperlink text","m:f","m:rad")) { if (-not $documentXml.Contains($text)) { throw "Missing document review mutation evidence: $text" } }
    foreach ($text in @('<w:commentRangeStart w:id="1"','<w:t>range</w:t>','<w:commentRangeEnd w:id="1"','<w:commentReference w:id="1"')) { if (-not $documentXml.Contains($text)) { throw "Missing retargeted comment range evidence: $text" } }
    foreach ($text in @("<w:ins","<w:del","w:delText")) { if ($documentXml.Contains($text)) { throw "Revision markup should have been resolved but found: $text" } }
    if (-not $footnotesXml.Contains("Replaced visual footnote body")) { throw "Missing replaced footnote body evidence" }
    if ($footnotesXml.Contains("Removed visual footnote body")) { throw "Removed footnote body still exists" }
    if (-not $endnotesXml.Contains("Replaced visual endnote body")) { throw "Missing replaced endnote body evidence" }
    if ($endnotesXml.Contains("Removed visual endnote body")) { throw "Removed endnote body still exists" }
    if (-not $commentsXml.Contains("Replaced visual comment body")) { throw "Missing replaced comment body evidence" }
    if (-not $commentsXml.Contains("In-place range comment body")) { throw "Missing in-place comment range evidence" }
    if ($commentsXml.Contains("Removed visual comment body")) { throw "Removed comment body still exists" }
    foreach ($text in @("/relationships/footnotes","/relationships/endnotes","/relationships/comments","/relationships/hyperlink")) { if (-not $relationshipsXml.Contains($text)) { throw "Missing relationship evidence: $text" } }
    foreach ($text in @("/word/footnotes.xml","/word/endnotes.xml","/word/comments.xml")) { if (-not $contentTypesXml.Contains($text)) { throw "Missing content type override: $text" } }
}
$repoRoot=Resolve-RepoRoot
$resolvedBuildDir=Resolve-RepoPath $repoRoot $BuildDir
$resolvedOutputDir=Resolve-RepoPath $repoRoot $OutputDir
$targetName="featherdoc_sample_review_mutation_visual"
$docxPath=Join-Path $resolvedOutputDir "review_mutation_visual.docx"
$wordSmokeOutputDir=Join-Path $resolvedOutputDir "word-smoke"
$wordSmokeScript=Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
if (-not $SkipBuild) { Write-Step "Configuring sample build directory $resolvedBuildDir"; & cmake -S $repoRoot -B $resolvedBuildDir -DBUILD_SAMPLES=ON -DBUILD_CLI=OFF -DBUILD_TESTING=OFF; if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }; Write-Step "Building $targetName"; & cmake --build $resolvedBuildDir --target $targetName --config Debug; if ($LASTEXITCODE -ne 0) { throw "Sample build failed." } }
$sampleExecutable=Find-BuildExecutable $resolvedBuildDir $targetName
Write-Step "Generating DOCX via $sampleExecutable"
& $sampleExecutable $docxPath
if ($LASTEXITCODE -ne 0) { throw "Review mutation sample failed." }
Write-Step "Checking generated DOCX XML evidence"
Assert-GeneratedDocxEvidence $docxPath
if (-not $SkipVisual) { Write-Step "Running Word visual smoke for generated DOCX"; & $wordSmokeScript -InputDocx $docxPath -OutputDir $wordSmokeOutputDir -SkipBuild -Dpi $Dpi -KeepWordOpen:$KeepWordOpen.IsPresent -VisibleWord:$VisibleWord.IsPresent -ReviewVerdict $ReviewVerdict -ReviewNote $ReviewNote; if ($LASTEXITCODE -ne 0) { throw "Word visual smoke failed for generated DOCX." } }
Write-Step "Completed review mutation visual regression"
Write-Host "DOCX: $docxPath"
Write-Host "Visual output: $wordSmokeOutputDir"
