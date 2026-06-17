$repoRoot = Resolve-RepoRoot
$msvcBootstrapRequired = -not ($SkipConfigure -and $SkipBuild -and $SkipTests -and $SkipInstallSmoke -and $SkipVisualGate)
$msvcBootstrap = if ($msvcBootstrapRequired) {
    Get-MsvcBootstrap
} else {
    [ordered]@{
        mode = "not_required"
        vcvars_path = ""
    }
}

$resolvedBuildDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedInstallDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $InstallDir
$resolvedConsumerBuildDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $ConsumerBuildDir
$resolvedGateOutputDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $GateOutputDir
$resolvedTaskOutputRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $TaskOutputRoot
$resolvedSummaryOutputDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryOutputDir
$projectVersion = Get-ProjectVersion -RepoRoot $repoRoot
$reportDir = Join-Path $resolvedSummaryOutputDir "report"
$summaryPath = Join-Path $reportDir "summary.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
$releaseBodyZhCnPath = Join-Path $reportDir "release_body.zh-CN.md"
$releaseSummaryZhCnPath = Join-Path $reportDir "release_summary.zh-CN.md"
$artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$schemaApprovalHistoryJsonPath = Join-Path $reportDir "project_template_schema_approval_history.json"
$schemaApprovalHistoryMarkdownPath = Join-Path $reportDir "project_template_schema_approval_history.md"
$startHerePath = Join-Path $resolvedSummaryOutputDir "START_HERE.md"
$releaseAssetsManifestSignoffPath = if ([string]::IsNullOrWhiteSpace($projectVersion)) {
    "output\release-assets\v<version>\release_assets_manifest.json"
} else {
    Join-Path (Join-Path "output\release-assets" ("v{0}" -f $projectVersion)) "release_assets_manifest.json"
}
$releaseNoteBundleEntrypoints = @(
    [ordered]@{
        id = "start_here"
        path = $startHerePath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $startHerePath
        location = "summary_root"
        required = $true
    },
    [ordered]@{
        id = "artifact_guide"
        path = $artifactGuidePath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $artifactGuidePath
        location = "report"
        required = $true
    },
    [ordered]@{
        id = "reviewer_checklist"
        path = $reviewerChecklistPath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $reviewerChecklistPath
        location = "report"
        required = $true
    },
    [ordered]@{
        id = "release_handoff"
        path = $releaseHandoffPath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseHandoffPath
        location = "report"
        required = $true
    },
    [ordered]@{
        id = "release_body_zh_cn"
        path = $releaseBodyZhCnPath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseBodyZhCnPath
        location = "report"
        required = $true
    },
    [ordered]@{
        id = "release_summary_zh_cn"
        path = $releaseSummaryZhCnPath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseSummaryZhCnPath
        location = "report"
        required = $true
    }
)
$manifestSignoffEntrypoints = @(
    [ordered]@{
        id = "start_here"
        path = $startHerePath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $startHerePath
        required = $true
    },
    [ordered]@{
        id = "artifact_guide"
        path = $artifactGuidePath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $artifactGuidePath
        required = $true
    },
    [ordered]@{
        id = "reviewer_checklist"
        path = $reviewerChecklistPath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $reviewerChecklistPath
        required = $true
    }
)
$releaseManifestSignoffEntrypoints = if ($ReleaseEvidenceScope -eq "pdf-only") {
    $null
} else {
    [ordered]@{
        status = "declared"
        release_assets_manifest = $releaseAssetsManifestSignoffPath
        required_entrypoint_count = @($manifestSignoffEntrypoints).Count
        entrypoints = @($manifestSignoffEntrypoints)
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
            "requires_reviewer_action",
            "reviewer_action_summary",
            "reviewer_action_reason",
            "reviewer_actions",
            "source_report_display",
            "source_json_display"
        )
        checklist_marker = "reviewer_manifest_scoped_project_template_trace"
    }
}
$releaseProjectTemplateReadinessChecklistEntrypoints = if ($ReleaseEvidenceScope -eq "pdf-only") {
    $null
} else {
    [ordered]@{
        status = "declared"
        checklist_label = "Project template release readiness checklist"
        checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
        required_entrypoint_count = @($manifestSignoffEntrypoints).Count
        entrypoints = @($manifestSignoffEntrypoints)
        checklist_marker = "release_entry_project_template_readiness_checklist_trace"
    }
}
$releaseEntryProjectTemplateReadinessChecklistMaterialSafetyAudit = if ($ReleaseEvidenceScope -eq "pdf-only") {
    $null
} else {
    [ordered]@{
        status = "passed"
        audit_script = ".\scripts\assert_release_material_safety.ps1"
        audited_entrypoint_count = 3
        audited_entrypoints = @(
            "start_here",
            "artifact_guide",
            "reviewer_checklist"
        )
        compact_evidence_label = "Project-template readiness checklist handoff evidence"
        compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
        compact_evidence_source_schema = "featherdoc.release_candidate_summary"
        checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
        checklist_marker = "release_entry_project_template_readiness_checklist_trace"
        material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    }
}
$resolvedPdfVisualGateSummaryJson = ""
if (-not [string]::IsNullOrWhiteSpace($PdfVisualGateSummaryJson)) {
    $resolvedPdfVisualGateSummaryJson = Resolve-FullPath -RepoRoot $repoRoot -InputPath $PdfVisualGateSummaryJson
} else {
    $autoPdfVisualGateSummaryJson = Join-Path $repoRoot "output\pdf-visual-release-gate-current\report\summary.json"
    if (Test-Path -LiteralPath $autoPdfVisualGateSummaryJson) {
        $resolvedPdfVisualGateSummaryJson = $autoPdfVisualGateSummaryJson
    }
}
$pdfVisualGateSummaryInfo = Get-PdfVisualGateSummaryInfo -SummaryJson $resolvedPdfVisualGateSummaryJson
$resolvedPdfVisualGateAttemptSummaryJson = ""
if (-not [string]::IsNullOrWhiteSpace($PdfVisualGateAttemptSummaryJson)) {
    $resolvedPdfVisualGateAttemptSummaryJson = Resolve-FullPath -RepoRoot $repoRoot -InputPath $PdfVisualGateAttemptSummaryJson
} else {
    $autoPdfVisualGateAttemptSummaryJson = Join-Path $repoRoot "output\pdf-visual-release-gate-current\report\attempt-summary.json"
    if (Test-Path -LiteralPath $autoPdfVisualGateAttemptSummaryJson) {
        $resolvedPdfVisualGateAttemptSummaryJson = $autoPdfVisualGateAttemptSummaryJson
    }
}
$pdfVisualGateAttemptSummaryInfo = Get-PdfVisualGateAttemptSummaryInfo -SummaryJson $resolvedPdfVisualGateAttemptSummaryJson
$resolvedPdfVisualSegmentedGateSummaryJson = ""
if (-not [string]::IsNullOrWhiteSpace($PdfVisualSegmentedGateSummaryJson)) {
    $resolvedPdfVisualSegmentedGateSummaryJson = Resolve-FullPath -RepoRoot $repoRoot -InputPath $PdfVisualSegmentedGateSummaryJson
} else {
    $autoPdfVisualSegmentedGateSummaryJson = Join-Path $repoRoot "output\pdf-visual-release-gate-current\report\segmented-summary.json"
    if (Test-Path -LiteralPath $autoPdfVisualSegmentedGateSummaryJson) {
        $resolvedPdfVisualSegmentedGateSummaryJson = $autoPdfVisualSegmentedGateSummaryJson
    }
}
$pdfVisualSegmentedGateSummaryInfo = Get-PdfVisualSegmentedGateSummaryInfo -SummaryJson $resolvedPdfVisualSegmentedGateSummaryJson
$resolvedPdfBoundedCtestSummaryJson = @(Resolve-PdfBoundedCtestSummaryPaths `
        -RepoRoot $repoRoot `
        -SummaryJson $PdfBoundedCtestSummaryJson)
