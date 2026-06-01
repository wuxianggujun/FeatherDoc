param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "registered", "fail_on_unregistered")]
    [string]$Scenario = "all"
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

function Test-Scenario {
    param([string]$Name)
    return ($Scenario -eq "all" -or $Scenario -eq $Name)
}

function Write-JsonFile {
    param([string]$Path, $Value)

    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-DiscoveryScript {
    param([string[]]$Arguments)

    $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
    $powerShellPath = if ($powerShellCommand) { $powerShellCommand.Source } else { (Get-Process -Id $PID).Path }
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
        $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = (@($output | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\discover_project_template_smoke_candidates.ps1"
$samplesRoot = Join-Path $resolvedRepoRoot "samples"

if (Test-Scenario -Name "registered") {
    $scenarioDir = Join-Path $resolvedWorkingDir "registered"
    $manifestPath = Join-Path $scenarioDir "project_template_smoke.manifest.json"
    $outputPath = Join-Path $scenarioDir "candidate_discovery.json"

    Write-JsonFile -Path $manifestPath -Value ([ordered]@{
        entries = @(
            [ordered]@{
                name = "chinese-invoice-template"
                input_docx = "samples/chinese_invoice_template.docx"
            }
        )
        candidate_exclusions = @(
            [ordered]@{
                path = "samples/my_test.docx"
                reason = "Generic test fixture duplicate; not a project template smoke target."
            }
        )
    })

    $result = Invoke-DiscoveryScript -Arguments @(
        "-ManifestPath", $manifestPath,
        "-SearchRoot", $samplesRoot,
        "-Json",
        "-IncludeRegistered",
        "-IncludeExcluded",
        "-OutputPath", $outputPath,
        "-FailOnUnregistered"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Registered/excluded candidate discovery should pass with -FailOnUnregistered. Output: $($result.Text)"
    Assert-True -Condition (Test-Path -LiteralPath $outputPath) `
        -Message "Registered discovery should write a JSON report."

    $report = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputPath | ConvertFrom-Json
    Assert-Equal -Actual ([bool]$report.fail_on_unregistered) -Expected $true `
        -Message "Report should preserve fail_on_unregistered."
    Assert-Equal -Actual ([int]$report.unregistered_candidate_count) -Expected 0 `
        -Message "Registered discovery should have no unregistered candidates."
    Assert-Equal -Actual ([int]$report.registered_candidate_count) -Expected 1 `
        -Message "Registered discovery should include the registered sample candidate."
    Assert-Equal -Actual ([int]$report.visible_excluded_candidate_count) -Expected 1 `
        -Message "Registered discovery should show the excluded fixture when requested."
    Assert-ContainsText -Text ((@($report.candidates) | ForEach-Object { [string]$_.display_path }) -join "`n") `
        -ExpectedText ".\samples\chinese_invoice_template.docx" `
        -Message "Registered discovery should include the registered invoice template."
    Assert-ContainsText -Text ((@($report.candidates) | ForEach-Object { [string]$_.display_path }) -join "`n") `
        -ExpectedText ".\samples\my_test.docx" `
        -Message "Registered discovery should include the excluded fixture."
}

if (Test-Scenario -Name "fail_on_unregistered") {
    $scenarioDir = Join-Path $resolvedWorkingDir "fail-on-unregistered"
    $manifestPath = Join-Path $scenarioDir "project_template_smoke.manifest.json"
    $outputPath = Join-Path $scenarioDir "candidate_discovery.json"

    Write-JsonFile -Path $manifestPath -Value ([ordered]@{
        entries = @(
            [ordered]@{
                name = "chinese-invoice-template"
                input_docx = "samples/chinese_invoice_template.docx"
            }
        )
        candidate_exclusions = @()
    })

    $result = Invoke-DiscoveryScript -Arguments @(
        "-ManifestPath", $manifestPath,
        "-SearchRoot", $samplesRoot,
        "-Json",
        "-OutputPath", $outputPath,
        "-FailOnUnregistered"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Discovery should fail with -FailOnUnregistered when a tracked sample DOCX is unregistered. Output: $($result.Text)"
    Assert-True -Condition (Test-Path -LiteralPath $outputPath) `
        -Message "Fail-on-unregistered discovery should still write a JSON report."

    $report = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputPath | ConvertFrom-Json
    Assert-Equal -Actual ([bool]$report.fail_on_unregistered) -Expected $true `
        -Message "Failing report should preserve fail_on_unregistered."
    Assert-Equal -Actual ([int]$report.unregistered_candidate_count) -Expected 1 `
        -Message "Failing report should count the unregistered sample candidate."
    $candidate = @($report.candidates | Where-Object { [string]$_.display_path -eq ".\samples\my_test.docx" })[0]
    Assert-True -Condition ($null -ne $candidate) `
        -Message "Failing report should list the unregistered sample fixture."
    Assert-Equal -Actual ([bool]$candidate.registered) -Expected $false `
        -Message "Unregistered fixture should not be marked registered."
    Assert-Equal -Actual ([bool]$candidate.excluded) -Expected $false `
        -Message "Unregistered fixture should not be marked excluded."
    Assert-ContainsText -Text ([string]$candidate.suggested_register_command) `
        -ExpectedText "register_project_template_smoke_manifest_entry.ps1" `
        -Message "Unregistered fixture should include a register command."
    Assert-ContainsText -Text ([string]$candidate.suggested_register_command) `
        -ExpectedText ".\samples\my_test.docx" `
        -Message "Register command should point at the unregistered fixture."
}

Write-Host "Project template smoke candidate discovery regression passed."
