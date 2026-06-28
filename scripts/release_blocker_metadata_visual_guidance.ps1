function Add-ReleaseBlockerActionGuidanceLine {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [string]$Text
    )

    if (-not [string]::IsNullOrWhiteSpace($Text)) {
        [void]$Lines.Add($Text)
    }
}

function Get-PdfVisualPreflightBlockingSummaryLine {
    param([AllowNull()]$Item)

    $blockingSummary = Get-ReleaseBlockerPropertyObject -Object $Item -Name "blocking_summary"
    if ($null -eq $blockingSummary) {
        return ""
    }

    $requiredCheckCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "required_check_count"
    $blockingCheckCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "blocking_check_count"
    $missingCliPdfCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "missing_cli_pdf_count"
    $visualBaselineSampleCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "visual_baseline_sample_count"
    $missingVisualBaselinePdfCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "missing_visual_baseline_pdf_count"
    $cjkTextLayerSampleCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "cjk_text_layer_sample_count"
    $missingCjkTextLayerPdfCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "missing_cjk_text_layer_pdf_count"
    $buildDirEntryCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "build_dir_entry_count"
    $ctestRequiredPatternCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "ctest_required_pattern_count"
    $outputGapCount = Get-ReleaseBlockerIntPropertyValue -Object $Item -Name "output_gap_count"
    $missingOutputCount = Get-ReleaseBlockerIntPropertyValue -Object $Item -Name "missing_output_count"
    $memoryGuardBlocked = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "memory_guard_blocked"
    $memoryGuardSkipped = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "memory_guard_skipped"
    $freeMemoryMb = Get-ReleaseBlockerPropertyValue -Object $blockingSummary -Name "free_memory_mb"
    $minFreeMemoryMb = Get-ReleaseBlockerPropertyValue -Object $blockingSummary -Name "min_free_memory_mb"
    $pdfBuildOptionsEnabled = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "pdf_build_options_enabled"
    $disabledPdfBuildOptions = @(Get-ReleaseBlockerArrayProperty -Object $blockingSummary -Name "disabled_pdf_build_options" | ForEach-Object { [string]$_ })
    $missingPdfBuildOptions = @(Get-ReleaseBlockerArrayProperty -Object $blockingSummary -Name "missing_pdf_build_options" | ForEach-Object { [string]$_ })

    if (($requiredCheckCount + $blockingCheckCount + $missingCliPdfCount +
            $visualBaselineSampleCount + $missingVisualBaselinePdfCount +
            $cjkTextLayerSampleCount + $missingCjkTextLayerPdfCount +
            $buildDirEntryCount + $ctestRequiredPatternCount +
            $outputGapCount + $missingOutputCount) -le 0 -and
        [string]::IsNullOrWhiteSpace($memoryGuardBlocked) -and
        [string]::IsNullOrWhiteSpace($memoryGuardSkipped) -and
        [string]::IsNullOrWhiteSpace($freeMemoryMb) -and
        [string]::IsNullOrWhiteSpace($minFreeMemoryMb) -and
        [string]::IsNullOrWhiteSpace($pdfBuildOptionsEnabled) -and
        $disabledPdfBuildOptions.Count -eq 0 -and
        $missingPdfBuildOptions.Count -eq 0) {
        return ""
    }

    $summaryLine = ("PDF preflight blocker summary: required checks={0}, blocking checks={1}, missing CLI PDFs={2}, visual baseline samples={3}, missing visual baseline PDFs={4}, CJK text-layer samples={5}, missing CJK text-layer PDFs={6}, build dir entries={7}, CTest required patterns={8}, output gap checks={9}, missing outputs={10}." -f `
        $requiredCheckCount,
        $blockingCheckCount,
        $missingCliPdfCount,
        $visualBaselineSampleCount,
        $missingVisualBaselinePdfCount,
        $cjkTextLayerSampleCount,
        $missingCjkTextLayerPdfCount,
        $buildDirEntryCount,
        $ctestRequiredPatternCount,
        $outputGapCount,
        $missingOutputCount)
    $memoryParts = @()
    if (-not [string]::IsNullOrWhiteSpace($memoryGuardBlocked)) {
        $memoryParts += "memory guard blocked=$memoryGuardBlocked"
    }
    if (-not [string]::IsNullOrWhiteSpace($memoryGuardSkipped)) {
        $memoryParts += "memory guard skipped=$memoryGuardSkipped"
    }
    if (-not [string]::IsNullOrWhiteSpace($freeMemoryMb)) {
        $memoryParts += "free memory MB=$freeMemoryMb"
    }
    if (-not [string]::IsNullOrWhiteSpace($minFreeMemoryMb)) {
        $memoryParts += "minimum free memory MB=$minFreeMemoryMb"
    }
    if ($memoryParts.Count -gt 0) {
        $summaryLine += " Memory guard: $($memoryParts -join ', ')."
    }
    $pdfBuildOptionParts = @()
    if (-not [string]::IsNullOrWhiteSpace($pdfBuildOptionsEnabled)) {
        $pdfBuildOptionParts += "enabled=$pdfBuildOptionsEnabled"
    }
    if ($disabledPdfBuildOptions.Count -gt 0) {
        $pdfBuildOptionParts += "disabled=$($disabledPdfBuildOptions -join ',')"
    }
    if ($missingPdfBuildOptions.Count -gt 0) {
        $pdfBuildOptionParts += "missing=$($missingPdfBuildOptions -join ',')"
    }
    if ($pdfBuildOptionParts.Count -gt 0) {
        $summaryLine += " PDF build options: $($pdfBuildOptionParts -join ', ')."
    }

    return $summaryLine
}

function Get-PdfVisualPreflightBuildCandidateSummaryLine {
    param([AllowNull()]$Item)

    $candidates = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "build_dir_auto_candidates")
    if ($candidates.Count -eq 0) {
        return ""
    }

    $candidateParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($candidate in $candidates) {
        $relativePath = Get-ReleaseBlockerPropertyValue -Object $candidate -Name "relative_path"
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            $relativePath = Get-ReleaseBlockerPropertyValue -Object $candidate -Name "path"
        }
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            $relativePath = "(unknown)"
        }

        $stateParts = @()
        foreach ($stateName in @(
                [ordered]@{ Label = "exists"; Name = "exists" },
                [ordered]@{ Label = "CMakeCache"; Name = "cmake_cache_exists" },
                [ordered]@{ Label = "CTest"; Name = "ctest_manifest_exists" },
                [ordered]@{ Label = "PDF options"; Name = "pdf_build_options_enabled" },
                [ordered]@{ Label = "reusable"; Name = "looks_reusable" }
            )) {
            $value = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $candidate -Name $stateName.Name
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                $stateParts += "$($stateName.Label)=$value"
            }
        }

        $pdfOptionParts = @()
        foreach ($option in @(Get-ReleaseBlockerArrayProperty -Object $candidate -Name "pdf_build_options")) {
            $name = Get-ReleaseBlockerPropertyValue -Object $option -Name "name"
            if ([string]::IsNullOrWhiteSpace($name)) {
                continue
            }

            $present = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $option -Name "present"
            $value = Get-ReleaseBlockerPropertyValue -Object $option -Name "value"
            $enabled = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $option -Name "enabled"
            $optionStateParts = @()
            if (-not [string]::IsNullOrWhiteSpace($present)) {
                $optionStateParts += "present=$present"
            }
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                $optionStateParts += "value=$value"
            }
            if (-not [string]::IsNullOrWhiteSpace($enabled)) {
                $optionStateParts += "enabled=$enabled"
            }
            if ($optionStateParts.Count -gt 0) {
                $pdfOptionParts += "$name($($optionStateParts -join ','))"
            } else {
                $pdfOptionParts += $name
            }
        }

        if ($pdfOptionParts.Count -gt 0) {
            $stateParts += "options=$($pdfOptionParts -join ';')"
        }

        if ($stateParts.Count -gt 0) {
            $candidateParts.Add("$relativePath($($stateParts -join ', '))") | Out-Null
        } else {
            $candidateParts.Add($relativePath) | Out-Null
        }
    }

    return "PDF preflight build auto candidates: $($candidateParts.ToArray() -join '; ')."
}

function Get-PdfVisualPreflightDependencyInputSummaryLine {
    param([AllowNull()]$Item)

    $dependencyInputs = Get-ReleaseBlockerPropertyObject -Object $Item -Name "pdf_dependency_inputs"
    $blockingSummary = Get-ReleaseBlockerPropertyObject -Object $Item -Name "blocking_summary"
    $status = Get-ReleaseBlockerPropertyValue -Object $dependencyInputs -Name "status"
    if ([string]::IsNullOrWhiteSpace($status)) {
        $status = Get-ReleaseBlockerPropertyValue -Object $blockingSummary -Name "pdf_dependency_inputs_status"
    }
    $selectedPdfiumProvider = Get-ReleaseBlockerPropertyValue -Object $dependencyInputs -Name "selected_pdfium_provider"
    if ([string]::IsNullOrWhiteSpace($selectedPdfiumProvider)) {
        $selectedPdfiumProvider = Get-ReleaseBlockerPropertyValue -Object $blockingSummary -Name "selected_pdfium_provider"
    }
    $pdfioReady = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $dependencyInputs -Name "pdfio_ready"
    if ([string]::IsNullOrWhiteSpace($pdfioReady)) {
        $pdfioReady = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "pdfio_dependency_ready"
    }
    $pdfiumReady = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $dependencyInputs -Name "pdfium_ready"
    if ([string]::IsNullOrWhiteSpace($pdfiumReady)) {
        $pdfiumReady = Get-ReleaseBlockerBoolPropertyDisplayValue -Object $blockingSummary -Name "pdfium_dependency_ready"
    }

    $missingInputCount = Get-ReleaseBlockerIntPropertyValue -Object $dependencyInputs -Name "missing_input_count" -DefaultValue -1
    if ($missingInputCount -lt 0) {
        $missingInputCount = Get-ReleaseBlockerIntPropertyValue -Object $blockingSummary -Name "pdf_dependency_missing_input_count" -DefaultValue -1
    }
    $missingInputs = @(Get-ReleaseBlockerArrayProperty -Object $dependencyInputs -Name "missing_inputs" | ForEach-Object { [string]$_ })
    if ($missingInputs.Count -eq 0) {
        $missingInputs = @(Get-ReleaseBlockerArrayProperty -Object $blockingSummary -Name "pdf_dependency_missing_inputs_preview" | ForEach-Object { [string]$_ })
    }

    $parts = @()
    if (-not [string]::IsNullOrWhiteSpace($status)) {
        $parts += "status=$status"
    }
    if (-not [string]::IsNullOrWhiteSpace($selectedPdfiumProvider)) {
        $parts += "selected PDFium provider=$selectedPdfiumProvider"
    }
    if (-not [string]::IsNullOrWhiteSpace($pdfioReady)) {
        $parts += "PDFio ready=$pdfioReady"
    }
    if (-not [string]::IsNullOrWhiteSpace($pdfiumReady)) {
        $parts += "PDFium ready=$pdfiumReady"
    }
    if ($missingInputCount -ge 0) {
        $parts += "missing inputs=$missingInputCount"
    }
    if ($missingInputs.Count -gt 0) {
        $publicMissingInputs = @(
            $missingInputs |
                Select-Object -First 5 |
                ForEach-Object { Convert-ReleaseBlockerLocalPathsForPublicText -Text ([string]$_) }
        )
        $parts += "missing input preview=$($publicMissingInputs -join '; ')"
    }

    if ($parts.Count -eq 0) {
        return ""
    }

    return "PDF dependency inputs: $($parts -join ', ')."
}

function Get-PdfVisualPreflightReadinessActionEvidenceLine {
    param([AllowNull()]$Item)

    $evidenceItems = @(Get-ReleaseBlockerArrayProperty -Object $Item -Name "readiness_action_evidence")
    if ($evidenceItems.Count -eq 0) {
        return ""
    }

    $evidenceParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($evidence in $evidenceItems) {
        $action = Get-ReleaseBlockerDisplayValue `
            -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "action") `
            -Fallback "(unknown action)"
        $issueKey = Get-ReleaseBlockerDisplayValue `
            -Value (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "issue_key") `
            -Fallback "(unknown issue)"
        $item = Convert-ReleaseBlockerLocalPathsForPublicText `
            -Text (Get-ReleaseBlockerPropertyValue -Object $evidence -Name "item")

        $part = "$action/$issueKey"
        if (-not [string]::IsNullOrWhiteSpace($item)) {
            $part += " -> $item"
        }

        [void]$evidenceParts.Add($part)
    }

    $summaryLine = "Readiness action evidence: $($evidenceParts.ToArray() -join '; ')."
    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        foreach ($evidence in $evidenceItems) {
            $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $evidence -Name "source_json_display"
            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                break
            }
        }
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $summaryLine += " source JSON: $sourceJsonDisplay"
    }

    return $summaryLine
}

