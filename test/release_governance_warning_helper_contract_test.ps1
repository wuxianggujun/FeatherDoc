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

$restoreAuditActionItem = [pscustomobject]@{
    id = "review_style_merge_restore_audit"
    action = "review_style_merge_restore_audit"
    title = "Review style merge restore audit and Word render"
    source_schema = "featherdoc.style_merge_restore_audit.v1"
    source_report_display = ".\output\document-skeleton-governance\style-merge.restore-audit.summary.json"
    command = "pwsh -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 -DocxPath output/document-skeleton-governance/merged-styles.docx -DocumentSourceKind style-merge-restore-audit -Mode review-only"
    open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit -PrintPrompt"
    audit_command = "featherdoc_cli restore-style-merge merged-styles.docx --rollback-plan style-merge.apply.rollback.json --dry-run --json"
}

$styleNumberingBlocker = [pscustomobject]@{
    composite_id = "source0.blocker0.numbering_catalog_governance.style_numbering_issues"
    id = "numbering_catalog_governance.style_numbering_issues"
    action = "review_style_numbering_audit"
    status = "blocked"
    severity = "error"
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
    source_report_display = ".\output\numbering-catalog-governance\summary.json"
    message = "Style numbering audit reported unresolved issues."
}

$styleMergeBlocker = [pscustomobject]@{
    id = "style_merge.restore_audit_issues"
    action = "review_style_merge_restore_audit"
    status = "blocked"
    severity = "error"
    source_schema = "featherdoc.style_merge_restore_audit.v1"
    source_report_display = ".\output\document-skeleton-governance\style-merge.restore-audit.summary.json"
    message = "Style merge restore audit has unresolved rollback issues."
}

$normalizedBlockers = @(Get-NormalizedReleaseGovernanceBlockers -Blockers @($styleNumberingBlocker, $styleMergeBlocker))
Assert-Equal -Actual $normalizedBlockers.Count -Expected 2 `
    -Message "Normalized governance blockers should preserve every blocker entry."
Assert-Equal -Actual ([string]$normalizedBlockers[0].composite_id) -Expected "source0.blocker0.numbering_catalog_governance.style_numbering_issues" `
    -Message "Normalized governance blocker should preserve composite id."
Assert-Equal -Actual ([string]$normalizedBlockers[0].action) -Expected "review_style_numbering_audit" `
    -Message "Normalized governance blocker should preserve action."
Assert-Equal -Actual ([string]$normalizedBlockers[0].source_report_display) -Expected ".\output\numbering-catalog-governance\summary.json" `
    -Message "Normalized governance blocker should preserve source report display."
Assert-Equal -Actual (Get-ReleaseGovernanceBlockerCount -SummaryObject ([pscustomobject]@{ release_blocker_count = 5; release_blockers = @($styleNumberingBlocker) })) -Expected 5 `
    -Message "Declared release_blocker_count should win over materialized governance blockers."

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

$blockerSummaryText = Get-ReleaseGovernanceBlockerSummaryText -Blocker $styleNumberingBlocker
Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'id: `numbering_catalog_governance.style_numbering_issues`' `
    -Message "Governance blocker summary should include the source blocker id."
Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'composite_id: `source0.blocker0.numbering_catalog_governance.style_numbering_issues`' `
    -Message "Governance blocker summary should include the rollup composite id when present."
Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'action: `review_style_numbering_audit`' `
    -Message "Governance blocker summary should include the action."
Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'status: `blocked`' `
    -Message "Governance blocker summary should include status."
Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'severity: `error`' `
    -Message "Governance blocker summary should include severity."
Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'source_schema: `featherdoc.numbering_catalog_governance_report.v1`' `
    -Message "Governance blocker summary should include source schema."
Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'source_report_display: `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Governance blocker summary should include source report display."
Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'message: Style numbering audit reported unresolved issues.' `
    -Message "Governance blocker summary should include message."

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

$normalizedActionItems = @(Get-NormalizedReleaseGovernanceActionItems -ActionItems @($restoreAuditActionItem))
Assert-Equal -Actual $normalizedActionItems.Count -Expected 1 `
    -Message "Normalized action items should preserve action entries."
Assert-Equal -Actual ([string]$normalizedActionItems[0].id) -Expected "review_style_merge_restore_audit" `
    -Message "Normalized action item should preserve id."
Assert-ContainsText -Text ([string]$normalizedActionItems[0].open_command) -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Normalized action item should preserve open-latest commands."
Assert-ContainsText -Text ([string]$normalizedActionItems[0].audit_command) -ExpectedText "restore-style-merge" `
    -Message "Normalized action item should preserve audit commands."
