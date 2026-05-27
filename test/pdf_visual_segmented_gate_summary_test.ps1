param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $RepoRoot "build\pdf_visual_segmented_gate_summary_test"
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-SegmentedSummaryScript {
    param(
        [string]$ReportDir,
        [string]$OutputJson
    )

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Unable to resolve PowerShell for nested script invocation."
        }
        $powerShellPath = $powerShellCommand.Source
    }

    $scriptPath = Join-Path $resolvedRepoRoot "scripts\write_pdf_visual_segmented_gate_summary.ps1"
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath `
            -RepoRoot $resolvedRepoRoot `
            -ReportDir $ReportDir `
            -OutputJson $OutputJson 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    if ($exitCode -ne 0) {
        throw "write_pdf_visual_segmented_gate_summary.ps1 failed: $($output -join [Environment]::NewLine)"
    }
}

function New-SegmentedFixture {
    param([string]$Root)

    $reportDir = Join-Path $Root "output\pdf-visual-release-gate-current\report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

    $contactSheetPath = Join-Path $reportDir "aggregate-contact-sheet.png"
    [System.IO.File]::WriteAllBytes($contactSheetPath, [byte[]](1, 2, 3, 4, 5, 6))

    Write-JsonFile -Path (Join-Path $reportDir "attempt-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_gate_attempt_summary.v1"
        status = "partial"
        verdict = "not_complete"
        full_visual_gate_status = "not_complete"
        evidence_scope = "bounded_attempt_auxiliary_only"
        stage_count = 6
        passed_stage_count = 6
        failed_stage_count = 0
        incomplete_stage_count = 0
        visual_baseline_render_status = "pass"
        visual_baseline_fresh_rendered_count = 44
        expected_visual_render_count = 44
        aggregate_contact_sheet_status = "pass"
        aggregate_contact_sheet = $contactSheetPath
        aggregate_contact_sheet_bytes = 6
    })

    Write-JsonFile -Path (Join-Path $reportDir "aggregate-contact-sheet-rebuild-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1"
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "aggregate_contact_sheet_rebuild_only"
        aggregate_contact_sheet = $contactSheetPath
        selected_baseline_count = 44
        total_baseline_count = 44
        visual_baseline_manifest_count = 42
        expected_visual_render_count = 44
    })

    foreach ($slice in @(
            [ordered]@{ Offset = 0; Limit = 28; Selected = 28 },
            [ordered]@{ Offset = 22; Limit = 25; Selected = 25 }
        )) {
        $sliceId = "visual-baseline-slice-offset-$($slice.Offset)-limit-$($slice.Limit)"
        $sliceContactSheet = Join-Path $reportDir "$sliceId-contact-sheet.png"
        [System.IO.File]::WriteAllBytes($sliceContactSheet, [byte[]](1, 2, 3))
        Write-JsonFile -Path (Join-Path $reportDir "$sliceId-summary.json") -Value ([ordered]@{
            schema = "featherdoc.pdf_visual_baseline_slice.v1"
            status = "pass"
            verdict = "pass"
            full_visual_gate_status = "not_complete"
            evidence_scope = "visual_baseline_slice_only"
            visual_baseline_offset = [int]$slice.Offset
            visual_baseline_limit = [int]$slice.Limit
            selected_baseline_count = [int]$slice.Selected
            total_baseline_count = 44
            expected_visual_render_count = 44
            slice_contact_sheet = $sliceContactSheet
        })
    }

    return [pscustomobject]@{
        ReportDir = $reportDir
        OutputJson = Join-Path $reportDir "segmented-summary.json"
    }
}

function New-SegmentedResumeFixture {
    param([string]$Root)

    $reportDir = Join-Path $Root "output\pdf-visual-release-gate-current\report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

    $contactSheetPath = Join-Path $reportDir "aggregate-contact-sheet.png"
    [System.IO.File]::WriteAllBytes($contactSheetPath, [byte[]](1, 2, 3, 4, 5, 6, 7, 8))

    Write-JsonFile -Path (Join-Path $reportDir "attempt-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_gate_attempt_summary.v1"
        status = "partial"
        verdict = "not_complete"
        full_visual_gate_status = "not_complete"
        evidence_scope = "bounded_attempt_auxiliary_only"
        stage_count = 6
        passed_stage_count = 4
        failed_stage_count = 0
        incomplete_stage_count = 2
        visual_baseline_render_status = "partial"
        visual_baseline_fresh_rendered_count = 3
        expected_visual_render_count = 44
        aggregate_contact_sheet_status = "stale"
        aggregate_contact_sheet = $contactSheetPath
        aggregate_contact_sheet_bytes = 8
    })

    Write-JsonFile -Path (Join-Path $reportDir "segmented-resume-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_segmented_resume_summary.v1"
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "visual_segmented_resume_auxiliary_only"
        boundary = "segmented_resume_does_not_replace_full_visual_gate_verdict"
        resume_tail_fully_planned = $true
        total_resume_slice_count = 7
        planned_slice_count = 7
        executed_slice_count = 7
        passed_slice_count = 7
        failed_slice_count = 0
        timeout_slice_count = 0
        aggregate_rebuild_status = "pass"
        segmented_summary_status = "not_run"
    })

    Write-JsonFile -Path (Join-Path $reportDir "aggregate-contact-sheet-rebuild-summary.json") -Value ([ordered]@{
        schema = "featherdoc.pdf_visual_aggregate_contact_sheet_rebuild.v1"
        status = "pass"
        verdict = "pass"
        full_visual_gate_status = "not_complete"
        evidence_scope = "aggregate_contact_sheet_rebuild_only"
        aggregate_contact_sheet = $contactSheetPath
        selected_baseline_count = 44
        total_baseline_count = 44
        visual_baseline_manifest_count = 42
        expected_visual_render_count = 44
    })

    foreach ($slice in @(
            [ordered]@{ Offset = 3; Limit = 6; Selected = 6 },
            [ordered]@{ Offset = 9; Limit = 6; Selected = 6 },
            [ordered]@{ Offset = 15; Limit = 6; Selected = 6 },
            [ordered]@{ Offset = 21; Limit = 6; Selected = 6 },
            [ordered]@{ Offset = 27; Limit = 6; Selected = 6 },
            [ordered]@{ Offset = 33; Limit = 6; Selected = 6 },
            [ordered]@{ Offset = 39; Limit = 5; Selected = 5 }
        )) {
        $sliceId = "visual-baseline-slice-offset-$($slice.Offset)-limit-$($slice.Limit)"
        $sliceContactSheet = Join-Path $reportDir "$sliceId-contact-sheet.png"
        [System.IO.File]::WriteAllBytes($sliceContactSheet, [byte[]](1, 2, 3))
        Write-JsonFile -Path (Join-Path $reportDir "$sliceId-summary.json") -Value ([ordered]@{
            schema = "featherdoc.pdf_visual_baseline_slice.v1"
            status = "pass"
            verdict = "pass"
            full_visual_gate_status = "not_complete"
            evidence_scope = "visual_baseline_slice_only"
            visual_baseline_offset = [int]$slice.Offset
            visual_baseline_limit = [int]$slice.Limit
            selected_baseline_count = [int]$slice.Selected
            total_baseline_count = 44
            expected_visual_render_count = 44
            slice_contact_sheet = $sliceContactSheet
        })
    }

    return [pscustomobject]@{
        ReportDir = $reportDir
        OutputJson = Join-Path $reportDir "segmented-summary.json"
    }
}

$resolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$fixture = New-SegmentedFixture -Root $resolvedWorkingDir
Invoke-SegmentedSummaryScript -ReportDir $fixture.ReportDir -OutputJson $fixture.OutputJson

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $fixture.OutputJson | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.pdf_visual_segmented_gate_summary.v1" `
    -Message "Segmented summary should expose its schema."
