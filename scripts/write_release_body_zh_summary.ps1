function Add-ProjectTemplateGovernanceContractSummaryLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary
    )

    $readinessReport = Get-FirstGovernanceReport `
        -Summary $Summary `
        -Id "project_template_delivery_readiness" `
        -Schema "featherdoc.project_template_delivery_readiness_report.v1"
    $onboardingReport = Get-FirstGovernanceReport `
        -Summary $Summary `
        -Id "" `
        -Schema "featherdoc.project_template_onboarding_governance_report.v1"

    if ($null -eq $readinessReport -and $null -eq $onboardingReport) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add("## Project template governance contracts")

    if ($null -ne $readinessReport) {
        $readinessParts = New-Object 'System.Collections.Generic.List[string]'
        foreach ($part in @(
                "project_template_delivery_readiness",
                "project_template_delivery_readiness_contract",
                "source_schema=$(Get-ReleaseBlockerDisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $readinessReport -Name "schema") -Fallback "featherdoc.project_template_delivery_readiness_report.v1")"
            )) {
            [void]$readinessParts.Add($part)
        }
        foreach ($fieldName in @(
                "status",
                "release_ready",
                "latest_schema_approval_gate_status",
                "schema_history_blocked_run_count",
                "schema_history_pending_run_count",
                "schema_history_passed_run_count",
                "onboarding_governance_next_action_group_count",
                "template_count",
                "ready_template_count",
                "blocked_template_count",
                "release_blocker_count",
                "action_item_count",
                "warning_count",
                "source_failure_count"
            )) {
            $fieldValue = Get-ReleaseBlockerPropertyValue -Object $readinessReport -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                [void]$readinessParts.Add("${fieldName}=$fieldValue")
            }

            if ($fieldName -eq "latest_schema_approval_gate_status") {
                $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
                    -Value (Get-ReleaseBlockerPropertyObject -Object $readinessReport -Name "schema_approval_status_summary")
                if (-not [string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
                    [void]$readinessParts.Add("schema_approval_status_summary=$schemaApprovalSummary")
                }
            }
        }
        $onboardingGovernanceNextAction = Format-ProjectTemplateNextActionSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $readinessReport -Name "onboarding_governance_next_action")
        if (-not [string]::IsNullOrWhiteSpace($onboardingGovernanceNextAction)) {
            [void]$readinessParts.Add("onboarding_governance_next_action=$onboardingGovernanceNextAction")
        }
        $onboardingGovernanceNextActionSummary = Format-ProjectTemplateNextActionSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $readinessReport -Name "onboarding_governance_next_action_summary")
        if (-not [string]::IsNullOrWhiteSpace($onboardingGovernanceNextActionSummary)) {
            [void]$readinessParts.Add("onboarding_governance_next_action_summary=$onboardingGovernanceNextActionSummary")
        }
        [void]$readinessParts.Add("source_report_display=$(Get-GovernanceSourceReportDisplay -Item $readinessReport)")
        [void]$readinessParts.Add("source_json_display=$(Get-GovernanceSourceJsonDisplay -Item $readinessReport)")
        [void]$Lines.Add("- Project template readiness: $($readinessParts -join ' ')")
    }

    if ($null -ne $onboardingReport) {
        $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $onboardingReport -Name "schema_approval_status_summary") `
            -Fallback (Get-ReleaseBlockerPropertyValue -Object $onboardingReport -Name "status")
        if ([string]::IsNullOrWhiteSpace($schemaApprovalSummary)) {
            $schemaApprovalSummary = "unknown"
        }

        $onboardingParts = New-Object 'System.Collections.Generic.List[string]'
        foreach ($part in @(
                "project_template_onboarding.schema_approval",
                "project_template_onboarding_governance",
                "project_template_onboarding_governance_contract",
                "source_schema=featherdoc.project_template_onboarding_governance_report.v1"
            )) {
            [void]$onboardingParts.Add($part)
        }
        foreach ($fieldName in @("status", "release_ready")) {
            $fieldValue = Get-ReleaseBlockerPropertyValue -Object $onboardingReport -Name $fieldName
            if (-not [string]::IsNullOrWhiteSpace($fieldValue)) {
                [void]$onboardingParts.Add("${fieldName}=$fieldValue")
            }
        }
        [void]$onboardingParts.Add("schema_approval_status_summary=$schemaApprovalSummary")
        $nextAction = Format-ProjectTemplateNextActionSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $onboardingReport -Name "next_action")
        if (-not [string]::IsNullOrWhiteSpace($nextAction)) {
            [void]$onboardingParts.Add("next_action=$nextAction")
        }
        $nextActionSummary = Format-ProjectTemplateNextActionSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $onboardingReport -Name "next_action_summary")
        if (-not [string]::IsNullOrWhiteSpace($nextActionSummary)) {
            [void]$onboardingParts.Add("next_action_summary=$nextActionSummary")
        }
        $nextActionGroupCount = Get-ReleaseBlockerPropertyValue -Object $onboardingReport -Name "next_action_group_count"
        if (-not [string]::IsNullOrWhiteSpace($nextActionGroupCount)) {
            [void]$onboardingParts.Add("next_action_group_count=$nextActionGroupCount")
        }
        [void]$onboardingParts.Add("source_report_display=$(Get-GovernanceSourceReportDisplay -Item $onboardingReport)")
        [void]$onboardingParts.Add("source_json_display=$(Get-GovernanceSourceJsonDisplay -Item $onboardingReport)")
        [void]$Lines.Add("- Project template onboarding: $($onboardingParts -join ' ')")
    }
}

function Add-ProjectTemplateReadinessChecklistEvidenceSummaryLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary
    )

    $checklistHandoffEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine -Summary $Summary
    $packagedAuditEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine -Summary $Summary
    if ([string]::IsNullOrWhiteSpace($checklistHandoffEvidenceLine) -and
        [string]::IsNullOrWhiteSpace($packagedAuditEvidenceLine)) {
        return
    }

    [void]$Lines.Add("")
    [void]$Lines.Add("## Project template release checklist evidence")
    if (-not [string]::IsNullOrWhiteSpace($checklistHandoffEvidenceLine)) {
        [void]$Lines.Add("- $checklistHandoffEvidenceLine")
    }
    if (-not [string]::IsNullOrWhiteSpace($packagedAuditEvidenceLine)) {
        [void]$Lines.Add("- $packagedAuditEvidenceLine")
    }
}

function Add-ProjectTemplateReadinessChecklistEvidenceShortSummaryBullets {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary
    )

    $checklistHandoffEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistEntrypointsEvidenceLine -Summary $Summary
    if (-not [string]::IsNullOrWhiteSpace($checklistHandoffEvidenceLine)) {
        Add-UniqueLine -Lines $Lines -Line (
            "project-template readiness checklist handoff evidence 已进入短摘要：$checklistHandoffEvidenceLine"
        )
    }

    $packagedAuditEvidenceLine = Get-ReleaseGovernanceProjectTemplateReadinessChecklistMaterialSafetyAuditEvidenceLine -Summary $Summary
    if (-not [string]::IsNullOrWhiteSpace($packagedAuditEvidenceLine)) {
        Add-UniqueLine -Lines $Lines -Line (
            "project-template readiness checklist packaged audit evidence 已进入短摘要：$packagedAuditEvidenceLine"
        )
    }
}

function Add-ProjectTemplateGovernanceContractShortSummaryBullets {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$Summary
    )

    $readinessReport = Get-FirstGovernanceReport `
        -Summary $Summary `
        -Id "project_template_delivery_readiness" `
        -Schema "featherdoc.project_template_delivery_readiness_report.v1"
    $onboardingReport = Get-FirstGovernanceReport `
        -Summary $Summary `
        -Id "" `
        -Schema "featherdoc.project_template_onboarding_governance_report.v1"

    if ($null -ne $readinessReport) {
        $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $readinessReport -Name "schema_approval_status_summary")
        $onboardingGovernanceNextAction = Format-ProjectTemplateNextActionSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $readinessReport -Name "onboarding_governance_next_action")
        Add-UniqueLine -Lines $Lines -Line (
            'project-template readiness governance contract 已进入短摘要： status={0} release_ready={1} latest_schema_approval_gate_status={2} schema_approval_status_summary={3} onboarding_governance_next_action={4} onboarding_governance_next_action_group_count={5} release_blocker_count={6} warning_count={7} source_report_display={8} source_json_display={9}。' -f `
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $readinessReport -Name "status")),
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $readinessReport -Name "release_ready")),
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $readinessReport -Name "latest_schema_approval_gate_status")),
                (Get-DisplayValue -Value $schemaApprovalSummary),
                (Get-DisplayValue -Value $onboardingGovernanceNextAction),
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $readinessReport -Name "onboarding_governance_next_action_group_count")),
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $readinessReport -Name "release_blocker_count")),
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $readinessReport -Name "warning_count")),
                (Get-DisplayValue -Value (Get-GovernanceSourceReportDisplay -Item $readinessReport)),
                (Get-DisplayValue -Value (Get-GovernanceSourceJsonDisplay -Item $readinessReport))
        )
    }

    if ($null -ne $onboardingReport) {
        $schemaApprovalSummary = Format-ProjectTemplateSchemaApprovalStatusSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $onboardingReport -Name "schema_approval_status_summary") `
            -Fallback (Get-ReleaseBlockerPropertyValue -Object $onboardingReport -Name "status")
        $nextAction = Format-ProjectTemplateNextActionSummary `
            -Value (Get-ReleaseBlockerPropertyObject -Object $onboardingReport -Name "next_action")

        Add-UniqueLine -Lines $Lines -Line (
            'project-template onboarding governance contract 已进入短摘要： status={0} release_ready={1} schema_approval_status_summary={2} next_action={3} next_action_group_count={4} source_report_display={5} source_json_display={6}。' -f `
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $onboardingReport -Name "status")),
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $onboardingReport -Name "release_ready")),
                (Get-DisplayValue -Value $schemaApprovalSummary),
                (Get-DisplayValue -Value $nextAction),
                (Get-DisplayValue -Value (Get-ReleaseBlockerPropertyValue -Object $onboardingReport -Name "next_action_group_count")),
                (Get-DisplayValue -Value (Get-GovernanceSourceReportDisplay -Item $onboardingReport)),
                (Get-DisplayValue -Value (Get-GovernanceSourceJsonDisplay -Item $onboardingReport))
        )
    }
}

function Add-PdfVisualGateEvidenceShortSummaryBullets {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$PdfVisualGateEvidence,
        [string]$RepoRoot
    )

    if ($null -eq $PdfVisualGateEvidence) {
        return
    }

    $status = Get-OptionalPropertyValue -Object $PdfVisualGateEvidence -Name "status"
    if ($status -ne "loaded") {
        return
    }

    Add-UniqueLine -Lines $Lines -Line (
        'PDF visual gate 已进入短摘要：verdict={0} summary={1} aggregate_contact_sheet={2} cjk_manifest_count={3} cjk_copy_search_count={4} visual_baseline_manifest_count={5} visual_baseline_count={6}。' -f `
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfVisualGateEvidence -Name "verdict")),
            (Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $RepoRoot -Value (Get-OptionalPropertyValue -Object $PdfVisualGateEvidence -Name "summary_json"))),
            (Get-DisplayValue -Value (Get-PublicArtifactPath -RepoRoot $RepoRoot -Value (Get-OptionalPropertyValue -Object $PdfVisualGateEvidence -Name "aggregate_contact_sheet"))),
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfVisualGateEvidence -Name "cjk_manifest_count")),
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfVisualGateEvidence -Name "cjk_copy_search_count")),
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfVisualGateEvidence -Name "visual_baseline_manifest_count")),
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfVisualGateEvidence -Name "visual_baseline_count"))
    )
}

