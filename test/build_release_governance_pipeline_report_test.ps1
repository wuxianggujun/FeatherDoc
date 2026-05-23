param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("aggregate", "fail_on_blocker", "markdown_counts")]
    [string]$Scenario = "aggregate"
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

function Assert-MatchesText {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) {
        throw "$Message Pattern='$Pattern'."
    }
}

function Get-StageById {
    param($Summary, [string]$Id)

    $matches = @($Summary.stages | Where-Object { [string]$_.id -eq $Id })
    Assert-Equal -Actual $matches.Count -Expected 1 `
        -Message "Pipeline should include exactly one stage $Id."
    return $matches[0]
}

function Write-JsonFile {
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-SkeletonRollup {
    return [ordered]@{
        schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
        status = "needs_review"
        clean = $false
        document_count = 1
        total_style_numbering_issue_count = 1
        total_style_numbering_suggestion_count = 1
        total_numbering_definition_count = 2
        total_numbering_instance_count = 3
        total_style_usage_count = 4
        total_command_failure_count = 0
        issue_summary = @([ordered]@{ issue = "missing_numbering_definition"; count = 1 })
        catalog_exemplars = @(
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                input_docx_display = ".\samples\contract.docx"
                exemplar_catalog_path = "output/document-skeleton-governance/contract/exemplar.numbering-catalog.json"
                exemplar_catalog_display = ".\output\document-skeleton-governance\contract\exemplar.numbering-catalog.json"
                definition_count = 2
                instance_count = 3
            }
        )
        release_blockers = @(
            [ordered]@{
                id = "document_skeleton.style_numbering_issues"
                document_name = "contract.docx"
                severity = "error"
                status = "needs_review"
                action = "review_style_numbering_audit"
                message = "Style numbering audit reported issues."
            }
        )
        action_items = @(
            [ordered]@{
                id = "review_style_numbering_audit"
                document_name = "contract.docx"
                action = "review_style_numbering_audit"
                title = "Review contract style numbering audit"
                audit_command = "featherdoc_cli audit-style-numbering samples/contract.docx --json"
                review_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1"
            }
        )
    }
}

function New-NumberingManifest {
    return [ordered]@{
        generated_at = "2026-05-03T00:00:00"
        manifest_path = "baselines/numbering-catalog/manifest.json"
        entry_count = 1
        drift_count = 0
        dirty_baseline_count = 0
        issue_entry_count = 0
        passed = $true
        entries = @(
            [ordered]@{
                name = "contract"
                input_docx = "samples/contract.docx"
                matches = $true
                clean = $true
                catalog_file = "baselines/numbering-catalog/contract.json"
                catalog_lint_clean = $true
                catalog_lint_issue_count = 0
                generated_output_path = "output/numbering-catalog-manifest-checks/contract.generated.numbering-catalog.json"
                baseline_issue_count = 0
                generated_issue_count = 0
                added_definition_count = 0
                removed_definition_count = 0
                changed_definition_count = 0
            }
        )
    }
}

function New-TableLayoutRollup {
    return [ordered]@{
        schema = "featherdoc.table_layout_delivery_rollup_report.v1"
        status = "needs_review"
        ready = $false
        document_count = 1
        document_entries = @(
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                input_docx_display = ".\samples\contract.docx"
                status = "needs_review"
                ready = $false
                preset = "margin-anchor"
                table_style_issue_count = 1
                automatic_tblLook_fix_count = 1
                manual_table_style_fix_count = 1
                table_position_automatic_count = 0
                table_position_review_count = 1
                table_position_already_matching_count = 0
                command_failure_count = 0
                table_position_plan_path = "output/table-layout-delivery/contract/table-position.plan.json"
                table_position_plan_display = ".\output\table-layout-delivery\contract\table-position.plan.json"
            }
        )
        table_position_plans = @(
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                preset = "margin-anchor"
                table_position_plan_path = "output/table-layout-delivery/contract/table-position.plan.json"
                table_position_plan_display = ".\output\table-layout-delivery\contract\table-position.plan.json"
                automatic_count = 0
                review_count = 1
                already_matching_count = 0
            }
        )
        total_table_style_issue_count = 1
        total_automatic_tblLook_fix_count = 1
        total_manual_table_style_fix_count = 1
        total_table_position_automatic_count = 0
        total_table_position_review_count = 1
        total_table_position_already_matching_count = 0
        total_command_failure_count = 0
        release_blockers = @(
            [ordered]@{
                id = "table_layout.positioned_tables_need_review"
                document_name = "contract.docx"
                severity = "warning"
                status = "needs_review"
                action = "review_table_position_plan"
                message = "Floating table position plan needs review."
            }
        )
        action_items = @(
            [ordered]@{
                id = "review_table_position_plan"
                document_name = "contract.docx"
                action = "review_table_position_plan"
                title = "Review floating table position plan"
            }
        )
    }
}

function New-OnboardingGovernance {
    return [ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = "blocked"
        release_ready = $false
        entries = @(
            [ordered]@{
                name = "contract-template"
                input_docx = "samples/contract.docx"
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
                        id = "review_contract_schema"
                        action = "review_schema_update_candidate"
                        title = "Review contract schema"
                    }
                )
                manual_review_recommendations = @()
            }
        )
    }
}

function New-SchemaApprovalHistory {
    return [ordered]@{
        schema = "featherdoc.project_template_schema_approval_history.v1"
        generated_at = "2026-05-03T00:00:00"
        summary_count = 1
        latest_gate_status = "pending"
        blocked_run_count = 0
        pending_run_count = 1
        passed_run_count = 0
        entry_histories = @(
            [ordered]@{
                name = "contract-template"
                project_id = "project-finance"
                template_name = "invoice-template"
                run_count = 1
                blocked_run_count = 0
                pending_run_count = 1
                approved_run_count = 0
                latest_generated_at = "2026-05-03T00:00:00"
                latest_status = "pending_review"
                latest_decision = "pending"
                latest_action = "review_schema_update_candidate"
                latest_summary_json = "output/project-template-smoke/summary.json"
                issue_keys = @()
                runs = @(
                    [ordered]@{
                        project_id = "project-finance"
                        template_name = "invoice-template"
                        schema_patch_review_count = 1
                        schema_patch_review_changed_count = 1
                        schema_patch_reviews = @(
                            [ordered]@{
                                name = "contract-template"
                                project_id = "project-finance"
                                template_name = "invoice-template"
                                candidate_type = "rename"
                                changed = $true
                                baseline_slot_count = 2
                                generated_slot_count = 3
                                upsert_slot_count = 1
                                remove_target_count = 0
                                remove_slot_count = 0
                                rename_slot_count = 0
                                update_slot_count = 0
                                inserted_slots = 1
                                replaced_slots = 0
                            }
                        )
                        schema_patch_approval_items = @(
                            [ordered]@{
                                name = "contract-template"
                                project_id = "project-finance"
                                template_name = "invoice-template"
                                candidate_type = "rename"
                                status = "pending_review"
                                decision = "pending"
                                approved = $false
                                pending = $true
                                compliance_issue_count = 0
                            }
                        )
                    }
                )
            }
        )
    }
}

function New-ContentControlInspection {
    return [ordered]@{
        part = "body"
        entry_name = "word/document.xml"
        count = 2
        content_controls = @(
            [ordered]@{
                index = 0
                kind = "block"
                form_kind = "date"
                tag = "due_date"
                alias = "Due Date"
                id = "10"
                lock = "sdtLocked"
                data_binding_store_item_id = "{55555555-5555-5555-5555-555555555555}"
                data_binding_xpath = "/invoice/dueDate"
                data_binding_prefix_mappings = "xmlns:fd=`"urn:featherdoc`""
                checked = $null
                date_format = "yyyy/MM/dd"
                date_locale = "zh-CN"
                selected_list_item = $null
                list_items = @()
                showing_placeholder = $true
                text = "Due date placeholder"
            },
            [ordered]@{
                index = 1
                kind = "block"
                form_kind = "plain_text"
                tag = "due_date_copy"
                alias = "Due Date Copy"
                id = "11"
                lock = ""
                data_binding_store_item_id = "{55555555-5555-5555-5555-555555555555}"
                data_binding_xpath = "/invoice/dueDate"
                data_binding_prefix_mappings = ""
                checked = $null
                date_format = ""
                date_locale = ""
                selected_list_item = $null
                list_items = @()
                showing_placeholder = $false
                text = "Due date: 2026-07-15"
            }
        )
    }
}

