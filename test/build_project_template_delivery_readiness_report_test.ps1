param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "ready", "malformed", "fail_on_blocker", "missing_inputs", "manifest_description", "smoke_summary", "direct_onboarding_evidence")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_project_template_delivery_readiness_report_test"
}

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

function Assert-HasProperty {
    param($Object, [string]$Name, [string]$Message)
    if (-not ($Object.PSObject.Properties.Name -contains $Name)) {
        throw "$Message Missing property '$Name'."
    }
}

function Assert-NonEmptyString {
    param($Value, [string]$Message)
    if ([string]::IsNullOrWhiteSpace([string]$Value)) {
        throw $Message
    }
}

function Get-TraceItemLabel {
    param($Item)

    foreach ($property in @("id", "template_name", "action")) {
        if ($Item.PSObject.Properties.Name -contains $property) {
            $value = [string]$Item.$property
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                return $value
            }
        }
    }

    return "<missing-id>"
}

function Assert-GovernanceTraceMetadata {
    param(
        [object[]]$Items,
        [string]$CollectionName,
        [bool]$ExpectOpenCommandProperty = $false
    )

    foreach ($item in @($Items)) {
        $itemLabel = Get-TraceItemLabel -Item $item
        $context = "Project-template readiness $CollectionName item $itemLabel"

        foreach ($property in @("source_schema", "source_report_display", "source_json_display")) {
            Assert-HasProperty -Object $item -Name $property `
                -Message "$context should expose $property."
            Assert-NonEmptyString -Value $item.$property `
                -Message "$context should keep non-empty $property."
        }

        if ($ExpectOpenCommandProperty) {
            Assert-HasProperty -Object $item -Name "open_command" `
                -Message "$context should expose reviewer open command metadata."
            Assert-NonEmptyString -Value $item.open_command `
                -Message "$context should keep a non-empty reviewer open command."
        }
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
        next_action = [ordered]@{
            action = "review_schema_update_candidate"
            status = "pending_review"
            blocker_id = "project_template_onboarding.schema_approval"
            reason = "Schema approval is pending."
        }
        next_action_summary = @(
            [ordered]@{
                action = "review_schema_update_candidate"
                status = "pending_review"
                blocker_id = "project_template_onboarding.schema_approval"
                reason = "Schema approval is pending."
            }
        )
        next_action_group_count = 1
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
        next_action = [ordered]@{
            action = "publish_project_template"
            status = "ready"
            blocker_id = ""
            reason = "Project template onboarding governance is release-ready."
        }
        next_action_summary = @(
            [ordered]@{
                action = "publish_project_template"
                status = "ready"
                blocker_id = ""
                reason = "Project template onboarding governance is release-ready."
            }
        )
        next_action_group_count = 1
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
    Assert-Equal -Actual ([int]$summary.onboarding_governance_next_action_group_count) -Expected 1 `
        -Message "Summary should expose onboarding governance next-action group count."
    Assert-Equal -Actual ([string]$summary.onboarding_governance_next_action.action) -Expected "review_schema_update_candidate" `
        -Message "Summary should preserve the onboarding governance next action."

    $onboardingBlockers = @($summary.release_blockers | Where-Object { [string]$_.id -eq "project_template_onboarding.schema_approval" })
    Assert-Equal -Actual $onboardingBlockers.Count -Expected 1 `
        -Message "Summary should retain the onboarding-derived release blocker."
    Assert-Equal -Actual ([string]$onboardingBlockers[0].source_schema) -Expected "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Onboarding-derived blockers should retain the onboarding governance source schema."
    Assert-ContainsText -Text ([string]$onboardingBlockers[0].source_json_display) `
        -ExpectedText "governance\summary.json" `
        -Message "Onboarding-derived blockers should retain the onboarding governance source JSON display."
    Assert-ContainsText -Text ([string]$onboardingBlockers[0].source_report_display) `
        -ExpectedText "governance\summary.json" `
        -Message "Onboarding-derived blockers should retain the onboarding governance source report display."
    Assert-Equal -Actual ([string]$onboardingBlockers[0].blocker_id) -Expected "project_template_onboarding.schema_approval" `
        -Message "Onboarding-derived blockers should retain a stable blocker_id."
    Assert-Equal -Actual ([string]$onboardingBlockers[0].onboarding_governance_next_action.action) -Expected "review_schema_update_candidate" `
        -Message "Onboarding-derived blockers should retain onboarding governance next action."
    Assert-Equal -Actual ([int]$onboardingBlockers[0].onboarding_governance_next_action_group_count) -Expected 1 `
        -Message "Onboarding-derived blockers should retain onboarding governance next-action group count."
    $historyGateBlocker = @(
        $summary.release_blockers |
            Where-Object { [string]$_.id -eq "project_template_delivery_readiness.schema_approval_history_gate" }
    )[0]
    Assert-ContainsText -Text ([string]$historyGateBlocker.source_json_display) `
        -ExpectedText "history\history.json" `
        -Message "History gate blockers should retain the schema approval history source JSON display."
    Assert-ContainsText -Text ([string]$historyGateBlocker.source_report_display) `
        -ExpectedText "history\history.json" `
        -Message "History gate blockers should retain the schema approval history source report display."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.project_template_delivery_readiness_report.v1" `
        -Message "Delivery-generated blockers should expose the delivery readiness source schema."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
        -ExpectedText "summary.json" `
        -Message "Release blockers should expose reviewer source JSON display paths."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
        -ExpectedText "summary.json" `
        -Message "Release blockers should expose reviewer source report display paths."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Onboarding-derived action items should retain the onboarding governance source schema."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
        -ExpectedText "governance\summary.json" `
        -Message "Action items should expose the onboarding governance source report display."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "sync_project_template_schema_approval.ps1" `
        -Message "Action items should expose the reviewer open command."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.reason }) -join "`n") `
        -ExpectedText "Review invoice schema" `
        -Message "Action items should expose reviewer reason text."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.onboarding_governance_next_action.action }) -join "`n") `
        -ExpectedText "review_schema_update_candidate" `
        -Message "Action items should retain onboarding governance next action."
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "release_blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "action_items" `
        -ExpectOpenCommandProperty $true

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
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display=" `
        -Message "Markdown should include source JSON display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
        -Message "Markdown should include source report display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
        -Message "Markdown should include action item open commands."
}

