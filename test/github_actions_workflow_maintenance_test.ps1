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

Write-Host "GitHub Actions workflow maintenance contract passed."