Assert-Equal -Actual (Get-ReleaseGovernanceActionItemCount -SummaryObject ([pscustomobject]@{ action_item_count = 7; action_items = @($restoreAuditActionItem) })) -Expected 7 `
    -Message "Declared action_item_count should win over materialized action items."

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

$actionSummaryText = Get-ReleaseGovernanceActionItemSummaryText -ActionItem $restoreAuditActionItem
Assert-ContainsText -Text $actionSummaryText -ExpectedText 'id: `review_style_merge_restore_audit`' `
    -Message "Action item summary should include the action item id."
Assert-ContainsText -Text $actionSummaryText -ExpectedText 'action: `review_style_merge_restore_audit`' `
    -Message "Action item summary should include the action."
Assert-ContainsText -Text $actionSummaryText -ExpectedText 'source_schema: `featherdoc.style_merge_restore_audit.v1`' `
    -Message "Action item summary should include source schema."

$actionSubsectionLines = New-Object 'System.Collections.Generic.List[string]'
$actionSubsectionRendered = Add-ReleaseGovernanceActionItemMarkdownSubsection `
    -Lines $actionSubsectionLines `
    -Heading "Release blocker rollup action items" `
    -SummaryObject ([pscustomobject]@{
        action_item_count = 1
        action_items = @($restoreAuditActionItem)
    })
Assert-True -Condition $actionSubsectionRendered `
    -Message "Action item Markdown subsection should render when action items exist."
$actionSubsectionMarkdown = $actionSubsectionLines -join "`n"
Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText "### Release blocker rollup action items" `
    -Message "Action item Markdown subsection should include its heading."
Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText '- action_item_count: `1`' `
    -Message "Action item Markdown subsection should include action count."
Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText 'open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit' `
    -Message "Action item Markdown subsection should include open-latest commands."
Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText 'featherdoc_cli restore-style-merge merged-styles.docx --rollback-plan style-merge.apply.rollback.json --dry-run --json' `
    -Message "Action item Markdown subsection should include audit commands."

$blockerSubsectionLines = New-Object 'System.Collections.Generic.List[string]'
$blockerSubsectionRendered = Add-ReleaseGovernanceBlockerMarkdownSubsection `
    -Lines $blockerSubsectionLines `
    -Heading "Release blocker rollup blockers" `
    -SummaryObject ([pscustomobject]@{
        release_blocker_count = 2
        release_blockers = @($styleNumberingBlocker, $styleMergeBlocker)
    })
Assert-True -Condition $blockerSubsectionRendered `
    -Message "Blocker Markdown subsection should render when governance blockers exist."
$blockerSubsectionMarkdown = $blockerSubsectionLines -join "`n"
Assert-ContainsText -Text $blockerSubsectionMarkdown -ExpectedText "### Release blocker rollup blockers" `
    -Message "Blocker Markdown subsection should include its heading."
Assert-ContainsText -Text $blockerSubsectionMarkdown -ExpectedText '- release_blocker_count: `2`' `
    -Message "Blocker Markdown subsection should include blocker count."
Assert-ContainsText -Text $blockerSubsectionMarkdown -ExpectedText 'id: `numbering_catalog_governance.style_numbering_issues`' `
    -Message "Blocker Markdown subsection should include source blocker ids."
Assert-ContainsText -Text $blockerSubsectionMarkdown -ExpectedText 'composite_id: `source0.blocker0.numbering_catalog_governance.style_numbering_issues`' `
    -Message "Blocker Markdown subsection should include rollup composite blocker ids."
