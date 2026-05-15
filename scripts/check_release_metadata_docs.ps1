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

    $relativePath = [System.Uri]::UnescapeDataString($relativeUri.ToString())
    if ($relativePath -match '^\.\./') {
        return ""
    }

    return $relativePath.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
}

function Write-SummaryJson {
    param(
        [string]$Path,
        [string]$Status,
        [string]$RepoRoot,
        [object[]]$CheckedDocuments,
        [string[]]$PipelineMarkers,
        [string[]]$ChecklistMarkers,
        [string[]]$PolicyMarkers,
        [string[]]$ReadmeMarkers,
        [string[]]$IndexMarkers,
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
        required_marker_count = $PipelineMarkers.Count + $ChecklistMarkers.Count + $PolicyMarkers.Count +
            $ReadmeMarkers.Count + $IndexMarkers.Count
        required_pipeline_marker_count = $PipelineMarkers.Count
        required_checklist_marker_count = $ChecklistMarkers.Count
        required_policy_marker_count = $PolicyMarkers.Count
        required_readme_marker_count = $ReadmeMarkers.Count
        required_index_marker_count = $IndexMarkers.Count
        checked_documents = $CheckedDocuments
        required_pipeline_markers = $PipelineMarkers
        required_checklist_markers = $ChecklistMarkers
        required_policy_markers = $PolicyMarkers
        required_readme_markers = $ReadmeMarkers
        required_index_markers = $IndexMarkers
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
    "write_release_note_bundle.ps1",
    "review_task_summary",
    "assert_release_material_safety.ps1",
    "-SkipMaterialSafetyAudit",
    "warning_count",
    "release_blocker_rollup.md",
    "release_governance_handoff.md",
    "release_governance_pipeline.md",
    '``REVIEWER_CHECKLIST.md``',
    "checkbox",
    '``id``',
    '``action``',
    '``message``',
    '``source_schema``',
    '``style_merge_suggestion_count``'
)
$checklistExpectedMarkers = @(
    ':doc:`release_metadata_pipeline_zh`',
    "word_visual_release_gate_smoke_verdict",
    "release_candidate_visual_verdict",
    "sync_visual_review_verdict_(section_page_setup|page_number_fields|curated_visual_bundle)",
    "release_note_bundle_visual_verdict_metadata",
    "public_release_wording_regression_test.ps1",
    "git diff --check",
    "release_governance_warning_contract_test.ps1",
    "release_governance_warning_helper_contract_test.ps1",
    "release governance warning",
    "warning_count",
    "release_blocker_rollup",
    "release_governance_handoff",
    "release_governance_pipeline",
    "source_schema",
    "style_merge_suggestion_count"
)
$policyExpectedMarkers = @(':doc:`release_metadata_pipeline_zh`')
$readmeExpectedMarkers = @(
    "governance warnings section",
    '`id`',
    '`action`',
    '`message`',
    '`source_schema`',
    '`style_merge_suggestion_count`',
    "checkbox guidance",
    "release_governance_warning_contract_test.ps1",
    "release_governance_warning_helper_contract_test.ps1"
)
$indexExpectedMarkers = @(
    '``id``',
    '``action``',
    '``message``',
    '``source_schema``',
    '``style_merge_suggestion_count``',
    '``document_skeleton.style_merge_suggestions_pending``',
    '``review_style_merge_suggestions``'
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
            relative_path = "docs\release_metadata_pipeline_zh.rst"
            path = Join-Path $resolvedRepoRoot "docs\release_metadata_pipeline_zh.rst"
        },
        [pscustomobject]@{
            label = "release metadata maintenance checklist doc"
            relative_path = "docs\release_metadata_maintenance_checklist_zh.rst"
            path = Join-Path $resolvedRepoRoot "docs\release_metadata_maintenance_checklist_zh.rst"
        },
        [pscustomobject]@{
            label = "release policy doc"
            relative_path = "docs\release_policy_zh.rst"
            path = Join-Path $resolvedRepoRoot "docs\release_policy_zh.rst"
        },
        [pscustomobject]@{
            label = "repository README"
            relative_path = "README.md"
            path = Join-Path $resolvedRepoRoot "README.md"
        },
        [pscustomobject]@{
            label = "documentation index doc"
            relative_path = "docs\index.rst"
            path = Join-Path $resolvedRepoRoot "docs\index.rst"
        }
    )
    $pipelinePath = $checkedDocuments[0].path
    $checklistPath = $checkedDocuments[1].path
    $policyPath = $checkedDocuments[2].path
    $readmePath = $checkedDocuments[3].path
    $indexPath = $checkedDocuments[4].path

    Assert-FileExists -Path $pipelinePath -Label "release metadata pipeline doc"
    Assert-FileExists -Path $checklistPath -Label "release metadata maintenance checklist doc"
    Assert-FileExists -Path $policyPath -Label "release policy doc"
    Assert-FileExists -Path $readmePath -Label "repository README"
    Assert-FileExists -Path $indexPath -Label "documentation index doc"

    $pipelineText = Read-Utf8Text -Path $pipelinePath
    $checklistText = Read-Utf8Text -Path $checklistPath
    $policyText = Read-Utf8Text -Path $policyPath
    $readmeText = Read-Utf8Text -Path $readmePath
    $indexText = Read-Utf8Text -Path $indexPath

    foreach ($doc in @(
            @{ Path = $pipelinePath; Text = $pipelineText },
            @{ Path = $checklistPath; Text = $checklistText },
            @{ Path = $policyPath; Text = $policyText },
            @{ Path = $readmePath; Text = $readmeText },
            @{ Path = $indexPath; Text = $indexText }
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

    foreach ($expected in $policyExpectedMarkers) {
        Assert-ContainsText `
            -Text $policyText `
            -ExpectedText $expected `
            -Label "release policy doc" `
            -Path $policyPath
    }

    foreach ($expected in $readmeExpectedMarkers) {
        Assert-ContainsText `
            -Text $readmeText `
            -ExpectedText $expected `
            -Label "repository README" `
            -Path $readmePath
    }

    foreach ($expected in $indexExpectedMarkers) {
        Assert-ContainsText `
            -Text $indexText `
            -ExpectedText $expected `
            -Label "documentation index doc" `
            -Path $indexPath
    }

    Write-SummaryJson `
        -Path $summaryJsonPath `
        -Status "passed" `
        -RepoRoot $resolvedRepoRoot `
        -CheckedDocuments $checkedDocuments `
        -PipelineMarkers $pipelineExpectedMarkers `
        -ChecklistMarkers $checklistExpectedMarkers `
        -PolicyMarkers $policyExpectedMarkers `
        -ReadmeMarkers $readmeExpectedMarkers `
        -IndexMarkers $indexExpectedMarkers

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
                -PolicyMarkers $policyExpectedMarkers `
                -ReadmeMarkers $readmeExpectedMarkers `
                -IndexMarkers $indexExpectedMarkers `
                -ErrorMessage $errorMessage
        } catch {
            Write-Warning "Unable to write release metadata docs failure summary: $($_.Exception.Message)"
        }
    }

    throw
}
