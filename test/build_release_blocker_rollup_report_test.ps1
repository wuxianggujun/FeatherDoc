param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "passing", "comma_input", "empty", "malformed", "dedupe")]
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

function Invoke-RollupScript {
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
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_blocker_rollup_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$fixtureRoot = Join-Path $resolvedWorkingDir "fixtures"
$documentSkeletonPath = Join-Path $fixtureRoot "document-skeleton\document_skeleton_governance.summary.json"
$numberingGovernancePath = Join-Path $fixtureRoot "numbering-governance\summary.json"
$tableLayoutPath = Join-Path $fixtureRoot "table-layout\summary.json"
$contentControlPath = Join-Path $fixtureRoot "content-control\summary.json"
$projectTemplateReadinessPath = Join-Path $fixtureRoot "project-template-readiness\summary.json"
$releaseCandidatePath = Join-Path $fixtureRoot "release-candidate\summary.json"
$styleGovernancePath = Join-Path $fixtureRoot "style-governance\summary.json"
$emptyPath = Join-Path $fixtureRoot "empty\summary.json"
$malformedPath = Join-Path $fixtureRoot "malformed\summary.json"
$dedupePath = Join-Path $fixtureRoot "dedupe\summary.json"

Write-JsonFile -Path $documentSkeletonPath -Value ([ordered]@{
    schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "document_skeleton.style_numbering_issues"
            severity = "error"
            status = "needs_review"
            message = "Style numbering audit reported issues."
            action = "review_style_numbering_audit"
            source_json = "output/document-skeleton-governance/contract/style-numbering-audit.json"
            source_json_display = ".\output\document-skeleton-governance\contract\style-numbering-audit.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            action = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
            command = "featherdoc_cli repair-style-numbering input.docx --plan-only --json"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "document_skeleton.exemplar_catalog_missing"
            action = "open_document_skeleton_rollup"
            message = "One exemplar catalog path is missing."
            source_json = "output/document-skeleton-governance-rollup/summary.json"
            source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
        }
    )
})

Write-JsonFile -Path $numberingGovernancePath -Value ([ordered]@{
    schema = "featherdoc.numbering_catalog_governance_report.v1"
    status = "needs_review"
    release_ready = $false
    real_corpus_confidence_score = 56
    real_corpus_confidence_level = "low"
    real_corpus_confidence = [ordered]@{
        score = 56
        level = "low"
        matched_document_count = 2
        unmatched_catalog_document_count = 0
        unmatched_baseline_document_count = 0
        alignment_gap_count = 0
        catalog_coverage_percent = 100
        baseline_coverage_percent = 100
        penalty_summary = @(
            [ordered]@{ factor = "style_numbering_issues"; count = 3; penalty = 15 }
        )
    }
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
})

Write-JsonFile -Path $tableLayoutPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_governance_report.v1"
    delivery_quality_score = 0
    delivery_quality_level = "blocked"
    delivery_quality = [ordered]@{
        score = 0
        level = "blocked"
        document_count = 2
        ready_document_count = 1
        ready_document_percent = 50
        needs_review_document_count = 1
        failed_document_count = 0
        table_style_issue_count = 3
        automatic_tblLook_fix_count = 2
        manual_table_style_fix_count = 1
        table_position_automatic_count = 2
        table_position_review_count = 1
        command_failure_count = 0
        unresolved_item_count = 10
        penalty_summary = @(
            [ordered]@{ factor = "safe_tblLook_fixes_pending"; count = 2; penalty = 8 },
            [ordered]@{ factor = "floating_table_plans_pending"; count = 3; penalty = 14 }
        )
    }
    release_blocker_count = 2
    release_blockers = @(
        [ordered]@{
            id = "table_layout.manual_table_style_quality_work"
            severity = "warning"
            message = "Manual table style work remains."
            action = "review_table_style_quality_plan"
        },
        [ordered]@{
            id = "table_layout.positioned_tables_need_review"
            severity = "warning"
            message = "Existing floating positions need review."
            action = "review_table_position_plan"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_table_position_preset"
            action = "review_table_position_preset"
            title = "Review table position preset"
            command = "featherdoc_cli apply-table-position-plan plan.json --dry-run --json"
        }
    )
})

