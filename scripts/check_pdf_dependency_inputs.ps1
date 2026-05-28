<#
.SYNOPSIS
Checks local PDFio and PDFium inputs before enabling the experimental PDF build.

.DESCRIPTION
This script is intentionally read-only. It does not run CMake, CTest, Ninja,
MSBuild, dependency downloads, PDF rendering, or browser automation. It only
checks whether the local paths needed by FEATHERDOC_BUILD_PDF and
FEATHERDOC_BUILD_PDF_IMPORT are present.
#>
param(
    [string]$PdfioSourceDir = "tmp\pdfio-src",
    [ValidateSet("auto", "source", "prebuilt", "package")]
    [string]$PdfiumProvider = "auto",
    [string]$PdfiumSourceDir = "tmp\pdfium-workspace\pdfium",
    [string]$PdfiumPrebuiltRoot = "",
    [string]$PdfiumLibrary = "",
    [string]$PdfiumIncludeDir = "",
    [string]$PdfiumRuntimeDll = "",
    [string]$PdfiumRuntimeDir = "",
    [string]$OutputJson = "",
    [switch]$Strict
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $Path))
}

function Get-FirstExistingFile {
    param([string[]]$Paths)

    foreach ($path in $Paths) {
        if (-not [string]::IsNullOrWhiteSpace($path) -and
            (Test-Path -LiteralPath $path -PathType Leaf)) {
            return $path
        }
    }

    if ($Paths.Count -gt 0) {
        return $Paths[0]
    }
    return ""
}

function New-CheckResult {
    param(
        [string]$Name,
        [string]$Status,
        [bool]$Required,
        [string]$Message,
        [string]$Path = "",
        [object]$Details = $null
    )

    $result = [ordered]@{
        name = $Name
        status = $Status
        required = $Required
        message = $Message
    }
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        $result.path = $Path
    }
    if ($null -ne $Details) {
        $result.details = $Details
    }
    return $result
}

function Add-CheckResult {
    param(
        [System.Collections.ArrayList]$Checks,
        [string]$Name,
        [string]$Status,
        [bool]$Required,
        [string]$Message,
        [string]$Path = "",
        [object]$Details = $null
    )

    [void]$Checks.Add((New-CheckResult `
        -Name $Name `
        -Status $Status `
        -Required $Required `
        -Message $Message `
        -Path $Path `
        -Details $Details))
}

$repoRoot = Resolve-RepoRoot
$checks = New-Object System.Collections.ArrayList
$missingInputs = New-Object System.Collections.ArrayList

$resolvedPdfioSourceDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $PdfioSourceDir
$pdfioHeader = if ([string]::IsNullOrWhiteSpace($resolvedPdfioSourceDir)) {
    ""
} else {
    Join-Path $resolvedPdfioSourceDir "pdfio.h"
}
$pdfioReady = (-not [string]::IsNullOrWhiteSpace($pdfioHeader)) -and
    (Test-Path -LiteralPath $pdfioHeader -PathType Leaf)

if ($pdfioReady) {
    Add-CheckResult -Checks $checks `
        -Name "pdfio_source_header_exists" `
        -Status "pass" `
        -Required $true `
        -Message "PDFio source input contains pdfio.h." `
        -Path $pdfioHeader
} else {
    [void]$missingInputs.Add("PDFio source header: $pdfioHeader")
    Add-CheckResult -Checks $checks `
        -Name "pdfio_source_header_exists" `
        -Status "missing" `
        -Required $true `
        -Message "PDFio source input does not contain pdfio.h." `
        -Path $pdfioHeader
}

$resolvedPdfiumSourceDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $PdfiumSourceDir
$pdfiumSourceHeader = if ([string]::IsNullOrWhiteSpace($resolvedPdfiumSourceDir)) {
    ""
} else {
    Join-Path $resolvedPdfiumSourceDir "public\fpdfview.h"
}
$pdfiumSourceReady = (-not [string]::IsNullOrWhiteSpace($pdfiumSourceHeader)) -and
    (Test-Path -LiteralPath $pdfiumSourceHeader -PathType Leaf)

