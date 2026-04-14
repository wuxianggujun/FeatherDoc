<#
.SYNOPSIS
Synchronizes the newest screenshot-backed visual verdicts with minimal input.

.DESCRIPTION
Resolves the newest document, fixed-grid, and section page setup review tasks
plus page number fields review tasks from the latest-task pointer files,
infers the matching Word visual gate output directory, tries to find the
corresponding release-candidate summary, and then delegates to
sync_visual_review_verdict.ps1.

.PARAMETER TaskOutputRoot
Optional explicit task root containing latest_task.json plus latest source-kind
task pointers. When omitted, the script auto-detects the newest task root
under output/word-visual-smoke.

.PARAMETER OutputSearchRoot
Root directory searched for a matching release-candidate report/summary.json
when ReleaseCandidateSummaryJson is not provided explicitly.

.PARAMETER ReleaseCandidateSummaryJson
Optional explicit release-candidate report/summary.json path.

.PARAMETER SkipReleaseBundle
Skips -RefreshReleaseBundle even when a release-candidate summary is found.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1
#>
param(
    [string]$TaskOutputRoot = "",
    [string]$OutputSearchRoot = "output",
    [string]$ReleaseCandidateSummaryJson = "",
    [switch]$SkipReleaseBundle
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[sync-latest-visual-review-verdict] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
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

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        throw "Expected $Label was not found: $Path"
    }
}

