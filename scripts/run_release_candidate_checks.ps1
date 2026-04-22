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
    [string]$TemplateSchemaInputDocx = "",
    [string]$TemplateSchemaBaseline = "",
    [string]$TemplateSchemaGeneratedOutput = "",
    [switch]$TemplateSchemaSectionTargets,
    [switch]$TemplateSchemaResolvedSectionTargets,
    [string]$TemplateSchemaManifestPath = "",
    [string]$TemplateSchemaManifestOutputDir = "",
    [string]$ProjectTemplateSmokeManifestPath = "",
    [string]$ProjectTemplateSmokeOutputDir = "",
    [switch]$SkipReviewTasks,
    [ValidateSet("review-only", "review-and-repair")]
    [string]$ReviewMode = "review-only",
    [switch]$RefreshReadmeAssets,
    [string]$ReadmeAssetsDir = "docs/assets/readme",
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

    if ($MsvcBootstrap.mode -eq "current_env") {
        & cmd.exe /c $CommandText
    } else {
        & cmd.exe /c "call `"$($MsvcBootstrap.vcvars_path)`" && $CommandText"
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

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
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

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
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

function Parse-TemplateSchemaCheckOutput {
    param([string[]]$Lines)

    $jsonLine = $Lines |
        Where-Object { $_ -match '^\{"command":"check-template-schema",' } |
        Select-Object -Last 1
    if ([string]::IsNullOrWhiteSpace($jsonLine)) {
        throw "Template schema check did not emit a JSON result line."
    }

    return $jsonLine | ConvertFrom-Json
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
$msvcBootstrap = Get-MsvcBootstrap

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
$startHerePath = Join-Path $resolvedSummaryOutputDir "START_HERE.md"
$installSmokeScript = Join-Path $repoRoot "scripts\run_install_find_package_smoke.ps1"
$templateSchemaCheckScript = Join-Path $repoRoot "scripts\check_template_schema_baseline.ps1"
$templateSchemaManifestScript = Join-Path $repoRoot "scripts\check_template_schema_manifest.ps1"
$projectTemplateSmokeScript = Join-Path $repoRoot "scripts\run_project_template_smoke.ps1"
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
$templateSchemaRequested = -not [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaInputDocx) -or
    -not [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaBaseline)

New-Item -ItemType Directory -Path $resolvedSummaryOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

$summary = [ordered]@{
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
    release_handoff = $releaseHandoffPath
    release_body_zh_cn = $releaseBodyZhCnPath
    release_summary_zh_cn = $releaseSummaryZhCnPath
    artifact_guide = $artifactGuidePath
    reviewer_checklist = $reviewerChecklistPath
    start_here = $startHerePath
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
        project_template_smoke = [ordered]@{ status = if ($projectTemplateSmokeRequested) { "pending" } else { "not_requested" } }
        install_smoke = [ordered]@{ status = if ($SkipInstallSmoke) { "skipped" } else { "pending" } }
        visual_gate = [ordered]@{ status = if ($SkipVisualGate) { "skipped" } else { "pending" } }
    }
}

$activeStep = ""

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
        Assert-PathExists -Path $resolvedBuildDir -Label "existing build directory"
    }

    if (-not $SkipBuild) {
        $activeStep = "build"
        Write-Step "Building configured targets"
        Invoke-MsvcCommand -MsvcBootstrap $msvcBootstrap -CommandText (
            "cmake --build `"$resolvedBuildDir`""
        )
        $summary.steps.build.status = "completed"
    } else {
        Assert-PathExists -Path $resolvedBuildDir -Label "existing build directory"
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

        $templateSchemaInfo = Parse-TemplateSchemaCheckOutput -Lines @(
            $templateSchemaOutput | ForEach-Object { $_.ToString() }
        )
        $summary.steps.template_schema.status = if ($templateSchemaExitCode -eq 0) {
            "completed"
        } else {
            "failed"
        }
        $summary.steps.template_schema.matches = [bool]$templateSchemaInfo.matches
        $summary.steps.template_schema.schema_file = [string]$templateSchemaInfo.schema_file
        if (-not [string]::IsNullOrWhiteSpace([string]$templateSchemaInfo.generated_output_path)) {
            $summary.steps.template_schema.generated_output_path = [string]$templateSchemaInfo.generated_output_path
        }
        $summary.steps.template_schema.added_target_count = [int]$templateSchemaInfo.added_target_count
        $summary.steps.template_schema.removed_target_count = [int]$templateSchemaInfo.removed_target_count
        $summary.steps.template_schema.changed_target_count = [int]$templateSchemaInfo.changed_target_count
        $summary.template_schema.matches = [bool]$templateSchemaInfo.matches
        $summary.template_schema.added_target_count = [int]$templateSchemaInfo.added_target_count
        $summary.template_schema.removed_target_count = [int]$templateSchemaInfo.removed_target_count
        $summary.template_schema.changed_target_count = [int]$templateSchemaInfo.changed_target_count

        if ($templateSchemaExitCode -ne 0) {
            throw "Template schema baseline drift detected."
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

        $summary.template_schema_manifest.summary_json = $resolvedTemplateSchemaManifestSummaryPath
        $summary.template_schema_manifest.manifest_path = [string]$templateSchemaManifestInfo.manifest_path
        $summary.template_schema_manifest.output_dir = [string]$resolvedTemplateSchemaManifestOutputDir
        $summary.template_schema_manifest.passed = [bool]$templateSchemaManifestInfo.passed
        $summary.template_schema_manifest.entry_count = [int]$templateSchemaManifestInfo.entry_count
        $summary.template_schema_manifest.drift_count = [int]$templateSchemaManifestInfo.drift_count

        if ($templateSchemaManifestExitCode -ne 0) {
            throw "Template schema manifest drift detected."
        }
    }

    if ($projectTemplateSmokeRequested) {
        $activeStep = "project_template_smoke"
        Write-Step "Running project template smoke"
        Assert-PathExists -Path $resolvedProjectTemplateSmokeManifestPath -Label "project template smoke manifest"
        New-Item -ItemType Directory -Path $resolvedProjectTemplateSmokeOutputDir -Force | Out-Null

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
        $summary.steps.project_template_smoke.visual_entry_count = [int]$projectTemplateSmokeInfo.visual_entry_count
        $summary.steps.project_template_smoke.visual_verdict = [string]$projectTemplateSmokeInfo.visual_verdict
        $summary.steps.project_template_smoke.manual_review_pending_count = [int]$projectTemplateSmokeInfo.manual_review_pending_count
        $summary.steps.project_template_smoke.visual_review_undetermined_count = [int]$projectTemplateSmokeInfo.visual_review_undetermined_count
        $summary.steps.project_template_smoke.overall_status = [string]$projectTemplateSmokeInfo.overall_status

        $summary.project_template_smoke.summary_json = $resolvedProjectTemplateSmokeSummaryPath
        $summary.project_template_smoke.manifest_path = [string]$projectTemplateSmokeInfo.manifest_path
        $summary.project_template_smoke.output_dir = [string]$resolvedProjectTemplateSmokeOutputDir
        $summary.project_template_smoke.passed = [bool]$projectTemplateSmokeInfo.passed
        $summary.project_template_smoke.entry_count = [int]$projectTemplateSmokeInfo.entry_count
        $summary.project_template_smoke.failed_entry_count = [int]$projectTemplateSmokeInfo.failed_entry_count
        $summary.project_template_smoke.visual_entry_count = [int]$projectTemplateSmokeInfo.visual_entry_count
        $summary.project_template_smoke.visual_verdict = [string]$projectTemplateSmokeInfo.visual_verdict
        $summary.project_template_smoke.manual_review_pending_count = [int]$projectTemplateSmokeInfo.manual_review_pending_count
        $summary.project_template_smoke.visual_review_undetermined_count = [int]$projectTemplateSmokeInfo.visual_review_undetermined_count
        $summary.project_template_smoke.overall_status = [string]$projectTemplateSmokeInfo.overall_status

        if ($projectTemplateSmokeExitCode -ne 0) {
            throw "Project template smoke reported failing entries."
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
        $visualGateArgs = @(
            "-GateOutputDir"
            $resolvedGateOutputDir
            "-SmokeBuildDir"
            $resolvedBuildDir
            "-FixedGridBuildDir"
            $resolvedBuildDir
            "-SectionPageSetupBuildDir"
            $resolvedBuildDir
            "-PageNumberFieldsBuildDir"
            $resolvedBuildDir
            "-TaskOutputRoot"
            $resolvedTaskOutputRoot
            "-ReviewMode"
            $ReviewMode
            "-Dpi"
            $Dpi.ToString()
            "-SkipBuild"
        )
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
    $releaseHandoffDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseHandoffPath
    $releaseBodyDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseBodyZhCnPath
    $releaseSummaryDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $releaseSummaryZhCnPath
    $artifactGuideDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $artifactGuidePath
    $reviewerChecklistDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $reviewerChecklistPath
    $startHereDisplayPath = Get-RepoRelativePath -RepoRoot $repoRoot -Path $startHerePath

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

    $finalReview = @"
# Release Candidate Checks

- Generated at: $(Get-Date -Format s)
- Workspace: $repoRootDisplay
- Summary JSON: $summaryDisplayPath
- Execution status: $($summary.execution_status)
- Failed step: $($summary.failed_step)
- Error: $($summary.error)
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
$readmeGalleryStatusLine

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
- Release handoff: $releaseHandoffDisplayPath
- Release body: $releaseBodyDisplayPath
- Release summary: $releaseSummaryDisplayPath
- Artifact guide: $artifactGuideDisplayPath
- Reviewer checklist: $reviewerChecklistDisplayPath
- Start here: $startHereDisplayPath
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
Write-Host "Start here: $startHerePath"
