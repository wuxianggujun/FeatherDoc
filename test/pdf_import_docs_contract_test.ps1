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

function Get-TextSection {
    param(
        [string]$Text,
        [string]$StartText,
        [string]$EndText,
        [string]$Label
    )

    $startIndex = $Text.IndexOf($StartText, [System.StringComparison]::Ordinal)
    if ($startIndex -lt 0) {
        throw "$Label does not contain start marker '$StartText'."
    }

    $endIndex = $Text.IndexOf(
        $EndText,
        $startIndex + $StartText.Length,
        [System.StringComparison]::Ordinal)
    if ($endIndex -lt 0) {
        throw "$Label does not contain end marker '$EndText'."
    }

    return $Text.Substring($startIndex, $endIndex - $startIndex)
}

function Assert-CMakeInstallFilesToDestination {
    param(
        [string]$Text,
        [string[]]$ExpectedFiles,
        [string]$Destination,
        [string]$Label
    )

    $pattern = 'install\s*\(\s*FILES(?<files>.*?)DESTINATION\s+"(?<destination>[^"]+)"(?<tail>.*?)\)'
    foreach ($match in [regex]::Matches(
            $Text,
            $pattern,
            [System.Text.RegularExpressions.RegexOptions]::Singleline)) {
        if ($match.Groups["destination"].Value -ne $Destination) {
            continue
        }

        $filesText = $match.Groups["files"].Value
        $missingFiles = @()
        foreach ($expectedFile in $ExpectedFiles) {
            if (-not $filesText.Contains($expectedFile)) {
                $missingFiles += $expectedFile
            }
        }

        if ($missingFiles.Count -eq 0) {
            return
        }
    }

    throw "$Label does not install expected files to '$Destination': $($ExpectedFiles -join ', ')."
}

function Get-RstToctreeEntries {
    param(
        [string]$Text
    )

    $lines = $Text -split "`r?`n"
    $entries = @()
    for ($index = 0; $index -lt $lines.Count; ++$index) {
        if ($lines[$index] -notmatch '^\.\. toctree::\s*$') {
            continue
        }

        for ($lineIndex = $index + 1; $lineIndex -lt $lines.Count; ++$lineIndex) {
            $line = $lines[$lineIndex]
            if ([string]::IsNullOrWhiteSpace($line)) {
                continue
            }

            if ($line -notmatch '^   (?<entry>\S.*)$') {
                break
            }

            $entry = $matches["entry"].Trim()
            if ($entry.StartsWith(":")) {
                continue
            }

            $entries += $entry
        }
    }

    return $entries
}

