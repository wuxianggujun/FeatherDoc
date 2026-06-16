function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-HasProperty {
    param($Object, [string]$Name, [string]$Message)
    if (-not ($Object.PSObject.Properties.Name -contains $Name)) {
        throw "$Message Missing property '$Name'."
    }
}

function Assert-NonEmptyString {
    param($Value, [string]$Message)
    if ([string]::IsNullOrWhiteSpace([string]$Value)) {
        throw $Message
    }
}

function Assert-MatchesText {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) {
        throw "$Message Pattern='$Pattern'."
    }
}

function Get-StageById {
    param($Summary, [string]$Id)

    $matches = @($Summary.stages | Where-Object { [string]$_.id -eq $Id })
    Assert-Equal -Actual $matches.Count -Expected 1 `
        -Message "Pipeline should include exactly one stage $Id."
    return $matches[0]
}

function Assert-StageGovernanceItemSourceTrace {
    param(
        $Stage,
        [string]$PropertyName,
        [bool]$ExpectOpenCommandProperty = $false
    )

    foreach ($item in @($Stage.$PropertyName)) {
        $itemId = [string]$item.id
        $context = "Pipeline stage $($Stage.id) $PropertyName item $itemId"

        foreach ($property in @("stage_id", "stage_title", "source_report_display", "source_json_display")) {
            Assert-HasProperty -Object $item -Name $property `
                -Message "$context should expose $property."
        }

        Assert-Equal -Actual ([string]$item.stage_id) -Expected ([string]$Stage.id) `
            -Message "$context should keep its parent stage id."
        Assert-Equal -Actual ([string]$item.stage_title) -Expected ([string]$Stage.title) `
            -Message "$context should keep its parent stage title."
        Assert-NonEmptyString -Value $item.source_report_display `
            -Message "$context should keep a source report display path."
        Assert-NonEmptyString -Value $item.source_json_display `
            -Message "$context should keep a source JSON display path."

        if ($ExpectOpenCommandProperty) {
            Assert-HasProperty -Object $item -Name "open_command" `
                -Message "$context should expose reviewer open command metadata."
            Assert-NonEmptyString -Value $item.open_command `
                -Message "$context should keep a non-empty reviewer open command."
        }
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-PngFixture {
    param([string]$Path)

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    $pngBase64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGNkaGAAAAJMAJj6eGpbAAAAAElFTkSuQmCC"
    $pngBytes = [System.Convert]::FromBase64String($pngBase64)
    $paddedBytes = New-Object byte[] 2048
    [System.Array]::Copy($pngBytes, $paddedBytes, $pngBytes.Length)
    [System.IO.File]::WriteAllBytes($Path, $paddedBytes)
}

function Write-DocxVisualSmokeFixture {
    param([string]$Root)

    $reportDir = Join-Path $Root "report"
    $evidenceDir = Join-Path $Root "evidence"
    $pageDir = Join-Path $evidenceDir "pages"
    New-Item -ItemType Directory -Path $reportDir, $pageDir -Force | Out-Null
    Write-JsonFile -Path (Join-Path $reportDir "summary.json") -Value ([ordered]@{
            status = "pass"
            page_count = 1
        })
    Write-JsonFile -Path (Join-Path $reportDir "review_result.json") -Value ([ordered]@{
            status = "reviewed"
            verdict = "pass"
        })
    Set-Content -LiteralPath (Join-Path $Root "table_visual_smoke.pdf") -Encoding ASCII -Value "%PDF-1.4`n% fixture"
    Write-PngFixture -Path (Join-Path $evidenceDir "contact_sheet.png")
    Write-PngFixture -Path (Join-Path $pageDir "page-01.png")
}

function New-SkeletonRollup {
    return [ordered]@{
        schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
        status = "needs_review"
        clean = $false
        document_count = 1
        total_style_numbering_issue_count = 1
        total_style_numbering_suggestion_count = 1
        total_numbering_definition_count = 2
        total_numbering_instance_count = 3
        total_style_usage_count = 4
        total_command_failure_count = 0
        issue_summary = @([ordered]@{ issue = "missing_numbering_definition"; count = 1 })
        catalog_exemplars = @(
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                input_docx_display = ".\samples\contract.docx"
                exemplar_catalog_path = "output/document-skeleton-governance/contract/exemplar.numbering-catalog.json"
                exemplar_catalog_display = ".\output\document-skeleton-governance\contract\exemplar.numbering-catalog.json"
                definition_count = 2
                instance_count = 3
            }
        )
        release_blockers = @(
            [ordered]@{
                id = "document_skeleton.style_numbering_issues"
                document_name = "contract.docx"
                severity = "error"
                status = "needs_review"
                action = "review_style_numbering_audit"
                message = "Style numbering audit reported issues."
            }
        )
        action_items = @(
            [ordered]@{
                id = "review_style_numbering_audit"
                document_name = "contract.docx"
                action = "review_style_numbering_audit"
                title = "Review contract style numbering audit"
                audit_command = "featherdoc_cli audit-style-numbering samples/contract.docx --json"
                review_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_document_skeleton_governance_rollup_report.ps1"
            },
            [ordered]@{
                id = "promote_numbering_catalog_exemplar"
                document_name = "contract.docx"
                action = "promote_numbering_catalog_exemplar"
                title = "Review and promote the generated exemplar numbering catalog"
                command = "featherdoc_cli check-numbering-catalog samples/contract.docx --json"
                category = "release_checklist"
                severity = "info"
                release_blocking = $false
                optional = $true
            },
            [ordered]@{
                id = "register_numbering_catalog_baseline"
                document_name = "contract.docx"
                action = "register_numbering_catalog_baseline"
                title = "Register the exemplar catalog in the numbering catalog baseline flow"
                command = "pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx samples/contract.docx"
                category = "release_checklist"
                severity = "info"
                release_blocking = $false
                optional = $true
            }
        )
    }
}

