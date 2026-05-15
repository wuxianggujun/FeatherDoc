param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if (-not $Text.Contains($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText'."
    }
}

function Assert-DoesNotContainText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Label
    )

    if ($Text.Contains($UnexpectedText)) {
        throw "$Label contains unexpected text '$UnexpectedText'."
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$pdfImportDocsPath = Join-Path $resolvedRepoRoot "docs\pdf_import.rst"
$docsIndexPath = Join-Path $resolvedRepoRoot "docs\index.rst"
$readmePath = Join-Path $resolvedRepoRoot "README.md"
$readmeZhPath = Join-Path $resolvedRepoRoot "README.zh-CN.md"
$cmakeListsPath = Join-Path $resolvedRepoRoot "CMakeLists.txt"

$pdfImportDocsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImportDocsPath
$docsIndexText = Get-Content -Raw -Encoding UTF8 -LiteralPath $docsIndexPath
$readmeText = Get-Content -Raw -Encoding UTF8 -LiteralPath $readmePath
$readmeZhText = Get-Content -Raw -Encoding UTF8 -LiteralPath $readmeZhPath
$cmakeListsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakeListsPath

$requiredPdfImportDocsTerms = @(
    "PDF Import",
    "PDF import JSON diagnostics",
    "PDF import supported scope and limits",
    "featherdoc_cli import-pdf input.pdf --output imported.docx --json",
    "--import-table-candidates-as-tables",
    "--min-table-continuation-confidence",
    '"table_continuation_diagnostics_count": 2',
    '"table_continuation_diagnostics": []',
    "page_index",
    "block_index",
    "source_row_offset",
    "continuation_confidence",
    "minimum_continuation_confidence",
    "has_previous_table",
    "is_first_block_on_page",
    "is_near_page_top",
    "source_rows_consistent",
    "column_count_matches",
    "column_anchors_match",
    "previous_has_repeating_header",
    "source_has_repeating_header",
    "header_matches_previous",
    "header_match_kind",
    "not_required",
    "exact",
    "normalized_text",
    "plural_variant",
    "canonical_text",
    "token_set",
    "skipped_repeating_header",
    "disposition",
    "created_new_table",
    "merged_with_previous_table",
    "blocker",
    "no_previous_table",
    "not_first_block_on_page",
    "not_near_page_top",
    "inconsistent_source_rows",
    "column_count_mismatch",
    "column_anchors_mismatch",
    "repeated_header_mismatch",
    "continuation_confidence_below_threshold",
    '"ok": false',
    '"stage": "import"',
    '"failure_kind": "table_candidates_detected"',
    "parse_failed",
    "document_create_failed",
    "document_population_failed",
    "extract_text_disabled",
    "extract_geometry_disabled",
    "table_candidates_detected",
    "no_text_paragraphs"
)

foreach ($term in $requiredPdfImportDocsTerms) {
    Assert-ContainsText -Text $pdfImportDocsText -ExpectedText $term -Label "docs/pdf_import.rst"
}

Assert-ContainsText -Text $docsIndexText -ExpectedText "   pdf_import" -Label "docs/index.rst"
Assert-ContainsText -Text $docsIndexText -ExpectedText ':doc:`pdf_import`' -Label "docs/index.rst"
Assert-DoesNotContainText -Text $docsIndexText -UnexpectedText "PDF import JSON diagnostics" -Label "docs/index.rst"
Assert-DoesNotContainText -Text $docsIndexText -UnexpectedText "PDF import supported scope and limits" -Label "docs/index.rst"

Assert-ContainsText -Text $readmeText -ExpectedText "docs/pdf_import.rst" -Label "README.md"
Assert-ContainsText -Text $readmeZhText -ExpectedText "docs/pdf_import.rst" -Label "README.zh-CN.md"
Assert-ContainsText -Text $cmakeListsText -ExpectedText 'docs/pdf_import.rst' -Label "CMakeLists.txt"
Assert-ContainsText -Text $cmakeListsText -ExpectedText '${FEATHERDOC_INSTALL_DATADIR}/docs' -Label "CMakeLists.txt"
Assert-DoesNotContainText `
    -Text $readmeText `
    -UnexpectedText 'docs/index.rst` for the field-level schema' `
    -Label "README.md"

Write-Host "PDF import docs contract passed."
