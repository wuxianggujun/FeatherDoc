function Get-ReleaseBlockerActionGuidanceLines {
    param(
        [AllowNull()]$Blocker,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $action = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "action"
    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    if (-not (Test-ReleaseBlockerActionRegistered -Action $action)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Unregistered release blocker action `{0}`: add a fixed checklist runbook in `release_blocker_metadata_helpers.ps1`; until then, follow the blocker summary, update the owning evidence, rerun the relevant release checks, and regenerate the release note bundle before public release.' -f $action)
        return $guidanceLines.ToArray()
    }

    switch ($action) {
        "fix_schema_patch_approval_result" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `fix_schema_patch_approval_result`: update blocked `schema_patch_approval_result.json` record(s), provide reviewer and reviewed_at for non-pending decisions, then refresh release metadata.'

            $historyMarkdown = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "history_markdown"
            if (-not [string]::IsNullOrWhiteSpace($historyMarkdown)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Open the schema approval history markdown before editing approval records: {0}" -f (Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $historyMarkdown))
            }

            $summaryJson = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "summary_json"
            $displaySummaryJson = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $summaryJson
            $displayReleaseSummaryJson = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Run `sync_project_template_schema_approval.ps1` after updating approval records, using project template summary `{0}` and release summary `{1}`, then regenerate the release note bundle.' -f $displaySummaryJson, $displayReleaseSummaryJson)
            break
        }
        "complete_visual_manual_review" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `complete_visual_manual_review`: open the referenced visual review task, record the reviewer verdict and reviewed_at, then resync the visual verdict metadata before public release.'
            break
        }
        { $_ -in @(
                "resolve_pending_schema_approvals",
                "fix_invalid_approval_records",
                "add_explicit_confidence_metadata",
                "add_business_template_source_metadata",
                "add_business_template_document_type_metadata",
                "add_business_template_corpus_role_metadata",
                "review_schema_patch_confidence_calibration_evidence"
            ) } {
            Add-SchemaPatchConfidenceCalibrationGuidanceLines `
                -Lines $guidanceLines `
                -Item $Blocker `
                -RepoRoot $RepoRoot `
                -ReleaseSummaryJson $ReleaseSummaryJson
            break
        }
        { $_ -in @(
                "fix_custom_xml_data_binding_source",
                "sync_or_fill_bound_content_control",
                "review_content_control_lock_strategy",
                "review_unbound_form_content_control",
                "review_duplicate_content_control_binding",
                "collect_content_control_data_binding_evidence",
                "review_content_control_data_binding_evidence",
                "run_content_control_custom_xml_sync"
            ) } {
            Add-ContentControlDataBindingGovernanceGuidanceLines `
                -Lines $guidanceLines `
                -Item $Blocker `
                -RepoRoot $RepoRoot `
                -ReleaseSummaryJson $ReleaseSummaryJson
            break
        }
        { $_ -in @(
                "approve_project_template_schema",
                "collect_project_template_onboarding_governance_evidence",
                "freeze_schema_baseline",
                "promote_schema_update_candidate",
                "review_project_template_delivery_readiness_evidence",
                "review_project_template_smoke_failure",
                "review_schema_approval_history",
                "review_schema_baseline",
                "review_schema_update_candidate",
                "run_project_template_smoke_for_registered_manifest",
                "run_project_template_smoke_then_review_schema_patch_approval",
                "update_template_or_schema_before_retry"
            ) } {
            Add-ProjectTemplateGovernanceGuidanceLines `
                -Lines $guidanceLines `
                -Item $Blocker `
                -RepoRoot $RepoRoot `
                -ReleaseSummaryJson $ReleaseSummaryJson
            break
        }
        { $_ -in @(
                "fix_numbering_catalog_baseline_lint",
                "promote_numbering_catalog_exemplar",
                "preview_style_numbering_repair",
                "rebuild_document_skeleton_governance_rollup",
                "rebuild_numbering_catalog_manifest_summary",
                "rerun_document_skeleton_governance_report",
                "refresh_numbering_catalog_baseline_or_repair_docx",
                "register_numbering_catalog_baseline",
                "review_numbering_catalog_check_issues",
                "review_numbering_catalog_governance_sources",
                "review_numbering_catalog_real_corpus_alignment",
                "review_style_numbering_audit"
            ) } {
            Add-NumberingCatalogGovernanceGuidanceLines `
                -Lines $guidanceLines `
                -Item $Blocker `
                -RepoRoot $RepoRoot `
                -ReleaseSummaryJson $ReleaseSummaryJson
            break
        }
        { $_ -in @(
                "apply_safe_tblLook_fixes",
                "apply_safe_tblLook_fixes_then_visual_regression",
                "dry_run_apply_table_position_plans",
                "rebuild_table_layout_delivery_rollup",
                "review_floating_table_position_plans",
                "review_manual_table_style_definition_work",
                "review_table_layout_delivery_governance_sources",
                "review_table_position_plan",
                "review_table_style_quality_plan",
                "run_table_style_quality_visual_regression"
            ) } {
            Add-TableLayoutDeliveryGovernanceGuidanceLines `
                -Lines $guidanceLines `
                -Item $Blocker `
                -RepoRoot $RepoRoot `
                -ReleaseSummaryJson $ReleaseSummaryJson
            break
        }
        "prepare_pdf_visual_release_gate_build_outputs" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `prepare_pdf_visual_release_gate_build_outputs`: prepare or reuse the PDF visual release gate build outputs before attempting the full PDF visual gate.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightBlockingSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightBuildCandidateSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightDependencyInputSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $Blocker)
            $issueKeys = @(Get-ReleaseBlockerArrayProperty -Object $Blocker -Name "issue_keys" | ForEach-Object { [string]$_ })
            if ($issueKeys -contains "cmake_cache_exists") {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Preflight issue `cmake_cache_exists` means the selected build directory is not a reusable CMake build; prepare or point at a build directory containing `CMakeCache.txt` before checking CTest registration or PDF outputs.'
            }
            if ($issueKeys -contains "pdf_build_options_enabled") {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Preflight issue `pdf_build_options_enabled` means the selected CMake cache does not enable the full PDF writer/import build; reconfigure with `-DFEATHERDOC_BUILD_PDF=ON` and `-DFEATHERDOC_BUILD_PDF_IMPORT=ON` plus the required PDFio/PDFium inputs before generating visual gate outputs.'
            }

            $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_report_display"
            if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
                $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_report")
            }
            if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Open the PDF preflight governance report first: {0}" -f $sourceReportDisplay)
            }

            $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_json_display"
            if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_json")
            }
            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Open the source preflight JSON before changing build outputs: {0}" -f $sourceJsonDisplay)
            }

            $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "command_template"
            if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
                $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly'
            }
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Run the lightweight preflight command first: `{0}`' -f $commandTemplate)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'The `-PreflightOnly` command only verifies PDF visual gate prerequisites; it is not release-ready evidence until the full PDF visual release gate passes.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Only after preflight is ready and workstation resources allow it, run the full PDF visual release gate, rebuild the release blocker rollup, and regenerate the release note bundle.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'After each PDF preflight or gate attempt, clean up only task-owned PDF gate processes and transient outputs after capturing the required evidence; do not terminate unrelated external build, Office, browser, node, or PowerShell processes.'
            break
        }
        "rerun_pdf_controlled_visual_smoke_check" {
            Add-PdfControlledVisualSmokeCheckGuidanceLines `
                -Lines $guidanceLines `
                -Item $Blocker `
                -RepoRoot $RepoRoot `
                -ReleaseSummaryJson $ReleaseSummaryJson
            break
        }
        "rerun_pdf_visual_release_gate_preflight" {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'Use action `rerun_pdf_visual_release_gate_preflight`: regenerate the PDF visual release gate preflight summary, then rebuild the PDF preflight governance report.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightBlockingSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightBuildCandidateSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightDependencyInputSummaryLine -Item $Blocker)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text (Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $Blocker)

            $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_json_display"
            if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "source_json")
            }
            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ("Inspect the missing or unreadable preflight JSON path before rerunning: {0}" -f $sourceJsonDisplay)
            }

            $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Blocker -Name "command_template"
            if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
                $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json'
            }
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Run the lightweight preflight regeneration command: `{0}`' -f $commandTemplate)
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'A regenerated PDF preflight summary only refreshes prerequisite evidence; it is not release-ready evidence until the full PDF visual release gate passes.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'After the preflight summary is readable, rerun `write_pdf_visual_release_gate_preflight_governance_report.ps1`, rebuild the release blocker rollup, and regenerate the release note bundle.'
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text 'After each PDF preflight or gate attempt, clean up only task-owned PDF gate processes and transient outputs after capturing the required evidence; do not terminate unrelated external build, Office, browser, node, or PowerShell processes.'
            break
        }
        "restore_docx_functional_smoke_evidence" {
            Add-DocxFunctionalSmokeEvidenceGuidanceLines `
                -Lines $guidanceLines `
                -Item $Blocker `
                -RepoRoot $RepoRoot `
                -ReleaseSummaryJson $ReleaseSummaryJson
            break
        }
        default {
            Add-ReleaseBlockerActionGuidanceLine -Lines $guidanceLines -Text ('Registered release blocker action `{0}` does not have a checklist runbook yet; add one in `release_blocker_metadata_helpers.ps1` before relying on this action for handoff.' -f $action)
        }
    }

    return $guidanceLines.ToArray()
}