function New-NumberingManifest {
    return [ordered]@{
        generated_at = "2026-05-03T00:00:00"
        manifest_path = "baselines/numbering-catalog/manifest.json"
        entry_count = 1
        drift_count = 0
        dirty_baseline_count = 0
        issue_entry_count = 0
        passed = $true
        entries = @(
            [ordered]@{
                name = "contract"
                input_docx = "samples/contract.docx"
                matches = $true
                clean = $true
                catalog_file = "baselines/numbering-catalog/contract.json"
                catalog_lint_clean = $true
                catalog_lint_issue_count = 0
                generated_output_path = "output/numbering-catalog-manifest-checks/contract.generated.numbering-catalog.json"
                baseline_issue_count = 0
                generated_issue_count = 0
                added_definition_count = 0
                removed_definition_count = 0
                changed_definition_count = 0
            }
        )
    }
}

function New-TableLayoutRollup {
    return [ordered]@{
        schema = "featherdoc.table_layout_delivery_rollup_report.v1"
        status = "needs_review"
        ready = $false
        document_count = 1
        document_entries = @(
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                input_docx_display = ".\samples\contract.docx"
                status = "needs_review"
                ready = $false
                preset = "margin-anchor"
                table_style_issue_count = 1
                automatic_tblLook_fix_count = 1
                manual_table_style_fix_count = 1
                table_position_automatic_count = 0
                table_position_review_count = 1
                table_position_already_matching_count = 0
                pdf_floating_table_support_status = "partial"
                pdf_floating_table_layout_boundary = "docx-table-position metadata"
                pdf_floating_table_supported_geometry_count = 4
                pdf_floating_table_metadata_only_count = 4
                pdf_floating_table_tracked_geometry_count = 9
                pdf_floating_table_supported_geometry_percent = 44
                command_failure_count = 0
                table_position_plan_path = "output/table-layout-delivery/contract/table-position.plan.json"
                table_position_plan_display = ".\output\table-layout-delivery\contract\table-position.plan.json"
            }
        )
        pdf_floating_table_support = @(
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                status = "partial"
                boundary = "docx-table-position metadata"
                supported_geometry_count = 4
                metadata_only_count = 4
                tracked_geometry_count = 9
                supported_geometry_percent = 44
                review_required_count = 2
                supported_geometry = @(
                    "tblpPr anchor metadata",
                    "horizontal absolute positioning",
                    "vertical absolute positioning",
                    "cell-margin stable geometry"
                )
                metadata_only = @(
                    "leftFromText",
                    "rightFromText",
                    "topFromText outside paragraph anchoring",
                    "tblOverlap"
                )
                review_required = @(
                    "full Word-compatible floating table text wrapping",
                    "table overlap avoidance and collision resolution"
                )
            }
        )
        table_position_plans = @(
            [ordered]@{
                document_name = "contract.docx"
                input_docx = "samples/contract.docx"
                preset = "margin-anchor"
                table_position_plan_path = "output/table-layout-delivery/contract/table-position.plan.json"
                table_position_plan_display = ".\output\table-layout-delivery\contract\table-position.plan.json"
                automatic_count = 0
                review_count = 1
                already_matching_count = 0
            }
        )
        total_table_style_issue_count = 1
        total_automatic_tblLook_fix_count = 1
        total_manual_table_style_fix_count = 1
        total_table_position_automatic_count = 0
        total_table_position_review_count = 1
        total_table_position_already_matching_count = 0
        total_command_failure_count = 0
        release_blockers = @(
            [ordered]@{
                id = "table_layout.positioned_tables_need_review"
                document_name = "contract.docx"
                severity = "warning"
                status = "needs_review"
                action = "review_table_position_plan"
                message = "Floating table position plan needs review."
            }
        )
        action_items = @(
            [ordered]@{
                id = "review_table_position_plan"
                document_name = "contract.docx"
                action = "review_table_position_plan"
                title = "Review floating table position plan"
            }
        )
    }
}

