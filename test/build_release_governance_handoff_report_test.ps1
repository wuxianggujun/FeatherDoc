param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "missing", "failed_report", "fail_on_missing", "explicit_input", "include_rollup")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\build_release_governance_handoff_report_test"
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

function Assert-MarkdownListBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$ExpectedFragments,
        [string]$Message
    )

    $lines = $Text -split "\r?\n"
    for ($index = 0; $index -lt $lines.Count; $index++) {
        if ($lines[$index] -notmatch [regex]::Escape($Anchor)) {
            continue
        }

        $blockLines = @($lines[$index])
        for ($next = $index + 1; $next -lt $lines.Count; $next++) {
            if ([string]::IsNullOrWhiteSpace($lines[$next])) {
                break
            }
            if ($lines[$next] -notmatch '^\s+-\s') {
                break
            }
            $blockLines += $lines[$next]
        }

        $blockText = $blockLines -join "`n"
        $hasAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            if ($blockText -notmatch [regex]::Escape($fragment)) {
                $hasAllFragments = $false
                break
            }
        }

        if ($hasAllFragments) {
            return
        }
    }

    throw $Message
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
                    input_docx = "samples/invoice.docx"
                    input_docx_display = ".\samples\invoice.docx"
                    template_name = "invoice-template"
                    schema_target = "invoice"
                    target_mode = "resolved-section-targets"
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
                    input_docx = "samples/invoice.docx"
                    input_docx_display = ".\samples\invoice.docx"
                    template_name = "invoice-template"
                    schema_target = "invoice"
                    target_mode = "resolved-section-targets"
                }
            )
        })
    }

    if ($IncludeProjectTemplate) {
        Write-JsonFile -Path (Join-Path $Root "project-template-onboarding-governance\summary.json") -Value ([ordered]@{
            schema = "featherdoc.project_template_onboarding_governance_report.v1"
            status = "pending_review"
            release_ready = $false
            schema_approval_status_summary = @(
                [ordered]@{
                    status = "pending_review"
                    count = 1
                }
            )
            release_blocker_count = 1
            action_item_count = 1
        })

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
                    source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                    source_report = "output/project-template-delivery-readiness/summary.json"
                    source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                    source_json = "output/project-template-onboarding-governance/summary.json"
                    source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                    schema_approval_status_summary = @(
                        [ordered]@{
                            status = "pending_review"
                            count = 1
                        }
                    )
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "approve_project_template_schema"
                    action = "approve_project_template_schema"
                    title = "Approve project template schema before release"
                    source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
                    source_report = "output/project-template-delivery-readiness/summary.json"
                    source_report_display = ".\output\project-template-delivery-readiness\summary.json"
                    source_json = "output/project-template-onboarding-governance/summary.json"
                    source_json_display = ".\output\project-template-onboarding-governance\summary.json"
                    schema_approval_status_summary = @(
                        [ordered]@{
                            status = "pending_review"
                            count = 1
                        }
                    )
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
    $projectTemplateReport = ($summary.reports |
        Where-Object { [string]$_.id -eq "project_template_delivery_readiness" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$projectTemplateReport.source_report_display) -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Aggregate handoff should preserve project-template report source_report_display."
    Assert-ContainsText -Text ([string]$projectTemplateReport.source_json_display) -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Aggregate handoff should preserve project-template report source_json_display."
    $projectTemplateBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.report_id -eq "project_template_delivery_readiness" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$projectTemplateBlocker.readiness_status) -Expected "blocked" `
        -Message "Aggregate handoff should propagate project-template readiness status to blockers."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.readiness_release_ready) -Expected "False" `
        -Message "Aggregate handoff should propagate project-template release_ready to blockers."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.source_schema) -Expected "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Aggregate handoff should preserve project-template onboarding source schema on blockers."
    Assert-ContainsText -Text ([string]$projectTemplateBlocker.source_json_display) -ExpectedText "project-template-onboarding-governance\summary.json" `
        -Message "Aggregate handoff should preserve project-template onboarding source_json_display on blockers."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.onboarding_governance_status) -Expected "pending_review" `
        -Message "Aggregate handoff should carry onboarding governance report status separately from delivery readiness."
    Assert-Equal -Actual ([string]$projectTemplateBlocker.onboarding_governance_release_ready) -Expected "False" `
        -Message "Aggregate handoff should carry onboarding governance report release_ready separately from delivery readiness."
    Assert-ContainsText -Text ([string]$projectTemplateBlocker.onboarding_governance_source_json_display) -ExpectedText "project-template-onboarding-governance\summary.json" `
        -Message "Aggregate handoff should point onboarding governance contract at onboarding summary JSON."
    $projectTemplateAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "approve_project_template_schema" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$projectTemplateAction.readiness_status) -Expected "blocked" `
        -Message "Aggregate handoff should propagate project-template readiness status to action items."
    Assert-Equal -Actual ([string]$projectTemplateAction.readiness_release_ready) -Expected "False" `
        -Message "Aggregate handoff should propagate project-template release_ready to action items."
    Assert-Equal -Actual ([string]$projectTemplateAction.source_schema) -Expected "featherdoc.project_template_onboarding_governance_report.v1" `
        -Message "Aggregate handoff should preserve project-template onboarding source schema on action items."
    Assert-ContainsText -Text ([string]$projectTemplateAction.source_json_display) -ExpectedText "project-template-onboarding-governance\summary.json" `
        -Message "Aggregate handoff should preserve project-template onboarding source_json_display on action items."
    Assert-Equal -Actual ([string]$projectTemplateAction.onboarding_governance_status) -Expected "pending_review" `
        -Message "Aggregate handoff should carry onboarding governance report status separately from action readiness."
    Assert-Equal -Actual ([string]$projectTemplateAction.onboarding_governance_release_ready) -Expected "False" `
        -Message "Aggregate handoff should carry onboarding governance report release_ready separately from action readiness."
    Assert-ContainsText -Text ([string]$projectTemplateAction.onboarding_governance_source_json_display) -ExpectedText "project-template-onboarding-governance\summary.json" `
        -Message "Aggregate handoff should point action onboarding governance contract at onboarding summary JSON."
    $metricText = ($summary.governance_metrics | ForEach-Object { "$($_.report_id):$($_.metric):$($_.level):$($_.score)" }) -join "`n"
    Assert-ContainsText -Text $metricText -ExpectedText "numbering_catalog_governance:real_corpus_confidence:low:56" `
        -Message "Aggregate handoff should preserve numbering real-corpus confidence metric."
    Assert-ContainsText -Text $metricText -ExpectedText "table_layout_delivery_governance:delivery_quality:release_ready:100" `
        -Message "Aggregate handoff should preserve table layout delivery quality metric."
    $numberingMetric = ($summary.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$numberingMetric.source_json) -ExpectedText "numbering-catalog-governance\summary.json" `
        -Message "Aggregate handoff should preserve numbering confidence raw source JSON."
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
    $numberingAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "preview_style_numbering_repair" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$numberingAction.open_command) -ExpectedText "build_numbering_catalog_governance_report.ps1" `
        -Message "Aggregate handoff should provide a reviewer open command when numbering action input omits one."
    $tableVisualAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "run_table_style_quality_visual_regression" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$tableVisualAction.open_command) -ExpectedText "build_table_layout_delivery_governance_report.ps1" `
        -Message "Aggregate handoff should provide a reviewer open command when table action input omits one."
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
    Assert-Equal -Actual ([string]$contentControlBlocker.input_docx) -Expected "samples/invoice.docx" `
        -Message "Aggregate handoff should preserve content-control blocker input_docx provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.input_docx_display) -Expected ".\samples\invoice.docx" `
        -Message "Aggregate handoff should preserve content-control blocker input_docx_display provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve content-control blocker template_name provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.schema_target) -Expected "invoice" `
        -Message "Aggregate handoff should preserve content-control blocker schema_target provenance."
    Assert-Equal -Actual ([string]$contentControlBlocker.target_mode) -Expected "resolved-section-targets" `
        -Message "Aggregate handoff should preserve content-control blocker target_mode provenance."
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
    Assert-Equal -Actual ([string]$contentControlAction.input_docx) -Expected "samples/invoice.docx" `
        -Message "Aggregate handoff should preserve content-control action input_docx provenance."
    Assert-Equal -Actual ([string]$contentControlAction.input_docx_display) -Expected ".\samples\invoice.docx" `
        -Message "Aggregate handoff should preserve content-control action input_docx_display provenance."
    Assert-Equal -Actual ([string]$contentControlAction.template_name) -Expected "invoice-template" `
        -Message "Aggregate handoff should preserve content-control action template_name provenance."
    Assert-Equal -Actual ([string]$contentControlAction.schema_target) -Expected "invoice" `
        -Message "Aggregate handoff should preserve content-control action schema_target provenance."
    Assert-Equal -Actual ([string]$contentControlAction.target_mode) -Expected "resolved-section-targets" `
        -Message "Aggregate handoff should preserve content-control action target_mode provenance."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Governance Handoff" `
        -Message "Markdown should include handoff title."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Markdown should include project-template delivery readiness."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.project_template_delivery_readiness_report.v1" `
        -Message "Markdown should include project-template delivery readiness schema."
    Assert-ContainsText -Text $markdown -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Markdown should include project-template source display path."
    Assert-ContainsText -Text $markdown -ExpectedText "latest_schema_approval_gate_status" `
        -Message "Markdown should include project-template schema approval gate status."
    Assert-ContainsText -Text $markdown -ExpectedText "schema_approval_status_summary" `
        -Message "Markdown should include project-template schema approval status summary."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`project_template_delivery_readiness`' -ExpectedFragments @(
        'status=',
        'ready=',
        'source_report_display:',
        'source_json_display:',
        'latest_schema_approval_gate_status:',
        'schema_approval_status_summary:'
    ) -Message "Markdown should keep project-template readiness status, ready flag, schema approval summary, and source displays in one report-status block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`project_template_delivery_readiness` / `project_template_delivery.pending_schema_approval`' -ExpectedFragments @(
        'source_report_display:',
        'source_json_display:',
        'readiness_status:',
        'readiness_release_ready:',
        'project_template_onboarding_governance_contract:',
        'status: `pending_review`',
        'release_ready: `False`',
        'schema_approval_status_summary: `pending_review=1`'
    ) -Message "Markdown should keep project-template readiness status with the handoff blocker evidence block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`project_template_delivery_readiness` / `approve_project_template_schema`' -ExpectedFragments @(
        'source_report_display:',
        'source_json_display:',
        'readiness_status:',
        'readiness_release_ready:',
        'project_template_onboarding_governance_contract:',
        'status: `pending_review`',
        'release_ready: `False`',
        'schema_approval_status_summary: `pending_review=1`'
    ) -Message "Markdown should keep project-template onboarding contract status with the handoff action item evidence block."
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
    Assert-ContainsText -Text $markdown -ExpectedText "source_failures=``0``" `
        -Message "Markdown should include per-report source failure counts."
    Assert-ContainsText -Text $markdown -ExpectedText "source_report:" `
        -Message "Markdown should include raw source report paths for traceability."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json:" `
        -Message "Markdown should include raw source JSON paths for traceability."
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
    Assert-ContainsText -Text $markdown -ExpectedText "source_report: ``$([string]$numberingMetric.source_report)``" `
        -Message "Markdown should include raw metric source report paths."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json: ``$([string]$numberingMetric.source_json)``" `
        -Message "Markdown should include raw metric source JSON paths."
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
    Assert-ContainsText -Text $markdown -ExpectedText "source_failures=``0``" `
        -Message "Failed handoff Markdown should expose the failed report source failure count."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count=``0``" `
        -Message "Failed handoff Markdown should expose the failed report source failure count as a machine-readable field."
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
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Missing reports: ``1``" `
        -Message "Fail-on-missing handoff Markdown should summarize the missing report count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Fail-on-missing handoff Markdown should include the missing report id."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``missing``" `
        -Message "Fail-on-missing handoff Markdown should expose the missing report status."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failures=``0``" `
        -Message "Fail-on-missing handoff Markdown should expose the missing report source failure count."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count=``0``" `
        -Message "Fail-on-missing handoff Markdown should expose the missing report source failure count as a machine-readable field."
    Assert-ContainsText -Text $markdown -ExpectedText "build_project_template_delivery_readiness_report.ps1" `
        -Message "Fail-on-missing handoff Markdown should include the rebuild command."
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
    $explicitRoot = Join-Path $resolvedWorkingDir "include-rollup-explicit"
    $outputDir = Join-Path $resolvedWorkingDir "include-rollup-report"
    $releaseCandidateSummaryPath = Join-Path $explicitRoot "release-candidate-summary.json"
    Write-GovernanceFixtures -Root $inputRoot
    Write-JsonFile -Path $releaseCandidateSummaryPath -Value ([ordered]@{
        project_template_smoke = [ordered]@{
            status = "ready"
        }
        status = "ready"
        release_ready = $true
        pdf_visual_gate_summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
        manifest_signoff_entrypoints = [ordered]@{
            status = "declared"
            release_assets_manifest = "output\release-assets\v<version>\release_assets_manifest.json"
            required_entrypoint_count = 3
            entrypoints = @(
                [ordered]@{
                    id = "start_here"
                    path_display = ".\output\release-candidate-checks\START_HERE.md"
                    required = $true
                },
                [ordered]@{
                    id = "artifact_guide"
                    path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                    required = $true
                },
                [ordered]@{
                    id = "reviewer_checklist"
                    path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
                    required = $true
                }
            )
            required_contracts = @(
                "project_template_delivery_readiness_contract",
                "project_template_onboarding_governance_contract"
            )
            required_fields = @(
                "status",
                "release_ready",
                "schema_approval_status_summary",
                "source_report_display",
                "source_json_display"
            )
            checklist_marker = "reviewer_manifest_scoped_project_template_trace"
        }
        project_template_readiness_checklist_entrypoints = [ordered]@{
            status = "declared"
            checklist_label = "Project template release readiness checklist"
            checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
            required_entrypoint_count = 3
            entrypoints = @(
                [ordered]@{
                    id = "start_here"
                    path_display = ".\output\release-candidate-checks\START_HERE.md"
                    required = $true
                },
                [ordered]@{
                    id = "artifact_guide"
                    path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"
                    required = $true
                },
                [ordered]@{
                    id = "reviewer_checklist"
                    path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"
                    required = $true
                }
            )
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
        }
        release_entry_project_template_readiness_checklist_material_safety_audit = [ordered]@{
            status = "passed"
            audit_script = ".\scripts\assert_release_material_safety.ps1"
            audited_entrypoint_count = 3
            audited_entrypoints = @("start_here", "artifact_guide", "reviewer_checklist")
            compact_evidence_label = "Project-template readiness checklist handoff evidence"
            compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
            checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
            material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
        }
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        steps = [ordered]@{
            pdf_visual_gate = [ordered]@{
                status = "loaded"
                verdict = "pass"
                finalizable = $true
                summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
                aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
                cjk_manifest_count = 43
                cjk_copy_search_count = 43
                cjk_missing_text_count = 0
                visual_baseline_manifest_count = 42
                visual_baseline_count = 44
            }
            pdf_bounded_ctest = [ordered]@{
                status = "pass"
                summary_count = 7
                pass_count = 7
                selected_test_count = 70
                skipped_test_count = 0
                subsets = @(
                    "smoke-import",
                    "contract-static",
                    "cjk-flow-static",
                    "regression-basic-text",
                    "regression-styled-document",
                    "regression-business-samples",
                    "regression-table-layout"
                )
                summary_json_display = @(
                    ".\build\pdf-ctest-bounded-subset-current\summary.json",
                    ".\build\pdf-ctest-bounded-contract-static-current\summary.json",
                    ".\build\pdf-ctest-bounded-cjk-flow-static-current\summary.json",
                    ".\build\pdf-ctest-bounded-regression-basic-text-current\summary.json",
                    ".\build\pdf-ctest-bounded-regression-styled-document-current\summary.json",
                    ".\build\pdf-ctest-bounded-regression-business-samples-current\summary.json",
                    ".\build\pdf-ctest-bounded-regression-table-layout-current\summary.json"
                )
            }
            pdf_visual_gate_attempt = [ordered]@{
                status = "partial"
                verdict = "not_complete"
                full_visual_gate_status = "not_complete"
                evidence_scope = "bounded_attempt_auxiliary_only"
                summary_json = "output/pdf-visual-release-gate-current/report/attempt-summary.json"
                stage_count = 6
                passed_stage_count = 4
                failed_stage_count = 0
                incomplete_stage_count = 2
                pdf_cli_export_status = "pass"
                pdf_regression_status = "pass"
                pdf_regression_selected_test_count = 91
                pdf_regression_failed_test_count = 0
                pdf_regression_skipped_test_count = 7
                unicode_font_status = "pass"
                cjk_copy_search_status = "pass"
                cjk_copy_search_count = 43
                cjk_copy_search_missing_text_count = 0
                visual_baseline_render_status = "partial"
                visual_baseline_fresh_rendered_count = 22
                expected_visual_render_count = 44
                aggregate_contact_sheet_status = "stale"
                aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
            }
        }
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", $releaseCandidateSummaryPath,
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
    Assert-Equal -Actual ([string]$summary.release_blocker_rollup.status) -Expected "blocked" `
        -Message "Handoff summary should consume nested rollup status."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 6 `
        -Message "Handoff summary should consume nested rollup source report count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_blocker_count) -Expected 4 `
        -Message "Handoff summary should consume nested rollup blocker count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 5 `
        -Message "Handoff summary should consume nested rollup action item count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.warning_count) -Expected 2 `
        -Message "Handoff summary should consume nested rollup warning count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.pdf_visual_gate_evidence_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested PDF visual gate evidence count."
    $pdfEvidence = $summary.release_blocker_rollup.pdf_visual_gate_evidence_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $pdfEvidence) `
        -Message "Handoff summary should expose at least one PDF visual gate evidence source report."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_status) -Expected "loaded" `
        -Message "Handoff summary should expose PDF visual gate status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.full_visual_gate_status) -Expected "pass" `
        -Message "Handoff summary should expose the normalized full PDF visual gate status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_verdict) -Expected "pass" `
        -Message "Handoff summary should expose PDF visual gate verdict from the nested rollup."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_visual_gate_finalizable) -Expected $true `
        -Message "Handoff summary should expose PDF visual gate finalizable state from the nested rollup."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_gate_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\summary.json" `
        -Message "Handoff summary should expose PDF visual gate summary display path from the nested rollup."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_gate_aggregate_contact_sheet_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\aggregate-contact-sheet.png" `
        -Message "Handoff summary should expose PDF visual gate contact-sheet display path from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_cjk_manifest_count) -Expected 43 `
        -Message "Handoff summary should expose PDF CJK manifest count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_cjk_copy_search_count) -Expected 43 `
        -Message "Handoff summary should expose PDF CJK copy/search count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_visual_baseline_manifest_count) -Expected 42 `
        -Message "Handoff summary should expose PDF visual baseline manifest count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_visual_baseline_count) -Expected 44 `
        -Message "Handoff summary should expose PDF visual baseline count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_bounded_ctest_summary_count) -Expected 7 `
        -Message "Handoff summary should expose PDF bounded CTest summary count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_bounded_ctest_pass_count) -Expected 7 `
        -Message "Handoff summary should expose PDF bounded CTest pass count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_bounded_ctest_selected_test_count) -Expected 70 `
        -Message "Handoff summary should expose PDF bounded CTest selected test count from the nested rollup."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_bounded_ctest_skipped_test_count) -Expected 0 `
        -Message "Handoff summary should expose PDF bounded CTest skipped test count from the nested rollup."
    Assert-ContainsText -Text (@($pdfEvidence.pdf_bounded_ctest_subsets) -join ",") `
        -ExpectedText "regression-business-samples" `
        -Message "Handoff summary should expose PDF bounded CTest subset names from the nested rollup."
    Assert-ContainsText -Text (@($pdfEvidence.pdf_bounded_ctest_summary_json_display) -join ",") `
        -ExpectedText "pdf-ctest-bounded-regression-table-layout-current\summary.json" `
        -Message "Handoff summary should expose PDF bounded CTest summary display paths from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_status) -Expected "partial" `
        -Message "Handoff summary should expose PDF visual gate attempt status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_verdict) -Expected "not_complete" `
        -Message "Handoff summary should expose PDF visual gate attempt verdict from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_full_visual_gate_status) -Expected "not_complete" `
        -Message "Handoff summary should keep partial attempts separate from the full visual gate status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_evidence_scope) -Expected "bounded_attempt_auxiliary_only" `
        -Message "Handoff summary should expose PDF visual gate attempt evidence scope."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_gate_attempt_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\attempt-summary.json" `
        -Message "Handoff summary should expose PDF visual gate attempt summary display path."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_stage_count) -Expected 6 `
        -Message "Handoff summary should expose PDF visual gate attempt stage count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_passed_stage_count) -Expected 4 `
        -Message "Handoff summary should expose PDF visual gate attempt passed stage count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_incomplete_stage_count) -Expected 2 `
        -Message "Handoff summary should expose PDF visual gate attempt incomplete stage count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_pdf_regression_selected_test_count) -Expected 91 `
        -Message "Handoff summary should expose PDF visual gate attempt pdf_regression selected count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_pdf_regression_failed_test_count) -Expected 0 `
        -Message "Handoff summary should expose PDF visual gate attempt pdf_regression failed count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_pdf_regression_skipped_test_count) -Expected 7 `
        -Message "Handoff summary should expose PDF visual gate attempt pdf_regression skipped count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_cjk_copy_search_count) -Expected 43 `
        -Message "Handoff summary should expose PDF visual gate attempt CJK count."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_visual_baseline_render_status) -Expected "partial" `
        -Message "Handoff summary should expose PDF visual gate attempt render status."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count) -Expected 22 `
        -Message "Handoff summary should expose PDF visual gate attempt fresh render count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_expected_visual_render_count) -Expected 44 `
        -Message "Handoff summary should expose PDF visual gate attempt expected render count."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_aggregate_contact_sheet_status) -Expected "stale" `
        -Message "Handoff summary should expose PDF visual gate attempt contact sheet status."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.manifest_signoff_entrypoints_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested manifest signoff evidence count."
    $manifestSignoffEvidence = $summary.release_blocker_rollup.manifest_signoff_entrypoints_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $manifestSignoffEvidence) `
        -Message "Handoff summary should expose at least one manifest signoff evidence source report."
    Assert-Equal -Actual ([string]$manifestSignoffEvidence.manifest_signoff_entrypoints_status) -Expected "declared" `
        -Message "Handoff summary should expose manifest signoff status from the nested rollup."
    Assert-ContainsText -Text ([string]$manifestSignoffEvidence.manifest_signoff_entrypoints_release_assets_manifest_display) `
        -ExpectedText "release_assets_manifest.json" `
        -Message "Handoff summary should expose release assets manifest display path from the nested rollup."
    Assert-Equal -Actual ([int]$manifestSignoffEvidence.manifest_signoff_entrypoints_required_entrypoint_count) -Expected 3 `
        -Message "Handoff summary should expose required manifest signoff entrypoint count."
    Assert-ContainsText -Text (@($manifestSignoffEvidence.manifest_signoff_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "reviewer_checklist" `
        -Message "Handoff summary should expose reviewer checklist manifest signoff entrypoint."
    Assert-ContainsText -Text (@($manifestSignoffEvidence.manifest_signoff_entrypoints_required_contracts) -join "`n") `
        -ExpectedText "project_template_delivery_readiness_contract" `
        -Message "Handoff summary should expose manifest signoff required project-template delivery contract."
    Assert-ContainsText -Text (@($manifestSignoffEvidence.manifest_signoff_entrypoints_required_fields) -join "`n") `
        -ExpectedText "source_json_display" `
        -Message "Handoff summary should expose manifest signoff required traceability fields."
    Assert-Equal -Actual ([string]$manifestSignoffEvidence.manifest_signoff_entrypoints_checklist_marker) -Expected "reviewer_manifest_scoped_project_template_trace" `
        -Message "Handoff summary should expose manifest signoff reviewer checklist marker."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested project-template readiness checklist entrypoints evidence count."
    $projectTemplateChecklistEvidence = $summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $projectTemplateChecklistEvidence) `
        -Message "Handoff summary should expose at least one project-template readiness checklist evidence source report."
    Assert-Equal -Actual ([string]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_status) -Expected "declared" `
        -Message "Handoff summary should expose project-template readiness checklist status from the nested rollup."
    Assert-Equal -Actual ([string]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_checklist_label) -Expected "Project template release readiness checklist" `
        -Message "Handoff summary should expose project-template readiness checklist label from the nested rollup."
    Assert-Equal -Actual ([string]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_checklist_path) -Expected "docs/project_template_release_readiness_checklist_zh.rst" `
        -Message "Handoff summary should expose project-template readiness checklist path from the nested rollup."
    Assert-Equal -Actual ([int]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_required_entrypoint_count) -Expected 3 `
        -Message "Handoff summary should expose project-template readiness checklist required entrypoint count."
    Assert-ContainsText -Text (@($projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "reviewer_checklist" `
        -Message "Handoff summary should expose reviewer checklist project-template readiness entrypoint."
    Assert-Equal -Actual ([string]$projectTemplateChecklistEvidence.project_template_readiness_checklist_entrypoints_checklist_marker) -Expected "release_entry_project_template_readiness_checklist_trace" `
        -Message "Handoff summary should expose project-template readiness checklist marker."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested release-entry checklist material-safety audit evidence count."
    $releaseEntryChecklistAuditEvidence = $summary.release_blocker_rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $releaseEntryChecklistAuditEvidence) `
        -Message "Handoff summary should expose at least one release-entry checklist material-safety audit source report."
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_status) -Expected "passed" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety audit status."
    Assert-ContainsText -Text ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_script) `
        -ExpectedText "assert_release_material_safety.ps1" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety audit script."
    Assert-ContainsText -Text (@($releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints) -join "`n") `
        -ExpectedText "reviewer_checklist" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety audited entrypoints."
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field) -Expected "project_template_readiness_checklist_entrypoints_source_reports" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety compact evidence field."
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker) -Expected "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety marker."

    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Nested rollup should expose release blocker rollup schema."
    Assert-Equal -Actual ([int]$rollupSummary.source_report_count) -Expected 6 `
        -Message "Nested rollup should consume all loaded governance reports and explicit release-candidate evidence."
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
    $rollupReleaseCandidateSourceReport = ($rollupSummary.source_reports |
        Where-Object { [string]$_.schema -eq "featherdoc.release_candidate_summary" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.manifest_signoff_entrypoints_status) -Expected "declared" `
        -Message "Nested rollup should preserve manifest signoff status from release candidate summaries."
    Assert-ContainsText -Text (@($rollupReleaseCandidateSourceReport.manifest_signoff_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "start_here" `
        -Message "Nested rollup should preserve manifest signoff START_HERE entrypoint."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_status) -Expected "declared" `
        -Message "Nested rollup should preserve project-template readiness checklist entrypoint status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_checklist_path) -Expected "docs/project_template_release_readiness_checklist_zh.rst" `
        -Message "Nested rollup should preserve project-template readiness checklist path."
    Assert-ContainsText -Text (@($rollupReleaseCandidateSourceReport.project_template_readiness_checklist_entrypoints_entrypoint_ids) -join "`n") `
        -ExpectedText "start_here" `
        -Message "Nested rollup should preserve project-template readiness START_HERE entrypoint."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_status) -Expected "passed" `
        -Message "Nested rollup should preserve packaged release-entry checklist material-safety audit status."
    Assert-ContainsText -Text (@($rollupReleaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints) -join "`n") `
        -ExpectedText "artifact_guide" `
        -Message "Nested rollup should preserve packaged release-entry checklist material-safety audited entrypoints."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_status) -Expected "partial" `
        -Message "Nested rollup should preserve PDF visual gate attempt status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_verdict) -Expected "not_complete" `
        -Message "Nested rollup should preserve PDF visual gate attempt verdict."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_pdf_regression_skipped_test_count) -Expected 7 `
        -Message "Nested rollup should preserve PDF visual gate attempt skipped count."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count) -Expected 22 `
        -Message "Nested rollup should preserve PDF visual gate attempt fresh rendered count."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "PDF visual gate evidence source reports: ``1``" `
        -Message "Handoff Markdown should expose the PDF visual gate evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_verdict: ``pass``" `
        -Message "Handoff Markdown should expose the PDF visual gate verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "full_visual_gate_status: ``pass``" `
        -Message "Handoff Markdown should expose the normalized full PDF visual gate conclusion."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_summary_json_display" `
        -Message "Handoff Markdown should expose the PDF visual gate summary display field."
    Assert-ContainsText -Text $markdown -ExpectedText "aggregate-contact-sheet.png" `
        -Message "Handoff Markdown should expose the PDF visual gate contact sheet."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_cjk_manifest_count: ``43``" `
        -Message "Handoff Markdown should expose the PDF CJK manifest count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_cjk_copy_search_count: ``43``" `
        -Message "Handoff Markdown should expose the PDF CJK copy/search count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_visual_baseline_manifest_count: ``42``" `
        -Message "Handoff Markdown should expose the PDF visual baseline manifest count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_visual_baseline_count: ``44``" `
        -Message "Handoff Markdown should expose the PDF visual baseline count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_bounded_ctest_summary_count: ``7``" `
        -Message "Handoff Markdown should expose the PDF bounded CTest summary count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_bounded_ctest_skipped_test_count: ``0``" `
        -Message "Handoff Markdown should expose the PDF bounded CTest skipped test count."
    Assert-ContainsText -Text $markdown -ExpectedText "regression-business-samples" `
        -Message "Handoff Markdown should expose the PDF bounded CTest subset names."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_status: ``partial``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_verdict: ``not_complete``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt evidence scope."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt skipped count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt render status."
    Assert-ContainsText -Text $markdown -ExpectedText "Manifest signoff entrypoints evidence source reports: ``1``" `
        -Message "Handoff Markdown should expose the manifest signoff evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "manifest_signoff_entrypoints_status: ``declared``" `
        -Message "Handoff Markdown should expose the manifest signoff status."
    Assert-ContainsText -Text $markdown -ExpectedText "manifest_signoff_entrypoints_release_assets_manifest_display" `
        -Message "Handoff Markdown should expose the release assets manifest display field."
    Assert-ContainsText -Text $markdown -ExpectedText "manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``" `
        -Message "Handoff Markdown should expose all manifest signoff entrypoint ids."
    Assert-ContainsText -Text $markdown -ExpectedText "reviewer_manifest_scoped_project_template_trace" `
        -Message "Handoff Markdown should expose the manifest signoff checklist marker."
    Assert-ContainsText -Text $markdown -ExpectedText "Project-template readiness checklist entrypoints evidence source reports: ``1``" `
        -Message "Handoff Markdown should expose the project-template readiness checklist evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_status: ``declared``" `
        -Message "Handoff Markdown should expose the project-template readiness checklist status."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_checklist_label: ``Project template release readiness checklist``" `
        -Message "Handoff Markdown should expose the project-template readiness checklist label."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``" `
        -Message "Handoff Markdown should expose the project-template readiness checklist path."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``" `
        -Message "Handoff Markdown should expose all project-template readiness checklist entrypoint ids."
    Assert-ContainsText -Text $markdown -ExpectedText "release_entry_project_template_readiness_checklist_trace" `
        -Message "Handoff Markdown should expose the project-template readiness checklist marker."
    Assert-ContainsText -Text $markdown -ExpectedText "Release-entry project-template readiness checklist material-safety audit source reports: ``1``" `
        -Message "Handoff Markdown should expose release-entry checklist material-safety audit evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``" `
        -Message "Handoff Markdown should expose release-entry checklist material-safety audit status."
    Assert-ContainsText -Text $markdown -ExpectedText "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``" `
        -Message "Handoff Markdown should expose release-entry checklist material-safety audited entrypoints."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
        -Message "Handoff Markdown should expose release-entry checklist material-safety marker."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "pdf_visual_gate_status: ``loaded``",
        "full_visual_gate_status: ``pass``",
        "pdf_visual_gate_verdict: ``pass``",
        "pdf_visual_gate_finalizable: ``True``",
        "pdf_visual_gate_summary_json_display:",
        "summary.json",
        "pdf_visual_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_gate_cjk_manifest_count: ``43``",
        "pdf_visual_gate_cjk_copy_search_count: ``43``",
        "pdf_visual_gate_visual_baseline_manifest_count: ``42``",
        "pdf_visual_gate_visual_baseline_count: ``44``",
        "pdf_bounded_ctest_summary_count: ``7``",
        "pdf_bounded_ctest_pass_count: ``7``",
        "pdf_bounded_ctest_skipped_test_count: ``0``",
        "pdf_bounded_ctest_selected_test_count: ``70``",
        "pdf_bounded_ctest_subsets:",
        "regression-table-layout",
        "pdf_bounded_ctest_summary_json_display:",
        "pdf-ctest-bounded-regression-business-samples-current\summary.json"
    ) -Message "Handoff Markdown should keep PDF visual gate source-report evidence in one source_report block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``"
    ) -Message "Handoff Markdown should keep release-entry checklist material-safety audit evidence in one source_report block."
}

Write-Host "Release governance handoff report regression passed."
