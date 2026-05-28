<#
.SYNOPSIS
Runs the local FeatherDoc release-candidate preflight.

.DESCRIPTION
Builds and verifies the local MSVC release candidate pipeline, including
ctest, install/find_package smoke, the Word visual gate, and optional
repository README gallery refresh. It can also optionally verify a template
DOCX against a committed template-schema baseline.

.PARAMETER RefreshReadmeAssets
Refreshes docs/assets/readme from the latest screenshot-backed Word visual
evidence produced by the visual gate. This requires the visual gate to run.

.PARAMETER ReadmeAssetsDir
Target repository directory for the refreshed README gallery PNG files.

.PARAMETER SmokeReviewVerdict
Optional screenshot-backed verdict to pass into the Word visual smoke flow.

.PARAMETER FixedGridReviewVerdict
Optional screenshot-backed verdict to seed into the fixed-grid review task.

.PARAMETER SectionPageSetupReviewVerdict
Optional screenshot-backed verdict to seed into the section page setup review task.

.PARAMETER PageNumberFieldsReviewVerdict
Optional screenshot-backed verdict to seed into the page number fields review task.

.PARAMETER CuratedVisualReviewVerdict
Optional screenshot-backed verdict to seed into curated visual regression review tasks.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 `
    -SkipConfigure `
    -SkipBuild `
    -BuildDir build-msvc-nmake `
    -InstallDir build-msvc-install-release-checks `
    -ConsumerBuildDir build-msvc-install-consumer-release-checks `
    -GateOutputDir output/word-visual-release-gate-release-checks `
    -TaskOutputRoot output/word-visual-smoke/tasks-release-checks `
    -SummaryOutputDir output/release-candidate-checks `
    -RefreshReadmeAssets

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 `
    -SkipConfigure `
    -SkipBuild `
    -BuildDir build-codex-clang-column-visual-verify `
    -TemplateSchemaInputDocx output\template-schema-validation-smoke\template_schema_validation_two_sections.docx `
    -TemplateSchemaBaseline output\template-schema-validation-smoke\script_frozen_template_schema.json `
    -TemplateSchemaResolvedSectionTargets
#>
param(
    [string]$BuildDir = "build-msvc-nmake",
    [string]$InstallDir = "build-msvc-install",
    [string]$ConsumerBuildDir = "build-msvc-install-consumer",
    [string]$GateOutputDir = "output/word-visual-release-gate",
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks-release-gate",
    [string]$SummaryOutputDir = "output/release-candidate-checks",
    [string]$Generator = "NMake Makefiles",
    [string]$Config = "Release",
    [int]$CtestTimeoutSeconds = 60,
    [int]$Dpi = 144,
    [switch]$SkipConfigure,
    [switch]$SkipBuild,
    [switch]$SkipTests,
    [switch]$SkipInstallSmoke,
    [switch]$SkipVisualGate,
    [switch]$SkipSectionPageSetup,
    [switch]$SkipPageNumberFields,
    [switch]$IncludeTableStyleQuality,
    [string]$TemplateSchemaInputDocx = "",
    [string]$TemplateSchemaBaseline = "",
    [string]$TemplateSchemaGeneratedOutput = "",
    [switch]$TemplateSchemaSectionTargets,
    [switch]$TemplateSchemaResolvedSectionTargets,
    [string]$TemplateSchemaManifestPath = "",
    [string]$TemplateSchemaManifestOutputDir = "",
    [string]$ProjectTemplateSmokeManifestPath = "",
    [string]$ProjectTemplateSmokeOutputDir = "",
    [switch]$ProjectTemplateSmokeRequireFullCoverage,
    [string[]]$ReleaseBlockerRollupInputJson = @(),
    [string[]]$ReleaseBlockerRollupInputRoot = @(),
    [switch]$ReleaseBlockerRollupAutoDiscover,
    [string]$ReleaseBlockerRollupAutoDiscoverRoot = "output",
    [string]$ReleaseBlockerRollupOutputDir = "",
    [switch]$ReleaseBlockerRollupFailOnBlocker,
    [switch]$ReleaseBlockerRollupFailOnWarning,
    [switch]$ReleaseGovernanceHandoff,
    [string]$ReleaseGovernanceHandoffInputRoot = "output",
    [string[]]$ReleaseGovernanceHandoffInputJson = @(),
    [string]$ReleaseGovernanceHandoffOutputDir = "",
    [switch]$ReleaseGovernanceHandoffIncludeRollup,
    [switch]$ReleaseGovernanceHandoffFailOnMissing,
    [switch]$ReleaseGovernanceHandoffFailOnBlocker,
    [switch]$ReleaseGovernanceHandoffFailOnWarning,
    [ValidateSet("full", "explicit-only")]
    [string]$ReleaseGovernanceHandoffExpectedReportProfile = "full",
    [ValidateSet("full", "pdf-only")]
    [string]$ReleaseEvidenceScope = "full",
    [string]$PdfVisualGateSummaryJson = "",
    [string]$PdfVisualGateAttemptSummaryJson = "",
    [string]$PdfVisualSegmentedGateSummaryJson = "",
    [string[]]$PdfBoundedCtestSummaryJson = @(),
    [string]$PdfReleaseReadinessSummaryJson = "",
    [switch]$SkipReviewTasks,
    [ValidateSet("review-only", "review-and-repair")]
    [string]$ReviewMode = "review-only",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$SmokeReviewVerdict = "undecided",
    [string]$SmokeReviewNote = "",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$FixedGridReviewVerdict = "undecided",
    [string]$FixedGridReviewNote = "",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$SectionPageSetupReviewVerdict = "undecided",
    [string]$SectionPageSetupReviewNote = "",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$PageNumberFieldsReviewVerdict = "undecided",
    [string]$PageNumberFieldsReviewNote = "",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$CuratedVisualReviewVerdict = "undecided",
    [string]$CuratedVisualReviewNote = "",
    [switch]$RefreshReadmeAssets,
    [string]$ReadmeAssetsDir = "docs/assets/readme",
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "release_blocker_metadata_helpers.ps1")
. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[release-candidate-checks] $Message"
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

function Get-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Convert-ReleaseMaterialString {
    param(
        [string]$RepoRoot,
        [AllowNull()][string]$Text
    )

    if ($null -eq $Text) {
        return $null
    }

    if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
        return $Text
    }

    $normalized = [string]$Text
    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $repoRootForms = @(
        $resolvedRepoRoot,
        ($resolvedRepoRoot -replace '\\', '/'),
        ($resolvedRepoRoot -replace '\\', '\\')
    ) | Sort-Object -Unique

    foreach ($rootForm in $repoRootForms) {
        if ([string]::IsNullOrWhiteSpace($rootForm)) {
            continue
        }

        $pattern = [regex]::Escape($rootForm) + '(?<suffix>\\\\|[\\/]|(?=$|[\s"''`<>|;,)]+))'
        $normalized = [regex]::Replace(
            $normalized,
            $pattern,
            '.${suffix}',
            [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    }

    return $normalized.Replace('./', '.\')
}

function Convert-ReleaseMaterialObject {
    param(
        [string]$RepoRoot,
        [AllowNull()]$Value
    )

    if ($null -eq $Value) {
        return $null
    }

    if ($Value -is [string]) {
        return Convert-ReleaseMaterialString -RepoRoot $RepoRoot -Text $Value
    }

    if ($Value -is [System.Collections.IDictionary]) {
        $converted = [ordered]@{}
        foreach ($key in $Value.Keys) {
            $converted[$key] = Convert-ReleaseMaterialObject -RepoRoot $RepoRoot -Value $Value[$key]
        }

        return $converted
    }

    if ($Value -is [pscustomobject]) {
        $converted = [ordered]@{}
        foreach ($property in $Value.PSObject.Properties) {
            $converted[$property.Name] = Convert-ReleaseMaterialObject -RepoRoot $RepoRoot -Value $property.Value
        }

        return $converted
    }

    if ($Value -is [System.Collections.IEnumerable]) {
        return ,@(
            foreach ($item in $Value) {
                Convert-ReleaseMaterialObject -RepoRoot $RepoRoot -Value $item
            }
        )
    }

    return $Value
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

function Get-MsvcBootstrap {
    $existingCl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($env:VSCMD_VER -or $env:VCINSTALLDIR -or $existingCl) {
        return [ordered]@{
            mode = "current_env"
            vcvars_path = ""
        }
    }

    $vswherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswherePath) {
        $installationPath = & $vswherePath `
            -latest `
            -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($installationPath)) {
            $vcvarsPath = Join-Path $installationPath.Trim() "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvarsPath) {
                return [ordered]@{
                    mode = "vcvars"
                    vcvars_path = $vcvarsPath
                }
            }
        }
    }

    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return [ordered]@{
                mode = "vcvars"
                vcvars_path = $path
            }
        }
    }

    throw "Could not locate vcvars64.bat and no active MSVC developer environment was detected."
}

function Invoke-MsvcCommand {
    param(
        $MsvcBootstrap,
        [string]$CommandText
    )

    # Keep native CLI JSON output decodable when CTest launches PowerShell
    # scripts from the MSVC cmd environment.
    $utf8CommandText = "chcp 65001 >NUL && $CommandText"
    if ($MsvcBootstrap.mode -eq "current_env") {
        & cmd.exe /c $utf8CommandText
    } else {
        & cmd.exe /c "call `"$($MsvcBootstrap.vcvars_path)`" && $utf8CommandText"
    }

    if ($LASTEXITCODE -ne 0) {
        throw "MSVC command failed: $CommandText"
    }
}

function Invoke-ChildPowerShell {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Unable to resolve a PowerShell executable for the nested script invocation."
        }
        $powerShellPath = $powerShellCommand.Source
    }

    $commandOutput = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }

    return @($commandOutput | ForEach-Object { $_.ToString() })
}

function Convert-ToCmdArgument {
    param([string]$Value)

    if ($Value -notmatch '[\s"]') {
        return $Value
    }

    return "`"$Value`""
}

function Invoke-ChildPowerShellInMsvcEnv {
    param(
        $MsvcBootstrap,
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    if ($MsvcBootstrap.mode -eq "current_env") {
        return Invoke-ChildPowerShell -ScriptPath $ScriptPath `
            -Arguments $Arguments `
            -FailureMessage $FailureMessage
    }

    $commandParts = @(
        "call"
        (Convert-ToCmdArgument -Value $MsvcBootstrap.vcvars_path)
        "&&"
        "powershell.exe"
        "-ExecutionPolicy"
        "Bypass"
        "-File"
        (Convert-ToCmdArgument -Value $ScriptPath)
    ) + ($Arguments | ForEach-Object { Convert-ToCmdArgument -Value $_ })

    $commandText = $commandParts -join " "
    $commandOutput = @(& cmd.exe /c $commandText 2>&1)
    $exitCode = $LASTEXITCODE

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }

    return @($commandOutput | ForEach-Object { $_.ToString() })
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path $Path)) {
        throw "Expected $Label was not found: $Path"
    }
}

function Assert-BuildExecutablePresent {
    param(
        [string]$BuildRoot,
        [string]$ExecutableStem,
        [string]$Label
    )

    $candidate = Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object {
            $_.Name -ieq $ExecutableStem -or $_.Name -ieq "$ExecutableStem.exe"
        } |
        Select-Object -First 1

    if (-not $candidate) {
        throw "Build directory is missing $Label ($ExecutableStem). Re-run without -SkipBuild or point to a fully built MSVC tree."
    }

    return $candidate.FullName
}

function Get-OptionalPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary] -and $Object.Contains($Name)) {
        return $Object[$Name]
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}


function Set-OptionalSummaryValue {
    param(
        [System.Collections.IDictionary]$Target,
        [string]$Name,
        $Value
    )

    if ($null -ne $Value -and -not [string]::IsNullOrWhiteSpace([string]$Value)) {
        $Target[$Name] = $Value
    }
}

