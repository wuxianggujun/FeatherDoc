param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    return Join-Path $Root $RelativePath
}

function Get-RepoFileText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    $path = Resolve-RepoPath -Root $Root -RelativePath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required file is missing: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

function Assert-ContainsText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$ExpectedText,

        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    if (-not $Text.Contains($ExpectedText)) {
        throw $Message
    }
}

function Assert-NotContainsText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$UnexpectedText,

        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    if ($Text.Contains($UnexpectedText)) {
        throw $Message
    }
}

function Assert-ReleaseOutputRootCleanupContract {
    param(
        [Parameter(Mandatory = $true)]
        [string]$WorkflowText,

        [Parameter(Mandatory = $true)]
        [string]$WorkflowName
    )

    foreach ($marker in @(
            '$workspaceRoot = [System.IO.Path]::GetFullPath((Get-Location).Path)',
            '$resolvedReleaseOutputRoot = [System.IO.Path]::GetFullPath($env:RELEASE_OUTPUT_ROOT)',
            'Refusing to clean release output outside the workspace',
            'Test-Path -LiteralPath $resolvedReleaseOutputRoot',
            'Remove-Item -LiteralPath $resolvedReleaseOutputRoot -Recurse -Force',
            'New-Item -ItemType Directory -Path $resolvedReleaseOutputRoot -Force'
        )) {
        Assert-ContainsText -Text $WorkflowText -ExpectedText $marker `
            -Message "$WorkflowName should clean RELEASE_OUTPUT_ROOT inside the workspace before uploading release artifacts. Missing marker '$marker'."
    }

    foreach ($marker in @(
            '$releaseOutputRelativePath = [System.IO.Path]::GetRelativePath($workspaceRoot, $resolvedReleaseOutputRoot)',
            '$parentDirectoryPrefix = ".." + [System.IO.Path]::DirectorySeparatorChar',
            '$parentAltDirectoryPrefix = ".." + [System.IO.Path]::AltDirectorySeparatorChar',
            '$releaseOutputOutsideWorkspace = (',
            '[System.IO.Path]::IsPathRooted($releaseOutputRelativePath)',
            '$releaseOutputRelativePath -eq ".."',
            '$releaseOutputRelativePath.StartsWith($parentDirectoryPrefix, [System.StringComparison]::Ordinal)',
            '$releaseOutputRelativePath.StartsWith($parentAltDirectoryPrefix, [System.StringComparison]::Ordinal)',
            '$releaseOutputRelativePath -eq "." -or $releaseOutputOutsideWorkspace'
        )) {
        Assert-ContainsText -Text $WorkflowText -ExpectedText $marker `
            -Message "$WorkflowName should use relative-path containment checks before recursive cleanup. Missing marker '$marker'."
    }

    Assert-NotContainsText -Text $WorkflowText -UnexpectedText 'StartsWith($workspaceRoot, [System.StringComparison]::OrdinalIgnoreCase)' `
        -Message "$WorkflowName should not rely on string-prefix workspace checks before recursive cleanup."
}

function Assert-ReleaseOutputArtifactPathContract {
    param(
        [Parameter(Mandatory = $true)]
        [string]$WorkflowText,

        [Parameter(Mandatory = $true)]
        [string]$WorkflowName,

        [Parameter(Mandatory = $true)]
        [string]$ArtifactName
    )

    foreach ($marker in @(
            "RELEASE_OUTPUT_ROOT: output/release-assets",
            "name: $ArtifactName",
            '${{ env.RELEASE_OUTPUT_ROOT }}/**',
            "if-no-files-found: warn"
        )) {
        Assert-ContainsText -Text $WorkflowText -ExpectedText $marker `
            -Message "$WorkflowName should upload the generated RELEASE_OUTPUT_ROOT artifact. Missing marker '$marker'."
    }

    foreach ($unexpected in @(
            "output/release-assets/**",
            "output/release-assets-ci"
        )) {
        Assert-NotContainsText -Text $WorkflowText -UnexpectedText $unexpected `
            -Message "$WorkflowName should not upload a hard-coded or CI-preview release artifact path '$unexpected'."
    }
}

$resolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$workflowRelativePaths = @(
    ".github\workflows\windows-msvc.yml",
    ".github\workflows\linux-cmake.yml",
    ".github\workflows\macos-cmake.yml",
    ".github\workflows\release-refresh.yml",
    ".github\workflows\release-publish.yml"
)

$workflowTexts = @{}
foreach ($relativePath in $workflowRelativePaths) {
    $workflowTexts[$relativePath] = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath $relativePath
}

$allWorkflowText = ($workflowTexts.Values -join "`n")
foreach ($unexpected in @(
        "actions/checkout@v4",
        "actions/upload-artifact@v4",
        "ilammy/msvc-dev-cmd@",
        "runs-on: windows-latest"
    )) {
    Assert-NotContainsText -Text $allWorkflowText -UnexpectedText $unexpected `
        -Message "Workflow maintenance contract should not contain deprecated or unstable marker '$unexpected'."
}

foreach ($relativePath in $workflowRelativePaths) {
    Assert-ContainsText -Text $workflowTexts[$relativePath] -ExpectedText "uses: actions/checkout@v6" `
        -Message "Workflow '$relativePath' should use the Node 24-compatible checkout major."
}

foreach ($relativePath in $workflowRelativePaths) {
    if ($relativePath -eq ".github\workflows\release-publish.yml" -or
        $relativePath -eq ".github\workflows\release-refresh.yml" -or
        $relativePath -eq ".github\workflows\linux-cmake.yml" -or
        $relativePath -eq ".github\workflows\macos-cmake.yml" -or
        $relativePath -eq ".github\workflows\windows-msvc.yml") {
        Assert-ContainsText -Text $workflowTexts[$relativePath] -ExpectedText "uses: actions/upload-artifact@v7" `
            -Message "Workflow '$relativePath' should use the Node 24-compatible upload-artifact major."
    }
}

$windowsWorkflow = $workflowTexts[".github\workflows\windows-msvc.yml"]
foreach ($marker in @(
        "runs-on: windows-2022",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "vcvars64.bat",
        "GITHUB_ENV",
        "MSVC x64 environment exported"
    )) {
    Assert-ContainsText -Text $windowsWorkflow -ExpectedText $marker `
        -Message "Windows workflow should keep the inline MSVC setup marker '$marker'."
}

foreach ($marker in @(
        "Verify Ninja",
        'cmake -S . -B build-msvc-ninja -G "Ninja"',
        "cmake --build build-msvc-ninja --parallel 4 --verbose",
        "ctest --test-dir build-msvc-ninja --parallel 2 --output-on-failure --timeout 60",
        '-BuildDir build-msvc-ninja',
        '-Generator "Ninja"',
        "build-msvc-ninja/my_test.docx",
        "build-msvc-ninja/output/**/*.docx"
    )) {
    Assert-ContainsText -Text $windowsWorkflow -ExpectedText $marker `
        -Message "Windows workflow should keep the Ninja parallel build marker '$marker'."
}

$releaseMetadataStepStart = $windowsWorkflow.IndexOf("Generate release metadata bundle")
if ($releaseMetadataStepStart -lt 0) {
    throw "Windows workflow should keep the release metadata bundle step."
}

$releaseMetadataNextStep = $windowsWorkflow.IndexOf("Generate artifact root start-here note", $releaseMetadataStepStart)
if ($releaseMetadataNextStep -lt 0) {
    throw "Windows workflow should keep the artifact root start-here step after release metadata generation."
}

$releaseMetadataStep = $windowsWorkflow.Substring($releaseMetadataStepStart, $releaseMetadataNextStep - $releaseMetadataStepStart)
foreach ($marker in @(
        "run_release_candidate_checks.ps1",
        "-BuildDir build-msvc-ninja",
        "-InstallDir build-msvc-install",
        "-ConsumerBuildDir build-msvc-install-consumer",
        '-Generator "Ninja"'
    )) {
    Assert-ContainsText -Text $releaseMetadataStep -ExpectedText $marker `
        -Message "Windows release metadata step should keep the Ninja-compatible marker '$marker'."
}

foreach ($unexpected in @(
        "NMake Makefiles",
        "build-msvc-nmake"
    )) {
    Assert-NotContainsText -Text $windowsWorkflow -UnexpectedText $unexpected `
        -Message "Windows workflow should not regress to the serial NMake marker '$unexpected'."
}

foreach ($marker in @(
        "output/release-assets-ci/v*/release_assets_manifest.json",
        "output/release-assets-ci/v*/FeatherDoc-*-msvc-install.zip",
        "output/release-assets-ci/v*/FeatherDoc-*-visual-validation-gallery.zip",
        "output/release-assets-ci/v*/FeatherDoc-*-release-evidence.zip",
        "compression-level: 0"
    )) {
    Assert-ContainsText -Text $windowsWorkflow -ExpectedText $marker `
        -Message "Windows workflow should keep the release asset preview upload marker '$marker'."
}

$releasePublishWorkflow = $workflowTexts[".github\workflows\release-publish.yml"]
foreach ($marker in @(
        "allow-ci-artifact-publish:",
        "Allow publishing a CI artifact bundle when Word visual gate was skipped",
        "RELEASE_OUTPUT_ROOT: output/release-assets",
        '$visualGateStatus -in @("skipped", "visual_gate_skipped")',
        '$visualVerdict -in @("", "visual_gate_skipped", "pending_manual_review")',
        "Refusing Release Publish because visual_verdict is",
        "Refusing Release Publish because visual_gate.status is",
        '"-ExecutionPolicy", "Bypass"',
        '"-File", ".\scripts\publish_github_release.ps1"',
        '"-OutputRoot", $env:RELEASE_OUTPUT_ROOT',
        '"-Publish"',
        "-AllowCiArtifactPublish",
        "name: release-publish-output",
        '${{ env.RELEASE_OUTPUT_ROOT }}/**'
    )) {
    Assert-ContainsText -Text $releasePublishWorkflow -ExpectedText $marker `
        -Message "Release Publish workflow should keep the CI artifact boundary marker '$marker'."
}
Assert-ReleaseOutputRootCleanupContract -WorkflowText $releasePublishWorkflow -WorkflowName "Release Publish workflow"
Assert-ReleaseOutputArtifactPathContract -WorkflowText $releasePublishWorkflow -WorkflowName "Release Publish workflow" -ArtifactName "release-publish-output"

$releaseRefreshWorkflow = $workflowTexts[".github\workflows\release-refresh.yml"]
foreach ($marker in @(
        "workflow_dispatch:",
        "contents: write",
        "RELEASE_OUTPUT_ROOT: output/release-assets",
        "-File .\scripts\publish_github_release.ps1",
        '-OutputRoot $env:RELEASE_OUTPUT_ROOT',
        "name: release-refresh-output",
        '${{ env.RELEASE_OUTPUT_ROOT }}/**'
    )) {
    Assert-ContainsText -Text $releaseRefreshWorkflow -ExpectedText $marker `
        -Message "Release Refresh workflow should keep the release output artifact marker '$marker'."
}
Assert-ReleaseOutputRootCleanupContract -WorkflowText $releaseRefreshWorkflow -WorkflowName "Release Refresh workflow"
Assert-ReleaseOutputArtifactPathContract -WorkflowText $releaseRefreshWorkflow -WorkflowName "Release Refresh workflow" -ArtifactName "release-refresh-output"

Write-Host "GitHub Actions workflow maintenance contract passed."
