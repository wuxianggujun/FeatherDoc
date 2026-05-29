param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-NotContainsText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Message
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Message Unexpected='$UnexpectedText'."
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$readme = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.md"
$pipelineDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$maintenanceDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_maintenance_checklist_zh.rst"
$workflowDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\automation\word_visual_workflow_zh.rst"
$scriptText = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_word_visual_release_gate_preflight.ps1"
$regressionText = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\check_word_visual_release_gate_preflight_test.ps1"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

foreach ($assertion in @(
        [ordered]@{
            label = "README"
            text = $readme
            markers = @(
                "scripts\check_word_visual_release_gate_preflight.ps1",
                "featherdoc.word_visual_release_gate_preflight.v1",
                "word_visual_release_gate_preflight_static_contract_only",
                "does not run Word, CMake, CTest"
            )
        },
        [ordered]@{
            label = "release metadata pipeline"
            text = $pipelineDoc
            markers = @(
                "check_word_visual_release_gate_preflight.ps1",
                "featherdoc.word_visual_release_gate_preflight.v1",
                "word_visual_release_gate_preflight_static_contract_only",
                "minimum_risk_next_action_command",
                "preflight_ready",
                "release_ready"
            )
        },
        [ordered]@{
            label = "release metadata maintenance checklist"
            text = $maintenanceDoc
            markers = @(
                "check_word_visual_release_gate_preflight.ps1",
                "check_word_visual_release_gate_preflight_test.ps1",
                "featherdoc.word_visual_release_gate_preflight.v1",
                "word_visual_release_gate_preflight_static_contract_only",
                "preflight_ready",
                "release_ready"
            )
        },
        [ordered]@{
            label = "Word visual workflow"
            text = $workflowDoc
            markers = @(
                "check_word_visual_release_gate_preflight.ps1",
                "featherdoc.word_visual_release_gate_preflight.v1",
                "word_visual_release_gate_preflight_static_contract_only",
                "release-ready evidence"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve Word visual preflight marker '$marker'."
    }
}

foreach ($marker in @(
        "featherdoc.word_visual_release_gate_preflight.v1",
        "word_visual_release_gate_preflight_static_contract_only",
        "preflight_ready",
        "minimum_risk_next_action_command",
        "strict_preflight_command_template",
        "full_gate_command_template",
        'release_ready = $false',
        "This read-only preflight",
        "does not run Word, CMake, CTest",
        "word_visual_gate_scripts_exist",
        "word_visual_gate_contract_markers",
        "word_visual_gate_core_flows_wired",
        "word_visual_gate_floating_image_flows_wired",
        "word_visual_gate_cmake_contract_registered",
        "word_visual_gate_docs_linked"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText $marker `
        -Message "Word visual preflight script should preserve marker '$marker'."
}

foreach ($unexpected in @(
        "New-Object -ComObject Word.Application",
        "ctest --test-dir",
        "cmake --build",
        "Start-Process"
    )) {
    Assert-NotContainsText -Text $scriptText -UnexpectedText $unexpected `
        -Message "Word visual preflight script should remain lightweight."
}

foreach ($marker in @(
        "featherdoc.word_visual_release_gate_preflight.v1",
        "word_visual_release_gate_preflight_static_contract_only",
        "minimum_risk_next_action_command",
        "preflight_ready should be true",
        "Static preflight should never claim release readiness",
        "does not run Word, CMake, CTest",
        "word_visual_gate_floating_image_flows_wired"
    )) {
    Assert-ContainsText -Text $regressionText -ExpectedText $marker `
        -Message "Word visual preflight regression should preserve marker '$marker'."
}

foreach ($marker in @(
        "check_word_visual_release_gate_preflight",
        "check_word_visual_release_gate_preflight_test.ps1",
        "word_visual_release_gate_preflight_route_docs_contract",
        "word_visual_release_gate_preflight_route_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "word;visual;release-gate;smoke"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep Word visual preflight contracts wired."
}

Write-Host "Word visual release gate preflight route docs contract passed."
