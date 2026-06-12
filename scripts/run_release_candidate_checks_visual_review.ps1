function Convert-ReviewTimestamp {
    param([AllowNull()]$Value)

    if ($null -eq $Value) {
        return ""
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$Value
}

function Add-OptionalVisualReviewArguments {
    param(
        [object[]]$Arguments,
        [string]$VerdictArgument,
        [string]$Verdict,
        [string]$NoteArgument,
        [string]$Note
    )

    $result = @($Arguments)
    if ($Verdict -ne "undecided" -or -not [string]::IsNullOrWhiteSpace($Note)) {
        $result += @($VerdictArgument, $Verdict)
    }
    if (-not [string]::IsNullOrWhiteSpace($Note)) {
        $result += @($NoteArgument, $Note)
    }

    return $result
}

function Add-GateFlowReviewSummary {
    param(
        [System.Collections.IDictionary]$Target,
        [string]$Prefix,
        [AllowNull()]$FlowInfo
    )

    $reviewStatus = Get-OptionalPropertyValue -Object $FlowInfo -Name "review_status"
    $reviewVerdict = Get-OptionalPropertyValue -Object $FlowInfo -Name "review_verdict"
    $reviewedAt = Convert-ReviewTimestamp -Value (Get-OptionalPropertyValue -Object $FlowInfo -Name "reviewed_at")
    $reviewMethod = Get-OptionalPropertyValue -Object $FlowInfo -Name "review_method"
    $reviewNote = Get-OptionalPropertyValue -Object $FlowInfo -Name "review_note"
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_review_status" -f $Prefix) -Value $reviewStatus
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_verdict" -f $Prefix) -Value $reviewVerdict
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_reviewed_at" -f $Prefix) -Value $reviewedAt
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_review_method" -f $Prefix) -Value $reviewMethod
    Set-OptionalSummaryValue -Target $Target -Name ("{0}_review_note" -f $Prefix) -Value $reviewNote
}

function Get-CuratedVisualReviewSummaryEntries {
    param([AllowNull()]$GateSummary)

    $curatedFlows = @(Get-OptionalPropertyValue -Object $GateSummary -Name "curated_visual_regressions")
    return @(
        $curatedFlows | Where-Object {
            -not [string]::IsNullOrWhiteSpace([string](Get-OptionalPropertyValue -Object $_ -Name "review_verdict"))
        } | ForEach-Object {
            $taskInfo = Get-OptionalPropertyValue -Object $_ -Name "task"
            [ordered]@{
                id = Get-OptionalPropertyValue -Object $_ -Name "id"
                label = Get-OptionalPropertyValue -Object $_ -Name "label"
                verdict = Get-OptionalPropertyValue -Object $_ -Name "review_verdict"
                review_status = Get-OptionalPropertyValue -Object $_ -Name "review_status"
                reviewed_at = Convert-ReviewTimestamp -Value (Get-OptionalPropertyValue -Object $_ -Name "reviewed_at")
                review_method = Get-OptionalPropertyValue -Object $_ -Name "review_method"
                review_note = Get-OptionalPropertyValue -Object $_ -Name "review_note"
                task_id = Get-OptionalPropertyValue -Object $taskInfo -Name "task_id"
                task_dir = Get-OptionalPropertyValue -Object $taskInfo -Name "task_dir"
                review_result_path = Get-OptionalPropertyValue -Object $taskInfo -Name "review_result_path"
                final_review_path = Get-OptionalPropertyValue -Object $taskInfo -Name "final_review_path"
            }
        }
    )
}

function Get-ReleaseCandidateDisplayValue {
    param(
        [AllowNull()]$Value,
        [string]$Fallback = "(not available)"
    )

    if ($null -eq $Value -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        return $Fallback
    }

    return [string]$Value
}

function Get-CompleteVisualGateReviewTaskSummary {
    param([AllowNull()]$Summary)

    if ($null -eq $Summary) {
        return $null
    }

    $totalCount = Get-OptionalPropertyValue -Object $Summary -Name "total_count"
    $standardCount = Get-OptionalPropertyValue -Object $Summary -Name "standard_count"
    $curatedCount = Get-OptionalPropertyValue -Object $Summary -Name "curated_count"
    if ([string]::IsNullOrWhiteSpace([string]$totalCount) -or
        [string]::IsNullOrWhiteSpace([string]$standardCount) -or
        [string]::IsNullOrWhiteSpace([string]$curatedCount)) {
        return $null
    }

    return [pscustomobject]@{
        total_count = $totalCount
        standard_count = $standardCount
        curated_count = $curatedCount
    }
}

function Get-VisualGateReviewTaskSummaryLine {
    param([AllowNull()]$VisualGateStep)

    $reviewTaskSummary = Get-CompleteVisualGateReviewTaskSummary -Summary (Get-OptionalPropertyValue -Object $VisualGateStep -Name "review_task_summary")
    if ($null -eq $reviewTaskSummary) {
        return ""
    }

    return "- Review task count: {0} total ({1} standard, {2} curated)" -f `
        $reviewTaskSummary.total_count,
        $reviewTaskSummary.standard_count,
        $reviewTaskSummary.curated_count
}

function Get-VisualGateReviewSummaryMarkdown {
    param(
        [string]$RepoRoot,
        [AllowNull()]$VisualGateStep
    )

    $reviewLines = New-Object 'System.Collections.Generic.List[string]'
    $standardFlows = @(
        [pscustomobject]@{ Label = "Smoke"; Prefix = "smoke"; TaskProperty = "document_task_dir" },
        [pscustomobject]@{ Label = "Fixed grid"; Prefix = "fixed_grid"; TaskProperty = "fixed_grid_task_dir" },
        [pscustomobject]@{ Label = "Section page setup"; Prefix = "section_page_setup"; TaskProperty = "section_page_setup_task_dir" },
        [pscustomobject]@{ Label = "Page number fields"; Prefix = "page_number_fields"; TaskProperty = "page_number_fields_task_dir" }
    )

    foreach ($flow in $standardFlows) {
        $verdict = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_verdict" -f $flow.Prefix)
        $reviewStatus = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_review_status" -f $flow.Prefix)
        $reviewedAt = Convert-ReviewTimestamp -Value (Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_reviewed_at" -f $flow.Prefix))
        $reviewMethod = Get-OptionalPropertyValue -Object $VisualGateStep -Name ("{0}_review_method" -f $flow.Prefix)
        if ([string]::IsNullOrWhiteSpace([string]$verdict) -and
            [string]::IsNullOrWhiteSpace([string]$reviewStatus) -and
            [string]::IsNullOrWhiteSpace([string]$reviewedAt) -and
            [string]::IsNullOrWhiteSpace([string]$reviewMethod)) {
            continue
        }

        $line = "- {0}: verdict={1}, review_status={2}, reviewed_at={3}, review_method={4}" -f `
            $flow.Label,
            (Get-ReleaseCandidateDisplayValue -Value $verdict),
            (Get-ReleaseCandidateDisplayValue -Value $reviewStatus),
            (Get-ReleaseCandidateDisplayValue -Value $reviewedAt),
            (Get-ReleaseCandidateDisplayValue -Value $reviewMethod)
        $taskDir = Get-OptionalPropertyValue -Object $VisualGateStep -Name $flow.TaskProperty
        if (-not [string]::IsNullOrWhiteSpace([string]$taskDir)) {
            $line += ", task=$(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $taskDir)"
        }

        [void]$reviewLines.Add($line)
    }

    $curatedEntries = @(Get-OptionalPropertyValue -Object $VisualGateStep -Name "curated_visual_regressions")
    foreach ($entry in $curatedEntries) {
        $verdict = Get-OptionalPropertyValue -Object $entry -Name "verdict"
        $reviewStatus = Get-OptionalPropertyValue -Object $entry -Name "review_status"
        $reviewedAt = Convert-ReviewTimestamp -Value (Get-OptionalPropertyValue -Object $entry -Name "reviewed_at")
        $reviewMethod = Get-OptionalPropertyValue -Object $entry -Name "review_method"
        if ([string]::IsNullOrWhiteSpace([string]$verdict) -and
            [string]::IsNullOrWhiteSpace([string]$reviewStatus) -and
            [string]::IsNullOrWhiteSpace([string]$reviewedAt) -and
            [string]::IsNullOrWhiteSpace([string]$reviewMethod)) {
            continue
        }

        $label = Get-OptionalPropertyValue -Object $entry -Name "label"
        if ([string]::IsNullOrWhiteSpace([string]$label)) {
            $label = Get-OptionalPropertyValue -Object $entry -Name "id"
        }
        if ([string]::IsNullOrWhiteSpace([string]$label)) {
            $label = Get-OptionalPropertyValue -Object $entry -Name "task_id"
        }
        if ([string]::IsNullOrWhiteSpace([string]$label)) {
            $label = "Curated visual regression"
        }

        $line = "- Curated - {0}: verdict={1}, review_status={2}, reviewed_at={3}, review_method={4}" -f `
            $label,
            (Get-ReleaseCandidateDisplayValue -Value $verdict),
            (Get-ReleaseCandidateDisplayValue -Value $reviewStatus),
            (Get-ReleaseCandidateDisplayValue -Value $reviewedAt),
            (Get-ReleaseCandidateDisplayValue -Value $reviewMethod)
        $taskDir = Get-OptionalPropertyValue -Object $entry -Name "task_dir"
        if (-not [string]::IsNullOrWhiteSpace([string]$taskDir)) {
            $line += ", task=$(Get-RepoRelativePath -RepoRoot $RepoRoot -Path $taskDir)"
        }

        [void]$reviewLines.Add($line)
    }

    if ($reviewLines.Count -eq 0) {
        return ""
    }

    $sectionLines = New-Object 'System.Collections.Generic.List[string]'
    [void]$sectionLines.Add("## Visual review verdicts")
    [void]$sectionLines.Add("")
    foreach ($line in $reviewLines) {
        [void]$sectionLines.Add($line)
    }
    [void]$sectionLines.Add("")

    return ($sectionLines -join [Environment]::NewLine)
}
