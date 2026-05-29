param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    Assert-True -Condition ($Text.Contains($ExpectedText)) `
        -Message "$Message Missing='$ExpectedText'."
}

function Get-TestBlock {
    param(
        [string]$Text,
        [string]$Name
    )

    $pattern = '(?s)add_test\(\s*NAME\s+' + [regex]::Escape($Name) + '\s+COMMAND.*?set_tests_properties\(' + [regex]::Escape($Name) + '\s+PROPERTIES\s+TIMEOUT\s+60\s+LABELS\s+"word;visual;release-verdict;smoke"\)'
    $match = [regex]::Match($Text, $pattern)
    Assert-True -Condition $match.Success `
        -Message "CMake should keep '$Name' registered with TIMEOUT 60 and release-verdict labels."
    return $match.Value
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$cmakePath = Join-Path $resolvedRepoRoot "test\CMakeLists.txt"
$cmakeText = Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakePath

$contracts = @(
    [ordered]@{
        name = "sync_latest_visual_review_verdict"
        script = "sync_latest_visual_review_verdict_test.ps1"
        working_dir = "sync_latest_visual_review_verdict"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_uses_latest_release_summary_candidate"
        script = "sync_latest_visual_review_verdict_uses_latest_release_summary_candidate_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_uses_latest_release_summary_candidate"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_tiebreaks_release_summary_candidate"
        script = "sync_latest_visual_review_verdict_tiebreaks_release_summary_candidate_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_tiebreaks_release_summary_candidate"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_curated_visual_bundle"
        script = "sync_latest_visual_review_verdict_curated_visual_bundle_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_curated_visual_bundle"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_table_style_quality"
        script = "sync_latest_visual_review_verdict_table_style_quality_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_table_style_quality"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_no_release_summary"
        script = "sync_latest_visual_review_verdict_no_release_summary_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_no_release_summary"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_malformed_release_summary_candidate_only"
        script = "sync_latest_visual_review_verdict_malformed_release_summary_candidate_only_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_malformed_release_summary_candidate_only"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_conflicting_gates"
        script = "sync_latest_visual_review_verdict_conflicting_gates_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_conflicting_gates"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_missing_source_path"
        script = "sync_latest_visual_review_verdict_missing_source_path_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_missing_source_path"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_unsupported_source_kind"
        script = "sync_latest_visual_review_verdict_unsupported_source_kind_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_unsupported_source_kind"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_malformed_pointer_json"
        script = "sync_latest_visual_review_verdict_malformed_pointer_json_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_malformed_pointer_json"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_malformed_release_summary"
        script = "sync_latest_visual_review_verdict_malformed_release_summary_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_malformed_release_summary"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_skips_malformed_release_summary_candidate"
        script = "sync_latest_visual_review_verdict_skips_malformed_release_summary_candidate_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_skips_malformed_release_summary_candidate"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_missing_release_summary"
        script = "sync_latest_visual_review_verdict_missing_release_summary_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_missing_release_summary"
    },
    [ordered]@{
        name = "sync_latest_visual_review_verdict_explicit_gate_mismatch"
        script = "sync_latest_visual_review_verdict_explicit_gate_mismatch_test.ps1"
        working_dir = "sync_latest_visual_review_verdict_explicit_gate_mismatch"
    }
)

foreach ($contract in $contracts) {
    $block = Get-TestBlock -Text $cmakeText -Name ([string]$contract.name)
    Assert-ContainsText -Text $block -ExpectedText ([string]$contract.script) `
        -Message "CMake block should point '$($contract.name)' at the expected test script."
    Assert-ContainsText -Text $block -ExpectedText ([string]$contract.working_dir) `
        -Message "CMake block should keep '$($contract.name)' working directory stable."
}

Write-Host "Sync latest visual review verdict CMake contract passed."
