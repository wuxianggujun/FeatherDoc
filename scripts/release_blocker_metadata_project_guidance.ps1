function Add-SchemaPatchConfidenceCalibrationGuidanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = "",
        [string]$ContextText = ""
    )

    $action = Get-ReleaseBlockerPropertyValue -Object $Item -Name "action"
    $contextSuffix = if ([string]::IsNullOrWhiteSpace($ContextText)) { "" } else { " $ContextText" }

    switch ($action) {
        "resolve_pending_schema_approvals" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `resolve_pending_schema_approvals`{0}: review pending schema patch approval outcome(s), write an explicit approval decision, reviewer, and reviewed_at, then rerun schema patch confidence calibration.' -f $contextSuffix)
            break
        }
        "fix_invalid_approval_records" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `fix_invalid_approval_records`{0}: repair invalid schema patch approval record(s), including missing reviewer or reviewed_at for non-pending decisions, then rerun schema patch confidence calibration.' -f $contextSuffix)
            break
        }
        "add_explicit_confidence_metadata" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `add_explicit_confidence_metadata`{0}: add explicit confidence metadata for unscored schema patch candidate(s), then rerun schema patch confidence calibration.' -f $contextSuffix)
            break
        }
        "add_business_template_source_metadata" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `add_business_template_source_metadata`{0}: add project_id, template_name, and source summary metadata for schema patch candidate(s), then rerun schema patch confidence calibration.' -f $contextSuffix)
            break
        }
        "add_business_template_document_type_metadata" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `add_business_template_document_type_metadata`{0}: add business_document_type metadata for schema patch candidate(s), then rerun schema patch confidence calibration.' -f $contextSuffix)
            break
        }
        default {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_schema_patch_confidence_calibration_evidence`{0}: review schema patch confidence calibration evidence, then rerun the calibration report.' -f $contextSuffix)
        }
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Open the schema patch confidence calibration report first: {0}" -f $sourceReportDisplay)
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Inspect the schema patch confidence calibration JSON before editing evidence: {0}" -f $sourceJsonDisplay)
    }

    $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "open_command"
    }
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1'
    }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Run the schema patch confidence calibration command after updating evidence: `{0}`' -f $commandTemplate)

    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($releaseSummaryDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'After calibration evidence is passing, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle before publishing.'
    } else {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('After calibration evidence is passing, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
    }
}

function Add-ContentControlDataBindingGovernanceGuidanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = "",
        [string]$ContextText = ""
    )

    $action = Get-ReleaseBlockerPropertyValue -Object $Item -Name "action"
    $contextSuffix = if ([string]::IsNullOrWhiteSpace($ContextText)) { "" } else { " $ContextText" }

    switch ($action) {
        "fix_custom_xml_data_binding_source" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `fix_custom_xml_data_binding_source`{0}: fix the Custom XML value or mapping that failed sync, rerun content-control Custom XML sync, then rebuild the data-binding governance report.' -f $contextSuffix)
            break
        }
        "sync_or_fill_bound_content_control" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `sync_or_fill_bound_content_control`{0}: rerun Custom XML sync or explicitly fill the bound content control so it no longer shows placeholder text before release.' -f $contextSuffix)
            break
        }
        "review_content_control_lock_strategy" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_content_control_lock_strategy`{0}: confirm whether the bound content control lock is intentional; clear it only when template data should overwrite the control.' -f $contextSuffix)
            break
        }
        "review_unbound_form_content_control" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_unbound_form_content_control`{0}: bind the form content control to a Custom XML path or document why it is intentionally unbound.' -f $contextSuffix)
            break
        }
        "review_duplicate_content_control_binding" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_duplicate_content_control_binding`{0}: review the shared binding group, then split controls across distinct Custom XML paths or confirm the repeated binding is intentional.' -f $contextSuffix)
            break
        }
        "collect_content_control_data_binding_evidence" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `collect_content_control_data_binding_evidence`{0}: collect both `inspect-content-controls` and `sync-content-controls-from-custom-xml` JSON evidence before trusting the governance summary.' -f $contextSuffix)
            break
        }
        "run_content_control_custom_xml_sync" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `run_content_control_custom_xml_sync`{0}: run Custom XML sync evidence for the inspected bound controls, then rebuild the data-binding governance report.' -f $contextSuffix)
            break
        }
        default {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_content_control_data_binding_evidence`{0}: review content-control data-binding evidence, then rebuild the governance report.' -f $contextSuffix)
        }
    }

    $status = Get-ReleaseBlockerPropertyValue -Object $Item -Name "status"
    $message = Get-ReleaseBlockerPropertyValue -Object $Item -Name "message"
    if (-not [string]::IsNullOrWhiteSpace($status) -or -not [string]::IsNullOrWhiteSpace($message)) {
        $statusLine = "Content-control data-binding governance item"
        if (-not [string]::IsNullOrWhiteSpace($status)) {
            $statusLine += ": $status"
        }
        if (-not [string]::IsNullOrWhiteSpace($message)) {
            $statusLine += "; message: $message"
        }
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text $statusLine
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Open the content-control data-binding governance report first: {0}" -f $sourceReportDisplay)
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Inspect the content-control data-binding source JSON before editing evidence: {0}" -f $sourceJsonDisplay)
    }

    $provenanceParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in @("input_docx_display", "input_docx", "template_name", "schema_target", "target_mode")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$provenanceParts.Add("$fieldName=$value")
        }
    }
    if ($provenanceParts.Count -gt 0) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Content-control provenance: {0}" -f ($provenanceParts.ToArray() -join ", "))
    }

    $controlParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in @("part_entry_name", "content_control_index", "tag", "alias", "store_item_id", "xpath")) {
        $value = Get-ReleaseBlockerPropertyObject -Object $Item -Name $fieldName
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            [void]$controlParts.Add("$fieldName=$value")
        }
    }
    if ($controlParts.Count -gt 0) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Content-control target: {0}" -f ($controlParts.ToArray() -join ", "))
    }

    $duplicateBindingKey = Get-ReleaseBlockerPropertyValue -Object $Item -Name "duplicate_binding_key"
    $duplicateMemberCount = Get-ReleaseBlockerPropertyObject -Object $Item -Name "duplicate_member_count"
    if (-not [string]::IsNullOrWhiteSpace($duplicateBindingKey) -or ($null -ne $duplicateMemberCount -and -not [string]::IsNullOrWhiteSpace([string]$duplicateMemberCount))) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Duplicate binding summary: key={0}, members={1}" -f (Get-ReleaseBlockerDisplayValue -Value $duplicateBindingKey), (Get-ReleaseBlockerDisplayValue -Value $duplicateMemberCount -Fallback "0"))
    }

    $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "open_command"
    }
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        switch ($action) {
            "collect_content_control_data_binding_evidence" {
                $commandTemplate = "featherdoc_cli inspect-content-controls <input.docx> --json; featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
                break
            }
            "run_content_control_custom_xml_sync" {
                $commandTemplate = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
                break
            }
            default {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1'
            }
        }
    }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Run or inspect the content-control data-binding command: `{0}`' -f $commandTemplate)

    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($releaseSummaryDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'After content-control data-binding evidence is passing, rerun `build_content_control_data_binding_governance_report.ps1`, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle before publishing.'
    } else {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('After content-control data-binding evidence is passing, rerun `build_content_control_data_binding_governance_report.ps1`, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
    }
}

function Add-ProjectTemplateGovernanceGuidanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = "",
        [string]$ContextText = ""
    )

    $action = Get-ReleaseBlockerPropertyValue -Object $Item -Name "action"
    $contextSuffix = if ([string]::IsNullOrWhiteSpace($ContextText)) { "" } else { " $ContextText" }

    switch ($action) {
        "approve_project_template_schema" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `approve_project_template_schema`{0}: write an explicit project-template schema approval decision, reviewer, and reviewed_at, then sync schema approval metadata.' -f $contextSuffix)
            break
        }
        "promote_schema_update_candidate" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `promote_schema_update_candidate`{0}: promote the approved schema update candidate only after smoke evidence and approval history agree.' -f $contextSuffix)
            break
        }
        "review_schema_update_candidate" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_schema_update_candidate`{0}: review the schema patch candidate, record the approval result, then rerun project-template schema approval sync.' -f $contextSuffix)
            break
        }
        "update_template_or_schema_before_retry" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `update_template_or_schema_before_retry`{0}: update the DOCX template or schema candidate, rerun smoke, and resync schema approval before retrying release readiness.' -f $contextSuffix)
            break
        }
        "freeze_schema_baseline" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `freeze_schema_baseline`{0}: freeze the schema baseline candidate from the project template DOCX before smoke or release readiness review.' -f $contextSuffix)
            break
        }
        "review_schema_baseline" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_schema_baseline`{0}: review the frozen baseline for unexpected, duplicate, or malformed fields before approving follow-up schema changes.' -f $contextSuffix)
            break
        }
        "review_schema_approval_history" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_schema_approval_history`{0}: rebuild and review project-template schema approval history before treating delivery readiness as release-ready.' -f $contextSuffix)
            break
        }
        "run_project_template_smoke_for_registered_manifest" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `run_project_template_smoke_for_registered_manifest`{0}: run project-template smoke for the registered manifest, then rebuild onboarding and delivery readiness evidence.' -f $contextSuffix)
            break
        }
        "run_project_template_smoke_then_review_schema_patch_approval" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `run_project_template_smoke_then_review_schema_patch_approval`{0}: run project-template smoke, review generated schema patch approval items, and sync the approval result.' -f $contextSuffix)
            break
        }
        "review_project_template_smoke_failure" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_project_template_smoke_failure`{0}: inspect the failing project-template smoke entry, repair the template/schema/data inputs, then rerun smoke.' -f $contextSuffix)
            break
        }
        "collect_project_template_onboarding_governance_evidence" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `collect_project_template_onboarding_governance_evidence`{0}: rebuild project-template onboarding governance evidence before trusting delivery readiness.' -f $contextSuffix)
            break
        }
        default {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_project_template_delivery_readiness_evidence`{0}: review project-template delivery readiness evidence, then rebuild the readiness report.' -f $contextSuffix)
        }
    }

    $status = Get-ReleaseBlockerPropertyValue -Object $Item -Name "status"
    $message = Get-ReleaseBlockerPropertyValue -Object $Item -Name "message"
    if (-not [string]::IsNullOrWhiteSpace($status) -or -not [string]::IsNullOrWhiteSpace($message)) {
        $statusLine = "Project-template governance item"
        if (-not [string]::IsNullOrWhiteSpace($status)) {
            $statusLine += ": $status"
        }
        if (-not [string]::IsNullOrWhiteSpace($message)) {
            $statusLine += "; message: $message"
        }
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text $statusLine
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Open the project-template governance report first: {0}" -f $sourceReportDisplay)
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Inspect the project-template source JSON before changing evidence: {0}" -f $sourceJsonDisplay)
    }

    $onboardingReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "onboarding_governance_source_report_display"
    if (-not [string]::IsNullOrWhiteSpace($onboardingReportDisplay) -and
        -not [string]::Equals($onboardingReportDisplay, $sourceReportDisplay, [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Open onboarding governance source report: {0}" -f $onboardingReportDisplay)
    }

    $onboardingJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "onboarding_governance_source_json_display"
    if (-not [string]::IsNullOrWhiteSpace($onboardingJsonDisplay) -and
        -not [string]::Equals($onboardingJsonDisplay, $sourceJsonDisplay, [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Inspect onboarding governance source JSON: {0}" -f $onboardingJsonDisplay)
    }

    $provenanceParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in @("project_id", "template_name", "input_docx_display", "input_docx", "schema_target", "target_mode", "candidate_type", "manifest_path_display", "manifest_path", "source_kind", "scope")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$provenanceParts.Add("$fieldName=$value")
        }
    }
    if ($provenanceParts.Count -gt 0) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Project-template provenance: {0}" -f ($provenanceParts.ToArray() -join ", "))
    }

    $schemaStatusParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in @("schema_approval_status", "gate_status", "latest_schema_approval_gate_status", "onboarding_governance_status", "onboarding_governance_release_ready", "release_ready", "schema_approval_history_required", "schema_history_available")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$schemaStatusParts.Add("$fieldName=$value")
        }
    }
    if ($schemaStatusParts.Count -gt 0) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Project-template schema approval state: {0}" -f ($schemaStatusParts.ToArray() -join ", "))
    }

    $schemaApprovalSummaryValue = Get-ReleaseBlockerPropertyObject -Object $Item -Name "onboarding_governance_schema_approval_status_summary"
    if ($null -eq $schemaApprovalSummaryValue) {
        $schemaApprovalSummaryValue = Get-ReleaseBlockerPropertyObject -Object $Item -Name "schema_approval_status_summary"
    }
    $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
        -Value $schemaApprovalSummaryValue `
        -Fallback (Get-ReleaseBlockerPropertyValue -Object $Item -Name "schema_approval_status")
    if (-not [string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Project-template schema approval summary: {0}" -f $schemaApprovalSummary)
    }

    $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        foreach ($commandName in @("open_command", "command", "review_command")) {
            $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name $commandName
            if (-not [string]::IsNullOrWhiteSpace($commandTemplate)) {
                break
            }
        }
    }
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        switch ($action) {
            "freeze_schema_baseline" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\freeze_template_schema_baseline.ps1 -InputDocx <template.docx> -SchemaOutput <schema.json>'
                break
            }
            "review_schema_baseline" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\freeze_template_schema_baseline.ps1 -InputDocx <template.docx> -SchemaOutput <schema.json>'
                break
            }
            { $_ -in @(
                    "run_project_template_smoke_for_registered_manifest",
                    "run_project_template_smoke_then_review_schema_patch_approval",
                    "review_project_template_smoke_failure"
                ) } {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_project_template_smoke.ps1 -ManifestPath .\samples\project_template_smoke.manifest.json -OutputDir .\output\project-template-smoke -SkipBuild'
                break
            }
            { $_ -in @(
                    "approve_project_template_schema",
                    "promote_schema_update_candidate",
                    "review_schema_update_candidate",
                    "update_template_or_schema_before_retry"
                ) } {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\sync_project_template_schema_approval.ps1'
                break
            }
            "review_schema_approval_history" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\write_project_template_schema_approval_history.ps1 -SummaryJsonDir .\output\project-template-smoke -OutputJson .\output\project-template-schema-approval-history\history.json'
                break
            }
            "collect_project_template_onboarding_governance_evidence" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_project_template_onboarding_governance_report.ps1 -InputRoot .\output\project-template-smoke -OutputDir .\output\project-template-onboarding-governance'
                break
            }
            default {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1'
            }
        }
    }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Run or inspect the project-template governance command: `{0}`' -f $commandTemplate)

    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($releaseSummaryDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'After project-template governance evidence is passing, rerun `build_project_template_onboarding_governance_report.ps1` when onboarding evidence changed, rerun `build_project_template_delivery_readiness_report.ps1`, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle before publishing.'
    } else {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('After project-template governance evidence is passing, rerun `build_project_template_onboarding_governance_report.ps1` when onboarding evidence changed, rerun `build_project_template_delivery_readiness_report.ps1`, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
    }
}

function Add-NumberingCatalogGovernanceGuidanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = "",
        [string]$ContextText = ""
    )

    $action = Get-ReleaseBlockerPropertyValue -Object $Item -Name "action"
    $contextSuffix = if ([string]::IsNullOrWhiteSpace($ContextText)) { "" } else { " $ContextText" }

    switch ($action) {
        "review_numbering_catalog_real_corpus_alignment" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_numbering_catalog_real_corpus_alignment`{0}: compare catalog exemplar document keys with baseline manifest document keys, then repair missing or stale real-corpus evidence before release.' -f $contextSuffix)
            break
        }
        "fix_numbering_catalog_baseline_lint" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `fix_numbering_catalog_baseline_lint`{0}: lint and repair the committed numbering catalog baseline before rebuilding manifest evidence.' -f $contextSuffix)
            break
        }
        "refresh_numbering_catalog_baseline_or_repair_docx" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `refresh_numbering_catalog_baseline_or_repair_docx`{0}: compare generated numbering catalog output with the committed baseline, then either refresh the baseline or repair the source DOCX.' -f $contextSuffix)
            break
        }
        "review_numbering_catalog_check_issues" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_numbering_catalog_check_issues`{0}: review numbering catalog check issues, repair the DOCX or catalog JSON, and rerun the baseline check.' -f $contextSuffix)
            break
        }
        "rebuild_document_skeleton_governance_rollup" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `rebuild_document_skeleton_governance_rollup`{0}: regenerate document skeleton governance rollup evidence before rebuilding numbering catalog governance.' -f $contextSuffix)
            break
        }
        "rebuild_numbering_catalog_manifest_summary" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `rebuild_numbering_catalog_manifest_summary`{0}: rerun the numbering catalog manifest check and keep its summary as release evidence.' -f $contextSuffix)
            break
        }
        "review_numbering_catalog_governance_sources" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_numbering_catalog_governance_sources`{0}: inspect numbering governance input JSON kind and rebuild the owning source report when evidence is skipped or unreadable.' -f $contextSuffix)
            break
        }
        "review_style_numbering_audit" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_style_numbering_audit`{0}: review style numbering audit issues, then decide whether to repair style numbering or update the numbering catalog baseline.' -f $contextSuffix)
            break
        }
        "preview_style_numbering_repair" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `preview_style_numbering_repair`{0}: generate a repair-style-numbering plan against the exported catalog before applying any DOCX mutation.' -f $contextSuffix)
            break
        }
        "promote_numbering_catalog_exemplar" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `promote_numbering_catalog_exemplar`{0}: review the generated exemplar numbering catalog before promoting it into the baseline flow.' -f $contextSuffix)
            break
        }
        "register_numbering_catalog_baseline" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `register_numbering_catalog_baseline`{0}: register the reviewed exemplar catalog in the numbering catalog baseline manifest.' -f $contextSuffix)
            break
        }
        default {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `rerun_document_skeleton_governance_report`{0}: rerun the document skeleton governance report, then rebuild rollup and numbering catalog governance evidence.' -f $contextSuffix)
        }
    }

    $status = Get-ReleaseBlockerPropertyValue -Object $Item -Name "status"
    $message = Get-ReleaseBlockerPropertyValue -Object $Item -Name "message"
    if (-not [string]::IsNullOrWhiteSpace($status) -or -not [string]::IsNullOrWhiteSpace($message)) {
        $statusLine = "Numbering governance item"
        if (-not [string]::IsNullOrWhiteSpace($status)) {
            $statusLine += ": $status"
        }
        if (-not [string]::IsNullOrWhiteSpace($message)) {
            $statusLine += "; message: $message"
        }
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text $statusLine
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Open the numbering governance report first: {0}" -f $sourceReportDisplay)
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Inspect the numbering governance source JSON before changing evidence: {0}" -f $sourceJsonDisplay)
    }

    $provenanceParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in @("scope", "document_name", "input_docx_display", "input_docx", "catalog_file_display", "catalog_file", "generated_output_path", "exemplar_catalog_display", "exemplar_catalog_path", "source_kind", "category")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$provenanceParts.Add("$fieldName=$value")
        }
    }
    if ($provenanceParts.Count -gt 0) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Numbering governance provenance: {0}" -f ($provenanceParts.ToArray() -join ", "))
    }

    $metricParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in @(
            "matched_document_count",
            "unmatched_catalog_document_count",
            "unmatched_baseline_document_count",
            "catalog_coverage_percent",
            "baseline_coverage_percent",
            "coverage_score",
            "alignment_gap_count",
            "real_corpus_confidence_score",
            "real_corpus_confidence_level",
            "total_style_numbering_issue_count",
            "drift_count",
            "dirty_baseline_count",
            "issue_entry_count"
        )) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$metricParts.Add("$fieldName=$value")
        }
    }
    if ($metricParts.Count -gt 0) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Numbering governance metrics: {0}" -f ($metricParts.ToArray() -join ", "))
    }

    foreach ($arrayInfo in @(
            [ordered]@{ Name = "catalog_document_keys"; Label = "catalog document keys" },
            [ordered]@{ Name = "baseline_document_keys"; Label = "baseline document keys" },
            [ordered]@{ Name = "matched_document_keys"; Label = "matched document keys" }
        )) {
        $values = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name $arrayInfo.Name | ForEach-Object { [string]$_ } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($values.Count -gt 0) {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Numbering governance {0}: {1}" -f $arrayInfo.Label, ($values -join ","))
        }
    }

    $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        foreach ($commandName in @("open_command", "command", "audit_command", "review_command")) {
            $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name $commandName
            if (-not [string]::IsNullOrWhiteSpace($commandTemplate)) {
                break
            }
        }
    }
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        switch ($action) {
            { $_ -in @(
                    "fix_numbering_catalog_baseline_lint",
                    "refresh_numbering_catalog_baseline_or_repair_docx",
                    "review_numbering_catalog_check_issues",
                    "register_numbering_catalog_baseline"
                ) } {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx <input.docx> -CatalogFile <catalog.json> -BuildDir <build-dir> -SkipBuild'
                break
            }
            "rebuild_document_skeleton_governance_rollup" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1 -OutputDir .\output\document-skeleton-governance-rollup'
                break
            }
            "rebuild_numbering_catalog_manifest_summary" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_manifest.ps1 -ManifestPath .\baselines\numbering-catalog\manifest.json -BuildDir <build-dir> -OutputDir .\output\numbering-catalog-manifest-checks -SkipBuild'
                break
            }
            "review_style_numbering_audit" {
                $commandTemplate = 'featherdoc_cli audit-style-numbering <input.docx> --fail-on-issue --json'
                break
            }
            "preview_style_numbering_repair" {
                $commandTemplate = 'featherdoc_cli repair-style-numbering <input.docx> --catalog-file <numbering-catalog.json> --plan-only --json'
                break
            }
            "promote_numbering_catalog_exemplar" {
                $commandTemplate = 'featherdoc_cli check-numbering-catalog <input.docx> --catalog-file <numbering-catalog.json> --json'
                break
            }
            "rerun_document_skeleton_governance_report" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_report.ps1 -InputDocx <input.docx> -OutputDir .\output\document-skeleton-governance'
                break
            }
            default {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_numbering_catalog_governance_report.ps1 -InputRoot .\output -OutputDir .\output\numbering-catalog-governance'
            }
        }
    }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Run or inspect the numbering governance command: `{0}`' -f $commandTemplate)

    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($releaseSummaryDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'After numbering governance evidence is passing, rerun `build_document_skeleton_governance_rollup_report.ps1` when skeleton evidence changed, rerun `build_numbering_catalog_governance_report.ps1`, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle before publishing.'
    } else {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('After numbering governance evidence is passing, rerun `build_document_skeleton_governance_rollup_report.ps1` when skeleton evidence changed, rerun `build_numbering_catalog_governance_report.ps1`, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
    }
}

function Add-TableLayoutDeliveryGovernanceGuidanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = "",
        [string]$ContextText = ""
    )

    $action = Get-ReleaseBlockerPropertyValue -Object $Item -Name "action"
    $contextSuffix = if ([string]::IsNullOrWhiteSpace($ContextText)) { "" } else { " $ContextText" }

    switch ($action) {
        { $_ -in @("apply_safe_tblLook_fixes", "apply_safe_tblLook_fixes_then_visual_regression") } {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `{0}`{1}: apply only safe tblLook fixes to a reviewed output DOCX, then regenerate table layout visual regression evidence before release.' -f $action, $contextSuffix)
            break
        }
        { $_ -in @("review_table_style_quality_plan", "review_manual_table_style_definition_work") } {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `{0}`{1}: inspect table style quality findings, separate safe tblLook-only fixes from manual style-definition work, and keep manual edits auditable.' -f $action, $contextSuffix)
            break
        }
        { $_ -in @("review_table_position_plan", "review_floating_table_position_plans") } {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `{0}`{1}: review floating table position plans with existing positions before applying presets, then run a fingerprint-checked dry-run.' -f $action, $contextSuffix)
            break
        }
        "dry_run_apply_table_position_plans" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `dry_run_apply_table_position_plans`{0}: run apply-table-position-plan with `--dry-run` and confirm table fingerprints still match before writing DOCX output.' -f $contextSuffix)
            break
        }
        "run_table_style_quality_visual_regression" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `run_table_style_quality_visual_regression`{0}: generate Word-rendered table style quality before/after evidence and keep the bundle linked from release materials.' -f $contextSuffix)
            break
        }
        "review_table_layout_delivery_governance_sources" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `review_table_layout_delivery_governance_sources`{0}: inspect skipped or unreadable table layout governance input JSON, then rebuild the owning source report.' -f $contextSuffix)
            break
        }
        "rebuild_table_layout_delivery_rollup" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `rebuild_table_layout_delivery_rollup`{0}: regenerate table layout delivery rollup evidence before rebuilding the governance report.' -f $contextSuffix)
            break
        }
        default {
            Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `{0}`{1}: review table layout delivery governance evidence, then rebuild rollup and governance reports before release.' -f $action, $contextSuffix)
        }
    }

    $status = Get-ReleaseBlockerPropertyValue -Object $Item -Name "status"
    $message = Get-ReleaseBlockerPropertyValue -Object $Item -Name "message"
    if (-not [string]::IsNullOrWhiteSpace($status) -or -not [string]::IsNullOrWhiteSpace($message)) {
        $statusLine = "Table layout delivery governance item"
        if (-not [string]::IsNullOrWhiteSpace($status)) {
            $statusLine += ": $status"
        }
        if (-not [string]::IsNullOrWhiteSpace($message)) {
            $statusLine += "; message: $message"
        }
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text $statusLine
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Open the table layout delivery governance report first: {0}" -f $sourceReportDisplay)
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Inspect the table layout delivery source JSON before changing evidence: {0}" -f $sourceJsonDisplay)
    }

    $provenanceParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in @("scope", "document_name", "input_docx_display", "input_docx", "source_kind", "report_id", "source_schema")) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$provenanceParts.Add("$fieldName=$value")
        }
    }
    if ($provenanceParts.Count -gt 0) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Table layout delivery provenance: {0}" -f ($provenanceParts.ToArray() -join ", "))
    }

    $metricParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in @(
            "table_style_issue_count",
            "automatic_tblLook_fix_count",
            "manual_table_style_fix_count",
            "total_table_style_issue_count",
            "total_automatic_tblLook_fix_count",
            "total_manual_table_style_fix_count",
            "table_position_automatic_count",
            "table_position_review_count",
            "total_table_position_automatic_count",
            "total_table_position_review_count",
            "command_failure_count",
            "ready_document_percent",
            "unresolved_item_count",
            "pdf_floating_table_capability_status",
            "pdf_floating_table_layout_boundary",
            "pdf_floating_table_supported_geometry_count",
            "pdf_floating_table_metadata_only_count"
        )) {
        $value = Get-ReleaseBlockerPropertyValue -Object $Item -Name $fieldName
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [void]$metricParts.Add("$fieldName=$value")
        }
    }
    if ($metricParts.Count -gt 0) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Table layout delivery metrics: {0}" -f ($metricParts.ToArray() -join ", "))
    }

    $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        foreach ($commandName in @("open_command", "command", "audit_command", "review_command")) {
            $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name $commandName
            if (-not [string]::IsNullOrWhiteSpace($commandTemplate)) {
                break
            }
        }
    }
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        switch ($action) {
            { $_ -in @("apply_safe_tblLook_fixes", "apply_safe_tblLook_fixes_then_visual_regression") } {
                $commandTemplate = 'featherdoc_cli apply-table-style-quality-fixes <input.docx> --look-only --output <reviewed.docx> --json'
                break
            }
            { $_ -in @("review_table_style_quality_plan", "review_manual_table_style_definition_work") } {
                $commandTemplate = 'featherdoc_cli inspect-table-style <input.docx> <style-id> --json'
                break
            }
            { $_ -in @("review_table_position_plan", "review_floating_table_position_plans", "dry_run_apply_table_position_plans") } {
                $commandTemplate = 'featherdoc_cli apply-table-position-plan <table-position.plan.json> --dry-run --json'
                break
            }
            "run_table_style_quality_visual_regression" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_table_style_quality_visual_regression.ps1'
                break
            }
            "rebuild_table_layout_delivery_rollup" {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_rollup_report.ps1 -InputRoot .\output -OutputDir .\output\table-layout-delivery-rollup'
                break
            }
            default {
                $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1 -InputRoot .\output -OutputDir .\output\table-layout-delivery-governance'
            }
        }
    }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Run or inspect the table layout delivery command: `{0}`' -f $commandTemplate)

    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($releaseSummaryDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'After table layout delivery evidence is passing, rerun `build_table_layout_delivery_rollup_report.ps1` when source documents changed, rerun `build_table_layout_delivery_governance_report.ps1`, refresh `run_table_style_quality_visual_regression.ps1` evidence when layout or tblLook output changed, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle before publishing.'
    } else {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('After table layout delivery evidence is passing, rerun `build_table_layout_delivery_rollup_report.ps1` when source documents changed, rerun `build_table_layout_delivery_governance_report.ps1`, refresh `run_table_style_quality_visual_regression.ps1` evidence when layout or tblLook output changed, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
    }
}
