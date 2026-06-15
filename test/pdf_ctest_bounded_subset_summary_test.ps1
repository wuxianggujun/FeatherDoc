param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "pdf_import_diagnostics_contract_field_helpers.ps1")

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-SequenceEqual {
    param([object[]]$Actual, [object[]]$Expected, [string]$Message)

    Assert-Equal -Actual $Actual.Count -Expected $Expected.Count -Message "$Message Count mismatch."
    for ($index = 0; $index -lt $Expected.Count; ++$index) {
        Assert-Equal -Actual ([string]$Actual[$index]) -Expected ([string]$Expected[$index]) `
            -Message "$Message Index $index mismatch."
    }
}

function Invoke-PowerShellScript {
    param([string]$ScriptPath, [string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellPath = (Get-Command powershell.exe -ErrorAction Stop).Source
    }

    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    throw "WorkingDir is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\run_pdf_ctest_bounded_subset.ps1"
$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "Bounded PDF CTest subset script has parse errors."
}

$buildDir = Join-Path $resolvedWorkingDir "build"
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

$fakeCtest = Join-Path $resolvedWorkingDir "fake-ctest-smoke-import.ps1"
@'
$tests = @(
    "pdf_document_generator_probe",
    "pdf_font_resolver",
    "pdf_text_metrics",
    "pdf_text_shaper",
    "pdf_document_adapter_font",
    "pdf_cli_export",
    "pdf_cli_import",
    "pdf_import_structure",
    "pdf_import_failure",
    "pdf_import_table_heuristic"
)

if ($args -contains "-N") {
    Write-Output "Test project fixture"
    for ($index = 0; $index -lt $tests.Count; ++$index) {
        Write-Output ("  Test #{0}: {1}" -f ($index + 1), $tests[$index])
    }
    exit 0
}

$timeoutIndex = [Array]::IndexOf($args, "--timeout")
if ($timeoutIndex -lt 0 -or $timeoutIndex + 1 -ge $args.Count -or $args[$timeoutIndex + 1] -ne "120") {
    Write-Error "Expected smoke-import bounded CTest timeout 120, got args: $($args -join ' ')"
    exit 2
}

Write-Output "Test project fixture"
for ($index = 0; $index -lt $tests.Count; ++$index) {
    Write-Output ("{0}/{1} Test #{2}: {3} ........................   Passed    0.01 sec" -f ($index + 1), $tests.Count, ($index + 1), $tests[$index])
}
Write-Output "100% tests passed, 0 tests failed out of 10"
Write-Output "Total Test time (real) = 0.10 sec"
exit 0
'@ | Set-Content -LiteralPath $fakeCtest -Encoding UTF8

$summaryPath = Join-Path $resolvedWorkingDir "smoke-import-summary.json"
$result = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-Subset", "smoke-import",
    "-BuildDir", $buildDir,
    "-OutputJson", $summaryPath,
    "-CtestExecutable", $fakeCtest
)
Assert-Equal -Actual $result.ExitCode -Expected 0 `
    -Message "Passing fake smoke-import subset should complete successfully. Output: $($result.Text)"

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.status) -Expected "pass" `
    -Message "Smoke-import summary should report pass status."
Assert-Equal -Actual ([string]$summary.verdict) -Expected "pass" `
    -Message "Smoke-import summary should report pass verdict."
Assert-Equal -Actual ([string]$summary.subset) -Expected "smoke-import" `
    -Message "Smoke-import summary should preserve subset name."
Assert-Equal -Actual ([string]$summary.import_visual_gate_scope) -Expected "bounded_smoke_import_preflight" `
    -Message "Smoke-import summary should expose bounded import visual gate scope."
Assert-Equal -Actual ([string]$summary.import_visual_gate_boundary) -Expected "bounded_smoke_import_preflight_does_not_replace_full_visual_gate_verdict" `
    -Message "Smoke-import summary should expose the full visual gate boundary."
Assert-Equal -Actual ([string]$summary.import_visual_artifact_policy) -Expected "does_not_generate_or_commit_output_visual_artifacts" `
    -Message "Smoke-import summary should expose the visual artifact policy."
Assert-Equal -Actual ([int]$summary.selected_test_count) -Expected 10 `
    -Message "Smoke-import summary should preserve selected test count."
Assert-Equal -Actual ([int]$summary.skipped_test_count) -Expected 0 `
    -Message "Smoke-import summary should preserve zero skipped tests."
Assert-Equal -Actual ([int]$summary.exit_code) -Expected 0 `
    -Message "Smoke-import summary should preserve ctest exit code."
Assert-Equal -Actual ([int]$summary.ctest_timeout_seconds) -Expected 120 `
    -Message "Smoke-import summary should preserve the bounded ctest timeout."

Assert-SequenceEqual `
    -Actual @($summary.import_diagnostics_contract_tests | ForEach-Object { [string]$_ }) `
    -Expected @("pdf_cli_import", "pdf_import_failure", "pdf_import_table_heuristic") `
    -Message "Smoke-import summary should preserve import diagnostics contract tests."
Assert-SequenceEqual `
    -Actual @($summary.import_diagnostics_contract_fields | ForEach-Object { [string]$_ }) `
    -Expected @(Get-PdfImportDiagnosticsContractFields) `
    -Message "Smoke-import summary should preserve import diagnostics contract fields."
Assert-SequenceEqual `
    -Actual @($summary.import_negative_boundary_contract_cases | ForEach-Object { [string]$_ }) `
    -Expected @(
        "short_label_prose_remains_paragraphs",
        "invoice_summary_form_remains_paragraphs"
    ) `
    -Message "Smoke-import summary should preserve import negative boundary contract cases."
Assert-SequenceEqual `
    -Actual @($summary.selected_tests | ForEach-Object { [string]$_ }) `
    -Expected @(
        "pdf_document_generator_probe",
        "pdf_font_resolver",
        "pdf_text_metrics",
        "pdf_text_shaper",
        "pdf_document_adapter_font",
        "pdf_cli_export",
        "pdf_cli_import",
        "pdf_import_structure",
        "pdf_import_failure",
        "pdf_import_table_heuristic"
    ) `
    -Message "Smoke-import summary should preserve selected smoke/import test order."

$fakeContractStaticCtest = Join-Path $resolvedWorkingDir "fake-ctest-contract-static.ps1"
@'
$tests = @(
    "pdf_import_docs_contract",
    "pdf_ctest_timeout_contract",
    "pdf_ctest_label_contract",
    "pdf_bidi_line_layout_static_contract",
    "pdf_document_style_gallery_contract",
    "pdf_document_font_matrix_contract",
    "pdf_document_table_font_matrix_contract",
    "pdf_cjk_copy_search_matrix_contract",
    "pdf_cjk_font_embed_matrix_contract",
    "pdf_cjk_anchor_font_matrix_boundary_contract"
)

if ($args -contains "-N") {
    Write-Output "Test project fixture"
    for ($index = 0; $index -lt $tests.Count; ++$index) {
        Write-Output ("  Test #{0}: {1}" -f ($index + 1), $tests[$index])
    }
    exit 0
}

$timeoutIndex = [Array]::IndexOf($args, "--timeout")
if ($timeoutIndex -lt 0 -or $timeoutIndex + 1 -ge $args.Count -or $args[$timeoutIndex + 1] -ne "60") {
    Write-Error "Expected contract-static bounded CTest timeout 60, got args: $($args -join ' ')"
    exit 2
}

Write-Output "Test project fixture"
for ($index = 0; $index -lt $tests.Count; ++$index) {
    Write-Output ("{0}/{1} Test #{2}: {3} ........................   Passed    0.01 sec" -f ($index + 1), $tests.Count, ($index + 1), $tests[$index])
}
Write-Output "100% tests passed, 0 tests failed out of 10"
Write-Output "Total Test time (real) = 0.10 sec"
exit 0
'@ | Set-Content -LiteralPath $fakeContractStaticCtest -Encoding UTF8

$contractStaticSummaryPath = Join-Path $resolvedWorkingDir "contract-static-summary.json"
$contractStaticResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-Subset", "contract-static",
    "-BuildDir", $buildDir,
    "-OutputJson", $contractStaticSummaryPath,
    "-CtestExecutable", $fakeContractStaticCtest
)
Assert-Equal -Actual $contractStaticResult.ExitCode -Expected 0 `
    -Message "Passing fake contract-static subset should complete successfully. Output: $($contractStaticResult.Text)"

$contractStaticSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $contractStaticSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$contractStaticSummary.status) -Expected "pass" `
    -Message "Contract-static summary should report pass status."
