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
        return [System.IO.Path]::GetFullPath((Join-Path $resolvedRepoRoot "output\check-script-task-index-test"))
    }

    return [System.IO.Path]::GetFullPath($WorkingDir)
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

function Assert-FileContainsText {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Message
    )

    $text = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
    if ($text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
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

function Invoke-IndexCheck {
    param(
        [string]$Root,
        [string]$SummaryJson,
        [string]$ReportMarkdown,
        [switch]$Quiet
    )

    $parameters = @{
        RepoRoot = $Root
        SummaryJson = $SummaryJson
    }
    if (-not [string]::IsNullOrWhiteSpace($ReportMarkdown)) {
        $parameters.ReportMarkdown = $ReportMarkdown
    }
    if ($Quiet) {
        $parameters.Quiet = $true
    }

    return @(& $checkerScript @parameters *>&1)
}

$resolvedRepoRoot = Resolve-DefaultRepoRoot
$resolvedWorkingDir = Resolve-DefaultWorkingDir
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$checkerScript = Join-Path $resolvedRepoRoot "scripts\check_script_task_index.ps1"
Assert-ScriptParses -Path $checkerScript

$passingSummaryJson = Join-Path $resolvedWorkingDir "passing-summary.json"
$passingReportMarkdown = Join-Path $resolvedWorkingDir "passing-report.md"
$passingOutput = Invoke-IndexCheck `
    -Root $resolvedRepoRoot `
    -SummaryJson $passingSummaryJson `
    -ReportMarkdown $passingReportMarkdown
$joinedPassingOutput = ($passingOutput | ForEach-Object { $_.ToString() }) -join "`n"
if ($joinedPassingOutput -notmatch [regex]::Escape("Script task index check passed.")) {
    throw "check_script_task_index.ps1 did not print the success marker."
}
if (-not (Test-Path -LiteralPath $passingSummaryJson -PathType Leaf)) {
    throw "check_script_task_index.ps1 did not write a passing summary."
}
if (-not (Test-Path -LiteralPath $passingReportMarkdown -PathType Leaf)) {
    throw "check_script_task_index.ps1 did not write a passing Markdown report."
}
Assert-FileHasNoBom -Path $passingSummaryJson
Assert-FileHasNoBom -Path $passingReportMarkdown
$passingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $passingSummaryJson | ConvertFrom-Json
if ($passingSummary.status -ne "passed") {
    throw "Expected passing summary status, got: $($passingSummary.status)"
}
if ($passingSummary.schema -ne "featherdoc.script_task_index_check.v1") {
    throw "Unexpected script task index summary schema: $($passingSummary.schema)"
}
if ($passingSummary.checker_name -ne "check_script_task_index.ps1") {
    throw "Unexpected checker name: $($passingSummary.checker_name)"
}
$checkedAtUtc = [string]$passingSummary.checked_at_utc
if ($checkedAtUtc -notmatch '^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$') {
    throw "Expected checked_at_utc to use UTC timestamp format, got: $($passingSummary.checked_at_utc)"
}
if ($passingSummary.powershell_version -notmatch '^\d+\.\d+') {
    throw "Expected powershell_version to start with a version number, got: $($passingSummary.powershell_version)"
}
if ($passingSummary.output_encoding -ne "UTF-8 without BOM") {
    throw "Unexpected script task index output encoding: $($passingSummary.output_encoding)"
}
if ($passingSummary.report_markdown_path -ne $passingReportMarkdown) {
    throw "Unexpected Markdown report path: $($passingSummary.report_markdown_path)"
}
if ($passingSummary.report_markdown_relative_path -notmatch [regex]::Escape("passing-report.md")) {
    throw "Unexpected Markdown report relative path: $($passingSummary.report_markdown_relative_path)"
}
if ($passingSummary.script_reference_count -lt 20) {
    throw "Expected at least 20 indexed scripts, got: $($passingSummary.script_reference_count)"
}
if ($passingSummary.total_script_reference_count -ne $passingSummary.script_reference_count) {
    throw "Expected no duplicate script references in the maintained index."
}
if ($passingSummary.script_reference_group_count -lt 8) {
    throw "Expected at least 8 script reference groups, got: $($passingSummary.script_reference_group_count)"
}
if ($passingSummary.documentation_entrypoint_count -ne 2) {
    throw "Expected two documentation entrypoints, got: $($passingSummary.documentation_entrypoint_count)"
}
Assert-ArrayContains `
    -Values @($passingSummary.documentation_entrypoints | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "README.md" `
    -Message "Summary should list the English README documentation entrypoint."
Assert-ArrayContains `
    -Values @($passingSummary.documentation_entrypoints | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "README.zh-CN.md" `
    -Message "Summary should list the Chinese README documentation entrypoint."
$passingEntryPointMarkerCount = [int](($passingSummary.documentation_entrypoints |
            ForEach-Object { $_.marker_count } |
            Measure-Object -Sum).Sum)
if ($passingEntryPointMarkerCount -ne 4) {
    throw "Expected four total documentation entrypoint markers, got: $passingEntryPointMarkerCount"
}
$passingGroupUniqueCount = [int](($passingSummary.script_reference_groups |
            ForEach-Object { $_.script_reference_count } |
            Measure-Object -Sum).Sum)
$passingGroupTotalCount = [int](($passingSummary.script_reference_groups |
            ForEach-Object { $_.total_script_reference_count } |
            Measure-Object -Sum).Sum)
if ($passingGroupUniqueCount -ne $passingSummary.script_reference_count) {
    throw "Script reference group unique total does not match summary count."
}
if ($passingGroupTotalCount -ne $passingSummary.total_script_reference_count) {
    throw "Script reference group total does not match summary count."
}
if ($passingSummary.script_reference_extension_count -lt 1) {
    throw "Expected at least one script reference extension, got: $($passingSummary.script_reference_extension_count)"
}
$passingExtensionUniqueCount = [int](($passingSummary.script_reference_extensions |
            ForEach-Object { $_.script_reference_count } |
            Measure-Object -Sum).Sum)
$passingExtensionTotalCount = [int](($passingSummary.script_reference_extensions |
            ForEach-Object { $_.total_script_reference_count } |
            Measure-Object -Sum).Sum)
if ($passingExtensionUniqueCount -ne $passingSummary.script_reference_count) {
    throw "Script reference extension unique total does not match summary count."
}
if ($passingExtensionTotalCount -ne $passingSummary.total_script_reference_count) {
    throw "Script reference extension total does not match summary count."
}
Assert-ArrayContains `
    -Values @($passingSummary.script_reference_extensions | ForEach-Object { $_.extension }) `
    -ExpectedValue ".ps1" `
    -Message "Script reference extensions should include PowerShell scripts."
Assert-ArrayContains `
    -Values @($passingSummary.script_reference_extensions | ForEach-Object { $_.script_references } | ForEach-Object { $_ }) `
    -ExpectedValue "scripts\check_script_task_index.ps1" `
    -Message "Script reference extensions should include the script index checker."
Assert-ArrayContains `
    -Values @($passingSummary.script_reference_groups | ForEach-Object { $_.script_references } | ForEach-Object { $_ }) `
    -ExpectedValue "scripts\check_script_task_index.ps1" `
    -Message "Script reference groups should include the script index checker."
Assert-ArrayContains `
    -Values @($passingSummary.script_reference_groups | ForEach-Object { $_.script_references } | ForEach-Object { $_ }) `
    -ExpectedValue "scripts\run_release_candidate_checks.ps1" `
    -Message "Script reference groups should include release candidate checks."
if ($passingSummary.duplicate_script_reference_count -ne 0) {
    throw "Expected no duplicate script references, got: $($passingSummary.duplicate_script_reference_count)"
}
if ($passingSummary.missing_script_count -ne 0) {
    throw "Expected no missing scripts, got: $($passingSummary.missing_script_count)"
}
if ($passingSummary.missing_marker_count -ne 0) {
    throw "Expected no missing markers, got: $($passingSummary.missing_marker_count)"
}
Assert-ArrayContains `
    -Values @($passingSummary.checked_scripts | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "scripts\check_script_task_index.ps1" `
    -Message "Summary should list the script index checker."
Assert-ArrayContains `
    -Values @($passingSummary.checked_scripts | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "scripts\run_release_candidate_checks.ps1" `
    -Message "Summary should list the release candidate checks script."
Assert-ArrayContains `
    -Values @($passingSummary.checked_scripts | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "scripts\check_pdf_release_readiness.ps1" `
    -Message "Summary should list the PDF release readiness script."
foreach ($marker in @(
        '# Script Task Index Check',
        '- schema: `featherdoc.script_task_index_check.v1`',
        '- status: `passed`',
        '- checked_at_utc:',
        '- powershell_version:',
        '- output_encoding: `UTF-8 without BOM`',
        '- documentation_entrypoint_count: `2`',
        '- total_script_reference_count:',
        '- script_reference_group_count:',
        '- script_reference_extension_count:',
        '- duplicate_script_reference_count: `0`',
        '- missing_script_count: `0`',
        '- missing_marker_count: `0`',
        '## Checked Scripts',
        '## Documentation Entry Points',
        '`README.md`: 2 markers',
        '`README.zh-CN.md`: 2 markers',
        '## Script Reference Groups',
        '## Script Reference Extensions',
        '`.ps1`:',
        'unique /',
        '## Duplicate Script References',
        '[ok] `scripts\check_script_task_index.ps1`',
        '[ok] `scripts\run_release_candidate_checks.ps1`'
    )) {
    Assert-FileContainsText -Path $passingReportMarkdown -ExpectedText $marker `
        -Message "Passing Markdown report should include marker."
}

$quietSummaryJson = Join-Path $resolvedWorkingDir "quiet-summary.json"
$quietReportMarkdown = Join-Path $resolvedWorkingDir "quiet-report.md"
$quietOutput = Invoke-IndexCheck `
    -Root $resolvedRepoRoot `
    -SummaryJson $quietSummaryJson `
    -ReportMarkdown $quietReportMarkdown `
    -Quiet
$joinedQuietOutput = ($quietOutput | ForEach-Object { $_.ToString() }) -join "`n"
if ($joinedQuietOutput -match [regex]::Escape("Script task index check passed.")) {
    throw "check_script_task_index.ps1 printed the success marker in quiet mode."
}
if (-not (Test-Path -LiteralPath $quietReportMarkdown -PathType Leaf)) {
    throw "check_script_task_index.ps1 did not write a quiet Markdown report."
}

$failingRoot = Join-Path $resolvedWorkingDir "failing-repo"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "docs\script_task_index_zh.rst") `
    -Text (@(
        "Script task index",
        "=================",
        "",
        "- ``scripts/check_script_task_index.ps1``",
        "- ``scripts/existing_tool.ps1``",
        "- ``scripts/missing_tool.ps1``",
        "- ``scripts/existing_tool.ps1``"
    ) -join "`n")
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "docs\index.rst") `
    -Text "script_task_index_zh"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "docs\documentation_maintenance_zh.rst") `
    -Text "docs/script_task_index_zh.rst`ncheck_script_task_index.ps1"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "docs\project_score_assessment_zh.rst") `
    -Text "docs/script_task_index_zh.rst`ncheck_script_task_index.ps1"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "README.md") `
    -Text "docs/documentation_maintenance_zh.rst`ndocs/script_task_index_zh.rst"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "README.zh-CN.md") `
    -Text "docs/script_task_index_zh.rst"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "test\CMakeLists.txt") `
    -Text (@(
        "check_script_task_index",
        "check_script_task_index_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;scripts"'
    ) -join "`n")
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "scripts\check_script_task_index.ps1") `
    -Text "param()`n"
