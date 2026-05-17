<#
.SYNOPSIS
Builds the final release-governance report pipeline from existing summaries.

.DESCRIPTION
Runs the read-only final governance scripts for numbering catalog, table
layout delivery, and project-template delivery readiness, then writes release
governance handoff and release blocker rollup outputs. The script consumes
existing JSON summaries only; it does not rerun CLI, CMake, Word, or visual
automation.
#>
param(
    [string]$InputRoot = "output",
    [string]$OutputRoot = "output/release-governance-pipeline",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$IncludeHandoffRollup,
    [switch]$FailOnStageFailure,
    [switch]$FailOnMissing,
    [switch]$FailOnBlocker,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[release-governance-pipeline] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$Path, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $root = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolved = [System.IO.Path]::GetFullPath($Path)
    if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
        return ".\" + ($relative -replace '/', '\')
    }
    return $resolved
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Get-StageDisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $RepoRoot $Path
    }
    return Get-DisplayPath -RepoRoot $RepoRoot -Path $candidate
}

function Get-StageSourceDisplay {
    param(
        [string]$RepoRoot,
        [object]$Item,
        [string]$PathProperty,
        [string]$DisplayProperty,
        [string]$DefaultDisplay
    )

    $displayValue = Get-JsonString -Object $Item -Name $DisplayProperty
    if (-not [string]::IsNullOrWhiteSpace($displayValue)) { return $displayValue }

    $pathValue = Get-JsonString -Object $Item -Name $PathProperty
    if (-not [string]::IsNullOrWhiteSpace($pathValue)) {
        return Get-StageDisplayPath -RepoRoot $RepoRoot -Path $pathValue
    }

    return $DefaultDisplay
}

function Get-StageDefaultOpenCommand {
    param(
        [string]$RepoRoot,
        [string]$ScriptPath
    )

    $scriptDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $ScriptPath
    if ([string]::IsNullOrWhiteSpace($scriptDisplay)) {
        return ""
    }

    return "pwsh -ExecutionPolicy Bypass -File $scriptDisplay"
}

function New-StageBlockerItems {
    param(
        [string]$RepoRoot,
        [string]$StageId,
        [string]$StageTitle,
        [string]$StageSchema,
        [string]$SummaryJson,
        [string]$SummaryJsonDisplay,
        [object[]]$Items
    )

    return @(
        foreach ($item in @($Items)) {
            $sourceReportDisplay = Get-StageSourceDisplay `
                -RepoRoot $RepoRoot `
                -Item $item `
                -PathProperty "source_report" `
                -DisplayProperty "source_report_display" `
                -DefaultDisplay $SummaryJsonDisplay
            $sourceJsonDisplay = Get-StageSourceDisplay `
                -RepoRoot $RepoRoot `
                -Item $item `
                -PathProperty "source_json" `
                -DisplayProperty "source_json_display" `
                -DefaultDisplay $sourceReportDisplay
            [ordered]@{
                stage_id = $StageId
                stage_title = $StageTitle
                id = Get-JsonString -Object $item -Name "id" -DefaultValue "release_blocker"
                project_id = Get-JsonString -Object $item -Name "project_id"
                template_name = Get-JsonString -Object $item -Name "template_name"
                candidate_type = Get-JsonString -Object $item -Name "candidate_type"
                severity = Get-JsonString -Object $item -Name "severity" -DefaultValue "error"
                status = Get-JsonString -Object $item -Name "status"
                action = Get-JsonString -Object $item -Name "action"
                message = Get-JsonString -Object $item -Name "message"
                source_schema = Get-JsonString -Object $item -Name "source_schema" -DefaultValue $StageSchema
                source_report = Get-JsonString -Object $item -Name "source_report" -DefaultValue $SummaryJson
                source_report_display = $sourceReportDisplay
                source_json = Get-JsonString -Object $item -Name "source_json" -DefaultValue (Get-JsonString -Object $item -Name "source_report" -DefaultValue $SummaryJson)
                source_json_display = $sourceJsonDisplay
            }
        }
    )
}

