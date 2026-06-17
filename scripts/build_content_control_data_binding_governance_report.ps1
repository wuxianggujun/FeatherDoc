<#
.SYNOPSIS
Builds a content-control data-binding governance report.

.DESCRIPTION
Reads existing inspect-content-controls and sync-content-controls-from-custom-xml
JSON outputs, then writes a read-only JSON/Markdown handoff for Custom XML data
binding coverage, sync issues, placeholder risk, lock review, and action items.
The script does not open DOCX files or rerun CLI commands.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/content-control-data-binding-governance",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnBlocker,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

$contentControlGovernanceSchema = "featherdoc.content_control_data_binding_governance_report.v1"
$contentControlGovernanceOpenCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"
. (Join-Path $PSScriptRoot "build_content_control_data_binding_governance_report_helpers.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "content_control_data_binding_governance.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
$summaryDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) input JSON file(s)"

$contentControls = New-Object 'System.Collections.Generic.List[object]'
$syncItems = New-Object 'System.Collections.Generic.List[object]'
$syncIssues = New-Object 'System.Collections.Generic.List[object]'
$sourceFiles = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

foreach ($path in @($inputPaths)) {
    $kind = "unknown"
    $status = "ignored"
    $errorMessage = ""
    $json = $null
    $provenance = $null
    if (-not (Test-Path -LiteralPath $path)) {
        $kind = "missing"
        $status = "missing"
        $warnings.Add([ordered]@{
            id = "input_json_missing"
            action = "collect_content_control_data_binding_evidence"
            source_schema = $contentControlGovernanceSchema
            source_report = $summaryPath
            source_report_display = $summaryDisplay
            source_json = $path
            source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            message = "Input JSON was not found."
        }) | Out-Null
    } else {
        try {
            $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
            $provenance = New-ContentControlEvidenceProvenance -RepoRoot $repoRoot -Path $path -Json $json
            $kind = Get-EvidenceKind -Json $json
            switch ($kind) {
                "inspect_content_controls" {
                    foreach ($item in @(Convert-InspectContentControls -RepoRoot $repoRoot -Path $path -Json $json -Provenance $provenance)) {
                        $contentControls.Add($item) | Out-Null
                    }
                    $status = "loaded"
                }
                "custom_xml_sync_result" {
                    foreach ($item in @(Convert-SyncItems -RepoRoot $repoRoot -Path $path -Json $json -Provenance $provenance)) {
                        $syncItems.Add($item) | Out-Null
                    }
                    foreach ($issue in @(Convert-SyncIssues -RepoRoot $repoRoot -Path $path -Json $json -Provenance $provenance)) {
                        $syncIssues.Add($issue) | Out-Null
                    }
                    $status = "loaded"
                }
                "content_control_data_binding_governance_report" {
                    foreach ($item in @(Get-JsonArray -Object $json -Name "content_controls")) {
                        $contentControls.Add($item) | Out-Null
                    }
                    foreach ($item in @(Get-JsonArray -Object $json -Name "sync_items")) {
                        $syncItems.Add($item) | Out-Null
                    }
                    foreach ($issue in @(Get-JsonArray -Object $json -Name "sync_issues")) {
                        $syncIssues.Add($issue) | Out-Null
                    }
                    $status = "loaded"
                }
                default {
                    $status = "skipped"
                    $warnings.Add([ordered]@{
                        id = "source_json_schema_skipped"
                        action = "review_content_control_data_binding_evidence"
                        source_schema = $contentControlGovernanceSchema
                        source_report = $summaryPath
                        source_report_display = $summaryDisplay
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        message = "Input JSON kind '$kind' is not content-control data-binding evidence."
                    }) | Out-Null
                }
            }
        } catch {
            $status = "failed"
            $errorMessage = $_.Exception.Message
            $warnings.Add([ordered]@{
                id = "source_json_read_failed"
                action = "review_content_control_data_binding_evidence"
                source_schema = $contentControlGovernanceSchema
                source_report = $summaryPath
                source_report_display = $summaryDisplay
                source_json = $path
                source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                message = $errorMessage
            }) | Out-Null
        }
    }

    $sourceFiles.Add([ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        kind = $kind
        status = $status
        error = $errorMessage
        input_docx = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "input_docx" }
        input_docx_display = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "input_docx_display" }
        template_name = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "template_name" }
        schema_target = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "schema_target" }
        target_mode = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "target_mode" }
    }) | Out-Null
}

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'