function New-OnboardingGovernance {
    return [ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = "blocked"
        release_ready = $false
        entries = @(
            [ordered]@{
                name = "contract-template"
                input_docx = "samples/contract.docx"
                source_kind = "onboarding_summary"
                schema_approval_state = [ordered]@{
                    status = "pending_review"
                    gate_status = "pending"
                    release_blocked = $true
                    action = "review_schema_update_candidate"
                }
                schema_approval_status = "pending_review"
                release_blocked = $true
                release_blockers = @(
                    [ordered]@{
                        id = "project_template_onboarding.schema_approval"
                        severity = "error"
                        status = "pending_review"
                        action = "review_schema_update_candidate"
                        message = "Schema approval is pending."
                    }
                )
                action_items = @(
                    [ordered]@{
                        id = "review_contract_schema"
                        action = "review_schema_update_candidate"
                        title = "Review contract schema"
                    }
                )
                manual_review_recommendations = @()
            }
        )
    }
}

function New-ProjectTemplateSmokeSummary {
    return [ordered]@{
        schema = "featherdoc.project_template_smoke_summary.v1"
        manifest_path = "samples/project_template_smoke.manifest.json"
        entry_count = 1
        overall_status = "passed"
        passed = $true
        failed_entry_count = 0
        entries = @(
            [ordered]@{
                name = "smoke-ready-template"
                input_docx = "samples/contract.docx"
                status = "passed"
                passed = $true
                checks = [ordered]@{}
            }
        )
        schema_patch_review_count = 1
        schema_patch_review_changed_count = 0
        schema_patch_reviews = @(
            [ordered]@{
                name = "smoke-ready-template"
                project_id = "project-finance"
                template_name = "smoke-ready-template"
                changed = $false
                baseline_slot_count = 2
                generated_slot_count = 2
                upsert_slot_count = 0
                remove_target_count = 0
                remove_slot_count = 0
                rename_slot_count = 0
                update_slot_count = 0
                inserted_slots = 0
                replaced_slots = 0
            }
        )
    }
}

function New-SchemaApprovalHistory {
    return [ordered]@{
        schema = "featherdoc.project_template_schema_approval_history.v1"
        generated_at = "2026-05-03T00:00:00"
        summary_count = 1
        latest_gate_status = "pending"
        blocked_run_count = 0
        pending_run_count = 1
        passed_run_count = 0
        entry_histories = @(
            [ordered]@{
                name = "smoke-ready-template"
                project_id = "project-finance"
                template_name = "smoke-ready-template"
                run_count = 1
                blocked_run_count = 0
                pending_run_count = 0
                approved_run_count = 1
                latest_generated_at = "2026-05-03T00:01:00"
                latest_status = "approved"
                latest_decision = "approved"
                latest_action = "none"
                latest_summary_json = "output/project-template-smoke/summary.json"
                issue_keys = @()
                runs = @()
            },
            [ordered]@{
                name = "contract-template"
                project_id = "project-finance"
                template_name = "invoice-template"
                run_count = 1
                blocked_run_count = 0
                pending_run_count = 1
                approved_run_count = 0
                latest_generated_at = "2026-05-03T00:00:00"
                latest_status = "pending_review"
                latest_decision = "pending"
                latest_action = "review_schema_update_candidate"
                latest_summary_json = "output/project-template-smoke/summary.json"
                issue_keys = @()
                runs = @(
                    [ordered]@{
                        project_id = "project-finance"
                        template_name = "invoice-template"
                        schema_patch_review_count = 1
                        schema_patch_review_changed_count = 1
                        schema_patch_reviews = @(
                            [ordered]@{
                                name = "contract-template"
                                project_id = "project-finance"
                                template_name = "invoice-template"
                                candidate_type = "rename"
                                changed = $true
                                baseline_slot_count = 2
                                generated_slot_count = 3
                                upsert_slot_count = 1
                                remove_target_count = 0
                                remove_slot_count = 0
                                rename_slot_count = 0
                                update_slot_count = 0
                                inserted_slots = 1
                                replaced_slots = 0
                            }
                        )
                        schema_patch_approval_items = @(
                            [ordered]@{
                                name = "contract-template"
                                project_id = "project-finance"
                                template_name = "invoice-template"
                                candidate_type = "rename"
                                status = "pending_review"
                                decision = "pending"
                                approved = $false
                                pending = $true
                                compliance_issue_count = 0
                            }
                        )
                    }
                )
            }
        )
    }
}