function New-StageActionItems {
    param(
        [string]$RepoRoot,
        [string]$StageId,
        [string]$StageTitle,
        [string]$StageSchema,
        [string]$SummaryJson,
        [string]$SummaryJsonDisplay,
        [string]$DefaultOpenCommand,
        [object[]]$Items
    )

    return @(
        foreach ($item in @($Items)) {
            $sourceReportDisplay = Get-StageSourceDisplay `
                -RepoRoot $RepoRoot `
                -Item $item `
                -PathProperty "source_report" `
                -DisplayProperty "source_report_display" `
                -DefaultDisplay $SummaryJsonDisplay
            $sourceJsonDisplay = Get-StageSourceDisplay `
                -RepoRoot $RepoRoot `
                -Item $item `
                -PathProperty "source_json" `
                -DisplayProperty "source_json_display" `
                -DefaultDisplay $sourceReportDisplay
            $command = Get-JsonString -Object $item -Name "command"
            $openCommandDefault = if (-not [string]::IsNullOrWhiteSpace($command)) {
                $command
            } else {
                $DefaultOpenCommand
            }
            [ordered]@{
                stage_id = $StageId
                stage_title = $StageTitle
                id = Get-JsonString -Object $item -Name "id" -DefaultValue "action_item"
                project_id = Get-JsonString -Object $item -Name "project_id"
                template_name = Get-JsonString -Object $item -Name "template_name"
                candidate_type = Get-JsonString -Object $item -Name "candidate_type"
                action = Get-JsonString -Object $item -Name "action"
                title = Get-JsonString -Object $item -Name "title"
                command = $command
                open_command = Get-JsonString -Object $item -Name "open_command" -DefaultValue $openCommandDefault
                source_schema = Get-JsonString -Object $item -Name "source_schema" -DefaultValue $StageSchema
                source_report = Get-JsonString -Object $item -Name "source_report" -DefaultValue $SummaryJson
                source_report_display = $sourceReportDisplay
                source_json = Get-JsonString -Object $item -Name "source_json" -DefaultValue (Get-JsonString -Object $item -Name "source_report" -DefaultValue $SummaryJson)
                source_json_display = $sourceJsonDisplay
            }
        }
    )
}

function New-StageWarningItems {
    param(
        [string]$RepoRoot,
        [string]$StageId,
        [string]$StageTitle,
        [string]$StageSchema,
        [string]$SummaryJson,
        [string]$SummaryJsonDisplay,
        [object[]]$Items
    )

    return @(
        foreach ($item in @($Items)) {
            $sourceReportDisplay = Get-StageSourceDisplay `
                -RepoRoot $RepoRoot `
                -Item $item `
                -PathProperty "source_report" `
                -DisplayProperty "source_report_display" `
                -DefaultDisplay $SummaryJsonDisplay
            $sourceJsonDisplay = Get-StageSourceDisplay `
                -RepoRoot $RepoRoot `
                -Item $item `
                -PathProperty "source_json" `
                -DisplayProperty "source_json_display" `
                -DefaultDisplay $sourceReportDisplay
            [ordered]@{
                stage_id = $StageId
                stage_title = $StageTitle
                id = Get-JsonString -Object $item -Name "id" -DefaultValue "warning"
                project_id = Get-JsonString -Object $item -Name "project_id"
                template_name = Get-JsonString -Object $item -Name "template_name"
                candidate_type = Get-JsonString -Object $item -Name "candidate_type"
                action = Get-JsonString -Object $item -Name "action" -DefaultValue "review_release_governance_warning"
                message = Get-JsonString -Object $item -Name "message"
                source_schema = Get-JsonString -Object $item -Name "source_schema" -DefaultValue $StageSchema
                source_report = Get-JsonString -Object $item -Name "source_report" -DefaultValue $SummaryJson
                source_report_display = $sourceReportDisplay
                source_json = Get-JsonString -Object $item -Name "source_json" -DefaultValue (Get-JsonString -Object $item -Name "source_report" -DefaultValue $SummaryJson)
                source_json_display = $sourceJsonDisplay
            }
        }
    )
}

