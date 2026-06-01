param(
    [string]$RepoRoot = "",
    [string]$WorkingDir = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-DefaultRepoRoot {
    if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }

    return (Resolve-Path $RepoRoot).Path
}

function Resolve-DefaultWorkingDir {
    if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
        $defaultDir = Join-Path $resolvedRepoRoot "output\check-release-metadata-docs-test"
        return [System.IO.Path]::GetFullPath($defaultDir)
    }

    return [System.IO.Path]::GetFullPath($WorkingDir)
}

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $parentDir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $parentDir -Force | Out-Null

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Write-Utf8BomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $parentDir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $parentDir -Force | Out-Null

    $encoding = New-Object System.Text.UTF8Encoding($true)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}


function Assert-FileHasNoBom {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw "File must be UTF-8 without BOM: $Path"
    }
}

function Assert-ArrayContains {
    param(
        [object[]]$Values,
        [string]$ExpectedValue,
        [string]$Message
    )

    foreach ($value in $Values) {
        if ($value -eq $ExpectedValue) {
            return
        }
    }

    throw $Message
}

function Assert-ScriptParses {
    param([string]$Path)

    $parseTokens = $null
    $parseErrors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$parseTokens, [ref]$parseErrors) | Out-Null
    if ($parseErrors.Count -gt 0) {
        throw "PowerShell script has parse errors: $Path"
    }
}



function Assert-SummaryAuditFields {
    param([object]$Summary)

    if ($Summary.checker_name -ne "check_release_metadata_docs.ps1") {
        throw "Expected JSON checker name check_release_metadata_docs.ps1, got: $($Summary.checker_name)"
    }
    $checkedAtUtc = $Summary.checked_at_utc
    if ($checkedAtUtc -is [DateTime]) {
        $checkedAtUtc = $checkedAtUtc.ToUniversalTime().ToString(
            "yyyy-MM-ddTHH:mm:ss'Z'",
            [System.Globalization.CultureInfo]::InvariantCulture
        )
    }
    if ($checkedAtUtc -notmatch '^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$') {
        throw "Expected JSON checked_at_utc to use UTC timestamp format, got: $($Summary.checked_at_utc)"
    }
    if ([string]::IsNullOrWhiteSpace($Summary.powershell_edition)) {
        throw "Expected JSON powershell_edition to be populated."
    }
    if ($Summary.powershell_version -notmatch '^\d+\.\d+') {
        throw "Expected JSON powershell_version to start with a version number, got: $($Summary.powershell_version)"
    }
    if ($Summary.output_encoding -ne "UTF-8 without BOM") {
        throw "Expected JSON output_encoding UTF-8 without BOM, got: $($Summary.output_encoding)"
    }
}

function Assert-SummaryMarkerCountsConsistent {
    param([object]$Summary)

    $markerGroups = @(
        [pscustomobject]@{
            Label = "pipeline"
            CountName = "required_pipeline_marker_count"
            ValuesName = "required_pipeline_markers"
        },
        [pscustomobject]@{
            Label = "checklist"
            CountName = "required_checklist_marker_count"
            ValuesName = "required_checklist_markers"
        },
        [pscustomobject]@{
            Label = "document governance"
            CountName = "required_document_governance_marker_count"
            ValuesName = "required_document_governance_markers"
        },
        [pscustomobject]@{
            Label = "policy"
            CountName = "required_policy_marker_count"
            ValuesName = "required_policy_markers"
        },
        [pscustomobject]@{
            Label = "entrypoint"
            CountName = "required_entrypoint_marker_count"
            ValuesName = "required_entrypoint_markers"
        }
    )

    $categoryTotal = 0
    foreach ($group in $markerGroups) {
        $expectedCount = [int]$Summary.PSObject.Properties[$group.CountName].Value
        $actualCount = @($Summary.PSObject.Properties[$group.ValuesName].Value).Count
        if ($actualCount -ne $expectedCount) {
            throw "Expected $($group.Label) marker array count $expectedCount, got $actualCount."
        }

        $categoryTotal += $expectedCount
    }

    if ([int]$Summary.required_marker_count -ne $categoryTotal) {
        throw "Expected total marker count $categoryTotal, got $($Summary.required_marker_count)."
    }
}

function Assert-SummaryCheckedDocumentsConsistent {
    param([object]$Summary)

    $checkedDocuments = @($Summary.checked_documents)
    if ([int]$Summary.checked_document_count -ne $checkedDocuments.Count) {
        throw "Expected checked document count $($Summary.checked_document_count), got $($checkedDocuments.Count)."
    }

    foreach ($document in $checkedDocuments) {
        foreach ($propertyName in @("label", "relative_path", "path")) {
            $property = $document.PSObject.Properties[$propertyName]
            if ($null -eq $property) {
                throw "Expected checked document $propertyName property to exist."
            }

            $propertyValue = [string]$property.Value
            if ([string]::IsNullOrWhiteSpace($propertyValue)) {
                throw "Expected checked document $propertyName to be populated."
            }
        }

        $relativePath = [string]$document.relative_path
        if ([System.IO.Path]::IsPathRooted($relativePath)) {
            throw "Expected checked document relative_path to stay repository-relative, got: $relativePath"
        }
    }

    $labels = @($Summary.checked_document_labels)
    if ($labels.Count -ne $checkedDocuments.Count) {
        throw "Expected checked_document_labels count $($checkedDocuments.Count), got $($labels.Count)."
    }
    $relativePaths = @($Summary.checked_document_relative_paths)
    if ($relativePaths.Count -ne $checkedDocuments.Count) {
        throw "Expected checked_document_relative_paths count $($checkedDocuments.Count), got $($relativePaths.Count)."
    }
    foreach ($document in $checkedDocuments) {
        Assert-ArrayContains `
            -Values $labels `
            -ExpectedValue ([string]$document.label) `
            -Message "Expected checked_document_labels to include $($document.label)."
        Assert-ArrayContains `
            -Values $relativePaths `
            -ExpectedValue ([string]$document.relative_path) `
            -Message "Expected checked_document_relative_paths to include $($document.relative_path)."
    }
}

