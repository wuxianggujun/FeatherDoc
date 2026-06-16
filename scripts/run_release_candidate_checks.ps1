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
    [switch]$SkipMaterialSafetyAudit,
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

. (Join-Path $PSScriptRoot "run_release_candidate_checks_core.ps1")
. (Join-Path $PSScriptRoot "run_release_candidate_checks_pdf_summaries.ps1")
. (Join-Path $PSScriptRoot "run_release_candidate_checks_visual_review.ps1")
. (Join-Path $PSScriptRoot "run_release_candidate_checks_schema_approval.ps1")
. (Join-Path $PSScriptRoot "run_release_candidate_checks_release_governance.ps1")
. (Join-Path $PSScriptRoot "run_release_candidate_checks_output_parsers.ps1")

. (Join-Path $PSScriptRoot "run_release_candidate_checks_initialize.ps1")

try {
    . (Join-Path $PSScriptRoot "run_release_candidate_checks_validate_and_build.ps1")
    . (Join-Path $PSScriptRoot "run_release_candidate_checks_template_smoke.ps1")
    . (Join-Path $PSScriptRoot "run_release_candidate_checks_install_visual.ps1")
} catch {
    $summary.execution_status = "fail"
    $summary.failed_step = $activeStep
    $summary.error = $_.Exception.Message
    if ($activeStep -and $summary.steps[$activeStep].status -eq "pending") {
        $summary.steps[$activeStep].status = "failed"
    }
    throw
} finally {
    . (Join-Path $PSScriptRoot "run_release_candidate_checks_finalize_governance.ps1")
    . (Join-Path $PSScriptRoot "run_release_candidate_checks_finalize_materials.ps1")
}

. (Join-Path $PSScriptRoot "run_release_candidate_checks_final_output.ps1")
