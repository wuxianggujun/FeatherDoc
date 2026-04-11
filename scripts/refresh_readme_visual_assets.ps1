<#
.SYNOPSIS
Refreshes repository README gallery PNG assets from Word visual evidence.

.DESCRIPTION
Copies the repository-facing preview images under docs/assets/readme from the
latest document and fixed-grid review tasks, with fallback to the original
task source bundles when task-local evidence has not been materialized yet.

.PARAMETER TaskOutputRoot
Root directory containing the review-task latest-pointer files.

.PARAMETER DocumentTaskDir
Optional explicit document review task directory. If omitted, the script
resolves the latest document task from TaskOutputRoot.

.PARAMETER FixedGridTaskDir
Optional explicit fixed-grid review task directory. If omitted, the script
resolves the latest fixed-grid task from TaskOutputRoot.

.PARAMETER AssetsDir
Target repository directory for the refreshed README gallery PNG files.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\refresh_readme_visual_assets.ps1 `
    -TaskOutputRoot output/word-visual-smoke/tasks-readme-refresh-validation
#>
param(
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [string]$DocumentTaskDir = "",
    [string]$FixedGridTaskDir = "",
    [string]$ColumnWidthVisualDir = "output/word-visual-sample-edit-existing-table-column-widths",
    [string]$AssetsDir = "docs/assets/readme"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[refresh-readme-visual-assets] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

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

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing ${Label}: $Path"
    }
}

function Resolve-TaskDirFromPointer {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$Label
    )

    Assert-PathExists -Path $ScriptPath -Label "$Label resolver script"

    $output = & powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to resolve $Label task directory."
    }

    if ($null -eq $output) {
        throw "Resolver for $Label task directory returned no output."
    }

    $resolved = ($output | Select-Object -Last 1).ToString().Trim()
    if ([string]::IsNullOrWhiteSpace($resolved)) {
        throw "Resolver for $Label task directory returned an empty value."
    }

    return $resolved
}

function Read-TaskManifest {
    param([string]$TaskDir)

    $manifestPath = Join-Path $TaskDir "task_manifest.json"
    if (-not (Test-Path -LiteralPath $manifestPath)) {
        return $null
    }

    return (Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json)
}

function Resolve-DocumentEvidence {
    param([string]$TaskDir)

    $taskEvidenceDir = Join-Path $TaskDir "evidence"
    $taskContactSheet = Join-Path $taskEvidenceDir "contact_sheet.png"
    $taskPage05 = Join-Path $taskEvidenceDir "pages\page-05.png"
    $taskPage06 = Join-Path $taskEvidenceDir "pages\page-06.png"

    if ((Test-Path -LiteralPath $taskContactSheet) -and
        (Test-Path -LiteralPath $taskPage05) -and
        (Test-Path -LiteralPath $taskPage06)) {
        return [ordered]@{
            contact_sheet = $taskContactSheet
            page_05 = $taskPage05
            page_06 = $taskPage06
        }
    }

    $manifest = Read-TaskManifest -TaskDir $TaskDir
    if ($null -ne $manifest -and
        $null -ne $manifest.source -and
        $manifest.source.kind -eq "document" -and
        -not [string]::IsNullOrWhiteSpace($manifest.source.path)) {
        $sourceDocDir = Split-Path -Path $manifest.source.path -Parent
        $sourceEvidenceDir = Join-Path $sourceDocDir "evidence"
        $sourceContactSheet = Join-Path $sourceEvidenceDir "contact_sheet.png"
        $sourcePage05 = Join-Path $sourceEvidenceDir "pages\page-05.png"
        $sourcePage06 = Join-Path $sourceEvidenceDir "pages\page-06.png"

        if ((Test-Path -LiteralPath $sourceContactSheet) -and
            (Test-Path -LiteralPath $sourcePage05) -and
            (Test-Path -LiteralPath $sourcePage06)) {
            return [ordered]@{
                contact_sheet = $sourceContactSheet
                page_05 = $sourcePage05
                page_06 = $sourcePage06
            }
        }
    }

    throw "Unable to resolve document evidence from task directory or task manifest: $TaskDir"
}

function Resolve-FixedGridEvidence {
    param([string]$TaskDir)

    $taskAggregateContactSheet = Join-Path $TaskDir "evidence\aggregate_contact_sheet.png"
    if (Test-Path -LiteralPath $taskAggregateContactSheet) {
        return [ordered]@{
            aggregate_contact_sheet = $taskAggregateContactSheet
        }
    }

    $manifest = Read-TaskManifest -TaskDir $TaskDir
    if ($null -ne $manifest -and
        $null -ne $manifest.source -and
        $manifest.source.kind -eq "fixed-grid-regression-bundle" -and
        -not [string]::IsNullOrWhiteSpace($manifest.source.path)) {
        $sourceAggregateContactSheet = Join-Path $manifest.source.path "aggregate-evidence\contact_sheet.png"
        if (Test-Path -LiteralPath $sourceAggregateContactSheet) {
            return [ordered]@{
                aggregate_contact_sheet = $sourceAggregateContactSheet
            }
        }
    }

    throw "Unable to resolve fixed-grid aggregate evidence from task directory or task manifest: $TaskDir"
}

function Resolve-ColumnWidthEvidence {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return $null
    }

    $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $InputPath
    $pagePath = Join-Path $resolvedRoot "evidence\pages\page-01.png"
    if (-not (Test-Path -LiteralPath $pagePath)) {
        return $null
    }

    return [ordered]@{
        page_01 = $pagePath
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedTaskOutputRoot = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $TaskOutputRoot
$resolvedAssetsDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $AssetsDir

Assert-PathExists -Path $resolvedTaskOutputRoot -Label "task output root"

if ([string]::IsNullOrWhiteSpace($DocumentTaskDir)) {
    $documentResolverScript = Join-Path $PSScriptRoot "open_latest_word_review_task.ps1"
    $DocumentTaskDir = Resolve-TaskDirFromPointer `
        -ScriptPath $documentResolverScript `
        -Arguments @(
            "-TaskOutputRoot", $resolvedTaskOutputRoot,
            "-SourceKind", "document",
            "-PrintField", "task_dir"
        ) `
        -Label "document"
}