Write-Utf8NoBomFile `
    -Path (Join-Path $failingRoot "scripts\existing_tool.ps1") `
    -Text "param()`n"

$failingSummaryJson = Join-Path $failingRoot "summary.json"
$failingReportMarkdown = Join-Path $failingRoot "script-task-index-report.md"
$failed = $false
$failureOutput = @()
try {
    $failureOutput = Invoke-IndexCheck `
        -Root $failingRoot `
        -SummaryJson $failingSummaryJson `
        -ReportMarkdown $failingReportMarkdown
} catch {
    $failed = $true
    $failureOutput += $_.Exception.Message
}
if (-not $failed) {
    throw "check_script_task_index.ps1 unexpectedly passed with a missing indexed script."
}
$joinedFailureOutput = ($failureOutput | ForEach-Object { $_.ToString() }) -join "`n"
if ($joinedFailureOutput -notmatch [regex]::Escape("MissingScripts=1")) {
    throw "Expected missing script count in failure output, got: $joinedFailureOutput"
}
if ($joinedFailureOutput -notmatch [regex]::Escape("DuplicateScriptReferences=1")) {
    throw "Expected duplicate script reference count in failure output, got: $joinedFailureOutput"
}
if ($joinedFailureOutput -notmatch [regex]::Escape("MissingMarkers=1")) {
    throw "Expected missing marker count in failure output, got: $joinedFailureOutput"
}
if (-not (Test-Path -LiteralPath $failingSummaryJson -PathType Leaf)) {
    throw "check_script_task_index.ps1 did not write a failing summary."
}
if (-not (Test-Path -LiteralPath $failingReportMarkdown -PathType Leaf)) {
    throw "check_script_task_index.ps1 did not write a failing Markdown report."
}
Assert-FileHasNoBom -Path $failingSummaryJson
Assert-FileHasNoBom -Path $failingReportMarkdown
$failingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failingSummaryJson | ConvertFrom-Json
if ($failingSummary.status -ne "failed") {
    throw "Expected failing summary status, got: $($failingSummary.status)"
}
if ($failingSummary.missing_script_count -ne 1) {
    throw "Expected one missing script, got: $($failingSummary.missing_script_count)"
}
if ($failingSummary.duplicate_script_reference_count -ne 1) {
    throw "Expected one duplicate script reference, got: $($failingSummary.duplicate_script_reference_count)"
}
if ($failingSummary.missing_marker_count -ne 1) {
    throw "Expected one missing marker, got: $($failingSummary.missing_marker_count)"
}
if ($failingSummary.script_reference_group_count -ne 1) {
    throw "Expected one script reference group in failing fixture, got: $($failingSummary.script_reference_group_count)"
}
if ($failingSummary.documentation_entrypoint_count -ne 2) {
    throw "Expected two documentation entrypoints in failing fixture, got: $($failingSummary.documentation_entrypoint_count)"
}
if ($failingSummary.script_reference_extension_count -ne 1) {
    throw "Expected one script reference extension in failing fixture, got: $($failingSummary.script_reference_extension_count)"
}
$failingGroup = @($failingSummary.script_reference_groups)[0]
if ($failingGroup.total_script_reference_count -ne 4) {
    throw "Expected four total script references in failing fixture group, got: $($failingGroup.total_script_reference_count)"
}
if ($failingGroup.script_reference_count -ne 3) {
    throw "Expected three unique script references in failing fixture group, got: $($failingGroup.script_reference_count)"
}
if ($failingGroup.duplicate_script_reference_count -ne 1) {
    throw "Expected one duplicate script reference in failing fixture group, got: $($failingGroup.duplicate_script_reference_count)"
}
$failingExtension = @($failingSummary.script_reference_extensions)[0]
if ($failingExtension.extension -ne ".ps1") {
    throw "Expected .ps1 extension in failing fixture, got: $($failingExtension.extension)"
}
if ($failingExtension.total_script_reference_count -ne 4) {
    throw "Expected four total script references in failing fixture extension, got: $($failingExtension.total_script_reference_count)"
}
if ($failingExtension.script_reference_count -ne 3) {
    throw "Expected three unique script references in failing fixture extension, got: $($failingExtension.script_reference_count)"
}
if ($failingExtension.duplicate_script_reference_count -ne 1) {
    throw "Expected one duplicate script reference in failing fixture extension, got: $($failingExtension.duplicate_script_reference_count)"
}
Assert-ArrayContains `
    -Values @($failingSummary.missing_scripts) `
    -ExpectedValue "scripts\missing_tool.ps1" `
    -Message "Failing summary should list the missing script."
Assert-ArrayContains `
    -Values @($failingSummary.duplicate_script_references | ForEach-Object { $_.relative_path }) `
    -ExpectedValue "scripts\existing_tool.ps1" `
    -Message "Failing summary should list the duplicate script reference."
Assert-ArrayContains `
    -Values @($failingSummary.missing_markers | ForEach-Object { "$($_.document)|$($_.marker)" }) `
    -ExpectedValue "README.zh-CN.md|docs/documentation_maintenance_zh.rst" `
    -Message "Failing summary should list the missing README marker."
foreach ($marker in @(
        '- status: `failed`',
        '- documentation_entrypoint_count: `2`',
        '- script_reference_group_count: `1`',
        '- script_reference_extension_count: `1`',
        '- duplicate_script_reference_count: `1`',
        '- missing_script_count: `1`',
        '- missing_marker_count: `1`',
        '[missing] `scripts\missing_tool.ps1`',
        '## Documentation Entry Points',
        '`README.md`: 2 markers',
        '`README.zh-CN.md`: 2 markers',
        '## Script Reference Groups',
        '`Script task index`: 3 unique / 4 total',
        '## Script Reference Extensions',
        '`.ps1`: 3 unique / 4 total',
        '## Duplicate Script References',
        '`scripts\existing_tool.ps1` x2',
        '## Missing Scripts',
        '`scripts\missing_tool.ps1`',
        '## Missing Markers',
        '`README.zh-CN.md` missing `docs/documentation_maintenance_zh.rst`'
    )) {
    Assert-FileContainsText -Path $failingReportMarkdown -ExpectedText $marker `
        -Message "Failing Markdown report should include marker."
}

Write-Host "Script task index checker regression passed."
