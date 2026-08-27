param(
    [string]$RepoRoot = "",
    [string]$ReportDir = "output/pdf-visual-release-gate-current/report",
    [string]$OutputJson = "",
    [string]$ManifestPath = "test/pdf_regression_manifest.json",
    [string]$AttemptStartedAfter = "",
    [switch]$OuterGuardTimedOut,
    [int]$OuterGuardTimeoutSeconds = 0
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    if (-not [string]::IsNullOrWhiteSpace($RepoRoot)) {
        return (Resolve-Path -LiteralPath $RepoRoot).Path
    }

    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$Root,
        [string]$Path
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $Root $Path))
}

function Get-RepoRelativePath {
    param(
        [string]$Root,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $rootFullPath = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    if ($fullPath.StartsWith($rootFullPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $fullPath.Substring($rootFullPath.Length).TrimStart('\', '/')
        return ".\" + ($relative -replace '/', '\')
    }

    return $fullPath
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return "" }
    return [string]$value
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    return @($value)
}

function Read-JsonFile {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Get-CtestLogEvidence {
    param([string]$Path)

    $evidence = [ordered]@{
        status = "missing"
        verdict = "not_available"
        log_path = $Path
        log_path_display = ""
        selected_test_count = 0
        passed_test_count = 0
        failed_test_count = 0
        skipped_test_count = 0
        total_time_seconds = ""
        last_write_time = ""
        error = ""
    }

    if (-not (Test-Path -LiteralPath $Path)) {
        return [pscustomobject]$evidence
    }

    $item = Get-Item -LiteralPath $Path
    $text = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
    $evidence.status = "loaded"
    $evidence.verdict = "unknown"
    $evidence.last_write_time = $item.LastWriteTime.ToString("s")

    $summaryMatch = [regex]::Match($text, '(\d+)% tests passed,\s*(\d+) tests failed out of\s*(\d+)')
    if ($summaryMatch.Success) {
        $failedCount = [int]$summaryMatch.Groups[2].Value
        $selectedCount = [int]$summaryMatch.Groups[3].Value
        $skippedCount = [regex]::Matches($text, '\*\*\*Skipped').Count
        $passedCount = [Math]::Max(0, $selectedCount - $failedCount - $skippedCount)

        $evidence.selected_test_count = $selectedCount
        $evidence.passed_test_count = $passedCount
        $evidence.failed_test_count = $failedCount
        $evidence.skipped_test_count = $skippedCount
        $evidence.status = if ($failedCount -eq 0) { "pass" } else { "fail" }
        $evidence.verdict = if ($failedCount -eq 0) { "pass" } else { "fail" }
    }

    $timeMatch = [regex]::Match($text, 'Total Test time \(real\)\s*=\s*([0-9.]+)\s*sec')
    if ($timeMatch.Success) {
        $evidence.total_time_seconds = $timeMatch.Groups[1].Value
    }

    return [pscustomobject]$evidence
}

function Get-CjkCopySearchEvidence {
    param(
        [string]$Path,
        [int]$ExpectedCount,
        $AttemptStart
    )

    $summaryFiles = @()
    if (Test-Path -LiteralPath $Path) {
        $summaryFiles = @(Get-ChildItem -LiteralPath $Path -Filter "*-summary.json" -File -ErrorAction SilentlyContinue)
    }

    $missingTextCount = 0
    $unreadableCount = 0
    $freshCount = 0
    foreach ($file in $summaryFiles) {
        if ($null -ne $AttemptStart -and $file.LastWriteTime -ge $AttemptStart) {
            $freshCount++
        }

        try {
            $summary = Read-JsonFile -Path $file.FullName
            $missingTextCount += @(Get-JsonArray -Object $summary -Name "missing_text").Count
        } catch {
            $unreadableCount++
        }
    }

    $status = if ($summaryFiles.Count -eq 0) {
        "missing"
    } elseif ($unreadableCount -gt 0 -or $missingTextCount -gt 0) {
        "fail"
    } elseif ($ExpectedCount -gt 0 -and $summaryFiles.Count -lt $ExpectedCount) {
        "partial"
    } else {
        "pass"
    }

    return [pscustomobject][ordered]@{
        status = $status
        verdict = if ($status -eq "fail") { "fail" } elseif ($status -eq "pass") { "pass" } else { "not_complete" }
        log_dir = $Path
        summary_count = $summaryFiles.Count
        fresh_summary_count = $freshCount
        expected_summary_count = $ExpectedCount
        missing_text_count = $missingTextCount
        unreadable_summary_count = $unreadableCount
    }
}

function Get-PngDimensions {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 24) {
        return $null
    }

    $pngSignature = [byte[]](137, 80, 78, 71, 13, 10, 26, 10)
    for ($index = 0; $index -lt $pngSignature.Length; $index++) {
        if ($bytes[$index] -ne $pngSignature[$index]) {
            return $null
        }
    }

    $width = ([int]$bytes[16] -shl 24) -bor ([int]$bytes[17] -shl 16) -bor `
        ([int]$bytes[18] -shl 8) -bor [int]$bytes[19]
    $height = ([int]$bytes[20] -shl 24) -bor ([int]$bytes[21] -shl 16) -bor `
        ([int]$bytes[22] -shl 8) -bor [int]$bytes[23]

    if ($width -le 0 -or $height -le 0) {
        return $null
    }

    return [pscustomobject][ordered]@{
        width = $width
        height = $height
    }
}

function Get-JsonIntValue {
    param(
        $Object,
        [string]$Name,
        [int]$DefaultValue = 0
    )

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) {
        return $DefaultValue
    }

    return [int]$value
}

function Get-VisualBaselineEvidence {
    param(
        [string]$Path,
        [int]$ExpectedCount,
        $AttemptStart,
        [string[]]$ExpectedSampleNames = @()
    )

    $summaryFiles = @()
    if (Test-Path -LiteralPath $Path) {
        $summaryFiles = @(Get-ChildItem -LiteralPath $Path -Filter "summary.json" -File -Recurse -ErrorAction SilentlyContinue)
    }

    $freshCount = 0
    $unreadableSummaryCount = 0
    $missingPdfCount = 0
    $totalPdfBytes = [int64]0
    $pngPageCount = 0
    $missingPngPageCount = 0
    $totalPngBytes = [int64]0
    $unreadablePngDimensionCount = 0
    $dimensionCounts = @{}
    $renderedSampleNames = New-Object System.Collections.Generic.List[string]
    $freshRenderedSampleNames = New-Object System.Collections.Generic.List[string]
    $staleRenderedSampleNames = New-Object System.Collections.Generic.List[string]
    $sampleArtifacts = New-Object System.Collections.Generic.List[object]
    foreach ($file in @($summaryFiles | Sort-Object FullName)) {
        $sampleName = Split-Path -Leaf (Split-Path -Parent $file.FullName)
        if (-not [string]::IsNullOrWhiteSpace($sampleName)) {
            $renderedSampleNames.Add($sampleName) | Out-Null
        }

        $isFresh = $false
        if ($null -ne $AttemptStart -and $file.LastWriteTime -ge $AttemptStart) {
            $freshCount++
            $isFresh = $true
            if (-not [string]::IsNullOrWhiteSpace($sampleName)) {
                $freshRenderedSampleNames.Add($sampleName) | Out-Null
            }
        } elseif (-not [string]::IsNullOrWhiteSpace($sampleName)) {
            $staleRenderedSampleNames.Add($sampleName) | Out-Null
        }

        $summary = $null
        try {
            $summary = Read-JsonFile -Path $file.FullName
        } catch {
            $unreadableSummaryCount++
            $sampleArtifacts.Add([pscustomobject][ordered]@{
                    sample = $sampleName
                    status = "unreadable_summary"
                    fresh = $isFresh
                    summary_path = $file.FullName
                    input_pdf = ""
                    pdf_exists = $false
                    pdf_bytes = 0
                    expected_page_count = 0
                    png_page_count = 0
                    missing_png_page_count = 0
                    png_total_bytes = 0
                    page_dimensions = @()
                }) | Out-Null
            continue
        }

        $inputPdf = Get-JsonString -Object $summary -Name "input_pdf"
        $pdfExists = -not [string]::IsNullOrWhiteSpace($inputPdf) -and
            (Test-Path -LiteralPath $inputPdf -PathType Leaf)
        $pdfBytes = 0
        if ($pdfExists) {
            $pdfBytes = (Get-Item -LiteralPath $inputPdf).Length
            $totalPdfBytes += [int64]$pdfBytes
        } else {
            $missingPdfCount++
        }

        $expectedPageCount = Get-JsonIntValue -Object $summary -Name "page_count"
        $listedPagePaths = @(Get-JsonArray -Object $summary -Name "pages" | ForEach-Object { [string]$_ })
        if ($listedPagePaths.Count -eq 0) {
            $pagesDir = Join-Path (Split-Path -Parent $file.FullName) "pages"
            if (Test-Path -LiteralPath $pagesDir) {
                $listedPagePaths = @(
                    Get-ChildItem -LiteralPath $pagesDir -Filter "page-*.png" -File -ErrorAction SilentlyContinue |
                        Sort-Object Name |
                        ForEach-Object { $_.FullName }
                )
            }
        }
        if ($expectedPageCount -le 0) {
            $expectedPageCount = $listedPagePaths.Count
        }

        $existingPngPaths = @($listedPagePaths | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf })
        $listedMissingPngCount = @($listedPagePaths | Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) }).Count
        $expectedMissingPngCount = [Math]::Max(0, $expectedPageCount - $existingPngPaths.Count)
        $sampleMissingPngCount = [Math]::Max($listedMissingPngCount, $expectedMissingPngCount)
        $missingPngPageCount += $sampleMissingPngCount
        $pngPageCount += $existingPngPaths.Count

        $samplePngBytes = [int64]0
        $sampleUnreadablePngDimensionCount = 0
        $sampleDimensions = New-Object System.Collections.Generic.List[object]
        foreach ($pagePath in $existingPngPaths) {
            $pageItem = Get-Item -LiteralPath $pagePath
            $samplePngBytes += [int64]$pageItem.Length
            $dimensions = Get-PngDimensions -Path $pagePath
            if ($null -eq $dimensions) {
                $sampleUnreadablePngDimensionCount++
                $unreadablePngDimensionCount++
                continue
            }

            $dimensionKey = "$($dimensions.width)x$($dimensions.height)"
            if (-not $dimensionCounts.ContainsKey($dimensionKey)) {
                $dimensionCounts[$dimensionKey] = 0
            }
            $dimensionCounts[$dimensionKey]++
            $sampleDimensions.Add($dimensions) | Out-Null
        }
        $totalPngBytes += $samplePngBytes

        $artifactStatus = if (-not $pdfExists) {
            "missing_pdf"
        } elseif ($sampleMissingPngCount -gt 0) {
            "missing_png"
        } elseif ($sampleUnreadablePngDimensionCount -gt 0) {
            "unreadable_png_dimension"
        } else {
            "pass"
        }

        $sampleArtifacts.Add([pscustomobject][ordered]@{
                sample = $sampleName
                status = $artifactStatus
                fresh = $isFresh
                summary_path = $file.FullName
                input_pdf = $inputPdf
                pdf_exists = $pdfExists
                pdf_bytes = $pdfBytes
                expected_page_count = $expectedPageCount
                png_page_count = $existingPngPaths.Count
                missing_png_page_count = $sampleMissingPngCount
                png_total_bytes = $samplePngBytes
                page_dimensions = @($sampleDimensions.ToArray())
            }) | Out-Null
    }

    $expectedNames = @($ExpectedSampleNames | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $renderedLookup = @{}
    foreach ($name in @($renderedSampleNames.ToArray())) {
        $renderedLookup[$name] = $true
    }

    $freshLookup = @{}
    foreach ($name in @($freshRenderedSampleNames.ToArray())) {
        $freshLookup[$name] = $true
    }

    $missingSampleNames = @($expectedNames | Where-Object { -not $renderedLookup.ContainsKey($_) })
    $freshMissingSampleNames = @($expectedNames | Where-Object { -not $freshLookup.ContainsKey($_) })
    $firstFreshMissingOffset = -1
    for ($index = 0; $index -lt $expectedNames.Count; $index++) {
        if (-not $freshLookup.ContainsKey($expectedNames[$index])) {
            $firstFreshMissingOffset = $index
            break
        }
    }

    $resumeNeeded = $freshMissingSampleNames.Count -gt 0
    $resumeOffset = if ($resumeNeeded -and $firstFreshMissingOffset -ge 0) { $firstFreshMissingOffset } else { 0 }
    $resumeLimit = if ($resumeNeeded -and $firstFreshMissingOffset -ge 0) {
        $expectedNames.Count - $firstFreshMissingOffset
    } else {
        0
    }

    $attemptCount = if ($null -ne $AttemptStart) { $freshCount } else { $summaryFiles.Count }
    $artifactIssueCount = $unreadableSummaryCount + $missingPdfCount +
        $missingPngPageCount + $unreadablePngDimensionCount
    $status = if ($summaryFiles.Count -eq 0) {
        "missing"
    } elseif ($artifactIssueCount -gt 0) {
        "fail"
    } elseif ($ExpectedCount -gt 0 -and $attemptCount -lt $ExpectedCount) {
        "partial"
    } else {
        "pass"
    }

    $dimensionSummary = @(
        foreach ($key in @($dimensionCounts.Keys | Sort-Object)) {
            $parts = $key -split "x", 2
            [pscustomobject][ordered]@{
                width = [int]$parts[0]
                height = [int]$parts[1]
                count = [int]$dimensionCounts[$key]
            }
        }
    )

    return [pscustomobject][ordered]@{
        status = $status
        verdict = if ($status -eq "fail") { "fail" } elseif ($status -eq "pass") { "pass" } else { "not_complete" }
        baseline_dir = $Path
        rendered_summary_count = $summaryFiles.Count
        fresh_rendered_summary_count = $freshCount
        expected_rendered_summary_count = $ExpectedCount
        expected_sample_count = $expectedNames.Count
        expected_sample_names = @($expectedNames)
        rendered_sample_names = @($renderedSampleNames.ToArray() | Sort-Object)
        fresh_rendered_sample_names = @($freshRenderedSampleNames.ToArray())
        stale_rendered_sample_names = @($staleRenderedSampleNames.ToArray() | Sort-Object)
        missing_sample_count = $missingSampleNames.Count
        missing_sample_names = @($missingSampleNames)
        fresh_missing_sample_count = $freshMissingSampleNames.Count
        fresh_missing_sample_names = @($freshMissingSampleNames)
        resume_needed = $resumeNeeded
        resume_slice_offset = $resumeOffset
        resume_slice_limit = $resumeLimit
        unreadable_summary_count = $unreadableSummaryCount
        missing_pdf_count = $missingPdfCount
        pdf_total_bytes = $totalPdfBytes
        png_page_count = $pngPageCount
        missing_png_page_count = $missingPngPageCount
        png_total_bytes = $totalPngBytes
        unreadable_png_dimension_count = $unreadablePngDimensionCount
        png_dimension_summary = @($dimensionSummary)
        sample_artifacts = @($sampleArtifacts.ToArray())
    }
}

