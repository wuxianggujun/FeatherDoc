param(
    [string]$RepoRoot = "",
    [string]$SummaryJson = "",
    [switch]$Quiet
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    param([string]$InputRoot)

    if ([string]::IsNullOrWhiteSpace($InputRoot)) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }

    return (Resolve-Path $InputRoot).Path
}

function Read-Utf8Text {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        Set-FailureDetail -Kind "utf8_bom" -Path $Path
        throw "File must be UTF-8 without BOM: $Path"
    }

    return [System.Text.Encoding]::UTF8.GetString($bytes)
}


function Set-FailureDetail {
    param(
        [string]$Kind,
        [string]$RuleId = "",
        [string]$Label = "",
        [string]$Path = "",
        [string]$ExpectedText = "",
        [int]$LineNumber = 0,
        [int]$ColumnNumber = 0,
        [string]$Excerpt = ""
    )

    if ([string]::IsNullOrWhiteSpace($RuleId) -and
        -not [string]::IsNullOrWhiteSpace($Kind)) {
        $RuleId = "release_metadata_docs.$Kind"
    }

    $script:FailureKind = $Kind
    $script:FailureRuleId = $RuleId
    $script:FailureLabel = $Label
    $script:FailurePath = $Path
    $script:FailureExpectedText = $ExpectedText
    $script:FailureLineNumber = $LineNumber
    $script:FailureColumnNumber = $ColumnNumber
    $script:FailureExcerpt = $Excerpt
}

function Resolve-OptionalOutputPath {
    param(
        [string]$Path,
        [string]$BaseRoot
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $BaseRoot $Path))
}

function Resolve-FallbackOutputPath {
    param(
        [string]$Path,
        [string]$InputRoot,
        [string]$ResolvedRoot
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    if (-not [string]::IsNullOrWhiteSpace($ResolvedRoot)) {
        return [System.IO.Path]::GetFullPath((Join-Path $ResolvedRoot $Path))
    }

    if (-not [string]::IsNullOrWhiteSpace($InputRoot)) {
        return [System.IO.Path]::GetFullPath((Join-Path ([System.IO.Path]::GetFullPath($InputRoot)) $Path))
    }

    return [System.IO.Path]::GetFullPath((Join-Path (Join-Path $PSScriptRoot "..") $Path))
}

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $parentDir = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parentDir)) {
        New-Item -ItemType Directory -Path $parentDir -Force | Out-Null
    }

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}