function New-SmokeSummaryEvidence {
    param([string]$Root)

    $smokePath = Join-Path $Root "project-template-smoke\summary.json"
    $historyPath = Join-Path $Root "project-template-schema-approval-history\history.json"
    Write-JsonFile -Path $smokePath -Value ([ordered]@{
        schema = "featherdoc.project_template_smoke_summary.v1"
        generated_at = "2026-05-27T12:30:00"
        manifest_path = "samples/project_template_smoke.manifest.json"
        workspace = $resolvedRepoRoot
        build_dir = "build-project-template-smoke-nmake"
        output_dir = "output/project-template-smoke"
        entry_count = 1
        failed_entry_count = 0
        dirty_schema_baseline_count = 0
        schema_patch_review_count = 1
        schema_patch_review_changed_count = 1
        schema_patch_approval_pending_count = 0
        schema_patch_approval_approved_count = 1
        schema_patch_approval_rejected_count = 0
        schema_patch_approval_compliance_issue_count = 0
        schema_patch_approval_invalid_result_count = 0
        schema_patch_approval_gate_status = "passed"
        schema_patch_approval_gate_blocked = $false
        schema_patch_approval_items = @(
            [ordered]@{
                name = "invoice-template"
                required = $true
                pending = $false
                approved = $true
                status = "approved"
                decision = "approved"
                action = "promote_schema_update_candidate"
                compliance_issue_count = 0
                compliance_issues = @()
            }
        )
        visual_entry_count = 0
        visual_verdict = "not_applicable"
        manual_review_pending_count = 0
        visual_review_undetermined_count = 0
        passed = $true
        overall_status = "passed"
        entries = @(
            [ordered]@{
                name = "invoice-template"
                input_docx = "samples/invoice.docx"
                artifact_dir = "output/project-template-smoke/entries/01-invoice-template"
                status = "passed"
                passed = $true
                manual_review_pending = $false
                checks = [ordered]@{
                    template_validations = @()
                    schema_validation = [ordered]@{ enabled = $false }
                    schema_baseline = [ordered]@{
                        enabled = $true
                        matches = $true
                        schema_lint_clean = $true
                        schema_patch_approval = [ordered]@{
                            name = "invoice-template"
                            required = $true
                            pending = $false
                            approved = $true
                            status = "approved"
                            decision = "approved"
                            action = "promote_schema_update_candidate"
                            compliance_issue_count = 0
                            compliance_issues = @()
                        }
                    }
                    visual_smoke = [ordered]@{ enabled = $false }
                }
                issues = @()
            }
        )
    })

    Write-JsonFile -Path $historyPath -Value ([ordered]@{
        schema = "featherdoc.project_template_schema_approval_history.v1"
        generated_at = "2026-05-27T12:31:00"
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
                latest_generated_at = "2026-05-27T12:30:00"
                latest_status = "approved"
                latest_decision = "approved"
                latest_action = "promote_schema_update_candidate"
                latest_summary_json = "output/project-template-smoke/summary.json"
                issue_keys = @()
            }
        )
    })

    return [pscustomobject]@{
        Smoke = $smokePath
        History = $historyPath
    }
}

