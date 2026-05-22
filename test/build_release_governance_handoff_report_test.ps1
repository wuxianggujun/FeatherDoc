param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "missing", "failed_report", "fail_on_missing", "explicit_input", "include_rollup")]
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

function Invoke-HandoffScript {
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
    ($Value | ConvertTo-Json -Depth 20) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-GovernanceFixtures {
    param(
        [string]$Root,
        [bool]$IncludeContentControl = $true,
        [bool]$IncludeProjectTemplate = $true,
        [bool]$IncludeCalibration = $true
    )

    Write-JsonFile -Path (Join-Path $Root "numbering-catalog-governance\summary.json") -Value ([ordered]@{
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
        release_blocker_count = 1
        warning_count = 1
        release_blockers = @(
            [ordered]@{
                id = "numbering_catalog_governance.style_numbering_issues"
                severity = "error"
                status = "blocked"
                action = "review_style_numbering_audit"
                message = "Style numbering audit reported issues."
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "preview_style_numbering_repair"
                action = "preview_style_numbering_repair"
                title = "Preview style numbering repair"
            }
        )
        warnings = @(
            [ordered]@{
                id = "numbering_catalog_manifest_summary_missing"
                message = "No numbering catalog manifest summary was loaded."
            }
        )
    })

    Write-JsonFile -Path (Join-Path $Root "table-layout-delivery-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.table_layout_delivery_governance_report.v1"
        status = "ready"
        release_ready = $true
        delivery_quality_score = 100
        delivery_quality_level = "release_ready"
        delivery_quality = [ordered]@{
            score = 100
            level = "release_ready"
            document_count = 3
            ready_document_count = 3
            ready_document_percent = 100
            needs_review_document_count = 0
            failed_document_count = 0
            table_style_issue_count = 0
            automatic_tblLook_fix_count = 0
            manual_table_style_fix_count = 0
            table_position_automatic_count = 0
            table_position_review_count = 0
            command_failure_count = 0
            unresolved_item_count = 0
            penalty_summary = @(
                [ordered]@{ factor = "safe_tblLook_fixes_pending"; count = 0; penalty = 0 }
            )
        }
        release_blocker_count = 0
        warning_count = 0
        release_blockers = @()
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "run_table_style_quality_visual_regression"
                action = "run_table_style_quality_visual_regression"
                title = "Generate Word-rendered table layout evidence"
            }
        )
    })

    if ($IncludeContentControl) {
        Write-JsonFile -Path (Join-Path $Root "content-control-data-binding-governance\summary.json") -Value ([ordered]@{
            schema = "featherdoc.content_control_data_binding_governance_report.v1"
            status = "blocked"
            release_ready = $false
            release_blocker_count = 1
            warning_count = 0
            release_blockers = @(
                [ordered]@{
                    id = "content_control_data_binding.bound_placeholder"
                    severity = "error"
                    status = "placeholder_visible"
                    action = "sync_or_fill_bound_content_control"
                    message = "A data-bound content control is still showing placeholder text."
                    repair_strategy = "sync_bound_content_control"
                    repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
                    command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
                    source_report = "output/content-control-data-binding-governance/summary.json"
                    source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                    source_json = "output/content-control-data-binding/inspect-content-controls.json"
                    source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "review_duplicate_content_control_binding"
                    action = "review_duplicate_content_control_binding"
                    title = "Review repeated content controls that share one Custom XML binding"
                    open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
                    repair_strategy = "deduplicate_or_confirm_shared_binding"
                    repair_hint = "Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths."
                    command_template = "featherdoc_cli inspect-content-controls <input.docx> --tag due_date --json"
                    source_report = "output/content-control-data-binding-governance/summary.json"
                    source_report_display = ".\output\content-control-data-binding-governance\summary.json"
                    source_json = "output/content-control-data-binding/inspect-content-controls.json"
                    source_json_display = ".\output\content-control-data-binding\inspect-content-controls.json"
                }
            )
        })
    }

    if ($IncludeProjectTemplate) {
        Write-JsonFile -Path (Join-Path $Root "project-template-delivery-readiness\summary.json") -Value ([ordered]@{
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
            warning_count = 0
            release_blockers = @(
                [ordered]@{
                    id = "project_template_delivery.pending_schema_approval"
                    severity = "error"
                    status = "blocked"
                    action = "approve_project_template_schema"
                    message = "Schema approval is still pending."
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "approve_project_template_schema"
                    action = "approve_project_template_schema"
                    title = "Approve project template schema before release"
                }
            )
        })
    }

    if ($IncludeCalibration) {
        Write-JsonFile -Path (Join-Path $Root "schema-patch-confidence-calibration\summary.json") -Value ([ordered]@{
            schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            status = "pending_review"
            release_ready = $false
            release_blocker_count = 1
            warning_count = 1
            release_blockers = @(
                [ordered]@{
                    id = "schema_patch_confidence_calibration.pending_schema_approvals"
                    severity = "error"
                    status = "pending_review"
                    action = "resolve_pending_schema_approvals"
                    message = "Schema patch confidence calibration has pending approvals."
                    project_id = "project-finance"
                    template_name = "invoice-template"
                    candidate_type = "rename"
                    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                    source_report = "output/schema-patch-confidence-calibration/summary.json"
                    source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                    source_json = "output/schema-patch-confidence-calibration/summary.json"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "resolve_pending_schema_approvals"
                    action = "resolve_pending_schema_approvals"
                    title = "Resolve pending schema approvals"
                    open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
                    project_id = "project-finance"
                    template_name = "invoice-template"
                    candidate_type = "rename"
                    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                    source_report = "output/schema-patch-confidence-calibration/summary.json"
                    source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                    source_json = "output/schema-patch-confidence-calibration/summary.json"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                }
            )
            warnings = @(
                [ordered]@{
                    id = "schema_patch_confidence_calibration.unscored_candidates"
                    action = "add_explicit_confidence_metadata"
                    message = "Some schema patch candidates do not carry explicit confidence metadata."
                    project_id = "project-finance"
                    template_name = "invoice-template"
                    candidate_type = "rename"
                    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                    source_report = "output/schema-patch-confidence-calibration/summary.json"
                    source_report_display = ".\output\schema-patch-confidence-calibration\summary.json"
                    source_json = "output/schema-patch-confidence-calibration/summary.json"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                }
            )
        })
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_governance_handoff_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "aggregate") {
    $inputRoot = Join-Path $resolvedWorkingDir "aggregate-input"
    $outputDir = Join-Path $resolvedWorkingDir "aggregate-report"
    Write-GovernanceFixtures -Root $inputRoot

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Aggregate handoff should pass without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "release_governance_handoff.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Aggregate handoff should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Aggregate handoff should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_governance_handoff_report.v1" `
        -Message "Summary should expose release governance handoff schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Aggregate handoff should be blocked when source blockers exist."
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 5 `
        -Message "Aggregate handoff should load all five default reports."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Aggregate handoff should not mark default reports missing."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Aggregate handoff should normalize release blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 5 `
        -Message "Aggregate handoff should normalize action items."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
        -Message "Aggregate handoff should preserve warning counts."
    Assert-Equal -Actual ([int]$summary.governance_metric_count) -Expected 2 `
        -Message "Aggregate handoff should preserve governance metric count."
    $metricText = ($summary.governance_metrics | ForEach-Object { "$($_.report_id):$($_.metric):$($_.level):$($_.score)" }) -join "`n"
    Assert-ContainsText -Text $metricText -ExpectedText "numbering_catalog_governance:real_corpus_confidence:low:56" `
        -Message "Aggregate handoff should preserve numbering real-corpus confidence metric."
    Assert-ContainsText -Text $metricText -ExpectedText "table_layout_delivery_governance:delivery_quality:release_ready:100" `
        -Message "Aggregate handoff should preserve table layout delivery quality metric."
    $numberingMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$numberingMetric.source_json_display) -ExpectedText "numbering-catalog-governance\summary.json" `
        -Message "Aggregate handoff should preserve numbering confidence source JSON display."
    Assert-Equal -Actual ([int]$numberingMetric.details.catalog_coverage_percent) -Expected 100 `
        -Message "Aggregate handoff should preserve numbering confidence detail fields."
    Assert-Equal -Actual ([int]$numberingMetric.details.matched_document_count) -Expected 2 `
        -Message "Aggregate handoff should preserve numbering real-corpus alignment detail fields."
    Assert-ContainsText -Text (($numberingMetric.details.penalty_summary | ForEach-Object { [string]$_.factor }) -join "`n") `
        -ExpectedText "style_numbering_issues" `
        -Message "Aggregate handoff should preserve numbering confidence penalty summary."
    $tableMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "table_layout_delivery_governance.delivery_quality" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$tableMetric.source_json_display) -ExpectedText "table-layout-delivery-governance\summary.json" `
        -Message "Aggregate handoff should preserve table delivery source JSON display."
    Assert-Equal -Actual ([int]$tableMetric.details.unresolved_item_count) -Expected 0 `
        -Message "Aggregate handoff should preserve table layout delivery quality detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.table_position_automatic_count) -Expected 0 `
        -Message "Aggregate handoff should preserve automatic floating table delivery detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.table_position_review_count) -Expected 0 `
        -Message "Aggregate handoff should preserve review floating table delivery detail fields."
    Assert-ContainsText -Text (($tableMetric.details.penalty_summary | ForEach-Object { [string]$_.factor }) -join "`n") `
        -ExpectedText "safe_tblLook_fixes_pending" `
        -Message "Aggregate handoff should preserve table layout delivery penalty summary."
    Assert-Equal -Actual (@($summary.warnings).Count) -Expected 2 `
        -Message "Aggregate handoff should normalize warning details."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "numbering_catalog_manifest_summary_missing" `
        -Message "Aggregate handoff should preserve numbering warning id."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Aggregate handoff should preserve warning source schema."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
        -ExpectedText "schema-patch-confidence-calibration\summary.json" `
        -Message "Aggregate handoff should preserve warning source JSON display."
    $calibrationBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "schema_patch_confidence_calibration.pending_schema_approvals" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationBlocker.project_id) -Expected "project-finance" `
        -Message "Aggregate handoff should preserve calibration blocker project id."
    Assert-Equal -Actual ([string]$calibrationBlocker.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve calibration blocker template name."
    Assert-Equal -Actual ([string]$calibrationBlocker.candidate_type) -Expected "rename" `
        -Message "Aggregate handoff should preserve calibration blocker candidate type."
    Assert-ContainsText -Text ([string]$calibrationBlocker.source_report) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration blocker raw source report."
    Assert-ContainsText -Text ([string]$calibrationBlocker.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration blocker raw source JSON."
    $calibrationAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "resolve_pending_schema_approvals" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationAction.project_id) -Expected "project-finance" `
        -Message "Aggregate handoff should preserve calibration action project id."
    Assert-Equal -Actual ([string]$calibrationAction.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve calibration action template name."
    Assert-Equal -Actual ([string]$calibrationAction.candidate_type) -Expected "rename" `
        -Message "Aggregate handoff should preserve calibration action candidate type."
    Assert-ContainsText -Text ([string]$calibrationAction.source_report) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration action raw source report."
    Assert-ContainsText -Text ([string]$calibrationAction.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration action raw source JSON."
    $calibrationWarning = ($summary.warnings |
        Where-Object { [string]$_.id -eq "schema_patch_confidence_calibration.unscored_candidates" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$calibrationWarning.project_id) -Expected "project-finance" `
        -Message "Aggregate handoff should preserve calibration warning project id."
    Assert-Equal -Actual ([string]$calibrationWarning.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve calibration warning template name."
    Assert-Equal -Actual ([string]$calibrationWarning.candidate_type) -Expected "rename" `
        -Message "Aggregate handoff should preserve calibration warning candidate type."
    Assert-ContainsText -Text ([string]$calibrationWarning.source_report) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration warning raw source report."
    Assert-ContainsText -Text ([string]$calibrationWarning.source_json) -ExpectedText "schema-patch-confidence-calibration/summary.json" `
        -Message "Aggregate handoff should preserve calibration warning raw source JSON."
    Assert-ContainsText -Text (($summary.next_commands | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "ReleaseBlockerRollupAutoDiscover" `
        -Message "Aggregate handoff should hand off to release candidate auto-discovery."
    $contentControlBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "content_control_data_binding.bound_placeholder" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$contentControlBlocker.repair_strategy) -Expected "sync_bound_content_control" `
        -Message "Aggregate handoff should preserve content-control blocker repair strategy."
    Assert-ContainsText -Text ([string]$contentControlBlocker.command_template) -ExpectedText "sync-content-controls-from-custom-xml" `
        -Message "Aggregate handoff should preserve content-control blocker command template."
    Assert-ContainsText -Text ([string]$contentControlBlocker.source_report) -ExpectedText "content-control-data-binding-governance/summary.json" `
        -Message "Aggregate handoff should preserve content-control blocker raw source report."
    Assert-ContainsText -Text ([string]$contentControlBlocker.source_report_display) -ExpectedText "content-control-data-binding-governance\summary.json" `
        -Message "Aggregate handoff should preserve content-control blocker source report display."
    Assert-ContainsText -Text ([string]$contentControlBlocker.source_json) -ExpectedText "content-control-data-binding/inspect-content-controls.json" `
        -Message "Aggregate handoff should preserve content-control blocker raw source JSON."
    Assert-ContainsText -Text ([string]$contentControlBlocker.source_json_display) -ExpectedText "content-control-data-binding\inspect-content-controls.json" `
        -Message "Aggregate handoff should preserve content-control blocker source JSON display."
    $contentControlAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "review_duplicate_content_control_binding" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$contentControlAction.repair_strategy) -Expected "deduplicate_or_confirm_shared_binding" `
        -Message "Aggregate handoff should preserve content-control action repair strategy."
    Assert-ContainsText -Text ([string]$contentControlAction.command_template) -ExpectedText "inspect-content-controls" `
        -Message "Aggregate handoff should preserve content-control action command template."
    Assert-ContainsText -Text ([string]$contentControlAction.source_report) -ExpectedText "content-control-data-binding-governance/summary.json" `
        -Message "Aggregate handoff should preserve content-control action raw source report."
    Assert-ContainsText -Text ([string]$contentControlAction.source_report_display) -ExpectedText "content-control-data-binding-governance\summary.json" `
        -Message "Aggregate handoff should preserve content-control action source report display."
    Assert-ContainsText -Text ([string]$contentControlAction.source_json) -ExpectedText "content-control-data-binding/inspect-content-controls.json" `
        -Message "Aggregate handoff should preserve content-control action raw source JSON."
    Assert-ContainsText -Text ([string]$contentControlAction.source_json_display) -ExpectedText "content-control-data-binding\inspect-content-controls.json" `
        -Message "Aggregate handoff should preserve content-control action source JSON display."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Governance Handoff" `
        -Message "Markdown should include handoff title."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Markdown should include project-template delivery readiness."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.project_template_delivery_readiness_report.v1" `
        -Message "Markdown should include project-template delivery readiness schema."
    Assert-ContainsText -Text $markdown -ExpectedText "latest_schema_approval_gate_status" `
        -Message "Markdown should include project-template schema approval gate status."
    Assert-ContainsText -Text $markdown -ExpectedText "schema_approval_status_summary" `
        -Message "Markdown should include project-template schema approval status summary."
    Assert-ContainsText -Text $markdown -ExpectedText "content_control_data_binding_governance" `
        -Message "Markdown should include content-control data-binding governance."
    Assert-ContainsText -Text $markdown -ExpectedText "content-control-data-binding-governance\summary.json" `
        -Message "Markdown should include content-control source report display."
    Assert-ContainsText -Text $markdown -ExpectedText "content-control-data-binding\inspect-content-controls.json" `
        -Message "Markdown should include content-control source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "schema_patch_confidence_calibration" `
        -Message "Markdown should include schema patch confidence calibration."
    Assert-ContainsText -Text $markdown -ExpectedText 'project=`project-finance` template=`invoice-template` candidate=`rename`' `
        -Message "Markdown should include calibration project/template/candidate routing fields."
    Assert-ContainsText -Text $markdown -ExpectedText "## Warnings" `
        -Message "Markdown should include handoff warnings."
    Assert-ContainsText -Text $markdown -ExpectedText "numbering_catalog_manifest_summary_missing" `
        -Message "Markdown should include warning id."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display" `
        -Message "Markdown should include warning source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "Governance Metrics" `
        -Message "Markdown should include governance metrics."
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
    Assert-ContainsText -Text $markdown -ExpectedText "numbering-catalog-governance\summary.json" `
        -Message "Markdown should include numbering confidence source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "delivery_quality" `
        -Message "Markdown should include table delivery quality metric."
    Assert-ContainsText -Text $markdown -ExpectedText "table_layout_delivery_governance.delivery_quality" `
        -Message "Markdown should include full table delivery quality metric contract id."
    Assert-ContainsText -Text $markdown -ExpectedText "table-layout-delivery-governance\summary.json" `
        -Message "Markdown should include table delivery source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "catalog_coverage_percent=100" `
        -Message "Markdown should include numbering confidence detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "matched_document_count=2" `
        -Message "Markdown should include numbering real-corpus alignment detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "style_numbering_issues(count=3, penalty=15)" `
        -Message "Markdown should include numbering confidence penalty summary."
    Assert-ContainsText -Text $markdown -ExpectedText "unresolved_item_count=0" `
        -Message "Markdown should include table layout delivery detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "safe_tblLook_fixes_pending(count=0, penalty=0)" `
        -Message "Markdown should include table layout delivery penalty summary."
    Assert-ContainsText -Text $markdown -ExpectedText "repair_strategy" `
        -Message "Markdown should include repair strategy details."
    Assert-ContainsText -Text $markdown -ExpectedText "command_template" `
        -Message "Markdown should include command template details."
}

if (Test-Scenario -Name "missing") {
    $inputRoot = Join-Path $resolvedWorkingDir "missing-input"
    $outputDir = Join-Path $resolvedWorkingDir "missing-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Missing handoff should pass without fail-on-missing. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Existing blockers should still drive blocked status."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 1 `
        -Message "Missing handoff should record the missing project-template report."
    Assert-ContainsText -Text (($summary.reports | Where-Object { $_.status -eq "missing" } | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "project_template_delivery_readiness" `
        -Message "Missing handoff should identify the absent project-template report."
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Missing reports: ``1``" `
        -Message "Missing handoff Markdown should summarize the missing report count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Missing handoff Markdown should include the absent project-template report id."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``missing``" `
        -Message "Missing handoff Markdown should expose the missing report status."
    Assert-ContainsText -Text $markdown -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Missing handoff Markdown should include the expected missing summary path."
    Assert-ContainsText -Text $markdown -ExpectedText "build_project_template_delivery_readiness_report.ps1" `
        -Message "Missing handoff Markdown should include the rebuild command for the absent report."
}

if (Test-Scenario -Name "failed_report") {
    $inputRoot = Join-Path $resolvedWorkingDir "failed-report-input"
    $outputDir = Join-Path $resolvedWorkingDir "failed-report"
    Write-GovernanceFixtures -Root $inputRoot
    Set-Content -LiteralPath (Join-Path $inputRoot "project-template-delivery-readiness\summary.json") `
        -Encoding UTF8 `
        -Value "{ this is not valid json"

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir
    )
    if ($result.ExitCode -eq 0) {
        throw "Failed-report handoff should fail when one summary is unreadable. Output: $($result.Text)"
    }

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "failed" `
        -Message "Unreadable handoff source should produce failed status."
    Assert-Equal -Actual ([int]$summary.failed_report_count) -Expected 1 `
        -Message "Unreadable handoff source should count one failed report."
    Assert-ContainsText -Text (($summary.reports | Where-Object { $_.status -eq "failed" } | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "project_template_delivery_readiness" `
        -Message "Unreadable handoff source should identify the failed project-template report."
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Failed reports: ``1``" `
        -Message "Failed handoff Markdown should summarize the failed report count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Failed handoff Markdown should include the failed report id."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``failed``" `
        -Message "Failed handoff Markdown should expose the failed report status."
    Assert-ContainsText -Text $markdown -ExpectedText "error:" `
        -Message "Failed handoff Markdown should include the source failure error."
    Assert-ContainsText -Text $markdown -ExpectedText "{ this is not valid json" `
        -Message "Failed handoff Markdown should preserve the unreadable JSON payload preview."
}

if (Test-Scenario -Name "fail_on_missing") {
    $inputRoot = Join-Path $resolvedWorkingDir "fail-missing-input"
    $outputDir = Join-Path $resolvedWorkingDir "fail-missing-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir,
        "-FailOnMissing"
    )
    if ($result.ExitCode -eq 0) {
        throw "Fail-on-missing handoff should fail. Output: $($result.Text)"
    }
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $outputDir "summary.json")) `
        -Message "Fail-on-missing handoff should still write summary.json."
}

if (Test-Scenario -Name "explicit_input") {
    $inputRoot = Join-Path $resolvedWorkingDir "explicit-input-root"
    $explicitRoot = Join-Path $resolvedWorkingDir "explicit-input"
    $outputDir = Join-Path $resolvedWorkingDir "explicit-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false
    Write-JsonFile -Path (Join-Path $explicitRoot "project-summary.json") -Value ([ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
    })
    Write-JsonFile -Path (Join-Path $explicitRoot "style-summary.json") -Value ([ordered]@{
        schema = "featherdoc.style_catalog_governance_report.v1"
        real_corpus_confidence_score = 12
        real_corpus_confidence_level = "experimental"
        real_corpus_confidence = [ordered]@{
            score = 12
            level = "experimental"
        }
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", ((Join-Path $explicitRoot "project-summary.json") + "," + (Join-Path $explicitRoot "style-summary.json")),
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Explicit handoff should accept an explicit replacement report. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 5 `
        -Message "Explicit handoff should count loaded defaults plus the explicit project-template report."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Explicit handoff should replace the default project-template report without missing defaults."
    Assert-Equal -Actual ([string]($summary.reports | Where-Object { $_.id -eq "project_template_delivery_readiness" } | Select-Object -First 1).source) -Expected "explicit" `
        -Message "Explicit handoff should mark the replacement source."
    Assert-Equal -Actual ([int]$summary.governance_metric_count) -Expected 2 `
        -Message "Explicit handoff should ignore unsupported style real-corpus metric contracts."
    $metricText = ($summary.governance_metrics | ForEach-Object { "$($_.report_id):$($_.metric):$($_.level):$($_.score)" }) -join "`n"
    if ($metricText -match "experimental") {
        throw "Explicit handoff should not treat style governance real_corpus_confidence as numbering confidence."
    }
}

if (Test-Scenario -Name "include_rollup") {
    $inputRoot = Join-Path $resolvedWorkingDir "include-rollup-input"
    $outputDir = Join-Path $resolvedWorkingDir "include-rollup-report"
    Write-GovernanceFixtures -Root $inputRoot

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir,
        "-IncludeReleaseBlockerRollup"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Include-rollup handoff should pass without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $rollupSummaryPath = Join-Path $outputDir "release-blocker-rollup\summary.json"
    $rollupMarkdownPath = Join-Path $outputDir "release-blocker-rollup\release_blocker_rollup.md"
    foreach ($path in @($summaryPath, $rollupSummaryPath, $rollupMarkdownPath)) {
        Assert-True -Condition (Test-Path -LiteralPath $path) `
            -Message "Include-rollup handoff should write artifact: $path"
    }

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([bool]$summary.release_blocker_rollup.included) -Expected $true `
        -Message "Handoff summary should record the included rollup."
    Assert-Equal -Actual ([string]$summary.release_blocker_rollup.summary_json) -Expected $rollupSummaryPath `
        -Message "Handoff summary should expose nested rollup summary path."

    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Nested rollup should expose release blocker rollup schema."
    Assert-Equal -Actual ([int]$rollupSummary.source_report_count) -Expected 5 `
        -Message "Nested rollup should consume all loaded governance reports."
    Assert-Equal -Actual ([int]$rollupSummary.release_blocker_count) -Expected 4 `
        -Message "Nested rollup should preserve blocker count."
    Assert-Equal -Actual ([int]$rollupSummary.action_item_count) -Expected 5 `
        -Message "Nested rollup should preserve action item count."
    Assert-Equal -Actual ([int]$rollupSummary.governance_metric_count) -Expected 2 `
        -Message "Nested rollup should preserve governance metric count."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "real_corpus_confidence:low:56" `
        -Message "Nested rollup should preserve numbering confidence metric."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.id):$($_.report_id):$($_.source_schema)" }) -join "`n") `
        -ExpectedText "numbering_catalog_governance.real_corpus_confidence:numbering_catalog_governance:featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Nested rollup should preserve numbering confidence metric contract."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "delivery_quality:release_ready:100" `
        -Message "Nested rollup should preserve table delivery quality metric."
    Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Nested rollup should preserve calibration source schema."
    Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.project_id }) -join "`n") `
        -ExpectedText "project-finance" `
        -Message "Nested rollup should preserve calibration project id."
    Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.template_name }) -join "`n") `
        -ExpectedText "invoice-template" `
        -Message "Nested rollup should preserve calibration template name."
    Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.candidate_type }) -join "`n") `
        -ExpectedText "rename" `
        -Message "Nested rollup should preserve calibration candidate type."
}

Write-Host "Release governance handoff report regression passed."