function Assert-SummaryFailure {
    param(
        [string]$Path,
        [string]$ExpectedMessage,
        [string]$ExpectedFailureKind,
        [string]$ExpectedFailureRuleId = "",
        [string]$ExpectedFailureRelativePath,
        [string]$ExpectedFailureExpectedText = "",
        [int]$ExpectedFailureLineNumber = 0,
        [int]$ExpectedFailureColumnNumber = 0,
        [string]$ExpectedFailureExcerpt = ""
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "check_release_metadata_docs.ps1 did not write failure JSON summary: $Path"
    }

    Assert-FileHasNoBom -Path $Path
    $summary = Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    if ($summary.status -ne "failed") {
        throw "Expected JSON summary status to be failed, got: $($summary.status)"
    }
    if ($summary.error_message -notmatch [regex]::Escape($ExpectedMessage)) {
        throw "Expected JSON failure message '$ExpectedMessage', got: $($summary.error_message)"
    }
    if ($summary.failure_kind -ne $ExpectedFailureKind) {
        throw "Expected JSON failure kind '$ExpectedFailureKind', got: $($summary.failure_kind)"
    }
    if ([string]::IsNullOrWhiteSpace($ExpectedFailureRuleId)) {
        $ExpectedFailureRuleId = "release_metadata_docs.$ExpectedFailureKind"
    }
    if ($summary.failure_rule_id -ne $ExpectedFailureRuleId) {
        throw "Expected JSON failure rule id '$ExpectedFailureRuleId', got: $($summary.failure_rule_id)"
    }
    if ($summary.failure_relative_path -ne $ExpectedFailureRelativePath) {
        throw "Expected JSON failure relative path '$ExpectedFailureRelativePath', got: $($summary.failure_relative_path)"
    }
    if (-not [string]::IsNullOrWhiteSpace($ExpectedFailureExpectedText) -and
        $summary.failure_expected_text -ne $ExpectedFailureExpectedText) {
        throw "Expected JSON failure expected text '$ExpectedFailureExpectedText', got: $($summary.failure_expected_text)"
    }
    if ($ExpectedFailureLineNumber -gt 0 -and
        $summary.failure_line_number -ne $ExpectedFailureLineNumber) {
        throw "Expected JSON failure line number '$ExpectedFailureLineNumber', got: $($summary.failure_line_number)"
    }
    if ($ExpectedFailureColumnNumber -gt 0 -and
        $summary.failure_column_number -ne $ExpectedFailureColumnNumber) {
        throw "Expected JSON failure column number '$ExpectedFailureColumnNumber', got: $($summary.failure_column_number)"
    }
    if ($ExpectedFailureExcerpt.Length -gt 0 -and
        $summary.failure_excerpt -ne $ExpectedFailureExcerpt) {
        throw "Expected JSON failure excerpt '$ExpectedFailureExcerpt', got: $($summary.failure_excerpt)"
    }

    $expectedSummaryJsonPath = [System.IO.Path]::GetFullPath($Path)
    if ($summary.summary_json_path -ne $expectedSummaryJsonPath) {
        throw "Expected JSON summary path '$expectedSummaryJsonPath', got: $($summary.summary_json_path)"
    }
    if ($summary.summary_json_relative_path -ne "docs-check-summary.json") {
        throw "Expected JSON relative summary path docs-check-summary.json, got: $($summary.summary_json_relative_path)"
    }
    if ($summary.summary_schema_version -ne 1) {
        throw "Expected JSON summary schema version 1, got: $($summary.summary_schema_version)"
    }
    Assert-SummaryAuditFields -Summary $summary
    Assert-SummaryMarkerCountsConsistent -Summary $summary
    Assert-SummaryCheckedDocumentsConsistent -Summary $summary
    if ($summary.required_marker_count -ne 225) {
        throw "Expected JSON summary to count 225 required markers, got: $($summary.required_marker_count)"
    }
}

function New-DocsCase {
    param(
        [string]$Name,
        [string]$PipelineText = $defaultPipelineText,
        [string]$ChecklistText = $defaultChecklistText,
        [string]$DocumentGovernanceText = $defaultDocumentGovernanceText,
        [string]$PolicyText = $defaultPolicyText,
        [string]$IndexText = $defaultIndexText,
        [string]$ReadmeText = $defaultReadmeText,
        [string]$ReadmeZhText = $defaultReadmeZhText
    )

    $caseRoot = Join-Path $resolvedWorkingDir ("{0}-{1}" -f $Name, [System.Guid]::NewGuid().ToString("N"))
    $docsDir = Join-Path $caseRoot "docs"

    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_metadata_pipeline_zh.rst") -Text $PipelineText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_metadata_maintenance_checklist_zh.rst") -Text $ChecklistText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "document_governance_acceptance_zh.rst") -Text $DocumentGovernanceText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_policy_zh.rst") -Text $PolicyText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "index.rst") -Text $IndexText
    Write-Utf8NoBomFile -Path (Join-Path $caseRoot "README.md") -Text $ReadmeText
    Write-Utf8NoBomFile -Path (Join-Path $caseRoot "README.zh-CN.md") -Text $ReadmeZhText

    return $caseRoot
}

function Invoke-DocsCheck {
    param(
        [string]$CaseRoot,
        [switch]$ShouldFail,
        [string]$ExpectedMessage = "",
        [string]$SummaryJson = "",
        [switch]$Quiet
    )

    $failed = $false
    $output = @()

    $parameters = @{ RepoRoot = $CaseRoot }
    if (-not [string]::IsNullOrWhiteSpace($SummaryJson)) {
        $parameters.SummaryJson = $SummaryJson
    }
    if ($Quiet) {
        $parameters.Quiet = $true
    }

    try {
        $output = @(& $docsCheckScript @parameters *>&1)
    } catch {
        $failed = $true
        $output += $_.Exception.Message
    }

    $joinedOutput = ($output | ForEach-Object { $_.ToString() }) -join "`n"

    if ($ShouldFail) {
        if (-not $failed) {
            throw "check_release_metadata_docs.ps1 unexpectedly passed for $CaseRoot."
        }
        if (-not [string]::IsNullOrWhiteSpace($ExpectedMessage) -and
            $joinedOutput -notmatch [regex]::Escape($ExpectedMessage)) {
            throw "Expected failure message '$ExpectedMessage', got: $joinedOutput"
        }
        return
    }

    if ($failed) {
        throw "check_release_metadata_docs.ps1 failed unexpectedly for ${CaseRoot}: $joinedOutput"
    }

    if ($Quiet) {
        if ($joinedOutput -match [regex]::Escape("Release metadata docs check passed.")) {
            throw "check_release_metadata_docs.ps1 printed the success marker in quiet mode."
        }
    } elseif ($joinedOutput -notmatch [regex]::Escape("Release metadata docs check passed.")) {
        throw "check_release_metadata_docs.ps1 did not print the success marker. Output: $joinedOutput"
    }
}

$resolvedRepoRoot = Resolve-DefaultRepoRoot
$resolvedWorkingDir = Resolve-DefaultWorkingDir
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$docsCheckScript = Join-Path $resolvedRepoRoot "scripts\check_release_metadata_docs.ps1"
Assert-ScriptParses -Path $docsCheckScript