function Add-ReleaseBlockerMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$RepoRoot,
        [string]$Heading = "## Release Blockers",
        [switch]$IncludeWhenEmpty,
        [switch]$PublicArtifactPaths
    )

    Assert-ReleaseBlockerMetadataQuality -Summary $Summary -Context $Heading
    $releaseBlockerCount = Get-ReleaseBlockerCount -Summary $Summary
    $releaseBlockers = @(Get-ReleaseBlockers -Summary $Summary)
    if ($releaseBlockerCount -le 0 -and -not $IncludeWhenEmpty.IsPresent) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    [void]$Lines.Add("- Release blockers: $releaseBlockerCount")
    if ($releaseBlockerCount -le 0) {
        return
    }

    foreach ($releaseBlocker in $releaseBlockers) {
        [void]$Lines.Add("- $(Get-ReleaseBlockerSummaryText -Blocker $releaseBlocker)")
        $message = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name "message"
        if (-not [string]::IsNullOrWhiteSpace($message)) {
            [void]$Lines.Add("  - Message: $message")
        }

        foreach ($pathInfo in @(
                @{ Name = "summary_json"; Label = "Summary JSON" },
                @{ Name = "history_json"; Label = "History JSON" },
                @{ Name = "history_markdown"; Label = "History Markdown" }
            )) {
            $pathValue = Get-ReleaseBlockerPropertyValue -Object $releaseBlocker -Name $pathInfo.Name
            if (-not [string]::IsNullOrWhiteSpace($pathValue)) {
                $displayPath = if ($PublicArtifactPaths.IsPresent) {
                    Get-ReleaseBlockerPublicDisplayPath -RepoRoot $RepoRoot -Path $pathValue
                } else {
                    Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $pathValue
                }
                [void]$Lines.Add("  - $($pathInfo.Label): $displayPath")
            }
        }

        foreach ($item in @(Get-ReleaseBlockerArrayProperty -Object $releaseBlocker -Name "items")) {
            [void]$Lines.Add("  - Item $(Get-ReleaseBlockerItemSummaryText -Item $item)")
        }
    }
}
