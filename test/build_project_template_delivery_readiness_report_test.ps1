param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "ready", "malformed", "fail_on_blocker")]
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

function Invoke-ReadinessScript {
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
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-BlockedEvidence {
    param([string]$Root)

    $governancePath = Join-Path $Root "governance\summary.json"
    $historyPath = Join-Path $Root "history\history.json"

    Write-JsonFile -Path $governancePath -Value ([ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = "blocked"
        release_ready = $false
        entries = @(
            [ordered]@{
                name = "invoice-template"
                input_docx = "samples/invoice.docx"
                source_kind = "onboarding_summary"
                schema_approval_state = [ordered]@{
                    status = "pending_review"
                    gate_status = "pending"
                    release_blocked = $true
                    action = "review_schema_update_candidate"
                }
                schema_approval_status = "pending_review"
                release_blocked = $true
                release_blockers = @(
                    [ordered]@{
                        id = "project_template_onboarding.schema_approval"
                        severity = "error"
                        status = "pending_review"
                        action = "review_schema_update_candidate"
                        message = "Schema approval is pending."
                    }
                )
                action_items = @(
                    [ordered]@{
                        id = "review_invoice_schema"
                        action = "review_schema_update_candidate"
                        title = "Review invoice schema"
                        command = "pwsh -File .\scripts\sync_project_template_schema_approval.ps1"
                    }
                )
                manual_review_recommendations = @(
                    [ordered]@{
                        id = "manual_invoice_schema_review"
                        priority = "high"
                        action = "review_schema_update_candidate"
                        title = "Review invoice schema"
                        artifact = "invoice.schema.json"
                    }
                )
            },
            [ordered]@{
                name = "statement-template"
                input_docx = "samples/statement.docx"
                source_kind = "onboarding_summary"
                schema_approval_state = [ordered]@{
                    status = "approved"
                    gate_status = "passed"
                    release_blocked = $false
                    action = "promote_schema_update_candidate"
                }
                schema_approval_status = "approved"
                release_blocked = $false
                release_blockers = @()
                action_items = @()
                manual_review_recommendations = @()
            }
        )
    })

    Write-JsonFile -Path $historyPath -Value ([ordered]@{
        schema = "featherdoc.project_template_schema_approval_history.v1"
        generated_at = "2026-05-01T12:00:00"
        summary_count = 1
        latest_gate_status = "pending"
        blocked_run_count = 0
        pending_run_count = 1
        passed_run_count = 0
        entry_histories = @(
            [ordered]@{
                name = "invoice-template"
                run_count = 1
                blocked_run_count = 0
                pending_run_count = 1
                approved_run_count = 0
                latest_generated_at = "2026-05-01T12:00:00"
                latest_status = "pending_review"
                latest_decision = "pending"
                latest_action = "review_schema_update_candidate"
                latest_summary_json = "output/project-template-smoke/summary.json"
                issue_keys = @()
            },
            [ordered]@{
                name = "statement-template"
                run_count = 1
                blocked_run_count = 0
                pending_run_count = 0
                approved_run_count = 1
                latest_generated_at = "2026-05-01T12:00:00"
                latest_status = "approved"
                latest_decision = "approved"
                latest_action = "promote_schema_update_candidate"
                latest_summary_json = "output/project-template-smoke/summary.json"
                issue_keys = @()
            }
        )
    })

    return [pscustomobject]@{
        Governance = $governancePath
        History = $historyPath
    }
}

function New-ReadyEvidence {
    param([string]$Root)

    $governancePath = Join-Path $Root "governance\summary.json"
    $historyPath = Join-Path $Root "history\history.json"

    Write-JsonFile -Path $governancePath -Value ([ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = "ready"
        release_ready = $true
        entries = @(
            [ordered]@{
                name = "invoice-template"
                input_docx = "samples/invoice.docx"
                schema_approval_state = [ordered]@{
                    status = "approved"
                    gate_status = "passed"
                    release_blocked = $false
                    action = "promote_schema_update_candidate"
                }
                schema_approval_status = "approved"
                release_blocked = $false
                release_blockers = @()
                action_items = @()
                manual_review_recommendations = @()
            }
        )
    })

    Write-JsonFile -Path $historyPath -Value ([ordered]@{
        schema = "featherdoc.project_template_schema_approval_history.v1"
        generated_at = "2026-05-02T12:00:00"
        summary_count = 1
        latest_gate_status = "passed"
        blocked_run_count = 0
        pending_run_count = 0
        passed_run_count = 1
        entry_histories = @(
            [ordered]@{
                name = "invoice-template"
                run_count = 1
                blocked_run_count = 0
                pending_run_count = 0
                approved_run_count = 1
                latest_generated_at = "2026-05-02T12:00:00"
                latest_status = "approved"
                latest_decision = "approved"
                latest_action = "promote_schema_update_candidate"
                latest_summary_json = "output/project-template-smoke/summary.json"
                issue_keys = @()
            }
        )
    })

    return [pscustomobject]@{
        Governance = $governancePath
        History = $historyPath
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_project_template_delivery_readiness_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "aggregate") {
    $evidence = New-BlockedEvidence -Root (Join-Path $resolvedWorkingDir "aggregate-evidence")
    $outputDir = Join-Path $resolvedWorkingDir "aggregate-report"
    $result = Invoke-ReadinessScript -Arguments @(
        "-InputJson", "$($evidence.Governance),$($evidence.History)",
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Readiness report should succeed without -FailOnBlocker. Output: $($result.Text)"
    Assert-ContainsText -Text $result.Text -ExpectedText "Summary JSON:" `
        -Message "Script should print summary path."

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "project_template_delivery_readiness.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Readiness report should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Readiness report should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.project_template_delivery_readiness_report.v1" `
        -Message "Summary should expose the readiness schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Summary should be blocked when onboarding or history evidence blocks release."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Summary should not be release-ready with blockers."
    Assert-Equal -Actual ([int]$summary.template_count) -Expected 2 `
        -Message "Summary should aggregate two templates."
    Assert-Equal -Actual ([int]$summary.ready_template_count) -Expected 1 `
        -Message "Summary should count ready templates."
    Assert-Equal -Actual ([int]$summary.blocked_template_count) -Expected 1 `
        -Message "Summary should count blocked templates."
    Assert-Equal -Actual ([int]$summary.schema_history_count) -Expected 1 `
        -Message "Summary should count schema approval history reports."
    Assert-Equal -Actual ([string]$summary.latest_schema_approval_gate_status) -Expected "pending" `
        -Message "Summary should expose the latest history gate status."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 3 `
        -Message "Summary should include onboarding, template-history, and global history blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 1 `
        -Message "Summary should aggregate onboarding action items."
    Assert-Equal -Actual ([int]$summary.manual_review_recommendation_count) -Expected 1 `
        -Message "Summary should aggregate manual review recommendations."

    $invoice = $summary.templates | Where-Object { $_.template_name -eq "invoice-template" } | Select-Object -First 1
    Assert-Equal -Actual ([bool]$invoice.schema_history_available) -Expected $true `
        -Message "Template should be linked to matching schema history."
    Assert-Equal -Actual ([string]$invoice.schema_history.status) -Expected "pending_review" `
        -Message "Template history should preserve pending readiness status."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Project Template Delivery Readiness Report" `
        -Message "Markdown should include a title."
    Assert-ContainsText -Text $markdown -ExpectedText "Schema Approval History" `
        -Message "Markdown should include schema approval history."
    Assert-ContainsText -Text $markdown -ExpectedText "invoice-template" `
        -Message "Markdown should include blocked template names."
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blockers" `
        -Message "Markdown should include release blockers."
}

if (Test-Scenario -Name "ready") {
    $evidence = New-ReadyEvidence -Root (Join-Path $resolvedWorkingDir "ready-evidence")
    $outputDir = Join-Path $resolvedWorkingDir "ready-report"
    $result = Invoke-ReadinessScript -Arguments @(
        "-InputJson", "$($evidence.Governance),$($evidence.History)",
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Readiness report should pass -FailOnBlocker for ready evidence. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Ready evidence should produce ready status."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Ready evidence should be release-ready."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Ready evidence should not expose blockers."
    Assert-Equal -Actual ([string]$summary.latest_schema_approval_gate_status) -Expected "passed" `
        -Message "Ready evidence should expose passed history gate."
}

if (Test-Scenario -Name "malformed") {
    $evidenceRoot = Join-Path $resolvedWorkingDir "malformed-evidence"
    New-Item -ItemType Directory -Path $evidenceRoot -Force | Out-Null
    $badJson = Join-Path $evidenceRoot "summary.json"
    "{ not valid json" | Set-Content -LiteralPath $badJson -Encoding UTF8
    $outputDir = Join-Path $resolvedWorkingDir "malformed-report"
    $result = Invoke-ReadinessScript -Arguments @(
        "-InputJson", $badJson,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Malformed input should make the readiness report fail. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "failed" `
        -Message "Malformed input should produce failed status."
    Assert-Equal -Actual ([int]$summary.source_failure_count) -Expected 1 `
        -Message "Malformed input should count one source failure."
    Assert-Equal -Actual ([string]$summary.source_files[0].status) -Expected "failed" `
        -Message "Malformed input should mark the source as failed."
}

if (Test-Scenario -Name "fail_on_blocker") {
    $evidence = New-BlockedEvidence -Root (Join-Path $resolvedWorkingDir "fail-on-blocker-evidence")
    $outputDir = Join-Path $resolvedWorkingDir "fail-on-blocker-report"
    $result = Invoke-ReadinessScript -Arguments @(
        "-InputJson", "$($evidence.Governance),$($evidence.History)",
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Readiness report should fail with -FailOnBlocker when blockers exist. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Failing readiness report should preserve blocked status."
    Assert-True -Condition ([int]$summary.release_blocker_count -gt 0) `
        -Message "Failing readiness report should write blockers."
}

Write-Host "Project template delivery readiness report regression passed."
