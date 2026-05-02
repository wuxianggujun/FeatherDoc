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

Write-Host "Release blocker action registry self-check passed."