function Get-OptionalPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-OptionalPropertyObject {
    param(
        [AllowNull()]$Object,
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

function Set-PropertyValue {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name,
        $Value
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        Add-Member -InputObject $Object -NotePropertyName $Name -NotePropertyValue $Value
    } else {
        $property.Value = $Value
    }
}

function Get-OrCreateChildObject {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $existing = Get-OptionalPropertyObject -Object $Object -Name $Name
    if ($null -ne $existing) {
        return $existing
    }

    $created = [pscustomobject]@{}
    Set-PropertyValue -Object $Object -Name $Name -Value $created
    return $created
}

function Read-JsonFile {
    param([string]$Path)

    Assert-PathExists -Path $Path -Label "JSON file"
    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Convert-LatestPointerToTaskInfo {
    param($Pointer)

    $source = Get-OptionalPropertyObject -Object $Pointer -Name "source"
    $document = Get-OptionalPropertyObject -Object $Pointer -Name "document"
    $pointerFiles = Get-OptionalPropertyObject -Object $Pointer -Name "pointer_files"

    return [pscustomobject]@{
        task_id = Get-OptionalPropertyValue -Object $Pointer -Name "task_id"
        source_kind = Get-OptionalPropertyValue -Object $source -Name "kind"
        source_path = Get-OptionalPropertyValue -Object $source -Name "path"
        document_path = Get-OptionalPropertyValue -Object $document -Name "path"
        mode = Get-OptionalPropertyValue -Object $Pointer -Name "mode"
        task_dir = Get-OptionalPropertyValue -Object $Pointer -Name "task_dir"
        prompt_path = Get-OptionalPropertyValue -Object $Pointer -Name "prompt_path"
        manifest_path = Get-OptionalPropertyValue -Object $Pointer -Name "manifest_path"
        evidence_dir = Get-OptionalPropertyValue -Object $Pointer -Name "evidence_dir"
        report_dir = Get-OptionalPropertyValue -Object $Pointer -Name "report_dir"
        repair_dir = Get-OptionalPropertyValue -Object $Pointer -Name "repair_dir"
        review_result_path = Get-OptionalPropertyValue -Object $Pointer -Name "review_result_path"
        final_review_path = Get-OptionalPropertyValue -Object $Pointer -Name "final_review_path"
        latest_task_pointer_path = Get-OptionalPropertyValue -Object $pointerFiles -Name "generic"
        latest_source_kind_task_pointer_path = Get-OptionalPropertyValue -Object $pointerFiles -Name "source_kind"
    }
}

function Get-ReviewTaskKeyFromSourceKind {
    param([string]$SourceKind)

    switch ($SourceKind) {
        "document" { return "document" }
        "fixed-grid-regression-bundle" { return "fixed_grid" }
        "section-page-setup-regression-bundle" { return "section_page_setup" }
        "page-number-fields-regression-bundle" { return "page_number_fields" }
        default { throw "Unsupported source kind for review-task refresh: $SourceKind" }
    }
}

function Get-GateFlowKeyFromSourceKind {
    param([string]$SourceKind)

    switch ($SourceKind) {
        "document" { return "smoke" }
        "fixed-grid-regression-bundle" { return "fixed_grid" }
        "section-page-setup-regression-bundle" { return "section_page_setup" }
        "page-number-fields-regression-bundle" { return "page_number_fields" }
        default { throw "Unsupported source kind for gate-flow refresh: $SourceKind" }
    }
}

function Update-GateSummaryReviewTasksFromPointers {
    param(
        [string]$GateSummaryPath,
        [hashtable]$LatestPointers
    )

    if ($LatestPointers.Count -eq 0) {
        return
    }

    $gateSummary = Read-JsonFile -Path $GateSummaryPath
    $reviewTasks = Get-OrCreateChildObject -Object $gateSummary -Name "review_tasks"

    foreach ($sourceKind in $LatestPointers.Keys) {
        $taskInfo = Convert-LatestPointerToTaskInfo -Pointer $LatestPointers[$sourceKind]
        $reviewTaskKey = Get-ReviewTaskKeyFromSourceKind -SourceKind $sourceKind
        $flowKey = Get-GateFlowKeyFromSourceKind -SourceKind $sourceKind
        $flowInfo = Get-OrCreateChildObject -Object $gateSummary -Name $flowKey

        Set-PropertyValue -Object $reviewTasks -Name $reviewTaskKey -Value $taskInfo
        Set-PropertyValue -Object $flowInfo -Name "task" -Value $taskInfo

        Write-Step "Refreshed $reviewTaskKey task metadata in gate summary: $($taskInfo.task_dir)"
    }

    ($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $GateSummaryPath -Encoding UTF8
}

function Update-ReleaseSummaryTaskDirsFromPointers {
    param(
        [string]$ReleaseSummaryPath,
        [hashtable]$LatestPointers
    )

    if ([string]::IsNullOrWhiteSpace($ReleaseSummaryPath) -or $LatestPointers.Count -eq 0) {
        return
    }

    $summary = Read-JsonFile -Path $ReleaseSummaryPath
    $steps = Get-OrCreateChildObject -Object $summary -Name "steps"
    $visualGate = Get-OrCreateChildObject -Object $steps -Name "visual_gate"

    foreach ($sourceKind in $LatestPointers.Keys) {
        $pointer = $LatestPointers[$sourceKind]
        $taskDir = Get-OptionalPropertyValue -Object $pointer -Name "task_dir"

        switch ($sourceKind) {
            "document" {
                Set-PropertyValue -Object $visualGate -Name "document_task_dir" -Value $taskDir
            }
            "fixed-grid-regression-bundle" {
                Set-PropertyValue -Object $visualGate -Name "fixed_grid_task_dir" -Value $taskDir
            }
            "section-page-setup-regression-bundle" {
                Set-PropertyValue -Object $visualGate -Name "section_page_setup_task_dir" -Value $taskDir
            }
            "page-number-fields-regression-bundle" {
                Set-PropertyValue -Object $visualGate -Name "page_number_fields_task_dir" -Value $taskDir
            }
        }

        Write-Step "Refreshed release summary task dir for ${sourceKind}: $taskDir"
    }

    ($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $ReleaseSummaryPath -Encoding UTF8
}

function Update-AuditReportMetadata {
    param(
        [string]$GateSummaryPath,
        [string]$ReleaseSummaryPath,
        [string]$ReportPath
    )

    if (-not [string]::IsNullOrWhiteSpace($GateSummaryPath)) {
        $gateSummary = Read-JsonFile -Path $GateSummaryPath
        Set-PropertyValue -Object $gateSummary -Name "superseded_review_tasks_report" -Value $ReportPath
        ($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $GateSummaryPath -Encoding UTF8
    }

    if (-not [string]::IsNullOrWhiteSpace($ReleaseSummaryPath)) {
        $summary = Read-JsonFile -Path $ReleaseSummaryPath
        $steps = Get-OrCreateChildObject -Object $summary -Name "steps"
        $visualGate = Get-OrCreateChildObject -Object $steps -Name "visual_gate"
        Set-PropertyValue -Object $summary -Name "superseded_review_tasks_report" -Value $ReportPath
        Set-PropertyValue -Object $visualGate -Name "superseded_review_tasks_report" -Value $ReportPath
        ($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $ReleaseSummaryPath -Encoding UTF8
    }
}

function Find-LatestTaskRoot {
    param([string]$SearchRoot)

    if (-not (Test-Path -LiteralPath $SearchRoot)) {
        throw "Task-root search base does not exist: $SearchRoot"
    }

    $pointerFiles = Get-ChildItem -Path $SearchRoot -Recurse -File |
        Where-Object {
            $_.Name -in @(
                "latest_document_task.json",
                "latest_fixed-grid-regression-bundle_task.json",
                "latest_section-page-setup-regression-bundle_task.json",
                "latest_page-number-fields-regression-bundle_task.json"
            )
        }

    if (-not $pointerFiles) {
        throw "Could not auto-detect a task root under $SearchRoot."
    }

    $group = $pointerFiles |
        Group-Object DirectoryName |
        Sort-Object {
            ($_.Group | Measure-Object -Property LastWriteTimeUtc -Maximum).Maximum
        } -Descending |
        Select-Object -First 1

    return $group.Name
}

function Get-LatestPointerPath {
    param(
        [string]$TaskRoot,
        [string]$SourceKind
    )

    switch ($SourceKind) {
        "document" {
            return Join-Path $TaskRoot "latest_document_task.json"
        }
        "fixed-grid-regression-bundle" {
            return Join-Path $TaskRoot "latest_fixed-grid-regression-bundle_task.json"
        }
        "section-page-setup-regression-bundle" {
            return Join-Path $TaskRoot "latest_section-page-setup-regression-bundle_task.json"
        }
        "page-number-fields-regression-bundle" {
            return Join-Path $TaskRoot "latest_page-number-fields-regression-bundle_task.json"
        }
        default {
            throw "Unsupported source kind: $SourceKind"
        }
    }
}

function Resolve-GateOutputDirFromPointer {
    param($Pointer)

    $source = Get-OptionalPropertyObject -Object $Pointer -Name "source"
    $sourceKind = Get-OptionalPropertyValue -Object $source -Name "kind"
    $sourcePath = Get-OptionalPropertyValue -Object $source -Name "path"
    Assert-PathExists -Path $sourcePath -Label "$sourceKind source path"

    switch ($sourceKind) {
        "document" {
            return Split-Path -Parent (Split-Path -Parent $sourcePath)
        }
        "fixed-grid-regression-bundle" {
            return Split-Path -Parent $sourcePath
        }
        "section-page-setup-regression-bundle" {
            return Split-Path -Parent $sourcePath
        }
        "page-number-fields-regression-bundle" {
            return Split-Path -Parent $sourcePath
        }
        default {
            throw "Unsupported latest-task source kind for gate inference: $sourceKind"
        }
    }
}

function Find-ReleaseCandidateSummary {
    param(
        [string]$SearchRoot,
        [string]$GateSummaryPath
    )

    if (-not (Test-Path -LiteralPath $SearchRoot)) {
        return ""
    }

    $candidates = Get-ChildItem -Path $SearchRoot -Recurse -File -Filter "summary.json" |
        Sort-Object LastWriteTimeUtc -Descending

    foreach ($candidate in $candidates) {
        try {
            $summary = Get-Content -Raw -LiteralPath $candidate.FullName | ConvertFrom-Json
        } catch {
            continue
        }

        $steps = Get-OptionalPropertyObject -Object $summary -Name "steps"
        $visualGate = Get-OptionalPropertyObject -Object $steps -Name "visual_gate"
        $summaryJsonPath = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"

        if (-not [string]::IsNullOrWhiteSpace($summaryJsonPath) -and
            [System.StringComparer]::OrdinalIgnoreCase.Equals(
                [System.IO.Path]::GetFullPath($summaryJsonPath),
                [System.IO.Path]::GetFullPath($GateSummaryPath))) {
            return $candidate.FullName
        }
    }

    return ""
}

function Invoke-ChildPowerShell {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedTaskOutputRoot = if ([string]::IsNullOrWhiteSpace($TaskOutputRoot)) {
    $autoSearchRoot = Join-Path $repoRoot "output\word-visual-smoke"
    $autoDetectedTaskRoot = Find-LatestTaskRoot -SearchRoot $autoSearchRoot
    Write-Step "Auto-detected latest task root: $autoDetectedTaskRoot"
    $autoDetectedTaskRoot
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TaskOutputRoot
}
Assert-PathExists -Path $resolvedTaskOutputRoot -Label "task output root"

$documentPointerPath = Get-LatestPointerPath -TaskRoot $resolvedTaskOutputRoot -SourceKind "document"
$fixedGridPointerPath = Get-LatestPointerPath -TaskRoot $resolvedTaskOutputRoot -SourceKind "fixed-grid-regression-bundle"
$sectionPageSetupPointerPath = Get-LatestPointerPath -TaskRoot $resolvedTaskOutputRoot -SourceKind "section-page-setup-regression-bundle"
$pageNumberFieldsPointerPath = Get-LatestPointerPath -TaskRoot $resolvedTaskOutputRoot -SourceKind "page-number-fields-regression-bundle"

$gateOutputCandidates = @()
$latestPointers = @{}
if (Test-Path -LiteralPath $documentPointerPath) {
    $documentPointer = Read-JsonFile -Path $documentPointerPath
    $latestPointers["document"] = $documentPointer
    $gateOutputCandidates += Resolve-GateOutputDirFromPointer -Pointer $documentPointer
    Write-Step "Resolved latest document task: $(Get-OptionalPropertyValue -Object $documentPointer -Name 'task_dir')"
}
if (Test-Path -LiteralPath $fixedGridPointerPath) {
    $fixedGridPointer = Read-JsonFile -Path $fixedGridPointerPath
    $latestPointers["fixed-grid-regression-bundle"] = $fixedGridPointer
    $gateOutputCandidates += Resolve-GateOutputDirFromPointer -Pointer $fixedGridPointer
    Write-Step "Resolved latest fixed-grid task: $(Get-OptionalPropertyValue -Object $fixedGridPointer -Name 'task_dir')"
}
if (Test-Path -LiteralPath $sectionPageSetupPointerPath) {
    $sectionPageSetupPointer = Read-JsonFile -Path $sectionPageSetupPointerPath
    $latestPointers["section-page-setup-regression-bundle"] = $sectionPageSetupPointer
    $gateOutputCandidates += Resolve-GateOutputDirFromPointer -Pointer $sectionPageSetupPointer
    Write-Step "Resolved latest section page setup task: $(Get-OptionalPropertyValue -Object $sectionPageSetupPointer -Name 'task_dir')"
}
if (Test-Path -LiteralPath $pageNumberFieldsPointerPath) {
    $pageNumberFieldsPointer = Read-JsonFile -Path $pageNumberFieldsPointerPath
    $latestPointers["page-number-fields-regression-bundle"] = $pageNumberFieldsPointer
    $gateOutputCandidates += Resolve-GateOutputDirFromPointer -Pointer $pageNumberFieldsPointer
    Write-Step "Resolved latest page number fields task: $(Get-OptionalPropertyValue -Object $pageNumberFieldsPointer -Name 'task_dir')"
}

$gateOutputCandidates = @($gateOutputCandidates | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
if ($gateOutputCandidates.Count -eq 0) {
    throw "No latest document/fixed-grid/section-page-setup/page-number-fields task pointers were found under $resolvedTaskOutputRoot."
}
if ($gateOutputCandidates.Count -gt 1) {
    throw "Latest task pointers do not agree on one gate output directory: $($gateOutputCandidates -join ', ')"
}

$resolvedGateOutputDir = $gateOutputCandidates[0]
$resolvedGateSummaryPath = Join-Path $resolvedGateOutputDir "report\gate_summary.json"
Assert-PathExists -Path $resolvedGateSummaryPath -Label "inferred gate summary"
Write-Step "Resolved gate summary: $resolvedGateSummaryPath"

$resolvedReleaseSummaryPath = ""
if (-not [string]::IsNullOrWhiteSpace($ReleaseCandidateSummaryJson)) {
    $resolvedReleaseSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReleaseCandidateSummaryJson
    Assert-PathExists -Path $resolvedReleaseSummaryPath -Label "explicit release-candidate summary"
    Write-Step "Using explicit release summary: $resolvedReleaseSummaryPath"
} else {
    $resolvedOutputSearchRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputSearchRoot
    $resolvedReleaseSummaryPath = Find-ReleaseCandidateSummary `
        -SearchRoot $resolvedOutputSearchRoot `
        -GateSummaryPath $resolvedGateSummaryPath
    if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
        Write-Step "Auto-detected release summary: $resolvedReleaseSummaryPath"
    } else {
        Write-Step "No matching release summary was found under $resolvedOutputSearchRoot"
    }
}

Update-GateSummaryReviewTasksFromPointers `
    -GateSummaryPath $resolvedGateSummaryPath `
    -LatestPointers $latestPointers
Update-ReleaseSummaryTaskDirsFromPointers `
    -ReleaseSummaryPath $resolvedReleaseSummaryPath `
    -LatestPointers $latestPointers

$auditScript = Join-Path $repoRoot "scripts\find_superseded_review_tasks.ps1"
$supersededReviewTasksReportPath = Join-Path $resolvedTaskOutputRoot "superseded_review_tasks.json"
Invoke-ChildPowerShell -ScriptPath $auditScript `
    -Arguments @(
        "-TaskOutputRoot"
        $resolvedTaskOutputRoot
        "-ReportPath"
        $supersededReviewTasksReportPath
    ) `
    -FailureMessage "Failed to audit superseded review tasks."
Write-Step "Updated superseded review-task report: $supersededReviewTasksReportPath"
Update-AuditReportMetadata `
    -GateSummaryPath $resolvedGateSummaryPath `
    -ReleaseSummaryPath $resolvedReleaseSummaryPath `
    -ReportPath $supersededReviewTasksReportPath

$delegateScript = Join-Path $repoRoot "scripts\sync_visual_review_verdict.ps1"
$delegateArgs = @(
    "-GateSummaryJson"
    $resolvedGateSummaryPath
)
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    $delegateArgs += @(
        "-ReleaseCandidateSummaryJson"
        $resolvedReleaseSummaryPath
    )
    if (-not $SkipReleaseBundle) {
        $delegateArgs += "-RefreshReleaseBundle"
    }
}

Invoke-ChildPowerShell -ScriptPath $delegateScript `
    -Arguments $delegateArgs `
    -FailureMessage "Failed to synchronize the latest screenshot-backed visual verdict."

Write-Host "Task output root: $resolvedTaskOutputRoot"
Write-Host "Gate summary: $resolvedGateSummaryPath"
if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    Write-Host "Release summary: $resolvedReleaseSummaryPath"
}
