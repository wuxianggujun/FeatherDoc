param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [string]$BuildDir = "build-codex-clang-compat",
    [string]$OutputDir = "output/table-layout-delivery-report",
    [ValidateSet("paragraph-callout", "page-corner", "margin-anchor")]
    [string]$PositionPreset = "paragraph-callout",
    [string]$PositionedOutputDocx = "",
    [string]$StyleLookOutputDocx = "",
    [string]$VisualRegressionOutputDir = "",
    [switch]$ReplacePositioned,
    [switch]$SkipBuild,
    [switch]$FailOnIssue
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Write-Step { param([string]$Message) Write-Host "[table-layout-delivery] $Message" }
function Resolve-RepoRoot { return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path }
function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$InputPath)
    if ([System.IO.Path]::IsPathRooted($InputPath)) { return [System.IO.Path]::GetFullPath($InputPath) }
    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}
function Get-VcvarsPath {
    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )
    foreach ($path in $candidates) { if (Test-Path -LiteralPath $path) { return $path } }
    throw "Could not locate vcvars64.bat for MSVC command-line builds."
}
function Invoke-MsvcCommand {
    param([string]$VcvarsPath, [string]$CommandText)
    & cmd.exe /c "call `"$VcvarsPath`" && $CommandText"
    if ($LASTEXITCODE -ne 0) { throw "MSVC command failed: $CommandText" }
}
function Resolve-BuildSearchRoot {
    param([string]$RepoRoot, [string]$PreferredBuildRoot, [string[]]$TargetNames)
    $candidateRoots = @()
    if (Test-Path -LiteralPath $PreferredBuildRoot) { $candidateRoots += (Resolve-Path $PreferredBuildRoot).Path }
    $candidateRoots += @(Get-ChildItem -LiteralPath $RepoRoot -Directory -Filter "build*" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -ExpandProperty FullName)
    foreach ($root in ($candidateRoots | Select-Object -Unique)) {
        $hasAllTargets = $true
        foreach ($targetName in $TargetNames) {
            $target = Get-ChildItem -LiteralPath $root -Recurse -File -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -ieq "$targetName.exe" -or $_.Name -ieq $targetName } |
                Select-Object -First 1
            if (-not $target) { $hasAllTargets = $false; break }
        }
        if ($hasAllTargets) { return $root }
    }
    throw "Could not locate a build directory containing targets: $($TargetNames -join ', ')."
}
function Find-BuildExecutable {
    param([string]$BuildRoot, [string]$TargetName)
    $candidates = @(Get-ChildItem -LiteralPath $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } |
        Sort-Object LastWriteTime -Descending)
    if ($candidates.Count -eq 0) { throw "Could not find built executable for target '$TargetName' under $BuildRoot." }
    return $candidates[0].FullName
}
function Invoke-CommandCapture {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureMessage
    )
    $commandOutput = @(& $ExecutablePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) { $lines | Set-Content -LiteralPath $OutputPath -Encoding UTF8 }
    if ($exitCode -ne 0) { throw $FailureMessage }
}
function Read-JsonFile { param([string]$Path) return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json }
function Get-PropertyValue {
    param($Object, [string]$Name, $Default = $null)
    if ($null -eq $Object) { return $Default }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $Default }
    return $property.Value
}
function Get-IntValue {
    param($Object, [string]$Name)
    $value = Get-PropertyValue -Object $Object -Name $Name -Default 0
    if ($null -eq $value) { return 0 }
    return [int]$value
}
function ConvertTo-CommandLine {
    param([string[]]$Parts)
    $escaped = foreach ($part in $Parts) {
        if ($null -eq $part) { continue }
        if ($part -match '^[A-Za-z0-9_./:\\=-]+$') { $part } else { '"' + ($part -replace '"', '\"') + '"' }
    }
    return ($escaped -join " ")
}
function ConvertTo-SafeFileName {
    param([string]$Value)
    $safe = $Value
    foreach ($char in [System.IO.Path]::GetInvalidFileNameChars()) { $safe = $safe.Replace([string]$char, "_") }
    if ([string]::IsNullOrWhiteSpace($safe)) { return "unnamed" }
    return $safe
}
function Get-UniqueTableStyleIds {
    param($TablesJson)
    $ids = New-Object System.Collections.Generic.List[string]
    foreach ($table in @($TablesJson.tables)) {
        $styleId = Get-PropertyValue -Object $table -Name "style_id" -Default $null
        if (-not [string]::IsNullOrWhiteSpace([string]$styleId) -and -not $ids.Contains([string]$styleId)) {
            $ids.Add([string]$styleId)
        }
    }
    return $ids.ToArray()
}
function New-OutputSuggestion {
    param([string]$Id, [string]$Label, [string]$Command, [string]$Reason)
    return [ordered]@{
        id = $Id
        label = $Label
        command = $Command
        reason = $Reason
    }
}
function Write-MarkdownReport {
    param(
        [string]$Path,
        [object]$Summary,
        [object[]]$Suggestions,
        [string[]]$StyleIds,
        [object[]]$VisualEntries
    )
    $lines = @(
        "# Table layout delivery report",
        "",
        "## Scope",
        "",
        "- Input DOCX: $($Summary.input_docx)",
        "- Position preset: $($Summary.position_preset)",
        "- Table count: $($Summary.table_count)",
        "- Styled table count: $($Summary.styled_table_count)",
        "- Positioned table count: $($Summary.positioned_table_count)",
        "- Unpositioned table count: $($Summary.unpositioned_table_count)",
        "",
        "## Quality gates",
        "",
        "- Table style quality issues: $($Summary.table_style_quality_issue_count)",
        "- tblLook issues: $($Summary.table_style_look_issue_count)",
        "- Position automatic count: $($Summary.position_automatic_count)",
        "- Position review count: $($Summary.position_review_count)",
        "- Position already matching count: $($Summary.position_already_matching_count)",
        "- Dry-run validated: $($Summary.position_plan_dry_run_ok)",
        ""
    )
    if ($StyleIds.Count -gt 0) {
        $lines += "## Table styles"
        $lines += ""
        foreach ($styleId in $StyleIds) { $lines += "- $styleId" }
        $lines += ""
    }
    $lines += "## Suggested actions"
    $lines += ""
    if ($Suggestions.Count -eq 0) {
        $lines += "- No automatic action is currently required."
    } else {
        foreach ($suggestion in $Suggestions) {
            $lines += "- $($suggestion.label): $($suggestion.reason)"
            $lines += "  - Command: ``$($suggestion.command)``"
        }
    }
    $lines += ""
    $lines += "## Visual regression entries"
    $lines += ""
    foreach ($entry in $VisualEntries) {
        $lines += "- $($entry.label): ``$($entry.command)``"
    }
    $lines += ""
    $lines += "## Raw artifacts"
    $lines += ""
    $lines += "- JSON summary: $($Summary.summary_json)"
    $lines += "- Inspect tables: $($Summary.inspect_tables_json)"
    $lines += "- Table style quality audit: $($Summary.table_style_quality_audit_json)"
    $lines += "- Table style quality plan: $($Summary.table_style_quality_plan_json)"
    $lines += "- tblLook check: $($Summary.table_style_look_check_json)"
    $lines += "- tblLook repair plan: $($Summary.table_style_look_repair_plan_json)"
    $lines += "- Position plan command output: $($Summary.position_plan_json)"
    $lines += "- Position replay plan: $($Summary.position_replay_plan_json)"
    $lines += "- Position dry run: $($Summary.position_plan_dry_run_json)"
    $lines | Set-Content -LiteralPath $Path -Encoding UTF8
}

$repoRoot = Resolve-RepoRoot
$resolvedInputDocx = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
if (-not (Test-Path -LiteralPath $resolvedInputDocx)) { throw "Input DOCX does not exist: $resolvedInputDocx" }
New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

if ([string]::IsNullOrWhiteSpace($PositionedOutputDocx)) {
    $stem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedInputDocx)
    $PositionedOutputDocx = Join-Path $resolvedOutputDir "$stem-table-position-$PositionPreset.docx"
} else {
    $PositionedOutputDocx = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $PositionedOutputDocx
}
if ([string]::IsNullOrWhiteSpace($StyleLookOutputDocx)) {
    $stem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedInputDocx)
    $StyleLookOutputDocx = Join-Path $resolvedOutputDir "$stem-table-style-look-fixed.docx"
} else {
    $StyleLookOutputDocx = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $StyleLookOutputDocx
}
if ([string]::IsNullOrWhiteSpace($VisualRegressionOutputDir)) {
    $VisualRegressionOutputDir = Join-Path $resolvedOutputDir "visual-regression"
} else {
    $VisualRegressionOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $VisualRegressionOutputDir
}

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_CLI=ON -DBUILD_TESTING=OFF"
    Write-Step "Building featherdoc_cli"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli"
}
$buildSearchRoot = Resolve-BuildSearchRoot -RepoRoot $repoRoot -PreferredBuildRoot $resolvedBuildDir -TargetNames @("featherdoc_cli")
if ($buildSearchRoot -ne $resolvedBuildDir) { Write-Step "Using existing build directory $buildSearchRoot" }
$cliExecutable = Find-BuildExecutable -BuildRoot $buildSearchRoot -TargetName "featherdoc_cli"

$inspectTablesJsonPath = Join-Path $resolvedOutputDir "inspect-tables.json"
$styleDefinitionsDir = Join-Path $resolvedOutputDir "table-style-definitions"
$styleQualityAuditJsonPath = Join-Path $resolvedOutputDir "audit-table-style-quality.json"
$styleQualityPlanJsonPath = Join-Path $resolvedOutputDir "plan-table-style-quality-fixes.json"
$styleLookCheckJsonPath = Join-Path $resolvedOutputDir "check-table-style-look.json"
$styleLookRepairPlanJsonPath = Join-Path $resolvedOutputDir "repair-table-style-look-plan.json"
$positionPlanJsonPath = Join-Path $resolvedOutputDir "plan-table-position-$PositionPreset.json"
$positionReplayPlanJsonPath = Join-Path $resolvedOutputDir "table-position-$PositionPreset.replay-plan.json"
$positionDryRunJsonPath = Join-Path $resolvedOutputDir "apply-table-position-plan-dry-run.json"
$summaryJsonPath = Join-Path $resolvedOutputDir "table-layout-delivery-report.json"
$summaryMarkdownPath = Join-Path $resolvedOutputDir "table-layout-delivery-report.md"

Write-Step "Inspecting body tables"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("inspect-tables", $resolvedInputDocx, "--json") -OutputPath $inspectTablesJsonPath -FailureMessage "inspect-tables failed."
$inspectTables = Read-JsonFile -Path $inspectTablesJsonPath
$styleIds = @(Get-UniqueTableStyleIds -TablesJson $inspectTables)
if ($styleIds.Count -gt 0) {
    New-Item -ItemType Directory -Path $styleDefinitionsDir -Force | Out-Null
    foreach ($styleId in $styleIds) {
        $stylePath = Join-Path $styleDefinitionsDir ("inspect-table-style-{0}.json" -f (ConvertTo-SafeFileName -Value $styleId))
        Write-Step "Inspecting table style $styleId"
        Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("inspect-table-style", $resolvedInputDocx, $styleId, "--json") -OutputPath $stylePath -FailureMessage "inspect-table-style failed for '$styleId'."
    }
}

Write-Step "Auditing table style quality"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("audit-table-style-quality", $resolvedInputDocx, "--json") -OutputPath $styleQualityAuditJsonPath -FailureMessage "audit-table-style-quality failed."
Write-Step "Planning table style quality fixes"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("plan-table-style-quality-fixes", $resolvedInputDocx, "--json") -OutputPath $styleQualityPlanJsonPath -FailureMessage "plan-table-style-quality-fixes failed."
Write-Step "Checking tblLook routing"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("check-table-style-look", $resolvedInputDocx, "--json") -OutputPath $styleLookCheckJsonPath -FailureMessage "check-table-style-look failed."
Write-Step "Planning tblLook repair"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("repair-table-style-look", $resolvedInputDocx, "--plan-only", "--json") -OutputPath $styleLookRepairPlanJsonPath -FailureMessage "repair-table-style-look --plan-only failed."

$positionPlanArgs = @("plan-table-position-presets", $resolvedInputDocx, "--preset", $PositionPreset, "--output", $PositionedOutputDocx, "--output-plan", $positionReplayPlanJsonPath, "--json")
if ($ReplacePositioned) { $positionPlanArgs += "--replace-positioned" }
Write-Step "Planning floating table preset $PositionPreset"
Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments $positionPlanArgs -OutputPath $positionPlanJsonPath -FailureMessage "plan-table-position-presets failed."
$positionPlan = Read-JsonFile -Path $positionPlanJsonPath
$positionAutomaticCount = Get-IntValue -Object $positionPlan -Name "automatic_count"
if ($positionAutomaticCount -gt 0) {
    Write-Step "Dry-running table position plan replay"
    Invoke-CommandCapture -ExecutablePath $cliExecutable -Arguments @("apply-table-position-plan", $positionReplayPlanJsonPath, "--dry-run", "--json") -OutputPath $positionDryRunJsonPath -FailureMessage "apply-table-position-plan --dry-run failed."
} else {
    Write-Step "Skipping table position plan replay dry-run because no automatic preset changes are pending"
    ([ordered]@{
        command = "apply-table-position-plan"
        ok = $true
        skipped = $true
        reason = "no automatic table indices to apply"
        plan_path = $positionReplayPlanJsonPath
    } | ConvertTo-Json -Depth 5) | Set-Content -LiteralPath $positionDryRunJsonPath -Encoding UTF8
}

$styleQualityAudit = Read-JsonFile -Path $styleQualityAuditJsonPath
$styleQualityPlan = Read-JsonFile -Path $styleQualityPlanJsonPath
$styleLookCheck = Read-JsonFile -Path $styleLookCheckJsonPath
$styleLookRepairPlan = Read-JsonFile -Path $styleLookRepairPlanJsonPath
$positionDryRun = Read-JsonFile -Path $positionDryRunJsonPath

$tableCount = Get-IntValue -Object $inspectTables -Name "count"
$styledTableCount = 0
$positionedTableCount = 0
foreach ($table in @($inspectTables.tables)) {
    if (-not [string]::IsNullOrWhiteSpace([string](Get-PropertyValue -Object $table -Name "style_id" -Default ""))) { $styledTableCount++ }
    if ($null -ne (Get-PropertyValue -Object $table -Name "position" -Default $null)) { $positionedTableCount++ }
}
$unpositionedTableCount = [Math]::Max(0, $tableCount - $positionedTableCount)

$styleQualityIssueCount = Get-IntValue -Object $styleQualityAudit -Name "issue_count"
$styleQualityAutoFixCount = Get-IntValue -Object $styleQualityPlan -Name "automatic_fix_count"
$styleQualityManualFixCount = Get-IntValue -Object $styleQualityPlan -Name "manual_fix_count"
$styleLookIssueCount = Get-IntValue -Object $styleLookCheck -Name "issue_count"
$positionReviewCount = Get-IntValue -Object $positionPlan -Name "review_count"
$positionAlreadyMatchingCount = Get-IntValue -Object $positionPlan -Name "already_matching_count"
$positionDryRunOk = [bool](Get-PropertyValue -Object $positionDryRun -Name "ok" -Default $false)

$suggestions = New-Object System.Collections.Generic.List[object]
if ($styleQualityAutoFixCount -gt 0) {
    $suggestions.Add((New-OutputSuggestion `
        -Id "apply-table-style-quality-look-only" `
        -Label "Apply safe tblLook quality fixes" `
        -Command (ConvertTo-CommandLine -Parts @($cliExecutable, "apply-table-style-quality-fixes", $resolvedInputDocx, "--look-only", "--output", $StyleLookOutputDocx, "--json")) `
        -Reason "The table style quality plan reports $styleQualityAutoFixCount automatic look-only fix(es)."))
}
if ($styleLookIssueCount -gt 0) {
    $suggestions.Add((New-OutputSuggestion `
        -Id "repair-table-style-look" `
        -Label "Repair table instance tblLook flags" `
        -Command (ConvertTo-CommandLine -Parts @($cliExecutable, "repair-table-style-look", $resolvedInputDocx, "--apply", "--output", $StyleLookOutputDocx, "--json")) `
        -Reason "The tblLook check reports $styleLookIssueCount issue(s) that can be reviewed through the repair plan."))
}
if ($positionAutomaticCount -gt 0) {
    $suggestions.Add((New-OutputSuggestion `
        -Id "apply-table-position-plan" `
        -Label "Apply floating table position preset" `
        -Command (ConvertTo-CommandLine -Parts @($cliExecutable, "apply-table-position-plan", $positionReplayPlanJsonPath, "--json")) `
        -Reason "The $PositionPreset preset can be applied to $positionAutomaticCount table(s) after plan review."))
}
if ($positionReviewCount -gt 0) {
    $suggestions.Add((New-OutputSuggestion `
        -Id "review-positioned-tables" `
        -Label "Review existing floating table positions" `
        -Command (ConvertTo-CommandLine -Parts @($cliExecutable, "plan-table-position-presets", $resolvedInputDocx, "--preset", $PositionPreset, "--replace-positioned", "--output", $PositionedOutputDocx, "--output-plan", $positionReplayPlanJsonPath, "--json")) `
        -Reason "$positionReviewCount table(s) already have a position and need explicit replacement review."))
}
if ($styleQualityManualFixCount -gt 0) {
    $suggestions.Add((New-OutputSuggestion `
        -Id "manual-table-style-definition-review" `
        -Label "Review table style definition manually" `
        -Command (ConvertTo-CommandLine -Parts @($cliExecutable, "inspect-table-style", $resolvedInputDocx, "<style-id>", "--json")) `
        -Reason "The style quality plan reports $styleQualityManualFixCount manual table style definition fix(es)."))
}

$wordVisualSmokeCommand = ConvertTo-CommandLine -Parts @(
    "powershell", "-ExecutionPolicy", "Bypass", "-File", ".\scripts\run_word_visual_smoke.ps1",
    "-BuildDir", $BuildDir, "-InputDocx", $resolvedInputDocx, "-OutputDir", (Join-Path $VisualRegressionOutputDir "word-smoke"), "-SkipBuild"
)
$tableStyleQualityVisualCommand = ConvertTo-CommandLine -Parts @(
    "powershell", "-ExecutionPolicy", "Bypass", "-File", ".\scripts\run_table_style_quality_visual_regression.ps1",
    "-BuildDir", $BuildDir, "-OutputDir", (Join-Path $VisualRegressionOutputDir "table-style-quality"), "-SkipBuild"
)
$visualEntries = @(
    [ordered]@{
        id = "word-visual-smoke"
        label = "Render this DOCX through Word visual smoke"
        command = $wordVisualSmokeCommand
    },
    [ordered]@{
        id = "table-style-quality-visual-regression"
        label = "Run curated table style quality visual regression"
        command = $tableStyleQualityVisualCommand
    }
)

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    input_docx = $resolvedInputDocx
    output_dir = $resolvedOutputDir
    requested_build_dir = $resolvedBuildDir
    build_dir = $buildSearchRoot
    cli_executable = $cliExecutable
    position_preset = $PositionPreset
    replace_positioned = [bool]$ReplacePositioned
    table_count = $tableCount
    styled_table_count = $styledTableCount
    positioned_table_count = $positionedTableCount
    unpositioned_table_count = $unpositionedTableCount
    table_style_ids = $styleIds
    table_style_quality_issue_count = $styleQualityIssueCount
    table_style_quality_automatic_fix_count = $styleQualityAutoFixCount
    table_style_quality_manual_fix_count = $styleQualityManualFixCount
    table_style_look_issue_count = $styleLookIssueCount
    table_style_look_repair_plan_item_count = Get-IntValue -Object $styleLookRepairPlan -Name "plan_item_count"
    position_automatic_count = $positionAutomaticCount
    position_review_count = $positionReviewCount
    position_already_matching_count = $positionAlreadyMatchingCount
    position_plan_dry_run_ok = $positionDryRunOk
    suggested_actions = @($suggestions.ToArray())
    visual_regression_entries = $visualEntries
    summary_json = $summaryJsonPath
    summary_markdown = $summaryMarkdownPath
    inspect_tables_json = $inspectTablesJsonPath
    table_style_definitions_dir = if ($styleIds.Count -gt 0) { $styleDefinitionsDir } else { $null }
    table_style_quality_audit_json = $styleQualityAuditJsonPath
    table_style_quality_plan_json = $styleQualityPlanJsonPath
    table_style_look_check_json = $styleLookCheckJsonPath
    table_style_look_repair_plan_json = $styleLookRepairPlanJsonPath
    position_plan_json = $positionPlanJsonPath
    position_replay_plan_json = $positionReplayPlanJsonPath
    position_plan_dry_run_json = $positionDryRunJsonPath
    positioned_output_docx = $PositionedOutputDocx
    style_look_output_docx = $StyleLookOutputDocx
}

($summary | ConvertTo-Json -Depth 20) | Set-Content -LiteralPath $summaryJsonPath -Encoding UTF8
Write-MarkdownReport -Path $summaryMarkdownPath -Summary $summary -Suggestions @($suggestions.ToArray()) -StyleIds $styleIds -VisualEntries $visualEntries

Write-Step "Completed table layout delivery report"
Write-Host "JSON: $summaryJsonPath"
Write-Host "Markdown: $summaryMarkdownPath"

if ($FailOnIssue -and (($styleQualityIssueCount -gt 0) -or ($styleLookIssueCount -gt 0) -or ($positionAutomaticCount -gt 0) -or ($positionReviewCount -gt 0) -or (-not $positionDryRunOk))) {
    throw "Table layout delivery report found issues or pending layout actions."
}
