param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "missing", "failed_report", "fail_on_missing", "fail_on_blocker", "fail_on_warning", "explicit_input", "explicit_only", "include_rollup", "informational_actions")]
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

function Assert-DoesNotContainText {
    param([string]$Text, [string]$UnexpectedText, [string]$Message)
    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Message Unexpected='$UnexpectedText'."
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

function Get-GovernanceTraceItemLabel {
    param($Item, [string]$CollectionName, [int]$Index)

    $itemId = [string]$Item.id
    if ([string]::IsNullOrWhiteSpace($itemId)) {
        $itemId = "item$Index"
    }
    return "$CollectionName $itemId"
}

function Assert-GovernanceTraceMetadata {
    param(
        [object[]]$Items,
        [string]$CollectionName,
        [bool]$ExpectOpenCommand = $false
    )

    $index = 0
    foreach ($item in @($Items)) {
        $index++
        $context = Get-GovernanceTraceItemLabel -Item $item -CollectionName $CollectionName -Index $index

        foreach ($property in @("source_schema", "source_report_display", "source_json_display")) {
            Assert-HasProperty -Object $item -Name $property `
                -Message "$context should expose $property."
            Assert-NonEmptyString -Value $item.$property `
                -Message "$context should keep a non-empty $property."
        }

        if ($ExpectOpenCommand) {
            Assert-HasProperty -Object $item -Name "open_command" `
                -Message "$context should expose reviewer open command metadata."
            Assert-NonEmptyString -Value $item.open_command `
                -Message "$context should keep a non-empty reviewer open command."
        }
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

        $blockText = ($blockLines -join "`n") -replace '/', '\'
        $hasAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($blockText -notmatch [regex]::Escape($normalizedFragment)) {
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

$wordVisualStandardReviewMetadata = @(
    [ordered]@{
        task_key = "smoke"
        review_task_key = "document"
        label = "Word visual smoke"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:10:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\document\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\document\report\final_review.md"
    },
    [ordered]@{
        task_key = "fixed_grid"
        review_task_key = "fixed_grid"
        label = "Fixed-grid merge/unmerge"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:20:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md"
    },
    [ordered]@{
        task_key = "section_page_setup"
        review_task_key = "section_page_setup"
        label = "Section page setup"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:30:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md"
    },
    [ordered]@{
        task_key = "page_number_fields"
        review_task_key = "page_number_fields"
        label = "Page number fields"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:40:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md"
    }
)

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
            catalog_document_keys = @("contract.docx", "invoice.docx")
            baseline_document_keys = @("contract.docx", "invoice.docx")
            matched_document_keys = @("contract.docx", "invoice.docx")
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
                title = "Preview style numbering repair"
            }
        )
        warnings = @(
            [ordered]@{
                id = "numbering_catalog_manifest_summary_missing"
                repair_strategy = "rebuild_numbering_catalog_manifest_summary"
                repair_hint = "Restore the numbering catalog manifest and generate a real manifest check summary; do not synthesize a pass summary when the manifest or catalog outputs are absent."
                command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_manifest.ps1 -ManifestPath .\baselines\numbering-catalog\manifest.json -BuildDir <build-dir> -OutputDir .\output\numbering-catalog-manifest-checks -SkipBuild"
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
            pdf_floating_table_supported_geometry_count = 4
            pdf_floating_table_metadata_only_count = 5
            pdf_floating_table_tracked_geometry_count = 9
            pdf_floating_table_supported_geometry_percent = 44
            pdf_floating_table_support_coverage = "4/9 supported (44 percent); metadata_only=5"
            pdf_floating_table_reviewer_focus = "review metadata-only tblpPr fields before approving PDF-layout-sensitive release."
            pdf_floating_table_metadata_only_fields = @(
                "leftFromText",
                "rightFromText",
                "topFromText outside paragraph anchoring",
                "tblOverlap"
            )
            pdf_floating_table_review_required_fields = @(
                "full Word-compatible floating table text wrapping",
                "table overlap avoidance and collision resolution"
            )
            metadata_only_fields = @(
                "leftFromText",
                "rightFromText",
                "topFromText outside paragraph anchoring",
                "tblOverlap"
            )
            review_required_fields = @(
                "full Word-compatible floating table text wrapping",
                "table overlap avoidance and collision resolution"
            )
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
            next_action = [ordered]@{
                action = "approve_project_template_schema"
                status = "pending_review"
                blocker_id = "project_template_delivery.pending_schema_approval"
                reason = "Schema approval is still pending."
            }
            next_action_summary = @(
                [ordered]@{
                    action = "approve_project_template_schema"
                    status = "pending_review"
                    blocker_id = "project_template_delivery.pending_schema_approval"
                    reason = "Schema approval is still pending."
                }
            )
            next_action_group_count = 1
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
            onboarding_governance_next_action = [ordered]@{
                action = "approve_project_template_schema"
                status = "pending_review"
                blocker_id = "project_template_delivery.pending_schema_approval"
                reason = "Schema approval is still pending."
            }
            onboarding_governance_next_action_summary = @(
                [ordered]@{
                    action = "approve_project_template_schema"
                    status = "pending_review"
                    blocker_id = "project_template_delivery.pending_schema_approval"
                    reason = "Schema approval is still pending."
                }
            )
            onboarding_governance_next_action_group_count = 1
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

    Write-JsonFile -Path (Join-Path $Root "docx-functional-smoke-readiness\summary.json") -Value ([ordered]@{
        schema = "featherdoc.docx_functional_smoke_readiness.v1"
        status = "pass"
        verdict = "pass"
        release_ready = $true
        docx_functional_smoke_ready = $true
        summary_json_display = ".\output\docx-functional-smoke-readiness\summary.json"
        report_markdown_display = ".\output\docx-functional-smoke-readiness\docx_functional_smoke_readiness.md"
        evidence_scope = "persisted_docx_functional_smoke_evidence_only"
        evidence_scope_note = "This read-only gate does not run CMake, CTest, Word, LibreOffice, browsers, or document rendering."
        boundary = "Pass means persisted DOCX functional evidence is coherent, reused visual PNGs are non-empty, and screenshot-backed review verdicts are pass; it does not claim a fresh Word COM render."
        marker = "docx_functional_smoke_readiness_trace"
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
    })
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
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 6 `
        -Message "Aggregate handoff should load all six default reports."
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
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "handoff release blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "handoff warnings"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "handoff action items" `
        -ExpectOpenCommand $true
    Assert-GovernanceTraceMetadata -Items @($summary.informational_action_items) -CollectionName "handoff informational action items" `
        -ExpectOpenCommand $true
    $projectTemplateReport = ($summary.reports |
        Where-Object { [string]$_.id -eq "project_template_delivery_readiness" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$projectTemplateReport.source_report_display) -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Aggregate handoff should preserve project-template report source_report_display."
    Assert-ContainsText -Text ([string]$projectTemplateReport.source_json_display) -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Aggregate handoff should preserve project-template report source_json_display."
    $docxReadinessReport = ($summary.reports |
        Where-Object { [string]$_.id -eq "docx_functional_smoke_readiness" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$docxReadinessReport.evidence_scope) -Expected "persisted_docx_functional_smoke_evidence_only" `
        -Message "Aggregate handoff should expose DOCX readiness evidence scope."
    Assert-Equal -Actual ([string]$docxReadinessReport.marker) -Expected "docx_functional_smoke_readiness_trace" `
        -Message "Aggregate handoff should expose DOCX readiness trace marker."
    Assert-ContainsText -Text ([string]$docxReadinessReport.summary_json_display) -ExpectedText "docx-functional-smoke-readiness\summary.json" `
        -Message "Aggregate handoff should expose DOCX readiness summary display path."
    Assert-ContainsText -Text ([string]$docxReadinessReport.report_markdown_display) -ExpectedText "docx_functional_smoke_readiness.md" `
        -Message "Aggregate handoff should expose DOCX readiness report markdown display path."
    Assert-ContainsText -Text ([string]$docxReadinessReport.boundary) -ExpectedText "does not claim a fresh Word COM render" `
        -Message "Aggregate handoff should expose DOCX readiness boundary."
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
    Assert-Equal -Actual ([string]$projectTemplateBlocker.onboarding_governance_next_action.action) -Expected "approve_project_template_schema" `
        -Message "Aggregate handoff should carry onboarding governance next action on blockers."
    Assert-Equal -Actual ([int]$projectTemplateBlocker.onboarding_governance_next_action_group_count) -Expected 1 `
        -Message "Aggregate handoff should carry onboarding governance next-action group count on blockers."
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
    Assert-Equal -Actual ([string]$projectTemplateAction.onboarding_governance_next_action.action) -Expected "approve_project_template_schema" `
        -Message "Aggregate handoff should carry onboarding governance next action on action items."
    Assert-Equal -Actual ([int]$projectTemplateAction.onboarding_governance_next_action_group_count) -Expected 1 `
        -Message "Aggregate handoff should carry onboarding governance next-action group count on action items."
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
    Assert-ContainsText -Text (($numberingMetric.details.catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Aggregate handoff should preserve numbering catalog document keys."
    Assert-ContainsText -Text (($numberingMetric.details.baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice.docx" `
        -Message "Aggregate handoff should preserve numbering baseline document keys."
    Assert-ContainsText -Text (($numberingMetric.details.matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Aggregate handoff should preserve numbering matched document keys."
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
    Assert-Equal -Actual ([int]$tableMetric.details.pdf_floating_table_tracked_geometry_count) -Expected 9 `
        -Message "Aggregate handoff should preserve tracked PDF floating table geometry detail fields."
    Assert-Equal -Actual ([int]$tableMetric.details.pdf_floating_table_supported_geometry_percent) -Expected 44 `
        -Message "Aggregate handoff should preserve supported PDF floating table geometry percentage detail fields."
    Assert-ContainsText -Text (($tableMetric.details.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "tblOverlap" `
        -Message "Aggregate handoff should preserve generic PDF floating table metadata-only fields."
    Assert-ContainsText -Text (($tableMetric.details.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "table overlap avoidance and collision resolution" `
        -Message "Aggregate handoff should preserve generic PDF floating table review-required fields."
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
    $numberingWarning = @($summary.warnings |
        Where-Object { [string]$_.id -eq "numbering_catalog_manifest_summary_missing" })[0]
    Assert-Equal -Actual ([string]$numberingWarning.repair_strategy) -Expected "rebuild_numbering_catalog_manifest_summary" `
        -Message "Aggregate handoff should preserve warning repair strategy."
    Assert-ContainsText -Text ([string]$numberingWarning.repair_hint) -ExpectedText "do not synthesize" `
        -Message "Aggregate handoff should preserve warning repair hints."
    Assert-ContainsText -Text ([string]$numberingWarning.command_template) -ExpectedText "check_numbering_catalog_manifest.ps1" `
        -Message "Aggregate handoff should preserve warning command templates."
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
    Assert-Equal -Actual ([string]$numberingAction.action) -Expected "preview_style_numbering_repair" `
        -Message "Aggregate handoff should fall back to action item id when action is absent."
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
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`docx_functional_smoke_readiness`' -ExpectedFragments @(
        'docx_functional_smoke_ready:',
        'evidence_scope: `persisted_docx_functional_smoke_evidence_only`',
        'evidence_scope_note:',
        'does not run CMake, CTest, Word, LibreOffice, browsers, or document rendering',
        'boundary:',
        'does not claim a fresh Word COM render',
        'marker: `docx_functional_smoke_readiness_trace`',
        'summary_json_display:',
        'docx-functional-smoke-readiness\summary.json',
        'report_markdown_display:',
        'docx_functional_smoke_readiness.md'
    ) -Message "Markdown should keep DOCX readiness evidence boundary in one report-status block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`project_template_delivery_readiness` / `project_template_delivery.pending_schema_approval`' -ExpectedFragments @(
        'source_report_display:',
        'source_json_display:',
        'readiness_status:',
        'readiness_release_ready:',
        'project_template_onboarding_governance_contract:',
        'status: `pending_review`',
        'release_ready: `False`',
        'schema_approval_status_summary: `pending_review=1`',
        'next_action:',
        'next_action_summary:',
        'next_action_group_count: `1`'
    ) -Message "Markdown should keep project-template readiness status with the handoff blocker evidence block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor '`project_template_delivery_readiness` / `approve_project_template_schema`' -ExpectedFragments @(
        'source_report_display:',
        'source_json_display:',
        'readiness_status:',
        'readiness_release_ready:',
        'project_template_onboarding_governance_contract:',
        'status: `pending_review`',
        'release_ready: `False`',
        'schema_approval_status_summary: `pending_review=1`',
        'next_action:',
        'next_action_summary:',
        'next_action_group_count: `1`'
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
    Assert-ContainsText -Text $markdown -ExpectedText "repair_hint:" `
        -Message "Markdown should include warning repair hints."
    Assert-ContainsText -Text $markdown -ExpectedText "check_numbering_catalog_manifest.ps1" `
        -Message "Markdown should include warning command templates."
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
    Assert-ContainsText -Text $markdown -ExpectedText "catalog_document_keys=contract.docx,invoice.docx" `
        -Message "Markdown should include numbering catalog document keys."
    Assert-ContainsText -Text $markdown -ExpectedText "baseline_document_keys=contract.docx,invoice.docx" `
        -Message "Markdown should include numbering baseline document keys."
    Assert-ContainsText -Text $markdown -ExpectedText "matched_document_keys=contract.docx,invoice.docx" `
        -Message "Markdown should include numbering matched document keys."
    Assert-ContainsText -Text $markdown -ExpectedText "style_numbering_issues(count=3, penalty=15)" `
        -Message "Markdown should include numbering confidence penalty summary."
    Assert-ContainsText -Text $markdown -ExpectedText "unresolved_item_count=0" `
        -Message "Markdown should include table layout delivery detail fields."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_supported_geometry_percent=44" `
        -Message "Markdown should include PDF floating table supported geometry percentage."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_tracked_geometry_count=9" `
        -Message "Markdown should include PDF floating table tracked geometry count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_support_coverage: ``4/9 supported (44 percent); metadata_only=5``" `
        -Message "Markdown should include PDF floating table support coverage summary."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_support_coverage=4/9 supported (44 percent); metadata_only=5" `
        -Message "Markdown should include PDF floating table metadata-only count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_reviewer_focus: ``review metadata-only tblpPr fields before approving PDF-layout-sensitive release.``" `
        -Message "Markdown should include PDF floating table reviewer focus marker."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_reviewer_focus=review metadata-only tblpPr fields before approving PDF-layout-sensitive release." `
        -Message "Markdown should include PDF floating table reviewer focus guidance."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_metadata_only_fields: ``leftFromText, rightFromText, topFromText outside paragraph anchoring, tblOverlap``" `
        -Message "Markdown should include concrete PDF floating table metadata-only fields."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_review_required_fields: ``full Word-compatible floating table text wrapping, table overlap avoidance and collision resolution``" `
        -Message "Markdown should include concrete PDF floating table reviewer-required fields."
    Assert-ContainsText -Text $markdown -ExpectedText "metadata_only_fields=leftFromText,rightFromText,topFromText outside paragraph anchoring,tblOverlap" `
        -Message "Markdown details should include generic PDF floating table metadata-only fields."
    Assert-ContainsText -Text $markdown -ExpectedText "review_required_fields=full Word-compatible floating table text wrapping,table overlap avoidance and collision resolution" `
        -Message "Markdown details should include generic PDF floating table reviewer-required fields."
    Assert-ContainsText -Text $markdown -ExpectedText "metadata_only_fields: ``leftFromText, rightFromText, topFromText outside paragraph anchoring, tblOverlap``" `
        -Message "Markdown should include generic PDF floating table metadata-only field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "review_required_fields: ``full Word-compatible floating table text wrapping, table overlap avoidance and collision resolution``" `
        -Message "Markdown should include generic PDF floating table review-required field marker."
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

if (Test-Scenario -Name "fail_on_blocker") {
    $inputRoot = Join-Path $resolvedWorkingDir "fail-blocker-input"
    $outputDir = Join-Path $resolvedWorkingDir "fail-blocker-report"
    Write-GovernanceFixtures -Root $inputRoot

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Handoff should fail fail-on-blocker when governance blockers are present. Output: $($result.Text)"
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $outputDir "summary.json")) `
        -Message "Fail-on-blocker handoff should still write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")) `
        -Message "Fail-on-blocker handoff should still write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Fail-on-blocker handoff should keep blocker summaries in blocked status."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Fail-on-blocker handoff should still write structured blocker evidence."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "project_template_delivery.pending_schema_approval" `
        -Message "Fail-on-blocker handoff should preserve project-template blockers in summary output."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Release blockers: ``4``" `
        -Message "Fail-on-blocker handoff Markdown should summarize the blocker count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery.pending_schema_approval" `
        -Message "Fail-on-blocker handoff Markdown should include blocker ids."
}

if (Test-Scenario -Name "fail_on_warning") {
    $inputRoot = Join-Path $resolvedWorkingDir "fail-warning-input"
    $explicitRoot = Join-Path $resolvedWorkingDir "fail-warning-explicit"
    $outputDir = Join-Path $resolvedWorkingDir "fail-warning-report"
    $releaseCandidateSummaryPath = Join-Path $explicitRoot "release-candidate-summary.json"
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    Write-JsonFile -Path $releaseCandidateSummaryPath -Value ([ordered]@{
        schema = "featherdoc.release_candidate_summary"
        status = "needs_review"
        release_ready = $false
        warning_count = 1
        warnings = @(
            [ordered]@{
                id = "pdf_controlled_visual_smoke.unavailable_or_failed"
                action = "rerun_pdf_controlled_visual_smoke_check"
                status = "fail"
                message = "Controlled PDF visual smoke evidence was provided but is not passing."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\controlled-visual-smoke-failed.json"
            }
        )
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", $releaseCandidateSummaryPath,
        "-OutputDir", $outputDir,
        "-ExpectedReportProfile", "explicit-only",
        "-FailOnWarning"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Handoff should fail fail-on-warning when explicit PDF warnings are present. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Fail-on-warning handoff should keep warning-only summaries in needs_review status."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Fail-on-warning handoff should still write structured warning evidence."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
        -Message "Fail-on-warning handoff should preserve PDF preflight warnings in summary output."
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
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 6 `
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
    $pdfPreflightGovernanceSummaryPath = Join-Path $explicitRoot "pdf-preflight-governance-summary.json"
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
                "release_blocker_count",
                "warning_count",
                "schema_approval_status_summary",
                "onboarding_governance_next_action",
                "onboarding_governance_next_action_summary",
                "onboarding_governance_next_action_group_count",
                "next_action",
                "next_action_summary",
                "next_action_group_count",
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
            compact_evidence_source_schema = "featherdoc.release_candidate_summary"
            checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
            checklist_marker = "release_entry_project_template_readiness_checklist_trace"
            material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
        }
        word_visual_standard_review_metadata_count = 4
        word_visual_standard_review_metadata = $wordVisualStandardReviewMetadata
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
            pdf_full_ctest_readiness = [ordered]@{
                requested = $true
                status = "pass"
                verdict = "pass_with_warnings"
                release_ready = $true
                summary_json = "output/pdf-release-readiness-current/summary.json"
                full_ctest_summary_json = "build/pdf-ctest-full-current/summary.json"
                full_ctest_summary_json_display = ".\build\pdf-ctest-full-current\summary.json"
                full_ctest_status = "timeout"
                full_ctest_verdict = "not_complete"
                outer_guard_status = "timed_out"
                outer_guard_timed_out = $true
                selected_test_count = 139
                completed_test_count = 133
                passed_test_count = 126
                failed_test_count = 0
                skipped_test_count = 7
                not_run_test_count = 6
                completion_percent = 95.7
                remaining_test_count = 6
                zero_failed_tests_observed = $true
                boundary = "full_ctest_timeout_is_not_release_blocking_when_zero_failures_observed"
                marker = "pdf_full_ctest_readiness_trace"
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
                outer_guard_status = "timed_out"
                outer_guard_timed_out = $true
                outer_guard_timeout_seconds = 60
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
            pdf_visual_segmented_gate = [ordered]@{
                status = "pass"
                verdict = "pass"
                full_visual_gate_status = "not_complete"
                evidence_scope = "segmented_visual_gate_auxiliary_only"
                boundary = "segmented_summary_does_not_replace_full_visual_gate_verdict"
                summary_json = "output/pdf-visual-release-gate-current/report/segmented-summary.json"
                slice_summary_count = 4
                slice_pass_count = 4
                slice_failed_count = 0
                covered_baseline_count = 44
                expected_visual_render_count = 44
                attempt_stage_count = 6
                attempt_passed_stage_count = 6
                visual_baseline_render_status = "pass"
                aggregate_contact_sheet_status = "pass"
                aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
                aggregate_contact_sheet_bytes = 1822428
                aggregate_rebuild_status = "pass"
                aggregate_rebuild_selected_baseline_count = 44
            }
        }
    })
    Write-JsonFile -Path $pdfPreflightGovernanceSummaryPath -Value ([ordered]@{
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
        warning_count = 1
        warnings = @(
            [ordered]@{
                id = "pdf_controlled_visual_smoke.unavailable_or_failed"
                action = "rerun_pdf_controlled_visual_smoke_check"
                status = "fail"
                message = "Controlled PDF visual smoke evidence was provided but is not passing."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\controlled-visual-smoke-failed.json"
            }
        )
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", (@($releaseCandidateSummaryPath, $pdfPreflightGovernanceSummaryPath) -join ","),
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
    Assert-ContainsText -Text ([string]$summary.release_blocker_rollup.summary_json) -ExpectedText "release-blocker-rollup\summary.json" `
        -Message "Handoff summary should expose nested rollup summary display path."
    Assert-DoesNotContainText -Text ([string]$summary.release_blocker_rollup.summary_json) -UnexpectedText $resolvedRepoRoot `
        -Message "Handoff summary should not expose a local absolute nested rollup summary path."
    Assert-Equal -Actual ([string]$summary.release_blocker_rollup.status) -Expected "blocked" `
        -Message "Handoff summary should consume nested rollup status."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 8 `
        -Message "Handoff summary should consume nested rollup source report count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_blocker_count) -Expected 5 `
        -Message "Handoff summary should consume nested rollup blocker count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 6 `
        -Message "Handoff summary should consume nested rollup action item count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.warning_count) -Expected 3 `
        -Message "Handoff summary should consume nested rollup warning count."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.blocker_source_schema_summary | ForEach-Object { "$($_.source_schema):$($_.count)" }) -join "`n") `
        -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1:1" `
        -Message "Handoff summary should consume nested rollup blocker source schema summary."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.action_item_source_schema_summary | ForEach-Object { "$($_.source_schema):$($_.count)" }) -join "`n") `
        -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1:1" `
        -Message "Handoff summary should consume nested rollup action item source schema summary."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.warning_source_schema_summary | ForEach-Object { "$($_.source_schema):$($_.count)" }) -join "`n") `
        -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1:1" `
        -Message "Handoff summary should consume nested rollup warning source schema summary."
    Assert-Equal -Actual (@($summary.release_blocker_rollup.informational_action_item_source_schema_summary).Count) -Expected 0 `
        -Message "Handoff summary should keep empty nested rollup informational action source schema summary."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.governance_metric_count) -Expected 2 `
        -Message "Handoff summary should consume nested rollup governance metric count."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "real_corpus_confidence:low:56" `
        -Message "Handoff summary should consume nested numbering confidence metric."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "delivery_quality:release_ready:100" `
        -Message "Handoff summary should consume nested table delivery metric."
    $summaryNestedNumberingMetric = ($summary.release_blocker_rollup.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text (($summaryNestedNumberingMetric.details.catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Handoff summary should preserve nested numbering catalog document keys."
    Assert-ContainsText -Text (($summaryNestedNumberingMetric.details.baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice.docx" `
        -Message "Handoff summary should preserve nested numbering baseline document keys."
    Assert-ContainsText -Text (($summaryNestedNumberingMetric.details.matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Handoff summary should preserve nested numbering matched document keys."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.docx_functional_smoke_readiness_evidence_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested DOCX functional smoke readiness evidence count."
    $docxRollupEvidence = $summary.release_blocker_rollup.docx_functional_smoke_readiness_evidence_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $docxRollupEvidence) `
        -Message "Handoff summary should expose at least one DOCX functional smoke readiness evidence source report."
    Assert-Equal -Actual ([string]$docxRollupEvidence.evidence_scope) -Expected "persisted_docx_functional_smoke_evidence_only" `
        -Message "Handoff summary should expose DOCX readiness evidence scope from the nested rollup."
    Assert-Equal -Actual ([string]$docxRollupEvidence.marker) -Expected "docx_functional_smoke_readiness_trace" `
        -Message "Handoff summary should expose DOCX readiness marker from the nested rollup."
    Assert-ContainsText -Text ([string]$docxRollupEvidence.summary_json_display) -ExpectedText "docx-functional-smoke-readiness\summary.json" `
        -Message "Handoff summary should expose DOCX readiness summary display from the nested rollup."
    Assert-ContainsText -Text ([string]$docxRollupEvidence.report_markdown_display) -ExpectedText "docx_functional_smoke_readiness.md" `
        -Message "Handoff summary should expose DOCX readiness report markdown display from the nested rollup."
    Assert-ContainsText -Text ([string]$docxRollupEvidence.boundary) -ExpectedText "does not claim a fresh Word COM render" `
        -Message "Handoff summary should expose DOCX readiness boundary from the nested rollup."
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
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_status) -Expected "pass" `
        -Message "Handoff summary should expose PDF full CTest readiness status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_verdict) -Expected "pass_with_warnings" `
        -Message "Handoff summary should expose PDF full CTest readiness verdict from the nested rollup."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_full_ctest_readiness_release_ready) -Expected $true `
        -Message "Handoff summary should expose PDF full CTest release readiness from the nested rollup."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_full_ctest_readiness_summary_json_display) `
        -ExpectedText "pdf-release-readiness-current\summary.json" `
        -Message "Handoff summary should expose PDF release readiness summary display path."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_full_ctest_readiness_full_ctest_summary_json_display) `
        -ExpectedText "pdf-ctest-full-current\summary.json" `
        -Message "Handoff summary should expose PDF full CTest summary display path."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_full_ctest_status) -Expected "timeout" `
        -Message "Handoff summary should expose PDF full CTest observed status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_full_ctest_verdict) -Expected "not_complete" `
        -Message "Handoff summary should expose PDF full CTest verdict."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_outer_guard_status) -Expected "timed_out" `
        -Message "Handoff summary should expose PDF full CTest guard status."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_full_ctest_readiness_outer_guard_timed_out) -Expected $true `
        -Message "Handoff summary should expose PDF full CTest guard timeout flag."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_full_ctest_readiness_selected_test_count) -Expected 139 `
        -Message "Handoff summary should expose PDF full CTest selected count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_full_ctest_readiness_completed_test_count) -Expected 133 `
        -Message "Handoff summary should expose PDF full CTest completed count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_full_ctest_readiness_failed_test_count) -Expected 0 `
        -Message "Handoff summary should expose PDF full CTest observed failure count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_full_ctest_readiness_not_run_test_count) -Expected 6 `
        -Message "Handoff summary should expose PDF full CTest not-run count."
    Assert-Equal -Actual ([double]$pdfEvidence.pdf_full_ctest_readiness_completion_percent) -Expected 95.7 `
        -Message "Handoff summary should expose PDF full CTest completion percent."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_full_ctest_readiness_zero_failed_tests_observed) -Expected $true `
        -Message "Handoff summary should expose PDF full CTest zero-failure observation."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_full_ctest_readiness_marker) -Expected "pdf_full_ctest_readiness_trace" `
        -Message "Handoff summary should expose PDF full CTest readiness marker."
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
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_gate_attempt_outer_guard_status) -Expected "timed_out" `
        -Message "Handoff summary should expose PDF visual gate attempt outer guard status."
    Assert-Equal -Actual ([bool]$pdfEvidence.pdf_visual_gate_attempt_outer_guard_timed_out) -Expected $true `
        -Message "Handoff summary should expose PDF visual gate attempt outer guard timeout flag."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_gate_attempt_outer_guard_timeout_seconds) -Expected 60 `
        -Message "Handoff summary should expose PDF visual gate attempt outer guard timeout seconds."
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
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_status) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate status from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_verdict) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate verdict from the nested rollup."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_full_visual_gate_status) -Expected "not_complete" `
        -Message "Handoff summary should keep segmented evidence separate from the full visual gate status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_evidence_scope) -Expected "segmented_visual_gate_auxiliary_only" `
        -Message "Handoff summary should expose segmented PDF visual gate evidence scope."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_boundary) -Expected "segmented_summary_does_not_replace_full_visual_gate_verdict" `
        -Message "Handoff summary should expose segmented PDF visual gate boundary."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_segmented_gate_summary_json_display) `
        -ExpectedText "pdf-visual-release-gate-current\report\segmented-summary.json" `
        -Message "Handoff summary should expose segmented PDF visual gate summary display path."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_slice_summary_count) -Expected 4 `
        -Message "Handoff summary should expose segmented PDF visual gate slice count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_slice_pass_count) -Expected 4 `
        -Message "Handoff summary should expose segmented PDF visual gate passing slice count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_slice_failed_count) -Expected 0 `
        -Message "Handoff summary should expose segmented PDF visual gate failed slice count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_covered_baseline_count) -Expected 44 `
        -Message "Handoff summary should expose segmented PDF visual gate covered baseline count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_expected_visual_render_count) -Expected 44 `
        -Message "Handoff summary should expose segmented PDF visual gate expected baseline count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_attempt_stage_count) -Expected 6 `
        -Message "Handoff summary should expose segmented PDF visual gate attempt stage count."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_attempt_passed_stage_count) -Expected 6 `
        -Message "Handoff summary should expose segmented PDF visual gate passed attempt stage count."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_visual_baseline_render_status) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate render status."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_aggregate_contact_sheet_status) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate contact-sheet status."
    Assert-ContainsText -Text ([string]$pdfEvidence.pdf_visual_segmented_gate_aggregate_contact_sheet_display) `
        -ExpectedText "aggregate-contact-sheet.png" `
        -Message "Handoff summary should expose segmented PDF visual gate contact-sheet path."
    Assert-Equal -Actual ([int64]$pdfEvidence.pdf_visual_segmented_gate_aggregate_contact_sheet_bytes) -Expected 1822428 `
        -Message "Handoff summary should expose segmented PDF visual gate contact-sheet byte size."
    Assert-Equal -Actual ([string]$pdfEvidence.pdf_visual_segmented_gate_aggregate_rebuild_status) -Expected "pass" `
        -Message "Handoff summary should expose segmented PDF visual gate aggregate rebuild status."
    Assert-Equal -Actual ([int]$pdfEvidence.pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count) -Expected 44 `
        -Message "Handoff summary should expose segmented PDF visual gate aggregate rebuild selected baseline count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.manifest_signoff_entrypoints_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested manifest signoff evidence count."
    $manifestSignoffEvidence = $summary.release_blocker_rollup.manifest_signoff_entrypoints_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $manifestSignoffEvidence) `
        -Message "Handoff summary should expose at least one manifest signoff evidence source report."
    Assert-Equal -Actual ([string]$manifestSignoffEvidence.schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Handoff summary should preserve manifest signoff release-candidate source schema."
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
    $manifestSignoffRequiredFieldsText = @($manifestSignoffEvidence.manifest_signoff_entrypoints_required_fields) -join "`n"
    foreach ($requiredField in @(
        "status",
        "release_ready",
        "release_blocker_count",
        "warning_count",
        "schema_approval_status_summary",
        "onboarding_governance_next_action",
        "onboarding_governance_next_action_summary",
        "onboarding_governance_next_action_group_count",
        "next_action",
        "next_action_summary",
        "next_action_group_count",
        "source_report_display",
        "source_json_display"
    )) {
        Assert-ContainsText -Text $manifestSignoffRequiredFieldsText `
            -ExpectedText $requiredField `
            -Message "Handoff summary should expose manifest signoff required field '$requiredField'."
    }
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
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety compact evidence source schema."
    Assert-Equal -Actual ([string]$releaseEntryChecklistAuditEvidence.release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker) -Expected "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace" `
        -Message "Handoff summary should expose packaged release-entry checklist material-safety marker."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.word_visual_standard_review_metadata_source_report_count) -Expected 1 `
        -Message "Handoff summary should consume nested Word visual standard review metadata evidence count."
    $wordVisualMetadataEvidence = $summary.release_blocker_rollup.word_visual_standard_review_metadata_source_reports | Select-Object -First 1
    Assert-True -Condition ($null -ne $wordVisualMetadataEvidence) `
        -Message "Handoff summary should expose at least one Word visual standard review metadata source report."
    Assert-Equal -Actual ([string]$wordVisualMetadataEvidence.schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Handoff summary should preserve Word visual metadata release-candidate source schema."
    Assert-Equal -Actual ([int]$wordVisualMetadataEvidence.word_visual_standard_review_metadata_count) -Expected 4 `
        -Message "Handoff summary should expose Word visual standard review metadata count."
    Assert-ContainsText -Text (@($wordVisualMetadataEvidence.word_visual_standard_review_task_keys) -join "`n") `
        -ExpectedText "page_number_fields" `
        -Message "Handoff summary should expose Word visual standard review task keys."
    Assert-Equal -Actual ([string]$wordVisualMetadataEvidence.word_visual_standard_review_status_summary) -Expected "reviewed=4" `
        -Message "Handoff summary should expose Word visual standard review status summary."
    Assert-Equal -Actual ([string]$wordVisualMetadataEvidence.word_visual_standard_review_verdict_summary) -Expected "pass=4" `
        -Message "Handoff summary should expose Word visual standard review verdict summary."
    $handoffWordVisualMetadata = @($wordVisualMetadataEvidence.word_visual_standard_review_metadata)
    Assert-Equal -Actual $handoffWordVisualMetadata.Count -Expected 4 `
        -Message "Handoff summary should expose four Word visual standard review metadata entries."
    $handoffWordVisualMetadataByTask = @{}
    foreach ($entry in $handoffWordVisualMetadata) {
        $handoffWordVisualMetadataByTask[[string]$entry.task_key] = $entry
        Assert-True -Condition ($null -eq $entry.PSObject.Properties["review_note"]) `
            -Message "Handoff summary should not expose Word visual standard review notes."
    }
    $handoffSmokeWordVisualMetadata = $handoffWordVisualMetadataByTask["smoke"]
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.review_task_key) -Expected "document" `
        -Message "Handoff summary should preserve the smoke Word visual review task key."
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.label) -Expected "Word visual smoke" `
        -Message "Handoff summary should preserve the smoke Word visual review label."
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.verdict) -Expected "pass" `
        -Message "Handoff summary should preserve the smoke Word visual review verdict."
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.review_status) -Expected "reviewed" `
        -Message "Handoff summary should preserve the smoke Word visual review status."
    Assert-Equal -Actual ([string]$handoffSmokeWordVisualMetadata.review_method) -Expected "operator_supplied" `
        -Message "Handoff summary should preserve the smoke Word visual review method."
    Assert-ContainsText -Text ([string]$handoffSmokeWordVisualMetadata.review_result_path) `
        -ExpectedText "review_result.json" `
        -Message "Handoff summary should preserve the smoke Word visual review result path."
    Assert-ContainsText -Text ([string]$handoffSmokeWordVisualMetadata.final_review_path) `
        -ExpectedText "final_review.md" `
        -Message "Handoff summary should preserve the smoke Word visual final review path."
    Assert-DoesNotContainText -Text ($summary | ConvertTo-Json -Depth 32) -UnexpectedText "review_note" `
        -Message "Handoff summary JSON should not expose private Word visual review notes."

    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Nested rollup should expose release blocker rollup schema."
    Assert-Equal -Actual ([int]$rollupSummary.source_report_count) -Expected 8 `
        -Message "Nested rollup should consume all loaded governance reports and explicit release-candidate evidence."
    Assert-Equal -Actual ([int]$rollupSummary.release_blocker_count) -Expected 5 `
        -Message "Nested rollup should preserve blocker count."
    Assert-Equal -Actual ([int]$rollupSummary.action_item_count) -Expected 6 `
        -Message "Nested rollup should preserve action item count."
    Assert-Equal -Actual ([int]$rollupSummary.governance_metric_count) -Expected 2 `
        -Message "Nested rollup should preserve governance metric count."
    Assert-Equal -Actual (
        ($summary.release_blocker_rollup.blocker_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Expected (
        ($rollupSummary.blocker_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Message "Handoff summary should mirror nested blocker source schema summary."
    Assert-Equal -Actual (
        ($summary.release_blocker_rollup.action_item_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Expected (
        ($rollupSummary.action_item_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Message "Handoff summary should mirror nested action item source schema summary."
    Assert-Equal -Actual (
        ($summary.release_blocker_rollup.warning_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Expected (
        ($rollupSummary.warning_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Message "Handoff summary should mirror nested warning source schema summary."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "real_corpus_confidence:low:56" `
        -Message "Nested rollup should preserve numbering confidence metric."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.id):$($_.report_id):$($_.source_schema)" }) -join "`n") `
        -ExpectedText "numbering_catalog_governance.real_corpus_confidence:numbering_catalog_governance:featherdoc.numbering_catalog_governance_report.v1" `
        -Message "Nested rollup should preserve numbering confidence metric contract."
    Assert-ContainsText -Text (($rollupSummary.governance_metrics | ForEach-Object { "$($_.metric):$($_.level):$($_.score)" }) -join "`n") `
        -ExpectedText "delivery_quality:release_ready:100" `
        -Message "Nested rollup should preserve table delivery quality metric."
    $rollupNumberingMetric = ($rollupSummary.governance_metrics |
        Where-Object { [string]$_.id -eq "numbering_catalog_governance.real_corpus_confidence" } |
        Select-Object -First 1)
    Assert-ContainsText -Text (($rollupNumberingMetric.details.catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Nested rollup should preserve numbering catalog document keys."
    Assert-ContainsText -Text (($rollupNumberingMetric.details.baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice.docx" `
        -Message "Nested rollup should preserve numbering baseline document keys."
    Assert-ContainsText -Text (($rollupNumberingMetric.details.matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract.docx" `
        -Message "Nested rollup should preserve numbering matched document keys."
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
    Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
        -Message "Nested rollup should preserve PDF preflight warnings."
    Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1" `
        -Message "Nested rollup should preserve PDF preflight warning source schema."
    Assert-ContainsText -Text (($rollupSummary.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
        -ExpectedText "controlled-visual-smoke-failed.json" `
        -Message "Nested rollup should preserve PDF preflight warning source JSON display."
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
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema) -Expected "featherdoc.release_candidate_summary" `
        -Message "Nested rollup should preserve packaged release-entry checklist material-safety compact evidence source schema."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_full_ctest_readiness_status) -Expected "pass" `
        -Message "Nested rollup should preserve PDF full CTest readiness status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_full_ctest_readiness_full_ctest_status) -Expected "timeout" `
        -Message "Nested rollup should preserve PDF full CTest observed status."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_full_ctest_readiness_completed_test_count) -Expected 133 `
        -Message "Nested rollup should preserve PDF full CTest completed count."
    Assert-Equal -Actual ([bool]$rollupReleaseCandidateSourceReport.pdf_full_ctest_readiness_zero_failed_tests_observed) -Expected $true `
        -Message "Nested rollup should preserve PDF full CTest zero-failure observation."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_status) -Expected "partial" `
        -Message "Nested rollup should preserve PDF visual gate attempt status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_verdict) -Expected "not_complete" `
        -Message "Nested rollup should preserve PDF visual gate attempt verdict."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_status) -Expected "timed_out" `
        -Message "Nested rollup should preserve PDF visual gate attempt outer guard status."
    Assert-Equal -Actual ([bool]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_timed_out) -Expected $true `
        -Message "Nested rollup should preserve PDF visual gate attempt outer guard timeout flag."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_outer_guard_timeout_seconds) -Expected 60 `
        -Message "Nested rollup should preserve PDF visual gate attempt outer guard timeout seconds."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_pdf_regression_skipped_test_count) -Expected 7 `
        -Message "Nested rollup should preserve PDF visual gate attempt skipped count."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count) -Expected 22 `
        -Message "Nested rollup should preserve PDF visual gate attempt fresh rendered count."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_segmented_gate_status) -Expected "pass" `
        -Message "Nested rollup should preserve segmented PDF visual gate status."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.pdf_visual_segmented_gate_full_visual_gate_status) -Expected "not_complete" `
        -Message "Nested rollup should keep segmented evidence separate from full visual gate pass."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.pdf_visual_segmented_gate_covered_baseline_count) -Expected 44 `
        -Message "Nested rollup should preserve segmented PDF visual gate coverage."
    Assert-Equal -Actual ([int]$rollupReleaseCandidateSourceReport.word_visual_standard_review_metadata_count) -Expected 4 `
        -Message "Nested rollup should preserve Word visual standard review metadata count."
    Assert-ContainsText -Text (@($rollupReleaseCandidateSourceReport.word_visual_standard_review_task_keys) -join "`n") `
        -ExpectedText "section_page_setup" `
        -Message "Nested rollup should preserve Word visual standard review task keys."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.word_visual_standard_review_status_summary) -Expected "reviewed=4" `
        -Message "Nested rollup should preserve Word visual standard review status summary."
    Assert-Equal -Actual ([string]$rollupReleaseCandidateSourceReport.word_visual_standard_review_verdict_summary) -Expected "pass=4" `
        -Message "Nested rollup should preserve Word visual standard review verdict summary."
    $nestedWordVisualMetadata = @($rollupReleaseCandidateSourceReport.word_visual_standard_review_metadata)
    Assert-Equal -Actual $nestedWordVisualMetadata.Count -Expected 4 `
        -Message "Nested rollup should preserve four Word visual standard review metadata entries."
    $nestedSmokeWordVisualMetadata = $nestedWordVisualMetadata |
        Where-Object { [string]$_.task_key -eq "smoke" } |
        Select-Object -First 1
    Assert-Equal -Actual ([string]$nestedSmokeWordVisualMetadata.review_task_key) -Expected "document" `
        -Message "Nested rollup should preserve the smoke Word visual review task key."
    Assert-Equal -Actual ([string]$nestedSmokeWordVisualMetadata.review_status) -Expected "reviewed" `
        -Message "Nested rollup should preserve the smoke Word visual review status."
    Assert-ContainsText -Text ([string]$nestedSmokeWordVisualMetadata.review_result_path) `
        -ExpectedText "review_result.json" `
        -Message "Nested rollup should preserve the smoke Word visual review result path."
    Assert-DoesNotContainText -Text ($rollupSummary | ConvertTo-Json -Depth 32) -UnexpectedText "review_note" `
        -Message "Nested rollup summary should not expose private Word visual review notes."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Blocker source schemas:" `
        -Message "Handoff Markdown should expose nested rollup blocker source schema summary label."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1=1" `
        -Message "Handoff Markdown should expose nested rollup blocker source schema summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Action item source schemas:" `
        -Message "Handoff Markdown should expose nested rollup action item source schema summary label."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1=1" `
        -Message "Handoff Markdown should expose nested rollup action item source schema summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Informational action item source schemas: ``(none)``" `
        -Message "Handoff Markdown should expose empty nested rollup informational action source schema summary."
    Assert-ContainsText -Text $markdown -ExpectedText "Warning source schemas:" `
        -Message "Handoff Markdown should expose nested rollup warning source schema summary label."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1=1" `
        -Message "Handoff Markdown should expose nested rollup warning source schema summary."
    Assert-ContainsText -Text $markdown -ExpectedText "DOCX functional smoke readiness evidence source reports: ``1``" `
        -Message "Handoff Markdown should expose the DOCX functional smoke readiness evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "evidence_scope: ``persisted_docx_functional_smoke_evidence_only``" `
        -Message "Handoff Markdown should expose the DOCX evidence scope."
    Assert-ContainsText -Text $markdown -ExpectedText "marker: ``docx_functional_smoke_readiness_trace``" `
        -Message "Handoff Markdown should expose the DOCX readiness marker."
    Assert-ContainsText -Text $markdown -ExpectedText "docx_functional_smoke_readiness.md" `
        -Message "Handoff Markdown should expose the DOCX readiness report markdown display."
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
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_status: ``pass``" `
        -Message "Handoff Markdown should expose PDF full CTest readiness status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_verdict: ``pass_with_warnings``" `
        -Message "Handoff Markdown should expose PDF full CTest readiness verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_full_ctest_status: ``timeout``" `
        -Message "Handoff Markdown should expose PDF full CTest observed status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_completed_test_count: ``133``" `
        -Message "Handoff Markdown should expose PDF full CTest completed count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_full_ctest_readiness_zero_failed_tests_observed: ``True``" `
        -Message "Handoff Markdown should expose PDF full CTest zero-failure observation."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_status: ``partial``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_verdict: ``not_complete``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt verdict."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt evidence scope."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_status: ``timed_out``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt outer guard status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_timed_out: ``True``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt outer guard timeout flag."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt outer guard timeout seconds."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt skipped count."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``" `
        -Message "Handoff Markdown should expose the PDF visual gate attempt render status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_segmented_gate_status: ``pass``" `
        -Message "Handoff Markdown should expose segmented PDF visual gate status."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``" `
        -Message "Handoff Markdown should expose segmented PDF visual gate full-status boundary."
    Assert-ContainsText -Text $markdown -ExpectedText "segmented_summary_does_not_replace_full_visual_gate_verdict" `
        -Message "Handoff Markdown should expose segmented PDF visual gate boundary."
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
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "manifest_signoff_entrypoints_status: ``declared``",
        "manifest_signoff_entrypoints_release_assets_manifest_display:",
        "release_assets_manifest.json",
        "manifest_signoff_entrypoints_required_entrypoint_count: ``3``",
        "manifest_signoff_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``",
        "manifest_signoff_entrypoints_required_contracts: ``project_template_delivery_readiness_contract, project_template_onboarding_governance_contract``",
        "manifest_signoff_entrypoints_required_fields: ``status, release_ready, release_blocker_count, warning_count, schema_approval_status_summary, onboarding_governance_next_action, onboarding_governance_next_action_summary, onboarding_governance_next_action_group_count, next_action, next_action_summary, next_action_group_count, source_report_display, source_json_display``",
        "manifest_signoff_entrypoints_checklist_marker: ``reviewer_manifest_scoped_project_template_trace``"
    ) -Message "Handoff Markdown should keep manifest signoff evidence and release-candidate source identity in one source_report block."
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
    Assert-ContainsText -Text $markdown -ExpectedText "Word visual standard review metadata source reports: ``1``" `
        -Message "Handoff Markdown should expose Word visual standard review metadata evidence count."
    Assert-ContainsText -Text $markdown -ExpectedText "word_visual_standard_review_metadata_count: ``4``" `
        -Message "Handoff Markdown should expose Word visual standard review metadata count."
    Assert-ContainsText -Text $markdown -ExpectedText "word_visual_standard_review_task_keys: ``smoke, fixed_grid, section_page_setup, page_number_fields``" `
        -Message "Handoff Markdown should expose Word visual standard review task keys."
    Assert-ContainsText -Text $markdown -ExpectedText "word_visual_standard_review_status_summary: ``reviewed=4``" `
        -Message "Handoff Markdown should expose Word visual standard review status summary."
    Assert-ContainsText -Text $markdown -ExpectedText "word_visual_standard_review_verdict_summary: ``pass=4``" `
        -Message "Handoff Markdown should expose Word visual standard review verdict summary."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "Word visual standard review metadata source reports" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "``smoke``: review_task_key=``document`` verdict=``pass`` review_status=``reviewed`` review_method=``operator_supplied``",
        "label: ``Word visual smoke``",
        "review_result.json",
        "final_review.md"
    ) -Message "Handoff Markdown should keep Word visual metadata and release-candidate source identity in one evidence block."
    Assert-DoesNotContainText -Text $markdown -UnexpectedText "review_note" `
        -Message "Handoff Markdown should not expose private Word visual review notes."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "schema=``featherdoc.docx_functional_smoke_readiness.v1``" -ExpectedFragments @(
        "status: ``loaded``",
        "verdict: ``pass``",
        "release_ready: ``True``",
        "docx_functional_smoke_ready: ``True``",
        "evidence_scope: ``persisted_docx_functional_smoke_evidence_only``",
        "does not run CMake, CTest, Word, LibreOffice, browsers, or document rendering",
        "does not claim a fresh Word COM render",
        "marker: ``docx_functional_smoke_readiness_trace``",
        "summary_json_display:",
        "docx-functional-smoke-readiness\summary.json",
        "report_markdown_display:",
        "docx_functional_smoke_readiness.md"
    ) -Message "Handoff Markdown should keep DOCX readiness evidence and source identity in one source_report block."
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
        "pdf-ctest-bounded-regression-business-samples-current\summary.json",
        "pdf_full_ctest_readiness_status: ``pass``",
        "pdf_full_ctest_readiness_verdict: ``pass_with_warnings``",
        "pdf_full_ctest_readiness_release_ready: ``True``",
        "pdf_full_ctest_readiness_summary_json_display:",
        "pdf-release-readiness-current\summary.json",
        "pdf_full_ctest_readiness_full_ctest_summary_json_display:",
        "pdf-ctest-full-current\summary.json",
        "pdf_full_ctest_readiness_full_ctest_status: ``timeout``",
        "pdf_full_ctest_readiness_full_ctest_verdict: ``not_complete``",
        "pdf_full_ctest_readiness_selected_test_count: ``139``",
        "pdf_full_ctest_readiness_completed_test_count: ``133``",
        "pdf_full_ctest_readiness_failed_test_count: ``0``",
        "pdf_full_ctest_readiness_not_run_test_count: ``6``",
        "pdf_full_ctest_readiness_completion_percent: ``95.7``",
        "pdf_full_ctest_readiness_zero_failed_tests_observed: ``True``",
        "pdf_full_ctest_readiness_marker: ``pdf_full_ctest_readiness_trace``",
        "pdf_visual_gate_attempt_status: ``partial``",
        "pdf_visual_gate_attempt_verdict: ``not_complete``",
        "pdf_visual_gate_attempt_full_visual_gate_status: ``not_complete``",
        "pdf_visual_gate_attempt_evidence_scope: ``bounded_attempt_auxiliary_only``",
        "pdf_visual_gate_attempt_summary_json_display:",
        "attempt-summary.json",
        "pdf_visual_gate_attempt_outer_guard_status: ``timed_out``",
        "pdf_visual_gate_attempt_outer_guard_timed_out: ``True``",
        "pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``60``",
        "pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``91``",
        "pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``0``",
        "pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``7``",
        "pdf_visual_gate_attempt_visual_baseline_render_status: ``partial``",
        "pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``22``",
        "pdf_visual_gate_attempt_expected_visual_render_count: ``44``",
        "pdf_visual_segmented_gate_status: ``pass``",
        "pdf_visual_segmented_gate_verdict: ``pass``",
        "pdf_visual_segmented_gate_full_visual_gate_status: ``not_complete``",
        "pdf_visual_segmented_gate_evidence_scope: ``segmented_visual_gate_auxiliary_only``",
        "pdf_visual_segmented_gate_boundary: ``segmented_summary_does_not_replace_full_visual_gate_verdict``",
        "pdf_visual_segmented_gate_summary_json_display:",
        "segmented-summary.json",
        "pdf_visual_segmented_gate_slice_summary_count: ``4``",
        "pdf_visual_segmented_gate_slice_pass_count: ``4``",
        "pdf_visual_segmented_gate_slice_failed_count: ``0``",
        "pdf_visual_segmented_gate_covered_baseline_count: ``44``",
        "pdf_visual_segmented_gate_expected_visual_render_count: ``44``",
        "pdf_visual_segmented_gate_attempt_stage_count: ``6``",
        "pdf_visual_segmented_gate_attempt_passed_stage_count: ``6``",
        "pdf_visual_segmented_gate_visual_baseline_render_status: ``pass``",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``pass``",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_display:",
        "aggregate-contact-sheet.png",
        "pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``1822428``",
        "pdf_visual_segmented_gate_aggregate_rebuild_status: ``pass``",
        "pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``44``"
    ) -Message "Handoff Markdown should keep PDF visual gate source-report evidence in one source_report block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "project_template_readiness_checklist_entrypoints_status: ``declared``",
        "project_template_readiness_checklist_entrypoints_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``",
        "project_template_readiness_checklist_entrypoints_entrypoint_ids: ``start_here, artifact_guide, reviewer_checklist``",
        "project_template_readiness_checklist_entrypoints_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``"
    ) -Message "Handoff Markdown should keep project-template checklist entrypoint evidence and release-candidate source identity in one source_report block."
    Assert-MarkdownListBlockContainsAll -Text $markdown -Anchor "source_report:" -ExpectedFragments @(
        "schema=``featherdoc.release_candidate_summary``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_status: ``passed``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_script: ``.\scripts\assert_release_material_safety.ps1``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``3``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``start_here, artifact_guide, reviewer_checklist``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``Project-template readiness checklist handoff evidence``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``project_template_readiness_checklist_entrypoints_source_reports``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``featherdoc.release_candidate_summary``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``docs/project_template_release_readiness_checklist_zh.rst``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``release_entry_project_template_readiness_checklist_trace``",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace``"
    ) -Message "Handoff Markdown should keep release-entry checklist material-safety audit evidence in one source_report block."
}

if (Test-Scenario -Name "explicit_only") {
    $inputRoot = Join-Path $resolvedWorkingDir "explicit-only-input"
    $explicitRoot = Join-Path $resolvedWorkingDir "explicit-only-explicit"
    $outputDir = Join-Path $resolvedWorkingDir "explicit-only-report"
    $releaseCandidateSummaryPath = Join-Path $explicitRoot "release-candidate-summary.json"
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    Write-JsonFile -Path $releaseCandidateSummaryPath -Value ([ordered]@{
        schema = "featherdoc.release_candidate_summary"
        status = "pass"
        release_ready = $true
        release_evidence_scope = "pdf-only"
        warning_count = 0
        warnings = @()
        pdf_visual_gate_summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
        pdf_visual_gate = [ordered]@{
            status = "loaded"
            verdict = "pass"
            full_visual_gate_status = "pass"
            finalizable = $true
            summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
            aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
            cjk_manifest_count = 43
            cjk_copy_search_count = 43
            cjk_missing_text_count = 0
            visual_baseline_manifest_count = 42
            visual_baseline_count = 44
        }
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", $releaseCandidateSummaryPath,
        "-OutputDir", $outputDir,
        "-ExpectedReportProfile", "explicit-only",
        "-IncludeReleaseBlockerRollup"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Explicit-only handoff should accept a PDF-only release candidate without requiring non-PDF reports. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Explicit-only handoff should be ready when explicit PDF evidence has no blockers or warnings."
    Assert-Equal -Actual ([string]$summary.expected_report_profile) -Expected "explicit-only" `
        -Message "Explicit-only handoff should record the expected report profile."
    Assert-Equal -Actual ([int]$summary.expected_report_count) -Expected 0 `
        -Message "Explicit-only handoff should not count default all-project governance reports."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Explicit-only handoff should not mark omitted non-PDF reports missing."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 1 `
        -Message "Explicit-only handoff should still build a rollup from the explicit release candidate."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.pdf_visual_gate_evidence_source_report_count) -Expected 1 `
        -Message "Explicit-only handoff should preserve PDF visual gate release evidence."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.manifest_signoff_entrypoints_source_report_count) -Expected 0 `
        -Message "PDF-only explicit handoff should not synthesize project-template manifest signoff evidence."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_report_count) -Expected 0 `
        -Message "PDF-only explicit handoff should not synthesize project-template checklist entrypoint evidence."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Reports loaded: ``0`` / ``0``" `
        -Message "Explicit-only handoff Markdown should show that no default reports were expected."
    Assert-ContainsText -Text $markdown -ExpectedText "PDF visual gate evidence source reports: ``1``" `
        -Message "Explicit-only handoff Markdown should expose PDF visual evidence from the explicit release candidate."
}

if (Test-Scenario -Name "informational_actions") {
    $inputRoot = Join-Path $resolvedWorkingDir "informational-actions-input"
    $explicitRoot = Join-Path $resolvedWorkingDir "informational-actions-explicit"
    $outputDir = Join-Path $resolvedWorkingDir "informational-actions-report"
    $numberingSummaryPath = Join-Path $explicitRoot "numbering-summary.json"
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    Write-JsonFile -Path $numberingSummaryPath -Value ([ordered]@{
        schema = "featherdoc.numbering_catalog_governance_report.v1"
        status = "clean"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        informational_action_item_count = 2
        informational_action_items = @(
            [ordered]@{
                id = "promote_numbering_catalog_exemplar"
                action = "promote_numbering_catalog_exemplar"
                title = "Review and promote the generated exemplar numbering catalog"
                category = "release_checklist"
                severity = "info"
                release_blocking = $false
                optional = $true
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report = "output/document-skeleton-governance-rollup/summary.json"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
                open_command = "featherdoc_cli check-numbering-catalog samples/invoice.docx --json"
            },
            [ordered]@{
                id = "register_numbering_catalog_baseline"
                action = "register_numbering_catalog_baseline"
                title = "Register the exemplar catalog in the numbering catalog baseline flow"
                category = "release_checklist"
                severity = "info"
                release_blocking = $false
                optional = $true
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report = "output/document-skeleton-governance-rollup/summary.json"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx samples/invoice.docx"
            }
        )
        warning_count = 0
        warnings = @()
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", $numberingSummaryPath,
        "-OutputDir", $outputDir,
        "-ExpectedReportProfile", "explicit-only",
        "-IncludeReleaseBlockerRollup"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Handoff should accept informational release checklist actions without creating actionable release items. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 0 `
        -Message "Handoff should not count informational release checklist entries as actionable items."
    Assert-Equal -Actual ([int]$summary.informational_action_item_count) -Expected 2 `
        -Message "Handoff should preserve informational release checklist entries."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 0 `
        -Message "Nested rollup should not count informational release checklist entries as actionable items."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.informational_action_item_count) -Expected 2 `
        -Message "Nested rollup should preserve informational release checklist entries."
    $rollupSummaryPath = Join-Path $outputDir "release-blocker-rollup\summary.json"
    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
    Assert-ContainsText -Text (($summary.release_blocker_rollup.informational_action_item_source_schema_summary | ForEach-Object { "$($_.source_schema):$($_.count)" }) -join "`n") `
        -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1:2" `
        -Message "Handoff summary should consume nested informational action source schema summary."
    Assert-Equal -Actual (
        ($summary.release_blocker_rollup.informational_action_item_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Expected (
        ($rollupSummary.informational_action_item_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Message "Handoff summary should mirror nested informational action source schema summary."
    foreach ($id in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline")) {
        Assert-Equal -Actual (@($summary.action_items | Where-Object { [string]$_.id -eq $id }).Count) -Expected 0 `
            -Message "Handoff should not duplicate informational release checklist action '$id' as an actionable item."
        Assert-Equal -Actual (@($rollupSummary.action_items | Where-Object { [string]$_.id -eq $id }).Count) -Expected 0 `
            -Message "Nested rollup should not duplicate informational release checklist action '$id' as an actionable item."
    }

    $promoteAction = ($summary.informational_action_items |
        Where-Object { [string]$_.id -eq "promote_numbering_catalog_exemplar" } |
        Select-Object -First 1)
    $registerAction = ($summary.informational_action_items |
        Where-Object { [string]$_.id -eq "register_numbering_catalog_baseline" } |
        Select-Object -First 1)
    foreach ($item in @($promoteAction, $registerAction)) {
        Assert-NonEmptyString -Value $item `
            -Message "Handoff should preserve informational release checklist action details."
        Assert-Equal -Actual ([string]$item.source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
            -Message "Handoff informational action should preserve source schema."
        Assert-ContainsText -Text ([string]$item.source_json_display) -ExpectedText "document-skeleton-governance-rollup\summary.json" `
            -Message "Handoff informational action should preserve source JSON display."
        Assert-NonEmptyString -Value $item.open_command `
            -Message "Handoff informational action should preserve reviewer open command."
        Assert-Equal -Actual ([string]$item.release_blocking) -Expected "False" `
            -Message "Handoff informational action should remain non-blocking."
        Assert-Equal -Actual ([string]$item.optional) -Expected "True" `
            -Message "Handoff informational action should remain optional."
    }
    Assert-ContainsText -Text ([string]$promoteAction.open_command) -ExpectedText "check-numbering-catalog" `
        -Message "Handoff promote informational action should keep the exemplar review command."
    Assert-ContainsText -Text ([string]$registerAction.open_command) -ExpectedText "check_numbering_catalog_baseline.ps1" `
        -Message "Handoff register informational action should keep the baseline registration command."

    $nestedInformationalActions = @($rollupSummary.informational_action_items)
    foreach ($item in @($nestedInformationalActions)) {
        Assert-Equal -Actual ([string]$item.source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
            -Message "Nested rollup informational action should preserve source schema."
        Assert-ContainsText -Text ([string]$item.source_json_display) -ExpectedText "document-skeleton-governance-rollup\summary.json" `
            -Message "Nested rollup informational action should preserve source JSON display."
        Assert-NonEmptyString -Value $item.open_command `
            -Message "Nested rollup informational action should preserve reviewer open command."
        Assert-Equal -Actual ([string]$item.release_blocking) -Expected "False" `
            -Message "Nested rollup informational action should remain non-blocking."
        Assert-Equal -Actual ([string]$item.optional) -Expected "True" `
            -Message "Nested rollup informational action should remain optional."
    }

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Informational Action Items" `
        -Message "Handoff Markdown should expose informational action items separately."
    Assert-ContainsText -Text $markdown -ExpectedText "release_blocking=``False`` optional=``True``" `
        -Message "Handoff Markdown should mark informational actions as non-blocking optional checklist entries."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Handoff Markdown should include informational action source schema."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display:" `
        -Message "Handoff Markdown should include informational action source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "document-skeleton-governance-rollup\summary.json" `
        -Message "Handoff Markdown should point informational actions at source JSON."
    Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
        -Message "Handoff Markdown should include informational action open commands."
    Assert-ContainsText -Text $markdown -ExpectedText "check-numbering-catalog" `
        -Message "Handoff Markdown should include the exemplar review command."
    Assert-ContainsText -Text $markdown -ExpectedText "check_numbering_catalog_baseline.ps1" `
        -Message "Handoff Markdown should include the baseline registration command."
}

Write-Host "Release governance handoff report regression passed."