$pdfBoundedCtestSummaryInfo = Get-PdfBoundedCtestSummaryInfo `
    -SummaryJson $resolvedPdfBoundedCtestSummaryJson `
    -RepoRoot $repoRoot
$resolvedPdfReleaseReadinessSummaryJson = ""
if (-not [string]::IsNullOrWhiteSpace($PdfReleaseReadinessSummaryJson)) {
    $resolvedPdfReleaseReadinessSummaryJson = Resolve-FullPath -RepoRoot $repoRoot -InputPath $PdfReleaseReadinessSummaryJson
} else {
    $autoPdfReleaseReadinessSummaryJson = Join-Path $repoRoot "output\pdf-release-readiness-current\summary.json"
    if (Test-Path -LiteralPath $autoPdfReleaseReadinessSummaryJson) {
        $resolvedPdfReleaseReadinessSummaryJson = $autoPdfReleaseReadinessSummaryJson
    }
}
$pdfFullCtestReadinessInfo = Get-PdfFullCtestReadinessSummaryInfo `
    -SummaryJson $resolvedPdfReleaseReadinessSummaryJson `
    -RepoRoot $repoRoot
$installSmokeScript = Join-Path $repoRoot "scripts\run_install_find_package_smoke.ps1"
$templateSchemaCheckScript = Join-Path $repoRoot "scripts\check_template_schema_baseline.ps1"
$templateSchemaManifestScript = Join-Path $repoRoot "scripts\check_template_schema_manifest.ps1"
$projectTemplateSmokeScript = Join-Path $repoRoot "scripts\run_project_template_smoke.ps1"
$projectTemplateSchemaApprovalHistoryScript = Join-Path $repoRoot "scripts\write_project_template_schema_approval_history.ps1"
$projectTemplateSmokeDiscoverScript = Join-Path $repoRoot "scripts\discover_project_template_smoke_candidates.ps1"
$releaseBlockerRollupScript = Join-Path $repoRoot "scripts\build_release_blocker_rollup_report.ps1"
$releaseGovernanceHandoffScript = Join-Path $repoRoot "scripts\build_release_governance_handoff_report.ps1"
$projectTemplateWorkflowDashboardScript = Join-Path $repoRoot "scripts\build_project_template_workflow_dashboard.ps1"
$visualGateScript = Join-Path $repoRoot "scripts\run_word_visual_release_gate.ps1"
$releaseNoteBundleScript = Join-Path $repoRoot "scripts\write_release_note_bundle.ps1"
$releaseMaterialAuditScript = Join-Path $repoRoot "scripts\assert_release_material_safety.ps1"

