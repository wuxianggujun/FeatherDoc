param(
    [string]$RepoRoot = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing expected text: $ExpectedText"
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}
$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
. (Join-Path $resolvedRepoRoot "scripts\release_blocker_metadata_helpers.ps1")

$rollupLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceRollupMarkdownSection `
    -Lines $rollupLines `
    -Summary ([pscustomobject]@{
        release_blocker_rollup = [pscustomobject]@{
            requested = $true
            status = "blocked"
            source_report_count = 1
            source_failure_count = 1
            source_reports = @(
                [pscustomobject]@{
                    schema = "featherdoc.table_layout_delivery_governance_report.v1"
                    status = "failed"
                    release_ready = $false
                    path = "output/table-layout-delivery-governance/summary.json"
                    path_display = ".\output\table-layout-delivery-governance\summary.json"
                    source_json = "output/table-layout-delivery-governance/evidence.json"
                    source_json_display = ".\output\table-layout-delivery-governance\evidence.json"
                    source_failure_count = 0
                    error = "Unexpected token while reading table layout governance summary."
                }
            )
            governance_metrics = @()
            release_blockers = @()
            warnings = @()
            action_items = @()
        }
    }) `
    -RepoRoot $resolvedRepoRoot
$rollupMarkdown = $rollupLines -join "`n"
Assert-ContainsText -Text $rollupMarkdown -ExpectedText "### Rollup Source Report Issues" `
    -Message "Rollup Markdown should include source report issue details."
Assert-ContainsText -Text $rollupMarkdown -ExpectedText "source_report: output/table-layout-delivery-governance/summary.json" `
    -Message "Rollup Markdown should render failed source report paths."
Assert-ContainsText -Text $rollupMarkdown -ExpectedText "source_report_display: .\output\table-layout-delivery-governance\summary.json" `
    -Message "Rollup Markdown should render failed source report display paths."
Assert-ContainsText -Text $rollupMarkdown -ExpectedText "source_json: output/table-layout-delivery-governance/evidence.json" `
    -Message "Rollup Markdown should render failed source JSON paths."
Assert-ContainsText -Text $rollupMarkdown -ExpectedText "source_json_display: .\output\table-layout-delivery-governance\evidence.json" `
    -Message "Rollup Markdown should render failed source JSON display paths."
Assert-ContainsText -Text $rollupMarkdown -ExpectedText "error: Unexpected token while reading table layout governance summary." `
    -Message "Rollup Markdown should render failed source report errors."

$handoffLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceHandoffMarkdownSection `
    -Lines $handoffLines `
    -Summary ([pscustomobject]@{
        release_governance_handoff = [pscustomobject]@{
            requested = $true
            status = "failed"
            loaded_report_count = 0
            expected_report_count = 2
            missing_report_count = 1
            failed_report_count = 1
            reports = @(
                [pscustomobject]@{
                    id = "table_layout_delivery_governance"
                    status = "failed"
                    release_ready = $false
                    expected_summary = "output/table-layout-delivery-governance/summary.json"
                    expected_summary_display = ".\output\table-layout-delivery-governance\summary.json"
                    source_json = "output/table-layout-delivery-governance/evidence.json"
                    source_json_display = ".\output\table-layout-delivery-governance\evidence.json"
                    source_failure_count = 1
                    schema = "featherdoc.table_layout_delivery_governance_report.v1"
                    error = "Failed to parse table layout governance summary."
                    build_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_table_layout_delivery_governance_report.ps1"
                },
                [pscustomobject]@{
                    id = "numbering_catalog_governance"
                    status = "missing"
                    release_ready = $false
                    expected_summary = "output/numbering-catalog-governance/summary.json"
                    expected_summary_display = ".\output\numbering-catalog-governance\summary.json"
                    source_failure_count = 0
                    schema = ""
                    build_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_numbering_catalog_governance_report.ps1"
                }
            )
            governance_metrics = @()
            release_blockers = @()
            warnings = @()
            action_items = @()
        }
    }) `
    -RepoRoot $resolvedRepoRoot
$handoffMarkdown = $handoffLines -join "`n"
Assert-ContainsText -Text $handoffMarkdown -ExpectedText "### Handoff Report Issues" `
    -Message "Handoff Markdown should include missing and failed report details."
Assert-ContainsText -Text $handoffMarkdown -ExpectedText "table_layout_delivery_governance: status=failed" `
    -Message "Handoff Markdown should render failed report status."
Assert-ContainsText -Text $handoffMarkdown -ExpectedText "source_report: output/table-layout-delivery-governance/summary.json" `
    -Message "Handoff Markdown should render failed report source paths."
Assert-ContainsText -Text $handoffMarkdown -ExpectedText "source_json: output/table-layout-delivery-governance/evidence.json" `
    -Message "Handoff Markdown should render failed source JSON paths."
Assert-ContainsText -Text $handoffMarkdown -ExpectedText "source_json_display: .\output\table-layout-delivery-governance\evidence.json" `
    -Message "Handoff Markdown should render failed source JSON display paths."
Assert-ContainsText -Text $handoffMarkdown -ExpectedText "error: Failed to parse table layout governance summary." `
    -Message "Handoff Markdown should render failed report errors."
Assert-ContainsText -Text $handoffMarkdown -ExpectedText "numbering_catalog_governance: status=missing" `
    -Message "Handoff Markdown should render missing report status."
Assert-ContainsText -Text $handoffMarkdown -ExpectedText "build: pwsh -ExecutionPolicy Bypass -File .\scripts\build_numbering_catalog_governance_report.ps1" `
    -Message "Handoff Markdown should render missing report build commands."

Write-Host "Release governance report issue Markdown contract passed."