$defaultPipelineText = @(
    'Release metadata pipeline',
    '=========================',
    '',
    '- run_word_visual_release_gate.ps1',
    '- run_release_candidate_checks.ps1',
    '- sync_visual_review_verdict.ps1',
    '- sync_latest_visual_review_verdict.ps1',
    '- -GateSummaryJson',
    '- readme_gallery',
    '- assets_dir',
    '- refresh_readme_visual_assets.ps1',
    '- write_release_note_bundle.ps1',
    '- check_word_visual_release_gate_preflight.ps1',
    '- ``featherdoc.word_visual_release_gate_preflight.v1``',
    '- ``word_visual_release_gate_preflight_static_contract_only``',
    '- ``output_encoding``',
    '- UTF-8 without BOM',
    '- ``preflight_ready``',
    '- ``release_ready``',
    '- docx_functional_smoke_readiness',
    '- ``featherdoc.docx_functional_smoke_readiness.v1``',
    '- docx_functional_smoke_readiness_trace',
    '- persisted_docx_functional_smoke_evidence_only',
    '- summary_json_display',
    '- report_markdown_display',
    '- word_visual_smoke.pending_manual_review',
    '- release_blocker_count',
    '- review_task_summary',
    '- release_note_bundle',
    '- entrypoint_count',
    '- required_entrypoint_count',
    '- entrypoints[]',
    '- ``path_display``',
    '- ``location``',
    '- release_assets_manifest.json',
    '- package_release_assets.ps1',
    '- ``required``',
    '- assert_release_material_safety.ps1',
    '- -SkipMaterialSafetyAudit',
    '- reviewer_manifest_scoped_project_template_trace',
    '- manifest_signoff_entrypoints_release_trace',
    '- manifest_signoff_entrypoints_manifest_trace',
    '- release_entry_project_template_readiness_checklist_trace',
    '- release governance warning contract',
    '- warning_count',
    '- release_blocker_rollup.md',
    '- release_governance_handoff.md',
    '- release_governance_pipeline.md',
    '- ``featherdoc.release_governance_local_closure.v1``',
    '- local_governance_closure',
    '- local_governance_closure.status',
    '- local_governance_closure.closed',
    '- governance_detail_source',
    '- pipeline_summary_json',
    '- pipeline_summary_json_display',
    '- pipeline_report_markdown',
    '- pipeline_report_markdown_display',
    '- release_governance_handoff.release_blockers[]',
    '- release_governance_handoff.warnings[]',
    '- release_governance_handoff.action_items[]',
    '- ``source_json_display``',
    '- ``featherdoc.release_governance_pipeline_report.v1``',
    '- ``stages[]``',
    '- ``stage_id``',
    '- ``stage_title``',
    '- ``open_command``',
    '- final_governance_report_count',
    '- final_governance_reports',
    '- required_stage_count',
    '- completed_required_stage_count',
    '- required_stages',
    '- rerun_pdf_controlled_visual_smoke_check',
    '- check_pdf_controlled_visual_smoke.ps1',
    '- controlled_visual_smoke_json_display',
    '- restore_docx_functional_smoke_evidence',
    '- Word visual standard review metadata evidence',
    '- word_visual_standard_review_metadata_source_reports',
    '- task_reviews=',
    '- release-candidate-checks',
    '- release-candidate-checks-source',
    '- report/ARTIFACT_GUIDE.md',
    '- report/REVIEWER_CHECKLIST.md',
    '- report/release_handoff.md',
    '- report/release_body.zh-CN.md',
    '- report/release_summary.zh-CN.md',
    '- ``id``',
    '- ``action``',
    '- ``message``',
    '- ``source_schema``',
    '- ``source_report_display``',
    '- ``output_gap_count``',
    '- ``missing_output_count``',
    '- ``output gap checks``',
    '- ``missing outputs``',
    '- ``style_merge_suggestion_count``',
    '- ``featherdoc.project_template_delivery_readiness_report.v1``',
    '- ``featherdoc.content_control_data_binding_governance_report.v1``',
    '- ``featherdoc.numbering_catalog_governance_report.v1``',
    '- ``featherdoc.table_layout_delivery_governance_report.v1``',
    '- ``sync_bound_content_control``',
    '- ``numbering_catalog_governance.real_corpus_alignment_gap``',
    '- ``delivery_quality``',
    '- ReleaseBlockerRollupFailOnWarning',
    '- ReleaseGovernanceHandoffFailOnWarning',
    ''
) -join "`n"

