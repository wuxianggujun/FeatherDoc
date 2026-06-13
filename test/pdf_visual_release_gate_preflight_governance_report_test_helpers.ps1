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