function Get-PdfVisualGateSummaryInfo {
    param(
        [string]$SummaryJson
    )

    $info = [ordered]@{
        requested = -not [string]::IsNullOrWhiteSpace($SummaryJson)
        status = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { "not_requested" } elseif (Test-Path -LiteralPath $SummaryJson) { "available" } else { "missing" }
        summary_json = $SummaryJson
        full_visual_gate_status = ""
        verdict = ""
        aggregate_contact_sheet = ""
        cjk_manifest_count = 0
        cjk_copy_search_count = 0
        cjk_copy_search_missing_text_count = 0
        visual_baseline_manifest_count = 0
        visual_baseline_count = 0
        finalizable = $false
        error = ""
    }

    if (-not [bool]$info.requested -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return $info
    }

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryJson | ConvertFrom-Json
        $info.status = "loaded"
        $info.verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
        if ($info.verdict -in @("pass", "fail")) {
            $info.full_visual_gate_status = $info.verdict
        }
        $info.aggregate_contact_sheet = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet")
        $info.cjk_manifest_count = [int](Get-OptionalPropertyValue -Object $summary -Name "cjk_manifest_count")
        $info.cjk_copy_search_count = [int](Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_count")
        $cjkMissingTextCount = Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_missing_text_count"
        if ($null -eq $cjkMissingTextCount) {
            $cjkMissingTextCount = Get-OptionalPropertyValue -Object $summary -Name "cjk_missing_text_count"
        }
        if ($null -ne $cjkMissingTextCount -and -not [string]::IsNullOrWhiteSpace([string]$cjkMissingTextCount)) {
            $info.cjk_copy_search_missing_text_count = [int]$cjkMissingTextCount
        } else {
            $computedMissingTextCount = 0
            foreach ($entry in @(Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search")) {
                if ($null -eq $entry) {
                    continue
                }

                $missingText = Get-OptionalPropertyValue -Object $entry -Name "missing_text"
                if ($null -eq $missingText) {
                    continue
                }
                if ($missingText -is [string]) {
                    if (-not [string]::IsNullOrWhiteSpace($missingText)) {
                        $computedMissingTextCount += 1
                    }
                    continue
                }
                if ($missingText -is [System.Collections.IEnumerable]) {
                    $computedMissingTextCount += @($missingText | Where-Object {
                            $null -ne $_ -and -not [string]::IsNullOrWhiteSpace([string]$_)
                        }).Count
                    continue
                }

                $computedMissingTextCount += 1
            }

            $info.cjk_copy_search_missing_text_count = $computedMissingTextCount
        }
        $info.visual_baseline_manifest_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_manifest_count")
        $info.visual_baseline_count = [int](Get-OptionalPropertyValue -Object $summary -Name "baselines_count")
        $info.finalizable = -not [string]::IsNullOrWhiteSpace([string]$info.verdict) -and
            [int]$info.cjk_copy_search_count -gt 0 -and
            [int]$info.visual_baseline_count -gt 0 -and
            -not [string]::IsNullOrWhiteSpace([string]$info.aggregate_contact_sheet)
    } catch {
        $info.status = "unreadable"
        $info.error = $_.Exception.Message
    }

    return $info
}

function Get-PdfVisualGateAttemptSummaryInfo {
    param(
        [string]$SummaryJson
    )

    $info = [ordered]@{
        requested = -not [string]::IsNullOrWhiteSpace($SummaryJson)
        status = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { "not_requested" } elseif (Test-Path -LiteralPath $SummaryJson) { "available" } else { "missing" }
        summary_json = $SummaryJson
        verdict = ""
        full_visual_gate_status = ""
        evidence_scope = ""
        stage_count = 0
        passed_stage_count = 0
        failed_stage_count = 0
        incomplete_stage_count = 0
        pdf_cli_export_status = ""
        pdf_regression_status = ""
        pdf_regression_selected_test_count = 0
        pdf_regression_failed_test_count = 0
        pdf_regression_skipped_test_count = 0
        unicode_font_status = ""
        cjk_copy_search_status = ""
        cjk_copy_search_count = 0
        cjk_copy_search_missing_text_count = 0
        visual_baseline_render_status = ""
        visual_baseline_fresh_rendered_count = 0
        expected_visual_render_count = 0
        visual_baseline_fresh_missing_sample_count = 0
        visual_baseline_resume_needed = $false
        visual_baseline_resume_slice_offset = 0
        visual_baseline_resume_slice_limit = 0
        visual_baseline_resume_slice_command_template = ""
        outer_guard_status = ""
        outer_guard_timed_out = $false
        outer_guard_timeout_seconds = 0
        aggregate_contact_sheet_status = ""
        aggregate_contact_sheet = ""
        aggregate_contact_sheet_bytes = 0
        error = ""
    }

    if (-not [bool]$info.requested -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return $info
    }

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryJson | ConvertFrom-Json
        $info.status = [string](Get-OptionalPropertyValue -Object $summary -Name "status")
        $info.verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
        $info.full_visual_gate_status = [string](Get-OptionalPropertyValue -Object $summary -Name "full_visual_gate_status")
        $info.evidence_scope = [string](Get-OptionalPropertyValue -Object $summary -Name "evidence_scope")
        $info.stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "stage_count")
        $info.passed_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "passed_stage_count")
        $info.failed_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "failed_stage_count")
        $info.incomplete_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "incomplete_stage_count")
        $info.pdf_cli_export_status = [string](Get-OptionalPropertyValue -Object $summary -Name "pdf_cli_export_status")
        $info.pdf_regression_status = [string](Get-OptionalPropertyValue -Object $summary -Name "pdf_regression_status")
        $info.pdf_regression_selected_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "pdf_regression_selected_test_count")
        $info.pdf_regression_failed_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "pdf_regression_failed_test_count")
        $info.pdf_regression_skipped_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "pdf_regression_skipped_test_count")
        $info.unicode_font_status = [string](Get-OptionalPropertyValue -Object $summary -Name "unicode_font_status")
        $info.cjk_copy_search_status = [string](Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_status")
        $info.cjk_copy_search_count = [int](Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_count")
        $info.cjk_copy_search_missing_text_count = [int](Get-OptionalPropertyValue -Object $summary -Name "cjk_copy_search_missing_text_count")
        $info.visual_baseline_render_status = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_render_status")
        $info.visual_baseline_fresh_rendered_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_fresh_rendered_count")
        $info.expected_visual_render_count = [int](Get-OptionalPropertyValue -Object $summary -Name "expected_visual_render_count")
        $info.visual_baseline_fresh_missing_sample_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_fresh_missing_sample_count")
        $info.visual_baseline_resume_needed = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_resume_needed")
        $info.visual_baseline_resume_slice_offset = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_resume_slice_offset")
        $info.visual_baseline_resume_slice_limit = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_resume_slice_limit")
        $info.visual_baseline_resume_slice_command_template = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_resume_slice_command_template")
        $info.outer_guard_status = [string](Get-OptionalPropertyValue -Object $summary -Name "outer_guard_status")
        $info.outer_guard_timed_out = [bool](Get-OptionalPropertyValue -Object $summary -Name "outer_guard_timed_out")
        $info.outer_guard_timeout_seconds = [int](Get-OptionalPropertyValue -Object $summary -Name "outer_guard_timeout_seconds")
        $info.aggregate_contact_sheet_status = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet_status")
        $info.aggregate_contact_sheet = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet")
        $info.aggregate_contact_sheet_bytes = [int](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet_bytes")
    } catch {
        $info.status = "unreadable"
        $info.error = $_.Exception.Message
    }

    return $info
}

function Get-PdfVisualGateAttemptReleaseWarnings {
    param(
        [System.Collections.IDictionary]$ReleaseSummary,
        [string]$RepoRoot
    )

    $warnings = New-Object 'System.Collections.Generic.List[object]'
    $attempt = Get-OptionalPropertyValue -Object $ReleaseSummary -Name "pdf_visual_gate_attempt"
    if ($null -eq $attempt) {
        return @()
    }

    $attemptStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "status")
    $attemptVerdict = [string](Get-OptionalPropertyValue -Object $attempt -Name "verdict")
    $attemptFullStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "full_visual_gate_status")
    $outerGuardTimedOut = [bool](Get-OptionalPropertyValue -Object $attempt -Name "outer_guard_timed_out")
    $outerGuardStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "outer_guard_status")
    $renderStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_render_status")
    $contactSheetStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "aggregate_contact_sheet_status")
    $attemptPassed = ($attemptStatus -eq "pass" -and $attemptVerdict -eq "pass" -and $attemptFullStatus -eq "pass")
    $attemptStageCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "stage_count")
    $attemptPassedStageCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "passed_stage_count")
    $attemptFailedStageCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "failed_stage_count")
    $attemptIncompleteStageCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "incomplete_stage_count")
    $renderedCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_fresh_rendered_count")
    $expectedRenderCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "expected_visual_render_count")
    $freshMissingSampleCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_fresh_missing_sample_count")
    $resumeNeeded = [bool](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_resume_needed")
    $resumeSliceOffset = [int](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_resume_slice_offset")
    $resumeSliceLimit = [int](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_resume_slice_limit")
    $resumeSliceCommandTemplate = [string](Get-OptionalPropertyValue -Object $attempt -Name "visual_baseline_resume_slice_command_template")
    $pdfRegressionStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_status")
    $pdfRegressionSelected = [int](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_selected_test_count")
    $pdfRegressionFailed = [int](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_failed_test_count")
    $pdfRegressionSkipped = [int](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_skipped_test_count")
    $cjkCopySearchStatus = [string](Get-OptionalPropertyValue -Object $attempt -Name "cjk_copy_search_status")
    $cjkCopySearchCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "cjk_copy_search_count")
    $cjkMissingTextCount = [int](Get-OptionalPropertyValue -Object $attempt -Name "cjk_copy_search_missing_text_count")
    $readiness = Get-OptionalPropertyValue -Object $ReleaseSummary -Name "pdf_full_ctest_readiness"
    $visualGate = Get-OptionalPropertyValue -Object $ReleaseSummary -Name "pdf_visual_gate"
    $readinessReleaseReady = [bool](Get-OptionalPropertyValue -Object $readiness -Name "release_ready")
    $visualReleaseEvidenceAccepted = [bool](Get-OptionalPropertyValue -Object $readiness -Name "visual_gate_release_evidence_accepted")
    $freshFullGuardedEvidence = [bool](Get-OptionalPropertyValue -Object $readiness -Name "visual_gate_fresh_full_guarded_evidence")
    $passSummaryBeforeOuterTimeout = [bool](Get-OptionalPropertyValue -Object $readiness -Name "visual_gate_pass_summary_before_outer_timeout")
    $segmentedFullCoverageEvidence = [bool](Get-OptionalPropertyValue -Object $readiness -Name "visual_gate_segmented_full_coverage_evidence")
    $finalizeOnlyEvidence = [bool](Get-OptionalPropertyValue -Object $visualGate -Name "finalize_only")
    $skipPreflightEvidence = [bool](Get-OptionalPropertyValue -Object $visualGate -Name "skip_preflight")
    $readinessVerdict = [string](Get-OptionalPropertyValue -Object $readiness -Name "verdict")
    $needsWarning = (-not $attemptPassed) -and (
        $attemptStatus -eq "partial" -or
        $attemptVerdict -eq "not_complete" -or
        $attemptFullStatus -eq "not_complete" -or
        $outerGuardTimedOut -or
        $outerGuardStatus -eq "timed_out"
    )

    if (-not $needsWarning) {
        return @()
    }

    $attemptAuxiliaryEvidenceComplete = (
        $attemptStageCount -gt 0 -and
        $attemptPassedStageCount -eq $attemptStageCount -and
        $attemptFailedStageCount -eq 0 -and
        $attemptIncompleteStageCount -eq 0 -and
        $pdfRegressionStatus -eq "pass" -and
        $pdfRegressionFailed -eq 0 -and
        $cjkCopySearchStatus -eq "pass" -and
        $cjkMissingTextCount -eq 0 -and
        $renderStatus -eq "pass" -and
        $expectedRenderCount -gt 0 -and
        $renderedCount -ge $expectedRenderCount -and
        $contactSheetStatus -eq "pass"
    )
    $segmentedReleaseEvidenceAccepted = (
        $readinessReleaseReady -and
        $visualReleaseEvidenceAccepted -and
        $segmentedFullCoverageEvidence
    )
    if ($segmentedReleaseEvidenceAccepted -and $attemptAuxiliaryEvidenceComplete) {
        return @()
    }

    $summaryJsonPath = [string](Get-OptionalPropertyValue -Object $attempt -Name "summary_json")
    $summaryJsonDisplay = Get-RepoRelativePath -RepoRoot $RepoRoot -Path $summaryJsonPath
    $outerGuardTimeoutSeconds = [int](Get-OptionalPropertyValue -Object $attempt -Name "outer_guard_timeout_seconds")
    $message = if ($segmentedFullCoverageEvidence) {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard; release evidence is accepted through strict segmented full-coverage visual evidence, but this does not replace a completed single-run full visual gate."
    } elseif ($freshFullGuardedEvidence) {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard; release evidence is accepted through an earlier completed guarded full visual gate, while this attempt remains traceability debt."
    } elseif ($finalizeOnlyEvidence) {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard; release still relies on explicit FinalizeOnly summary/contact-sheet evidence while baseline render/contact-sheet refresh remains incomplete."
    } elseif ($visualReleaseEvidenceAccepted) {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard; release evidence is accepted by the PDF readiness gate, but the current attempt remains traceability debt."
    } else {
        "Fresh non-FinalizeOnly PDF visual gate attempt did not complete within the 60-second outer guard, and PDF visual release evidence is not accepted by the current readiness gate."
    }

    [void]$warnings.Add([ordered]@{
            id = "pdf_visual_gate_attempt.incomplete_fresh_render"
            action = "review_pdf_visual_gate_attempt_and_finalize_evidence"
            message = $message
            source_schema = "featherdoc.release_candidate_summary"
            source_report = $summaryJsonPath
            source_report_display = $summaryJsonDisplay
            source_json = $summaryJsonPath
            source_json_display = $summaryJsonDisplay
            attempt_status = $attemptStatus
            attempt_verdict = $attemptVerdict
            full_visual_gate_status = $attemptFullStatus
            evidence_scope = [string](Get-OptionalPropertyValue -Object $attempt -Name "evidence_scope")
            outer_guard_status = $outerGuardStatus
            outer_guard_timed_out = $outerGuardTimedOut
            outer_guard_timeout_seconds = $outerGuardTimeoutSeconds
            pdf_release_readiness_verdict = $readinessVerdict
            visual_gate_release_evidence_accepted = $visualReleaseEvidenceAccepted
            visual_gate_fresh_full_guarded_evidence = $freshFullGuardedEvidence
            visual_gate_pass_summary_before_outer_timeout = $passSummaryBeforeOuterTimeout
            visual_gate_segmented_full_coverage_evidence = $segmentedFullCoverageEvidence
            visual_gate_finalize_only = $finalizeOnlyEvidence
            visual_gate_skip_preflight = $skipPreflightEvidence
            pdf_cli_export_status = [string](Get-OptionalPropertyValue -Object $attempt -Name "pdf_cli_export_status")
            pdf_regression_status = [string](Get-OptionalPropertyValue -Object $attempt -Name "pdf_regression_status")
            pdf_regression_selected_test_count = $pdfRegressionSelected
            pdf_regression_failed_test_count = $pdfRegressionFailed
            pdf_regression_skipped_test_count = $pdfRegressionSkipped
            unicode_font_status = [string](Get-OptionalPropertyValue -Object $attempt -Name "unicode_font_status")
            cjk_copy_search_status = [string](Get-OptionalPropertyValue -Object $attempt -Name "cjk_copy_search_status")
            cjk_copy_search_count = $cjkCopySearchCount
            cjk_copy_search_missing_text_count = $cjkMissingTextCount
            visual_baseline_render_status = $renderStatus
            visual_baseline_fresh_rendered_count = $renderedCount
            expected_visual_render_count = $expectedRenderCount
            visual_baseline_fresh_missing_sample_count = $freshMissingSampleCount
            visual_baseline_resume_needed = $resumeNeeded
            visual_baseline_resume_slice_offset = $resumeSliceOffset
            visual_baseline_resume_slice_limit = $resumeSliceLimit
            visual_baseline_resume_slice_command_template = $resumeSliceCommandTemplate
            aggregate_contact_sheet_status = $contactSheetStatus
            aggregate_contact_sheet = [string](Get-OptionalPropertyValue -Object $attempt -Name "aggregate_contact_sheet")
            release_owner_acceptance_required = $true
            release_owner_acceptance_policy = "release_owner_may_accept_segmented_full_coverage_with_explicit_single_run_debt"
            release_owner_acceptance_boundary = "acceptance_does_not_replace_fresh_single_run_full_visual_gate"
            release_owner_acceptance_command_template = "pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 -PdfReleaseReadinessSummaryJson <summary.json>; document release-owner acceptance in final_review.md without changing full_visual_gate_status."
            release_owner_acceptance_trace_marker = "pdf_visual_gate_release_owner_acceptance_trace"
        })

    return @($warnings.ToArray())
}

function Set-ReleaseSummaryWarnings {
    param(
        [System.Collections.IDictionary]$ReleaseSummary,
        [object[]]$Warnings
    )

    $normalizedWarnings = New-Object 'System.Collections.Generic.List[object]'
    foreach ($warning in @($Warnings)) {
        if ($null -ne $warning) {
            [void]$normalizedWarnings.Add($warning)
        }
    }

    $ReleaseSummary["warnings"] = @($normalizedWarnings.ToArray())
    $ReleaseSummary["warning_count"] = $normalizedWarnings.Count
}

function Get-PdfVisualSegmentedGateSummaryInfo {
    param(
        [string]$SummaryJson
    )

    $info = [ordered]@{
        requested = -not [string]::IsNullOrWhiteSpace($SummaryJson)
        status = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { "not_requested" } elseif (Test-Path -LiteralPath $SummaryJson) { "available" } else { "missing" }
        summary_json = $SummaryJson
        verdict = ""
        full_visual_gate_status = ""
        evidence_scope = ""
        boundary = ""
        slice_summary_count = 0
        slice_pass_count = 0
        slice_failed_count = 0
        covered_baseline_count = 0
        expected_visual_render_count = 0
        visual_baseline_manifest_count = 0
        attempt_status = ""
        attempt_verdict = ""
        attempt_full_visual_gate_status = ""
        attempt_stage_count = 0
        attempt_passed_stage_count = 0
        attempt_incomplete_stage_count = 0
        visual_baseline_render_status = ""
        visual_baseline_fresh_rendered_count = 0
        aggregate_contact_sheet_status = ""
        aggregate_contact_sheet = ""
        aggregate_contact_sheet_bytes = 0
        aggregate_rebuild_status = ""
        aggregate_rebuild_verdict = ""
        aggregate_rebuild_selected_baseline_count = 0
        aggregate_rebuild_expected_visual_render_count = 0
        error = ""
    }

    if (-not [bool]$info.requested -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return $info
    }

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryJson | ConvertFrom-Json
        $info.status = [string](Get-OptionalPropertyValue -Object $summary -Name "status")
        $info.verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
        $info.full_visual_gate_status = [string](Get-OptionalPropertyValue -Object $summary -Name "full_visual_gate_status")
        $info.evidence_scope = [string](Get-OptionalPropertyValue -Object $summary -Name "evidence_scope")
        $info.boundary = [string](Get-OptionalPropertyValue -Object $summary -Name "boundary")
        $info.slice_summary_count = [int](Get-OptionalPropertyValue -Object $summary -Name "slice_summary_count")
        $info.slice_pass_count = [int](Get-OptionalPropertyValue -Object $summary -Name "slice_pass_count")
        $info.slice_failed_count = [int](Get-OptionalPropertyValue -Object $summary -Name "slice_failed_count")
        $info.covered_baseline_count = [int](Get-OptionalPropertyValue -Object $summary -Name "covered_baseline_count")
        $info.expected_visual_render_count = [int](Get-OptionalPropertyValue -Object $summary -Name "expected_visual_render_count")
        $info.visual_baseline_manifest_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_manifest_count")
        $info.attempt_status = [string](Get-OptionalPropertyValue -Object $summary -Name "attempt_status")
        $info.attempt_verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "attempt_verdict")
        $info.attempt_full_visual_gate_status = [string](Get-OptionalPropertyValue -Object $summary -Name "attempt_full_visual_gate_status")
        $info.attempt_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "attempt_stage_count")
        $info.attempt_passed_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "attempt_passed_stage_count")
        $info.attempt_incomplete_stage_count = [int](Get-OptionalPropertyValue -Object $summary -Name "attempt_incomplete_stage_count")
        $info.visual_baseline_render_status = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_render_status")
        $info.visual_baseline_fresh_rendered_count = [int](Get-OptionalPropertyValue -Object $summary -Name "visual_baseline_fresh_rendered_count")
        $info.aggregate_contact_sheet_status = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet_status")
        $info.aggregate_contact_sheet = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet")
        $info.aggregate_contact_sheet_bytes = [int64](Get-OptionalPropertyValue -Object $summary -Name "aggregate_contact_sheet_bytes")
        $info.aggregate_rebuild_status = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_rebuild_status")
        $info.aggregate_rebuild_verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "aggregate_rebuild_verdict")
        $info.aggregate_rebuild_selected_baseline_count = [int](Get-OptionalPropertyValue -Object $summary -Name "aggregate_rebuild_selected_baseline_count")
        $info.aggregate_rebuild_expected_visual_render_count = [int](Get-OptionalPropertyValue -Object $summary -Name "aggregate_rebuild_expected_visual_render_count")
    } catch {
        $info.status = "unreadable"
        $info.error = $_.Exception.Message
    }

    return $info
}

function Get-PdfBoundedCtestDefaultSummaryPaths {
    param([string]$RepoRoot)

    return @(
        "build\pdf-ctest-bounded-subset-current\summary.json",
        "build\pdf-ctest-bounded-contract-static-current\summary.json",
        "build\pdf-ctest-bounded-cjk-flow-static-current\summary.json",
        "build\pdf-ctest-bounded-regression-basic-text-current\summary.json",
        "build\pdf-ctest-bounded-regression-styled-document-current\summary.json",
        "build\pdf-ctest-bounded-regression-business-samples-current\summary.json",
        "build\pdf-ctest-bounded-regression-table-layout-current\summary.json"
    ) | ForEach-Object { Join-Path $RepoRoot $_ }
}

function Resolve-PdfBoundedCtestSummaryPaths {
    param(
        [string]$RepoRoot,
        [string[]]$SummaryJson
    )

    $requested = @($SummaryJson | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    $candidates = if ($requested.Count -gt 0) {
        @($requested | ForEach-Object { Resolve-FullPath -RepoRoot $RepoRoot -InputPath ([string]$_) })
    } else {
        @(Get-PdfBoundedCtestDefaultSummaryPaths -RepoRoot $RepoRoot)
    }

    $seen = @{}
    return @(
        foreach ($candidate in $candidates) {
            if ([string]::IsNullOrWhiteSpace([string]$candidate)) { continue }
            $resolved = [System.IO.Path]::GetFullPath([string]$candidate)
            if (-not (Test-Path -LiteralPath $resolved)) { continue }
            $key = $resolved.ToLowerInvariant()
            if ($seen.ContainsKey($key)) { continue }
            $seen[$key] = $true
            $resolved
        }
    )
}

function Get-PdfBoundedCtestSummaryInfo {
    param(
        [string[]]$SummaryJson,
        [string]$RepoRoot
    )

    $paths = @($SummaryJson | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) })
    $summaries = New-Object 'System.Collections.Generic.List[object]'
    $subsets = New-Object 'System.Collections.Generic.List[string]'
    $displayPaths = New-Object 'System.Collections.Generic.List[string]'
    $passCount = 0
    $selectedTestCount = 0
    $skippedTestCount = 0
    $errorCount = 0
    $errors = New-Object 'System.Collections.Generic.List[string]'

    foreach ($path in $paths) {
        try {
            $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
            $status = [string](Get-OptionalPropertyValue -Object $summary -Name "status")
            $verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
            $subset = [string](Get-OptionalPropertyValue -Object $summary -Name "subset")
            $selected = [int](Get-OptionalPropertyValue -Object $summary -Name "selected_test_count")
            $skipped = [int](Get-OptionalPropertyValue -Object $summary -Name "skipped_test_count")
            if ($status -eq "pass" -and $verdict -eq "pass") {
                $passCount++
            }
            if (-not [string]::IsNullOrWhiteSpace($subset)) {
                $subsets.Add($subset) | Out-Null
            }
            $selectedTestCount += $selected
            $skippedTestCount += $skipped
            $displayPaths.Add((Get-RepoRelativePath -RepoRoot $RepoRoot -Path $path)) | Out-Null
            $summaries.Add([ordered]@{
                    summary_json = $path
                    summary_json_display = Get-RepoRelativePath -RepoRoot $RepoRoot -Path $path
                    status = $status
                    verdict = $verdict
                    subset = $subset
                    selected_test_count = $selected
                    skipped_test_count = $skipped
                    ctest_timeout_seconds = [int](Get-OptionalPropertyValue -Object $summary -Name "ctest_timeout_seconds")
                    exit_code = [int](Get-OptionalPropertyValue -Object $summary -Name "exit_code")
                }) | Out-Null
        } catch {
            $errorCount++
            $errors.Add(("{0}: {1}" -f (Get-RepoRelativePath -RepoRoot $RepoRoot -Path $path), $_.Exception.Message)) | Out-Null
        }
    }

    $summaryCount = $summaries.Count
    $statusValue = if ($summaryCount -eq 0) {
        "not_available"
    } elseif ($errorCount -gt 0) {
        "partial"
    } elseif ($passCount -eq $summaryCount -and $skippedTestCount -eq 0) {
        "pass"
    } else {
        "needs_review"
    }

    return [ordered]@{
        status = $statusValue
        summary_count = $summaryCount
        pass_count = $passCount
        skipped_test_count = $skippedTestCount
        selected_test_count = $selectedTestCount
        subsets = @($subsets.ToArray())
        summary_json = @($paths)
        summary_json_display = @($displayPaths.ToArray())
        summaries = @($summaries.ToArray())
        error_count = $errorCount
        errors = @($errors.ToArray())
    }
}

