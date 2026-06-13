if (Test-Scenario -Name "include_rollup") {
    . (Join-Path $PSScriptRoot "build_release_governance_handoff_report_include_rollup_setup.ps1")
    . (Join-Path $PSScriptRoot "build_release_governance_handoff_report_include_rollup_summary_assertions.ps1")
    . (Join-Path $PSScriptRoot "build_release_governance_handoff_report_include_rollup_rollup_assertions.ps1")
    . (Join-Path $PSScriptRoot "build_release_governance_handoff_report_include_rollup_markdown_assertions.ps1")
}
