<#
.SYNOPSIS
Helper functions for run_word_visual_release_gate.ps1.
#>

function Write-Step {
    param([string]$Message)
    Write-Host "[word-visual-release-gate] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $script:WordVisualReleaseGateScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Get-RelativePathCompat {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $resolvedBase = [System.IO.Path]::GetFullPath($BasePath)
    $resolvedTarget = [System.IO.Path]::GetFullPath($TargetPath)

    if ([System.IO.Path]::GetPathRoot($resolvedBase) -ne
        [System.IO.Path]::GetPathRoot($resolvedTarget)) {
        return $resolvedTarget
    }

    if ((Test-Path $resolvedBase -PathType Container) -and
        -not $resolvedBase.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedBase.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedBase += [System.IO.Path]::DirectorySeparatorChar
    }

    if ((Test-Path $resolvedTarget -PathType Container) -and
        -not $resolvedTarget.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedTarget.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedTarget += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = New-Object System.Uri($resolvedBase)
    $targetUri = New-Object System.Uri($resolvedTarget)
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)

    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace("/", "\")
}

function Convert-ToChildScriptPath {
    param(
        [string]$RepoRoot,
        [string]$TargetPath,
        [string]$Label
    )

    $relativePath = Get-RelativePathCompat -BasePath $RepoRoot -TargetPath $TargetPath
    if ([System.IO.Path]::IsPathRooted($relativePath)) {
        throw "$Label must stay on the same drive as the workspace: $TargetPath"
    }

    return $relativePath
}

function Invoke-ChildPowerShell {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }

    return @($commandOutput | ForEach-Object { $_.ToString() })
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path $Path)) {
        throw "Expected $Label was not found: $Path"
    }
}
function Get-OptionalPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Test-ReviewTaskPresent {
    param([AllowNull()]$Task)

    if ($null -eq $Task) {
        return $false
    }

    if ($Task -is [string]) {
        return -not [string]::IsNullOrWhiteSpace($Task)
    }

    return $true
}

function Add-OptionalSummaryValue {
    param(
        [System.Collections.IDictionary]$Target,
        [string]$Name,
        $Value
    )

    if ($null -ne $Value -and -not [string]::IsNullOrWhiteSpace([string]$Value)) {
        $Target[$Name] = $Value
    }
}

function Read-ReviewResult {
    param([string]$ReviewResultPath)

    Assert-PathExists -Path $ReviewResultPath -Label "smoke review result JSON"
    return Get-Content -Raw -LiteralPath $ReviewResultPath | ConvertFrom-Json
}

function Parse-SmokeRunOutput {
    param([string[]]$Lines)

    $result = [ordered]@{}
    foreach ($line in $Lines) {
        if ($line -match '^DOCX:\s*(.+)$') {
            $result.docx_path = $Matches[1].Trim()
        } elseif ($line -match '^PDF:\s*(.+)$') {
            $result.pdf_path = $Matches[1].Trim()
        } elseif ($line -match '^Evidence:\s*(.+)$') {
            $result.evidence_dir = $Matches[1].Trim()
        } elseif ($line -match '^Pages:\s*(.+)$') {
            $result.pages_dir = $Matches[1].Trim()
        } elseif ($line -match '^Contact sheet:\s*(.+)$') {
            $result.contact_sheet = $Matches[1].Trim()
        } elseif ($line -match '^Report:\s*(.+)$') {
            $result.report_dir = $Matches[1].Trim()
        } elseif ($line -match '^Summary:\s*(.+)$') {
            $result.summary_path = $Matches[1].Trim()
        } elseif ($line -match '^Checklist:\s*(.+)$') {
            $result.checklist_path = $Matches[1].Trim()
        } elseif ($line -match '^Review result:\s*(.+)$') {
            $result.review_result_path = $Matches[1].Trim()
        } elseif ($line -match '^Final review:\s*(.+)$') {
            $result.final_review_path = $Matches[1].Trim()
        } elseif ($line -match '^Repair:\s*(.+)$') {
            $result.repair_dir = $Matches[1].Trim()
        }
    }

    $required = @(
        @{ Name = "DOCX path"; Value = $result.docx_path },
        @{ Name = "PDF path"; Value = $result.pdf_path },
        @{ Name = "evidence directory"; Value = $result.evidence_dir },
        @{ Name = "pages directory"; Value = $result.pages_dir },
        @{ Name = "contact sheet"; Value = $result.contact_sheet },
        @{ Name = "report directory"; Value = $result.report_dir },
        @{ Name = "summary JSON"; Value = $result.summary_path },
        @{ Name = "review checklist"; Value = $result.checklist_path },
        @{ Name = "review result JSON"; Value = $result.review_result_path },
        @{ Name = "final review Markdown"; Value = $result.final_review_path },
        @{ Name = "repair directory"; Value = $result.repair_dir }
    )

    foreach ($item in $required) {
        Assert-PathExists -Path $item.Value -Label $item.Name
    }

    return $result
}