function Add-PdfBoundedCtestEvidenceShortSummaryBullets {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        [AllowNull()]$PdfBoundedCtestEvidence
    )

    if ($null -eq $PdfBoundedCtestEvidence) {
        return
    }

    $status = Get-OptionalPropertyValue -Object $PdfBoundedCtestEvidence -Name "status"
    if ([string]::IsNullOrWhiteSpace($status) -or $status -eq "not_available") {
        return
    }

    Add-UniqueLine -Lines $Lines -Line (
        'PDF bounded CTest 辅助证据已进入短摘要：status={0} summaries={1} pass={2} selected_tests={3} skipped_tests={4}；该证据只补充资源受限复核，不替代 full visual gate verdict。' -f `
            (Get-DisplayValue -Value $status),
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfBoundedCtestEvidence -Name "summary_count")),
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfBoundedCtestEvidence -Name "pass_count")),
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfBoundedCtestEvidence -Name "selected_test_count")),
            (Get-DisplayValue -Value (Get-OptionalPropertyValue -Object $PdfBoundedCtestEvidence -Name "skipped_test_count"))
    )
}

function Normalize-ReleaseFacingText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }

    $normalized = ($Text -replace '\s+', ' ').Trim()
    $normalized = $normalized.Replace("public release drafts", "public release notes")
    $normalized = $normalized.Replace("release drafts and reviewer handoff material", "release notes and reviewer handoff material")
    $normalized = $normalized.Replace("release-note drafts", "release notes")
    $normalized = $normalized.Replace("release-note drafting", "release-note preparation")
    $normalized = $normalized.Replace("release-body drafts", "release bodies")
    $normalized = $normalized.Replace("release-body draft", "release body")
    $normalized = $normalized.Replace("draft review", "release-note review")
    $normalized = $normalized.Replace("release drafts", "release notes")
    $normalized = $normalized.Replace("release draft", "release notes")
    $normalized = $normalized.Replace("draft's", "generated")
    $normalized = $normalized.Replace("`release_summary.zh-CN.md` draft", "`release_summary.zh-CN.md` summary")
    $normalized = $normalized.Replace("发布说明草稿", "发布说明")
    $normalized = $normalized.Replace("草稿", "说明")
    $normalized = $normalized.Replace("C:\Users\...", "Windows absolute paths")
    $normalized = $normalized.Replace("/Users/...", "Unix-style absolute paths")

    return $normalized
}

function Normalize-ChangelogBullet {
    param(
        [string]$Bullet,
        [string]$SectionName = ""
    )

    $normalizedBullet = Normalize-ReleaseFacingText -Text $Bullet
    switch ($SectionName) {
        "Added" { $normalizedBullet = $normalizedBullet -replace '^(Added|Updated)\s+', '' }
        "Changed" { $normalizedBullet = $normalizedBullet -replace '^(Changed|Updated)\s+', '' }
        "Fixed" { $normalizedBullet = $normalizedBullet -replace '^Fixed\s+', '' }
        "Validation" { $normalizedBullet = $normalizedBullet -replace '^Validated\s+', '' }
        "Dependencies" { $normalizedBullet = $normalizedBullet -replace '^Updated\s+', '' }
    }

    return $normalizedBullet.Trim()
}

function Get-NormalizedChangelogEntries {
    param($Sections)

    $entries = New-Object 'System.Collections.Generic.List[object]'
    if ($null -eq $Sections -or $Sections.Count -eq 0) {
        return $entries
    }

    $preferredOrder = @("Added", "Changed", "Fixed", "Validation", "Performance", "Dependencies")
    $orderedNames = New-Object 'System.Collections.Generic.List[string]'
    foreach ($name in $preferredOrder) {
        if ($Sections.Contains($name)) {
            [void]$orderedNames.Add($name)
        }
    }
    foreach ($property in $Sections.Keys) {
        if (-not $orderedNames.Contains($property)) {
            [void]$orderedNames.Add($property)
        }
    }

    foreach ($sectionName in $orderedNames) {
        $bullets = $Sections[$sectionName]
        if ($null -eq $bullets) {
            continue
        }

        foreach ($bullet in $bullets) {
            $normalizedBullet = Normalize-ChangelogBullet -Bullet $bullet -SectionName $sectionName
            if ([string]::IsNullOrWhiteSpace($normalizedBullet)) {
                continue
            }

            [void]$entries.Add([pscustomobject]@{
                Section = $sectionName
                Text = $normalizedBullet
            })
        }
    }

    return $entries
}

function Test-ChangelogEntryMatch {
    param(
        [System.Collections.Generic.List[object]]$Entries,
        [string]$Pattern
    )

    foreach ($entry in $Entries) {
        if ($entry.Text -match $Pattern) {
            return $true
        }
    }

    return $false
}

function Get-ShortSummaryFallbackBullet {
    param(
        [string]$SectionName,
        [string]$Bullet
    )

    if ([string]::IsNullOrWhiteSpace($Bullet)) {
        return ""
    }

    $label = Get-ChineseSectionName -SectionName $SectionName
    $trimmed = Normalize-ReleaseFacingText -Text $Bullet
    if ($trimmed.Length -gt 92) {
        $trimmed = $trimmed.Substring(0, 89).TrimEnd() + "..."
    }

    return "$label：$trimmed"
}

function Get-ValidationSummaryBullet {
    param(
        [string]$ExecutionStatus,
        [string]$ConfigureStatus,
        [string]$BuildStatus,
        [string]$TestsStatus,
        [string]$InstallSmokeStatus,
        [string]$VisualGateStatus,
        [string]$VisualVerdict
    )

    $resolvedVerdict = if ([string]::IsNullOrWhiteSpace($VisualVerdict)) {
        "pending_manual_review"
    } else {
        $VisualVerdict
    }

    if ($VisualGateStatus -eq "skipped" -or $VisualGateStatus -eq "visual_gate_skipped") {
        return 'CI 侧 release-preflight 当前跳过了 Word visual gate；最终截图级结论仍需在本地 Windows + Microsoft Word 环境补齐。'
    }

    if ($ExecutionStatus -eq "pass" -and $resolvedVerdict -eq "pass") {
        return '本地 full release-preflight 当前为 pass：MSVC configure/build、ctest、install smoke 和 Word visual gate 均已通过，visual verdict 为 `pass`。'
    }

    if ($ExecutionStatus -ne "pass") {
        return ('release-preflight 当前未完全通过：MSVC configure/build={0}/{1}，ctest={2}，install smoke={3}，visual gate={4}，visual verdict={5}。' -f `
            $ConfigureStatus, $BuildStatus, $TestsStatus, $InstallSmokeStatus, $VisualGateStatus, $resolvedVerdict)
    }

    return ('release-preflight 当前已完成，但 visual verdict 仍为 `{0}`；对外发布前还需要补齐最终人工复核。' -f $resolvedVerdict)
}

