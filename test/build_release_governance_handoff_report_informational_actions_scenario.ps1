if (Test-Scenario -Name "informational_actions") {
    $inputRoot = Join-Path $resolvedWorkingDir "informational-actions-input"
    $explicitRoot = Join-Path $resolvedWorkingDir "informational-actions-explicit"
    $outputDir = Join-Path $resolvedWorkingDir "informational-actions-report"
    $numberingSummaryPath = Join-Path $explicitRoot "numbering-summary.json"
    New-Item -ItemType Directory -Path $inputRoot -Force | Out-Null
    Write-JsonFile -Path $numberingSummaryPath -Value ([ordered]@{
        schema = "featherdoc.numbering_catalog_governance_report.v1"
        status = "clean"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        informational_action_item_count = 2
        informational_action_items = @(
            [ordered]@{
                id = "promote_numbering_catalog_exemplar"
                action = "promote_numbering_catalog_exemplar"
                title = "Review and promote the generated exemplar numbering catalog"
                category = "release_checklist"
                severity = "info"
                release_blocking = $false
                optional = $true
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report = "output/document-skeleton-governance-rollup/summary.json"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
                open_command = "featherdoc_cli check-numbering-catalog samples/invoice.docx --json"
            },
            [ordered]@{
                id = "register_numbering_catalog_baseline"
                action = "register_numbering_catalog_baseline"
                title = "Register the exemplar catalog in the numbering catalog baseline flow"
                category = "release_checklist"
                severity = "info"
                release_blocking = $false
                optional = $true
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                source_report = "output/document-skeleton-governance-rollup/summary.json"
                source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
                source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
                open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx samples/invoice.docx"
            }
        )
        warning_count = 0
        warnings = @()
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", $numberingSummaryPath,
        "-OutputDir", $outputDir,
        "-ExpectedReportProfile", "explicit-only",
        "-IncludeReleaseBlockerRollup"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Handoff should accept informational release checklist actions without creating actionable release items. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 0 `
        -Message "Handoff should not count informational release checklist entries as actionable items."
    Assert-Equal -Actual ([int]$summary.informational_action_item_count) -Expected 2 `
        -Message "Handoff should preserve informational release checklist entries."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 0 `
        -Message "Nested rollup should not count informational release checklist entries as actionable items."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.informational_action_item_count) -Expected 2 `
        -Message "Nested rollup should preserve informational release checklist entries."
    $rollupSummaryPath = Join-Path $outputDir "release-blocker-rollup\summary.json"
    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
    Assert-ContainsText -Text (($summary.release_blocker_rollup.informational_action_item_source_schema_summary | ForEach-Object { "$($_.source_schema):$($_.count)" }) -join "`n") `
        -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1:2" `
        -Message "Handoff summary should consume nested informational action source schema summary."
    Assert-Equal -Actual (
        ($summary.release_blocker_rollup.informational_action_item_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Expected (
        ($rollupSummary.informational_action_item_source_schema_summary | ConvertTo-Json -Depth 12 -Compress)
    ) -Message "Handoff summary should mirror nested informational action source schema summary."
    foreach ($id in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline")) {
        Assert-Equal -Actual (@($summary.action_items | Where-Object { [string]$_.id -eq $id }).Count) -Expected 0 `
            -Message "Handoff should not duplicate informational release checklist action '$id' as an actionable item."
        Assert-Equal -Actual (@($rollupSummary.action_items | Where-Object { [string]$_.id -eq $id }).Count) -Expected 0 `
            -Message "Nested rollup should not duplicate informational release checklist action '$id' as an actionable item."
    }

    $promoteAction = ($summary.informational_action_items |
        Where-Object { [string]$_.id -eq "promote_numbering_catalog_exemplar" } |
        Select-Object -First 1)
    $registerAction = ($summary.informational_action_items |
        Where-Object { [string]$_.id -eq "register_numbering_catalog_baseline" } |
        Select-Object -First 1)
    foreach ($item in @($promoteAction, $registerAction)) {
        Assert-NonEmptyString -Value $item `
            -Message "Handoff should preserve informational release checklist action details."
        Assert-Equal -Actual ([string]$item.source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
            -Message "Handoff informational action should preserve source schema."
        Assert-ContainsText -Text ([string]$item.source_json_display) -ExpectedText "document-skeleton-governance-rollup\summary.json" `
            -Message "Handoff informational action should preserve source JSON display."
        Assert-NonEmptyString -Value $item.open_command `
            -Message "Handoff informational action should preserve reviewer open command."
        Assert-Equal -Actual ([string]$item.release_blocking) -Expected "False" `
            -Message "Handoff informational action should remain non-blocking."
        Assert-Equal -Actual ([string]$item.optional) -Expected "True" `
            -Message "Handoff informational action should remain optional."
    }
    Assert-ContainsText -Text ([string]$promoteAction.open_command) -ExpectedText "check-numbering-catalog" `
        -Message "Handoff promote informational action should keep the exemplar review command."
    Assert-ContainsText -Text ([string]$registerAction.open_command) -ExpectedText "check_numbering_catalog_baseline.ps1" `
        -Message "Handoff register informational action should keep the baseline registration command."

    $nestedInformationalActions = @($rollupSummary.informational_action_items)
    foreach ($item in @($nestedInformationalActions)) {
        Assert-Equal -Actual ([string]$item.source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
            -Message "Nested rollup informational action should preserve source schema."
        Assert-ContainsText -Text ([string]$item.source_json_display) -ExpectedText "document-skeleton-governance-rollup\summary.json" `
            -Message "Nested rollup informational action should preserve source JSON display."
        Assert-NonEmptyString -Value $item.open_command `
            -Message "Nested rollup informational action should preserve reviewer open command."
        Assert-Equal -Actual ([string]$item.release_blocking) -Expected "False" `
            -Message "Nested rollup informational action should remain non-blocking."
        Assert-Equal -Actual ([string]$item.optional) -Expected "True" `
            -Message "Nested rollup informational action should remain optional."
    }

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Informational Action Items" `
        -Message "Handoff Markdown should expose informational action items separately."
    Assert-ContainsText -Text $markdown -ExpectedText "release_blocking=``False`` optional=``True``" `
        -Message "Handoff Markdown should mark informational actions as non-blocking optional checklist entries."
    Assert-ContainsText -Text $markdown -ExpectedText "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Handoff Markdown should include informational action source schema."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display:" `
        -Message "Handoff Markdown should include informational action source JSON display."
    Assert-ContainsText -Text $markdown -ExpectedText "document-skeleton-governance-rollup\summary.json" `
        -Message "Handoff Markdown should point informational actions at source JSON."
    Assert-ContainsText -Text $markdown -ExpectedText "open_command:" `
        -Message "Handoff Markdown should include informational action open commands."
    Assert-ContainsText -Text $markdown -ExpectedText "check-numbering-catalog" `
        -Message "Handoff Markdown should include the exemplar review command."
    Assert-ContainsText -Text $markdown -ExpectedText "check_numbering_catalog_baseline.ps1" `
        -Message "Handoff Markdown should include the baseline registration command."
}