function Get-FileEvidence {
    param(
        [string]$Path,
        $AttemptStart
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        return [pscustomobject][ordered]@{
            status = "missing"
            path = $Path
            path_display = ""
            bytes = 0
            fresh = $false
            last_write_time = ""
        }
    }

    $item = Get-Item -LiteralPath $Path
    $fresh = $null -eq $AttemptStart -or $item.LastWriteTime -ge $AttemptStart
    return [pscustomobject][ordered]@{
        status = if ($fresh) { "pass" } else { "stale" }
        path = $Path
        path_display = ""
        bytes = $item.Length
        fresh = $fresh
        last_write_time = $item.LastWriteTime.ToString("s")
    }
}

function Get-AttemptStart {
    param(
        [string]$ReportPath,
        [string]$ExplicitStart
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitStart)) {
        return [datetime]::Parse($ExplicitStart)
    }

    $candidateFiles = @(
        Join-Path $ReportPath "preflight-summary.json"
        Join-Path $ReportPath "pdf-cli-export-test.log"
        Join-Path $ReportPath "pdf-regression-test.log"
        Join-Path $ReportPath "unicode-font.log"
    ) | Where-Object { Test-Path -LiteralPath $_ }

    if (@($candidateFiles).Count -eq 0) {
        return $null
    }

    return @($candidateFiles | ForEach-Object { (Get-Item -LiteralPath $_).LastWriteTime } | Sort-Object | Select-Object -First 1)[0]
}

