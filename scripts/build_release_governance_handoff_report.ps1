<#
.SYNOPSIS
Builds a release governance handoff from the default delivery reports.

.DESCRIPTION
Reads the final numbering catalog governance, table layout delivery governance,
content-control data-binding governance, and project-template delivery readiness
summaries, then writes one read-only JSON/Markdown handoff for release reviewers.
The script does not rerun CLI, CMake, Word, or visual automation.
#>
param(
    [string]$InputRoot = "output",
    [string[]]$InputJson = @(),
    [string]$OutputDir = "output/release-governance-handoff",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [ValidateSet("full", "explicit-only")]
    [string]$ExpectedReportProfile = "full",
    [switch]$IncludeReleaseBlockerRollup,
    [switch]$FailOnMissing,
    [switch]$FailOnBlocker,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[release-governance-handoff] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$Path, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $root = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolved = [System.IO.Path]::GetFullPath($Path)
    if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
        return ".\" + ($relative -replace '/', '\')
    }
    return $resolved
}

function Convert-ReleaseMaterialString {
    param(
        [string]$RepoRoot,
        [AllowNull()][string]$Text
    )

    if ($null -eq $Text -or [string]::IsNullOrWhiteSpace($RepoRoot)) {
        return $Text
    }

    $normalized = [string]$Text
    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $repoRootForms = @(
        $resolvedRepoRoot,
        ($resolvedRepoRoot -replace '\\', '/'),
        ($resolvedRepoRoot -replace '\\', '\\')
    ) | Sort-Object -Unique

    foreach ($rootForm in $repoRootForms) {
        if ([string]::IsNullOrWhiteSpace($rootForm)) {
            continue
        }

        $pattern = [regex]::Escape($rootForm) + '(?<suffix>\\\\|[\\/]|(?=$|[\s"''`<>|;,)]+))'
        $normalized = [regex]::Replace(
            $normalized,
            $pattern,
            '.${suffix}',
            [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    }

    return $normalized.Replace('./', '.\')
}

function Write-ReleaseMaterialFiles {
    param(
        [AllowNull()]$Summary,
        [string]$SummaryPath,
        [string]$MarkdownPath,
        [int]$JsonDepth
    )

    $summaryJson = $Summary | ConvertTo-Json -Depth $JsonDepth
    (Convert-ReleaseMaterialString -RepoRoot $repoRoot -Text $summaryJson) |
        Set-Content -LiteralPath $SummaryPath -Encoding UTF8

    $markdown = @(New-ReportMarkdown -Summary $Summary) -join [Environment]::NewLine
    (Convert-ReleaseMaterialString -RepoRoot $repoRoot -Text $markdown) |
        Set-Content -LiteralPath $MarkdownPath -Encoding UTF8
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    if ($value -is [bool]) { return [bool]$value }
    $parsed = $false
    if ([bool]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Test-InformationalActionItem {
    param($Item)

    $category = Get-JsonString -Object $Item -Name "category"
    if ($category -in @("release_checklist", "manual_release_checklist", "informational")) {
        return $true
    }

    if (Get-JsonBool -Object $Item -Name "optional") {
        return $true
    }

    $releaseBlockingProperty = Get-JsonProperty -Object $Item -Name "release_blocking"
    if ($null -ne $releaseBlockingProperty -and -not (Get-JsonBool -Object $Item -Name "release_blocking" -DefaultValue $true)) {
        return $true
    }

    $id = Get-JsonString -Object $Item -Name "id"
    $action = Get-JsonString -Object $Item -Name "action"
    return ($id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline") -or
        $action -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline"))
}

function Copy-ActionItemWithReleaseChecklistDefaults {
    param($Item)

    $copy = [ordered]@{}
    if ($Item -is [System.Collections.IDictionary]) {
        foreach ($key in @($Item.Keys)) {
            $copy[[string]$key] = $Item[$key]
        }
    } else {
        foreach ($property in @($Item.PSObject.Properties)) {
            $copy[$property.Name] = $property.Value
        }
    }

    if ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $copy -Name "category"))) {
        $copy["category"] = "release_checklist"
    }
    if ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $copy -Name "severity"))) {
        $copy["severity"] = "info"
    }
    $copy["release_blocking"] = $false
    $copy["optional"] = $true

    return $copy
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Get-FirstJsonProperty {
    param($Object, [string[]]$Names)

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -eq $value) { continue }
        if ($value -is [string] -and [string]::IsNullOrWhiteSpace($value)) { continue }
        return $value
    }

    return $null
}

function Get-PdfVisualGateRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $verdict = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_verdict"
            $status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_status"
            $segmentedStatus = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_status"
            $fullCtestReadinessStatus = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_status"
            if ([string]::IsNullOrWhiteSpace($verdict) -and
                [string]::IsNullOrWhiteSpace($status) -and
                [string]::IsNullOrWhiteSpace($segmentedStatus) -and
                [string]::IsNullOrWhiteSpace($fullCtestReadinessStatus)) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                full_visual_gate_status = Get-JsonString -Object $sourceReport -Name "full_visual_gate_status"
                pdf_visual_gate_status = $status
                pdf_visual_gate_verdict = $verdict
                pdf_visual_gate_finalizable = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_finalizable")
                pdf_visual_gate_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_summary_json_display"
                pdf_visual_gate_aggregate_contact_sheet_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_aggregate_contact_sheet_display"
                pdf_visual_gate_cjk_manifest_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_cjk_manifest_count")
                pdf_visual_gate_cjk_copy_search_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_cjk_copy_search_count")
                pdf_visual_gate_cjk_missing_text_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_cjk_missing_text_count")
                pdf_visual_gate_visual_baseline_manifest_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_visual_baseline_manifest_count")
                pdf_visual_gate_visual_baseline_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_visual_baseline_count")
                pdf_bounded_ctest_summary_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_bounded_ctest_summary_count")
                pdf_bounded_ctest_pass_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_bounded_ctest_pass_count")
                pdf_bounded_ctest_skipped_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_bounded_ctest_skipped_test_count")
                pdf_bounded_ctest_selected_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_bounded_ctest_selected_test_count")
                pdf_bounded_ctest_subsets = @(Get-JsonArray -Object $sourceReport -Name "pdf_bounded_ctest_subsets")
                pdf_bounded_ctest_summary_json_display = @(Get-JsonArray -Object $sourceReport -Name "pdf_bounded_ctest_summary_json_display")
                pdf_full_ctest_readiness_requested = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_requested")
                pdf_full_ctest_readiness_status = $fullCtestReadinessStatus
                pdf_full_ctest_readiness_verdict = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_verdict"
                pdf_full_ctest_readiness_release_ready = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_release_ready")
                pdf_full_ctest_readiness_visual_gate_release_evidence_accepted = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_visual_gate_release_evidence_accepted")
                pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence")
                pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence")
                pdf_full_ctest_readiness_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_summary_json_display"
                pdf_full_ctest_readiness_full_ctest_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_full_ctest_summary_json_display"
                pdf_full_ctest_readiness_full_ctest_status = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_full_ctest_status"
                pdf_full_ctest_readiness_full_ctest_verdict = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_full_ctest_verdict"
                pdf_full_ctest_readiness_outer_guard_status = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_outer_guard_status"
                pdf_full_ctest_readiness_outer_guard_timed_out = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_outer_guard_timed_out")
                pdf_full_ctest_readiness_selected_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_selected_test_count")
                pdf_full_ctest_readiness_completed_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_completed_test_count")
                pdf_full_ctest_readiness_passed_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_passed_test_count")
                pdf_full_ctest_readiness_failed_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_failed_test_count")
                pdf_full_ctest_readiness_skipped_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_skipped_test_count")
                pdf_full_ctest_readiness_not_run_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_not_run_test_count")
                pdf_full_ctest_readiness_completion_percent = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_completion_percent")
                pdf_full_ctest_readiness_remaining_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_remaining_test_count")
                pdf_full_ctest_readiness_zero_failed_tests_observed = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_full_ctest_readiness_zero_failed_tests_observed")
                pdf_full_ctest_readiness_boundary = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_boundary"
                pdf_full_ctest_readiness_marker = Get-JsonString -Object $sourceReport -Name "pdf_full_ctest_readiness_marker"
                pdf_visual_gate_attempt_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_status"
                pdf_visual_gate_attempt_verdict = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_verdict"
                pdf_visual_gate_attempt_full_visual_gate_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_full_visual_gate_status"
                pdf_visual_gate_attempt_evidence_scope = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_evidence_scope"
                pdf_visual_gate_attempt_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_summary_json_display"
                pdf_visual_gate_attempt_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_stage_count")
                pdf_visual_gate_attempt_passed_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_passed_stage_count")
                pdf_visual_gate_attempt_failed_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_failed_stage_count")
                pdf_visual_gate_attempt_incomplete_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_incomplete_stage_count")
                pdf_visual_gate_attempt_outer_guard_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_outer_guard_status"
                pdf_visual_gate_attempt_outer_guard_timed_out = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_outer_guard_timed_out")
                pdf_visual_gate_attempt_outer_guard_timeout_seconds = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_outer_guard_timeout_seconds")
                pdf_visual_gate_attempt_pdf_cli_export_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_pdf_cli_export_status"
                pdf_visual_gate_attempt_pdf_regression_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_pdf_regression_status"
                pdf_visual_gate_attempt_pdf_regression_selected_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_pdf_regression_selected_test_count")
                pdf_visual_gate_attempt_pdf_regression_failed_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_pdf_regression_failed_test_count")
                pdf_visual_gate_attempt_pdf_regression_skipped_test_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_pdf_regression_skipped_test_count")
                pdf_visual_gate_attempt_unicode_font_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_unicode_font_status"
                pdf_visual_gate_attempt_cjk_copy_search_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_cjk_copy_search_status"
                pdf_visual_gate_attempt_cjk_copy_search_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_cjk_copy_search_count")
                pdf_visual_gate_attempt_cjk_copy_search_missing_text_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_cjk_copy_search_missing_text_count")
                pdf_visual_gate_attempt_visual_baseline_render_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_visual_baseline_render_status"
                pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count")
                pdf_visual_gate_attempt_expected_visual_render_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_gate_attempt_expected_visual_render_count")
                pdf_visual_gate_attempt_aggregate_contact_sheet_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_aggregate_contact_sheet_status"
                pdf_visual_gate_attempt_aggregate_contact_sheet_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_gate_attempt_aggregate_contact_sheet_display"
                pdf_visual_segmented_gate_status = $segmentedStatus
                pdf_visual_segmented_gate_verdict = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_verdict"
                pdf_visual_segmented_gate_full_visual_gate_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_full_visual_gate_status"
                pdf_visual_segmented_gate_evidence_scope = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_evidence_scope"
                pdf_visual_segmented_gate_boundary = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_boundary"
                pdf_visual_segmented_gate_summary_json_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_summary_json_display"
                pdf_visual_segmented_gate_slice_summary_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_slice_summary_count")
                pdf_visual_segmented_gate_slice_pass_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_slice_pass_count")
                pdf_visual_segmented_gate_slice_failed_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_slice_failed_count")
                pdf_visual_segmented_gate_covered_baseline_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_covered_baseline_count")
                pdf_visual_segmented_gate_expected_visual_render_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_expected_visual_render_count")
                pdf_visual_segmented_gate_attempt_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_attempt_stage_count")
                pdf_visual_segmented_gate_attempt_passed_stage_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_attempt_passed_stage_count")
                pdf_visual_segmented_gate_visual_baseline_render_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_visual_baseline_render_status"
                pdf_visual_segmented_gate_aggregate_contact_sheet_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_status"
                pdf_visual_segmented_gate_aggregate_contact_sheet_display = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_aggregate_contact_sheet_display"
                pdf_visual_segmented_gate_aggregate_contact_sheet_bytes = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_aggregate_contact_sheet_bytes")
                pdf_visual_segmented_gate_aggregate_rebuild_status = Get-JsonString -Object $sourceReport -Name "pdf_visual_segmented_gate_aggregate_rebuild_status"
                pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count = Get-FirstJsonProperty -Object $sourceReport -Names @("pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count")
            }
        }
    )
}

function Get-ManifestSignoffEntrypointsRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $status = Get-JsonString -Object $sourceReport -Name "manifest_signoff_entrypoints_status"
            $releaseAssetsManifest = Get-JsonString -Object $sourceReport -Name "manifest_signoff_entrypoints_release_assets_manifest"
            if ([string]::IsNullOrWhiteSpace($status) -and [string]::IsNullOrWhiteSpace($releaseAssetsManifest)) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                manifest_signoff_entrypoints_status = $status
                manifest_signoff_entrypoints_release_assets_manifest = $releaseAssetsManifest
                manifest_signoff_entrypoints_release_assets_manifest_display = Get-JsonString -Object $sourceReport -Name "manifest_signoff_entrypoints_release_assets_manifest_display"
                manifest_signoff_entrypoints_required_entrypoint_count = Get-FirstJsonProperty -Object $sourceReport -Names @("manifest_signoff_entrypoints_required_entrypoint_count")
                manifest_signoff_entrypoints_entrypoint_ids = @(Get-JsonArray -Object $sourceReport -Name "manifest_signoff_entrypoints_entrypoint_ids")
                manifest_signoff_entrypoints_entrypoints = @(Get-JsonArray -Object $sourceReport -Name "manifest_signoff_entrypoints_entrypoints")
                manifest_signoff_entrypoints_required_contracts = @(Get-JsonArray -Object $sourceReport -Name "manifest_signoff_entrypoints_required_contracts")
                manifest_signoff_entrypoints_required_fields = @(Get-JsonArray -Object $sourceReport -Name "manifest_signoff_entrypoints_required_fields")
                manifest_signoff_entrypoints_checklist_marker = Get-JsonString -Object $sourceReport -Name "manifest_signoff_entrypoints_checklist_marker"
            }
        }
    )
}

function Get-ProjectTemplateReadinessChecklistEntrypointsRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $status = Get-JsonString -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_status"
            $checklistPath = Get-JsonString -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_checklist_path"
            if ([string]::IsNullOrWhiteSpace($status) -and [string]::IsNullOrWhiteSpace($checklistPath)) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                project_template_readiness_checklist_entrypoints_status = $status
                project_template_readiness_checklist_entrypoints_checklist_label = Get-JsonString -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_checklist_label"
                project_template_readiness_checklist_entrypoints_checklist_path = $checklistPath
                project_template_readiness_checklist_entrypoints_required_entrypoint_count = Get-FirstJsonProperty -Object $sourceReport -Names @("project_template_readiness_checklist_entrypoints_required_entrypoint_count")
                project_template_readiness_checklist_entrypoints_entrypoint_ids = @(Get-JsonArray -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_entrypoint_ids")
                project_template_readiness_checklist_entrypoints_entrypoints = @(Get-JsonArray -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_entrypoints")
                project_template_readiness_checklist_entrypoints_checklist_marker = Get-JsonString -Object $sourceReport -Name "project_template_readiness_checklist_entrypoints_checklist_marker"
            }
        }
    )
}

function Get-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditRollupEvidence {
    param($RollupSummary)

    return @(
        foreach ($sourceReport in @(Get-JsonArray -Object $RollupSummary -Name "source_reports")) {
            $status = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_status"
            $marker = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker"
            if ([string]::IsNullOrWhiteSpace($status) -and [string]::IsNullOrWhiteSpace($marker)) {
                continue
            }

            [ordered]@{
                schema = Get-JsonString -Object $sourceReport -Name "schema"
                path_display = Get-JsonString -Object $sourceReport -Name "path_display"
                release_entry_project_template_readiness_checklist_material_safety_audit_status = $status
                release_entry_project_template_readiness_checklist_material_safety_audit_script = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_script"
                release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count = Get-FirstJsonProperty -Object $sourceReport -Names @("release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count")
                release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints = @(Get-JsonArray -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints")
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label"
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field"
                release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema"
                release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path"
                release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker = Get-JsonString -Object $sourceReport -Name "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker"
                release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker = $marker
            }
        }
    )
}

function Get-GovernanceMetricByContract {
    param(
        $Metrics,
        [string]$Metric,
        [string]$Id,
        [string]$ReportId,
        [string]$SourceSchema
    )

    return @($Metrics | Where-Object {
        [string]$_.metric -eq $Metric -and
        [string]$_.id -eq $Id -and
        [string]$_.report_id -eq $ReportId -and
        [string]$_.source_schema -eq $SourceSchema
    }) | Select-Object -First 1
}

function Add-GovernanceMetricDetailLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        $Metric
    )

    $details = Get-JsonProperty -Object $Metric -Name "details"
    if ($null -eq $details) { return }

    $detailFields = @(
        "document_count",
        "catalog_exemplar_count",
        "baseline_entry_count",
        "matched_document_count",
        "unmatched_catalog_document_count",
        "unmatched_baseline_document_count",
        "alignment_gap_count",
        "catalog_coverage_percent",
        "baseline_coverage_percent",
        "coverage_score",
        "ready_document_count",
        "ready_document_percent",
        "needs_review_document_count",
        "failed_document_count",
        "table_style_issue_count",
        "automatic_tblLook_fix_count",
        "manual_table_style_fix_count",
        "table_position_automatic_count",
        "table_position_review_count",
        "command_failure_count",
        "unresolved_item_count"
    )
    $detailParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($fieldName in $detailFields) {
        $value = Get-JsonProperty -Object $details -Name $fieldName
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            $detailParts.Add("$fieldName=$value") | Out-Null
        }
    }
    if ($detailParts.Count -gt 0) {
        $Lines.Add("  - details: ``$($detailParts -join ', ')``") | Out-Null
    }

    $penaltyParts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($penalty in @(Get-JsonArray -Object $details -Name "penalty_summary")) {
        $factor = Get-JsonString -Object $penalty -Name "factor"
        if ([string]::IsNullOrWhiteSpace($factor)) { continue }
        $count = Get-JsonProperty -Object $penalty -Name "count"
        $penaltyValue = Get-JsonProperty -Object $penalty -Name "penalty"
        $penaltyParts.Add("$factor(count=$count, penalty=$penaltyValue)") | Out-Null
    }
    if ($penaltyParts.Count -gt 0) {
        $Lines.Add("  - penalty_summary: ``$($penaltyParts -join '; ')``") | Out-Null
    }
}

function New-GovernanceMetrics {
    param(
        $Summary,
        [string]$ReportId,
        [string]$ReportTitle,
        [string]$SourceSchema,
        [string]$SourceReport,
        [string]$SourceReportDisplay
    )

    $metrics = New-Object 'System.Collections.Generic.List[object]'

    if ($ReportId -eq "numbering_catalog_governance" -and
        $SourceSchema -eq "featherdoc.numbering_catalog_governance_report.v1" -and
        ($null -ne (Get-JsonProperty -Object $Summary -Name "real_corpus_confidence_score") -or
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Summary -Name "real_corpus_confidence_level")))) {
        $metrics.Add([ordered]@{
            id = "numbering_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = $ReportId
            report_title = $ReportTitle
            source_schema = $SourceSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceReport
            source_json_display = $SourceReportDisplay
            score = Get-JsonInt -Object $Summary -Name "real_corpus_confidence_score"
            level = Get-JsonString -Object $Summary -Name "real_corpus_confidence_level"
            details = Get-JsonProperty -Object $Summary -Name "real_corpus_confidence"
        }) | Out-Null
    }

    if ($ReportId -eq "table_layout_delivery_governance" -and
        $SourceSchema -eq "featherdoc.table_layout_delivery_governance_report.v1" -and
        ($null -ne (Get-JsonProperty -Object $Summary -Name "delivery_quality_score") -or
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Summary -Name "delivery_quality_level")))) {
        $metrics.Add([ordered]@{
            id = "table_layout_delivery_governance.delivery_quality"
            metric = "delivery_quality"
            report_id = $ReportId
            report_title = $ReportTitle
            source_schema = $SourceSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceReport
            source_json_display = $SourceReportDisplay
            score = Get-JsonInt -Object $Summary -Name "delivery_quality_score"
            level = Get-JsonString -Object $Summary -Name "delivery_quality_level"
            details = Get-JsonProperty -Object $Summary -Name "delivery_quality"
        }) | Out-Null
    }

    return @($metrics.ToArray())
}