Assert-ContainsText -Text $blockerSubsectionMarkdown -ExpectedText 'action: `review_style_merge_restore_audit`' `
    -Message "Blocker Markdown subsection should include fallback blocker action."

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

$blockerSectionLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceBlockersMarkdownSection `
    -Lines $blockerSectionLines `
    -Summary ([pscustomobject]@{
        release_blocker_rollup = [pscustomobject]@{
            release_blocker_count = 1
            release_blockers = @($styleNumberingBlocker)
        }
        release_governance_handoff = [pscustomobject]@{
            release_blocker_count = 1
            release_blockers = @($styleMergeBlocker)
            release_blocker_rollup = [pscustomobject]@{
                release_blocker_count = 1
                release_blockers = @($styleNumberingBlocker)
            }
        }
    })
$blockerSectionMarkdown = $blockerSectionLines -join "`n"
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText "## Release Governance Blockers" `
    -Message "Markdown section should include the top-level governance blocker heading."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText "### Release blocker rollup blockers" `
    -Message "Markdown section should include the release blocker rollup subsection."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText "### Release governance handoff blockers" `
    -Message "Markdown section should include the handoff blocker subsection."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText "### Release governance handoff nested rollup blockers" `
    -Message "Markdown section should include the nested handoff rollup blocker subsection."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText 'action: `review_style_numbering_audit`' `
    -Message "Markdown section should render rollup blocker actions."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText 'source_report_display: `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Markdown section should render blocker source report displays."
Assert-ContainsText -Text $blockerSectionMarkdown -ExpectedText 'id: `style_merge.restore_audit_issues`' `
    -Message "Markdown section should render handoff blocker ids when composite ids are absent."

$blockerChecklistItems = @(Get-ReleaseGovernanceBlockerChecklistItems -Summary ([pscustomobject]@{
            release_blocker_rollup = [pscustomobject]@{
                release_blocker_count = 1
                release_blockers = @($styleNumberingBlocker)
            }
            release_governance_handoff = [pscustomobject]@{
                release_blocker_count = 1
                release_blockers = @($styleMergeBlocker)
                release_blocker_rollup = [pscustomobject]@{
                    release_blocker_count = 1
                    release_blockers = @($styleNumberingBlocker)
                }
            }
        }))
Assert-Equal -Actual $blockerChecklistItems.Count -Expected 3 `
    -Message "Checklist items should preserve blockers from every release governance source."
$blockerChecklistText = Get-ReleaseGovernanceBlockerChecklistText -BlockerItem $blockerChecklistItems[0]
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'Resolve release governance blocker `numbering_catalog_governance.style_numbering_issues`' `
    -Message "Blocker checklist text should include blocker id."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'action `review_style_numbering_audit`' `
    -Message "Blocker checklist text should include blocker action."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'source_schema `featherdoc.numbering_catalog_governance_report.v1`' `
    -Message "Blocker checklist text should include source schema."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'composite_id `source0.blocker0.numbering_catalog_governance.style_numbering_issues`' `
    -Message "Blocker checklist text should include rollup composite id."
Assert-ContainsText -Text $blockerChecklistText -ExpectedText 'source_report `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Blocker checklist text should include source report display."

$blockerGuidance = (Get-ReleaseGovernanceBlockerActionGuidanceLines `
        -Blocker $styleNumberingBlocker `
        -RepoRoot $resolvedRepoRoot `
        -ReleaseSummaryJson (Join-Path $resolvedWorkingDir "summary.json")) -join "`n"
Assert-ContainsText -Text $blockerGuidance -ExpectedText 'Open source report `.\output\numbering-catalog-governance\summary.json`' `
    -Message "Blocker guidance should point reviewers at the source report."
Assert-ContainsText -Text $blockerGuidance -ExpectedText 'blocker action `review_style_numbering_audit`' `
    -Message "Blocker guidance should include the blocker action."

$actionSectionLines = New-Object 'System.Collections.Generic.List[string]'
Add-ReleaseGovernanceActionItemsMarkdownSection `
    -Lines $actionSectionLines `
    -Summary ([pscustomobject]@{
        release_blocker_rollup = [pscustomobject]@{
            action_item_count = 1
            action_items = @($restoreAuditActionItem)
        }
        release_governance_handoff = [pscustomobject]@{
            action_item_count = 1
            action_items = @($restoreAuditActionItem)
            release_blocker_rollup = [pscustomobject]@{
                action_item_count = 1
                action_items = @($restoreAuditActionItem)
            }
        }
    })
$actionSectionMarkdown = $actionSectionLines -join "`n"
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "## Release Governance Action Items" `
    -Message "Markdown section should include the top-level action item heading."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "### Release blocker rollup action items" `
    -Message "Markdown section should include the release blocker action subsection."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "### Release governance handoff action items" `
    -Message "Markdown section should include the handoff action subsection."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "### Release governance handoff nested rollup action items" `
    -Message "Markdown section should include the nested handoff action subsection."
Assert-ContainsText -Text $actionSectionMarkdown -ExpectedText "open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit" `
    -Message "Markdown section should render restore audit helper commands."

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
