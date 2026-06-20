if ([int]$manifest.word_visual_standard_review_metadata_count -ne 4) {
    throw "release_assets_manifest.json did not record four standard Word visual review metadata entries."
}
if ($wordVisualStandardReviewMetadata.Count -ne 4) {
    throw "release_assets_manifest.json lost standard Word visual review metadata entries."
}
$wordVisualStandardReviewMetadataByTask = @{}
foreach ($entry in $wordVisualStandardReviewMetadata) {
    $wordVisualStandardReviewMetadataByTask[[string]$entry.task_key] = $entry
    if ($entry.PSObject.Properties["review_note"]) {
        throw "release_assets_manifest.json must not expose standard Word visual review notes."
    }
    foreach ($pathField in @("review_result_path", "final_review_path")) {
        $pathValue = [string]$entry.$pathField
        if ([string]::IsNullOrWhiteSpace($pathValue)) {
            throw "release_assets_manifest.json lost standard Word visual review $pathField for '$($entry.task_key)'."
        }
        if ($pathValue.Contains($resolvedRepoRoot)) {
            throw "release_assets_manifest.json did not public-sanitize standard Word visual review $pathField for '$($entry.task_key)'."
        }
    }
}
foreach ($expectedTaskKey in @("smoke", "fixed_grid", "section_page_setup", "page_number_fields")) {
    if (-not $wordVisualStandardReviewMetadataByTask.ContainsKey($expectedTaskKey)) {
        throw "release_assets_manifest.json lost standard Word visual review metadata task '$expectedTaskKey'."
    }
}
$manifestSmokeReviewMetadata = $wordVisualStandardReviewMetadataByTask["smoke"]
if ([string]$manifestSmokeReviewMetadata.review_task_key -ne "document") {
    throw "release_assets_manifest.json did not preserve the smoke review task key alias."
}
if ((Convert-TestComparableValue -Value $manifestSmokeReviewMetadata.verdict) -ne "pass" -or
    (Convert-TestComparableValue -Value $manifestSmokeReviewMetadata.review_status) -ne "reviewed" -or
    (Convert-TestComparableValue -Value $manifestSmokeReviewMetadata.reviewed_at) -ne "2026-04-12T12:10:00" -or
    (Convert-TestComparableValue -Value $manifestSmokeReviewMetadata.review_method) -ne "operator_supplied") {
    throw "release_assets_manifest.json lost the smoke standard Word visual review status metadata."
}
if ([string]$manifestSmokeReviewMetadata.review_result_path -ne $expectedSmokeReviewResultPath) {
    throw "release_assets_manifest.json did not preserve the smoke standard Word visual review result path."
}
if ([string]$manifestSmokeReviewMetadata.final_review_path -ne $expectedSmokeFinalReviewPath) {
    throw "release_assets_manifest.json did not preserve the smoke standard Word visual final review path."
}
if ([string]$manifest.workspace -ne ".") {
    throw "release_assets_manifest.json did not rewrite workspace to a public relative path."
}
if ([string]$manifest.summary_json -ne $expectedManifestSummaryPath) {
    throw "release_assets_manifest.json did not preserve summary_json as a public display path."
}
if ([string]$manifest.pdf_visual_gate_summary_json -ne $pdfGateSummaryPathDisplay) {
    throw "release_assets_manifest.json did not preserve pdf_visual_gate_summary_json as a public display path."
}
if ([string]$manifest.pdf_visual_gate_output_dir -ne $expectedManifestPdfGateOutputDir) {
    throw "release_assets_manifest.json did not preserve pdf_visual_gate_output_dir as a public display path."
}
if ($manifest.governance_metric_count -ne 3) {
    throw "release_assets_manifest.json did not preserve governance_metric_count=3."
}