function Expand-InputPathList {
    param([string[]]$Paths)

    return @(
        foreach ($path in @($Paths)) {
            foreach ($part in ([string]$path -split ",")) {
                $trimmed = $part.Trim()
                if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                    $trimmed
                }
            }
        }
    )
}

function Get-ReportKind {
    param($Summary)

    $schema = Get-JsonString -Object $Summary -Name "schema"
    switch ($schema) {
        "featherdoc.numbering_catalog_governance_report.v1" { return "numbering_catalog_governance" }
        "featherdoc.table_layout_delivery_governance_report.v1" { return "table_layout_delivery_governance" }
        "featherdoc.content_control_data_binding_governance_report.v1" { return "content_control_data_binding_governance" }
        "featherdoc.project_template_delivery_readiness_report.v1" { return "project_template_delivery_readiness" }
        "featherdoc.schema_patch_confidence_calibration_report.v1" { return "schema_patch_confidence_calibration" }
        "featherdoc.docx_functional_smoke_readiness.v1" { return "docx_functional_smoke_readiness" }
        default { return $schema }
    }
}

function Test-ProjectTemplateDeliveryReadinessReportEntry {
    param([object]$Report)

    $id = Get-JsonString -Object $Report -Name "id"
    $schema = Get-JsonString -Object $Report -Name "schema"
    return (
        [string]::Equals($id, "project_template_delivery_readiness", [System.StringComparison]::OrdinalIgnoreCase) -or
        [string]::Equals($schema, "featherdoc.project_template_delivery_readiness_report.v1", [System.StringComparison]::OrdinalIgnoreCase)
    )
}

function New-ProjectTemplateOnboardingGovernanceContract {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [object]$Json
    )

    if ($null -eq $Json) {
        return $null
    }

    $schema = Get-JsonString -Object $Json -Name "schema"
    if (-not [string]::Equals($schema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $null
    }

    return [ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = Get-JsonString -Object $Json -Name "status"
        release_ready = if ($null -ne (Get-JsonProperty -Object $Json -Name "release_ready")) { [string](Get-JsonBool -Object $Json -Name "release_ready") } else { "" }
        schema_approval_status_summary = @(Get-JsonArray -Object $Json -Name "schema_approval_status_summary")
        source_report = $Path
        source_report_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        source_json = $Path
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
    }
}

function Get-ProjectTemplateOnboardingGovernanceContract {
    param(
        [string]$RepoRoot,
        [string]$InputRoot,
        [string[]]$InputJson
    )

    $candidatePaths = New-Object 'System.Collections.Generic.List[string]'
    $candidatePaths.Add((Join-Path $InputRoot "project-template-onboarding-governance\summary.json")) | Out-Null
    foreach ($input in @(Expand-InputPathList -Paths $InputJson)) {
        if ([string]::IsNullOrWhiteSpace($input)) {
            continue
        }
        $candidatePaths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $input -AllowMissing)) | Out-Null
    }

    foreach ($candidatePath in @($candidatePaths.ToArray())) {
        if (-not (Test-Path -LiteralPath $candidatePath)) {
            continue
        }
        try {
            $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidatePath | ConvertFrom-Json
            $contract = New-ProjectTemplateOnboardingGovernanceContract -RepoRoot $RepoRoot -Path $candidatePath -Json $json
            if ($null -ne $contract) {
                return $contract
            }
        } catch {
            continue
        }
    }

    return $null
}

function New-ExpectedReport {
    param(
        [string]$Id,
        [string]$Title,
        [string]$RelativeSummary,
        [string]$BuildCommand
    )

    return [ordered]@{
        id = $Id
        title = $Title
        relative_summary = $RelativeSummary
        build_command = $BuildCommand
    }
}

function Invoke-ReleaseBlockerRollup {
    param(
        [string]$RepoRoot,
        [string]$OutputDir,
        [string[]]$InputJson
    )

    if (@($InputJson).Count -eq 0) {
        throw "Release blocker rollup requires at least one loaded governance report."
    }

    $scriptPath = Join-Path $RepoRoot "scripts\build_release_blocker_rollup_report.ps1"
    $arguments = @(
        "-InputJson"
        (@($InputJson) -join ",")
        "-OutputDir"
        $OutputDir
    )
    $commandOutput = @(& (Get-Process -Id $PID).Path -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @arguments 2>&1)
    if ($LASTEXITCODE -ne 0) {
        $errorText = (@($commandOutput | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
        throw "Failed to build release blocker rollup. Output: $errorText"
    }
}

function New-ReportEntry {
    param(
        [string]$RepoRoot,
        [string]$Id,
        [string]$Title,
        [string]$ExpectedSummaryPath,
        [string]$BuildCommand,
        [string]$Source = "default",
        [object]$Json = $null,
        [string]$Status = "missing",
        [string]$ErrorMessage = ""
    )

    $schema = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "schema" }
    $metrics = if ($null -eq $Json) {
        @()
    } else {
        @(New-GovernanceMetrics `
            -Summary $Json `
            -ReportId $Id `
            -ReportTitle $Title `
            -SourceSchema $schema `
            -SourceReport $ExpectedSummaryPath `
            -SourceReportDisplay (Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath))
    }
    return [ordered]@{
        id = $Id
        title = $Title
        source = $Source
        expected_summary = $ExpectedSummaryPath
        expected_summary_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath
        source_report = $ExpectedSummaryPath
        source_report_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath
        source_json = $ExpectedSummaryPath
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $ExpectedSummaryPath
        build_command = $BuildCommand
        schema = $schema
        status_present = ($null -ne $Json -and $null -ne (Get-JsonProperty -Object $Json -Name "status"))
        status = if ($null -eq $Json) { $Status } else { Get-JsonString -Object $Json -Name "status" -DefaultValue $Status }
        release_ready_present = ($null -ne $Json -and $null -ne (Get-JsonProperty -Object $Json -Name "release_ready"))
        release_ready = if ($null -eq $Json) { $false } else { Get-JsonBool -Object $Json -Name "release_ready" }
        release_blocker_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "release_blocker_count" }
        action_item_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "action_item_count" }
        warning_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "warning_count" }
        source_failure_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "source_failure_count" }
        latest_schema_approval_gate_status = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "latest_schema_approval_gate_status" }
        schema_approval_status_summary = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "schema_approval_status_summary") }
        report_markdown = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "report_markdown" }
        report_markdown_display = if ($null -eq $Json) { "" } else { Get-JsonString -Object $Json -Name "report_markdown_display" }
        release_blockers = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "release_blockers") }
        action_items = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "action_items") }
        informational_action_item_count = if ($null -eq $Json) { 0 } else { Get-JsonInt -Object $Json -Name "informational_action_item_count" }
        informational_action_items = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "informational_action_items") }
        warnings = if ($null -eq $Json) { @() } else { @(Get-JsonArray -Object $Json -Name "warnings") }
        governance_metric_count = @($metrics).Count
        governance_metrics = @($metrics)
        error = $ErrorMessage
    }
}