function Get-RepoRelativePath {
    param(
        [string]$BaseRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($BaseRoot) -or
        [string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $resolvedBaseRoot = [System.IO.Path]::GetFullPath($BaseRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if (-not $resolvedBaseRoot.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $resolvedBaseRoot += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = New-Object System.Uri($resolvedBaseRoot)
    $pathUri = New-Object System.Uri($resolvedPath)
    if ($baseUri.Scheme -ne $pathUri.Scheme) {
        return ""
    }

    $relativeUri = $baseUri.MakeRelativeUri($pathUri)
    if ($relativeUri.IsAbsoluteUri) {
        return ""
    }

    $relativePath = [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace('\', '/')
    if ($relativePath -match '^\.\./') {
        return ""
    }

    return $relativePath
}

function Write-SummaryJson {
    param(
        [string]$Path,
        [string]$Status,
        [string]$RepoRoot,
        [object[]]$CheckedDocuments,
        [string[]]$PipelineMarkers,
        [string[]]$ChecklistMarkers,
        [string[]]$DocumentGovernanceMarkers,
        [string[]]$PolicyMarkers,
        [string[]]$EntrypointMarkers,
        [string]$ErrorMessage = ""
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $checkedAtUtc = [DateTime]::UtcNow.ToString(
        "yyyy-MM-ddTHH:mm:ss'Z'",
        [System.Globalization.CultureInfo]::InvariantCulture
    )
    $powershellEdition = ""
    if ($PSVersionTable.ContainsKey("PSEdition")) {
        $powershellEdition = [string]$PSVersionTable.PSEdition
    }

    $summary = [pscustomobject]@{
        summary_schema_version = 1
        checker_name = "check_release_metadata_docs.ps1"
        checked_at_utc = $checkedAtUtc
        powershell_edition = $powershellEdition
        powershell_version = $PSVersionTable.PSVersion.ToString()
        output_encoding = "UTF-8 without BOM"
        status = $Status
        error_message = $ErrorMessage
        failure_kind = $script:FailureKind
        failure_rule_id = $script:FailureRuleId
        failure_label = $script:FailureLabel
        failure_path = $script:FailurePath
        failure_relative_path = Get-RepoRelativePath -BaseRoot $RepoRoot -Path $script:FailurePath
        failure_expected_text = $script:FailureExpectedText
        failure_line_number = $script:FailureLineNumber
        failure_column_number = $script:FailureColumnNumber
        failure_excerpt = $script:FailureExcerpt
        summary_json_path = $Path
        summary_json_relative_path = Get-RepoRelativePath -BaseRoot $RepoRoot -Path $Path
        repo_root = $RepoRoot
        checked_document_count = $CheckedDocuments.Count
        required_marker_count = $PipelineMarkers.Count + $ChecklistMarkers.Count + `
            $DocumentGovernanceMarkers.Count + $PolicyMarkers.Count + $EntrypointMarkers.Count
        required_pipeline_marker_count = $PipelineMarkers.Count
        required_checklist_marker_count = $ChecklistMarkers.Count
        required_document_governance_marker_count = $DocumentGovernanceMarkers.Count
        required_policy_marker_count = $PolicyMarkers.Count
        required_entrypoint_marker_count = $EntrypointMarkers.Count
        checked_documents = $CheckedDocuments
        required_pipeline_markers = $PipelineMarkers
        required_checklist_markers = $ChecklistMarkers
        required_document_governance_markers = $DocumentGovernanceMarkers
        required_policy_markers = $PolicyMarkers
        required_entrypoint_markers = $EntrypointMarkers
    }

    $json = $summary | ConvertTo-Json -Depth 6
    Write-Utf8NoBomFile -Path $Path -Text ($json + [Environment]::NewLine)
}

function Assert-FileExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        Set-FailureDetail -Kind "missing_file" -Label $Label -Path $Path
        throw "Missing ${Label}: $Path"
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label,
        [string]$Path = ""
    )

    if (-not $Text.Contains($ExpectedText)) {
        Set-FailureDetail `
            -Kind "missing_text" `
            -Label $Label `
            -Path $Path `
            -ExpectedText $ExpectedText
        throw "$Label is missing expected text: $ExpectedText"
    }
}

function Assert-NoTrailingWhitespace {
    param(
        [string]$Text,
        [string]$Path
    )

    $lines = $Text -split "`r?`n"
    for ($index = 0; $index -lt $lines.Count; ++$index) {
        if ($lines[$index].TrimEnd() -ne $lines[$index]) {
            $columnNumber = $lines[$index].TrimEnd().Length + 1
            Set-FailureDetail `
                -Kind "trailing_whitespace" `
                -Path $Path `
                -LineNumber ($index + 1) `
                -ColumnNumber $columnNumber `
                -Excerpt ($lines[$index])
            throw "Trailing whitespace in $Path line $($index + 1), column $columnNumber."
        }
    }
}

function Assert-NoTabs {
    param(
        [string]$Text,
        [string]$Path
    )

    $lines = $Text -split "`r?`n"
    for ($index = 0; $index -lt $lines.Count; ++$index) {
        $tabColumnIndex = $lines[$index].IndexOf("`t")
        if ($tabColumnIndex -ge 0) {
            $columnNumber = $tabColumnIndex + 1
            Set-FailureDetail `
                -Kind "tab_character" `
                -Path $Path `
                -LineNumber ($index + 1) `
                -ColumnNumber $columnNumber `
                -Excerpt ($lines[$index])
            throw "Tab character found in $Path line $($index + 1), column $columnNumber."
        }
    }
}

$script:FailureKind = ""
$script:FailureRuleId = ""
$script:FailureLabel = ""
$script:FailurePath = ""
$script:FailureExpectedText = ""
$script:FailureLineNumber = 0
$script:FailureColumnNumber = 0
$script:FailureExcerpt = ""

$pipelineExpectedMarkers = @(
    "run_word_visual_release_gate.ps1",
    "run_release_candidate_checks.ps1",
    "sync_visual_review_verdict.ps1",
    "sync_latest_visual_review_verdict.ps1",
    "-GateSummaryJson",
    "readme_gallery",
    "assets_dir",
    "refresh_readme_visual_assets.ps1",
    "write_release_note_bundle.ps1",
    "check_word_visual_release_gate_preflight.ps1",
    '``featherdoc.word_visual_release_gate_preflight.v1``',
    '``word_visual_release_gate_preflight_static_contract_only``',
    '``output_encoding``',
    "UTF-8 without BOM",
    '``preflight_ready``',
    '``release_ready``',
    "docx_functional_smoke_readiness",
    '``featherdoc.docx_functional_smoke_readiness.v1``',
    "docx_functional_smoke_readiness_trace",
    "persisted_docx_functional_smoke_evidence_only",
    "summary_json_display",
    "report_markdown_display",
    "word_visual_smoke.pending_manual_review",
    "release_blocker_count",
    "review_task_summary",
    "release_note_bundle",
    "entrypoint_count",
    "required_entrypoint_count",
    "entrypoints[]",
    '``path_display``',
    '``location``',
    "release_assets_manifest.json",
    "package_release_assets.ps1",
    '``required``',
    "assert_release_material_safety.ps1",
    "-SkipMaterialSafetyAudit",
    "warning_count",
    "release_blocker_rollup.md",
    "release_governance_handoff.md",
    "release_governance_pipeline.md",
    '``featherdoc.release_governance_local_closure.v1``',
    "local_governance_closure",
    "local_governance_closure.status",
    "local_governance_closure.closed",
    "governance_detail_source",
    "pipeline_summary_json",
    "pipeline_summary_json_display",
    "pipeline_report_markdown",
    "pipeline_report_markdown_display",
    "release_governance_handoff.release_blockers[]",
    "release_governance_handoff.warnings[]",
    "release_governance_handoff.action_items[]",
    '``source_json_display``',
    '``featherdoc.release_governance_pipeline_report.v1``',
    '``stages[]``',
    '``stage_id``',
    '``stage_title``',
    '``open_command``',
    "final_governance_report_count",
    "final_governance_reports",
    "required_stage_count",
    "completed_required_stage_count",
    "required_stages",
    "rerun_pdf_controlled_visual_smoke_check",
    "check_pdf_controlled_visual_smoke.ps1",
    "controlled_visual_smoke_json_display",
    "restore_docx_functional_smoke_evidence",
    "Word visual standard review metadata evidence",
    "word_visual_standard_review_metadata_source_reports",
    "task_reviews=",
    "release-candidate-checks",
    "release-candidate-checks-source",
    "report/ARTIFACT_GUIDE.md",
    "report/REVIEWER_CHECKLIST.md",
    "report/release_handoff.md",
    "report/release_body.zh-CN.md",
    "report/release_summary.zh-CN.md",
    '``id``',
    '``action``',
    '``message``',
    '``source_schema``',
    '``source_report_display``',
    '``output_gap_count``',
    '``missing_output_count``',
    '``output gap checks``',
    '``missing outputs``',
    '``style_merge_suggestion_count``',
    '``featherdoc.project_template_delivery_readiness_report.v1``',
    '``featherdoc.content_control_data_binding_governance_report.v1``',
    '``featherdoc.numbering_catalog_governance_report.v1``',
    '``featherdoc.table_layout_delivery_governance_report.v1``',
    '``sync_bound_content_control``',
    '``numbering_catalog_governance.real_corpus_alignment_gap``',
    '``delivery_quality``',
    "ReleaseBlockerRollupFailOnWarning",
    "ReleaseGovernanceHandoffFailOnWarning"
)
$checklistExpectedMarkers = @(
    ':doc:`release_metadata_pipeline_zh`',
    ':doc:`document_governance_acceptance_zh`',
    "check_word_visual_release_gate_preflight.ps1",
    "check_word_visual_release_gate_preflight_test.ps1",
    "word_visual_release_gate_preflight_route_docs_contract",
    "word_visual_release_gate_preflight_route_docs_contract_test.ps1",
    '``featherdoc.word_visual_release_gate_preflight.v1``',
    '``word_visual_release_gate_preflight_static_contract_only``',
    '``output_encoding``',
    "UTF-8 without BOM",
    '``preflight_ready``',
    '``release_ready``',
    "word_visual_release_gate_smoke_verdict",
    "release_candidate_visual_verdict",
    "sync_latest_visual_review_verdict_table_style_quality",
    "sync_latest_visual_review_verdict_cmake_contract",
    "sync_visual_review_verdict_(section_page_setup|page_number_fields|curated_visual_bundle)",
    "open_latest_word_review_task_curated_source_kind_test.ps1",
    "open_latest_word_review_task.ps1 -SourceKind table-style-quality-visual-regression-bundle",
    "latest_table-style-quality-visual-regression-bundle_task.json",
    "release_note_bundle_visual_verdict_metadata",
    "Word visual standard review metadata evidence",
    "word_visual_standard_review_metadata_source_reports",
    "task_reviews=",
    "source_schema=featherdoc.release_candidate_summary",
    "release-candidate-checks",
    "public_release_wording_regression_test.ps1",
    "git diff --check",
    "release_note_bundle",
    "release_assets_manifest.json",
    "manifest_signoff_entrypoints",
    "entrypoint path contract",
    "package_release_assets.ps1",
    "package_release_assets_safety_test.ps1",
    "package_release_assets_allow_incomplete_test.ps1",
    "assert_release_material_safety.ps1",
    "release governance warning",
    "warning_count",
    "release_blocker_rollup",
    "release_governance_handoff",
    "release_governance_pipeline",
    "local_governance_closure",
    "local_governance_closure.status",
    "local_governance_closure.closed",
    "governance_detail_source",
    "pipeline_summary_json_display",
    "pipeline_report_markdown_display",
    "release_governance_handoff.release_blockers[]",
    "release_governance_handoff.warnings[]",
    "release_governance_handoff.action_items[]",
    "stages[]",
    "stage_id",
    "stage_title",
    "final_governance_report_count",
    "final_governance_reports",
    "required_stage_count",
    "completed_required_stage_count",
    "required_stages",
    "source_schema",
    "source_report_display",
    "source_json_display",
    "style_merge_suggestion_count",
    "check_docx_functional_smoke_readiness.ps1",
    "docx_functional_smoke_readiness_test.ps1",
    "docx_functional_smoke_readiness_route_docs_contract",
    "docx_functional_smoke_readiness_route_docs_contract_test.ps1",
    "docx_functional_smoke_readiness",
    '``featherdoc.docx_functional_smoke_readiness.v1``',
    "docx_functional_smoke_readiness_trace",
    "persisted_docx_functional_smoke_evidence_only",
    "summary_json_display",
    "report_markdown_display",
    "word_visual_smoke.pending_manual_review",
    "release_blocker_count",
    "ReleaseBlockerRollupFailOnWarning",
    "ReleaseGovernanceHandoffFailOnWarning",
    "build_project_template_delivery_readiness_report_test.ps1",
    "build_content_control_data_binding_governance_report_test.ps1",
    "build_numbering_catalog_governance_report_test.ps1",
    "build_table_layout_delivery_governance_report_test.ps1",
    "pdf_floating_table_support_coverage",
    "pdf_floating_table_reviewer_focus",
    "metadata-only tblpPr"
)
$documentGovernanceExpectedMarkers = @(
    "document_governance_acceptance.v1",
    "document_governance_primary_track",
    "featherdoc.project_template_delivery_readiness_report.v1",
    "featherdoc.content_control_data_binding_governance_report.v1",
    "content_control_data_binding.bound_placeholder",
    "sync_bound_content_control",
    "featherdoc.numbering_catalog_governance_report.v1",
    "numbering_catalog_governance.real_corpus_alignment_gap",
    "featherdoc.table_layout_delivery_governance_report.v1",
    "delivery_quality",
    "pdf_floating_table_support_coverage",
    "pdf_floating_table_reviewer_focus",
    "metadata-only tblpPr"
)
$policyExpectedMarkers = @(
    ':doc:`release_metadata_pipeline_zh`',
    "ReleaseBlockerRollupFailOnWarning",
    "ReleaseGovernanceHandoffFailOnWarning",
    "Word visual standard review metadata evidence",
    "word_visual_standard_review_metadata_source_reports",
    "task_reviews=",
    "source_schema=featherdoc.release_candidate_summary",
    "release-candidate-checks",
    "report/ARTIFACT_GUIDE.md",
    "report/REVIEWER_CHECKLIST.md",
    "report/release_handoff.md",
    "release_note_bundle",
    "release_assets_manifest.json",
    "package_release_assets.ps1",
    "assert_release_material_safety.ps1",
    '``required``',
    '``location``',
    '``path_display``'
)
$entrypointExpectedMarkers = @(
    "release_metadata_pipeline_zh",
    "release_metadata_maintenance_checklist_zh"
)
$resolvedRepoRoot = ""
$summaryJsonPath = ""
$checkedDocuments = @()

try {
    $resolvedRepoRoot = Resolve-RepoRoot -InputRoot $RepoRoot
    $summaryJsonPath = Resolve-OptionalOutputPath -Path $SummaryJson -BaseRoot $resolvedRepoRoot
    $checkedDocuments = @(
        [pscustomobject]@{
            label = "release metadata pipeline doc"
            relative_path = "docs/release_metadata_pipeline_zh.rst"
            path = Join-Path (Join-Path $resolvedRepoRoot "docs") "release_metadata_pipeline_zh.rst"
        },
        [pscustomobject]@{
            label = "release metadata maintenance checklist doc"
            relative_path = "docs/release_metadata_maintenance_checklist_zh.rst"
            path = Join-Path (Join-Path $resolvedRepoRoot "docs") "release_metadata_maintenance_checklist_zh.rst"
        },
        [pscustomobject]@{
            label = "document governance acceptance doc"
            relative_path = "docs/document_governance_acceptance_zh.rst"
            path = Join-Path (Join-Path $resolvedRepoRoot "docs") "document_governance_acceptance_zh.rst"
        },
        [pscustomobject]@{
            label = "release policy doc"
            relative_path = "docs/release_policy_zh.rst"
            path = Join-Path (Join-Path $resolvedRepoRoot "docs") "release_policy_zh.rst"
        },
        [pscustomobject]@{
            label = "Sphinx index doc"
            relative_path = "docs/index.rst"
            path = Join-Path (Join-Path $resolvedRepoRoot "docs") "index.rst"
        },
        [pscustomobject]@{
            label = "English README"
            relative_path = "README.md"
            path = Join-Path $resolvedRepoRoot "README.md"
        },
        [pscustomobject]@{
            label = "Chinese README"
            relative_path = "README.zh-CN.md"
            path = Join-Path $resolvedRepoRoot "README.zh-CN.md"
        }
    )
    $pipelinePath = $checkedDocuments[0].path
    $checklistPath = $checkedDocuments[1].path
    $documentGovernancePath = $checkedDocuments[2].path
    $policyPath = $checkedDocuments[3].path
    $indexPath = $checkedDocuments[4].path
    $readmePath = $checkedDocuments[5].path
    $readmeZhPath = $checkedDocuments[6].path

    Assert-FileExists -Path $pipelinePath -Label "release metadata pipeline doc"
    Assert-FileExists -Path $checklistPath -Label "release metadata maintenance checklist doc"
    Assert-FileExists -Path $documentGovernancePath -Label "document governance acceptance doc"
    Assert-FileExists -Path $policyPath -Label "release policy doc"
    Assert-FileExists -Path $indexPath -Label "Sphinx index doc"
    Assert-FileExists -Path $readmePath -Label "English README"
    Assert-FileExists -Path $readmeZhPath -Label "Chinese README"

    $pipelineText = Read-Utf8Text -Path $pipelinePath
    $checklistText = Read-Utf8Text -Path $checklistPath
    $documentGovernanceText = Read-Utf8Text -Path $documentGovernancePath
    $policyText = Read-Utf8Text -Path $policyPath
    $indexText = Read-Utf8Text -Path $indexPath
    $readmeText = Read-Utf8Text -Path $readmePath
    $readmeZhText = Read-Utf8Text -Path $readmeZhPath

    foreach ($doc in @(
            @{ Path = $pipelinePath; Text = $pipelineText },
            @{ Path = $checklistPath; Text = $checklistText },
            @{ Path = $documentGovernancePath; Text = $documentGovernanceText },
            @{ Path = $policyPath; Text = $policyText },
            @{ Path = $indexPath; Text = $indexText },
            @{ Path = $readmePath; Text = $readmeText },
            @{ Path = $readmeZhPath; Text = $readmeZhText }
        )) {
        Assert-NoTrailingWhitespace -Text $doc.Text -Path $doc.Path
        Assert-NoTabs -Text $doc.Text -Path $doc.Path
    }

    foreach ($expected in $pipelineExpectedMarkers) {
        Assert-ContainsText `
            -Text $pipelineText `
            -ExpectedText $expected `
            -Label "release metadata pipeline doc" `
            -Path $pipelinePath
    }

    foreach ($expected in $checklistExpectedMarkers) {
        Assert-ContainsText `
            -Text $checklistText `
            -ExpectedText $expected `
            -Label "release metadata maintenance checklist doc" `
            -Path $checklistPath
    }

    foreach ($expected in $documentGovernanceExpectedMarkers) {
        Assert-ContainsText `
            -Text $documentGovernanceText `
            -ExpectedText $expected `
            -Label "document governance acceptance doc" `
            -Path $documentGovernancePath
    }

    foreach ($expected in $policyExpectedMarkers) {
        Assert-ContainsText `
            -Text $policyText `
            -ExpectedText $expected `
            -Label "release policy doc" `
            -Path $policyPath
    }

    foreach ($expected in $entrypointExpectedMarkers) {
        foreach ($entrypoint in @(
                @{ Label = "Sphinx index doc"; Text = $indexText; Path = $indexPath },
                @{ Label = "English README"; Text = $readmeText; Path = $readmePath },
                @{ Label = "Chinese README"; Text = $readmeZhText; Path = $readmeZhPath }
            )) {
            Assert-ContainsText `
                -Text $entrypoint.Text `
                -ExpectedText $expected `
                -Label $entrypoint.Label `
                -Path $entrypoint.Path
        }
    }

    Write-SummaryJson `
        -Path $summaryJsonPath `
        -Status "passed" `
        -RepoRoot $resolvedRepoRoot `
        -CheckedDocuments $checkedDocuments `
        -PipelineMarkers $pipelineExpectedMarkers `
        -ChecklistMarkers $checklistExpectedMarkers `
        -DocumentGovernanceMarkers $documentGovernanceExpectedMarkers `
        -PolicyMarkers $policyExpectedMarkers `
        -EntrypointMarkers $entrypointExpectedMarkers

    if (-not $Quiet) {
        Write-Host "Release metadata docs check passed."
    }
} catch {
    $errorMessage = $_.Exception.Message
    $summaryJsonPath = Resolve-FallbackOutputPath `
        -Path $SummaryJson `
        -InputRoot $RepoRoot `
        -ResolvedRoot $resolvedRepoRoot

    if (-not [string]::IsNullOrWhiteSpace($summaryJsonPath)) {
        try {
            Write-SummaryJson `
                -Path $summaryJsonPath `
                -Status "failed" `
                -RepoRoot $resolvedRepoRoot `
                -CheckedDocuments $checkedDocuments `
                -PipelineMarkers $pipelineExpectedMarkers `
                -ChecklistMarkers $checklistExpectedMarkers `
                -DocumentGovernanceMarkers $documentGovernanceExpectedMarkers `
                -PolicyMarkers $policyExpectedMarkers `
                -EntrypointMarkers $entrypointExpectedMarkers `
                -ErrorMessage $errorMessage
        } catch {
            Write-Warning "Unable to write release metadata docs failure summary: $($_.Exception.Message)"
        }
    }

    throw
}
