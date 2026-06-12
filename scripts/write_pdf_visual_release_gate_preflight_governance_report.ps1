<#
.SYNOPSIS
Writes a release-governance report for the PDF visual release gate preflight.

.DESCRIPTION
Turns the lightweight PDF visual release gate preflight summary into a
release_blockers/action_items report that can be consumed by the existing
release blocker rollup. The script is read-only for source and build outputs:
it does not build, render PDFs, install dependencies, or create virtual
environments.
#>
param(
    [string]$PreflightJson = "",
    [string]$BuildDir = ".bpdf-roundtrip-msvc",
    [string]$PdfioSourceDir = "",
    [string]$PdfiumProvider = "",
    [string]$PdfiumSourceDir = "",
    [string]$PdfiumPrebuiltRoot = "",
    [string]$PdfiumLibrary = "",
    [string]$PdfiumIncludeDir = "",
    [string]$PdfiumRuntimeDll = "",
    [string]$PdfiumRuntimeDir = "",
    [string]$ControlledVisualSmokeJson = "",
    [string]$OutputDir = "output/pdf-visual-release-gate-preflight-governance",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [string]$CTestExecutable = "ctest",
    [int]$MinFreeMemoryMB = 2048,
    [switch]$SkipMemoryGuard,
    [switch]$FailOnBlocker
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$reportSchema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
. (Join-Path $PSScriptRoot "write_pdf_visual_release_gate_preflight_governance_report_helpers.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "pdf_visual_release_gate_preflight_governance.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$preflightSummaryPath = if ([string]::IsNullOrWhiteSpace($PreflightJson)) {
    Join-Path $resolvedOutputDir "preflight-summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $PreflightJson -AllowMissing
}

$preflightSummary = $null
$loadFailureMessage = ""
if ([string]::IsNullOrWhiteSpace($PreflightJson)) {
    $preflightScriptPath = Join-Path $repoRoot "scripts\check_pdf_visual_release_gate_preflight.ps1"
    if (-not (Test-Path -LiteralPath $preflightScriptPath -PathType Leaf)) {
        $loadFailureMessage = "PDF visual release gate preflight script was not found: $preflightScriptPath"
    } else {
        Write-Step "Running lightweight preflight"
        $preflightArguments = @{
            BuildDir = $BuildDir
            OutputJson = $preflightSummaryPath
            CTestExecutable = $CTestExecutable
            MinFreeMemoryMB = $MinFreeMemoryMB
            SkipMemoryGuard = [bool]$SkipMemoryGuard
        }
        $preflightOverrideCommandArguments = New-Object 'System.Collections.Generic.List[string]'
        foreach ($entry in @(
            @{ Name = "PdfioSourceDir"; Value = $PdfioSourceDir },
            @{ Name = "PdfiumProvider"; Value = $PdfiumProvider },
            @{ Name = "PdfiumSourceDir"; Value = $PdfiumSourceDir },
            @{ Name = "PdfiumPrebuiltRoot"; Value = $PdfiumPrebuiltRoot },
            @{ Name = "PdfiumLibrary"; Value = $PdfiumLibrary },
            @{ Name = "PdfiumIncludeDir"; Value = $PdfiumIncludeDir },
            @{ Name = "PdfiumRuntimeDll"; Value = $PdfiumRuntimeDll },
            @{ Name = "PdfiumRuntimeDir"; Value = $PdfiumRuntimeDir }
        )) {
            Add-OptionalPreflightOverride `
                -Arguments $preflightArguments `
                -CommandArguments $preflightOverrideCommandArguments `
                -Name ([string]$entry.Name) `
                -Value ([string]$entry.Value)
        }

        & $preflightScriptPath @preflightArguments | Out-Null
    }
}

if ([string]::IsNullOrWhiteSpace($loadFailureMessage)) {
    try {
        $preflightSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $preflightSummaryPath | ConvertFrom-Json
    } catch {
        $loadFailureMessage = $_.Exception.Message
    }
}
if (-not [string]::IsNullOrWhiteSpace($loadFailureMessage)) {
    $preflightSummary = [pscustomobject]@{
        status = "unavailable"
        checks = @()
        blocking_checks = @("preflight_summary_unavailable")
    }
}

$preflightStatus = Get-JsonString -Object $preflightSummary -Name "status" -DefaultValue "unknown"
$checks = @(Get-JsonArray -Object $preflightSummary -Name "checks")
$blockingChecks = @(Get-BlockingCheckNames -PreflightSummary $preflightSummary)
if (-not [string]::IsNullOrWhiteSpace($loadFailureMessage) -and $blockingChecks.Count -eq 0) {
    $blockingChecks = @("preflight_summary_unavailable")
}

$preflightDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $preflightSummaryPath
$summaryDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
$memoryGuardCommandArgs = if ($SkipMemoryGuard) {
    "-SkipMemoryGuard"
} else {
    "-MinFreeMemoryMB $MinFreeMemoryMB"
}
$preflightOverrideCommandArguments = New-Object 'System.Collections.Generic.List[string]'
$unusedPreflightArguments = @{}
foreach ($entry in @(
    @{ Name = "PdfioSourceDir"; Value = $PdfioSourceDir },
    @{ Name = "PdfiumProvider"; Value = $PdfiumProvider },
    @{ Name = "PdfiumSourceDir"; Value = $PdfiumSourceDir },
    @{ Name = "PdfiumPrebuiltRoot"; Value = $PdfiumPrebuiltRoot },
    @{ Name = "PdfiumLibrary"; Value = $PdfiumLibrary },
    @{ Name = "PdfiumIncludeDir"; Value = $PdfiumIncludeDir },
    @{ Name = "PdfiumRuntimeDll"; Value = $PdfiumRuntimeDll },
    @{ Name = "PdfiumRuntimeDir"; Value = $PdfiumRuntimeDir }
)) {
    Add-OptionalPreflightOverride `
        -Arguments $unusedPreflightArguments `
        -CommandArguments $preflightOverrideCommandArguments `
        -Name ([string]$entry.Name) `
        -Value ([string]$entry.Value)
}
$preflightOverrideCommandArgs = ($preflightOverrideCommandArguments.ToArray() -join " ")
$commandTemplate = "powershell -ExecutionPolicy Bypass -File .\scripts\run_pdf_visual_release_gate.ps1 -BuildDir $BuildDir -PreflightOnly $memoryGuardCommandArgs"
if (-not [string]::IsNullOrWhiteSpace($preflightOverrideCommandArgs)) {
    $commandTemplate += " $preflightOverrideCommandArgs"
}
$summaryBuildDir = Get-JsonString -Object $preflightSummary -Name "build_dir" -DefaultValue (Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir -AllowMissing)
$summaryRequestedBuildDir = Get-JsonString -Object $preflightSummary -Name "requested_build_dir" -DefaultValue (Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir -AllowMissing)
$summaryBuildDirSource = Get-JsonString -Object $preflightSummary -Name "build_dir_source" -DefaultValue "requested"
$summaryBuildDirAutoCandidates = @(Get-JsonArray -Object $preflightSummary -Name "build_dir_auto_candidates")
$summaryEvidenceKindDetails = Get-PreflightEvidenceKindSummary -PreflightSummary $preflightSummary -Checks $checks
$summaryEvidenceKind = Get-JsonString -Object $summaryEvidenceKindDetails -Name "evidence_kind" -DefaultValue "unknown"
$syntheticEvidenceMarkerCount = Get-JsonInt -Object $summaryEvidenceKindDetails -Name "synthetic_marker_count"
$syntheticEvidenceMarkers = @(
    Get-JsonArray -Object $summaryEvidenceKindDetails -Name "synthetic_markers" |
        ForEach-Object { [string]$_ } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
)
$syntheticEvidenceIsBlocking = $summaryEvidenceKind -eq "synthetic_fixture"
$buildDirSnapshot = Get-BuildDirectorySnapshotFromChecks -Checks $checks
$outputGapSummary = @(Get-PreflightOutputGapSummary `
        -Checks $checks `
        -SourceReport $summaryPath `
        -SourceReportDisplay $summaryDisplay `
        -SourceJson $preflightSummaryPath `
        -SourceJsonDisplay $preflightDisplay)
$missingOutputCount = Get-PreflightMissingOutputCount -OutputGapSummary $outputGapSummary
$preflightBlockingSummary = Get-PreflightBlockingSummary `
    -PreflightSummary $preflightSummary `
    -Checks $checks `
    -BlockingChecks $blockingChecks `
    -BuildDirSnapshot $buildDirSnapshot
$pdfDependencyInputs = Get-JsonProperty -Object $preflightSummary -Name "pdf_dependency_inputs"
$pdfDependencyInputsStatus = Get-JsonString -Object $pdfDependencyInputs -Name "status" -DefaultValue (Get-JsonValueText -Object $preflightBlockingSummary -Name "pdf_dependency_inputs_status")
$pdfDependencyMissingInputCount = Get-JsonInt -Object $pdfDependencyInputs -Name "missing_input_count" -DefaultValue (Get-JsonInt -Object $preflightBlockingSummary -Name "pdf_dependency_missing_input_count")
$selectedPdfiumProvider = Get-JsonString -Object $pdfDependencyInputs -Name "selected_pdfium_provider" -DefaultValue (Get-JsonValueText -Object $preflightBlockingSummary -Name "selected_pdfium_provider")
$pdfioDependencyReady = Get-JsonBool -Object $pdfDependencyInputs -Name "pdfio_ready" -DefaultValue (Get-JsonBool -Object $preflightBlockingSummary -Name "pdfio_dependency_ready")
$pdfiumDependencyReady = Get-JsonBool -Object $pdfDependencyInputs -Name "pdfium_ready" -DefaultValue (Get-JsonBool -Object $preflightBlockingSummary -Name "pdfium_dependency_ready")
$pdfDependencyMissingInputsPreview = @(
    Get-JsonArray -Object $pdfDependencyInputs -Name "missing_inputs" |
        Select-Object -First 5 |
        ForEach-Object { [string]$_ }
)
if ($pdfDependencyMissingInputsPreview.Count -eq 0) {
    $pdfDependencyMissingInputsPreview = @(
        Get-JsonArray -Object $preflightBlockingSummary -Name "pdf_dependency_missing_inputs_preview" |
            Select-Object -First 5 |
            ForEach-Object { [string]$_ }
    )
}
$pdfDependencyInputsAreBlocking = ($pdfDependencyInputsStatus -ne "ready" -or $pdfDependencyMissingInputCount -gt 0)
if ($pdfDependencyInputsAreBlocking -and
    @($blockingChecks | Where-Object { [string]$_ -eq "pdf_dependency_inputs_ready" }).Count -eq 0) {
    $blockingChecks = @($blockingChecks + "pdf_dependency_inputs_ready" | Sort-Object -Unique)
}
if ($syntheticEvidenceIsBlocking -and
    @($blockingChecks | Where-Object { [string]$_ -eq "synthetic_preflight_evidence" }).Count -eq 0) {
    $blockingChecks = @($blockingChecks + "synthetic_preflight_evidence" | Sort-Object -Unique)
}
if ($pdfDependencyInputsAreBlocking) {
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "blocking_check_count" -Value $blockingChecks.Count
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "pdf_dependency_inputs_status" -Value $pdfDependencyInputsStatus
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "pdf_dependency_missing_input_count" -Value $pdfDependencyMissingInputCount
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "selected_pdfium_provider" -Value $selectedPdfiumProvider
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "pdfio_dependency_ready" -Value $pdfioDependencyReady
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "pdfium_dependency_ready" -Value $pdfiumDependencyReady
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "pdf_dependency_missing_inputs_preview" -Value @($pdfDependencyMissingInputsPreview)
}
if ($syntheticEvidenceIsBlocking) {
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "blocking_check_count" -Value $blockingChecks.Count
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "evidence_kind" -Value $summaryEvidenceKind
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "synthetic_evidence_marker_count" -Value $syntheticEvidenceMarkerCount
    Set-JsonPropertyValue -Object $preflightBlockingSummary -Name "synthetic_evidence_markers_preview" -Value @($syntheticEvidenceMarkers | Select-Object -First 5)
}
$readinessActionEvidence = @(Get-PreflightReadinessActionEvidence `
        -PdfDependencyInputs $pdfDependencyInputs `
        -PreflightBlockingSummary $preflightBlockingSummary `
        -SourceReport $summaryPath `
        -SourceReportDisplay $summaryDisplay `
        -SourceJson $preflightSummaryPath `
        -SourceJsonDisplay $preflightDisplay `
        -CommandTemplate $commandTemplate)
$memoryGuardBlocked = Get-JsonBool -Object $preflightBlockingSummary -Name "memory_guard_blocked"
$memoryGuardSkipped = Get-JsonBool -Object $preflightBlockingSummary -Name "memory_guard_skipped"
$freeMemoryMb = Get-JsonValueText -Object $preflightBlockingSummary -Name "free_memory_mb"
$minFreeMemoryMb = Get-JsonValueText -Object $preflightBlockingSummary -Name "min_free_memory_mb"
$controlledVisualSmoke = Get-ControlledVisualSmokeEvidence -RepoRoot $repoRoot -Path $ControlledVisualSmokeJson

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

if (-not [string]::IsNullOrWhiteSpace($ControlledVisualSmokeJson) -and
    (-not (Get-JsonBool -Object $controlledVisualSmoke -Name "available") -or
        -not (Get-JsonBool -Object $controlledVisualSmoke -Name "passed"))) {
    $warnings.Add([ordered]@{
        id = "pdf_controlled_visual_smoke.unavailable_or_failed"
        source = "pdf_controlled_visual_smoke"
        severity = "warning"
        status = Get-JsonString -Object $controlledVisualSmoke -Name "status"
        action = "rerun_pdf_controlled_visual_smoke_check"
        message = "Controlled PDF visual smoke evidence was provided but is not passing."
        source_schema = $reportSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = Get-JsonString -Object $controlledVisualSmoke -Name "source_json"
        source_json_display = Get-JsonString -Object $controlledVisualSmoke -Name "source_json_display"
        error_message = Get-JsonString -Object $controlledVisualSmoke -Name "error_message"
    }) | Out-Null
}

if (-not [string]::IsNullOrWhiteSpace($loadFailureMessage)) {
    $message = "PDF visual release gate preflight summary could not be read: $loadFailureMessage"
    $releaseBlockers.Add([ordered]@{
        id = "pdf_visual_release_gate_preflight.summary_unavailable"
        source = "pdf_visual_release_gate_preflight"
        severity = "error"
        status = "blocked"
        action = "rerun_pdf_visual_release_gate_preflight"
        message = $message
        source_schema = $reportSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = $preflightSummaryPath
        source_json_display = $preflightDisplay
        repair_strategy = "rerun_preflight"
        repair_hint = "Regenerate the PDF visual release gate preflight summary, then rebuild this governance report."
        command_template = "powershell -ExecutionPolicy Bypass -File .\scripts\check_pdf_visual_release_gate_preflight.ps1 -BuildDir $BuildDir -OutputJson $preflightSummaryPath $memoryGuardCommandArgs"
        blocked_item_count = 1
        issue_keys = @("preflight_summary_unavailable")
    }) | Out-Null
} elseif ($preflightStatus -ne "ready" -or $blockingChecks.Count -gt 0) {
    $blockedText = if ($blockingChecks.Count -gt 0) {
        $blockingChecks -join ", "
    } else {
        "preflight_status=$preflightStatus"
    }
    $memoryGuardIsBlocking = @($blockingChecks | Where-Object { [string]$_ -eq "workstation_free_memory_available" }).Count -gt 0
    $pdfBuildOptionsAreBlocking = @($blockingChecks | Where-Object { [string]$_ -eq "pdf_build_options_enabled" }).Count -gt 0
    $buildOutputRepairHint = "Prepare a reusable PDF build directory with CTest registration, CLI baseline PDFs, manifest PDF outputs, helper scripts, and render Python before running the full PDF visual release gate."
    $repairHint = $buildOutputRepairHint
    if ($memoryGuardIsBlocking) {
        $repairHint = "Free physical memory is below the PDF visual release gate threshold. Close unrelated applications or wait for external render/test processes to finish, then rerun the preflight; use -SkipMemoryGuard only after a manual resource check."
        if (-not [string]::IsNullOrWhiteSpace($freeMemoryMb) -or
            -not [string]::IsNullOrWhiteSpace($minFreeMemoryMb)) {
            $repairHint += " Current memory snapshot: $freeMemoryMb MB free, $minFreeMemoryMb MB required."
        }
        $nonMemoryBlockingChecks = @($blockingChecks | Where-Object { [string]$_ -ne "workstation_free_memory_available" })
        if ($nonMemoryBlockingChecks.Count -gt 0) {
            $repairHint += " Also $buildOutputRepairHint"
        }
    }
    if ($pdfBuildOptionsAreBlocking) {
        $pdfOptionsCheck = Get-PreflightCheck -Checks $checks -Name "pdf_build_options_enabled"
        $pdfOptionsDetails = Get-JsonProperty -Object $pdfOptionsCheck -Name "details"
        $disabledPdfBuildOptions = @(
            Get-JsonArray -Object $pdfOptionsDetails -Name "disabled_options" |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        $missingPdfBuildOptions = @(
            Get-JsonArray -Object $pdfOptionsDetails -Name "missing_options" |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
        $optionHints = @()
        if ($disabledPdfBuildOptions.Count -gt 0) {
            $optionHints += "disabled=$($disabledPdfBuildOptions -join ', ')"
        }
        if ($missingPdfBuildOptions.Count -gt 0) {
            $optionHints += "missing=$($missingPdfBuildOptions -join ', ')"
        }
        if ($optionHints.Count -gt 0) {
            $repairHint += " Current CMakeCache.txt PDF options are not release-gate ready: $($optionHints -join '; '). Reconfigure with -DFEATHERDOC_BUILD_PDF=ON and -DFEATHERDOC_BUILD_PDF_IMPORT=ON, including the required PDFio/PDFium inputs, before generating visual gate outputs."
        }
    }
    if ($pdfDependencyInputsAreBlocking) {
        $dependencyHints = @()
        if (-not [string]::IsNullOrWhiteSpace($pdfDependencyInputsStatus)) {
            $dependencyHints += "status=$pdfDependencyInputsStatus"
        }
        if (-not [string]::IsNullOrWhiteSpace($selectedPdfiumProvider)) {
            $dependencyHints += "selected_pdfium_provider=$selectedPdfiumProvider"
        }
        $dependencyHints += "missing_input_count=$pdfDependencyMissingInputCount"
        if ($pdfDependencyMissingInputsPreview.Count -gt 0) {
            $dependencyHints += "missing_inputs_preview=$($pdfDependencyMissingInputsPreview -join '; ')"
        }
        $repairHint += " PDF dependency inputs are also not release-gate ready: $($dependencyHints -join '; ')."
    }
    if ($syntheticEvidenceIsBlocking) {
        $syntheticHints = @("evidence_kind=synthetic_fixture")
        if ($syntheticEvidenceMarkerCount -gt 0) {
            $syntheticHints += "marker_count=$syntheticEvidenceMarkerCount"
        }
        $syntheticPreview = @($syntheticEvidenceMarkers | Select-Object -First 3)
        if ($syntheticPreview.Count -gt 0) {
            $syntheticHints += "marker_preview=$($syntheticPreview -join '; ')"
        }
        $repairHint += " The preflight evidence is synthetic test-fixture evidence and cannot release the real PDF visual gate: $($syntheticHints -join '; '). Rerun preflight against a real reusable PDF build before the full visual gate."
    }
    if ($buildDirSnapshot.Count -gt 0) {
        $snapshotHints = @()
        if (-not [bool]$buildDirSnapshot["cmake_cache_exists"]) {
            $snapshotHints += "CMakeCache.txt missing"
        }
        if (-not [bool]$buildDirSnapshot["ctest_manifest_exists"]) {
            $snapshotHints += "CTestTestfile.cmake missing"
        }
        if ($snapshotHints.Count -gt 0) {
            $repairHint += " Current build directory snapshot: $($snapshotHints -join '; ')."
        }
    }
    $message = "PDF visual release gate preflight is not ready; blocking checks: $blockedText."
    $releaseBlockers.Add([ordered]@{
        id = "pdf_visual_release_gate_preflight.build_outputs_missing"
        source = "pdf_visual_release_gate_preflight"
        severity = "error"
        status = "blocked"
        action = "prepare_pdf_visual_release_gate_build_outputs"
        message = $message
        source_schema = $reportSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = $preflightSummaryPath
        source_json_display = $preflightDisplay
        repair_strategy = "reuse_or_prepare_pdf_visual_release_gate_build_outputs"
        repair_hint = $repairHint
        command_template = $commandTemplate
        blocked_item_count = $blockingChecks.Count
        issue_keys = @($blockingChecks)
        blocking_summary = $preflightBlockingSummary
        pdf_dependency_inputs = $pdfDependencyInputs
        evidence_kind = $summaryEvidenceKind
        evidence_kind_details = $summaryEvidenceKindDetails
        synthetic_evidence_marker_count = $syntheticEvidenceMarkerCount
        synthetic_evidence_markers = @($syntheticEvidenceMarkers)
        build_dir_auto_candidates = @($summaryBuildDirAutoCandidates)
        output_gap_count = $outputGapSummary.Count
        missing_output_count = $missingOutputCount
        output_gap_summary = @($outputGapSummary)
        readiness_action_evidence_count = $readinessActionEvidence.Count
        readiness_action_evidence = @($readinessActionEvidence)
    }) | Out-Null
    $actionItems.Add([ordered]@{
        id = "prepare_pdf_visual_release_gate_build_outputs"
        action = "prepare_pdf_visual_release_gate_build_outputs"
        title = "Prepare reusable PDF visual release gate build outputs"
        open_command = $commandTemplate
        command_template = $commandTemplate
        source_schema = $reportSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = $preflightSummaryPath
        source_json_display = $preflightDisplay
        repair_strategy = "reuse_or_prepare_pdf_visual_release_gate_build_outputs"
        repair_hint = $repairHint
        blocked_item_count = $blockingChecks.Count
        issue_keys = @($blockingChecks)
        blocking_summary = $preflightBlockingSummary
        pdf_dependency_inputs = $pdfDependencyInputs
        evidence_kind = $summaryEvidenceKind
        evidence_kind_details = $summaryEvidenceKindDetails
        synthetic_evidence_marker_count = $syntheticEvidenceMarkerCount
        synthetic_evidence_markers = @($syntheticEvidenceMarkers)
        build_dir_auto_candidates = @($summaryBuildDirAutoCandidates)
        output_gap_count = $outputGapSummary.Count
        missing_output_count = $missingOutputCount
        output_gap_summary = @($outputGapSummary)
        readiness_action_evidence_count = $readinessActionEvidence.Count
        readiness_action_evidence = @($readinessActionEvidence)
    }) | Out-Null
}

$sourceFailureCount = if ([string]::IsNullOrWhiteSpace($loadFailureMessage)) { 0 } else { 1 }
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0) {
    "blocked"
} else {
    "ready"
}

$summary = [ordered]@{
    schema = $reportSchema
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    preflight_ready = ($status -eq "ready")
    full_visual_gate_required = $true
    full_visual_gate_status = "not_run_by_preflight_governance"
    controlled_visual_smoke_available = Get-JsonBool -Object $controlledVisualSmoke -Name "available"
    controlled_visual_smoke_status = Get-JsonString -Object $controlledVisualSmoke -Name "status"
    controlled_visual_smoke_passed = Get-JsonBool -Object $controlledVisualSmoke -Name "passed"
    controlled_visual_smoke_case_count = Get-JsonInt -Object $controlledVisualSmoke -Name "case_count"
    controlled_visual_smoke_json = Get-JsonString -Object $controlledVisualSmoke -Name "source_json"
    controlled_visual_smoke_json_display = Get-JsonString -Object $controlledVisualSmoke -Name "source_json_display"
    controlled_visual_smoke = $controlledVisualSmoke
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = $summaryDisplay
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    preflight_status = $preflightStatus
    preflight_summary_json = $preflightSummaryPath
    preflight_summary_json_display = $preflightDisplay
    build_dir = $summaryBuildDir
    build_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryBuildDir
    build_dir_source = $summaryBuildDirSource
    evidence_kind = $summaryEvidenceKind
    evidence_kind_details = $summaryEvidenceKindDetails
    synthetic_evidence_marker_count = $syntheticEvidenceMarkerCount
    synthetic_evidence_markers = @($syntheticEvidenceMarkers)
    requested_build_dir = $summaryRequestedBuildDir
    requested_build_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryRequestedBuildDir
    build_dir_snapshot = $buildDirSnapshot
    build_dir_auto_candidates = @($summaryBuildDirAutoCandidates)
    pdf_dependency_inputs_status = $pdfDependencyInputsStatus
    pdf_dependency_missing_input_count = $pdfDependencyMissingInputCount
    selected_pdfium_provider = $selectedPdfiumProvider
    pdfio_dependency_ready = $pdfioDependencyReady
    pdfium_dependency_ready = $pdfiumDependencyReady
    pdf_dependency_inputs = $pdfDependencyInputs
    free_memory_mb = $freeMemoryMb
    min_free_memory_mb = $minFreeMemoryMb
    memory_guard_blocked = $memoryGuardBlocked
    memory_guard_skipped = $memoryGuardSkipped
    check_count = $checks.Count
    checks = @($checks)
    blocking_check_count = $blockingChecks.Count
    blocking_checks = @($blockingChecks)
    blocking_summary = $preflightBlockingSummary
    output_gap_count = $outputGapSummary.Count
    missing_output_count = $missingOutputCount
    output_gap_summary = @($outputGapSummary)
    readiness_action_evidence_count = $readinessActionEvidence.Count
    readiness_action_evidence = @($readinessActionEvidence)
    source_failure_count = $sourceFailureCount
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
