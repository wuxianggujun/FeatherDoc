if (Test-Scenario -Name "explicit_only") {
    $inputRoot = Join-Path $resolvedWorkingDir "explicit-only-input"
    $explicitRoot = Join-Path $resolvedWorkingDir "explicit-only-explicit"
    $outputDir = Join-Path $resolvedWorkingDir "explicit-only-report"
    $releaseCandidateSummaryPath = Join-Path $explicitRoot "release-candidate-summary.json"
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    Write-JsonFile -Path $releaseCandidateSummaryPath -Value ([ordered]@{
        schema = "featherdoc.release_candidate_summary"
        status = "pass"
        release_ready = $true
        release_evidence_scope = "pdf-only"
        warning_count = 0
        warnings = @()
        pdf_visual_gate_summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
        pdf_visual_gate = [ordered]@{
            status = "loaded"
            verdict = "pass"
            full_visual_gate_status = "pass"
            finalizable = $true
            summary_json = "output/pdf-visual-release-gate-current/report/summary.json"
            aggregate_contact_sheet = "output/pdf-visual-release-gate-current/report/aggregate-contact-sheet.png"
            cjk_manifest_count = 43
            cjk_copy_search_count = 43
            cjk_missing_text_count = 0
            visual_baseline_manifest_count = 42
            visual_baseline_count = 44
        }
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", $releaseCandidateSummaryPath,
        "-OutputDir", $outputDir,
        "-ExpectedReportProfile", "explicit-only",
        "-IncludeReleaseBlockerRollup"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Explicit-only handoff should accept a PDF-only release candidate without requiring non-PDF reports. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Explicit-only handoff should be ready when explicit PDF evidence has no blockers or warnings."
    Assert-Equal -Actual ([string]$summary.expected_report_profile) -Expected "explicit-only" `
        -Message "Explicit-only handoff should record the expected report profile."
    Assert-Equal -Actual ([int]$summary.expected_report_count) -Expected 0 `
        -Message "Explicit-only handoff should not count default all-project governance reports."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Explicit-only handoff should not mark omitted non-PDF reports missing."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 1 `
        -Message "Explicit-only handoff should still build a rollup from the explicit release candidate."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.pdf_visual_gate_evidence_source_report_count) -Expected 1 `
        -Message "Explicit-only handoff should preserve PDF visual gate release evidence."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.manifest_signoff_entrypoints_source_report_count) -Expected 0 `
        -Message "PDF-only explicit handoff should not synthesize project-template manifest signoff evidence."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.project_template_readiness_checklist_entrypoints_source_report_count) -Expected 0 `
        -Message "PDF-only explicit handoff should not synthesize project-template checklist entrypoint evidence."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Reports loaded: ``0`` / ``0``" `
        -Message "Explicit-only handoff Markdown should show that no default reports were expected."
    Assert-ContainsText -Text $markdown -ExpectedText "PDF visual gate evidence source reports: ``1``" `
        -Message "Explicit-only handoff Markdown should expose PDF visual evidence from the explicit release candidate."
}