Assert-Equal -Actual ([string]$contractStaticSummary.verdict) -Expected "pass" `
    -Message "Contract-static summary should report pass verdict."
Assert-Equal -Actual ([string]$contractStaticSummary.subset) -Expected "contract-static" `
    -Message "Contract-static summary should preserve subset name."
Assert-Equal -Actual ([int]$contractStaticSummary.selected_test_count) -Expected 10 `
    -Message "Contract-static summary should preserve selected test count."
Assert-Equal -Actual ([int]$contractStaticSummary.ctest_timeout_seconds) -Expected 60 `
    -Message "Contract-static summary should preserve the default bounded ctest timeout."
Assert-SequenceEqual `
    -Actual @($contractStaticSummary.selected_tests | ForEach-Object { [string]$_ }) `
    -Expected @(
        "pdf_import_docs_contract",
        "pdf_ctest_timeout_contract",
        "pdf_ctest_label_contract",
        "pdf_bidi_line_layout_static_contract",
        "pdf_document_style_gallery_contract",
        "pdf_document_font_matrix_contract",
        "pdf_document_table_font_matrix_contract",
        "pdf_cjk_copy_search_matrix_contract",
        "pdf_cjk_font_embed_matrix_contract",
        "pdf_cjk_anchor_font_matrix_boundary_contract"
    ) `
    -Message "Contract-static summary should preserve selected static contract test order."

Write-Host "PDF bounded CTest subset summary contract passed."