function Get-ReleaseBlockerRegisteredActions {
    return @(
        "apply_safe_tblLook_fixes",
        "apply_safe_tblLook_fixes_then_visual_regression",
        "complete_visual_manual_review",
        "add_explicit_confidence_metadata",
        "collect_content_control_data_binding_evidence",
        "approve_project_template_schema",
        "collect_project_template_onboarding_governance_evidence",
        "dry_run_apply_table_position_plans",
        "fix_schema_patch_approval_result",
        "fix_custom_xml_data_binding_source",
        "fix_invalid_approval_records",
        "fix_numbering_catalog_baseline_lint",
        "freeze_schema_baseline",
        "promote_schema_update_candidate",
        "promote_numbering_catalog_exemplar",
        "preview_style_numbering_repair",
        "prepare_pdf_visual_release_gate_build_outputs",
        "rebuild_table_layout_delivery_rollup",
        "rebuild_document_skeleton_governance_rollup",
        "rebuild_numbering_catalog_manifest_summary",
        "rerun_pdf_controlled_visual_smoke_check",
        "rerun_pdf_visual_release_gate_preflight",
        "rerun_document_skeleton_governance_report",
        "review_content_control_data_binding_evidence",
        "review_content_control_lock_strategy",
        "review_duplicate_content_control_binding",
        "review_floating_table_position_plans",
        "review_manual_table_style_definition_work",
        "refresh_numbering_catalog_baseline_or_repair_docx",
        "register_numbering_catalog_baseline",
        "review_numbering_catalog_check_issues",
        "review_numbering_catalog_governance_sources",
        "review_numbering_catalog_real_corpus_alignment",
        "review_project_template_delivery_readiness_evidence",
        "review_project_template_smoke_failure",
        "review_schema_approval_history",
        "review_schema_baseline",
        "review_table_layout_delivery_governance_sources",
        "review_table_position_plan",
        "review_table_style_quality_plan",
        "review_unbound_form_content_control",
        "resolve_pending_schema_approvals",
        "add_business_template_source_metadata",
        "add_business_template_document_type_metadata",
        "add_business_template_corpus_role_metadata",
        "align_business_template_corpus_metadata",
        "review_schema_patch_confidence_calibration_evidence",
        "review_schema_update_candidate",
        "review_style_numbering_audit",
        "run_content_control_custom_xml_sync",
        "run_table_style_quality_visual_regression",
        "run_project_template_smoke_for_registered_manifest",
        "run_project_template_smoke_then_review_schema_patch_approval",
        "sync_or_fill_bound_content_control",
        "update_template_or_schema_before_retry",
        "restore_docx_functional_smoke_evidence"
    )
}