$manifestReleaseNoteBundle = $manifest.release_note_bundle
if ($null -eq $manifestReleaseNoteBundle) {
    throw "release_assets_manifest.json lost release_note_bundle."
}
if ([string]$manifestReleaseNoteBundle.status -ne "declared") {
    throw "release_assets_manifest.json lost release_note_bundle status."
}
if ([int]$manifestReleaseNoteBundle.entrypoint_count -ne 6) {
    throw "release_assets_manifest.json lost release_note_bundle entrypoint count."
}
if ([int]$manifestReleaseNoteBundle.required_entrypoint_count -ne 6) {
    throw "release_assets_manifest.json lost release_note_bundle required entrypoint count."
}
$manifestReleaseNoteBundleEntrypointsById = @{}
foreach ($entrypoint in @($manifestReleaseNoteBundle.entrypoints)) {
    $manifestReleaseNoteBundleEntrypointsById[[string]$entrypoint.id] = $entrypoint
}
foreach ($entrypointExpectation in @(
        [ordered]@{ id = "start_here"; path = $startHerePath; path_display = ".\output\release-candidate-checks\START_HERE.md"; location = "summary_root" },
        [ordered]@{ id = "artifact_guide"; path = $artifactGuidePath; path_display = ".\output\release-candidate-checks\report\ARTIFACT_GUIDE.md"; location = "report" },
        [ordered]@{ id = "reviewer_checklist"; path = $reviewerChecklistPath; path_display = ".\output\release-candidate-checks\report\REVIEWER_CHECKLIST.md"; location = "report" },
        [ordered]@{ id = "release_handoff"; path = $releaseHandoffPath; path_display = ".\output\release-candidate-checks\report\release_handoff.md"; location = "report" },
        [ordered]@{ id = "release_body_zh_cn"; path = $releaseBodyPath; path_display = ".\output\release-candidate-checks\report\release_body.zh-CN.md"; location = "report" },
        [ordered]@{ id = "release_summary_zh_cn"; path = $releaseSummaryPath; path_display = ".\output\release-candidate-checks\report\release_summary.zh-CN.md"; location = "report" }
    )) {
    $entrypointId = [string]$entrypointExpectation.id
    if (-not (@($manifestReleaseNoteBundle.entrypoint_ids | ForEach-Object { [string]$_ }) -contains $entrypointId)) {
        throw "release_assets_manifest.json lost release_note_bundle entrypoint id '$entrypointId'."
    }
    if (-not $manifestReleaseNoteBundleEntrypointsById.ContainsKey($entrypointId)) {
        throw "release_assets_manifest.json lost release_note_bundle entrypoint '$entrypointId'."
    }

    $entrypoint = $manifestReleaseNoteBundleEntrypointsById[$entrypointId]
    if (-not [bool]$entrypoint.required) {
        throw "release_assets_manifest.json lost required=true for release_note_bundle entrypoint '$entrypointId'."
    }
    if ([string]$entrypoint.location -ne [string]$entrypointExpectation.location) {
        throw "release_assets_manifest.json lost release_note_bundle entrypoint '$entrypointId' location."
    }

    $expectedEntrypointDisplay = Convert-TestEvidencePathToPublicDisplay `
        -Path ([string]$entrypointExpectation.path) `
        -RepoRoot $resolvedRepoRoot
    if ([string]$entrypoint.path -ne $expectedEntrypointDisplay) {
        throw "release_assets_manifest.json did not public-sanitize release_note_bundle entrypoint '$entrypointId' path."
    }

    $expectedEntrypointPathDisplay = [string]$entrypointExpectation.path_display
    if ((Convert-TestComparablePathValue -Value $entrypoint.path_display) -ne (Convert-TestComparablePathValue -Value $expectedEntrypointPathDisplay)) {
        throw "release_assets_manifest.json lost release_note_bundle entrypoint '$entrypointId' path_display. Expected='$expectedEntrypointPathDisplay' Actual='$($entrypoint.path_display)'."
    }
}

