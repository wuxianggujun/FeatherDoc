function Write-Step {
    param([string]$Message)
    Write-Host "[package-release-assets] $Message"
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
        throw "Missing ${Label}: $Path"
    }
}

function Get-ProjectVersion {
    param([string]$RepoRoot)

    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        return ""
    }

    $content = Get-Content -Raw -LiteralPath $cmakePath
    $match = [regex]::Match($content, 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        return ""
    }

    return $match.Groups[1].Value
}

function New-CleanDirectory {
    param([string]$Path)

    if (Test-Path -LiteralPath $Path) {
        Remove-Item -LiteralPath $Path -Recurse -Force
    }

    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Copy-PathTree {
    param(
        [string]$Source,
        [string]$Destination
    )

    Assert-PathExists -Path $Source -Label "copy source"

    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }

    New-Item -ItemType Directory -Path (Split-Path -Parent $Destination) -Force | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force
}

function Copy-FileToPath {
    param(
        [string]$Source,
        [string]$Destination
    )

    Assert-PathExists -Path $Source -Label "copy source file"
    New-Item -ItemType Directory -Path (Split-Path -Parent $Destination) -Force | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Test-TextLineContainsAll {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    foreach ($line in ($Text -split "\r?\n")) {
        $containsAll = $true
        foreach ($needle in $Needles) {
            if ($line -notlike "*$needle*") {
                $containsAll = $false
                break
            }
        }

        if ($containsAll) {
            return $true
        }
    }

    return $false
}

function Assert-ProjectTemplateChecklistHandoffEvidenceLine {
    param(
        [string]$Path,
        [string]$Label
    )

    Assert-PathExists -Path $Path -Label $Label
    $content = Get-Content -Raw -LiteralPath $Path
    $requiredFragments = @(
        "Project-template readiness checklist handoff evidence",
        "project_template_readiness_checklist_entrypoints_source_reports=",
        "status=",
        "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
        "required_entrypoint_count=3",
        "entrypoints=",
        "start_here",
        "artifact_guide",
        "reviewer_checklist",
        "entrypoint_paths=",
        "START_HERE.md",
        "ARTIFACT_GUIDE.md",
        "REVIEWER_CHECKLIST.md",
        "marker=release_entry_project_template_readiness_checklist_trace",
        "source_schema=featherdoc.release_candidate_summary",
        "source_report="
    )

    if (-not (Test-TextLineContainsAll -Text $content -Needles $requiredFragments)) {
        throw "$Label must keep project-template readiness checklist handoff evidence count, status, checklist path, required entrypoint count, entrypoint ids, entrypoint paths, marker, source schema, and source report on the same compact evidence line."
    }
}

function Assert-StagedProjectTemplateChecklistHandoffEvidence {
    param([string]$ReleaseCandidateRoot)

    Assert-ProjectTemplateChecklistHandoffEvidenceLine `
        -Path (Join-Path $ReleaseCandidateRoot "START_HERE.md") `
        -Label "staged START_HERE.md project-template checklist handoff evidence"
    Assert-ProjectTemplateChecklistHandoffEvidenceLine `
        -Path (Join-Path $ReleaseCandidateRoot "report\ARTIFACT_GUIDE.md") `
        -Label "staged ARTIFACT_GUIDE.md project-template checklist handoff evidence"
    Assert-ProjectTemplateChecklistHandoffEvidenceLine `
        -Path (Join-Path $ReleaseCandidateRoot "report\REVIEWER_CHECKLIST.md") `
        -Label "staged REVIEWER_CHECKLIST.md project-template checklist handoff evidence"
}

function Assert-StagedWordVisualStandardReviewMetadataHandoffEvidence {
    param(
        [string]$ReleaseCandidateRoot,
        [int]$ExpectedMetadataCount
    )

    $path = Join-Path $ReleaseCandidateRoot "report\release_governance_handoff.md"
    $label = "staged release_governance_handoff.md Word visual standard review metadata evidence"
    Assert-PathExists -Path $path -Label $label

    $content = Get-Content -Raw -LiteralPath $path
    $requiredFragments = @(
        "Word visual standard review metadata source reports",
        "word_visual_standard_review_metadata_count: ``$ExpectedMetadataCount``",
        "word_visual_standard_review_task_keys",
        "word_visual_standard_review_status_summary",
        "word_visual_standard_review_verdict_summary",
        "review_result_path",
        "final_review_path"
    )

    foreach ($fragment in $requiredFragments) {
        if ($content.IndexOf($fragment, [System.StringComparison]::Ordinal) -lt 0) {
            throw "$label must keep detailed Word visual standard review metadata source reports, including '$fragment'."
        }
    }
}

function New-ZipArchive {
    param(
        [string[]]$SourcePaths,
        [string]$ZipPath
    )

    foreach ($source in $SourcePaths) {
        Assert-PathExists -Path $source -Label "zip source"
    }

    if (Test-Path -LiteralPath $ZipPath) {
        Remove-Item -LiteralPath $ZipPath -Force
    }

    Compress-Archive -LiteralPath $SourcePaths -DestinationPath $ZipPath -CompressionLevel Optimal
}

function Get-ResolvedReleaseVersion {
    param(
        [string]$ExplicitVersion,
        $Summary,
        [string]$RepoRoot
    )

    if (-not [string]::IsNullOrWhiteSpace($ExplicitVersion)) {
        return $ExplicitVersion
    }

    $summaryVersion = Get-OptionalPropertyValue -Object $Summary -Name "release_version"
    if (-not [string]::IsNullOrWhiteSpace($summaryVersion)) {
        return $summaryVersion
    }

    return Get-ProjectVersion -RepoRoot $RepoRoot
}