$resolvedRepoRoot = Resolve-RepoRoot
$resolvedReportDir = Resolve-RepoPath -Root $resolvedRepoRoot -Path $ReportDir
$resolvedManifestPath = Resolve-RepoPath -Root $resolvedRepoRoot -Path $ManifestPath
$resolvedOutputJson = if ([string]::IsNullOrWhiteSpace($OutputJson)) {
    Join-Path $resolvedReportDir "attempt-summary.json"
} else {
    Resolve-RepoPath -Root $resolvedRepoRoot -Path $OutputJson
}

$outputDir = Split-Path -Parent $resolvedReportDir
$baselineDir = Join-Path $outputDir "baseline"
$cjkCopySearchDir = Join-Path $resolvedReportDir "cjk-copy-search"
$summaryJson = Join-Path $resolvedReportDir "summary.json"
$aggregateContactSheet = Join-Path $resolvedReportDir "aggregate-contact-sheet.png"
$pdfCliExportLog = Join-Path $resolvedReportDir "pdf-cli-export-test.log"
$pdfRegressionLog = Join-Path $resolvedReportDir "pdf-regression-test.log"
$unicodeFontLog = Join-Path $resolvedReportDir "unicode-font.log"
$attemptStart = Get-AttemptStart -ReportPath $resolvedReportDir -ExplicitStart $AttemptStartedAfter