if ($manifest.governance_metrics.Count -ne 3) {
    throw "release_assets_manifest.json did not preserve all governance metrics."
}
if (-not (@($manifest.governance_metrics | ForEach-Object { $_.metric }) -contains "real_corpus_confidence")) {
    throw "release_assets_manifest.json lost real_corpus_confidence governance metric."
}
if (-not (@($manifest.governance_metrics | ForEach-Object { $_.metric }) -contains "delivery_quality")) {
    throw "release_assets_manifest.json lost delivery_quality governance metric."
}
$manifestNumberingConfidence = $manifest.numbering_catalog_real_corpus_confidence
if ($null -eq $manifestNumberingConfidence) {
    throw "release_assets_manifest.json lost numbering_catalog_real_corpus_confidence."
}
if ([string]$manifestNumberingConfidence.id -ne "numbering_catalog_governance.real_corpus_confidence") {
    throw "release_assets_manifest.json lost numbering real corpus confidence id."
}
if ([string]$manifestNumberingConfidence.metric -ne "real_corpus_confidence") {
    throw "release_assets_manifest.json lost numbering real corpus confidence metric."
}
if ([string]$manifestNumberingConfidence.report_id -ne "numbering_catalog_governance") {
    throw "release_assets_manifest.json lost numbering real corpus confidence report_id."
}
if ([string]$manifestNumberingConfidence.source_schema -ne "featherdoc.numbering_catalog_governance_report.v1") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_schema."
}
if ([string]$manifestNumberingConfidence.source_report -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_report."
}
if ([string]$manifestNumberingConfidence.source_report_display -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_report_display."
}
if ([string]$manifestNumberingConfidence.source_json -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_json."
}
if ([string]$manifestNumberingConfidence.source_json_display -ne ".\output\numbering-catalog-governance\summary.json") {
    throw "release_assets_manifest.json lost numbering real corpus confidence source_json_display."
}
if ([int]$manifestNumberingConfidence.score -ne 56) {
    throw "release_assets_manifest.json lost numbering real corpus confidence score."
}
if ([string]$manifestNumberingConfidence.level -ne "low") {
    throw "release_assets_manifest.json lost numbering real corpus confidence level."
}
if ($null -eq $manifestNumberingConfidence.details) {
    throw "release_assets_manifest.json lost numbering real corpus confidence details."
}
if ([int]$manifestNumberingConfidence.details.document_count -ne 2) {
    throw "release_assets_manifest.json used the wrong real corpus confidence details for numbering."
}
if ([int]$manifestNumberingConfidence.details.catalog_coverage_percent -ne 100) {
    throw "release_assets_manifest.json lost numbering real corpus confidence catalog coverage."
}
if ([int]$manifestNumberingConfidence.details.matched_document_count -ne 2) {
    throw "release_assets_manifest.json lost numbering real corpus confidence matched document count."
}
if ([int]$manifestNumberingConfidence.details.alignment_gap_count -ne 0) {
    throw "release_assets_manifest.json lost numbering real corpus confidence alignment gap count."
}
if (-not (@($manifestNumberingConfidence.details.matched_document_keys) -contains "contract.docx")) {
    throw "release_assets_manifest.json lost numbering real corpus confidence matched document keys."
}
$manifestNumberingPenaltyFactors = @($manifestNumberingConfidence.details.penalty_summary |
    ForEach-Object { [string]$_.factor })