function New-ContentControlSyncResult {
    return [ordered]@{
        scanned_content_controls = 2
        bound_content_controls = 2
        synced_content_controls = 1
        issue_count = 1
        synced_items = @(
            [ordered]@{
                part_entry_name = "word/document.xml"
                content_control_index = 1
                tag = "due_date_copy"
                alias = "Due Date Copy"
                store_item_id = "{55555555-5555-5555-5555-555555555555}"
                xpath = "/invoice/dueDate"
                previous_text = "old"
                value = "Due date: 2026-07-15"
            }
        )
        issues = @(
            [ordered]@{
                part_entry_name = "word/document.xml"
                content_control_index = 0
                tag = "due_date"
                alias = "Due Date"
                store_item_id = "{55555555-5555-5555-5555-555555555555}"
                xpath = "/invoice/missingDueDate"
                reason = "custom_xml_value_not_found"
            }
        )
    }
}

function New-PdfPreflightGovernance {
    return [ordered]@{
        schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
        status = "blocked"
        release_ready = $false
        release_blocker_count = 1
        release_blockers = @(
            [ordered]@{
                id = "pdf_visual_release_gate_preflight.build_outputs_missing"
                severity = "error"
                status = "blocked"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                message = "PDF visual release gate build outputs are missing."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "prepare_pdf_visual_release_gate_build_outputs"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                title = "Prepare PDF visual release gate build outputs"
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1"
            }
        )
    }
}

