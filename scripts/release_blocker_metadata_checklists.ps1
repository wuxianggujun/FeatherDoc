function Get-ReleaseGovernanceChecklistGuidanceLines {
    param(
        [AllowNull()]$Item,
        [string]$ItemKind,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    $guidanceLines = New-Object 'System.Collections.Generic.List[string]'
    $id = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "id") -Fallback "(unknown)"
    $action = Get-ReleaseBlockerPropertyValue -Object $Item -Name "action"
    $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_schema")
    $sourceReportDisplay = Get-ReleaseGovernanceChecklistSourceDisplay `
        -Item $Item `
        -RepoRoot $RepoRoot `
        -DisplayPropertyName "source_report_display" `
        -PathPropertyName "source_report"
    $sourceJsonDisplay = Get-ReleaseGovernanceChecklistSourceDisplay `
        -Item $Item `
        -RepoRoot $RepoRoot `
        -DisplayPropertyName "source_json_display" `
        -PathPropertyName "source_json"
    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson

    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Open source report `{0}` while handling release governance {1} `{2}`.' -f $sourceReportDisplay, $ItemKind, $id)
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay) -and
        -not [string]::Equals($sourceJsonDisplay, $sourceReportDisplay, [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Open source JSON `{0}` while handling release governance {1} `{2}`.' -f $sourceJsonDisplay, $ItemKind, $id)
    }

    if ([string]::Equals($action, "prepare_pdf_visual_release_gate_build_outputs", [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Use action `{0}` for release governance {1} `{2}`: prepare or reuse the PDF visual release gate build outputs before attempting the full PDF visual gate.' -f $action, $ItemKind, $id)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightBlockingSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightBuildCandidateSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightDependencyInputSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $Item)
        $issueKeys = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "issue_keys" | ForEach-Object { [string]$_ })
        if ($issueKeys -contains "cmake_cache_exists") {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text 'Preflight issue `cmake_cache_exists` means the selected build directory is not a reusable CMake build; prepare or point at a build directory containing `CMakeCache.txt` before checking CTest registration or PDF outputs.'
        }
        if ($issueKeys -contains "pdf_build_options_enabled") {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text 'Preflight issue `pdf_build_options_enabled` means the selected CMake cache does not enable the full PDF writer/import build; reconfigure with `-DFEATHERDOC_BUILD_PDF=ON` and `-DFEATHERDOC_BUILD_PDF_IMPORT=ON` plus the required PDFio/PDFium inputs before generating visual gate outputs.'
        }
        $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
        if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
            $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir .\.bpdf-roundtrip-msvc -PreflightOnly'
        }
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Run the lightweight PDF preflight command first: `{0}`' -f $commandTemplate)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text 'The `-PreflightOnly` command only verifies PDF visual gate prerequisites; it is not release-ready evidence until the full PDF visual release gate passes.'
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Only after preflight is ready and workstation resources allow it, run the full PDF visual release gate, rerun release governance checks, and regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text 'After each PDF preflight or gate attempt, clean up only task-owned PDF gate processes and transient outputs after capturing the required evidence; do not terminate unrelated external build, Office, browser, node, or PowerShell processes.'
    } elseif ([string]::Equals($action, "rerun_pdf_controlled_visual_smoke_check", [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-PdfControlledVisualSmokeCheckGuidanceLines `
            -Lines $guidanceLines `
            -Item $Item `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson `
            -ContextText ('for release governance {0} `{1}`' -f $ItemKind, $id)
    } elseif ([string]::Equals($action, "rerun_pdf_visual_release_gate_preflight", [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Use action `{0}` for release governance {1} `{2}`: regenerate the PDF visual release gate preflight summary, then rebuild the PDF preflight governance report.' -f $action, $ItemKind, $id)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightBlockingSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightBuildCandidateSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightDependencyInputSummaryLine -Item $Item)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text (Get-PdfVisualPreflightReadinessActionEvidenceLine -Item $Item)
        $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
        if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
            $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir .\.bpdf-roundtrip-msvc -OutputJson .\output\pdf-visual-release-gate-preflight-governance\preflight-summary.json'
        }
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('Run the lightweight PDF preflight regeneration command: `{0}`' -f $commandTemplate)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text 'A regenerated PDF preflight summary only refreshes prerequisite evidence; it is not release-ready evidence until the full PDF visual release gate passes.'
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text ('After the preflight summary is readable, rerun `write_pdf_visual_release_gate_preflight_governance_report.ps1`, rerun release governance checks, and regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
        Add-ReleaseBlockerActionGuidanceLine `
            -Lines $guidanceLines `
            -Text 'After each PDF preflight or gate attempt, clean up only task-owned PDF gate processes and transient outputs after capturing the required evidence; do not terminate unrelated external build, Office, browser, node, or PowerShell processes.'
    } elseif ([string]::Equals($action, "restore_docx_functional_smoke_evidence", [System.StringComparison]::OrdinalIgnoreCase)) {
        Add-DocxFunctionalSmokeEvidenceGuidanceLines `
            -Lines $guidanceLines `
            -Item $Item `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson `
            -ContextText ('for release governance {0} `{1}`' -f $ItemKind, $id)
    } elseif ($action -in @(
            "resolve_pending_schema_approvals",
            "fix_invalid_approval_records",
            "add_explicit_confidence_metadata",
            "review_schema_patch_confidence_calibration_evidence"
        )) {
        Add-SchemaPatchConfidenceCalibrationGuidanceLines `
            -Lines $guidanceLines `
            -Item $Item `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson `
            -ContextText ('for release governance {0} `{1}`' -f $ItemKind, $id)
    } elseif ($action -in @(
            "fix_custom_xml_data_binding_source",
            "sync_or_fill_bound_content_control",
            "review_content_control_lock_strategy",
            "review_unbound_form_content_control",
            "review_duplicate_content_control_binding",
            "collect_content_control_data_binding_evidence",
            "review_content_control_data_binding_evidence",
            "run_content_control_custom_xml_sync"
        )) {
        Add-ContentControlDataBindingGovernanceGuidanceLines `
            -Lines $guidanceLines `
            -Item $Item `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson `
            -ContextText ('for release governance {0} `{1}`' -f $ItemKind, $id)
    } elseif ($action -in @(
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
        )) {
        Add-ProjectTemplateGovernanceGuidanceLines `
            -Lines $guidanceLines `
            -Item $Item `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson `
            -ContextText ('for release governance {0} `{1}`' -f $ItemKind, $id)
    } elseif ($action -in @(
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
        )) {
        Add-NumberingCatalogGovernanceGuidanceLines `
            -Lines $guidanceLines `
            -Item $Item `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson `
            -ContextText ('for release governance {0} `{1}`' -f $ItemKind, $id)
    } elseif ($action -in @(
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
        )) {
        Add-TableLayoutDeliveryGovernanceGuidanceLines `
            -Lines $guidanceLines `
            -Item $Item `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson `
            -ContextText ('for release governance {0} `{1}`' -f $ItemKind, $id)
    }

    $hadCommand = $false
    foreach ($commandName in @("command", "open_command", "audit_command", "review_command")) {
        $commandValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $commandName
        if (-not [string]::IsNullOrWhiteSpace($commandValue)) {
            $hadCommand = $true
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Run `{0}` for release governance {1} `{2}`: `{3}`' -f $commandName, $ItemKind, $id, $commandValue)
        }
    }

    foreach ($repairField in @("repair_strategy", "repair_hint", "command_template")) {
        $repairValue = Get-ReleaseBlockerPropertyValue -Object $Item -Name $repairField
        if (-not [string]::IsNullOrWhiteSpace($repairValue)) {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Review `{0}` for release governance {1} `{2}`: {3}' -f $repairField, $ItemKind, $id, $repairValue)
        }
    }

    if (-not $hadCommand) {
        if ([string]::IsNullOrWhiteSpace($action)) {
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Action is missing for release governance {0} `{1}` from source_schema `{2}`; update the owning governance report, rerun release governance checks, and regenerate the release note bundle from `{3}` before publishing.' -f $ItemKind, $id, $sourceSchema, $releaseSummaryDisplay)
        } else {
            if ([string]::Equals($ItemKind, "warning", [System.StringComparison]::OrdinalIgnoreCase) -and
                [string]::Equals($action, "review_style_merge_suggestions", [System.StringComparison]::OrdinalIgnoreCase)) {
                Add-ReleaseBlockerActionGuidanceLine `
                    -Lines $guidanceLines `
                    -Text ('Use action `{0}` for release governance warning `{1}` before publishing.' -f $action, $id)
                $pendingCount = Get-ReleaseBlockerPropertyObject -Object $Item -Name "style_merge_suggestion_pending_count"
                if ($null -ne $pendingCount -and -not [string]::IsNullOrWhiteSpace([string]$pendingCount)) {
                    Add-ReleaseBlockerActionGuidanceLine `
                        -Lines $guidanceLines `
                        -Text ('Current pending style merge suggestion count is `{0}`.' -f [string]$pendingCount)
                }
            }

            $actionLabel = if ([string]::Equals($ItemKind, "warning", [System.StringComparison]::OrdinalIgnoreCase)) {
                "warning action"
            } elseif ([string]::Equals($ItemKind, "blocker", [System.StringComparison]::OrdinalIgnoreCase)) {
                "blocker action"
            } else {
                "action"
            }
            Add-ReleaseBlockerActionGuidanceLine `
                -Lines $guidanceLines `
                -Text ('Follow {0} `{1}` for release governance {2} `{3}` from source_schema `{4}`: update the owning governance evidence, rerun release governance checks, and regenerate the release note bundle from `{5}` before publishing.' -f $actionLabel, $action, $ItemKind, $id, $sourceSchema, $releaseSummaryDisplay)
        }
    }

    return $guidanceLines.ToArray()
}

function Get-ReleaseGovernanceBlockerActionGuidanceLines {
    param(
        [AllowNull()]$Blocker,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $Blocker `
            -ItemKind "blocker" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Get-ReleaseGovernanceActionItemActionGuidanceLines {
    param(
        [AllowNull()]$ActionItem,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $ActionItem `
            -ItemKind "action item" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Get-ReleaseGovernanceWarningActionGuidanceLines {
    param(
        [AllowNull()]$Warning,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = ""
    )

    return @(Get-ReleaseGovernanceChecklistGuidanceLines `
            -Item $Warning `
            -ItemKind "warning" `
            -RepoRoot $RepoRoot `
            -ReleaseSummaryJson $ReleaseSummaryJson)
}

function Add-ReleaseGovernanceMetricsMarkdownSection {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary,
        [string]$Heading = "### Governance Metrics"
    )

    $metrics = @(Get-ReleaseBlockerArrayProperty -Object $Summary -Name "governance_metrics")
    if ($metrics.Count -eq 0) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add($Heading)
    [void]$Lines.Add("")
    foreach ($metric in $metrics) {
        $reportId = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "report_id")
        $metricName = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "metric")
        $metricId = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "id")
        $level = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "level")
        $score = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "score")
        $sourceSchema = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_schema")
        $sourceReport = Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_report"
        $sourceJson = Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_json"
        $sourceReportDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_report_display")
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $metric -Name "source_json_display")

        [void]$Lines.Add("- ${metricId}: report=$reportId metric=$metricName level=$level score=$score source_schema=$sourceSchema")
        if (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
            [void]$Lines.Add("  - source_report: $sourceReport")
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
            [void]$Lines.Add("  - source_json: $sourceJson")
        }
        [void]$Lines.Add("  - source_report_display: $sourceReportDisplay")
        [void]$Lines.Add("  - source_json_display: $sourceJsonDisplay")
        Add-ReleaseGovernanceMetricDetailLines -Lines $Lines -Metric $metric
    }
}