$resolvedPdfiumPrebuiltRoot = Resolve-RepoPath -RepoRoot $repoRoot -Path $PdfiumPrebuiltRoot
$pdfiumPrebuiltRootExists = (-not [string]::IsNullOrWhiteSpace($resolvedPdfiumPrebuiltRoot)) -and
    (Test-Path -LiteralPath $resolvedPdfiumPrebuiltRoot -PathType Container)
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfiumPrebuiltRoot)) {
    if ([string]::IsNullOrWhiteSpace($PdfiumLibrary)) {
        $PdfiumLibrary = Get-FirstExistingFile -Paths @(
            (Join-Path $resolvedPdfiumPrebuiltRoot "lib\pdfium.dll.lib"),
            (Join-Path $resolvedPdfiumPrebuiltRoot "lib\pdfium.lib")
        )
    }
    if ([string]::IsNullOrWhiteSpace($PdfiumIncludeDir)) {
        $PdfiumIncludeDir = Join-Path $resolvedPdfiumPrebuiltRoot "include"
    }
    if ([string]::IsNullOrWhiteSpace($PdfiumRuntimeDll) -and
        [string]::IsNullOrWhiteSpace($PdfiumRuntimeDir)) {
        $PdfiumRuntimeDll = Join-Path $resolvedPdfiumPrebuiltRoot "bin\pdfium.dll"
    }
}

$resolvedPdfiumLibrary = Resolve-RepoPath -RepoRoot $repoRoot -Path $PdfiumLibrary
$resolvedPdfiumIncludeDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $PdfiumIncludeDir
$resolvedPdfiumRuntimeDll = Resolve-RepoPath -RepoRoot $repoRoot -Path $PdfiumRuntimeDll
$resolvedPdfiumRuntimeDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $PdfiumRuntimeDir
$pdfiumPrebuiltHeader = if ([string]::IsNullOrWhiteSpace($resolvedPdfiumIncludeDir)) {
    ""
} else {
    Join-Path $resolvedPdfiumIncludeDir "fpdfview.h"
}
$pdfiumPrebuiltLibraryExists = (-not [string]::IsNullOrWhiteSpace($resolvedPdfiumLibrary)) -and
    (Test-Path -LiteralPath $resolvedPdfiumLibrary -PathType Leaf)
$pdfiumPrebuiltHeaderExists = (-not [string]::IsNullOrWhiteSpace($pdfiumPrebuiltHeader)) -and
    (Test-Path -LiteralPath $pdfiumPrebuiltHeader -PathType Leaf)
$pdfiumRuntimeDllExists = [string]::IsNullOrWhiteSpace($resolvedPdfiumRuntimeDll) -or
    (Test-Path -LiteralPath $resolvedPdfiumRuntimeDll -PathType Leaf)
$pdfiumRuntimeDirExists = [string]::IsNullOrWhiteSpace($resolvedPdfiumRuntimeDir) -or
    (Test-Path -LiteralPath $resolvedPdfiumRuntimeDir -PathType Container)
$pdfiumPrebuiltReady = $pdfiumPrebuiltLibraryExists -and
    $pdfiumPrebuiltHeaderExists -and
    $pdfiumRuntimeDllExists -and
    $pdfiumRuntimeDirExists

$selectedPdfiumProvider = ""
$pdfiumReady = $false
switch ($PdfiumProvider) {
    "source" {
        $selectedPdfiumProvider = "source"
        $pdfiumReady = $pdfiumSourceReady
    }
    "prebuilt" {
        $selectedPdfiumProvider = "prebuilt"
        $pdfiumReady = $pdfiumPrebuiltReady
    }
    "package" {
        $selectedPdfiumProvider = "package"
        $pdfiumReady = $false
    }
    default {
        if ($pdfiumPrebuiltReady) {
            $selectedPdfiumProvider = "prebuilt"
            $pdfiumReady = $true
        } elseif ($pdfiumSourceReady) {
            $selectedPdfiumProvider = "source"
            $pdfiumReady = $true
        } else {
            $selectedPdfiumProvider = "unresolved"
            $pdfiumReady = $false
        }
    }
}

$sourceRequired = ($PdfiumProvider -eq "source") -or
    (($PdfiumProvider -eq "auto") -and (-not $pdfiumPrebuiltReady))