$manifest = Read-JsonFile -Path $resolvedManifestPath
$manifestSamples = @(Get-JsonArray -Object $manifest -Name "samples")
$cjkManifestCount = @($manifestSamples | Where-Object {
        $property = $_.PSObject.Properties["expect_cjk"]
        $null -ne $property -and $property.Value -eq $true
    }).Count
$visualBaselineManifestCount = @($manifestSamples | Where-Object {
        $property = $_.PSObject.Properties["expect_visual_baseline"]
        $null -ne $property -and $property.Value -eq $true
    }).Count
$expectedVisualBaselineSampleNames = @(
    $manifestSamples |
        Where-Object {
            $property = $_.PSObject.Properties["expect_visual_baseline"]
            $null -ne $property -and $property.Value -eq $true
        } |
        ForEach-Object { [string]$_.id }
    "cli-font-map-source"
    "cli-cjk-font-source"
)
$expectedRenderedVisualCount = if ($expectedVisualBaselineSampleNames.Count -gt 0) { $expectedVisualBaselineSampleNames.Count } else { 0 }

$pdfCliExport = Get-CtestLogEvidence -Path $pdfCliExportLog
$pdfRegression = Get-CtestLogEvidence -Path $pdfRegressionLog
$unicodeFont = Get-CtestLogEvidence -Path $unicodeFontLog
$cjkCopySearch = Get-CjkCopySearchEvidence -Path $cjkCopySearchDir -ExpectedCount $cjkManifestCount -AttemptStart $attemptStart
$visualBaseline = Get-VisualBaselineEvidence `
    -Path $baselineDir `
    -ExpectedCount $expectedRenderedVisualCount `
    -AttemptStart $attemptStart `
    -ExpectedSampleNames $expectedVisualBaselineSampleNames
