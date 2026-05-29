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

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Get-CheckByName {
    param(
        [object]$Summary,
        [string]$Name
    )

    $checks = @($Summary.checks | Where-Object { [string]$_.name -eq $Name })
    if ($checks.Count -ne 1) {
        throw "Expected exactly one check named '$Name'."
    }
    return $checks[0]
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

$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_word_visual_release_gate_preflight.ps1"
$summaryPath = Join-Path $resolvedWorkingDir "word-visual-preflight-summary.json"
$reportPath = Join-Path $resolvedWorkingDir "word_visual_release_gate_preflight.md"

& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File $scriptPath `
    -RepoRoot $resolvedRepoRoot `
    -OutputJson $summaryPath `
    -ReportMarkdown $reportPath | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Word visual release gate preflight should pass for the current repository contract."
}

Assert-True -Condition (Test-Path -LiteralPath $summaryPath -PathType Leaf) `
    -Message "Preflight should write a JSON summary."
Assert-True -Condition (Test-Path -LiteralPath $reportPath -PathType Leaf) `
    -Message "Preflight should write a Markdown report."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.word_visual_release_gate_preflight.v1" `
    -Message "Preflight schema mismatch."
Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
    -Message "Current repository preflight should be ready."
Assert-True -Condition ([bool]$summary.preflight_ready) `
    -Message "preflight_ready should be true."
Assert-True -Condition (-not [bool]$summary.release_ready) `
    -Message "Static preflight should never claim release readiness."
Assert-Equal -Actual ([string]$summary.evidence_scope) -Expected "word_visual_release_gate_preflight_static_contract_only" `
    -Message "Evidence scope mismatch."
Assert-ContainsText -Text ([string]$summary.boundary) -ExpectedText "does not run Word, CMake, CTest" `
    -Message "Boundary should explicitly stay read-only."

foreach ($checkName in @(
        "word_visual_gate_scripts_exist",
        "word_visual_gate_parseable",
        "word_visual_gate_contract_markers",
        "word_visual_gate_core_flows_wired",
        "word_visual_gate_cmake_contract_registered",
        "word_visual_gate_docs_linked"
    )) {
    $check = Get-CheckByName -Summary $summary -Name $checkName
    Assert-Equal -Actual ([string]$check.status) -Expected "pass" `
        -Message "Check '$checkName' should pass."
}

$markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $reportPath
foreach ($marker in @(
        "Word Visual Release Gate Preflight",
        "featherdoc.word_visual_release_gate_preflight.v1",
        "word_visual_release_gate_preflight_static_contract_only",
        "word_visual_gate_core_flows_wired"
    )) {
    Assert-ContainsText -Text $markdown -ExpectedText $marker `
        -Message "Markdown report should preserve marker '$marker'."
}

$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
foreach ($forbiddenText in @(
        "New-Object -ComObject Word.Application",
        "ctest --test-dir",
        "cmake --build",
        "Start-Process"
    )) {
    Assert-True -Condition ($scriptText -notmatch [regex]::Escape($forbiddenText)) `
        -Message "Preflight script should not invoke heavyweight runtime marker '$forbiddenText'."
}

$fixtureRoot = Join-Path $resolvedWorkingDir "missing-contract-fixture"
New-Item -ItemType Directory -Path (Join-Path $fixtureRoot "scripts") -Force | Out-Null
$missingSummaryPath = Join-Path $resolvedWorkingDir "missing-contract-summary.json"
$missingReportPath = Join-Path $resolvedWorkingDir "missing-contract.md"
& powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
    -File $scriptPath `
    -RepoRoot $fixtureRoot `
    -OutputJson $missingSummaryPath `
    -ReportMarkdown $missingReportPath | Out-Host
if ($LASTEXITCODE -ne 0) {
    throw "Preflight should report not_ready without failing when -Strict is not set."
}

$missingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $missingSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$missingSummary.status) -Expected "not_ready" `
    -Message "Missing fixture preflight should be not_ready."
Assert-True -Condition ((@($missingSummary.blocking_checks) | ForEach-Object { [string]$_ }) -contains "word_visual_gate_scripts_exist") `
    -Message "Missing fixture should report missing scripts as a blocker."
Assert-True -Condition ((@($missingSummary.blocking_checks) | ForEach-Object { [string]$_ }) -contains "word_visual_gate_docs_linked") `
    -Message "Missing fixture should report missing docs as a blocker."

$strictSummaryPath = Join-Path $resolvedWorkingDir "missing-contract-strict-summary.json"
$strictReportPath = Join-Path $resolvedWorkingDir "missing-contract-strict.md"
$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    & powershell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass `
        -File $scriptPath `
        -RepoRoot $fixtureRoot `
        -OutputJson $strictSummaryPath `
        -ReportMarkdown $strictReportPath `
        -Strict | Out-Host
    $strictExitCode = $LASTEXITCODE
} finally {
    $ErrorActionPreference = $previousErrorActionPreference
}
Assert-True -Condition ($strictExitCode -ne 0) `
    -Message "Strict preflight should fail when static contract blockers remain."
Assert-True -Condition (Test-Path -LiteralPath $strictSummaryPath -PathType Leaf) `
    -Message "Strict failure should still write the summary JSON."

Write-Host "Word visual release gate preflight contract passed."