$defaultChecklistText = @(
    'Release metadata maintenance checklist',
    '======================================',
    '',
    '- :doc:`release_metadata_pipeline_zh`',
    '- word_visual_release_gate_smoke_verdict',
    '- check_word_visual_release_gate_preflight.ps1',
    '- check_word_visual_release_gate_preflight_test.ps1',
    '- word_visual_release_gate_preflight_route_docs_contract',
    '- word_visual_release_gate_preflight_route_docs_contract_test.ps1',
    '- ``featherdoc.word_visual_release_gate_preflight.v1``',
    '- ``word_visual_release_gate_preflight_static_contract_only``',
    '- ``checked_documents[]``',
    '- checked_document_count',
    '- checked_document_labels',
    '- checked_document_relative_paths',
    '- required_marker_count',
    '- ``output_encoding``',
    '- UTF-8 without BOM',
    '- ``preflight_ready``',
    '- ``release_ready``',
    '- release_candidate_visual_verdict',
    '- sync_latest_visual_review_verdict_table_style_quality',
    '- sync_latest_visual_review_verdict_cmake_contract',
    '- sync_visual_review_verdict_(section_page_setup|page_number_fields|curated_visual_bundle)',
    '- open_latest_word_review_task_curated_source_kind_test.ps1',
    '- open_latest_word_review_task.ps1 -SourceKind table-style-quality-visual-regression-bundle',
    '- latest_table-style-quality-visual-regression-bundle_task.json',
    '- release_note_bundle_visual_verdict_metadata',
    '- Word visual standard review metadata evidence',
    '- word_visual_standard_review_metadata_source_reports',
    '- task_reviews=',
    '- source_schema=featherdoc.release_candidate_summary',
    '- release-candidate-checks',
    '- release-candidate-checks-source',
    '- public_release_wording_regression_test.ps1',
    '- git diff --check',
    '- release_note_bundle',
    '- release_assets_manifest.json',
    '- manifest_signoff_entrypoints',
    '- entrypoint path contract',
    '- package_release_assets.ps1',
    '- package_release_assets_safety_test.ps1',
    '- package_release_assets_allow_incomplete_test.ps1',
    '- assert_release_material_safety.ps1',
    '- release governance warning contract',
    '- warning_count',
    '- release_blocker_rollup',
    '- release_governance_handoff',
    '- release_governance_pipeline',
    '- local_governance_closure',
    '- local_governance_closure.status',
    '- local_governance_closure.closed',
    '- governance_detail_source',
    '- pipeline_summary_json_display',
    '- pipeline_report_markdown_display',
    '- release_governance_handoff.release_blockers[]',
    '- release_governance_handoff.warnings[]',
    '- release_governance_handoff.action_items[]',
    '- stages[]',
    '- stage_id',
    '- stage_title',
    '- final_governance_report_count',
    '- final_governance_reports',
    '- required_stage_count',
    '- completed_required_stage_count',
    '- required_stages',
    '- source_schema',
    '- source_report_display',
    '- source_json_display',
    '- style_merge_suggestion_count',
    '- check_docx_functional_smoke_readiness.ps1',
    '- docx_functional_smoke_readiness_test.ps1',
    '- docx_functional_smoke_readiness_route_docs_contract',
    '- docx_functional_smoke_readiness_route_docs_contract_test.ps1',
    '- docx_functional_smoke_readiness',
    '- ``featherdoc.docx_functional_smoke_readiness.v1``',
    '- docx_functional_smoke_readiness_trace',
    '- persisted_docx_functional_smoke_evidence_only',
    '- summary_json_display',
    '- report_markdown_display',
    '- word_visual_smoke.pending_manual_review',
    '- release_blocker_count',
    '- ReleaseBlockerRollupFailOnWarning',
    '- ReleaseGovernanceHandoffFailOnWarning',
    '- :doc:`document_governance_acceptance_zh`',
    '- build_project_template_delivery_readiness_report_test.ps1',
    '- build_content_control_data_binding_governance_report_test.ps1',
    '- build_numbering_catalog_governance_report_test.ps1',
    '- build_table_layout_delivery_governance_report_test.ps1',
    '- pdf_floating_table_support_coverage',
    '- pdf_floating_table_reviewer_focus',
    '- metadata-only tblpPr',
    '- metadata_only_fields',
    '- review_required_fields',
    ''
) -join "`n"

$defaultDocumentGovernanceText = @(
    'Document governance acceptance',
    '==============================',
    '',
    '- document_governance_acceptance.v1',
    '- document_governance_primary_track',
    '- featherdoc.project_template_delivery_readiness_report.v1',
    '- featherdoc.content_control_data_binding_governance_report.v1',
    '- content_control_data_binding.bound_placeholder',
    '- sync_bound_content_control',
    '- featherdoc.numbering_catalog_governance_report.v1',
    '- numbering_catalog_governance.real_corpus_alignment_gap',
    '- featherdoc.table_layout_delivery_governance_report.v1',
    '- delivery_quality',
    '- pdf_floating_table_support_coverage',
    '- pdf_floating_table_reviewer_focus',
    '- metadata-only tblpPr',
    ''
) -join "`n"

$defaultPolicyText = @(
    'Release policy',
    '==============',
    '',
    'See :doc:`release_metadata_pipeline_zh`.',
    '- ReleaseBlockerRollupFailOnWarning',
    '- ReleaseGovernanceHandoffFailOnWarning',
    '- Word visual standard review metadata evidence',
    '- word_visual_standard_review_metadata_source_reports',
    '- task_reviews=',
    '- source_schema=featherdoc.release_candidate_summary',
    '- release-candidate-checks',
    '- release-candidate-checks-source',
    '- report/ARTIFACT_GUIDE.md',
    '- report/REVIEWER_CHECKLIST.md',
    '- report/release_handoff.md',
    '- release_note_bundle',
    '- release_assets_manifest.json',
    '- package_release_assets.ps1',
    '- assert_release_material_safety.ps1',
    '- ``required``',
    '- ``location``',
    '- ``path_display``',
    ''
) -join "`n"

$defaultIndexText = @(
    'FeatherDoc',
    '==========',
    '',
    '.. toctree::',
    '',
    '   release_metadata_pipeline_zh',
    '   release_metadata_maintenance_checklist_zh',
    ''
) -join "`n"

$defaultReadmeText = @(
    '# FeatherDoc',
    '',
    '- Release metadata pipeline guide: `docs/release_metadata_pipeline_zh.rst`',
    '- Release metadata maintenance checklist: `docs/release_metadata_maintenance_checklist_zh.rst`',
    ''
) -join "`n"

$defaultReadmeZhText = @(
    '# FeatherDoc',
    '',
    '- Release metadata 流水线：`docs/release_metadata_pipeline_zh.rst`',
    '- Release metadata 维护清单：`docs/release_metadata_maintenance_checklist_zh.rst`',
    ''
) -join "`n"

$passingCaseRoot = New-DocsCase -Name "passing"
$summaryJsonPath = Join-Path $passingCaseRoot "docs-check-summary.json"
Invoke-DocsCheck -CaseRoot $passingCaseRoot -SummaryJson $summaryJsonPath

if (-not (Test-Path -LiteralPath $summaryJsonPath)) {
    throw "check_release_metadata_docs.ps1 did not write the requested JSON summary."
}
Assert-FileHasNoBom -Path $summaryJsonPath
$summary = Get-Content -Raw -LiteralPath $summaryJsonPath | ConvertFrom-Json
if ($summary.status -ne "passed") {
    throw "Expected JSON summary status to be passed, got: $($summary.status)"
}
$expectedSummaryJsonPath = [System.IO.Path]::GetFullPath($summaryJsonPath)
if ($summary.summary_json_path -ne $expectedSummaryJsonPath) {
    throw "Expected JSON summary path '$expectedSummaryJsonPath', got: $($summary.summary_json_path)"
}
if ($summary.summary_json_relative_path -ne "docs-check-summary.json") {
    throw "Expected JSON relative summary path docs-check-summary.json, got: $($summary.summary_json_relative_path)"
}
if ($summary.summary_schema_version -ne 1) {
    throw "Expected JSON summary schema version 1, got: $($summary.summary_schema_version)"
}
Assert-SummaryAuditFields -Summary $summary
Assert-SummaryMarkerCountsConsistent -Summary $summary
Assert-SummaryCheckedDocumentsConsistent -Summary $summary
if ($summary.checked_document_count -ne 7) {
    throw "Expected JSON summary checked document count 7, got: $($summary.checked_document_count)"
}
if ($summary.required_pipeline_marker_count -ne 100) {
    throw "Expected JSON summary pipeline marker count 100, got: $($summary.required_pipeline_marker_count)"
}
if ($summary.required_checklist_marker_count -ne 91) {
    throw "Expected JSON summary checklist marker count 91, got: $($summary.required_checklist_marker_count)"
}
if ($summary.required_document_governance_marker_count -ne 13) {
    throw "Expected JSON summary document governance marker count 13, got: $($summary.required_document_governance_marker_count)"
}
if ($summary.required_policy_marker_count -ne 19) {
    throw "Expected JSON summary policy marker count 19, got: $($summary.required_policy_marker_count)"
}
if ($summary.required_entrypoint_marker_count -ne 2) {
    throw "Expected JSON summary entrypoint marker count 2, got: $($summary.required_entrypoint_marker_count)"
}
if ($summary.required_marker_count -ne 225) {
    throw "Expected JSON summary total marker count 225, got: $($summary.required_marker_count)"
}
if ($summary.checked_documents.Count -ne 7) {
    throw "Expected JSON summary to list 7 checked documents, got: $($summary.checked_documents.Count)"
}
Assert-ArrayContains `
    -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
    -ExpectedValue 'docs/release_metadata_pipeline_zh.rst' `
    -Message "JSON summary should list the release metadata pipeline doc."