function New-DirectOnboardingEvidence {
    param([string]$Root)

    $summaryPath = Join-Path $Root "invoice-onboarding\onboarding_summary.json"
    $planPath = Join-Path $Root "smoke-onboarding-plan\plan.json"

    Write-JsonFile -Path $summaryPath -Value ([ordered]@{
        schema = "featherdoc.project_template_onboarding_summary.v1"
        summary_schema_version = 1
        template_name = "invoice-template"
        input_docx = "samples/invoice.docx"
        status = "blocked"
        schema_approval_state = [ordered]@{
            status = "pending_review"
            gate_status = "pending"
            release_blocked = $true
            history_required = $false
            action = "review_schema_update_candidate"
        }
        release_blockers = @(
            [ordered]@{
                id = "direct_summary_schema_review"
                severity = "error"
                status = "pending_review"
                action = "review_schema_update_candidate"
                message = "Direct onboarding summary requires schema approval review."
            }
        )
        action_items = @(
            [ordered]@{
                id = "review_direct_summary_schema"
                action = "review_schema_update_candidate"
                title = "Review direct onboarding summary schema"
                command = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1"
            }
        )
        manual_review_recommendations = @()
    })

    Write-JsonFile -Path $planPath -Value ([ordered]@{
        schema = "featherdoc.project_template_smoke_onboarding_plan.v1"
        summary_schema_version = 1
        onboarding_entry_count = 1
        entries = @(
            [ordered]@{
                name = "statement-template"
                input_docx = "samples/statement.docx"
                status = "planned"
                schema_approval_state = [ordered]@{
                    status = "not_evaluated"
                    gate_status = "not_evaluated"
                    release_blocked = $true
                    history_required = $false
                    action = "run_project_template_smoke_then_review_schema_patch_approval"
                }
                release_blockers = @(
                    [ordered]@{
                        id = "direct_plan_schema_review"
                        severity = "error"
                        status = "not_evaluated"
                        action = "run_project_template_smoke_then_review_schema_patch_approval"
                        message = "Direct onboarding plan requires smoke evidence."
                    }
                )
                action_items = @(
                    [ordered]@{
                        id = "run_direct_plan_smoke"
                        action = "run_project_template_smoke"
                        title = "Run direct onboarding plan smoke"
                        command = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_project_template_smoke.ps1"
                    }
                )
                manual_review_recommendations = @()
            }
        )
    })

    return [pscustomobject]@{
        Summary = $summaryPath
        Plan = $planPath
    }
}

