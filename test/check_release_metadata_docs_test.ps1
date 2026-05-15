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
    if ($summary.required_marker_count -ne 31) {
        throw "Expected JSON summary to count 31 required markers, got: $($summary.required_marker_count)"
    }
}

function New-DocsCase {
    param(
        [string]$Name,
        [string]$PipelineText = $defaultPipelineText,
        [string]$ChecklistText = $defaultChecklistText,
        [string]$PolicyText = $defaultPolicyText
    )

    $caseRoot = Join-Path $resolvedWorkingDir ("{0}-{1}" -f $Name, [System.Guid]::NewGuid().ToString("N"))
    $docsDir = Join-Path $caseRoot "docs"

    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_metadata_pipeline_zh.rst") -Text $PipelineText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_metadata_maintenance_checklist_zh.rst") -Text $ChecklistText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_policy_zh.rst") -Text $PolicyText

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
    '- write_release_note_bundle.ps1',
    '- review_task_summary',
    '- assert_release_material_safety.ps1',
    '- -SkipMaterialSafetyAudit',
    '- release governance warning contract',
    '- warning_count',
    '- release_blocker_rollup.md',
    '- release_governance_handoff.md',
    '- release_governance_pipeline.md',
    '- ``id``',
    '- ``action``',
    '- ``message``',
    '- ``source_schema``',
    '- ``style_merge_suggestion_count``',
    ''
) -join "`n"

$defaultChecklistText = @(
    'Release metadata maintenance checklist',
    '======================================',
    '',
    '- :doc:`release_metadata_pipeline_zh`',
    '- word_visual_release_gate_smoke_verdict',
    '- release_candidate_visual_verdict',
    '- sync_visual_review_verdict_(section_page_setup|page_number_fields|curated_visual_bundle)',
    '- release_note_bundle_visual_verdict_metadata',
    '- public_release_wording_regression_test.ps1',
    '- git diff --check',
    '- release governance warning contract',
    '- warning_count',
    '- release_blocker_rollup',
    '- release_governance_handoff',
    '- release_governance_pipeline',
    '- source_schema',
    '- style_merge_suggestion_count',
    ''
) -join "`n"

$defaultPolicyText = @(
    'Release policy',
    '==============',
    '',
    'See :doc:`release_metadata_pipeline_zh`.',
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
if ($summary.checked_document_count -ne 3) {
    throw "Expected JSON summary checked document count 3, got: $($summary.checked_document_count)"
}
if ($summary.required_pipeline_marker_count -ne 16) {
    throw "Expected JSON summary pipeline marker count 16, got: $($summary.required_pipeline_marker_count)"
}
if ($summary.required_checklist_marker_count -ne 14) {
    throw "Expected JSON summary checklist marker count 14, got: $($summary.required_checklist_marker_count)"
}
if ($summary.required_policy_marker_count -ne 1) {
    throw "Expected JSON summary policy marker count 1, got: $($summary.required_policy_marker_count)"
}
if ($summary.required_marker_count -ne 31) {
    throw "Expected JSON summary total marker count 31, got: $($summary.required_marker_count)"
}
if ($summary.checked_documents.Count -ne 3) {
    throw "Expected JSON summary to list 3 checked documents, got: $($summary.checked_documents.Count)"
}
Assert-ArrayContains `
    -Values @($summary.checked_documents | ForEach-Object { $_.relative_path }) `
    -ExpectedValue 'docs\release_metadata_pipeline_zh.rst' `
    -Message "JSON summary should list the release metadata pipeline doc."
Assert-ArrayContains `
    -Values @($summary.required_checklist_markers) `
    -ExpectedValue "release_note_bundle_visual_verdict_metadata" `
    -Message "JSON summary should list required checklist markers."


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
    -ExpectedFailureRelativePath 'docs\release_policy_zh.rst'

$trailingWhitespacePipelineText = $defaultPipelineText.Replace(
    "- review_task_summary",
    "- review_task_summary "
)
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
    -ExpectedFailureRelativePath 'docs\release_metadata_pipeline_zh.rst' `
    -ExpectedFailureLineNumber 8 `
    -ExpectedFailureColumnNumber 22 `
    -ExpectedFailureExcerpt "- review_task_summary "

$tabChecklistText = $defaultChecklistText.Replace(
    "- git diff --check",
    "- git`t diff --check"
)
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
    -ExpectedFailureRelativePath 'docs\release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureLineNumber 10 `
    -ExpectedFailureColumnNumber 6 `
    -ExpectedFailureExcerpt "- git`t diff --check"

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
    -ExpectedFailureRelativePath 'docs\release_metadata_maintenance_checklist_zh.rst' `
    -ExpectedFailureExpectedText "release_note_bundle_visual_verdict_metadata"

$bomCaseRoot = New-DocsCase -Name "bom-pipeline"
Write-Utf8BomFile `
    -Path (Join-Path $bomCaseRoot "docs\release_metadata_pipeline_zh.rst") `
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
    -ExpectedFailureRelativePath 'docs\release_metadata_pipeline_zh.rst'

Write-Host "Release metadata docs checker regression passed."
