param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

function Assert-ScriptParses {
    param([string]$Path)

    $parseTokens = $null
    $parseErrors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$parseTokens, [ref]$parseErrors) | Out-Null
    if ($parseErrors.Count -gt 0) {
        throw "PowerShell script has parse errors: $Path"
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$helpersPath = Join-Path $resolvedRepoRoot "scripts\release_blocker_metadata_helpers.ps1"
Assert-ScriptParses -Path $helpersPath
. $helpersPath

$warningWithStyleMergeCount = [pscustomobject]@{
    id = "document_skeleton.style_merge_suggestions_pending"
    action = "review_style_merge_suggestions"
    message = "Document skeleton governance reports 2 duplicate style merge suggestion(s) awaiting review."
    source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    style_merge_suggestion_count = 2
    style_merge_suggestion_pending_count = 2
}

$warningWithoutStyleMergeCount = [pscustomobject]@{
    id = "numbering_catalog.manifest_missing"
    action = "rebuild_numbering_catalog"
    message = "Numbering catalog report is missing source metadata."
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
}

$normalizedWarnings = @(Get-NormalizedReleaseGovernanceWarnings -Warnings @($warningWithStyleMergeCount, $warningWithoutStyleMergeCount))
Assert-Equal -Actual $normalizedWarnings.Count -Expected 2 `
    -Message "Normalized warnings should preserve every warning entry."
Assert-Equal -Actual ([string]$normalizedWarnings[0].id) -Expected "document_skeleton.style_merge_suggestions_pending" `
    -Message "Normalized warning should preserve id."
Assert-Equal -Actual ([string]$normalizedWarnings[0].action) -Expected "review_style_merge_suggestions" `
    -Message "Normalized warning should preserve action."
Assert-Equal -Actual ([string]$normalizedWarnings[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
    -Message "Normalized warning should preserve source schema."
Assert-True -Condition ($normalizedWarnings[0].PSObject.Properties.Name -contains "style_merge_suggestion_count") `
    -Message "Normalized warning should preserve style merge counts when provided."
Assert-True -Condition ($normalizedWarnings[0].PSObject.Properties.Name -contains "style_merge_suggestion_pending_count") `
    -Message "Normalized warning should preserve pending style merge counts when provided."
Assert-True -Condition (-not ($normalizedWarnings[1].PSObject.Properties.Name -contains "style_merge_suggestion_count")) `
    -Message "Normalized warning should omit style merge counts when absent."
Assert-True -Condition (-not ($normalizedWarnings[1].PSObject.Properties.Name -contains "style_merge_suggestion_pending_count")) `
    -Message "Normalized warning should omit pending style merge counts when absent."

$declaredCountSummary = [pscustomobject]@{
    warning_count = 5
    warnings = @($warningWithStyleMergeCount)
}
$fallbackCountSummary = [pscustomobject]@{
    warnings = @($warningWithStyleMergeCount, $warningWithoutStyleMergeCount)
}
Assert-Equal -Actual (Get-ReleaseGovernanceWarningCount -SummaryObject $declaredCountSummary) -Expected 5 `
    -Message "Declared warning_count should win over materialized warnings."
Assert-Equal -Actual (Get-ReleaseGovernanceWarningCount -SummaryObject $fallbackCountSummary) -Expected 2 `
    -Message "Missing warning_count should fall back to materialized warnings."

$richSummaryText = Get-ReleaseGovernanceWarningSummaryText -Warning $warningWithStyleMergeCount
Assert-ContainsText -Text $richSummaryText -ExpectedText 'id: `document_skeleton.style_merge_suggestions_pending`' `
    -Message "Summary text should include the warning id."
Assert-ContainsText -Text $richSummaryText -ExpectedText 'action: `review_style_merge_suggestions`' `
    -Message "Summary text should include the warning action."
Assert-ContainsText -Text $richSummaryText -ExpectedText 'message: Document skeleton governance reports 2 duplicate style merge suggestion(s) awaiting review.' `
    -Message "Summary text should include the warning message."
Assert-ContainsText -Text $richSummaryText -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' `
    -Message "Summary text should include the warning source schema."
Assert-ContainsText -Text $richSummaryText -ExpectedText 'style_merge_suggestion_count: `2`' `
    -Message "Summary text should include the optional style merge count when present."
Assert-ContainsText -Text $richSummaryText -ExpectedText 'style_merge_suggestion_pending_count: `2`' `
    -Message "Summary text should include the optional pending style merge count when present."

$plainSummaryText = Get-ReleaseGovernanceWarningSummaryText -Warning $warningWithoutStyleMergeCount
Assert-True -Condition ($plainSummaryText -notmatch 'style_merge_suggestion_count') `
    -Message "Summary text should omit the optional style merge count when absent."
Assert-True -Condition ($plainSummaryText -notmatch 'style_merge_suggestion_pending_count') `
    -Message "Summary text should omit the optional pending style merge count when absent."

$subsectionLines = New-Object 'System.Collections.Generic.List[string]'
$subsectionRendered = Add-ReleaseGovernanceWarningMarkdownSubsection `
    -Lines $subsectionLines `
    -Heading "Release blocker rollup warnings" `
    -SummaryObject ([pscustomobject]@{
        warning_count = 2
        warnings = @($warningWithStyleMergeCount, $warningWithoutStyleMergeCount)
    })
Assert-True -Condition $subsectionRendered `
    -Message "Markdown subsection should render when warnings exist."
$subsectionMarkdown = $subsectionLines -join "`n"
Assert-ContainsText -Text $subsectionMarkdown -ExpectedText "### Release blocker rollup warnings" `
    -Message "Markdown subsection should include its heading."
Assert-ContainsText -Text $subsectionMarkdown -ExpectedText '- warning_count: `2`' `
    -Message "Markdown subsection should include the warning count."
Assert-ContainsText -Text $subsectionMarkdown -ExpectedText 'style_merge_suggestion_count: `2`' `
    -Message "Markdown subsection should include the optional style merge count."
Assert-ContainsText -Text $subsectionMarkdown -ExpectedText 'style_merge_suggestion_pending_count: `2`' `
    -Message "Markdown subsection should include the optional pending style merge count."

$emptyLines = New-Object 'System.Collections.Generic.List[string]'
$emptyRendered = Add-ReleaseGovernanceWarningMarkdownSubsection `
    -Lines $emptyLines `
    -Heading "Empty warnings" `
    -SummaryObject ([pscustomobject]@{
        warnings = @()
    })
Assert-True -Condition (-not $emptyRendered) `
    -Message "Markdown subsection should skip empty warning groups."
Assert-Equal -Actual $emptyLines.Count -Expected 0 `
    -Message "Skipped markdown subsection should not append output."

$sectionLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceWarningsMarkdownSection `
    -Lines $sectionLines `
    -Summary ([pscustomobject]@{
        release_blocker_rollup = [pscustomobject]@{
            warning_count = 1
            warnings = @($warningWithStyleMergeCount)
        }
        release_governance_handoff = [pscustomobject]@{
            warning_count = 1
            warnings = @($warningWithoutStyleMergeCount)
            release_blocker_rollup = [pscustomobject]@{
                warning_count = 1
                warnings = @($warningWithStyleMergeCount)
            }
        }
    })
$sectionMarkdown = $sectionLines -join "`n"
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "## Release Governance Warnings" `
    -Message "Markdown section should include the top-level warning heading."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "### Release blocker rollup warnings" `
    -Message "Markdown section should include the release blocker subsection."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "### Release governance handoff warnings" `
    -Message "Markdown section should include the handoff subsection."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "### Release governance handoff nested rollup warnings" `
    -Message "Markdown section should include the nested handoff rollup subsection."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText "Numbering catalog report is missing source metadata." `
    -Message "Markdown section should render plain warning messages."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText 'style_merge_suggestion_count: `2`' `
    -Message "Markdown section should render optional style merge counts."
Assert-ContainsText -Text $sectionMarkdown -ExpectedText 'style_merge_suggestion_pending_count: `2`' `
    -Message "Markdown section should render optional pending style merge counts."

$checklistItems = @(Get-ReleaseGovernanceWarningChecklistItems -Summary ([pscustomobject]@{
            release_blocker_rollup = [pscustomobject]@{
                warning_count = 1
                warnings = @($warningWithStyleMergeCount)
            }
            release_governance_handoff = [pscustomobject]@{
                warning_count = 1
                warnings = @($warningWithoutStyleMergeCount)
                release_blocker_rollup = [pscustomobject]@{
                    warning_count = 1
                    warnings = @($warningWithStyleMergeCount)
                }
            }
        }))
Assert-Equal -Actual $checklistItems.Count -Expected 3 `
    -Message "Checklist items should preserve warnings from every release governance source."
$checklistText = Get-ReleaseGovernanceWarningChecklistText -WarningItem $checklistItems[0]
Assert-ContainsText -Text $checklistText -ExpectedText 'Review release governance warning `document_skeleton.style_merge_suggestions_pending`' `
    -Message "Checklist text should include warning id."
Assert-ContainsText -Text $checklistText -ExpectedText 'action `review_style_merge_suggestions`' `
    -Message "Checklist text should include warning action."
Assert-ContainsText -Text $checklistText -ExpectedText 'source_schema `featherdoc.document_skeleton_governance_rollup_report.v1`' `
    -Message "Checklist text should include warning source schema."

$styleMergeGuidance = (Get-ReleaseGovernanceWarningActionGuidanceLines `
        -Warning $warningWithStyleMergeCount `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "summary.json")) -join "`n"
Assert-ContainsText -Text $styleMergeGuidance -ExpectedText 'Use action `review_style_merge_suggestions`' `
    -Message "Style-merge warning guidance should mention its action."
Assert-ContainsText -Text $styleMergeGuidance -ExpectedText 'Current pending style merge suggestion count is `2`' `
    -Message "Style-merge warning guidance should preserve pending suggestion count."

$plainGuidance = (Get-ReleaseGovernanceWarningActionGuidanceLines `
        -Warning $warningWithoutStyleMergeCount `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "summary.json")) -join "`n"
Assert-ContainsText -Text $plainGuidance -ExpectedText 'Follow warning action `rebuild_numbering_catalog`' `
    -Message "Generic warning guidance should include the warning action."
Assert-ContainsText -Text $plainGuidance -ExpectedText 'source_schema `featherdoc.numbering_catalog_governance_report.v1`' `
    -Message "Generic warning guidance should include the warning source schema."

Write-Host "Release governance warning helper regression passed."
