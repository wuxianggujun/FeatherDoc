$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$vcvarsPath = Get-VcvarsPath

if (-not $SkipBuild) {
    Write-Step "Configuring build directory $resolvedBuildDir"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building featherdoc_cli and ensure-style visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_ensure_style_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_ensure_style_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$paragraphTargetText = "Paragraph style target: this paragraph already uses ReviewPara, so ensure-paragraph-style should rewrite the style definition and restyle this whole line without rebinding the paragraph."
$inheritedParagraphTargetText = "Inherited style target: this paragraph uses ReviewParaChild, so rewriting ReviewPara should flow through the basedOn chain and restyle this line without rebinding the child paragraph."
$runTargetText = "ALPHA 123 hello world"
$inheritedRunTargetText = "OMEGA 456 inherited marker"
$reviewTableStyleId = "ReviewTable"
$reviewTableStyleName = "Review Table"

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared ensure-style baseline sample."

$baselineParagraphJson = Join-Path $resolvedOutputDir "baseline-review-para-paragraph.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-paragraphs", $baselineDocxPath, "--paragraph", "1", "--json") `
    -OutputPath $baselineParagraphJson `
    -FailureMessage "Failed to inspect baseline ReviewPara paragraph."
Assert-ParagraphState `
    -JsonPath $baselineParagraphJson `
    -ExpectedIndex 1 `
    -ExpectedText $paragraphTargetText `
    -ExpectedStyleId "ReviewPara" `
    -Label "baseline-review-para"

$baselineInheritedParagraphJson = Join-Path $resolvedOutputDir "baseline-review-para-child-paragraph.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-paragraphs", $baselineDocxPath, "--paragraph", "2", "--json") `
    -OutputPath $baselineInheritedParagraphJson `
    -FailureMessage "Failed to inspect baseline ReviewParaChild paragraph."
Assert-ParagraphState `
    -JsonPath $baselineInheritedParagraphJson `
    -ExpectedIndex 2 `
    -ExpectedText $inheritedParagraphTargetText `
    -ExpectedStyleId "ReviewParaChild" `
    -Label "baseline-review-para-child"

$baselineRunJson = Join-Path $resolvedOutputDir "baseline-accent-marker-run.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-runs", $baselineDocxPath, "3", "--run", "1", "--json") `
    -OutputPath $baselineRunJson `
    -FailureMessage "Failed to inspect baseline AccentMarker run."
Assert-RunState `
    -JsonPath $baselineRunJson `
    -ExpectedParagraphIndex 3 `
    -ExpectedRunIndex 1 `
    -ExpectedText $runTargetText `
    -ExpectedStyleId "AccentMarker" `
    -Label "baseline-accent-marker-run"

$baselineReviewTableJson = Join-Path $resolvedOutputDir "baseline-review-table.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-tables", $baselineDocxPath, "--table", "0", "--json") `
    -OutputPath $baselineReviewTableJson `
    -FailureMessage "Failed to inspect baseline ReviewTable target."
Assert-TableState `
    -JsonPath $baselineReviewTableJson `
    -ExpectedIndex 0 `
    -ExpectedStyleId $reviewTableStyleId `
    -ExpectedRowCount 3 `
    -ExpectedColumnCount 2 `
    -Label "baseline-review-table"

$baselineReviewParaStyleJson = Join-Path $resolvedOutputDir "baseline-review-para-style.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-styles", $baselineDocxPath, "--style", "ReviewPara", "--json") `
    -OutputPath $baselineReviewParaStyleJson `
    -FailureMessage "Failed to inspect baseline ReviewPara style."
Assert-StyleCatalogState `
    -JsonPath $baselineReviewParaStyleJson `
    -ExpectedStyleId "ReviewPara" `
    -ExpectedName "Review Paragraph" `
    -ExpectedBasedOn "Normal" `
    -ExpectedKind "paragraph" `
    -ExpectedType "paragraph" `
    -ExpectedCustom $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedQuickFormat $true `
    -Label "baseline-review-para-style"

$baselineReviewParaChildStyleJson = Join-Path $resolvedOutputDir "baseline-review-para-child-style.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-styles", $baselineDocxPath, "--style", "ReviewParaChild", "--json") `
    -OutputPath $baselineReviewParaChildStyleJson `
    -FailureMessage "Failed to inspect baseline ReviewParaChild style."
Assert-StyleCatalogState `
    -JsonPath $baselineReviewParaChildStyleJson `
    -ExpectedStyleId "ReviewParaChild" `
    -ExpectedName "Review Paragraph Child" `
    -ExpectedBasedOn "ReviewPara" `
    -ExpectedKind "paragraph" `
    -ExpectedType "paragraph" `
    -ExpectedCustom $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedQuickFormat $true `
    -Label "baseline-review-para-child-style"

$baselineReviewParaChildInheritanceJson = Join-Path $resolvedOutputDir "baseline-review-para-child-inheritance.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-style-inheritance", $baselineDocxPath, "ReviewParaChild", "--json") `
    -OutputPath $baselineReviewParaChildInheritanceJson `
    -FailureMessage "Failed to inspect baseline ReviewParaChild inheritance."