$resolvedTemplateSchemaInputDocx = if ([string]::IsNullOrWhiteSpace($TemplateSchemaInputDocx)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaInputDocx
}
$resolvedTemplateSchemaBaseline = if ([string]::IsNullOrWhiteSpace($TemplateSchemaBaseline)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaBaseline
}
$resolvedTemplateSchemaGeneratedOutput = if ([string]::IsNullOrWhiteSpace($TemplateSchemaGeneratedOutput)) {
    Join-Path $reportDir "generated_template_schema.json"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaGeneratedOutput
}
$resolvedTemplateSchemaManifestPath = if ([string]::IsNullOrWhiteSpace($TemplateSchemaManifestPath)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaManifestPath
}
$templateSchemaManifestRequested = -not [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaManifestPath)
$resolvedTemplateSchemaManifestOutputDir = if ($templateSchemaManifestRequested) {
    if ([string]::IsNullOrWhiteSpace($TemplateSchemaManifestOutputDir)) {
        Join-Path $reportDir "template-schema-manifest-checks"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaManifestOutputDir
    }
} else {
    ""
}
$resolvedTemplateSchemaManifestSummaryPath = if ($templateSchemaManifestRequested) {
    Join-Path $resolvedTemplateSchemaManifestOutputDir "summary.json"
} else {
    ""
}
$resolvedProjectTemplateSmokeManifestPath = if ([string]::IsNullOrWhiteSpace($ProjectTemplateSmokeManifestPath)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ProjectTemplateSmokeManifestPath
}
$projectTemplateSmokeRequested = -not [string]::IsNullOrWhiteSpace($resolvedProjectTemplateSmokeManifestPath)
$resolvedProjectTemplateSmokeOutputDir = if ($projectTemplateSmokeRequested) {
    if ([string]::IsNullOrWhiteSpace($ProjectTemplateSmokeOutputDir)) {
        Join-Path $reportDir "project-template-smoke"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $ProjectTemplateSmokeOutputDir
    }
} else {
    ""
}
$resolvedProjectTemplateSmokeSummaryPath = if ($projectTemplateSmokeRequested) {
    Join-Path $resolvedProjectTemplateSmokeOutputDir "summary.json"
} else {
    ""
}
$resolvedProjectTemplateSmokeCandidateDiscoveryPath = if ($projectTemplateSmokeRequested) {
    Join-Path $resolvedProjectTemplateSmokeOutputDir "candidate_discovery.json"
} else {
    ""
}
$templateSchemaRequested = -not [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaInputDocx) -or
    -not [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaBaseline)