Assert-ArrayContains `
    -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
    -ExpectedValue 'docs/document_governance_acceptance_zh.rst' `
    -Message "JSON summary should list the document governance acceptance doc."
Assert-ArrayContains `
    -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
    -ExpectedValue 'docs/index.rst' `
    -Message "JSON summary should list the Sphinx index doc."
Assert-ArrayContains `
    -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
    -ExpectedValue 'README.md' `
    -Message "JSON summary should list the English README."
Assert-ArrayContains `
    -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
    -ExpectedValue 'README.zh-CN.md' `
    -Message "JSON summary should list the Chinese README."
Assert-ArrayContains `
    -Values @($summary.required_entrypoint_markers) `
    -ExpectedValue "release_metadata_pipeline_zh" `
    -Message "JSON summary should list release metadata pipeline entrypoint marker."
Assert-ArrayContains `
    -Values @($summary.required_entrypoint_markers) `
    -ExpectedValue "release_metadata_maintenance_checklist_zh" `
    -Message "JSON summary should list release metadata maintenance checklist entrypoint marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "release_note_bundle_visual_verdict_metadata" `
    -Message "JSON summary should list required checklist markers."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "Word visual standard review metadata evidence" `
    -Message "JSON summary checklist should list Word visual metadata compact evidence marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "word_visual_standard_review_metadata_source_reports" `
    -Message "JSON summary checklist should list Word visual metadata source reports marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "task_reviews=" `
    -Message "JSON summary checklist should list Word visual metadata task review compact marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "source_schema=featherdoc.release_candidate_summary" `
    -Message "JSON summary checklist should list Word visual metadata source schema marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "release-candidate-checks" `
    -Message "JSON summary checklist should list release-candidate summary source marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "release-candidate-checks-source" `
    -Message "JSON summary checklist should list source-only release candidate summary marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``output_gap_count``' `
    -Message "JSON summary should list PDF preflight output gap count marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``missing outputs``' `
    -Message "JSON summary should list PDF preflight missing outputs marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``featherdoc.word_visual_release_gate_preflight.v1``' `
    -Message "JSON summary should list Word visual release gate preflight schema marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``output_encoding``' `
    -Message "JSON summary should list Word visual preflight output encoding field marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue 'UTF-8 without BOM' `
    -Message "JSON summary should list Word visual preflight output encoding value marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "docx_functional_smoke_readiness" `
    -Message "JSON summary should list DOCX functional smoke readiness marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``featherdoc.docx_functional_smoke_readiness.v1``' `
    -Message "JSON summary should list DOCX functional smoke readiness schema marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``featherdoc.release_governance_local_closure.v1``' `
    -Message "JSON summary should list release governance local closure schema marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "local_governance_closure" `
    -Message "JSON summary should list release governance local closure marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "local_governance_closure.status" `
    -Message "JSON summary should list local closure status marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "local_governance_closure.closed" `
    -Message "JSON summary should list local closure closed marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "governance_detail_source" `
    -Message "JSON summary should list local closure governance detail source marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "pipeline_summary_json" `
    -Message "JSON summary should list local closure pipeline summary path marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "pipeline_summary_json_display" `
    -Message "JSON summary should list local closure pipeline summary display marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "pipeline_report_markdown" `
    -Message "JSON summary should list local closure pipeline Markdown path marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "pipeline_report_markdown_display" `
    -Message "JSON summary should list local closure pipeline Markdown display marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "release_governance_handoff.action_items[]" `
    -Message "JSON summary should list release governance handoff action item detail marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``featherdoc.release_governance_pipeline_report.v1``' `
    -Message "JSON summary should list release governance pipeline report schema marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``stages[]``' `
    -Message "JSON summary should list release governance pipeline stages marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue '``source_json_display``' `
    -Message "JSON summary should list release governance source JSON display marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "final_governance_report_count" `
    -Message "JSON summary should list local closure final governance report count marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "final_governance_reports" `
    -Message "JSON summary should list local closure final governance reports marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "required_stage_count" `
    -Message "JSON summary should list local closure required stage count marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "completed_required_stage_count" `
    -Message "JSON summary should list local closure completed required stage count marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "required_stages" `
    -Message "JSON summary should list local closure required stages marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "restore_docx_functional_smoke_evidence" `
    -Message "JSON summary should list DOCX functional smoke readiness blocker action marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "Word visual standard review metadata evidence" `
    -Message "JSON summary should list Word visual metadata compact evidence marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "word_visual_standard_review_metadata_source_reports" `
    -Message "JSON summary should list Word visual metadata source reports marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "task_reviews=" `
    -Message "JSON summary should list Word visual metadata task review compact marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "release-candidate-checks" `
    -Message "JSON summary should list release-candidate summary source marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "release-candidate-checks-source" `
    -Message "JSON summary should list source-only release candidate summary marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "reviewer_manifest_scoped_project_template_trace" `
    -Message "JSON summary should list reviewer manifest scoped project-template trace marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "manifest_signoff_entrypoints_release_trace" `
    -Message "JSON summary should list manifest signoff release trace marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "manifest_signoff_entrypoints_manifest_trace" `
    -Message "JSON summary should list manifest signoff packaged manifest trace marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "release_entry_project_template_readiness_checklist_trace" `
    -Message "JSON summary should list project-template readiness checklist release entry trace marker."