function Get-VisualValidationDetailBullet {
    param(
        [string]$SmokeVerdict,
        [string]$FixedGridVerdict,
        [string]$SectionPageSetupVerdict,
        [string]$PageNumberFieldsVerdict,
        [object[]]$CuratedVisualReviewEntries
    )

    $parts = New-Object 'System.Collections.Generic.List[string]'

    if (-not [string]::IsNullOrWhiteSpace($SmokeVerdict)) {
        [void]$parts.Add('smoke=`{0}`' -f $SmokeVerdict)
    }
    if (-not [string]::IsNullOrWhiteSpace($FixedGridVerdict)) {
        [void]$parts.Add('fixed-grid=`{0}`' -f $FixedGridVerdict)
    }
    if (-not [string]::IsNullOrWhiteSpace($SectionPageSetupVerdict)) {
        [void]$parts.Add('section page setup=`{0}`' -f $SectionPageSetupVerdict)
    }
    if (-not [string]::IsNullOrWhiteSpace($PageNumberFieldsVerdict)) {
        [void]$parts.Add('page number fields=`{0}`' -f $PageNumberFieldsVerdict)
    }

    $curatedWithVerdicts = @(
        $CuratedVisualReviewEntries |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_.verdict) }
    )
    $takeCount = [Math]::Min($curatedWithVerdicts.Count, 3)
    for ($index = 0; $index -lt $takeCount; $index++) {
        $entry = $curatedWithVerdicts[$index]
        [void]$parts.Add(('{0}=`{1}`' -f $entry.label, $entry.verdict))
    }
    if ($curatedWithVerdicts.Count -gt $takeCount) {
        [void]$parts.Add('其余 {0} 个 curated bundle 见 `release_body.zh-CN.md`' -f ($curatedWithVerdicts.Count - $takeCount))
    }

    if ($parts.Count -eq 0) {
        return ""
    }

    return 'Word visual gate 细分结论：{0}。' -f ($parts -join '，')
}

