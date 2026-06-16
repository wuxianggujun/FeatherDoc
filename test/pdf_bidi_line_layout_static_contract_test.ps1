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
$textHeaderPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_text.hpp"
$textImplPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_text.cpp"
$paragraphsHeaderPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_paragraphs.hpp"
$paragraphsImplPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_paragraphs.cpp"
$tableLayoutPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_table_layout.cpp"
$renderPath = Join-Path $resolvedRepoRoot "src\pdf\pdf_document_adapter_render.cpp"
$cmakePath = Join-Path $resolvedRepoRoot "test\CMakeLists.txt"
$cmakeModuleDir = Join-Path $resolvedRepoRoot "test\cmake"
$cmakeTextParts = @(Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakePath)
if (Test-Path -LiteralPath $cmakeModuleDir) {
    $cmakeTextParts += Get-ChildItem -LiteralPath $cmakeModuleDir -Filter "*.cmake" |
        Sort-Object Name |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
}
$textHeader = Get-Content -Raw -LiteralPath $textHeaderPath
$textImpl = Get-Content -Raw -LiteralPath $textImplPath
$paragraphsHeader = Get-Content -Raw -LiteralPath $paragraphsHeaderPath
$paragraphsImpl = Get-Content -Raw -LiteralPath $paragraphsImplPath
$tableLayout = Get-Content -Raw -LiteralPath $tableLayoutPath
$render = Get-Content -Raw -LiteralPath $renderPath
$cmake = $cmakeTextParts -join "`n"

Assert-ContainsText -Text $textHeader -ExpectedText "bool rtl{false};" `
    -Message "PDF adapter text structs should carry run-level RTL metadata."
Assert-ContainsText -Text $textHeader -ExpectedText "bool bidi{false};" `
    -Message "PDF adapter lines should carry paragraph or fragment bidi metadata."
Assert-ContainsText -Text $textHeader -ExpectedText "line_contains_rtl_fragments" `
    -Message "PDF adapter text header should expose RTL fragment detection."

Assert-ContainsText -Text $textImpl -ExpectedText "fragment.rtl == rtl" `
    -Message "PDF adapter fragment coalescing should keep RTL in the style key."
Assert-ContainsText -Text $textImpl -ExpectedText "line.bidi = line.bidi || rtl;" `
    -Message "PDF adapter line wrapping should mark lines when RTL fragments are appended."
Assert-ContainsText -Text $textImpl -ExpectedText "PdfGlyphDirection::right_to_left" `
    -Message "PDF adapter RTL detection should account for shaping direction."
Assert-ContainsText -Text $textImpl -ExpectedText "style.rtl" `
    -Message "PDF adapter tokenization should propagate resolved RTL style metadata."
Assert-ContainsText -Text $textImpl -ExpectedText "token.rtl" `
    -Message "PDF adapter wrapping should propagate token RTL metadata into fragments."

Assert-ContainsText -Text $paragraphsHeader -ExpectedText "resolve_paragraph_bidi" `
    -Message "PDF paragraph adapter should expose paragraph bidi resolution."
Assert-ContainsText -Text $paragraphsImpl -ExpectedText "resolve_paragraph_style_properties" `
    -Message "PDF paragraph adapter should resolve paragraph style bidi metadata."
Assert-ContainsText -Text $paragraphsImpl -ExpectedText "paragraph.bidi.value_or" `
    -Message "PDF paragraph adapter should prefer explicit paragraph bidi metadata."
Assert-ContainsText -Text $paragraphsImpl -ExpectedText "document.default_paragraph_bidi()" `
    -Message "PDF paragraph adapter should fall back to document default paragraph bidi."
Assert-ContainsText -Text $paragraphsImpl -ExpectedText "paragraph.bidi().value_or" `
    -Message "PDF cursor paragraph wrapping should preserve handle-level bidi metadata."
Assert-ContainsText -Text $paragraphsImpl -ExpectedText "line_contains_rtl_fragments(line)" `
    -Message "PDF paragraph wrapping should mark lines containing RTL fragments."

Assert-ContainsText -Text $tableLayout -ExpectedText "auto cell_bidi = false;" `
    -Message "PDF table wrapping should collect cell paragraph bidi metadata."
Assert-ContainsText -Text $tableLayout -ExpectedText "cell_bidi = cell_bidi || paragraph.bidi().value_or(false);" `
    -Message "PDF table wrapping should preserve bidi metadata across cell paragraphs."
Assert-ContainsText -Text $tableLayout -ExpectedText "line.bidi = cell_bidi || line.bidi || line_contains_rtl_fragments(line);" `
    -Message "PDF table wrapping should mark bidi lines after cell wrapping."

Assert-ContainsText -Text $render -ExpectedText "#include <iterator>" `
    -Message "PDF bidi rendering helper should include iterator for std::distance."
Assert-ContainsText -Text $render -ExpectedText "fragment_visual_advances" `
    -Message "PDF rendering should calculate per-fragment visual advances."
Assert-ContainsText -Text $render -ExpectedText "if (!line.bidi)" `
    -Message "PDF rendering should keep the fast path for non-bidi lines."
Assert-ContainsText -Text $render -ExpectedText "first_rtl" `
    -Message "PDF rendering should find the first RTL fragment before reordering advances."
Assert-ContainsText -Text $render -ExpectedText "std::distance" `
    -Message "PDF rendering should preserve prefix width before RTL visual advance handling."
Assert-ContainsText -Text $render -ExpectedText "visual_advances[index]" `
    -Message "PDF rendering should emit fragments using calculated visual advances."

Assert-ContainsText -Text $cmake -ExpectedText "pdf_bidi_line_layout_static_contract" `
    -Message "CMake should register the PDF bidi line layout static contract."
Assert-ContainsText -Text $cmake -ExpectedText "pdf_bidi_line_layout_static_contract_test.ps1" `
    -Message "CMake should reference the PDF bidi line layout contract script."
Assert-ContainsText -Text $cmake -ExpectedText "LABELS `"pdf;layout;smoke`"" `
    -Message "PDF bidi line layout contract should carry pdf/layout/smoke labels."

Write-Host "PDF bidi line layout static contract passed."
