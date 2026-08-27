param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $RepoRoot "build\pdf_visual_gate_attempt_summary_test"
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-TextFile {
    param([string]$Path, [string]$Text)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    $Text | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-BinaryFile {
    param([string]$Path, [byte[]]$Bytes)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    [System.IO.File]::WriteAllBytes($Path, $Bytes)
}

function Write-FakePngHeader {
    param(
        [string]$Path,
        [int]$Width,
        [int]$Height
    )

    $bytes = [byte[]]::new(24)
    [byte[]](137, 80, 78, 71, 13, 10, 26, 10) | ForEach-Object -Begin { $index = 0 } -Process {
        $bytes[$index] = $_
        $index++
    }
    [byte[]](0, 0, 0, 13, 73, 72, 68, 82) | ForEach-Object -Begin { $index = 8 } -Process {
        $bytes[$index] = $_
        $index++
    }
    $bytes[16] = [byte](($Width -shr 24) -band 0xFF)
    $bytes[17] = [byte](($Width -shr 16) -band 0xFF)
    $bytes[18] = [byte](($Width -shr 8) -band 0xFF)
    $bytes[19] = [byte]($Width -band 0xFF)
    $bytes[20] = [byte](($Height -shr 24) -band 0xFF)
    $bytes[21] = [byte](($Height -shr 16) -band 0xFF)
    $bytes[22] = [byte](($Height -shr 8) -band 0xFF)
    $bytes[23] = [byte]($Height -band 0xFF)
    Write-BinaryFile -Path $Path -Bytes $bytes
}

function Invoke-AttemptSummaryScript {
    param(
        [string]$ReportDir,
        [string]$OutputJson,
        [switch]$OuterGuardTimedOut,
        [int]$OuterGuardTimeoutSeconds = 0
    )

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Unable to resolve PowerShell for nested script invocation."
        }
        $powerShellPath = $powerShellCommand.Source
    }

    $scriptPath = Join-Path $resolvedRepoRoot "scripts\write_pdf_visual_gate_attempt_summary.ps1"
    $arguments = @(
        "-NoProfile",
        "-NonInteractive",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $scriptPath,
        "-RepoRoot",
        $resolvedRepoRoot,
        "-ReportDir",
        $ReportDir,
        "-OutputJson",
        $OutputJson
    )
    if ($OuterGuardTimedOut) {
        $arguments += "-OuterGuardTimedOut"
    }
    if ($OuterGuardTimeoutSeconds -gt 0) {
        $arguments += @("-OuterGuardTimeoutSeconds", [string]$OuterGuardTimeoutSeconds)
    }

    $output = @(& $powerShellPath @arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    if ($exitCode -ne 0) {
        throw "write_pdf_visual_gate_attempt_summary.ps1 failed: $($output -join [Environment]::NewLine)"
    }
}

function New-AttemptFixture {
    param(
        [string]$Root,
        [bool]$FreshFullPass
    )

    $reportDir = Join-Path $Root "output\pdf-visual-release-gate-current\report"
    $baselineDir = Join-Path $Root "output\pdf-visual-release-gate-current\baseline"
    $cjkDir = Join-Path $reportDir "cjk-copy-search"
    New-Item -ItemType Directory -Path $reportDir, $baselineDir, $cjkDir -Force | Out-Null

    $attemptStart = Get-Date "2026-05-26T12:50:00"
    $oldTime = Get-Date "2026-05-26T12:00:00"
    $freshTime = Get-Date "2026-05-26T12:55:00"
    $manifest = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $resolvedRepoRoot "test\pdf_regression_manifest.json") | ConvertFrom-Json
    $visualSampleNames = @(
        $manifest.samples |
            Where-Object {
                $property = $_.PSObject.Properties["expect_visual_baseline"]
                $null -ne $property -and $property.Value -eq $true
            } |
            ForEach-Object { [string]$_.id }
        "cli-font-map-source"
        "cli-cjk-font-source"
    )

    Write-JsonFile -Path (Join-Path $reportDir "preflight-summary.json") -Value ([ordered]@{
        status = "ready"
    })
    (Get-Item -LiteralPath (Join-Path $reportDir "preflight-summary.json")).LastWriteTime = $attemptStart

    Write-TextFile -Path (Join-Path $reportDir "pdf-cli-export-test.log") -Text @"
