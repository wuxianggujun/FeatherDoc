<#
.SYNOPSIS
Packages release-facing FeatherDoc artifacts from a release-candidate summary.

.DESCRIPTION
Builds the public release ZIP files for the installed MSVC package, the
visual-validation gallery, and the screenshot-backed release evidence bundle.
The script stages a normalized install tree so the public gallery always picks
up the latest repository README assets even when an existing install prefix was
created before the newest gallery files were added.

.PARAMETER SummaryJson
Path to the release-candidate summary JSON produced by
run_release_candidate_checks.ps1.

.PARAMETER OutputRoot
Root directory under which versioned release assets are written.

.PARAMETER ReleaseVersion
Optional explicit version override. Defaults to the version embedded in the
summary or CMakeLists.txt.

.PARAMETER InstallPrefix
Optional explicit install-prefix override. Defaults to
summary.steps.install_smoke.install_prefix.

.PARAMETER ReadmeAssetsDir
Optional explicit README gallery directory override. Defaults to
summary.readme_gallery.assets_dir or docs/assets/readme.

.PARAMETER UploadReleaseTag
Optional GitHub release tag. When set, the generated ZIP files are uploaded via
gh release upload --clobber.

.PARAMETER AllowIncomplete
Allows packaging even when execution_status / visual_verdict are not pass.

