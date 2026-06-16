function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

function Assert-LineContainsAll {
    param(
        [string]$Path,
        [string[]]$ExpectedFragments,
        [string]$Label
    )

    $lines = Get-Content -LiteralPath $Path
    foreach ($line in $lines) {
        $normalizedLine = $line -replace '/', '\'
        $matchesAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($normalizedLine -notmatch [regex]::Escape($normalizedFragment)) {
                $matchesAllFragments = $false
                break
            }
        }

        if ($matchesAllFragments) {
            return
        }
    }

    throw ("{0} does not contain one line with all expected fragments: {1}" -f $Label, ($ExpectedFragments -join ", "))
}

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$UnexpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if (-not [string]::IsNullOrWhiteSpace($UnexpectedText) -and
        $content -match [regex]::Escape($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText': $Path"
    }
}

function Convert-TestComparableValue {
    param($Value)

    if ($null -eq $Value) {
        return ""
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$Value
}

function Convert-TestComparablePathValue {
    param($Value)

    return (Convert-TestComparableValue -Value $Value) -replace '/', '\'
}

function Convert-TestPathToRepoRelativeDisplay {
    param(
        [string]$Path,
        [string]$RepoRoot
    )

    $normalizedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $normalizedPath = [System.IO.Path]::GetFullPath($Path)
    if ($normalizedPath.StartsWith($normalizedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relativePath = $normalizedPath.Substring($normalizedRepoRoot.Length).TrimStart('\', '/')
        return ".\" + ($relativePath -replace '/', '\')
    }

    return $normalizedPath
}

function Convert-TestEvidencePathToPublicDisplay {
    param(
        [string]$Path,
        [string]$RepoRoot
    )

    $normalizedDisplay = ([System.IO.Path]::GetFullPath($Path)) -replace '/', '\'
    foreach ($anchor in @(
            "\output\",
            "\release-assets\",
            "\release-assets-ci\",
            "\build-msvc-install\",
            "\build-msvc-install"
        )) {
        $index = $normalizedDisplay.LastIndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
        if ($index -ge 0) {
            return ".\" + $normalizedDisplay.Substring($index + 1)
        }
    }

    $repoDisplay = Convert-TestPathToRepoRelativeDisplay -Path $Path -RepoRoot $RepoRoot
    $normalizedPath = [System.IO.Path]::GetFullPath($Path)
    if ($repoDisplay -ne $normalizedPath) {
        return $repoDisplay
    }

    foreach ($anchor in @("\output\", "\release-assets\", "\release-assets-ci\")) {
        $index = $normalizedDisplay.IndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
        if ($index -ge 0) {
            return ".\" + $normalizedDisplay.Substring($index + 1)
        }
    }

    return $normalizedPath
}

function New-TestReleaseNoteBundleContract {
    param(
        [string]$SummaryOutputDir,
        [string]$ReportDir,
        [string]$StartHerePath,
        [string]$ArtifactGuidePath,
        [string]$ReviewerChecklistPath,
        [string]$ReleaseHandoffPath,
        [string]$ReleaseBodyPath,
        [string]$ReleaseSummaryPath,
        [string]$RepoRoot
    )

    $entrypoints = @(
        [ordered]@{ id = "start_here"; path = $StartHerePath; location = "summary_root" },
        [ordered]@{ id = "artifact_guide"; path = $ArtifactGuidePath; location = "report" },
        [ordered]@{ id = "reviewer_checklist"; path = $ReviewerChecklistPath; location = "report" },
        [ordered]@{ id = "release_handoff"; path = $ReleaseHandoffPath; location = "report" },
        [ordered]@{ id = "release_body_zh_cn"; path = $ReleaseBodyPath; location = "report" },
        [ordered]@{ id = "release_summary_zh_cn"; path = $ReleaseSummaryPath; location = "report" }
    )

    foreach ($entrypoint in $entrypoints) {
        $entrypoint["path_display"] = Convert-TestPathToRepoRelativeDisplay -Path ([string]$entrypoint.path) -RepoRoot $RepoRoot
        $entrypoint["required"] = $true
    }

    return [ordered]@{
        status = "declared"
        output_root = $SummaryOutputDir
        report_dir = $ReportDir
        entrypoint_count = @($entrypoints).Count
        required_entrypoint_count = @($entrypoints).Count
        entrypoint_ids = @($entrypoints | ForEach-Object { [string]$_["id"] })
        entrypoints = @($entrypoints)
    }
}
