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

function Get-CppEnumMembers {
    param(
        [string]$Text,
        [string]$EnumName
    )

    $pattern = "enum class\s+" + [regex]::Escape($EnumName) + "\s*\{(?<body>.*?)\};"
    $match = [regex]::Match(
        $Text,
        $pattern,
        [System.Text.RegularExpressions.RegexOptions]::Singleline)
    if (-not $match.Success) {
        throw "Could not find enum '$EnumName'."
    }

    $members = @()
    foreach ($entry in ($match.Groups["body"].Value -split ",")) {
        $cleanEntry = ($entry -replace "//.*", "").Trim()
        if ([string]::IsNullOrWhiteSpace($cleanEntry)) {
            continue
        }

        $memberName = (($cleanEntry -split "=", 2)[0]).Trim()
        if ($memberName -notmatch "^[A-Za-z_][A-Za-z0-9_]*$") {
            throw "Could not parse enum member '$cleanEntry' from '$EnumName'."
        }

        $members += $memberName
    }

    if ($members.Count -eq 0) {
        throw "Enum '$EnumName' has no parsed members."
    }

    return $members
}

function Assert-DocumentedEnumMembers {
    param(
        [string]$Text,
        [string[]]$Members,
        [string[]]$ExcludedMembers,
        [string]$Label
    )

    foreach ($member in $Members) {
        if ($ExcludedMembers -contains $member) {
            continue
        }

        Assert-ContainsText -Text $Text -ExpectedText $member -Label $Label
    }
}

function Assert-CliMapsEnumMembers {
    param(
        [string]$Text,
        [string[]]$Members,
        [string]$Label
    )

    foreach ($member in $Members) {
        Assert-ContainsText `
            -Text $Text `
            -ExpectedText ('return "{0}";' -f $member) `
            -Label $Label
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
$pdfImporterHeaderPath = Join-Path $resolvedRepoRoot "include\featherdoc\pdf\pdf_document_importer.hpp"
$cliPath = Join-Path $resolvedRepoRoot "cli\featherdoc_cli.cpp"

$pdfImportDocsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImportDocsPath
$docsIndexText = Get-Content -Raw -Encoding UTF8 -LiteralPath $docsIndexPath
$readmeText = Get-Content -Raw -Encoding UTF8 -LiteralPath $readmePath
$readmeZhText = Get-Content -Raw -Encoding UTF8 -LiteralPath $readmeZhPath
$cmakeListsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakeListsPath
$pdfImporterHeaderText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImporterHeaderPath
$cliText = Get-Content -Raw -Encoding UTF8 -LiteralPath $cliPath

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

$failureKindMembers = Get-CppEnumMembers `
    -Text $pdfImporterHeaderText `
    -EnumName "PdfDocumentImportFailureKind"
$dispositionMembers = Get-CppEnumMembers `
    -Text $pdfImporterHeaderText `
    -EnumName "PdfTableContinuationDisposition"
$blockerMembers = Get-CppEnumMembers `
    -Text $pdfImporterHeaderText `
    -EnumName "PdfTableContinuationBlocker"
$headerMatchKindMembers = Get-CppEnumMembers `
    -Text $pdfImporterHeaderText `
    -EnumName "PdfTableContinuationHeaderMatchKind"

Assert-DocumentedEnumMembers `
    -Text $pdfImportDocsText `
    -Members $failureKindMembers `
    -ExcludedMembers @("none") `
    -Label "docs/pdf_import.rst"
Assert-DocumentedEnumMembers `
    -Text $pdfImportDocsText `
    -Members $dispositionMembers `
    -ExcludedMembers @() `
    -Label "docs/pdf_import.rst"
Assert-DocumentedEnumMembers `
    -Text $pdfImportDocsText `
    -Members $blockerMembers `
    -ExcludedMembers @() `
    -Label "docs/pdf_import.rst"
Assert-DocumentedEnumMembers `
    -Text $pdfImportDocsText `
    -Members $headerMatchKindMembers `
    -ExcludedMembers @() `
    -Label "docs/pdf_import.rst"

Assert-CliMapsEnumMembers `
    -Text $cliText `
    -Members $failureKindMembers `
    -Label "cli/featherdoc_cli.cpp"
Assert-CliMapsEnumMembers `
    -Text $cliText `
    -Members $dispositionMembers `
    -Label "cli/featherdoc_cli.cpp"
Assert-CliMapsEnumMembers `
    -Text $cliText `
    -Members $blockerMembers `
    -Label "cli/featherdoc_cli.cpp"
Assert-CliMapsEnumMembers `
    -Text $cliText `
    -Members $headerMatchKindMembers `
    -Label "cli/featherdoc_cli.cpp"

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
