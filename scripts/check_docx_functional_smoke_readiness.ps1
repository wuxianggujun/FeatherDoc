<#
.SYNOPSIS
Checks persisted DOCX functional-smoke evidence without launching Word.

.DESCRIPTION
This read-only gate verifies that FeatherDoc still has a coherent low-cost
DOCX evidence chain for open/save package integrity, paragraph/table/image
samples, section header/footer targeting, content-control flows, and reused
Word visual-smoke screenshots. It does not run CMake, CTest, Word, LibreOffice,
browsers, or document rendering.
#>
param(
    [string]$RepoRoot = "",
    [string]$OutputJson = "output/docx-functional-smoke-readiness-current/summary.json",
    [string]$OutputDir = "",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [string[]]$InputJson = @(),
    [string[]]$VisualSmokeRoots = @(
        "output/word-visual-smoke-minimal-20260519",
        "output/word-visual-smoke-rerun-20260519"
    ),
    [int]$MinContactSheetBytes = 1024,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    if (-not [string]::IsNullOrWhiteSpace($RepoRoot)) {
        return (Resolve-Path $RepoRoot).Path
    }

    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param([string]$Root, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $Root $Path))
}

function Get-DisplayPath {
    param([string]$Root, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $rootPath = [System.IO.Path]::GetFullPath($Root)
    if (-not $rootPath.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $rootPath.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $rootPath += [System.IO.Path]::DirectorySeparatorChar
    }

    $resolved = [System.IO.Path]::GetFullPath($Path)
    if ($resolved.StartsWith($rootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolved.Substring($rootPath.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
        return ".\" + ($relative -replace '/', '\')
    }

    return $resolved
}

function Ensure-Directory {
    param([string]$Path)

    $dir = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($dir)) {
        New-Item -ItemType Directory -Path $dir -Force | Out-Null
    }
}

function Add-Check {
    param(
        [System.Collections.Generic.List[object]]$Checks,
        [string]$Name,
        [string]$Status,
        [bool]$Required,
        [string]$Message,
        [object]$Details
    )

    [void]$Checks.Add([ordered]@{
            name = $Name
            status = $Status
            required = $Required
            message = $Message
            details = $Details
        })
}

function Test-RequiredPaths {
    param([string]$Root, [string[]]$Paths)

    $items = @(
        foreach ($path in @($Paths)) {
            $resolved = Resolve-RepoPath -Root $Root -Path $path
            [ordered]@{
                path = $resolved
                path_display = Get-DisplayPath -Root $Root -Path $resolved
                exists = (Test-Path -LiteralPath $resolved)
            }
        }
    )

    $missing = @($items | Where-Object { -not [bool]$_.exists })
    return [ordered]@{
        path_count = $items.Count
        missing_count = $missing.Count
        paths = @($items)
        missing_paths = @($missing | ForEach-Object { [string]$_.path_display })
    }
}

function Test-DocxPackageEntries {
    param([string]$Root, [string]$Path, [string[]]$Entries)

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $resolved = Resolve-RepoPath -Root $Root -Path $Path
    $result = [ordered]@{
        path = $resolved
        path_display = Get-DisplayPath -Root $Root -Path $resolved
        exists = (Test-Path -LiteralPath $resolved)
        required_entries = @($Entries)
        missing_entries = @()
        error = ""
    }

    if (-not [bool]$result.exists) {
        $result.missing_entries = @($Entries)
        return $result
    }

    $archive = $null
    try {
        $archive = [System.IO.Compression.ZipFile]::OpenRead($resolved)
        $missing = New-Object 'System.Collections.Generic.List[string]'
        foreach ($entryName in @($Entries)) {
            if ($null -eq $archive.GetEntry($entryName)) {
                [void]$missing.Add($entryName)
            }
        }
        $result.missing_entries = @($missing.ToArray())
    } catch {
        $result.error = $_.Exception.Message
        $result.missing_entries = @($Entries)
    } finally {
        if ($null -ne $archive) { $archive.Dispose() }
    }

    return $result
}

function Test-ImageNonEmpty {
    param([string]$Root, [string]$Path)

    $resolved = Resolve-RepoPath -Root $Root -Path $Path
    $result = [ordered]@{
        path = $resolved
        path_display = Get-DisplayPath -Root $Root -Path $resolved
        exists = (Test-Path -LiteralPath $resolved)
        bytes = 0
        width = 0
        height = 0
        sampled_pixels = 0
        sampled_non_white = 0
        non_empty_visual = $false
        error = ""
    }

    if (-not [bool]$result.exists) {
        $result.error = "image_missing"
        return $result
    }

    $result.bytes = (Get-Item -LiteralPath $resolved).Length
    $bitmap = $null
    try {
        Add-Type -AssemblyName System.Drawing
        $bitmap = [System.Drawing.Bitmap]::new($resolved)
        $result.width = $bitmap.Width
        $result.height = $bitmap.Height
        $stepX = [Math]::Max(1, [int]($bitmap.Width / 80))
        $stepY = [Math]::Max(1, [int]($bitmap.Height / 120))
        for ($y = 0; $y -lt $bitmap.Height; $y += $stepY) {
            for ($x = 0; $x -lt $bitmap.Width; $x += $stepX) {
                $color = $bitmap.GetPixel($x, $y)
                $result.sampled_pixels++
                if ($color.R -lt 245 -or $color.G -lt 245 -or $color.B -lt 245) {
                    $result.sampled_non_white++
                }
            }
        }
        $result.non_empty_visual = ([int]$result.sampled_non_white -gt 0)
    } catch {
        $result.error = $_.Exception.Message
    } finally {
        if ($null -ne $bitmap) { $bitmap.Dispose() }
    }

    return $result
}

function Read-JsonOrNull {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

$repoRootPath = Resolve-RepoRoot
$effectiveOutputJson = if (-not [string]::IsNullOrWhiteSpace($SummaryJson)) {
    $SummaryJson
} elseif (-not [string]::IsNullOrWhiteSpace($OutputDir)) {
    Join-Path $OutputDir "summary.json"
} else {
    $OutputJson
}
$resolvedOutputJson = Resolve-RepoPath -Root $repoRootPath -Path $effectiveOutputJson
Ensure-Directory -Path $resolvedOutputJson
$resolvedOutputDir = if (-not [string]::IsNullOrWhiteSpace($OutputDir)) {
    Resolve-RepoPath -Root $repoRootPath -Path $OutputDir
} else {
    Split-Path -Parent $resolvedOutputJson
}
$resolvedReportMarkdown = if (-not [string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Resolve-RepoPath -Root $repoRootPath -Path $ReportMarkdown
} elseif (-not [string]::IsNullOrWhiteSpace($OutputDir)) {
    Join-Path $resolvedOutputDir "docx_functional_smoke_readiness.md"
} else {
    ""
}
if (-not [string]::IsNullOrWhiteSpace($resolvedReportMarkdown)) {
    Ensure-Directory -Path $resolvedReportMarkdown
}

$checks = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

$docxEntries = @("[Content_Types].xml", "_rels/.rels", "word/document.xml", "word/_rels/document.xml.rels")
$invoiceDocxEntries = Test-DocxPackageEntries -Root $repoRootPath -Path "samples/chinese_invoice_template.docx" -Entries $docxEntries
$myTestDocxEntries = Test-DocxPackageEntries -Root $repoRootPath -Path "samples/my_test.docx" -Entries $docxEntries
$docxPackageMissing = @($invoiceDocxEntries.missing_entries).Count + @($myTestDocxEntries.missing_entries).Count
Add-Check -Checks $checks -Name "docx_package_integrity" -Status $(if ($docxPackageMissing -eq 0 -and [bool]$invoiceDocxEntries.exists -and [bool]$myTestDocxEntries.exists) { "pass" } else { "fail" }) -Required $true -Message "Sample DOCX files must be readable OOXML packages." -Details ([ordered]@{
        samples = @($invoiceDocxEntries, $myTestDocxEntries)
    })

$capabilities = @(
    [ordered]@{
        id = "paragraphs_and_runs"
        label = "Paragraph/run editing"
        paths = @(
            "samples/sample_edit_existing_part_paragraphs.cpp",
            "samples/sample_insert_paragraph_before.cpp",
            "samples/sample_insert_run_like_existing.cpp",
            "samples/sample_paragraph_run_style_visual.cpp"
        )
    },
    [ordered]@{
        id = "tables"
        label = "Table editing and table geometry"
        paths = @(
            "samples/sample_edit_existing_part_tables.cpp",
            "samples/sample_insert_table_like_existing.cpp",
            "samples/sample_table_cell_margin_visual.cpp",
            "samples/sample_table_row_repeat_header_visual.cpp",
            "test/template_render_plan_test.ps1"
        )
    },
    [ordered]@{
        id = "images"
        label = "Image insertion/replacement"
        paths = @(
            "samples/sample_edit_existing_part_images.cpp",
            "samples/sample_edit_existing_part_append_images.cpp",
            "samples/sample_content_control_image_replacement_visual.cpp",
            "samples/sample_extended_image_formats_visual.cpp"
        )
    },
    [ordered]@{
        id = "sections_headers_footers"
        label = "Sections, headers, and footers"
        paths = @(
            "samples/sample_section_page_setup.cpp",
            "samples/sample_section_part_refs_visual.cpp",
            "samples/sample_part_template_validation.cpp",
            "test/render_template_document_from_data_test.ps1",
            "test/template_render_test_fixture_helpers.ps1"
        )
    },
    [ordered]@{
        id = "content_controls"
        label = "Content controls and Custom XML binding"
        paths = @(
            "samples/sample_content_control_rich_replacement_visual.cpp",
            "samples/sample_content_control_image_replacement_visual.cpp",
            "scripts/build_content_control_data_binding_governance_report.ps1",
            "test/build_content_control_data_binding_governance_report_test.ps1"
        )
    },
    [ordered]@{
        id = "fields_page_numbers"
        label = "Fields and page numbers"
        paths = @(
            "samples/sample_generic_fields_visual.cpp",
            "samples/sample_page_number_fields.cpp",
            "test/word_visual_release_gate_smoke_verdict_test.ps1"
        )
    },
    [ordered]@{
        id = "template_rendering"
        label = "Template rendering from data"
        paths = @(
            "samples/chinese_invoice_template.render_data.json",
            "samples/chinese_invoice_template.render_data_mapping.json",
            "scripts/render_template_document_from_data.ps1",
            "test/render_template_document_from_data_test.ps1",
            "test/validate_render_data_mapping_test.ps1"
        )
    },
    [ordered]@{
        id = "word_visual_gate_entrypoints"
        label = "Word visual smoke and release gate"
        paths = @(
            "scripts/run_word_visual_smoke.ps1",
            "scripts/run_word_visual_release_gate.ps1",
            "scripts/word_visual_review_report.ps1",
            "test/word_visual_review_report_test.ps1",
            "test/word_visual_release_gate_smoke_verdict_test.ps1"
        )
    }
)

$capabilitySummaries = @(
    foreach ($capability in @($capabilities)) {
        $pathResult = Test-RequiredPaths -Root $repoRootPath -Paths ([string[]]$capability.paths)
        $status = if ([int]$pathResult.missing_count -eq 0) { "pass" } else { "fail" }
        Add-Check -Checks $checks -Name ("capability.{0}" -f $capability.id) -Status $status -Required $true -Message ("Required DOCX capability evidence must exist: {0}" -f $capability.label) -Details $pathResult
        [ordered]@{
            id = [string]$capability.id
            label = [string]$capability.label
            status = $status
            required_path_count = [int]$pathResult.path_count
            missing_path_count = [int]$pathResult.missing_count
            missing_paths = @($pathResult.missing_paths)
        }
    }
)

$visualSmokeSummaries = @(
    foreach ($root in @($VisualSmokeRoots)) {
        $resolvedRoot = Resolve-RepoPath -Root $repoRootPath -Path $root
        $summaryPath = Join-Path $resolvedRoot "report\summary.json"
        $reviewPath = Join-Path $resolvedRoot "report\review_result.json"
        $contactSheetPath = Join-Path $resolvedRoot "evidence\contact_sheet.png"
        $pagePath = Join-Path $resolvedRoot "evidence\pages\page-01.png"
        $pdfPath = Join-Path $resolvedRoot "table_visual_smoke.pdf"
        $summary = Read-JsonOrNull -Path $summaryPath
        $review = Read-JsonOrNull -Path $reviewPath
        $contactProbe = Test-ImageNonEmpty -Root $repoRootPath -Path $contactSheetPath
        $pageProbe = Test-ImageNonEmpty -Root $repoRootPath -Path $pagePath
        $summaryPageCount = if ($null -eq $summary) { 0 } else { [int]$summary.page_count }
        $reviewVerdict = if ($null -eq $review) { "" } else { [string]$review.verdict }
        $reviewStatus = if ($null -eq $review) { "" } else { [string]$review.status }
        $status = if ((Test-Path -LiteralPath $summaryPath) -and
            (Test-Path -LiteralPath $reviewPath) -and
            (Test-Path -LiteralPath $pdfPath) -and
            [int]$summaryPageCount -gt 0 -and
            [int]$contactProbe.bytes -ge $MinContactSheetBytes -and
            [bool]$contactProbe.non_empty_visual -and
            [bool]$pageProbe.non_empty_visual) {
            "pass"
        } else {
            "fail"
        }
        if ($reviewVerdict -ne "pass") {
            [void]$warnings.Add([ordered]@{
                    id = "word_visual_smoke.pending_manual_review"
                    message = "Persisted Word visual smoke evidence is non-empty, but the review verdict is not pass."
                    root = $resolvedRoot
                    root_display = Get-DisplayPath -Root $repoRootPath -Path $resolvedRoot
                    review_status = $reviewStatus
                    review_verdict = $reviewVerdict
                })
        }
        [ordered]@{
            root = $resolvedRoot
            root_display = Get-DisplayPath -Root $repoRootPath -Path $resolvedRoot
            status = $status
            summary_json = $summaryPath
            summary_json_display = Get-DisplayPath -Root $repoRootPath -Path $summaryPath
            review_result_json = $reviewPath
            review_result_json_display = Get-DisplayPath -Root $repoRootPath -Path $reviewPath
            review_status = $reviewStatus
            review_verdict = $reviewVerdict
            page_count = $summaryPageCount
            pdf = $pdfPath
            pdf_display = Get-DisplayPath -Root $repoRootPath -Path $pdfPath
            contact_sheet = $contactProbe.path
            contact_sheet_display = $contactProbe.path_display
            contact_sheet_bytes = $contactProbe.bytes
            contact_sheet_width = $contactProbe.width
            contact_sheet_height = $contactProbe.height
            contact_sheet_non_empty_visual = $contactProbe.non_empty_visual
            page_image = $pageProbe.path
            page_image_display = $pageProbe.path_display
            page_image_non_empty_visual = $pageProbe.non_empty_visual
        }
    }
)

$failedVisualSmokeCount = @($visualSmokeSummaries | Where-Object { [string]$_.status -ne "pass" }).Count
Add-Check -Checks $checks -Name "word_visual_smoke_reused_evidence" -Status $(if ($failedVisualSmokeCount -eq 0 -and @($visualSmokeSummaries).Count -gt 0) { "pass" } else { "fail" }) -Required $true -Message "Persisted Word visual smoke contact sheets and page PNGs must exist and be non-empty." -Details ([ordered]@{
        visual_smoke_count = @($visualSmokeSummaries).Count
        failed_visual_smoke_count = $failedVisualSmokeCount
        visual_smoke = @($visualSmokeSummaries)
    })

$failedChecks = @($checks.ToArray() | Where-Object { [string]$_.status -eq "fail" -and [bool]$_.required })
$warningArray = @($warnings.ToArray())
$releaseBlockers = @(
    foreach ($failedCheck in @($failedChecks)) {
        [ordered]@{
            id = "docx_functional_smoke.$([string]$failedCheck.name)"
            severity = "error"
            status = "open"
            action = "restore_docx_functional_smoke_evidence"
            message = [string]$failedCheck.message
            source_schema = "featherdoc.docx_functional_smoke_readiness.v1"
            source_report = $resolvedOutputJson
            source_report_display = Get-DisplayPath -Root $repoRootPath -Path $resolvedOutputJson
            source_json = $resolvedOutputJson
            source_json_display = Get-DisplayPath -Root $repoRootPath -Path $resolvedOutputJson
        }
    }
)
$actionItems = @()
$statusValue = if ($failedChecks.Count -gt 0) { "fail" } else { "pass" }
$verdictValue = if ($failedChecks.Count -gt 0) {
    "fail"
} elseif ($warningArray.Count -gt 0) {
    "pass_with_warnings"
} else {
    "pass"
}

$summaryObject = [ordered]@{
    schema = "featherdoc.docx_functional_smoke_readiness.v1"
    generated_at = (Get-Date).ToString("s")
    status = $statusValue
    verdict = $verdictValue
    docx_functional_smoke_ready = ($failedChecks.Count -eq 0)
    release_ready = ($failedChecks.Count -eq 0 -and $warningArray.Count -eq 0)
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -Root $repoRootPath -Path $resolvedOutputDir
    summary_json = $resolvedOutputJson
    summary_json_display = Get-DisplayPath -Root $repoRootPath -Path $resolvedOutputJson
    report_markdown = $resolvedReportMarkdown
    report_markdown_display = Get-DisplayPath -Root $repoRootPath -Path $resolvedReportMarkdown
    input_json = @($InputJson)
    evidence_scope = "persisted_docx_functional_smoke_evidence_only"
    evidence_scope_note = "This read-only gate does not run CMake, CTest, Word, LibreOffice, browsers, or document rendering."
    check_count = $checks.Count
    failed_check_count = $failedChecks.Count
    warning_count = $warningArray.Count
    release_blocker_count = @($releaseBlockers).Count
    action_item_count = @($actionItems).Count
    capability_count = @($capabilitySummaries).Count
    capability_pass_count = @($capabilitySummaries | Where-Object { [string]$_.status -eq "pass" }).Count
    capabilities = @($capabilitySummaries)
    visual_smoke_reused_evidence = @($visualSmokeSummaries)
    checks = @($checks.ToArray())
    failed_checks = @($failedChecks)
    release_blockers = @($releaseBlockers)
    action_items = @($actionItems)
    warnings = @($warningArray)
    boundary = "Pass-with-warnings means persisted DOCX functional evidence is coherent and visual PNGs are non-empty; it does not claim a fresh Word COM render or completed human visual review."
    marker = "docx_functional_smoke_readiness_trace"
}

if (-not [string]::IsNullOrWhiteSpace($resolvedReportMarkdown)) {
    $markdownLines = New-Object 'System.Collections.Generic.List[string]'
    [void]$markdownLines.Add("# DOCX Functional Smoke Readiness")
    [void]$markdownLines.Add("")
    [void]$markdownLines.Add("- Status: ``$statusValue``")
    [void]$markdownLines.Add("- Verdict: ``$verdictValue``")
    [void]$markdownLines.Add("- DOCX functional smoke ready: ``$($summaryObject.docx_functional_smoke_ready)``")
    [void]$markdownLines.Add("- Failed checks: ``$($summaryObject.failed_check_count)``")
    [void]$markdownLines.Add("- Warnings: ``$($summaryObject.warning_count)``")
    [void]$markdownLines.Add("- Capability evidence: ``$($summaryObject.capability_pass_count)/$($summaryObject.capability_count)``")
    [void]$markdownLines.Add("- Summary JSON: ``$($summaryObject.summary_json_display)``")
    [void]$markdownLines.Add("")
    [void]$markdownLines.Add("## Reused Word Visual Smoke Evidence")
    [void]$markdownLines.Add("")
    foreach ($visualEvidence in @($visualSmokeSummaries)) {
        [void]$markdownLines.Add("- ``$($visualEvidence.root_display)``: status=``$($visualEvidence.status)``, review_verdict=``$($visualEvidence.review_verdict)``, contact_sheet_non_empty=``$($visualEvidence.contact_sheet_non_empty_visual)``, page_image_non_empty=``$($visualEvidence.page_image_non_empty_visual)``")
    }
    [void]$markdownLines.Add("")
    [void]$markdownLines.Add("## Boundary")
    [void]$markdownLines.Add("")
    [void]$markdownLines.Add($summaryObject.boundary)
    if ($warningArray.Count -gt 0) {
        [void]$markdownLines.Add("")
        [void]$markdownLines.Add("## Warnings")
        [void]$markdownLines.Add("")
        foreach ($warning in @($warningArray)) {
            [void]$markdownLines.Add("- ``$($warning.id)``: $($warning.message) root=``$($warning.root_display)`` review_verdict=``$($warning.review_verdict)``")
        }
    }
    ($markdownLines -join [System.Environment]::NewLine) | Set-Content -LiteralPath $resolvedReportMarkdown -Encoding UTF8
}

($summaryObject | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
$summaryObject | ConvertTo-Json -Depth 32

if ($failedChecks.Count -gt 0) {
    exit 1
}
if ($FailOnWarning -and $warningArray.Count -gt 0) {
    exit 2
}