100% tests passed, 0 tests failed out of 1
Total Test time (real) =  10.82 sec
"@
    (Get-Item -LiteralPath (Join-Path $reportDir "pdf-cli-export-test.log")).LastWriteTime = $freshTime

    $skippedLines = @(
        "1/91 Test #39: pdf_regression_document-table-font-matrix-text ........***Skipped 0.01 sec",
        "2/91 Test #68: pdf_regression_document-table-cjk-wrap-flow-text ......***Skipped 0.01 sec",
        "3/91 Test #69: pdf_regression_document-table-cjk-vertical-merged-cant-split-text ......***Skipped 0.01 sec",
        "4/91 Test #70: pdf_regression_document-table-cjk-merged-repeat-text ..***Skipped 0.01 sec",
        "5/91 Test #87: pdf_regression_document-cjk-style-overlay-page-flow-text ......***Skipped 0.01 sec",
        "6/91 Test #88: pdf_regression_document-cjk-complex-layout-text .......***Skipped 0.01 sec",
        "7/91 Test #99: pdf_regression_document-cjk-table-wrap-page-flow-text .***Skipped 0.01 sec"
    ) -join [Environment]::NewLine
    Write-TextFile -Path (Join-Path $reportDir "pdf-regression-test.log") -Text @"
$skippedLines
100% tests passed, 0 tests failed out of 91
Total Test time (real) =  17.16 sec
"@
    (Get-Item -LiteralPath (Join-Path $reportDir "pdf-regression-test.log")).LastWriteTime = $freshTime

    Write-TextFile -Path (Join-Path $reportDir "unicode-font.log") -Text @"
100% tests passed, 0 tests failed out of 1
Visual summary written to output/pdf-visual-release-gate-current/unicode-font/report/summary.json
"@
    (Get-Item -LiteralPath (Join-Path $reportDir "unicode-font.log")).LastWriteTime = $freshTime

    foreach ($index in 1..43) {
        $summaryPath = Join-Path $cjkDir ("sample-{0:D2}-summary.json" -f $index)
        Write-JsonFile -Path $summaryPath -Value ([ordered]@{
            matched_text = @("示例")
            missing_text = @()
            page_count = 1
        })
        (Get-Item -LiteralPath $summaryPath).LastWriteTime = $freshTime
    }

    for ($index = 0; $index -lt $visualSampleNames.Count; $index++) {
        $sampleDir = Join-Path $baselineDir $visualSampleNames[$index]
        $pagesDir = Join-Path $sampleDir "pages"
        $samplePdfPath = Join-Path $Root ("pdfs\{0}.pdf" -f $visualSampleNames[$index])
        $pagePath = Join-Path $pagesDir "page-01.png"
        Write-BinaryFile -Path $samplePdfPath -Bytes ([byte[]](37, 80, 68, 70, 45, 49, 46, 55))
        Write-FakePngHeader -Path $pagePath -Width 320 -Height 480
        $summaryPath = Join-Path $sampleDir "summary.json"
        Write-JsonFile -Path $summaryPath -Value ([ordered]@{
            input_pdf = $samplePdfPath
            page_count = 1
            pages = @($pagePath)
            contact_sheet = ""
            contact_sheet_skipped = $true
        })
        $timestamp = if ($FreshFullPass -or $index -lt 22) { $freshTime } else { $oldTime }
        (Get-Item -LiteralPath $summaryPath).LastWriteTime = $timestamp
        (Get-Item -LiteralPath $samplePdfPath).LastWriteTime = $timestamp
        (Get-Item -LiteralPath $pagePath).LastWriteTime = $timestamp
    }

    $contactSheetPath = Join-Path $reportDir "aggregate-contact-sheet.png"
    [System.IO.File]::WriteAllBytes($contactSheetPath, [byte[]](1, 2, 3, 4))
    (Get-Item -LiteralPath $contactSheetPath).LastWriteTime = if ($FreshFullPass) { $freshTime } else { $oldTime }

    Write-JsonFile -Path (Join-Path $reportDir "summary.json") -Value ([ordered]@{
        status = "pass"
        verdict = "pass"
        finalize_only = -not $FreshFullPass
        aggregate_contact_sheet = $contactSheetPath
        summary_detail_payload_included = $FreshFullPass
        summary_detail_status = if ($FreshFullPass) { "complete" } else { "core_pass_written_before_detail_payload" }
        marker = "pdf_visual_gate_core_pass_summary_trace"
    })
    (Get-Item -LiteralPath (Join-Path $reportDir "summary.json")).LastWriteTime = if ($FreshFullPass) { $freshTime.AddMinutes(1) } else { $oldTime }

    return [pscustomobject]@{
        ReportDir = $reportDir
        BaselineDir = $baselineDir
        OutputJson = Join-Path $reportDir "attempt-summary.json"
    }
}

$resolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$partialFixture = New-AttemptFixture -Root (Join-Path $resolvedWorkingDir "partial") -FreshFullPass $false
Invoke-AttemptSummaryScript -ReportDir $partialFixture.ReportDir -OutputJson $partialFixture.OutputJson `
    -OuterGuardTimedOut `
    -OuterGuardTimeoutSeconds 60
$partialSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $partialFixture.OutputJson | ConvertFrom-Json

Assert-Equal -Actual $partialSummary.status -Expected "partial" `
    -Message "Partial attempt should not be promoted to pass."
Assert-Equal -Actual $partialSummary.verdict -Expected "not_complete" `
    -Message "Partial attempt should carry an explicit not_complete verdict."
Assert-Equal -Actual $partialSummary.full_visual_gate_status -Expected "not_complete" `
    -Message "Partial attempt should not set full_visual_gate_status to pass."
Assert-Equal -Actual $partialSummary.pdf_cli_export_status -Expected "pass" `
    -Message "Attempt summary should preserve pdf_cli_export pass evidence."
Assert-Equal -Actual $partialSummary.pdf_regression_status -Expected "pass" `
    -Message "Attempt summary should preserve pdf_regression pass evidence."
Assert-Equal -Actual ([int]$partialSummary.pdf_regression_selected_test_count) -Expected 91 `
    -Message "Attempt summary should preserve the pdf_regression selected test count."
Assert-Equal -Actual ([int]$partialSummary.pdf_regression_failed_test_count) -Expected 0 `
    -Message "Attempt summary should preserve the pdf_regression failed test count."
Assert-Equal -Actual ([int]$partialSummary.pdf_regression_skipped_test_count) -Expected 7 `
    -Message "Attempt summary should preserve skipped boundary cases."
Assert-Equal -Actual ([int]$partialSummary.cjk_copy_search_count) -Expected 43 `
    -Message "Attempt summary should preserve CJK copy/search evidence."
Assert-Equal -Actual $partialSummary.visual_baseline_render_status -Expected "partial" `
    -Message "Attempt summary should identify incomplete visual baseline rendering."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_fresh_rendered_count) -Expected 22 `
    -Message "Attempt summary should count only fresh rendered baselines for the attempt."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_expected_sample_count) -Expected 44 `
    -Message "Attempt summary should expose the expected visual baseline sample count."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_missing_pdf_count) -Expected 0 `
    -Message "Attempt summary should verify that every rendered visual baseline has a source PDF."
Assert-Equal -Actual ([int64]$partialSummary.visual_baseline_pdf_total_bytes) -Expected 352 `
    -Message "Attempt summary should aggregate visual baseline PDF bytes."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_png_page_count) -Expected 44 `
    -Message "Attempt summary should aggregate rendered PNG page counts."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_missing_png_page_count) -Expected 0 `
    -Message "Attempt summary should report missing rendered PNG pages."
Assert-Equal -Actual ([int64]$partialSummary.visual_baseline_png_total_bytes) -Expected 1056 `
    -Message "Attempt summary should aggregate rendered PNG bytes."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_unreadable_png_dimension_count) -Expected 0 `
    -Message "Attempt summary should report unreadable PNG dimensions."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_png_dimension_summary[0].width) -Expected 320 `
    -Message "Attempt summary should expose rendered PNG width evidence."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_png_dimension_summary[0].height) -Expected 480 `
    -Message "Attempt summary should expose rendered PNG height evidence."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_png_dimension_summary[0].count) -Expected 44 `
    -Message "Attempt summary should aggregate identical rendered PNG dimensions."
Assert-Equal -Actual ([int](@($partialSummary.visual_baseline_sample_artifacts)).Count) -Expected 44 `
    -Message "Attempt summary should expose per-sample visual baseline artifact evidence."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_fresh_missing_sample_count) -Expected 22 `
    -Message "Attempt summary should expose how many baselines remain stale for the current attempt."
Assert-Equal -Actual ([bool]$partialSummary.visual_baseline_resume_needed) -Expected $true `
    -Message "Partial visual baseline evidence should mark that a slice resume is needed."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_resume_slice_offset) -Expected 22 `
    -Message "Attempt summary should point to the first non-fresh baseline slice offset."
Assert-Equal -Actual ([int]$partialSummary.visual_baseline_resume_slice_limit) -Expected 22 `
    -Message "Attempt summary should expose the remaining tail slice size."
Assert-True -Condition ((@($partialSummary.visual_baseline_fresh_missing_sample_names)).Count -eq 22) `
    -Message "Attempt summary should list the current-attempt fresh-missing baseline names."
Assert-True -Condition ([string]$partialSummary.visual_baseline_resume_slice_command_template -match "VisualBaselineOffset 22") `
    -Message "Attempt summary should include a resume command template with the next slice offset."
Assert-True -Condition ([string]$partialSummary.visual_baseline_resume_slice_command_template -match "VisualBaselineLimit 22") `
    -Message "Attempt summary should include a resume command template with the remaining slice limit."
Assert-Equal -Actual $partialSummary.aggregate_contact_sheet_status -Expected "stale" `
    -Message "Attempt summary should not treat stale contact sheets as fresh attempt evidence."