.PARAMETER KeepStaging
Keeps the staged packaging directory under output/release-assets for inspection.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 `
    -SummaryJson .\output\release-candidate-checks\report\summary.json

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\package_release_assets.ps1 `
    -SummaryJson .\output\release-candidate-checks\report\summary.json `
    -UploadReleaseTag v1.6.4
#>
param(
    [string]$SummaryJson = "output/release-candidate-checks/report/summary.json",
    [string]$OutputRoot = "output/release-assets",
    [string]$ReleaseVersion = "",
    [string]$InstallPrefix = "",
    [string]$ReadmeAssetsDir = "",
    [string]$UploadReleaseTag = "",
    [switch]$AllowIncomplete,
    [switch]$KeepStaging
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function Get-OptionalPropertyValue {
    param(
        $Object,
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

function Resolve-GateRoot {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    $summaryJson = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($summaryJson)) {
        $resolvedSummaryJson = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $summaryJson
        if (Test-Path -LiteralPath $resolvedSummaryJson) {
            return Split-Path -Parent (Split-Path -Parent $resolvedSummaryJson)
        }
    }

    $gateOutputDir = Get-OptionalPropertyValue -Object $Summary -Name "gate_output_dir"
    if (-not [string]::IsNullOrWhiteSpace($gateOutputDir)) {
        return Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateOutputDir
    }

    return ""
}

function Get-GalleryFileNames {
    return @(
        "visual-smoke-contact-sheet.png",
        "visual-smoke-page-05.png",
        "visual-smoke-page-06.png",
        "reopened-fixed-layout-column-widths-page-01.png",
        "fixed-grid-merge-right-page-01.png",
        "fixed-grid-merge-down-page-01.png",
        "fixed-grid-aggregate-contact-sheet.png",
        "sample-chinese-template-page-01.png"
    )
}

function Resolve-GallerySourceFile {
    param(
        [string]$ReadmeAssetsDir,
        [string]$InstallPrefix,
        [string]$FileName
    )

    $readmeCandidate = Join-Path $ReadmeAssetsDir $FileName
    if (Test-Path -LiteralPath $readmeCandidate) {
        return $readmeCandidate
    }

    $installCandidate = Join-Path $InstallPrefix ("share\FeatherDoc\visual-validation\{0}" -f $FileName)
    if (Test-Path -LiteralPath $installCandidate) {
        return $installCandidate
    }

    throw "Could not resolve gallery asset '$FileName' from README assets or install prefix."
}

function Get-VisualVerdict {
    param(
        [string]$RepoRoot,
        $Summary
    )

    $summaryVerdict = Get-OptionalPropertyValue -Object $Summary -Name "visual_verdict"
    if (-not [string]::IsNullOrWhiteSpace($summaryVerdict)) {
        return $summaryVerdict
    }

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    $stepVerdict = Get-OptionalPropertyValue -Object $visualGate -Name "visual_verdict"
    if (-not [string]::IsNullOrWhiteSpace($stepVerdict)) {
        return $stepVerdict
    }

    $gateSummaryPath = Get-OptionalPropertyValue -Object $visualGate -Name "summary_json"
    if (-not [string]::IsNullOrWhiteSpace($gateSummaryPath)) {
        $resolvedGateSummaryPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $gateSummaryPath
        if (Test-Path -LiteralPath $resolvedGateSummaryPath) {
            $gateSummary = Get-Content -Raw -LiteralPath $resolvedGateSummaryPath | ConvertFrom-Json
            return Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
        }
    }

    return ""
}

function Get-VisualGateStatus {
    param($Summary)

    $visualGate = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $Summary -Name "steps") -Name "visual_gate"
    return Get-OptionalPropertyValue -Object $visualGate -Name "status"
}

function Get-AssetDescriptor {
    param(
        [string]$Path,
        [string]$Label
    )

    $item = Get-Item -LiteralPath $Path
    $hash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()

    return [ordered]@{
        label = $Label
        path = $item.FullName
        size_bytes = $item.Length
        sha256 = $hash
    }
}

function Get-TextLikeFiles {
    param([string[]]$RootPaths)

    $textExtensions = @(
        ".md",
        ".txt",
        ".json",
        ".xml",
        ".yml",
        ".yaml",
        ".csv",
        ".log",
        ".rst"
    )

    $results = [System.Collections.Generic.List[string]]::new()
    foreach ($rootPath in $RootPaths) {
        if ([string]::IsNullOrWhiteSpace($rootPath) -or -not (Test-Path -LiteralPath $rootPath)) {
            continue
        }

        foreach ($file in Get-ChildItem -LiteralPath $rootPath -Recurse -File) {
            if ($textExtensions -contains $file.Extension.ToLowerInvariant()) {
                [void]$results.Add($file.FullName)
            }
        }
    }

    return @($results | Sort-Object -Unique)
}

function Convert-RepoPathToRelative {
    param(
        [string]$Value,
        [string]$RepoRoot
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $normalizedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $normalizedValue = $Value -replace '/', '\'
    if (-not [System.IO.Path]::IsPathRooted($normalizedValue)) {
        return $Value
    }

    $resolvedValue = [System.IO.Path]::GetFullPath($normalizedValue)
    if (-not $resolvedValue.StartsWith($normalizedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Value
    }

    $relative = $resolvedValue.Substring($normalizedRepoRoot.Length).TrimStart('\', '/')
    if ([string]::IsNullOrWhiteSpace($relative)) {
        return "."
    }

    return ".\" + ($relative -replace '/', '\')
}

function Convert-ReleaseTextToPublic {
    param(
        [string]$Value,
        [string]$RepoRoot
    )

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $result = $Value
    $result = $result.Replace($RepoRoot, ".")
    $result = $result.Replace(($RepoRoot -replace '\\', '/'), ".")
    $result = [regex]::Replace($result, '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+', '<windows-absolute-path>')
    $result = [regex]::Replace($result, '(?<!\w)/(?:Users|home)/[^\s"''`<>|]+', '<unix-absolute-path>')

    $replacements = @(
        @{ Pattern = '(?i)发布说明草稿'; Replacement = '发布说明预览版' }
        @{ Pattern = '(?i)请在发布前补齐'; Replacement = '请在公开发布前完善' }
        @{ Pattern = '(?i)\brelease body draft\b'; Replacement = 'release body preview' }
        @{ Pattern = '(?i)\brelease-note drafts\b'; Replacement = 'release-note previews' }
        @{ Pattern = '(?i)\bpublic release drafts\b'; Replacement = 'public release previews' }
        @{ Pattern = '(?i)\brelease drafts\b'; Replacement = 'release previews' }
        @{ Pattern = '(?i)\bdraft review\b'; Replacement = 'release-note review' }
        @{ Pattern = '(?i)\bdraft boilerplate\b'; Replacement = 'placeholder boilerplate' }
        @{ Pattern = '(?i)\brelease-note drafting\b'; Replacement = 'release-note preparation' }
        @{ Pattern = '(?i)\bdraft=false\b'; Replacement = 'public-release-enabled' }
        @{ Pattern = '(?i)\bdrafts\b'; Replacement = 'previews' }
        @{ Pattern = '(?i)\bdrafting\b'; Replacement = 'preparation' }
        @{ Pattern = '(?i)\bdraft\b'; Replacement = 'preview' }
        @{ Pattern = '草稿'; Replacement = '预览版' }
    )

    foreach ($replacement in $replacements) {
        $result = [regex]::Replace($result, $replacement.Pattern, $replacement.Replacement)
    }

    return $result
}

