<#
.SYNOPSIS
Audits release-facing FeatherDoc materials for draft wording and local path leaks.

.DESCRIPTION
Scans one or more files or directories and fails if release-facing text files
contain draft/草稿 wording or machine-local absolute paths. Directories are
scanned recursively but only text-like files are considered.

.PARAMETER Path
One or more files or directories to scan.

.PARAMETER AdditionalForbiddenPattern
Optional extra regex patterns to audit in addition to the built-in rules.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\assert_release_material_safety.ps1 `
    -Path .\output\release-candidate-checks\START_HERE.md, .\output\release-candidate-checks\report
#>
param(
    [Parameter(Mandatory = $true)]
    [string[]]$Path,
    [string[]]$AdditionalForbiddenPattern = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$PSDefaultParameterValues["Get-Content:Encoding"] = "utf8"
$PSDefaultParameterValues["Select-String:Encoding"] = "utf8"

. (Join-Path $PSScriptRoot "assert_release_material_safety_core.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_release_entry_traces.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_release_summary_traces.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_release_handoff_traces.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_final_review_traces.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_json_contracts.ps1")
. (Join-Path $PSScriptRoot "assert_release_material_safety_manifest_contracts.ps1")

$repoRoot = Resolve-RepoRoot
$scanFiles = @(Get-ScanFiles -RepoRoot $repoRoot -InputPaths $Path)
if ($scanFiles.Count -eq 0) {
    Write-Step "No text-like files matched the requested paths."
    exit 0
}

$rules = @(
    (New-Rule `
        -Label "draft wording" `
        -Pattern '(?i)发布说明草稿|请在发布前补齐|草稿|release body draft|release-note drafts|release drafts|public release drafts|draft release notes|still drafting')
    (New-Rule `
        -Label "local absolute path" `
        -Pattern '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+|(?<!\w)/(?:Users|home)/[^\s"''`<>|]+')
)

foreach ($extraPattern in $AdditionalForbiddenPattern) {
    if (-not [string]::IsNullOrWhiteSpace($extraPattern)) {
        $rules += New-Rule -Label "custom forbidden pattern" -Pattern $extraPattern
    }
}

$violations = [System.Collections.Generic.List[object]]::new()
foreach ($file in $scanFiles) {
    foreach ($rule in $rules) {
        $matches = Select-String -LiteralPath $file -Pattern $rule.pattern -AllMatches
        foreach ($match in $matches) {
            [void]$violations.Add([ordered]@{
                file = $file
                line = $match.LineNumber
                label = $rule.label
                text = $match.Line.Trim()
            })
        }
    }

    if ([System.IO.Path]::GetExtension($file).ToLowerInvariant() -eq ".md") {
        $content = Get-Content -Raw -LiteralPath $file
        Add-ReleaseEntryDocumentGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseEntryProjectTemplateReadinessChecklistEntrypointsEvidenceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseEntryProjectTemplateWorkflowDashboardTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseEntryWordVisualStandardReviewMetadataEvidenceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseMetadataProjectTemplateReadinessChecklistEntrypointsTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseMetadataProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseEntryPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseSummaryProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseSummaryPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBodyProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBodyPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBlockerRollupPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBlockerRollupPdfVisualGateAttemptTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBlockerRollupPdfVisualSegmentedGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBlockerRollupManifestSignoffEntrypointsTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBlockerRollupProjectTemplateReadinessChecklistEntrypointsTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseBlockerRollupProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseHandoffProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseHandoffPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffPdfVisualGateAttemptTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffPdfVisualSegmentedGateTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffManifestSignoffEntrypointsTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistEntrypointsTraceViolations -File $file -Content $content -Violations $violations
        Add-ReleaseGovernanceHandoffProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewProjectTemplateReadinessChecklistEntrypointsTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewProjectTemplateReadinessChecklistMaterialSafetyAuditTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewProjectTemplateGovernanceTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewPdfVisualGateTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewPdfVisualGateAttemptTraceViolations -File $file -Content $content -Violations $violations
        Add-FinalReviewPdfVisualSegmentedGateTraceViolations -File $file -Content $content -Violations $violations
    }

    if ([System.IO.Path]::GetExtension($file).ToLowerInvariant() -eq ".json") {
        $leafName = (Split-Path -Leaf $file).ToLowerInvariant()
        try {
            $json = Get-Content -Raw -LiteralPath $file | ConvertFrom-Json
            Add-GovernanceMetricContractViolations -File $file -Json $json -Violations $violations
            Add-ContentControlRepairContractViolations -File $file -Json $json -Violations $violations
            Add-ProjectTemplateDeliveryReadinessContractViolations -File $file -Json $json -Violations $violations
            Add-ProjectTemplateOnboardingGovernanceContractViolations -File $file -Json $json -Violations $violations
            Add-ManifestSignoffEntrypointsContractViolations -File $file -Json $json -Violations $violations
            Add-ProjectTemplateReadinessChecklistEntrypointsContractViolations -File $file -Json $json -Violations $violations
            Add-ReleaseNoteBundleContractViolations -File $file -Json $json -Violations $violations
            Add-ReleaseEntryProjectTemplateReadinessChecklistMaterialSafetyAuditContractViolations -File $file -Json $json -Violations $violations
            Add-PdfVisualGateManifestContractViolations -File $file -Json $json -Violations $violations
            Add-WordVisualStandardReviewManifestContractViolations -File $file -Json $json -Violations $violations
            Add-ReleaseUploadRemoteAssetsContractViolations -File $file -Json $json -Violations $violations
        } catch {
            if ($leafName -eq "summary.json" -or $leafName -eq "release_assets_manifest.json") {
                Add-AuditViolation `
                    -Violations $violations `
                    -File $file `
                    -Label "governance metrics contract" `
                    -Text "Release governance JSON could not be parsed: $($_.Exception.Message)"
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Step "Detected forbidden release material content:"
    foreach ($violation in $violations) {
        Write-Host ("- [{0}] {1}:{2}" -f $violation.label, $violation.file, $violation.line)
        Write-Host ("  {0}" -f $violation.text)
    }

    throw "Release material safety audit failed with $($violations.Count) violation(s)."
}

Write-Step ("Audit passed for {0} file(s)." -f $scanFiles.Count)