if ($manifestNumberingPenaltyFactors -contains "style_catalog_only") {
    throw "release_assets_manifest.json mirrored style catalog confidence details into numbering confidence."
}
if (-not ($manifestNumberingPenaltyFactors -contains "style_numbering_issues")) {
    throw "release_assets_manifest.json lost numbering real corpus confidence penalty summary."
}
$manifestTableLayoutDeliveryQuality = $manifest.table_layout_delivery_quality
if ($null -eq $manifestTableLayoutDeliveryQuality) {
    throw "release_assets_manifest.json lost table_layout_delivery_quality."
}
if ([string]$manifestTableLayoutDeliveryQuality.id -ne "table_layout_delivery_governance.delivery_quality") {
    throw "release_assets_manifest.json lost table layout delivery quality id."
}
if ([string]$manifestTableLayoutDeliveryQuality.metric -ne "delivery_quality") {
    throw "release_assets_manifest.json lost table layout delivery quality metric."
}
if ([string]$manifestTableLayoutDeliveryQuality.report_id -ne "table_layout_delivery_governance") {
    throw "release_assets_manifest.json lost table layout delivery quality report_id."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_schema -ne "featherdoc.table_layout_delivery_governance_report.v1") {
    throw "release_assets_manifest.json lost table layout delivery quality source_schema."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_report -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "release_assets_manifest.json lost table layout delivery quality source_report."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_report_display -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "release_assets_manifest.json lost table layout delivery quality source_report_display."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_json -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "release_assets_manifest.json lost table layout delivery quality source_json."
}
if ([string]$manifestTableLayoutDeliveryQuality.source_json_display -ne ".\output\table-layout-delivery-governance\summary.json") {
    throw "release_assets_manifest.json lost table layout delivery quality source_json_display."
}
if ([int]$manifestTableLayoutDeliveryQuality.score -ne 100) {
    throw "release_assets_manifest.json lost table layout delivery quality score."
}
if ([string]$manifestTableLayoutDeliveryQuality.level -ne "release_ready") {
    throw "release_assets_manifest.json lost table layout delivery quality level."
}
if ($null -eq $manifestTableLayoutDeliveryQuality.details) {
    throw "release_assets_manifest.json lost table layout delivery quality details."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.document_count -ne 3) {
    throw "release_assets_manifest.json lost table layout delivery quality document_count."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.unresolved_item_count -ne 0) {
    throw "release_assets_manifest.json lost table layout delivery quality unresolved_item_count."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.pdf_floating_table_supported_geometry_percent -ne 44) {
    throw "release_assets_manifest.json lost table layout delivery quality PDF floating table support percentage."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.pdf_floating_table_tracked_geometry_count -ne 9) {
    throw "release_assets_manifest.json lost table layout delivery quality PDF floating table tracked geometry count."
}
if (-not (@($manifestTableLayoutDeliveryQuality.details.metadata_only_fields) -contains "leftFromText")) {
    throw "release_assets_manifest.json lost table layout delivery quality metadata_only_fields."
}
if (-not (@($manifestTableLayoutDeliveryQuality.details.review_required_fields) -contains "full Word-compatible floating table text wrapping")) {
    throw "release_assets_manifest.json lost table layout delivery quality review_required_fields."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.table_position_automatic_count -ne 0) {
    throw "release_assets_manifest.json lost table layout delivery quality table_position_automatic_count."
}
if ([int]$manifestTableLayoutDeliveryQuality.details.table_position_review_count -ne 0) {
    throw "release_assets_manifest.json lost table layout delivery quality table_position_review_count."
}
if (-not (@($manifestTableLayoutDeliveryQuality.details.penalty_summary |
        ForEach-Object { [string]$_.factor }) -contains "floating_table_plans_pending")) {
    throw "release_assets_manifest.json lost table layout delivery quality penalty summary."
}
if ($manifest.content_control_repair_contract_count -ne 1) {
    throw "release_assets_manifest.json did not preserve content_control_repair_contract_count=1."
}
$manifestContentControlContract = @($manifest.content_control_repair_contracts | Where-Object {
    [string]$_.id -eq "content_control_data_binding.bound_placeholder"
}) | Select-Object -First 1
if ($null -eq $manifestContentControlContract) {
    throw "release_assets_manifest.json lost content_control_data_binding.bound_placeholder repair contract."
}
if ([string]$manifestContentControlContract.source_schema -ne "featherdoc.content_control_data_binding_governance_report.v1") {
    throw "release_assets_manifest.json lost the content-control governance source schema."
}
if ([string]$manifestContentControlContract.input_docx -ne "samples/invoice.docx") {
    throw "release_assets_manifest.json lost content-control input_docx provenance."
}
if ([string]$manifestContentControlContract.input_docx_display -ne ".\samples\invoice.docx") {
    throw "release_assets_manifest.json lost content-control input_docx_display provenance."
}
if ([string]$manifestContentControlContract.template_name -ne "invoice-template") {
    throw "release_assets_manifest.json lost content-control template_name provenance."
}
if ([string]$manifestContentControlContract.schema_target -ne "invoice") {
    throw "release_assets_manifest.json lost content-control schema_target provenance."
}
if ([string]$manifestContentControlContract.target_mode -ne "resolved-section-targets") {
    throw "release_assets_manifest.json lost content-control target_mode provenance."
}
if ([string]$manifestContentControlContract.repair_strategy -ne "sync_bound_content_control") {
    throw "release_assets_manifest.json lost sync_bound_content_control repair strategy."
}
if ([string]$manifestContentControlContract.repair_hint -ne "Rerun Custom XML sync or explicitly fill the bound content control before release.") {
    throw "release_assets_manifest.json lost content-control repair_hint."
}
if ([string]$manifestContentControlContract.command_template -notmatch "sync-content-controls-from-custom-xml") {
    throw "release_assets_manifest.json lost sync-content-controls-from-custom-xml command template."
}
$manifestProjectTemplateReadiness = $manifest.project_template_delivery_readiness_contract
if ($null -eq $manifestProjectTemplateReadiness) {
    throw "release_assets_manifest.json lost project_template_delivery_readiness_contract."
}
if ([string]$manifestProjectTemplateReadiness.schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
    throw "release_assets_manifest.json lost project template delivery readiness schema."
}
if ([string]$manifestProjectTemplateReadiness.source_schema -ne "featherdoc.project_template_delivery_readiness_report.v1") {
    throw "release_assets_manifest.json lost project template delivery readiness source_schema."
}
if ([string]$manifestProjectTemplateReadiness.status -ne "ready") {
    throw "release_assets_manifest.json lost project template delivery readiness status."
}
if (-not [bool]$manifestProjectTemplateReadiness.release_ready) {
    throw "release_assets_manifest.json lost project template delivery readiness release_ready=true."
}
if ([string]$manifestProjectTemplateReadiness.latest_schema_approval_gate_status -ne "passed") {
    throw "release_assets_manifest.json lost project template latest schema approval gate status."
}
if (@($manifestProjectTemplateReadiness.schema_approval_status_summary).Count -ne 1) {
    throw "release_assets_manifest.json lost project template delivery readiness schema_approval_status_summary."
}
if ([string]$manifestProjectTemplateReadiness.schema_approval_status_summary[0].status -ne "approved" -or
    [int]$manifestProjectTemplateReadiness.schema_approval_status_summary[0].count -ne 4) {
    throw "release_assets_manifest.json lost project template delivery readiness schema_approval_status_summary values."
}
if (@($manifestProjectTemplateReadiness.business_document_type_summary).Count -ne 2 -or
    [string]$manifestProjectTemplateReadiness.business_document_type_summary[0].document_type -ne "invoice" -or
    [string]$manifestProjectTemplateReadiness.business_document_type_summary[1].document_type -ne "policy") {
    throw "release_assets_manifest.json lost project template delivery readiness business_document_type_summary."
}
if (@($manifestProjectTemplateReadiness.corpus_role_summary).Count -ne 2 -or
    [string]$manifestProjectTemplateReadiness.corpus_role_summary[0].corpus_role -ne "planned-business-template" -or
    [string]$manifestProjectTemplateReadiness.corpus_role_summary[1].corpus_role -ne "registered-business-template") {
    throw "release_assets_manifest.json lost project template delivery readiness corpus_role_summary."
}
if ([int]$manifestProjectTemplateReadiness.template_count -ne 4) {
    throw "release_assets_manifest.json lost project template delivery readiness template_count."
}
if ([int]$manifestProjectTemplateReadiness.ready_template_count -ne 4) {
    throw "release_assets_manifest.json lost project template delivery readiness ready_template_count."
}
if ([int]$manifestProjectTemplateReadiness.release_blocker_count -ne 0) {
    throw "release_assets_manifest.json lost project template delivery readiness release_blocker_count."
}
if ([int]$manifestProjectTemplateReadiness.warning_count -ne 0) {
    throw "release_assets_manifest.json lost project template delivery readiness warning_count."
}
$expectedProjectTemplateDeliveryReadinessDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $projectTemplateDeliveryReadinessSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$manifestProjectTemplateReadiness.source_report_display -ne $expectedProjectTemplateDeliveryReadinessDisplay) {
    throw "release_assets_manifest.json lost project template delivery readiness source_report_display."
}
if ([string]$manifestProjectTemplateReadiness.source_json_display -ne $expectedProjectTemplateDeliveryReadinessDisplay) {
    throw "release_assets_manifest.json lost project template delivery readiness source_json_display."
}
if ([bool]$manifestProjectTemplateReadiness.requires_reviewer_action) {
    throw "release_assets_manifest.json lost project template delivery readiness requires_reviewer_action=false."
}
if ([string]$manifestProjectTemplateReadiness.reviewer_action_summary -ne "none") {
    throw "release_assets_manifest.json lost project template delivery readiness reviewer_action_summary."
}
if ([string]$manifestProjectTemplateReadiness.reviewer_action_reason -ne "latest_review_state=approved; no reviewer action required") {
    throw "release_assets_manifest.json lost project template delivery readiness reviewer_action_reason."
}
if (@($manifestProjectTemplateReadiness.reviewer_actions).Count -ne 0) {
    throw "release_assets_manifest.json lost project template delivery readiness empty reviewer_actions."
}
$manifestProjectTemplateOnboarding = $manifest.project_template_onboarding_governance_contract
if ($null -eq $manifestProjectTemplateOnboarding) {
    throw "release_assets_manifest.json lost project_template_onboarding_governance_contract."
}
if ([string]$manifestProjectTemplateOnboarding.schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
    throw "release_assets_manifest.json lost project template onboarding governance schema."
}
if ([string]$manifestProjectTemplateOnboarding.source_schema -ne "featherdoc.project_template_onboarding_governance_report.v1") {
    throw "release_assets_manifest.json lost project template onboarding governance source_schema."
}
if ([string]$manifestProjectTemplateOnboarding.status -ne "ready") {
    throw "release_assets_manifest.json lost project template onboarding governance status."
}
if (-not [bool]$manifestProjectTemplateOnboarding.release_ready) {
    throw "release_assets_manifest.json lost project template onboarding governance release_ready=true."
}
if ([int]$manifestProjectTemplateOnboarding.source_file_count -ne 3) {
    throw "release_assets_manifest.json lost project template onboarding governance source_file_count."
}
if ([int]$manifestProjectTemplateOnboarding.entry_count -ne 4) {
    throw "release_assets_manifest.json lost project template onboarding governance entry_count."
}
if ([int]$manifestProjectTemplateOnboarding.approved_entry_count -ne 3) {
    throw "release_assets_manifest.json lost project template onboarding governance approved_entry_count."
}
if ([int]$manifestProjectTemplateOnboarding.not_required_entry_count -ne 1) {
    throw "release_assets_manifest.json lost project template onboarding governance not_required_entry_count."
}
if ([int]$manifestProjectTemplateOnboarding.release_blocker_count -ne 0) {
    throw "release_assets_manifest.json lost project template onboarding governance release_blocker_count."
}
if (@($manifestProjectTemplateOnboarding.schema_approval_status_summary).Count -ne 2) {
    throw "release_assets_manifest.json lost project template onboarding governance schema_approval_status_summary."
}
if (@($manifestProjectTemplateOnboarding.business_document_type_summary).Count -ne 2 -or
    [string]$manifestProjectTemplateOnboarding.business_document_type_summary[0].document_type -ne "invoice" -or
    [string]$manifestProjectTemplateOnboarding.business_document_type_summary[1].document_type -ne "policy") {
    throw "release_assets_manifest.json lost project template onboarding governance business_document_type_summary."
}
if (@($manifestProjectTemplateOnboarding.corpus_role_summary).Count -ne 2 -or
    [string]$manifestProjectTemplateOnboarding.corpus_role_summary[0].corpus_role -ne "planned-business-template" -or
    [string]$manifestProjectTemplateOnboarding.corpus_role_summary[1].corpus_role -ne "registered-business-template") {
    throw "release_assets_manifest.json lost project template onboarding governance corpus_role_summary."
}
$expectedProjectTemplateOnboardingGovernanceDisplay = Convert-TestEvidencePathToPublicDisplay `
    -Path $projectTemplateOnboardingGovernanceSummaryPath `
    -RepoRoot $resolvedRepoRoot
if ([string]$manifestProjectTemplateOnboarding.source_report_display -ne $expectedProjectTemplateOnboardingGovernanceDisplay) {
    throw "release_assets_manifest.json lost project template onboarding governance source_report_display."
}
if ([string]$manifestProjectTemplateOnboarding.source_json_display -ne $expectedProjectTemplateOnboardingGovernanceDisplay) {
    throw "release_assets_manifest.json lost project template onboarding governance source_json_display."
}
if ([bool]$manifestProjectTemplateOnboarding.requires_reviewer_action) {
    throw "release_assets_manifest.json lost project template onboarding governance requires_reviewer_action=false."
}
if ([string]$manifestProjectTemplateOnboarding.reviewer_action_summary -ne "none") {
    throw "release_assets_manifest.json lost project template onboarding governance reviewer_action_summary."
}
if ([string]$manifestProjectTemplateOnboarding.reviewer_action_reason -ne "latest_review_state=approved; no reviewer action required") {
    throw "release_assets_manifest.json lost project template onboarding governance reviewer_action_reason."
}
if (@($manifestProjectTemplateOnboarding.reviewer_actions).Count -ne 0) {
    throw "release_assets_manifest.json lost project template onboarding governance empty reviewer_actions."
}