foreach ($issue in @($syncIssues.ToArray())) {
    $issueSourceJson = Get-JsonString -Object $issue -Name "source_json" -DefaultValue $summaryPath
    $issueSourceJsonDisplay = Get-JsonString -Object $issue -Name "source_json_display" -DefaultValue $summaryDisplay
    $issueInputDocx = Get-JsonString -Object $issue -Name "input_docx"
    $issueInputDocxDisplay = Get-JsonString -Object $issue -Name "input_docx_display"
    $issueTemplateName = Get-JsonString -Object $issue -Name "template_name"
    $issueSchemaTarget = Get-JsonString -Object $issue -Name "schema_target"
    $issueTargetMode = Get-JsonString -Object $issue -Name "target_mode"
    $releaseBlockers.Add((New-ReleaseBlocker `
                -Id "content_control_data_binding.custom_xml_sync_issue" `
                -SourceJson $issueSourceJson `
                -PartEntryName (Get-JsonString -Object $issue -Name "part_entry_name") `
                -ContentControlIndex (Get-JsonProperty -Object $issue -Name "content_control_index") `
                -Tag (Get-JsonString -Object $issue -Name "tag") `
                -Alias (Get-JsonString -Object $issue -Name "alias") `
                -StoreItemId (Get-JsonString -Object $issue -Name "store_item_id") `
                -XPath (Get-JsonString -Object $issue -Name "xpath") `
                -Status (Get-JsonString -Object $issue -Name "reason") `
                -Action "fix_custom_xml_data_binding_source" `
                -Message "Custom XML sync failed for a bound content control." `
                -RepairStrategy "fix_custom_xml_source" `
                -RepairHint "Fix the Custom XML source value or mapping, then rerun sync-content-controls-from-custom-xml." `
                -CommandTemplate (New-SyncContentControlCommandTemplate) `
                -SourceJsonDisplay $issueSourceJsonDisplay `
                -InputDocx $issueInputDocx `
                -InputDocxDisplay $issueInputDocxDisplay `
                -TemplateName $issueTemplateName `
                -SchemaTarget $issueSchemaTarget `
                -TargetMode $issueTargetMode)) | Out-Null
}

