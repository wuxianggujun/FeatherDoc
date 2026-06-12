param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-PowerShellScript {
    param([string]$ScriptPath, [string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Unable to resolve a PowerShell executable for the nested script invocation."
        }
        $powerShellPath = $powerShellCommand.Source
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

$scriptPath = Join-Path $resolvedRepoRoot "scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1"
$scriptHelperPath = Join-Path $resolvedRepoRoot "scripts\write_pdf_visual_release_gate_preflight_governance_report_helpers.ps1"
$preflightScriptPath = Join-Path $resolvedRepoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1"
$preflightScriptHelperPath = Join-Path $resolvedRepoRoot "scripts\check_pdf_visual_release_gate_preflight_helpers.ps1"
$rollupScriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_blocker_rollup_report.ps1"

foreach ($pathToParse in @($scriptPath, $scriptHelperPath, $preflightScriptPath, $preflightScriptHelperPath)) {
    $tokens = $null
    $errors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($pathToParse, [ref]$tokens, [ref]$errors) | Out-Null
    if ($errors.Count -gt 0) {
        throw "PDF visual preflight governance report script has parse errors: $pathToParse"
    }
}

$notReadyPreflightPath = Join-Path $resolvedWorkingDir "fixtures\not-ready-preflight.json"
$readyPreflightPath = Join-Path $resolvedWorkingDir "fixtures\ready-preflight.json"
$syntheticReadyPreflightPath = Join-Path $resolvedWorkingDir "fixtures\synthetic-ready-preflight.json"
$controlledVisualSmokePath = Join-Path $resolvedWorkingDir "fixtures\controlled-visual-smoke.json"
$failedControlledVisualSmokePath = Join-Path $resolvedWorkingDir "fixtures\controlled-visual-smoke-failed.json"

Write-JsonFile -Path $notReadyPreflightPath -Value ([ordered]@{
    generated_at = "2026-05-20T00:00:00"
    status = "not_ready"
    strict = $false
    repo_root = $resolvedRepoRoot
    build_dir = Join-Path $resolvedRepoRoot ".bpdf-roundtrip-msvc"
    build_dir_source = "requested"
    requested_build_dir = Join-Path $resolvedRepoRoot ".bpdf-roundtrip-msvc"
    build_dir_auto_candidates = @(
        [ordered]@{
            relative_path = "build"
            path = Join-Path $resolvedRepoRoot "build"
            exists = $true
            cmake_cache_exists = $true
            ctest_manifest_exists = $true
            pdf_build_options_enabled = $false
            pdf_build_options = @(
                [ordered]@{
                    name = "FEATHERDOC_BUILD_PDF"
                    present = $true
                    value = "OFF"
                    enabled = $false
                },
                [ordered]@{
                    name = "FEATHERDOC_BUILD_PDF_IMPORT"
                    present = $true
                    value = "OFF"
                    enabled = $false
                }
            )
            looks_reusable = $false
        },
        [ordered]@{
            relative_path = "out\build"
            path = Join-Path $resolvedRepoRoot "out\build"
            exists = $false
            cmake_cache_exists = $false
            ctest_manifest_exists = $false
            pdf_build_options_enabled = $false
            pdf_build_options = @(
                [ordered]@{
                    name = "FEATHERDOC_BUILD_PDF"
                    present = $false
                    value = ""
                    enabled = $false
                },
                [ordered]@{
                    name = "FEATHERDOC_BUILD_PDF_IMPORT"
                    present = $false
                    value = ""
                    enabled = $false
                }
            )
            looks_reusable = $false
        }
    )
    checks = @(
        [ordered]@{
            name = "workstation_free_memory_available"
            status = "missing"
            required = $true
            message = "Free physical memory is below the required visual-gate preflight threshold."
            details = [ordered]@{
                min_free_memory_mb = 2048
                guard_skipped = $false
                guard_blocked = $true
                available = $true
                total_mb = 16384
                free_mb = 512.5
                error_message = ""
            }
        },
        [ordered]@{
            name = "build_dir_exists"
            status = "missing"
            required = $true
            message = "Build directory is missing."
            details = [ordered]@{
                cmake_cache_path = Join-Path $resolvedRepoRoot ".bpdf-roundtrip-msvc\CMakeCache.txt"
                cmake_cache_exists = $false
                ctest_manifest_path = Join-Path $resolvedRepoRoot ".bpdf-roundtrip-msvc\CTestTestfile.cmake"
                ctest_manifest_exists = $false
                entry_count = 1
                entries_preview = @(
                    [ordered]@{
                        name = "tmp"
                        type = "directory"
                        bytes = $null
                    }
                )
            }
        },
        [ordered]@{
            name = "ctest_manifest_exists"
            status = "missing"
            required = $true
            message = "CTestTestfile.cmake is missing."
        },
        [ordered]@{
            name = "cmake_cache_exists"
            status = "missing"
            required = $true
            message = "CMakeCache.txt is missing."
        },
        [ordered]@{
            name = "pdf_build_options_enabled"
            status = "missing"
            required = $true
            message = "CMakeCache.txt does not enable every PDF writer/import build option required by the full visual gate."
            details = [ordered]@{
                required_options = @(
                    "FEATHERDOC_BUILD_PDF",
                    "FEATHERDOC_BUILD_PDF_IMPORT"
                )
                missing_options = @()
                disabled_options = @(
                    "FEATHERDOC_BUILD_PDF",
                    "FEATHERDOC_BUILD_PDF_IMPORT"
                )
            }
        },
        [ordered]@{
            name = "pdf_cli_export_baseline_pdfs_exist"
            status = "missing"
            required = $true
            message = "PDF CLI export source PDFs are missing."
            details = [ordered]@{
                required_paths = @(
                    "test\pdf_cli_export\font-map-source.pdf",
                    "test\pdf_cli_export\cjk-font-source.pdf"
                )
                missing_paths = @(
                    "test\pdf_cli_export\font-map-source.pdf",
                    "test\pdf_cli_export\cjk-font-source.pdf"
                )
            }
        },
        [ordered]@{
            name = "visual_baseline_manifest_pdfs_exist"
            status = "missing"
            required = $true
            message = "PDF visual baseline manifest outputs are missing."
            details = [ordered]@{
                sample_count = 42
                missing_count = 42
                missing_paths_preview = @(
                    "test\pdf_visual_baselines\cjk\font_map_text.pdf",
                    "test\pdf_visual_baselines\layout\table_grid.pdf"
                )
            }
        },
        [ordered]@{
            name = "cjk_text_layer_manifest_pdfs_exist"
            status = "missing"
            required = $true
            message = "PDF CJK text-layer manifest outputs are missing."
            details = [ordered]@{
                sample_count = 43
                missing_count = 43
                missing_paths_preview = @(
                    "test\pdf_text_layer_cjk\copy_search\mixed_cjk_text.pdf",
                    "test\pdf_text_layer_cjk\bullets\nested_cjk_bullets.pdf"
                )
            }
        },
        [ordered]@{
            name = "render_python_reusable"
            status = "pass"
            required = $true
            message = "A reusable render Python with PIL and fitz is available."
        }
    )
    blocking_checks = @(
        "workstation_free_memory_available",
        "build_dir_exists",
        "cmake_cache_exists",
        "ctest_manifest_exists",
        "pdf_build_options_enabled",
        "pdf_cli_export_baseline_pdfs_exist",
        "visual_baseline_manifest_pdfs_exist",
        "cjk_text_layer_manifest_pdfs_exist"
    )
    blocking_summary = [ordered]@{
        required_check_count = 9
        blocking_check_count = 8
        missing_cli_pdf_count = 2
        visual_baseline_sample_count = 42
        missing_visual_baseline_pdf_count = 42
        cjk_text_layer_sample_count = 43
        missing_cjk_text_layer_pdf_count = 43
        build_dir_entry_count = 1
        ctest_required_pattern_count = 0
        free_memory_mb = 512.5
        min_free_memory_mb = 2048
        memory_guard_blocked = $true
        memory_guard_skipped = $false
        pdf_build_options_enabled = $false
        pdf_build_option_count = 2
        disabled_pdf_build_options = @(
            "FEATHERDOC_BUILD_PDF",
            "FEATHERDOC_BUILD_PDF_IMPORT"
        )
        missing_pdf_build_options = @()
        pdf_dependency_inputs_status = "not_ready"
        pdf_dependency_check_available = $true
        pdf_dependency_missing_input_count = 3
        selected_pdfium_provider = "unresolved"
        pdfio_dependency_ready = $false
        pdfium_dependency_ready = $false
        pdf_dependency_missing_inputs_preview = @(
            "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h",
            "PDFium source header: C:\repo\tmp\pdfium-workspace\pdfium\public\fpdfview.h"
        )
    }
    pdf_dependency_inputs = [ordered]@{
        available = $true
        script_path = Join-Path $resolvedRepoRoot "scripts\check_pdf_dependency_inputs.ps1"
        schema = "featherdoc.pdf_dependency_inputs_check.v1"
        status = "not_ready"
        check_exit_code = 0
        error_message = ""
        pdfio_ready = $false
        pdfium_provider = "auto"
        selected_pdfium_provider = "unresolved"
        pdfium_ready = $false
        missing_input_count = 3
        missing_inputs = @(
            "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h",
            "PDFium source header: C:\repo\tmp\pdfium-workspace\pdfium\public\fpdfview.h",
            "PDFium input: provide prebuilt library/include inputs or a source checkout with public/fpdfview.h."
        )
    }
})

Write-JsonFile -Path $readyPreflightPath -Value ([ordered]@{
    generated_at = "2026-05-20T00:00:00"
    status = "ready"
    strict = $false
    repo_root = $resolvedRepoRoot
    build_dir = Join-Path $resolvedRepoRoot ".bpdf-roundtrip-msvc"
    build_dir_source = "requested"
    requested_build_dir = Join-Path $resolvedRepoRoot ".bpdf-roundtrip-msvc"
    checks = @(
        [ordered]@{
            name = "build_dir_exists"
            status = "pass"
            required = $true
            message = "Build directory exists."
        },
        [ordered]@{
            name = "render_python_reusable"
            status = "pass"
            required = $true
            message = "A reusable render Python with PIL and fitz is available."
        }
    )
    blocking_checks = @()
    blocking_summary = [ordered]@{
        required_check_count = 2
        blocking_check_count = 0
        missing_cli_pdf_count = 0
        visual_baseline_sample_count = 0
        missing_visual_baseline_pdf_count = 0
        cjk_text_layer_sample_count = 0
        missing_cjk_text_layer_pdf_count = 0
        build_dir_entry_count = 1
        ctest_required_pattern_count = 0
        free_memory_mb = 4096.5
        min_free_memory_mb = 2048
        memory_guard_blocked = $false
        memory_guard_skipped = $false
        pdf_build_options_enabled = $true
        pdf_build_option_count = 2
        disabled_pdf_build_options = @()
        missing_pdf_build_options = @()
        pdf_dependency_inputs_status = "not_ready"
        pdf_dependency_check_available = $true
        pdf_dependency_missing_input_count = 2
        selected_pdfium_provider = "unresolved"
        pdfio_dependency_ready = $false
        pdfium_dependency_ready = $false
        pdf_dependency_missing_inputs_preview = @(
            "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h"
        )
    }
    pdf_dependency_inputs = [ordered]@{
        available = $true
        script_path = Join-Path $resolvedRepoRoot "scripts\check_pdf_dependency_inputs.ps1"
        schema = "featherdoc.pdf_dependency_inputs_check.v1"
        status = "not_ready"
        check_exit_code = 0
        error_message = ""
        pdfio_ready = $false
        pdfium_provider = "auto"
        selected_pdfium_provider = "unresolved"
        pdfium_ready = $false
        missing_input_count = 2
        missing_inputs = @(
            "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h",
            "PDFium input: provide prebuilt library/include inputs or a source checkout with public/fpdfview.h."
        )
    }
})

Write-JsonFile -Path $syntheticReadyPreflightPath -Value ([ordered]@{
    generated_at = "2026-05-20T00:00:00"
    status = "ready"
    strict = $false
    repo_root = $resolvedRepoRoot
    build_dir = Join-Path $resolvedWorkingDir "fake-pdf-build"
    build_dir_source = "requested"
    requested_build_dir = Join-Path $resolvedWorkingDir "fake-pdf-build"
    checks = @(
        [ordered]@{
            name = "build_dir_exists"
            status = "pass"
            required = $true
            message = "Build directory exists."
        },
        [ordered]@{
            name = "ctest_list_contains_pdf_gate_tests"
            status = "pass"
            required = $true
            message = "ctest -N lists the PDF visual gate prerequisites."
            details = [ordered]@{
                ctest_executable = Join-Path $resolvedWorkingDir "fake-ctest.cmd"
                exit_code = 0
                required_patterns = @(
                    "pdf_cli_export",
                    "pdf_regression_",
                    "pdf_visual_release_gate_style_baselines",
                    "pdf_visual_release_gate_text_shaping_baselines"
                )
                missing_patterns = @()
            }
        },
        [ordered]@{
            name = "render_python_reusable"
            status = "pass"
            required = $true
            message = "A reusable render Python with PIL and fitz is available."
            details = [ordered]@{
                selected_python_source = "FEATHERDOC_RENDER_PYTHON_EXECUTABLE"
                selected_python_path = Join-Path $resolvedWorkingDir "fake-python.cmd"
            }
        }
    )
    blocking_checks = @()
    blocking_summary = [ordered]@{
        required_check_count = 3
        blocking_check_count = 0
        missing_cli_pdf_count = 0
        visual_baseline_sample_count = 0
        missing_visual_baseline_pdf_count = 0
        cjk_text_layer_sample_count = 0
        missing_cjk_text_layer_pdf_count = 0
        build_dir_entry_count = 6
        ctest_required_pattern_count = 4
        free_memory_mb = 4096.5
        min_free_memory_mb = 2048
        memory_guard_blocked = $false
        memory_guard_skipped = $false
        pdf_build_options_enabled = $true
        pdf_build_option_count = 2
        disabled_pdf_build_options = @()
        missing_pdf_build_options = @()
        pdf_dependency_inputs_status = "ready"
        pdf_dependency_check_available = $true
        pdf_dependency_missing_input_count = 0
        selected_pdfium_provider = "prebuilt"
        pdfio_dependency_ready = $true
        pdfium_dependency_ready = $true
        pdf_dependency_missing_inputs_preview = @()
    }
    pdf_dependency_inputs = [ordered]@{
        available = $true
        script_path = Join-Path $resolvedRepoRoot "scripts\check_pdf_dependency_inputs.ps1"
        schema = "featherdoc.pdf_dependency_inputs_check.v1"
        status = "ready"
        check_exit_code = 0
        error_message = ""
        pdfio_ready = $true
        pdfium_provider = "prebuilt"
        selected_pdfium_provider = "prebuilt"
        pdfium_ready = $true
        pdfium_prebuilt_root = Join-Path $resolvedWorkingDir "fake-pdfium-prebuilt"
        pdfium_prebuilt_root_exists = $true
        missing_input_count = 0
        missing_inputs = @()
        cmake_cache_variables = [ordered]@{
            FEATHERDOC_PDFIO_SOURCE_DIR = Join-Path $resolvedWorkingDir "fake-pdfio-src"
            FEATHERDOC_PDFIUM_PROVIDER = "prebuilt"
            FEATHERDOC_PDFIUM_PREBUILT_ROOT = Join-Path $resolvedWorkingDir "fake-pdfium-prebuilt"
        }
    }
})

Write-JsonFile -Path $controlledVisualSmokePath -Value ([ordered]@{
    schema = "featherdoc.pdf_controlled_visual_smoke_check.v1"
    status = "pass"
    passed = $true
    root = Join-Path $resolvedWorkingDir "controlled-smoke"
    case_count = 2
    cases = @(
        [ordered]@{
            case = "minimal"
            passed = $true
            png_metrics = @(
                [ordered]@{
                    path = Join-Path $resolvedWorkingDir "controlled-smoke\minimal\pages\page-01.png"
                    width = 816
                    height = 1056
                    nonwhite_ratio = 0.00534063
                },
                [ordered]@{
                    path = Join-Path $resolvedWorkingDir "controlled-smoke\minimal\contact-sheet.png"
                    width = 408
                    height = 549
                    nonwhite_ratio = 0.20376621
                }
            )
        },
        [ordered]@{
            case = "rerun"
            passed = $true
            png_metrics = @(
                [ordered]@{
                    path = Join-Path $resolvedWorkingDir "controlled-smoke\rerun\pages\page-01.png"
                    width = 816
                    height = 1056
                    nonwhite_ratio = 0.00534063
                },
                [ordered]@{
                    path = Join-Path $resolvedWorkingDir "controlled-smoke\rerun\contact-sheet.png"
                    width = 408
                    height = 549
                    nonwhite_ratio = 0.20376621
                }
            )
        }
    )
})

Write-JsonFile -Path $failedControlledVisualSmokePath -Value ([ordered]@{
    schema = "featherdoc.pdf_controlled_visual_smoke_check.v1"
    status = "fail"
    passed = $false
    root = Join-Path $resolvedWorkingDir "controlled-smoke-failed"
    case_count = 1
    cases = @(
        [ordered]@{
            case = "minimal"
            passed = $false
            png_metrics = @(
                [ordered]@{
                    path = Join-Path $resolvedWorkingDir "controlled-smoke-failed\minimal\pages\page-01.png"
                    width = 816
                    height = 1056
                    nonwhite_ratio = 0.0
                },
                [ordered]@{
                    path = Join-Path $resolvedWorkingDir "controlled-smoke-failed\minimal\contact-sheet.png"
                    width = 408
                    height = 549
                    nonwhite_ratio = 0.0
                }
            )
        }
    )
})

$missingPreflightPath = Join-Path $resolvedWorkingDir "fixtures\missing-preflight.json"
$missingPreflightOutputDir = Join-Path $resolvedWorkingDir "missing-preflight-report"
$missingPreflightResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PreflightJson", $missingPreflightPath,
    "-OutputDir", $missingPreflightOutputDir,
    "-BuildDir", ".bpdf-roundtrip-msvc"
)
Assert-Equal -Actual $missingPreflightResult.ExitCode -Expected 1 `
    -Message "Missing explicit preflight JSON should fail after writing structured governance evidence. Output: $($missingPreflightResult.Text)"

$missingPreflightSummaryPath = Join-Path $missingPreflightOutputDir "summary.json"
$missingPreflightMarkdownPath = Join-Path $missingPreflightOutputDir "pdf_visual_release_gate_preflight_governance.md"
Assert-True -Condition (Test-Path -LiteralPath $missingPreflightSummaryPath) `
    -Message "Missing explicit preflight JSON should still write summary.json."
Assert-True -Condition (Test-Path -LiteralPath $missingPreflightMarkdownPath) `
    -Message "Missing explicit preflight JSON should still write Markdown."

$missingPreflightMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $missingPreflightMarkdownPath
Assert-ContainsText -Text $missingPreflightMarkdown `
    -ExpectedText "Source failures: ``1``" `
    -Message "Missing explicit preflight JSON Markdown should summarize source failures."
Assert-ContainsText -Text $missingPreflightMarkdown `
    -ExpectedText "source_failure_count: ``1``" `
    -Message "Missing explicit preflight JSON Markdown should expose a machine-readable source failure count."

$missingPreflightSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $missingPreflightSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$missingPreflightSummary.status) -Expected "failed" `
    -Message "Missing explicit preflight JSON should produce failed governance status."
Assert-Equal -Actual ([int]$missingPreflightSummary.source_failure_count) -Expected 1 `
    -Message "Missing explicit preflight JSON should count one source failure."
Assert-Equal -Actual ([int]$missingPreflightSummary.release_blocker_count) -Expected 1 `
    -Message "Missing explicit preflight JSON should emit one source failure blocker."
Assert-Equal -Actual ([string]$missingPreflightSummary.release_blockers[0].id) `
    -Expected "pdf_visual_release_gate_preflight.summary_unavailable" `
    -Message "Missing explicit preflight JSON should emit the summary unavailable blocker."
Assert-ContainsText -Text (($missingPreflightSummary.release_blockers[0].issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "preflight_summary_unavailable" `
    -Message "Missing explicit preflight JSON blocker should preserve the issue key."
Assert-ContainsText -Text ([string]$missingPreflightSummary.release_blockers[0].source_json_display) `
    -ExpectedText "missing-preflight.json" `
    -Message "Missing explicit preflight JSON blocker should point to the missing source path."

$missingHelperRepoRoot = Join-Path $resolvedWorkingDir "missing-helper-repo"
$missingHelperScriptsDir = Join-Path $missingHelperRepoRoot "scripts"
New-Item -ItemType Directory -Path $missingHelperScriptsDir -Force | Out-Null
$missingHelperScriptPath = Join-Path $missingHelperScriptsDir "write_pdf_visual_release_gate_preflight_governance_report.ps1"
Copy-Item -LiteralPath $scriptPath -Destination $missingHelperScriptPath -Force
Copy-Item -LiteralPath $scriptHelperPath -Destination (Join-Path $missingHelperScriptsDir "write_pdf_visual_release_gate_preflight_governance_report_helpers.ps1") -Force
$missingHelperOutputDir = Join-Path $resolvedWorkingDir "missing-helper-report"
$missingHelperResult = Invoke-PowerShellScript -ScriptPath $missingHelperScriptPath -Arguments @(
    "-OutputDir", $missingHelperOutputDir,
    "-BuildDir", ".bpdf-roundtrip-msvc"
)
Assert-Equal -Actual $missingHelperResult.ExitCode -Expected 1 `
    -Message "Missing implicit preflight helper should fail after writing structured governance evidence. Output: $($missingHelperResult.Text)"

$missingHelperSummaryPath = Join-Path $missingHelperOutputDir "summary.json"
$missingHelperMarkdownPath = Join-Path $missingHelperOutputDir "pdf_visual_release_gate_preflight_governance.md"
Assert-True -Condition (Test-Path -LiteralPath $missingHelperSummaryPath) `
    -Message "Missing implicit preflight helper should still write summary.json."
Assert-True -Condition (Test-Path -LiteralPath $missingHelperMarkdownPath) `
    -Message "Missing implicit preflight helper should still write Markdown."

$missingHelperMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $missingHelperMarkdownPath
Assert-ContainsText -Text $missingHelperMarkdown `
    -ExpectedText "Source failures: ``1``" `
    -Message "Missing implicit preflight helper Markdown should summarize source failures."
Assert-ContainsText -Text $missingHelperMarkdown `
    -ExpectedText "source_failure_count: ``1``" `
    -Message "Missing implicit preflight helper Markdown should expose a machine-readable source failure count."

$missingHelperSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $missingHelperSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$missingHelperSummary.status) -Expected "failed" `
    -Message "Missing implicit preflight helper should produce failed governance status."
Assert-Equal -Actual ([int]$missingHelperSummary.source_failure_count) -Expected 1 `
    -Message "Missing implicit preflight helper should count one source failure."
Assert-Equal -Actual ([int]$missingHelperSummary.release_blocker_count) -Expected 1 `
    -Message "Missing implicit preflight helper should emit one source failure blocker."
Assert-Equal -Actual ([string]$missingHelperSummary.release_blockers[0].id) `
    -Expected "pdf_visual_release_gate_preflight.summary_unavailable" `
    -Message "Missing implicit preflight helper should emit the summary unavailable blocker."
Assert-ContainsText -Text ([string]$missingHelperSummary.release_blockers[0].message) `
    -ExpectedText "preflight script was not found" `
    -Message "Missing implicit preflight helper blocker should explain the missing helper."
Assert-ContainsText -Text (($missingHelperSummary.release_blockers[0].issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "preflight_summary_unavailable" `
    -Message "Missing implicit preflight helper blocker should preserve the issue key."

$blockedOutputDir = Join-Path $resolvedWorkingDir "blocked-report"
$blockedResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PreflightJson", $notReadyPreflightPath,
    "-ControlledVisualSmokeJson", $controlledVisualSmokePath,
    "-OutputDir", $blockedOutputDir,
    "-BuildDir", ".bpdf-roundtrip-msvc"
)
Assert-Equal -Actual $blockedResult.ExitCode -Expected 0 `
    -Message "Governance report should write blocked output without failing by default. Output: $($blockedResult.Text)"

$blockedSummaryPath = Join-Path $blockedOutputDir "summary.json"
$blockedMarkdownPath = Join-Path $blockedOutputDir "pdf_visual_release_gate_preflight_governance.md"
Assert-True -Condition (Test-Path -LiteralPath $blockedSummaryPath) `
    -Message "Blocked governance report should write summary.json."
Assert-True -Condition (Test-Path -LiteralPath $blockedMarkdownPath) `
    -Message "Blocked governance report should write Markdown."

$blockedSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $blockedSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$blockedSummary.schema) `
    -Expected "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
    -Message "Governance report should expose a stable schema."
Assert-Equal -Actual ([string]$blockedSummary.status) -Expected "blocked" `
    -Message "Not-ready preflight should block release governance."
Assert-Equal -Actual ([bool]$blockedSummary.release_ready) -Expected $false `
    -Message "Not-ready preflight should not be release-ready."
Assert-Equal -Actual ([bool]$blockedSummary.preflight_ready) -Expected $false `
    -Message "Not-ready preflight should not be marked preflight-ready."
Assert-Equal -Actual ([bool]$blockedSummary.full_visual_gate_required) -Expected $true `
    -Message "Governance report should make the full visual gate requirement explicit."
Assert-Equal -Actual ([string]$blockedSummary.full_visual_gate_status) -Expected "not_run_by_preflight_governance" `
    -Message "Preflight governance should not claim the full visual gate has run."
Assert-Equal -Actual ([int]$blockedSummary.release_blocker_count) -Expected 1 `
    -Message "Not-ready preflight should emit one release blocker."
Assert-Equal -Actual ([int]$blockedSummary.action_item_count) -Expected 1 `
    -Message "Not-ready preflight should emit one action item."
Assert-Equal -Actual ([int]$blockedSummary.blocking_check_count) -Expected 9 `
    -Message "Governance report should include PDF dependency input readiness in blocking check count."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.required_check_count) -Expected 9 `
    -Message "Governance report should preserve the required check count."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.blocking_check_count) -Expected 9 `
    -Message "Governance report should include PDF dependency input readiness in blocking summary count."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.missing_cli_pdf_count) -Expected 2 `
    -Message "Governance report should preserve the missing CLI PDF count summary."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.visual_baseline_sample_count) -Expected 42 `
    -Message "Governance report should preserve the visual baseline sample count summary."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.missing_visual_baseline_pdf_count) -Expected 42 `
    -Message "Governance report should preserve the missing visual baseline PDF count summary."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.cjk_text_layer_sample_count) -Expected 43 `
    -Message "Governance report should preserve the CJK text-layer sample count summary."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.missing_cjk_text_layer_pdf_count) -Expected 43 `
    -Message "Governance report should preserve the missing CJK text-layer PDF count summary."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.build_dir_entry_count) -Expected 1 `
    -Message "Governance report should preserve the build dir entry count summary."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.ctest_required_pattern_count) -Expected 0 `
    -Message "Governance report should preserve the CTest required pattern count summary."
Assert-Equal -Actual ([string]$blockedSummary.free_memory_mb) -Expected "512.5" `
    -Message "Governance report should promote the preflight free memory summary."
