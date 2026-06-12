    if ($templateSchemaManifestRequested) {
        $activeStep = "template_schema_manifest"
        Write-Step "Running template schema manifest check"
        Assert-PathExists -Path $resolvedTemplateSchemaManifestPath -Label "template schema manifest"
        New-Item -ItemType Directory -Path $resolvedTemplateSchemaManifestOutputDir -Force | Out-Null

        $templateSchemaManifestArgs = @(
            "-ManifestPath"
            $resolvedTemplateSchemaManifestPath
            "-BuildDir"
            $resolvedBuildDir
            "-OutputDir"
            $resolvedTemplateSchemaManifestOutputDir
            "-SkipBuild"
        )

        $templateSchemaManifestOutput = @(
            & powershell.exe -ExecutionPolicy Bypass -File $templateSchemaManifestScript @templateSchemaManifestArgs 2>&1
        )
        $templateSchemaManifestExitCode = $LASTEXITCODE
        foreach ($line in $templateSchemaManifestOutput) {
            Write-Host $line
        }
        if ($templateSchemaManifestExitCode -notin @(0, 1)) {
            throw "Template schema manifest check failed."
        }

        $templateSchemaManifestInfo = Parse-TemplateSchemaManifestSummary -SummaryPath $resolvedTemplateSchemaManifestSummaryPath
        $summary.steps.template_schema_manifest.status = if ($templateSchemaManifestExitCode -eq 0) {
            "completed"
        } else {
            "failed"
        }
        $summary.steps.template_schema_manifest.summary_json = $resolvedTemplateSchemaManifestSummaryPath
        $summary.steps.template_schema_manifest.manifest_path = [string]$templateSchemaManifestInfo.manifest_path
        $summary.steps.template_schema_manifest.output_dir = [string]$resolvedTemplateSchemaManifestOutputDir
        $summary.steps.template_schema_manifest.passed = [bool]$templateSchemaManifestInfo.passed
        $summary.steps.template_schema_manifest.entry_count = [int]$templateSchemaManifestInfo.entry_count
        $summary.steps.template_schema_manifest.drift_count = [int]$templateSchemaManifestInfo.drift_count
        $summary.steps.template_schema_manifest.dirty_baseline_count = [int]$templateSchemaManifestInfo.dirty_baseline_count

        $summary.template_schema_manifest.summary_json = $resolvedTemplateSchemaManifestSummaryPath
        $summary.template_schema_manifest.manifest_path = [string]$templateSchemaManifestInfo.manifest_path
        $summary.template_schema_manifest.output_dir = [string]$resolvedTemplateSchemaManifestOutputDir
        $summary.template_schema_manifest.passed = [bool]$templateSchemaManifestInfo.passed
        $summary.template_schema_manifest.entry_count = [int]$templateSchemaManifestInfo.entry_count
        $summary.template_schema_manifest.drift_count = [int]$templateSchemaManifestInfo.drift_count
        $summary.template_schema_manifest.dirty_baseline_count = [int]$templateSchemaManifestInfo.dirty_baseline_count

        if ($templateSchemaManifestExitCode -ne 0) {
            if ([int]$templateSchemaManifestInfo.drift_count -gt 0 -and
                [int]$templateSchemaManifestInfo.dirty_baseline_count -gt 0) {
                throw "Template schema manifest drift and lint failures detected."
            }
            if ([int]$templateSchemaManifestInfo.drift_count -gt 0) {
                throw "Template schema manifest drift detected."
            }
            if ([int]$templateSchemaManifestInfo.dirty_baseline_count -gt 0) {
                throw "Template schema manifest lint failed."
            }
            throw "Template schema manifest check failed."
        }
    }

    if ($projectTemplateSmokeRequested) {
        $activeStep = "project_template_smoke"
        Write-Step "Running project template smoke"
        Assert-PathExists -Path $resolvedProjectTemplateSmokeManifestPath -Label "project template smoke manifest"
        New-Item -ItemType Directory -Path $resolvedProjectTemplateSmokeOutputDir -Force | Out-Null

        $projectTemplateSmokeCoverageArgs = @(
            "-ManifestPath"
            $resolvedProjectTemplateSmokeManifestPath
            "-BuildDir"
            $resolvedBuildDir
            "-OutputPath"
            $resolvedProjectTemplateSmokeCandidateDiscoveryPath
            "-Json"
            "-IncludeRegistered"
            "-IncludeExcluded"
        )
        if ($ProjectTemplateSmokeRequireFullCoverage) {
            $projectTemplateSmokeCoverageArgs += "-FailOnUnregistered"
        }

        $projectTemplateSmokeCoverageOutput = @(
            & powershell.exe -ExecutionPolicy Bypass -File $projectTemplateSmokeDiscoverScript @projectTemplateSmokeCoverageArgs 2>&1
        )
        $projectTemplateSmokeCoverageExitCode = $LASTEXITCODE
        foreach ($line in $projectTemplateSmokeCoverageOutput) {
            Write-Host $line
        }
        if ($projectTemplateSmokeCoverageExitCode -notin @(0, 1)) {
            throw "Project template smoke candidate discovery failed."
        }

        Assert-PathExists -Path $resolvedProjectTemplateSmokeCandidateDiscoveryPath -Label "project template smoke candidate discovery JSON"
        $projectTemplateSmokeCoverageInfo = Get-Content -Raw -LiteralPath $resolvedProjectTemplateSmokeCandidateDiscoveryPath | ConvertFrom-Json
        $summary.steps.project_template_smoke.candidate_coverage = [ordered]@{
            status = if ($projectTemplateSmokeCoverageExitCode -eq 0) { "completed" } else { "failed" }
            require_full_coverage = [bool]$ProjectTemplateSmokeRequireFullCoverage
            candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
            candidate_count = [int]$projectTemplateSmokeCoverageInfo.candidate_count
            registered_candidate_count = [int]$projectTemplateSmokeCoverageInfo.registered_candidate_count
            registered_manifest_entry_count = [int]$projectTemplateSmokeCoverageInfo.registered_manifest_entry_count
            unregistered_candidate_count = [int]$projectTemplateSmokeCoverageInfo.unregistered_candidate_count
            excluded_candidate_count = [int]$projectTemplateSmokeCoverageInfo.excluded_candidate_count
        }
        $summary.project_template_smoke.candidate_coverage = $summary.steps.project_template_smoke.candidate_coverage
        $summary.project_template_smoke.candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
        $summary.project_template_smoke.candidate_count = [int]$projectTemplateSmokeCoverageInfo.candidate_count
        $summary.project_template_smoke.registered_candidate_count = [int]$projectTemplateSmokeCoverageInfo.registered_candidate_count
        $summary.project_template_smoke.registered_manifest_entry_count = [int]$projectTemplateSmokeCoverageInfo.registered_manifest_entry_count
        $summary.project_template_smoke.unregistered_candidate_count = [int]$projectTemplateSmokeCoverageInfo.unregistered_candidate_count
        $summary.project_template_smoke.excluded_candidate_count = [int]$projectTemplateSmokeCoverageInfo.excluded_candidate_count

        if ($ProjectTemplateSmokeRequireFullCoverage -and $projectTemplateSmokeCoverageExitCode -ne 0) {
            throw "Project template smoke candidate coverage is incomplete."
        }

        $projectTemplateSmokeArgs = @(
            "-ManifestPath"
            $resolvedProjectTemplateSmokeManifestPath
            "-BuildDir"
            $resolvedBuildDir
            "-OutputDir"
            $resolvedProjectTemplateSmokeOutputDir
            "-Dpi"
            $Dpi.ToString()
            "-SkipBuild"
        )
        if ($KeepWordOpen) {
            $projectTemplateSmokeArgs += "-KeepWordOpen"
        }
        if ($VisibleWord) {
            $projectTemplateSmokeArgs += "-VisibleWord"
        }

        $projectTemplateSmokeOutput = @(
            & powershell.exe -ExecutionPolicy Bypass -File $projectTemplateSmokeScript @projectTemplateSmokeArgs 2>&1
        )
        $projectTemplateSmokeExitCode = $LASTEXITCODE
        foreach ($line in $projectTemplateSmokeOutput) {
            Write-Host $line
        }
        if ($projectTemplateSmokeExitCode -notin @(0, 1)) {
            throw "Project template smoke failed."
        }

        $projectTemplateSmokeInfo = Parse-ProjectTemplateSmokeSummary -SummaryPath $resolvedProjectTemplateSmokeSummaryPath
        $projectTemplateSmokeSchemaBaselineCounts = Get-ProjectTemplateSmokeSchemaBaselineCounts -Summary $projectTemplateSmokeInfo
        $projectTemplateSmokeDirtySchemaBaselineCount = Get-ProjectTemplateSmokeDirtySchemaBaselineCount -Summary $projectTemplateSmokeInfo
        $projectTemplateSmokeSchemaBaselineDriftCount = [int]$projectTemplateSmokeSchemaBaselineCounts.drift
        $projectTemplateSmokeApprovalGate = Get-ProjectTemplateSmokeSchemaApprovalGateInfo -Summary $projectTemplateSmokeInfo
        $summary.steps.project_template_smoke.status = if ($projectTemplateSmokeExitCode -eq 0) {
            "completed"
        } else {
            "failed"
        }
        $summary.steps.project_template_smoke.summary_json = $resolvedProjectTemplateSmokeSummaryPath
        $summary.steps.project_template_smoke.manifest_path = [string]$projectTemplateSmokeInfo.manifest_path
        $summary.steps.project_template_smoke.output_dir = [string]$resolvedProjectTemplateSmokeOutputDir
        $summary.steps.project_template_smoke.passed = [bool]$projectTemplateSmokeInfo.passed
        $summary.steps.project_template_smoke.entry_count = [int]$projectTemplateSmokeInfo.entry_count
        $summary.steps.project_template_smoke.failed_entry_count = [int]$projectTemplateSmokeInfo.failed_entry_count
        $summary.steps.project_template_smoke.dirty_schema_baseline_count = $projectTemplateSmokeDirtySchemaBaselineCount
        $summary.steps.project_template_smoke.schema_baseline_drift_count = $projectTemplateSmokeSchemaBaselineDriftCount
        $summary.steps.project_template_smoke.visual_entry_count = [int]$projectTemplateSmokeInfo.visual_entry_count
        $summary.steps.project_template_smoke.visual_verdict = [string]$projectTemplateSmokeInfo.visual_verdict
        $summary.steps.project_template_smoke.manual_review_pending_count = [int]$projectTemplateSmokeInfo.manual_review_pending_count
        $summary.steps.project_template_smoke.visual_review_undetermined_count = [int]$projectTemplateSmokeInfo.visual_review_undetermined_count
        $summary.steps.project_template_smoke.overall_status = [string]$projectTemplateSmokeInfo.overall_status
        $summary.steps.project_template_smoke.schema_patch_approval_pending_count = [int]$projectTemplateSmokeApprovalGate.pending_count
        $summary.steps.project_template_smoke.schema_patch_approval_approved_count = [int]$projectTemplateSmokeApprovalGate.approved_count
        $summary.steps.project_template_smoke.schema_patch_approval_rejected_count = [int]$projectTemplateSmokeApprovalGate.rejected_count
        $summary.steps.project_template_smoke.schema_patch_approval_compliance_issue_count = [int]$projectTemplateSmokeApprovalGate.compliance_issue_count
        $summary.steps.project_template_smoke.schema_patch_approval_invalid_result_count = [int]$projectTemplateSmokeApprovalGate.invalid_result_count
        $summary.steps.project_template_smoke.schema_patch_approval_gate_status = [string]$projectTemplateSmokeApprovalGate.gate_status
        $summary.steps.project_template_smoke.schema_patch_approval_gate_blocked = [bool]$projectTemplateSmokeApprovalGate.gate_blocked

        $summary.project_template_smoke.summary_json = $resolvedProjectTemplateSmokeSummaryPath
        $summary.project_template_smoke.manifest_path = [string]$projectTemplateSmokeInfo.manifest_path
        $summary.project_template_smoke.output_dir = [string]$resolvedProjectTemplateSmokeOutputDir
        $summary.project_template_smoke.passed = [bool]$projectTemplateSmokeInfo.passed
        $summary.project_template_smoke.entry_count = [int]$projectTemplateSmokeInfo.entry_count
        $summary.project_template_smoke.failed_entry_count = [int]$projectTemplateSmokeInfo.failed_entry_count
        $summary.project_template_smoke.dirty_schema_baseline_count = $projectTemplateSmokeDirtySchemaBaselineCount
        $summary.project_template_smoke.schema_baseline_drift_count = $projectTemplateSmokeSchemaBaselineDriftCount
        $summary.project_template_smoke.visual_entry_count = [int]$projectTemplateSmokeInfo.visual_entry_count
        $summary.project_template_smoke.visual_verdict = [string]$projectTemplateSmokeInfo.visual_verdict
        $summary.project_template_smoke.manual_review_pending_count = [int]$projectTemplateSmokeInfo.manual_review_pending_count
        $summary.project_template_smoke.visual_review_undetermined_count = [int]$projectTemplateSmokeInfo.visual_review_undetermined_count
        $summary.project_template_smoke.overall_status = [string]$projectTemplateSmokeInfo.overall_status
        $summary.project_template_smoke.schema_patch_approval_pending_count = [int]$projectTemplateSmokeApprovalGate.pending_count
        $summary.project_template_smoke.schema_patch_approval_approved_count = [int]$projectTemplateSmokeApprovalGate.approved_count
        $summary.project_template_smoke.schema_patch_approval_rejected_count = [int]$projectTemplateSmokeApprovalGate.rejected_count
        $summary.project_template_smoke.schema_patch_approval_compliance_issue_count = [int]$projectTemplateSmokeApprovalGate.compliance_issue_count
        $summary.project_template_smoke.schema_patch_approval_invalid_result_count = [int]$projectTemplateSmokeApprovalGate.invalid_result_count
        $summary.project_template_smoke.schema_patch_approval_gate_status = [string]$projectTemplateSmokeApprovalGate.gate_status
        $summary.project_template_smoke.schema_patch_approval_gate_blocked = [bool]$projectTemplateSmokeApprovalGate.gate_blocked
        $summary.project_template_smoke.schema_patch_approval_history_json = $schemaApprovalHistoryJsonPath
        $summary.project_template_smoke.schema_patch_approval_history_markdown = $schemaApprovalHistoryMarkdownPath
        $summary.steps.project_template_smoke.schema_patch_approval_history_json = $schemaApprovalHistoryJsonPath
        $summary.steps.project_template_smoke.schema_patch_approval_history_markdown = $schemaApprovalHistoryMarkdownPath

        if ([bool]$projectTemplateSmokeApprovalGate.gate_blocked) {
            $summary.steps.project_template_smoke.status = "failed"
            throw "Project template smoke schema approval gate blocked."
        }

        if ($projectTemplateSmokeExitCode -ne 0) {
            $failedEntryCount = [int]$projectTemplateSmokeInfo.failed_entry_count
            if ($projectTemplateSmokeDirtySchemaBaselineCount -gt 0 -and
                $projectTemplateSmokeSchemaBaselineDriftCount -gt 0) {
                throw "Project template smoke schema baseline lint and drift failures detected."
            }
            if ($projectTemplateSmokeDirtySchemaBaselineCount -gt 0 -and
                $failedEntryCount -gt $projectTemplateSmokeDirtySchemaBaselineCount) {
                throw "Project template smoke schema baseline lint and entry failures detected."
            }
            if ($projectTemplateSmokeDirtySchemaBaselineCount -gt 0) {
                throw "Project template smoke schema baseline lint failed."
            }
            if ($projectTemplateSmokeSchemaBaselineDriftCount -gt 0) {
                throw "Project template smoke schema baseline drift detected."
            }
            if ($failedEntryCount -gt 0) {
                throw "Project template smoke reported failing entries."
            }

            throw "Project template smoke failed."
        }
    }
