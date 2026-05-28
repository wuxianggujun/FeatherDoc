param(
    [string]$RepoRoot
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

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$fontResolverPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_font_resolver.cpp"
$fontResolverTestsPath = Join-Path $resolvedRepoRoot "test\pdf_font_resolver_tests.cpp"
$adapterFontTestsPath = Join-Path $resolvedRepoRoot "test\pdf_document_adapter_font_tests.cpp"
$tableLayoutPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_table_layout.cpp"
$tableLayoutHeaderPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_table_layout.hpp"
$tableEmitterPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_tables.cpp"

$fontResolverText = Get-Content -Raw -LiteralPath $fontResolverPath
$fontResolverTestsText = Get-Content -Raw -LiteralPath $fontResolverTestsPath
$adapterFontTestsText = Get-Content -Raw -LiteralPath $adapterFontTestsPath
$tableLayoutText = Get-Content -Raw -LiteralPath $tableLayoutPath
$tableLayoutHeaderText = Get-Content -Raw -LiteralPath $tableLayoutHeaderPath
$tableEmitterText = Get-Content -Raw -LiteralPath $tableEmitterPath

Assert-ContainsText -Text $fontResolverText -ExpectedText "resolve_unicode_fallback_path" `
    -Message "PDF font resolver should keep the Unicode fallback helper."
Assert-ContainsText -Text $fontResolverText -ExpectedText "font_path_supports_text" `
    -Message "PDF font resolver should keep glyph support checks before fallback."
Assert-ContainsText -Text $fontResolverText -ExpectedText "unicode_text && resolved_path" `
    -Message "PDF font resolver should re-check resolved fonts for Unicode text."
Assert-ContainsText -Text $fontResolverText -ExpectedText "fallback_supports_text" `
    -Message "PDF font resolver should confirm fallback font glyph coverage."
Assert-ContainsText -Text $fontResolverText -ExpectedText "resolved_family = std::string{east_asia_font_family}" `
    -Message "PDF font resolver should switch the resolved family to East Asia mapping."
Assert-ContainsText -Text $fontResolverText -ExpectedText "default_cjk_font_file_path" `
    -Message "PDF font resolver should keep configured default CJK font fallback."

Assert-ContainsText -Text $fontResolverTestsText -ExpectedText "font resolver switches to East Asia mapping when Latin glyphs are missing" `
    -Message "Font resolver tests should keep East Asia glyph fallback coverage."
Assert-ContainsText -Text $fontResolverTestsText -ExpectedText "font_has_glyph(cjk_font, codepoint)" `
    -Message "Font resolver tests should compare Latin and CJK glyph support."
Assert-ContainsText -Text $fontResolverTestsText -ExpectedText "font resolver falls back unicode prefixes to east Asia font mappings" `
    -Message "Font resolver tests should keep Unicode prefix East Asia fallback coverage."
Assert-ContainsText -Text $fontResolverTestsText -ExpectedText "font resolver falls back unicode prefixes to default CJK font path" `
    -Message "Font resolver tests should keep Unicode prefix default CJK fallback coverage."

Assert-ContainsText -Text $adapterFontTestsText -ExpectedText "document PDF adapter falls back bullet prefixes to east Asia fonts" `
    -Message "Adapter font tests should keep bullet prefix East Asia fallback coverage."
Assert-ContainsText -Text $adapterFontTestsText -ExpectedText "CHECK_EQ(bullet_run->font_family, `"Unit CJK`")" `
    -Message "Adapter font tests should assert bullet prefixes use the East Asia font family."
Assert-ContainsText -Text $adapterFontTestsText -ExpectedText "CHECK_EQ(bullet_run->font_file_path, cjk_font)" `
    -Message "Adapter font tests should assert bullet prefixes use the CJK font path."
Assert-ContainsText -Text $adapterFontTestsText -ExpectedText "document PDF adapter keeps exact-height table header text visible" `
    -Message "Adapter font tests should keep exact-height repeated table header coverage."
Assert-ContainsText -Text $adapterFontTestsText -ExpectedText "CHECK(rows.set_repeats_header())" `
    -Message "Adapter font tests should mark the table header rows as repeated."
Assert-ContainsText -Text $adapterFontTestsText -ExpectedText 'CHECK(page_has_text(layout.pages.front(), "Merged banner"))' `
    -Message "Adapter font tests should assert the merged exact-height header remains visible."

Assert-ContainsText -Text $tableLayoutHeaderText -ExpectedText "spanned_row_bottom" `
    -Message "Table layout header should expose spanned row bottom calculation."
Assert-ContainsText -Text $tableLayoutText -ExpectedText "double spanned_row_bottom" `
    -Message "Table layout implementation should keep spanned row bottom calculation."
Assert-ContainsText -Text $tableLayoutText -ExpectedText "clamped_row_span" `
    -Message "Table layout implementation should base spanned row height on clamped row span."
Assert-ContainsText -Text $tableEmitterText -ExpectedText "row_emission_height" `
    -Message "Table emitter should calculate emitted row height for repeated header fitting."
Assert-ContainsText -Text $tableEmitterText -ExpectedText "repeated_headers_fit_with_row" `
    -Message "Table emitter should guard repeated header fitting before emitting headers."
Assert-ContainsText -Text $tableEmitterText -ExpectedText "header_block_height +" `
    -Message "Table emitter should combine repeated header height with row emission height."
Assert-ContainsText -Text $tableEmitterText -ExpectedText "spanned_row_bottom(rows, row_index, cell, row_top" `
    -Message "Table emitter should use spanned row bottom when drawing merged cells."

Write-Host "PDF CJK fallback and table static contract passed."
