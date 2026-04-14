param(
    [string]$BuildDir = "build-template-validation-regression-nmake",
    [string]$OutputDir = "output/template-validation-regression",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[template-validation-regression] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        return [System.IO.Path]::GetFullPath($InputPath)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}

function Get-VcvarsPath {
    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "Could not locate vcvars64.bat for MSVC command-line builds."
}

function Invoke-MsvcCommand {
    param(
        [string]$VcvarsPath,
        [string]$CommandText
    )

    & cmd.exe /c "call `"$VcvarsPath`" && $CommandText"
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC command failed: $CommandText"
    }
}

function Find-BuildExecutable {
    param(
        [string]$BuildRoot,
        [string]$TargetName
    )

    $candidates = Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } |
        Sort-Object LastWriteTime -Descending

    if (-not $candidates) {
        throw "Could not find built executable for target '$TargetName' under $BuildRoot."
    }

    return $candidates[0].FullName
}

function Invoke-CommandCapture {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureMessage
    )

    $commandOutput = @(& $ExecutablePath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
        $lines | Set-Content -Path $OutputPath -Encoding UTF8
    }
    foreach ($line in $lines) {
        Write-Host $line
    }
    if ($exitCode -ne 0) {
        throw $FailureMessage
    }
}

function Read-JsonFile {
    param([string]$Path)
    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Get-StringArray {
    param($Value)

    if ($null -eq $Value) {
        return ,@()
    }

    return ,@($Value | ForEach-Object { $_.ToString() })
}

function Test-StringArraysEqual {
    param(
        [string[]]$Left,
        [string[]]$Right
    )

    if ($Left.Count -ne $Right.Count) {
        return $false
    }

    for ($index = 0; $index -lt $Left.Count; $index++) {
        if ($Left[$index] -ne $Right[$index]) {
            return $false
        }
    }

    return $true
}

function Assert-ValidationResultMatches {
    param(
        $Expected,
        $Actual,
        [string]$Label
    )

    if ([bool]$Expected.passed -ne [bool]$Actual.passed) {
        throw "$Label passed flag mismatch."
    }

    $expectedMissing = Get-StringArray $Expected.missing_required
    $actualMissing = Get-StringArray $Actual.missing_required
    if (-not (Test-StringArraysEqual -Left $expectedMissing -Right $actualMissing)) {
        throw "$Label missing_required mismatch."
    }

    $expectedDuplicates = Get-StringArray $Expected.duplicate_bookmarks
    $actualDuplicates = Get-StringArray $Actual.duplicate_bookmarks
    if (-not (Test-StringArraysEqual -Left $expectedDuplicates -Right $actualDuplicates)) {
        throw "$Label duplicate_bookmarks mismatch."
    }

    $expectedMalformed = Get-StringArray $Expected.malformed_placeholders
    $actualMalformed = Get-StringArray $Actual.malformed_placeholders
    if (-not (Test-StringArraysEqual -Left $expectedMalformed -Right $actualMalformed)) {
        throw "$Label malformed_placeholders mismatch."
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir

if (-not $SkipBuild) {
    $vcvarsPath = Get-VcvarsPath
    Write-Step "Configuring dedicated build directory $resolvedBuildDir"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building template validation regression targets"
    Invoke-MsvcCommand -VcvarsPath $vcvarsPath -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_sample_template_validation featherdoc_sample_part_template_validation featherdoc_cli"
}

$bodySampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_template_validation"
$partSampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_part_template_validation"
$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null

$reviewManifestPath = Join-Path $resolvedOutputDir "review_manifest.json"
$reviewChecklistPath = Join-Path $resolvedOutputDir "review_checklist.md"
$finalReviewPath = Join-Path $resolvedOutputDir "final_review.md"

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    visual_enabled = $false
    review_manifest = $reviewManifestPath
    review_checklist = $reviewChecklistPath
    final_review = $finalReviewPath
    cases = @()
}

$reviewManifest = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = $false
    cases = @()
}

$bodyCaseDir = Join-Path $resolvedOutputDir "body-template"
$bodySampleOutputDir = Join-Path $bodyCaseDir "sample-output"
$bodyValidDocxPath = Join-Path $bodySampleOutputDir "valid_template.docx"
$bodyInvalidDocxPath = Join-Path $bodySampleOutputDir "invalid_template.docx"
$bodySampleReportJsonPath = Join-Path $bodySampleOutputDir "validation_summary.json"
$bodySampleReportMarkdownPath = Join-Path $bodySampleOutputDir "validation_summary.md"
$bodyCliValidJsonPath = Join-Path $bodyCaseDir "cli_valid.json"
$bodyCliInvalidJsonPath = Join-Path $bodyCaseDir "cli_invalid.json"

New-Item -ItemType Directory -Path $bodyCaseDir -Force | Out-Null

Write-Step "Generating body template validation samples via $bodySampleExecutable"
& $bodySampleExecutable $bodySampleOutputDir
if ($LASTEXITCODE -ne 0) {
    throw "Sample execution failed: featherdoc_sample_template_validation"
}

Write-Step "Validating valid_template.docx through featherdoc_cli"
Invoke-CommandCapture `
    -ExecutablePath $cliExecutable `
    -Arguments @(
        "validate-template",
        $bodyValidDocxPath,
        "--part",
        "body",
        "--slot",
        "customer:text",
        "--slot",
        "intro_block:block",
        "--slot",
        "line_items:table_rows",
        "--slot",
        "summary_table:table",
        "--slot",
        "signature_image:image",
        "--slot",
        "seal_image:floating_image",
        "--json"
    ) `
    -OutputPath $bodyCliValidJsonPath `
    -FailureMessage "Failed to validate valid_template.docx through featherdoc_cli."

Write-Step "Validating invalid_template.docx through featherdoc_cli"
Invoke-CommandCapture `
    -ExecutablePath $cliExecutable `
    -Arguments @(
        "validate-template",
        $bodyInvalidDocxPath,
        "--part",
        "body",
        "--slot",
        "customer:text",
        "--slot",
        "invoice:text",
        "--slot",
        "summary_block:block",
        "--slot",
        "line_items:table_rows",
        "--json"
    ) `
    -OutputPath $bodyCliInvalidJsonPath `
    -FailureMessage "Failed to validate invalid_template.docx through featherdoc_cli."

$bodySampleReport = Read-JsonFile -Path $bodySampleReportJsonPath
$bodyCliValid = Read-JsonFile -Path $bodyCliValidJsonPath
$bodyCliInvalid = Read-JsonFile -Path $bodyCliInvalidJsonPath

Assert-ValidationResultMatches -Expected $bodySampleReport.valid_template -Actual $bodyCliValid -Label "body valid template"
Assert-ValidationResultMatches -Expected $bodySampleReport.invalid_template -Actual $bodyCliInvalid -Label "body invalid template"

$bodyCaseSummary = [ordered]@{
    id = "body-template"
    description = "Body template sample outputs and featherdoc_cli validation must agree for both the valid and invalid body schemas."
    sample_executable = $bodySampleExecutable
    cli_executable = $cliExecutable
    sample_output_dir = $bodySampleOutputDir
    valid_docx_path = $bodyValidDocxPath
    invalid_docx_path = $bodyInvalidDocxPath
    sample_report_json = $bodySampleReportJsonPath
    sample_report_markdown = $bodySampleReportMarkdownPath
    cli_valid_json = $bodyCliValidJsonPath
    cli_invalid_json = $bodyCliInvalidJsonPath
    sample_results = [ordered]@{
        valid = $bodySampleReport.valid_template
        invalid = $bodySampleReport.invalid_template
    }
    cli_results = [ordered]@{
        valid = $bodyCliValid
        invalid = $bodyCliInvalid
    }
}

$summary.cases += $bodyCaseSummary
$reviewManifest.cases += $bodyCaseSummary

$partCaseDir = Join-Path $resolvedOutputDir "part-template"
$partSampleOutputDir = Join-Path $partCaseDir "sample-output"
$partDocxPath = Join-Path $partSampleOutputDir "part_template_validation.docx"
$partSampleReportJsonPath = Join-Path $partSampleOutputDir "validation_summary.json"
$partSampleReportMarkdownPath = Join-Path $partSampleOutputDir "validation_summary.md"
$partCliHeaderJsonPath = Join-Path $partCaseDir "cli_header.json"
$partCliFooterJsonPath = Join-Path $partCaseDir "cli_footer.json"

New-Item -ItemType Directory -Path $partCaseDir -Force | Out-Null

Write-Step "Generating part template validation samples via $partSampleExecutable"
& $partSampleExecutable $partSampleOutputDir
if ($LASTEXITCODE -ne 0) {
    throw "Sample execution failed: featherdoc_sample_part_template_validation"
}

Write-Step "Validating header part through featherdoc_cli"
Invoke-CommandCapture `
    -ExecutablePath $cliExecutable `
    -Arguments @(
        "validate-template",
        $partDocxPath,
        "--part",
        "header",
        "--index",
        "0",
        "--slot",
        "header_title:text",
        "--slot",
        "header_note:block",
        "--slot",
        "header_rows:table_rows",
        "--json"
    ) `
    -OutputPath $partCliHeaderJsonPath `
    -FailureMessage "Failed to validate header template through featherdoc_cli."

Write-Step "Validating section footer part through featherdoc_cli"
Invoke-CommandCapture `
    -ExecutablePath $cliExecutable `
    -Arguments @(
        "validate-template",
        $partDocxPath,
        "--part",
        "section-footer",
        "--section",
        "0",
        "--kind",
        "default",
        "--slot",
        "footer_company:text",
        "--slot",
        "footer_summary:block",
        "--slot",
        "footer_signature:text",
        "--json"
    ) `
    -OutputPath $partCliFooterJsonPath `
    -FailureMessage "Failed to validate footer template through featherdoc_cli."

$partSampleReport = Read-JsonFile -Path $partSampleReportJsonPath
$partCliHeader = Read-JsonFile -Path $partCliHeaderJsonPath
$partCliFooter = Read-JsonFile -Path $partCliFooterJsonPath

Assert-ValidationResultMatches -Expected $partSampleReport.header -Actual $partCliHeader -Label "part header template"
Assert-ValidationResultMatches -Expected $partSampleReport.footer -Actual $partCliFooter -Label "part footer template"

$partCaseSummary = [ordered]@{
    id = "part-template"
    description = "Header/footer template sample outputs and featherdoc_cli validation must agree for both the valid header schema and invalid footer schema."
    sample_executable = $partSampleExecutable
    cli_executable = $cliExecutable
    sample_output_dir = $partSampleOutputDir
    docx_path = $partDocxPath
    sample_report_json = $partSampleReportJsonPath
    sample_report_markdown = $partSampleReportMarkdownPath
    cli_header_json = $partCliHeaderJsonPath
    cli_footer_json = $partCliFooterJsonPath
    sample_results = [ordered]@{
        header = $partSampleReport.header
        footer = $partSampleReport.footer
    }
    cli_results = [ordered]@{
        header = $partCliHeader
        footer = $partCliFooter
    }
}

$summary.cases += $partCaseSummary
$reviewManifest.cases += $partCaseSummary

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8
($reviewManifest | ConvertTo-Json -Depth 8) | Set-Content -Path $reviewManifestPath -Encoding UTF8

$reviewChecklistLines = @(
    "# Template validation regression checklist",
    "",
    "- body-template: compare the sample report against featherdoc_cli for both valid_template.docx and invalid_template.docx.",
    "- part-template: compare the sample report against featherdoc_cli for the valid header and invalid footer schema checks.",
    "- Verify all generated JSON artifacts stay machine-readable and keep the expected entry_name values.",
    "",
    "Artifacts:",
    "",
    "- Summary JSON: $summaryPath",
    "- Review manifest: $reviewManifestPath",
    "- Final review: $finalReviewPath"
)
$reviewChecklistLines | Set-Content -Path $reviewChecklistPath -Encoding UTF8

$finalReviewLines = @(
    "# Template validation final review",
    "",
    "- Generated at: $(Get-Date -Format s)",
    "- Summary JSON: $summaryPath",
    "- Review manifest: $reviewManifestPath",
    "",
    "## Verdict",
    "",
    "- Overall verdict:",
    "- Notes:",
    "",
    "## Case findings",
    "",
    "- body-template:",
    "  Verdict:",
    "  Notes:",
    "",
    "- part-template:",
    "  Verdict:",
    "  Notes:"
)
$finalReviewLines | Set-Content -Path $finalReviewPath -Encoding UTF8

Write-Step "Completed template validation regression run"
Write-Host "Summary: $summaryPath"
Write-Host "Review manifest: $reviewManifestPath"
Write-Host "Review checklist: $reviewChecklistPath"
foreach ($case in $summary.cases) {
    Write-Host "Case: $($case.id)"
    if ($case.PSObject.Properties.Name -contains "docx_path") {
        Write-Host "  DOCX: $($case.docx_path)"
    }
    if ($case.PSObject.Properties.Name -contains "valid_docx_path") {
        Write-Host "  Valid DOCX: $($case.valid_docx_path)"
        Write-Host "  Invalid DOCX: $($case.invalid_docx_path)"
    }
    Write-Host "  Sample report: $($case.sample_report_json)"
}