function Test-ReleaseBlockerActionRegistered {
    param([string]$Action)

    if ([string]::IsNullOrWhiteSpace($Action)) {
        return $false
    }

    foreach ($registeredAction in @(Get-ReleaseBlockerRegisteredActions)) {
        if ([string]::Equals($registeredAction, $Action, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $true
        }
    }

    return $false
}

function Add-PdfControlledVisualSmokeCheckGuidanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = "",
        [string]$ContextText = ""
    )

    $contextSuffix = if ([string]::IsNullOrWhiteSpace($ContextText)) { "" } else { " $ContextText" }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `rerun_pdf_controlled_visual_smoke_check`{0}: rerun the controlled PDF visual smoke check against existing PNG/text evidence, then rebuild the PDF preflight governance report.' -f $contextSuffix)

    $status = Get-ReleaseBlockerPropertyValue -Object $Item -Name "status"
    $errorMessage = Get-ReleaseBlockerPropertyValue -Object $Item -Name "error_message"
    if (-not [string]::IsNullOrWhiteSpace($status) -or -not [string]::IsNullOrWhiteSpace($errorMessage)) {
        $statusLine = "Controlled PDF visual smoke status"
        if (-not [string]::IsNullOrWhiteSpace($status)) {
            $statusLine += ": $status"
        }
        if (-not [string]::IsNullOrWhiteSpace($errorMessage)) {
            $statusLine += "; error: $errorMessage"
        }
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text $statusLine
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Inspect the controlled PDF visual smoke JSON before rerunning: {0}" -f $sourceJsonDisplay)
    }

    $sourceReportDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report_display"
    if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        $sourceReportDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_report")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Open the PDF preflight governance report that emitted the smoke warning: {0}" -f $sourceReportDisplay)
    }

    $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        $commandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_controlled_visual_smoke.ps1 -Root .\output\pdf-controlled-visual-smoke-20260520 -OutputJson .\output\pdf-controlled-visual-smoke-20260520\controlled-visual-smoke-check-latest.json'
    }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Run the controlled smoke check command: `{0}`' -f $commandTemplate)
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'The controlled smoke check is read-only: it validates existing PNG/text evidence and does not run CMake, CTest, Word, LibreOffice, browser automation, PDF rendering, virtual environment creation, or dependency installation.'
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'A passing controlled smoke check refreshes only low-resource evidence; it is not release-ready evidence until the full PDF visual release gate and release governance checks pass.'

    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($releaseSummaryDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'After the controlled smoke JSON is passing, rerun `write_pdf_visual_release_gate_preflight_governance_report.ps1`, rebuild the release blocker rollup, and regenerate the release note bundle before publishing.'
    } else {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('After the controlled smoke JSON is passing, rerun `write_pdf_visual_release_gate_preflight_governance_report.ps1`, rerun release governance checks, and regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
    }
}

