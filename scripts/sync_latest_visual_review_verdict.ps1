<#
.SYNOPSIS
Synchronizes the newest screenshot-backed visual verdicts with minimal input.

.DESCRIPTION
Resolves the newest document, fixed-grid, and section page setup review tasks
plus page number fields review tasks and any curated visual-regression bundle
tasks from the latest-task pointer files, infers the matching Word visual gate
output directory, tries to find the corresponding release-candidate summary,
and then delegates to sync_visual_review_verdict.ps1.

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

.PARAMETER SkipSupersededReviewTaskAudit
Skips the superseded review-task audit when a focused caller only needs verdict sync.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1
#>
param(
    [string]$TaskOutputRoot = "",
    [string]$OutputSearchRoot = "output",
    [string]$ReleaseCandidateSummaryJson = "",
    [switch]$SkipReleaseBundle,
    [switch]$SkipSupersededReviewTaskAudit
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
    try {
        return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    } catch {
        throw ("Failed to parse JSON file: {0}. {1}" -f $Path, $_.Exception.Message)
    }
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

function Update-GateSummaryReviewTasksFromPointers {
    param(
        [string]$GateSummaryPath,
        [object[]]$LatestPointerDescriptors
    )

    if ($LatestPointerDescriptors.Count -eq 0) {
        return
    }

    $gateSummary = Read-JsonFile -Path $GateSummaryPath
    $reviewTasks = Get-OrCreateChildObject -Object $gateSummary -Name "review_tasks"
    $curatedReviewTasks = @()
    $curatedReviewTasksProperty = Get-OptionalPropertyObject -Object $reviewTasks -Name "curated_visual_regressions"
    if ($null -ne $curatedReviewTasksProperty) {
        $curatedReviewTasks = @($curatedReviewTasksProperty)
    }

    foreach ($descriptor in $LatestPointerDescriptors) {
        $sourceKind = $descriptor.source_kind
        $taskInfo = Convert-LatestPointerToTaskInfo -Pointer $descriptor.pointer

        switch ($sourceKind) {
            "document" {
                $flowInfo = Get-OrCreateChildObject -Object $gateSummary -Name "smoke"
                Set-PropertyValue -Object $reviewTasks -Name "document" -Value $taskInfo
                Set-PropertyValue -Object $flowInfo -Name "task" -Value $taskInfo
                Write-Step "Refreshed document task metadata in gate summary: $($taskInfo.task_dir)"
            }
            "fixed-grid-regression-bundle" {
                $flowInfo = Get-OrCreateChildObject -Object $gateSummary -Name "fixed_grid"
                Set-PropertyValue -Object $reviewTasks -Name "fixed_grid" -Value $taskInfo
                Set-PropertyValue -Object $flowInfo -Name "task" -Value $taskInfo
                Write-Step "Refreshed fixed_grid task metadata in gate summary: $($taskInfo.task_dir)"
            }
            "section-page-setup-regression-bundle" {
                $flowInfo = Get-OrCreateChildObject -Object $gateSummary -Name "section_page_setup"
                Set-PropertyValue -Object $reviewTasks -Name "section_page_setup" -Value $taskInfo
                Set-PropertyValue -Object $flowInfo -Name "task" -Value $taskInfo
                Write-Step "Refreshed section_page_setup task metadata in gate summary: $($taskInfo.task_dir)"
            }
            "page-number-fields-regression-bundle" {
                $flowInfo = Get-OrCreateChildObject -Object $gateSummary -Name "page_number_fields"
                Set-PropertyValue -Object $reviewTasks -Name "page_number_fields" -Value $taskInfo
                Set-PropertyValue -Object $flowInfo -Name "task" -Value $taskInfo
                Write-Step "Refreshed page_number_fields task metadata in gate summary: $($taskInfo.task_dir)"
            }
            "visual-regression-bundle" {
                $bundleId = [string]$descriptor.bundle_id
                $curatedFlowsProperty = Get-OptionalPropertyObject -Object $gateSummary -Name "curated_visual_regressions"
                $curatedFlows = if ($null -eq $curatedFlowsProperty) { @() } else { @($curatedFlowsProperty) }
                $flowInfo = $curatedFlows | Where-Object { (Get-OptionalPropertyValue -Object $_ -Name "id") -eq $bundleId } | Select-Object -First 1
                $bundleLabel = if ($null -ne $flowInfo) {
                    Get-OptionalPropertyValue -Object $flowInfo -Name "label"
                } else {
                    $bundleId
                }
                if ($null -ne $flowInfo) {
                    Set-PropertyValue -Object $flowInfo -Name "task" -Value $taskInfo
                }

                $existingBundleTask = $curatedReviewTasks |
                    Where-Object { (Get-OptionalPropertyValue -Object $_ -Name "id") -eq $bundleId } |
                    Select-Object -First 1
                if ($null -eq $existingBundleTask) {
                    $existingBundleTask = [pscustomobject]@{
                        id = $bundleId
                        label = $bundleLabel
                        task = $taskInfo
                    }
                    $curatedReviewTasks += $existingBundleTask
                } else {
                    Set-PropertyValue -Object $existingBundleTask -Name "id" -Value $bundleId
                    Set-PropertyValue -Object $existingBundleTask -Name "label" -Value $bundleLabel
                    Set-PropertyValue -Object $existingBundleTask -Name "task" -Value $taskInfo
                }

                Write-Step "Refreshed curated visual task metadata for ${bundleId}: $($taskInfo.task_dir)"
            }
            default {
                throw "Unsupported source kind for review-task refresh: $sourceKind"
            }
        }
    }

    if ($curatedReviewTasks.Count -gt 0) {
        Set-PropertyValue -Object $reviewTasks -Name "curated_visual_regressions" -Value @($curatedReviewTasks)
    }

    ($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $GateSummaryPath -Encoding UTF8
}

function Update-ReleaseSummaryTaskDirsFromPointers {
    param(
        [string]$ReleaseSummaryPath,
        [object[]]$LatestPointerDescriptors
    )

    if ([string]::IsNullOrWhiteSpace($ReleaseSummaryPath) -or $LatestPointerDescriptors.Count -eq 0) {
        return
    }

    $summary = Read-JsonFile -Path $ReleaseSummaryPath
    $steps = Get-OrCreateChildObject -Object $summary -Name "steps"
    $visualGate = Get-OrCreateChildObject -Object $steps -Name "visual_gate"
    $curatedReleaseTasks = @()
    $curatedReleaseTasksProperty = Get-OptionalPropertyObject -Object $visualGate -Name "curated_visual_regressions"
    if ($null -ne $curatedReleaseTasksProperty) {
        $curatedReleaseTasks = @($curatedReleaseTasksProperty)
    }

    foreach ($descriptor in $LatestPointerDescriptors) {
        $sourceKind = $descriptor.source_kind
        $pointer = $descriptor.pointer
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
            "visual-regression-bundle" {
                $source = Get-OptionalPropertyObject -Object $pointer -Name "source"
                $sourcePath = Get-OptionalPropertyValue -Object $source -Name "path"
                $bundleId = [string]$descriptor.bundle_id
                $existingBundleTask = $curatedReleaseTasks |
                    Where-Object { (Get-OptionalPropertyValue -Object $_ -Name "id") -eq $bundleId } |
                    Select-Object -First 1

                if ($null -eq $existingBundleTask) {
                    $existingBundleTask = [pscustomobject]@{
                        id = $bundleId
                        source_kind = $sourceKind
                        source_path = $sourcePath
                        task_dir = $taskDir
                    }
                    $curatedReleaseTasks += $existingBundleTask
                } else {
                    Set-PropertyValue -Object $existingBundleTask -Name "id" -Value $bundleId
                    Set-PropertyValue -Object $existingBundleTask -Name "source_kind" -Value $sourceKind
                    Set-PropertyValue -Object $existingBundleTask -Name "source_path" -Value $sourcePath
                    Set-PropertyValue -Object $existingBundleTask -Name "task_dir" -Value $taskDir
                }
            }
            default {
                throw "Unsupported source kind for release-summary refresh: $sourceKind"
            }
        }

        Write-Step "Refreshed release summary task dir for ${sourceKind}: $taskDir"
    }

    if ($curatedReleaseTasks.Count -gt 0) {
        Set-PropertyValue -Object $visualGate -Name "curated_visual_regressions" -Value @($curatedReleaseTasks)
    }

    ($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $ReleaseSummaryPath -Encoding UTF8
}


function Update-ReleaseSummaryDiscoveryMetadata {
    param(
        [string]$GateSummaryPath,
        [string]$ReleaseSummaryPath,
        [string]$Mode,
        [string]$Reason,
        [string]$SelectedSummaryPath,
        [string]$OutputSearchRoot,
        [bool]$SkipReleaseBundle
    )

    $gateSummary = Read-JsonFile -Path $GateSummaryPath
    $releaseBundleRefreshRequested = (-not [string]::IsNullOrWhiteSpace($SelectedSummaryPath)) -and (-not $SkipReleaseBundle)
    $metadata = [pscustomobject]@{
        mode = $Mode
        reason = $Reason
        selected_summary_path = $SelectedSummaryPath
        output_search_root = $OutputSearchRoot
        release_bundle_refresh_requested = $releaseBundleRefreshRequested
    }

    Set-PropertyValue -Object $gateSummary -Name "selected_release_summary_path" -Value $SelectedSummaryPath
    Set-PropertyValue -Object $gateSummary -Name "release_summary_discovery" -Value $metadata
    ($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $GateSummaryPath -Encoding UTF8

    if (-not [string]::IsNullOrWhiteSpace($ReleaseSummaryPath)) {
        $summary = Read-JsonFile -Path $ReleaseSummaryPath
        Set-PropertyValue -Object $summary -Name "selected_release_summary_path" -Value $SelectedSummaryPath
        Set-PropertyValue -Object $summary -Name "release_summary_discovery" -Value $metadata
        ($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $ReleaseSummaryPath -Encoding UTF8
    }
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
            $_.Name -like "latest_*_task.json" -and $_.Name -ne "latest_task.json"
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

function Get-LatestPointerDescriptorsFromTaskRoot {
    param([string]$TaskRoot)

    $pointerFiles = Get-ChildItem -Path $TaskRoot -File |
        Where-Object {
            $_.Name -like "latest_*_task.json" -and $_.Name -ne "latest_task.json"
        } |
        Sort-Object Name

    $descriptors = @()
    foreach ($pointerFile in $pointerFiles) {
        $sourceKind = ""
        $bundleId = ""

        switch -Regex ($pointerFile.Name) {
            '^latest_document_task\.json$' {
                $sourceKind = "document"
                break
            }
            '^latest_fixed-grid-regression-bundle_task\.json$' {
                $sourceKind = "fixed-grid-regression-bundle"
                break
            }
            '^latest_section-page-setup-regression-bundle_task\.json$' {
                $sourceKind = "section-page-setup-regression-bundle"
                break
            }
            '^latest_page-number-fields-regression-bundle_task\.json$' {
                $sourceKind = "page-number-fields-regression-bundle"
                break
            }
            '^latest_(.+)-visual-regression-bundle_task\.json$' {
                $sourceKind = "visual-regression-bundle"
                $bundleId = $Matches[1]
                break
            }
            default {
                continue
            }
        }

        $descriptors += [pscustomobject]@{
            source_kind = $sourceKind
            bundle_id = $bundleId
            pointer_path = $pointerFile.FullName
            pointer = Read-JsonFile -Path $pointerFile.FullName
        }
    }

    return @($descriptors)
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
        "visual-regression-bundle" {
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
        Sort-Object `
            @{ Expression = { $_.LastWriteTimeUtc }; Descending = $true },
            @{ Expression = { $_.FullName } }

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

$latestPointerDescriptors = Get-LatestPointerDescriptorsFromTaskRoot -TaskRoot $resolvedTaskOutputRoot
$gateOutputCandidates = @()
$supportedSourceKinds = @(
    "document",
    "fixed-grid-regression-bundle",
    "section-page-setup-regression-bundle",
    "page-number-fields-regression-bundle",
    "visual-regression-bundle"
)
$latestPointerDescriptors = @($latestPointerDescriptors | Where-Object { $_.source_kind -in $supportedSourceKinds })
foreach ($descriptor in $latestPointerDescriptors) {
    $pointer = $descriptor.pointer
    $taskDir = Get-OptionalPropertyValue -Object $pointer -Name "task_dir"
    $gateOutputCandidates += Resolve-GateOutputDirFromPointer -Pointer $pointer

    if ($descriptor.source_kind -eq "visual-regression-bundle") {
        Write-Step "Resolved latest curated visual task for $($descriptor.bundle_id): $taskDir"
    } else {
        Write-Step "Resolved latest $($descriptor.source_kind) task: $taskDir"
    }
}

$gateOutputCandidates = @($gateOutputCandidates | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
if ($gateOutputCandidates.Count -eq 0) {
    throw "No latest screenshot-backed task pointers were found under $resolvedTaskOutputRoot."
}
if ($gateOutputCandidates.Count -gt 1) {
    throw "Latest task pointers do not agree on one gate output directory: $($gateOutputCandidates -join ', ')"
}

$resolvedGateOutputDir = $gateOutputCandidates[0]
$resolvedGateSummaryPath = Join-Path $resolvedGateOutputDir "report\gate_summary.json"
Assert-PathExists -Path $resolvedGateSummaryPath -Label "inferred gate summary"
Write-Step "Resolved gate summary: $resolvedGateSummaryPath"

$resolvedReleaseSummaryPath = ""
$resolvedOutputSearchRoot = ""
$releaseSummaryDiscoveryMode = "auto"
$releaseSummaryDiscoveryReason = "no_matching_summary"
if (-not [string]::IsNullOrWhiteSpace($ReleaseCandidateSummaryJson)) {
    $resolvedReleaseSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReleaseCandidateSummaryJson
    $releaseSummaryDiscoveryMode = "explicit"
    $releaseSummaryDiscoveryReason = "explicit_path"
    Assert-PathExists -Path $resolvedReleaseSummaryPath -Label "explicit release-candidate summary"
    Write-Step "Using explicit release summary: $resolvedReleaseSummaryPath"
} else {
    $resolvedOutputSearchRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputSearchRoot
    $resolvedReleaseSummaryPath = Find-ReleaseCandidateSummary `
        -SearchRoot $resolvedOutputSearchRoot `
        -GateSummaryPath $resolvedGateSummaryPath
    if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
        $releaseSummaryDiscoveryReason = "matched_gate_summary"
        Write-Step "Auto-detected release summary: $resolvedReleaseSummaryPath"
    } else {
        $releaseSummaryDiscoveryMode = "auto_not_found"
        Write-Step "No matching release summary was found under $resolvedOutputSearchRoot"
    }
}

if (-not [string]::IsNullOrWhiteSpace($resolvedReleaseSummaryPath)) {
    [void](Read-JsonFile -Path $resolvedReleaseSummaryPath)
}

Update-ReleaseSummaryDiscoveryMetadata `
    -GateSummaryPath $resolvedGateSummaryPath `
    -ReleaseSummaryPath $resolvedReleaseSummaryPath `
    -Mode $releaseSummaryDiscoveryMode `
    -Reason $releaseSummaryDiscoveryReason `
    -SelectedSummaryPath $resolvedReleaseSummaryPath `
    -OutputSearchRoot $resolvedOutputSearchRoot `
    -SkipReleaseBundle ([bool]$SkipReleaseBundle)

Update-GateSummaryReviewTasksFromPointers `
    -GateSummaryPath $resolvedGateSummaryPath `
    -LatestPointerDescriptors $latestPointerDescriptors
Update-ReleaseSummaryTaskDirsFromPointers `
    -ReleaseSummaryPath $resolvedReleaseSummaryPath `
    -LatestPointerDescriptors $latestPointerDescriptors

if (-not $SkipSupersededReviewTaskAudit.IsPresent) {
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
} else {
    Write-Step "Skipped superseded review-task audit."
}

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
