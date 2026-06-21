param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "release_candidate_summary", "release_candidate_handoff_contracts", "ready", "missing_inputs", "fail_on_blocker")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_project_template_workflow_dashboard_test"
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

function Test-Scenario {
    param([string]$Name)
    return ($Scenario -eq "all" -or $Scenario -eq $Name)
}

function Write-JsonFile {
    param([string]$Path, $Value)

    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-DashboardScript {
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

function New-OnboardingGovernanceReport {
    param(
        [string]$Status = "blocked",
        [bool]$ReleaseReady = $false,
        [int]$BlockerCount = 1,
        [int]$WarningCount = 1
    )

    $schemaApprovalStatusSummary = if ($ReleaseReady) { "approved=1" } else { "pending_review=1" }
    $nextAction = [ordered]@{
        action = "review_schema_update_candidate"
        reason = "Schema approval is pending."
        blocker_id = "project_template_onboarding.schema_approval"
        command = "pwsh -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1"
    }
    $nextActionSummary = @(
        [ordered]@{
            action = "review_schema_update_candidate"
            blocker_id = "project_template_onboarding.schema_approval"
            entry_names = @("invoice-template")
        }
    )
    $nextActionGroupCount = 1
    if ($ReleaseReady) {
        $nextAction = [ordered]@{
            action = "project_template_onboarding_governance_ready"
            reason = "Onboarding governance evidence is ready."
            blocker_id = ""
            command = ""
        }
        $nextActionSummary = @()
        $nextActionGroupCount = 0
    }

    return [ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        summary_schema_version = 1
        status = $Status
        release_ready = $ReleaseReady
        release_blocker_count = $BlockerCount
        warning_count = $WarningCount
        schema_approval_status_summary = $schemaApprovalStatusSummary
        business_document_type_summary = @(
            [ordered]@{
                document_type = "invoice"
                count = 1
            }
        )
        corpus_role_summary = @(
            [ordered]@{
                corpus_role = "registered-business-template"
                count = 1
            }
        )
        next_action = $nextAction
        next_action_summary = $nextActionSummary
        next_action_group_count = $nextActionGroupCount
        source_report_display = ".\output\project-template-onboarding-governance\summary.json"
        source_json_display = ".\output\project-template-onboarding-governance\summary.json"
    }
}

function New-DeliveryReadinessReport {
    param(
        [string]$Status = "blocked",
        [bool]$ReleaseReady = $false,
        [int]$BlockerCount = 1,
        [int]$WarningCount = 0
    )

    $latestSchemaApprovalGateStatus = if ($ReleaseReady) { "ready" } else { "blocked" }
    $schemaApprovalStatusSummary = if ($ReleaseReady) { "approved=1" } else { "pending_review=1" }
    $nextAction = [ordered]@{
        action = "collect_project_template_onboarding_governance_evidence"
        reason = "Delivery readiness depends on onboarding governance evidence."
        blocker_id = "project_template_delivery_readiness.onboarding_governance"
        command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_onboarding_governance_report.ps1"
    }
    $nextActionSummary = @(
        [ordered]@{
            action = "collect_project_template_onboarding_governance_evidence"
            blocker_id = "project_template_delivery_readiness.onboarding_governance"
            entry_names = @("invoice-template")
        }
    )
    $nextActionGroupCount = 1
    if ($ReleaseReady) {
        $nextAction = [ordered]@{
            action = "project_template_delivery_readiness_ready"
            reason = "Delivery readiness evidence is ready."
            blocker_id = ""
            command = ""
        }
        $nextActionSummary = @()
        $nextActionGroupCount = 0
    }
    $plannedRegistrationActions = @()
    if (-not $ReleaseReady) {
        $plannedRegistrationActions = @(
            [ordered]@{
                id = "project-procurement-tender-template"
                project_id = "project-procurement"
                template_name = "tender-template"
                document_type = "tender"
                registration_blocker = "Tender template requires schema, render-data, and visual-smoke fixtures before registration."
                next_action = "Register the tender template after a small procurement sample and render-data contract are available."
            }
        )
    }

    return [ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        summary_schema_version = 1
        status = $Status
        release_ready = $ReleaseReady
        release_blocker_count = $BlockerCount
        warning_count = $WarningCount
        latest_schema_approval_gate_status = $latestSchemaApprovalGateStatus
        schema_approval_status_summary = $schemaApprovalStatusSummary
        business_document_type_summary = @(
            [ordered]@{
                document_type = "tender"
                count = 1
            }
        )
        corpus_role_summary = @(
            [ordered]@{
                corpus_role = "planned-business-template"
                count = 1
            }
        )
        onboarding_governance_next_action = $nextAction
        onboarding_governance_next_action_summary = $nextActionSummary
        onboarding_governance_next_action_group_count = $nextActionGroupCount
        planned_business_template_registration_action_count = $plannedRegistrationActions.Count
        planned_business_template_registration_actions = $plannedRegistrationActions
        source_report_display = ".\output\project-template-delivery-readiness\summary.json"
        source_json_display = ".\output\project-template-delivery-readiness\summary.json"
    }
}

function New-ReleaseCandidateSummary {
    param(
        [string]$OnboardingGovernancePath,
        [string]$DeliveryReadinessPath
    )

    return [ordered]@{
        schema = "featherdoc.release_candidate_summary"
        project_template_onboarding_governance = $OnboardingGovernancePath
        project_template_delivery_readiness = $DeliveryReadinessPath
    }
}

function New-ReleaseCandidateSummaryWithHandoffContracts {
    param(
        [string]$OnboardingGovernancePath,
        [string]$DeliveryReadinessPath
    )

    return [ordered]@{
        schema = "featherdoc.release_candidate_summary"
        release_governance_handoff = [ordered]@{
            project_template_onboarding_governance_contract = [ordered]@{
                source_json_display = $OnboardingGovernancePath
                source_report_display = $OnboardingGovernancePath
            }
            project_template_delivery_readiness_contract = [ordered]@{
                source_json_display = $DeliveryReadinessPath
                source_report_display = $DeliveryReadinessPath
            }
        }
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_project_template_workflow_dashboard.ps1"
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "aggregate") {
    $scenarioDir = Join-Path $resolvedWorkingDir "aggregate"
    $onboardingPath = Join-Path $scenarioDir "project_template_onboarding_governance_summary.json"
    $deliveryPath = Join-Path $scenarioDir "project_template_delivery_readiness_summary.json"
    $outputJson = Join-Path $scenarioDir "project_template_workflow_dashboard.json"
    $outputMarkdown = Join-Path $scenarioDir "project_template_workflow_dashboard.md"

    Write-JsonFile -Path $onboardingPath -Value (New-OnboardingGovernanceReport)
    Write-JsonFile -Path $deliveryPath -Value (New-DeliveryReadinessReport)

    $result = Invoke-DashboardScript -Arguments @(
        "-OnboardingGovernanceJson", $onboardingPath,
        "-DeliveryReadinessJson", $deliveryPath,
        "-SummaryJson", $outputJson,
        "-ReportMarkdown", $outputMarkdown
    )

    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Workflow dashboard aggregate scenario should pass."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputJson | ConvertFrom-Json
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputMarkdown

    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.project_template_workflow_dashboard.v1" `
        -Message "Workflow dashboard schema mismatch."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Workflow dashboard should be blocked when inputs have blockers."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $false `
        -Message "Workflow dashboard release_ready should be false."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 2 `
        -Message "Workflow dashboard should include both source reports."
    Assert-Equal -Actual ([string]$summary.next_action.action) -Expected "review_schema_update_candidate" `
        -Message "Workflow dashboard should expose the first blocking next action."
    Assert-Equal -Actual ([int]$summary.next_action_group_count) -Expected 2 `
        -Message "Workflow dashboard should aggregate both source next action groups."
    $nextActionSourceIds = (@($summary.next_action_summary) | ForEach-Object { [string]$_.source_report_id }) -join [System.Environment]::NewLine
    Assert-ContainsText -Text $nextActionSourceIds -ExpectedText "project_template_onboarding_governance" `
        -Message "Workflow dashboard next action summary should include onboarding governance."
    Assert-ContainsText -Text $nextActionSourceIds -ExpectedText "project_template_delivery_readiness" `
        -Message "Workflow dashboard next action summary should include delivery readiness."
    Assert-Equal -Actual (@($summary.next_action_summary_by_source).Count) -Expected 2 `
        -Message "Workflow dashboard should group next action summaries by source report."
    Assert-Equal -Actual ([string]$summary.next_action_summary_by_source[0].source_report_id) -Expected "project_template_onboarding_governance" `
        -Message "Workflow dashboard grouped next actions should keep onboarding source ids."
    Assert-Equal -Actual ([int]$summary.next_action_summary_by_source[0].action_group_count) -Expected 1 `
        -Message "Workflow dashboard grouped next actions should expose per-source action counts."
    Assert-Equal -Actual ([string]$summary.next_action_summary_by_source[0].action_groups[0].action) -Expected "review_schema_update_candidate" `
        -Message "Workflow dashboard grouped next actions should keep action details."
    Assert-Equal -Actual ([string]$summary.business_document_type_summary[0].document_type) -Expected "invoice" `
        -Message "Workflow dashboard should aggregate business document type summary across source reports."
    Assert-Equal -Actual ([int]$summary.business_document_type_summary[0].count) -Expected 1 `
        -Message "Workflow dashboard should preserve business document type counts."
    Assert-Equal -Actual ([string]$summary.business_document_type_summary[1].document_type) -Expected "tender" `
        -Message "Workflow dashboard should keep all business document types."
    Assert-Equal -Actual ([string]$summary.corpus_role_summary[0].corpus_role) -Expected "planned-business-template" `
        -Message "Workflow dashboard should aggregate corpus role summary across source reports."
    Assert-Equal -Actual ([string]$summary.corpus_role_summary[1].corpus_role) -Expected "registered-business-template" `
        -Message "Workflow dashboard should keep all corpus roles."
    Assert-Equal -Actual ([int]$summary.planned_business_template_registration_action_count) -Expected 1 `
        -Message "Workflow dashboard should aggregate planned business template registration action count."
    Assert-Equal -Actual ([string]$summary.planned_business_template_registration_actions[0].source_report_id) -Expected "project_template_delivery_readiness" `
        -Message "Workflow dashboard planned registration actions should keep their source report."
    Assert-Equal -Actual ([string]$summary.planned_business_template_registration_actions[0].id) -Expected "project-procurement-tender-template" `
        -Message "Workflow dashboard planned registration actions should keep their ids."
    Assert-Equal -Actual ([string]$summary.planned_business_template_registration_actions[0].next_action) -Expected "Register the tender template after a small procurement sample and render-data contract are available." `
        -Message "Workflow dashboard planned registration actions should keep next actions."
    Assert-ContainsText -Text $markdown -ExpectedText "Project Template Workflow Dashboard" `
        -Message "Workflow dashboard Markdown should have a title."
    Assert-ContainsText -Text $markdown -ExpectedText "Business document types" `
        -Message "Workflow dashboard Markdown should include business document type summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Corpus roles" `
        -Message "Workflow dashboard Markdown should include corpus role summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Next action groups" `
        -Message "Workflow dashboard Markdown should include next action groups."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_onboarding_governance" `
        -Message "Workflow dashboard Markdown should include onboarding governance."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Workflow dashboard Markdown should include delivery readiness."
    Assert-ContainsText -Text $markdown -ExpectedText "action_group_count=``1``" `
        -Message "Workflow dashboard Markdown should include grouped action counts."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json=``.\output\project-template-onboarding-governance\summary.json``" `
        -Message "Workflow dashboard Markdown should include grouped source JSON paths."
    Assert-ContainsText -Text $markdown -ExpectedText "business_document_type: ``invoice`` count=``1``" `
        -Message "Workflow dashboard Markdown should expose source-level business document types."
    Assert-ContainsText -Text $markdown -ExpectedText "corpus_role: ``planned-business-template`` count=``1``" `
        -Message "Workflow dashboard Markdown should expose source-level corpus roles."
    Assert-ContainsText -Text $markdown -ExpectedText "Planned business template registration actions" `
        -Message "Workflow dashboard Markdown should include planned business template registration actions."
    Assert-ContainsText -Text $markdown -ExpectedText "project-procurement-tender-template" `
        -Message "Workflow dashboard Markdown should include planned registration action ids."
    Assert-ContainsText -Text $markdown -ExpectedText "Register the tender template after a small procurement sample and render-data contract are available." `
        -Message "Workflow dashboard Markdown should include planned registration next actions."
}

if (Test-Scenario -Name "release_candidate_summary") {
    $scenarioDir = Join-Path $resolvedWorkingDir "release-candidate-summary"
    $onboardingPath = Join-Path $scenarioDir "project_template_onboarding_governance_summary.json"
    $deliveryPath = Join-Path $scenarioDir "project_template_delivery_readiness_summary.json"
    $releaseCandidateSummaryPath = Join-Path $scenarioDir "release_candidate_summary.json"
    $outputJson = Join-Path $scenarioDir "project_template_workflow_dashboard.json"

    Write-JsonFile -Path $onboardingPath -Value (New-OnboardingGovernanceReport)
    Write-JsonFile -Path $deliveryPath -Value (New-DeliveryReadinessReport)
    Write-JsonFile -Path $releaseCandidateSummaryPath -Value (New-ReleaseCandidateSummary `
            -OnboardingGovernancePath $onboardingPath `
            -DeliveryReadinessPath $deliveryPath)

    $result = Invoke-DashboardScript -Arguments @(
        "-ReleaseCandidateSummaryJson", $releaseCandidateSummaryPath,
        "-SummaryJson", $outputJson
    )

    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Workflow dashboard release-candidate-summary scenario should pass."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputJson | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Workflow dashboard should read source paths from release-candidate summary."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 2 `
        -Message "Workflow dashboard should include both release-candidate source reports."
    Assert-ContainsText -Text ([string]$summary.release_candidate_summary_json_display) -ExpectedText "release_candidate_summary.json" `
        -Message "Workflow dashboard should expose release-candidate summary display path."
}

if (Test-Scenario -Name "release_candidate_handoff_contracts") {
    $scenarioDir = Join-Path $resolvedWorkingDir "release-candidate-handoff-contracts"
    $onboardingPath = Join-Path $scenarioDir "project_template_onboarding_governance_summary.json"
    $deliveryPath = Join-Path $scenarioDir "project_template_delivery_readiness_summary.json"
    $releaseCandidateSummaryPath = Join-Path $scenarioDir "release_candidate_summary.json"
    $outputJson = Join-Path $scenarioDir "project_template_workflow_dashboard.json"

    Write-JsonFile -Path $onboardingPath -Value (New-OnboardingGovernanceReport)
    Write-JsonFile -Path $deliveryPath -Value (New-DeliveryReadinessReport)
    Write-JsonFile -Path $releaseCandidateSummaryPath -Value (New-ReleaseCandidateSummaryWithHandoffContracts `
            -OnboardingGovernancePath $onboardingPath `
            -DeliveryReadinessPath $deliveryPath)

    $result = Invoke-DashboardScript -Arguments @(
        "-ReleaseCandidateSummaryJson", $releaseCandidateSummaryPath,
        "-SummaryJson", $outputJson
    )

    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Workflow dashboard release-candidate-handoff-contracts scenario should pass."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputJson | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Workflow dashboard should read source paths from release governance handoff contracts."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 2 `
        -Message "Workflow dashboard should include both handoff contract source reports."
}

if (Test-Scenario -Name "ready") {
    $scenarioDir = Join-Path $resolvedWorkingDir "ready"
    $onboardingPath = Join-Path $scenarioDir "project_template_onboarding_governance_summary.json"
    $deliveryPath = Join-Path $scenarioDir "project_template_delivery_readiness_summary.json"
    $outputJson = Join-Path $scenarioDir "project_template_workflow_dashboard.json"

    Write-JsonFile -Path $onboardingPath -Value (New-OnboardingGovernanceReport -Status "ready" -ReleaseReady $true -BlockerCount 0 -WarningCount 0)
    Write-JsonFile -Path $deliveryPath -Value (New-DeliveryReadinessReport -Status "ready" -ReleaseReady $true -BlockerCount 0 -WarningCount 0)

    $result = Invoke-DashboardScript -Arguments @(
        "-OnboardingGovernanceJson", $onboardingPath,
        "-DeliveryReadinessJson", $deliveryPath,
        "-SummaryJson", $outputJson
    )

    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Workflow dashboard ready scenario should pass."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputJson | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Workflow dashboard should be ready when all inputs are release-ready."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Workflow dashboard release_ready should be true."
    Assert-Equal -Actual ([string]$summary.next_action.action) -Expected "project_template_workflow_ready" `
        -Message "Workflow dashboard should expose a ready next action."
    Assert-Equal -Actual ([int]$summary.next_action_group_count) -Expected 0 `
        -Message "Workflow dashboard should not report next action groups when inputs are ready."
}

if (Test-Scenario -Name "missing_inputs") {
    $scenarioDir = Join-Path $resolvedWorkingDir "missing-inputs"
    New-Item -ItemType Directory -Path $scenarioDir -Force | Out-Null
    $outputJson = Join-Path $scenarioDir "project_template_workflow_dashboard.json"

    $result = Invoke-DashboardScript -Arguments @("-SummaryJson", $outputJson)

    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Workflow dashboard missing-inputs scenario should still write a summary."
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $outputJson | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "missing_input" `
        -Message "Workflow dashboard should report missing_input."
    Assert-Equal -Actual ([int]$summary.missing_input_count) -Expected 2 `
        -Message "Workflow dashboard should report two missing inputs."
}

if (Test-Scenario -Name "fail_on_blocker") {
    $scenarioDir = Join-Path $resolvedWorkingDir "fail-on-blocker"
    $onboardingPath = Join-Path $scenarioDir "project_template_onboarding_governance_summary.json"
    $deliveryPath = Join-Path $scenarioDir "project_template_delivery_readiness_summary.json"
    $outputJson = Join-Path $scenarioDir "project_template_workflow_dashboard.json"

    Write-JsonFile -Path $onboardingPath -Value (New-OnboardingGovernanceReport)
    Write-JsonFile -Path $deliveryPath -Value (New-DeliveryReadinessReport)

    $result = Invoke-DashboardScript -Arguments @(
        "-OnboardingGovernanceJson", $onboardingPath,
        "-DeliveryReadinessJson", $deliveryPath,
        "-SummaryJson", $outputJson,
        "-FailOnBlocker"
    )

    Assert-True -Condition ($result.ExitCode -ne 0) `
        -Message "Workflow dashboard -FailOnBlocker should fail when dashboard is blocked."
    Assert-ContainsText -Text $result.Text -ExpectedText "not release-ready" `
        -Message "Workflow dashboard -FailOnBlocker output should explain the failure."
}

Write-Host "Project template workflow dashboard regression passed."
