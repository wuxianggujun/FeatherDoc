param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("aggregate", "fail_on_blocker")]
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

function Write-JsonFile {
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function New-InputFixture {
    param([string]$Root)

    Write-JsonFile -Path (Join-Path $Root "numbering-catalog-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.numbering_catalog_governance_report.v1"
        status = "needs_review"
        release_ready = $false
        release_blocker_count = 1
        release_blockers = @(
            [ordered]@{
                id = "document_skeleton.style_numbering_issues"
                severity = "error"
                status = "needs_review"
                action = "review_style_numbering_audit"
                message = "Style numbering audit reported issues."
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "review_style_numbering_audit"
                action = "review_style_numbering_audit"
                title = "Review contract style numbering audit"
            }
        )
        warning_count = 1
        warnings = @(
            [ordered]@{
                id = "document_skeleton.style_merge_suggestions_pending"
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                action = "review_style_merge_suggestions"
                style_merge_suggestion_count = 2
                message = "Document skeleton governance reports 2 duplicate style merge suggestion(s) awaiting review."
            }
        )
    })
    Write-JsonFile -Path (Join-Path $Root "table-layout-delivery-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.table_layout_delivery_governance_report.v1"
        status = "needs_review"
        release_ready = $false
        release_blocker_count = 4
        release_blockers = @(
            [ordered]@{
                id = "table_layout.positioned_tables_need_review"
                severity = "warning"
                status = "needs_review"
                action = "review_table_position_plan"
                message = "Floating table position plan needs review."
            },
            [ordered]@{
                id = "table_layout_delivery.safe_tblLook_fixes_pending"
                severity = "warning"
                status = "needs_review"
                action = "apply_safe_tblLook_fixes_then_visual_regression"
                message = "Safe tblLook fixes are pending."
            },
            [ordered]@{
                id = "table_layout_delivery.manual_table_style_work"
                severity = "warning"
                status = "needs_review"
                action = "review_manual_table_style_definition_work"
                message = "Manual table style definition work remains."
            },
            [ordered]@{
                id = "table_layout_delivery.floating_table_review_pending"
                severity = "warning"
                status = "needs_review"
                action = "review_floating_table_position_plans"
                message = "Floating table position review remains."
            }
        )
        action_item_count = 4
        action_items = @(
            [ordered]@{ id = "review_table_position_plan"; action = "review_table_position_plan"; title = "Review floating table position plan" },
            [ordered]@{ id = "apply_safe_tblLook_fixes"; action = "apply_safe_tblLook_fixes_then_visual_regression"; title = "Apply safe tblLook fixes" },
            [ordered]@{ id = "review_floating_table_position_plans"; action = "review_floating_table_position_plans"; title = "Review floating table position plans" },
            [ordered]@{ id = "run_table_style_quality_visual_regression"; action = "run_table_style_quality_visual_regression"; title = "Run table visual regression" }
        )
        warning_count = 0
        warnings = @()
    })
    Write-JsonFile -Path (Join-Path $Root "content-control-data-binding-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.content_control_data_binding_governance_report.v1"
        status = "blocked"
        release_ready = $false
        release_blocker_count = 2
        release_blockers = @(
            [ordered]@{
                id = "content_control_data_binding.custom_xml_sync_issue"
                severity = "error"
                status = "blocked"
                action = "fix_custom_xml_data_binding_source"
                message = "A Custom XML binding issue remains."
            },
            [ordered]@{
                id = "content_control_data_binding.bound_placeholder"
                severity = "error"
                status = "placeholder_visible"
                action = "sync_or_fill_bound_content_control"
                message = "A data-bound content control is still showing placeholder text."
            }
        )
        action_item_count = 2
        action_items = @(
            [ordered]@{ id = "review_content_control_lock_strategy"; action = "review_content_control_lock_strategy"; title = "Review lock strategy" },
            [ordered]@{ id = "review_duplicate_content_control_binding"; action = "review_duplicate_content_control_binding"; title = "Review duplicate content control binding" }
        )
        warning_count = 1
        warnings = @(
            [ordered]@{ id = "input_json_missing"; message = "Input JSON was not found." }
        )
    })
    Write-JsonFile -Path (Join-Path $Root "project-template-delivery-readiness\summary.json") -Value ([ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        status = "blocked"
        release_ready = $false
        release_blocker_count = 3
        release_blockers = @(
            [ordered]@{
                id = "project_template_delivery_readiness.schema_approval_history_gate"
                severity = "error"
                status = "blocked"
                action = "review_schema_approval_history"
                message = "Schema approval history gate is blocked."
            },
            [ordered]@{
                id = "project_template_onboarding.schema_approval"
                severity = "error"
                status = "pending_review"
                action = "review_schema_update_candidate"
                message = "Schema approval is pending."
            },
            [ordered]@{
                id = "project_template_delivery_readiness.schema_approval_history"
                severity = "error"
                status = "pending_review"
                action = "review_schema_update_candidate"
                message = "Schema approval history is pending."
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "review_contract_schema"
                action = "review_schema_update_candidate"
                title = "Review contract schema"
            }
        )
        warning_count = 0
        warnings = @()
    })
    Write-JsonFile -Path (Join-Path $Root "document-skeleton-governance\style-merge.restore-audit.summary.json") -Value ([ordered]@{
        schema = "featherdoc.style_merge_restore_audit.v1"
        status = "blocked"
        release_ready = $false
        release_blocker_count = 1
        release_blockers = @(
            [ordered]@{
                id = "style_merge.restore_audit_issues"
                severity = "error"
                status = "blocked"
                action = "review_style_merge_restore_audit"
                message = "Style merge restore audit found dry-run issues."
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "review_style_merge_restore_audit"
                action = "review_style_merge_restore_audit"
                title = "Review style merge restore audit and Word render"
                command = "pwsh -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 -DocxPath output/document-skeleton-governance/merged-styles.docx -DocumentSourceKind style-merge-restore-audit -DocumentSourceLabel `"Style merge restore audit`" -Mode review-only"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit -PrintPrompt"
            }
        )
        warning_count = 0
        warnings = @()
    })
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
New-InputFixture -Root $inputRoot

$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_governance_pipeline_report.ps1"
$arguments = @(
    "-InputRoot"
    $inputRoot
    "-OutputRoot"
    $outputRoot
    "-UseExistingGovernanceReports"
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
$rollupSummaryPath = Join-Path $outputRoot "release-blocker-rollup\summary.json"
foreach ($path in @($summaryPath, $markdownPath, $handoffSummaryPath, $rollupSummaryPath)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected pipeline artifact to exist: $path"
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_governance_pipeline_report.v1" `
    -Message "Pipeline summary should expose schema."
Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
    -Message "Pipeline should be blocked by fixture governance reports."
Assert-Equal -Actual ([int]$summary.stage_count) -Expected 6 `
    -Message "Pipeline should run six read-only stages."
Assert-Equal -Actual ([int]$summary.completed_stage_count) -Expected 6 `
    -Message "Pipeline should complete every stage."
Assert-Equal -Actual ([int]$summary.failed_stage_count) -Expected 0 `
    -Message "Pipeline should not record stage failures."
Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 11 `
    -Message "Pipeline should mirror final rollup blocker count."
Assert-True -Condition ([int]$summary.action_item_count -ge 4) `
    -Message "Pipeline should mirror final rollup action count."
Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
    -Message "Pipeline should mirror final rollup warning count."

$stageIds = @($summary.stages | ForEach-Object { [string]$_.id })
foreach ($expectedStage in @(
        "numbering_catalog_governance",
        "table_layout_delivery_governance",
        "content_control_data_binding_governance",
        "project_template_delivery_readiness",
        "release_governance_handoff",
        "release_blocker_rollup"
    )) {
    Assert-ContainsText -Text ($stageIds -join "`n") -ExpectedText $expectedStage `
        -Message "Pipeline should include stage $expectedStage."
}

$handoffStage = @($summary.stages | Where-Object { [string]$_.id -eq "release_governance_handoff" } | Select-Object -First 1)
$rollupStage = @($summary.stages | Where-Object { [string]$_.id -eq "release_blocker_rollup" } | Select-Object -First 1)
Assert-Equal -Actual ([int]$handoffStage[0].warning_count) -Expected 2 `
    -Message "Pipeline should preserve handoff warning count."
Assert-Equal -Actual ([int]$rollupStage[0].warning_count) -Expected 2 `
    -Message "Pipeline should preserve final rollup warning count."
Assert-Equal -Actual ([int]$rollupStage[0].release_blocker_count) -Expected 11 `
    -Message "Pipeline final rollup should include restore audit blockers discovered from the input root."
Assert-ContainsText -Text (($rollupStage[0].release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "style_merge.restore_audit_issues" `
    -Message "Pipeline stage summary should preserve restore audit blocker ids."
Assert-ContainsText -Text (($rollupStage[0].release_blockers | ForEach-Object { [string]$_.action }) -join "`n") `
    -ExpectedText "review_style_merge_restore_audit" `
    -Message "Pipeline stage summary should preserve restore audit blocker actions."
Assert-ContainsText -Text (($rollupStage[0].action_items | ForEach-Object {
            if ($_.PSObject.Properties["open_command"]) { [string]$_.open_command }
        }) -join "`n") `
    -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Pipeline stage summary should preserve restore audit open-latest commands."
$handoffStyleMergeWarning = @($handoffStage[0].warnings | Where-Object { [string]$_.id -eq "document_skeleton.style_merge_suggestions_pending" } | Select-Object -First 1)
$rollupStyleMergeWarning = @($rollupStage[0].warnings | Where-Object { [string]$_.id -eq "document_skeleton.style_merge_suggestions_pending" } | Select-Object -First 1)
Assert-Equal -Actual ([string]$handoffStyleMergeWarning[0].action) -Expected "review_style_merge_suggestions" `
    -Message "Pipeline should preserve normalized warning actions on the handoff stage."
Assert-Equal -Actual ([string]$handoffStyleMergeWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Pipeline should preserve normalized warning source schema on the handoff stage."
Assert-Equal -Actual ([int]$handoffStyleMergeWarning[0].style_merge_suggestion_count) -Expected 2 `
    -Message "Pipeline should preserve normalized warning style merge counts on the handoff stage."
Assert-Equal -Actual ([string]$rollupStyleMergeWarning[0].action) -Expected "review_style_merge_suggestions" `
    -Message "Pipeline should preserve normalized warning actions on the final rollup stage."
Assert-Equal -Actual ([string]$rollupStyleMergeWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Pipeline should preserve normalized warning source schema on the final rollup stage."
Assert-Equal -Actual ([int]$rollupStyleMergeWarning[0].style_merge_suggestion_count) -Expected 2 `
    -Message "Pipeline should preserve normalized warning style merge counts on the final rollup stage."

$handoffSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $handoffSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$handoffSummary.schema) -Expected "featherdoc.release_governance_handoff_report.v1" `
    -Message "Pipeline handoff should expose schema."
Assert-Equal -Actual ([bool]$handoffSummary.release_blocker_rollup.included) -Expected $false `
    -Message "Pipeline handoff should skip nested rollup unless explicitly requested."
Assert-ContainsText -Text (($handoffSummary.reports | Where-Object { $_.id -eq "numbering_catalog_governance" } | ForEach-Object { $_.warnings } | ForEach-Object { [string]$_.id }) -join "`n") `
    -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
    -Message "Pipeline handoff should preserve source warning details."

$rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
Assert-ContainsText -Text (($rollupSummary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
    -ExpectedText "style-merge.restore-audit.summary.json" `
    -Message "Pipeline final rollup should scan restore audit summary files from the governance root."
Assert-ContainsText -Text (($rollupSummary.action_items | ForEach-Object {
            if ($_.PSObject.Properties["open_command"]) { [string]$_.open_command }
        }) -join "`n") `
    -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Pipeline final rollup should preserve restore audit open-latest commands."

$markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
Assert-ContainsText -Text $markdown -ExpectedText "# Release Governance Pipeline" `
    -Message "Pipeline Markdown should include title."
Assert-ContainsText -Text $markdown -ExpectedText "release_blocker_rollup" `
    -Message "Pipeline Markdown should include final rollup stage."
Assert-ContainsText -Text $markdown -ExpectedText "## Release Blockers" `
    -Message "Pipeline Markdown should include release blocker section."
Assert-ContainsText -Text $markdown -ExpectedText "### release_blocker_rollup release blockers" `
    -Message "Pipeline Markdown should include final rollup blocker subsection."
Assert-ContainsText -Text $markdown -ExpectedText "style_merge.restore_audit_issues" `
    -Message "Pipeline Markdown should include restore audit blocker ids."
Assert-ContainsText -Text $markdown -ExpectedText "review_style_merge_restore_audit" `
    -Message "Pipeline Markdown should include restore audit blocker actions."
Assert-ContainsText -Text $markdown -ExpectedText "## Action Items" `
    -Message "Pipeline Markdown should include action item section."
Assert-ContainsText -Text $markdown -ExpectedText "### release_blocker_rollup action items" `
    -Message "Pipeline Markdown should include final rollup action item subsection."
Assert-ContainsText -Text $markdown -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Pipeline Markdown should include restore audit open-latest commands."
Assert-ContainsText -Text $markdown -ExpectedText "## Warnings" `
    -Message "Pipeline Markdown should include warnings section."
Assert-ContainsText -Text $markdown -ExpectedText "### release_governance_handoff warnings" `
    -Message "Pipeline Markdown should include handoff stage warning subsection."
Assert-ContainsText -Text $markdown -ExpectedText "### release_blocker_rollup warnings" `
    -Message "Pipeline Markdown should include final rollup warning subsection."
Assert-ContainsText -Text $markdown -ExpectedText 'action: `review_style_merge_suggestions`' `
    -Message "Pipeline Markdown should include warning actions."
Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' `
    -Message "Pipeline Markdown should include warning source schema."
Assert-ContainsText -Text $markdown -ExpectedText 'style_merge_suggestion_count: `2`' `
    -Message "Pipeline Markdown should include warning style merge counts."

Write-Host "Release governance pipeline report regression passed."