function Convert-StructuredValueToPublic {
    param(
        $Value,
        [string]$RepoRoot
    )

    if ($null -eq $Value) {
        return $null
    }

    if ($Value -is [string]) {
        $relativeValue = Convert-RepoPathToRelative -Value $Value -RepoRoot $RepoRoot
        return Convert-ReleaseTextToPublic -Value $relativeValue -RepoRoot $RepoRoot
    }

    if ($Value -is [System.Collections.IDictionary]) {
        $result = [ordered]@{}
        foreach ($key in $Value.Keys) {
            $result[$key] = Convert-StructuredValueToPublic -Value $Value[$key] -RepoRoot $RepoRoot
        }
        return $result
    }

    if ($Value -is [pscustomobject]) {
        $result = [ordered]@{}
        foreach ($property in $Value.PSObject.Properties) {
            $result[$property.Name] = Convert-StructuredValueToPublic -Value $property.Value -RepoRoot $RepoRoot
        }
        return $result
    }

    if ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [string])) {
        $result = @()
        foreach ($item in $Value) {
            $result += ,(Convert-StructuredValueToPublic -Value $item -RepoRoot $RepoRoot)
        }
        return $result
    }

    return $Value
}

function Sanitize-StagedReleaseMaterials {
    param(
        [string]$RepoRoot,
        [string[]]$RootPaths
    )

    foreach ($filePath in Get-TextLikeFiles -RootPaths $RootPaths) {
        if ([System.IO.Path]::GetExtension($filePath).Equals(".json", [System.StringComparison]::OrdinalIgnoreCase)) {
            $content = Get-Content -Raw -LiteralPath $filePath | ConvertFrom-Json
            $sanitized = Convert-StructuredValueToPublic -Value $content -RepoRoot $RepoRoot
            ($sanitized | ConvertTo-Json -Depth 100) | Set-Content -LiteralPath $filePath -Encoding UTF8
            continue
        }

        $content = Get-Content -Raw -LiteralPath $filePath
        $content = Convert-ReleaseTextToPublic -Value $content -RepoRoot $RepoRoot
        Set-Content -LiteralPath $filePath -Encoding UTF8 -Value $content
    }
}

$repoRoot = Resolve-RepoRoot
$releaseMaterialAuditScript = Join-Path $repoRoot "scripts\assert_release_material_safety.ps1"
$resolvedSummaryPath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedOutputRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $OutputRoot

Assert-PathExists -Path $resolvedSummaryPath -Label "release summary JSON"

$summary = Get-Content -Raw -LiteralPath $resolvedSummaryPath | ConvertFrom-Json
$releaseVersion = Get-ResolvedReleaseVersion -ExplicitVersion $ReleaseVersion -Summary $summary -RepoRoot $repoRoot
if ([string]::IsNullOrWhiteSpace($releaseVersion)) {
    throw "Could not resolve the release version from the summary or CMakeLists.txt."
}