foreach ($control in @($contentControls.ToArray())) {
    $controlBindingKey = Get-JsonString -Object $control -Name "binding_key"
    $controlSourceJson = Get-JsonString -Object $control -Name "source_json" -DefaultValue $summaryPath
    $controlSourceJsonDisplay = Get-JsonString -Object $control -Name "source_json_display" -DefaultValue $summaryDisplay
    $controlPartEntryName = Get-JsonString -Object $control -Name "part_entry_name"
    $controlIndex = Get-JsonProperty -Object $control -Name "content_control_index"
    $controlTag = Get-JsonString -Object $control -Name "tag"
    $controlAlias = Get-JsonString -Object $control -Name "alias"
    $controlStoreItemId = Get-JsonString -Object $control -Name "store_item_id"
    $controlXPath = Get-JsonString -Object $control -Name "xpath"
    $controlLock = Get-JsonString -Object $control -Name "lock"
    $controlFormKind = Get-JsonString -Object $control -Name "form_kind"
    $controlInputDocx = Get-JsonString -Object $control -Name "input_docx"
    $controlInputDocxDisplay = Get-JsonString -Object $control -Name "input_docx_display"
    $controlTemplateName = Get-JsonString -Object $control -Name "template_name"
    $controlSchemaTarget = Get-JsonString -Object $control -Name "schema_target"
    $controlTargetMode = Get-JsonString -Object $control -Name "target_mode"
    $hasBinding = -not [string]::IsNullOrWhiteSpace($controlBindingKey)
    if ($hasBinding -and (Get-JsonBool -Object $control -Name "showing_placeholder")) {
        $releaseBlockers.Add((New-ReleaseBlocker `
                    -Id "content_control_data_binding.bound_placeholder" `
                    -SourceJson $controlSourceJson `
                    -PartEntryName $controlPartEntryName `
                    -ContentControlIndex $controlIndex `
                    -Tag $controlTag `
                    -Alias $controlAlias `
                    -StoreItemId $controlStoreItemId `
                    -XPath $controlXPath `
                    -Status "placeholder_visible" `
                    -Action "sync_or_fill_bound_content_control" `
                    -Message "A data-bound content control is still showing placeholder text." `
                    -RepairStrategy "sync_bound_content_control" `
                    -RepairHint "Rerun Custom XML sync or explicitly fill the bound content control before release." `
                    -CommandTemplate (New-SyncContentControlCommandTemplate) `
                    -SourceJsonDisplay $controlSourceJsonDisplay `
                    -InputDocx $controlInputDocx `
                    -InputDocxDisplay $controlInputDocxDisplay `
                    -TemplateName $controlTemplateName `
                    -SchemaTarget $controlSchemaTarget `
                    -TargetMode $controlTargetMode)) | Out-Null
    }

    if ($hasBinding -and -not [string]::IsNullOrWhiteSpace($controlLock)) {
        $actionItems.Add((New-ActionItem `
                    -Id "review_content_control_lock_strategy" `
                    -Action "review_content_control_lock_strategy" `
                    -Title "Review lock state for data-bound content control" `
                    -SourceJson $controlSourceJson `
                    -PartEntryName $controlPartEntryName `
                    -ContentControlIndex $controlIndex `
                    -Tag $controlTag `
                    -Alias $controlAlias `
                    -StoreItemId $controlStoreItemId `
                    -XPath $controlXPath `
                    -RepairStrategy "review_lock_state" `
                    -RepairHint "Confirm whether the lock is intentional; clear it only if template data should overwrite this control." `
                    -CommandTemplate (New-ClearLockCommandTemplate -Tag $controlTag -Alias $controlAlias) `
                    -SourceJsonDisplay $controlSourceJsonDisplay `
                    -InputDocx $controlInputDocx `
                    -InputDocxDisplay $controlInputDocxDisplay `
                    -TemplateName $controlTemplateName `
                    -SchemaTarget $controlSchemaTarget `
                    -TargetMode $controlTargetMode)) | Out-Null
    }

    if (-not $hasBinding -and $controlFormKind -notin @("", "rich_text", "plain_text")) {
        $actionItems.Add((New-ActionItem `
                    -Id "review_unbound_form_content_control" `
                    -Action "review_unbound_form_content_control" `
                    -Title "Review whether form content control should bind to template data" `
                    -SourceJson $controlSourceJson `
                    -PartEntryName $controlPartEntryName `
                    -ContentControlIndex $controlIndex `
                    -Tag $controlTag `
                    -Alias $controlAlias `
                    -StoreItemId "" `
                    -XPath "" `
                    -RepairStrategy "bind_or_exempt_form_control" `
                    -RepairHint "Bind the form control to a Custom XML path, or document why it is intentionally unbound." `
                    -CommandTemplate (New-BindContentControlCommandTemplate -Tag $controlTag -Alias $controlAlias) `
                    -SourceJsonDisplay $controlSourceJsonDisplay `
                    -InputDocx $controlInputDocx `
                    -InputDocxDisplay $controlInputDocxDisplay `
                    -TemplateName $controlTemplateName `
                    -SchemaTarget $controlSchemaTarget `
                    -TargetMode $controlTargetMode)) | Out-Null
    }
}

$bindingGroups = @($contentControls.ToArray() | Where-Object {
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "binding_key"))
    } | Group-Object { Get-JsonString -Object $_ -Name "binding_key" } | Where-Object { $_.Count -gt 1 })