if ([string]::IsNullOrWhiteSpace($FixedGridTaskDir)) {
    $fixedGridResolverScript = Join-Path $PSScriptRoot "open_latest_fixed_grid_review_task.ps1"
    $FixedGridTaskDir = Resolve-TaskDirFromPointer `
        -ScriptPath $fixedGridResolverScript `
        -Arguments @(
            "-TaskOutputRoot", $resolvedTaskOutputRoot,
            "-PrintField", "task_dir"
        ) `
        -Label "fixed-grid"
}

$resolvedDocumentTaskDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $DocumentTaskDir
$resolvedFixedGridTaskDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $FixedGridTaskDir

Assert-PathExists -Path $resolvedDocumentTaskDir -Label "document task directory"
Assert-PathExists -Path $resolvedFixedGridTaskDir -Label "fixed-grid task directory"

$documentEvidence = Resolve-DocumentEvidence -TaskDir $resolvedDocumentTaskDir
$fixedGridEvidence = Resolve-FixedGridEvidence -TaskDir $resolvedFixedGridTaskDir
$columnWidthEvidence = Resolve-ColumnWidthEvidence -RepoRoot $repoRoot -InputPath $ColumnWidthVisualDir

New-Item -ItemType Directory -Path $resolvedAssetsDir -Force | Out-Null

$copies = @(
    @{
        Label = "visual smoke contact sheet"
        Source = $documentEvidence.contact_sheet
        Destination = Join-Path $resolvedAssetsDir "visual-smoke-contact-sheet.png"
    },
    @{
        Label = "visual smoke page 05"
        Source = $documentEvidence.page_05
        Destination = Join-Path $resolvedAssetsDir "visual-smoke-page-05.png"
    },
    @{
        Label = "visual smoke page 06"
        Source = $documentEvidence.page_06
        Destination = Join-Path $resolvedAssetsDir "visual-smoke-page-06.png"
    },
    @{
        Label = "reopened fixed-layout column-width page 01"
        Source = if ($null -eq $columnWidthEvidence) { "" } else { $columnWidthEvidence.page_01 }
        Destination = Join-Path $resolvedAssetsDir "reopened-fixed-layout-column-widths-page-01.png"
        Optional = $true
    },
    @{
        Label = "fixed-grid aggregate contact sheet"
        Source = $fixedGridEvidence.aggregate_contact_sheet
        Destination = Join-Path $resolvedAssetsDir "fixed-grid-aggregate-contact-sheet.png"
    }
)

foreach ($copy in $copies) {
    $isOptional = $copy.ContainsKey("Optional") -and $copy.Optional
    if ($isOptional -and [string]::IsNullOrWhiteSpace($copy.Source)) {
        Write-Step "Skipping optional asset because source evidence is unavailable: $($copy.Destination)"
        continue
    }

    Assert-PathExists -Path $copy.Source -Label $copy.Label
    Copy-Item -LiteralPath $copy.Source -Destination $copy.Destination -Force
}

Write-Step "Completed README visual asset refresh"
Write-Host "Task output root: $resolvedTaskOutputRoot"
Write-Host "Document task: $resolvedDocumentTaskDir"
Write-Host "Fixed-grid task: $resolvedFixedGridTaskDir"
Write-Host "Assets directory: $resolvedAssetsDir"
foreach ($copy in $copies) {
    Write-Host "$($copy.Label): $($copy.Destination)"
}