function New-ContentControlInspection {
    return [ordered]@{
        part = "body"
        entry_name = "word/document.xml"
        count = 2
        content_controls = @(
            [ordered]@{
                index = 0
                kind = "block"
                form_kind = "date"
                tag = "due_date"
                alias = "Due Date"
                id = "10"
                lock = "sdtLocked"
                data_binding_store_item_id = "{55555555-5555-5555-5555-555555555555}"
                data_binding_xpath = "/invoice/dueDate"
                data_binding_prefix_mappings = "xmlns:fd=`"urn:featherdoc`""
                checked = $null
                date_format = "yyyy/MM/dd"
                date_locale = "zh-CN"
                selected_list_item = $null
                list_items = @()
                showing_placeholder = $true
                text = "Due date placeholder"
            },
            [ordered]@{
                index = 1
                kind = "block"
                form_kind = "plain_text"
                tag = "due_date_copy"
                alias = "Due Date Copy"
                id = "11"
                lock = ""
                data_binding_store_item_id = "{55555555-5555-5555-5555-555555555555}"
                data_binding_xpath = "/invoice/dueDate"
                data_binding_prefix_mappings = ""
                checked = $null
                date_format = ""
                date_locale = ""
                selected_list_item = $null
                list_items = @()
                showing_placeholder = $false
                text = "Due date: 2026-07-15"
            }
        )
    }
}

function New-ContentControlSyncResult {
    return [ordered]@{
        scanned_content_controls = 2
        bound_content_controls = 2
        synced_content_controls = 1
        issue_count = 1
        synced_items = @(
            [ordered]@{
                part_entry_name = "word/document.xml"
                content_control_index = 1
                tag = "due_date_copy"
                alias = "Due Date Copy"
                store_item_id = "{55555555-5555-5555-5555-555555555555}"
                xpath = "/invoice/dueDate"
                previous_text = "old"
                value = "Due date: 2026-07-15"
            }
        )
        issues = @(
            [ordered]@{
                part_entry_name = "word/document.xml"
                content_control_index = 0
                tag = "due_date"
                alias = "Due Date"
                store_item_id = "{55555555-5555-5555-5555-555555555555}"
                xpath = "/invoice/missingDueDate"
                reason = "custom_xml_value_not_found"
            }
        )
    }
}

function New-PdfPreflightGovernance {
    return [ordered]@{
        schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
        status = "blocked"
        release_ready = $false
        release_blocker_count = 1
        release_blockers = @(
            [ordered]@{
                id = "pdf_visual_release_gate_preflight.build_outputs_missing"
                severity = "error"
                status = "blocked"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                message = "PDF visual release gate build outputs are missing."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "prepare_pdf_visual_release_gate_build_outputs"
                action = "prepare_pdf_visual_release_gate_build_outputs"
                title = "Prepare PDF visual release gate build outputs"
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_pdf_visual_release_gate_preflight_governance_report.ps1"
            }
        )
        warning_count = 1
        warnings = @(
            [ordered]@{
                id = "pdf_controlled_visual_smoke.unavailable_or_failed"
                action = "rerun_pdf_controlled_visual_smoke_check"
                status = "fail"
                message = "Controlled PDF visual smoke evidence was provided but is not passing."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\controlled-visual-smoke-failed.json"
            }
        )
    }
}

function New-InputFixture {
    param([string]$Root)

    Write-JsonFile -Path (Join-Path $Root "document-skeleton-governance-rollup\summary.json") -Value (New-SkeletonRollup)
    Write-JsonFile -Path (Join-Path $Root "numbering-catalog-manifest-checks\summary.json") -Value (New-NumberingManifest)
    Write-JsonFile -Path (Join-Path $Root "table-layout-delivery-rollup\summary.json") -Value (New-TableLayoutRollup)
    Write-JsonFile -Path (Join-Path $Root "content-control-data-binding\inspect-content-controls.json") -Value (New-ContentControlInspection)
    Write-JsonFile -Path (Join-Path $Root "content-control-data-binding\sync-content-controls-from-custom-xml.json") -Value (New-ContentControlSyncResult)
    Write-JsonFile -Path (Join-Path $Root "project-template-onboarding-governance\summary.json") -Value (New-OnboardingGovernance)
    Write-JsonFile -Path (Join-Path $Root "project-template-smoke\summary.json") -Value (New-ProjectTemplateSmokeSummary)
    Write-JsonFile -Path (Join-Path $Root "project-template-schema-approval-history\history.json") -Value (New-SchemaApprovalHistory)
    Write-JsonFile -Path (Join-Path $Root "pdf-visual-release-gate-preflight-governance\summary.json") -Value (New-PdfPreflightGovernance)
    Write-DocxVisualSmokeFixture -Root (Join-Path $Root "docx-functional-smoke-visual\passing-smoke")
}

function Invoke-Pipeline {
    param([string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = ($lines -join [System.Environment]::NewLine)
    }
}
