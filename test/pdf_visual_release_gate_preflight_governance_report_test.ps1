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
$rollupScriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_blocker_rollup_report.ps1"

$tokens = $null
$errors = $null
[System.Management.Automation.Language.Parser]::ParseFile($scriptPath, [ref]$tokens, [ref]$errors) | Out-Null
if ($errors.Count -gt 0) {
    throw "PDF visual preflight governance report script has parse errors."
}

$notReadyPreflightPath = Join-Path $resolvedWorkingDir "fixtures\not-ready-preflight.json"
$readyPreflightPath = Join-Path $resolvedWorkingDir "fixtures\ready-preflight.json"

Write-JsonFile -Path $notReadyPreflightPath -Value ([ordered]@{
    generated_at = "2026-05-20T00:00:00"
    status = "not_ready"
    strict = $false
    repo_root = $resolvedRepoRoot
    build_dir = Join-Path $resolvedRepoRoot ".bpdf-roundtrip-msvc"
    build_dir_source = "auto:build"
    requested_build_dir = Join-Path $resolvedRepoRoot ".bpdf-roundtrip-msvc"
    checks = @(
        [ordered]@{
            name = "build_dir_exists"
            status = "missing"
            required = $true
            message = "Build directory is missing."
        },
        [ordered]@{
            name = "ctest_manifest_exists"
            status = "missing"
            required = $true
            message = "CTestTestfile.cmake is missing."
        },
        [ordered]@{
            name = "render_python_reusable"
            status = "pass"
            required = $true
            message = "A reusable render Python with PIL and fitz is available."
        }
    )
    blocking_checks = @(
        "build_dir_exists",
        "ctest_manifest_exists"
    )
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
})

$blockedOutputDir = Join-Path $resolvedWorkingDir "blocked-report"
$blockedResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PreflightJson", $notReadyPreflightPath,
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
Assert-Equal -Actual ([int]$blockedSummary.release_blocker_count) -Expected 1 `
    -Message "Not-ready preflight should emit one release blocker."
Assert-Equal -Actual ([int]$blockedSummary.action_item_count) -Expected 1 `
    -Message "Not-ready preflight should emit one action item."
Assert-Equal -Actual ([int]$blockedSummary.blocking_check_count) -Expected 2 `
    -Message "Governance report should preserve blocking check count."
Assert-Equal -Actual ([string]$blockedSummary.build_dir_source) -Expected "auto:build" `
    -Message "Governance report should preserve the selected preflight build-dir source."
Assert-ContainsText -Text ([string]$blockedSummary.requested_build_dir_display) `
    -ExpectedText ".bpdf-roundtrip-msvc" `
    -Message "Governance report should preserve the originally requested build directory."

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
Assert-ContainsText -Text ([string]$blocker.command_template) `
    -ExpectedText "run_pdf_visual_release_gate.ps1" `
    -Message "Blocker should include the preflight-only command template."
Assert-ContainsText -Text ([string]$blocker.command_template) `
    -ExpectedText "-PreflightOnly" `
    -Message "Blocker command should not run the full PDF visual gate."
Assert-ContainsText -Text (($blocker.issue_keys | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "ctest_manifest_exists" `
    -Message "Blocker should preserve individual failing preflight checks."

$blockedMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $blockedMarkdownPath
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "PDF Visual Release Gate Preflight Governance Report" `
    -Message "Markdown should include the report title."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "build_outputs_missing" `
    -Message "Markdown should include the blocker id."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "ctest_manifest_exists" `
    -Message "Markdown should include blocking checks."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "Build dir source" `
    -Message "Markdown should include the selected build-dir source."
Assert-ContainsText -Text $blockedMarkdown `
    -ExpectedText "auto:build" `
    -Message "Markdown should preserve the selected build-dir source value."

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

$readyOutputDir = Join-Path $resolvedWorkingDir "ready-report"
$readyResult = Invoke-PowerShellScript -ScriptPath $scriptPath -Arguments @(
    "-PreflightJson", $readyPreflightPath,
    "-OutputDir", $readyOutputDir,
    "-BuildDir", ".bpdf-roundtrip-msvc"
)
Assert-Equal -Actual $readyResult.ExitCode -Expected 0 `
    -Message "Ready governance report should succeed. Output: $($readyResult.Text)"
$readySummary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $readyOutputDir "summary.json") | ConvertFrom-Json
Assert-Equal -Actual ([string]$readySummary.status) -Expected "ready" `
    -Message "Ready preflight should produce a ready governance report."
Assert-Equal -Actual ([bool]$readySummary.release_ready) -Expected $true `
    -Message "Ready preflight should be release-ready."
Assert-Equal -Actual ([int]$readySummary.release_blocker_count) -Expected 0 `
    -Message "Ready preflight should not emit blockers."
Assert-Equal -Actual ([int]$readySummary.action_item_count) -Expected 0 `
    -Message "Ready preflight should not emit action items."

Write-Host "PDF visual release gate preflight governance report contract passed."
