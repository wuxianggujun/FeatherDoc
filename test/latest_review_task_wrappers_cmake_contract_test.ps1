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

    $pattern = '(?s)add_test\(\s*NAME\s+' + [regex]::Escape($Name) + '\s+COMMAND.*?set_tests_properties\(' + [regex]::Escape($Name) + '\s+PROPERTIES\s+TIMEOUT\s+60\s+LABELS\s+"word;visual;review-task;smoke"\)'
    $match = [regex]::Match($Text, $pattern)
    Assert-True -Condition $match.Success `
        -Message "CMake should keep '$Name' registered with TIMEOUT 60 and review-task labels."
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
        name = "prepare_document_review_task"
        script = "prepare_document_review_task_test.ps1"
        working_dir = "prepare_document_review_task"
    },
    [ordered]@{
        name = "prepare_word_review_task_verdict"
        script = "prepare_word_review_task_verdict_test.ps1"
        working_dir = "prepare_word_review_task_verdict"
    },
    [ordered]@{
        name = "prepare_visual_regression_bundle_review_task"
        script = "prepare_visual_regression_bundle_review_task_test.ps1"
        working_dir = "prepare_visual_regression_bundle_review_task"
    },
    [ordered]@{
        name = "refresh_readme_visual_assets"
        script = "refresh_readme_visual_assets_test.ps1"
        working_dir = "refresh_readme_visual_assets"
    },
    [ordered]@{
        name = "prepare_section_page_setup_review_task"
        script = "prepare_section_page_setup_review_task_test.ps1"
        working_dir = "prepare_section_page_setup_review_task"
    },
    [ordered]@{
        name = "prepare_page_number_fields_review_task"
        script = "prepare_page_number_fields_review_task_test.ps1"
        working_dir = "prepare_page_number_fields_review_task"
    },
    [ordered]@{
        name = "open_latest_section_page_setup_review_task"
        script = "open_latest_section_page_setup_review_task_test.ps1"
        working_dir = "open_latest_section_page_setup_review_task"
    },
    [ordered]@{
        name = "open_latest_page_number_fields_review_task"
        script = "open_latest_page_number_fields_review_task_test.ps1"
        working_dir = "open_latest_page_number_fields_review_task"
    },
    [ordered]@{
        name = "open_latest_fixed_grid_review_task"
        script = "open_latest_fixed_grid_review_task_test.ps1"
        working_dir = "open_latest_fixed_grid_review_task"
    },
    [ordered]@{
        name = "open_latest_word_review_task_curated_source_kind"
        script = "open_latest_word_review_task_curated_source_kind_test.ps1"
        working_dir = "open_latest_word_review_task_curated_source_kind"
    },
    [ordered]@{
        name = "print_latest_fixed_grid_review_prompt"
        script = "print_latest_fixed_grid_review_prompt_test.ps1"
        working_dir = "print_latest_fixed_grid_review_prompt"
    }
)

foreach ($contract in $contracts) {
    $block = Get-TestBlock -Text $cmakeText -Name ([string]$contract.name)
    Assert-ContainsText -Text $block -ExpectedText ([string]$contract.script) `
        -Message "CMake block should point '$($contract.name)' at the expected test script."
    Assert-ContainsText -Text $block -ExpectedText ([string]$contract.working_dir) `
        -Message "CMake block should keep '$($contract.name)' working directory stable."
}

Write-Host "Latest review task wrappers CMake contract passed."
