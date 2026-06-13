if (Test-Scenario -Name "missing") {
    $inputRoot = Join-Path $resolvedWorkingDir "missing-input"
    $outputDir = Join-Path $resolvedWorkingDir "missing-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Missing handoff should pass without fail-on-missing. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Existing blockers should still drive blocked status."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 1 `
        -Message "Missing handoff should record the missing project-template report."
    Assert-ContainsText -Text (($summary.reports | Where-Object { $_.status -eq "missing" } | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "project_template_delivery_readiness" `
        -Message "Missing handoff should identify the absent project-template report."
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Missing reports: ``1``" `
        -Message "Missing handoff Markdown should summarize the missing report count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Missing handoff Markdown should include the absent project-template report id."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``missing``" `
        -Message "Missing handoff Markdown should expose the missing report status."
    Assert-ContainsText -Text $markdown -ExpectedText "project-template-delivery-readiness\summary.json" `
        -Message "Missing handoff Markdown should include the expected missing summary path."
    Assert-ContainsText -Text $markdown -ExpectedText "build_project_template_delivery_readiness_report.ps1" `
        -Message "Missing handoff Markdown should include the rebuild command for the absent report."
}

if (Test-Scenario -Name "failed_report") {
    $inputRoot = Join-Path $resolvedWorkingDir "failed-report-input"
    $outputDir = Join-Path $resolvedWorkingDir "failed-report"
    Write-GovernanceFixtures -Root $inputRoot
    Set-Content -LiteralPath (Join-Path $inputRoot "project-template-delivery-readiness\summary.json") `
        -Encoding UTF8 `
        -Value "{ this is not valid json"

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir
    )
    if ($result.ExitCode -eq 0) {
        throw "Failed-report handoff should fail when one summary is unreadable. Output: $($result.Text)"
    }

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "failed" `
        -Message "Unreadable handoff source should produce failed status."
    Assert-Equal -Actual ([int]$summary.failed_report_count) -Expected 1 `
        -Message "Unreadable handoff source should count one failed report."
    Assert-ContainsText -Text (($summary.reports | Where-Object { $_.status -eq "failed" } | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "project_template_delivery_readiness" `
        -Message "Unreadable handoff source should identify the failed project-template report."
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Failed reports: ``1``" `
        -Message "Failed handoff Markdown should summarize the failed report count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Failed handoff Markdown should include the failed report id."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``failed``" `
        -Message "Failed handoff Markdown should expose the failed report status."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failures=``0``" `
        -Message "Failed handoff Markdown should expose the failed report source failure count."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count=``0``" `
        -Message "Failed handoff Markdown should expose the failed report source failure count as a machine-readable field."
    Assert-ContainsText -Text $markdown -ExpectedText "error:" `
        -Message "Failed handoff Markdown should include the source failure error."
    Assert-ContainsText -Text $markdown -ExpectedText "{ this is not valid json" `
        -Message "Failed handoff Markdown should preserve the unreadable JSON payload preview."
}

if (Test-Scenario -Name "fail_on_missing") {
    $inputRoot = Join-Path $resolvedWorkingDir "fail-missing-input"
    $outputDir = Join-Path $resolvedWorkingDir "fail-missing-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir,
        "-FailOnMissing"
    )
    if ($result.ExitCode -eq 0) {
        throw "Fail-on-missing handoff should fail. Output: $($result.Text)"
    }
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $outputDir "summary.json")) `
        -Message "Fail-on-missing handoff should still write summary.json."
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Missing reports: ``1``" `
        -Message "Fail-on-missing handoff Markdown should summarize the missing report count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Fail-on-missing handoff Markdown should include the missing report id."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``missing``" `
        -Message "Fail-on-missing handoff Markdown should expose the missing report status."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failures=``0``" `
        -Message "Fail-on-missing handoff Markdown should expose the missing report source failure count."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count=``0``" `
        -Message "Fail-on-missing handoff Markdown should expose the missing report source failure count as a machine-readable field."
    Assert-ContainsText -Text $markdown -ExpectedText "build_project_template_delivery_readiness_report.ps1" `
        -Message "Fail-on-missing handoff Markdown should include the rebuild command."
}

if (Test-Scenario -Name "fail_on_blocker") {
    $inputRoot = Join-Path $resolvedWorkingDir "fail-blocker-input"
    $outputDir = Join-Path $resolvedWorkingDir "fail-blocker-report"
    Write-GovernanceFixtures -Root $inputRoot

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Handoff should fail fail-on-blocker when governance blockers are present. Output: $($result.Text)"
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $outputDir "summary.json")) `
        -Message "Fail-on-blocker handoff should still write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")) `
        -Message "Fail-on-blocker handoff should still write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Fail-on-blocker handoff should keep blocker summaries in blocked status."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Fail-on-blocker handoff should still write structured blocker evidence."
    Assert-ContainsText -Text (($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "project_template_delivery.pending_schema_approval" `
        -Message "Fail-on-blocker handoff should preserve project-template blockers in summary output."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Release blockers: ``4``" `
        -Message "Fail-on-blocker handoff Markdown should summarize the blocker count."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery.pending_schema_approval" `
        -Message "Fail-on-blocker handoff Markdown should include blocker ids."
    if ($Scenario -ne "all") {
        return
    }
}