Assert-ArrayContains `
    -Values @($summary.required_policy_markers) `
    -ExpectedValue "Word visual standard review metadata evidence" `
    -Message "JSON summary policy should list Word visual metadata compact evidence marker."
Assert-ArrayContains `
    -Values @($summary.required_policy_markers) `
    -ExpectedValue "word_visual_standard_review_metadata_source_reports" `
    -Message "JSON summary policy should list Word visual metadata source reports marker."
Assert-ArrayContains `
    -Values @($summary.required_policy_markers) `
    -ExpectedValue "task_reviews=" `
    -Message "JSON summary policy should list Word visual metadata task review compact marker."
Assert-ArrayContains `
    -Values @($summary.required_policy_markers) `
    -ExpectedValue "source_schema=featherdoc.release_candidate_summary" `
    -Message "JSON summary policy should list Word visual metadata source schema marker."
Assert-ArrayContains `
    -Values @($summary.required_policy_markers) `
    -ExpectedValue "release-candidate-checks" `
    -Message "JSON summary policy should list release-candidate summary source marker."
Assert-ArrayContains `
    -Values @($summary.required_policy_markers) `
    -ExpectedValue "release-candidate-checks-source" `
    -Message "JSON summary policy should list source-only release candidate summary marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "readme_gallery" `
    -Message "JSON summary should list README gallery metadata marker."
Assert-ArrayContains `
    -Values @($summary.required_pipeline_markers) `
    -ExpectedValue "refresh_readme_visual_assets.ps1" `
    -Message "JSON summary should list README gallery refresh marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "check_word_visual_release_gate_preflight_test.ps1" `
    -Message "JSON summary should list Word visual preflight test marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue '``output_encoding``' `
    -Message "JSON summary checklist should list Word visual preflight output encoding field marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue 'UTF-8 without BOM' `
    -Message "JSON summary checklist should list Word visual preflight output encoding value marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "word_visual_release_gate_preflight_route_docs_contract_test.ps1" `
    -Message "JSON summary should list Word visual preflight route docs contract test marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "check_docx_functional_smoke_readiness.ps1" `
    -Message "JSON summary should list DOCX functional smoke readiness checklist marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "docx_functional_smoke_readiness_test.ps1" `
    -Message "JSON summary should list DOCX functional smoke readiness regression marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "docx_functional_smoke_readiness_route_docs_contract_test.ps1" `
    -Message "JSON summary should list DOCX functional smoke readiness route docs contract marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "local_governance_closure" `
    -Message "JSON summary checklist should list local governance closure marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "local_governance_closure.status" `
    -Message "JSON summary checklist should list local closure status marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "local_governance_closure.closed" `
    -Message "JSON summary checklist should list local closure closed marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "final_governance_reports" `
    -Message "JSON summary checklist should list local closure final governance reports marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "required_stages" `
    -Message "JSON summary checklist should list local closure required stages marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "open_latest_word_review_task_curated_source_kind_test.ps1" `
    -Message "JSON summary should list curated open-latest test marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "pdf_floating_table_reviewer_focus" `
    -Message "JSON summary should list PDF floating table reviewer focus checklist marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "metadata-only tblpPr" `
    -Message "JSON summary should list metadata-only tblpPr checklist marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "metadata_only_fields" `
    -Message "JSON summary should list PDF floating table metadata-only field marker."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "review_required_fields" `
    -Message "JSON summary should list PDF floating table review-required field marker."
Assert-ArrayContains `
    -Values @($summary.required_document_governance_markers) `
    -ExpectedValue "sync_bound_content_control" `
    -Message "JSON summary should list required document governance markers."
Assert-ArrayContains `
    -Values @($summary.required_document_governance_markers) `
    -ExpectedValue "pdf_floating_table_support_coverage" `
    -Message "JSON summary should list PDF floating table support coverage document governance marker."


$quietCaseRoot = New-DocsCase -Name "quiet-passing"
$quietSummaryJsonPath = Join-Path $quietCaseRoot "docs-check-summary.json"
Invoke-DocsCheck -CaseRoot $quietCaseRoot -SummaryJson $quietSummaryJsonPath -Quiet
if (-not (Test-Path -LiteralPath $quietSummaryJsonPath)) {
    throw "check_release_metadata_docs.ps1 did not write JSON summary in quiet mode."
}


$missingPolicyCaseRoot = Join-Path $resolvedWorkingDir ("missing-policy-{0}" -f [System.Guid]::NewGuid().ToString("N"))
$missingPolicyDocsDir = Join-Path $missingPolicyCaseRoot "docs"
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "release_metadata_pipeline_zh.rst") `
    -Text $defaultPipelineText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "release_metadata_maintenance_checklist_zh.rst") `
    -Text $defaultChecklistText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "document_governance_acceptance_zh.rst") `
    -Text $defaultDocumentGovernanceText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyDocsDir "index.rst") `
    -Text $defaultIndexText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyCaseRoot "README.md") `
    -Text $defaultReadmeText
Write-Utf8NoBomFile `
    -Path (Join-Path $missingPolicyCaseRoot "README.zh-CN.md") `
    -Text $defaultReadmeZhText
$missingPolicySummaryJsonPath = Join-Path $missingPolicyCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPolicyCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Missing release policy doc" `
    -SummaryJson $missingPolicySummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPolicySummaryJsonPath `
    -ExpectedMessage "Missing release policy doc" `
    -ExpectedFailureKind "missing_file" `
    -ExpectedFailureRelativePath 'docs/release_policy_zh.rst'

$missingIndexEntrypointText = $defaultIndexText.Replace(
    "release_metadata_maintenance_checklist_zh",
    "release_metadata_maintenance_checklist_removed"
)
$missingIndexEntrypointCaseRoot = New-DocsCase `
    -Name "missing-index-entrypoint" `
    -IndexText $missingIndexEntrypointText
$missingIndexEntrypointSummaryJsonPath = Join-Path $missingIndexEntrypointCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingIndexEntrypointCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Sphinx index doc is missing expected text: release_metadata_maintenance_checklist_zh" `
    -SummaryJson $missingIndexEntrypointSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingIndexEntrypointSummaryJsonPath `
    -ExpectedMessage "Sphinx index doc is missing expected text: release_metadata_maintenance_checklist_zh" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/index.rst' `
    -ExpectedFailureExpectedText "release_metadata_maintenance_checklist_zh"