$expandedReleaseBlockerRollupInputJson = @(Expand-TemplateSchemaArgumentList -Values $ReleaseBlockerRollupInputJson)
$expandedReleaseBlockerRollupInputRoot = @(Expand-TemplateSchemaArgumentList -Values $ReleaseBlockerRollupInputRoot)
$resolvedReleaseBlockerRollupAutoDiscoverRoot = Resolve-FullPath -RepoRoot $repoRoot `
    -InputPath $ReleaseBlockerRollupAutoDiscoverRoot
$autoDiscoveredReleaseBlockerRollupInputJson = if ($ReleaseBlockerRollupAutoDiscover) {
    @(Get-ReleaseBlockerRollupAutoDiscoveredInputJson `
            -RepoRoot $repoRoot `
            -InputRoot $resolvedReleaseBlockerRollupAutoDiscoverRoot)
} else {
    @()
}
$resolvedReleaseBlockerRollupInputJson = @(
    foreach ($path in @($expandedReleaseBlockerRollupInputJson + $autoDiscoveredReleaseBlockerRollupInputJson)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            Resolve-FullPath -RepoRoot $repoRoot -InputPath $path
        }
    }
)
$resolvedReleaseBlockerRollupInputJson = @(Select-UniqueReleaseBlockerRollupPathList `
        -Paths $resolvedReleaseBlockerRollupInputJson)
$resolvedReleaseBlockerRollupInputRoot = @(
    foreach ($path in @($expandedReleaseBlockerRollupInputRoot)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            Resolve-FullPath -RepoRoot $repoRoot -InputPath $path
        }
    }
)
$resolvedReleaseBlockerRollupInputRoot = @(Select-UniqueReleaseBlockerRollupPathList `
        -Paths $resolvedReleaseBlockerRollupInputRoot)
$releaseBlockerRollupInputCount = @(Get-ReleaseBlockerRollupInputList `
        -InputJson $resolvedReleaseBlockerRollupInputJson `
        -InputRoot $resolvedReleaseBlockerRollupInputRoot).Count
$releaseBlockerRollupRequested = $releaseBlockerRollupInputCount -gt 0
$resolvedReleaseBlockerRollupOutputDir = if ($releaseBlockerRollupRequested) {
    if ([string]::IsNullOrWhiteSpace($ReleaseBlockerRollupOutputDir)) {
        Join-Path $reportDir "release-blocker-rollup"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReleaseBlockerRollupOutputDir
    }
} else {
    ""
}
$releaseBlockerRollupSummaryPath = if ($releaseBlockerRollupRequested) {
    Join-Path $resolvedReleaseBlockerRollupOutputDir "summary.json"
} else {
    ""
}
$releaseBlockerRollupMarkdownPath = if ($releaseBlockerRollupRequested) {
    Join-Path $resolvedReleaseBlockerRollupOutputDir "release_blocker_rollup.md"
} else {
    ""
}
$expandedReleaseGovernanceHandoffInputJson = @(Expand-TemplateSchemaArgumentList -Values $ReleaseGovernanceHandoffInputJson)
$resolvedReleaseGovernanceHandoffInputJson = @(
    foreach ($path in @($expandedReleaseGovernanceHandoffInputJson)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            Resolve-FullPath -RepoRoot $repoRoot -InputPath $path
        }
    }
)
$resolvedReleaseGovernanceHandoffInputJson = @(Select-UniqueReleaseBlockerRollupPathList `
        -Paths $resolvedReleaseGovernanceHandoffInputJson)
$releaseGovernanceHandoffInputRootIsDefault = [string]::Equals(
    $ReleaseGovernanceHandoffInputRoot,
    "output",
    [System.StringComparison]::OrdinalIgnoreCase)