Write-JsonFile -Path $styleGovernancePath -Value ([ordered]@{
    schema = "featherdoc.style_catalog_governance_report.v1"
    real_corpus_confidence_score = 12
    real_corpus_confidence_level = "experimental"
    real_corpus_confidence = [ordered]@{
        score = 12
        level = "experimental"
    }
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
})

Write-JsonFile -Path $contentControlPath -Value ([ordered]@{
    schema = "featherdoc.content_control_data_binding_governance_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            severity = "error"
            status = "placeholder_visible"
            message = "A data-bound content control is still showing placeholder text."
            action = "sync_or_fill_bound_content_control"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_content_control_lock_strategy"
            action = "review_content_control_lock_strategy"
            title = "Review lock state for data-bound content control"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
            repair_strategy = "review_lock_state"
            repair_hint = "Confirm whether the lock is intentional; clear it only if template data should overwrite this control."
            command_template = "featherdoc_cli set-content-control-form-state <input.docx> --tag due_date --clear-lock --output <reviewed.docx> --json"
            source_json = "output/content-control-data-binding/inspect-content-controls.json"
            source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
        }
    )
})

Write-JsonFile -Path $projectTemplateReadinessPath -Value ([ordered]@{
    schema = "featherdoc.project_template_delivery_readiness_report.v1"
    status = "blocked"
    release_ready = $false
    latest_schema_approval_gate_status = "pending"
    schema_approval_status_summary = @(
        [ordered]@{
            status = "approved"
            count = 1
        },
        [ordered]@{
            status = "pending_review"
            count = 1
        }
    )
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_onboarding.schema_approval"
            severity = "error"
            status = "pending_review"
            message = "Schema approval is pending."
            action = "review_schema_update_candidate"
            source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
            source_json_display = ".\output\project-template-onboarding-governance\summary.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_project_template_schema_approval"
            action = "review_schema_update_candidate"
            title = "Review project template schema approval"
            open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1"
            source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
            source_json_display = ".\output\project-template-onboarding-governance\summary.json"
        }
    )
})

Write-JsonFile -Path $releaseCandidatePath -Value ([ordered]@{
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            severity = "error"
            status = "blocked"
            message = "Schema approval blocks release."
            action = "fix_schema_patch_approval_result"
        }
    )
    action_items = @()
})

Write-JsonFile -Path $emptyPath -Value ([ordered]@{
    schema = "featherdoc.empty_report.v1"
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
})

Write-JsonFile -Path $malformedPath -Value ([ordered]@{
    schema = "featherdoc.malformed_report.v1"
    release_blocker_count = 3
    release_blockers = @(
        [ordered]@{
            id = "malformed.actual_blocker"
            severity = "error"
            message = "Only one blocker is present."
            action = "fix_malformed_fixture"
        }
    )
    action_items = @()
})

Write-JsonFile -Path $dedupePath -Value ([ordered]@{
    schema = "featherdoc.dedupe_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            severity = "error"
            status = "blocked"
            message = "A second report has the same id."
            action = "fix_schema_patch_approval_result"
        }
    )
    action_items = @(
        [ordered]@{
            id = "same_action"
            action = "fix_schema_patch_approval_result"
            title = "Fix schema approval"
        }
    )
})

