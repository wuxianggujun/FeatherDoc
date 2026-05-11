param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "fail_on_blocker")]
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

function Invoke-GovernanceReportScript {
    param([string[]]$Arguments)

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
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function Write-JsonFile {
    param(
        [string]$Path,
        $Value
    )

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_project_template_onboarding_governance_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$evidenceRoot = Join-Path $resolvedWorkingDir "evidence"
$onboardingSummaryPath = Join-Path $evidenceRoot "invoice\onboarding_summary.json"
$onboardingPlanPath = Join-Path $evidenceRoot "plan\plan.json"
$smokeSummaryPath = Join-Path $evidenceRoot "smoke\summary.json"

Write-JsonFile -Path $onboardingSummaryPath -Value ([ordered]@{
    template_name = "invoice-template"
    input_docx = "samples/invoice.docx"
    schema_approval_state = [ordered]@{
        status = "not_evaluated"
        gate_status = "not_evaluated"
        release_blocked = $true
        action = "run_project_template_smoke_then_review_schema_patch_approval"
        pending_count = 0
        approved_count = 0
        rejected_count = 0
        compliance_issue_count = 0
        invalid_result_count = 0
    }
    release_blockers = @(
        [ordered]@{
            id = "project_template_onboarding.schema_approval_not_evaluated"
            severity = "error"
            status = "not_evaluated"
            action = "run_project_template_smoke_then_review_schema_patch_approval"
            message = "Schema approval is not evaluated."
        }
    )
    action_items = @(
        [ordered]@{
            id = "run_project_template_smoke"
            action = "run_project_template_smoke_then_review_schema_patch_approval"
            title = "Run smoke"
            command = "pwsh -File .\scripts\run_project_template_smoke.ps1"
        }
    )
    manual_review_recommendations = @(
        [ordered]@{
            id = "review_schema_approval"
            priority = "high"
            action = "review_schema_update_candidate"
            title = "Review schema update candidate"
            artifact = "approval.json"
        }
    )
})

Write-JsonFile -Path $onboardingPlanPath -Value ([ordered]@{
    onboarding_entry_count = 1
    entries = @(
        [ordered]@{
            name = "contract-template"
            input_docx = "samples/contract.docx"
            schema_approval_state = [ordered]@{
                status = "not_evaluated"
                gate_status = "not_evaluated"
                release_blocked = $true
                action = "run_project_template_smoke_then_review_schema_patch_approval"
            }
            release_blockers = @(
                [ordered]@{
                    id = "project_template_onboarding.schema_approval_not_evaluated"
                    severity = "error"
                    status = "not_evaluated"
                    action = "run_project_template_smoke_then_review_schema_patch_approval"
                    message = "Candidate needs smoke."
                }
            )
            action_items = @(
                [ordered]@{
                    id = "freeze_schema_baseline"
                    action = "freeze_schema_baseline"
                    title = "Freeze schema"
                    command = "pwsh -File .\scripts\freeze_template_schema_baseline.ps1"
                }
            )
            manual_review_recommendations = @(
                [ordered]@{
                    id = "review_schema_baseline"
                    priority = "high"
                    action = "review_schema_baseline"
                    title = "Review schema baseline"
                    artifact = "contract.schema.json"
                }
            )
        }
    )
})

Write-JsonFile -Path $smokeSummaryPath -Value ([ordered]@{
    schema_patch_review_count = 1
    schema_patch_review_changed_count = 1
    schema_patch_approval_pending_count = 1
    schema_patch_approval_approved_count = 0
    schema_patch_approval_rejected_count = 0
    schema_patch_approval_compliance_issue_count = 0
    schema_patch_approval_invalid_result_count = 0
    schema_patch_approval_gate_status = "pending"
    schema_patch_approval_items = @(
        [ordered]@{
            name = "smoke-template"
            required = $true
            pending = $true
            approved = $false
            status = "pending_review"
            decision = "pending"
            action = "review_schema_update_candidate"
            compliance_issue_count = 0
            schema_update_candidate = "generated.schema.json"
            approval_result = "schema_patch_approval_result.json"
            review_json = "schema_patch_review.json"
        }
    )
})

if (Test-Scenario -Name "aggregate") {
    $outputDir = Join-Path $resolvedWorkingDir "aggregate-report"
    $result = Invoke-GovernanceReportScript -Arguments @(
        "-InputRoot", $evidenceRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Governance report should succeed without -FailOnBlocker. Output: $($result.Text)"
    Assert-ContainsText -Text $result.Text -ExpectedText "Summary JSON:" `
        -Message "Script should print summary path."

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "project_template_onboarding_governance.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Governance report should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Governance report should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Summary should expose the governance schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Summary should be blocked when entries expose release blockers."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Summary should not be release-ready with blockers."
    Assert-Equal -Actual ([int]$summary.entry_count) -Expected 3 `
        -Message "Summary should aggregate three template entries."
    Assert-Equal -Actual ([int]$summary.not_evaluated_entry_count) -Expected 2 `
        -Message "Summary should count not-evaluated entries."
    Assert-Equal -Actual ([int]$summary.pending_review_entry_count) -Expected 1 `
        -Message "Summary should count smoke pending-review entries."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 3 `
        -Message "Summary should expose release blockers from all blocked entries."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 2 `
        -Message "Summary should aggregate onboarding action items."
    Assert-Equal -Actual ([int]$summary.manual_review_recommendation_count) -Expected 2 `
        -Message "Summary should aggregate manual review recommendations."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Project Template Onboarding Governance Report" `
        -Message "Markdown should include a title."
    Assert-ContainsText -Text $markdown -ExpectedText "Schema Approval Status" `
        -Message "Markdown should expose status summary."
    Assert-ContainsText -Text $markdown -ExpectedText "contract-template" `
        -Message "Markdown should include onboarding plan entries."
    Assert-ContainsText -Text $markdown -ExpectedText "smoke-template" `
        -Message "Markdown should include smoke approval items."
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blockers" `
        -Message "Markdown should include release blockers."
}

if (Test-Scenario -Name "fail_on_blocker") {
    $failOutputDir = Join-Path $resolvedWorkingDir "fail-on-blocker-report"
    $result = Invoke-GovernanceReportScript -Arguments @(
        "-InputRoot", $evidenceRoot,
        "-OutputDir", $failOutputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Governance report should fail with -FailOnBlocker when blockers exist. Output: $($result.Text)"

    $summaryPath = Join-Path $failOutputDir "summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Failing governance report should still write summary.json."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Failing summary should preserve blocked status."
}

Write-Host "Project template onboarding governance report regression passed."
