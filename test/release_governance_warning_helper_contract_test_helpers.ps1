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

function Assert-DoesNotContainText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Message
    )

    if (-not [string]::IsNullOrWhiteSpace($UnexpectedText) -and
        $Text -match [regex]::Escape($UnexpectedText)) {
        throw $Message
    }
}

function Assert-ThrowsContains {
    param(
        [scriptblock]$ScriptBlock,
        [string]$ExpectedText,
        [string]$Message
    )

    $failed = $false
    $actualMessage = ""
    try {
        & $ScriptBlock
    } catch {
        $failed = $true
        $actualMessage = $_.Exception.Message
    }

    if (-not $failed) {
        throw "$Message Expected the script block to throw."
    }
    if ($actualMessage -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Expected error to contain '$ExpectedText'. Actual error: $actualMessage"
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


$warningWithStyleMergeCount = [pscustomobject]@{
    id = "document_skeleton.style_merge_suggestions_pending"
    action = "review_style_merge_suggestions"
    message = "Document skeleton governance reports 2 duplicate style merge suggestion(s) awaiting review."
    project_id = "project-finance"
    template_name = "invoice-template"
    candidate_type = "rename"
    source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    style_merge_suggestion_count = 2
    style_merge_suggestion_pending_count = 2
}

$warningWithoutStyleMergeCount = [pscustomobject]@{
    id = "numbering_catalog.manifest_missing"
    action = "rebuild_numbering_catalog"
    message = "Numbering catalog report is missing source metadata."
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
    source_report_display = ".\output\numbering-catalog-governance\summary.json"
}

$restoreAuditActionItem = [pscustomobject]@{
    id = "review_style_merge_restore_audit"
    action = "review_style_merge_restore_audit"
    title = "Review style merge restore audit and Word render"
    project_id = "project-finance"
    template_name = "invoice-template"
    candidate_type = "rename"
    source_schema = "featherdoc.style_merge_restore_audit.v1"
    source_report_display = ".\output\document-skeleton-governance\style-merge.restore-audit.summary.json"
    source_json_display = ".\output\document-skeleton-governance\style-merge.restore-audit.json"
    issue_keys = @("word_visual_review_pending", "style_merge_restore_audit_pending")
    command = "pwsh -ExecutionPolicy Bypass -File .\scripts\prepare_word_review_task.ps1 -DocxPath output/document-skeleton-governance/merged-styles.docx -DocumentSourceKind style-merge-restore-audit -Mode review-only"
    open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\open_latest_word_review_task.ps1 -SourceKind style-merge-restore-audit -PrintPrompt"
    audit_command = "featherdoc_cli restore-style-merge merged-styles.docx --rollback-plan style-merge.apply.rollback.json --dry-run --json"
    repair_strategy = "review_style_merge_restore_audit"
    repair_hint = "Open the review task, confirm the restored style merge output in Word, then rerun the restore audit before release."
    command_template = "featherdoc_cli restore-style-merge <input.docx> --rollback-plan <rollback.json> --dry-run --json"
}

$styleNumberingBlocker = [pscustomobject]@{
    composite_id = "source0.blocker0.numbering_catalog_governance.style_numbering_issues"
    id = "numbering_catalog_governance.style_numbering_issues"
    action = "review_style_numbering_audit"
    project_id = "project-finance"
    template_name = "invoice-template"
    candidate_type = "rename"
    status = "blocked"
    severity = "error"
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
    source_report = "output/numbering-catalog-governance/summary.json"
    source_report_display = ".\output\numbering-catalog-governance\summary.json"
    source_json = "output/numbering-catalog-governance/style-numbering-audit.json"
    source_json_display = ".\output\numbering-catalog-governance\style-numbering-audit.json"
    message = "Style numbering audit reported unresolved issues."
    matched_document_count = 1
    unmatched_catalog_document_count = 1
    unmatched_baseline_document_count = 1
    alignment_gap_count = 2
    catalog_coverage_percent = 50
    baseline_coverage_percent = 50
    coverage_score = 50
    catalog_document_keys = @("contract", "invoice")
    baseline_document_keys = @("invoice", "obsolete")
    matched_document_keys = @("invoice")
}

$styleMergeBlocker = [pscustomobject]@{
    id = "style_merge.restore_audit_issues"
    action = "review_style_merge_restore_audit"
    status = "blocked"
    severity = "error"
    source_schema = "featherdoc.style_merge_restore_audit.v1"
    source_report = "output/document-skeleton-governance/style-merge.restore-audit.summary.json"
    source_report_display = ".\output\document-skeleton-governance\style-merge.restore-audit.summary.json"
    message = "Style merge restore audit has unresolved rollback issues."
}

$numberingGovernanceMetric = [pscustomobject]@{
    id = "numbering_catalog_governance.catalog_coverage"
    report_id = "numbering_catalog_governance"
    metric = "catalog_coverage"
    level = "warning"
    score = 72
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
    source_report = "output/numbering-catalog-governance/summary.json"
    source_report_display = ".\output\numbering-catalog-governance\summary.json"
    source_json = "output/numbering-catalog-governance/coverage.json"
    source_json_display = ".\output\numbering-catalog-governance\coverage.json"
    details = [pscustomobject]@{
        matched_document_count = 1
        unmatched_catalog_document_count = 1
        unmatched_baseline_document_count = 1
        alignment_gap_count = 2
        catalog_coverage_percent = 50
        baseline_coverage_percent = 50
        coverage_score = 50
        catalog_document_keys = @("contract", "invoice")
        baseline_document_keys = @("invoice", "obsolete")
        matched_document_keys = @("invoice")
    }
}

$sourceSchemaSummaryWarning = [pscustomobject]@{
    id = "document_skeleton.style_merge_suggestions_pending"
    action = "review_style_merge_suggestions"
    message = "Document skeleton governance reports duplicate style merge suggestion(s) awaiting review."
    source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    source_report_display = ".\output\document-skeleton-governance-rollup\summary.json"
    source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
}

$sourceSchemaSummaryFixture = [pscustomobject]@{
    release_blocker_rollup = [pscustomobject]@{
        release_blocker_count = 1
        release_blockers = @($styleNumberingBlocker)
        blocker_source_schema_summary = @(
            [pscustomobject]@{
                source_schema = "featherdoc.numbering_catalog_governance_report.v1"
                count = 1
            }
        )
        action_item_count = 1
        action_items = @($restoreAuditActionItem)
        action_item_source_schema_summary = @(
            [pscustomobject]@{
                source_schema = "featherdoc.style_merge_restore_audit.v1"
                count = 1
            }
        )
        informational_action_item_count = 0
        informational_action_items = @()
        informational_action_item_source_schema_summary = @()
        warning_count = 1
        warnings = @($sourceSchemaSummaryWarning)
        warning_source_schema_summary = @(
            [pscustomobject]@{
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                count = 1
            }
        )
    }
}

function Assert-ReleaseGovernanceWarningNormalizationContracts {
    [CmdletBinding()]
    param()

    Assert-ReleaseGovernanceReviewerMetadataQuality `
        -Summary $sourceSchemaSummaryFixture `
        -Context "source schema summary contract"

    $mismatchedSourceSchemaSummaryFixture = $sourceSchemaSummaryFixture | ConvertTo-Json -Depth 10 | ConvertFrom-Json
    @($mismatchedSourceSchemaSummaryFixture.release_blocker_rollup.blocker_source_schema_summary)[0].count = 2
    Assert-ThrowsContains `
        -ScriptBlock { Assert-ReleaseGovernanceReviewerMetadataQuality -Summary $mismatchedSourceSchemaSummaryFixture -Context "source schema summary contract" } `
        -ExpectedText "release_blocker_rollup.blocker_source_schema_summary count mismatch" `
        -Message "Reviewer metadata quality should reject source schema summary count mismatches."

    $orphanedSourceSchemaSummaryFixture = $sourceSchemaSummaryFixture | ConvertTo-Json -Depth 10 | ConvertFrom-Json
    $orphanedSourceSchemaSummaryFixture.release_blocker_rollup.blocker_source_schema_summary = @(
        [pscustomobject]@{
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            count = 1
        },
        [pscustomobject]@{
            source_schema = "featherdoc.unknown_source_schema.v1"
            count = 1
        }
    )
    Assert-ThrowsContains `
        -ScriptBlock { Assert-ReleaseGovernanceReviewerMetadataQuality -Summary $orphanedSourceSchemaSummaryFixture -Context "source schema summary contract" } `
        -ExpectedText "release_blocker_rollup.blocker_source_schema_summary has source_schema 'featherdoc.unknown_source_schema.v1' but release_blocker_rollup.release_blockers contains no matching item" `
        -Message "Reviewer metadata quality should reject source schema summaries that omit materialized item schemas."

    $missingSourceSchemaSummaryFixture = $sourceSchemaSummaryFixture | ConvertTo-Json -Depth 10 | ConvertFrom-Json
    $missingSourceSchemaSummaryFixture.release_blocker_rollup.blocker_source_schema_summary = @(
        [pscustomobject]@{
            count = 1
        }
    )
    Assert-ThrowsContains `
        -ScriptBlock { Assert-ReleaseGovernanceReviewerMetadataQuality -Summary $missingSourceSchemaSummaryFixture -Context "source schema summary contract" } `
        -ExpectedText "release_blocker_rollup.blocker_source_schema_summary[0].source_schema must not be empty" `
        -Message "Reviewer metadata quality should reject source schema summary groups without a schema."

    $missingCountSourceSchemaSummaryFixture = $sourceSchemaSummaryFixture | ConvertTo-Json -Depth 10 | ConvertFrom-Json
    $missingCountSourceSchemaSummaryFixture.release_blocker_rollup.blocker_source_schema_summary = @(
        [pscustomobject]@{
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        }
    )
    Assert-ThrowsContains `
        -ScriptBlock { Assert-ReleaseGovernanceReviewerMetadataQuality -Summary $missingCountSourceSchemaSummaryFixture -Context "source schema summary contract" } `
        -ExpectedText "release_blocker_rollup.blocker_source_schema_summary[0].count must not be empty" `
        -Message "Reviewer metadata quality should reject source schema summary groups without a count."

    $normalizedBlockers = @(Get-NormalizedReleaseGovernanceBlockers -Blockers @($styleNumberingBlocker, $styleMergeBlocker))
    Assert-Equal -Actual $normalizedBlockers.Count -Expected 2 `
        -Message "Normalized governance blockers should preserve every blocker entry."
    Assert-Equal -Actual ([string]$normalizedBlockers[0].composite_id) -Expected "source0.blocker0.numbering_catalog_governance.style_numbering_issues" `
        -Message "Normalized governance blocker should preserve composite id."
    Assert-Equal -Actual ([string]$normalizedBlockers[0].action) -Expected "review_style_numbering_audit" `
        -Message "Normalized governance blocker should preserve action."
    Assert-Equal -Actual ([string]$normalizedBlockers[0].project_id) -Expected "project-finance" `
        -Message "Normalized governance blocker should preserve project id."
    Assert-Equal -Actual ([string]$normalizedBlockers[0].template_name) -Expected "invoice-template" `
        -Message "Normalized governance blocker should preserve template name."
    Assert-Equal -Actual ([string]$normalizedBlockers[0].candidate_type) -Expected "rename" `
        -Message "Normalized governance blocker should preserve candidate type."
    Assert-Equal -Actual ([string]$normalizedBlockers[0].source_report_display) -Expected ".\output\numbering-catalog-governance\summary.json" `
        -Message "Normalized governance blocker should preserve source report display."
    Assert-Equal -Actual ([string]$normalizedBlockers[0].source_json_display) -Expected ".\output\numbering-catalog-governance\style-numbering-audit.json" `
        -Message "Normalized governance blocker should preserve source JSON display."
    Assert-Equal -Actual ([int]$normalizedBlockers[0].alignment_gap_count) -Expected 2 `
        -Message "Normalized governance blocker should preserve real-corpus alignment gap count."
    Assert-Equal -Actual ([int]$normalizedBlockers[0].catalog_coverage_percent) -Expected 50 `
        -Message "Normalized governance blocker should preserve catalog coverage percent."
    Assert-ContainsText -Text (($normalizedBlockers[0].catalog_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "contract" `
        -Message "Normalized governance blocker should preserve catalog document keys."
    Assert-ContainsText -Text (($normalizedBlockers[0].baseline_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "obsolete" `
        -Message "Normalized governance blocker should preserve baseline document keys."
    Assert-ContainsText -Text (($normalizedBlockers[0].matched_document_keys | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "invoice" `
        -Message "Normalized governance blocker should preserve matched document keys."
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
    Assert-Equal -Actual ([string]$normalizedWarnings[0].project_id) -Expected "project-finance" `
        -Message "Normalized warning should preserve project id."
    Assert-Equal -Actual ([string]$normalizedWarnings[0].template_name) -Expected "invoice-template" `
        -Message "Normalized warning should preserve template name."
    Assert-Equal -Actual ([string]$normalizedWarnings[0].candidate_type) -Expected "rename" `
        -Message "Normalized warning should preserve candidate type."
    Assert-Equal -Actual ([string]$normalizedWarnings[1].source_report_display) -Expected ".\output\numbering-catalog-governance\summary.json" `
        -Message "Normalized warning should preserve source report display."
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
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'project_id: `project-finance`' `
        -Message "Governance blocker summary should include project id."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'template_name: `invoice-template`' `
        -Message "Governance blocker summary should include template name."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'candidate_type: `rename`' `
        -Message "Governance blocker summary should include candidate type."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'status: `blocked`' `
        -Message "Governance blocker summary should include status."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'severity: `error`' `
        -Message "Governance blocker summary should include severity."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'source_schema: `featherdoc.numbering_catalog_governance_report.v1`' `
        -Message "Governance blocker summary should include source schema."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'source_report_display: `.\output\numbering-catalog-governance\summary.json`' `
        -Message "Governance blocker summary should include source report display."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'source_json_display: `.\output\numbering-catalog-governance\style-numbering-audit.json`' `
        -Message "Governance blocker summary should include source JSON display."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'message: Style numbering audit reported unresolved issues.' `
        -Message "Governance blocker summary should include message."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'matched_document_count: `1`' `
        -Message "Governance blocker summary should include matched document count."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'alignment_gap_count: `2`' `
        -Message "Governance blocker summary should include alignment gap count."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'catalog_coverage_percent: `50`' `
        -Message "Governance blocker summary should include catalog coverage percent."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'catalog_document_keys: `contract,invoice`' `
        -Message "Governance blocker summary should include catalog document keys."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'baseline_document_keys: `invoice,obsolete`' `
        -Message "Governance blocker summary should include baseline document keys."
    Assert-ContainsText -Text $blockerSummaryText -ExpectedText 'matched_document_keys: `invoice`' `
        -Message "Governance blocker summary should include matched document keys."

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
    Assert-Equal -Actual ([string]$normalizedActionItems[0].project_id) -Expected "project-finance" `
        -Message "Normalized action item should preserve project id."
    Assert-Equal -Actual ([string]$normalizedActionItems[0].template_name) -Expected "invoice-template" `
        -Message "Normalized action item should preserve template name."
    Assert-Equal -Actual ([string]$normalizedActionItems[0].candidate_type) -Expected "rename" `
        -Message "Normalized action item should preserve candidate type."
    Assert-Equal -Actual ([string]$normalizedActionItems[0].source_json_display) -Expected ".\output\document-skeleton-governance\style-merge.restore-audit.json" `
        -Message "Normalized action item should preserve source JSON display."
    Assert-ContainsText -Text ([string]$normalizedActionItems[0].audit_command) -ExpectedText "restore-style-merge" `
        -Message "Normalized action item should preserve audit commands."
    Assert-Equal -Actual ([string]$normalizedActionItems[0].repair_strategy) -Expected "review_style_merge_restore_audit" `
        -Message "Normalized action item should preserve repair strategy."
    Assert-ContainsText -Text ([string]$normalizedActionItems[0].repair_hint) -ExpectedText "confirm the restored style merge output" `
        -Message "Normalized action item should preserve repair hints."
    Assert-ContainsText -Text ([string]$normalizedActionItems[0].command_template) -ExpectedText "restore-style-merge <input.docx>" `
        -Message "Normalized action item should preserve command templates."
    $actionFallbackItem = [pscustomobject]@{
        id = "review_missing_action_metadata"
        title = "Review action metadata"
        source_schema = "featherdoc.test.v1"
        source_report_display = ".\output\release-candidate-checks\summary.json"
        source_json_display = ".\output\release-candidate-checks\summary.json"
        open_command = "pwsh -File .\scripts\review-action-metadata.ps1"
    }
    $normalizedFallbackAction = @(Get-NormalizedReleaseGovernanceActionItems -ActionItems @($actionFallbackItem))[0]
    Assert-Equal -Actual ([string]$normalizedFallbackAction.action) -Expected "review_missing_action_metadata" `
        -Message "Normalized action item should fall back to id when action is absent."
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
    Assert-ContainsText -Text $richSummaryText -ExpectedText 'project_id: `project-finance`' `
        -Message "Summary text should include warning project id."
    Assert-ContainsText -Text $richSummaryText -ExpectedText 'template_name: `invoice-template`' `
        -Message "Summary text should include warning template name."
    Assert-ContainsText -Text $richSummaryText -ExpectedText 'candidate_type: `rename`' `
        -Message "Summary text should include warning candidate type."
    Assert-ContainsText -Text $richSummaryText -ExpectedText 'style_merge_suggestion_count: `2`' `
        -Message "Summary text should include the optional style merge count when present."
    Assert-ContainsText -Text $richSummaryText -ExpectedText 'style_merge_suggestion_pending_count: `2`' `
        -Message "Summary text should include the optional pending style merge count when present."

    $plainSummaryText = Get-ReleaseGovernanceWarningSummaryText -Warning $warningWithoutStyleMergeCount
    Assert-True -Condition ($plainSummaryText -notmatch 'style_merge_suggestion_count') `
        -Message "Summary text should omit the optional style merge count when absent."
    Assert-True -Condition ($plainSummaryText -notmatch 'style_merge_suggestion_pending_count') `
        -Message "Summary text should omit the optional pending style merge count when absent."
    Assert-ContainsText -Text $plainSummaryText -ExpectedText 'source_report: `.\output\numbering-catalog-governance\summary.json`' `
        -Message "Summary text should include warning source report display."

    $actionSummaryText = Get-ReleaseGovernanceActionItemSummaryText -ActionItem $restoreAuditActionItem
    Assert-ContainsText -Text $actionSummaryText -ExpectedText 'id: `review_style_merge_restore_audit`' `
        -Message "Action item summary should include the action item id."
    Assert-ContainsText -Text $actionSummaryText -ExpectedText 'action: `review_style_merge_restore_audit`' `
        -Message "Action item summary should include the action."
    Assert-ContainsText -Text $actionSummaryText -ExpectedText 'source_schema: `featherdoc.style_merge_restore_audit.v1`' `
        -Message "Action item summary should include source schema."
    Assert-ContainsText -Text $actionSummaryText -ExpectedText 'project_id: `project-finance`' `
        -Message "Action item summary should include project id."
    Assert-ContainsText -Text $actionSummaryText -ExpectedText 'template_name: `invoice-template`' `
        -Message "Action item summary should include template name."
    Assert-ContainsText -Text $actionSummaryText -ExpectedText 'candidate_type: `rename`' `
        -Message "Action item summary should include candidate type."

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
    Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText 'project_id: `project-finance`' `
        -Message "Action item Markdown subsection should include project id."
    Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText 'repair_strategy: review_style_merge_restore_audit' `
        -Message "Action item Markdown subsection should include repair strategy."
    Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText 'repair_hint: Open the review task, confirm the restored style merge output in Word, then rerun the restore audit before release.' `
        -Message "Action item Markdown subsection should include repair hint."
    Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText 'command_template: featherdoc_cli restore-style-merge <input.docx> --rollback-plan <rollback.json> --dry-run --json' `
        -Message "Action item Markdown subsection should include command template."
    Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText 'template_name: `invoice-template`' `
        -Message "Action item Markdown subsection should include template name."
    Assert-ContainsText -Text $actionSubsectionMarkdown -ExpectedText 'candidate_type: `rename`' `
        -Message "Action item Markdown subsection should include candidate type."
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
    Assert-ContainsText -Text $blockerSubsectionMarkdown -ExpectedText 'project_id: `project-finance`' `
        -Message "Blocker Markdown subsection should include project id."
    Assert-ContainsText -Text $blockerSubsectionMarkdown -ExpectedText 'template_name: `invoice-template`' `
        -Message "Blocker Markdown subsection should include template name."
    Assert-ContainsText -Text $blockerSubsectionMarkdown -ExpectedText 'candidate_type: `rename`' `
        -Message "Blocker Markdown subsection should include candidate type."

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
    Assert-ContainsText -Text $subsectionMarkdown -ExpectedText 'project_id: `project-finance`' `
        -Message "Warning Markdown subsection should include project id."
    Assert-ContainsText -Text $subsectionMarkdown -ExpectedText 'template_name: `invoice-template`' `
        -Message "Warning Markdown subsection should include template name."
    Assert-ContainsText -Text $subsectionMarkdown -ExpectedText 'candidate_type: `rename`' `
        -Message "Warning Markdown subsection should include candidate type."

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
}