Assert-Equal -Actual $partialSummary.evidence_scope -Expected "bounded_attempt_auxiliary_only" `
    -Message "Attempt summary should declare its auxiliary scope."
Assert-Equal -Actual $partialSummary.outer_guard_status -Expected "timed_out" `
    -Message "Timed-out guarded attempts should preserve the outer guard status."
Assert-True -Condition ([bool]$partialSummary.outer_guard_timed_out) `
    -Message "Timed-out guarded attempts should preserve the outer guard timed-out flag."
Assert-Equal -Actual ([int]$partialSummary.outer_guard_timeout_seconds) -Expected 60 `
    -Message "Timed-out guarded attempts should preserve the outer guard timeout seconds."

$passFixture = New-AttemptFixture -Root (Join-Path $resolvedWorkingDir "pass") -FreshFullPass $true
Invoke-AttemptSummaryScript -ReportDir $passFixture.ReportDir -OutputJson $passFixture.OutputJson
$passSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passFixture.OutputJson | ConvertFrom-Json

Assert-Equal -Actual $passSummary.status -Expected "pass" `
    -Message "Fresh non-finalize full summary should allow a pass status."
Assert-Equal -Actual $passSummary.verdict -Expected "pass" `
    -Message "Fresh non-finalize full summary should allow a pass verdict."
Assert-Equal -Actual $passSummary.full_visual_gate_status -Expected "pass" `
    -Message "Fresh non-finalize full summary should preserve full_visual_gate_status=pass."
Assert-True -Condition ([bool]$passSummary.full_summary_fresh_for_attempt) `
    -Message "Fresh full summary should be marked fresh for the attempt."
Assert-Equal -Actual ([string]$passSummary.full_summary_detail_status) -Expected "complete" `
    -Message "Attempt summary should preserve the fresh full summary detail status."
Assert-True -Condition ([bool]$passSummary.full_summary_detail_payload_included) `
    -Message "Attempt summary should preserve that the full detail payload was included when present."
Assert-Equal -Actual $passSummary.outer_guard_status -Expected "completed" `
    -Message "Fresh non-finalize full pass should preserve a completed outer guard status."
Assert-Equal -Actual ([bool]$passSummary.outer_guard_timed_out) -Expected $false `
    -Message "Fresh non-finalize full pass should not mark the outer guard as timed out."
Assert-Equal -Actual ([int]$passSummary.visual_baseline_fresh_missing_sample_count) -Expected 0 `
    -Message "Fresh non-finalize full pass should not report fresh-missing baselines."
Assert-Equal -Actual ([bool]$passSummary.visual_baseline_resume_needed) -Expected $false `
    -Message "Fresh non-finalize full pass should not require slice resume."

$missingArtifactFixture = New-AttemptFixture -Root (Join-Path $resolvedWorkingDir "missing-artifact") -FreshFullPass $true
$missingPagePath = @(
    Get-ChildItem -LiteralPath $missingArtifactFixture.BaselineDir -Filter "page-01.png" -File -Recurse |
        Sort-Object FullName |
        Select-Object -First 1
)[0].FullName
Remove-Item -LiteralPath $missingPagePath -Force
Invoke-AttemptSummaryScript -ReportDir $missingArtifactFixture.ReportDir -OutputJson $missingArtifactFixture.OutputJson
$missingArtifactSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $missingArtifactFixture.OutputJson | ConvertFrom-Json

Assert-Equal -Actual $missingArtifactSummary.status -Expected "fail" `
    -Message "Missing rendered PNG evidence should fail the attempt summary even when a full summary exists."
Assert-Equal -Actual $missingArtifactSummary.verdict -Expected "fail" `
    -Message "Missing rendered PNG evidence should produce a fail verdict."
Assert-Equal -Actual $missingArtifactSummary.full_visual_gate_status -Expected "fail" `
    -Message "Missing rendered PNG evidence should not preserve full_visual_gate_status=pass."
Assert-Equal -Actual $missingArtifactSummary.visual_baseline_render_status -Expected "fail" `
    -Message "Missing rendered PNG evidence should fail the visual baseline render stage."
Assert-Equal -Actual ([int]$missingArtifactSummary.visual_baseline_missing_png_page_count) -Expected 1 `
    -Message "Attempt summary should count missing rendered PNG pages."
Assert-Equal -Actual ([int](@($missingArtifactSummary.visual_baseline_sample_artifacts | Where-Object { [string]$_.status -eq "missing_png" })).Count) -Expected 1 `
    -Message "Attempt summary should identify the sample with missing rendered PNG evidence."