function Get-ChildPowerShell {
    $powerShellPath = (Get-Process -Id $PID).Path
    if (-not [string]::IsNullOrWhiteSpace($powerShellPath)) {
        return $powerShellPath
    }

    $command = Get-Command pwsh -ErrorAction SilentlyContinue
    if (-not $command) {
        $command = Get-Command powershell.exe -ErrorAction SilentlyContinue
    }
    if (-not $command) {
        throw "Unable to resolve a PowerShell executable for nested script invocation."
    }
    return $command.Source
}

function Invoke-ReportScript {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments
    )

    $output = @(& (Get-ChildPowerShell) -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    foreach ($line in $output) {
        Write-Host $line
    }
    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = @($output | ForEach-Object { $_.ToString() })
    }
}

function Read-Summary {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }
    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function New-StageEntry {
    param(
        [string]$RepoRoot,
        [string]$Id,
        [string]$Title,
        [string]$Script,
        [string]$OutputDir,
        [string]$SummaryJson,
        [string]$ReportMarkdown,
        [string[]]$InputJson,
        [int]$ExitCode,
        [object]$Summary,
        [string]$ErrorMessage = ""
    )

    $status = if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        "failed"
    } elseif ($null -eq $Summary) {
        "missing_summary"
    } else {
        Get-JsonString -Object $Summary -Name "status" -DefaultValue "completed"
    }

    $summaryJsonDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $SummaryJson
    $stageSchema = Get-JsonString -Object $Summary -Name "schema" -DefaultValue "unknown"
    $defaultOpenCommand = Get-StageDefaultOpenCommand -RepoRoot $RepoRoot -ScriptPath $Script
    $releaseBlockers = @(New-StageBlockerItems `
            -RepoRoot $RepoRoot `
            -StageId $Id `
            -StageTitle $Title `
            -StageSchema $stageSchema `
            -SummaryJson $SummaryJson `
            -SummaryJsonDisplay $summaryJsonDisplay `
            -Items @(Get-JsonArray -Object $Summary -Name "release_blockers"))
    $actionItems = @(New-StageActionItems `
            -RepoRoot $RepoRoot `
            -StageId $Id `
            -StageTitle $Title `
            -StageSchema $stageSchema `
            -SummaryJson $SummaryJson `
            -SummaryJsonDisplay $summaryJsonDisplay `
            -DefaultOpenCommand $defaultOpenCommand `
            -Items @(Get-JsonArray -Object $Summary -Name "action_items"))
    $warnings = @(New-StageWarningItems `
            -RepoRoot $RepoRoot `
            -StageId $Id `
            -StageTitle $Title `
            -StageSchema $stageSchema `
            -SummaryJson $SummaryJson `
            -SummaryJsonDisplay $summaryJsonDisplay `
            -Items @(Get-JsonArray -Object $Summary -Name "warnings"))

    return [ordered]@{
        id = $Id
        title = $Title
        script = $Script
        input_json = @($InputJson)
        output_dir = $OutputDir
        output_dir_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $OutputDir
        summary_json = $SummaryJson
        summary_json_display = $summaryJsonDisplay
        report_markdown = $ReportMarkdown
        report_markdown_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $ReportMarkdown
        exit_code = $ExitCode
        status = $status
        release_blocker_count = Get-JsonInt -Object $Summary -Name "release_blocker_count"
        release_blockers = @($releaseBlockers)
        action_item_count = Get-JsonInt -Object $Summary -Name "action_item_count"
        action_items = @($actionItems)
        warning_count = Get-JsonInt -Object $Summary -Name "warning_count"
        warnings = @($warnings)
        missing_report_count = Get-JsonInt -Object $Summary -Name "missing_report_count"
        failed_report_count = Get-JsonInt -Object $Summary -Name "failed_report_count"
        source_failure_count = Get-JsonInt -Object $Summary -Name "source_failure_count"
        error = $ErrorMessage
    }
}