if (Test-Scenario -Name "missing_inputs") {
    $missingInputRoot = Join-Path $resolvedWorkingDir "missing-inputs"
    $missingOutputDir = Join-Path $resolvedWorkingDir "missing-inputs-report"
    New-Item -ItemType Directory -Path $missingInputRoot -Force | Out-Null
    $result = Invoke-ReadinessScript -Arguments @(
        "-InputRoot", $missingInputRoot,
        "-OutputDir", $missingOutputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Missing template evidence run should pass with actionable warnings. Output: $($result.Text)"

    $summaryPath = Join-Path $missingOutputDir "summary.json"
    $markdownPath = Join-Path $missingOutputDir "project_template_delivery_readiness.md"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Missing template evidence should keep the readiness summary in needs_review."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Missing template evidence should not be release-ready."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Missing template evidence should remain warning-only."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Missing template evidence should emit one warning."
    $warning = @($summary.warnings | Where-Object { [string]$_.id -eq "template_evidence_missing" })[0]
    Assert-True -Condition ($null -ne $warning) `
        -Message "Missing template evidence should emit template_evidence_missing."
    Assert-Equal -Actual ([string]$warning.repair_strategy) -Expected "collect_project_template_onboarding_governance_evidence" `
        -Message "Template warning should expose a repair strategy."
    Assert-ContainsText -Text ([string]$warning.repair_hint) -ExpectedText "empty feeder summary" `
        -Message "Template warning should preserve the no-synthetic-evidence boundary."
    Assert-ContainsText -Text ([string]$warning.command_template) -ExpectedText "build_project_template_onboarding_governance_report.ps1" `
        -Message "Template warning should include the onboarding governance command template."
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    Assert-ContainsText -Text $markdown -ExpectedText "command_template:" `
        -Message "Missing template Markdown should expose warning command templates."
}

if (Test-Scenario -Name "manifest_description") {
    $manifestRoot = Join-Path $resolvedWorkingDir "manifest-description"
    $manifestPath = Join-Path $manifestRoot "summary.json"
    $outputDir = Join-Path $resolvedWorkingDir "manifest-description-report"
    Write-JsonFile -Path $manifestPath -Value ([ordered]@{
        schema = "featherdoc.project_template_smoke_manifest_description.v1"
        generated_at = "2026-05-27T12:00:00"
        manifest_path = "samples/project_template_smoke.manifest.json"
        manifest_path_display = ".\samples\project_template_smoke.manifest.json"
        summary_json = ""
        summary_json_display = ""
        latest_summary_available = $false
        entry_count = 3
        registered_entry_count = 3
        schema_validation_entry_count = 2
        schema_baseline_entry_count = 2
        visual_smoke_entry_count = 2
        latest_available_entry_count = 0
        latest_missing_entry_count = 3
        entries = @(
            [ordered]@{ name = "contract-template"; latest_available = $false },
            [ordered]@{ name = "invoice-template"; latest_available = $false },
            [ordered]@{ name = "report-template"; latest_available = $false }
        )
    })

    $result = Invoke-ReadinessScript -Arguments @(
        "-InputJson", $manifestPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Manifest-only readiness run should pass with a specific warning. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "project_template_delivery_readiness.md"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-Equal -Actual ([int]$summary.template_count) -Expected 0 `
        -Message "Manifest descriptions should not be treated as template readiness evidence."
    Assert-Equal -Actual ([int]$summary.manifest_description_count) -Expected 1 `
        -Message "Summary should count manifest description evidence."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Manifest-only readiness should emit one warning."
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Manifest-only readiness should keep the summary in needs_review."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Manifest-only readiness should not be release-ready."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Manifest-only readiness should remain warning-only."
    Assert-Equal -Actual ([string]$summary.source_files[0].kind) -Expected "project_template_smoke_manifest_description" `
        -Message "Manifest description input should be loaded as a known evidence kind."

    $missingSmokeWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "project_template_smoke_summary_missing" })[0]
    Assert-True -Condition ($null -ne $missingSmokeWarning) `
        -Message "Manifest-only readiness should emit project_template_smoke_summary_missing."
    Assert-Equal -Actual ([string]$missingSmokeWarning.repair_strategy) -Expected "run_project_template_smoke_for_registered_manifest" `
        -Message "Manifest-only warning should expose a smoke-run repair strategy."
    Assert-Equal -Actual ([int]$missingSmokeWarning.registered_template_count) -Expected 3 `
        -Message "Manifest-only warning should preserve registered template count."
    Assert-Equal -Actual ([int]$missingSmokeWarning.latest_missing_entry_count) -Expected 3 `
        -Message "Manifest-only warning should preserve missing latest summary count."
    Assert-ContainsText -Text ([string]$missingSmokeWarning.command_template) -ExpectedText "run_project_template_smoke.ps1" `
        -Message "Manifest-only warning should include the smoke command template."
    Assert-ContainsText -Text ([string]$missingSmokeWarning.source_json_display) -ExpectedText "manifest-description\summary.json" `
        -Message "Manifest-only warning should point at the manifest description source JSON."
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_smoke_summary_missing" `
        -Message "Manifest-only Markdown should surface the specific smoke summary warning."
    Assert-ContainsText -Text $markdown -ExpectedText "run_project_template_smoke.ps1" `
        -Message "Manifest-only Markdown should surface the smoke command template."
}

if (Test-Scenario -Name "smoke_summary") {
    $evidence = New-SmokeSummaryEvidence -Root (Join-Path $resolvedWorkingDir "smoke-summary-evidence")
    $outputDir = Join-Path $resolvedWorkingDir "smoke-summary-report"
    $result = Invoke-ReadinessScript -Arguments @(
        "-InputJson", "$($evidence.Smoke),$($evidence.History)",
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Smoke summary readiness run should pass when every smoke entry is approved. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "project_template_delivery_readiness.md"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Approved smoke summary evidence should produce ready delivery status."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Approved smoke summary evidence should be release-ready."
    Assert-Equal -Actual ([int]$summary.template_count) -Expected 1 `
        -Message "Smoke summary entries should be converted into readiness templates."
    Assert-Equal -Actual ([int]$summary.smoke_summary_count) -Expected 1 `
        -Message "Readiness summary should count loaded smoke summary evidence."
    Assert-Equal -Actual ([int]$summary.schema_history_count) -Expected 1 `
        -Message "Smoke summary readiness should retain matching schema approval history."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 0 `
        -Message "Loaded smoke summary evidence should not emit missing-smoke warnings."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 0 `
        -Message "Approved smoke summary evidence should not emit blockers."
    Assert-ContainsText -Text (($summary.source_files | ForEach-Object { [string]$_.kind }) -join "`n") -ExpectedText "project_template_smoke_summary" `
        -Message "Smoke summary input should be loaded as a known evidence kind."
    Assert-Equal -Actual ([string]$summary.templates[0].source_kind) -Expected "project_template_smoke_summary" `
        -Message "Template readiness entry should retain smoke summary provenance."
    Assert-Equal -Actual ([string]$summary.templates[0].schema_approval_status) -Expected "approved" `
        -Message "Template readiness entry should preserve approved schema patch state."
    Assert-ContainsText -Text (($summary.schema_approval_status_summary | ForEach-Object { [string]$_.status }) -join "`n") -ExpectedText "approved" `
        -Message "Readiness status summary should include approved schema state."
    Assert-ContainsText -Text ([string]$summary.templates[0].source_json_display) -ExpectedText "project-template-smoke\summary.json" `
        -Message "Template readiness entry should point back to the smoke summary."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_smoke_summary" `
        -Message "Markdown should surface smoke summary provenance."
}