function Parse-PrepareTaskOutput {
    param([string[]]$Lines)

    $taskInfo = [ordered]@{}
    foreach ($line in $Lines) {
        if ($line -match '^Task id:\s*(.+)$') {
            $taskInfo.task_id = $Matches[1].Trim()
        } elseif ($line -match '^Source kind:\s*(.+)$') {
            $taskInfo.source_kind = $Matches[1].Trim()
        } elseif ($line -match '^Source:\s*(.+)$') {
            $taskInfo.source_path = $Matches[1].Trim()
        } elseif ($line -match '^Document:\s*(.+)$') {
            $taskInfo.document_path = $Matches[1].Trim()
        } elseif ($line -match '^Mode:\s*(.+)$') {
            $taskInfo.mode = $Matches[1].Trim()
        } elseif ($line -match '^Task directory:\s*(.+)$') {
            $taskInfo.task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Prompt:\s*(.+)$') {
            $taskInfo.prompt_path = $Matches[1].Trim()
        } elseif ($line -match '^Manifest:\s*(.+)$') {
            $taskInfo.manifest_path = $Matches[1].Trim()
        } elseif ($line -match '^Review result:\s*(.+)$') {
            $taskInfo.review_result_path = $Matches[1].Trim()
        } elseif ($line -match '^Final review:\s*(.+)$') {
            $taskInfo.final_review_path = $Matches[1].Trim()
        } elseif ($line -match '^Latest task pointer:\s*(.+)$') {
            $taskInfo.latest_task_pointer_path = $Matches[1].Trim()
        } elseif ($line -match '^Latest source-kind task pointer:\s*(.+)$') {
            $taskInfo.latest_source_kind_task_pointer_path = $Matches[1].Trim()
        }
    }

    $required = @(
        @{ Name = "task directory"; Value = $taskInfo.task_dir },
        @{ Name = "task prompt"; Value = $taskInfo.prompt_path },
        @{ Name = "task manifest"; Value = $taskInfo.manifest_path },
        @{ Name = "task review result"; Value = $taskInfo.review_result_path },
        @{ Name = "task final review"; Value = $taskInfo.final_review_path },
        @{ Name = "latest task pointer"; Value = $taskInfo.latest_task_pointer_path },
        @{ Name = "latest source-kind task pointer"; Value = $taskInfo.latest_source_kind_task_pointer_path }
    )

    foreach ($item in $required) {
        Assert-PathExists -Path $item.Value -Label $item.Name
    }

    return $taskInfo
}

function Get-TaskSummaryBlock {
    param(
        [string]$Label,
        $TaskInfo
    )

    if ($null -eq $TaskInfo) {
        return "- $Label review task: not requested"
    }

    return @(
        "- $Label task id: $($TaskInfo.task_id)"
        "- $Label task dir: $($TaskInfo.task_dir)"
        "- $Label prompt: $($TaskInfo.prompt_path)"
        "- $Label latest pointer: $($TaskInfo.latest_source_kind_task_pointer_path)"
    ) -join [Environment]::NewLine
}