$contactSheet = Get-FileEvidence -Path $aggregateContactSheet -AttemptStart $attemptStart

$fullSummary = Read-JsonFile -Path $summaryJson
$fullSummaryItem = if (Test-Path -LiteralPath $summaryJson) { Get-Item -LiteralPath $summaryJson } else { $null }
$fullSummaryVerdict = Get-JsonString -Object $fullSummary -Name "verdict"
$fullSummaryFinalizeOnly = [bool](Get-JsonProperty -Object $fullSummary -Name "finalize_only")
$fullSummaryDetailPayloadIncluded = [bool](Get-JsonProperty -Object $fullSummary -Name "summary_detail_payload_included")
$fullSummaryDetailStatus = Get-JsonString -Object $fullSummary -Name "summary_detail_status"
$latestAttemptEvidence = @(
    $pdfCliExportLog,
    $pdfRegressionLog,
    $unicodeFontLog
) | Where-Object { Test-Path -LiteralPath $_ } | ForEach-Object { (Get-Item -LiteralPath $_).LastWriteTime } | Sort-Object -Descending | Select-Object -First 1
$fullSummaryFresh = $false
if ($null -ne $fullSummaryItem -and $latestAttemptEvidence) {
    $fullSummaryFresh = $fullSummaryItem.LastWriteTime -ge $latestAttemptEvidence
}