if (Test-Scenario -Name "passing") {
    $outputDir = Join-Path $resolvedWorkingDir "passing-report"
    $passingInputRoot = Join-Path $resolvedWorkingDir "passing-input"
    Write-JsonFile -Path (Join-Path $passingInputRoot "document-skeleton\document_skeleton_governance.summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $documentSkeletonPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "numbering-governance\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $numberingGovernancePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "table-layout\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $tableLayoutPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "style-governance\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $styleGovernancePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "content-control\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $contentControlPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "project-template-readiness\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $projectTemplateReadinessPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "release-candidate\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseCandidatePath | ConvertFrom-Json)
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $passingInputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Rollup should succeed without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "release_blocker_rollup.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Rollup should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Rollup should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Summary should expose rollup schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Rollup should be blocked when blockers exist."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 6 `
        -Message "Rollup should aggregate all blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 4 `
        -Message "Rollup should aggregate action items."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 7 `
        -Message "Rollup should keep source report count."
    Assert-Equal -Actual ([int]$summary.governance_metric_count) -Expected 2 `
        -Message "Rollup should aggregate report-level governance metrics."
    $metricText = ($summary.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n"
    Assert-ContainsText -Text $metricText -ExpectedText "real_corpus_confidence:low:56" `
        -Message "Rollup should preserve numbering real-corpus confidence metric."
    $metricContractText = ($summary.governance_metrics | ForEach-Object { "$($_.id):$($_.report_id):$($_.source_schema)" }) -join "`n"
    Assert-ContainsText -Text $metricContractText -ExpectedText "numbering_catalog_governance.real_corpus_confidence:numbering_catalog_governance:featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Rollup should preserve numbering real-corpus confidence contract."
    if ($metricText -match "experimental") {
        throw "Rollup should not treat style governance real_corpus_confidence as numbering confidence."
    }
    Assert-ContainsText -Text $metricText -ExpectedText "delivery_quality:blocked:0" `
        -Message "Rollup should preserve table layout delivery quality metric."
    $numberingMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$numberingMetric.source_json_display) -ExpectedText "numbering-governance\summary.json" `
        -Message "Rollup should preserve numbering confidence source JSON display."
    Assert-Equal -Actual ([int]$numberingMetric.details.catalog_coverage_percent) -Expected 100 `
        -Message "Rollup should preserve numbering confidence detail fields."
    Assert-Equal -Actual ([int]$numberingMetric.details.matched_document_count) -Expected 2 `
        -Message "Rollup should preserve numbering real-corpus alignment detail fields."
    Assert-ContainsText -Text (($numberingMetric.details.penalty_summary | ForEach-Object { [string]$_.factor }) -join "`n") `
        -ExpectedText "style_numbering_issues" `
        -Message "Rollup should preserve numbering confidence penalty summary."
    $tableMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "table_layout_delivery_governance.delivery_quality" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$tableMetric.source_json_display) -ExpectedText "table-layout\summary.json" `
        -Message "Rollup should preserve table delivery source JSON display."
    Assert-Equal -Actual ([int]$tableMetric.details.unresolved_item_count) -Expected 10 `
        -Message "Rollup should preserve table layout delivery quality detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.table_position_automatic_count) -Expected 2 `
        -Message "Rollup should preserve automatic floating table delivery detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.table_position_review_count) -Expected 1 `
        -Message "Rollup should preserve review floating table delivery detail fields."
    Assert-ContainsText -Text (($tableMetric.details.penalty_summary | ForEach-Object { [string]$_.factor }) -join "`n") `
        -ExpectedText "floating_table_plans_pending" `
        -Message "Rollup should preserve table layout delivery penalty summary."
    $skeletonBlocker = @(
        $summary.release_blockers |
            Where-Object { [string]$_.id -eq "document_skeleton.style_numbering_issues" } |
            Select-Object -First 1
    )
    Assert-True -Condition ($null -ne $skeletonBlocker) `
        -Message "Rollup should include the document skeleton blocker."
    Assert-True -Condition ([string]$skeletonBlocker.composite_id -match '^source\d+\.blocker\d+\.document_skeleton\.style_numbering_issues$') `
        -Message "Rollup should generate composite blocker ids."
    Assert-Equal -Actual ([string]$skeletonBlocker.source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Rollup should preserve document skeleton source schema."
    Assert-Equal -Actual ([string]$skeletonBlocker.action) -Expected "review_style_numbering_audit" `
        -Message "Rollup should preserve blocker action."
    Assert-ContainsText -Text ([string]$skeletonBlocker.message) -ExpectedText "Style numbering audit" `
        -Message "Rollup should preserve blocker message."
    Assert-ContainsText -Text ([string]$skeletonBlocker.source_json_display) -ExpectedText "style-numbering-audit.json" `
        -Message "Rollup should preserve blocker source JSON display."
    $skeletonAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "preview_style_numbering_repair" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$skeletonAction.open_command) -ExpectedText "repair-style-numbering" `
        -Message "Rollup should expose action item open command."
    $contentControlBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "content_control_data_binding.bound_placeholder" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$contentControlBlocker.repair_strategy) -Expected "sync_bound_content_control" `
        -Message "Rollup should preserve content-control repair strategy."
    Assert-ContainsText -Text ([string]$contentControlBlocker.command_template) -ExpectedText "sync-content-controls-from-custom-xml" `
        -Message "Rollup should preserve content-control blocker command template."
    $contentControlAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "review_content_control_lock_strategy" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$contentControlAction.repair_strategy) -Expected "review_lock_state" `
        -Message "Rollup should preserve content-control action repair strategy."
    Assert-ContainsText -Text ([string]$contentControlAction.command_template) -ExpectedText "--clear-lock" `
        -Message "Rollup should preserve content-control action command template."
    $projectTemplateSourceReport = ($summary.source_reports |
        Where-Object { [string]$_.schema -eq "featherdoc.project_template_delivery_readiness_report.v1" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$projectTemplateSourceReport.latest_schema_approval_gate_status) -Expected "pending" `
        -Message "Rollup should preserve project-template latest schema approval gate status."
    Assert-ContainsText -Text (($projectTemplateSourceReport.schema_approval_status_summary | ForEach-Object { "$($_.status)=$($_.count)" }) -join "`n") `
        -ExpectedText "pending_review=1" `
        -Message "Rollup should preserve project-template schema approval status summary."
    $skeletonWarning = ($summary.warnings |
        Where-Object { [string]$_.id -eq "document_skeleton.exemplar_catalog_missing" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$skeletonWarning.action) -Expected "open_document_skeleton_rollup" `
        -Message "Rollup should preserve warning action."
    Assert-ContainsText -Text ([string]$skeletonWarning.message) -ExpectedText "exemplar catalog" `
        -Message "Rollup should preserve warning message."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blocker Rollup Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_smoke.schema_approval" `
        -Message "Markdown should include release candidate blocker."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display" `
        -Message "Markdown should include source JSON display details."
    Assert-ContainsText -Text $markdown -ExpectedText "Governance Metrics" `
        -Message "Markdown should include governance metrics."
    Assert-ContainsText -Text $markdown -ExpectedText "Source Report Contracts" `
        -Message "Markdown should include source report contracts."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.project_template_delivery_readiness_report.v1" `
        -Message "Markdown should include project-template delivery readiness contract schema."
    Assert-ContainsText -Text $markdown -ExpectedText "latest_schema_approval_gate_status" `
        -Message "Markdown should include project-template gate status field."
    Assert-ContainsText -Text $markdown -ExpectedText "schema_approval_status_summary" `
        -Message "Markdown should include project-template schema approval status summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Governance Metric Review Focus" `
        -Message "Markdown should include governance metric review focus."
    Assert-ContainsText -Text $markdown -ExpectedText "Numbering real-corpus confidence" `
        -Message "Markdown should highlight numbering real-corpus confidence for reviewers."
    Assert-ContainsText -Text $markdown -ExpectedText "Table/layout delivery quality" `
        -Message "Markdown should highlight table layout delivery quality for reviewers."
    Assert-ContainsText -Text $markdown -ExpectedText "real_corpus_confidence" `
        -Message "Markdown should include numbering confidence metric."
    Assert-ContainsText -Text $markdown -ExpectedText "numbering_catalog_governance.real_corpus_confidence" `
        -Message "Markdown should include full numbering confidence metric contract id."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Markdown should include numbering confidence source schema."
    Assert-ContainsText -Text $markdown -ExpectedText "numbering-governance\summary.json" `
        -Message "Markdown should include numbering confidence source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "delivery_quality" `
        -Message "Markdown should include table delivery quality metric."
    Assert-ContainsText -Text $markdown -ExpectedText "table_layout_delivery_governance.delivery_quality" `
        -Message "Markdown should include full table delivery quality metric contract id."
    Assert-ContainsText -Text $markdown -ExpectedText "table-layout\summary.json" `
        -Message "Markdown should include table delivery source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "catalog_coverage_percent=100" `
        -Message "Markdown should include numbering confidence detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "matched_document_count=2" `
        -Message "Markdown should include numbering real-corpus alignment detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "style_numbering_issues(count=3, penalty=15)" `
        -Message "Markdown should include numbering confidence penalty summary."
    Assert-ContainsText -Text $markdown -ExpectedText "unresolved_item_count=10" `
        -Message "Markdown should include table layout delivery detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "table_position_automatic_count=2" `
        -Message "Markdown should include automatic floating table detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "table_position_review_count=1" `
        -Message "Markdown should include review floating table detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "floating_table_plans_pending(count=3, penalty=14)" `
        -Message "Markdown should include table layout delivery penalty summary."
    Assert-ContainsText -Text $markdown -ExpectedText "repair_strategy" `
        -Message "Markdown should include repair strategy details."
    Assert-ContainsText -Text $markdown -ExpectedText "command_template" `
        -Message "Markdown should include command template details."
}

if (Test-Scenario -Name "empty") {
    $outputDir = Join-Path $resolvedWorkingDir "empty-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $emptyPath,
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Empty rollup should pass fail-on-blocker. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Empty rollup should be ready."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Empty rollup should be release-ready."
}

if (Test-Scenario -Name "comma_input") {
    $outputDir = Join-Path $resolvedWorkingDir "comma-input-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", "$documentSkeletonPath,$tableLayoutPath,$releaseCandidatePath",
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Rollup should accept comma-separated input JSON paths. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 3 `
        -Message "Comma-separated input should keep all three source reports."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Comma-separated input should aggregate all blockers."
    Assert-ContainsText -Text (($summary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
        -ExpectedText "release-candidate" `
        -Message "Comma-separated input should include the final source path."
}

if (Test-Scenario -Name "malformed") {
    $outputDir = Join-Path $resolvedWorkingDir "malformed-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $malformedPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Malformed count should warn but not fail by default. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Malformed count should produce one warning."
    Assert-ContainsText -Text ([string]$summary.warnings[0].message) -ExpectedText "release_blocker_count is 3" `
        -Message "Warning should explain count mismatch."
}

if (Test-Scenario -Name "dedupe") {
    $outputDir = Join-Path $resolvedWorkingDir "dedupe-report"
    $dedupeInputRoot = Join-Path $resolvedWorkingDir "dedupe-input"
    Write-JsonFile -Path (Join-Path $dedupeInputRoot "release-candidate\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseCandidatePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $dedupeInputRoot "dedupe\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $dedupePath | ConvertFrom-Json)
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $dedupeInputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Duplicate ids across reports should remain traceable. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Duplicate blocker ids from different reports should both be retained."
    Assert-Equal -Actual ([int]$summary.blocker_id_summary[0].count) -Expected 2 `
        -Message "Blocker id summary should count duplicates."
    Assert-True -Condition ([string]$summary.release_blockers[0].composite_id -ne [string]$summary.release_blockers[1].composite_id) `
        -Message "Composite ids should keep duplicate blockers distinct."
}

Write-Host "Release blocker rollup report regression passed."