function Get-ReviewTaskSummary {
    param($ReviewTasks)

    $summary = [ordered]@{
        total_count = 0
        standard_count = 0
        curated_count = 0
    }

    if ($null -eq $ReviewTasks) {
        return $summary
    }

    foreach ($name in @("document", "fixed_grid", "section_page_setup", "page_number_fields")) {
        $task = if ($ReviewTasks -is [System.Collections.IDictionary]) {
            if ($ReviewTasks.Contains($name)) {
                $ReviewTasks[$name]
            } else {
                $null
            }
        } else {
            Get-OptionalPropertyValue -Object $ReviewTasks -Name $name
        }

        if (Test-ReviewTaskPresent -Task $task) {
            $summary.standard_count += 1
        }
    }

    $curatedTasks = if ($ReviewTasks -is [System.Collections.IDictionary]) {
        if ($ReviewTasks.Contains("curated_visual_regressions")) {
            $ReviewTasks["curated_visual_regressions"]
        } else {
            @()
        }
    } else {
        Get-OptionalPropertyValue -Object $ReviewTasks -Name "curated_visual_regressions"
    }

    foreach ($task in @($curatedTasks)) {
        if (Test-ReviewTaskPresent -Task $task) {
            $summary.curated_count += 1
        }
    }

    $summary.total_count = $summary.standard_count + $summary.curated_count
    return $summary
}

function Get-FlowStatusLine {
    param(
        [string]$Label,
        $FlowInfo,
        [string]$PathField
    )

    if ($null -eq $FlowInfo) {
        return "- $Label flow: not requested"
    }

    if ($FlowInfo.status -eq "completed") {
        return "- $Label flow: completed ($($FlowInfo.$PathField))"
    }

    return "- $Label flow: $($FlowInfo.status)"
}

function Resolve-AggregateContactSheetPath {
    param(
        [string]$AggregateEvidenceDir,
        [string]$Label
    )

    $candidate = Join-Path $AggregateEvidenceDir "before_after_contact_sheet.png"
    if (Test-Path $candidate) {
        return $candidate
    }

    throw "Expected $Label aggregate-evidence\before_after_contact_sheet.png was not found under: $AggregateEvidenceDir"
}

function New-VisualRegressionRefreshCommand {
    param(
        [string]$ScriptPath,
        [string]$BuildDir,
        [string]$OutputDir,
        [int]$Dpi,
        [bool]$SkipBuild
    )

    $normalizedOutputDir = $OutputDir.TrimEnd('\', '/')

    $parts = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$ScriptPath`""
        "-BuildDir"
        "`"$BuildDir`""
        "-OutputDir"
        "`"$normalizedOutputDir`""
        "-Dpi"
        $Dpi.ToString()
    )

    if ($SkipBuild) {
        $parts += "-SkipBuild"
    }

    return ($parts -join " ")
}

function Invoke-VisualRegressionBundleFlow {
    param(
        [string]$Label,
        [string]$Id,
        [string]$ScriptPath,
        [string]$BuildDir,
        [string]$OutputDirForChild,
        [string]$ResolvedOutputDir,
        [int]$Dpi,
        [bool]$SkipBuild,
        [bool]$KeepWordOpen,
        [bool]$VisibleWord
    )

    Write-Step "Running $Label visual regression flow"

    $arguments = @(
        "-BuildDir"
        $BuildDir
        "-OutputDir"
        $OutputDirForChild
        "-Dpi"
        $Dpi.ToString()
    )
    if ($SkipBuild) {
        $arguments += "-SkipBuild"
    }
    if ($KeepWordOpen) {
        $arguments += "-KeepWordOpen"
    }
    if ($VisibleWord) {
        $arguments += "-VisibleWord"
    }

    Invoke-ChildPowerShell -ScriptPath $ScriptPath `
        -Arguments $arguments `
        -FailureMessage "$Label visual regression gate step failed." | Out-Null

    $summaryPath = Join-Path $ResolvedOutputDir "summary.json"
    $aggregateEvidenceDir = Join-Path $ResolvedOutputDir "aggregate-evidence"
    Assert-PathExists -Path $summaryPath -Label "$Label summary JSON"
    Assert-PathExists -Path $aggregateEvidenceDir -Label "$Label aggregate evidence directory"

    $aggregateContactSheetPath = Resolve-AggregateContactSheetPath `
        -AggregateEvidenceDir $aggregateEvidenceDir `
        -Label $Label

    return [ordered]@{
        id = $Id
        label = $Label
        status = "completed"
        output_dir = $ResolvedOutputDir
        summary_json = $summaryPath
        aggregate_evidence_dir = $aggregateEvidenceDir
        aggregate_contact_sheet = $aggregateContactSheetPath
    }
}
