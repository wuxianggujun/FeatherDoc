function Assert-PackageReleaseAssetsAllowIncompleteRegression {
    [CmdletBinding()]
    param()

    $packageScript = Join-Path $resolvedRepoRoot "scripts\package_release_assets.ps1"
    & $packageScript `
        -SummaryJson $summaryPath `
        -OutputRoot $outputRoot `
        -AllowIncomplete `
        -KeepStaging

    $stagingRoot = Join-Path $outputRoot "v1.6.4\staging"
    $placeholderPath = Join-Path $stagingRoot "word-visual-release-gate\README.md"
    $stagedStartHerePath = Join-Path $stagingRoot "release-candidate-checks\START_HERE.md"
    $stagedSummaryPath = Join-Path $stagingRoot "release-candidate-checks\report\summary.json"
    $stagedGovernanceHandoffPath = Join-Path $stagingRoot "release-candidate-checks\report\release_governance_handoff.md"
    $stagedFinalReviewPath = Join-Path $stagingRoot "release-candidate-checks\report\final_review.md"
    $stagedArtifactGuidePath = Join-Path $stagingRoot "release-candidate-checks\report\ARTIFACT_GUIDE.md"
    $stagedReviewerChecklistPath = Join-Path $stagingRoot "release-candidate-checks\report\REVIEWER_CHECKLIST.md"
    $manifestPath = Join-Path $outputRoot "v1.6.4\release_assets_manifest.json"
    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json

    Assert-NotContains -Path $manifestPath -UnexpectedText $resolvedRepoRoot -Label 'release_assets_manifest.json'
    Assert-NotContains -Path $manifestPath -UnexpectedText '<windows-absolute-path>' -Label 'release_assets_manifest.json'
    Assert-NotContains -Path $manifestPath -UnexpectedText '<unix-absolute-path>' -Label 'release_assets_manifest.json'

    if (-not (Test-Path -LiteralPath $placeholderPath)) {
        throw "Expected incomplete visual-gate placeholder note was not created."
    }

    $placeholderContent = Get-Content -Raw -LiteralPath $placeholderPath
    if ($placeholderContent -notmatch 'visual_gate=skipped') {
        throw "Incomplete visual-gate placeholder note does not mention skipped visual gate."
    }

    if ($manifest.visual_gate_status -ne "skipped") {
        throw "Release assets manifest did not record visual_gate_status=skipped."
    }

    if ([bool]$manifest.visual_gate_evidence_included) {
        throw "Release assets manifest incorrectly reported visual gate evidence as included."
    }

    if ($manifest.governance_metric_count -ne 3) {
        throw "Release assets manifest did not preserve governance_metric_count=3."
    }

    $manifestReleaseNoteBundle = $manifest.release_note_bundle
    if ($null -eq $manifestReleaseNoteBundle) {
        throw "Release assets manifest lost release_note_bundle in AllowIncomplete mode."
    }
    if ([string]$manifestReleaseNoteBundle.status -ne "declared") {
        throw "Release assets manifest lost release_note_bundle status in AllowIncomplete mode."
    }
    if ([int]$manifestReleaseNoteBundle.entrypoint_count -ne 6) {
        throw "Release assets manifest lost release_note_bundle entrypoint count in AllowIncomplete mode."
    }
    if ([int]$manifestReleaseNoteBundle.required_entrypoint_count -ne 6) {
        throw "Release assets manifest lost release_note_bundle required entrypoint count in AllowIncomplete mode."
    }
    $manifestReleaseNoteBundleEntrypointsById = @{}
    foreach ($entrypoint in @($manifestReleaseNoteBundle.entrypoints)) {
        $manifestReleaseNoteBundleEntrypointsById[[string]$entrypoint.id] = $entrypoint
    }
    foreach ($entrypointExpectation in @(
            [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks-ci\START_HERE.md"; location = "summary_root" },
            [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md"; location = "report" },
            [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md"; location = "report" },
            [ordered]@{ id = "release_handoff"; path = $releaseHandoffPath; path_display = ".\output\release-candidate-checks-ci\report\release_handoff.md"; location = "report" },
            [ordered]@{ id = "release_body_zh_cn"; path = $releaseBodyPath; path_display = ".\output\release-candidate-checks-ci\report\release_body.zh-CN.md"; location = "report" },
            [ordered]@{ id = "release_summary_zh_cn"; path = $releaseSummaryPath; path_display = ".\output\release-candidate-checks-ci\report\release_summary.zh-CN.md"; location = "report" }
        )) {
        $entrypointId = [string]$entrypointExpectation.id
        if (-not (@($manifestReleaseNoteBundle.entrypoint_ids | ForEach-Object { [string]$_ }) -contains $entrypointId)) {
            throw "Release assets manifest lost release_note_bundle entrypoint id '$entrypointId' in AllowIncomplete mode."
        }
        if (-not $manifestReleaseNoteBundleEntrypointsById.ContainsKey($entrypointId)) {
            throw "Release assets manifest lost release_note_bundle entrypoint '$entrypointId' in AllowIncomplete mode."
        }

        $entrypoint = $manifestReleaseNoteBundleEntrypointsById[$entrypointId]
        if (-not [bool]$entrypoint.required) {
            throw "Release assets manifest lost required=true for release_note_bundle entrypoint '$entrypointId' in AllowIncomplete mode."
        }
        if ([string]$entrypoint.location -ne [string]$entrypointExpectation.location) {
            throw "Release assets manifest lost release_note_bundle entrypoint '$entrypointId' location in AllowIncomplete mode."
        }

        $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
            -Path ([string]$entrypointExpectation.path) `
            -RepoRoot $resolvedRepoRoot
        if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
            throw "Release assets manifest did not public-sanitize release_note_bundle entrypoint '$entrypointId' path in AllowIncomplete mode."
        }

        $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
        if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
            throw "Release assets manifest lost release_note_bundle entrypoint '$entrypointId' path_display in AllowIncomplete mode. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
        }
    }

    $manifestSignoffEntrypoints = $manifest.manifest_signoff_entrypoints
    if ($null -eq $manifestSignoffEntrypoints) {
        throw "Release assets manifest lost manifest_signoff_entrypoints in AllowIncomplete mode."
    }
    if ([string]$manifestSignoffEntrypoints.status -ne "declared") {
        throw "Release assets manifest lost manifest signoff status in AllowIncomplete mode."
    }
    if ([int]$manifestSignoffEntrypoints.required_entrypoint_count -ne 3) {
        throw "Release assets manifest lost manifest signoff required entrypoint count in AllowIncomplete mode."
    }
    if ([string]$manifestSignoffEntrypoints.release_assets_manifest -ne "release_assets_manifest.json") {
        throw "Release assets manifest lost packaged manifest signoff path in AllowIncomplete mode."
    }
    foreach ($requiredContract in @(
            "project_template_delivery_readiness_contract",
            "project_template_onboarding_governance_contract"
        )) {
        if (-not (@($manifestSignoffEntrypoints.required_contracts | ForEach-Object { [string]$_ }) -contains $requiredContract)) {
            throw "Release assets manifest lost manifest signoff required contract '$requiredContract' in AllowIncomplete mode."
        }
    }
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
            "requires_reviewer_action",
            "reviewer_action_summary",
            "reviewer_action_reason",
            "reviewer_actions",
            "business_document_type_summary",
            "corpus_role_summary",
            "source_report_display",
            "source_json_display"
        )) {
        if (-not (@($manifestSignoffEntrypoints.required_fields | ForEach-Object { [string]$_ }) -contains $requiredField)) {
            throw "Release assets manifest lost manifest signoff required field '$requiredField' in AllowIncomplete mode."
        }
    }
    if ([string]$manifestSignoffEntrypoints.checklist_marker -ne "reviewer_manifest_scoped_project_template_trace") {
        throw "Release assets manifest lost manifest signoff checklist marker in AllowIncomplete mode."
    }

    $manifestProjectTemplateChecklistEntrypoints = $manifest.project_template_readiness_checklist_entrypoints
    if ($null -eq $manifestProjectTemplateChecklistEntrypoints) {
        throw "Release assets manifest lost project_template_readiness_checklist_entrypoints in AllowIncomplete mode."
    }
    if ([string]$manifestProjectTemplateChecklistEntrypoints.status -ne "declared") {
        throw "Release assets manifest lost project-template readiness checklist entrypoints status in AllowIncomplete mode."
    }
    if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_label -ne "Project template release readiness checklist") {
        throw "Release assets manifest lost project-template readiness checklist label in AllowIncomplete mode."
    }
    if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst") {
        throw "Release assets manifest lost project-template readiness checklist path in AllowIncomplete mode."
    }
    if ([int]$manifestProjectTemplateChecklistEntrypoints.required_entrypoint_count -ne 3) {
        throw "Release assets manifest lost project-template readiness checklist required entrypoint count in AllowIncomplete mode."
    }
    if ([string]$manifestProjectTemplateChecklistEntrypoints.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace") {
        throw "Release assets manifest lost project-template readiness checklist marker in AllowIncomplete mode."
    }

    $manifestProjectTemplateChecklistMaterialSafetyAudit = $manifest.release_entry_project_template_readiness_checklist_material_safety_audit
    if ($null -eq $manifestProjectTemplateChecklistMaterialSafetyAudit) {
        throw "Release assets manifest lost release_entry_project_template_readiness_checklist_material_safety_audit in AllowIncomplete mode."
    }
    if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.status -ne "passed") {
        throw "Release assets manifest lost project-template checklist material-safety audit pass status in AllowIncomplete mode."
    }
    if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.audit_script -ne ".\scripts\assert_release_material_safety.ps1") {
        throw "Release assets manifest lost project-template checklist material-safety audit script in AllowIncomplete mode."
    }
    if ([int]$manifestProjectTemplateChecklistMaterialSafetyAudit.audited_entrypoint_count -ne 3) {
        throw "Release assets manifest lost project-template checklist material-safety audited entrypoint count in AllowIncomplete mode."
    }
    foreach ($entrypointId in @("start_here", "artifact_guide", "reviewer_checklist")) {
        if (-not (@($manifestProjectTemplateChecklistMaterialSafetyAudit.audited_entrypoints | ForEach-Object { [string]$_ }) -contains $entrypointId)) {
            throw "Release assets manifest lost project-template checklist material-safety audited entrypoint '$entrypointId' in AllowIncomplete mode."
        }
    }
    if ([string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_label -ne "Project-template readiness checklist handoff evidence" -or
        [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_field -ne "project_template_readiness_checklist_entrypoints_source_reports" -or
        [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.compact_evidence_source_schema -ne "featherdoc.release_candidate_summary" -or
        [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.checklist_path -ne "docs/project_template_release_readiness_checklist_zh.rst" -or
        [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.checklist_marker -ne "release_entry_project_template_readiness_checklist_trace" -or
        [string]$manifestProjectTemplateChecklistMaterialSafetyAudit.material_safety_marker -ne "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace") {
        throw "Release assets manifest lost project-template checklist material-safety audit identity in AllowIncomplete mode."
    }

    $manifestSignoffEntrypointsById = @{}
    foreach ($entrypoint in @($manifestSignoffEntrypoints.entrypoints)) {
        $manifestSignoffEntrypointsById[[string]$entrypoint.id] = $entrypoint
    }
    foreach ($entrypointExpectation in @(
            [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks-ci\START_HERE.md" },
            [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md" },
            [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md" }
        )) {
        $entrypointId = [string]$entrypointExpectation.id
        if (-not $manifestSignoffEntrypointsById.ContainsKey($entrypointId)) {
            throw "Release assets manifest lost manifest signoff entrypoint '$entrypointId' in AllowIncomplete mode."
        }

        $entrypoint = $manifestSignoffEntrypointsById[$entrypointId]
        if (-not [bool]$entrypoint.required) {
            throw "Release assets manifest lost required=true for manifest signoff entrypoint '$entrypointId' in AllowIncomplete mode."
        }

        $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
            -Path ([string]$entrypointExpectation.path) `
            -RepoRoot $resolvedRepoRoot
        if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
            throw "Release assets manifest did not public-sanitize manifest signoff entrypoint '$entrypointId' path in AllowIncomplete mode."
        }

        $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
        if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
            throw "Release assets manifest lost manifest signoff entrypoint '$entrypointId' path_display in AllowIncomplete mode. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
        }
    }

    $manifestProjectTemplateChecklistEntrypointsById = @{}
    foreach ($entrypoint in @($manifestProjectTemplateChecklistEntrypoints.entrypoints)) {
        $manifestProjectTemplateChecklistEntrypointsById[[string]$entrypoint.id] = $entrypoint
    }
    foreach ($entrypointExpectation in @(
            [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks-ci\START_HERE.md" },
            [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md" },
            [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md" }
        )) {
        $entrypointId = [string]$entrypointExpectation.id
        if (-not $manifestProjectTemplateChecklistEntrypointsById.ContainsKey($entrypointId)) {
            throw "Release assets manifest lost project-template readiness checklist entrypoint '$entrypointId' in AllowIncomplete mode."
        }

        $entrypoint = $manifestProjectTemplateChecklistEntrypointsById[$entrypointId]
        if (-not [bool]$entrypoint.required) {
            throw "Release assets manifest lost required=true for project-template readiness checklist entrypoint '$entrypointId' in AllowIncomplete mode."
        }

        $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
            -Path ([string]$entrypointExpectation.path) `
            -RepoRoot $resolvedRepoRoot
        if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
            throw "Release assets manifest did not public-sanitize project-template readiness checklist entrypoint '$entrypointId' path in AllowIncomplete mode."
        }

        $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
        if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
            throw "Release assets manifest lost project-template readiness checklist entrypoint '$entrypointId' path_display in AllowIncomplete mode. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
        }
    }

    if ($manifest.governance_metrics.Count -ne 3) {
        throw "Release assets manifest did not preserve all governance metrics."
    }

    if (-not (@($manifest.governance_metrics | ForEach-Object { $_.metric }) -contains "real_corpus_confidence")) {
        throw "Release assets manifest lost real_corpus_confidence governance metric."
    }

    if (-not (@($manifest.governance_metrics | ForEach-Object { $_.metric }) -contains "delivery_quality")) {
        throw "Release assets manifest lost delivery_quality governance metric."
    }

    $manifestStyleConfidence = @($manifest.governance_metrics | Where-Object {
        [string]$_.id -eq "style_catalog_governance.real_corpus_confidence"
    }) | Select-Object -First 1
    if ($null -eq $manifestStyleConfidence) {
        throw "Release assets manifest lost style_catalog_governance.real_corpus_confidence governance metric."
    }

    $manifestNumberingConfidence = $manifest.numbering_catalog_real_corpus_confidence
    if ($null -eq $manifestNumberingConfidence) {
        throw "Release assets manifest lost numbering_catalog_real_corpus_confidence in AllowIncomplete mode."
    }
    if ([string]$manifestNumberingConfidence.id -ne "numbering_catalog_governance.real_corpus_confidence") {
        throw "Release assets manifest used the wrong real corpus confidence id for numbering."
    }
    if ([string]$manifestNumberingConfidence.metric -ne "real_corpus_confidence") {
        throw "Release assets manifest lost numbering real corpus confidence metric."
    }
    if ([string]$manifestNumberingConfidence.report_id -ne "numbering_catalog_governance") {
        throw "Release assets manifest used the wrong real corpus confidence report_id for numbering."
    }
    if ([string]$manifestNumberingConfidence.source_schema -ne "featherdoc.numbering_catalog_governance_report.v1") {
        throw "Release assets manifest used the wrong real corpus confidence source_schema for numbering."
    }
    if ([string]$manifestNumberingConfidence.source_report -ne ".\output\numbering-catalog-governance\summary.json") {
        throw "Release assets manifest lost numbering real corpus confidence source_report in AllowIncomplete mode."
    }
    if ([string]$manifestNumberingConfidence.source_report_display -ne ".\output\numbering-catalog-governance\summary.json") {
        throw "Release assets manifest lost numbering real corpus confidence source_report_display in AllowIncomplete mode."
    }
    if ([string]$manifestNumberingConfidence.source_json -ne ".\output\numbering-catalog-governance\summary.json") {
        throw "Release assets manifest lost numbering real corpus confidence source_json in AllowIncomplete mode."
    }
    if ([string]$manifestNumberingConfidence.source_json_display -ne ".\output\numbering-catalog-governance\summary.json") {
        throw "Release assets manifest lost numbering real corpus confidence source_json_display in AllowIncomplete mode."
    }
    if ([int]$manifestNumberingConfidence.score -ne 56) {
        throw "Release assets manifest lost numbering real corpus confidence score."
    }
    if ([string]$manifestNumberingConfidence.level -ne "low") {
        throw "Release assets manifest lost numbering real corpus confidence level."
    }
    if ($null -eq $manifestNumberingConfidence.details) {
        throw "Release assets manifest lost numbering real corpus confidence details in AllowIncomplete mode."
    }
    if ([int]$manifestNumberingConfidence.details.document_count -ne 2) {
        throw "Release assets manifest used the wrong real corpus confidence details for numbering in AllowIncomplete mode."
    }
    if ([int]$manifestNumberingConfidence.details.matched_document_count -ne 2) {
        throw "Release assets manifest lost numbering real corpus confidence matched document count in AllowIncomplete mode."
    }
    if ([int]$manifestNumberingConfidence.details.alignment_gap_count -ne 0) {
        throw "Release assets manifest lost numbering real corpus confidence alignment gap count in AllowIncomplete mode."
    }
    if (-not (@($manifestNumberingConfidence.details.matched_document_keys) -contains "contract.docx")) {
        throw "Release assets manifest lost numbering real corpus confidence matched document keys in AllowIncomplete mode."
    }
    $manifestNumberingPenaltyFactors = @($manifestNumberingConfidence.details.penalty_summary |
        ForEach-Object { [string]$_.factor })
    if ($manifestNumberingPenaltyFactors -contains "style_catalog_only") {
        throw "Release assets manifest mirrored style catalog confidence details into numbering confidence in AllowIncomplete mode."
    }
    if (-not ($manifestNumberingPenaltyFactors -contains "style_numbering_issues")) {
        throw "Release assets manifest lost numbering real corpus confidence penalty summary in AllowIncomplete mode."
    }

    $manifestTableLayoutDeliveryQuality = $manifest.table_layout_delivery_quality
    if ($null -eq $manifestTableLayoutDeliveryQuality) {
        throw "Release assets manifest lost table_layout_delivery_quality in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.id -ne "table_layout_delivery_governance.delivery_quality") {
        throw "Release assets manifest used the wrong table layout delivery quality id in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.metric -ne "delivery_quality") {
        throw "Release assets manifest lost table layout delivery quality metric in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.report_id -ne "table_layout_delivery_governance") {
        throw "Release assets manifest lost table layout delivery quality report_id in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.source_schema -ne "featherdoc.table_layout_delivery_governance_report.v1") {
        throw "Release assets manifest used the wrong table layout delivery quality source_schema in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.source_report -ne ".\output\table-layout-delivery-governance\summary.json") {
        throw "Release assets manifest lost table layout delivery quality source_report in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.source_report_display -ne ".\output\table-layout-delivery-governance\summary.json") {
        throw "Release assets manifest lost table layout delivery quality source_report_display in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.source_json -ne ".\output\table-layout-delivery-governance\summary.json") {
        throw "Release assets manifest lost table layout delivery quality source_json in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.source_json_display -ne ".\output\table-layout-delivery-governance\summary.json") {
        throw "Release assets manifest lost table layout delivery quality source_json_display in AllowIncomplete mode."
    }
    if ([int]$manifestTableLayoutDeliveryQuality.score -ne 100) {
        throw "Release assets manifest lost table layout delivery quality score in AllowIncomplete mode."
    }
    if ([string]$manifestTableLayoutDeliveryQuality.level -ne "release_ready") {
        throw "Release assets manifest lost table layout delivery quality level in AllowIncomplete mode."
    }
    if ($null -eq $manifestTableLayoutDeliveryQuality.details) {
        throw "Release assets manifest lost table layout delivery quality details in AllowIncomplete mode."
    }
    if ([int]$manifestTableLayoutDeliveryQuality.details.document_count -ne 3) {
        throw "Release assets manifest lost table layout delivery quality document_count in AllowIncomplete mode."
    }
    if ([int]$manifestTableLayoutDeliveryQuality.details.unresolved_item_count -ne 0) {
        throw "Release assets manifest lost table layout delivery quality unresolved_item_count in AllowIncomplete mode."
    }
    if ([int]$manifestTableLayoutDeliveryQuality.details.pdf_floating_table_supported_geometry_percent -ne 44) {
        throw "Release assets manifest lost table layout delivery quality PDF floating table support percentage in AllowIncomplete mode."
    }
    if ([int]$manifestTableLayoutDeliveryQuality.details.pdf_floating_table_tracked_geometry_count -ne 9) {
        throw "Release assets manifest lost table layout delivery quality PDF floating table tracked geometry count in AllowIncomplete mode."
    }
    if (-not (@($manifestTableLayoutDeliveryQuality.details.metadata_only_fields) -contains "leftFromText")) {
        throw "Release assets manifest lost table layout delivery quality metadata_only_fields in AllowIncomplete mode."
    }
    if (-not (@($manifestTableLayoutDeliveryQuality.details.review_required_fields) -contains "full Word-compatible floating table text wrapping")) {
        throw "Release assets manifest lost table layout delivery quality review_required_fields in AllowIncomplete mode."
    }
    if ([int]$manifestTableLayoutDeliveryQuality.details.table_position_automatic_count -ne 0) {
        throw "Release assets manifest lost table layout delivery quality table_position_automatic_count in AllowIncomplete mode."
    }
    if ([int]$manifestTableLayoutDeliveryQuality.details.table_position_review_count -ne 0) {
        throw "Release assets manifest lost table layout delivery quality table_position_review_count in AllowIncomplete mode."
    }
    if (-not (@($manifestTableLayoutDeliveryQuality.details.penalty_summary |
            ForEach-Object { [string]$_.factor }) -contains "floating_table_plans_pending")) {
        throw "Release assets manifest lost table layout delivery quality penalty summary in AllowIncomplete mode."
    }

    $contentControlContract = @($manifest.content_control_repair_contracts | Where-Object {
        [string]$_.id -eq "content_control_data_binding.bound_placeholder"
    }) | Select-Object -First 1
    if ($null -eq $contentControlContract) {
        throw "Release assets manifest lost content_control_data_binding.bound_placeholder repair contract in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.source_schema -ne "featherdoc.content_control_data_binding_governance_report.v1") {
        throw "Release assets manifest lost content-control repair contract source_schema in AllowIncomplete mode."
    }
    $expectedContentControlSummaryDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path $contentControlSummaryPath `
        -RepoRoot $resolvedRepoRoot
    if ([string]$contentControlContract.source_json_display -ne $expectedContentControlSummaryDisplay) {
        throw "Release assets manifest lost content-control repair contract source_json_display in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.input_docx -ne "samples/invoice.docx") {
        throw "Release assets manifest lost content-control input_docx provenance in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.input_docx_display -ne ".\samples\invoice.docx") {
        throw "Release assets manifest lost content-control input_docx_display provenance in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.template_name -ne "invoice-template") {
        throw "Release assets manifest lost content-control template_name provenance in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.schema_target -ne "invoice") {
        throw "Release assets manifest lost content-control schema_target provenance in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.target_mode -ne "resolved-section-targets") {
        throw "Release assets manifest lost content-control target_mode provenance in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.repair_strategy -ne "sync_bound_content_control") {
        throw "Release assets manifest lost sync_bound_content_control repair strategy in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.repair_hint -ne "Rerun Custom XML sync or explicitly fill the bound content control before release.") {
        throw "Release assets manifest lost content-control repair_hint in AllowIncomplete mode."
    }
    if ([string]$contentControlContract.command_template -notmatch "sync-content-controls-from-custom-xml") {
        throw "Release assets manifest lost sync-content-controls-from-custom-xml command template in AllowIncomplete mode."
    }

    $projectTemplateDeliveryReadinessContract = $manifest.project_template_delivery_readiness_contract
    if ($null -eq $projectTemplateDeliveryReadinessContract) {
        throw "Release assets manifest lost project_template_delivery_readiness_contract in AllowIncomplete mode."
    }
    $expectedProjectTemplateDeliveryReadinessDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path $projectTemplateDeliveryReadinessSummaryPath `
        -RepoRoot $resolvedRepoRoot
    if ([string]$projectTemplateDeliveryReadinessContract.schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
        throw "Release assets manifest lost project template delivery readiness schema in AllowIncomplete mode."
    }
    if ([string]$projectTemplateDeliveryReadinessContract.source_schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
        throw "Release assets manifest lost project template delivery readiness source_schema in AllowIncomplete mode."
    }
    if ([string]$projectTemplateDeliveryReadinessContract.source_report_display -ne $expectedProjectTemplateDeliveryReadinessDisplay) {
        throw "Release assets manifest lost project template delivery readiness source_report_display in AllowIncomplete mode."
    }
    if ([string]$projectTemplateDeliveryReadinessContract.source_json_display -ne $expectedProjectTemplateDeliveryReadinessDisplay) {
        throw "Release assets manifest lost project template delivery readiness source_json_display in AllowIncomplete mode."
    }
    $projectTemplateDeliveryReadinessSchemaSummary = @($projectTemplateDeliveryReadinessContract.schema_approval_status_summary)
    if ($projectTemplateDeliveryReadinessSchemaSummary.Count -ne 1 -or
        [string]$projectTemplateDeliveryReadinessSchemaSummary[0].status -ne "approved" -or
        [int]$projectTemplateDeliveryReadinessSchemaSummary[0].count -ne 4) {
        throw "Release assets manifest lost project template delivery readiness schema_approval_status_summary in AllowIncomplete mode."
    }
    $projectTemplateDeliveryReadinessBusinessTypes = @($projectTemplateDeliveryReadinessContract.business_document_type_summary)
    if ($projectTemplateDeliveryReadinessBusinessTypes.Count -ne 2 -or
        [string]$projectTemplateDeliveryReadinessBusinessTypes[0].document_type -ne "invoice" -or
        [string]$projectTemplateDeliveryReadinessBusinessTypes[1].document_type -ne "policy") {
        throw "Release assets manifest lost project template delivery readiness business_document_type_summary in AllowIncomplete mode."
    }
    $projectTemplateDeliveryReadinessCorpusRoles = @($projectTemplateDeliveryReadinessContract.corpus_role_summary)
    if ($projectTemplateDeliveryReadinessCorpusRoles.Count -ne 2 -or
        [string]$projectTemplateDeliveryReadinessCorpusRoles[0].corpus_role -ne "planned-business-template" -or
        [string]$projectTemplateDeliveryReadinessCorpusRoles[1].corpus_role -ne "registered-business-template") {
        throw "Release assets manifest lost project template delivery readiness corpus_role_summary in AllowIncomplete mode."
    }
    if ([bool]$projectTemplateDeliveryReadinessContract.requires_reviewer_action) {
        throw "Release assets manifest lost project template delivery readiness requires_reviewer_action=false in AllowIncomplete mode."
    }
    if ([string]$projectTemplateDeliveryReadinessContract.reviewer_action_summary -ne "none") {
        throw "Release assets manifest lost project template delivery readiness reviewer_action_summary in AllowIncomplete mode."
    }
    if ([string]$projectTemplateDeliveryReadinessContract.reviewer_action_reason -ne "latest_review_state=approved; no reviewer action required") {
        throw "Release assets manifest lost project template delivery readiness reviewer_action_reason in AllowIncomplete mode."
    }
    if (@($projectTemplateDeliveryReadinessContract.reviewer_actions).Count -ne 0) {
        throw "Release assets manifest lost project template delivery readiness empty reviewer_actions in AllowIncomplete mode."
    }

    $projectTemplateOnboardingGovernanceContract = $manifest.project_template_onboarding_governance_contract
    if ($null -eq $projectTemplateOnboardingGovernanceContract) {
        throw "Release assets manifest lost project_template_onboarding_governance_contract in AllowIncomplete mode."
    }
    $expectedProjectTemplateOnboardingGovernanceDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path $projectTemplateOnboardingGovernanceSummaryPath `
        -RepoRoot $resolvedRepoRoot
    if ([string]$projectTemplateOnboardingGovernanceContract.schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
        throw "Release assets manifest lost project template onboarding governance schema in AllowIncomplete mode."
    }
    if ([string]$projectTemplateOnboardingGovernanceContract.source_schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
        throw "Release assets manifest lost project template onboarding governance source_schema in AllowIncomplete mode."
    }
    if ([string]$projectTemplateOnboardingGovernanceContract.status -ne "ready") {
        throw "Release assets manifest lost project template onboarding governance status in AllowIncomplete mode."
    }
    if (-not [bool]$projectTemplateOnboardingGovernanceContract.release_ready) {
        throw "Release assets manifest lost project template onboarding governance release_ready=true in AllowIncomplete mode."
    }
    if ([string]$projectTemplateOnboardingGovernanceContract.source_report_display -ne $expectedProjectTemplateOnboardingGovernanceDisplay) {
        throw "Release assets manifest lost project template onboarding governance source_report_display in AllowIncomplete mode."
    }
    if ([string]$projectTemplateOnboardingGovernanceContract.source_json_display -ne $expectedProjectTemplateOnboardingGovernanceDisplay) {
        throw "Release assets manifest lost project template onboarding governance source_json_display in AllowIncomplete mode."
    }
    $projectTemplateOnboardingGovernanceBusinessTypes = @($projectTemplateOnboardingGovernanceContract.business_document_type_summary)
    if ($projectTemplateOnboardingGovernanceBusinessTypes.Count -ne 2 -or
        [string]$projectTemplateOnboardingGovernanceBusinessTypes[0].document_type -ne "invoice" -or
        [string]$projectTemplateOnboardingGovernanceBusinessTypes[1].document_type -ne "policy") {
        throw "Release assets manifest lost project template onboarding governance business_document_type_summary in AllowIncomplete mode."
    }
    $projectTemplateOnboardingGovernanceCorpusRoles = @($projectTemplateOnboardingGovernanceContract.corpus_role_summary)
    if ($projectTemplateOnboardingGovernanceCorpusRoles.Count -ne 2 -or
        [string]$projectTemplateOnboardingGovernanceCorpusRoles[0].corpus_role -ne "planned-business-template" -or
        [string]$projectTemplateOnboardingGovernanceCorpusRoles[1].corpus_role -ne "registered-business-template") {
        throw "Release assets manifest lost project template onboarding governance corpus_role_summary in AllowIncomplete mode."
    }
    if ([bool]$projectTemplateOnboardingGovernanceContract.requires_reviewer_action) {
        throw "Release assets manifest lost project template onboarding governance requires_reviewer_action=false in AllowIncomplete mode."
    }
    if ([string]$projectTemplateOnboardingGovernanceContract.reviewer_action_summary -ne "none") {
        throw "Release assets manifest lost project template onboarding governance reviewer_action_summary in AllowIncomplete mode."
    }
    if ([string]$projectTemplateOnboardingGovernanceContract.reviewer_action_reason -ne "latest_review_state=approved; no reviewer action required") {
        throw "Release assets manifest lost project template onboarding governance reviewer_action_reason in AllowIncomplete mode."
    }
    if (@($projectTemplateOnboardingGovernanceContract.reviewer_actions).Count -ne 0) {
        throw "Release assets manifest lost project template onboarding governance empty reviewer_actions in AllowIncomplete mode."
    }

    $stagedSummaryContent = Get-Content -Raw -LiteralPath $stagedSummaryPath
    foreach ($expectedText in @("governance_metrics", "real_corpus_confidence", "delivery_quality", "content_control_data_binding_governance", "project_template_delivery_readiness", "project_template_onboarding_governance")) {
        if ($stagedSummaryContent -notmatch [regex]::Escape($expectedText)) {
            throw "Staged allow-incomplete summary lost governance metric text '$expectedText'."
        }
    }

    $stagedGovernanceHandoffContent = Get-Content -Raw -LiteralPath $stagedGovernanceHandoffPath
    foreach ($expectedText in @(
        "Report Status",
        "project_template_delivery_readiness",
        "featherdoc.project_template_delivery_readiness_report.v1",
        "latest_schema_approval_gate_status",
        "schema_approval_status_summary",
        "source_report_display",
        "source_json_display",
        ".\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract",
        "featherdoc.project_template_onboarding_governance_report.v1",
        'status: `ready`',
        'release_ready: `True`',
        ".\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
        "Governance Metrics",
        "real_corpus_confidence",
        "numbering_catalog_governance.real_corpus_confidence",
        "featherdoc.numbering_catalog_governance_report.v1",
        "delivery_quality",
        "table_layout_delivery_governance.delivery_quality"
    )) {
        if ($stagedGovernanceHandoffContent -notmatch [regex]::Escape($expectedText)) {
            throw "Staged allow-incomplete governance handoff lost '$expectedText'."
        }
    }

    $stagedFinalReviewContent = Get-Content -Raw -LiteralPath $stagedFinalReviewPath
    foreach ($expectedText in @(
        "Release governance handoff details",
        "project_template_delivery_readiness",
        "project_template_onboarding.schema_approval",
        "featherdoc.project_template_onboarding_governance_report.v1",
        "project_template_onboarding_governance_contract",
        "status: ready",
        "release_ready: True",
        "schema_approval_status_summary: approved",
        "source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json",
        "source_json_display: .\output\project-template-onboarding-governance\summary.json",
        "readiness_status: ready",
        "readiness_release_ready: True"
    )) {
        if ($stagedFinalReviewContent -notmatch [regex]::Escape($expectedText)) {
            throw "Staged allow-incomplete final_review.md lost '$expectedText'."
        }
    }

    $stagedStartHereContent = Get-Content -Raw -LiteralPath $stagedStartHerePath
    foreach ($expectedText in @(
        "content_control_data_binding.bound_placeholder",
        "featherdoc.content_control_data_binding_governance_report.v1",
        "source_json_display",
        "input_docx=samples/invoice.docx",
        "template_name=invoice-template",
        "schema_target=invoice",
        "target_mode=resolved-section-targets",
        "repair_strategy",
        "repair_hint",
        "sync_bound_content_control",
        "command_template",
        "sync-content-controls-from-custom-xml",
        "project_template_delivery_readiness",
        "project_template_delivery_readiness_contract",
        "featherdoc.project_template_delivery_readiness_report.v1",
        "latest_schema_approval_gate_status",
        "schema_approval_status_summary=approved=4",
        "source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
        "source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract",
        "featherdoc.project_template_onboarding_governance_report.v1",
        "status=ready",
        "release_ready=True",
        "schema_approval_status_summary",
        "source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
        "source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
        "Project-template readiness checklist handoff evidence",
        "project_template_readiness_checklist_entrypoints_source_reports=1",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "required_entrypoint_count=3",
        "entrypoints=start_here, artifact_guide, reviewer_checklist",
        "entrypoint_paths=",
        "start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md",
        "artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md",
        "reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md",
        "marker=release_entry_project_template_readiness_checklist_trace",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report=.\output\release-candidate-checks-ci\summary.json",
        "numbering_catalog_governance.real_corpus_confidence",
        "table_layout_delivery_governance.delivery_quality",
        "pdf_floating_table_support_coverage",
        "pdf_floating_table_reviewer_focus",
        "metadata-only tblpPr",
        "metadata_only_fields",
        "review_required_fields",
        "catalog_coverage_percent=100",
        "matched_document_count=2",
        "alignment_gap_count=0",
        "matched_document_keys=contract.docx,invoice.docx",
        "style_numbering_issues(count=4, penalty=20)",
        "unresolved_item_count=0",
        "floating_table_plans_pending(count=0, penalty=0)"
    )) {
        if ($stagedStartHereContent -notmatch [regex]::Escape($expectedText)) {
            throw "Staged allow-incomplete START_HERE lost '$expectedText'."
        }
    }

    $stagedArtifactGuideContent = Get-Content -Raw -LiteralPath $stagedArtifactGuidePath
    foreach ($expectedText in @(
        "content_control_data_binding.bound_placeholder",
        "featherdoc.content_control_data_binding_governance_report.v1",
        "source_json_display",
        "input_docx=samples/invoice.docx",
        "template_name=invoice-template",
        "schema_target=invoice",
        "target_mode=resolved-section-targets",
        "repair_strategy",
        "repair_hint",
        "sync_bound_content_control",
        "command_template",
        "sync-content-controls-from-custom-xml",
        "project_template_delivery_readiness",
        "project_template_delivery_readiness_contract",
        "featherdoc.project_template_delivery_readiness_report.v1",
        "latest_schema_approval_gate_status",
        "schema_approval_status_summary=approved=4",
        "source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
        "source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract",
        "featherdoc.project_template_onboarding_governance_report.v1",
        "status=ready",
        "release_ready=True",
        "schema_approval_status_summary",
        "source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
        "source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
        "Project-template readiness checklist handoff evidence",
        "project_template_readiness_checklist_entrypoints_source_reports=1",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "required_entrypoint_count=3",
        "entrypoints=start_here, artifact_guide, reviewer_checklist",
        "entrypoint_paths=",
        "start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md",
        "artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md",
        "reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md",
        "marker=release_entry_project_template_readiness_checklist_trace",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report=.\output\release-candidate-checks-ci\summary.json",
        "numbering_catalog_governance.real_corpus_confidence",
        "featherdoc.numbering_catalog_governance_report.v1",
        "table_layout_delivery_governance.delivery_quality",
        "pdf_floating_table_support_coverage",
        "pdf_floating_table_reviewer_focus",
        "metadata-only tblpPr",
        "metadata_only_fields",
        "review_required_fields",
        "catalog_coverage_percent=100",
        "matched_document_count=2",
        "alignment_gap_count=0",
        "matched_document_keys=contract.docx,invoice.docx",
        "style_numbering_issues(count=4, penalty=20)",
        "unresolved_item_count=0",
        "floating_table_plans_pending(count=0, penalty=0)"
    )) {
        if ($stagedArtifactGuideContent -notmatch [regex]::Escape($expectedText)) {
            throw "Staged allow-incomplete ARTIFACT_GUIDE lost '$expectedText'."
        }
    }

    $stagedReviewerChecklistContent = Get-Content -Raw -LiteralPath $stagedReviewerChecklistPath
    foreach ($expectedText in @(
        "content_control_data_binding.bound_placeholder",
        "featherdoc.content_control_data_binding_governance_report.v1",
        "source_json_display",
        "input_docx=samples/invoice.docx",
        "template_name=invoice-template",
        "schema_target=invoice",
        "target_mode=resolved-section-targets",
        "repair_strategy",
        "repair_hint",
        "sync_bound_content_control",
        "command_template",
        "sync-content-controls-from-custom-xml",
        "project_template_delivery_readiness",
        "project_template_delivery_readiness_contract",
        "featherdoc.project_template_delivery_readiness_report.v1",
        "latest_schema_approval_gate_status",
        "schema_approval_status_summary=approved=4",
        "source_report_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
        "source_json_display=.\output\release-candidate-checks-ci\report\project_template_delivery_readiness_summary.json",
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract",
        "featherdoc.project_template_onboarding_governance_report.v1",
        "status=ready",
        "release_ready=True",
        "schema_approval_status_summary",
        "source_report_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
        "source_json_display=.\output\release-candidate-checks-ci\report\project_template_onboarding_governance_summary.json",
        "Project-template readiness checklist handoff evidence",
        "project_template_readiness_checklist_entrypoints_source_reports=1",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "required_entrypoint_count=3",
        "entrypoints=start_here, artifact_guide, reviewer_checklist",
        "entrypoint_paths=",
        "start_here:required=True:path_display=.\output\release-candidate-checks-ci\START_HERE.md",
        "artifact_guide:required=True:path_display=.\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md",
        "reviewer_checklist:required=True:path_display=.\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md",
        "marker=release_entry_project_template_readiness_checklist_trace",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report=.\output\release-candidate-checks-ci\summary.json",
        "numbering_catalog_governance.real_corpus_confidence",
        "table_layout_delivery_governance.delivery_quality",
        "pdf_floating_table_support_coverage",
        "pdf_floating_table_reviewer_focus",
        "metadata-only tblpPr",
        "metadata_only_fields",
        "review_required_fields",
        "catalog_coverage_percent=100",
        "matched_document_count=2",
        "alignment_gap_count=0",
        "matched_document_keys=contract.docx,invoice.docx",
        "style_numbering_issues(count=4, penalty=20)",
        "unresolved_item_count=0",
        "floating_table_plans_pending(count=0, penalty=0)"
    )) {
        if ($stagedReviewerChecklistContent -notmatch [regex]::Escape($expectedText)) {
            throw "Staged allow-incomplete REVIEWER_CHECKLIST lost '$expectedText'."
        }
    }
}