$freshNonFinalizeFullPass = $fullSummaryFresh -and -not $fullSummaryFinalizeOnly -and $fullSummaryVerdict -eq "pass"
$stageEvidence = @(
    [ordered]@{ id = "pdf_cli_export"; status = $pdfCliExport.status; verdict = $pdfCliExport.verdict }
    [ordered]@{ id = "pdf_regression"; status = $pdfRegression.status; verdict = $pdfRegression.verdict }
    [ordered]@{ id = "unicode_font_visual"; status = $unicodeFont.status; verdict = $unicodeFont.verdict }
    [ordered]@{ id = "cjk_copy_search"; status = $cjkCopySearch.status; verdict = $cjkCopySearch.verdict }
    [ordered]@{ id = "visual_baseline_render"; status = $visualBaseline.status; verdict = $visualBaseline.verdict }
    [ordered]@{ id = "aggregate_contact_sheet"; status = $contactSheet.status; verdict = if ($contactSheet.status -eq "pass") { "pass" } else { "not_complete" } }
)

$passedStageCount = @($stageEvidence | Where-Object { $_.verdict -eq "pass" }).Count
$failedStageCount = @($stageEvidence | Where-Object { $_.verdict -eq "fail" }).Count
$incompleteStageCount = @($stageEvidence | Where-Object { $_.verdict -notin @("pass", "fail") }).Count
$completedStages = @($stageEvidence | Where-Object { $_.verdict -eq "pass" } | ForEach-Object { $_.id })
$freshNonFinalizeFullPass = $freshNonFinalizeFullPass -and $failedStageCount -eq 0 -and $incompleteStageCount -eq 0

$status = if ($freshNonFinalizeFullPass) {
    "pass"
} elseif ($failedStageCount -gt 0) {
    "fail"
} elseif ($passedStageCount -gt 0) {
    "partial"
} else {
    "not_available"
}
$verdict = if ($status -eq "pass") { "pass" } elseif ($status -eq "fail") { "fail" } else { "not_complete" }
$fullVisualGateStatus = if ($freshNonFinalizeFullPass) { "pass" } elseif ($status -eq "fail") { "fail" } else { "not_complete" }
$outerGuardStatus = if ($OuterGuardTimedOut) {
    "timed_out"
} elseif ($freshNonFinalizeFullPass) {
    "completed"
} elseif ($status -eq "not_available") {
    "not_available"
} else {
    "not_timed_out"
}
$outputDirDisplay = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $outputDir
$resumeSliceCommandTemplate = if ([bool]$visualBaseline.resume_needed) {
    "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir <build-dir> -OutputDir $outputDirDisplay -VisualBaselineSliceOnly -VisualBaselineOffset $($visualBaseline.resume_slice_offset) -VisualBaselineLimit $($visualBaseline.resume_slice_limit) -SkipPreflight"
} else {
    ""
}