$sourceStatus = if ($pdfiumSourceReady) {
    "pass"
} elseif ($sourceRequired) {
    "missing"
} else {
    "skipped"
}
Add-CheckResult -Checks $checks `
    -Name "pdfium_source_header_exists" `
    -Status $sourceStatus `
    -Required ([bool]$sourceRequired) `
    -Message $(if ($pdfiumSourceReady) { "PDFium source input contains public/fpdfview.h." } else { "PDFium source input does not contain public/fpdfview.h." }) `
    -Path $pdfiumSourceHeader

if ($sourceRequired -and -not $pdfiumSourceReady) {
    [void]$missingInputs.Add("PDFium source header: $pdfiumSourceHeader")
}

$prebuiltRequired = $PdfiumProvider -eq "prebuilt"
$prebuiltStatus = if ($pdfiumPrebuiltReady) {
    "pass"
} elseif ($prebuiltRequired) {
    "missing"
} else {
    "skipped"
}
Add-CheckResult -Checks $checks `
    -Name "pdfium_prebuilt_inputs_exist" `
    -Status $prebuiltStatus `
    -Required ([bool]$prebuiltRequired) `
    -Message $(if ($pdfiumPrebuiltReady) { "PDFium prebuilt library and include inputs are present." } else { "PDFium prebuilt inputs are incomplete." }) `
    -Details ([ordered]@{
        prebuilt_root = $resolvedPdfiumPrebuiltRoot
        prebuilt_root_exists = [bool]$pdfiumPrebuiltRootExists
        library = $resolvedPdfiumLibrary
        library_exists = [bool]$pdfiumPrebuiltLibraryExists
        include_dir = $resolvedPdfiumIncludeDir
        header = $pdfiumPrebuiltHeader
        header_exists = [bool]$pdfiumPrebuiltHeaderExists
        runtime_dll = $resolvedPdfiumRuntimeDll
        runtime_dll_exists = [bool]$pdfiumRuntimeDllExists
        runtime_dir = $resolvedPdfiumRuntimeDir
        runtime_dir_exists = [bool]$pdfiumRuntimeDirExists
    })

if ($prebuiltRequired -and -not $pdfiumPrebuiltReady) {
    if (-not $pdfiumPrebuiltLibraryExists) {
        [void]$missingInputs.Add("PDFium prebuilt library: $resolvedPdfiumLibrary")
    }
    if (-not $pdfiumPrebuiltHeaderExists) {
        [void]$missingInputs.Add("PDFium prebuilt header: $pdfiumPrebuiltHeader")
    }
    if (-not $pdfiumRuntimeDllExists) {
        [void]$missingInputs.Add("PDFium runtime DLL: $resolvedPdfiumRuntimeDll")
    }
    if (-not $pdfiumRuntimeDirExists) {
        [void]$missingInputs.Add("PDFium runtime directory: $resolvedPdfiumRuntimeDir")
    }
}

Add-CheckResult -Checks $checks `
    -Name "pdfium_package_provider_unchecked" `
    -Status "skipped" `
    -Required $false `
    -Message "Package-provider discovery requires CMake find_package(PDFium) and is not executed by this read-only input check."

if ($PdfiumProvider -eq "package") {
    [void]$missingInputs.Add("PDFium package discovery is unchecked by this script; run CMake or provide source/prebuilt inputs.")
}
if (($PdfiumProvider -eq "auto") -and (-not $pdfiumReady)) {
    [void]$missingInputs.Add("PDFium input: provide prebuilt library/include inputs or a source checkout with public/fpdfview.h.")
}

$ready = $pdfioReady -and $pdfiumReady
$summary = [ordered]@{
    schema = "featherdoc.pdf_dependency_inputs_check.v1"
    generated_at = (Get-Date).ToString("s")
    status = if ($ready) { "ready" } else { "not_ready" }
    repo_root = $repoRoot
    pdfio_ready = [bool]$pdfioReady
    pdfio_source_dir = $resolvedPdfioSourceDir
    pdfio_header = $pdfioHeader
    pdfium_provider = $PdfiumProvider
    selected_pdfium_provider = $selectedPdfiumProvider
    pdfium_ready = [bool]$pdfiumReady
    pdfium_source_dir = $resolvedPdfiumSourceDir
    pdfium_source_header = $pdfiumSourceHeader
    pdfium_prebuilt_root = $resolvedPdfiumPrebuiltRoot
    pdfium_prebuilt_root_exists = [bool]$pdfiumPrebuiltRootExists
    pdfium_library = $resolvedPdfiumLibrary
    pdfium_include_dir = $resolvedPdfiumIncludeDir
    pdfium_runtime_dll = $resolvedPdfiumRuntimeDll
    pdfium_runtime_dir = $resolvedPdfiumRuntimeDir
    missing_input_count = @($missingInputs).Count
    missing_inputs = @($missingInputs)
    checks = @($checks)
}

if (-not [string]::IsNullOrWhiteSpace($OutputJson)) {
    $resolvedOutputJson = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputJson
    $outputDir = Split-Path -Parent $resolvedOutputJson
    if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    }
    ($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8
}

$summary | ConvertTo-Json -Depth 10
if ($Strict -and -not $ready) {
    exit 1
}