function Invoke-PipelineStage {
    param(
        [string]$RepoRoot,
        [string]$Id,
        [string]$Title,
        [string]$ScriptPath,
        [string]$OutputDir,
        [string[]]$InputJson,
        [string[]]$ExtraArguments = @()
    )

    $summaryPath = Join-Path $OutputDir "summary.json"
    $markdownPath = Join-Path $OutputDir ("{0}.md" -f $Id)
    Ensure-Directory -Path $OutputDir

    $arguments = @(
        "-InputJson"
        (@($InputJson) -join ",")
        "-OutputDir"
        $OutputDir
        "-SummaryJson"
        $summaryPath
        "-ReportMarkdown"
        $markdownPath
    ) + @($ExtraArguments)

    $exitCode = 0
    $errorMessage = ""
    try {
        Write-Step "Running $Id"
        $result = Invoke-ReportScript -ScriptPath $ScriptPath -Arguments $arguments
        $exitCode = [int]$result.ExitCode
        if ($exitCode -ne 0) {
            $errorMessage = "Stage exited with code $exitCode."
        }
    } catch {
        $exitCode = 1
        $errorMessage = $_.Exception.Message
    }

    $summary = Read-Summary -Path $summaryPath
    return New-StageEntry `
        -RepoRoot $RepoRoot `
        -Id $Id `
        -Title $Title `
        -Script $ScriptPath `
        -OutputDir $OutputDir `
        -SummaryJson $summaryPath `
        -ReportMarkdown $markdownPath `
        -InputJson $InputJson `
        -ExitCode $exitCode `
        -Summary $summary `
        -ErrorMessage $errorMessage
}

function Add-StageGovernanceMarkdown {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [object[]]$Items,
        [string]$Label
    )

    foreach ($item in @($Items)) {
        $Lines.Add("  - $Label ``$($item.id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` schema=``$($item.source_schema)`` source_report_display=``$($item.source_report_display)`` source_json_display=``$($item.source_json_display)``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace([string](Get-JsonProperty -Object $item -Name "message"))) {
            $Lines.Add("    - message: ``$($item.message)``") | Out-Null
        }
        if ($Label -eq "action" -and -not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
            $Lines.Add("    - open_command: ``$($item.open_command)``") | Out-Null
        }
    }
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Release Governance Pipeline") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Stages: ``$($Summary.completed_stage_count)`` completed / ``$($Summary.stage_count)`` total") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($Summary.warning_count)``") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("## Stages") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($stage in @($Summary.stages)) {
        $lines.Add("- ``$($stage.id)``: status=``$($stage.status)`` blockers=``$($stage.release_blocker_count)`` actions=``$($stage.action_item_count)`` warnings=``$($stage.warning_count)``") | Out-Null
        $lines.Add("  - summary: ``$($stage.summary_json_display)``") | Out-Null
        if (-not [string]::IsNullOrWhiteSpace([string]$stage.error)) {
            $lines.Add("  - error: ``$($stage.error)``") | Out-Null
        }
        Add-StageGovernanceMarkdown -Lines $lines -Items @($stage.release_blockers) -Label "blocker"
        Add-StageGovernanceMarkdown -Lines $lines -Items @($stage.warnings) -Label "warning"
        Add-StageGovernanceMarkdown -Lines $lines -Items @($stage.action_items) -Label "action"
    }
    return @($lines)
}

