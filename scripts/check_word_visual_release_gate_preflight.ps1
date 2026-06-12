param(
    [string]$RepoRoot = "",
    [string]$OutputJson = "output/word-visual-release-gate-preflight-current/summary.json",
    [string]$ReportMarkdown = "output/word-visual-release-gate-preflight-current/word_visual_release_gate_preflight.md",
    [switch]$Strict
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    if (-not [string]::IsNullOrWhiteSpace($RepoRoot)) {
        return (Resolve-Path $RepoRoot).Path
    }

    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$Root,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }
    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        return [System.IO.Path]::GetFullPath($InputPath)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $Root $InputPath))
}

function Get-DisplayPath {
    param(
        [string]$Root,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    $rootPath = [System.IO.Path]::GetFullPath($Root)
    if (-not $rootPath.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $rootPath.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $rootPath += [System.IO.Path]::DirectorySeparatorChar
    }

    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($rootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relativePath = $resolvedPath.Substring($rootPath.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relativePath)) {
            return "."
        }
        return ".\" + ($relativePath -replace '/', '\')
    }

    return $resolvedPath
}

function Ensure-ParentDirectory {
    param([string]$Path)

    $parent = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
}

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    Ensure-ParentDirectory -Path $Path
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        return ""
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

function Get-RepoRelativePath {
    param(
        [string]$BaseRoot,
        [string]$Path
    )

    $root = [System.IO.Path]::GetFullPath($BaseRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }

    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $resolvedPath.Substring($root.Length).TrimStart('\', '/')
    }

    return $resolvedPath
}

function Get-CMakeTestRegistrationText {
    param([string]$Root)

    return @(
        Get-RepoFileText -Root $Root -RelativePath "test\CMakeLists.txt"
        Get-ChildItem -LiteralPath (Join-Path $Root "test\cmake") -Filter "*.cmake" -ErrorAction SilentlyContinue |
            Sort-Object FullName |
            ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
    ) -join "`n"
}

function Add-Check {
    param(
        [System.Collections.ArrayList]$Checks,
        [string]$Name,
        [string]$Status,
        [bool]$Required,
        [string]$Message,
        [object]$Details = $null
    )

    $check = [ordered]@{
        name = $Name
        status = $Status
        required = $Required
        message = $Message
    }
    if ($null -ne $Details) {
        $check.details = $Details
    }
    $Checks.Add($check) | Out-Null
}

function Get-MissingTextMarkers {
    param(
        [string]$Text,
        [string[]]$Markers
    )

    return @(
        $Markers | Where-Object {
            $Text -notmatch [regex]::Escape($_)
        }
    )
}

$repoRoot = Resolve-RepoRoot
$resolvedOutputJson = Resolve-RepoPath -Root $repoRoot -InputPath $OutputJson
$resolvedReportMarkdown = Resolve-RepoPath -Root $repoRoot -InputPath $ReportMarkdown
$checks = New-Object System.Collections.ArrayList

$gateScriptRoot = Join-Path $repoRoot "scripts"
$gateScriptRelativePaths = @(
    "scripts\run_word_visual_release_gate.ps1"
    Get-ChildItem -LiteralPath $gateScriptRoot -Filter "run_word_visual_release_gate_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-RepoRelativePath -BaseRoot $repoRoot -Path $_.FullName }
)
$gateScriptPaths = @(
    foreach ($relativePath in @($gateScriptRelativePaths)) {
        Join-Path $repoRoot $relativePath
    }
)
$gateScriptText = @(
    foreach ($relativePath in @($gateScriptRelativePaths)) {
        Get-RepoFileText -Root $repoRoot -RelativePath $relativePath
    }
) -join [Environment]::NewLine

