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
    issue_keys = @("cmake_cache_exists", "ctest_manifest_exists")
    blocking_summary = [pscustomobject]@{
        required_check_count = 11
        blocking_check_count = 6
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
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "cmake_cache_exists" `
    -Message "PDF preflight build-output blocker should explain the missing CMakeCache preflight issue."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "CMakeCache.txt" `
    -Message "PDF preflight build-output blocker should require a reusable CMake build directory."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing CLI PDFs=2" `
    -Message "PDF preflight build-output blocker should include the missing CLI PDF count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing visual baseline PDFs=42" `
    -Message "PDF preflight build-output blocker should include the missing visual baseline count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "missing CJK text-layer PDFs=43" `
    -Message "PDF preflight build-output blocker should include the missing CJK text-layer count."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "required checks=11" `
    -Message "PDF preflight build-output blocker should include the memory-aware required check count."
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
Assert-ContainsText -Text $pdfPreflightChecklistGuidance -ExpectedText "memory guard blocked=false" `
    -Message "PDF preflight checklist guidance should include the memory guard blocked state."

$pdfPreflightUnavailableBlocker = [pscustomobject]@{
    id = "pdf_visual_release_gate_preflight.summary_unavailable"
    source = "pdf_visual_release_gate_preflight"
    severity = "error"
    status = "blocked"
    action = "rerun_pdf_visual_release_gate_preflight"
    source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
    source_json_display = ".\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
    command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json"
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
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "release note bundle" `
    -Message "PDF preflight summary-unavailable blocker should tell reviewers to refresh release materials."
Assert-ContainsText -Text $pdfPreflightUnavailableGuidance -ExpectedText "clean up only task-owned PDF gate processes" `
    -Message "PDF preflight summary-unavailable blocker should remind reviewers to clean up only task-owned resources."

Write-Host "Release blocker action registry self-check passed."