foreach ($group in @($bindingGroups)) {
    $first = @($group.Group)[0]
    $firstSourceJson = Get-JsonString -Object $first -Name "source_json" -DefaultValue $summaryPath
    $firstSourceJsonDisplay = Get-JsonString -Object $first -Name "source_json_display" -DefaultValue $summaryDisplay
    $firstInputDocx = Get-JsonString -Object $first -Name "input_docx"
    $firstInputDocxDisplay = Get-JsonString -Object $first -Name "input_docx_display"
    $firstTemplateName = Get-JsonString -Object $first -Name "template_name"
    $firstSchemaTarget = Get-JsonString -Object $first -Name "schema_target"
    $firstTargetMode = Get-JsonString -Object $first -Name "target_mode"
    $duplicateMembers = @(
        $group.Group | ForEach-Object {
            [ordered]@{
                part_entry_name = Get-JsonString -Object $_ -Name "part_entry_name"
                content_control_index = Get-JsonProperty -Object $_ -Name "content_control_index"
                tag = Get-JsonString -Object $_ -Name "tag"
                alias = Get-JsonString -Object $_ -Name "alias"
                input_docx = Get-JsonString -Object $_ -Name "input_docx"
                input_docx_display = Get-JsonString -Object $_ -Name "input_docx_display"
                template_name = Get-JsonString -Object $_ -Name "template_name"
                schema_target = Get-JsonString -Object $_ -Name "schema_target"
                target_mode = Get-JsonString -Object $_ -Name "target_mode"
                source_json_display = Get-JsonString -Object $_ -Name "source_json_display" -DefaultValue $summaryDisplay
            }
        }
    )
    $actionItems.Add((New-ActionItem `
                -Id "review_duplicate_content_control_binding" `
                -Action "review_duplicate_content_control_binding" `
                -Title "Review repeated content controls that share one Custom XML binding" `
                -SourceJson $firstSourceJson `
                -PartEntryName (Get-JsonString -Object $first -Name "part_entry_name") `
                -ContentControlIndex (Get-JsonProperty -Object $first -Name "content_control_index") `
                -Tag (Get-JsonString -Object $first -Name "tag") `
                -Alias (Get-JsonString -Object $first -Name "alias") `
                -StoreItemId (Get-JsonString -Object $first -Name "store_item_id") `
                -XPath (Get-JsonString -Object $first -Name "xpath") `
                -RepairStrategy "deduplicate_or_confirm_shared_binding" `
                -RepairHint "Review the entire shared-binding group. Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths." `
                -CommandTemplate "featherdoc_cli inspect-content-controls <input.docx> --json" `
                -SourceJsonDisplay $firstSourceJsonDisplay `
                -DuplicateBindingKey ([string]$group.Name) `
                -DuplicateMemberCount ([int]$group.Count) `
                -DuplicateMembers @($duplicateMembers) `
                -InputDocx $firstInputDocx `
                -InputDocxDisplay $firstInputDocxDisplay `
                -TemplateName $firstTemplateName `
                -SchemaTarget $firstSchemaTarget `
                -TargetMode $firstTargetMode)) | Out-Null
}

$repairPlanItems = New-Object 'System.Collections.Generic.List[object]'
foreach ($blocker in @($releaseBlockers.ToArray())) {
    $repairItem = New-RepairPlanItem -SourceKind "release_blocker" -Item $blocker
    if ($null -ne $repairItem) { $repairPlanItems.Add($repairItem) | Out-Null }
}
foreach ($item in @($actionItems.ToArray())) {
    $repairItem = New-RepairPlanItem -SourceKind "action_item" -Item $item
    if ($null -ne $repairItem) { $repairPlanItems.Add($repairItem) | Out-Null }
}

$boundControls = @($contentControls.ToArray() | Where-Object {
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "binding_key"))
    })
$customXmlSyncEvidenceSourceJson = $summaryPath
$customXmlSyncEvidenceSourceJsonDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
$boundControlEvidence = @($boundControls | Where-Object {
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "source_json")) -and
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "source_json_display"))
    } | Select-Object -First 1)
if ($boundControlEvidence.Count -gt 0) {
    $selectedBoundControlEvidence = $boundControlEvidence[0]
    $customXmlSyncEvidenceSourceJson = Get-JsonString -Object $selectedBoundControlEvidence -Name "source_json" -DefaultValue $summaryPath
    $customXmlSyncEvidenceSourceJsonDisplay = Get-JsonString -Object $selectedBoundControlEvidence -Name "source_json_display" -DefaultValue $summaryDisplay
}
if ($contentControls.Count -eq 0 -and $syncItems.Count -eq 0 -and $syncIssues.Count -eq 0) {
    $warnings.Add([ordered]@{
        id = "content_control_binding_evidence_missing"
        action = "collect_content_control_data_binding_evidence"
        repair_strategy = "collect_content_control_data_binding_evidence"
        repair_hint = "Run content-control inspection and Custom XML sync evidence collection for the target DOCX/template, then rerun this governance report; do not treat an empty governance summary as binding evidence."
        command_template = "featherdoc_cli inspect-content-controls <input.docx> --json; featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        source_schema = $contentControlGovernanceSchema
        source_json = $summaryPath
        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        source_report = $summaryPath
        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        message = "No content-control inspection or Custom XML sync evidence was loaded."
    }) | Out-Null
}
if ($boundControls.Count -gt 0 -and $syncItems.Count -eq 0 -and $syncIssues.Count -eq 0) {
    $warnings.Add([ordered]@{
        id = "custom_xml_sync_evidence_missing"
        action = "run_content_control_custom_xml_sync"
        source_schema = $contentControlGovernanceSchema
        source_json = $customXmlSyncEvidenceSourceJson
        source_json_display = $customXmlSyncEvidenceSourceJsonDisplay
        source_report = $summaryPath
        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        message = "Data-bound content controls were inspected, but no Custom XML sync result was provided."
    }) | Out-Null
}

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$missingInputCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "missing" }).Count
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0) {
    "blocked"
} elseif ($warnings.Count -gt 0 -or $actionItems.Count -gt 0) {
    "needs_review"
} else {
    "ready"
}

