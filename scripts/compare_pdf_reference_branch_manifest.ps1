param(
    [string]$RepoRoot = "",
    [string]$ManifestPath = "test/pdf_regression_manifest.json",
    [string[]]$ReferenceBranch = @(
        "origin/codex/pdf-cjk-copy-search-gate",
        "origin/codex/pdf-cjk-bullet-fallback"
    ),
    [string]$OutputJson = "",
    [switch]$FailOnMissingInDev
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-ManifestAuditRepoRoot {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }

    return (Resolve-Path $Value).Path
}

function Invoke-ManifestAuditGit {
    param(
        [string]$ResolvedRepoRoot,
        [string[]]$Arguments
    )

    $output = & git -C $ResolvedRepoRoot @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw ("git {0} failed with exit code {1}." -f ($Arguments -join " "), $LASTEXITCODE)
    }

    return (@($output) -join [Environment]::NewLine)
}

function Read-ManifestAuditGitBlobText {
    param(
        [string]$ResolvedRepoRoot,
        [string]$RevisionPath
    )

    $objectName = (Invoke-ManifestAuditGit -ResolvedRepoRoot $ResolvedRepoRoot -Arguments @("rev-parse", $RevisionPath)).Trim()
    $tempPath = [System.IO.Path]::GetTempFileName()
    try {
        $escapedRepoRoot = $ResolvedRepoRoot.Replace('"', '\"')
        $escapedTempPath = $tempPath.Replace('"', '\"')
        $command = 'git -C "{0}" cat-file -p {1} > "{2}"' -f $escapedRepoRoot, $objectName, $escapedTempPath
        & cmd.exe /d /c $command
        if ($LASTEXITCODE -ne 0) {
            throw ("git cat-file failed for {0} with exit code {1}." -f $RevisionPath, $LASTEXITCODE)
        }

        return Get-Content -Raw -Encoding UTF8 -LiteralPath $tempPath
    } finally {
        if (Test-Path -LiteralPath $tempPath) {
            Remove-Item -LiteralPath $tempPath -Force
        }
    }
}

function Get-ManifestAuditPropertyValue {
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

function Test-ManifestAuditBooleanProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-ManifestAuditPropertyValue -Object $Object -Name $Name
    return ($null -ne $value -and $value -eq $true)
}

function Get-ManifestAuditInfo {
    param(
        [string]$ManifestText,
        [string]$Source
    )

    $normalizedManifestText = $ManifestText.TrimStart([char]0xFEFF)
    $manifest = $normalizedManifestText | ConvertFrom-Json
    $samples = @($manifest.samples)
    $ids = @($samples | ForEach-Object { [string]$_.id } | Sort-Object)
    $duplicateIds = @($ids | Group-Object | Where-Object { $_.Count -gt 1 } | ForEach-Object { $_.Name })
    if ($duplicateIds.Count -gt 0) {
        throw ("Manifest {0} has duplicate sample id(s): {1}" -f $Source, ($duplicateIds -join ","))
    }

    $cjkIds = @($samples |
        Where-Object { Test-ManifestAuditBooleanProperty -Object $_ -Name "expect_cjk" } |
        ForEach-Object { [string]$_.id } |
        Sort-Object)
    $visualIds = @($samples |
        Where-Object { Test-ManifestAuditBooleanProperty -Object $_ -Name "expect_visual_baseline" } |
        ForEach-Object { [string]$_.id } |
        Sort-Object)

    return [pscustomobject]@{
        source = $Source
        sample_count = $ids.Count
        cjk_sample_count = $cjkIds.Count
        visual_baseline_sample_count = $visualIds.Count
        sample_ids = $ids
        cjk_sample_ids = $cjkIds
        visual_baseline_sample_ids = $visualIds
    }
}

