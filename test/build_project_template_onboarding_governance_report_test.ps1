param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "fail_on_blocker")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_project_template_onboarding_governance_report_test"
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
    schema = "featherdoc.project_template_onboarding_summary.v1"
    summary_schema_version = 1
    template_name = "invoice-template"
    input_docx = "samples/invoice.docx"
    business_document_type = "invoice"
    corpus_role = "registered-business-template"
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
    next_action = [ordered]@{
        id = "run_project_template_smoke"
        status = "open"
        action = "run_project_template_smoke_then_review_schema_patch_approval"
        title = "Run smoke"
        reason = "schema approval evidence is missing."
        blocker_id = "project_template_onboarding.schema_approval_not_evaluated"
        command = "pwsh -File .\scripts\run_project_template_smoke.ps1"
        source = "action_items"
    }
    action_items = @(
        [ordered]@{
            id = "run_project_template_smoke"
            status = "open"
            action = "run_project_template_smoke_then_review_schema_patch_approval"
            title = "Run smoke"
            reason = "schema approval evidence is missing."
            blocker_id = "project_template_onboarding.schema_approval_not_evaluated"
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
    schema = "featherdoc.project_template_smoke_onboarding_plan.v1"
    summary_schema_version = 1
    onboarding_entry_count = 1
    entries = @(
        [ordered]@{
            name = "contract-template"
            input_docx = "samples/contract.docx"
            business_document_type = "contract"
            corpus_role = "planned-business-template"
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
            next_action = [ordered]@{
                id = "freeze_schema_baseline"
                status = "open"
                action = "freeze_schema_baseline"
                title = "Freeze schema"
                reason = "Schema approval needs a frozen baseline first."
                blocker_id = "project_template_onboarding.schema_approval_not_evaluated"
                command = "pwsh -File .\scripts\freeze_template_schema_baseline.ps1"
                source = "action_items"
            }
            action_items = @(
                [ordered]@{
                    id = "freeze_schema_baseline"
                    status = "open"
                    action = "freeze_schema_baseline"
                    title = "Freeze schema"
                    reason = "Schema approval needs a frozen baseline first."
                    blocker_id = "project_template_onboarding.schema_approval_not_evaluated"
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
            business_document_type = "invoice"
            corpus_role = "registered-business-template"
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
    Assert-Equal -Actual ([string]$summary.entries[0].business_document_type) -Expected "invoice" `
        -Message "Governance entries should preserve business document type from onboarding evidence."
    Assert-Equal -Actual ([string]$summary.entries[0].corpus_role) -Expected "registered-business-template" `
        -Message "Governance entries should preserve corpus role from onboarding evidence."
    Assert-Equal -Actual ([string](@($summary.entries | Where-Object { $_.name -eq "contract-template" })[0].business_document_type)) -Expected "contract" `
        -Message "Governance entries should preserve onboarding-plan business document type."
    Assert-Equal -Actual ([string](@($summary.entries | Where-Object { $_.name -eq "smoke-template" })[0].corpus_role)) -Expected "registered-business-template" `
        -Message "Governance smoke entries should preserve corpus role metadata."
    Assert-Equal -Actual ([string]$summary.business_document_type_summary[0].document_type) -Expected "invoice" `
        -Message "Governance summary should aggregate business document types."
    Assert-Equal -Actual ([int]$summary.business_document_type_summary[0].count) -Expected 2 `
        -Message "Governance summary should count repeated business document types."
    Assert-Equal -Actual ([string]$summary.business_document_type_summary[1].document_type) -Expected "contract" `
        -Message "Governance summary should keep secondary business document types."
    Assert-Equal -Actual ([string]$summary.corpus_role_summary[0].corpus_role) -Expected "registered-business-template" `
        -Message "Governance summary should aggregate corpus roles."
    Assert-Equal -Actual ([int]$summary.corpus_role_summary[0].count) -Expected 2 `
        -Message "Governance summary should count repeated corpus roles."
    Assert-Equal -Actual ([string]$summary.corpus_role_summary[1].corpus_role) -Expected "planned-business-template" `
        -Message "Governance summary should keep planned corpus roles."
    Assert-Equal -Actual ([string]$summary.next_action.id) -Expected "run_project_template_smoke" `
        -Message "Summary should expose the first global next action."
    Assert-Equal -Actual ([string]$summary.next_action.entry_name) -Expected "invoice-template" `
        -Message "Summary next_action should identify the selected entry."
    Assert-ContainsText -Text ([string]$summary.next_action.reason) `
        -ExpectedText "schema approval evidence is missing" `
        -Message "Summary next_action should preserve the reason."
    Assert-Equal -Actual ([string]$summary.next_action.blocker_id) -Expected "project_template_onboarding.schema_approval_not_evaluated" `
        -Message "Summary next_action should link to the selected blocker."
    Assert-ContainsText -Text ([string]$summary.next_action.open_command) `
        -ExpectedText "run_project_template_smoke.ps1" `
        -Message "Summary next_action should expose the reviewer command."
    Assert-Equal -Actual ([int]$summary.next_action_group_count) -Expected 3 `
        -Message "Summary should group entry next actions."
    Assert-ContainsText -Text (($summary.next_action_summary | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "freeze_schema_baseline" `
        -Message "Next action summary should include onboarding-plan actions."
    Assert-ContainsText -Text (($summary.next_action_summary | ForEach-Object { (@($_.entry_names) -join ",") }) -join "`n") `
        -ExpectedText "contract-template" `
        -Message "Next action summary should preserve grouped entry names."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Release blockers should expose the onboarding governance source schema."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
        -ExpectedText "onboarding_summary.json" `
        -Message "Release blockers should expose the source evidence JSON display path."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
        -ExpectedText "aggregate-report\summary.json" `
        -Message "Release blockers should expose the onboarding governance report display path."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Action items should expose the onboarding governance source schema."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
        -ExpectedText "onboarding_summary.json" `
        -Message "Action items should expose the source evidence JSON display path."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
        -ExpectedText "aggregate-report\summary.json" `
        -Message "Action items should expose the onboarding governance report display path."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "run_project_template_smoke.ps1" `
        -Message "Action items should expose the reviewer open command."
    Assert-ContainsText -Text (($summary.entries | ForEach-Object { [string]$_.next_action.reason }) -join "`n") `
        -ExpectedText "schema approval evidence is missing" `
        -Message "Entries should preserve next_action.reason from onboarding evidence."
    Assert-ContainsText -Text (($summary.entries | ForEach-Object { [string]$_.next_action.blocker_id }) -join "`n") `
        -ExpectedText "project_template_onboarding.schema_approval_not_evaluated" `
        -Message "Entries should preserve next_action.blocker_id from onboarding evidence."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.reason }) -join "`n") `
        -ExpectedText "Schema approval needs a frozen baseline first" `
        -Message "Aggregated action items should preserve reason."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.blocker_id }) -join "`n") `
        -ExpectedText "project_template_onboarding.schema_approval_not_evaluated" `
        -Message "Aggregated action items should preserve blocker_id."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Project Template Onboarding Governance Report" `
        -Message "Markdown should include a title."
    Assert-ContainsText -Text $markdown -ExpectedText "Schema Approval Status" `
        -Message "Markdown should expose status summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Business Document Types" `
        -Message "Markdown should expose business document type summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Corpus Roles" `
        -Message "Markdown should expose corpus role summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Next action: ``run_project_template_smoke``" `
        -Message "Markdown should expose the global next action."
    Assert-ContainsText -Text $markdown -ExpectedText "Next action reason" `
        -Message "Markdown should expose the global next action reason."
    Assert-ContainsText -Text $markdown -ExpectedText "Next Action Summary" `
        -Message "Markdown should expose grouped next actions."
    Assert-ContainsText -Text $markdown -ExpectedText "contract-template" `
        -Message "Markdown should include onboarding plan entries."
    Assert-ContainsText -Text $markdown -ExpectedText "smoke-template" `
        -Message "Markdown should include smoke approval items."
    Assert-ContainsText -Text $markdown -ExpectedText "business_document_type: invoice" `
        -Message "Markdown should expose entry-level business document type."
    Assert-ContainsText -Text $markdown -ExpectedText "corpus_role: registered-business-template" `
        -Message "Markdown should expose entry-level corpus role."
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blockers" `
        -Message "Markdown should include release blockers."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display=" `
        -Message "Markdown should include source JSON display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
        -Message "Markdown should include source report display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "next_action_reason" `
        -Message "Markdown should expose why the preferred next action was selected."
    Assert-ContainsText -Text $markdown -ExpectedText "blocker_id:" `
        -Message "Markdown should expose the blocker behind action items."
    Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
        -Message "Markdown should include action item open commands."
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
    $markdownPath = Join-Path $failOutputDir "project_template_onboarding_governance.md"
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Project Template Onboarding Governance Report" `
        -Message "Failing governance report Markdown should include the title."
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blockers" `
        -Message "Failing governance report Markdown should include the blocker section."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
        -Message "Failing governance report Markdown should include source report display fields."
    Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
        -Message "Failing governance report Markdown should include action item open commands."
}

Write-Host "Project template onboarding governance report regression passed."