Assert-Equal -Actual ([string]$summary.status) -Expected "pass" `
    -Message "Segmented summary should pass when all bounded segments are complete."
Assert-Equal -Actual ([string]$summary.verdict) -Expected "pass" `
    -Message "Segmented summary should expose an explicit pass verdict for auxiliary evidence."
Assert-Equal -Actual ([string]$summary.full_visual_gate_status) -Expected "not_complete" `
    -Message "Segmented summary must not promote the full visual gate status."
Assert-Equal -Actual ([string]$summary.evidence_scope) -Expected "segmented_visual_gate_auxiliary_only" `
    -Message "Segmented summary should keep auxiliary scope explicit."
Assert-Equal -Actual ([string]$summary.boundary) -Expected "segmented_summary_does_not_replace_full_visual_gate_verdict" `
    -Message "Segmented summary should keep the full-gate boundary explicit."
Assert-Equal -Actual ([int]$summary.slice_summary_count) -Expected 2 `
    -Message "Segmented summary should count slice summaries."
Assert-Equal -Actual ([int]$summary.slice_pass_count) -Expected 2 `
    -Message "Segmented summary should count passing slices."
Assert-Equal -Actual ([int]$summary.covered_baseline_count) -Expected 44 `
    -Message "Segmented summary should preserve full visual baseline coverage."