$missingReadmePipelineEntrypointText = $defaultReadmeText.Replace(
    "release_metadata_pipeline_zh",
    "release_metadata_pipeline_removed"
)
$missingReadmePipelineEntrypointCaseRoot = New-DocsCase `
    -Name "missing-readme-pipeline-entrypoint" `
    -ReadmeText $missingReadmePipelineEntrypointText
$missingReadmePipelineEntrypointSummaryJsonPath = Join-Path $missingReadmePipelineEntrypointCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingReadmePipelineEntrypointCaseRoot `
    -ShouldFail `
    -ExpectedMessage "English README is missing expected text: release_metadata_pipeline_zh" `
    -SummaryJson $missingReadmePipelineEntrypointSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingReadmePipelineEntrypointSummaryJsonPath `
    -ExpectedMessage "English README is missing expected text: release_metadata_pipeline_zh" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'README.md' `
    -ExpectedFailureExpectedText "release_metadata_pipeline_zh"

$missingReadmeZhChecklistEntrypointText = $defaultReadmeZhText.Replace(
    "release_metadata_maintenance_checklist_zh",
    "release_metadata_maintenance_checklist_removed"
)
$missingReadmeZhChecklistEntrypointCaseRoot = New-DocsCase `
    -Name "missing-readme-zh-checklist-entrypoint" `
    -ReadmeZhText $missingReadmeZhChecklistEntrypointText
$missingReadmeZhChecklistEntrypointSummaryJsonPath = Join-Path $missingReadmeZhChecklistEntrypointCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingReadmeZhChecklistEntrypointCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Chinese README is missing expected text: release_metadata_maintenance_checklist_zh" `
    -SummaryJson $missingReadmeZhChecklistEntrypointSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingReadmeZhChecklistEntrypointSummaryJsonPath `
    -ExpectedMessage "Chinese README is missing expected text: release_metadata_maintenance_checklist_zh" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'README.zh-CN.md' `
    -ExpectedFailureExpectedText "release_metadata_maintenance_checklist_zh"

$missingPolicyWordVisualMetadataText = $defaultPolicyText.Replace(
    "Word visual standard review metadata evidence",
    "Word visual metadata evidence removed"
)
$missingPolicyWordVisualMetadataCaseRoot = New-DocsCase `
    -Name "missing-policy-word-visual-metadata" `
    -PolicyText $missingPolicyWordVisualMetadataText
$missingPolicyWordVisualMetadataSummaryJsonPath = Join-Path $missingPolicyWordVisualMetadataCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPolicyWordVisualMetadataCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release policy doc is missing expected text: Word visual standard review metadata evidence" `
    -SummaryJson $missingPolicyWordVisualMetadataSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPolicyWordVisualMetadataSummaryJsonPath `
    -ExpectedMessage "release policy doc is missing expected text: Word visual standard review metadata evidence" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_policy_zh.rst' `
    -ExpectedFailureExpectedText "Word visual standard review metadata evidence"

$missingPipelineWordVisualMetadataText = $defaultPipelineText.Replace(
    "Word visual standard review metadata evidence",
    "Word visual metadata evidence removed"
)
$missingPipelineWordVisualMetadataCaseRoot = New-DocsCase `
    -Name "missing-pipeline-word-visual-metadata" `
    -PipelineText $missingPipelineWordVisualMetadataText
$missingPipelineWordVisualMetadataSummaryJsonPath = Join-Path $missingPipelineWordVisualMetadataCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPipelineWordVisualMetadataCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: Word visual standard review metadata evidence" `
    -SummaryJson $missingPipelineWordVisualMetadataSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPipelineWordVisualMetadataSummaryJsonPath `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: Word visual standard review metadata evidence" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureExpectedText "Word visual standard review metadata evidence"

$missingPipelineReleaseManifestText = $defaultPipelineText.Replace(
    "release_assets_manifest.json",
    "release_assets_manifest_removed.json"
)
$missingPipelineReleaseManifestCaseRoot = New-DocsCase `
    -Name "missing-pipeline-release-manifest" `
    -PipelineText $missingPipelineReleaseManifestText
$missingPipelineReleaseManifestSummaryJsonPath = Join-Path $missingPipelineReleaseManifestCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPipelineReleaseManifestCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: release_assets_manifest.json" `
    -SummaryJson $missingPipelineReleaseManifestSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPipelineReleaseManifestSummaryJsonPath `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: release_assets_manifest.json" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureExpectedText "release_assets_manifest.json"

$missingPipelineHandoffActionItemsText = $defaultPipelineText.Replace(
    "release_governance_handoff.action_items[]",
    "release_governance_handoff.action_items_removed[]"
)
$missingPipelineHandoffActionItemsCaseRoot = New-DocsCase `
    -Name "missing-pipeline-handoff-action-items" `
    -PipelineText $missingPipelineHandoffActionItemsText
$missingPipelineHandoffActionItemsSummaryJsonPath = Join-Path $missingPipelineHandoffActionItemsCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPipelineHandoffActionItemsCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: release_governance_handoff.action_items[]" `
    -SummaryJson $missingPipelineHandoffActionItemsSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPipelineHandoffActionItemsSummaryJsonPath `
    -ExpectedMessage "release metadata pipeline doc is missing expected text: release_governance_handoff.action_items[]" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureExpectedText "release_governance_handoff.action_items[]"

$missingChecklistPipelineStagesText = $defaultChecklistText.Replace(
    "stages[]",
    "stages_removed[]"
)
$missingChecklistPipelineStagesCaseRoot = New-DocsCase `
    -Name "missing-checklist-pipeline-stages" `
    -ChecklistText $missingChecklistPipelineStagesText
$missingChecklistPipelineStagesSummaryJsonPath = Join-Path $missingChecklistPipelineStagesCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistPipelineStagesCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: stages[]" `
    -SummaryJson $missingChecklistPipelineStagesSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistPipelineStagesSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: stages[]" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "stages[]"

$missingPolicyMaterialSafetyText = $defaultPolicyText.Replace(
    "assert_release_material_safety.ps1",
    "assert_release_material_removed.ps1"
)
$missingPolicyMaterialSafetyCaseRoot = New-DocsCase `
    -Name "missing-policy-material-safety" `
    -PolicyText $missingPolicyMaterialSafetyText
$missingPolicyMaterialSafetySummaryJsonPath = Join-Path $missingPolicyMaterialSafetyCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingPolicyMaterialSafetyCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release policy doc is missing expected text: assert_release_material_safety.ps1" `
    -SummaryJson $missingPolicyMaterialSafetySummaryJsonPath