$repoRoot = Resolve-RepoRoot
$resolvedInputRoot = Resolve-RepoPath -RepoRoot $repoRoot -Path $InputRoot -AllowMissing
$resolvedOutputRoot = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputRoot -AllowMissing
Ensure-Directory -Path $resolvedOutputRoot

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputRoot "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputRoot "release_governance_pipeline.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$scriptsDir = Join-Path $repoRoot "scripts"
$outputGovernanceRoot = Join-Path $resolvedOutputRoot "governance"
$numberingOutputDir = Join-Path $outputGovernanceRoot "numbering-catalog-governance"
$tableOutputDir = Join-Path $outputGovernanceRoot "table-layout-delivery-governance"
$contentControlOutputDir = Join-Path $outputGovernanceRoot "content-control-data-binding-governance"
$projectOutputDir = Join-Path $outputGovernanceRoot "project-template-delivery-readiness"
$calibrationOutputDir = Join-Path $outputGovernanceRoot "schema-patch-confidence-calibration"
$handoffOutputDir = Join-Path $resolvedOutputRoot "release-governance-handoff"
$rollupOutputDir = Join-Path $resolvedOutputRoot "release-blocker-rollup"

$numberingInputs = @(
    Join-Path $resolvedInputRoot "document-skeleton-governance-rollup\summary.json"
    Join-Path $resolvedInputRoot "numbering-catalog-manifest-checks\summary.json"
)
$tableInputs = @(
    Join-Path $resolvedInputRoot "table-layout-delivery-rollup\summary.json"
)
$contentControlInputs = @(
    Join-Path $resolvedInputRoot "content-control-data-binding\inspect-content-controls.json"
    Join-Path $resolvedInputRoot "content-control-data-binding\sync-content-controls-from-custom-xml.json"
    Join-Path $resolvedInputRoot "content-control-data-binding-governance\summary.json"
)
$projectInputs = @(
    Join-Path $resolvedInputRoot "project-template-onboarding-governance\summary.json"
    Join-Path $resolvedInputRoot "project-template-schema-approval-history\history.json"
)
$calibrationInputs = @(
    Join-Path $resolvedInputRoot "project-template-smoke\summary.json"
    Join-Path $resolvedInputRoot "project-template-schema-approval-history\history.json"
) | Where-Object { Test-Path -LiteralPath $_ }

$stages = New-Object 'System.Collections.Generic.List[object]'
$stages.Add((Invoke-PipelineStage `
            -RepoRoot $repoRoot `
            -Id "numbering_catalog_governance" `
            -Title "Numbering Catalog Governance" `
            -ScriptPath (Join-Path $scriptsDir "build_numbering_catalog_governance_report.ps1") `
            -OutputDir $numberingOutputDir `
            -InputJson $numberingInputs)) | Out-Null
$stages.Add((Invoke-PipelineStage `
            -RepoRoot $repoRoot `
            -Id "table_layout_delivery_governance" `
            -Title "Table Layout Delivery Governance" `
            -ScriptPath (Join-Path $scriptsDir "build_table_layout_delivery_governance_report.ps1") `
            -OutputDir $tableOutputDir `
            -InputJson $tableInputs)) | Out-Null
$stages.Add((Invoke-PipelineStage `
            -RepoRoot $repoRoot `
            -Id "content_control_data_binding_governance" `
            -Title "Content Control Data Binding Governance" `
            -ScriptPath (Join-Path $scriptsDir "build_content_control_data_binding_governance_report.ps1") `
            -OutputDir $contentControlOutputDir `
            -InputJson $contentControlInputs)) | Out-Null
$stages.Add((Invoke-PipelineStage `
            -RepoRoot $repoRoot `
            -Id "project_template_delivery_readiness" `
            -Title "Project Template Delivery Readiness" `
            -ScriptPath (Join-Path $scriptsDir "build_project_template_delivery_readiness_report.ps1") `
            -OutputDir $projectOutputDir `
            -InputJson $projectInputs)) | Out-Null