Assert-Equal -Actual ([string]$blockedSummary.min_free_memory_mb) -Expected "2048" `
    -Message "Governance report should promote the minimum free memory threshold."
Assert-Equal -Actual ([bool]$blockedSummary.memory_guard_blocked) -Expected $true `
    -Message "Governance report should expose whether the memory guard blocked preflight."
Assert-Equal -Actual ([bool]$blockedSummary.memory_guard_skipped) -Expected $false `
    -Message "Governance report should expose whether the memory guard was skipped."
Assert-Equal -Actual ([bool]$blockedSummary.blocking_summary.memory_guard_blocked) -Expected $true `
    -Message "Governance report should preserve the memory guard blocker summary."
Assert-Equal -Actual ([bool]$blockedSummary.blocking_summary.pdf_build_options_enabled) -Expected $false `
    -Message "Governance report should preserve the PDF build option blocker summary."
Assert-Equal -Actual ([string]$blockedSummary.pdf_dependency_inputs_status) -Expected "not_ready" `
    -Message "Governance report should promote the PDF dependency input status."
Assert-Equal -Actual ([int]$blockedSummary.pdf_dependency_missing_input_count) -Expected 3 `
    -Message "Governance report should promote the PDF dependency missing input count."
Assert-Equal -Actual ([string]$blockedSummary.selected_pdfium_provider) -Expected "unresolved" `
    -Message "Governance report should promote the selected PDFium provider."