function Get-ManifestAuditRelativePath {
    param(
        [string]$ResolvedRepoRoot,
        [string]$Path
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        $fullPath = [System.IO.Path]::GetFullPath($Path)
        $root = [System.IO.Path]::GetFullPath($ResolvedRepoRoot).TrimEnd('\', '/')
        if ($fullPath.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
            return ($fullPath.Substring($root.Length).TrimStart('\', '/') -replace '\\', '/')
        }
        return ($fullPath -replace '\\', '/')
    }

    return ($Path -replace '\\', '/')
}

$resolvedRepoRoot = Resolve-ManifestAuditRepoRoot -Value $RepoRoot
$manifestPathForGit = Get-ManifestAuditRelativePath -ResolvedRepoRoot $resolvedRepoRoot -Path $ManifestPath
$resolvedManifestPath = if ([System.IO.Path]::IsPathRooted($ManifestPath)) {
    [System.IO.Path]::GetFullPath($ManifestPath)
} else {
    Join-Path $resolvedRepoRoot $ManifestPath
}

if (-not (Test-Path -LiteralPath $resolvedManifestPath)) {
    throw "Manifest not found: $resolvedManifestPath"
}

$currentManifestText = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedManifestPath
$currentInfo = Get-ManifestAuditInfo -ManifestText $currentManifestText -Source "working-tree"
$referenceResults = New-Object 'System.Collections.Generic.List[object]'

foreach ($branch in $ReferenceBranch) {
    if ([string]::IsNullOrWhiteSpace($branch)) {
        continue
    }

    $branchTip = Invoke-ManifestAuditGit -ResolvedRepoRoot $resolvedRepoRoot -Arguments @("rev-parse", $branch)
    $branchManifestText = Read-ManifestAuditGitBlobText `
        -ResolvedRepoRoot $resolvedRepoRoot `
        -RevisionPath ("{0}:{1}" -f $branch, $manifestPathForGit)
    $branchInfo = Get-ManifestAuditInfo -ManifestText $branchManifestText -Source $branch

    $missingInDev = @($branchInfo.sample_ids |
        Where-Object { $currentInfo.sample_ids -notcontains $_ } |
        Sort-Object)
    $devOnly = @($currentInfo.sample_ids |
        Where-Object { $branchInfo.sample_ids -notcontains $_ } |
        Sort-Object)

    $recommendation = "do_not_merge_whole_branch"
    if ($missingInDev.Count -eq 0) {
        $recommendation = "manifest_ids_already_covered_in_dev"
    }

    [void]$referenceResults.Add([ordered]@{
            branch = $branch
            branch_tip = ($branchTip.Trim())
            sample_count = $branchInfo.sample_count
            cjk_sample_count = $branchInfo.cjk_sample_count
            visual_baseline_sample_count = $branchInfo.visual_baseline_sample_count
            matching_sample_id_count = ($branchInfo.sample_count - $missingInDev.Count)
            missing_in_dev_count = $missingInDev.Count
            missing_in_dev_ids = $missingInDev
            dev_only_count = $devOnly.Count
            dev_only_ids = $devOnly
            whole_branch_merge_recommended = $false
            recommendation = $recommendation
        })
}

$totalMissing = 0
foreach ($reference in $referenceResults) {
    $totalMissing += [int]$reference.missing_in_dev_count
}

$overallRecommendation = "manifest_ids_already_covered_in_dev"
if ($totalMissing -gt 0) {
    $overallRecommendation = "manual_review_missing_manifest_ids_before_pdf_branch_cleanup"
}

$summary = [ordered]@{
    source_schema = "featherdoc.pdf_reference_branch_manifest_comparison.v1"
    generated_at_utc = [DateTime]::UtcNow.ToString("o")
    repo_root = $resolvedRepoRoot
    manifest_path = $manifestPathForGit
    current = [ordered]@{
        source = "working-tree"
        sample_count = $currentInfo.sample_count
        cjk_sample_count = $currentInfo.cjk_sample_count
        visual_baseline_sample_count = $currentInfo.visual_baseline_sample_count
    }
    reference_count = $referenceResults.Count
    references = $referenceResults.ToArray()
    total_missing_in_dev_count = $totalMissing
    whole_branch_merge_recommended = $false
    recommendation = $overallRecommendation
}

if (-not [string]::IsNullOrWhiteSpace($OutputJson)) {
    $resolvedOutputJson = [System.IO.Path]::GetFullPath($OutputJson)
    New-Item -ItemType Directory -Path (Split-Path -Parent $resolvedOutputJson) -Force | Out-Null
    $summary | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 -LiteralPath $resolvedOutputJson
    Write-Host "PDF reference branch manifest comparison: $resolvedOutputJson"
} else {
    $summary | ConvertTo-Json -Depth 8
}

if ($FailOnMissingInDev -and $totalMissing -gt 0) {
    exit 1
}