$allEvidenceItems = @($contentControls.ToArray() + $syncItems.ToArray() + $syncIssues.ToArray())

$summary = [ordered]@{
    schema = $contentControlGovernanceSchema
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    source_file_count = $sourceFiles.Count
    source_failure_count = $sourceFailureCount
    missing_input_count = $missingInputCount
    source_files = @($sourceFiles.ToArray())
    input_docx = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "input_docx"
    input_docx_display = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "input_docx_display"
    template_name = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "template_name"
    schema_target = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "schema_target"
    target_mode = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "target_mode"
    provenance_summary = @(New-ProvenanceSummary -Items $allEvidenceItems)
    inspection_file_count = @($sourceFiles.ToArray() | Where-Object { $_.kind -eq "inspect_content_controls" -and $_.status -eq "loaded" }).Count
    sync_file_count = @($sourceFiles.ToArray() | Where-Object { $_.kind -eq "custom_xml_sync_result" -and $_.status -eq "loaded" }).Count
    content_control_count = $contentControls.Count
    bound_content_control_count = $boundControls.Count
    placeholder_bound_content_control_count = @($boundControls | Where-Object { [bool]$_.showing_placeholder }).Count
    locked_bound_content_control_count = @($boundControls | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_.lock) }).Count
    duplicate_binding_count = @($bindingGroups).Count
    synced_content_control_count = $syncItems.Count
    sync_issue_count = $syncIssues.Count
    content_controls = @($contentControls.ToArray())
    sync_items = @($syncItems.ToArray())
    sync_issues = @($syncIssues.ToArray())
    binding_coverage_summary = @(New-BindingCoverageSummary -Items $contentControls.ToArray())
    form_kind_summary = @(Add-SummaryGroup -Items $contentControls.ToArray() -PropertyName "form_kind" -OutputName "form_kind")
    sync_issue_reason_summary = @(Add-SummaryGroup -Items $syncIssues.ToArray() -PropertyName "reason" -OutputName "reason")
    repair_plan_schema = "featherdoc.content_control_data_binding_repair_plan.v1"
    repair_plan_item_count = $repairPlanItems.Count
    repair_plan_apply_supported_count = @($repairPlanItems.ToArray() | Where-Object { [bool]$_.apply_supported }).Count
    repair_plan_native_dry_run_supported_count = @($repairPlanItems.ToArray() | Where-Object { [bool]$_.native_dry_run_supported }).Count
    repair_plan_requires_user_values_count = @($repairPlanItems.ToArray() | Where-Object { @(Get-JsonArray -Object $_ -Name "required_user_values").Count -gt 0 }).Count
    repair_plan_requires_visual_verification_count = @($repairPlanItems.ToArray() | Where-Object { [bool]$_.requires_visual_verification }).Count
    repair_plan_status_summary = @(Add-SummaryGroup -Items $repairPlanItems.ToArray() -PropertyName "plan_status" -OutputName "plan_status")
    repair_action_class_summary = @(New-RepairActionClassSummary -Items $repairPlanItems.ToArray())
    repair_action_release_blocking_count = @($repairPlanItems.ToArray() | Where-Object { @(Get-JsonArray -Object $_ -Name "repair_action_classes") -contains "release_blocking" }).Count
    repair_action_auto_repair_candidate_count = @($repairPlanItems.ToArray() | Where-Object { @(Get-JsonArray -Object $_ -Name "repair_action_classes") -contains "auto_repair_candidate" }).Count
    repair_action_manual_confirmation_required_count = @($repairPlanItems.ToArray() | Where-Object { @(Get-JsonArray -Object $_ -Name "repair_action_classes") -contains "manual_confirmation_required" }).Count
    repair_plan_items = @($repairPlanItems.ToArray())
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    blocker_id_summary = @(Add-SummaryGroup -Items $releaseBlockers.ToArray() -PropertyName "id" -OutputName "id")
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    action_item_summary = @(Add-SummaryGroup -Items $actionItems.ToArray() -PropertyName "action" -OutputName "action")
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warnings.Count -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