$executionStatus = Get-OptionalPropertyValue -Object $summary -Name "execution_status"
$visualVerdict = Get-VisualVerdict -RepoRoot $repoRoot -Summary $summary
$visualGateStatus = Get-VisualGateStatus -Summary $summary

if (-not $AllowIncomplete) {
    if ($executionStatus -ne "pass") {
        throw "Refusing to package release assets because execution_status is '$executionStatus'. Use -AllowIncomplete to override."
    }

    if (-not [string]::IsNullOrWhiteSpace($visualVerdict) -and $visualVerdict -ne "pass") {
        throw "Refusing to package release assets because visual_verdict is '$visualVerdict'. Use -AllowIncomplete to override."
    }
}

$installSmoke = Get-OptionalPropertyObject -Object (Get-OptionalPropertyObject -Object $summary -Name "steps") -Name "install_smoke"
$resolvedInstallPrefix = if (-not [string]::IsNullOrWhiteSpace($InstallPrefix)) {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $InstallPrefix
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath (Get-OptionalPropertyValue -Object $installSmoke -Name "install_prefix")
}
Assert-PathExists -Path $resolvedInstallPrefix -Label "install prefix"

$resolvedReadmeAssetsDir = if (-not [string]::IsNullOrWhiteSpace($ReadmeAssetsDir)) {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReadmeAssetsDir
} else {
    $summaryReadmeAssetsDir = Get-OptionalPropertyValue -Object (Get-OptionalPropertyObject -Object $summary -Name "readme_gallery") -Name "assets_dir"
    if (-not [string]::IsNullOrWhiteSpace($summaryReadmeAssetsDir)) {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $summaryReadmeAssetsDir
    } else {
        Join-Path $repoRoot "docs\assets\readme"
    }
}
Assert-PathExists -Path $resolvedReadmeAssetsDir -Label "README gallery asset directory"

$reportDir = Split-Path -Parent $resolvedSummaryPath
$startHerePath = Get-OptionalPropertyValue -Object $summary -Name "start_here"
if ([string]::IsNullOrWhiteSpace($startHerePath)) {
    $startHerePath = Join-Path (Split-Path -Parent $reportDir) "START_HERE.md"
}
$resolvedStartHerePath = Resolve-FullPath -RepoRoot $repoRoot -InputPath $startHerePath
Assert-PathExists -Path $resolvedStartHerePath -Label "release START_HERE"

