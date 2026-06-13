if (Test-Scenario -Name "fail_on_warning") {
    $outputDir = Join-Path $resolvedWorkingDir "fail-on-warning-report"
    $inputRoot = Join-Path $resolvedWorkingDir "fail-on-warning-input"
    Write-PassingInputRoot -Root $inputRoot
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir,
        "-FailOnWarning"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Rollup should fail fail-on-warning when PDF preflight warnings are present. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 3 `
        -Message "Fail-on-warning rollup should still write structured warning evidence."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "pdf_controlled_visual_smoke.unavailable_or_failed" `
        -Message "Fail-on-warning rollup should preserve PDF preflight warnings in summary output."
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "release_blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "action_items" `
        -ExpectOpenCommandProperty $true
    Assert-GovernanceTraceMetadata -Items @($summary.informational_action_items) -CollectionName "informational_action_items" `
        -ExpectOpenCommandProperty $true
}

if (Test-Scenario -Name "empty") {
    $outputDir = Join-Path $resolvedWorkingDir "empty-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $emptyPath,
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Empty rollup should pass fail-on-blocker. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Empty rollup should be ready."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Empty rollup should be release-ready."
}

if (Test-Scenario -Name "comma_input") {
    $outputDir = Join-Path $resolvedWorkingDir "comma-input-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", "$documentSkeletonPath,$tableLayoutPath,$releaseCandidatePath",
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Rollup should accept comma-separated input JSON paths. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 3 `
        -Message "Comma-separated input should keep all three source reports."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Comma-separated input should aggregate all blockers."
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "release_blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "action_items" `
        -ExpectOpenCommandProperty $true
    Assert-ContainsText -Text (($summary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
        -ExpectedText "release-candidate" `
        -Message "Comma-separated input should include the final source path."
}

if (Test-Scenario -Name "malformed") {
    $outputDir = Join-Path $resolvedWorkingDir "malformed-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $malformedPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Malformed count should warn but not fail by default. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Malformed count should produce one warning."
    Assert-ContainsText -Text ([string]$summary.warnings[0].message) -ExpectedText "release_blocker_count is 3" `
        -Message "Warning should explain count mismatch."
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_blocker_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Source failures: ``0``" `
        -Message "Malformed-count Markdown should summarize source failure count separately from warnings."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count: ``0``" `
        -Message "Malformed-count Markdown should expose a machine-readable source failure count."
}

if (Test-Scenario -Name "failed_source") {
    $outputDir = Join-Path $resolvedWorkingDir "failed-source-report"
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($failedSourcePath)) -Force | Out-Null
    Set-Content -LiteralPath $failedSourcePath -Encoding UTF8 -Value "{ this is not valid json"

    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $failedSourcePath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 1 `
        -Message "Unreadable source report should fail the rollup after writing evidence. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.source_failure_count) -Expected 1 `
        -Message "Unreadable source report should count one source failure."
    Assert-Equal -Actual ([string]$summary.source_reports[0].status) -Expected "failed" `
        -Message "Unreadable source report should be marked failed."
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_blocker_rollup.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Source failures: ``1``" `
        -Message "Failed-source Markdown should summarize source failures."
    Assert-ContainsText -Text $markdown -ExpectedText "source_failure_count: ``1``" `
        -Message "Failed-source Markdown should expose a machine-readable source failure count."
    Assert-ContainsText -Text $markdown -ExpectedText "status=``failed``" `
        -Message "Failed-source Markdown should show the source report status."
    Assert-ContainsText -Text $markdown -ExpectedText "error:" `
        -Message "Failed-source Markdown should include the source read error."
    Assert-ContainsText -Text $markdown -ExpectedText "{ this is not valid json" `
        -Message "Failed-source Markdown should preserve the unreadable JSON payload preview."
}

if (Test-Scenario -Name "dedupe") {
    $outputDir = Join-Path $resolvedWorkingDir "dedupe-report"
    $dedupeInputRoot = Join-Path $resolvedWorkingDir "dedupe-input"
    Write-JsonFile -Path (Join-Path $dedupeInputRoot "release-candidate\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseCandidatePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $dedupeInputRoot "dedupe\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $dedupePath | ConvertFrom-Json)
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $dedupeInputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Duplicate ids across reports should remain traceable. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Duplicate blocker ids from different reports should both be retained."
    Assert-Equal -Actual ([int]$summary.blocker_id_summary[0].count) -Expected 2 `
        -Message "Blocker id summary should count duplicates."
    Assert-GovernanceTraceMetadata -Items @($summary.release_blockers) -CollectionName "release_blockers"
    Assert-GovernanceTraceMetadata -Items @($summary.warnings) -CollectionName "warnings"
    Assert-GovernanceTraceMetadata -Items @($summary.action_items) -CollectionName "action_items" `
        -ExpectOpenCommandProperty $true
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.open_command }) -join "`n") `
        -ExpectedText "build_release_blocker_rollup_report.ps1" `
        -Message "Action items without source commands should fall back to a rollup rebuild open command."
    Assert-True -Condition ([string]$summary.release_blockers[0].composite_id -ne [string]$summary.release_blockers[1].composite_id) `
        -Message "Composite ids should keep duplicate blockers distinct."
}