Assert-StyleInheritanceState `
    -JsonPath $baselineReviewParaChildInheritanceJson `
    -ExpectedStyleId "ReviewParaChild" `
    -ExpectedType "paragraph" `
    -ExpectedKind "paragraph" `
    -ExpectedBasedOn "ReviewPara" `
    -ExpectedInheritanceChain @("ReviewParaChild", "ReviewPara", "Normal") `
    -ExpectedFontFamilyValue "Courier New" `
    -ExpectedFontFamilySource "ReviewPara" `
    -ExpectedEastAsiaFontFamilyValue $null `
    -ExpectedEastAsiaFontFamilySource $null `
    -ExpectedLanguageValue $null `
    -ExpectedLanguageSource $null `
    -ExpectedEastAsiaLanguageValue $null `
    -ExpectedEastAsiaLanguageSource $null `
    -ExpectedBidiLanguageValue $null `
    -ExpectedBidiLanguageSource $null `
    -ExpectedRtlValue $null `
    -ExpectedRtlSource $null `
    -ExpectedParagraphBidiValue $null `
    -ExpectedParagraphBidiSource $null `
    -Label "baseline-review-para-child-inheritance"

$baselineAccentMarkerStyleJson = Join-Path $resolvedOutputDir "baseline-accent-marker-style.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-styles", $baselineDocxPath, "--style", "AccentMarker", "--json") `
    -OutputPath $baselineAccentMarkerStyleJson `
    -FailureMessage "Failed to inspect baseline AccentMarker style."
Assert-StyleCatalogState `
    -JsonPath $baselineAccentMarkerStyleJson `
    -ExpectedStyleId "AccentMarker" `
    -ExpectedName "Accent Marker" `
    -ExpectedBasedOn "DefaultParagraphFont" `
    -ExpectedKind "character" `
    -ExpectedType "character" `
    -ExpectedCustom $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedQuickFormat $true `
    -Label "baseline-accent-marker-style"

$baselineReviewTableStyleJson = Join-Path $resolvedOutputDir "baseline-review-table-style.json"
Invoke-Capture `
    -Executable $cliExecutable `
    -Arguments @("inspect-styles", $baselineDocxPath, "--style", $reviewTableStyleId, "--json") `
    -OutputPath $baselineReviewTableStyleJson `
    -FailureMessage "Failed to inspect baseline ReviewTable style."
Assert-StyleCatalogState `
    -JsonPath $baselineReviewTableStyleJson `
    -ExpectedStyleId $reviewTableStyleId `
    -ExpectedName $reviewTableStyleName `
    -ExpectedBasedOn "TableGrid" `
    -ExpectedKind "table" `
    -ExpectedType "table" `
    -ExpectedCustom $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedQuickFormat $true `
    -Label "baseline-review-table-style"

$baselineReviewParaXmlState = Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId "ReviewPara"
Assert-StyleXmlState `
    -State $baselineReviewParaXmlState `
    -ExpectedType "paragraph" `
    -ExpectedName "Review Paragraph" `
    -ExpectedBasedOn "Normal" `
    -ExpectedNextStyle "ReviewPara" `
    -ExpectedQFormat $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedParagraphBidi $false `
    -ExpectedOutlineLevel $null `
    -ExpectedFontAscii "Courier New" `
    -ExpectedLangValue $null `
    -ExpectedLangBidi $null `
    -ExpectedRtlValue $null `
    -Label "baseline-review-para-xml"

$baselineReviewParaChildXmlState = Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId "ReviewParaChild"
Assert-StyleXmlState `
    -State $baselineReviewParaChildXmlState `
    -ExpectedType "paragraph" `
    -ExpectedName "Review Paragraph Child" `
    -ExpectedBasedOn "ReviewPara" `
    -ExpectedNextStyle "ReviewParaChild" `
    -ExpectedQFormat $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedParagraphBidi $false `
    -ExpectedOutlineLevel $null `
    -ExpectedFontAscii $null `
    -ExpectedLangValue $null `
    -ExpectedLangBidi $null `
    -ExpectedRtlValue $null `
    -Label "baseline-review-para-child-xml"

$baselineAccentMarkerXmlState = Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId "AccentMarker"
Assert-StyleXmlState `
    -State $baselineAccentMarkerXmlState `
    -ExpectedType "character" `
    -ExpectedName "Accent Marker" `
    -ExpectedBasedOn "DefaultParagraphFont" `
    -ExpectedNextStyle $null `
    -ExpectedQFormat $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedParagraphBidi $false `
    -ExpectedOutlineLevel $null `
    -ExpectedFontAscii "Courier New" `
    -ExpectedLangValue $null `
    -ExpectedLangBidi $null `
    -ExpectedRtlValue "0" `
    -Label "baseline-accent-marker-xml"

$baselineReviewTableXmlState = Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId $reviewTableStyleId
Assert-StyleXmlState `
    -State $baselineReviewTableXmlState `
    -ExpectedType "table" `
    -ExpectedName $reviewTableStyleName `
    -ExpectedBasedOn "TableGrid" `
    -ExpectedNextStyle $null `
    -ExpectedQFormat $true `
    -ExpectedSemiHidden $false `
    -ExpectedUnhideWhenUsed $false `
    -ExpectedParagraphBidi $false `
    -ExpectedOutlineLevel $null `
    -ExpectedFontAscii $null `
    -ExpectedLangValue $null `
    -ExpectedLangBidi $null `
    -ExpectedRtlValue $null `
    -Label "baseline-review-table-xml"

Write-Step "Rendering Word evidence for the shared baseline document"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $baselineDocxPath `
    -OutputDir $baselineVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $baselineVisualDir -ExpectedCount 1 -Label "shared-baseline-visual"
