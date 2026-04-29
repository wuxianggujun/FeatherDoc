param(
    [string]$RepoRoot = "",
    [string]$SummaryJson = ""
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
        throw "File must be UTF-8 without BOM: $Path"
    }

    return [System.Text.Encoding]::UTF8.GetString($bytes)
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

function Write-SummaryJson {
    param(
        [string]$Path,
        [string]$Status,
        [string]$RepoRoot,
        [object[]]$CheckedDocuments,
        [string[]]$PipelineMarkers,
        [string[]]$ChecklistMarkers,
        [string[]]$PolicyMarkers,
        [string]$ErrorMessage = ""
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $summary = [pscustomobject]@{
        status = $Status
        error_message = $ErrorMessage
        summary_json_path = $Path
        repo_root = $RepoRoot
        checked_documents = $CheckedDocuments
        required_pipeline_markers = $PipelineMarkers
        required_checklist_markers = $ChecklistMarkers
        required_policy_markers = $PolicyMarkers
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
        throw "Missing ${Label}: $Path"
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if (-not $Text.Contains($ExpectedText)) {
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
            throw "Trailing whitespace in $Path line $($index + 1)."
        }
    }
}

function Assert-NoTabs {
    param(
        [string]$Text,
        [string]$Path
    )

    if ($Text.Contains("`t")) {
        throw "Tab character found in $Path."
    }
}

$pipelineExpectedMarkers = @(
    "run_word_visual_release_gate.ps1",
    "run_release_candidate_checks.ps1",
    "sync_visual_review_verdict.ps1",
    "write_release_note_bundle.ps1",
    "review_task_summary",
    "assert_release_material_safety.ps1",
    "-SkipMaterialSafetyAudit"
)
$checklistExpectedMarkers = @(
    ':doc:`release_metadata_pipeline_zh`',
    "word_visual_release_gate_smoke_verdict",
    "release_candidate_visual_verdict",
    "sync_visual_review_verdict_(section_page_setup|page_number_fields|curated_visual_bundle)",
    "release_note_bundle_visual_verdict_fallback",
    "public_release_wording_regression_test.ps1",
    "git diff --check"
)
$policyExpectedMarkers = @(':doc:`release_metadata_pipeline_zh`')
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
        }
    )
    $pipelinePath = $checkedDocuments[0].path
    $checklistPath = $checkedDocuments[1].path
    $policyPath = $checkedDocuments[2].path

    Assert-FileExists -Path $pipelinePath -Label "release metadata pipeline doc"
    Assert-FileExists -Path $checklistPath -Label "release metadata maintenance checklist doc"
    Assert-FileExists -Path $policyPath -Label "release policy doc"

    $pipelineText = Read-Utf8Text -Path $pipelinePath
    $checklistText = Read-Utf8Text -Path $checklistPath
    $policyText = Read-Utf8Text -Path $policyPath

    foreach ($doc in @(
            @{ Path = $pipelinePath; Text = $pipelineText },
            @{ Path = $checklistPath; Text = $checklistText },
            @{ Path = $policyPath; Text = $policyText }
        )) {
        Assert-NoTrailingWhitespace -Text $doc.Text -Path $doc.Path
        Assert-NoTabs -Text $doc.Text -Path $doc.Path
    }

    foreach ($expected in $pipelineExpectedMarkers) {
        Assert-ContainsText -Text $pipelineText -ExpectedText $expected -Label "release metadata pipeline doc"
    }

    foreach ($expected in $checklistExpectedMarkers) {
        Assert-ContainsText -Text $checklistText -ExpectedText $expected -Label "release metadata maintenance checklist doc"
    }

    foreach ($expected in $policyExpectedMarkers) {
        Assert-ContainsText -Text $policyText -ExpectedText $expected -Label "release policy doc"
    }

    Write-SummaryJson `
        -Path $summaryJsonPath `
        -Status "passed" `
        -RepoRoot $resolvedRepoRoot `
        -CheckedDocuments $checkedDocuments `
        -PipelineMarkers $pipelineExpectedMarkers `
        -ChecklistMarkers $checklistExpectedMarkers `
        -PolicyMarkers $policyExpectedMarkers

    Write-Host "Release metadata docs check passed."
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
                -ErrorMessage $errorMessage
        } catch {
            Write-Warning "Unable to write release metadata docs failure summary: $($_.Exception.Message)"
        }
    }

    throw
}