if (Test-Scenario -Name "fail_on_warning") {
    $inputRoot = Join-Path $resolvedWorkingDir "fail-warning-input"
    $explicitRoot = Join-Path $resolvedWorkingDir "fail-warning-explicit"
    $outputDir = Join-Path $resolvedWorkingDir "fail-warning-report"
    $releaseCandidateSummaryPath = Join-Path $explicitRoot "release-candidate-summary.json"
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    Write-JsonFile -Path $releaseCandidateSummaryPath -Value ([ordered]@{
        schema = "featherdoc.release_candidate_summary"
        status = "needs_review"
        release_ready = $false
        warning_count = 1
        warnings = @(
            [ordered]@{
                id = "pdf_controlled_visual_smoke.unavailable_or_failed"
                action = "rerun_pdf_controlled_visual_smoke_check"
                status = "fail"
                message = "Controlled PDF visual smoke evidence was provided but is not passing."
                source_schema = "featherdoc.pdf_visual_release_gate_preflight_governance_report.v1"
                source_report_display = "output\pdf-visual-release-gate-preflight-governance\summary.json"
                source_json_display = "output\pdf-visual-release-gate-preflight-governance\controlled-visual-smoke-failed.json"
            }
        )
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", $releaseCandidateSummaryPath,
        "-OutputDir", $outputDir,
        "-ExpectedReportProfile", "explicit-only",
        "-FailOnWarning"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Handoff should fail fail-on-warning when explicit PDF warnings are present. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Fail-on-warning handoff should keep warning-only summaries in needs_review status."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Fail-on-warning handoff should still write structured warning evidence."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
        -Message "Fail-on-warning handoff should preserve PDF preflight warnings in summary output."
    if ($Scenario -ne "all") {
        return
    }
}

if (Test-Scenario -Name "explicit_input") {
    $inputRoot = Join-Path $resolvedWorkingDir "explicit-input-root"
    $explicitRoot = Join-Path $resolvedWorkingDir "explicit-input"
    $outputDir = Join-Path $resolvedWorkingDir "explicit-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false
    Write-JsonFile -Path (Join-Path $explicitRoot "project-summary.json") -Value ([ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
    })
    Write-JsonFile -Path (Join-Path $explicitRoot "style-summary.json") -Value ([ordered]@{
        schema = "featherdoc.style_catalog_governance_report.v1"
        real_corpus_confidence_score = 12
        real_corpus_confidence_level = "experimental"
        real_corpus_confidence = [ordered]@{
            score = 12
            level = "experimental"
        }
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", ((Join-Path $explicitRoot "project-summary.json") + "," + (Join-Path $explicitRoot "style-summary.json")),
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Explicit handoff should accept an explicit replacement report. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 6 `
        -Message "Explicit handoff should count loaded defaults plus the explicit project-template report."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Explicit handoff should replace the default project-template report without missing defaults."
    Assert-Equal -Actual ([string]($summary.reports | Where-Object { $_.id -eq "project_template_delivery_readiness" } | Select-Object -First 1).source) -Expected "explicit" `
        -Message "Explicit handoff should mark the replacement source."
    Assert-Equal -Actual ([int]$summary.governance_metric_count) -Expected 2 `
        -Message "Explicit handoff should ignore unsupported style real-corpus metric contracts."
    $metricText = ($summary.governance_metrics | ForEach-Object { "$($_.report_id):$($_.metric):$($_.level):$($_.score)" }) -join "`n"
    if ($metricText -match "experimental") {
        throw "Explicit handoff should not treat style governance real_corpus_confidence as numbering confidence."
    }
}