function New-InputFixture {
    param([string]$Root)

    Write-JsonFile -Path (Join-Path $Root "document-skeleton-governance-rollup\summary.json") -Value (New-SkeletonRollup)
    Write-JsonFile -Path (Join-Path $Root "numbering-catalog-manifest-checks\summary.json") -Value (New-NumberingManifest)
    Write-JsonFile -Path (Join-Path $Root "table-layout-delivery-rollup\summary.json") -Value (New-TableLayoutRollup)
    Write-JsonFile -Path (Join-Path $Root "content-control-data-binding\inspect-content-controls.json") -Value (New-ContentControlInspection)
    Write-JsonFile -Path (Join-Path $Root "content-control-data-binding\sync-content-controls-from-custom-xml.json") -Value (New-ContentControlSyncResult)
    Write-JsonFile -Path (Join-Path $Root "project-template-onboarding-governance\summary.json") -Value (New-OnboardingGovernance)
    Write-JsonFile -Path (Join-Path $Root "project-template-schema-approval-history\history.json") -Value (New-SchemaApprovalHistory)
    Write-JsonFile -Path (Join-Path $Root "pdf-visual-release-gate-preflight-governance\summary.json") -Value (New-PdfPreflightGovernance)
}

function Invoke-Pipeline {
    param([string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$inputRoot = Join-Path $resolvedWorkingDir "input"
$outputRoot = Join-Path $resolvedWorkingDir "pipeline"
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_governance_pipeline_report.ps1"

if ($Scenario -eq "markdown_counts") {
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    $result = Invoke-Pipeline -Arguments @(
        "-InputRoot"
        $inputRoot
        "-OutputRoot"
        $outputRoot
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Pipeline markdown-counts run should finish without fail switches. Output: $($result.Text)"

    $markdownPath = Join-Path $outputRoot "release_governance_pipeline.md"
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Pipeline markdown-counts run should write Markdown."
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-MatchesText -Text $markdown -Pattern "Failed stages: ``\d+``" `
        -Message "Pipeline Markdown should include failed stage count."
    Assert-MatchesText -Text $markdown -Pattern "Missing reports: ``\d+``" `
        -Message "Pipeline Markdown should include missing report count."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``failed``" `
        -Message "Pipeline Markdown should expose failed stage status."
    Assert-MatchesText -Text $markdown -Pattern "missing_reports=``\d+``" `
        -Message "Pipeline Markdown should include stage missing report counts."
    Assert-MatchesText -Text $markdown -Pattern "failed_reports=``\d+``" `
        -Message "Pipeline Markdown should include stage failed report counts."
    Assert-MatchesText -Text $markdown -Pattern "source_failures=``\d+``" `
        -Message "Pipeline Markdown should include stage source failure counts."
    Assert-MatchesText -Text $markdown -Pattern "source_failure_count=``\d+``" `
        -Message "Pipeline Markdown should expose machine-readable stage source failure counts."
    Assert-ContainsText -Text $markdown -ExpectedText "error:" `
        -Message "Pipeline Markdown should include failed stage error details."
    Write-Host "Release governance pipeline markdown count regression passed."
    return
}

New-InputFixture -Root $inputRoot

$arguments = @(
    "-InputRoot"
    $inputRoot
    "-OutputRoot"
    $outputRoot
    "-IncludeHandoffRollup"
)
if ($Scenario -eq "fail_on_blocker") {
    $arguments += "-FailOnBlocker"
}

$result = Invoke-Pipeline -Arguments $arguments
if ($Scenario -eq "fail_on_blocker") {
    if ($result.ExitCode -eq 0) {
        throw "Pipeline fail-on-blocker run should fail. Output: $($result.Text)"
    }
} else {
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Pipeline aggregate run should pass. Output: $($result.Text)"
}

$summaryPath = Join-Path $outputRoot "summary.json"
$markdownPath = Join-Path $outputRoot "release_governance_pipeline.md"
$handoffSummaryPath = Join-Path $outputRoot "release-governance-handoff\summary.json"
$nestedHandoffRollupPath = Join-Path $outputRoot "release-governance-handoff\release-blocker-rollup\summary.json"
$rollupSummaryPath = Join-Path $outputRoot "release-blocker-rollup\summary.json"
foreach ($path in @($summaryPath, $markdownPath, $handoffSummaryPath, $nestedHandoffRollupPath, $rollupSummaryPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected pipeline artifact to exist: $path"
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_governance_pipeline_report.v1" `
    -Message "Pipeline summary should expose schema."
Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
    -Message "Pipeline should be blocked by fixture governance reports."
Assert-Equal -Actual ([string]$summary.governance_detail_source) -Expected "release_blocker_rollup" `
    -Message "Pipeline should expose final rollup as the top-level governance detail source."
Assert-Equal -Actual ([int]$summary.stage_count) -Expected 7 `
    -Message "Pipeline should run seven read-only stages."
Assert-Equal -Actual ([int]$summary.completed_stage_count) -Expected 7 `
    -Message "Pipeline should complete every stage."
Assert-Equal -Actual ([int]$summary.failed_stage_count) -Expected 0 `
    -Message "Pipeline should not record stage failures."
Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 12 `
    -Message "Pipeline should mirror final rollup blocker count."
Assert-True -Condition ([int]$summary.action_item_count -ge 4) `
    -Message "Pipeline should mirror final rollup action count."
Assert-ContainsText -Text (($summary.final_governance_reports | ForEach-Object { [string]$_ }) -join "`n") `
    -ExpectedText "pdf-visual-release-gate-preflight-governance\summary.json" `
    -Message "Pipeline should include an existing PDF preflight governance summary in final governance inputs."
Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "pdf_visual_release_gate_preflight.build_outputs_missing" `
    -Message "Pipeline top-level blocker details should mirror final rollup blockers."
Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.action }) -join "`n") `
    -ExpectedText "prepare_pdf_visual_release_gate_build_outputs" `
    -Message "Pipeline top-level action details should mirror final rollup actions."

$stageIds = @($summary.stages | ForEach-Object { [string]$_.id })
foreach ($expectedStage in @(
        "numbering_catalog_governance",
        "table_layout_delivery_governance",
        "content_control_data_binding_governance",
        "project_template_delivery_readiness",
        "schema_patch_confidence_calibration",
        "release_governance_handoff",
        "release_blocker_rollup"
    )) {
    Assert-ContainsText -Text ($stageIds -join "`n") -ExpectedText $expectedStage `
        -Message "Pipeline should include stage $expectedStage."
}

$numberingStage = Get-StageById -Summary $summary -Id "numbering_catalog_governance"
Assert-ContainsText -Text (($numberingStage.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Pipeline numbering stage should expose document skeleton blocker source schema."
Assert-ContainsText -Text (($numberingStage.release_blockers | ForEach-Object { [string]$_.source_report_display }) -join "`n") `
    -ExpectedText "document-skeleton-governance-rollup\summary.json" `
    -Message "Pipeline numbering stage should expose document skeleton source report display."
Assert-ContainsText -Text (($numberingStage.action_items | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "document-skeleton-governance-rollup\summary.json" `
    -Message "Pipeline numbering stage should expose document skeleton action source JSON display."
Assert-ContainsText -Text (($numberingStage.action_items | ForEach-Object { [string]$_.audit_command }) -join "`n") `
    -ExpectedText "audit-style-numbering" `
    -Message "Pipeline numbering stage should preserve action audit commands."
Assert-ContainsText -Text (($numberingStage.action_items | ForEach-Object { [string]$_.review_command }) -join "`n") `
    -ExpectedText "build_document_skeleton_governance_rollup_report.ps1" `
    -Message "Pipeline numbering stage should preserve action review commands."

$contentControlStage = Get-StageById -Summary $summary -Id "content_control_data_binding_governance"
Assert-ContainsText -Text (($contentControlStage.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.content_control_data_binding_governance_report.v1" `
    -Message "Pipeline content-control stage should expose governance blocker source schema."
Assert-ContainsText -Text (($contentControlStage.release_blockers | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "sync-content-controls-from-custom-xml.json" `
    -Message "Pipeline content-control stage should expose sync evidence JSON display."
$placeholderBlockers = @($contentControlStage.release_blockers | Where-Object { [string]$_.id -eq "content_control_data_binding.bound_placeholder" })
Assert-Equal -Actual $placeholderBlockers.Count -Expected 1 `
    -Message "Pipeline content-control stage should include one bound-placeholder blocker."
$placeholderBlocker = $placeholderBlockers[0]
Assert-Equal -Actual ([string]$placeholderBlocker.repair_strategy) -Expected "sync_bound_content_control" `
    -Message "Pipeline content-control stage should preserve blocker repair strategy."
Assert-ContainsText -Text ([string]$placeholderBlocker.repair_hint) `
    -ExpectedText "Rerun Custom XML sync" `
    -Message "Pipeline content-control stage should preserve blocker repair hint."
Assert-ContainsText -Text ([string]$placeholderBlocker.command_template) `
    -ExpectedText "sync-content-controls-from-custom-xml" `
    -Message "Pipeline content-control stage should preserve blocker command template."
Assert-ContainsText -Text (($contentControlStage.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_content_control_data_binding_governance_report.ps1" `
    -Message "Pipeline content-control stage should expose reviewer open command."
$lockActions = @($contentControlStage.action_items | Where-Object { [string]$_.id -eq "review_content_control_lock_strategy" })
Assert-Equal -Actual $lockActions.Count -Expected 1 `
    -Message "Pipeline content-control stage should include one lock review action."
$lockAction = $lockActions[0]
Assert-Equal -Actual ([string]$lockAction.repair_strategy) -Expected "review_lock_state" `
    -Message "Pipeline content-control stage should preserve action repair strategy."
Assert-ContainsText -Text ([string]$lockAction.command_template) `
    -ExpectedText "--clear-lock" `
    -Message "Pipeline content-control stage should preserve action command template."

$projectStage = Get-StageById -Summary $summary -Id "project_template_delivery_readiness"
Assert-ContainsText -Text (($projectStage.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.project_template_onboarding_governance_report.v1" `
    -Message "Pipeline project stage should expose onboarding blocker source schema."
Assert-ContainsText -Text (($projectStage.action_items | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "project-template-onboarding-governance\summary.json" `
    -Message "Pipeline project stage should expose onboarding action source JSON display."
Assert-ContainsText -Text (($projectStage.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "build_project_template_delivery_readiness_report.ps1" `
    -Message "Pipeline project stage should expose reviewer open command."

$calibrationStage = Get-StageById -Summary $summary -Id "schema_patch_confidence_calibration"
Assert-ContainsText -Text (($calibrationStage.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
    -Message "Pipeline calibration stage should expose blocker source schema."
Assert-ContainsText -Text (($calibrationStage.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
    -ExpectedText "schema-patch-confidence-calibration\summary.json" `
    -Message "Pipeline calibration stage should expose warning source JSON display."
Assert-ContainsText -Text (($calibrationStage.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "write_schema_patch_confidence_calibration_report.ps1" `
    -Message "Pipeline calibration stage should expose reviewer open command."
Assert-ContainsText -Text (($calibrationStage.release_blockers | ForEach-Object { [string]$_.project_id }) -join "`n") `
    -ExpectedText "project-finance" `
    -Message "Pipeline calibration stage should preserve blocker project id."
Assert-ContainsText -Text (($calibrationStage.action_items | ForEach-Object { [string]$_.template_name }) -join "`n") `
    -ExpectedText "invoice-template" `
    -Message "Pipeline calibration stage should preserve action template name."
Assert-ContainsText -Text (($calibrationStage.warnings | ForEach-Object { [string]$_.candidate_type }) -join "`n") `
    -ExpectedText "rename" `
    -Message "Pipeline calibration stage should preserve warning candidate type."

$handoffSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$handoffSummary.schema) -Expected "featherdoc.release_governance_handoff_report.v1" `
    -Message "Pipeline handoff should expose schema."
Assert-Equal -Actual ([bool]$handoffSummary.release_blocker_rollup.included) -Expected $true `
    -Message "Pipeline handoff should include nested release blocker rollup."

$rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "pdf_visual_release_gate_preflight.build_outputs_missing" `
    -Message "Pipeline final rollup should include PDF preflight blockers."
Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.action }) -join "`n") `
    -ExpectedText "prepare_pdf_visual_release_gate_build_outputs" `
    -Message "Pipeline final rollup should include PDF preflight actions."
Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
    -ExpectedText "write_pdf_visual_release_gate_preflight_governance_report.ps1" `
    -Message "Pipeline final rollup should preserve the real PDF preflight governance writer command."
Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
    -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
    -Message "Pipeline final rollup should preserve the PDF preflight source schema."

$markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
Assert-ContainsText -Text $markdown -ExpectedText "# Release Governance Pipeline" `
    -Message "Pipeline Markdown should include title."
Assert-ContainsText -Text $markdown -ExpectedText 'Governance detail source: `release_blocker_rollup`' `
    -Message "Pipeline Markdown should include the governance detail source."
Assert-ContainsText -Text $markdown -ExpectedText "release_blocker_rollup" `
    -Message "Pipeline Markdown should include final rollup stage."
Assert-ContainsText -Text $markdown -ExpectedText "Failed stages: ``0``" `
    -Message "Pipeline Markdown should include failed stage count."
Assert-ContainsText -Text $markdown -ExpectedText "Missing reports: ``0``" `
    -Message "Pipeline Markdown should include missing report count."
Assert-ContainsText -Text $markdown -ExpectedText "source_report_display=" `
    -Message "Pipeline Markdown should include stage source report displays."
Assert-ContainsText -Text $markdown -ExpectedText "source_json_display=" `
    -Message "Pipeline Markdown should include stage source JSON displays."
Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
    -Message "Pipeline Markdown should include stage reviewer open commands."
Assert-ContainsText -Text $markdown -ExpectedText "audit_command:" `
    -Message "Pipeline Markdown should include stage audit commands."
Assert-ContainsText -Text $markdown -ExpectedText "review_command:" `
    -Message "Pipeline Markdown should include stage review commands."
Assert-ContainsText -Text $markdown -ExpectedText "repair_strategy:" `
    -Message "Pipeline Markdown should include stage repair strategies."
Assert-ContainsText -Text $markdown -ExpectedText "command_template:" `
    -Message "Pipeline Markdown should include stage repair command templates."
Assert-ContainsText -Text $markdown -ExpectedText 'project=`project-finance` template=`invoice-template` candidate=`rename`' `
    -Message "Pipeline Markdown should include calibration project/template/candidate routing fields."
Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_release_gate_preflight.build_outputs_missing" `
    -Message "Pipeline Markdown should include PDF preflight blocker ids."
Assert-ContainsText -Text $markdown -ExpectedText "prepare_pdf_visual_release_gate_build_outputs" `
    -Message "Pipeline Markdown should include PDF preflight action ids."

Write-Host "Release governance pipeline report regression passed."