$resolvedReleaseGovernanceHandoffInputRoot = Resolve-FullPath -RepoRoot $repoRoot `
    -InputPath $ReleaseGovernanceHandoffInputRoot
if ($releaseGovernanceHandoffInputRootIsDefault) {
    $pipelineGovernanceInputRoot = Resolve-FullPath -RepoRoot $repoRoot `
        -InputPath "output\release-governance-pipeline-current\governance"
    if (Test-Path -LiteralPath $pipelineGovernanceInputRoot -PathType Container) {
        $resolvedReleaseGovernanceHandoffInputRoot = $pipelineGovernanceInputRoot
    }
}
$defaultProjectTemplateOnboardingGovernanceSummary = Resolve-FullPath -RepoRoot $repoRoot `
    -InputPath "output\project-template-onboarding-governance\summary.json"
if ($releaseGovernanceHandoffInputRootIsDefault -and
    (Test-Path -LiteralPath $defaultProjectTemplateOnboardingGovernanceSummary -PathType Leaf)) {
    $resolvedReleaseGovernanceHandoffInputJson = @(Select-UniqueReleaseBlockerRollupPathList `
            -Paths (@($resolvedReleaseGovernanceHandoffInputJson) + @($defaultProjectTemplateOnboardingGovernanceSummary)))
}
$releaseGovernanceHandoffRequested = [bool]$ReleaseGovernanceHandoff
$resolvedReleaseGovernanceHandoffOutputDir = if ($releaseGovernanceHandoffRequested) {
    if ([string]::IsNullOrWhiteSpace($ReleaseGovernanceHandoffOutputDir)) {
        Join-Path $reportDir "release-governance-handoff"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReleaseGovernanceHandoffOutputDir
    }
} else {
    ""
}
$releaseGovernanceHandoffSummaryPath = if ($releaseGovernanceHandoffRequested) {
    Join-Path $resolvedReleaseGovernanceHandoffOutputDir "summary.json"
} else {
    ""
}
$releaseGovernanceHandoffMarkdownPath = if ($releaseGovernanceHandoffRequested) {
    Join-Path $resolvedReleaseGovernanceHandoffOutputDir "release_governance_handoff.md"
} else {
    ""
}
$projectTemplateWorkflowDashboardRequested = $ReleaseEvidenceScope -ne "pdf-only"
$resolvedProjectTemplateWorkflowDashboardOutputDir = if ($projectTemplateWorkflowDashboardRequested) {
    Join-Path $reportDir "project-template-workflow-dashboard"
} else {
    ""
}
$projectTemplateWorkflowDashboardSummaryPath = if ($projectTemplateWorkflowDashboardRequested) {
    Join-Path $resolvedProjectTemplateWorkflowDashboardOutputDir "project_template_workflow_dashboard.json"
} else {
    ""
}
$projectTemplateWorkflowDashboardMarkdownPath = if ($projectTemplateWorkflowDashboardRequested) {
    Join-Path $resolvedProjectTemplateWorkflowDashboardOutputDir "project_template_workflow_dashboard.md"
} else {
    ""
}

New-Item -ItemType Directory -Path $resolvedSummaryOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