function Add-DocxFunctionalSmokeEvidenceGuidanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Item,
        [string]$RepoRoot = "",
        [string]$ReleaseSummaryJson = "",
        [string]$ContextText = ""
    )

    $contextSuffix = if ([string]::IsNullOrWhiteSpace($ContextText)) { "" } else { " $ContextText" }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Use action `restore_docx_functional_smoke_evidence`{0}: restore the missing persisted DOCX functional smoke evidence, then rerun the low-resource DOCX readiness check.' -f $contextSuffix)

    $status = Get-ReleaseBlockerPropertyValue -Object $Item -Name "status"
    $message = Get-ReleaseBlockerPropertyValue -Object $Item -Name "message"
    if (-not [string]::IsNullOrWhiteSpace($status) -or -not [string]::IsNullOrWhiteSpace($message)) {
        $statusLine = "DOCX functional smoke blocker"
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
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Open the DOCX functional smoke readiness report first: {0}" -f $sourceReportDisplay)
    }

    $sourceJsonDisplay = Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json_display"
    if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        $sourceJsonDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path (Get-ReleaseBlockerPropertyValue -Object $Item -Name "source_json")
    }
    if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ("Inspect the DOCX readiness source JSON before changing evidence: {0}" -f $sourceJsonDisplay)
    }

    $commandTemplate = Get-ReleaseBlockerPropertyValue -Object $Item -Name "command_template"
    if ([string]::IsNullOrWhiteSpace($commandTemplate)) {
        $commandTemplate = 'powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\check_docx_functional_smoke_readiness.ps1 -RepoRoot . -OutputDir .\output\docx-functional-smoke-readiness-current'
    }
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('Run the low-resource DOCX readiness command: `{0}`' -f $commandTemplate)
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'The DOCX functional smoke readiness check is read-only: it validates persisted package, feature, and Word visual smoke PNG evidence and does not run CMake, CTest, Word, LibreOffice, browser automation, document rendering, virtual environment creation, or dependency installation.'
    Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'A passing DOCX readiness check refreshes only persisted DOCX functional smoke evidence; it is not a fresh Word COM render and is not release-ready visual evidence until the required visual review or release gate also passes.'

    $releaseSummaryDisplay = Get-ReleaseBlockerDisplayPath -RepoRoot $RepoRoot -Path $ReleaseSummaryJson
    if ([string]::IsNullOrWhiteSpace($releaseSummaryDisplay)) {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text 'After DOCX readiness is passing, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle before publishing.'
    } else {
        Add-ReleaseBlockerActionGuidanceLine -Lines $Lines -Text ('After DOCX readiness is passing, rebuild release governance pipeline and handoff evidence, then regenerate the release note bundle from `{0}` before publishing.' -f $releaseSummaryDisplay)
    }
}