$resolvedGateRoot = Resolve-GateRoot -RepoRoot $repoRoot -Summary $summary
$resolvedGateReportDir = ""
$resolvedSmokeEvidenceDir = ""
$resolvedFixedGridAggregateDir = ""
$hasVisualGateEvidence = $false
if (-not [string]::IsNullOrWhiteSpace($resolvedGateRoot) -and (Test-Path -LiteralPath $resolvedGateRoot)) {
    $resolvedGateReportDir = Join-Path $resolvedGateRoot "report"
    $resolvedSmokeEvidenceDir = Join-Path $resolvedGateRoot "smoke\evidence"
    $resolvedFixedGridAggregateDir = Join-Path $resolvedGateRoot "fixed-grid\aggregate-evidence"
    $hasVisualGateEvidence = `
        (Test-Path -LiteralPath $resolvedGateReportDir) -and `
        (Test-Path -LiteralPath $resolvedSmokeEvidenceDir) -and `
        (Test-Path -LiteralPath $resolvedFixedGridAggregateDir)
}

if (-not $hasVisualGateEvidence) {
    if (-not $AllowIncomplete -or $visualGateStatus -ne "skipped") {
        Assert-PathExists -Path $resolvedGateRoot -Label "Word visual gate output directory"
        Assert-PathExists -Path $resolvedGateReportDir -Label "gate report directory"
        Assert-PathExists -Path $resolvedSmokeEvidenceDir -Label "smoke evidence directory"
        Assert-PathExists -Path $resolvedFixedGridAggregateDir -Label "fixed-grid aggregate evidence directory"
    }
}

$versionOutputDir = Join-Path $resolvedOutputRoot ("v{0}" -f $releaseVersion)
$stagingRoot = Join-Path $versionOutputDir "staging"
$installLeaf = (Get-Item -LiteralPath $resolvedInstallPrefix).Name
$stageInstallRoot = Join-Path $stagingRoot $installLeaf
$stageInstallGalleryDir = Join-Path $stageInstallRoot "share\FeatherDoc\visual-validation"
$stageGalleryRoot = Join-Path $stagingRoot "visual-validation"
$stageReleaseCandidateRoot = Join-Path $stagingRoot "release-candidate-checks"
$stageWordVisualRoot = Join-Path $stagingRoot "word-visual-release-gate"

Write-Step "Preparing staging directory"
New-Item -ItemType Directory -Path $versionOutputDir -Force | Out-Null
New-CleanDirectory -Path $stagingRoot

Write-Step "Staging install prefix"
Copy-PathTree -Source $resolvedInstallPrefix -Destination $stageInstallRoot
New-Item -ItemType Directory -Path $stageInstallGalleryDir -Force | Out-Null
New-Item -ItemType Directory -Path $stageGalleryRoot -Force | Out-Null

foreach ($fileName in Get-GalleryFileNames) {
    $resolvedSource = Resolve-GallerySourceFile `
        -ReadmeAssetsDir $resolvedReadmeAssetsDir `
        -InstallPrefix $resolvedInstallPrefix `
        -FileName $fileName
    Copy-FileToPath -Source $resolvedSource -Destination (Join-Path $stageInstallGalleryDir $fileName)
    Copy-FileToPath -Source $resolvedSource -Destination (Join-Path $stageGalleryRoot $fileName)
}

Write-Step "Staging release evidence bundle"
New-Item -ItemType Directory -Path $stageReleaseCandidateRoot -Force | Out-Null
Copy-FileToPath -Source $resolvedStartHerePath -Destination (Join-Path $stageReleaseCandidateRoot "START_HERE.md")
Copy-PathTree -Source $reportDir -Destination (Join-Path $stageReleaseCandidateRoot "report")

New-Item -ItemType Directory -Path $stageWordVisualRoot -Force | Out-Null
if ($hasVisualGateEvidence) {
    Copy-PathTree -Source $resolvedGateReportDir -Destination (Join-Path $stageWordVisualRoot "report")
    Copy-PathTree -Source $resolvedSmokeEvidenceDir -Destination (Join-Path $stageWordVisualRoot "smoke\evidence")
    Copy-PathTree -Source $resolvedFixedGridAggregateDir -Destination (Join-Path $stageWordVisualRoot "fixed-grid\aggregate-evidence")
} elseif ($AllowIncomplete -and $visualGateStatus -eq "skipped") {
    $incompleteNote = @'
# Word Visual Gate Skipped

- This release-evidence preview was packaged from a CI-style summary with `visual_gate=skipped`.
- Screenshot-backed Word evidence was not available in this environment, so this bundle only keeps the release-candidate report set.
- Run the local Windows preflight with Microsoft Word before publishing public release assets.
'@
    Set-Content -LiteralPath (Join-Path $stageWordVisualRoot "README.md") -Encoding UTF8 -Value $incompleteNote
} else {
    throw "Visual gate evidence is unavailable for packaging."
}

Write-Step "Sanitizing staged release materials"
Sanitize-StagedReleaseMaterials -RepoRoot $repoRoot -RootPaths @(
    $stageInstallRoot
    $stageReleaseCandidateRoot
    $stageWordVisualRoot
)

Write-Step "Auditing staged release materials"
& $releaseMaterialAuditScript -Path @(
    $stageInstallRoot
    $stageReleaseCandidateRoot
    $stageWordVisualRoot
)