function Get-ShortSummaryBullets {
    param(
        $Sections,
        [string]$ChangelogSourceLabel = '`Unreleased` 区块',
        [string]$ExecutionStatus,
        [string]$ConfigureStatus,
        [string]$BuildStatus,
        [string]$TestsStatus,
        [string]$InstallSmokeStatus,
        [string]$VisualGateStatus,
        [string]$VisualVerdict,
        [string]$InstalledDataDir,
        [string]$TemplateSchemaManifestStatus,
        [string]$TemplateSchemaManifestPassed,
        [string]$TemplateSchemaManifestEntryCount,
        [string]$TemplateSchemaManifestDriftCount,
        [string]$ProjectTemplateSmokeDirtySchemaBaselineCount,
        [string]$SmokeVerdict,
        [string]$FixedGridVerdict,
        [string]$SectionPageSetupVerdict,
        [string]$PageNumberFieldsVerdict,
        [object[]]$CuratedVisualReviewEntries
    )

    $bullets = New-Object 'System.Collections.Generic.List[string]'
    $entries = Get-NormalizedChangelogEntries -Sections $Sections

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'README\.zh-CN\.md|docs/index\.rst|VISUAL_VALIDATION(\.zh-CN)?\.md') {
        Add-UniqueLine -Lines $bullets -Line '文档首页、仓库首页和安装树入口现在统一展示完整 Word smoke contact sheet，以及 fixed-grid、merge/unmerge、纵排与 RTL/LTR/CJK 混排重点图。'
    }

    if (-not [string]::IsNullOrWhiteSpace($InstalledDataDir) -or
        (Test-ChangelogEntryMatch -Entries $entries -Pattern 'VISUAL_VALIDATION_QUICKSTART|RELEASE_ARTIFACT_TEMPLATE')) {
        Add-UniqueLine -Lines $bullets -Line '安装树 `share/FeatherDoc` 现在直接附带 quickstart、release 模板和预览 PNG，可从截图一路跳到复现脚本与 review task。'
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'write_release_artifact_handoff\.ps1|write_release_body_zh\.ps1|release_body\.zh-CN\.md|release_handoff\.md') {
        Add-UniqueLine -Lines $bullets -Line ('release-preflight 现在会自动产出 `release_handoff.md`、`release_body.zh-CN.md` 和 `release_summary.zh-CN.md`，并从 `CHANGELOG.md` 的 {0} 预填核心变化。' -f $ChangelogSourceLabel)
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'windows-msvc-release-metadata|windows-msvc\.yml') {
        Add-UniqueLine -Lines $bullets -Line 'Windows CI 现在会额外上传 `windows-msvc-release-metadata` artifact，把安装树文档入口和 release report 一并交给评审或发布人。'
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'table_style_look|w:tblLook') {
        Add-UniqueLine -Lines $bullets -Line '现有表格现在可以直接编辑 `w:tblLook`，首末行列强调和行列 banding 可在不落回原始 XML 的情况下安全调节。'
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'insert_cell_before|insert_cell_after|TableCell::remove\(\)|unmerge_right|unmerge_down|column_width_twips|set_column_width_twips|clear_column_width') {
        Add-UniqueLine -Lines $bullets -Line 'fixed-grid 表格现在补齐安全插列、删列、列宽编辑与 merge/unmerge 四象限能力，reopened fixed-layout 表格也能保持 `w:tblGrid` / `w:tcW` 一致。'
    }

    if (Test-ChangelogEntryMatch -Entries $entries -Pattern 'restart_paragraph_list') {
        Add-UniqueLine -Lines $bullets -Line '段落列表现在支持安全重启编号，Word 渲染下可以稳定得到新的 `1.` 起始序列。'
    }

    foreach ($entry in $entries) {
        if ($bullets.Count -ge 7) {
            break
        }

        $fallbackBullet = Get-ShortSummaryFallbackBullet -SectionName $entry.Section -Bullet $entry.Text
        Add-UniqueLine -Lines $bullets -Line $fallbackBullet
    }

    if ($bullets.Count -gt 7) {
        while ($bullets.Count -gt 7) {
            $bullets.RemoveAt($bullets.Count - 1)
        }
    }

    Add-UniqueLine -Lines $bullets -Line (Get-ValidationSummaryBullet `
        -ExecutionStatus $ExecutionStatus `
        -ConfigureStatus $ConfigureStatus `
        -BuildStatus $BuildStatus `
        -TestsStatus $TestsStatus `
        -InstallSmokeStatus $InstallSmokeStatus `
        -VisualGateStatus $VisualGateStatus `
        -VisualVerdict $VisualVerdict)

    if (-not [string]::IsNullOrWhiteSpace($TemplateSchemaManifestStatus) -and
        $TemplateSchemaManifestStatus -ne "not_requested") {
        if ($TemplateSchemaManifestPassed -eq "True") {
            Add-UniqueLine -Lines $bullets -Line (
                '仓库级 template schema manifest 当前覆盖 {0} 份 baseline，漂移数为 {1}，并已纳入 release preflight。' -f `
                    $TemplateSchemaManifestEntryCount, $TemplateSchemaManifestDriftCount
            )
        } else {
            Add-UniqueLine -Lines $bullets -Line (
                '仓库级 template schema manifest 当前覆盖 {0} 份 baseline，但仍有 {1} 份漂移待处理。' -f `
                    $TemplateSchemaManifestEntryCount, $TemplateSchemaManifestDriftCount
            )
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($ProjectTemplateSmokeDirtySchemaBaselineCount) -and
        $ProjectTemplateSmokeDirtySchemaBaselineCount -ne "0") {
        Add-UniqueLine -Lines $bullets -Line (
            'project template smoke 当前发现 {0} 份 schema baseline lint 未清理，发布前需先 repair 或重新登记 baseline。' -f `
                $ProjectTemplateSmokeDirtySchemaBaselineCount
        )
    }

    $visualValidationDetailBullet = Get-VisualValidationDetailBullet `
        -SmokeVerdict $SmokeVerdict `
        -FixedGridVerdict $FixedGridVerdict `
        -SectionPageSetupVerdict $SectionPageSetupVerdict `
        -PageNumberFieldsVerdict $PageNumberFieldsVerdict `
        -CuratedVisualReviewEntries $CuratedVisualReviewEntries
    Add-UniqueLine -Lines $bullets -Line $visualValidationDetailBullet

    $preservedTailCount = 1
    if (-not [string]::IsNullOrWhiteSpace($visualValidationDetailBullet)) {
        $preservedTailCount = 2
    }
    $maxBulletCount = 7 + $preservedTailCount
    if ($bullets.Count -gt $maxBulletCount) {
        while ($bullets.Count -gt $maxBulletCount) {
            $removalIndex = [Math]::Max(0, $bullets.Count - $preservedTailCount - 1)
            $bullets.RemoveAt($removalIndex)
        }
    }

    return $bullets
}

function Get-ValidationNote {
    param(
        [string]$ExecutionStatus,
        [string]$VisualGateStatus,
        [string]$VisualVerdict
    )

    if ($ExecutionStatus -ne "pass") {
        return "当前 preflight 未完全通过，发布前先处理失败步骤，再重刷本文件。"
    }

    if ($VisualGateStatus -eq "skipped" -or $VisualGateStatus -eq "visual_gate_skipped") {
        return "这份结果来自 CI 或跳过 visual gate 的本地检查；Word 截图级复核仍需在本地 Windows + Microsoft Word 环境补齐。"
    }

    if ($VisualVerdict -eq "pass") {
        return "本次 preflight 已补齐截图级 Word 复核，当前 visual verdict 为 pass。"
    }

    if ($VisualVerdict -eq "pending_manual_review" -or $VisualVerdict -eq "undecided" -or [string]::IsNullOrWhiteSpace($VisualVerdict)) {
        return "本次 preflight 已跑到 visual gate，但截图级人工复核尚未完成；发布前请先补写 visual verdict。"
    }

    if ($VisualVerdict -eq "fail") {
        return "截图级 Word 复核当前为 fail；发布前请先解决视觉回归。"
    }

    return "请根据当前验证结果确认是否可以对外发布。"
}
