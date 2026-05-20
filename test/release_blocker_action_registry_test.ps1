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
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "source preflight JSON" `
    -Message "PDF preflight build-output blocker should point reviewers at source_json_display evidence."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "release note bundle" `
    -Message "PDF preflight build-output blocker should tell reviewers to refresh release materials."
Assert-ContainsText -Text $pdfPreflightGuidance -ExpectedText "clean up only task-owned PDF gate processes" `
    -Message "PDF preflight build-output blocker should remind reviewers to clean up only task-owned resources."

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
