param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

. (Join-Path $resolvedRepoRoot "scripts\release_blocker_metadata_helpers.ps1")

$registeredActions = @(Get-ReleaseBlockerRegisteredActions)
Assert-True -Condition ($registeredActions.Count -gt 0) `
    -Message "Release blocker action registry should not be empty."

$uniqueActions = @($registeredActions | Sort-Object -Unique)
Assert-True -Condition ($uniqueActions.Count -eq $registeredActions.Count) `
    -Message "Release blocker action registry should not contain duplicate actions."

foreach ($action in $registeredActions) {
    Assert-True -Condition (-not [string]::IsNullOrWhiteSpace($action)) `
        -Message "Release blocker action registry should not contain blank actions."
    Assert-True -Condition (Test-ReleaseBlockerActionRegistered -Action $action) `
        -Message "Registered release blocker action should resolve as registered: $action"

    $blocker = [pscustomobject]@{
        id = "test.$action"
        source = "release_blocker_action_registry_test"
        severity = "warning"
        status = "blocked"
        action = $action
        summary_json = Join-Path $resolvedWorkingDir "$action.summary.json"
        history_markdown = Join-Path $resolvedWorkingDir "$action.history.md"
    }

    $guidanceLines = @(Get-ReleaseBlockerActionGuidanceLines `
            -Blocker $blocker `
            -RepoRoot $resolvedRepoRoot `
            -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json"))
    Assert-True -Condition ($guidanceLines.Count -gt 0) `
        -Message "Registered release blocker action should produce a checklist runbook: $action"

    $guidanceText = $guidanceLines -join "`n"
    Assert-ContainsText -Text $guidanceText -ExpectedText $action `
        -Message "Registered release blocker action runbook should mention its action token: $action"
    Assert-True -Condition ($guidanceText -notmatch [regex]::Escape("Unregistered release blocker action")) `
        -Message "Registered release blocker action should not use the unregistered fallback: $action"
}

$unknownAction = "custom_unregistered_action"
Assert-True -Condition (-not (Test-ReleaseBlockerActionRegistered -Action $unknownAction)) `
    -Message "Unknown release blocker action should not resolve as registered."

$unknownGuidance = @(Get-ReleaseBlockerActionGuidanceLines -Blocker ([pscustomobject]@{ action = $unknownAction })) -join "`n"
Assert-ContainsText -Text $unknownGuidance -ExpectedText ("Unregistered release blocker action ``{0}``" -f $unknownAction) `
    -Message "Unknown release blocker action should render the unregistered runbook fallback."

$pdfPreflightBlocker = [pscustomobject]@{
    id = "pdf_visual_release_gate_preflight.build_outputs_missing"
    source = "pdf_visual_release_gate_preflight"
    severity = "error"
    status = "blocked"
    action = "prepare_pdf_visual_release_gate_build_outputs"
    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
    source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
    command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly"
    issue_keys = @("cmake_cache_exists", "ctest_manifest_exists", "pdf_build_options_enabled")
    output_gap_count = 3
    missing_output_count = 87
    pdf_dependency_inputs = [pscustomobject]@{
        status = "not_ready"
        selected_pdfium_provider = "unresolved"
        pdfio_ready = $false
        pdfium_ready = $false
        missing_input_count = 3
        missing_inputs = @(
            "PDFio source header: C:\repo\tmp\pdfio-src\pdfio.h",
            "PDFium source header: C:\repo\tmp\pdfium-workspace\pdfium\public\fpdfview.h",
            "PDFium input: provide prebuilt library/include inputs or a source checkout with public/fpdfview.h."
        )
    }
    readiness_action_evidence = @(
        [pscustomobject]@{
            action = "provide_pdf_dependency_input"
            issue_key = "pdf_dependency_inputs_ready"
            item = "PDFio source header"
            source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
        },
        [pscustomobject]@{
            action = "enable_pdf_build_option"
            issue_key = "pdf_build_options_enabled"
            item = "FEATHERDOC_BUILD_PDF_IMPORT"
            source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
        }
    )
    build_dir_auto_candidates = @(
        [pscustomobject]@{
            relative_path = "build"
            exists = $true
            cmake_cache_exists = $true
            ctest_manifest_exists = $true
            pdf_build_options_enabled = $false
            looks_reusable = $false
            pdf_build_options = @(
                [pscustomobject]@{
                    name = "FEATHERDOC_BUILD_PDF"
                    present = $true
                    value = "OFF"
                    enabled = $false
                },
                [pscustomobject]@{
                    name = "FEATHERDOC_BUILD_PDF_IMPORT"
                    present = $true
                    value = "OFF"
                    enabled = $false
                }
            )
        },
        [pscustomobject]@{
            relative_path = "out\build"
            exists = $false
            cmake_cache_exists = $false
            ctest_manifest_exists = $false
            pdf_build_options_enabled = $false
            looks_reusable = $false
            pdf_build_options = @(
                [pscustomobject]@{
                    name = "FEATHERDOC_BUILD_PDF"
                    present = $false
                    value = ""
                    enabled = $false
                },
                [pscustomobject]@{
                    name = "FEATHERDOC_BUILD_PDF_IMPORT"
                    present = $false
                    value = ""
                    enabled = $false
                }
            )
        }
    )
    blocking_summary = [pscustomobject]@{
        required_check_count = 12
        blocking_check_count = 7
        missing_cli_pdf_count = 2
        visual_baseline_sample_count = 42
        missing_visual_baseline_pdf_count = 42
        cjk_text_layer_sample_count = 43
        missing_cjk_text_layer_pdf_count = 43
        build_dir_entry_count = 1
        ctest_required_pattern_count = 4
        memory_guard_blocked = $false
        memory_guard_skipped = $false
        free_memory_mb = 1140
        min_free_memory_mb = 2048
        pdf_build_options_enabled = $false
        disabled_pdf_build_options = @("FEATHERDOC_BUILD_PDF", "FEATHERDOC_BUILD_PDF_IMPORT")
        missing_pdf_build_options = @()
        pdf_dependency_inputs_status = "not_ready"
        selected_pdfium_provider = "unresolved"
        pdfio_dependency_ready = $false
        pdfium_dependency_ready = $false
        pdf_dependency_missing_input_count = 3
    }
}
$pdfPreflightGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $pdfPreflightBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "prepare_pdf_visual_release_gate_build_outputs" `
    -Message "PDF preflight build-output blocker should render its fixed action runbook."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "run_pdf_visual_release_gate.ps1" `
    -Message "PDF preflight build-output blocker should point at the PDF visual gate wrapper."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "-PreflightOnly" `
    -Message "PDF preflight build-output blocker should keep the first reviewer step lightweight."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "not release-ready evidence" `
    -Message "PDF preflight build-output blocker should state that preflight-only is not release-ready evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "cmake_cache_exists" `
    -Message "PDF preflight build-output blocker should explain the missing CMakeCache preflight issue."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "CMakeCache.txt" `
    -Message "PDF preflight build-output blocker should require a reusable CMake build directory."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "pdf_build_options_enabled" `
    -Message "PDF preflight build-output blocker should explain disabled PDF build options."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "-DFEATHERDOC_BUILD_PDF=ON" `
    -Message "PDF preflight build-output blocker should include the PDF writer build option."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "-DFEATHERDOC_BUILD_PDF_IMPORT=ON" `
    -Message "PDF preflight build-output blocker should include the PDF import build option."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing CLI PDFs=2" `
    -Message "PDF preflight build-output blocker should include the missing CLI PDF count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing visual baseline PDFs=42" `
    -Message "PDF preflight build-output blocker should include the missing visual baseline count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing CJK text-layer PDFs=43" `
    -Message "PDF preflight build-output blocker should include the missing CJK text-layer count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "output gap checks=3" `
    -Message "PDF preflight build-output blocker should include the output gap group count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing outputs=87" `
    -Message "PDF preflight build-output blocker should include the missing output total."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "required checks=12" `
    -Message "PDF preflight build-output blocker should include the memory-aware required check count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDF build options: enabled=false" `
    -Message "PDF preflight build-output blocker should summarize PDF build option readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "disabled=FEATHERDOC_BUILD_PDF,FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "PDF preflight build-output blocker should list disabled PDF build options."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDF preflight build auto candidates" `
    -Message "PDF preflight build-output blocker should summarize auto build candidate readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "build(exists=true, CMakeCache=true, CTest=true, PDF options=false, reusable=false" `
    -Message "PDF preflight build-output blocker should show the build candidate is rejected by PDF options."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "FEATHERDOC_BUILD_PDF(present=true,value=OFF,enabled=false)" `
    -Message "PDF preflight build-output blocker should show the writer option snapshot for build candidate."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT(present=true,value=OFF,enabled=false)" `
    -Message "PDF preflight build-output blocker should show the import option snapshot for build candidate."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDF dependency inputs: status=not_ready" `
    -Message "PDF preflight build-output blocker should summarize PDF dependency input readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "selected PDFium provider=unresolved" `
    -Message "PDF preflight build-output blocker should show the selected PDFium provider."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDFio ready=false" `
    -Message "PDF preflight build-output blocker should show PDFio readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDFium ready=false" `
    -Message "PDF preflight build-output blocker should show PDFium readiness."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing inputs=3" `
    -Message "PDF preflight build-output blocker should show the missing dependency input count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "PDFium source header" `
    -Message "PDF preflight build-output blocker should show dependency missing input preview."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "Readiness action evidence" `
    -Message "PDF preflight build-output blocker should summarize readiness action evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "provide_pdf_dependency_input/pdf_dependency_inputs_ready -> PDFio source header" `
    -Message "PDF preflight build-output blocker should show dependency input readiness evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "enable_pdf_build_option/pdf_build_options_enabled -> FEATHERDOC_BUILD_PDF_IMPORT" `
    -Message "PDF preflight build-output blocker should show disabled build option readiness evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "source JSON: .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json" `
    -Message "PDF preflight build-output blocker should keep readiness evidence traceable to source JSON."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "memory guard blocked=false" `
    -Message "PDF preflight build-output blocker should include the memory guard blocked state."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "memory guard skipped=false" `
    -Message "PDF preflight build-output blocker should include the memory guard skipped state."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "free memory MB=1140" `
    -Message "PDF preflight build-output blocker should include the observed free memory."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "minimum free memory MB=2048" `
    -Message "PDF preflight build-output blocker should include the configured minimum free memory."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "source preflight JSON" `
    -Message "PDF preflight build-output blocker should point reviewers at source_json_display evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "release note bundle" `
    -Message "PDF preflight build-output blocker should tell reviewers to refresh release materials."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "clean up only task-owned PDF gate processes" `
    -Message "PDF preflight build-output blocker should remind reviewers to clean up only task-owned resources."

$pdfPreflightChecklistGuidance = @(Get-ReleaseGovernanceChecklistGuidanceLines `
        -Item $pdfPreflightBlocker `
        -ItemKind "action item" `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing CLI PDFs=2" `
    -Message "PDF preflight checklist guidance should include the missing CLI PDF count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing visual baseline PDFs=42" `
    -Message "PDF preflight checklist guidance should include the missing visual baseline count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing CJK text-layer PDFs=43" `
    -Message "PDF preflight checklist guidance should include the missing CJK text-layer count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "output gap checks=3" `
    -Message "PDF preflight checklist guidance should include the output gap group count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing outputs=87" `
    -Message "PDF preflight checklist guidance should include the missing output total."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "memory guard blocked=false" `
    -Message "PDF preflight checklist guidance should include the memory guard blocked state."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "PDF preflight build auto candidates" `
    -Message "PDF preflight checklist guidance should summarize auto build candidate readiness."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "FEATHERDOC_BUILD_PDF_IMPORT(present=true,value=OFF,enabled=false)" `
    -Message "PDF preflight checklist guidance should show candidate PDF option snapshots."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "PDF dependency inputs: status=not_ready" `
    -Message "PDF preflight checklist guidance should summarize PDF dependency input readiness."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "missing inputs=3" `
    -Message "PDF preflight checklist guidance should show missing dependency input count."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "Readiness action evidence" `
    -Message "PDF preflight checklist guidance should summarize readiness action evidence."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "provide_pdf_dependency_input/pdf_dependency_inputs_ready -> PDFio source header" `
    -Message "PDF preflight checklist guidance should show dependency input readiness evidence."
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "not release-ready evidence" `
    -Message "PDF preflight checklist guidance should state that preflight-only is not release-ready evidence."

$controlledVisualSmokeWarning = [pscustomobject]@{
    id = "pdf_controlled_visual_smoke.unavailable_or_failed"
    source = "pdf_controlled_visual_smoke"
    severity = "warning"
    status = "fail"
    action = "rerun_pdf_controlled_visual_smoke_check"
    message = "Controlled PDF visual smoke evidence was provided but is not passing."
    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
    source_report_display = ".\output\pdf-visual-release-gate-preflight-governance\summary.json"
    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\controlled-visual-smoke-failed.json"
    error_message = "Controlled smoke page was blank."
}
$controlledVisualSmokeGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $controlledVisualSmokeWarning `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "rerun_pdf_controlled_visual_smoke_check" `
    -Message "Controlled PDF visual smoke warning should render its fixed action runbook."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "check_pdf_controlled_visual_smoke.ps1" `
    -Message "Controlled PDF visual smoke runbook should point at the smoke check wrapper."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "controlled-visual-smoke-failed.json" `
    -Message "Controlled PDF visual smoke runbook should keep source JSON evidence visible."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "read-only" `
    -Message "Controlled PDF visual smoke runbook should state the check is read-only."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "does not run CMake, CTest, Word" `
    -Message "Controlled PDF visual smoke runbook should keep heavyweight runtime boundaries explicit."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "not release-ready evidence" `
    -Message "Controlled PDF visual smoke runbook should state the evidence boundary."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "write_pdf_visual_release_gate_preflight_governance_report.ps1" `
    -Message "Controlled PDF visual smoke runbook should tell reviewers to rebuild governance evidence."
Assert-ContainsText -Text $controlledVisualSmokeGuidance -ExpectedText "release note bundle" `
    -Message "Controlled PDF visual smoke runbook should tell reviewers to refresh release materials."
Assert-True -Condition ($controlledVisualSmokeGuidance -notmatch [regex]::Escape("Unregistered release blocker action")) `
    -Message "Controlled PDF visual smoke registered action should not use the unregistered fallback."

$controlledVisualSmokeChecklistGuidance = @(Get-ReleaseGovernanceWarningActionGuidanceLines `
        -Warning $controlledVisualSmokeWarning `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $controlledVisualSmokeChecklistGuidance -ExpectedText "release governance warning" `
    -Message "Controlled PDF visual smoke checklist guidance should identify warning context."
Assert-ContainsText -Text $controlledVisualSmokeChecklistGuidance -ExpectedText "check_pdf_controlled_visual_smoke.ps1" `
    -Message "Controlled PDF visual smoke checklist guidance should point at the smoke check wrapper."
Assert-ContainsText -Text $controlledVisualSmokeChecklistGuidance -ExpectedText "controlled-visual-smoke-failed.json" `
    -Message "Controlled PDF visual smoke checklist guidance should keep source JSON visible."
Assert-ContainsText -Text $controlledVisualSmokeChecklistGuidance -ExpectedText "not release-ready evidence" `
    -Message "Controlled PDF visual smoke checklist guidance should state the evidence boundary."

$pdfPreflightUnavailableBlocker = [pscustomobject]@{
    id = "pdf_visual_release_gate_preflight.summary_unavailable"
    source = "pdf_visual_release_gate_preflight"
    severity = "error"
    status = "blocked"
    action = "rerun_pdf_visual_release_gate_preflight"
    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
    command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
    readiness_action_evidence = @(
        [pscustomobject]@{
            action = "rerun_pdf_visual_release_gate_preflight"
            issue_key = "summary_unavailable"
            item = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
            source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
        }
    )
}
$pdfPreflightUnavailableGuidance = @(Get-ReleaseBlockerActionGuidanceLines `
        -Blocker $pdfPreflightUnavailableBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "release-summary.json")) -join "`n"
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "rerun_pdf_visual_release_gate_preflight" `
    -Message "PDF preflight summary-unavailable blocker should render its fixed action runbook."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "check_pdf_visual_release_gate_preflight.ps1" `
    -Message "PDF preflight summary-unavailable blocker should point at lightweight preflight regeneration."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "write_pdf_visual_release_gate_preflight_governance_report.ps1" `
    -Message "PDF preflight summary-unavailable blocker should tell reviewers to rebuild governance evidence."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "Readiness action evidence" `
    -Message "PDF preflight summary-unavailable blocker should summarize readiness action evidence."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "rerun_pdf_visual_release_gate_preflight/summary_unavailable -> .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json" `
    -Message "PDF preflight summary-unavailable blocker should keep its rerun readiness evidence visible."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "not release-ready evidence" `
    -Message "PDF preflight summary-unavailable blocker should state that preflight summary is not release-ready evidence."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "release note bundle" `
    -Message "PDF preflight summary-unavailable blocker should tell reviewers to refresh release materials."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "clean up only task-owned PDF gate processes" `
    -Message "PDF preflight summary-unavailable blocker should remind reviewers to clean up only task-owned resources."

Write-Host "Release blocker action registry self-check passed."
