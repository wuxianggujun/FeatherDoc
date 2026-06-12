param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$scriptRoot = Join-Path $resolvedRepoRoot "scripts"

$rollupScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_blocker_rollup_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_blocker_rollup_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$pipelineScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_pipeline_report.ps1"
$handoffScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "build_release_governance_handoff_report_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$metadataHelpersScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\release_blocker_metadata_helpers.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "release_blocker_metadata_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$releaseCandidateScript = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\run_release_candidate_checks.ps1"
    Get-ChildItem -LiteralPath $scriptRoot -Filter "run_release_candidate_checks_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$reviewerChecklistScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_reviewer_checklist.ps1"
$releaseNoteBundleScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\write_release_note_bundle.ps1"
$governanceRoutesDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\governance_routes_zh.rst"
$cmakeLists = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test\cmake") -Filter "*.cmake" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

$rollupTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_blocker_rollup_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_blocker_rollup_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_blocker_rollup_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$pipelineTest = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_pipeline_report_test.ps1"
$handoffTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_handoff_report_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "build_release_governance_handoff_report_*.ps1" |
        Where-Object { $_.Name -ne "build_release_governance_handoff_report_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$candidateTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_candidate_blocker_rollup_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "release_candidate_blocker_rollup_test_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
$bundleTest = @(
    Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\release_note_bundle_version_test.ps1"
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "test") -Filter "release_note_bundle_version_*.ps1" |
        Where-Object { $_.Name -ne "release_note_bundle_version_test.ps1" } |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"

$detailFields = @(
    "source_schema",
    "source_report_display",
    "source_json_display",
    "candidate_type"
)

foreach ($field in $detailFields) {
    foreach ($entry in @(
        [ordered]@{ name = "release blocker rollup script"; text = $rollupScript },
        [ordered]@{ name = "release governance pipeline script"; text = $pipelineScript },
        [ordered]@{ name = "release governance handoff script"; text = $handoffScript },
        [ordered]@{ name = "release metadata helpers"; text = $metadataHelpersScript }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $field `
            -Message "$($entry.name) should keep release governance detail field '$field'."
    }
}

$testCommonDetailFields = @(
    "source_schema",
    "source_report_display",
    "source_json_display"
)

foreach ($field in $testCommonDetailFields) {
    foreach ($entry in @(
        [ordered]@{ name = "release blocker rollup test"; text = $rollupTest },
        [ordered]@{ name = "release governance pipeline test"; text = $pipelineTest },
        [ordered]@{ name = "release governance handoff test"; text = $handoffTest },
        [ordered]@{ name = "release candidate blocker rollup test"; text = $candidateTest },
        [ordered]@{ name = "release note bundle test"; text = $bundleTest }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $field `
            -Message "$($entry.name) should keep release governance detail field '$field'."
    }
}

foreach ($entry in @(
    [ordered]@{ name = "release blocker rollup test"; text = $rollupTest },
    [ordered]@{ name = "release governance pipeline test"; text = $pipelineTest },
    [ordered]@{ name = "release governance handoff test"; text = $handoffTest }
)) {
    Assert-ContainsText -Text $entry.text -ExpectedText "candidate_type" `
        -Message "$($entry.name) should keep release governance candidate routing coverage."
}

foreach ($entry in @(
    [ordered]@{ name = "release governance pipeline script"; text = $pipelineScript },
    [ordered]@{ name = "release governance handoff script"; text = $handoffScript },
    [ordered]@{ name = "release metadata helpers"; text = $metadataHelpersScript }
)) {
    Assert-ContainsText -Text $entry.text -ExpectedText "open_command" `
        -Message "$($entry.name) should preserve action open command fields."
}

Assert-ContainsText -Text $reviewerChecklistScript `
    -ExpectedText "For PDF/CJK-facing releases, manually verify a generated Chinese PDF can be copied and searched in at least one common reader" `
    -Message "Reviewer checklist should preserve the PDF/CJK manual copy-search release verification."

foreach ($field in @("audit_command", "review_command")) {
    foreach ($entry in @(
        [ordered]@{ name = "release governance pipeline script"; text = $pipelineScript },
        [ordered]@{ name = "release metadata helpers"; text = $metadataHelpersScript }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $field `
            -Message "$($entry.name) should preserve action command field '$field'."
    }
}

foreach ($field in @("release_blockers", "action_items", "warnings")) {
    foreach ($entry in @(
        [ordered]@{ name = "release blocker rollup script"; text = $rollupScript },
        [ordered]@{ name = "release governance pipeline script"; text = $pipelineScript },
        [ordered]@{ name = "release governance handoff script"; text = $handoffScript },
        [ordered]@{ name = "release candidate script"; text = $releaseCandidateScript }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $field `
            -Message "$($entry.name) should preserve release governance collection '$field'."
    }
}

foreach ($marker in @(
    "featherdoc.content_control_data_binding_governance_report.v1",
    "featherdoc.schema_patch_confidence_calibration_report.v1",
    "content-control-data-binding\inspect-content-controls.json",
    "schema-patch-confidence-calibration\summary.json",
    "write_schema_patch_confidence_calibration_report.ps1"
)) {
    foreach ($entry in @(
        [ordered]@{ name = "release blocker rollup test"; text = $rollupTest },
        [ordered]@{ name = "release governance pipeline test"; text = $pipelineTest },
        [ordered]@{ name = "release governance handoff test"; text = $handoffTest },
        [ordered]@{ name = "release candidate blocker rollup test"; text = $candidateTest },
        [ordered]@{ name = "release note bundle test"; text = $bundleTest }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $marker `
            -Message "$($entry.name) should lock governance detail marker '$marker'."
    }
}

foreach ($marker in @(
    "featherdoc.project_template_onboarding_governance_report.v1",
    "project-template-onboarding-governance\summary.json"
)) {
    foreach ($entry in @(
        [ordered]@{ name = "release blocker rollup test"; text = $rollupTest },
        [ordered]@{ name = "release governance pipeline test"; text = $pipelineTest },
        [ordered]@{ name = "release candidate blocker rollup test"; text = $candidateTest },
        [ordered]@{ name = "release note bundle test"; text = $bundleTest }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $marker `
            -Message "$($entry.name) should lock onboarding governance detail marker '$marker'."
    }
}

foreach ($marker in @(
    "New-StageBlockerItems",
    "New-StageActionItems",
    "New-StageWarningItems",
    "New-LocalGovernanceClosureSummary",
    "featherdoc.release_governance_local_closure.v1",
    "local_governance_closure",
    "Add-StageGovernanceMarkdown",
    "release_governance_handoff",
    "release_blocker_rollup"
)) {
    Assert-ContainsText -Text $pipelineScript -ExpectedText $marker `
        -Message "Release governance pipeline should keep stage detail marker '$marker'."
}

foreach ($marker in @(
    "build_release_governance_pipeline_report.ps1",
    "local_governance_closure",
    "local_governance_closure.closed",
    "docx_functional_smoke_readiness",
    "release_governance_handoff",
    "release_blocker_rollup"
)) {
    Assert-ContainsText -Text $governanceRoutesDoc -ExpectedText $marker `
        -Message "Governance routes docs should keep local release governance closure marker '$marker'."
}

foreach ($marker in @(
    "Add-NormalizedBlockers",
    "Add-NormalizedActions",
    "Add-NormalizedWarnings",
    "release_blocker_rollup = [ordered]@{"
)) {
    Assert-ContainsText -Text $handoffScript -ExpectedText $marker `
        -Message "Release governance handoff should keep normalized detail marker '$marker'."
}

foreach ($marker in @(
    "origin_source_report_display",
    'Get-FirstJsonString -Object $blocker -Names @("source_schema")',
    'Get-FirstJsonString -Object $item -Names @("open_command", "command")',
    'Get-FirstJsonString -Object $warning -Names @("source_schema")'
)) {
    Assert-ContainsText -Text $rollupScript -ExpectedText $marker `
        -Message "Release blocker rollup should keep detail passthrough marker '$marker'."
}

foreach ($marker in @(
    "blocker_source_schema_summary",
    "action_item_source_schema_summary",
    "informational_action_item_source_schema_summary",
    "warning_source_schema_summary"
)) {
    foreach ($entry in @(
        [ordered]@{ name = "release blocker rollup script"; text = $rollupScript },
        [ordered]@{ name = "release blocker rollup test"; text = $rollupTest }
    )) {
        Assert-ContainsText -Text $entry.text -ExpectedText $marker `
            -Message "$($entry.name) should keep source schema summary marker '$marker'."
    }
}

foreach ($marker in @(
    "Get-ReleaseGovernanceChecklistSections",
    "Assert-ReleaseGovernanceReviewerMetadataQuality",
    "Assert-ReleaseGovernanceReviewerMetadataCollectionQuality",
    "Assert-ReleaseGovernanceDeclaredCountConsistency",
    "Assert-ReleaseGovernanceSourceSchemaSummaryConsistency",
    '-SummaryProperty "blocker_source_schema_summary"',
    '-SummaryProperty "action_item_source_schema_summary"',
    '-SummaryProperty "informational_action_item_source_schema_summary"',
    '-SummaryProperty "warning_source_schema_summary"',
    "Release governance handoff nested rollup",
    "Get-NormalizedReleaseGovernanceBlockers",
    "Get-NormalizedReleaseGovernanceActionItems",
    "source_failure_count",
    "readiness_action_evidence_count"
)) {
    Assert-ContainsText -Text $metadataHelpersScript -ExpectedText $marker `
        -Message "Release metadata helpers should keep governance helper marker '$marker'."
}

foreach ($marker in @(
    "source_schema=featherdoc.project_template_onboarding_governance_report.v1",
    "source_schema=featherdoc.schema_patch_confidence_calibration_report.v1",
    "open_command: pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1",
    "source_json_display: .\output\schema-patch-confidence-calibration\summary.json",
    "summary.release-rollup-missing-source-json-display.json",
    "summary.release-handoff-missing-open-command.json"
)) {
    Assert-ContainsText -Text $bundleTest -ExpectedText $marker `
        -Message "Release note bundle test should keep reviewer-facing governance detail '$marker'."
}

foreach ($marker in @(
    "Get-ReleaseGovernanceBlockerChecklistItems",
    "Get-ReleaseGovernanceActionItemChecklistItems",
    "Get-ReleaseGovernanceWarningChecklistItems"
)) {
    Assert-ContainsText -Text $reviewerChecklistScript -ExpectedText $marker `
        -Message "Reviewer checklist should continue to consume governance checklist helper '$marker'."
}
Assert-ContainsText -Text $releaseCandidateScript -ExpectedText "release_blocker_rollup.release_blockers" `
    -Message "Release candidate summary should keep rollup blocker details."
Assert-ContainsText -Text $releaseCandidateScript -ExpectedText "release_governance_handoff.release_blockers" `
    -Message "Release candidate summary should keep handoff blocker details."
Assert-ContainsText -Text $releaseNoteBundleScript -ExpectedText "Assert-ReleaseGovernanceReviewerMetadataQuality" `
    -Message "Release note bundle should validate reviewer-facing governance detail before writing outputs."

foreach ($marker in @(
    "release_governance_warning_contract",
    "release_governance_warning_helper_contract",
    "release_governance_metrics_contract",
    "release_governance_detail_rollup_static_contract",
    "build_release_governance_pipeline_report_markdown_counts",
    "release_governance_warning_contract_test.ps1",
    "release_governance_warning_helper_contract_test.ps1",
    "release_governance_metrics_contract_test.ps1",
    "release_governance_detail_rollup_static_contract_test.ps1",
    "markdown_counts",
    "TIMEOUT 60",
    'LABELS "release;governance;smoke"'
)) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep release governance lightweight contract marker '$marker'."
}

Write-Host "Release governance detail rollup static contract passed."
