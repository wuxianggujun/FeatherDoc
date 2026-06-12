function Write-SummaryMarkdown {
    param(
        [string]$Path,
        [string]$RepoRoot,
        $Summary
    )

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Project Template Smoke")
    $lines.Add("")
    $lines.Add("- Generated at: $($Summary.generated_at)")
    $visualReviewSyncedAt = Get-OptionalObjectPropertyValue -Object $Summary -Name "visual_review_synced_at"
    if (-not [string]::IsNullOrWhiteSpace($visualReviewSyncedAt)) {
        $lines.Add("- Visual review synced at: $visualReviewSyncedAt")
    }
    $lines.Add("- Manifest: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $Summary.manifest_path)")
    $lines.Add("- Output directory: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $Summary.output_dir)")
    $lines.Add("- Overall status: $($Summary.overall_status)")
    $lines.Add("- Passed flag: $($Summary.passed)")
    $lines.Add("- Entry count: $($Summary.entry_count)")
    $lines.Add("- Failed entries: $($Summary.failed_entry_count)")
    $dirtySchemaBaselineCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "dirty_schema_baseline_count"
    if ([string]::IsNullOrWhiteSpace($dirtySchemaBaselineCount)) {
        $dirtySchemaBaselineCount = "0"
    }
    $lines.Add("- Dirty schema baselines: $dirtySchemaBaselineCount")
    $schemaPatchReviewCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_review_count"
    $schemaPatchReviewChangedCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_review_changed_count"
    if ([string]::IsNullOrWhiteSpace($schemaPatchReviewCount)) {
        $schemaPatchReviewCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchReviewChangedCount)) {
        $schemaPatchReviewChangedCount = "0"
    }
    $schemaPatchApprovalPendingCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_pending_count"
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalPendingCount)) {
        $schemaPatchApprovalPendingCount = "0"
    }
    $lines.Add("- Schema patch reviews: $schemaPatchReviewCount")
    $lines.Add("- Changed schema patch reviews: $schemaPatchReviewChangedCount")
    $schemaPatchApprovalApprovedCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_approved_count"
    $schemaPatchApprovalRejectedCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_rejected_count"
    $schemaPatchApprovalInvalidResultCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_invalid_result_count"
    $schemaPatchApprovalGateStatus = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_gate_status"
    $schemaPatchApprovalComplianceIssueCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_compliance_issue_count"
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalApprovedCount)) {
        $schemaPatchApprovalApprovedCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalRejectedCount)) {
        $schemaPatchApprovalRejectedCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalComplianceIssueCount)) {
        $schemaPatchApprovalComplianceIssueCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalInvalidResultCount)) {
        $schemaPatchApprovalInvalidResultCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalGateStatus)) {
        $schemaPatchApprovalGateStatus = "not_required"
    }
    $lines.Add("- Schema patch approvals pending: $schemaPatchApprovalPendingCount")
    $lines.Add("- Schema patch approvals approved: $schemaPatchApprovalApprovedCount")
    $lines.Add("- Schema patch approvals rejected: $schemaPatchApprovalRejectedCount")
    $lines.Add("- Schema patch approval compliance issues: $schemaPatchApprovalComplianceIssueCount")
    $lines.Add("- Schema patch approval invalid results: $schemaPatchApprovalInvalidResultCount")
    $lines.Add("- Schema patch approval gate status: $schemaPatchApprovalGateStatus")
    $lines.Add("- Visual smoke entries: $($Summary.visual_entry_count)")
    $lines.Add("- Visual verdict: $($Summary.visual_verdict)")
    $lines.Add("- Entries with pending visual review: $($Summary.manual_review_pending_count)")
    $lines.Add("- Entries with undetermined visual review: $($Summary.visual_review_undetermined_count)")
    $lines.Add("")

    $schemaPatchApprovalItems = Get-OptionalObjectArrayProperty -Object $Summary -Name "schema_patch_approval_items"
    if (@($schemaPatchApprovalItems).Count -gt 0) {
        $lines.Add("## Schema Patch Approvals")
        $lines.Add("")
        foreach ($approval in @($schemaPatchApprovalItems)) {
            $complianceIssues = Get-OptionalObjectArrayProperty -Object $approval -Name "compliance_issues"
            $complianceText = if (@($complianceIssues).Count -gt 0) { " compliance_issues=$($complianceIssues -join ',')" } else { "" }
            $lines.Add("- $($approval.name): status=$($approval.status) decision=$($approval.decision) candidate=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $approval.schema_update_candidate) approval=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $approval.approval_result) review=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $approval.review_json) action=$($approval.action)$complianceText")
        }
        $lines.Add("")
    }

    foreach ($entry in $Summary.entries) {
        $lines.Add("## $($entry.name)")
        $lines.Add("")
        $lines.Add("- Status: $($entry.status)")
        $lines.Add("- Input DOCX: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $entry.input_docx)")
        $lines.Add("- Entry artifact directory: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $entry.artifact_dir)")

        foreach ($validation in @($entry.checks.template_validations)) {
            $lines.Add("- Template validation '$($validation.name)': passed=$($validation.passed) output=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $validation.output_json)")
        }

        $schemaValidation = $entry.checks.schema_validation
        if ($schemaValidation.enabled) {
            $schemaValidationOutput = Get-OptionalObjectPropertyValue -Object $schemaValidation -Name "output_json"
            $lines.Add("- Schema validation: passed=$($schemaValidation.passed) output=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaValidationOutput)")
        }

        $schemaBaseline = $entry.checks.schema_baseline
        if ($schemaBaseline.enabled) {
            $schemaBaselineLog = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "log_path"
            $schemaPatchReviewOutputPath = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "schema_patch_review_output_path"
            $schemaPatchReview = Get-OptionalObjectPropertyObject -Object $schemaBaseline -Name "schema_patch_review"
            $schemaLintClean = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "schema_lint_clean"
            $schemaLintIssueCount = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "schema_lint_issue_count"
            if ([string]::IsNullOrWhiteSpace($schemaLintClean)) {
                $schemaLintClean = "(not available)"
            }
            if ([string]::IsNullOrWhiteSpace($schemaLintIssueCount)) {
                $schemaLintIssueCount = "(not available)"
            }
            $lines.Add("- Schema baseline: matches=$($schemaBaseline.matches) schema_lint_clean=$schemaLintClean schema_lint_issue_count=$schemaLintIssueCount log=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaBaselineLog)")
            if ($null -ne $schemaPatchReview) {
                $lines.Add("- Schema patch review: changed=$($schemaPatchReview.changed) baseline_slots=$($schemaPatchReview.baseline_slot_count) generated_slots=$($schemaPatchReview.generated_slot_count) inserted_slots=$($schemaPatchReview.inserted_slots) replaced_slots=$($schemaPatchReview.replaced_slots) review=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaPatchReviewOutputPath)")
            }
            $schemaPatchApproval = Get-OptionalObjectPropertyObject -Object $schemaBaseline -Name "schema_patch_approval"
            if ($null -ne $schemaPatchApproval) {
                $complianceIssues = Get-OptionalObjectArrayProperty -Object $schemaPatchApproval -Name "compliance_issues"
                $complianceText = if (@($complianceIssues).Count -gt 0) { " compliance_issues=$($complianceIssues -join ',')" } else { "" }
                $lines.Add("- Schema patch approval: status=$($schemaPatchApproval.status) decision=$($schemaPatchApproval.decision) required=$($schemaPatchApproval.required) pending=$($schemaPatchApproval.pending) candidate=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaPatchApproval.schema_update_candidate) approval=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaPatchApproval.approval_result) action=$($schemaPatchApproval.action)$complianceText")
            }
            $repairedSchemaOutputPath = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "repaired_schema_output_path"
            if (-not [string]::IsNullOrWhiteSpace($repairedSchemaOutputPath)) {
                $lines.Add("- Schema baseline repaired candidate: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $repairedSchemaOutputPath)")
            }
        }

        $renderData = $entry.checks.render_data
        if ($renderData.enabled) {
            $remainingPlaceholderCount = Get-OptionalObjectPropertyValue -Object $renderData -Name "remaining_placeholder_count"
            if ([string]::IsNullOrWhiteSpace($remainingPlaceholderCount)) {
                $remainingPlaceholderCount = "(not available)"
            }
            $renderedDocx = Get-OptionalObjectPropertyValue -Object $renderData -Name "output_docx"
            $renderLog = Get-OptionalObjectPropertyValue -Object $renderData -Name "log_path"
            $lines.Add("- Render data: status=$($renderData.status) remaining_placeholders=$remainingPlaceholderCount output=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $renderedDocx) log=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $renderLog)")
        }

        $visualSmoke = $entry.checks.visual_smoke
        if ($visualSmoke.enabled) {
            $contactSheet = Get-OptionalObjectPropertyValue -Object $visualSmoke -Name "contact_sheet"
            $findingsCount = Get-OptionalObjectPropertyValue -Object $visualSmoke -Name "findings_count"
            if ([string]::IsNullOrWhiteSpace($findingsCount)) {
                $findingsCount = "0"
            }
            $lines.Add("- Visual smoke: review_status=$($visualSmoke.review_status) review_verdict=$($visualSmoke.review_verdict) findings=$findingsCount contact_sheet=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $contactSheet)")
        }

        foreach ($issue in @($entry.issues)) {
            $lines.Add("- Issue: $issue")
        }

        $lines.Add("")
    }

    $lines | Set-Content -LiteralPath $Path -Encoding UTF8
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedManifestPath = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedOutputDir = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $OutputDir -AllowMissing
$resolvedBuildDir = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
$entriesOutputDir = Join-Path $resolvedOutputDir "entries"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$summaryMarkdownPath = Join-Path $resolvedOutputDir "summary.md"
$baselineScriptPath = Join-Path $PSScriptRoot "check_template_schema_baseline.ps1"
$renderDataScriptPath = Join-Path $PSScriptRoot "render_template_document_from_data.ps1"
$visualSmokeScriptPath = Join-Path $PSScriptRoot "run_word_visual_smoke.ps1"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $entriesOutputDir -Force | Out-Null

$manifestValidation = Test-ProjectTemplateSmokeManifest `
    -RepoRoot $repoRoot `
    -ManifestPath $resolvedManifestPath `
