Write-Step "Completed release-candidate checks"
Write-Host "Summary: $summaryPath"
Write-Host "Final review: $finalReviewPath"
Write-Host "Release handoff: $releaseHandoffPath"
Write-Host "Release body output: $releaseBodyZhCnPath"
Write-Host "Release summary output: $releaseSummaryZhCnPath"
Write-Host "Artifact guide: $artifactGuidePath"
Write-Host "Reviewer checklist: $reviewerChecklistPath"
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfVisualGateSummaryJson)) {
    Write-Host "PDF visual gate summary: $resolvedPdfVisualGateSummaryJson"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfVisualGateAttemptSummaryJson)) {
    Write-Host "PDF visual gate attempt summary: $resolvedPdfVisualGateAttemptSummaryJson"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfVisualSegmentedGateSummaryJson)) {
    Write-Host "PDF visual segmented gate summary: $resolvedPdfVisualSegmentedGateSummaryJson"
}
if (@($resolvedPdfBoundedCtestSummaryJson).Count -gt 0) {
    Write-Host "PDF bounded CTest summaries: $($resolvedPdfBoundedCtestSummaryJson -join ', ')"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedPdfReleaseReadinessSummaryJson)) {
    Write-Host "PDF release readiness summary: $resolvedPdfReleaseReadinessSummaryJson"
}
if ($projectTemplateSmokeRequested) {
    Write-Host "Project template schema approval history JSON: $schemaApprovalHistoryJsonPath"
    Write-Host "Project template schema approval history Markdown: $schemaApprovalHistoryMarkdownPath"
}
if ($releaseBlockerRollupRequested) {
    Write-Host "Release blocker rollup summary: $releaseBlockerRollupSummaryPath"
    Write-Host "Release blocker rollup report: $releaseBlockerRollupMarkdownPath"
}
if ($releaseGovernanceHandoffRequested) {
    Write-Host "Release governance handoff summary: $releaseGovernanceHandoffSummaryPath"
    Write-Host "Release governance handoff report: $releaseGovernanceHandoffMarkdownPath"
}
Write-Host "Start here: $startHerePath"

if ($null -ne $releaseBlockerRollupFailure) {
    throw $releaseBlockerRollupFailure
}
if ($null -ne $releaseGovernanceHandoffFailure) {
    throw $releaseGovernanceHandoffFailure
}

$global:LASTEXITCODE = 0
