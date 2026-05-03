<#
.SYNOPSIS
Builds a table layout delivery report for one DOCX/DOTX file.

.DESCRIPTION
Aggregates table style quality checks, safe tblLook repair planning, and
floating table preset planning into one JSON/Markdown handoff. The script does
not mutate the input document.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [string]$OutputDir = "output/table-layout-delivery",
    [ValidateSet("paragraph-callout", "page-corner", "margin-anchor")]
    [string]$Preset = "paragraph-callout",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [string]$CliPath = "",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$SkipBuild,
    [switch]$FailOnIssue
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[table-layout-delivery] $Message"
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-JsonInt {
    param($Object, [string]$Name)
    if ($null -eq $Object) { return 0 }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) { return 0 }
    return [int]$property.Value
}

function Read-JsonObjectFromLines {
    param([string[]]$Lines, [string]$Command)

    foreach ($line in $Lines) {
        $text = [string]$line
        if (-not $text.TrimStart().StartsWith("{")) { continue }
        try {
            $object = $text | ConvertFrom-Json
            $property = $object.PSObject.Properties["command"]
            if ($null -ne $property -and [string]$property.Value -eq $Command) {
                return $object
            }
        } catch {
            continue
        }
    }
    throw "Command '$Command' did not emit a JSON object."
}

function Invoke-CliJsonCommand {
    param(
        [string]$ExecutablePath,
        [string]$Name,
        [string[]]$Arguments,
        [string]$Command
    )

    $result = Invoke-TemplateSchemaCli -ExecutablePath $ExecutablePath -Arguments $Arguments
    foreach ($line in $result.Output) { Write-Host $line }

    $json = $null
    $errorMessage = ""
    try {
        $json = Read-JsonObjectFromLines -Lines $result.Output -Command $Command
    } catch {
        $errorMessage = $_.Exception.Message
    }

    if ($result.ExitCode -ne 0 -and [string]::IsNullOrWhiteSpace($errorMessage)) {
        $errorMessage = "featherdoc_cli $($Arguments -join ' ') failed with exit code $($result.ExitCode)."
    }

    return [ordered]@{
        name = $Name
        command = $Command
        arguments = @($Arguments)
        exit_code = $result.ExitCode
        ok = ($result.ExitCode -eq 0 -and [string]::IsNullOrWhiteSpace($errorMessage))
        json = $json
        error_message = $errorMessage
    }
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Table Layout Delivery Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Input DOCX: ``$($Summary.input_docx)``") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Ready: ``$($Summary.ready)``") | Out-Null
    $lines.Add("- Preset: ``$($Summary.preset)``") | Out-Null
    $lines.Add("- Table style quality issues: ``$($Summary.table_style_issue_count)``") | Out-Null
    $lines.Add("- Automatic tblLook fixes: ``$($Summary.automatic_tblLook_fix_count)``") | Out-Null
    $lines.Add("- Manual table style fixes: ``$($Summary.manual_table_style_fix_count)``") | Out-Null
    $lines.Add("- Position automatic count: ``$($Summary.table_position_automatic_count)``") | Out-Null
    $lines.Add("- Position review count: ``$($Summary.table_position_review_count)``") | Out-Null
    $lines.Add("- Command failures: ``$($Summary.command_failure_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Suggested Actions") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($item in @($Summary.action_items)) {
        $lines.Add("- $($item.title)") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace([string]$item.command)) {
            $lines.Add("  ``$($item.command)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.id)``: $($blocker.message)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Raw Artifacts") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Position plan: ``$($Summary.table_position_plan_path)``") | Out-Null
    $lines.Add("- JSON summary: ``$($Summary.summary_json)``") | Out-Null

    return @($lines)
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedOutputDir = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "table_layout_delivery_report.md"
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $ReportMarkdown -AllowMissing
}
$positionPlanPath = Join-Path $resolvedOutputDir "table-position-$Preset.plan.json"
$positionedOutputPath = Join-Path $resolvedOutputDir "$([System.IO.Path]::GetFileNameWithoutExtension($resolvedInputDocx))-table-position-$Preset.docx"

Write-Step "Resolving featherdoc_cli"
$resolvedCliPath = if ([string]::IsNullOrWhiteSpace($CliPath)) {
    Resolve-TemplateSchemaCliPath -RepoRoot $repoRoot -BuildDir $BuildDir -Generator $Generator -SkipBuild:$SkipBuild
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $CliPath
}

Write-Step "Auditing table style quality"
$auditResult = Invoke-CliJsonCommand -ExecutablePath $resolvedCliPath -Name "audit_table_style_quality" -Command "audit-table-style-quality" -Arguments @(
    "audit-table-style-quality",
    $resolvedInputDocx,
    "--json"
)