$requiredScripts = @(
    "scripts\run_word_visual_release_gate.ps1",
    "scripts\run_word_visual_release_gate_helpers.ps1",
    "scripts\run_word_visual_release_gate_setup.ps1",
    "scripts\run_word_visual_release_gate_descriptors.ps1",
    "scripts\run_word_visual_release_gate_standard_flows.ps1",
    "scripts\run_word_visual_release_gate_curated_report.ps1",
    "scripts\run_word_visual_smoke.ps1",
    "scripts\run_fixed_grid_merge_unmerge_regression.ps1",
    "scripts\run_section_page_setup_visual_regression.ps1",
    "scripts\run_page_number_fields_visual_regression.ps1",
    "scripts\run_bookmark_floating_image_visual_regression.ps1",
    "scripts\run_floating_image_z_order_visual_regression.ps1",
    "scripts\prepare_word_review_task.ps1",
    "scripts\refresh_readme_visual_assets.ps1",
    "scripts\build_image_contact_sheet.py"
)
$missingScripts = @(
    $requiredScripts | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $repoRoot $_) -PathType Leaf)
    }
)
Add-Check -Checks $checks `
    -Name "word_visual_gate_scripts_exist" `
    -Status $(if ($missingScripts.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($missingScripts.Count -eq 0) { "All required Word visual release gate scripts exist." } else { "Some required Word visual release gate scripts are missing." }) `
    -Details ([ordered]@{
        required_scripts = $requiredScripts
        missing_scripts = @($missingScripts)
    })

$parseErrors = @()
foreach ($gateScriptFilePath in @($gateScriptPaths)) {
    if (Test-Path -LiteralPath $gateScriptFilePath -PathType Leaf) {
        $parseTokens = $null
        $parseErrorRef = $null
        [System.Management.Automation.Language.Parser]::ParseFile($gateScriptFilePath, [ref]$parseTokens, [ref]$parseErrorRef) | Out-Null
        $parseErrors += @($parseErrorRef | ForEach-Object { $_.Message })
    } else {
        $parseErrors += @("Gate script is missing: $gateScriptFilePath")
    }
}
Add-Check -Checks $checks `
    -Name "word_visual_gate_parseable" `
    -Status $(if ($parseErrors.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($parseErrors.Count -eq 0) { "Word visual release gate script parses successfully." } else { "Word visual release gate script has parse errors or is missing." }) `
    -Details ([ordered]@{
        script = "scripts\run_word_visual_release_gate.ps1"
        parse_error_count = $parseErrors.Count
        parse_errors = @($parseErrors)
    })

$requiredGateMarkers = @(
    '[string]$GateOutputDir = "output/word-visual-release-gate"',
    '[switch]$SkipBuild',
    '[switch]$SkipSmoke',
    '[switch]$SkipReviewTasks',
    '[switch]$RefreshReadmeAssets',
    '[string]$SmokeReviewVerdict = "undecided"',
    '[string]$FixedGridReviewVerdict = "undecided"',
    '[string]$SectionPageSetupReviewVerdict = "undecided"',
    '[string]$PageNumberFieldsReviewVerdict = "undecided"',
    '[string]$CuratedVisualReviewVerdict = "undecided"',
    '[string]$BookmarkFloatingImageBuildDir = "build-msvc-nmake-bookmark-floating-image-visual"',
    '[string]$FloatingImageZOrderBuildDir = "build-floating-image-z-order-visual-nmake"',
    '[switch]$SkipBookmarkFloatingImage',
    '[switch]$SkipFloatingImageZOrder',
    '$gateSummary.review_task_summary = Get-ReviewTaskSummary -ReviewTasks $gateSummary.review_tasks',
    'visual_verdict = if ($SkipReviewTasks)',
    'Gate summary: $gateSummaryPath',
    'Gate final review: $gateFinalReviewPath'
)
$missingGateMarkers = @(Get-MissingTextMarkers -Text $gateScriptText -Markers $requiredGateMarkers)
Add-Check -Checks $checks `
    -Name "word_visual_gate_contract_markers" `
    -Status $(if ($missingGateMarkers.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($missingGateMarkers.Count -eq 0) { "Word visual release gate keeps the controlled contract markers." } else { "Word visual release gate lost controlled contract markers." }) `
    -Details ([ordered]@{
        required_markers = $requiredGateMarkers
        missing_markers = @($missingGateMarkers)
    })

$coreFlowMarkers = @(
    'scripts\run_word_visual_smoke.ps1',
    'scripts\run_fixed_grid_merge_unmerge_regression.ps1',
    'scripts\run_section_page_setup_visual_regression.ps1',
    'scripts\run_page_number_fields_visual_regression.ps1',
    'scripts\run_bookmark_floating_image_visual_regression.ps1',
    'scripts\run_floating_image_z_order_visual_regression.ps1',
    'scripts\prepare_word_review_task.ps1',
    'scripts\refresh_readme_visual_assets.ps1',
    'curatedVisualFlowDescriptors'
)
$missingCoreFlowMarkers = @(Get-MissingTextMarkers -Text $gateScriptText -Markers $coreFlowMarkers)
Add-Check -Checks $checks `
    -Name "word_visual_gate_core_flows_wired" `
    -Status $(if ($missingCoreFlowMarkers.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($missingCoreFlowMarkers.Count -eq 0) { "Word visual release gate still wires the smoke, fixed-grid, section, page-number, curated, task, and gallery flows." } else { "Word visual release gate lost a core flow marker." }) `
    -Details ([ordered]@{
        required_markers = $coreFlowMarkers
        missing_markers = @($missingCoreFlowMarkers)
    })

$floatingImageFlowMarkers = @(
    'id = "bookmark-floating-image"',
    'label = "Bookmark floating image"',
    '$SkipBookmarkFloatingImage.IsPresent',
    '$BookmarkFloatingImageBuildDir',
    '$bookmarkFloatingImageScript',
    'id = "floating-image-z-order"',
    'label = "Floating image z-order"',
    '$SkipFloatingImageZOrder.IsPresent',
    '$FloatingImageZOrderBuildDir',
    '$floatingImageZOrderScript'
)
$missingFloatingImageFlowMarkers = @(Get-MissingTextMarkers -Text $gateScriptText -Markers $floatingImageFlowMarkers)
Add-Check -Checks $checks `
    -Name "word_visual_gate_floating_image_flows_wired" `
    -Status $(if ($missingFloatingImageFlowMarkers.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($missingFloatingImageFlowMarkers.Count -eq 0) { "Word visual release gate still wires bookmark-floating-image and floating-image-z-order visual flows." } else { "Word visual release gate lost a floating image visual flow marker." }) `
    -Details ([ordered]@{
        required_markers = $floatingImageFlowMarkers
        missing_markers = @($missingFloatingImageFlowMarkers)
    })

$cmakeListsText = Get-CMakeTestRegistrationText -Root $repoRoot
$cmakeMarkers = @(
    "word_visual_release_gate_smoke_verdict",
    "word_visual_release_gate_smoke_verdict_test.ps1",
    "check_word_visual_release_gate_preflight",
    "check_word_visual_release_gate_preflight_test.ps1",
    "TIMEOUT 60",
    'LABELS "word;visual;release-gate;smoke"'
)
$missingCMakeMarkers = @(Get-MissingTextMarkers -Text $cmakeListsText -Markers $cmakeMarkers)
Add-Check -Checks $checks `
    -Name "word_visual_gate_cmake_contract_registered" `
    -Status $(if ($missingCMakeMarkers.Count -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($missingCMakeMarkers.Count -eq 0) { "CMake keeps the Word visual gate preflight and smoke-verdict contracts registered." } else { "CMake lost Word visual gate contract registration markers." }) `
    -Details ([ordered]@{
        required_markers = $cmakeMarkers
        missing_markers = @($missingCMakeMarkers)
    })

$workflowDocText = Get-RepoFileText -Root $repoRoot -RelativePath "docs\automation\word_visual_workflow_zh.rst"
$maintenanceDocText = Get-RepoFileText -Root $repoRoot -RelativePath "docs\documentation_maintenance_zh.rst"
$governanceRoutesDocText = Get-RepoFileText -Root $repoRoot -RelativePath "docs\governance_routes_zh.rst"
$docMarkers = @(
    "check_word_visual_release_gate_preflight.ps1",
    "featherdoc.word_visual_release_gate_preflight.v1",
    "word_visual_release_gate_preflight_static_contract_only"
)
$missingWorkflowMarkers = @(Get-MissingTextMarkers -Text $workflowDocText -Markers $docMarkers)
$missingMaintenanceMarkers = @(Get-MissingTextMarkers -Text $maintenanceDocText -Markers $docMarkers)
$missingGovernanceRouteMarkers = @(Get-MissingTextMarkers -Text $governanceRoutesDocText -Markers $docMarkers)
$missingDocMarkerCount = $missingWorkflowMarkers.Count + $missingMaintenanceMarkers.Count + $missingGovernanceRouteMarkers.Count
Add-Check -Checks $checks `
    -Name "word_visual_gate_docs_linked" `
    -Status $(if ($missingDocMarkerCount -eq 0) { "pass" } else { "missing" }) `
    -Required $true `
    -Message $(if ($missingDocMarkerCount -eq 0) { "Word workflow docs, governance routes, and maintenance docs describe the controlled preflight boundary." } else { "Word visual release gate preflight documentation markers are incomplete." }) `
    -Details ([ordered]@{
        required_markers = $docMarkers
        missing_workflow_markers = @($missingWorkflowMarkers)
        missing_maintenance_markers = @($missingMaintenanceMarkers)
        missing_governance_route_markers = @($missingGovernanceRouteMarkers)
    })

$blockingChecks = @($checks | Where-Object { [bool]$_.required -and [string]$_.status -ne "pass" })
$status = if ($blockingChecks.Count -eq 0) { "ready" } else { "not_ready" }
$boundary = "Ready means the Word visual release gate static contract is coherent. This preflight does not run Word, CMake, CTest, browsers, LibreOffice, or visual rendering, and it is not release-ready evidence until the full screenshot-backed gate and review verdicts pass."
$strictPreflightCommandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\check_word_visual_release_gate_preflight.ps1 -RepoRoot . -Strict'
$fullGateCommandTemplate = 'powershell -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1'
$minimumRiskNextActionCommand = if ($blockingChecks.Count -eq 0) {
    $fullGateCommandTemplate
} else {
    $strictPreflightCommandTemplate
}
$summary = [ordered]@{
    schema = "featherdoc.word_visual_release_gate_preflight.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    strict = [bool]$Strict
    preflight_ready = ($status -eq "ready")
    release_ready = $false
    repo_root = $repoRoot
    output_json = $resolvedOutputJson
    output_json_display = Get-DisplayPath -Root $repoRoot -Path $resolvedOutputJson
    report_markdown = $resolvedReportMarkdown
    report_markdown_display = Get-DisplayPath -Root $repoRoot -Path $resolvedReportMarkdown
    output_encoding = "UTF-8 without BOM"
    evidence_scope = "word_visual_release_gate_preflight_static_contract_only"
    evidence_scope_note = "This read-only preflight checks static scripts, docs, and test registration only."
    check_count = $checks.Count
    blocking_check_count = $blockingChecks.Count
    checks = @($checks)
    blocking_checks = @($blockingChecks | ForEach-Object { $_.name })
    minimum_risk_next_action = if ($blockingChecks.Count -eq 0) {
        "Keep dev pushed, then run the full Word visual release gate only on a stable Windows workstation with Microsoft Word available."
    } else {
        "Repair the blocking static Word visual release gate contract checks before starting the full gate."
    }
    minimum_risk_next_action_command = $minimumRiskNextActionCommand
    strict_preflight_command_template = $strictPreflightCommandTemplate
    full_gate_command_template = $fullGateCommandTemplate
    boundary = $boundary
}

Write-Utf8NoBomFile -Path $resolvedOutputJson -Text (($summary | ConvertTo-Json -Depth 12) + [Environment]::NewLine)

if (-not [string]::IsNullOrWhiteSpace($resolvedReportMarkdown)) {
    $markdownLines = New-Object 'System.Collections.Generic.List[string]'
    $markdownLines.Add("# Word Visual Release Gate Preflight") | Out-Null
    $markdownLines.Add("") | Out-Null
    $markdownLines.Add("- Schema: ``$($summary.schema)``") | Out-Null
    $markdownLines.Add("- Status: ``$($summary.status)``") | Out-Null
    $markdownLines.Add("- Output encoding: ``$($summary.output_encoding)``") | Out-Null
    $markdownLines.Add("- Evidence scope: ``$($summary.evidence_scope)``") | Out-Null
    $markdownLines.Add("- Evidence scope note: $($summary.evidence_scope_note)") | Out-Null
    $markdownLines.Add("- Blocking checks: ``$($summary.blocking_check_count)``") | Out-Null
    $markdownLines.Add("- Boundary: $boundary") | Out-Null
    $markdownLines.Add("") | Out-Null
    $markdownLines.Add("## Next Action") | Out-Null
    $markdownLines.Add("") | Out-Null
    $markdownLines.Add("- Minimum risk action: $($summary.minimum_risk_next_action)") | Out-Null
    $markdownLines.Add("- Command template: ``$($summary.minimum_risk_next_action_command)``") | Out-Null
    $markdownLines.Add("- Strict preflight: ``$($summary.strict_preflight_command_template)``") | Out-Null
    $markdownLines.Add("- Full gate: ``$($summary.full_gate_command_template)``") | Out-Null
    $markdownLines.Add("") | Out-Null
    $markdownLines.Add("## Checks") | Out-Null
    $markdownLines.Add("") | Out-Null
    foreach ($check in @($checks)) {
        $markdownLines.Add("- ``$($check.name)``: ``$($check.status)`` - $($check.message)") | Out-Null
    }
    Write-Utf8NoBomFile -Path $resolvedReportMarkdown -Text (($markdownLines -join [Environment]::NewLine) + [Environment]::NewLine)
}

$summary | ConvertTo-Json -Depth 12
if ($Strict -and $status -ne "ready") {
    exit 1
}