$summary = [ordered]@{
    schema = "featherdoc.release_candidate_summary"
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    install_dir = $resolvedInstallDir
    consumer_build_dir = $resolvedConsumerBuildDir
    gate_output_dir = $resolvedGateOutputDir
    task_output_root = $resolvedTaskOutputRoot
    config = $Config
    generator = $Generator
    ctest_timeout_seconds = $CtestTimeoutSeconds
    review_mode = if ($SkipReviewTasks) { "" } else { $ReviewMode }
    msvc_bootstrap_mode = $msvcBootstrap.mode
    release_version = $projectVersion
    visual_verdict = if ($SkipVisualGate) { "visual_gate_skipped" } else { "pending_manual_review" }
    execution_status = "running"
    failed_step = ""
    error = ""
    release_blockers = @()
    release_blocker_count = 0
    warning_count = 0
    warnings = @()
    governance_metric_count = 0
    governance_metrics = @()
    release_handoff = $releaseHandoffPath
    release_body_zh_cn = $releaseBodyZhCnPath
    release_summary_zh_cn = $releaseSummaryZhCnPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    start_here = $startHerePath
    release_note_bundle = [ordered]@{
        status = "declared"
        output_root = $resolvedSummaryOutputDir
        report_dir = $reportDir
        entrypoint_count = @($releaseNoteBundleEntrypoints).Count
        required_entrypoint_count = @($releaseNoteBundleEntrypoints).Count
        entrypoint_ids = @($releaseNoteBundleEntrypoints | ForEach-Object { [string]$_["id"] })
        entrypoints = @($releaseNoteBundleEntrypoints)
    }
    release_evidence_scope = $ReleaseEvidenceScope
    manifest_signoff_entrypoints = $releaseManifestSignoffEntrypoints
    project_template_readiness_checklist_entrypoints = $releaseProjectTemplateReadinessChecklistEntrypoints
    release_entry_project_template_readiness_checklist_material_safety_audit = $releaseEntryProjectTemplateReadinessChecklistMaterialSafetyAudit
    project_template_workflow_dashboard = $projectTemplateWorkflowDashboardSummaryPath
    project_template_workflow_dashboard_report = [ordered]@{
        requested = $projectTemplateWorkflowDashboardRequested
        status = if ($projectTemplateWorkflowDashboardRequested) { "pending" } else { "not_requested" }
        output_dir = $resolvedProjectTemplateWorkflowDashboardOutputDir
        summary_json = $projectTemplateWorkflowDashboardSummaryPath
        report_markdown = $projectTemplateWorkflowDashboardMarkdownPath
        release_ready = $false
        release_blocker_count = 0
        warning_count = 0
        source_report_count = 0
        next_action = $null
        next_action_summary = @()
        next_action_group_count = 0
        error = ""
    }
    pdf_visual_gate_summary_json = $resolvedPdfVisualGateSummaryJson
    pdf_visual_gate = $pdfVisualGateSummaryInfo
    pdf_visual_gate_attempt_summary_json = $resolvedPdfVisualGateAttemptSummaryJson
    pdf_visual_gate_attempt = $pdfVisualGateAttemptSummaryInfo
    pdf_visual_segmented_gate_summary_json = $resolvedPdfVisualSegmentedGateSummaryJson
    pdf_visual_segmented_gate = $pdfVisualSegmentedGateSummaryInfo
    pdf_bounded_ctest = $pdfBoundedCtestSummaryInfo
    pdf_release_readiness_summary_json = $resolvedPdfReleaseReadinessSummaryJson
    pdf_full_ctest_readiness = $pdfFullCtestReadinessInfo
    release_blocker_rollup = [ordered]@{
        requested = $releaseBlockerRollupRequested
        status = if ($releaseBlockerRollupRequested) { "pending" } else { "not_requested" }
        auto_discover = [bool]$ReleaseBlockerRollupAutoDiscover
        auto_discover_root = $resolvedReleaseBlockerRollupAutoDiscoverRoot
        auto_discovered_input_json = @($autoDiscoveredReleaseBlockerRollupInputJson)
        input_json = @($resolvedReleaseBlockerRollupInputJson)
        input_root = @($resolvedReleaseBlockerRollupInputRoot)
        output_dir = $resolvedReleaseBlockerRollupOutputDir
        summary_json = $releaseBlockerRollupSummaryPath
        report_markdown = $releaseBlockerRollupMarkdownPath
        fail_on_blocker = [bool]$ReleaseBlockerRollupFailOnBlocker
        fail_on_warning = [bool]$ReleaseBlockerRollupFailOnWarning
        source_report_count = 0
        source_failure_count = 0
        release_blocker_count = 0
        release_blockers = @()
        blocker_source_schema_summary = @()
        action_item_count = 0
        action_items = @()
        action_item_source_schema_summary = @()
        informational_action_item_source_schema_summary = @()
        warning_count = 0
        warnings = @()
        warning_source_schema_summary = @()
        governance_metric_count = 0
        governance_metrics = @()
        manifest_signoff_entrypoints_source_report_count = 0
        manifest_signoff_entrypoints_source_reports = @()
        error = ""
    }
    release_governance_handoff = [ordered]@{
        requested = $releaseGovernanceHandoffRequested
        status = if ($releaseGovernanceHandoffRequested) { "pending" } else { "not_requested" }
        input_root = $resolvedReleaseGovernanceHandoffInputRoot
        input_json = @($resolvedReleaseGovernanceHandoffInputJson)
        output_dir = $resolvedReleaseGovernanceHandoffOutputDir
        summary_json = $releaseGovernanceHandoffSummaryPath
        report_markdown = $releaseGovernanceHandoffMarkdownPath
        include_rollup = [bool]$ReleaseGovernanceHandoffIncludeRollup
        fail_on_missing = [bool]$ReleaseGovernanceHandoffFailOnMissing
        fail_on_blocker = [bool]$ReleaseGovernanceHandoffFailOnBlocker
        fail_on_warning = [bool]$ReleaseGovernanceHandoffFailOnWarning
        expected_report_profile = $ReleaseGovernanceHandoffExpectedReportProfile
        expected_report_count = 0
        loaded_report_count = 0
        missing_report_count = 0
        failed_report_count = 0
        report_count = 0
        reports = @()
        governance_metric_count = 0
        governance_metrics = @()
        project_template_delivery_readiness_contract = $null
        project_template_onboarding_governance_contract = $null
        release_blocker_rollup = $null
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        manifest_signoff_entrypoints_source_report_count = 0
        manifest_signoff_entrypoints_source_reports = @()
        project_template_readiness_checklist_entrypoints_source_report_count = 0
        project_template_readiness_checklist_entrypoints_source_reports = @()
        release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 0
        release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @()
        word_visual_standard_review_metadata_source_report_count = 0
        word_visual_standard_review_metadata_source_reports = @()
        error = ""
    }
    template_schema = [ordered]@{
        requested = $templateSchemaRequested
        baseline = $resolvedTemplateSchemaBaseline
        input_docx = $resolvedTemplateSchemaInputDocx
        generated_output = if ($templateSchemaRequested) { $resolvedTemplateSchemaGeneratedOutput } else { "" }
    }
    template_schema_manifest = [ordered]@{
        requested = $templateSchemaManifestRequested
        manifest_path = $resolvedTemplateSchemaManifestPath
        output_dir = $resolvedTemplateSchemaManifestOutputDir
        summary_json = $resolvedTemplateSchemaManifestSummaryPath
    }
    project_template_smoke = [ordered]@{
        requested = $projectTemplateSmokeRequested
        manifest_path = $resolvedProjectTemplateSmokeManifestPath
        output_dir = $resolvedProjectTemplateSmokeOutputDir
        summary_json = $resolvedProjectTemplateSmokeSummaryPath
        require_full_coverage = [bool]$ProjectTemplateSmokeRequireFullCoverage
        candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
        candidate_count = 0
        registered_candidate_count = 0
        registered_manifest_entry_count = 0
        unregistered_candidate_count = 0
        excluded_candidate_count = 0
        dirty_schema_baseline_count = 0
        schema_baseline_drift_count = 0
        schema_patch_approval_pending_count = 0
        schema_patch_approval_approved_count = 0
        schema_patch_approval_rejected_count = 0
        schema_patch_approval_compliance_issue_count = 0
        schema_patch_approval_invalid_result_count = 0
        schema_patch_approval_gate_status = "not_required"
        schema_patch_approval_gate_blocked = $false
        schema_patch_approval_history_status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
        schema_patch_approval_history_json = if ($projectTemplateSmokeRequested) { $schemaApprovalHistoryJsonPath } else { "" }
        schema_patch_approval_history_markdown = if ($projectTemplateSmokeRequested) { $schemaApprovalHistoryMarkdownPath } else { "" }
        schema_patch_approval_history_input_count = 0
        schema_patch_approval_history_error = ""
        candidate_coverage = [ordered]@{
            status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
            require_full_coverage = [bool]$ProjectTemplateSmokeRequireFullCoverage
            candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
            candidate_count = 0
            registered_candidate_count = 0
            registered_manifest_entry_count = 0
            unregistered_candidate_count = 0
            excluded_candidate_count = 0
        }
    }
    readme_gallery = [ordered]@{
        status = if ($SkipVisualGate) { "visual_gate_skipped" } else { "pending" }
    }
    steps = [ordered]@{
        configure = [ordered]@{ status = if ($SkipConfigure) { "skipped" } else { "pending" } }
        build = [ordered]@{ status = if ($SkipBuild) { "skipped" } else { "pending" } }
        tests = [ordered]@{ status = if ($SkipTests) { "skipped" } else { "pending" } }
        template_schema = [ordered]@{ status = if ($templateSchemaRequested) { "pending" } else { "not_requested" } }
        template_schema_manifest = [ordered]@{ status = if ($templateSchemaManifestRequested) { "pending" } else { "not_requested" } }
        project_template_smoke = [ordered]@{
            status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
            dirty_schema_baseline_count = 0
            schema_baseline_drift_count = 0
            schema_patch_approval_pending_count = 0
            schema_patch_approval_approved_count = 0
            schema_patch_approval_rejected_count = 0
            schema_patch_approval_compliance_issue_count = 0
            schema_patch_approval_invalid_result_count = 0
            schema_patch_approval_gate_status = "not_required"
            schema_patch_approval_gate_blocked = $false
            schema_patch_approval_history_status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
            schema_patch_approval_history_json = if ($projectTemplateSmokeRequested) { $schemaApprovalHistoryJsonPath } else { "" }
            schema_patch_approval_history_markdown = if ($projectTemplateSmokeRequested) { $schemaApprovalHistoryMarkdownPath } else { "" }
            schema_patch_approval_history_input_count = 0
            schema_patch_approval_history_error = ""
            candidate_coverage = [ordered]@{
                status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
                require_full_coverage = [bool]$ProjectTemplateSmokeRequireFullCoverage
                candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
                candidate_count = 0
                registered_candidate_count = 0
                registered_manifest_entry_count = 0
                unregistered_candidate_count = 0
                excluded_candidate_count = 0
            }
        }
        install_smoke = [ordered]@{ status = if ($SkipInstallSmoke) { "skipped" } else { "pending" } }
        visual_gate = [ordered]@{ status = if ($SkipVisualGate) { "skipped" } else { "pending" } }
        release_blocker_rollup = [ordered]@{
            status = if ($releaseBlockerRollupRequested) { "pending" } else { "not_requested" }
            summary_json = $releaseBlockerRollupSummaryPath
            report_markdown = $releaseBlockerRollupMarkdownPath
            source_report_count = 0
            source_failure_count = 0
            release_blocker_count = 0
            release_blockers = @()
            blocker_source_schema_summary = @()
            action_item_count = 0
            action_items = @()
            action_item_source_schema_summary = @()
            informational_action_item_source_schema_summary = @()
            warning_count = 0
            warnings = @()
            warning_source_schema_summary = @()
            governance_metric_count = 0
            governance_metrics = @()
            manifest_signoff_entrypoints_source_report_count = 0
            manifest_signoff_entrypoints_source_reports = @()
            error = ""
        }
        release_governance_handoff = [ordered]@{
            status = if ($releaseGovernanceHandoffRequested) { "pending" } else { "not_requested" }
            summary_json = $releaseGovernanceHandoffSummaryPath
            report_markdown = $releaseGovernanceHandoffMarkdownPath
            expected_report_profile = $ReleaseGovernanceHandoffExpectedReportProfile
            expected_report_count = 0
            loaded_report_count = 0
            missing_report_count = 0
            failed_report_count = 0
            report_count = 0
            reports = @()
            governance_metric_count = 0
            governance_metrics = @()
            project_template_delivery_readiness_contract = $null
            project_template_onboarding_governance_contract = $null
            release_blocker_rollup = $null
            release_blocker_count = 0
            release_blockers = @()
            action_item_count = 0
            action_items = @()
            warning_count = 0
            warnings = @()
            manifest_signoff_entrypoints_source_report_count = 0
            manifest_signoff_entrypoints_source_reports = @()
            project_template_readiness_checklist_entrypoints_source_report_count = 0
            project_template_readiness_checklist_entrypoints_source_reports = @()
            release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 0
            release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @()
            word_visual_standard_review_metadata_source_report_count = 0
            word_visual_standard_review_metadata_source_reports = @()
            error = ""
        }
        project_template_workflow_dashboard = [ordered]@{
            status = if ($projectTemplateWorkflowDashboardRequested) { "pending" } else { "not_requested" }
            summary_json = $projectTemplateWorkflowDashboardSummaryPath
            report_markdown = $projectTemplateWorkflowDashboardMarkdownPath
            release_ready = $false
            release_blocker_count = 0
            warning_count = 0
            source_report_count = 0
            next_action = $null
            next_action_summary = @()
            next_action_group_count = 0
            error = ""
        }
        pdf_visual_gate = $pdfVisualGateSummaryInfo
        pdf_visual_gate_attempt = $pdfVisualGateAttemptSummaryInfo
        pdf_visual_segmented_gate = $pdfVisualSegmentedGateSummaryInfo
        pdf_bounded_ctest = $pdfBoundedCtestSummaryInfo
        pdf_full_ctest_readiness = $pdfFullCtestReadinessInfo
    }
}

$activeStep = ""
$releaseBlockerRollupFailure = $null
$releaseGovernanceHandoffFailure = $null