$installZipPath = Join-Path $versionOutputDir ("FeatherDoc-v{0}-msvc-install.zip" -f $releaseVersion)
$galleryZipPath = Join-Path $versionOutputDir ("FeatherDoc-v{0}-visual-validation-gallery.zip" -f $releaseVersion)
$evidenceZipPath = Join-Path $versionOutputDir ("FeatherDoc-v{0}-release-evidence.zip" -f $releaseVersion)

Write-Step "Creating ZIP archives"
New-ZipArchive -SourcePaths @($stageInstallRoot) -ZipPath $installZipPath
New-ZipArchive -SourcePaths @($stageGalleryRoot) -ZipPath $galleryZipPath
New-ZipArchive -SourcePaths @($stageReleaseCandidateRoot, $stageWordVisualRoot) -ZipPath $evidenceZipPath

$releaseView = $null
if (-not [string]::IsNullOrWhiteSpace($UploadReleaseTag)) {
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        throw "GitHub CLI 'gh' is required when -UploadReleaseTag is used."
    }

    Write-Step "Uploading ZIP archives to GitHub release $UploadReleaseTag"
    & gh release upload $UploadReleaseTag --clobber $installZipPath $galleryZipPath $evidenceZipPath
    if ($LASTEXITCODE -ne 0) {
        throw "gh release upload failed with exit code ${LASTEXITCODE}."
    }

    $releaseViewJson = & gh release view $UploadReleaseTag --json tagName,url,assets
    if ($LASTEXITCODE -ne 0) {
        throw "gh release view failed after upload."
    }

    $releaseView = $releaseViewJson | ConvertFrom-Json
}

$manifestPath = Join-Path $versionOutputDir "release_assets_manifest.json"
$manifest = [ordered]@{
    generated_at = Get-Date -Format s
    workspace = $repoRoot
    summary_json = $resolvedSummaryPath
    release_version = $releaseVersion
    execution_status = $executionStatus
    visual_verdict = $visualVerdict
    install_prefix = $resolvedInstallPrefix
    readme_assets_dir = $resolvedReadmeAssetsDir
    gate_output_dir = $resolvedGateRoot
    output_dir = $versionOutputDir
    visual_gate_status = $visualGateStatus
    visual_gate_evidence_included = $hasVisualGateEvidence
    assets = @(
        (Get-AssetDescriptor -Path $installZipPath -Label "msvc_install")
        (Get-AssetDescriptor -Path $galleryZipPath -Label "visual_validation_gallery")
        (Get-AssetDescriptor -Path $evidenceZipPath -Label "release_evidence")
    )
    upload = [ordered]@{
        requested_tag = $UploadReleaseTag
        uploaded = ($null -ne $releaseView)
        release_url = if ($null -ne $releaseView) { Get-OptionalPropertyValue -Object $releaseView -Name "url" } else { "" }
    }
}

if ($null -ne $releaseView) {
    $releaseAssets = @()
    foreach ($asset in $manifest.assets) {
        $remote = @($releaseView.assets | Where-Object { $_.name -eq (Split-Path -Leaf $asset.path) }) | Select-Object -First 1
        if ($null -ne $remote) {
            $releaseAssets += [ordered]@{
                name = $remote.name
                url = $remote.url
                size_bytes = $remote.size
                download_count = $remote.downloadCount
            }
        }
    }

    $manifest.upload.remote_assets = $releaseAssets
}

($manifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $manifestPath -Encoding UTF8

if (-not $KeepStaging) {
    Remove-Item -LiteralPath $stagingRoot -Recurse -Force
}

Write-Host "Release assets directory: $versionOutputDir"
Write-Host "Asset manifest: $manifestPath"
Write-Host "MSVC install ZIP: $installZipPath"
Write-Host "Visual gallery ZIP: $galleryZipPath"
Write-Host "Release evidence ZIP: $evidenceZipPath"
if ($null -ne $releaseView) {
    Write-Host "Uploaded release: $(Get-OptionalPropertyValue -Object $releaseView -Name 'url')"
}