Assert-Equal -Actual ([bool]$blockedSummary.pdfio_dependency_ready) -Expected $false `
    -Message "Governance report should promote PDFio dependency readiness."
Assert-Equal -Actual ([bool]$blockedSummary.pdfium_dependency_ready) -Expected $false `
    -Message "Governance report should promote PDFium dependency readiness."
Assert-Equal -Actual ([string]$blockedSummary.blocking_summary.pdf_dependency_inputs_status) -Expected "not_ready" `
    -Message "Governance report should preserve the PDF dependency input status summary."
Assert-Equal -Actual ([int]$blockedSummary.blocking_summary.pdf_dependency_missing_input_count) -Expected 3 `
    -Message "Governance report should preserve the PDF dependency missing input count summary."
Assert-ContainsText -Text (($blockedSummary.blocking_summary.disabled_pdf_build_options | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "Governance report should preserve disabled PDF build options."
Assert-Equal -Actual ([string]$blockedSummary.build_dir_source) -Expected "requested" `
    -Message "Governance report should preserve the selected preflight build-dir source."
Assert-ContainsText -Text ([string]$blockedSummary.requested_build_dir_display) `
    -ExpectedText ".bpdf-roundtrip-msvc" `
    -Message "Governance report should preserve the originally requested build directory."
Assert-Equal -Actual ([bool]$blockedSummary.build_dir_snapshot.cmake_cache_exists) -Expected $false `
    -Message "Governance report should expose whether the selected build dir has CMakeCache.txt."
Assert-Equal -Actual ([bool]$blockedSummary.build_dir_snapshot.ctest_manifest_exists) -Expected $false `
    -Message "Governance report should expose whether the selected build dir has CTestTestfile.cmake."
Assert-Equal -Actual ([int]$blockedSummary.build_dir_snapshot.entry_count) -Expected 1 `
    -Message "Governance report should preserve the selected build dir entry count."
Assert-Equal -Actual (@($blockedSummary.build_dir_auto_candidates).Count) -Expected 2 `
    -Message "Governance report should preserve auto build candidate records."
$buildAutoCandidate = @($blockedSummary.build_dir_auto_candidates | Where-Object { [string]$_.relative_path -eq "build" }) |
    Select-Object -First 1
Assert-Equal -Actual ([bool]$buildAutoCandidate.exists) -Expected $true `
    -Message "Governance report should preserve whether the build auto-candidate exists."
Assert-Equal -Actual ([bool]$buildAutoCandidate.cmake_cache_exists) -Expected $true `
    -Message "Governance report should preserve the build auto-candidate CMake cache state."
Assert-Equal -Actual ([bool]$buildAutoCandidate.ctest_manifest_exists) -Expected $true `
    -Message "Governance report should preserve the build auto-candidate CTest manifest state."
Assert-Equal -Actual ([bool]$buildAutoCandidate.pdf_build_options_enabled) -Expected $false `
    -Message "Governance report should preserve the build auto-candidate PDF option readiness."
Assert-ContainsText -Text (($buildAutoCandidate.pdf_build_options | ForEach-Object { "$($_.name)=$($_.value):$($_.enabled)" }) -join "`n") `
    -ExpectedText "FEATHERDOC_BUILD_PDF=OFF:False" `
    -Message "Governance report should preserve the build auto-candidate PDF writer option snapshot."
Assert-ContainsText -Text (($buildAutoCandidate.pdf_build_options | ForEach-Object { "$($_.name)=$($_.value):$($_.enabled)" }) -join "`n") `
    -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT=OFF:False" `
    -Message "Governance report should preserve the build auto-candidate PDF import option snapshot."
Assert-Equal -Actual ([bool]$buildAutoCandidate.looks_reusable) -Expected $false `
    -Message "Governance report should preserve the build auto-candidate reusable decision."
Assert-Equal -Actual ([int]$blockedSummary.output_gap_count) -Expected 3 `
    -Message "Governance report should summarize checks with missing output paths."
Assert-Equal -Actual ([int]$blockedSummary.missing_output_count) -Expected 87 `
    -Message "Governance report should total missing PDF output counts."
Assert-Equal -Actual ([bool]$blockedSummary.controlled_visual_smoke_available) -Expected $true `
    -Message "Governance report should expose controlled visual smoke availability."
Assert-Equal -Actual ([string]$blockedSummary.controlled_visual_smoke_status) -Expected "pass" `
    -Message "Governance report should expose controlled visual smoke status."
Assert-Equal -Actual ([bool]$blockedSummary.controlled_visual_smoke_passed) -Expected $true `
    -Message "Governance report should expose controlled visual smoke pass state."
Assert-Equal -Actual ([int]$blockedSummary.controlled_visual_smoke_case_count) -Expected 2 `
    -Message "Governance report should expose controlled visual smoke case count."
Assert-ContainsText -Text ([string]$blockedSummary.controlled_visual_smoke_json_display) `
    -ExpectedText "controlled-visual-smoke.json" `
    -Message "Governance report should point reviewers at the controlled visual smoke JSON."
Assert-Equal -Actual ([int]$blockedSummary.controlled_visual_smoke.passed_case_count) -Expected 2 `
    -Message "Governance report should summarize controlled visual smoke passed cases."
Assert-Equal -Actual ([int]$blockedSummary.controlled_visual_smoke.failed_case_count) -Expected 0 `
    -Message "Governance report should summarize controlled visual smoke failed cases."
Assert-ContainsText -Text (($blockedSummary.controlled_visual_smoke.cases | ForEach-Object { [string]$_.contact_sheet_display }) -join "`n") `
    -ExpectedText "contact-sheet.png" `
    -Message "Governance report should expose controlled visual smoke contact sheet paths."
Assert-ContainsText -Text ([string]$blockedSummary.controlled_visual_smoke.min_nonwhite_ratio) `
    -ExpectedText "0.00534063" `
    -Message "Governance report should expose the minimum controlled smoke nonwhite ratio."
Assert-Equal -Actual ([int]$blockedSummary.warning_count) -Expected 0 `
    -Message "Passing controlled visual smoke evidence should not emit governance warnings."
Assert-Equal -Actual @($blockedSummary.warnings).Count -Expected 0 `
    -Message "Passing controlled visual smoke evidence should preserve an empty warnings array."

$failedSmokeOutputDir = Join-Path $resolvedWorkingDir "failed-smoke-report"
$failedSmokeResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PreflightJson", $notReadyPreflightPath,
    "-ControlledVisualSmokeJson", $failedControlledVisualSmokePath,
    "-OutputDir", $failedSmokeOutputDir,
    "-BuildDir", ".bpdf-roundtrip-msvc"
)
Assert-Equal -Actual $failedSmokeResult.ExitCode -Expected 0 `
    -Message "Failed controlled smoke should warn without changing default governance exit. Output: $($failedSmokeResult.Text)"
$failedSmokeSummaryPath = Join-Path $failedSmokeOutputDir "summary.json"
$failedSmokeMarkdownPath = Join-Path $failedSmokeOutputDir "pdf_visual_release_gate_preflight_governance.md"
$failedSmokeSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failedSmokeSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$failedSmokeSummary.status) -Expected "blocked" `
    -Message "Failed controlled smoke should preserve the preflight blocker status."
Assert-Equal -Actual ([int]$failedSmokeSummary.release_blocker_count) -Expected 1 `
    -Message "Failed controlled smoke should not add release blockers beyond the preflight blocker."
Assert-Equal -Actual ([int]$failedSmokeSummary.action_item_count) -Expected 1 `
    -Message "Failed controlled smoke should not change preflight action item semantics."
Assert-Equal -Actual ([int]$failedSmokeSummary.warning_count) -Expected 1 `
    -Message "Failed controlled smoke should emit one governance warning."
$failedSmokeWarning = @($failedSmokeSummary.warnings | Select-Object -First 1)[0]
Assert-Equal -Actual ([string]$failedSmokeWarning.id) `
    -Expected "pdf_controlled_visual_smoke.unavailable_or_failed" `
    -Message "Failed controlled smoke warning should use the stable warning id."
Assert-Equal -Actual ([string]$failedSmokeWarning.source_schema) `
    -Expected "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
    -Message "Failed controlled smoke warning should carry the governance schema."
Assert-Equal -Actual ([string]$failedSmokeWarning.status) -Expected "fail" `
    -Message "Failed controlled smoke warning should preserve the smoke status."
Assert-ContainsText -Text ([string]$failedSmokeWarning.source_json_display) `
    -ExpectedText "controlled-visual-smoke-failed.json" `
    -Message "Failed controlled smoke warning should point reviewers at the smoke JSON."
Assert-ContainsText -Text ([string]$failedSmokeWarning.source_report_display) `
    -ExpectedText "failed-smoke-report\summary.json" `
    -Message "Failed controlled smoke warning should point reviewers at the governance summary."
Assert-Equal -Actual ([bool]$failedSmokeSummary.controlled_visual_smoke_available) -Expected $true `
    -Message "Failed controlled smoke summary should still mark readable smoke evidence as available."
Assert-Equal -Actual ([bool]$failedSmokeSummary.controlled_visual_smoke_passed) -Expected $false `
    -Message "Failed controlled smoke summary should preserve the failing pass state."
Assert-Equal -Actual ([int]$failedSmokeSummary.controlled_visual_smoke.failed_case_count) -Expected 1 `
    -Message "Failed controlled smoke summary should count failed smoke cases."
$failedSmokeMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $failedSmokeMarkdownPath
Assert-ContainsText -Text $failedSmokeMarkdown `
    -ExpectedText "## Warnings" `
    -Message "Failed controlled smoke Markdown should include a warnings section."
Assert-ContainsText -Text $failedSmokeMarkdown `
    -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
    -Message "Failed controlled smoke Markdown should expose the warning id."
Assert-ContainsText -Text $failedSmokeMarkdown `
    -ExpectedText "Controlled PDF visual smoke evidence was provided but is not passing." `
    -Message "Failed controlled smoke Markdown should explain the warning."
Assert-ContainsText -Text $failedSmokeMarkdown `
    -ExpectedText "source_json_display: ``$([string]$failedSmokeWarning.source_json_display)``" `
    -Message "Failed controlled smoke Markdown should expose the warning source JSON display."
Assert-ContainsText -Text $failedSmokeMarkdown `
    -ExpectedText "source_report_display: ``$([string]$failedSmokeWarning.source_report_display)``" `
    -Message "Failed controlled smoke Markdown should expose the warning source report display."

$outputGapChecks = ($blockedSummary.output_gap_summary | ForEach-Object { [string]$_.check }) -join "`n"
Assert-ContainsText -Text $outputGapChecks `
    -ExpectedText "pdf_cli_export_baseline_pdfs_exist" `
    -Message "Output gap summary should include missing CLI export source PDFs."
Assert-ContainsText -Text $outputGapChecks `
    -ExpectedText "visual_baseline_manifest_pdfs_exist" `
    -Message "Output gap summary should include missing visual baseline PDFs."
Assert-ContainsText -Text $outputGapChecks `
    -ExpectedText "cjk_text_layer_manifest_pdfs_exist" `
    -Message "Output gap summary should include missing CJK text-layer PDFs."
$firstOutputGap = @($blockedSummary.output_gap_summary | Select-Object -First 1)[0]
Assert-Equal -Actual ([string]$firstOutputGap.source_report) -Expected $blockedSummaryPath `
    -Message "Output gap summary entries should point at the governance summary."
Assert-ContainsText -Text ([string]$firstOutputGap.source_report_display) `
    -ExpectedText "blocked-report\summary.json" `
    -Message "Output gap summary entries should expose the governance summary display path."
Assert-Equal -Actual ([string]$firstOutputGap.source_json) -Expected $notReadyPreflightPath `
    -Message "Output gap summary entries should point at the source preflight JSON."
Assert-ContainsText -Text ([string]$firstOutputGap.source_json_display) `
    -ExpectedText "not-ready-preflight.json" `
    -Message "Output gap summary entries should expose the source preflight JSON display path."
$outputGapPreview = ($blockedSummary.output_gap_summary | ForEach-Object { $_.missing_paths_preview } | ForEach-Object { [string]$_ }) -join "`n"
Assert-ContainsText -Text $outputGapPreview `
    -ExpectedText "test\pdf_cli_export\font-map-source.pdf" `
    -Message "Output gap summary should preserve representative CLI export missing paths."
Assert-ContainsText -Text $outputGapPreview `
    -ExpectedText "test\pdf_visual_baselines\cjk\font_map_text.pdf" `
    -Message "Output gap summary should preserve representative visual baseline missing paths."
Assert-ContainsText -Text $outputGapPreview `
    -ExpectedText "test\pdf_text_layer_cjk\copy_search\mixed_cjk_text.pdf" `
    -Message "Output gap summary should preserve representative CJK text-layer missing paths."

Assert-Equal -Actual ([int]$blockedSummary.readiness_action_evidence_count) -Expected 5 `
    -Message "Governance report should expose per-item readiness action evidence."
$readinessActionEvidence = @($blockedSummary.readiness_action_evidence)
Assert-Equal -Actual $readinessActionEvidence.Count -Expected 5 `
    -Message "Governance report should preserve every readiness action evidence item."
$readinessActionEvidenceText = ($readinessActionEvidence | ForEach-Object {
        "$($_.id)|$($_.action)|$($_.issue_key)|$($_.item)|$($_.source_json_display)"
    }) -join "`n"
Assert-ContainsText -Text $readinessActionEvidenceText `
    -ExpectedText "provide_pdf_dependency_input|pdf_dependency_inputs_ready|PDFio source header" `
    -Message "Readiness action evidence should identify PDFio dependency input work."
Assert-ContainsText -Text $readinessActionEvidenceText `
    -ExpectedText "PDFium input: provide prebuilt library/include inputs" `
    -Message "Readiness action evidence should preserve PDFium dependency input guidance."
Assert-ContainsText -Text $readinessActionEvidenceText `
    -ExpectedText "enable_pdf_build_option|pdf_build_options_enabled|FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "Readiness action evidence should identify disabled PDF import build option work."
$firstReadinessEvidence = $readinessActionEvidence[0]
Assert-Equal -Actual ([string]$firstReadinessEvidence.source_report) -Expected $blockedSummaryPath `
    -Message "Readiness action evidence should point at the governance summary."
Assert-ContainsText -Text ([string]$firstReadinessEvidence.source_report_display) `
    -ExpectedText "blocked-report\summary.json" `
    -Message "Readiness action evidence should expose the governance summary display path."
Assert-Equal -Actual ([string]$firstReadinessEvidence.source_json) -Expected $notReadyPreflightPath `
    -Message "Readiness action evidence should point at the source preflight JSON."
Assert-ContainsText -Text ([string]$firstReadinessEvidence.source_json_display) `
    -ExpectedText "not-ready-preflight.json" `
    -Message "Readiness action evidence should expose the source preflight JSON display path."
Assert-ContainsText -Text ([string]$firstReadinessEvidence.command_template) `
    -ExpectedText "-PreflightOnly" `
    -Message "Readiness action evidence should preserve the preflight-only command template."

$blocker = $blockedSummary.release_blockers[0]
Assert-Equal -Actual ([string]$blocker.id) `
    -Expected "pdf_visual_release_gate_preflight.build_outputs_missing" `
    -Message "Governance report should use the PDF visual preflight blocker id."
Assert-Equal -Actual ([string]$blocker.source_schema) `
    -Expected "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
    -Message "Blocker should carry source schema."
Assert-ContainsText -Text ([string]$blocker.source_json_display) `
    -ExpectedText "not-ready-preflight.json" `
    -Message "Blocker should point reviewers at the source preflight JSON."
Assert-ContainsText -Text ([string]$blocker.source_report_display) `
    -ExpectedText "blocked-report\summary.json" `
    -Message "Blocker should point reviewers at the source governance summary."
Assert-ContainsText -Text ([string]$blocker.command_template) `
    -ExpectedText "run_pdf_visual_release_gate.ps1" `
    -Message "Blocker should include the preflight-only command template."
Assert-ContainsText -Text ([string]$blocker.command_template) `
    -ExpectedText "-PreflightOnly" `
    -Message "Blocker command should not run the full PDF visual gate."
Assert-ContainsText -Text (($blocker.issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "ctest_manifest_exists" `
    -Message "Blocker should preserve individual failing preflight checks."
Assert-ContainsText -Text (($blocker.issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "workstation_free_memory_available" `
    -Message "Blocker should preserve the memory guard preflight check."
Assert-ContainsText -Text (($blocker.issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "pdf_build_options_enabled" `
    -Message "Blocker should preserve the PDF build option preflight check."
Assert-ContainsText -Text (($blocker.issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "pdf_dependency_inputs_ready" `
    -Message "Blocker should preserve the PDF dependency input readiness check."
Assert-ContainsText -Text ([string]$blocker.repair_hint) `
    -ExpectedText "Close unrelated applications" `
    -Message "Blocker should explain how to clear a low-memory preflight blocker."
Assert-ContainsText -Text ([string]$blocker.repair_hint) `
    -ExpectedText "512.5 MB free, 2048 MB required" `
    -Message "Blocker should include the preflight memory threshold and snapshot."
Assert-ContainsText -Text ([string]$blocker.repair_hint) `
    -ExpectedText "CMakeCache.txt missing" `
    -Message "Blocker should explain when the selected build dir is not a reusable CMake build."
Assert-ContainsText -Text ([string]$blocker.repair_hint) `
    -ExpectedText "-DFEATHERDOC_BUILD_PDF=ON" `
    -Message "Blocker should explain how to reconfigure disabled PDF build options."
Assert-ContainsText -Text ([string]$blocker.repair_hint) `
    -ExpectedText "selected_pdfium_provider=unresolved" `
    -Message "Blocker should surface the selected PDFium provider when dependency inputs are not ready."
Assert-ContainsText -Text ([string]$blocker.repair_hint) `
    -ExpectedText "missing_input_count=3" `
    -Message "Blocker should surface the PDF dependency missing input count."
Assert-Equal -Actual ([int]$blocker.output_gap_count) -Expected 3 `
    -Message "Blocker should expose how many output gap groups remain."
Assert-Equal -Actual ([int]$blocker.missing_output_count) -Expected 87 `
    -Message "Blocker should expose the total missing output count."
Assert-Equal -Actual ([int]$blocker.blocking_summary.missing_visual_baseline_pdf_count) -Expected 42 `
    -Message "Blocker should preserve the missing visual baseline PDF count summary."
Assert-Equal -Actual (@($blocker.build_dir_auto_candidates).Count) -Expected 2 `
    -Message "Blocker should carry auto build candidate records."
Assert-Equal -Actual ([string]$blocker.pdf_dependency_inputs.status) -Expected "not_ready" `
    -Message "Blocker should carry the PDF dependency input summary."
Assert-ContainsText -Text (($blocker.output_gap_summary | ForEach-Object { [string]$_.check }) -join "`n") `
    -ExpectedText "cjk_text_layer_manifest_pdfs_exist" `
    -Message "Blocker should preserve output gap summary details."
Assert-Equal -Actual ([int]$blocker.readiness_action_evidence_count) -Expected 5 `
    -Message "Blocker should expose per-item readiness action evidence."
Assert-ContainsText -Text (($blocker.readiness_action_evidence | ForEach-Object { "$($_.action)|$($_.item)" }) -join "`n") `
    -ExpectedText "provide_pdf_dependency_input|PDFio source header" `
    -Message "Blocker should preserve dependency readiness action evidence."
Assert-ContainsText -Text (($blocker.readiness_action_evidence | ForEach-Object { "$($_.action)|$($_.item)" }) -join "`n") `
    -ExpectedText "enable_pdf_build_option|FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "Blocker should preserve PDF build option readiness action evidence."

$actionItem = $blockedSummary.action_items[0]
Assert-ContainsText -Text (($actionItem.issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "cmake_cache_exists" `
    -Message "Action item should preserve individual failing preflight checks."
Assert-ContainsText -Text (($actionItem.issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "pdf_dependency_inputs_ready" `
    -Message "Action item should preserve the PDF dependency input readiness check."
Assert-Equal -Actual ([int]$actionItem.output_gap_count) -Expected 3 `
    -Message "Action item should expose how many output gap groups remain."
Assert-Equal -Actual ([int]$actionItem.missing_output_count) -Expected 87 `
    -Message "Action item should expose the total missing output count."
Assert-Equal -Actual ([int]$actionItem.blocking_summary.missing_cjk_text_layer_pdf_count) -Expected 43 `
    -Message "Action item should preserve the missing CJK text-layer PDF count summary."
Assert-Equal -Actual (@($actionItem.build_dir_auto_candidates).Count) -Expected 2 `
    -Message "Action item should carry auto build candidate records."
Assert-Equal -Actual ([string]$actionItem.pdf_dependency_inputs.status) -Expected "not_ready" `
    -Message "Action item should carry the PDF dependency input summary."
Assert-Equal -Actual ([int]$actionItem.readiness_action_evidence_count) -Expected 5 `
    -Message "Action item should expose per-item readiness action evidence."
Assert-ContainsText -Text (($actionItem.readiness_action_evidence | ForEach-Object { "$($_.action)|$($_.item)" }) -join "`n") `
    -ExpectedText "enable_pdf_build_option|FEATHERDOC_BUILD_PDF" `
    -Message "Action item should preserve PDF build option readiness action evidence."

$overrideOutputDir = Join-Path $resolvedWorkingDir "override-report"
$overridePdfioDir = Join-Path $resolvedWorkingDir "dependency overrides\pdfio"
$overridePdfiumPrebuiltRoot = Join-Path $resolvedWorkingDir "dependency overrides\pdfium prebuilt"
$overrideResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PreflightJson", $notReadyPreflightPath,
    "-OutputDir", $overrideOutputDir,
    "-BuildDir", ".bpdf-roundtrip-msvc",
    "-PdfioSourceDir", $overridePdfioDir,
    "-PdfiumProvider", "prebuilt",
    "-PdfiumPrebuiltRoot", $overridePdfiumPrebuiltRoot
)
Assert-Equal -Actual $overrideResult.ExitCode -Expected 0 `
    -Message "Governance report should accept PDF dependency override arguments. Output: $($overrideResult.Text)"
$overrideSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $overrideOutputDir "summary.json") | ConvertFrom-Json
$overrideCommandTemplate = [string]$overrideSummary.release_blockers[0].command_template
Assert-ContainsText -Text $overrideCommandTemplate `
    -ExpectedText "-PdfioSourceDir" `
    -Message "Override command template should preserve the PDFio override parameter."
Assert-ContainsText -Text $overrideCommandTemplate `
    -ExpectedText "dependency overrides\pdfio" `
    -Message "Override command template should preserve the PDFio override value."
Assert-ContainsText -Text $overrideCommandTemplate `
    -ExpectedText "-PdfiumProvider prebuilt" `
    -Message "Override command template should preserve the PDFium provider override."
Assert-ContainsText -Text $overrideCommandTemplate `
    -ExpectedText "-PdfiumPrebuiltRoot" `
    -Message "Override command template should preserve the PDFium prebuilt root override parameter."
Assert-ContainsText -Text $overrideCommandTemplate `
    -ExpectedText "dependency overrides\pdfium prebuilt" `
    -Message "Override command template should quote and preserve the PDFium prebuilt root override value."

$blockedMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $blockedMarkdownPath
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "PDF Visual Release Gate Preflight Governance Report" `
    -Message "Markdown should include the report title."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "build_outputs_missing" `
    -Message "Markdown should include the blocker id."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_report_display" `
    -Message "Markdown should include source report display details."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_report: ``$blockedSummaryPath``" `
    -Message "Markdown should include the raw blocker source report path."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_json: ``$notReadyPreflightPath``" `
    -Message "Markdown should include the raw blocker source JSON path."
Assert-ContainsText -Text ([string]$blockedSummary.release_blockers[0].source_report_display) `
    -ExpectedText "blocked-report\summary.json" `
    -Message "Blocker summary should preserve the source report display."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "summary.json" `
    -Message "Markdown should point reviewers at the governance summary."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "ctest_manifest_exists" `
    -Message "Markdown should include blocking checks."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Build dir source" `
    -Message "Markdown should include the selected build-dir source."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Preflight ready" `
    -Message "Markdown should distinguish preflight readiness from full visual gate readiness."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Full visual gate required" `
    -Message "Markdown should show the full visual gate is still required."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "not_run_by_preflight_governance" `
    -Message "Markdown should not imply preflight governance ran the full visual gate."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Controlled visual smoke status: ``pass``" `
    -Message "Markdown should expose controlled visual smoke status."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Controlled Visual Smoke" `
    -Message "Markdown should include a controlled visual smoke section."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "controlled-visual-smoke.json" `
    -Message "Markdown should point reviewers at the controlled visual smoke JSON."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "contact-sheet.png" `
    -Message "Markdown should point reviewers at controlled visual smoke contact sheets."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "min nonwhite ratio" `
    -Message "Markdown should expose controlled visual smoke nonblank pixel evidence."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Build dir source: ``requested``" `
    -Message "Markdown should preserve the selected build-dir source value."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Build CMake cache" `
    -Message "Markdown should include the selected build dir CMake cache status."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Build CTest manifest" `
    -Message "Markdown should include the selected build dir CTest manifest status."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Build auto candidates" `
    -Message "Markdown should include auto build candidate details."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText 'reusable=`False`' `
    -Message "Markdown should show why the auto build candidate was not reusable."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText 'PDFOptions=`False`' `
    -Message "Markdown should show whether the auto build candidate has release-ready PDF options."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText 'PDF option `FEATHERDOC_BUILD_PDF`: present=`True`; value=`OFF`; enabled=`False`' `
    -Message "Markdown should show the auto build candidate PDF writer option state."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText 'PDF option `FEATHERDOC_BUILD_PDF_IMPORT`: present=`True`; value=`OFF`; enabled=`False`' `
    -Message "Markdown should show the auto build candidate PDF import option state."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Missing CLI PDFs" `
    -Message "Markdown should expose the missing CLI PDF count summary."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "PDF build options enabled" `
    -Message "Markdown should expose the PDF build option readiness summary."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "PDF dependency inputs status" `
    -Message "Markdown should expose the PDF dependency input status."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Selected PDFium provider" `
    -Message "Markdown should expose the selected PDFium provider."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "PDF dependency missing inputs preview" `
    -Message "Markdown should expose a PDF dependency missing-input preview."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "Markdown should list disabled PDF build options."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Free memory MB" `
    -Message "Markdown should expose the free-memory preflight summary."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Memory guard blocked" `
    -Message "Markdown should expose whether the memory guard blocked preflight."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "workstation_free_memory_available" `
    -Message "Markdown should list the memory guard blocking check."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Readiness Action Evidence" `
    -Message "Markdown should include a readiness action evidence section."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "provide_pdf_dependency_input" `
    -Message "Markdown should expose dependency readiness actions."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "enable_pdf_build_option" `
    -Message "Markdown should expose PDF build option readiness actions."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "PDFio source header" `
    -Message "Markdown should include dependency readiness evidence items."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "Markdown should include PDF build option readiness evidence items."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_report: ``$([string]$firstReadinessEvidence.source_report)``" `
    -Message "Markdown should include raw readiness evidence source report paths."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_json: ``$([string]$firstReadinessEvidence.source_json)``" `
    -Message "Markdown should include raw readiness evidence source JSON paths."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_json_display: ``$([string]$firstReadinessEvidence.source_json_display)``" `
    -Message "Markdown should include readiness evidence source JSON display paths."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "issue_keys" `
    -Message "Markdown should include action item issue keys."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_report: ``$([string]$actionItem.source_report)``" `
    -Message "Markdown should include the raw action item source report path."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_json: ``$([string]$actionItem.source_json)``" `
    -Message "Markdown should include the raw action item source JSON path."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Warnings: ``0``" `
    -Message "Markdown should summarize the warning count."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "## Warnings`r`n`r`n- none" `
    -Message "Markdown should include an explicit empty warnings section."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Missing outputs" `
    -Message "Markdown should summarize the missing output count."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Output Gaps" `
    -Message "Markdown should include an output gaps section."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "visual_baseline_manifest_pdfs_exist" `
    -Message "Markdown should list output gap checks."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_report: ``$blockedSummaryPath``" `
    -Message "Markdown should include raw output-gap source report paths."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_json: ``$notReadyPreflightPath``" `
    -Message "Markdown should include raw output-gap source JSON paths."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "source_json_display: ``$([string]$firstOutputGap.source_json_display)``" `
    -Message "Markdown should include output-gap source JSON display paths."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "test\pdf_cli_export\font-map-source.pdf" `
    -Message "Markdown should list representative missing output paths."

$rollupOutputDir = Join-Path $resolvedWorkingDir "rollup-report"
$rollupResult = Invoke-PowerShellScript -ScriptPath $rollupScriptPath -Arguments @(
    "-InputJson", $blockedSummaryPath,
    "-OutputDir", $rollupOutputDir
)
Assert-Equal -Actual $rollupResult.ExitCode -Expected 0 `
    -Message "Release blocker rollup should consume the PDF preflight governance report. Output: $($rollupResult.Text)"
$rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $rollupOutputDir "summary.json") | ConvertFrom-Json
Assert-Equal -Actual ([string]$rollupSummary.status) -Expected "blocked" `
    -Message "Rollup should be blocked by the PDF visual preflight governance report."
Assert-Equal -Actual ([int]$rollupSummary.release_blocker_count) -Expected 1 `
    -Message "Rollup should aggregate the PDF visual preflight blocker."
Assert-Equal -Actual ([string]$rollupSummary.release_blockers[0].source_schema) `
    -Expected "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
    -Message "Rollup should preserve the PDF visual preflight governance schema."
Assert-ContainsText -Text ([string]$rollupSummary.release_blockers[0].source_json_display) `
    -ExpectedText "not-ready-preflight.json" `
    -Message "Rollup should preserve the source preflight JSON display."
Assert-Equal -Actual ([int]$rollupSummary.release_blockers[0].output_gap_count) -Expected 3 `
    -Message "Rollup should preserve PDF preflight blocker output gap count."
Assert-Equal -Actual ([int]$rollupSummary.release_blockers[0].missing_output_count) -Expected 87 `
    -Message "Rollup should preserve PDF preflight blocker missing output count."
Assert-Equal -Actual ([int]$rollupSummary.release_blockers[0].blocking_summary.missing_visual_baseline_pdf_count) -Expected 42 `
    -Message "Rollup should preserve PDF preflight blocker blocking summary details."
Assert-Equal -Actual (@($rollupSummary.release_blockers[0].build_dir_auto_candidates).Count) -Expected 2 `
    -Message "Rollup should preserve PDF preflight blocker build auto candidates."
Assert-Equal -Actual ([string]$rollupSummary.release_blockers[0].pdf_dependency_inputs.status) -Expected "not_ready" `
    -Message "Rollup should preserve the PDF dependency input summary."
Assert-Equal -Actual ([int]$rollupSummary.action_items[0].output_gap_count) -Expected 3 `
    -Message "Rollup should preserve PDF preflight action item output gap count."
Assert-Equal -Actual (@($rollupSummary.action_items[0].build_dir_auto_candidates).Count) -Expected 2 `
    -Message "Rollup should preserve PDF preflight action item build auto candidates."

$dependencyBlockedOutputDir = Join-Path $resolvedWorkingDir "dependency-blocked-report"
$readyResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PreflightJson", $readyPreflightPath,
    "-OutputDir", $dependencyBlockedOutputDir,
    "-BuildDir", ".bpdf-roundtrip-msvc"
)
Assert-Equal -Actual $readyResult.ExitCode -Expected 0 `
    -Message "Dependency-blocked governance report should succeed. Output: $($readyResult.Text)"
$dependencyBlockedSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $dependencyBlockedOutputDir "summary.json") | ConvertFrom-Json
Assert-Equal -Actual ([string]$dependencyBlockedSummary.status) -Expected "blocked" `
    -Message "Governance report should block a top-level ready preflight when PDF dependency inputs are not ready."
Assert-Equal -Actual ([bool]$dependencyBlockedSummary.release_ready) -Expected $false `
    -Message "Dependency-blocked preflight should not be release-ready."
Assert-Equal -Actual ([bool]$dependencyBlockedSummary.preflight_ready) -Expected $false `
    -Message "Dependency-blocked preflight should not be marked preflight-ready."
Assert-Equal -Actual ([bool]$dependencyBlockedSummary.full_visual_gate_required) -Expected $true `
    -Message "Dependency-blocked preflight should still require the full visual gate."
Assert-Equal -Actual ([string]$dependencyBlockedSummary.full_visual_gate_status) -Expected "not_run_by_preflight_governance" `
    -Message "Dependency-blocked preflight should not claim the full visual gate has run."
Assert-Equal -Actual ([int]$dependencyBlockedSummary.release_blocker_count) -Expected 1 `
    -Message "Dependency-blocked preflight should emit one blocker."
Assert-Equal -Actual ([int]$dependencyBlockedSummary.action_item_count) -Expected 1 `
    -Message "Dependency-blocked preflight should emit one action item."
Assert-Equal -Actual ([int]$dependencyBlockedSummary.blocking_check_count) -Expected 1 `
    -Message "Dependency-blocked preflight should add the dependency input blocking check."
Assert-ContainsText -Text (($dependencyBlockedSummary.blocking_checks | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "pdf_dependency_inputs_ready" `
    -Message "Dependency-blocked preflight should expose the dependency input blocking check."
Assert-ContainsText -Text (($dependencyBlockedSummary.release_blockers[0].issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "pdf_dependency_inputs_ready" `
    -Message "Dependency-blocked blocker should preserve the dependency input issue key."
Assert-ContainsText -Text ([string]$dependencyBlockedSummary.release_blockers[0].repair_hint) -ExpectedText "status=not_ready" `
    -Message "Dependency-blocked blocker should include dependency status guidance."
Assert-ContainsText -Text ([string]$dependencyBlockedSummary.release_blockers[0].repair_hint) -ExpectedText "missing_input_count=2" `
    -Message "Dependency-blocked blocker should include dependency missing input guidance."
Assert-Equal -Actual ([string]$dependencyBlockedSummary.pdf_dependency_inputs_status) -Expected "not_ready" `
    -Message "Dependency-blocked governance report should expose PDF dependency input status."
Assert-Equal -Actual ([int]$dependencyBlockedSummary.pdf_dependency_missing_input_count) -Expected 2 `
    -Message "Dependency-blocked governance report should expose PDF dependency missing input count."
Assert-Equal -Actual ([int]$dependencyBlockedSummary.blocking_summary.blocking_check_count) -Expected 1 `
    -Message "Dependency-blocked governance report should update blocking summary count."

$syntheticReadyOutputDir = Join-Path $resolvedWorkingDir "synthetic-ready-report"
$syntheticReadyResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PreflightJson", $syntheticReadyPreflightPath,
    "-OutputDir", $syntheticReadyOutputDir,
    "-BuildDir", ".bpdf-roundtrip-msvc"
)
Assert-Equal -Actual $syntheticReadyResult.ExitCode -Expected 0 `
    -Message "Synthetic-ready governance report should succeed while blocking release readiness. Output: $($syntheticReadyResult.Text)"
$syntheticReadySummary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $syntheticReadyOutputDir "summary.json") | ConvertFrom-Json
Assert-Equal -Actual ([string]$syntheticReadySummary.status) -Expected "blocked" `
    -Message "Governance report should block synthetic fixture evidence even when preflight status is ready."
Assert-Equal -Actual ([bool]$syntheticReadySummary.release_ready) -Expected $false `
    -Message "Synthetic fixture evidence should not be release-ready."
Assert-Equal -Actual ([bool]$syntheticReadySummary.preflight_ready) -Expected $false `
    -Message "Synthetic fixture evidence should not be treated as real preflight readiness."
Assert-Equal -Actual ([int]$syntheticReadySummary.blocking_check_count) -Expected 1 `
    -Message "Synthetic fixture evidence should add exactly one blocking governance check."
Assert-Equal -Actual ([string]$syntheticReadySummary.evidence_kind) -Expected "synthetic_fixture" `
    -Message "Synthetic-ready governance report should expose the synthetic evidence kind."
Assert-True -Condition ([int]$syntheticReadySummary.synthetic_evidence_marker_count -gt 0) `
    -Message "Synthetic-ready governance report should expose synthetic evidence markers."
Assert-ContainsText -Text (($syntheticReadySummary.blocking_checks | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "synthetic_preflight_evidence" `
    -Message "Synthetic-ready governance report should expose the synthetic evidence blocking check."
Assert-ContainsText -Text (($syntheticReadySummary.release_blockers[0].issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "synthetic_preflight_evidence" `
    -Message "Synthetic-ready blocker should preserve the synthetic evidence issue key."
Assert-ContainsText -Text ([string]$syntheticReadySummary.release_blockers[0].repair_hint) `
    -ExpectedText "synthetic test-fixture evidence" `
    -Message "Synthetic-ready blocker should explain that fake fixture evidence cannot release the real visual gate."
$syntheticMarkerText = ($syntheticReadySummary.synthetic_evidence_markers | ForEach-Object { [string]$_ }) -join "`n"
Assert-ContainsText -Text $syntheticMarkerText `
    -ExpectedText "fake-pdf-build" `
    -Message "Synthetic-ready governance report should identify the fake build marker."
Assert-ContainsText -Text $syntheticMarkerText `
    -ExpectedText "fake-ctest.cmd" `
    -Message "Synthetic-ready governance report should identify the fake CTest marker."
Assert-ContainsText -Text $syntheticMarkerText `
    -ExpectedText "fake-python.cmd" `
    -Message "Synthetic-ready governance report should identify the fake render Python marker."
Assert-ContainsText -Text $syntheticMarkerText `
    -ExpectedText "fake-pdfio-src" `
    -Message "Synthetic-ready governance report should identify the fake PDFio marker."
Assert-Equal -Actual ([int]$syntheticReadySummary.blocking_summary.blocking_check_count) -Expected 1 `
    -Message "Synthetic-ready governance report should update blocking summary count."

$governanceScriptText = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
    Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptHelperPath
) -join "`n"
foreach ($expectedText in @(
    "[int]`$MinFreeMemoryMB = 2048",
    "[switch]`$SkipMemoryGuard",
    "-MinFreeMemoryMB `$MinFreeMemoryMB",
    "SkipMemoryGuard = [bool]`$SkipMemoryGuard",
    "[string]`$PdfioSourceDir = `"`"",
    "[string]`$PdfiumProvider = `"`"",
    "[string]`$PdfiumPrebuiltRoot = `"`"",
    "[string]`$ControlledVisualSmokeJson = `"`"",
    "controlled_visual_smoke",
    "Add-OptionalPreflightOverride",
    "PdfioSourceDir",
    "PdfiumPrebuiltRoot",
    "PdfiumRuntimeDir",
    "memoryGuardCommandArgs",
    "run_pdf_visual_release_gate.ps1",
    "check_pdf_visual_release_gate_preflight.ps1"
)) {
    Assert-ContainsText -Text $governanceScriptText -ExpectedText $expectedText `
        -Message "Governance report should keep memory guard passthrough marker '$expectedText'."
}

$preflightScriptText = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $preflightScriptPath
    Get-Content -Raw -Encoding UTF8 -LiteralPath $preflightScriptHelperPath
) -join "`n"
foreach ($expectedText in @(
    "[int]`$MinFreeMemoryMB = 2048",
    "[switch]`$SkipMemoryGuard",
    "workstation_free_memory_available",
    "memory_guard_blocked"
)) {
    Assert-ContainsText -Text $preflightScriptText -ExpectedText $expectedText `
        -Message "Preflight script should keep memory guard contract marker '$expectedText'."
}

Write-Host "PDF visual release gate preflight governance report contract passed."