Assert-SummaryFailure `
    -Path $missingPolicyMaterialSafetySummaryJsonPath `
    -ExpectedMessage "release policy doc is missing expected text: assert_release_material_safety.ps1" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_policy_zh.rst' `
    -ExpectedFailureExpectedText "assert_release_material_safety.ps1"

$missingChecklistWordVisualMetadataText = $defaultChecklistText.Replace(
    "Word visual standard review metadata evidence",
    "Word visual metadata evidence removed"
)
$missingChecklistWordVisualMetadataCaseRoot = New-DocsCase `
    -Name "missing-checklist-word-visual-metadata" `
    -ChecklistText $missingChecklistWordVisualMetadataText
$missingChecklistWordVisualMetadataSummaryJsonPath = Join-Path $missingChecklistWordVisualMetadataCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistWordVisualMetadataCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: Word visual standard review metadata evidence" `
    -SummaryJson $missingChecklistWordVisualMetadataSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistWordVisualMetadataSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: Word visual standard review metadata evidence" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "Word visual standard review metadata evidence"

$trailingWhitespacePipelineText = $defaultPipelineText.Replace(
    "- review_task_summary",
    "- review_task_summary "
)
$trailingWhitespaceExpectedExcerpt = "- review_task_summary "
$trailingWhitespaceExpectedLines = @($trailingWhitespacePipelineText -split "`r?`n")
$trailingWhitespaceExpectedLineNumber = [array]::IndexOf(
    $trailingWhitespaceExpectedLines,
    $trailingWhitespaceExpectedExcerpt
) + 1
if ($trailingWhitespaceExpectedLineNumber -le 0) {
    throw "Expected trailing whitespace fixture line was not found."
}
$trailingWhitespaceExpectedColumnNumber = $trailingWhitespaceExpectedExcerpt.Length
$trailingWhitespaceCaseRoot = New-DocsCase `
    -Name "trailing-whitespace-pipeline" `
    -PipelineText $trailingWhitespacePipelineText
$trailingWhitespaceSummaryJsonPath = Join-Path $trailingWhitespaceCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $trailingWhitespaceCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Trailing whitespace" `
    -SummaryJson $trailingWhitespaceSummaryJsonPath
Assert-SummaryFailure `
    -Path $trailingWhitespaceSummaryJsonPath `
    -ExpectedMessage "Trailing whitespace" `
    -ExpectedFailureKind "trailing_whitespace" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst' `
    -ExpectedFailureLineNumber $trailingWhitespaceExpectedLineNumber `
    -ExpectedFailureColumnNumber $trailingWhitespaceExpectedColumnNumber `
    -ExpectedFailureExcerpt $trailingWhitespaceExpectedExcerpt

$tabChecklistText = $defaultChecklistText.Replace(
    "- git diff --check",
    "- git`t diff --check"
)
$tabExpectedExcerpt = "- git`t diff --check"
$tabExpectedLines = @($tabChecklistText -split "`r?`n")
$tabExpectedLineNumber = [array]::IndexOf($tabExpectedLines, $tabExpectedExcerpt) + 1
if ($tabExpectedLineNumber -le 0) {
    throw "Expected tab fixture line was not found."
}
$tabExpectedColumnNumber = $tabExpectedExcerpt.IndexOf("`t") + 1
$tabCaseRoot = New-DocsCase -Name "tab-checklist" -ChecklistText $tabChecklistText
$tabSummaryJsonPath = Join-Path $tabCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $tabCaseRoot `
    -ShouldFail `
    -ExpectedMessage "Tab character found" `
    -SummaryJson $tabSummaryJsonPath
Assert-SummaryFailure `
    -Path $tabSummaryJsonPath `
    -ExpectedMessage "Tab character found" `
    -ExpectedFailureKind "tab_character" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureLineNumber $tabExpectedLineNumber `
    -ExpectedFailureColumnNumber $tabExpectedColumnNumber `
    -ExpectedFailureExcerpt $tabExpectedExcerpt

$missingChecklistEntry = $defaultChecklistText.Replace(
    "release_note_bundle_visual_verdict_metadata",
    "release_note_bundle_visual_verdict_removed"
)
$missingChecklistCaseRoot = New-DocsCase -Name "missing-checklist-entry" -ChecklistText $missingChecklistEntry
$missingChecklistSummaryJsonPath = Join-Path $missingChecklistCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingChecklistCaseRoot `
    -ShouldFail `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: release_note_bundle_visual_verdict_metadata" `
    -SummaryJson $missingChecklistSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingChecklistSummaryJsonPath `
    -ExpectedMessage "release metadata maintenance checklist doc is missing expected text: release_note_bundle_visual_verdict_metadata" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "release_note_bundle_visual_verdict_metadata"

$missingDocumentGovernanceEntry = $defaultDocumentGovernanceText.Replace(
    "- sync_bound_content_control",
    "- sync_removed_content_control"
)
$missingDocumentGovernanceCaseRoot = New-DocsCase `
    -Name "missing-document-governance-entry" `
    -DocumentGovernanceText $missingDocumentGovernanceEntry
$missingDocumentGovernanceSummaryJsonPath = Join-Path $missingDocumentGovernanceCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $missingDocumentGovernanceCaseRoot `
    -ShouldFail `
    -ExpectedMessage "document governance acceptance doc is missing expected text: sync_bound_content_control" `
    -SummaryJson $missingDocumentGovernanceSummaryJsonPath
Assert-SummaryFailure `
    -Path $missingDocumentGovernanceSummaryJsonPath `
    -ExpectedMessage "document governance acceptance doc is missing expected text: sync_bound_content_control" `
    -ExpectedFailureKind "missing_text" `
    -ExpectedFailureRelativePath 'docs/document_governance_acceptance_zh.rst' `
    -ExpectedFailureExpectedText "sync_bound_content_control"

$bomCaseRoot = New-DocsCase -Name "bom-pipeline"
Write-Utf8BomFile `
    -Path (Join-Path $bomCaseRoot "docs/release_metadata_pipeline_zh.rst") `
    -Text $defaultPipelineText
$bomSummaryJsonPath = Join-Path $bomCaseRoot "docs-check-summary.json"
Invoke-DocsCheck `
    -CaseRoot $bomCaseRoot `
    -ShouldFail `
    -ExpectedMessage "File must be UTF-8 without BOM" `
    -SummaryJson $bomSummaryJsonPath
Assert-SummaryFailure `
    -Path $bomSummaryJsonPath `
    -ExpectedMessage "File must be UTF-8 without BOM" `
    -ExpectedFailureKind "utf8_bom" `
    -ExpectedFailureRelativePath 'docs/release_metadata_pipeline_zh.rst'

Write-Host "Release metadata docs checker regression passed."