Write-Step "Planning table style quality fixes"
$fixPlanResult = Invoke-CliJsonCommand -ExecutablePath $resolvedCliPath -Name "plan_table_style_quality_fixes" -Command "plan-table-style-quality-fixes" -Arguments @(
    "plan-table-style-quality-fixes",
    $resolvedInputDocx,
    "--json"
)

Write-Step "Planning floating table preset"
$positionPlanResult = Invoke-CliJsonCommand -ExecutablePath $resolvedCliPath -Name "plan_table_position_presets" -Command "plan-table-position-presets" -Arguments @(
    "plan-table-position-presets",
    $resolvedInputDocx,
    "--preset",
    $Preset,
    "--output",
    $positionedOutputPath,
    "--output-plan",
    $positionPlanPath,
    "--json"
)

$commands = @($auditResult, $fixPlanResult, $positionPlanResult)
$commandFailures = @($commands | Where-Object { -not [bool]$_.ok })
$audit = $auditResult.json
$fixPlan = $fixPlanResult.json
$positionPlan = $positionPlanResult.json
$issueCount = Get-JsonInt -Object $audit -Name "issue_count"
$automaticFixCount = Get-JsonInt -Object $fixPlan -Name "automatic_fix_count"
$manualFixCount = Get-JsonInt -Object $fixPlan -Name "manual_fix_count"
$positionAutomaticCount = Get-JsonInt -Object $positionPlan -Name "automatic_count"
$positionReviewCount = Get-JsonInt -Object $positionPlan -Name "review_count"
$positionAlreadyMatchingCount = Get-JsonInt -Object $positionPlan -Name "already_matching_count"

$releaseBlockers = @()
if ($commandFailures.Count -gt 0) {
    $releaseBlockers += [ordered]@{
        id = "table_layout.command_failures"
        severity = "error"
        message = "One or more table layout delivery commands failed."
        action = "inspect_failed_command_output"
        command_failure_count = $commandFailures.Count
    }
}
if ($issueCount -gt 0 -and $manualFixCount -gt 0) {
    $releaseBlockers += [ordered]@{
        id = "table_layout.manual_table_style_quality_work"
        severity = "warning"
        message = "Table style quality has issues that need manual style-definition work."
        action = "review_table_style_quality_plan"
        issue_count = $issueCount
        manual_fix_count = $manualFixCount
    }
}
if ($positionReviewCount -gt 0) {
    $releaseBlockers += [ordered]@{
        id = "table_layout.positioned_tables_need_review"
        severity = "warning"
        message = "Some existing floating table positions need manual review before applying the preset."
        action = "review_table_position_plan"
        review_count = $positionReviewCount
    }
}

$actionItems = @(
    [ordered]@{
        id = "review_table_style_quality"
        title = "Review table style quality audit and safe tblLook fixes"
        command = "featherdoc_cli plan-table-style-quality-fixes <input.docx> --json"
    },
    [ordered]@{
        id = "apply_safe_tblLook_fixes"
        title = "Apply safe tblLook-only fixes after review"
        command = "featherdoc_cli apply-table-style-quality-fixes <input.docx> --look-only --output quality-fixed.docx --json"
    },
    [ordered]@{
        id = "review_table_position_preset"
        title = "Review floating table preset plan before applying"
        command = "featherdoc_cli apply-table-position-plan `"$positionPlanPath`" --dry-run --json"
    },
    [ordered]@{
        id = "run_table_style_quality_visual_regression"
        title = "Generate Word-rendered evidence for table style delivery changes"
        command = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_table_style_quality_visual_regression.ps1"
    }
)

$status = if ($commandFailures.Count -gt 0) {
    "failed"
} elseif (@($releaseBlockers).Count -gt 0 -or $automaticFixCount -gt 0 -or $positionAutomaticCount -gt 0) {
    "needs_review"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = "featherdoc.table_layout_delivery_report.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    ready = ($status -eq "ready")
    input_docx = $resolvedInputDocx
    output_dir = $resolvedOutputDir
    summary_json = $summaryPath
    report_markdown = $markdownPath
    cli_path = $resolvedCliPath
    preset = $Preset
    table_style_quality_audit = $audit
    table_style_quality_fix_plan = $fixPlan
    table_position_plan = $positionPlan
    table_position_plan_path = $positionPlanPath
    positioned_output_docx = $positionedOutputPath
    table_style_issue_count = $issueCount
    automatic_tblLook_fix_count = $automaticFixCount
    manual_table_style_fix_count = $manualFixCount
    table_position_automatic_count = $positionAutomaticCount
    table_position_review_count = $positionReviewCount
    table_position_already_matching_count = $positionAlreadyMatchingCount
    release_blockers = @($releaseBlockers)
    release_blocker_count = @($releaseBlockers).Count
    action_items = @($actionItems)
    commands = @($commands)
    command_failure_count = $commandFailures.Count
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($commandFailures.Count -gt 0) { exit 1 }
if ($FailOnIssue -and @($releaseBlockers).Count -gt 0) { exit 1 }