$summary = [ordered]@{
    schema = "featherdoc.pdf_visual_gate_attempt_summary.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    verdict = $verdict
    full_visual_gate_status = $fullVisualGateStatus
    evidence_scope = "bounded_attempt_auxiliary_only"
    boundary = "attempt_summary_does_not_replace_full_visual_gate_verdict"
    repo_root = $resolvedRepoRoot
    report_dir = $resolvedReportDir
    report_dir_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $resolvedReportDir
    summary_json = $summaryJson
    summary_json_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $summaryJson
    full_summary_verdict = $fullSummaryVerdict
    full_summary_finalize_only = $fullSummaryFinalizeOnly
    full_summary_fresh_for_attempt = $fullSummaryFresh
    full_summary_detail_payload_included = $fullSummaryDetailPayloadIncluded
    full_summary_detail_status = $fullSummaryDetailStatus
    attempt_started_after = if ($null -ne $attemptStart) { $attemptStart.ToString("s") } else { "" }
    outer_guard_status = $outerGuardStatus
    outer_guard_timed_out = [bool]$OuterGuardTimedOut
    outer_guard_timeout_seconds = $OuterGuardTimeoutSeconds
    stage_count = $stageEvidence.Count
    passed_stage_count = $passedStageCount
    failed_stage_count = $failedStageCount
    incomplete_stage_count = $incompleteStageCount
    completed_stages = @($completedStages)
    cjk_manifest_count = $cjkManifestCount
    visual_baseline_manifest_count = $visualBaselineManifestCount
    expected_visual_render_count = $expectedRenderedVisualCount
    pdf_cli_export_status = $pdfCliExport.status
    pdf_cli_export_selected_test_count = $pdfCliExport.selected_test_count
    pdf_cli_export_failed_test_count = $pdfCliExport.failed_test_count
    pdf_regression_status = $pdfRegression.status
    pdf_regression_selected_test_count = $pdfRegression.selected_test_count
    pdf_regression_passed_test_count = $pdfRegression.passed_test_count
    pdf_regression_failed_test_count = $pdfRegression.failed_test_count
    pdf_regression_skipped_test_count = $pdfRegression.skipped_test_count
    unicode_font_status = $unicodeFont.status
    unicode_font_selected_test_count = $unicodeFont.selected_test_count
    unicode_font_failed_test_count = $unicodeFont.failed_test_count
    cjk_copy_search_status = $cjkCopySearch.status
    cjk_copy_search_count = $cjkCopySearch.summary_count
    cjk_copy_search_fresh_count = $cjkCopySearch.fresh_summary_count
    cjk_copy_search_missing_text_count = $cjkCopySearch.missing_text_count
    visual_baseline_render_status = $visualBaseline.status
    visual_baseline_rendered_count = $visualBaseline.rendered_summary_count
    visual_baseline_fresh_rendered_count = $visualBaseline.fresh_rendered_summary_count
    visual_baseline_expected_sample_count = $visualBaseline.expected_sample_count
    visual_baseline_expected_sample_names = @($visualBaseline.expected_sample_names)
    visual_baseline_rendered_sample_names = @($visualBaseline.rendered_sample_names)
    visual_baseline_fresh_rendered_sample_names = @($visualBaseline.fresh_rendered_sample_names)
    visual_baseline_stale_rendered_sample_names = @($visualBaseline.stale_rendered_sample_names)
    visual_baseline_missing_sample_count = $visualBaseline.missing_sample_count
    visual_baseline_missing_sample_names = @($visualBaseline.missing_sample_names)
    visual_baseline_fresh_missing_sample_count = $visualBaseline.fresh_missing_sample_count
    visual_baseline_fresh_missing_sample_names = @($visualBaseline.fresh_missing_sample_names)
    visual_baseline_resume_needed = $visualBaseline.resume_needed
    visual_baseline_resume_slice_offset = $visualBaseline.resume_slice_offset
    visual_baseline_resume_slice_limit = $visualBaseline.resume_slice_limit
    visual_baseline_resume_slice_command_template = $resumeSliceCommandTemplate
    visual_baseline_unreadable_summary_count = $visualBaseline.unreadable_summary_count
    visual_baseline_missing_pdf_count = $visualBaseline.missing_pdf_count
    visual_baseline_pdf_total_bytes = $visualBaseline.pdf_total_bytes
    visual_baseline_png_page_count = $visualBaseline.png_page_count
    visual_baseline_missing_png_page_count = $visualBaseline.missing_png_page_count
    visual_baseline_png_total_bytes = $visualBaseline.png_total_bytes
    visual_baseline_unreadable_png_dimension_count = $visualBaseline.unreadable_png_dimension_count
    visual_baseline_png_dimension_summary = @($visualBaseline.png_dimension_summary)
    visual_baseline_sample_artifacts = @($visualBaseline.sample_artifacts)
    aggregate_contact_sheet_status = $contactSheet.status
    aggregate_contact_sheet = $aggregateContactSheet
    aggregate_contact_sheet_display = Get-RepoRelativePath -Root $resolvedRepoRoot -Path $aggregateContactSheet
    aggregate_contact_sheet_bytes = $contactSheet.bytes
    stages = @($stageEvidence)
    logs = [ordered]@{
        pdf_cli_export = $pdfCliExportLog
        pdf_regression = $pdfRegressionLog
        unicode_font = $unicodeFontLog
        cjk_copy_search = $cjkCopySearchDir
    }
}

New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($resolvedOutputJson)) -Force | Out-Null
($summary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
Write-Host "PDF visual gate attempt summary written to $resolvedOutputJson"