function Add-NormalizedBlockers {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [object]$Report,
        [AllowNull()]$ProjectTemplateOnboardingGovernanceContract
    )

    $isProjectTemplateDeliveryReadiness = Test-ProjectTemplateDeliveryReadinessReportEntry -Report $Report
    $readinessStatus = if ($isProjectTemplateDeliveryReadiness -and (Get-JsonBool -Object $Report -Name "status_present")) {
        Get-JsonString -Object $Report -Name "status"
    } else {
        ""
    }
    $readinessReleaseReady = if ($isProjectTemplateDeliveryReadiness -and (Get-JsonBool -Object $Report -Name "release_ready_present")) {
        [string](Get-JsonBool -Object $Report -Name "release_ready")
    } else {
        ""
    }

    foreach ($blocker in @($Report.release_blockers)) {
        $sourceReportDisplay = Get-JsonString -Object $blocker -Name "source_report_display" -DefaultValue ([string]$Report.expected_summary_display)
        $sourceReport = Get-JsonString -Object $blocker -Name "source_report" -DefaultValue ([string]$Report.expected_summary)
        $sourceJsonDisplay = Get-JsonString -Object $blocker -Name "source_json_display" -DefaultValue $sourceReportDisplay
        $sourceJson = Get-JsonString -Object $blocker -Name "source_json" -DefaultValue $sourceReport
        $sourceSchema = Get-JsonString -Object $blocker -Name "source_schema" -DefaultValue ([string]$Report.schema)
        $onboardingStatus = Get-JsonString -Object $blocker -Name "onboarding_governance_status"
        $onboardingReleaseReady = Get-JsonString -Object $blocker -Name "onboarding_governance_release_ready"
        $onboardingSchemaApprovalSummary = @(Get-JsonArray -Object $blocker -Name "onboarding_governance_schema_approval_status_summary")
        $onboardingSourceReportDisplay = Get-JsonString -Object $blocker -Name "onboarding_governance_source_report_display"
        $onboardingSourceJsonDisplay = Get-JsonString -Object $blocker -Name "onboarding_governance_source_json_display"
        if ([string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase) -and $null -ne $ProjectTemplateOnboardingGovernanceContract) {
            if ([string]::IsNullOrWhiteSpace($onboardingStatus)) {
                $onboardingStatus = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "status"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingReleaseReady)) {
                $onboardingReleaseReady = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "release_ready"
            }
            if ($onboardingSchemaApprovalSummary.Count -eq 0) {
                $onboardingSchemaApprovalSummary = @(Get-JsonArray -Object $ProjectTemplateOnboardingGovernanceContract -Name "schema_approval_status_summary")
            }
            if ([string]::IsNullOrWhiteSpace($onboardingSourceReportDisplay)) {
                $onboardingSourceReportDisplay = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "source_report_display"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingSourceJsonDisplay)) {
                $onboardingSourceJsonDisplay = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "source_json_display"
            }
        }
        $Collection.Add([ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = Get-JsonString -Object $blocker -Name "id" -DefaultValue "release_blocker"
            project_id = Get-JsonString -Object $blocker -Name "project_id"
            template_name = Get-JsonString -Object $blocker -Name "template_name"
            candidate_type = Get-JsonString -Object $blocker -Name "candidate_type"
            severity = Get-JsonString -Object $blocker -Name "severity" -DefaultValue "error"
            status = Get-JsonString -Object $blocker -Name "status"
            action = Get-JsonString -Object $blocker -Name "action"
            message = Get-JsonString -Object $blocker -Name "message"
            source_schema = $sourceSchema
            source_report = $sourceReport
            source_report_display = $sourceReportDisplay
            source_json = $sourceJson
            source_json_display = $sourceJsonDisplay
            readiness_status = $readinessStatus
            readiness_release_ready = $readinessReleaseReady
            schema_approval_status_summary = @(Get-JsonArray -Object $blocker -Name "schema_approval_status_summary")
            onboarding_governance_status = $onboardingStatus
            onboarding_governance_release_ready = $onboardingReleaseReady
            onboarding_governance_schema_approval_status_summary = @($onboardingSchemaApprovalSummary)
            onboarding_governance_source_report_display = $onboardingSourceReportDisplay
            onboarding_governance_source_json_display = $onboardingSourceJsonDisplay
            input_docx = Get-JsonString -Object $blocker -Name "input_docx"
            input_docx_display = Get-JsonString -Object $blocker -Name "input_docx_display"
            schema_target = Get-JsonString -Object $blocker -Name "schema_target"
            target_mode = Get-JsonString -Object $blocker -Name "target_mode"
            repair_strategy = Get-JsonString -Object $blocker -Name "repair_strategy"
            repair_hint = Get-JsonString -Object $blocker -Name "repair_hint"
            command_template = Get-JsonString -Object $blocker -Name "command_template"
        }) | Out-Null
    }
}

function Add-NormalizedActions {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [System.Collections.Generic.List[object]]$InformationalCollection,
        [object]$Report,
        [AllowNull()]$ProjectTemplateOnboardingGovernanceContract,
        [switch]$ForceInformational
    )

    $isProjectTemplateDeliveryReadiness = Test-ProjectTemplateDeliveryReadinessReportEntry -Report $Report
    $readinessStatus = if ($isProjectTemplateDeliveryReadiness -and (Get-JsonBool -Object $Report -Name "status_present")) {
        Get-JsonString -Object $Report -Name "status"
    } else {
        ""
    }
    $readinessReleaseReady = if ($isProjectTemplateDeliveryReadiness -and (Get-JsonBool -Object $Report -Name "release_ready_present")) {
        [string](Get-JsonBool -Object $Report -Name "release_ready")
    } else {
        ""
    }

    $sourceItems = if ($ForceInformational) { @($Report.informational_action_items) } else { @($Report.action_items) }
    foreach ($item in @($sourceItems)) {
        $sourceReportDisplay = Get-JsonString -Object $item -Name "source_report_display" -DefaultValue ([string]$Report.expected_summary_display)
        $sourceReport = Get-JsonString -Object $item -Name "source_report" -DefaultValue ([string]$Report.expected_summary)
        $sourceJsonDisplay = Get-JsonString -Object $item -Name "source_json_display" -DefaultValue $sourceReportDisplay
        $sourceJson = Get-JsonString -Object $item -Name "source_json" -DefaultValue $sourceReport
        $openCommand = Get-JsonString -Object $item -Name "open_command" -DefaultValue (Get-JsonString -Object $item -Name "command")
        if ([string]::IsNullOrWhiteSpace($openCommand)) {
            $openCommand = [string]$Report.build_command
        }
        $sourceSchema = Get-JsonString -Object $item -Name "source_schema" -DefaultValue ([string]$Report.schema)
        $onboardingStatus = Get-JsonString -Object $item -Name "onboarding_governance_status"
        $onboardingReleaseReady = Get-JsonString -Object $item -Name "onboarding_governance_release_ready"
        $onboardingSchemaApprovalSummary = @(Get-JsonArray -Object $item -Name "onboarding_governance_schema_approval_status_summary")
        $onboardingSourceReportDisplay = Get-JsonString -Object $item -Name "onboarding_governance_source_report_display"
        $onboardingSourceJsonDisplay = Get-JsonString -Object $item -Name "onboarding_governance_source_json_display"
        if ([string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase) -and $null -ne $ProjectTemplateOnboardingGovernanceContract) {
            if ([string]::IsNullOrWhiteSpace($onboardingStatus)) {
                $onboardingStatus = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "status"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingReleaseReady)) {
                $onboardingReleaseReady = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "release_ready"
            }
            if ($onboardingSchemaApprovalSummary.Count -eq 0) {
                $onboardingSchemaApprovalSummary = @(Get-JsonArray -Object $ProjectTemplateOnboardingGovernanceContract -Name "schema_approval_status_summary")
            }
            if ([string]::IsNullOrWhiteSpace($onboardingSourceReportDisplay)) {
                $onboardingSourceReportDisplay = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "source_report_display"
            }
            if ([string]::IsNullOrWhiteSpace($onboardingSourceJsonDisplay)) {
                $onboardingSourceJsonDisplay = Get-JsonString -Object $ProjectTemplateOnboardingGovernanceContract -Name "source_json_display"
            }
        }
        $actionId = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
        $action = Get-JsonString -Object $item -Name "action"
        if ([string]::IsNullOrWhiteSpace($action)) {
            $action = $actionId
        }
        $normalizedAction = [ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = $actionId
            project_id = Get-JsonString -Object $item -Name "project_id"
            template_name = Get-JsonString -Object $item -Name "template_name"
            candidate_type = Get-JsonString -Object $item -Name "candidate_type"
            action = $action
            title = Get-JsonString -Object $item -Name "title"
            command = Get-JsonString -Object $item -Name "command"
            open_command = $openCommand
            category = Get-JsonString -Object $item -Name "category"
            severity = Get-JsonString -Object $item -Name "severity"
            release_blocking = Get-JsonBool -Object $item -Name "release_blocking" -DefaultValue $true
            optional = Get-JsonBool -Object $item -Name "optional"
            source_schema = $sourceSchema
            source_report = $sourceReport
            source_report_display = $sourceReportDisplay
            source_json = $sourceJson
            source_json_display = $sourceJsonDisplay
            readiness_status = $readinessStatus
            readiness_release_ready = $readinessReleaseReady
            schema_approval_status_summary = @(Get-JsonArray -Object $item -Name "schema_approval_status_summary")
            onboarding_governance_status = $onboardingStatus
            onboarding_governance_release_ready = $onboardingReleaseReady
            onboarding_governance_schema_approval_status_summary = @($onboardingSchemaApprovalSummary)
            onboarding_governance_source_report_display = $onboardingSourceReportDisplay
            onboarding_governance_source_json_display = $onboardingSourceJsonDisplay
            input_docx = Get-JsonString -Object $item -Name "input_docx"
            input_docx_display = Get-JsonString -Object $item -Name "input_docx_display"
            schema_target = Get-JsonString -Object $item -Name "schema_target"
            target_mode = Get-JsonString -Object $item -Name "target_mode"
            repair_strategy = Get-JsonString -Object $item -Name "repair_strategy"
            repair_hint = Get-JsonString -Object $item -Name "repair_hint"
            command_template = Get-JsonString -Object $item -Name "command_template"
        }
        if ($ForceInformational -or (Test-InformationalActionItem -Item $normalizedAction)) {
            $InformationalCollection.Add((Copy-ActionItemWithReleaseChecklistDefaults -Item $normalizedAction)) | Out-Null
        } else {
            $Collection.Add($normalizedAction) | Out-Null
        }
    }
}

function Add-NormalizedWarnings {
    param(
        [System.Collections.Generic.List[object]]$Collection,
        [object]$Report
    )

    foreach ($warning in @($Report.warnings)) {
        $sourceReportDisplay = Get-JsonString -Object $warning -Name "source_report_display" -DefaultValue ([string]$Report.expected_summary_display)
        $sourceReport = Get-JsonString -Object $warning -Name "source_report" -DefaultValue ([string]$Report.expected_summary)
        $sourceJsonDisplay = Get-JsonString -Object $warning -Name "source_json_display" -DefaultValue $sourceReportDisplay
        $sourceJson = Get-JsonString -Object $warning -Name "source_json" -DefaultValue $sourceReport
        $Collection.Add([ordered]@{
            report_id = [string]$Report.id
            report_title = [string]$Report.title
            id = Get-JsonString -Object $warning -Name "id" -DefaultValue "warning"
            project_id = Get-JsonString -Object $warning -Name "project_id"
            template_name = Get-JsonString -Object $warning -Name "template_name"
            candidate_type = Get-JsonString -Object $warning -Name "candidate_type"
            action = Get-JsonString -Object $warning -Name "action" -DefaultValue "review_release_governance_warning"
            message = Get-JsonString -Object $warning -Name "message"
            repair_strategy = Get-JsonString -Object $warning -Name "repair_strategy"
            repair_hint = Get-JsonString -Object $warning -Name "repair_hint"
            command_template = Get-JsonString -Object $warning -Name "command_template"
            source_schema = Get-JsonString -Object $warning -Name "source_schema" -DefaultValue ([string]$Report.schema)
            source_report = $sourceReport
            source_report_display = $sourceReportDisplay
            source_json = $sourceJson
            source_json_display = $sourceJsonDisplay
            input_docx = Get-JsonString -Object $warning -Name "input_docx"
            input_docx_display = Get-JsonString -Object $warning -Name "input_docx_display"
            schema_target = Get-JsonString -Object $warning -Name "schema_target"
            target_mode = Get-JsonString -Object $warning -Name "target_mode"
        }) | Out-Null
    }
}