if (Test-Scenario -Name "direct_onboarding_evidence") {
    $evidence = New-DirectOnboardingEvidence -Root (Join-Path $resolvedWorkingDir "direct-onboarding-evidence")
    $outputDir = Join-Path $resolvedWorkingDir "direct-onboarding-report"
    $result = Invoke-ReadinessScript -Arguments @(
        "-InputJson", "$($evidence.Summary),$($evidence.Plan)",
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Direct onboarding evidence readiness run should pass without -FailOnBlocker. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Direct onboarding blockers should keep readiness blocked."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Direct onboarding summary and plan should both propagate blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 2 `
        -Message "Direct onboarding summary and plan should both propagate action items."
    Assert-ContainsText -Text (($summary.source_files | ForEach-Object { [string]$_.kind }) -join "`n") `
        -ExpectedText "onboarding_summary" `
        -Message "Direct onboarding summary input should be loaded as a known evidence kind."
    Assert-ContainsText -Text (($summary.source_files | ForEach-Object { [string]$_.kind }) -join "`n") `
        -ExpectedText "onboarding_plan" `
        -Message "Direct onboarding plan input should be loaded as a known evidence kind."

    $summaryBlocker = @($summary.release_blockers | Where-Object { [string]$_.id -eq "direct_summary_schema_review" })[0]
    Assert-Equal -Actual ([string]$summaryBlocker.source_schema) -Expected "featherdoc.project_template_onboarding_summary.v1" `
        -Message "Direct onboarding summary blockers should retain the onboarding summary schema."
    $planBlocker = @($summary.release_blockers | Where-Object { [string]$_.id -eq "direct_plan_schema_review" })[0]
    Assert-Equal -Actual ([string]$planBlocker.source_schema) -Expected "featherdoc.project_template_smoke_onboarding_plan.v1" `
        -Message "Direct onboarding plan blockers should retain the onboarding plan schema."

    $summaryAction = @($summary.action_items | Where-Object { [string]$_.id -eq "review_direct_summary_schema" })[0]
    Assert-Equal -Actual ([string]$summaryAction.source_schema) -Expected "featherdoc.project_template_onboarding_summary.v1" `
        -Message "Direct onboarding summary action items should retain the onboarding summary schema."
    $planAction = @($summary.action_items | Where-Object { [string]$_.id -eq "run_direct_plan_smoke" })[0]
    Assert-Equal -Actual ([string]$planAction.source_schema) -Expected "featherdoc.project_template_smoke_onboarding_plan.v1" `
        -Message "Direct onboarding plan action items should retain the onboarding plan schema."
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "release_blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "action_items" `
        -ExpectOpenCommandProperty $true
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

    $markdownPath = Join-Path $outputDir "project_template_delivery_readiness.md"
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Failing readiness report should write Markdown output."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blockers" `
        -Message "Failing readiness report should preserve the release blocker section."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness.schema_approval_history_gate" `
        -Message "Failing readiness report should surface the schema approval history blocker."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display=" `
        -Message "Failing readiness report should surface source JSON tracing fields."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
        -Message "Failing readiness report should surface source report tracing fields."
}

Write-Host "Project template delivery readiness report regression passed."
