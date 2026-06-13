$checklistDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\project_template_release_readiness_checklist_zh.rst"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$templateSchemaDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\template_schema_mutation_zh.rst"
$releasePipelineDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$releasePolicyDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_policy_zh.rst"
$governanceRoutesDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\governance_routes_zh.rst"
$changelogDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "CHANGELOG.md"
$deliveryScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_project_template_delivery_readiness_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_project_template_delivery_readiness_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$contentControlScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_content_control_data_binding_governance_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_content_control_data_binding_governance_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseChecksScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\run_release_candidate_checks.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "run_release_candidate_checks_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseBlockerRollupScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_blocker_rollup_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_blocker_rollup_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseGovernancePipelineScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_pipeline_report.ps1"
$releaseGovernanceHandoffScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_governance_handoff_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$docxReadinessScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_docx_functional_smoke_readiness.ps1"
$releaseBlockerMetadataHelpersScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\release_blocker_metadata_helpers.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "release_blocker_metadata_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseVisualMetadataHelpersScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\release_visual_metadata_helpers.ps1"
$packageAssetsScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\package_release_assets.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "package_release_assets_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$materialSafetyScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\assert_release_material_safety.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "assert_release_material_safety_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$startHereScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_metadata_start_here.ps1"
$artifactGuideScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_artifact_guide.ps1"
$reviewerChecklistScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_reviewer_checklist.ps1"
$releaseBodyScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_body_zh.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "write_release_body_zh_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseBundleVersionTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_note_bundle_version_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "release_note_bundle_version_*.ps1" |
        Where-Object { $_.Name -ne "release_note_bundle_version_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseCandidateVisualTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_candidate_visual_verdict_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "release_candidate_visual_verdict_*.ps1" |
        Where-Object { $_.Name -ne "release_candidate_visual_verdict_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseBlockerRollupTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_blocker_rollup_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_blocker_rollup_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_blocker_rollup_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseGovernanceHandoffTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_handoff_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_governance_handoff_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_governance_handoff_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$cmakeLists = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test\cmake") -Filter "*.cmake" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