function New-ExplicitReleaseCandidateWarningReport {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [object]$Json,
        [int]$Index
    )

    return [ordered]@{
        id = ("release_candidate_summary_{0}" -f $Index)
        title = "Release Candidate Summary"
        source = "explicit"
        expected_summary = $Path
        expected_summary_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        source_report = $Path
        source_report_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        source_json = $Path
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        schema = Get-JsonString -Object $Json -Name "schema"
        warnings = @(Get-JsonArray -Object $Json -Name "warnings")
    }
}

function New-ReportMarkdown {
    param($Summary)

    function Add-TraceabilityMarkdownLines {
        param(
            [System.Collections.Generic.List[string]]$Lines,
            [object]$Item
        )

        foreach ($fieldName in @("source_report", "source_json")) {
            $fieldValue = Get-JsonString -Object $Item -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                $Lines.Add("  - ${fieldName}: ``$fieldValue``") | Out-Null
            }
        }
    }

    function Format-OnboardingSchemaApprovalStatusSummary {
        param(
            [object[]]$Values,
            [string]$Fallback = ""
        )

        $parts = @(
            foreach ($value in @($Values)) {
                if ($null -eq $value) {
                    continue
                }
                if ($value -is [string]) {
                    if (-not [string]::IsNullOrWhiteSpace($value)) {
                        [string]$value
                    }
                    continue
                }

                $status = Get-JsonString -Object $value -Name "status"
                $count = Get-JsonString -Object $value -Name "count"
                if (-not [string]::IsNullOrWhiteSpace($status) -and -not [string]::IsNullOrWhiteSpace($count)) {
                    "${status}=${count}"
                } elseif (-not [string]::IsNullOrWhiteSpace($status)) {
                    $status
                } elseif (-not [string]::IsNullOrWhiteSpace($count)) {
                    "count=${count}"
                } elseif (-not [string]::IsNullOrWhiteSpace([string]$value)) {
                    [string]$value
                }
            }
        )

        if ($parts.Count -eq 0) {
            return $Fallback
        }

        return ($parts -join ", ")
    }

    function Add-ProjectTemplateOnboardingContractMarkdownLines {
        param(
            [System.Collections.Generic.List[string]]$Lines,
            [object]$Item
        )

        $sourceSchema = Get-JsonString -Object $Item -Name "source_schema"
        if (-not [string]::Equals($sourceSchema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
            return
        }

        $schemaApprovalValues = @(Get-JsonArray -Object $Item -Name "onboarding_governance_schema_approval_status_summary")
        if ($schemaApprovalValues.Count -eq 0) {
            $schemaApprovalValues = @(Get-JsonArray -Object $Item -Name "schema_approval_status_summary")
        }
        $schemaApprovalSummary = Format-OnboardingSchemaApprovalStatusSummary `
            -Values $schemaApprovalValues `
            -Fallback (Get-JsonString -Object $Item -Name "onboarding_governance_status")
        if ([string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
            $schemaApprovalSummary = "unknown"
        }

        $status = Get-JsonString -Object $Item -Name "onboarding_governance_status"
        $releaseReady = Get-JsonString -Object $Item -Name "onboarding_governance_release_ready"

        $sourceReportDisplay = Get-JsonString -Object $Item -Name "onboarding_governance_source_report_display"
        if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            $sourceReportDisplay = Get-JsonString -Object $Item -Name "source_report_display"
        }
        $sourceJsonDisplay = Get-JsonString -Object $Item -Name "onboarding_governance_source_json_display"
        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            $sourceJsonDisplay = Get-JsonString -Object $Item -Name "source_json_display"
        }

        $Lines.Add("  - project_template_onboarding_governance_contract:") | Out-Null
        $Lines.Add("    - source_schema: ``featherdoc.project_template_onboarding_governance_report.v1``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace($status)) {
            $Lines.Add("    - status: ``$status``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace($releaseReady)) {
            $Lines.Add("    - release_ready: ``$releaseReady``") | Out-Null
        }
        $Lines.Add("    - schema_approval_status_summary: ``$schemaApprovalSummary``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            $Lines.Add("    - source_report_display: ``$sourceReportDisplay``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            $Lines.Add("    - source_json_display: ``$sourceJsonDisplay``") | Out-Null
        }
    }

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Release Governance Handoff") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Reports loaded: ``$($Summary.loaded_report_count)`` / ``$($Summary.expected_report_count)``") | Out-Null
    $lines.Add("- Missing reports: ``$($Summary.missing_report_count)``") | Out-Null
    $lines.Add("- Failed reports: ``$($Summary.failed_report_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Informational action items: ``$($Summary.informational_action_item_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($Summary.warning_count)``") | Out-Null
    $lines.Add("- Governance metrics: ``$($Summary.governance_metric_count)``") | Out-Null
    $lines.Add("") | Out-Null

    if ([bool]$Summary.release_blocker_rollup.included) {
        $rollup = $Summary.release_blocker_rollup
        $lines.Add("## Release Blocker Rollup") | Out-Null
        $lines.Add("") | Out-Null
        $lines.Add("- Status: ``$($rollup.status)``") | Out-Null
        $lines.Add("- Summary: ``$($rollup.summary_json_display)``") | Out-Null
        $lines.Add("- Report: ``$($rollup.report_markdown_display)``") | Out-Null
        $lines.Add("- Source reports: ``$($rollup.source_report_count)``") | Out-Null
        $lines.Add("- Release blockers: ``$($rollup.release_blocker_count)``") | Out-Null
        $lines.Add("- Action items: ``$($rollup.action_item_count)``") | Out-Null
        $lines.Add("- Informational action items: ``$($rollup.informational_action_item_count)``") | Out-Null
        $lines.Add("- Warnings: ``$($rollup.warning_count)``") | Out-Null
        $lines.Add("- PDF visual gate evidence source reports: ``$($rollup.pdf_visual_gate_evidence_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.pdf_visual_gate_evidence_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_status: ``$($evidence.pdf_visual_gate_status)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_verdict: ``$($evidence.pdf_visual_gate_verdict)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_finalizable: ``$($evidence.pdf_visual_gate_finalizable)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_summary_json_display: ``$($evidence.pdf_visual_gate_summary_json_display)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_aggregate_contact_sheet_display: ``$($evidence.pdf_visual_gate_aggregate_contact_sheet_display)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_cjk_manifest_count: ``$($evidence.pdf_visual_gate_cjk_manifest_count)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_cjk_copy_search_count: ``$($evidence.pdf_visual_gate_cjk_copy_search_count)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_visual_baseline_manifest_count: ``$($evidence.pdf_visual_gate_visual_baseline_manifest_count)``") | Out-Null
            $lines.Add("    - pdf_visual_gate_visual_baseline_count: ``$($evidence.pdf_visual_gate_visual_baseline_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_summary_count: ``$($evidence.pdf_bounded_ctest_summary_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_pass_count: ``$($evidence.pdf_bounded_ctest_pass_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_skipped_test_count: ``$($evidence.pdf_bounded_ctest_skipped_test_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_selected_test_count: ``$($evidence.pdf_bounded_ctest_selected_test_count)``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_subsets: ``$(@($evidence.pdf_bounded_ctest_subsets) -join ', ')``") | Out-Null
            $lines.Add("    - pdf_bounded_ctest_summary_json_display: ``$(@($evidence.pdf_bounded_ctest_summary_json_display) -join ', ')``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.pdf_full_ctest_readiness_status)) {
                $lines.Add("    - pdf_full_ctest_readiness_status: ``$($evidence.pdf_full_ctest_readiness_status)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_verdict: ``$($evidence.pdf_full_ctest_readiness_verdict)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_release_ready: ``$($evidence.pdf_full_ctest_readiness_release_ready)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_visual_gate_release_evidence_accepted: ``$($evidence.pdf_full_ctest_readiness_visual_gate_release_evidence_accepted)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence: ``$($evidence.pdf_full_ctest_readiness_visual_gate_fresh_full_guarded_evidence)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence: ``$($evidence.pdf_full_ctest_readiness_visual_gate_segmented_full_coverage_evidence)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_summary_json_display: ``$($evidence.pdf_full_ctest_readiness_summary_json_display)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_full_ctest_summary_json_display: ``$($evidence.pdf_full_ctest_readiness_full_ctest_summary_json_display)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_full_ctest_status: ``$($evidence.pdf_full_ctest_readiness_full_ctest_status)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_full_ctest_verdict: ``$($evidence.pdf_full_ctest_readiness_full_ctest_verdict)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_outer_guard_status: ``$($evidence.pdf_full_ctest_readiness_outer_guard_status)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_outer_guard_timed_out: ``$($evidence.pdf_full_ctest_readiness_outer_guard_timed_out)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_selected_test_count: ``$($evidence.pdf_full_ctest_readiness_selected_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_completed_test_count: ``$($evidence.pdf_full_ctest_readiness_completed_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_passed_test_count: ``$($evidence.pdf_full_ctest_readiness_passed_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_failed_test_count: ``$($evidence.pdf_full_ctest_readiness_failed_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_skipped_test_count: ``$($evidence.pdf_full_ctest_readiness_skipped_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_not_run_test_count: ``$($evidence.pdf_full_ctest_readiness_not_run_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_completion_percent: ``$($evidence.pdf_full_ctest_readiness_completion_percent)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_remaining_test_count: ``$($evidence.pdf_full_ctest_readiness_remaining_test_count)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_zero_failed_tests_observed: ``$($evidence.pdf_full_ctest_readiness_zero_failed_tests_observed)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_boundary: ``$($evidence.pdf_full_ctest_readiness_boundary)``") | Out-Null
                $lines.Add("    - pdf_full_ctest_readiness_marker: ``$($evidence.pdf_full_ctest_readiness_marker)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.pdf_visual_gate_attempt_status)) {
                $lines.Add("    - pdf_visual_gate_attempt_status: ``$($evidence.pdf_visual_gate_attempt_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_verdict: ``$($evidence.pdf_visual_gate_attempt_verdict)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_full_visual_gate_status: ``$($evidence.pdf_visual_gate_attempt_full_visual_gate_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_evidence_scope: ``$($evidence.pdf_visual_gate_attempt_evidence_scope)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_summary_json_display: ``$($evidence.pdf_visual_gate_attempt_summary_json_display)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_stage_count: ``$($evidence.pdf_visual_gate_attempt_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_passed_stage_count: ``$($evidence.pdf_visual_gate_attempt_passed_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_failed_stage_count: ``$($evidence.pdf_visual_gate_attempt_failed_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_incomplete_stage_count: ``$($evidence.pdf_visual_gate_attempt_incomplete_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_outer_guard_status: ``$($evidence.pdf_visual_gate_attempt_outer_guard_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_outer_guard_timed_out: ``$($evidence.pdf_visual_gate_attempt_outer_guard_timed_out)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_outer_guard_timeout_seconds: ``$($evidence.pdf_visual_gate_attempt_outer_guard_timeout_seconds)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_cli_export_status: ``$($evidence.pdf_visual_gate_attempt_pdf_cli_export_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_regression_status: ``$($evidence.pdf_visual_gate_attempt_pdf_regression_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_regression_selected_test_count: ``$($evidence.pdf_visual_gate_attempt_pdf_regression_selected_test_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_regression_failed_test_count: ``$($evidence.pdf_visual_gate_attempt_pdf_regression_failed_test_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_pdf_regression_skipped_test_count: ``$($evidence.pdf_visual_gate_attempt_pdf_regression_skipped_test_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_unicode_font_status: ``$($evidence.pdf_visual_gate_attempt_unicode_font_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_cjk_copy_search_status: ``$($evidence.pdf_visual_gate_attempt_cjk_copy_search_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_cjk_copy_search_count: ``$($evidence.pdf_visual_gate_attempt_cjk_copy_search_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_cjk_copy_search_missing_text_count: ``$($evidence.pdf_visual_gate_attempt_cjk_copy_search_missing_text_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_visual_baseline_render_status: ``$($evidence.pdf_visual_gate_attempt_visual_baseline_render_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count: ``$($evidence.pdf_visual_gate_attempt_visual_baseline_fresh_rendered_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_expected_visual_render_count: ``$($evidence.pdf_visual_gate_attempt_expected_visual_render_count)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_aggregate_contact_sheet_status: ``$($evidence.pdf_visual_gate_attempt_aggregate_contact_sheet_status)``") | Out-Null
                $lines.Add("    - pdf_visual_gate_attempt_aggregate_contact_sheet_display: ``$($evidence.pdf_visual_gate_attempt_aggregate_contact_sheet_display)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.pdf_visual_segmented_gate_status)) {
                $lines.Add("    - pdf_visual_segmented_gate_status: ``$($evidence.pdf_visual_segmented_gate_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_verdict: ``$($evidence.pdf_visual_segmented_gate_verdict)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_full_visual_gate_status: ``$($evidence.pdf_visual_segmented_gate_full_visual_gate_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_evidence_scope: ``$($evidence.pdf_visual_segmented_gate_evidence_scope)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_boundary: ``$($evidence.pdf_visual_segmented_gate_boundary)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_summary_json_display: ``$($evidence.pdf_visual_segmented_gate_summary_json_display)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_slice_summary_count: ``$($evidence.pdf_visual_segmented_gate_slice_summary_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_slice_pass_count: ``$($evidence.pdf_visual_segmented_gate_slice_pass_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_slice_failed_count: ``$($evidence.pdf_visual_segmented_gate_slice_failed_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_covered_baseline_count: ``$($evidence.pdf_visual_segmented_gate_covered_baseline_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_expected_visual_render_count: ``$($evidence.pdf_visual_segmented_gate_expected_visual_render_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_attempt_stage_count: ``$($evidence.pdf_visual_segmented_gate_attempt_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_attempt_passed_stage_count: ``$($evidence.pdf_visual_segmented_gate_attempt_passed_stage_count)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_visual_baseline_render_status: ``$($evidence.pdf_visual_segmented_gate_visual_baseline_render_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_contact_sheet_status: ``$($evidence.pdf_visual_segmented_gate_aggregate_contact_sheet_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_contact_sheet_display: ``$($evidence.pdf_visual_segmented_gate_aggregate_contact_sheet_display)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_contact_sheet_bytes: ``$($evidence.pdf_visual_segmented_gate_aggregate_contact_sheet_bytes)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_rebuild_status: ``$($evidence.pdf_visual_segmented_gate_aggregate_rebuild_status)``") | Out-Null
                $lines.Add("    - pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count: ``$($evidence.pdf_visual_segmented_gate_aggregate_rebuild_selected_baseline_count)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.full_visual_gate_status)) {
                $lines.Add("    - full_visual_gate_status: ``$($evidence.full_visual_gate_status)``") | Out-Null
            }
        }
        $lines.Add("- Manifest signoff entrypoints evidence source reports: ``$($rollup.manifest_signoff_entrypoints_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.manifest_signoff_entrypoints_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_status: ``$($evidence.manifest_signoff_entrypoints_status)``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_release_assets_manifest_display: ``$($evidence.manifest_signoff_entrypoints_release_assets_manifest_display)``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_required_entrypoint_count: ``$($evidence.manifest_signoff_entrypoints_required_entrypoint_count)``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_entrypoint_ids: ``$(@($evidence.manifest_signoff_entrypoints_entrypoint_ids) -join ', ')``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_required_contracts: ``$(@($evidence.manifest_signoff_entrypoints_required_contracts) -join ', ')``") | Out-Null
            $lines.Add("    - manifest_signoff_entrypoints_required_fields: ``$(@($evidence.manifest_signoff_entrypoints_required_fields) -join ', ')``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.manifest_signoff_entrypoints_checklist_marker)) {
                $lines.Add("    - manifest_signoff_entrypoints_checklist_marker: ``$($evidence.manifest_signoff_entrypoints_checklist_marker)``") | Out-Null
            }
        }
        $lines.Add("- Project-template readiness checklist entrypoints evidence source reports: ``$($rollup.project_template_readiness_checklist_entrypoints_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.project_template_readiness_checklist_entrypoints_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_status: ``$($evidence.project_template_readiness_checklist_entrypoints_status)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_checklist_label: ``$($evidence.project_template_readiness_checklist_entrypoints_checklist_label)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_checklist_path: ``$($evidence.project_template_readiness_checklist_entrypoints_checklist_path)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_required_entrypoint_count: ``$($evidence.project_template_readiness_checklist_entrypoints_required_entrypoint_count)``") | Out-Null
            $lines.Add("    - project_template_readiness_checklist_entrypoints_entrypoint_ids: ``$(@($evidence.project_template_readiness_checklist_entrypoints_entrypoint_ids) -join ', ')``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$evidence.project_template_readiness_checklist_entrypoints_checklist_marker)) {
                $lines.Add("    - project_template_readiness_checklist_entrypoints_checklist_marker: ``$($evidence.project_template_readiness_checklist_entrypoints_checklist_marker)``") | Out-Null
            }
        }
        $lines.Add("- Release-entry project-template readiness checklist material-safety audit source reports: ``$($rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count)``") | Out-Null
        foreach ($evidence in @($rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports)) {
            $lines.Add("  - source_report: ``$($evidence.path_display)`` schema=``$($evidence.schema)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_status: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_status)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_script: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_script)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: ``$(@($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints) -join ', ')``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_label)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker)``") | Out-Null
            $lines.Add("    - release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: ``$($evidence.release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker)``") | Out-Null
        }
        $lines.Add("") | Out-Null
    }

    $lines.Add("## Governance Metric Review Focus") | Out-Null
    $lines.Add("") | Out-Null
    $focusMetrics = @(
        [ordered]@{
            metric = "real_corpus_confidence"
            id = "numbering_catalog_governance.real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            label = "Numbering real-corpus confidence"
        },
        [ordered]@{
            metric = "delivery_quality"
            id = "table_layout_delivery_governance.delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            label = "Table/layout delivery quality"
        }
    )
    foreach ($focusMetric in $focusMetrics) {
        $metric = Get-GovernanceMetricByContract `
            -Metrics $Summary.governance_metrics `
            -Metric ([string]$focusMetric.metric) `
            -Id ([string]$focusMetric.id) `
            -ReportId ([string]$focusMetric.report_id) `
            -SourceSchema ([string]$focusMetric.source_schema)

        if ($null -eq $metric) {
            $lines.Add("- **$($focusMetric.label)** (``$($focusMetric.metric)``): missing") | Out-Null
            continue
        }

        $lines.Add("- **$($focusMetric.label)** ``$($metric.id)``: metric=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` report=``$($metric.report_id)`` schema=``$($metric.source_schema)``") | Out-Null
        Add-TraceabilityMarkdownLines -Lines $lines -Item $metric
        $lines.Add("  - source_report_display: ``$($metric.source_report_display)``") | Out-Null
        $lines.Add("  - source_json_display: ``$($metric.source_json_display)``") | Out-Null
        Add-GovernanceMetricDetailLines -Lines $lines -Metric $metric
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Report Status") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($report in @($Summary.reports)) {
        $lines.Add("- ``$($report.id)``: status=``$($report.status)`` ready=``$($report.release_ready)`` blockers=``$($report.release_blocker_count)`` actions=``$($report.action_item_count)`` informational_actions=``$($report.informational_action_item_count)`` source_failures=``$($report.source_failure_count)`` source_failure_count=``$($report.source_failure_count)`` schema=``$($report.schema)``") | Out-Null
        $lines.Add("  - summary: ``$($report.expected_summary_display)``") | Out-Null
        $lines.Add("  - source_report_display: ``$($report.source_report_display)``") | Out-Null
        $lines.Add("  - source_json_display: ``$($report.source_json_display)``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace([string]$report.latest_schema_approval_gate_status)) {
            $lines.Add("  - latest_schema_approval_gate_status: ``$($report.latest_schema_approval_gate_status)``") | Out-Null
        }
        if (@($report.schema_approval_status_summary).Count -gt 0) {
            $statusParts = @($report.schema_approval_status_summary | ForEach-Object { "$($_.status)=$($_.count)" })
            $lines.Add("  - schema_approval_status_summary: ``$($statusParts -join ', ')``") | Out-Null
        }
        foreach ($metric in @($report.governance_metrics)) {
            $lines.Add("  - metric ``$($metric.id)``: name=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` report=``$($metric.report_id)`` schema=``$($metric.source_schema)``") | Out-Null
        }
        if (-not [string]::IsNullOrWhiteSpace([string]$report.error)) {
            $lines.Add("  - error: ``$($report.error)``") | Out-Null
        }
        if ([string]::IsNullOrWhiteSpace([string]$report.schema)) {
            $lines.Add("  - build: ``$($report.build_command)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Governance Metrics") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.governance_metrics).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($metric in @($Summary.governance_metrics)) {
            $lines.Add("- ``$($metric.id)``: report=``$($metric.report_id)`` metric=``$($metric.metric)`` level=``$($metric.level)`` score=``$($metric.score)`` schema=``$($metric.source_schema)``") | Out-Null
            Add-TraceabilityMarkdownLines -Lines $lines -Item $metric
            $lines.Add("  - source_report_display: ``$($metric.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($metric.source_json_display)``") | Out-Null
            Add-GovernanceMetricDetailLines -Lines $lines -Metric $metric
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.report_id)`` / ``$($blocker.id)``: project=``$($blocker.project_id)`` template=``$($blocker.template_name)`` candidate=``$($blocker.candidate_type)`` action=``$($blocker.action)`` schema=``$($blocker.source_schema)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - $($blocker.message)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($blocker.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.repair_hint)) {
                $lines.Add("  - repair_hint: $($blocker.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.command_template)) {
                $lines.Add("  - command_template: ``$($blocker.command_template)``") | Out-Null
            }
            Add-TraceabilityMarkdownLines -Lines $lines -Item $blocker
            $lines.Add("  - source_report_display: ``$($blocker.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($blocker.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.readiness_status)) {
                $lines.Add("  - readiness_status: ``$($blocker.readiness_status)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.readiness_release_ready)) {
                $lines.Add("  - readiness_release_ready: ``$($blocker.readiness_release_ready)``") | Out-Null
            }
            Add-ProjectTemplateOnboardingContractMarkdownLines -Lines $lines -Item $blocker
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.report_id)`` / ``$($item.id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` category=``$($item.category)`` release_blocking=``$($item.release_blocking)`` optional=``$($item.optional)`` schema=``$($item.source_schema)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($item.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_hint)) {
                $lines.Add("  - repair_hint: $($item.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
            Add-TraceabilityMarkdownLines -Lines $lines -Item $item
            $lines.Add("  - source_report_display: ``$($item.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.readiness_status)) {
                $lines.Add("  - readiness_status: ``$($item.readiness_status)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.readiness_release_ready)) {
                $lines.Add("  - readiness_release_ready: ``$($item.readiness_release_ready)``") | Out-Null
            }
            Add-ProjectTemplateOnboardingContractMarkdownLines -Lines $lines -Item $item
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Informational Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.informational_action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.informational_action_items)) {
            $lines.Add("- ``$($item.report_id)`` / ``$($item.id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` category=``$($item.category)`` release_blocking=``$($item.release_blocking)`` optional=``$($item.optional)`` schema=``$($item.source_schema)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($item.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_hint)) {
                $lines.Add("  - repair_hint: $($item.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
            Add-TraceabilityMarkdownLines -Lines $lines -Item $item
            $lines.Add("  - source_report_display: ``$($item.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.readiness_status)) {
                $lines.Add("  - readiness_status: ``$($item.readiness_status)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.readiness_release_ready)) {
                $lines.Add("  - readiness_release_ready: ``$($item.readiness_release_ready)``") | Out-Null
            }
            Add-ProjectTemplateOnboardingContractMarkdownLines -Lines $lines -Item $item
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add("- ``$($warning.report_id)`` / ``$($warning.id)``: project=``$($warning.project_id)`` template=``$($warning.template_name)`` candidate=``$($warning.candidate_type)`` action=``$($warning.action)`` schema=``$($warning.source_schema)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.message)) {
                $lines.Add("  - $($warning.message)") | Out-Null
            }
            $repairStrategy = Get-JsonString -Object $warning -Name "repair_strategy"
            $repairHint = Get-JsonString -Object $warning -Name "repair_hint"
            $commandTemplate = Get-JsonString -Object $warning -Name "command_template"
            if (-not [string]::IsNullOrWhiteSpace($repairStrategy)) {
                $lines.Add("  - repair_strategy: ``$repairStrategy``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($repairHint)) {
                $lines.Add("  - repair_hint: $repairHint") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($commandTemplate)) {
                $lines.Add("  - command_template: ``$commandTemplate``") | Out-Null
            }
            Add-TraceabilityMarkdownLines -Lines $lines -Item $warning
            $lines.Add("  - source_report_display: ``$($warning.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($warning.source_json_display)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Next Commands") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($command in @($Summary.next_commands)) {
        $lines.Add("- ``$command``") | Out-Null
    }
    if (@($Summary.next_commands).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    }

    return @($lines)
}

$repoRoot = Resolve-RepoRoot
$resolvedInputRoot = Resolve-RepoPath -RepoRoot $repoRoot -Path $InputRoot -AllowMissing
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "release_governance_handoff.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))
$releaseBlockerRollupOutputDir = Join-Path $resolvedOutputDir "release-blocker-rollup"
$releaseBlockerRollupSummaryPath = Join-Path $releaseBlockerRollupOutputDir "summary.json"
$releaseBlockerRollupMarkdownPath = Join-Path $releaseBlockerRollupOutputDir "release_blocker_rollup.md"

$defaultExpectedReports = @(
    New-ExpectedReport `
        -Id "numbering_catalog_governance" `
        -Title "Numbering Catalog Governance" `
        -RelativeSummary "numbering-catalog-governance/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\build_numbering_catalog_governance_report.ps1 -InputJson .\output\document-skeleton-governance-rollup\summary.json,.\output\numbering-catalog-manifest-checks\summary.json -OutputDir .\output\numbering-catalog-governance"
    New-ExpectedReport `
        -Id "table_layout_delivery_governance" `
        -Title "Table Layout Delivery Governance" `
        -RelativeSummary "table-layout-delivery-governance/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1 -InputJson .\output\table-layout-delivery-rollup\summary.json -OutputDir .\output\table-layout-delivery-governance"
    New-ExpectedReport `
        -Id "content_control_data_binding_governance" `
        -Title "Content Control Data Binding Governance" `
        -RelativeSummary "content-control-data-binding-governance/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1 -InputJson .\output\content-control-data-binding\inspect-content-controls.json,.\output\content-control-data-binding\sync-content-controls-from-custom-xml.json -OutputDir .\output\content-control-data-binding-governance"
    New-ExpectedReport `
        -Id "project_template_delivery_readiness" `
        -Title "Project Template Delivery Readiness" `
        -RelativeSummary "project-template-delivery-readiness/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\build_project_template_delivery_readiness_report.ps1 -InputJson .\output\project-template-onboarding-governance\summary.json,.\output\project-template-schema-approval-history\history.json -OutputDir .\output\project-template-delivery-readiness"
    New-ExpectedReport `
        -Id "schema_patch_confidence_calibration" `
        -Title "Schema Patch Confidence Calibration" `
        -RelativeSummary "schema-patch-confidence-calibration/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1 -InputJson .\output\project-template-smoke\summary.json,.\output\project-template-schema-approval-history\history.json -OutputDir .\output\schema-patch-confidence-calibration"
    New-ExpectedReport `
        -Id "docx_functional_smoke_readiness" `
        -Title "DOCX Functional Smoke Readiness" `
        -RelativeSummary "docx-functional-smoke-readiness/summary.json" `
        -BuildCommand "pwsh -ExecutionPolicy Bypass -File .\scripts\check_docx_functional_smoke_readiness.ps1 -OutputDir .\output\docx-functional-smoke-readiness"
)
$expectedReports = @(
    if ($ExpectedReportProfile -ne "explicit-only") {
        $defaultExpectedReports
    }
)

$reports = New-Object 'System.Collections.Generic.List[object]'
$reportById = @{}
$knownExpectedReportById = @{}
foreach ($expected in @($defaultExpectedReports)) {
    $knownExpectedReportById[[string]$expected.id] = $expected
}
$explicitWarningReports = New-Object 'System.Collections.Generic.List[object]'
foreach ($expected in @($expectedReports)) {
    $path = Join-Path $resolvedInputRoot ([string]$expected.relative_summary)
    if (-not (Test-Path -LiteralPath $path)) {
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Status "missing"
        $reports.Add($entry) | Out-Null
        $reportById[[string]$expected.id] = $entry
        continue
    }

    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Source "default" `
            -Json $json `
            -Status "loaded"
        $reports.Add($entry) | Out-Null
        $reportById[[string]$expected.id] = $entry
    } catch {
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Status "failed" `
            -ErrorMessage $_.Exception.Message
        $reports.Add($entry) | Out-Null
        $reportById[[string]$expected.id] = $entry
    }
}

foreach ($input in @(Expand-InputPathList -Paths $InputJson)) {
    $path = Resolve-RepoPath -RepoRoot $repoRoot -Path $input -AllowMissing
    if (-not (Test-Path -LiteralPath $path)) { continue }
    try {
        $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        $kind = Get-ReportKind -Summary $json
        if (-not $knownExpectedReportById.ContainsKey($kind)) {
            if ([string]::Equals($kind, "featherdoc.release_candidate_summary", [System.StringComparison]::OrdinalIgnoreCase)) {
                $explicitWarningReports.Add((New-ExplicitReleaseCandidateWarningReport `
                            -RepoRoot $repoRoot `
                            -Path $path `
                            -Json $json `
                            -Index ($explicitWarningReports.Count + 1))) | Out-Null
            }
            continue
        }
        $expected = $knownExpectedReportById[$kind]
        $entry = New-ReportEntry `
            -RepoRoot $repoRoot `
            -Id ([string]$expected.id) `
            -Title ([string]$expected.title) `
            -ExpectedSummaryPath $path `
            -BuildCommand ([string]$expected.build_command) `
            -Source "explicit" `
            -Json $json `
            -Status "loaded"
        $replacedDefaultReport = $false
        for ($index = 0; $index -lt $reports.Count; $index++) {
            if ([string]$reports[$index].id -eq [string]$expected.id) {
                $reports[$index] = $entry
                $replacedDefaultReport = $true
                break
            }
        }
        if (-not $replacedDefaultReport) {
            $reports.Add($entry) | Out-Null
        }
        $reportById[[string]$expected.id] = $entry
    } catch {
        Write-Step "Skipping explicit report '$path': $($_.Exception.Message)"
    }
}

$projectTemplateOnboardingGovernanceContract = Get-ProjectTemplateOnboardingGovernanceContract `
    -RepoRoot $repoRoot `
    -InputRoot $resolvedInputRoot `
    -InputJson $InputJson

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$informationalActionItems = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'
foreach ($report in @($reports.ToArray())) {
    Add-NormalizedBlockers `
        -Collection $releaseBlockers `
        -Report $report `
        -ProjectTemplateOnboardingGovernanceContract $projectTemplateOnboardingGovernanceContract
    Add-NormalizedActions `
        -Collection $actionItems `
        -InformationalCollection $informationalActionItems `
        -Report $report `
        -ProjectTemplateOnboardingGovernanceContract $projectTemplateOnboardingGovernanceContract
    Add-NormalizedActions `
        -Collection $actionItems `
        -InformationalCollection $informationalActionItems `
        -Report $report `
        -ProjectTemplateOnboardingGovernanceContract $projectTemplateOnboardingGovernanceContract `
        -ForceInformational
    Add-NormalizedWarnings -Collection $warnings -Report $report
}
foreach ($report in @($explicitWarningReports.ToArray())) {
    Add-NormalizedWarnings -Collection $warnings -Report $report
}

$loadedReportCount = @($reports.ToArray() | Where-Object { $_.status -notin @("missing", "failed") }).Count
$missingReportCount = @($reports.ToArray() | Where-Object { $_.status -eq "missing" }).Count
$failedReportCount = @($reports.ToArray() | Where-Object { $_.status -eq "failed" -or $_.source_failure_count -gt 0 }).Count
$warningCount = 0
foreach ($report in @($reports.ToArray())) {
    $warningCount += [int]$report.warning_count
}
foreach ($report in @($explicitWarningReports.ToArray())) {
    $warningCount += @($report.warnings).Count
}
$governanceMetrics = New-Object 'System.Collections.Generic.List[object]'
foreach ($report in @($reports.ToArray())) {
    foreach ($metric in @($report.governance_metrics)) {
        $governanceMetrics.Add($metric) | Out-Null
    }
}

$status = if ($failedReportCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0) {
    "blocked"
} elseif ($missingReportCount -gt 0 -or $warningCount -gt 0) {
    "needs_review"
} else {
    "ready"
}

$rollupCommand = 'pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 -ReleaseBlockerRollupAutoDiscover'
$nextCommands = New-Object 'System.Collections.Generic.List[string]'
foreach ($report in @($reports.ToArray() | Where-Object { $_.status -eq "missing" })) {
    $nextCommands.Add([string]$report.build_command) | Out-Null
}
$nextCommands.Add($rollupCommand) | Out-Null

$summary = [ordered]@{
    schema = "featherdoc.release_governance_handoff_report.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    input_root = $resolvedInputRoot
    input_root_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedInputRoot
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    expected_report_profile = $ExpectedReportProfile
    release_blocker_rollup = [ordered]@{
        included = [bool]$IncludeReleaseBlockerRollup
        status = if ($IncludeReleaseBlockerRollup) { "pending" } else { "not_requested" }
        output_dir = $releaseBlockerRollupOutputDir
        output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupOutputDir
        summary_json = $releaseBlockerRollupSummaryPath
        summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupSummaryPath
        report_markdown = $releaseBlockerRollupMarkdownPath
        report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $releaseBlockerRollupMarkdownPath
        source_report_count = 0
        source_failure_count = 0
        release_blocker_count = 0
        action_item_count = 0
        informational_action_item_count = 0
        warning_count = 0
        pdf_visual_gate_evidence_source_report_count = 0
        pdf_visual_gate_evidence_source_reports = @()
        manifest_signoff_entrypoints_source_report_count = 0
        manifest_signoff_entrypoints_source_reports = @()
        project_template_readiness_checklist_entrypoints_source_report_count = 0
        project_template_readiness_checklist_entrypoints_source_reports = @()
        release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 0
        release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @()
    }
    expected_report_count = $expectedReports.Count
    loaded_report_count = $loadedReportCount
    missing_report_count = $missingReportCount
    failed_report_count = $failedReportCount
    reports = @($reports.ToArray())
    project_template_delivery_readiness_contract = ($reports.ToArray() | Where-Object { [string]$_.id -eq "project_template_delivery_readiness" } | Select-Object -First 1)
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
    governance_metric_count = $governanceMetrics.Count
    governance_metrics = @($governanceMetrics.ToArray())
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    informational_action_item_count = $informationalActionItems.Count
    informational_action_items = @($informationalActionItems.ToArray())
    warning_count = $warningCount
    warnings = @($warnings.ToArray())
    next_commands = @($nextCommands.ToArray())
}

Write-ReleaseMaterialFiles -Summary $summary -SummaryPath $summaryPath -MarkdownPath $markdownPath -JsonDepth 32

if ($IncludeReleaseBlockerRollup) {
    $loadedReports = @($reports.ToArray() | Where-Object { $_.status -notin @("missing", "failed") })
    $loadedReportInputs = @(
        foreach ($report in $loadedReports) {
            [string]$report.expected_summary
        }
        foreach ($input in @(Expand-InputPathList -Paths $InputJson)) {
            $path = Resolve-RepoPath -RepoRoot $repoRoot -Path $input -AllowMissing
            if (Test-Path -LiteralPath $path) {
                $path
            }
        }
    )
    Invoke-ReleaseBlockerRollup `
        -RepoRoot $repoRoot `
        -OutputDir $releaseBlockerRollupOutputDir `
        -InputJson $loadedReportInputs
    if (-not (Test-Path -LiteralPath $releaseBlockerRollupSummaryPath)) {
        throw "Release blocker rollup was not written: $releaseBlockerRollupSummaryPath"
    }
    if (-not (Test-Path -LiteralPath $releaseBlockerRollupMarkdownPath)) {
        throw "Release blocker rollup Markdown was not written: $releaseBlockerRollupMarkdownPath"
    }

    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseBlockerRollupSummaryPath | ConvertFrom-Json
    $pdfVisualGateEvidence = @(Get-PdfVisualGateRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.status = [string]$rollupSummary.status
    $summary.release_blocker_rollup.source_report_count = [int]$rollupSummary.source_report_count
    $summary.release_blocker_rollup.source_failure_count = [int]$rollupSummary.source_failure_count
    $summary.release_blocker_rollup.release_blocker_count = [int]$rollupSummary.release_blocker_count
    $summary.release_blocker_rollup.action_item_count = [int]$rollupSummary.action_item_count
    $summary.release_blocker_rollup.informational_action_item_count = [int](Get-JsonInt -Object $rollupSummary -Name "informational_action_item_count")
    $summary.release_blocker_rollup.warning_count = [int]$rollupSummary.warning_count
    $summary.release_blocker_rollup.pdf_visual_gate_evidence_source_report_count = @($pdfVisualGateEvidence).Count
    $summary.release_blocker_rollup.pdf_visual_gate_evidence_source_reports = @($pdfVisualGateEvidence)
    $manifestSignoffEvidence = @(Get-ManifestSignoffEntrypointsRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.manifest_signoff_entrypoints_source_report_count = @($manifestSignoffEvidence).Count
    $summary.release_blocker_rollup.manifest_signoff_entrypoints_source_reports = @($manifestSignoffEvidence)
    $projectTemplateChecklistEvidence = @(Get-ProjectTemplateReadinessChecklistEntrypointsRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_report_count = @($projectTemplateChecklistEvidence).Count
    $summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_reports = @($projectTemplateChecklistEvidence)
    $releaseEntryChecklistAuditEvidence = @(Get-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditRollupEvidence -RollupSummary $rollupSummary)
    $summary.release_blocker_rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = @($releaseEntryChecklistAuditEvidence).Count
    $summary.release_blocker_rollup.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @($releaseEntryChecklistAuditEvidence)
    Write-ReleaseMaterialFiles -Summary $summary -SummaryPath $summaryPath -MarkdownPath $markdownPath -JsonDepth 32
}

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($FailOnMissing -and $missingReportCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warningCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
if ($failedReportCount -gt 0) { exit 1 }