function Get-PdfFullCtestReadinessSummaryInfo {
    param(
        [string]$SummaryJson,
        [string]$RepoRoot
    )

    $info = [ordered]@{
        requested = -not [string]::IsNullOrWhiteSpace($SummaryJson)
        status = if ([string]::IsNullOrWhiteSpace($SummaryJson)) { "not_requested" } elseif (Test-Path -LiteralPath $SummaryJson) { "available" } else { "missing" }
        summary_json = $SummaryJson
        summary_json_display = Get-RepoRelativePath -RepoRoot $RepoRoot -Path $SummaryJson
        verdict = ""
        release_ready = $false
        warning_count = 0
        visual_gate_release_evidence_accepted = $false
        visual_gate_fresh_full_guarded_evidence = $false
        visual_gate_pass_summary_before_outer_timeout = $false
        visual_gate_segmented_full_coverage_evidence = $false
        visual_full_gate_status = ""
        visual_full_gate_verdict = ""
        full_ctest_status = ""
        full_ctest_verdict = ""
        full_ctest_summary_json = ""
        full_ctest_summary_json_display = ""
        outer_guard_status = ""
        outer_guard_timed_out = $false
        selected_test_count = 0
        completed_test_count = 0
        passed_test_count = 0
        failed_test_count = 0
        skipped_test_count = 0
        not_run_test_count = 0
        completion_percent = 0.0
        remaining_test_count = 0
        zero_failed_tests_observed = $false
        boundary = ""
        marker = ""
        error = ""
    }

    if (-not [bool]$info.requested -or -not (Test-Path -LiteralPath $SummaryJson)) {
        return $info
    }

    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $SummaryJson | ConvertFrom-Json
        $info.status = [string](Get-OptionalPropertyValue -Object $summary -Name "status")
        $info.verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "verdict")
        $info.release_ready = [bool](Get-OptionalPropertyValue -Object $summary -Name "release_ready")
        $info.warning_count = [int](Get-OptionalPropertyValue -Object $summary -Name "warning_count")
        $info.visual_gate_release_evidence_accepted = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_gate_release_evidence_accepted")
        $info.visual_gate_fresh_full_guarded_evidence = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_gate_fresh_full_guarded_evidence")
        $info.visual_gate_pass_summary_before_outer_timeout = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_gate_pass_summary_before_outer_timeout")
        $info.visual_gate_segmented_full_coverage_evidence = [bool](Get-OptionalPropertyValue -Object $summary -Name "visual_gate_segmented_full_coverage_evidence")
        $info.visual_full_gate_status = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_full_gate_status")
        $info.visual_full_gate_verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "visual_full_gate_verdict")
        $info.full_ctest_status = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_status")
        $info.full_ctest_verdict = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_verdict")
        $info.full_ctest_summary_json = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_summary_json")
        $info.full_ctest_summary_json_display = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_summary_json_display")
        $info.outer_guard_status = [string](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_outer_guard_status")
        $info.outer_guard_timed_out = [bool](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_outer_guard_timed_out")
        $info.selected_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_selected_test_count")
        $info.completed_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_completed_test_count")
        $info.passed_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_passed_test_count")
        $info.failed_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_failed_test_count")
        $info.skipped_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_skipped_test_count")
        $info.not_run_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_not_run_test_count")
        $info.completion_percent = [double](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_completion_percent")
        $info.remaining_test_count = [int](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_remaining_test_count")
        $info.zero_failed_tests_observed = [bool](Get-OptionalPropertyValue -Object $summary -Name "full_ctest_zero_failed_tests_observed")
        $info.boundary = [string](Get-OptionalPropertyValue -Object $summary -Name "boundary")
        $info.marker = [string](Get-OptionalPropertyValue -Object $summary -Name "marker")
    } catch {
        $info.status = "unreadable"
        $info.error = $_.Exception.Message
    }

    return $info
}

function Convert-ReviewTimestamp {
    param([AllowNull()]$Value)

    if ($null -eq $Value) {
        return ""
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$Value
}

function Add-OptionalVisualReviewArguments {
    param(
        [object[]]$Arguments,
        [string]$VerdictArgument,
        [string]$Verdict,
        [string]$NoteArgument,
        [string]$Note
    )

    $result = @($Arguments)
    if ($Verdict -ne "undecided" -or -not [string]::IsNullOrWhiteSpace($Note)) {
        $result += @($VerdictArgument, $Verdict)
    }
    if (-not [string]::IsNullOrWhiteSpace($Note)) {
        $result += @($NoteArgument, $Note)
    }

    return $result
}

function Add-GateFlowReviewSummary {
    param(
        [System.Collections.IDictionary]$Target,
        [string]$Prefix,
        [AllowNull()]$FlowInfo
    )

    $reviewStatus = Get-OptionalPropertyValue -Object $FlowInfo -Name "review_status"
    $reviewVerdict = Get-OptionalPropertyValue -Object $FlowInfo -Name "review_verdict"
    $reviewedAt = Convert-ReviewTimestamp -Value (Get-OptionalPropertyValue -Object $FlowInfo -Name "reviewed_at")
    $reviewMethod = Get-OptionalPropertyValue -Object $FlowInfo -Name "review_method"
    $reviewNote = Get-OptionalPropertyValue -Object $FlowInfo -Name "review_note"
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_review_status" -f $Prefix) -Value $reviewStatus
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_verdict" -f $Prefix) -Value $reviewVerdict
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_reviewed_at" -f $Prefix) -Value $reviewedAt
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_review_method" -f $Prefix) -Value $reviewMethod
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_review_note" -f $Prefix) -Value $reviewNote
}

function Get-CuratedVisualReviewSummaryEntries {
    param([AllowNull()]$GateSummary)

    $curatedFlows = @(Get-OptionalPropertyValue -Object $GateSummary -Name "curated_visual_regressions")
    return @(
        $curatedFlows | Where-Object {
            -not [string]::IsNullOrWhiteSpace([string](Get-OptionalPropertyValue -Object $_ -Name "review_verdict"))
        } | ForEach-Object {
            $taskInfo = Get-OptionalPropertyValue -Object $_ -Name "task"
            [ordered]@{
                id = Get-OptionalPropertyValue -Object $_ -Name "id"
                label = Get-OptionalPropertyValue -Object $_ -Name "label"
                verdict = Get-OptionalPropertyValue -Object $_ -Name "review_verdict"
                review_status = Get-OptionalPropertyValue -Object $_ -Name "review_status"
                reviewed_at = Convert-ReviewTimestamp -Value (Get-OptionalPropertyValue -Object $_ -Name "reviewed_at")
                review_method = Get-OptionalPropertyValue -Object $_ -Name "review_method"
                review_note = Get-OptionalPropertyValue -Object $_ -Name "review_note"
                task_id = Get-OptionalPropertyValue -Object $taskInfo -Name "task_id"
                task_dir = Get-OptionalPropertyValue -Object $taskInfo -Name "task_dir"
                review_result_path = Get-OptionalPropertyValue -Object $taskInfo -Name "review_result_path"
                final_review_path = Get-OptionalPropertyValue -Object $taskInfo -Name "final_review_path"
            }
        }
    )
}

function Get-ReleaseCandidateDisplayValue {
    param(
        [AllowNull()]$Value,
        [string]$Fallback = "(not available)"
    )

    if ($null -eq $Value -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        return $Fallback
    }

    return [string]$Value
}

function Get-CompleteVisualGateReviewTaskSummary {
    param([AllowNull()]$Summary)

    if ($null -eq $Summary) {
        return $null
    }

    $totalCount = Get-OptionalPropertyValue -Object $Summary -Name "total_count"
    $standardCount = Get-OptionalPropertyValue -Object $Summary -Name "standard_count"
    $curatedCount = Get-OptionalPropertyValue -Object $Summary -Name "curated_count"
    if ([string]::IsNullOrWhiteSpace([string]$totalCount) -or
        [string]::IsNullOrWhiteSpace([string]$standardCount) -or
        [string]::IsNullOrWhiteSpace([string]$curatedCount)) {
        return $null
    }

    return [pscustomobject]@{
        total_count = $totalCount
        standard_count = $standardCount
        curated_count = $curatedCount
    }
}

function Get-VisualGateReviewTaskSummaryLine {
    param([AllowNull()]$VisualGateStep)

    $reviewTaskSummary = Get-CompleteVisualGateReviewTaskSummary -Summary (Get-OptionalPropertyValue -Object $VisualGateStep -Name "review_task_summary")
    if ($null -eq $reviewTaskSummary) {
        return ""
    }

    return "- Review task count: {0} total ({1} standard, {2} curated)" -f `
        $reviewTaskSummary.total_count,
        $reviewTaskSummary.standard_count,
        $reviewTaskSummary.curated_count
}

function Get-VisualGateReviewSummaryMarkdown {
    param(
        [string]$RepoRoot,
        [AllowNull()]$VisualGateStep
    )

    $reviewLines = New-Object 'System.Collections.Generic.List[string]'
    $standardFlows = @(
        [pscustomobject]@{ Label = "Smoke"; Prefix = "smoke"; TaskProperty = "document_task_dir" },
        [pscustomobject]@{ Label = "Fixed grid"; Prefix = "fixed_grid"; TaskProperty = "fixed_grid_task_dir" },
        [pscustomobject]@{ Label = "Section page setup"; Prefix = "section_page_setup"; TaskProperty = "section_page_setup_task_dir" },
        [pscustomobject]@{ Label = "Page number fields"; Prefix = "page_number_fields"; TaskProperty = "page_number_fields_task_dir" }
    )

    foreach ($flow in $standardFlows) {
        $verdict = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_verdict" -f $flow.Prefix)
        $reviewStatus = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_review_status" -f $flow.Prefix)
        $reviewedAt = Convert-ReviewTimestamp -Value (Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_reviewed_at" -f $flow.Prefix))
        $reviewMethod = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_review_method" -f $flow.Prefix)
        if ([string]::IsNullOrWhiteSpace([string]$verdict) -and
            [string]::IsNullOrWhiteSpace([string]$reviewStatus) -and
            [string]::IsNullOrWhiteSpace([string]$reviewedAt) -and
            [string]::IsNullOrWhiteSpace([string]$reviewMethod)) {
            continue
        }

        $line = "- {0}: verdict={1}, review_status={2}, reviewed_at={3}, review_method={4}" -f `
            $flow.Label,
            (Get-ReleaseCandidateDisplayValue -Value $verdict),
            (Get-ReleaseCandidateDisplayValue -Value $reviewStatus),
            (Get-ReleaseCandidateDisplayValue -Value $reviewedAt),
            (Get-ReleaseCandidateDisplayValue -Value $reviewMethod)
        $taskDir = Get-OptionalPropertyValue -Object $VisualGateStep -Name $flow.TaskProperty
        if (-not [string]::IsNullOrWhiteSpace([string]$taskDir)) {
            $line += ", task=$(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $taskDir)"
        }

        [void]$reviewLines.Add($line)
    }

    $curatedEntries = @(Get-OptionalPropertyValue -Object $VisualGateStep -Name "curated_visual_regressions")
    foreach ($entry in $curatedEntries) {
        $verdict = Get-OptionalPropertyValue -Object $entry -Name "verdict"
        $reviewStatus = Get-OptionalPropertyValue -Object $entry -Name "review_status"
        $reviewedAt = Convert-ReviewTimestamp -Value (Get-OptionalPropertyValue -Object $entry -Name "reviewed_at")
        $reviewMethod = Get-OptionalPropertyValue -Object $entry -Name "review_method"
        if ([string]::IsNullOrWhiteSpace([string]$verdict) -and
            [string]::IsNullOrWhiteSpace([string]$reviewStatus) -and
            [string]::IsNullOrWhiteSpace([string]$reviewedAt) -and
            [string]::IsNullOrWhiteSpace([string]$reviewMethod)) {
            continue
        }

        $label = Get-OptionalPropertyValue -Object $entry -Name "label"
        if ([string]::IsNullOrWhiteSpace([string]$label)) {
            $label = Get-OptionalPropertyValue -Object $entry -Name "id"
        }
        if ([string]::IsNullOrWhiteSpace([string]$label)) {
            $label = Get-OptionalPropertyValue -Object $entry -Name "task_id"
        }
        if ([string]::IsNullOrWhiteSpace([string]$label)) {
            $label = "Curated visual regression"
        }

        $line = "- Curated - {0}: verdict={1}, review_status={2}, reviewed_at={3}, review_method={4}" -f `
            $label,
            (Get-ReleaseCandidateDisplayValue -Value $verdict),
            (Get-ReleaseCandidateDisplayValue -Value $reviewStatus),
            (Get-ReleaseCandidateDisplayValue -Value $reviewedAt),
            (Get-ReleaseCandidateDisplayValue -Value $reviewMethod)
        $taskDir = Get-OptionalPropertyValue -Object $entry -Name "task_dir"
        if (-not [string]::IsNullOrWhiteSpace([string]$taskDir)) {
            $line += ", task=$(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $taskDir)"
        }

        [void]$reviewLines.Add($line)
    }

    if ($reviewLines.Count -eq 0) {
        return ""
    }

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    [void]$sectionLines.Add("## Visual review verdicts")
    [void]$sectionLines.Add("")
    foreach ($line in $reviewLines) {
        [void]$sectionLines.Add($line)
    }
    [void]$sectionLines.Add("")

    return ($sectionLines -join [Environment]::NewLine)
}

function Convert-OptionalBoolean {
    param(
        [AllowNull()]$Value,
        [bool]$DefaultValue = $false
    )

    if ($null -eq $Value) {
        return $DefaultValue
    }

    if ($Value -is [bool]) {
        return [bool]$Value
    }

    if ($Value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($Value)) {
            return $DefaultValue
        }

        return $Value.Equals("true", [System.StringComparison]::OrdinalIgnoreCase)
    }

    return [bool]$Value
}

function Get-ProjectTemplateSmokeSchemaBaselineCounts {
    param([AllowNull()]$Summary)

    $counts = [ordered]@{
        dirty = 0
        drift = 0
    }

    foreach ($entry in @(Get-OptionalPropertyValue -Object $Summary -Name "entries")) {
        $checks = Get-OptionalPropertyValue -Object $entry -Name "checks"
        $schemaBaseline = Get-OptionalPropertyValue -Object $checks -Name "schema_baseline"
        if ($null -eq $schemaBaseline) {
            continue
        }

        if (-not (Convert-OptionalBoolean -Value (Get-OptionalPropertyValue -Object $schemaBaseline -Name "enabled"))) {
            continue
        }

        $matches = Convert-OptionalBoolean `
            -Value (Get-OptionalPropertyValue -Object $schemaBaseline -Name "matches") `
            -DefaultValue $true
        $baselineResult = Get-OptionalPropertyValue -Object $schemaBaseline -Name "result"
        if (-not $matches -and $null -ne $baselineResult) {
            $counts.drift += 1
        }

        $schemaLintClean = Convert-OptionalBoolean `
            -Value (Get-OptionalPropertyValue -Object $schemaBaseline -Name "schema_lint_clean") `
            -DefaultValue $true
        if (-not $schemaLintClean) {
            $counts.dirty += 1
        }
    }

    return [pscustomobject]$counts
}

function Get-ProjectTemplateSmokeDirtySchemaBaselineCount {
    param([AllowNull()]$Summary)

    $value = Get-OptionalPropertyValue -Object $Summary -Name "dirty_schema_baseline_count"
    if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
        return [int]$value
    }

    $counts = Get-ProjectTemplateSmokeSchemaBaselineCounts -Summary $Summary
    return [int]$counts.dirty
}

function Get-OptionalIntegerProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name,
        [int]$DefaultValue = 0
    )

    $value = Get-OptionalPropertyValue -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }

    return [int]$value
}

function Get-OptionalObjectArrayProperty {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    $value = Get-OptionalPropertyValue -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }
    if ($value -is [string]) {
        return @($value)
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Get-SchemaPatchApprovalGateStatus {
    param(
        [int]$PendingCount,
        [int]$ApprovalItemCount,
        [int]$InvalidResultCount,
        [int]$ComplianceIssueCount
    )

    if ($ComplianceIssueCount -gt 0 -or $InvalidResultCount -gt 0) {
        return "blocked"
    }
    if ($PendingCount -gt 0) {
        return "pending"
    }
    if ($ApprovalItemCount -gt 0) {
        return "passed"
    }

    return "not_required"
}

function Get-SchemaPatchApprovalItemCount {
    param(
        [int]$PendingCount,
        [int]$ApprovedCount,
        [int]$RejectedCount,
        [object[]]$ApprovalItems
    )

    $countedItems = $PendingCount + $ApprovedCount + $RejectedCount
    if ($ApprovalItems.Count -gt $countedItems) {
        return $ApprovalItems.Count
    }

    return $countedItems
}

function Get-ProjectTemplateSmokeSchemaApprovalGateInfo {
    param([AllowNull()]$Summary)

    $approvalItems = @(Get-OptionalObjectArrayProperty -Object $Summary -Name "schema_patch_approval_items")
    $invalidResultCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_invalid_result_count"
    if ($invalidResultCount -eq 0 -and $approvalItems.Count -gt 0) {
        $invalidResultCount = @($approvalItems | Where-Object { [string](Get-OptionalPropertyValue -Object $_ -Name "status") -eq "invalid_result" }).Count
    }

    $pendingCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_pending_count"
    $approvedCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_approved_count"
    $rejectedCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_rejected_count"
    $complianceIssueCount = Get-OptionalIntegerProperty -Object $Summary -Name "schema_patch_approval_compliance_issue_count"
    $approvalItemCount = Get-SchemaPatchApprovalItemCount `
        -PendingCount $pendingCount `
        -ApprovedCount $approvedCount `
        -RejectedCount $rejectedCount `
        -ApprovalItems $approvalItems
    $gateStatus = Get-SchemaPatchApprovalGateStatus `
        -PendingCount $pendingCount `
        -ApprovalItemCount $approvalItemCount `
        -InvalidResultCount $invalidResultCount `
        -ComplianceIssueCount $complianceIssueCount

    return [pscustomobject]@{
        pending_count = $pendingCount
        approved_count = $approvedCount
        rejected_count = $rejectedCount
        compliance_issue_count = $complianceIssueCount
        invalid_result_count = $invalidResultCount
        item_count = $approvalItemCount
        gate_status = $gateStatus
        gate_blocked = ($gateStatus -eq "blocked")
    }
}

function New-ProjectTemplateSchemaApprovalReleaseBlockerItem {
    param([AllowNull()]$Item)

    $status = [string](Get-OptionalPropertyValue -Object $Item -Name "status")
    $action = [string](Get-OptionalPropertyValue -Object $Item -Name "action")
    if ([string]::IsNullOrWhiteSpace($action) -and $status -eq "invalid_result") {
        $action = "fix_schema_patch_approval_result"
    }

    return [pscustomobject]@{
        name = [string](Get-OptionalPropertyValue -Object $Item -Name "name")
        status = $status
        decision = [string](Get-OptionalPropertyValue -Object $Item -Name "decision")
        action = $action
        compliance_issue_count = Get-OptionalIntegerProperty -Object $Item -Name "compliance_issue_count"
        compliance_issues = @(Get-OptionalObjectArrayProperty -Object $Item -Name "compliance_issues")
        approval_result = [string](Get-OptionalPropertyValue -Object $Item -Name "approval_result")
        schema_update_candidate = [string](Get-OptionalPropertyValue -Object $Item -Name "schema_update_candidate")
        review_json = [string](Get-OptionalPropertyValue -Object $Item -Name "review_json")
    }
}

function Get-ProjectTemplateSchemaApprovalBlockedItems {
    param([object[]]$ApprovalItems)

    return @(
        $ApprovalItems | Where-Object {
            $status = [string](Get-OptionalPropertyValue -Object $_ -Name "status")
            $complianceIssueCount = Get-OptionalIntegerProperty -Object $_ -Name "compliance_issue_count"
            $status -eq "invalid_result" -or $complianceIssueCount -gt 0
        } | ForEach-Object {
            New-ProjectTemplateSchemaApprovalReleaseBlockerItem -Item $_
        }
    )
}

function Get-ProjectTemplateSchemaApprovalIssueKeys {
    param([object[]]$BlockedItems)

    return @(
        $BlockedItems |
            ForEach-Object { @(Get-OptionalObjectArrayProperty -Object $_ -Name "compliance_issues") } |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
            Sort-Object -Unique
    )
}

function New-ProjectTemplateSchemaApprovalReleaseBlocker {
    param(
        [AllowNull()]$ProjectTemplateSmokeSummary,
        [AllowNull()]$HistorySummary,
        [string]$SummaryJsonPath,
        [string]$HistoryJsonPath,
        [string]$HistoryMarkdownPath
    )

    $gateInfo = Get-ProjectTemplateSmokeSchemaApprovalGateInfo -Summary $ProjectTemplateSmokeSummary
    $latestBlockingSummary = Get-OptionalPropertyValue -Object $HistorySummary -Name "latest_blocking_summary"
    $latestBlockingStatus = [string](Get-OptionalPropertyValue -Object $latestBlockingSummary -Name "status")
    $historyBlocked = $latestBlockingStatus -eq "blocked"

    if (-not [bool]$gateInfo.gate_blocked) {
        return $null
    }

    $approvalItems = @(Get-OptionalObjectArrayProperty -Object $ProjectTemplateSmokeSummary -Name "schema_patch_approval_items")
    $blockedItems = @(Get-ProjectTemplateSchemaApprovalBlockedItems -ApprovalItems $approvalItems)
    if ($historyBlocked) {
        $historyBlockedItems = @(Get-OptionalObjectArrayProperty -Object $latestBlockingSummary -Name "items" |
            ForEach-Object { New-ProjectTemplateSchemaApprovalReleaseBlockerItem -Item $_ })
        if ($historyBlockedItems.Count -gt 0) {
            $blockedItems = $historyBlockedItems
        }
    }

    $issueKeys = @(Get-ProjectTemplateSchemaApprovalIssueKeys -BlockedItems $blockedItems)
    if ($historyBlocked) {
        $historyIssueKeys = @(Get-OptionalObjectArrayProperty -Object $latestBlockingSummary -Name "issue_keys" |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } |
            Sort-Object -Unique)
        if ($historyIssueKeys.Count -gt 0) {
            $issueKeys = $historyIssueKeys
        }
    }

    $blockingSummaryJson = [string](Get-OptionalPropertyValue -Object $latestBlockingSummary -Name "summary_json")
    if ([string]::IsNullOrWhiteSpace($blockingSummaryJson)) {
        $blockingSummaryJson = $SummaryJsonPath
    }
    $blockedItemCount = $blockedItems.Count
    if ($historyBlocked) {
        $historyBlockedItemCount = Get-OptionalIntegerProperty -Object $latestBlockingSummary -Name "blocked_item_count" -DefaultValue $blockedItemCount
        if ($historyBlockedItemCount -gt 0) {
            $blockedItemCount = $historyBlockedItemCount
        }
    }

    return [pscustomobject]@{
        id = "project_template_smoke.schema_approval"
        source = "project_template_smoke"
        severity = "error"
        status = "blocked"
        message = "Project template smoke schema approval gate blocked."
        gate_status = "blocked"
        pending_count = [int]$gateInfo.pending_count
        approved_count = [int]$gateInfo.approved_count
        rejected_count = [int]$gateInfo.rejected_count
        compliance_issue_count = [int]$gateInfo.compliance_issue_count
        invalid_result_count = [int]$gateInfo.invalid_result_count
        blocked_item_count = [int]$blockedItemCount
        issue_keys = $issueKeys
        summary_json = $blockingSummaryJson
        history_json = $HistoryJsonPath
        history_markdown = $HistoryMarkdownPath
        action = "fix_schema_patch_approval_result"
        items = $blockedItems
    }
}

function Set-ProjectTemplateSchemaApprovalReleaseBlocker {
    param(
        [System.Collections.IDictionary]$ReleaseSummary,
        [AllowNull()]$Blocker
    )

    $blockers = New-Object 'System.Collections.Generic.List[object]'
    foreach ($existingBlocker in @(Get-OptionalObjectArrayProperty -Object $ReleaseSummary -Name "release_blockers")) {
        $existingId = [string](Get-OptionalPropertyValue -Object $existingBlocker -Name "id")
        if ($existingId -ne "project_template_smoke.schema_approval") {
            [void]$blockers.Add($existingBlocker)
        }
    }
    if ($null -ne $Blocker) {
        [void]$blockers.Add($Blocker)
    }

    $ReleaseSummary["release_blockers"] = $blockers.ToArray()
    $ReleaseSummary["release_blocker_count"] = $blockers.Count
}

function Read-ProjectTemplateSchemaApprovalHistorySummary {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Get-ProjectTemplateSchemaApprovalHistoryInputList {
    param(
        [string]$ReleaseSummaryPath,
        [string]$ProjectTemplateSmokeSummaryPath
    )

    $summaryPaths = New-Object 'System.Collections.Generic.List[string]'
    if (-not [string]::IsNullOrWhiteSpace($ProjectTemplateSmokeSummaryPath) -and
        (Test-Path -LiteralPath $ProjectTemplateSmokeSummaryPath)) {
        [void]$summaryPaths.Add($ProjectTemplateSmokeSummaryPath)
    }
    if (-not [string]::IsNullOrWhiteSpace($ReleaseSummaryPath) -and
        (Test-Path -LiteralPath $ReleaseSummaryPath)) {
        [void]$summaryPaths.Add($ReleaseSummaryPath)
    }

    return @($summaryPaths.ToArray() | Sort-Object -Unique)
}

function Invoke-ProjectTemplateSchemaApprovalHistory {
    param(
        [string]$ScriptPath,
        [string]$ReleaseSummaryPath,
        [string]$ProjectTemplateSmokeSummaryPath,
        [string]$OutputJson,
        [string]$OutputMarkdown
    )

    $summaryPaths = @(Get-ProjectTemplateSchemaApprovalHistoryInputList `
            -ReleaseSummaryPath $ReleaseSummaryPath `
            -ProjectTemplateSmokeSummaryPath $ProjectTemplateSmokeSummaryPath)
    if ($summaryPaths.Count -eq 0) {
        return [pscustomobject]@{
            status = "skipped"
            input_count = 0
        }
    }

    Invoke-ChildPowerShell -ScriptPath $ScriptPath `
        -Arguments @(
            "-SummaryJson"
            ($summaryPaths -join ",")
            "-OutputJson"
            $OutputJson
            "-OutputMarkdown"
            $OutputMarkdown
        ) `
        -FailureMessage "Failed to write project template schema approval history."

    return [pscustomobject]@{
        status = "completed"
        input_count = $summaryPaths.Count
    }
}

function Get-ReleaseBlockerRollupInputList {
    param(
        [string[]]$InputJson,
        [string[]]$InputRoot
    )

    return @(
        @($InputJson) + @($InputRoot) |
            Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }
    )
}

function Select-UniqueReleaseBlockerRollupPathList {
    param([string[]]$Paths)

    $seen = @{}
    return @(
        foreach ($path in @($Paths)) {
            if ([string]::IsNullOrWhiteSpace($path)) { continue }
            $resolved = [System.IO.Path]::GetFullPath($path)
            $key = $resolved.ToLowerInvariant()
            if (-not $seen.ContainsKey($key)) {
                $seen[$key] = $true
                $resolved
            }
        }
    )
}

function Get-ReleaseGovernanceHandoffEffectiveInputJson {
    param(
        [string[]]$InputJson,
        [string]$ReleaseSummaryPath
    )

    return @(Select-UniqueReleaseBlockerRollupPathList `
            -Paths (@($InputJson) + @($ReleaseSummaryPath)))
}

function Get-ReleaseBlockerRollupAutoDiscoveredInputJson {
    param(
        [string]$RepoRoot,
        [string]$InputRoot
    )

    $candidateRelativePaths = @(
        "document-skeleton-governance-rollup/summary.json",
        "numbering-catalog-governance/summary.json",
        "table-layout-delivery-governance/summary.json",
        "content-control-data-binding-governance/summary.json",
        "project-template-delivery-readiness/summary.json",
        "schema-patch-confidence-calibration/summary.json",
        "docx-functional-smoke-readiness/summary.json"
    )
    $resolvedInputRoot = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $InputRoot

    return @(
        foreach ($relativePath in $candidateRelativePaths) {
            $candidate = Join-Path $resolvedInputRoot $relativePath
            if (Test-Path -LiteralPath $candidate) {
                $candidate
            }
        }
    )
}

function Invoke-ReleaseBlockerRollup {
    param(
        [string]$ScriptPath,
        [string[]]$InputJson,
        [string[]]$InputRoot,
        [string]$OutputDir,
        [string]$SummaryJson,
        [string]$ReportMarkdown,
        [bool]$FailOnBlocker,
        [bool]$FailOnWarning
    )

    $arguments = @()
    $cleanInputJson = @($InputJson | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    $cleanInputRoot = @($InputRoot | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($cleanInputJson.Count -gt 0) {
        $arguments += "-InputJson"
        $arguments += ($cleanInputJson -join ",")
    }
    if ($cleanInputRoot.Count -gt 0) {
        $arguments += "-InputRoot"
        $arguments += ($cleanInputRoot -join ",")
    }
    $arguments += @(
        "-OutputDir"
        $OutputDir
        "-SummaryJson"
        $SummaryJson
        "-ReportMarkdown"
        $ReportMarkdown
    )
    if ($FailOnBlocker) {
        $arguments += "-FailOnBlocker"
    }
    if ($FailOnWarning) {
        $arguments += "-FailOnWarning"
    }

    Invoke-ChildPowerShell -ScriptPath $ScriptPath `
        -Arguments $arguments `
        -FailureMessage "Failed to build release blocker rollup."

    if (-not (Test-Path -LiteralPath $SummaryJson)) {
        throw "Release blocker rollup did not write summary JSON: $SummaryJson"
    }
    if (-not (Test-Path -LiteralPath $ReportMarkdown)) {
        throw "Release blocker rollup did not write Markdown report: $ReportMarkdown"
    }
}

function Read-ReleaseBlockerRollupSummary {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Invoke-ReleaseGovernanceHandoff {
    param(
        [string]$ScriptPath,
        [string]$InputRoot,
        [string[]]$InputJson,
        [string]$OutputDir,
        [string]$SummaryJson,
        [string]$ReportMarkdown,
        [string]$ExpectedReportProfile,
        [bool]$IncludeRollup,
        [bool]$FailOnMissing,
        [bool]$FailOnBlocker,
        [bool]$FailOnWarning
    )

    $arguments = @(
        "-InputRoot"
        $InputRoot
        "-OutputDir"
        $OutputDir
        "-SummaryJson"
        $SummaryJson
        "-ReportMarkdown"
        $ReportMarkdown
        "-ExpectedReportProfile"
        $ExpectedReportProfile
    )
    $cleanInputJson = @($InputJson | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($cleanInputJson.Count -gt 0) {
        $arguments += "-InputJson"
        $arguments += ($cleanInputJson -join ",")
    }
    if ($IncludeRollup) {
        $arguments += "-IncludeReleaseBlockerRollup"
    }
    if ($FailOnMissing) {
        $arguments += "-FailOnMissing"
    }
    if ($FailOnBlocker) {
        $arguments += "-FailOnBlocker"
    }
    if ($FailOnWarning) {
        $arguments += "-FailOnWarning"
    }

    Invoke-ChildPowerShell -ScriptPath $ScriptPath `
        -Arguments $arguments `
        -FailureMessage "Failed to build release governance handoff."

    if (-not (Test-Path -LiteralPath $SummaryJson)) {
        throw "Release governance handoff did not write summary JSON: $SummaryJson"
    }
    if (-not (Test-Path -LiteralPath $ReportMarkdown)) {
        throw "Release governance handoff did not write Markdown report: $ReportMarkdown"
    }
}

function Read-ReleaseGovernanceHandoffSummary {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}



function Parse-InstallSmokeOutput {
    param([string[]]$Lines)

    $result = [ordered]@{}
    foreach ($line in $Lines) {
        if ($line -match '^Install prefix:\s*(.+)$') {
            $result.install_prefix = $Matches[1].Trim()
        } elseif ($line -match '^Consumer document:\s*(.+)$') {
            $result.consumer_document = $Matches[1].Trim()
        }
    }

    Assert-PathExists -Path $result.install_prefix -Label "install prefix"
    Assert-PathExists -Path $result.consumer_document -Label "install smoke consumer document"

    return $result
}

function Get-ReadmeGalleryInfoFromGateSummary {
    param([string]$GateSummaryPath)

    $result = [ordered]@{
        status = "unknown"
    }

    if ([string]::IsNullOrWhiteSpace($GateSummaryPath) -or
        -not (Test-Path -LiteralPath $GateSummaryPath)) {
        return $result
    }

    $gateSummary = Get-Content -Raw -LiteralPath $GateSummaryPath | ConvertFrom-Json
    $readmeGallery = Get-OptionalPropertyValue -Object $gateSummary -Name "readme_gallery"
    if ($null -eq $readmeGallery) {
        return $result
    }

    $status = Get-OptionalPropertyValue -Object $readmeGallery -Name "status"
    if (-not [string]::IsNullOrWhiteSpace($status)) {
        $result.status = $status
    }

    foreach ($name in @(
        "assets_dir",
        "visual_smoke_contact_sheet",
        "visual_smoke_page_05",
        "visual_smoke_page_06",
        "fixed_grid_aggregate_contact_sheet"
    )) {
        $value = Get-OptionalPropertyValue -Object $readmeGallery -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $result[$name] = $value
        }
    }

    return $result
}

function Parse-VisualGateOutput {
    param([string[]]$Lines)

    $result = [ordered]@{}
    foreach ($line in $Lines) {
        if ($line -match '^Gate summary:\s*(.+)$') {
            $result.gate_summary = $Matches[1].Trim()
        } elseif ($line -match '^Gate final review:\s*(.+)$') {
            $result.gate_final_review = $Matches[1].Trim()
        } elseif ($line -match '^Document task:\s*(.+)$') {
            $result.document_task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Fixed-grid task:\s*(.+)$') {
            $result.fixed_grid_task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Section page setup task:\s*(.+)$') {
            $result.section_page_setup_task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Page number fields task:\s*(.+)$') {
            $result.page_number_fields_task_dir = $Matches[1].Trim()
        }
    }

    Assert-PathExists -Path $result.gate_summary -Label "visual gate summary"
    Assert-PathExists -Path $result.gate_final_review -Label "visual gate final review"

    if ($result.document_task_dir) {
        Assert-PathExists -Path $result.document_task_dir -Label "visual gate document task"
    }
    if ($result.fixed_grid_task_dir) {
        Assert-PathExists -Path $result.fixed_grid_task_dir -Label "visual gate fixed-grid task"
    }
    if ($result.section_page_setup_task_dir) {
        Assert-PathExists -Path $result.section_page_setup_task_dir -Label "visual gate section page setup task"
    }
    if ($result.page_number_fields_task_dir) {
        Assert-PathExists -Path $result.page_number_fields_task_dir -Label "visual gate page number fields task"
    }

    return $result
}

function Parse-TemplateSchemaCommandOutput {
    param(
        [string[]]$Lines,
        [string]$Command
    )

    if ([string]::IsNullOrWhiteSpace($Command)) {
        throw "Template schema command name must not be empty."
    }

    $pattern = '^\{"command":"' + [regex]::Escape($Command) + '",'
    $jsonLine = $Lines |
        Where-Object { $_ -match $pattern } |
        Select-Object -Last 1
    if ([string]::IsNullOrWhiteSpace([string]$jsonLine)) {
        throw "Template schema command '$Command' did not emit a JSON result line."
    }

    return $jsonLine | ConvertFrom-Json
}

function Parse-TemplateSchemaCheckOutput {
    param([string[]]$Lines)

    return Parse-TemplateSchemaCommandOutput -Lines $Lines -Command "check-template-schema"
}

function Parse-TemplateSchemaLintOutput {
    param([string[]]$Lines)

    return Parse-TemplateSchemaCommandOutput -Lines $Lines -Command "lint-template-schema"
}

function Parse-TemplateSchemaManifestSummary {
    param([string]$SummaryPath)

    Assert-PathExists -Path $SummaryPath -Label "template schema manifest summary"
    return Get-Content -Raw -LiteralPath $SummaryPath | ConvertFrom-Json
}

function Parse-ProjectTemplateSmokeSummary {
    param([string]$SummaryPath)

    Assert-PathExists -Path $SummaryPath -Label "project template smoke summary"
    return Get-Content -Raw -LiteralPath $SummaryPath | ConvertFrom-Json
}

$repoRoot = Resolve-RepoRoot
$msvcBootstrapRequired = -not ($SkipConfigure -and $SkipBuild -and $SkipTests -and $SkipInstallSmoke -and $SkipVisualGate)
$msvcBootstrap = if ($msvcBootstrapRequired) {
    Get-MsvcBootstrap
} else {
    [ordered]@{
        mode = "not_required"
        vcvars_path = ""
    }
}

$resolvedBuildDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedInstallDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $InstallDir
$resolvedConsumerBuildDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $ConsumerBuildDir
$resolvedGateOutputDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $GateOutputDir
$resolvedTaskOutputRoot = Resolve-FullPath -RepoRoot $repoRoot -InputPath $TaskOutputRoot
$resolvedSummaryOutputDir = Resolve-FullPath -RepoRoot $repoRoot -InputPath $SummaryOutputDir
$projectVersion = Get-ProjectVersion -RepoRoot $repoRoot
$reportDir = Join-Path $resolvedSummaryOutputDir "report"
$summaryPath = Join-Path $reportDir "summary.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$releaseHandoffPath = Join-Path $reportDir "release_handoff.md"
$releaseBodyZhCnPath = Join-Path $reportDir "release_body.zh-CN.md"
$releaseSummaryZhCnPath = Join-Path $reportDir "release_summary.zh-CN.md"
$artifactGuidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$reviewerChecklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$schemaApprovalHistoryJsonPath = Join-Path $reportDir "project_template_schema_approval_history.json"
$schemaApprovalHistoryMarkdownPath = Join-Path $reportDir "project_template_schema_approval_history.md"
$startHerePath = Join-Path $resolvedSummaryOutputDir "START_HERE.md"
$releaseAssetsManifestSignoffPath = if ([string]::IsNullOrWhiteSpace($projectVersion)) {
    "output\release-assets\v<version>\release_assets_manifest.json"
} else {
    Join-Path (Join-Path "output\release-assets" ("v{0}" -f $projectVersion)) "release_assets_manifest.json"
}
$manifestSignoffEntrypoints = @(
    [ordered]@{
        id = "start_here"
        path = $startHerePath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $startHerePath
        required = $true
    },
    [ordered]@{
        id = "artifact_guide"
        path = $artifactGuidePath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $artifactGuidePath
        required = $true
    },
    [ordered]@{
        id = "reviewer_checklist"
        path = $reviewerChecklistPath
        path_display = Get-RepoRelativePath -RepoRoot $repoRoot -Path $reviewerChecklistPath
        required = $true
    }
)
$releaseManifestSignoffEntrypoints = if ($ReleaseEvidenceScope -eq "pdf-only") {
    $null
} else {
    [ordered]@{
        status = "declared"
        release_assets_manifest = $releaseAssetsManifestSignoffPath
        required_entrypoint_count = @($manifestSignoffEntrypoints).Count
        entrypoints = @($manifestSignoffEntrypoints)
        required_contracts = @(
            "project_template_delivery_readiness_contract",
            "project_template_onboarding_governance_contract"
        )
        required_fields = @(
            "status",
            "release_ready",
            "schema_approval_status_summary",
            "source_report_display",
            "source_json_display"
        )
        checklist_marker = "reviewer_manifest_scoped_project_template_trace"
    }
}
$releaseProjectTemplateReadinessChecklistEntrypoints = if ($ReleaseEvidenceScope -eq "pdf-only") {
    $null
} else {
    [ordered]@{
        status = "declared"
        checklist_label = "Project template release readiness checklist"
        checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
        required_entrypoint_count = @($manifestSignoffEntrypoints).Count
        entrypoints = @($manifestSignoffEntrypoints)
        checklist_marker = "release_entry_project_template_readiness_checklist_trace"
    }
}
$releaseEntryProjectTemplateReadinessChecklistMaterialSafetyAudit = if ($ReleaseEvidenceScope -eq "pdf-only") {
    $null
} else {
    [ordered]@{
        status = "passed"
        audit_script = ".\scripts\assert_release_material_safety.ps1"
        audited_entrypoint_count = 3
        audited_entrypoints = @(
            "start_here",
            "artifact_guide",
            "reviewer_checklist"
        )
        compact_evidence_label = "Project-template readiness checklist handoff evidence"
        compact_evidence_field = "project_template_readiness_checklist_entrypoints_source_reports"
        compact_evidence_source_schema = "featherdoc.release_candidate_summary"
        checklist_path = "docs/project_template_release_readiness_checklist_zh.rst"
        checklist_marker = "release_entry_project_template_readiness_checklist_trace"
        material_safety_marker = "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    }
}
$resolvedPdfVisualGateSummaryJson = ""
if (-not [string]::IsNullOrWhiteSpace($PdfVisualGateSummaryJson)) {
    $resolvedPdfVisualGateSummaryJson = Resolve-FullPath -RepoRoot $repoRoot -InputPath $PdfVisualGateSummaryJson
} else {
    $autoPdfVisualGateSummaryJson = Join-Path $repoRoot "output\pdf-visual-release-gate-current\report\summary.json"
    if (Test-Path -LiteralPath $autoPdfVisualGateSummaryJson) {
        $resolvedPdfVisualGateSummaryJson = $autoPdfVisualGateSummaryJson
    }
}
$pdfVisualGateSummaryInfo = Get-PdfVisualGateSummaryInfo -SummaryJson $resolvedPdfVisualGateSummaryJson
$resolvedPdfVisualGateAttemptSummaryJson = ""
if (-not [string]::IsNullOrWhiteSpace($PdfVisualGateAttemptSummaryJson)) {
    $resolvedPdfVisualGateAttemptSummaryJson = Resolve-FullPath -RepoRoot $repoRoot -InputPath $PdfVisualGateAttemptSummaryJson
} else {
    $autoPdfVisualGateAttemptSummaryJson = Join-Path $repoRoot "output\pdf-visual-release-gate-current\report\attempt-summary.json"
    if (Test-Path -LiteralPath $autoPdfVisualGateAttemptSummaryJson) {
        $resolvedPdfVisualGateAttemptSummaryJson = $autoPdfVisualGateAttemptSummaryJson
    }
}
$pdfVisualGateAttemptSummaryInfo = Get-PdfVisualGateAttemptSummaryInfo -SummaryJson $resolvedPdfVisualGateAttemptSummaryJson
$resolvedPdfVisualSegmentedGateSummaryJson = ""
if (-not [string]::IsNullOrWhiteSpace($PdfVisualSegmentedGateSummaryJson)) {
    $resolvedPdfVisualSegmentedGateSummaryJson = Resolve-FullPath -RepoRoot $repoRoot -InputPath $PdfVisualSegmentedGateSummaryJson
} else {
    $autoPdfVisualSegmentedGateSummaryJson = Join-Path $repoRoot "output\pdf-visual-release-gate-current\report\segmented-summary.json"
    if (Test-Path -LiteralPath $autoPdfVisualSegmentedGateSummaryJson) {
        $resolvedPdfVisualSegmentedGateSummaryJson = $autoPdfVisualSegmentedGateSummaryJson
    }
}
$pdfVisualSegmentedGateSummaryInfo = Get-PdfVisualSegmentedGateSummaryInfo -SummaryJson $resolvedPdfVisualSegmentedGateSummaryJson
$resolvedPdfBoundedCtestSummaryJson = @(Resolve-PdfBoundedCtestSummaryPaths `
        -RepoRoot $repoRoot `
        -SummaryJson $PdfBoundedCtestSummaryJson)
$pdfBoundedCtestSummaryInfo = Get-PdfBoundedCtestSummaryInfo `
    -SummaryJson $resolvedPdfBoundedCtestSummaryJson `
    -RepoRoot $repoRoot
$resolvedPdfReleaseReadinessSummaryJson = ""
if (-not [string]::IsNullOrWhiteSpace($PdfReleaseReadinessSummaryJson)) {
    $resolvedPdfReleaseReadinessSummaryJson = Resolve-FullPath -RepoRoot $repoRoot -InputPath $PdfReleaseReadinessSummaryJson
} else {
    $autoPdfReleaseReadinessSummaryJson = Join-Path $repoRoot "output\pdf-release-readiness-current\summary.json"
    if (Test-Path -LiteralPath $autoPdfReleaseReadinessSummaryJson) {
        $resolvedPdfReleaseReadinessSummaryJson = $autoPdfReleaseReadinessSummaryJson
    }
}
$pdfFullCtestReadinessInfo = Get-PdfFullCtestReadinessSummaryInfo `
    -SummaryJson $resolvedPdfReleaseReadinessSummaryJson `
    -RepoRoot $repoRoot
$installSmokeScript = Join-Path $repoRoot "scripts\run_install_find_package_smoke.ps1"
$templateSchemaCheckScript = Join-Path $repoRoot "scripts\check_template_schema_baseline.ps1"
$templateSchemaManifestScript = Join-Path $repoRoot "scripts\check_template_schema_manifest.ps1"
$projectTemplateSmokeScript = Join-Path $repoRoot "scripts\run_project_template_smoke.ps1"
$projectTemplateSchemaApprovalHistoryScript = Join-Path $repoRoot "scripts\write_project_template_schema_approval_history.ps1"
$projectTemplateSmokeDiscoverScript = Join-Path $repoRoot "scripts\discover_project_template_smoke_candidates.ps1"
$releaseBlockerRollupScript = Join-Path $repoRoot "scripts\build_release_blocker_rollup_report.ps1"
$releaseGovernanceHandoffScript = Join-Path $repoRoot "scripts\build_release_governance_handoff_report.ps1"
$visualGateScript = Join-Path $repoRoot "scripts\run_word_visual_release_gate.ps1"
$releaseNoteBundleScript = Join-Path $repoRoot "scripts\write_release_note_bundle.ps1"

$resolvedTemplateSchemaInputDocx = if ([string]::IsNullOrWhiteSpace($TemplateSchemaInputDocx)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaInputDocx
}
$resolvedTemplateSchemaBaseline = if ([string]::IsNullOrWhiteSpace($TemplateSchemaBaseline)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaBaseline
}
$resolvedTemplateSchemaGeneratedOutput = if ([string]::IsNullOrWhiteSpace($TemplateSchemaGeneratedOutput)) {
    Join-Path $reportDir "generated_template_schema.json"
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaGeneratedOutput
}
$resolvedTemplateSchemaManifestPath = if ([string]::IsNullOrWhiteSpace($TemplateSchemaManifestPath)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaManifestPath
}
$templateSchemaManifestRequested = -not [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaManifestPath)
$resolvedTemplateSchemaManifestOutputDir = if ($templateSchemaManifestRequested) {
    if ([string]::IsNullOrWhiteSpace($TemplateSchemaManifestOutputDir)) {
        Join-Path $reportDir "template-schema-manifest-checks"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $TemplateSchemaManifestOutputDir
    }
} else {
    ""
}
$resolvedTemplateSchemaManifestSummaryPath = if ($templateSchemaManifestRequested) {
    Join-Path $resolvedTemplateSchemaManifestOutputDir "summary.json"
} else {
    ""
}
$resolvedProjectTemplateSmokeManifestPath = if ([string]::IsNullOrWhiteSpace($ProjectTemplateSmokeManifestPath)) {
    ""
} else {
    Resolve-FullPath -RepoRoot $repoRoot -InputPath $ProjectTemplateSmokeManifestPath
}
$projectTemplateSmokeRequested = -not [string]::IsNullOrWhiteSpace($resolvedProjectTemplateSmokeManifestPath)
$resolvedProjectTemplateSmokeOutputDir = if ($projectTemplateSmokeRequested) {
    if ([string]::IsNullOrWhiteSpace($ProjectTemplateSmokeOutputDir)) {
        Join-Path $reportDir "project-template-smoke"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $ProjectTemplateSmokeOutputDir
    }
} else {
    ""
}
$resolvedProjectTemplateSmokeSummaryPath = if ($projectTemplateSmokeRequested) {
    Join-Path $resolvedProjectTemplateSmokeOutputDir "summary.json"
} else {
    ""
}
$resolvedProjectTemplateSmokeCandidateDiscoveryPath = if ($projectTemplateSmokeRequested) {
    Join-Path $resolvedProjectTemplateSmokeOutputDir "candidate_discovery.json"
} else {
    ""
}
$templateSchemaRequested = -not [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaInputDocx) -or
    -not [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaBaseline)
$expandedReleaseBlockerRollupInputJson = @(Expand-TemplateSchemaArgumentList -Values $ReleaseBlockerRollupInputJson)
$expandedReleaseBlockerRollupInputRoot = @(Expand-TemplateSchemaArgumentList -Values $ReleaseBlockerRollupInputRoot)
$resolvedReleaseBlockerRollupAutoDiscoverRoot = Resolve-FullPath -RepoRoot $repoRoot `
    -InputPath $ReleaseBlockerRollupAutoDiscoverRoot
$autoDiscoveredReleaseBlockerRollupInputJson = if ($ReleaseBlockerRollupAutoDiscover) {
    @(Get-ReleaseBlockerRollupAutoDiscoveredInputJson `
            -RepoRoot $repoRoot `
            -InputRoot $resolvedReleaseBlockerRollupAutoDiscoverRoot)
} else {
    @()
}
$resolvedReleaseBlockerRollupInputJson = @(
    foreach ($path in @($expandedReleaseBlockerRollupInputJson + $autoDiscoveredReleaseBlockerRollupInputJson)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            Resolve-FullPath -RepoRoot $repoRoot -InputPath $path
        }
    }
)
$resolvedReleaseBlockerRollupInputJson = @(Select-UniqueReleaseBlockerRollupPathList `
        -Paths $resolvedReleaseBlockerRollupInputJson)
$resolvedReleaseBlockerRollupInputRoot = @(
    foreach ($path in @($expandedReleaseBlockerRollupInputRoot)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            Resolve-FullPath -RepoRoot $repoRoot -InputPath $path
        }
    }
)
$resolvedReleaseBlockerRollupInputRoot = @(Select-UniqueReleaseBlockerRollupPathList `
        -Paths $resolvedReleaseBlockerRollupInputRoot)
$releaseBlockerRollupInputCount = @(Get-ReleaseBlockerRollupInputList `
        -InputJson $resolvedReleaseBlockerRollupInputJson `
        -InputRoot $resolvedReleaseBlockerRollupInputRoot).Count
$releaseBlockerRollupRequested = $releaseBlockerRollupInputCount -gt 0
$resolvedReleaseBlockerRollupOutputDir = if ($releaseBlockerRollupRequested) {
    if ([string]::IsNullOrWhiteSpace($ReleaseBlockerRollupOutputDir)) {
        Join-Path $reportDir "release-blocker-rollup"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReleaseBlockerRollupOutputDir
    }
} else {
    ""
}
$releaseBlockerRollupSummaryPath = if ($releaseBlockerRollupRequested) {
    Join-Path $resolvedReleaseBlockerRollupOutputDir "summary.json"
} else {
    ""
}
$releaseBlockerRollupMarkdownPath = if ($releaseBlockerRollupRequested) {
    Join-Path $resolvedReleaseBlockerRollupOutputDir "release_blocker_rollup.md"
} else {
    ""
}
$expandedReleaseGovernanceHandoffInputJson = @(Expand-TemplateSchemaArgumentList -Values $ReleaseGovernanceHandoffInputJson)
$resolvedReleaseGovernanceHandoffInputJson = @(
    foreach ($path in @($expandedReleaseGovernanceHandoffInputJson)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            Resolve-FullPath -RepoRoot $repoRoot -InputPath $path
        }
    }
)
$resolvedReleaseGovernanceHandoffInputJson = @(Select-UniqueReleaseBlockerRollupPathList `
        -Paths $resolvedReleaseGovernanceHandoffInputJson)
$releaseGovernanceHandoffInputRootIsDefault = [string]::Equals(
    $ReleaseGovernanceHandoffInputRoot,
    "output",
    [System.StringComparison]::OrdinalIgnoreCase)
$resolvedReleaseGovernanceHandoffInputRoot = Resolve-FullPath -RepoRoot $repoRoot `
    -InputPath $ReleaseGovernanceHandoffInputRoot
if ($releaseGovernanceHandoffInputRootIsDefault) {
    $pipelineGovernanceInputRoot = Resolve-FullPath -RepoRoot $repoRoot `
        -InputPath "output\release-governance-pipeline-current\governance"
    if (Test-Path -LiteralPath $pipelineGovernanceInputRoot -PathType Container) {
        $resolvedReleaseGovernanceHandoffInputRoot = $pipelineGovernanceInputRoot
    }
}
$defaultProjectTemplateOnboardingGovernanceSummary = Resolve-FullPath -RepoRoot $repoRoot `
    -InputPath "output\project-template-onboarding-governance\summary.json"
if ($releaseGovernanceHandoffInputRootIsDefault -and
    (Test-Path -LiteralPath $defaultProjectTemplateOnboardingGovernanceSummary -PathType Leaf)) {
    $resolvedReleaseGovernanceHandoffInputJson = @(Select-UniqueReleaseBlockerRollupPathList `
            -Paths (@($resolvedReleaseGovernanceHandoffInputJson) + @($defaultProjectTemplateOnboardingGovernanceSummary)))
}
$releaseGovernanceHandoffRequested = [bool]$ReleaseGovernanceHandoff
$resolvedReleaseGovernanceHandoffOutputDir = if ($releaseGovernanceHandoffRequested) {
    if ([string]::IsNullOrWhiteSpace($ReleaseGovernanceHandoffOutputDir)) {
        Join-Path $reportDir "release-governance-handoff"
    } else {
        Resolve-FullPath -RepoRoot $repoRoot -InputPath $ReleaseGovernanceHandoffOutputDir
    }
} else {
    ""
}
$releaseGovernanceHandoffSummaryPath = if ($releaseGovernanceHandoffRequested) {
    Join-Path $resolvedReleaseGovernanceHandoffOutputDir "summary.json"
} else {
    ""
}
$releaseGovernanceHandoffMarkdownPath = if ($releaseGovernanceHandoffRequested) {
    Join-Path $resolvedReleaseGovernanceHandoffOutputDir "release_governance_handoff.md"
} else {
    ""
}

New-Item -ItemType Directory -Path $resolvedSummaryOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

$summary = [ordered]@{
    schema = "featherdoc.release_candidate_summary"
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    install_dir = $resolvedInstallDir
    consumer_build_dir = $resolvedConsumerBuildDir
    gate_output_dir = $resolvedGateOutputDir
    task_output_root = $resolvedTaskOutputRoot
    config = $Config
    generator = $Generator
    ctest_timeout_seconds = $CtestTimeoutSeconds
    review_mode = if ($SkipReviewTasks) { "" } else { $ReviewMode }
    msvc_bootstrap_mode = $msvcBootstrap.mode
    release_version = $projectVersion
    visual_verdict = if ($SkipVisualGate) { "visual_gate_skipped" } else { "pending_manual_review" }
    execution_status = "running"
    failed_step = ""
    error = ""
    release_blockers = @()
    release_blocker_count = 0
    warning_count = 0
    warnings = @()
    governance_metric_count = 0
    governance_metrics = @()
    release_handoff = $releaseHandoffPath
    release_body_zh_cn = $releaseBodyZhCnPath
    release_summary_zh_cn = $releaseSummaryZhCnPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    start_here = $startHerePath
    release_evidence_scope = $ReleaseEvidenceScope
    manifest_signoff_entrypoints = $releaseManifestSignoffEntrypoints
    project_template_readiness_checklist_entrypoints = $releaseProjectTemplateReadinessChecklistEntrypoints
    release_entry_project_template_readiness_checklist_material_safety_audit = $releaseEntryProjectTemplateReadinessChecklistMaterialSafetyAudit
    pdf_visual_gate_summary_json = $resolvedPdfVisualGateSummaryJson
    pdf_visual_gate = $pdfVisualGateSummaryInfo
    pdf_visual_gate_attempt_summary_json = $resolvedPdfVisualGateAttemptSummaryJson
    pdf_visual_gate_attempt = $pdfVisualGateAttemptSummaryInfo
    pdf_visual_segmented_gate_summary_json = $resolvedPdfVisualSegmentedGateSummaryJson
    pdf_visual_segmented_gate = $pdfVisualSegmentedGateSummaryInfo
    pdf_bounded_ctest = $pdfBoundedCtestSummaryInfo
    pdf_release_readiness_summary_json = $resolvedPdfReleaseReadinessSummaryJson
    pdf_full_ctest_readiness = $pdfFullCtestReadinessInfo
    release_blocker_rollup = [ordered]@{
        requested = $releaseBlockerRollupRequested
        status = if ($releaseBlockerRollupRequested) { "pending" } else { "not_requested" }
        auto_discover = [bool]$ReleaseBlockerRollupAutoDiscover
        auto_discover_root = $resolvedReleaseBlockerRollupAutoDiscoverRoot
        auto_discovered_input_json = @($autoDiscoveredReleaseBlockerRollupInputJson)
        input_json = @($resolvedReleaseBlockerRollupInputJson)
        input_root = @($resolvedReleaseBlockerRollupInputRoot)
        output_dir = $resolvedReleaseBlockerRollupOutputDir
        summary_json = $releaseBlockerRollupSummaryPath
        report_markdown = $releaseBlockerRollupMarkdownPath
        fail_on_blocker = [bool]$ReleaseBlockerRollupFailOnBlocker
        fail_on_warning = [bool]$ReleaseBlockerRollupFailOnWarning
        source_report_count = 0
        source_failure_count = 0
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        manifest_signoff_entrypoints_source_report_count = 0
        manifest_signoff_entrypoints_source_reports = @()
        error = ""
    }
    release_governance_handoff = [ordered]@{
        requested = $releaseGovernanceHandoffRequested
        status = if ($releaseGovernanceHandoffRequested) { "pending" } else { "not_requested" }
        input_root = $resolvedReleaseGovernanceHandoffInputRoot
        input_json = @($resolvedReleaseGovernanceHandoffInputJson)
        output_dir = $resolvedReleaseGovernanceHandoffOutputDir
        summary_json = $releaseGovernanceHandoffSummaryPath
        report_markdown = $releaseGovernanceHandoffMarkdownPath
        include_rollup = [bool]$ReleaseGovernanceHandoffIncludeRollup
        fail_on_missing = [bool]$ReleaseGovernanceHandoffFailOnMissing
        fail_on_blocker = [bool]$ReleaseGovernanceHandoffFailOnBlocker
        fail_on_warning = [bool]$ReleaseGovernanceHandoffFailOnWarning
        expected_report_profile = $ReleaseGovernanceHandoffExpectedReportProfile
        expected_report_count = 0
        loaded_report_count = 0
        missing_report_count = 0
        failed_report_count = 0
        report_count = 0
        reports = @()
        governance_metric_count = 0
        governance_metrics = @()
        project_template_delivery_readiness_contract = $null
        project_template_onboarding_governance_contract = $null
        release_blocker_rollup = $null
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
        warnings = @()
        manifest_signoff_entrypoints_source_report_count = 0
        manifest_signoff_entrypoints_source_reports = @()
        project_template_readiness_checklist_entrypoints_source_report_count = 0
        project_template_readiness_checklist_entrypoints_source_reports = @()
        release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 0
        release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @()
        error = ""
    }
    template_schema = [ordered]@{
        requested = $templateSchemaRequested
        baseline = $resolvedTemplateSchemaBaseline
        input_docx = $resolvedTemplateSchemaInputDocx
        generated_output = if ($templateSchemaRequested) { $resolvedTemplateSchemaGeneratedOutput } else { "" }
    }
    template_schema_manifest = [ordered]@{
        requested = $templateSchemaManifestRequested
        manifest_path = $resolvedTemplateSchemaManifestPath
        output_dir = $resolvedTemplateSchemaManifestOutputDir
        summary_json = $resolvedTemplateSchemaManifestSummaryPath
    }
    project_template_smoke = [ordered]@{
        requested = $projectTemplateSmokeRequested
        manifest_path = $resolvedProjectTemplateSmokeManifestPath
        output_dir = $resolvedProjectTemplateSmokeOutputDir
        summary_json = $resolvedProjectTemplateSmokeSummaryPath
        require_full_coverage = [bool]$ProjectTemplateSmokeRequireFullCoverage
        candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
        candidate_count = 0
        registered_candidate_count = 0
        registered_manifest_entry_count = 0
        unregistered_candidate_count = 0
        excluded_candidate_count = 0
        dirty_schema_baseline_count = 0
        schema_baseline_drift_count = 0
        schema_patch_approval_pending_count = 0
        schema_patch_approval_approved_count = 0
        schema_patch_approval_rejected_count = 0
        schema_patch_approval_compliance_issue_count = 0
        schema_patch_approval_invalid_result_count = 0
        schema_patch_approval_gate_status = "not_required"
        schema_patch_approval_gate_blocked = $false
        schema_patch_approval_history_status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
        schema_patch_approval_history_json = if ($projectTemplateSmokeRequested) { $schemaApprovalHistoryJsonPath } else { "" }
        schema_patch_approval_history_markdown = if ($projectTemplateSmokeRequested) { $schemaApprovalHistoryMarkdownPath } else { "" }
        schema_patch_approval_history_input_count = 0
        schema_patch_approval_history_error = ""
        candidate_coverage = [ordered]@{
            status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
            require_full_coverage = [bool]$ProjectTemplateSmokeRequireFullCoverage
            candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
            candidate_count = 0
            registered_candidate_count = 0
            registered_manifest_entry_count = 0
            unregistered_candidate_count = 0
            excluded_candidate_count = 0
        }
    }
    readme_gallery = [ordered]@{
        status = if ($SkipVisualGate) { "visual_gate_skipped" } else { "pending" }
    }
    steps = [ordered]@{
        configure = [ordered]@{ status = if ($SkipConfigure) { "skipped" } else { "pending" } }
        build = [ordered]@{ status = if ($SkipBuild) { "skipped" } else { "pending" } }
        tests = [ordered]@{ status = if ($SkipTests) { "skipped" } else { "pending" } }
        template_schema = [ordered]@{ status = if ($templateSchemaRequested) { "pending" } else { "not_requested" } }
        template_schema_manifest = [ordered]@{ status = if ($templateSchemaManifestRequested) { "pending" } else { "not_requested" } }
        project_template_smoke = [ordered]@{
            status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
            dirty_schema_baseline_count = 0
            schema_baseline_drift_count = 0
            schema_patch_approval_pending_count = 0
            schema_patch_approval_approved_count = 0
            schema_patch_approval_rejected_count = 0
            schema_patch_approval_compliance_issue_count = 0
            schema_patch_approval_invalid_result_count = 0
            schema_patch_approval_gate_status = "not_required"
            schema_patch_approval_gate_blocked = $false
            schema_patch_approval_history_status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
            schema_patch_approval_history_json = if ($projectTemplateSmokeRequested) { $schemaApprovalHistoryJsonPath } else { "" }
            schema_patch_approval_history_markdown = if ($projectTemplateSmokeRequested) { $schemaApprovalHistoryMarkdownPath } else { "" }
            schema_patch_approval_history_input_count = 0
            schema_patch_approval_history_error = ""
            candidate_coverage = [ordered]@{
                status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" }
                require_full_coverage = [bool]$ProjectTemplateSmokeRequireFullCoverage
                candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
                candidate_count = 0
                registered_candidate_count = 0
                registered_manifest_entry_count = 0
                unregistered_candidate_count = 0
                excluded_candidate_count = 0
            }
        }
        install_smoke = [ordered]@{ status = if ($SkipInstallSmoke) { "skipped" } else { "pending" } }
        visual_gate = [ordered]@{ status = if ($SkipVisualGate) { "skipped" } else { "pending" } }
        release_blocker_rollup = [ordered]@{
            status = if ($releaseBlockerRollupRequested) { "pending" } else { "not_requested" }
            summary_json = $releaseBlockerRollupSummaryPath
            report_markdown = $releaseBlockerRollupMarkdownPath
            source_report_count = 0
            source_failure_count = 0
            release_blocker_count = 0
            release_blockers = @()
            action_item_count = 0
            action_items = @()
            warning_count = 0
            warnings = @()
            manifest_signoff_entrypoints_source_report_count = 0
            manifest_signoff_entrypoints_source_reports = @()
            error = ""
        }
        release_governance_handoff = [ordered]@{
            status = if ($releaseGovernanceHandoffRequested) { "pending" } else { "not_requested" }
            summary_json = $releaseGovernanceHandoffSummaryPath
            report_markdown = $releaseGovernanceHandoffMarkdownPath
            expected_report_profile = $ReleaseGovernanceHandoffExpectedReportProfile
            expected_report_count = 0
            loaded_report_count = 0
            missing_report_count = 0
            failed_report_count = 0
            report_count = 0
            reports = @()
            governance_metric_count = 0
            governance_metrics = @()
            project_template_delivery_readiness_contract = $null
            project_template_onboarding_governance_contract = $null
            release_blocker_rollup = $null
            release_blocker_count = 0
            release_blockers = @()
            action_item_count = 0
            action_items = @()
            warning_count = 0
            warnings = @()
            manifest_signoff_entrypoints_source_report_count = 0
            manifest_signoff_entrypoints_source_reports = @()
            project_template_readiness_checklist_entrypoints_source_report_count = 0
            project_template_readiness_checklist_entrypoints_source_reports = @()
            release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = 0
            release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @()
            error = ""
        }
        pdf_visual_gate = $pdfVisualGateSummaryInfo
        pdf_visual_gate_attempt = $pdfVisualGateAttemptSummaryInfo
        pdf_visual_segmented_gate = $pdfVisualSegmentedGateSummaryInfo
        pdf_bounded_ctest = $pdfBoundedCtestSummaryInfo
        pdf_full_ctest_readiness = $pdfFullCtestReadinessInfo
    }
}

$activeStep = ""
$releaseBlockerRollupFailure = $null
$releaseGovernanceHandoffFailure = $null

try {
    if ($TemplateSchemaSectionTargets -and $TemplateSchemaResolvedSectionTargets) {
        $activeStep = "template_schema"
        throw "Template schema checking forbids using -TemplateSchemaSectionTargets and -TemplateSchemaResolvedSectionTargets together."
    }
    if ($templateSchemaRequested -and [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaBaseline)) {
        $activeStep = "template_schema"
        throw "Template schema checking requires -TemplateSchemaBaseline when template schema options are requested."
    }
    if ($templateSchemaRequested -and [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaInputDocx)) {
        $activeStep = "template_schema"
        throw "Template schema checking requires -TemplateSchemaInputDocx when template schema options are requested."
    }
    if ($templateSchemaManifestRequested -and -not (Test-Path -LiteralPath $resolvedTemplateSchemaManifestPath)) {
        $activeStep = "template_schema_manifest"
        throw "Template schema manifest does not exist: $resolvedTemplateSchemaManifestPath"
    }
    if ($projectTemplateSmokeRequested -and -not (Test-Path -LiteralPath $resolvedProjectTemplateSmokeManifestPath)) {
        $activeStep = "project_template_smoke"
        throw "Project template smoke manifest does not exist: $resolvedProjectTemplateSmokeManifestPath"
    }
    foreach ($path in @($resolvedReleaseBlockerRollupInputJson)) {
        if (-not (Test-Path -LiteralPath $path)) {
            $activeStep = "release_blocker_rollup"
            throw "Release blocker rollup input JSON does not exist: $path"
        }
    }
    foreach ($path in @($resolvedReleaseBlockerRollupInputRoot)) {
        if (-not (Test-Path -LiteralPath $path)) {
            $activeStep = "release_blocker_rollup"
            throw "Release blocker rollup input root does not exist: $path"
        }
    }
    foreach ($path in @($resolvedReleaseGovernanceHandoffInputJson)) {
        if (-not (Test-Path -LiteralPath $path)) {
            $activeStep = "release_governance_handoff"
            throw "Release governance handoff input JSON does not exist: $path"
        }
    }
    if ($releaseGovernanceHandoffRequested -and -not (Test-Path -LiteralPath $resolvedReleaseGovernanceHandoffInputRoot)) {
        $activeStep = "release_governance_handoff"
        throw "Release governance handoff input root does not exist: $resolvedReleaseGovernanceHandoffInputRoot"
    }

    if ($RefreshReadmeAssets -and $SkipVisualGate) {
        $activeStep = "visual_gate"
        throw "README gallery refresh requires the visual gate. Re-run without -SkipVisualGate."
    }

    if (-not $SkipConfigure) {
        $activeStep = "configure"
        Write-Step "Configuring MSVC build directory"
        Invoke-MsvcCommand -MsvcBootstrap $msvcBootstrap -CommandText (
            "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"$Generator`" " +
            "-DCMAKE_BUILD_TYPE=$Config -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON -DBUILD_CLI=ON"
        )
        $summary.steps.configure.status = "completed"
    } else {
        if ($msvcBootstrapRequired) {
            Assert-PathExists -Path $resolvedBuildDir -Label "existing build directory"
        }
    }

    if (-not $SkipBuild) {
        $activeStep = "build"
        Write-Step "Building configured targets"
        Invoke-MsvcCommand -MsvcBootstrap $msvcBootstrap -CommandText (
            "cmake --build `"$resolvedBuildDir`""
        )
        $summary.steps.build.status = "completed"
    } else {
        if ($msvcBootstrapRequired) {
            Assert-PathExists -Path $resolvedBuildDir -Label "existing build directory"
        }
        if (-not $SkipInstallSmoke) {
            $summary.steps.build.install_prereq_cli = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_cli" `
                -Label "installable CLI executable"
        }
        if (-not $SkipVisualGate) {
            $summary.steps.build.visual_prereq_section_page_setup_cli = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_cli" `
                -Label "section page setup regression CLI"
            $summary.steps.build.visual_prereq_smoke = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_visual_smoke_tables" `
                -Label "visual smoke sample"
            $summary.steps.build.visual_prereq_merge_right = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_sample_merge_right_fixed_grid" `
                -Label "fixed-grid merge-right sample"
            $summary.steps.build.visual_prereq_merge_down = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_sample_merge_down_fixed_grid" `
                -Label "fixed-grid merge-down sample"
            $summary.steps.build.visual_prereq_unmerge_right = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_sample_unmerge_right_fixed_grid" `
                -Label "fixed-grid unmerge-right sample"
            $summary.steps.build.visual_prereq_unmerge_down = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_sample_unmerge_down_fixed_grid" `
                -Label "fixed-grid unmerge-down sample"
            if (-not $SkipSectionPageSetup) {
                $summary.steps.build.visual_prereq_section_page_setup = Assert-BuildExecutablePresent `
                    -BuildRoot $resolvedBuildDir `
                    -ExecutableStem "featherdoc_sample_section_page_setup" `
                    -Label "section page setup sample"
            }
            if (-not $SkipPageNumberFields) {
                $summary.steps.build.visual_prereq_page_number_fields = Assert-BuildExecutablePresent `
                    -BuildRoot $resolvedBuildDir `
                    -ExecutableStem "featherdoc_sample_page_number_fields" `
                    -Label "page number fields sample"
            }
        }
    }

    if (-not $SkipTests) {
        $activeStep = "tests"
        Write-Step "Running ctest"
        Invoke-MsvcCommand -MsvcBootstrap $msvcBootstrap -CommandText (
            "ctest --test-dir `"$resolvedBuildDir`" --output-on-failure --timeout $CtestTimeoutSeconds"
        )
        $summary.steps.tests.status = "completed"
    }

    if ($templateSchemaRequested) {
        $activeStep = "template_schema"
        Write-Step "Running template schema baseline check"
        Assert-PathExists -Path $resolvedTemplateSchemaInputDocx -Label "template schema input DOCX"
        Assert-PathExists -Path $resolvedTemplateSchemaBaseline -Label "template schema baseline"

        $templateSchemaArgs = @(
            "-InputDocx"
            $resolvedTemplateSchemaInputDocx
            "-SchemaFile"
            $resolvedTemplateSchemaBaseline
            "-GeneratedSchemaOutput"
            $resolvedTemplateSchemaGeneratedOutput
            "-SkipBuild"
            "-BuildDir"
            $resolvedBuildDir
        )
        if ($TemplateSchemaSectionTargets) {
            $templateSchemaArgs += "-SectionTargets"
        } elseif ($TemplateSchemaResolvedSectionTargets) {
            $templateSchemaArgs += "-ResolvedSectionTargets"
        }

        $templateSchemaOutput = @(
            & powershell.exe -ExecutionPolicy Bypass -File $templateSchemaCheckScript @templateSchemaArgs 2>&1
        )
        $templateSchemaExitCode = $LASTEXITCODE
        foreach ($line in $templateSchemaOutput) {
            Write-Host $line
        }
        if ($templateSchemaExitCode -notin @(0, 1)) {
            throw "Template schema baseline check failed."
        }

        $templateSchemaLines = @($templateSchemaOutput | ForEach-Object { $_.ToString() })
        $templateSchemaLintInfo = Parse-TemplateSchemaLintOutput -Lines $templateSchemaLines
        $templateSchemaInfo = Parse-TemplateSchemaCheckOutput -Lines $templateSchemaLines
        $summary.steps.template_schema.status = if ($templateSchemaExitCode -eq 0) {
            "completed"
        } else {
            "failed"
        }
        $summary.steps.template_schema.schema_lint_clean = [bool]$templateSchemaLintInfo.clean
        $summary.steps.template_schema.schema_lint_issue_count = [int]$templateSchemaLintInfo.issue_count
        $summary.steps.template_schema.matches = [bool]$templateSchemaInfo.matches
        $summary.steps.template_schema.schema_file = [string]$templateSchemaInfo.schema_file
        if (-not [string]::IsNullOrWhiteSpace([string]$templateSchemaInfo.generated_output_path)) {
            $summary.steps.template_schema.generated_output_path = [string]$templateSchemaInfo.generated_output_path
        }
        $summary.steps.template_schema.added_target_count = [int]$templateSchemaInfo.added_target_count
        $summary.steps.template_schema.removed_target_count = [int]$templateSchemaInfo.removed_target_count
        $summary.steps.template_schema.changed_target_count = [int]$templateSchemaInfo.changed_target_count
        $summary.template_schema.schema_lint_clean = [bool]$templateSchemaLintInfo.clean
        $summary.template_schema.schema_lint_issue_count = [int]$templateSchemaLintInfo.issue_count
        $summary.template_schema.matches = [bool]$templateSchemaInfo.matches
        $summary.template_schema.added_target_count = [int]$templateSchemaInfo.added_target_count
        $summary.template_schema.removed_target_count = [int]$templateSchemaInfo.removed_target_count
        $summary.template_schema.changed_target_count = [int]$templateSchemaInfo.changed_target_count

        if ($templateSchemaExitCode -ne 0) {
            if (-not [bool]$templateSchemaInfo.matches -and -not [bool]$templateSchemaLintInfo.clean) {
                throw "Template schema baseline drift and lint failures detected."
            }
            if (-not [bool]$templateSchemaInfo.matches) {
                throw "Template schema baseline drift detected."
            }
            if (-not [bool]$templateSchemaLintInfo.clean) {
                throw "Template schema baseline lint failed."
            }
            throw "Template schema baseline check failed."
        }
    }

    if ($templateSchemaManifestRequested) {
        $activeStep = "template_schema_manifest"
        Write-Step "Running template schema manifest check"
        Assert-PathExists -Path $resolvedTemplateSchemaManifestPath -Label "template schema manifest"
        New-Item -ItemType Directory -Path $resolvedTemplateSchemaManifestOutputDir -Force | Out-Null

        $templateSchemaManifestArgs = @(
            "-ManifestPath"
            $resolvedTemplateSchemaManifestPath
            "-BuildDir"
            $resolvedBuildDir
            "-OutputDir"
            $resolvedTemplateSchemaManifestOutputDir
            "-SkipBuild"
        )

        $templateSchemaManifestOutput = @(
            & powershell.exe -ExecutionPolicy Bypass -File $templateSchemaManifestScript @templateSchemaManifestArgs 2>&1
        )
        $templateSchemaManifestExitCode = $LASTEXITCODE
        foreach ($line in $templateSchemaManifestOutput) {
            Write-Host $line
        }
        if ($templateSchemaManifestExitCode -notin @(0, 1)) {
            throw "Template schema manifest check failed."
        }

        $templateSchemaManifestInfo = Parse-TemplateSchemaManifestSummary -SummaryPath $resolvedTemplateSchemaManifestSummaryPath
        $summary.steps.template_schema_manifest.status = if ($templateSchemaManifestExitCode -eq 0) {
            "completed"
        } else {
            "failed"
        }
        $summary.steps.template_schema_manifest.summary_json = $resolvedTemplateSchemaManifestSummaryPath
        $summary.steps.template_schema_manifest.manifest_path = [string]$templateSchemaManifestInfo.manifest_path
        $summary.steps.template_schema_manifest.output_dir = [string]$resolvedTemplateSchemaManifestOutputDir
        $summary.steps.template_schema_manifest.passed = [bool]$templateSchemaManifestInfo.passed
        $summary.steps.template_schema_manifest.entry_count = [int]$templateSchemaManifestInfo.entry_count
        $summary.steps.template_schema_manifest.drift_count = [int]$templateSchemaManifestInfo.drift_count
        $summary.steps.template_schema_manifest.dirty_baseline_count = [int]$templateSchemaManifestInfo.dirty_baseline_count

        $summary.template_schema_manifest.summary_json = $resolvedTemplateSchemaManifestSummaryPath
        $summary.template_schema_manifest.manifest_path = [string]$templateSchemaManifestInfo.manifest_path
        $summary.template_schema_manifest.output_dir = [string]$resolvedTemplateSchemaManifestOutputDir
        $summary.template_schema_manifest.passed = [bool]$templateSchemaManifestInfo.passed
        $summary.template_schema_manifest.entry_count = [int]$templateSchemaManifestInfo.entry_count
        $summary.template_schema_manifest.drift_count = [int]$templateSchemaManifestInfo.drift_count
        $summary.template_schema_manifest.dirty_baseline_count = [int]$templateSchemaManifestInfo.dirty_baseline_count

        if ($templateSchemaManifestExitCode -ne 0) {
            if ([int]$templateSchemaManifestInfo.drift_count -gt 0 -and
                [int]$templateSchemaManifestInfo.dirty_baseline_count -gt 0) {
                throw "Template schema manifest drift and lint failures detected."
            }
            if ([int]$templateSchemaManifestInfo.drift_count -gt 0) {
                throw "Template schema manifest drift detected."
            }
            if ([int]$templateSchemaManifestInfo.dirty_baseline_count -gt 0) {
                throw "Template schema manifest lint failed."
            }
            throw "Template schema manifest check failed."
        }
    }

    if ($projectTemplateSmokeRequested) {
        $activeStep = "project_template_smoke"
        Write-Step "Running project template smoke"
        Assert-PathExists -Path $resolvedProjectTemplateSmokeManifestPath -Label "project template smoke manifest"
        New-Item -ItemType Directory -Path $resolvedProjectTemplateSmokeOutputDir -Force | Out-Null

        $projectTemplateSmokeCoverageArgs = @(
            "-ManifestPath"
            $resolvedProjectTemplateSmokeManifestPath
            "-BuildDir"
            $resolvedBuildDir
            "-OutputPath"
            $resolvedProjectTemplateSmokeCandidateDiscoveryPath
            "-Json"
            "-IncludeRegistered"
            "-IncludeExcluded"
        )
        if ($ProjectTemplateSmokeRequireFullCoverage) {
            $projectTemplateSmokeCoverageArgs += "-FailOnUnregistered"
        }

        $projectTemplateSmokeCoverageOutput = @(
            & powershell.exe -ExecutionPolicy Bypass -File $projectTemplateSmokeDiscoverScript @projectTemplateSmokeCoverageArgs 2>&1
        )
        $projectTemplateSmokeCoverageExitCode = $LASTEXITCODE
        foreach ($line in $projectTemplateSmokeCoverageOutput) {
            Write-Host $line
        }
        if ($projectTemplateSmokeCoverageExitCode -notin @(0, 1)) {
            throw "Project template smoke candidate discovery failed."
        }

        Assert-PathExists -Path $resolvedProjectTemplateSmokeCandidateDiscoveryPath -Label "project template smoke candidate discovery JSON"
        $projectTemplateSmokeCoverageInfo = Get-Content -Raw -LiteralPath $resolvedProjectTemplateSmokeCandidateDiscoveryPath | ConvertFrom-Json
        $summary.steps.project_template_smoke.candidate_coverage = [ordered]@{
            status = if ($projectTemplateSmokeCoverageExitCode -eq 0) { "completed" } else { "failed" }
            require_full_coverage = [bool]$ProjectTemplateSmokeRequireFullCoverage
            candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
            candidate_count = [int]$projectTemplateSmokeCoverageInfo.candidate_count
            registered_candidate_count = [int]$projectTemplateSmokeCoverageInfo.registered_candidate_count
            registered_manifest_entry_count = [int]$projectTemplateSmokeCoverageInfo.registered_manifest_entry_count
            unregistered_candidate_count = [int]$projectTemplateSmokeCoverageInfo.unregistered_candidate_count
            excluded_candidate_count = [int]$projectTemplateSmokeCoverageInfo.excluded_candidate_count
        }
        $summary.project_template_smoke.candidate_coverage = $summary.steps.project_template_smoke.candidate_coverage
        $summary.project_template_smoke.candidate_discovery_json = $resolvedProjectTemplateSmokeCandidateDiscoveryPath
        $summary.project_template_smoke.candidate_count = [int]$projectTemplateSmokeCoverageInfo.candidate_count
        $summary.project_template_smoke.registered_candidate_count = [int]$projectTemplateSmokeCoverageInfo.registered_candidate_count
        $summary.project_template_smoke.registered_manifest_entry_count = [int]$projectTemplateSmokeCoverageInfo.registered_manifest_entry_count
        $summary.project_template_smoke.unregistered_candidate_count = [int]$projectTemplateSmokeCoverageInfo.unregistered_candidate_count
        $summary.project_template_smoke.excluded_candidate_count = [int]$projectTemplateSmokeCoverageInfo.excluded_candidate_count

        if ($ProjectTemplateSmokeRequireFullCoverage -and $projectTemplateSmokeCoverageExitCode -ne 0) {
            throw "Project template smoke candidate coverage is incomplete."
        }

        $projectTemplateSmokeArgs = @(
            "-ManifestPath"
            $resolvedProjectTemplateSmokeManifestPath
            "-BuildDir"
            $resolvedBuildDir
            "-OutputDir"
            $resolvedProjectTemplateSmokeOutputDir
            "-Dpi"
            $Dpi.ToString()
            "-SkipBuild"
        )
        if ($KeepWordOpen) {
            $projectTemplateSmokeArgs += "-KeepWordOpen"
        }
        if ($VisibleWord) {
            $projectTemplateSmokeArgs += "-VisibleWord"
        }

        $projectTemplateSmokeOutput = @(
            & powershell.exe -ExecutionPolicy Bypass -File $projectTemplateSmokeScript @projectTemplateSmokeArgs 2>&1
        )
        $projectTemplateSmokeExitCode = $LASTEXITCODE
        foreach ($line in $projectTemplateSmokeOutput) {
            Write-Host $line
        }
        if ($projectTemplateSmokeExitCode -notin @(0, 1)) {
            throw "Project template smoke failed."
        }

        $projectTemplateSmokeInfo = Parse-ProjectTemplateSmokeSummary -SummaryPath $resolvedProjectTemplateSmokeSummaryPath
        $projectTemplateSmokeSchemaBaselineCounts = Get-ProjectTemplateSmokeSchemaBaselineCounts -Summary $projectTemplateSmokeInfo
        $projectTemplateSmokeDirtySchemaBaselineCount = Get-ProjectTemplateSmokeDirtySchemaBaselineCount -Summary $projectTemplateSmokeInfo
        $projectTemplateSmokeSchemaBaselineDriftCount = [int]$projectTemplateSmokeSchemaBaselineCounts.drift
        $projectTemplateSmokeApprovalGate = Get-ProjectTemplateSmokeSchemaApprovalGateInfo -Summary $projectTemplateSmokeInfo
        $summary.steps.project_template_smoke.status = if ($projectTemplateSmokeExitCode -eq 0) {
            "completed"
        } else {
            "failed"
        }
        $summary.steps.project_template_smoke.summary_json = $resolvedProjectTemplateSmokeSummaryPath
        $summary.steps.project_template_smoke.manifest_path = [string]$projectTemplateSmokeInfo.manifest_path
        $summary.steps.project_template_smoke.output_dir = [string]$resolvedProjectTemplateSmokeOutputDir
        $summary.steps.project_template_smoke.passed = [bool]$projectTemplateSmokeInfo.passed
        $summary.steps.project_template_smoke.entry_count = [int]$projectTemplateSmokeInfo.entry_count
        $summary.steps.project_template_smoke.failed_entry_count = [int]$projectTemplateSmokeInfo.failed_entry_count
        $summary.steps.project_template_smoke.dirty_schema_baseline_count = $projectTemplateSmokeDirtySchemaBaselineCount
        $summary.steps.project_template_smoke.schema_baseline_drift_count = $projectTemplateSmokeSchemaBaselineDriftCount
        $summary.steps.project_template_smoke.visual_entry_count = [int]$projectTemplateSmokeInfo.visual_entry_count
        $summary.steps.project_template_smoke.visual_verdict = [string]$projectTemplateSmokeInfo.visual_verdict
        $summary.steps.project_template_smoke.manual_review_pending_count = [int]$projectTemplateSmokeInfo.manual_review_pending_count
        $summary.steps.project_template_smoke.visual_review_undetermined_count = [int]$projectTemplateSmokeInfo.visual_review_undetermined_count
        $summary.steps.project_template_smoke.overall_status = [string]$projectTemplateSmokeInfo.overall_status
        $summary.steps.project_template_smoke.schema_patch_approval_pending_count = [int]$projectTemplateSmokeApprovalGate.pending_count
        $summary.steps.project_template_smoke.schema_patch_approval_approved_count = [int]$projectTemplateSmokeApprovalGate.approved_count
        $summary.steps.project_template_smoke.schema_patch_approval_rejected_count = [int]$projectTemplateSmokeApprovalGate.rejected_count
        $summary.steps.project_template_smoke.schema_patch_approval_compliance_issue_count = [int]$projectTemplateSmokeApprovalGate.compliance_issue_count
        $summary.steps.project_template_smoke.schema_patch_approval_invalid_result_count = [int]$projectTemplateSmokeApprovalGate.invalid_result_count
        $summary.steps.project_template_smoke.schema_patch_approval_gate_status = [string]$projectTemplateSmokeApprovalGate.gate_status
        $summary.steps.project_template_smoke.schema_patch_approval_gate_blocked = [bool]$projectTemplateSmokeApprovalGate.gate_blocked

        $summary.project_template_smoke.summary_json = $resolvedProjectTemplateSmokeSummaryPath
        $summary.project_template_smoke.manifest_path = [string]$projectTemplateSmokeInfo.manifest_path
        $summary.project_template_smoke.output_dir = [string]$resolvedProjectTemplateSmokeOutputDir
        $summary.project_template_smoke.passed = [bool]$projectTemplateSmokeInfo.passed
        $summary.project_template_smoke.entry_count = [int]$projectTemplateSmokeInfo.entry_count
        $summary.project_template_smoke.failed_entry_count = [int]$projectTemplateSmokeInfo.failed_entry_count
        $summary.project_template_smoke.dirty_schema_baseline_count = $projectTemplateSmokeDirtySchemaBaselineCount
        $summary.project_template_smoke.schema_baseline_drift_count = $projectTemplateSmokeSchemaBaselineDriftCount
        $summary.project_template_smoke.visual_entry_count = [int]$projectTemplateSmokeInfo.visual_entry_count
        $summary.project_template_smoke.visual_verdict = [string]$projectTemplateSmokeInfo.visual_verdict
        $summary.project_template_smoke.manual_review_pending_count = [int]$projectTemplateSmokeInfo.manual_review_pending_count
        $summary.project_template_smoke.visual_review_undetermined_count = [int]$projectTemplateSmokeInfo.visual_review_undetermined_count
        $summary.project_template_smoke.overall_status = [string]$projectTemplateSmokeInfo.overall_status
        $summary.project_template_smoke.schema_patch_approval_pending_count = [int]$projectTemplateSmokeApprovalGate.pending_count
        $summary.project_template_smoke.schema_patch_approval_approved_count = [int]$projectTemplateSmokeApprovalGate.approved_count
        $summary.project_template_smoke.schema_patch_approval_rejected_count = [int]$projectTemplateSmokeApprovalGate.rejected_count
        $summary.project_template_smoke.schema_patch_approval_compliance_issue_count = [int]$projectTemplateSmokeApprovalGate.compliance_issue_count
        $summary.project_template_smoke.schema_patch_approval_invalid_result_count = [int]$projectTemplateSmokeApprovalGate.invalid_result_count
        $summary.project_template_smoke.schema_patch_approval_gate_status = [string]$projectTemplateSmokeApprovalGate.gate_status
        $summary.project_template_smoke.schema_patch_approval_gate_blocked = [bool]$projectTemplateSmokeApprovalGate.gate_blocked
        $summary.project_template_smoke.schema_patch_approval_history_json = $schemaApprovalHistoryJsonPath
        $summary.project_template_smoke.schema_patch_approval_history_markdown = $schemaApprovalHistoryMarkdownPath
        $summary.steps.project_template_smoke.schema_patch_approval_history_json = $schemaApprovalHistoryJsonPath
        $summary.steps.project_template_smoke.schema_patch_approval_history_markdown = $schemaApprovalHistoryMarkdownPath

        if ([bool]$projectTemplateSmokeApprovalGate.gate_blocked) {
            $summary.steps.project_template_smoke.status = "failed"
            throw "Project template smoke schema approval gate blocked."
        }

        if ($projectTemplateSmokeExitCode -ne 0) {
            $failedEntryCount = [int]$projectTemplateSmokeInfo.failed_entry_count
            if ($projectTemplateSmokeDirtySchemaBaselineCount -gt 0 -and
                $projectTemplateSmokeSchemaBaselineDriftCount -gt 0) {
                throw "Project template smoke schema baseline lint and drift failures detected."
            }
            if ($projectTemplateSmokeDirtySchemaBaselineCount -gt 0 -and
                $failedEntryCount -gt $projectTemplateSmokeDirtySchemaBaselineCount) {
                throw "Project template smoke schema baseline lint and entry failures detected."
            }
            if ($projectTemplateSmokeDirtySchemaBaselineCount -gt 0) {
                throw "Project template smoke schema baseline lint failed."
            }
            if ($projectTemplateSmokeSchemaBaselineDriftCount -gt 0) {
                throw "Project template smoke schema baseline drift detected."
            }
            if ($failedEntryCount -gt 0) {
                throw "Project template smoke reported failing entries."
            }

            throw "Project template smoke failed."
        }
    }

    if (-not $SkipInstallSmoke) {
        $activeStep = "install_smoke"
        Write-Step "Running install + find_package smoke"
        $installSmokeOutput = Invoke-ChildPowerShellInMsvcEnv -MsvcBootstrap $msvcBootstrap `
            -ScriptPath $installSmokeScript `
            -Arguments @(
                "-BuildDir"
                $resolvedBuildDir
                "-InstallDir"
                $resolvedInstallDir
                "-ConsumerBuildDir"
                $resolvedConsumerBuildDir
                "-Generator"
                $Generator
                "-Config"
                $Config
            ) `
            -FailureMessage "Install + find_package smoke failed."
        $installSmokeInfo = Parse-InstallSmokeOutput -Lines $installSmokeOutput
        $summary.steps.install_smoke.status = "completed"
        $summary.steps.install_smoke.install_prefix = $installSmokeInfo.install_prefix
        $summary.steps.install_smoke.consumer_document = $installSmokeInfo.consumer_document
    }

    if (-not $SkipVisualGate) {
        $activeStep = "visual_gate"
        Write-Step "Running Word visual release gate"
        $visualGateBuildDirArguments = @(
            "-SmokeBuildDir",
            "-FixedGridBuildDir",
            "-SectionPageSetupBuildDir",
            "-PageNumberFieldsBuildDir",
            "-BookmarkFloatingImageBuildDir",
            "-BookmarkImageBuildDir",
            "-BookmarkBlockVisibilityBuildDir",
            "-TemplateBookmarkParagraphsBuildDir",
            "-BookmarkTableReplacementBuildDir",
            "-ParagraphListBuildDir",
            "-ParagraphNumberingBuildDir",
            "-ParagraphRunStyleBuildDir",
            "-ParagraphStyleNumberingBuildDir",
            "-FillBookmarksBuildDir",
            "-AppendImageBuildDir",
            "-FloatingImageZOrderBuildDir",
            "-TableRowBuildDir",
            "-TableRowHeightBuildDir",
            "-TableRowCantSplitBuildDir",
            "-TableRowRepeatHeaderBuildDir",
            "-TableCellFillBuildDir",
            "-TableCellBorderBuildDir",
            "-TableCellWidthBuildDir",
            "-TableCellMarginBuildDir",
            "-TableCellVerticalAlignmentBuildDir",
            "-TableCellTextDirectionBuildDir",
            "-TableCellMergeBuildDir",
            "-TemplateBookmarkMultilineBuildDir",
            "-SectionTextMultilineBuildDir",
            "-RemoveBookmarkBlockBuildDir",
            "-TemplateBookmarkParagraphsPaginationBuildDir",
            "-SectionOrderBuildDir",
            "-SectionPartRefsBuildDir",
            "-RunFontLanguageBuildDir",
            "-EnsureStyleBuildDir",
            "-TableStyleQualityBuildDir",
            "-TemplateTableCliBookmarkBuildDir",
            "-TemplateTableCliColumnBuildDir",
            "-TemplateTableCliDirectColumnBuildDir",
            "-TemplateTableCliBuildDir",
            "-TemplateTableCliMergeUnmergeBuildDir",
            "-TemplateTableCliDirectBuildDir",
            "-TemplateTableCliDirectMergeUnmergeBuildDir",
            "-TemplateTableCliSectionKindBuildDir",
            "-TemplateTableCliSectionKindRowBuildDir",
            "-TemplateTableCliSectionKindColumnBuildDir",
            "-TemplateTableCliSectionKindMergeUnmergeBuildDir",
            "-TemplateTableCliSelectorBuildDir",
            "-ReplaceRemoveImageBuildDir"
        )
        $visualGateArgs = @(
            "-GateOutputDir"
            $resolvedGateOutputDir
            "-TaskOutputRoot"
            $resolvedTaskOutputRoot
            "-ReviewMode"
            $ReviewMode
            "-Dpi"
            $Dpi.ToString()
            "-SkipBuild"
        )
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-SmokeReviewVerdict" `
            -Verdict $SmokeReviewVerdict `
            -NoteArgument "-SmokeReviewNote" `
            -Note $SmokeReviewNote
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-FixedGridReviewVerdict" `
            -Verdict $FixedGridReviewVerdict `
            -NoteArgument "-FixedGridReviewNote" `
            -Note $FixedGridReviewNote
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-SectionPageSetupReviewVerdict" `
            -Verdict $SectionPageSetupReviewVerdict `
            -NoteArgument "-SectionPageSetupReviewNote" `
            -Note $SectionPageSetupReviewNote
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-PageNumberFieldsReviewVerdict" `
            -Verdict $PageNumberFieldsReviewVerdict `
            -NoteArgument "-PageNumberFieldsReviewNote" `
            -Note $PageNumberFieldsReviewNote
        $visualGateArgs = Add-OptionalVisualReviewArguments -Arguments $visualGateArgs `
            -VerdictArgument "-CuratedVisualReviewVerdict" `
            -Verdict $CuratedVisualReviewVerdict `
            -NoteArgument "-CuratedVisualReviewNote" `
            -Note $CuratedVisualReviewNote
        foreach ($argumentName in $visualGateBuildDirArguments) {
            $visualGateArgs += @(
                $argumentName
                $resolvedBuildDir
            )
        }
        if ($IncludeTableStyleQuality) {
            $visualGateArgs += "-IncludeTableStyleQuality"
        }
        if ($SkipReviewTasks) {
            $visualGateArgs += "-SkipReviewTasks"
        }
        if ($SkipSectionPageSetup) {
            $visualGateArgs += "-SkipSectionPageSetup"
        }
        if ($SkipPageNumberFields) {
            $visualGateArgs += "-SkipPageNumberFields"
        }
        if ($RefreshReadmeAssets) {
            $visualGateArgs += @(
                "-RefreshReadmeAssets"
                "-ReadmeAssetsDir"
                $ReadmeAssetsDir
            )
        }
        if ($KeepWordOpen) {
            $visualGateArgs += "-KeepWordOpen"
        }
        if ($VisibleWord) {
            $visualGateArgs += "-VisibleWord"
        }

        $visualGateOutput = Invoke-ChildPowerShell -ScriptPath $visualGateScript `
            -Arguments $visualGateArgs `
            -FailureMessage "Word visual release gate failed."
        $visualGateInfo = Parse-VisualGateOutput -Lines $visualGateOutput
        $summary.steps.visual_gate.status = "completed"
        $summary.steps.visual_gate.summary_json = $visualGateInfo.gate_summary
        $summary.steps.visual_gate.final_review = $visualGateInfo.gate_final_review
        if ($visualGateInfo.document_task_dir) {
            $summary.steps.visual_gate.document_task_dir = $visualGateInfo.document_task_dir
        }
        if ($visualGateInfo.fixed_grid_task_dir) {
            $summary.steps.visual_gate.fixed_grid_task_dir = $visualGateInfo.fixed_grid_task_dir
        }
        if ($visualGateInfo.section_page_setup_task_dir) {
            $summary.steps.visual_gate.section_page_setup_task_dir = $visualGateInfo.section_page_setup_task_dir
        }
        if ($visualGateInfo.page_number_fields_task_dir) {
            $summary.steps.visual_gate.page_number_fields_task_dir = $visualGateInfo.page_number_fields_task_dir
        }

        $gateSummary = Get-Content -Raw -LiteralPath $visualGateInfo.gate_summary | ConvertFrom-Json
        $gateVisualVerdict = Get-OptionalPropertyValue -Object $gateSummary -Name "visual_verdict"
        if (-not [string]::IsNullOrWhiteSpace([string]$gateVisualVerdict)) {
            $summary.visual_verdict = [string]$gateVisualVerdict
            $summary.steps.visual_gate.visual_verdict = [string]$gateVisualVerdict
        }
        $reviewTaskSummary = Get-CompleteVisualGateReviewTaskSummary -Summary (Get-OptionalPropertyValue -Object $gateSummary -Name "review_task_summary")
        if ($null -ne $reviewTaskSummary) {
            $summary.steps.visual_gate.review_task_summary = $reviewTaskSummary
        }
        Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "smoke" `
            -FlowInfo (Get-OptionalPropertyValue -Object $gateSummary -Name "smoke")
        Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "fixed_grid" `
            -FlowInfo (Get-OptionalPropertyValue -Object $gateSummary -Name "fixed_grid")
        Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "section_page_setup" `
            -FlowInfo (Get-OptionalPropertyValue -Object $gateSummary -Name "section_page_setup")
        Add-GateFlowReviewSummary -Target $summary.steps.visual_gate -Prefix "page_number_fields" `
            -FlowInfo (Get-OptionalPropertyValue -Object $gateSummary -Name "page_number_fields")
        $curatedVisualReviewEntries = @(Get-CuratedVisualReviewSummaryEntries -GateSummary $gateSummary)
        if ($curatedVisualReviewEntries.Count -gt 0) {
            $summary.steps.visual_gate.curated_visual_regressions = $curatedVisualReviewEntries
        }

        $readmeGalleryInfo = Get-ReadmeGalleryInfoFromGateSummary -GateSummaryPath $visualGateInfo.gate_summary
        $summary.readme_gallery = $readmeGalleryInfo
        $summary.steps.visual_gate.readme_gallery_status = $readmeGalleryInfo.status
        if ($readmeGalleryInfo.Contains("assets_dir")) {
            $summary.steps.visual_gate.readme_gallery_assets_dir = $readmeGalleryInfo.assets_dir
        }
    }

    $summary.execution_status = "pass"
} catch {
    $summary.execution_status = "fail"
    $summary.failed_step = $activeStep
    $summary.error = $_.Exception.Message
    if ($activeStep -and $summary.steps[$activeStep].status -eq "pending") {
        $summary.steps[$activeStep].status = "failed"
    }
    throw
} finally {
    ($summary | ConvertTo-Json -Depth 8) | Set-Content -Path $summaryPath -Encoding UTF8

    if ($projectTemplateSmokeRequested) {
        try {
            $historyInfo = Invoke-ProjectTemplateSchemaApprovalHistory `
                -ScriptPath $projectTemplateSchemaApprovalHistoryScript `
                -ReleaseSummaryPath $summaryPath `
                -ProjectTemplateSmokeSummaryPath $summary.project_template_smoke.summary_json `
                -OutputJson $schemaApprovalHistoryJsonPath `
                -OutputMarkdown $schemaApprovalHistoryMarkdownPath
            $summary.project_template_smoke.schema_patch_approval_history_status = [string]$historyInfo.status
            $summary.project_template_smoke.schema_patch_approval_history_input_count = [int]$historyInfo.input_count
            $summary.project_template_smoke.schema_patch_approval_history_error = ""
            $summary.steps.project_template_smoke.schema_patch_approval_history_status = [string]$historyInfo.status
            $summary.steps.project_template_smoke.schema_patch_approval_history_input_count = [int]$historyInfo.input_count
            $summary.steps.project_template_smoke.schema_patch_approval_history_error = ""
        } catch {
            $historyError = $_.Exception.Message
            $summary.project_template_smoke.schema_patch_approval_history_status = "failed"
            $summary.project_template_smoke.schema_patch_approval_history_error = $historyError
            $summary.steps.project_template_smoke.schema_patch_approval_history_status = "failed"
            $summary.steps.project_template_smoke.schema_patch_approval_history_error = $historyError
            Write-Step "Project template schema approval history generation failed: $historyError"
        }

        $schemaApprovalHistorySummary = Read-ProjectTemplateSchemaApprovalHistorySummary -Path $schemaApprovalHistoryJsonPath
        $projectTemplateSmokeSummaryForBlocker = if (-not [string]::IsNullOrWhiteSpace($summary.project_template_smoke.summary_json) -and
            (Test-Path -LiteralPath $summary.project_template_smoke.summary_json)) {
            Get-Content -Raw -Encoding UTF8 -LiteralPath $summary.project_template_smoke.summary_json | ConvertFrom-Json
        } else {
            $summary.project_template_smoke
        }
        $schemaApprovalReleaseBlocker = New-ProjectTemplateSchemaApprovalReleaseBlocker `
            -ProjectTemplateSmokeSummary $projectTemplateSmokeSummaryForBlocker `
            -HistorySummary $schemaApprovalHistorySummary `
            -SummaryJsonPath $summary.project_template_smoke.summary_json `
            -HistoryJsonPath $summary.project_template_smoke.schema_patch_approval_history_json `
            -HistoryMarkdownPath $summary.project_template_smoke.schema_patch_approval_history_markdown
        Set-ProjectTemplateSchemaApprovalReleaseBlocker `
            -ReleaseSummary $summary `
            -Blocker $schemaApprovalReleaseBlocker
        ($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8
    }

    $releaseWarnings = @(Get-PdfVisualGateAttemptReleaseWarnings -ReleaseSummary $summary -RepoRoot $repoRoot)
    Set-ReleaseSummaryWarnings -ReleaseSummary $summary -Warnings $releaseWarnings
    ($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8

    $effectiveReleaseGovernanceHandoffInputJson = @(Get-ReleaseGovernanceHandoffEffectiveInputJson `
            -InputJson $resolvedReleaseGovernanceHandoffInputJson `
            -ReleaseSummaryPath $summaryPath)
    if ($releaseGovernanceHandoffRequested) {
        $summary.release_governance_handoff.input_json = @($effectiveReleaseGovernanceHandoffInputJson)
    }

    if ($releaseBlockerRollupRequested) {
        try {
            Write-Step "Building release blocker rollup"
            Invoke-ReleaseBlockerRollup `
                -ScriptPath $releaseBlockerRollupScript `
                -InputJson $resolvedReleaseBlockerRollupInputJson `
                -InputRoot $resolvedReleaseBlockerRollupInputRoot `
                -OutputDir $resolvedReleaseBlockerRollupOutputDir `
                -SummaryJson $releaseBlockerRollupSummaryPath `
                -ReportMarkdown $releaseBlockerRollupMarkdownPath `
                -FailOnBlocker ([bool]$ReleaseBlockerRollupFailOnBlocker) `
                -FailOnWarning ([bool]$ReleaseBlockerRollupFailOnWarning)
            $rollupSummary = Read-ReleaseBlockerRollupSummary -Path $releaseBlockerRollupSummaryPath
            $summary.release_blocker_rollup.status = if ($null -eq $rollupSummary) { "missing_summary" } else { [string]$rollupSummary.status }
            $summary.release_blocker_rollup.source_report_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.source_report_count }
            $summary.release_blocker_rollup.source_failure_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.source_failure_count }
            $summary.release_blocker_rollup.release_blocker_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.release_blocker_count }
            $summary.release_blocker_rollup.release_blockers = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "release_blockers") }
            $summary.release_blocker_rollup.action_item_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.action_item_count }
            $summary.release_blocker_rollup.action_items = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "action_items") }
            $summary.release_blocker_rollup.warning_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.warning_count }
            $summary.release_blocker_rollup.warnings = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "warnings") }
            $summary.release_blocker_rollup.error = ""
            $summary.steps.release_blocker_rollup.status = $summary.release_blocker_rollup.status
            $summary.steps.release_blocker_rollup.source_report_count = $summary.release_blocker_rollup.source_report_count
            $summary.steps.release_blocker_rollup.source_failure_count = $summary.release_blocker_rollup.source_failure_count
            $summary.steps.release_blocker_rollup.release_blocker_count = $summary.release_blocker_rollup.release_blocker_count
            $summary.steps.release_blocker_rollup.release_blockers = @($summary.release_blocker_rollup.release_blockers)
            $summary.steps.release_blocker_rollup.action_item_count = $summary.release_blocker_rollup.action_item_count
            $summary.steps.release_blocker_rollup.action_items = @($summary.release_blocker_rollup.action_items)
            $summary.steps.release_blocker_rollup.warning_count = $summary.release_blocker_rollup.warning_count
            $summary.steps.release_blocker_rollup.warnings = @($summary.release_blocker_rollup.warnings)
            $summary.steps.release_blocker_rollup.error = ""
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
        } catch {
            $rollupError = $_.Exception.Message
            $rollupSummary = Read-ReleaseBlockerRollupSummary -Path $releaseBlockerRollupSummaryPath
            $summary.release_blocker_rollup.status = "failed"
            $summary.release_blocker_rollup.source_report_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.source_report_count }
            $summary.release_blocker_rollup.source_failure_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.source_failure_count }
            $summary.release_blocker_rollup.release_blocker_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.release_blocker_count }
            $summary.release_blocker_rollup.release_blockers = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "release_blockers") }
            $summary.release_blocker_rollup.action_item_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.action_item_count }
            $summary.release_blocker_rollup.action_items = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "action_items") }
            $summary.release_blocker_rollup.warning_count = if ($null -eq $rollupSummary) { 0 } else { [int]$rollupSummary.warning_count }
            $summary.release_blocker_rollup.warnings = if ($null -eq $rollupSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $rollupSummary -Name "warnings") }
            $summary.release_blocker_rollup.error = $rollupError
            $summary.steps.release_blocker_rollup.status = "failed"
            $summary.steps.release_blocker_rollup.source_report_count = $summary.release_blocker_rollup.source_report_count
            $summary.steps.release_blocker_rollup.source_failure_count = $summary.release_blocker_rollup.source_failure_count
            $summary.steps.release_blocker_rollup.release_blocker_count = $summary.release_blocker_rollup.release_blocker_count
            $summary.steps.release_blocker_rollup.release_blockers = @($summary.release_blocker_rollup.release_blockers)
            $summary.steps.release_blocker_rollup.action_item_count = $summary.release_blocker_rollup.action_item_count
            $summary.steps.release_blocker_rollup.action_items = @($summary.release_blocker_rollup.action_items)
            $summary.steps.release_blocker_rollup.warning_count = $summary.release_blocker_rollup.warning_count
            $summary.steps.release_blocker_rollup.warnings = @($summary.release_blocker_rollup.warnings)
            $summary.steps.release_blocker_rollup.error = $rollupError
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
            Write-Step "Release blocker rollup failed: $rollupError"
            if ($ReleaseBlockerRollupFailOnBlocker -or $ReleaseBlockerRollupFailOnWarning) {
                $releaseBlockerRollupFailure = $rollupError
            }
        }
    }

    if ($releaseGovernanceHandoffRequested) {
        try {
            Write-Step "Building release governance handoff"
            Invoke-ReleaseGovernanceHandoff `
                -ScriptPath $releaseGovernanceHandoffScript `
                -InputRoot $resolvedReleaseGovernanceHandoffInputRoot `
                -InputJson $effectiveReleaseGovernanceHandoffInputJson `
                -OutputDir $resolvedReleaseGovernanceHandoffOutputDir `
                -SummaryJson $releaseGovernanceHandoffSummaryPath `
                -ReportMarkdown $releaseGovernanceHandoffMarkdownPath `
                -ExpectedReportProfile $ReleaseGovernanceHandoffExpectedReportProfile `
                -IncludeRollup ([bool]$ReleaseGovernanceHandoffIncludeRollup) `
                -FailOnMissing ([bool]$ReleaseGovernanceHandoffFailOnMissing) `
                -FailOnBlocker ([bool]$ReleaseGovernanceHandoffFailOnBlocker) `
                -FailOnWarning ([bool]$ReleaseGovernanceHandoffFailOnWarning)
            $handoffSummary = Read-ReleaseGovernanceHandoffSummary -Path $releaseGovernanceHandoffSummaryPath
            $summary.release_governance_handoff.status = if ($null -eq $handoffSummary) { "missing_summary" } else { [string]$handoffSummary.status }
            $summary.release_governance_handoff.expected_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.expected_report_count }
            $summary.release_governance_handoff.loaded_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.loaded_report_count }
            $summary.release_governance_handoff.missing_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.missing_report_count }
            $summary.release_governance_handoff.failed_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.failed_report_count }
            $summary.release_governance_handoff.release_blocker_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.release_blocker_count }
            $summary.release_governance_handoff.release_blockers = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "release_blockers") }
            $summary.release_governance_handoff.action_item_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.action_item_count }
            $summary.release_governance_handoff.action_items = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "action_items") }
            $summary.release_governance_handoff.warning_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.warning_count }
            $summary.release_governance_handoff.warnings = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "warnings") }
            [object[]]$handoffReports = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "reports") }
            [object[]]$handoffGovernanceMetrics = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "governance_metrics") }
            $summary.release_governance_handoff.report_count = @($handoffReports).Count
            $summary.release_governance_handoff.reports = @($handoffReports)
            $summary.release_governance_handoff.governance_metric_count = if ($null -eq $handoffSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffSummary -Name "governance_metric_count" -DefaultValue @($handoffGovernanceMetrics).Count) }
            $summary.release_governance_handoff.governance_metrics = @($handoffGovernanceMetrics)
            $summary.release_governance_handoff.project_template_delivery_readiness_contract = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "project_template_delivery_readiness_contract" }
            $summary.release_governance_handoff.project_template_onboarding_governance_contract = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "project_template_onboarding_governance_contract" }
            $summary.governance_metric_count = $summary.release_governance_handoff.governance_metric_count
            $summary.governance_metrics = @($summary.release_governance_handoff.governance_metrics)
            $handoffRollup = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "release_blocker_rollup" }
            $summary.release_governance_handoff.release_blocker_rollup = $handoffRollup
            $summary.release_governance_handoff.manifest_signoff_entrypoints_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "manifest_signoff_entrypoints_source_report_count") }
            $summary.release_governance_handoff.manifest_signoff_entrypoints_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "manifest_signoff_entrypoints_source_reports") }
            $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "project_template_readiness_checklist_entrypoints_source_report_count") }
            $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "project_template_readiness_checklist_entrypoints_source_reports") }
            $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count") }
            $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports") }
            $summary.release_governance_handoff.error = ""
            $summary.steps.release_governance_handoff.status = $summary.release_governance_handoff.status
            $summary.steps.release_governance_handoff.expected_report_count = $summary.release_governance_handoff.expected_report_count
            $summary.steps.release_governance_handoff.loaded_report_count = $summary.release_governance_handoff.loaded_report_count
            $summary.steps.release_governance_handoff.missing_report_count = $summary.release_governance_handoff.missing_report_count
            $summary.steps.release_governance_handoff.failed_report_count = $summary.release_governance_handoff.failed_report_count
            $summary.steps.release_governance_handoff.release_blocker_count = $summary.release_governance_handoff.release_blocker_count
            $summary.steps.release_governance_handoff.release_blockers = @($summary.release_governance_handoff.release_blockers)
            $summary.steps.release_governance_handoff.action_item_count = $summary.release_governance_handoff.action_item_count
            $summary.steps.release_governance_handoff.action_items = @($summary.release_governance_handoff.action_items)
            $summary.steps.release_governance_handoff.warning_count = $summary.release_governance_handoff.warning_count
            $summary.steps.release_governance_handoff.warnings = @($summary.release_governance_handoff.warnings)
            $summary.steps.release_governance_handoff.report_count = $summary.release_governance_handoff.report_count
            $summary.steps.release_governance_handoff.reports = @($summary.release_governance_handoff.reports)
            $summary.steps.release_governance_handoff.governance_metric_count = $summary.release_governance_handoff.governance_metric_count
            $summary.steps.release_governance_handoff.governance_metrics = @($summary.release_governance_handoff.governance_metrics)
            $summary.steps.release_governance_handoff.project_template_delivery_readiness_contract = $summary.release_governance_handoff.project_template_delivery_readiness_contract
            $summary.steps.release_governance_handoff.project_template_onboarding_governance_contract = $summary.release_governance_handoff.project_template_onboarding_governance_contract
            $summary.steps.release_governance_handoff.release_blocker_rollup = $summary.release_governance_handoff.release_blocker_rollup
            $summary.steps.release_governance_handoff.manifest_signoff_entrypoints_source_report_count = $summary.release_governance_handoff.manifest_signoff_entrypoints_source_report_count
            $summary.steps.release_governance_handoff.manifest_signoff_entrypoints_source_reports = @($summary.release_governance_handoff.manifest_signoff_entrypoints_source_reports)
            $summary.steps.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count = $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count
            $summary.steps.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports = @($summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports)
            $summary.steps.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count
            $summary.steps.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @($summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports)
            $summary.steps.release_governance_handoff.error = ""
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
        } catch {
            $handoffError = $_.Exception.Message
            $handoffSummary = Read-ReleaseGovernanceHandoffSummary -Path $releaseGovernanceHandoffSummaryPath
            $summary.release_governance_handoff.status = "failed"
            $summary.release_governance_handoff.expected_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.expected_report_count }
            $summary.release_governance_handoff.loaded_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.loaded_report_count }
            $summary.release_governance_handoff.missing_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.missing_report_count }
            $summary.release_governance_handoff.failed_report_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.failed_report_count }
            $summary.release_governance_handoff.release_blocker_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.release_blocker_count }
            $summary.release_governance_handoff.release_blockers = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "release_blockers") }
            $summary.release_governance_handoff.action_item_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.action_item_count }
            $summary.release_governance_handoff.action_items = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "action_items") }
            $summary.release_governance_handoff.warning_count = if ($null -eq $handoffSummary) { 0 } else { [int]$handoffSummary.warning_count }
            $summary.release_governance_handoff.warnings = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "warnings") }
            [object[]]$handoffReports = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "reports") }
            [object[]]$handoffGovernanceMetrics = if ($null -eq $handoffSummary) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffSummary -Name "governance_metrics") }
            $summary.release_governance_handoff.report_count = @($handoffReports).Count
            $summary.release_governance_handoff.reports = @($handoffReports)
            $summary.release_governance_handoff.governance_metric_count = if ($null -eq $handoffSummary) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffSummary -Name "governance_metric_count" -DefaultValue @($handoffGovernanceMetrics).Count) }
            $summary.release_governance_handoff.governance_metrics = @($handoffGovernanceMetrics)
            $summary.release_governance_handoff.project_template_delivery_readiness_contract = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "project_template_delivery_readiness_contract" }
            $summary.release_governance_handoff.project_template_onboarding_governance_contract = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "project_template_onboarding_governance_contract" }
            $summary.governance_metric_count = $summary.release_governance_handoff.governance_metric_count
            $summary.governance_metrics = @($summary.release_governance_handoff.governance_metrics)
            $handoffRollup = if ($null -eq $handoffSummary) { $null } else { Get-OptionalPropertyValue -Object $handoffSummary -Name "release_blocker_rollup" }
            $summary.release_governance_handoff.release_blocker_rollup = $handoffRollup
            $summary.release_governance_handoff.manifest_signoff_entrypoints_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "manifest_signoff_entrypoints_source_report_count") }
            $summary.release_governance_handoff.manifest_signoff_entrypoints_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "manifest_signoff_entrypoints_source_reports") }
            $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "project_template_readiness_checklist_entrypoints_source_report_count") }
            $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "project_template_readiness_checklist_entrypoints_source_reports") }
            $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = if ($null -eq $handoffRollup) { 0 } else { [int](Get-OptionalIntegerProperty -Object $handoffRollup -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count") }
            $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = if ($null -eq $handoffRollup) { @() } else { @(Get-OptionalObjectArrayProperty -Object $handoffRollup -Name "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports") }
            $summary.release_governance_handoff.error = $handoffError
            $summary.steps.release_governance_handoff.status = "failed"
            $summary.steps.release_governance_handoff.expected_report_count = $summary.release_governance_handoff.expected_report_count
            $summary.steps.release_governance_handoff.loaded_report_count = $summary.release_governance_handoff.loaded_report_count
            $summary.steps.release_governance_handoff.missing_report_count = $summary.release_governance_handoff.missing_report_count
            $summary.steps.release_governance_handoff.failed_report_count = $summary.release_governance_handoff.failed_report_count
            $summary.steps.release_governance_handoff.release_blocker_count = $summary.release_governance_handoff.release_blocker_count
            $summary.steps.release_governance_handoff.release_blockers = @($summary.release_governance_handoff.release_blockers)
            $summary.steps.release_governance_handoff.action_item_count = $summary.release_governance_handoff.action_item_count
            $summary.steps.release_governance_handoff.action_items = @($summary.release_governance_handoff.action_items)
            $summary.steps.release_governance_handoff.warning_count = $summary.release_governance_handoff.warning_count
            $summary.steps.release_governance_handoff.warnings = @($summary.release_governance_handoff.warnings)
            $summary.steps.release_governance_handoff.report_count = $summary.release_governance_handoff.report_count
            $summary.steps.release_governance_handoff.reports = @($summary.release_governance_handoff.reports)
            $summary.steps.release_governance_handoff.governance_metric_count = $summary.release_governance_handoff.governance_metric_count
            $summary.steps.release_governance_handoff.governance_metrics = @($summary.release_governance_handoff.governance_metrics)
            $summary.steps.release_governance_handoff.project_template_delivery_readiness_contract = $summary.release_governance_handoff.project_template_delivery_readiness_contract
            $summary.steps.release_governance_handoff.project_template_onboarding_governance_contract = $summary.release_governance_handoff.project_template_onboarding_governance_contract
            $summary.steps.release_governance_handoff.release_blocker_rollup = $summary.release_governance_handoff.release_blocker_rollup
            $summary.steps.release_governance_handoff.manifest_signoff_entrypoints_source_report_count = $summary.release_governance_handoff.manifest_signoff_entrypoints_source_report_count
            $summary.steps.release_governance_handoff.manifest_signoff_entrypoints_source_reports = @($summary.release_governance_handoff.manifest_signoff_entrypoints_source_reports)
            $summary.steps.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count = $summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_report_count
            $summary.steps.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports = @($summary.release_governance_handoff.project_template_readiness_checklist_entrypoints_source_reports)
            $summary.steps.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count = $summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_report_count
            $summary.steps.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports = @($summary.release_governance_handoff.release_entry_project_template_readiness_checklist_material_safety_audit_source_reports)
            $summary.steps.release_governance_handoff.error = $handoffError
            ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8
            Write-Step "Release governance handoff failed: $handoffError"
            if ($ReleaseGovernanceHandoffFailOnMissing -or
                $ReleaseGovernanceHandoffFailOnBlocker -or
                $ReleaseGovernanceHandoffFailOnWarning) {
                $releaseGovernanceHandoffFailure = $handoffError
            }
        }
    }

    $summary = Convert-ReleaseMaterialObject -RepoRoot $repoRoot -Value $summary
    ($summary | ConvertTo-Json -Depth 12) | Set-Content -Path $summaryPath -Encoding UTF8

    $repoRootDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $repoRoot
    $summaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summaryPath
    $buildDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedBuildDir
    $installDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedInstallDir
    $consumerBuildDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedConsumerBuildDir
    $gateOutputDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedGateOutputDir
    $taskOutputRootDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $resolvedTaskOutputRoot
    $templateSchemaBaselineDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema.baseline
    $templateSchemaGeneratedDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema.generated_output
    $templateSchemaManifestDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema_manifest.manifest_path
    $templateSchemaManifestSummaryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema_manifest.summary_json
    $templateSchemaManifestOutputDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.template_schema_manifest.output_dir
    $projectTemplateSmokeManifestDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.manifest_path
    $projectTemplateSmokeSummaryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.summary_json
    $projectTemplateSmokeOutputDirDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.output_dir
    $projectTemplateSmokeCandidateDiscoveryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.candidate_discovery_json
    $releaseGovernanceHandoffSummaryDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.release_governance_handoff.summary_json
    $releaseGovernanceHandoffReportDisplay = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.release_governance_handoff.report_markdown
    $releaseHandoffDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseHandoffPath
    $releaseBodyDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseBodyZhCnPath
    $releaseSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseSummaryZhCnPath
    $artifactGuideDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $artifactGuidePath
    $reviewerChecklistDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $reviewerChecklistPath
    $schemaApprovalHistoryJsonDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.schema_patch_approval_history_json
    $schemaApprovalHistoryMarkdownDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.project_template_smoke.schema_patch_approval_history_markdown
    $startHereDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $startHerePath
    $pdfVisualGateSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_gate.summary_json
    $pdfVisualGateContactSheetDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_gate.aggregate_contact_sheet
    $pdfVisualGateAttemptSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_gate_attempt.summary_json
    $pdfVisualGateAttemptContactSheetDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_gate_attempt.aggregate_contact_sheet
    $pdfVisualSegmentedGateSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_segmented_gate.summary_json
    $pdfVisualSegmentedGateContactSheetDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_visual_segmented_gate.aggregate_contact_sheet
    $pdfFullCtestReadinessSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_full_ctest_readiness.summary_json
    $pdfFullCtestSummaryDisplayPath = if (-not [string]::IsNullOrWhiteSpace([string]$summary.steps.pdf_full_ctest_readiness.full_ctest_summary_json_display)) {
        [string]$summary.steps.pdf_full_ctest_readiness.full_ctest_summary_json_display
    } else {
        Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.steps.pdf_full_ctest_readiness.full_ctest_summary_json
    }
    $pdfBoundedCtestSubsetsDisplay = if (@($summary.steps.pdf_bounded_ctest.subsets).Count -gt 0) {
        @($summary.steps.pdf_bounded_ctest.subsets) -join ", "
    } else {
        "(not available)"
    }
    $pdfBoundedCtestSummaryDisplay = if (@($summary.steps.pdf_bounded_ctest.summary_json_display).Count -gt 0) {
        @($summary.steps.pdf_bounded_ctest.summary_json_display) -join ", "
    } else {
        "(not available)"
    }

    $readmeGalleryStatusLine = switch ($summary.readme_gallery.status) {
        "completed" {
            "- README gallery refresh: completed ($(Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.readme_gallery.assets_dir))"
        }
        "not_requested" {
            "- README gallery refresh: not requested"
        }
        "visual_gate_skipped" {
            "- README gallery refresh: unavailable (visual gate skipped)"
        }
        default {
            "- README gallery refresh: $($summary.readme_gallery.status)"
        }
    }
    $visualGateReviewTaskSummaryLine = Get-VisualGateReviewTaskSummaryLine `
        -VisualGateStep $summary.steps.visual_gate
    $visualGateReviewSummary = Get-VisualGateReviewSummaryMarkdown -RepoRoot $repoRoot `
        -VisualGateStep $summary.steps.visual_gate
    $releaseGovernanceRollupLines = New-Object 'System.Collections.Generic.List[string]'
    Add-ReleaseGovernanceRollupMarkdownSection `
        -Lines $releaseGovernanceRollupLines `
        -Summary $summary `
        -RepoRoot $repoRoot `
        -Heading "## Release blocker rollup details"
    $releaseGovernanceRollupMarkdown = if ($releaseGovernanceRollupLines.Count -gt 0) {
        ($releaseGovernanceRollupLines.ToArray() -join [Environment]::NewLine) + [Environment]::NewLine
    } else {
        ""
    }
    $releaseGovernanceHandoffLines = New-Object 'System.Collections.Generic.List[string]'
    Add-ReleaseGovernanceHandoffMarkdownSection `
        -Lines $releaseGovernanceHandoffLines `
        -Summary $summary `
        -RepoRoot $repoRoot `
        -Heading "## Release governance handoff details"
    $releaseGovernanceHandoffMarkdown = if ($releaseGovernanceHandoffLines.Count -gt 0) {
        ($releaseGovernanceHandoffLines.ToArray() -join [Environment]::NewLine) + [Environment]::NewLine
    } else {
        ""
    }
    $projectTemplateChecklistHandoffEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine -Summary $summary
    $projectTemplateChecklistMaterialSafetyAuditEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine -Summary $summary
    $projectTemplateChecklistEvidenceLines = New-Object 'System.Collections.Generic.List[string]'
    foreach ($evidenceLine in @(
            $projectTemplateChecklistHandoffEvidenceLine,
            $projectTemplateChecklistMaterialSafetyAuditEvidenceLine
        )) {
        if (-not [string]::IsNullOrWhiteSpace($evidenceLine)) {
            [void]$projectTemplateChecklistEvidenceLines.Add("- $evidenceLine")
        }
    }
    $projectTemplateChecklistEvidenceMarkdown = if ($projectTemplateChecklistEvidenceLines.Count -gt 0) {
        "## Project-template release entry evidence" + [Environment]::NewLine +
        [Environment]::NewLine +
        ($projectTemplateChecklistEvidenceLines.ToArray() -join [Environment]::NewLine) +
        [Environment]::NewLine
    } else {
        ""
    }

    $finalReview = @"
# Release Candidate Checks

- Generated at: $(Get-Date -Format s)
- Workspace: $repoRootDisplay
- Summary JSON: $summaryDisplayPath
- Execution status: $($summary.execution_status)
- Failed step: $($summary.failed_step)
- Error: $($summary.error)
- Release blockers: $($summary.release_blocker_count)
- Warnings: $($summary.warning_count)
- MSVC bootstrap mode: $($summary.msvc_bootstrap_mode)

## Step status

- Configure: $($summary.steps.configure.status)
- Build: $($summary.steps.build.status)
- Tests: $($summary.steps.tests.status)
- Template schema: $($summary.steps.template_schema.status)
- Template schema manifest: $($summary.steps.template_schema_manifest.status)
- Project template smoke: $($summary.steps.project_template_smoke.status)
- Install smoke: $($summary.steps.install_smoke.status)
- Visual gate: $($summary.steps.visual_gate.status)
- PDF visual gate: $($summary.steps.pdf_visual_gate.status)
- PDF visual gate verdict: $($summary.steps.pdf_visual_gate.verdict)
- PDF visual gate counts: $($summary.steps.pdf_visual_gate.visual_baseline_count) visual baselines, $($summary.steps.pdf_visual_gate.cjk_copy_search_count) CJK copy/search
- PDF visual gate manifest counts: $($summary.steps.pdf_visual_gate.visual_baseline_manifest_count) visual baseline manifest samples, $($summary.steps.pdf_visual_gate.cjk_manifest_count) CJK manifest samples
- PDF visual gate finalizable: $($summary.steps.pdf_visual_gate.finalizable)
- PDF visual gate attempt: $($summary.steps.pdf_visual_gate_attempt.status)
- PDF visual gate attempt verdict: $($summary.steps.pdf_visual_gate_attempt.verdict)
- PDF visual gate attempt full status: $($summary.steps.pdf_visual_gate_attempt.full_visual_gate_status)
- PDF visual gate attempt stages: $($summary.steps.pdf_visual_gate_attempt.passed_stage_count)/$($summary.steps.pdf_visual_gate_attempt.stage_count) passed, $($summary.steps.pdf_visual_gate_attempt.incomplete_stage_count) incomplete
- PDF visual gate attempt pdf_regression: $($summary.steps.pdf_visual_gate_attempt.pdf_regression_selected_test_count) selected, $($summary.steps.pdf_visual_gate_attempt.pdf_regression_failed_test_count) failed, $($summary.steps.pdf_visual_gate_attempt.pdf_regression_skipped_test_count) skipped
- PDF visual gate attempt render: $($summary.steps.pdf_visual_gate_attempt.visual_baseline_fresh_rendered_count)/$($summary.steps.pdf_visual_gate_attempt.expected_visual_render_count) fresh baselines, contact sheet $($summary.steps.pdf_visual_gate_attempt.aggregate_contact_sheet_status)
- PDF visual segmented gate: $($summary.steps.pdf_visual_segmented_gate.status)
- PDF visual segmented gate verdict: $($summary.steps.pdf_visual_segmented_gate.verdict)
- PDF visual segmented gate full status: $($summary.steps.pdf_visual_segmented_gate.full_visual_gate_status)
- PDF visual segmented gate scope: $($summary.steps.pdf_visual_segmented_gate.evidence_scope)
- PDF visual segmented gate slices: $($summary.steps.pdf_visual_segmented_gate.slice_pass_count)/$($summary.steps.pdf_visual_segmented_gate.slice_summary_count) pass
- PDF visual segmented gate coverage: $($summary.steps.pdf_visual_segmented_gate.covered_baseline_count)/$($summary.steps.pdf_visual_segmented_gate.expected_visual_render_count) baselines, contact sheet $($summary.steps.pdf_visual_segmented_gate.aggregate_contact_sheet_status)
- PDF visual release evidence accepted: $($summary.steps.pdf_full_ctest_readiness.visual_gate_release_evidence_accepted) (fresh full guarded $($summary.steps.pdf_full_ctest_readiness.visual_gate_fresh_full_guarded_evidence), pass summary before outer timeout $($summary.steps.pdf_full_ctest_readiness.visual_gate_pass_summary_before_outer_timeout), segmented full coverage $($summary.steps.pdf_full_ctest_readiness.visual_gate_segmented_full_coverage_evidence))
- PDF bounded CTest summaries: $($summary.steps.pdf_bounded_ctest.summary_count) summaries, $($summary.steps.pdf_bounded_ctest.pass_count) pass
- PDF bounded CTest subsets: $pdfBoundedCtestSubsetsDisplay
- PDF bounded CTest selected tests: $($summary.steps.pdf_bounded_ctest.selected_test_count)
- PDF bounded CTest skipped tests: $($summary.steps.pdf_bounded_ctest.skipped_test_count)
- PDF full CTest readiness: $($summary.steps.pdf_full_ctest_readiness.full_ctest_status) ($($summary.steps.pdf_full_ctest_readiness.completion_percent)% complete)
- PDF full CTest progress: $($summary.steps.pdf_full_ctest_readiness.completed_test_count)/$($summary.steps.pdf_full_ctest_readiness.selected_test_count) completed, $($summary.steps.pdf_full_ctest_readiness.not_run_test_count) not run
- PDF full CTest observed failures: $($summary.steps.pdf_full_ctest_readiness.failed_test_count), zero failed observed $($summary.steps.pdf_full_ctest_readiness.zero_failed_tests_observed)
- Release blocker rollup: $($summary.steps.release_blocker_rollup.status)
- Release governance handoff: $($summary.steps.release_governance_handoff.status)
$visualGateReviewTaskSummaryLine
$readmeGalleryStatusLine
$visualGateReviewSummary
$releaseGovernanceRollupMarkdown
$releaseGovernanceHandoffMarkdown
$projectTemplateChecklistEvidenceMarkdown
## Key outputs

- Build directory: $buildDirDisplay
- Install directory: $installDirDisplay
- Consumer build directory: $consumerBuildDirDisplay
- Visual gate output: $gateOutputDirDisplay
- Review task root: $taskOutputRootDisplay
- Template schema baseline: $templateSchemaBaselineDisplay
- Template schema generated output: $templateSchemaGeneratedDisplay
- Template schema manifest: $templateSchemaManifestDisplay
- Template schema manifest summary: $templateSchemaManifestSummaryDisplay
- Template schema manifest output dir: $templateSchemaManifestOutputDirDisplay
- Project template smoke manifest: $projectTemplateSmokeManifestDisplay
- Project template smoke summary: $projectTemplateSmokeSummaryDisplay
- Project template smoke output dir: $projectTemplateSmokeOutputDirDisplay
- Project template smoke candidate discovery: $projectTemplateSmokeCandidateDiscoveryDisplay
- Project template smoke candidate coverage: $($summary.project_template_smoke.registered_candidate_count)/$($summary.project_template_smoke.unregistered_candidate_count)/$($summary.project_template_smoke.excluded_candidate_count) registered/unregistered/excluded
- Project template smoke dirty schema baselines: $($summary.project_template_smoke.dirty_schema_baseline_count)
- Project template smoke schema baseline drifts: $($summary.project_template_smoke.schema_baseline_drift_count)
- Project template smoke schema approval gate: $($summary.project_template_smoke.schema_patch_approval_gate_status)
- Project template smoke schema approval compliance issues: $($summary.project_template_smoke.schema_patch_approval_compliance_issue_count)
- Project template smoke schema approval invalid results: $($summary.project_template_smoke.schema_patch_approval_invalid_result_count)
- Project template smoke schema approval history: $($summary.project_template_smoke.schema_patch_approval_history_status) ($schemaApprovalHistoryJsonDisplayPath, $schemaApprovalHistoryMarkdownDisplayPath)
- Release blocker rollup summary: $(Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.release_blocker_rollup.summary_json)
- Release blocker rollup report: $(Get-RepoRelativePath -RepoRoot $repoRoot -Path $summary.release_blocker_rollup.report_markdown)
- Release blocker rollup counts: $($summary.release_blocker_rollup.release_blocker_count) blockers, $($summary.release_blocker_rollup.action_item_count) actions, $($summary.release_blocker_rollup.warning_count) warnings
- Release governance handoff summary: $releaseGovernanceHandoffSummaryDisplay
- Release governance handoff report: $releaseGovernanceHandoffReportDisplay
- Release governance handoff counts: $($summary.release_governance_handoff.loaded_report_count)/$($summary.release_governance_handoff.expected_report_count) reports, $($summary.release_governance_handoff.missing_report_count) missing, $($summary.release_governance_handoff.release_blocker_count) blockers, $($summary.release_governance_handoff.action_item_count) actions
- Release handoff: $releaseHandoffDisplayPath
- Release body: $releaseBodyDisplayPath
- Release summary: $releaseSummaryDisplayPath
- Artifact guide: $artifactGuideDisplayPath
- Reviewer checklist: $reviewerChecklistDisplayPath
- Start here: $startHereDisplayPath
- PDF visual gate summary: $pdfVisualGateSummaryDisplayPath
- PDF visual gate contact sheet: $pdfVisualGateContactSheetDisplayPath
- PDF visual gate attempt summary: $pdfVisualGateAttemptSummaryDisplayPath
- PDF visual gate attempt contact sheet: $pdfVisualGateAttemptContactSheetDisplayPath
- PDF visual segmented gate summary: $pdfVisualSegmentedGateSummaryDisplayPath
- PDF visual segmented gate contact sheet: $pdfVisualSegmentedGateContactSheetDisplayPath
- PDF bounded CTest summaries: $pdfBoundedCtestSummaryDisplay
- PDF release readiness summary: $pdfFullCtestReadinessSummaryDisplayPath
- PDF full CTest summary: $pdfFullCtestSummaryDisplayPath
"@
    $finalReview | Set-Content -Path $finalReviewPath -Encoding UTF8

    try {
        Invoke-ChildPowerShell -ScriptPath $releaseNoteBundleScript `
            -Arguments @(
                "-SummaryJson"
                $summaryPath
                "-ReleaseVersion"
                $projectVersion
                "-HandoffOutputPath"
                $releaseHandoffPath
                "-BodyOutputPath"
                $releaseBodyZhCnPath
                "-ShortOutputPath"
                $releaseSummaryZhCnPath
                "-GuideOutputPath"
                $artifactGuidePath
                "-ChecklistOutputPath"
                $reviewerChecklistPath
                "-StartHereOutputPath"
                $startHerePath
            ) `
            -FailureMessage "Failed to write release note bundle."
    } catch {
        Write-Step $_.Exception.Message
    }
}

Write-Step "Completed release-candidate checks"
Write-Host "Summary: $summaryPath"
Write-Host "Final review: $finalReviewPath"
Write-Host "Release handoff: $releaseHandoffPath"
Write-Host "Release body output: $releaseBodyZhCnPath"
Write-Host "Release summary output: $releaseSummaryZhCnPath"
Write-Host "Artifact guide: $artifactGuidePath"
Write-Host "Reviewer checklist: $reviewerChecklistPath"
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfVisualGateSummaryJson)) {
    Write-Host "PDF visual gate summary: $resolvedPdfVisualGateSummaryJson"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfVisualGateAttemptSummaryJson)) {
    Write-Host "PDF visual gate attempt summary: $resolvedPdfVisualGateAttemptSummaryJson"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfVisualSegmentedGateSummaryJson)) {
    Write-Host "PDF visual segmented gate summary: $resolvedPdfVisualSegmentedGateSummaryJson"
}
if (@($resolvedPdfBoundedCtestSummaryJson).Count -gt 0) {
    Write-Host "PDF bounded CTest summaries: $($resolvedPdfBoundedCtestSummaryJson -join ', ')"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfReleaseReadinessSummaryJson)) {
    Write-Host "PDF release readiness summary: $resolvedPdfReleaseReadinessSummaryJson"
}
if ($projectTemplateSmokeRequested) {
    Write-Host "Project template schema approval history JSON: $schemaApprovalHistoryJsonPath"
    Write-Host "Project template schema approval history Markdown: $schemaApprovalHistoryMarkdownPath"
}
if ($releaseBlockerRollupRequested) {
    Write-Host "Release blocker rollup summary: $releaseBlockerRollupSummaryPath"
    Write-Host "Release blocker rollup report: $releaseBlockerRollupMarkdownPath"
}
if ($releaseGovernanceHandoffRequested) {
    Write-Host "Release governance handoff summary: $releaseGovernanceHandoffSummaryPath"
    Write-Host "Release governance handoff report: $releaseGovernanceHandoffMarkdownPath"
}
Write-Host "Start here: $startHerePath"

if ($null -ne $releaseBlockerRollupFailure) {
    throw $releaseBlockerRollupFailure
}
if ($null -ne $releaseGovernanceHandoffFailure) {
    throw $releaseGovernanceHandoffFailure
}

$global:LASTEXITCODE = 0