$stages.Add((Invoke-PipelineStage `
            -RepoRoot $repoRoot `
            -Id "schema_patch_confidence_calibration" `
            -Title "Schema Patch Confidence Calibration" `
            -ScriptPath (Join-Path $scriptsDir "write_schema_patch_confidence_calibration_report.ps1") `
            -OutputDir $calibrationOutputDir `
            -InputJson $calibrationInputs `
            -ExtraArguments @("-InputRoot", $resolvedInputRoot))) | Out-Null

$handoffInputs = @(
    Join-Path $numberingOutputDir "summary.json"
    Join-Path $tableOutputDir "summary.json"
    Join-Path $contentControlOutputDir "summary.json"
    Join-Path $projectOutputDir "summary.json"
    Join-Path $calibrationOutputDir "summary.json"
)
$handoffExtraArguments = @(
    "-InputRoot"
    $outputGovernanceRoot
)
if ($IncludeHandoffRollup) {
    $handoffExtraArguments += "-IncludeReleaseBlockerRollup"
}
$stages.Add((Invoke-PipelineStage `
            -RepoRoot $repoRoot `
            -Id "release_governance_handoff" `
            -Title "Release Governance Handoff" `
            -ScriptPath (Join-Path $scriptsDir "build_release_governance_handoff_report.ps1") `
            -OutputDir $handoffOutputDir `
            -InputJson $handoffInputs `
            -ExtraArguments $handoffExtraArguments)) | Out-Null

$stages.Add((Invoke-PipelineStage `
            -RepoRoot $repoRoot `
            -Id "release_blocker_rollup" `
            -Title "Release Blocker Rollup" `
            -ScriptPath (Join-Path $scriptsDir "build_release_blocker_rollup_report.ps1") `
            -OutputDir $rollupOutputDir `
            -InputJson $handoffInputs)) | Out-Null

$stageItems = @($stages.ToArray())
$failedStageCount = @($stageItems | Where-Object {
        [string]$_.status -in @("failed", "missing_summary") -or [int]$_.exit_code -ne 0
    }).Count
$completedStageCount = @($stageItems | Where-Object {
        [string]$_.status -notin @("failed", "missing_summary") -and [int]$_.exit_code -eq 0
    }).Count
$missingReportCount = 0
$releaseBlockerCount = 0
$actionItemCount = 0
$warningCount = 0
foreach ($stage in $stageItems) {
    $missingReportCount += [int]$stage.missing_report_count
    $releaseBlockerCount += [int]$stage.release_blocker_count
    $actionItemCount += [int]$stage.action_item_count
    $warningCount += [int]$stage.warning_count
}
$finalRollup = @($stageItems | Where-Object { [string]$_.id -eq "release_blocker_rollup" } | Select-Object -First 1)
if ($finalRollup.Count -gt 0) {
    $releaseBlockerCount = [int]$finalRollup[0].release_blocker_count
    $actionItemCount = [int]$finalRollup[0].action_item_count
    $warningCount = [int]$finalRollup[0].warning_count
}

$status = if ($failedStageCount -gt 0) {
    "failed"
} elseif ($releaseBlockerCount -gt 0) {
    "blocked"
} elseif ($missingReportCount -gt 0 -or $warningCount -gt 0) {
    "needs_review"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = "featherdoc.release_governance_pipeline_report.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    input_root = $resolvedInputRoot
    input_root_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedInputRoot
    output_root = $resolvedOutputRoot
    output_root_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputRoot
    summary_json = $summaryPath
    summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    include_handoff_rollup = [bool]$IncludeHandoffRollup
    stage_count = $stageItems.Count
    completed_stage_count = $completedStageCount
    failed_stage_count = $failedStageCount
    missing_report_count = $missingReportCount
    release_blocker_count = $releaseBlockerCount
    action_item_count = $actionItemCount
    warning_count = $warningCount
    stages = $stageItems
    final_governance_reports = $handoffInputs
    release_governance_handoff_summary = Join-Path $handoffOutputDir "summary.json"
    release_blocker_rollup_summary = Join-Path $rollupOutputDir "summary.json"
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($FailOnStageFailure -and $failedStageCount -gt 0) { exit 1 }
if ($FailOnMissing -and $missingReportCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warningCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockerCount -gt 0) { exit 1 }