Assert-Equal -Actual ([int]$summary.expected_visual_render_count) -Expected 44 `
    -Message "Segmented summary should preserve expected baseline coverage."
Assert-Equal -Actual ([int]$summary.visual_baseline_manifest_count) -Expected 42 `
    -Message "Segmented summary should preserve visual baseline manifest count."
Assert-Equal -Actual ([int]$summary.attempt_passed_stage_count) -Expected 6 `
    -Message "Segmented summary should preserve attempt stage evidence."
Assert-Equal -Actual ([string]$summary.aggregate_contact_sheet_status) -Expected "pass" `
    -Message "Segmented summary should preserve aggregate contact-sheet status."
Assert-Equal -Actual ([int64]$summary.aggregate_contact_sheet_bytes) -Expected 6 `
    -Message "Segmented summary should preserve contact-sheet byte size."
Assert-ContainsText -Text ([string]$summary.summary_json_display) -ExpectedText "segmented-summary.json" `
    -Message "Segmented summary should expose reviewer-openable summary display path."
Assert-ContainsText -Text ([string]$summary.aggregate_contact_sheet_display) -ExpectedText "aggregate-contact-sheet.png" `
    -Message "Segmented summary should expose reviewer-openable contact-sheet display path."

$resumeFixture = New-SegmentedResumeFixture -Root (Join-Path $resolvedWorkingDir "resume-complete")
Invoke-SegmentedSummaryScript -ReportDir $resumeFixture.ReportDir -OutputJson $resumeFixture.OutputJson

$resumeSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $resumeFixture.OutputJson | ConvertFrom-Json
Assert-Equal -Actual ([string]$resumeSummary.status) -Expected "pass" `
    -Message "Segmented summary should pass when complete resume evidence closes a partial full-gate attempt."
Assert-Equal -Actual ([string]$resumeSummary.verdict) -Expected "pass" `
    -Message "Complete resume evidence should expose a pass verdict for segmented auxiliary evidence."
Assert-Equal -Actual ([string]$resumeSummary.full_visual_gate_status) -Expected "not_complete" `
    -Message "Complete resume evidence must not promote the full visual gate status."
Assert-Equal -Actual ([string]$resumeSummary.attempt_status) -Expected "partial" `
    -Message "Segmented summary should preserve the original partial attempt status."
Assert-Equal -Actual ([string]$resumeSummary.segmented_resume_status) -Expected "pass" `
    -Message "Segmented summary should expose consumed resume status."
Assert-Equal -Actual ([bool]$resumeSummary.segmented_resume_tail_fully_planned) -Expected $true `
    -Message "Segmented summary should expose full-tail resume planning evidence."
Assert-Equal -Actual ([int]$resumeSummary.segmented_resume_total_slice_count) -Expected 7 `
    -Message "Segmented summary should expose total resume slice count."
Assert-Equal -Actual ([int]$resumeSummary.segmented_resume_passed_slice_count) -Expected 7 `
    -Message "Segmented summary should expose passed resume slice count."
Assert-Equal -Actual ([string]$resumeSummary.segmented_resume_aggregate_rebuild_status) -Expected "pass" `
    -Message "Segmented summary should expose resume aggregate rebuild status."
Assert-Equal -Actual ([string]$resumeSummary.segmented_resume_summary_status) -Expected "not_run" `
    -Message "Segmented summary should preserve the interim resume refresh status without requiring a circular dependency."
Assert-Equal -Actual ([int]$resumeSummary.covered_baseline_count) -Expected 44 `
    -Message "Resume-backed segmented summary should preserve full visual baseline coverage."
Assert-ContainsText -Text ([string]$resumeSummary.segmented_resume_summary_json_display) -ExpectedText "segmented-resume-summary.json" `
    -Message "Segmented summary should expose reviewer-openable resume summary display path."

Write-Host "PDF visual segmented gate summary regression passed."
