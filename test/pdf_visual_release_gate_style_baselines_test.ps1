param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

function Assert-SuccessExitAfter {
    param(
        [string]$Text,
        [string]$Anchor,
        [string]$Message
    )

    $anchorIndex = $Text.IndexOf($Anchor, [System.StringComparison]::Ordinal)
    if ($anchorIndex -lt 0) {
        throw "$Message Missing anchor='$Anchor'."
    }

    $tail = $Text.Substring($anchorIndex, [Math]::Min(360, $Text.Length - $anchorIndex))
    if ($tail -notmatch [regex]::Escape("exit 0")) {
        throw "$Message Missing explicit success exit after anchor='$Anchor'."
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_pdf_visual_release_gate.ps1"
$scriptText = Get-Content -Raw -LiteralPath $scriptPath
$manifestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest.json"
$manifestText = Get-Content -Raw -LiteralPath $manifestPath

$parseTokens = $null
$parseErrors = $null
[System.Management.Automation.Language.Parser]::ParseFile(
    $scriptPath,
    [ref]$parseTokens,
    [ref]$parseErrors
) | Out-Null
if ($parseErrors.Count -gt 0) {
    throw "PDF visual release gate script has parse errors."
}

$requiredSamples = @(
    "styled-text",
    "mixed-style-text",
    "underline-text",
    "document-contract-cjk-style",
    "document-eastasia-style-probe"
)

foreach ($sample in $requiredSamples) {
    Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $sample) `
        -Message "PDF regression manifest should define style baseline sample '$sample'."
}

$requiredFocusMarkers = @(
    "font-size",
    "bold-italic-underline",
    "underline-signature",
    "east-asia-font-mapping"
)

foreach ($marker in $requiredFocusMarkers) {
    Assert-ContainsText -Text $manifestText -ExpectedText $marker `
        -Message "PDF regression manifest should record style focus marker '$marker'."
}

Assert-ContainsText -Text $scriptText -ExpectedText "style_focus = @(`$StyleFocus)" `
    -Message "Rendered baseline summary should carry style_focus metadata."
Assert-ContainsText -Text $scriptText -ExpectedText "expect_visual_baseline -eq `$true" `
    -Message "PDF visual release gate should select visual baseline samples from the manifest."
Assert-ContainsText -Text $scriptText -ExpectedText "visual_style_focus" `
    -Message "PDF visual release gate should carry manifest style focus metadata."
Assert-ContainsText -Text $scriptText -ExpectedText "unicode-font.log" `
    -Message "PDF visual release gate should define the unicode font visual regression log path."
Assert-ContainsText -Text $scriptText -ExpectedText '[string]$CtestExecutable = "ctest"' `
    -Message "PDF visual release gate should allow CTest to be passed explicitly."
Assert-ContainsText -Text $scriptText -ExpectedText "Resolve-CtestExecutable -Executable `$CtestExecutable" `
    -Message "PDF visual release gate should resolve CTest before nested invocations."
Assert-ContainsText -Text $scriptText -ExpectedText '"-CtestExecutable", $resolvedCtestExecutable' `
    -Message "PDF visual release gate should pass explicit CTest into the nested unicode visual regression."
Assert-ContainsText -Text (Get-Content -Raw -LiteralPath (Join-Path $resolvedRepoRoot "scripts\run_pdf_unicode_font_roundtrip_visual_regression.ps1")) -ExpectedText '[string]$CtestExecutable = "ctest"' `
    -Message "Unicode font visual regression should allow CTest to be passed explicitly."
Assert-ContainsText -Text (Get-Content -Raw -LiteralPath (Join-Path $resolvedRepoRoot "test\CMakeLists.txt")) -ExpectedText '${CMAKE_CTEST_COMMAND}' `
    -Message "CTest registration should pass an absolute CTest executable into the unicode visual regression."
Assert-ContainsText -Text $scriptText -ExpectedText '-ExecutablePath "powershell"' `
    -Message "PDF visual release gate should run unicode visual regression through captured command logging."
Assert-ContainsText -Text $scriptText -ExpectedText '"-File", $unicodeScriptPath' `
    -Message "PDF visual release gate should invoke the unicode visual regression script with explicit -File binding."
Assert-ContainsText -Text $scriptText -ExpectedText "-LogPath `$unicodeLog" `
    -Message "PDF visual release gate should write unicode visual regression output to its declared log."
Assert-ContainsText -Text $scriptText -ExpectedText "unicode_font = `$unicodeLog" `
    -Message "PDF visual release gate summary should expose the captured unicode visual regression log."
Assert-ContainsText -Text $scriptText -ExpectedText 'verdict = "pass"' `
    -Message "PDF visual release gate summary should expose an explicit pass/fail verdict."
Assert-ContainsText -Text $scriptText -ExpectedText 'status = "pass"' `
    -Message "PDF visual release gate summary should expose a machine-readable status."
Assert-ContainsText -Text $scriptText -ExpectedText "cjk_copy_search_count = `$cjkCopySearchResults.Count" `
    -Message "PDF visual release gate summary should expose CJK copy/search evidence count."
Assert-ContainsText -Text $scriptText -ExpectedText "visual_baseline_manifest_count = `$visualManifestSamples.Count" `
    -Message "PDF visual release gate summary should expose manifest visual baseline count."
Assert-ContainsText -Text $scriptText -ExpectedText "baselines_count = `$renderedSamples.Count" `
    -Message "PDF visual release gate summary should expose rendered baseline evidence count."
Assert-ContainsText -Text $scriptText -ExpectedText "[switch]`$VisualBaselineSliceOnly" `
    -Message "PDF visual release gate should expose a bounded visual baseline slice mode."
Assert-ContainsText -Text $scriptText -ExpectedText "featherdoc.pdf_visual_baseline_slice.v1" `
    -Message "PDF visual release gate should write a machine-readable slice summary."
Assert-ContainsText -Text $scriptText -ExpectedText "slice_summary_does_not_replace_full_visual_gate_verdict" `
    -Message "PDF visual baseline slice evidence must not replace the full visual gate verdict."
Assert-ContainsText -Text $scriptText -ExpectedText "VisualBaselineOffset" `
    -Message "PDF visual baseline slice mode should support explicit offsets."
Assert-ContainsText -Text $scriptText -ExpectedText "VisualBaselineLimit" `
    -Message "PDF visual baseline slice mode should support explicit limits."
Assert-ContainsText -Text $scriptText -ExpectedText "visual_baseline_slice_only" `
    -Message "PDF visual baseline slice summary should carry an auxiliary evidence scope."
Assert-ContainsText -Text $scriptText -ExpectedText "[switch]`$RebuildAggregateContactSheetOnly" `
    -Message "PDF visual release gate should expose a bounded aggregate contact-sheet rebuild mode."
Assert-ContainsText -Text $scriptText -ExpectedText "featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1" `
    -Message "PDF visual release gate should write a machine-readable aggregate rebuild summary."
Assert-ContainsText -Text $scriptText -ExpectedText "aggregate_rebuild_summary_does_not_replace_full_visual_gate_verdict" `
    -Message "PDF aggregate rebuild evidence must not replace the full visual gate verdict."
Assert-ContainsText -Text $scriptText -ExpectedText "aggregate_contact_sheet_rebuild_only" `
    -Message "PDF aggregate rebuild summary should carry an auxiliary evidence scope."
Assert-SuccessExitAfter -Text $scriptText -Anchor 'Write-Step "Visual baseline slice summary written to $sliceSummaryPath"' `
    -Message "PDF visual release gate slice mode should not inherit stale native exit codes after writing pass evidence."
Assert-SuccessExitAfter -Text $scriptText -Anchor 'Write-Step "Aggregate contact sheet rebuild summary written to $rebuildSummaryPath"' `
    -Message "PDF visual release gate aggregate rebuild mode should not inherit stale native exit codes after writing pass evidence."
Assert-SuccessExitAfter -Text $scriptText -Anchor 'Write-Step "Visual gate summary written to $summaryPath"' `
    -Message "PDF visual release gate full mode should not inherit stale native exit codes after writing pass evidence."
Assert-ContainsText -Text $scriptText -ExpectedText "[switch]`$FinalizeOnly" `
    -Message "PDF visual release gate should expose a finalize-only mode for already rendered outputs."
Assert-ContainsText -Text $scriptText -ExpectedText "Finalizing existing PDF visual release gate output" `
    -Message "PDF visual release gate should report when it is reusing existing rendered evidence."
Assert-ContainsText -Text $scriptText -ExpectedText "Read-ExistingRenderedSample" `
    -Message "PDF visual release gate should validate existing rendered sample summaries."
Assert-ContainsText -Text $scriptText -ExpectedText "Read-ExistingCjkCopySearchResult" `
    -Message "PDF visual release gate should validate existing CJK copy/search summaries."
Assert-ContainsText -Text $scriptText -ExpectedText '$previousErrorActionPreference = $ErrorActionPreference' `
    -Message "Captured PDF visual gate commands should preserve the caller error action preference."
Assert-ContainsText -Text $scriptText -ExpectedText '$ErrorActionPreference = "Continue"' `
    -Message "Captured PDF visual gate commands should keep native stderr capturable on Windows PowerShell."
Assert-ContainsText -Text $scriptText -ExpectedText '$ErrorActionPreference = $previousErrorActionPreference' `
    -Message "Captured PDF visual gate commands should restore the caller error action preference."
Assert-ContainsText -Text $manifestText -ExpectedText '"expect_visual_baseline": true' `
    -Message "PDF regression manifest should mark visual baseline samples."

Write-Host "PDF visual release gate style baseline regression passed."