function Assert-RstToctreeContainsEntries {
    param(
        [string]$Text,
        [string[]]$ExpectedEntries,
        [string]$Label
    )

    $entries = @(Get-RstToctreeEntries -Text $Text)
    foreach ($expectedEntry in $ExpectedEntries) {
        if ($entries -notcontains $expectedEntry) {
            throw "$Label toctree does not contain expected entry '$expectedEntry'."
        }
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

function Get-CliJsonBlockerMembers {
    param(
        [string]$Text
    )

    $members = @()
    foreach ($match in [regex]::Matches($Text, '"blocker":"(?<member>[A-Za-z_][A-Za-z0-9_]*)"')) {
        $member = $match.Groups["member"].Value
        if ($members -notcontains $member) {
            $members += $member
        }
    }

    return $members
}

function Get-RstCodeBlocks {
    param(
        [string]$Text,
        [string]$Language
    )

    $lines = $Text -split "`r?`n"
    $blocks = @()
    for ($index = 0; $index -lt $lines.Count; ++$index) {
        if ($lines[$index] -notmatch ('^\.\. code-block::\s+' + [regex]::Escape($Language) + '\s*$')) {
            continue
        }

        $blockLines = @()
        for ($lineIndex = $index + 1; $lineIndex -lt $lines.Count; ++$lineIndex) {
            $line = $lines[$lineIndex]
            if ([string]::IsNullOrWhiteSpace($line)) {
                if ($blockLines.Count -gt 0) {
                    $blockLines += ""
                }
                continue
            }

            if ($line -match '^    (?<body>.*)$') {
                $blockLines += $matches["body"]
                continue
            }

            if ($blockLines.Count -gt 0) {
                break
            }
        }

        if ($blockLines.Count -gt 0) {
            $blocks += (($blockLines -join "`n").Trim())
        }
    }

    return $blocks
}

function Assert-RstJsonCodeBlocksParse {
    param(
        [string]$Text,
        [string]$Label
    )

    $blocks = @(Get-RstCodeBlocks -Text $Text -Language "json")
    if ($blocks.Count -eq 0) {
        throw "$Label does not contain any JSON code blocks."
    }

    for ($index = 0; $index -lt $blocks.Count; ++$index) {
        try {
            $null = $blocks[$index] | ConvertFrom-Json
        }
        catch {
            throw "$Label JSON code block $($index + 1) is not valid JSON: $($_.Exception.Message)"
        }
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$pdfImportDocsPath = Join-Path $resolvedRepoRoot "docs\pdf_import.rst"
$pdfImportJsonDiagnosticsDocsPath = Join-Path $resolvedRepoRoot "docs\pdf_import_json_diagnostics.rst"
$pdfImportScopeDocsPath = Join-Path $resolvedRepoRoot "docs\pdf_import_scope.rst"
$docsIndexPath = Join-Path $resolvedRepoRoot "docs\index.rst"
$readmePath = Join-Path $resolvedRepoRoot "README.md"
$readmeZhPath = Join-Path $resolvedRepoRoot "README.zh-CN.md"
$cmakeListsPath = Join-Path $resolvedRepoRoot "CMakeLists.txt"
$pdfImporterHeaderPath = Join-Path $resolvedRepoRoot "include\featherdoc\pdf\pdf_document_importer.hpp"
$cliPath = Join-Path $resolvedRepoRoot "cli\featherdoc_cli.cpp"
$pdfCliImportTestsPath = Join-Path $resolvedRepoRoot "test\pdf_cli_import_tests.cpp"
$pdfImportStructureTestsPath = Join-Path $resolvedRepoRoot "test\pdf_import_structure_tests.cpp"
$pdfImportFailureTestsPath = Join-Path $resolvedRepoRoot "test\pdf_import_failure_tests.cpp"
$pdfImportTableHeuristicTestsPath = Join-Path $resolvedRepoRoot "test\pdf_import_table_heuristic_tests.cpp"

$pdfImportDocsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImportDocsPath
$pdfImportJsonDiagnosticsDocsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImportJsonDiagnosticsDocsPath
$pdfImportScopeDocsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImportScopeDocsPath
$docsIndexText = Get-Content -Raw -Encoding UTF8 -LiteralPath $docsIndexPath
$readmeText = Get-Content -Raw -Encoding UTF8 -LiteralPath $readmePath
$readmeZhText = Get-Content -Raw -Encoding UTF8 -LiteralPath $readmeZhPath
$cmakeListsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakeListsPath
$pdfImporterHeaderText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImporterHeaderPath
$cliText = Get-Content -Raw -Encoding UTF8 -LiteralPath $cliPath
$pdfCliImportTestsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfCliImportTestsPath
$pdfImportStructureTestsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImportStructureTestsPath
$pdfImportFailureTestsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImportFailureTestsPath
$pdfImportTableHeuristicTestsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $pdfImportTableHeuristicTestsPath

$readmePdfImportSection = Get-TextSection `
    -Text $readmeText `
    -StartText "When PDF import is enabled" `
    -EndText "## Build With MSVC" `
    -Label "README.md PDF import section"
$readmeZhPdfImportSection = Get-TextSection `
    -Text $readmeZhText `
    -StartText "featherdoc_cli import-pdf input.pdf --output output.docx" `
    -EndText "## MSVC" `
    -Label "README.zh-CN.md PDF import section"
$pdfImportInstalledDocs = @(
    'docs/pdf_import.rst',
    'docs/pdf_import_json_diagnostics.rst',
    'docs/pdf_import_scope.rst'
)
$pdfImportToctreeEntries = @(
    'pdf_import',
    'pdf_import_json_diagnostics',
    'pdf_import_scope'
)

$requiredPdfImportDocsTerms = @(
    "PDF Import",
    "featherdoc_cli import-pdf input.pdf --output imported.docx --json",
    "--import-table-candidates-as-tables",
    "--min-table-continuation-confidence",
    "pdf_import_json_diagnostics",
    "pdf_import_scope",
    "PDF Import JSON Diagnostics",
    "PDF Import Supported Scope And Limits"
)

$requiredPdfImportJsonDiagnosticsDocsTerms = @(
    "PDF Import JSON Diagnostics",
    "PDF import JSON diagnostics",
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
    "internal consistency guard",
    "column_count_mismatch",
    "column_anchors_mismatch",
    "repeated_header_mismatch",
    "continuation_confidence_below_threshold",
    "Common continuation blockers",
    '"blocker": "repeated_header_mismatch"',
    '"blocker": "column_anchors_mismatch"',
    '"blocker": "continuation_confidence_below_threshold"',
    '"blocker": "not_first_block_on_page"',
    '"blocker": "not_near_page_top"',
    '"is_first_block_on_page": false',
    '"is_near_page_top": false',
    '"minimum_continuation_confidence": 90',
    '"ok": false',
    '"stage": "import"',
    '"failure_kind": "table_candidates_detected"',
    "parse_failed",
    "Command-line parse errors",
    '"stage": "parse"',
    "missing value after --min-table-continuation-confidence",
    "invalid value after --min-table-continuation-confidence",
    "duplicate --min-table-continuation-confidence option",
    "Parse errors do not write the target DOCX",
    "document_create_failed",
    "document_population_failed",
    "extract_text_disabled",
    "extract_geometry_disabled",
    "table_candidates_detected",
    "no_text_paragraphs"
)

$requiredPdfImportScopeDocsTerms = @(
    "PDF Import Supported Scope And Limits",
    "PDF import supported scope and limits",
    "Paragraph import from extractable PDF text",
    "Conservative table-candidate detection",
    "Opt-in table promotion",
    "Cross-page table continuation",
    "Repeated-header detection",
    "Conservative subtotal / total summary-row handling",
    'Diagnostics through ``table_continuation_diagnostics``',
    "Column-count mismatches",
    "Ordinary two-column prose",
    "scanned PDFs",
    "OCR",
    "image-only",
    "arbitrary nested table semantics",
    "arbitrary local column drift"
)

$scopeCoverageAnchors = @(
    @{
        Label = "Paragraph import from extractable PDF text"
        DocExpected = "Paragraph import from extractable PDF text"
        Text = $pdfImportStructureTestsText
        Expected = "PDF text importer builds a plain FeatherDoc document"
    },
    @{
        Label = "Table candidates are rejected by default"
        DocExpected = "Table candidates are rejected by default"
        Text = $pdfImportFailureTestsText
        Expected = "PDF text importer classifies detected table candidates"
    },
    @{
        Label = "Opt-in table promotion"
        DocExpected = "Opt-in table promotion"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF text importer can opt in to table candidate import"
    },
    @{
        Label = "Simple key-value table recovery"
        DocExpected = "key-value tables"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF text importer can opt in to two-column key-value table import"
    },
    @{
        Label = "Borderless aligned table recovery"
        DocExpected = "selected borderless aligned tables"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF text importer can opt in to borderless key-value table import"
    },
    @{
        Label = "Cross-page compatible table continuation"
        DocExpected = "Cross-page table continuation"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import merges compatible table candidates across page boundary"
    },
    @{
        Label = "Repeated-header exact detection"
        DocExpected = "Repeated-header detection"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import skips repeated source header rows while merging cross-page repeated-header table"
    },
    @{
        Label = "Repeated-header normalized detection"
        DocExpected = "whitespace/case/punctuation"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import skips case and separator-varied repeated source header rows"
    },
    @{
        Label = "Repeated-header plural detection"
        DocExpected = "conservative plural variants"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import skips plural-varied repeated source header rows"
    },
    @{
        Label = "Repeated-header abbreviation detection"
        DocExpected = "small abbreviation whitelist"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import skips abbreviation-varied repeated source header rows"
    },
    @{
        Label = "Repeated-header word order detection"
        DocExpected = "token-set word"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import skips word-order-varied repeated source header rows"
    },
    @{
        Label = "Subtotal summary-row handling"
        DocExpected = "Conservative subtotal / total summary-row handling"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF text importer preserves inline subtotal-row table cells"
    },
    @{
        Label = "Continuation diagnostics"
        DocExpected = 'Diagnostics through ``table_continuation_diagnostics``'
        Text = $pdfCliImportTestsText
        Expected = '"table_continuation_diagnostics_count":2'
    },
    @{
        Label = "Column-count mismatch split"
        DocExpected = "Column-count mismatches"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import keeps cross-page subtotal tables separate for column count mismatches"
    },
    @{
        Label = "Column anchors mismatch split"
        DocExpected = "incompatible column anchors"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import does not merge cross-page tables with incompatible widths"
    },
    @{
        Label = "Semantic repeated-header mismatch split"
        DocExpected = "semantic repeated-header"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import keeps semantic header variants as separate cross-page tables"
    },
    @{
        Label = "Low continuation confidence split"
        DocExpected = "low continuation confidence"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import can require a higher confidence before cross-page merge"
    },
    @{
        Label = "Intervening paragraph split"
        DocExpected = "intervening paragraphs"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import does not merge through an intervening paragraph"
    },
    @{
        Label = "Too-low next-page table split"
        DocExpected = "near the top of the page"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import does not merge cross-page tables that start too low on the next page"
    },
    @{
        Label = "Ordinary two-column prose stays paragraphs"
        DocExpected = "Ordinary two-column prose"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDFium parser does not classify two-column prose as table candidate"
    },
    @{
        Label = "Numbered lists stay paragraphs"
        DocExpected = "numbered lists"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDFium parser does not classify aligned numbered list as table candidate"
    },
    @{
        Label = "Short-label prose stays paragraphs"
        DocExpected = "short-label prose"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDFium parser does not classify two-column short-label prose as table candidate"
    },
    @{
        Label = "Free-form column drift stays paragraphs"
        DocExpected = "free-form"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF text importer keeps free-form column drift prose as paragraphs"
    },
    @{
        Label = "Local anchor drift split"
        DocExpected = "arbitrary local column drift"
        Text = $pdfImportTableHeuristicTestsText
        Expected = "PDF table import keeps cross-page subtotal tables separate for local anchor drift"
    }
)

foreach ($term in $requiredPdfImportDocsTerms) {
    Assert-ContainsText -Text $pdfImportDocsText -ExpectedText $term -Label "docs/pdf_import.rst"
}

foreach ($term in $requiredPdfImportJsonDiagnosticsDocsTerms) {
    Assert-ContainsText -Text $pdfImportJsonDiagnosticsDocsText -ExpectedText $term -Label "docs/pdf_import_json_diagnostics.rst"
}
Assert-RstJsonCodeBlocksParse `
    -Text $pdfImportJsonDiagnosticsDocsText `
    -Label "docs/pdf_import_json_diagnostics.rst"

foreach ($term in $requiredPdfImportScopeDocsTerms) {
    Assert-ContainsText -Text $pdfImportScopeDocsText -ExpectedText $term -Label "docs/pdf_import_scope.rst"
}
foreach ($anchor in $scopeCoverageAnchors) {
    Assert-ContainsText `
        -Text $pdfImportScopeDocsText `
        -ExpectedText $anchor.DocExpected `
        -Label "docs/pdf_import_scope.rst"
    Assert-ContainsText `
        -Text $anchor.Text `
        -ExpectedText $anchor.Expected `
        -Label ("PDF import scope coverage for {0}" -f $anchor.Label)
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
    -Text $pdfImportJsonDiagnosticsDocsText `
    -Members $failureKindMembers `
    -ExcludedMembers @("none") `
    -Label "docs/pdf_import_json_diagnostics.rst"
Assert-DocumentedEnumMembers `
    -Text $pdfImportJsonDiagnosticsDocsText `
    -Members $dispositionMembers `
    -ExcludedMembers @() `
    -Label "docs/pdf_import_json_diagnostics.rst"
Assert-DocumentedEnumMembers `
    -Text $pdfImportJsonDiagnosticsDocsText `
    -Members $blockerMembers `
    -ExcludedMembers @() `
    -Label "docs/pdf_import_json_diagnostics.rst"
Assert-DocumentedEnumMembers `
    -Text $pdfImportJsonDiagnosticsDocsText `
    -Members $headerMatchKindMembers `
    -ExcludedMembers @() `
    -Label "docs/pdf_import_json_diagnostics.rst"

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

$cliJsonBlockerMembers = Get-CliJsonBlockerMembers -Text $pdfCliImportTestsText
foreach ($blockerMember in $blockerMembers) {
    if ($blockerMember -in @("none", "inconsistent_source_rows")) {
        continue
    }

    if ($cliJsonBlockerMembers -notcontains $blockerMember) {
        throw "test/pdf_cli_import_tests.cpp does not cover blocker '$blockerMember' in CLI JSON assertions."
    }
}
Assert-ContainsText `
    -Text $pdfImportJsonDiagnosticsDocsText `
    -ExpectedText '``inconsistent_source_rows`` is an internal consistency guard' `
    -Label "docs/pdf_import_json_diagnostics.rst"

Assert-ContainsText -Text $docsIndexText -ExpectedText "   pdf_import" -Label "docs/index.rst"
Assert-ContainsText -Text $docsIndexText -ExpectedText "   pdf_import_json_diagnostics" -Label "docs/index.rst"
Assert-ContainsText -Text $docsIndexText -ExpectedText "   pdf_import_scope" -Label "docs/index.rst"
Assert-RstToctreeContainsEntries `
    -Text $docsIndexText `
    -ExpectedEntries $pdfImportToctreeEntries `
    -Label "docs/index.rst"
Assert-ContainsText -Text $docsIndexText -ExpectedText ':doc:`pdf_import`' -Label "docs/index.rst"
Assert-ContainsText -Text $docsIndexText -ExpectedText ':doc:`pdf_import_json_diagnostics`' -Label "docs/index.rst"
Assert-ContainsText -Text $docsIndexText -ExpectedText ':doc:`pdf_import_scope`' -Label "docs/index.rst"
Assert-DoesNotContainText -Text $docsIndexText -UnexpectedText "PDF import JSON diagnostics" -Label "docs/index.rst"
Assert-DoesNotContainText -Text $docsIndexText -UnexpectedText "PDF import supported scope and limits" -Label "docs/index.rst"

Assert-ContainsText -Text $readmeText -ExpectedText "docs/pdf_import.rst" -Label "README.md"
Assert-ContainsText -Text $readmeText -ExpectedText "docs/pdf_import_json_diagnostics.rst" -Label "README.md"
Assert-ContainsText -Text $readmeZhText -ExpectedText "docs/pdf_import.rst" -Label "README.zh-CN.md"
Assert-ContainsText -Text $readmeZhText -ExpectedText "docs/pdf_import_json_diagnostics.rst" -Label "README.zh-CN.md"
foreach ($installedDoc in $pdfImportInstalledDocs) {
    Assert-ContainsText `
        -Text $readmePdfImportSection `
        -ExpectedText $installedDoc `
        -Label "README.md PDF import section"
    Assert-ContainsText `
        -Text $readmeZhPdfImportSection `
        -ExpectedText $installedDoc `
        -Label "README.zh-CN.md PDF import section"
}
Assert-ContainsText `
    -Text $readmeText `
    -ExpectedText "--min-table-continuation-confidence <score>" `
    -Label "README.md"
Assert-ContainsText `
    -Text $readmeZhText `
    -ExpectedText "--min-table-continuation-confidence <score>" `
    -Label "README.zh-CN.md"
Assert-DoesNotContainText `
    -Text $readmeText `
    -UnexpectedText "--min-table-continuation-confidence <count>" `
    -Label "README.md"
Assert-DoesNotContainText `
    -Text $readmeZhText `
    -UnexpectedText "--min-table-continuation-confidence <count>" `
    -Label "README.zh-CN.md"
Assert-ContainsText -Text $cmakeListsText -ExpectedText 'docs/pdf_import.rst' -Label "CMakeLists.txt"
Assert-ContainsText -Text $cmakeListsText -ExpectedText 'docs/pdf_import_json_diagnostics.rst' -Label "CMakeLists.txt"
Assert-ContainsText -Text $cmakeListsText -ExpectedText 'docs/pdf_import_scope.rst' -Label "CMakeLists.txt"
Assert-ContainsText -Text $cmakeListsText -ExpectedText '${FEATHERDOC_INSTALL_DATADIR}/docs' -Label "CMakeLists.txt"
Assert-CMakeInstallFilesToDestination `
    -Text $cmakeListsText `
    -ExpectedFiles $pdfImportInstalledDocs `
    -Destination '${FEATHERDOC_INSTALL_DATADIR}/docs' `
    -Label "CMakeLists.txt"
Assert-DoesNotContainText `
    -Text $readmeText `
    -UnexpectedText 'docs/index.rst` for the field-level schema' `
    -Label "README.md"

Write-Host "PDF import docs contract passed."
