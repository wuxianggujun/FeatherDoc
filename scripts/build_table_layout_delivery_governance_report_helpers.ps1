function Write-Step {
    param([string]$Message)
    Write-Host "[table-layout-delivery-governance] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$Path, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $root = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }
    try {
        $resolved = [System.IO.Path]::GetFullPath($Path)
        if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
            if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
            return ".\" + ($relative -replace '/', '\')
        }
        return $resolved
    } catch {
        return $Path
    }
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-FirstJsonString {
    param($Object, [string[]]$Names, [string]$DefaultValue = "")

    foreach ($name in @($Names)) {
        $value = Get-JsonString -Object $Object -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) { return $value }
    }
    return $DefaultValue
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    if ($value -is [bool]) { return [bool]$value }
    return [string]$value -in @("true", "True", "1", "yes", "Yes")
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IDictionary]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Set-DefaultJsonProperty {
    param($Object, [string]$Name, $Value)

    if ($null -eq $Object -or $null -eq $Value -or [string]::IsNullOrWhiteSpace([string]$Value)) {
        return
    }
    if (-not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Object -Name $Name))) {
        return
    }
    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }
    $Object | Add-Member -NotePropertyName $Name -NotePropertyValue $Value -Force
}

function Set-GovernanceTraceMetadata {
    param(
        $Item,
        [string]$RepoRoot,
        [string]$SummaryPath,
        [switch]$EnsureOpenCommand
    )

    $summaryDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $SummaryPath
    Set-DefaultJsonProperty -Object $Item -Name "source_schema" -Value "featherdoc.table_layout_delivery_governance_report.v1"
    Set-DefaultJsonProperty -Object $Item -Name "source_report" -Value $SummaryPath
    Set-DefaultJsonProperty -Object $Item -Name "source_report_display" -Value $summaryDisplay
    Set-DefaultJsonProperty -Object $Item -Name "source_json" -Value $SummaryPath
    Set-DefaultJsonProperty -Object $Item -Name "source_json_display" -Value $summaryDisplay
    if ($EnsureOpenCommand) {
        $openCommand = Get-FirstJsonString -Object $Item -Names @("open_command", "command", "command_template")
        Set-DefaultJsonProperty -Object $Item -Name "open_command" -Value $openCommand
    }
}

function Get-Percent {
    param([int]$Numerator, [int]$Denominator)

    if ($Denominator -le 0) { return 0 }
    $ratio = [Math]::Min(1.0, [double]$Numerator / [double]$Denominator)
    return [int][Math]::Round($ratio * 100, 0)
}

function Get-DeliveryQualityLevel {
    param(
        [int]$Score,
        [int]$DocumentCount,
        [int]$UnresolvedItemCount
    )

    if ($DocumentCount -le 0) { return "insufficient_evidence" }
    if ($Score -ge 95 -and $UnresolvedItemCount -eq 0) { return "release_ready" }
    if ($Score -ge 70) { return "medium" }
    if ($Score -ge 40) { return "low" }
    return "blocked"
}

function New-DeliveryQuality {
    param(
        [int]$DocumentCount,
        [int]$ReadyDocumentCount,
        [int]$NeedsReviewDocumentCount,
        [int]$FailedDocumentCount,
        [int]$TotalTableStyleIssueCount,
        [int]$TotalAutomaticTblLookFixCount,
        [int]$TotalManualTableStyleFixCount,
        [int]$TotalTablePositionAutomaticCount,
        [int]$TotalTablePositionReviewCount,
        [int]$TotalCommandFailureCount,
        [string]$PdfFloatingTableCapabilityStatus = "not_reported",
        [string]$PdfFloatingTableLayoutBoundary = "",
        [int]$PdfFloatingTableSupportedGeometryCount = 0,
        [int]$PdfFloatingTableMetadataOnlyCount = 0,
        [int]$PdfFloatingTableTrackedGeometryCount = 0,
        [int]$PdfFloatingTableSupportedGeometryPercent = 0,
        [string[]]$PdfFloatingTableMetadataOnlyFields = @(),
        [string[]]$PdfFloatingTableReviewRequiredFields = @()
    )

    $readyDocumentPercent = Get-Percent -Numerator $ReadyDocumentCount -Denominator $DocumentCount
    $needsReviewPenalty = [Math]::Min(30, $NeedsReviewDocumentCount * 15)
    $failedDocumentPenalty = [Math]::Min(40, $FailedDocumentCount * 25)
    $styleIssuePenalty = [Math]::Min(20, $TotalTableStyleIssueCount * 5)
    $safeFixPenalty = [Math]::Min(15, $TotalAutomaticTblLookFixCount * 4)
    $manualFixPenalty = [Math]::Min(20, $TotalManualTableStyleFixCount * 10)
    $floatingPlanPenalty = [Math]::Min(20, ($TotalTablePositionAutomaticCount * 3) + ($TotalTablePositionReviewCount * 8))
    $commandFailurePenalty = [Math]::Min(40, $TotalCommandFailureCount * 20)
    $unresolvedItemCount = $NeedsReviewDocumentCount + $FailedDocumentCount + $TotalTableStyleIssueCount +
        $TotalAutomaticTblLookFixCount + $TotalManualTableStyleFixCount +
        $TotalTablePositionAutomaticCount + $TotalTablePositionReviewCount + $TotalCommandFailureCount
    $score = [int][Math]::Max(0, $readyDocumentPercent - $needsReviewPenalty - $failedDocumentPenalty -
        $styleIssuePenalty - $safeFixPenalty - $manualFixPenalty - $floatingPlanPenalty - $commandFailurePenalty)
    $level = Get-DeliveryQualityLevel -Score $score -DocumentCount $DocumentCount -UnresolvedItemCount $unresolvedItemCount
    $pdfFloatingTableSupportCoverage = "{0}/{1} supported ({2} percent); metadata_only={3}" -f
        $PdfFloatingTableSupportedGeometryCount,
        $PdfFloatingTableTrackedGeometryCount,
        $PdfFloatingTableSupportedGeometryPercent,
        $PdfFloatingTableMetadataOnlyCount
    $pdfFloatingTableReviewerFocus = "review metadata-only tblpPr fields before approving PDF-layout-sensitive release."

    return [ordered]@{
        id = "table_layout_delivery_governance.delivery_quality"
        metric = "delivery_quality"
        report_id = "table_layout_delivery_governance"
        source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        score = $score
        level = $level
        document_count = $DocumentCount
        ready_document_count = $ReadyDocumentCount
        ready_document_percent = $readyDocumentPercent
        needs_review_document_count = $NeedsReviewDocumentCount
        failed_document_count = $FailedDocumentCount
        table_style_issue_count = $TotalTableStyleIssueCount
        automatic_tblLook_fix_count = $TotalAutomaticTblLookFixCount
        manual_table_style_fix_count = $TotalManualTableStyleFixCount
        table_position_automatic_count = $TotalTablePositionAutomaticCount
        table_position_review_count = $TotalTablePositionReviewCount
        pdf_floating_table_capability_status = $PdfFloatingTableCapabilityStatus
        pdf_floating_table_layout_boundary = $PdfFloatingTableLayoutBoundary
        pdf_floating_table_supported_geometry_count = $PdfFloatingTableSupportedGeometryCount
        pdf_floating_table_metadata_only_count = $PdfFloatingTableMetadataOnlyCount
        pdf_floating_table_tracked_geometry_count = $PdfFloatingTableTrackedGeometryCount
        pdf_floating_table_supported_geometry_percent = $PdfFloatingTableSupportedGeometryPercent
        pdf_floating_table_support_coverage = $pdfFloatingTableSupportCoverage
        pdf_floating_table_reviewer_focus = $pdfFloatingTableReviewerFocus
        pdf_floating_table_metadata_only_fields = @($PdfFloatingTableMetadataOnlyFields)
        pdf_floating_table_review_required_fields = @($PdfFloatingTableReviewRequiredFields)
        metadata_only_fields = @($PdfFloatingTableMetadataOnlyFields)
        review_required_fields = @($PdfFloatingTableReviewRequiredFields)
        command_failure_count = $TotalCommandFailureCount
        unresolved_item_count = $unresolvedItemCount
        penalty_summary = @(
            [ordered]@{ factor = "needs_review_documents"; count = $NeedsReviewDocumentCount; penalty = $needsReviewPenalty }
            [ordered]@{ factor = "failed_documents"; count = $FailedDocumentCount; penalty = $failedDocumentPenalty }
            [ordered]@{ factor = "table_style_issues"; count = $TotalTableStyleIssueCount; penalty = $styleIssuePenalty }
            [ordered]@{ factor = "safe_tblLook_fixes_pending"; count = $TotalAutomaticTblLookFixCount; penalty = $safeFixPenalty }
            [ordered]@{ factor = "manual_table_style_fixes"; count = $TotalManualTableStyleFixCount; penalty = $manualFixPenalty }
            [ordered]@{ factor = "floating_table_plans_pending"; count = ($TotalTablePositionAutomaticCount + $TotalTablePositionReviewCount); penalty = $floatingPlanPenalty }
            [ordered]@{ factor = "command_failures"; count = $TotalCommandFailureCount; penalty = $commandFailurePenalty }
        )
    }
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-TemplateSchemaArgumentList -Values $ExplicitPaths)) {
        $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
    }

    $scanRoots = @(Expand-TemplateSchemaArgumentList -Values $Roots)
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @("output/table-layout-delivery-rollup")
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }
        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                Where-Object { $_.Name -eq "summary.json" } |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Sort-Object -Unique)
}

function Get-EvidenceKind {
    param($Json)

    $schema = Get-JsonString -Object $Json -Name "schema"
    if ($schema -eq "featherdoc.table_layout_delivery_governance_report.v1") {
        return "table_layout_delivery_governance_report"
    }
    if ($schema -eq "featherdoc.table_layout_delivery_rollup_report.v1" -or
        $null -ne (Get-JsonProperty -Object $Json -Name "table_position_plans")) {
        return "table_layout_delivery_rollup_report"
    }
    return "unknown"
}

function Add-SummaryGroup {
    param([object[]]$Items, [string]$PropertyName, [string]$OutputName)

    return @(
        foreach ($group in @($Items | Group-Object $PropertyName |
            Sort-Object -Property @{ Expression = "Count"; Descending = $true },
                @{ Expression = "Name"; Ascending = $true })) {
            $summary = [ordered]@{}
            $summary[$OutputName] = [string]$group.Name
            $summary["count"] = [int]$group.Count
            $summary
        }
    )
}

function Add-PdfFloatingTableSupportSummary {
    param([object[]]$Items)

    $totals = @{}
    foreach ($item in @($Items)) {
        $status = Get-JsonString -Object $item -Name "status" -DefaultValue "not_reported"
        if ([string]::IsNullOrWhiteSpace($status)) { $status = "not_reported" }
        if (-not $totals.ContainsKey($status)) { $totals[$status] = 0 }
        $totals[$status] = [int]$totals[$status] + 1
    }

    return @(
        foreach ($status in @($totals.Keys | Sort-Object)) {
            [ordered]@{
                status = [string]$status
                count = [int]$totals[$status]
            }
        }
    )
}

function Get-UniqueStringValues {
    param([object[]]$Values)

    $seen = @{}
    $result = New-Object 'System.Collections.Generic.List[string]'
    foreach ($value in @($Values)) {
        $text = [string]$value
        if ([string]::IsNullOrWhiteSpace($text)) { continue }
        if ($seen.ContainsKey($text)) { continue }
        $seen[$text] = $true
        $result.Add($text) | Out-Null
    }
    return @($result.ToArray())
}

function New-ReleaseBlocker {
    param(
        [string]$Id,
        [string]$Scope,
        [string]$SourceKind,
        [string]$Severity = "warning",
        [string]$Status = "needs_review",
        [string]$Action,
        [string]$Message,
        [string]$RepairStrategy = "",
        [string]$RepairHint = "",
        [string]$CommandTemplate = ""
    )

    return [ordered]@{
        id = $Id
        scope = $Scope
        source_kind = $SourceKind
        severity = $Severity
        status = $Status
        action = $Action
        message = $Message
        repair_strategy = $RepairStrategy
        repair_hint = $RepairHint
        command_template = $CommandTemplate
    }
}

function New-ActionItem {
    param(
        [string]$Id,
        [string]$Scope,
        [string]$SourceKind,
        [string]$Action,
        [string]$Title,
        [string]$Command = "",
        [string]$RepairStrategy = "",
        [string]$RepairHint = "",
        [string]$CommandTemplate = ""
    )

    return [ordered]@{
        id = $Id
        scope = $Scope
        source_kind = $SourceKind
        action = $Action
        title = $Title
        command = $Command
        repair_strategy = $RepairStrategy
        repair_hint = $RepairHint
        command_template = $CommandTemplate
    }
}

function Add-UniqueBlocker {
    param($Collection, $Blocker)

    $key = "$($Blocker.scope)|$($Blocker.id)|$($Blocker.action)"
    foreach ($existing in @($Collection.ToArray())) {
        $existingKey = "$($existing.scope)|$($existing.id)|$($existing.action)"
        if ($existingKey -eq $key) { return }
    }
    $Collection.Add($Blocker) | Out-Null
}

function Add-UniqueAction {
    param($Collection, $Action)

    $key = "$($Action.scope)|$($Action.id)|$($Action.action)"
    foreach ($existing in @($Collection.ToArray())) {
        $existingKey = "$($existing.scope)|$($existing.id)|$($existing.action)"
        if ($existingKey -eq $key) { return }
    }
    $Collection.Add($Action) | Out-Null
}

function Get-MarkdownTraceSuffix {
    param($Item)

    $parts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($field in @("source_schema", "source_report_display", "source_json_display",
            "repair_strategy", "repair_hint", "command_template", "command")) {
        $value = Get-JsonString -Object $Item -Name $field
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $parts.Add(("{0}=``{1}``" -f $field, $value)) | Out-Null
        }
    }

    if ($parts.Count -eq 0) { return "" }
    return " " + ($parts.ToArray() -join " ")
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Table Layout Delivery Governance Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Documents: ``$($Summary.document_count)``") | Out-Null
    $lines.Add("- Ready documents: ``$($Summary.ready_document_count)``") | Out-Null
    $lines.Add("- Needs review documents: ``$($Summary.needs_review_document_count)``") | Out-Null
    $lines.Add("- Table style issues: ``$($Summary.total_table_style_issue_count)``") | Out-Null
    $lines.Add("- Safe tblLook fixes: ``$($Summary.total_automatic_tblLook_fix_count)``") | Out-Null
    $lines.Add("- Manual table style fixes: ``$($Summary.total_manual_table_style_fix_count)``") | Out-Null
    $lines.Add("- Floating table automatic plans: ``$($Summary.total_table_position_automatic_count)``") | Out-Null
    $lines.Add("- Floating table review plans: ``$($Summary.total_table_position_review_count)``") | Out-Null
    $lines.Add("- PDF floating table support reports: ``$($Summary.pdf_floating_table_support_report_count)``") | Out-Null
    $lines.Add("- PDF floating table supported geometry: ``$($Summary.pdf_floating_table_supported_geometry_count)/$($Summary.pdf_floating_table_tracked_geometry_count)`` (``$($Summary.pdf_floating_table_supported_geometry_percent)%``)") | Out-Null
    $lines.Add("- pdf_floating_table_support_coverage: ``$($Summary.pdf_floating_table_support_coverage)``") | Out-Null
    $lines.Add("- pdf_floating_table_reviewer_focus: ``$($Summary.pdf_floating_table_reviewer_focus)``") | Out-Null
    if (@($Summary.pdf_floating_table_metadata_only_fields).Count -gt 0) {
        $lines.Add("- pdf_floating_table_metadata_only_fields: ``$(@($Summary.pdf_floating_table_metadata_only_fields) -join ', ')``") | Out-Null
    }
    if (@($Summary.pdf_floating_table_review_required_fields).Count -gt 0) {
        $lines.Add("- pdf_floating_table_review_required_fields: ``$(@($Summary.pdf_floating_table_review_required_fields) -join ', ')``") | Out-Null
    }
    if (@($Summary.metadata_only_fields).Count -gt 0) {
        $lines.Add("- metadata_only_fields: ``$(@($Summary.metadata_only_fields) -join ', ')``") | Out-Null
    }
    if (@($Summary.review_required_fields).Count -gt 0) {
        $lines.Add("- review_required_fields: ``$(@($Summary.review_required_fields) -join ', ')``") | Out-Null
    }
    $lines.Add("- Delivery quality: ``$($Summary.delivery_quality_level)`` (score=``$($Summary.delivery_quality_score)``)") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Delivery Quality") | Out-Null
    $lines.Add("") | Out-Null
    $quality = $Summary.delivery_quality
    $lines.Add("- Metric id: ``$($quality.id)``") | Out-Null
    $lines.Add("- Metric: ``$($quality.metric)``") | Out-Null
    $lines.Add("- Report id: ``$($quality.report_id)``") | Out-Null
    $lines.Add("- Source schema: ``$($quality.source_schema)``") | Out-Null
    $lines.Add("- Level: ``$($quality.level)``") | Out-Null
    $lines.Add("- Score: ``$($quality.score)``") | Out-Null
    $lines.Add("- Ready documents: ``$($quality.ready_document_percent)%``") | Out-Null
    $lines.Add("- Table style issues: ``$($quality.table_style_issue_count)``") | Out-Null
    $lines.Add("- Automatic tblLook fixes: ``$($quality.automatic_tblLook_fix_count)``") | Out-Null
    $lines.Add("- Manual table style fixes: ``$($quality.manual_table_style_fix_count)``") | Out-Null
    $lines.Add("- Floating table automatic plans: ``$($quality.table_position_automatic_count)``") | Out-Null
    $lines.Add("- Floating table review plans: ``$($quality.table_position_review_count)``") | Out-Null
    $lines.Add("- PDF floating table capability: ``$($quality.pdf_floating_table_capability_status)``") | Out-Null
    $lines.Add("- PDF floating table boundary: ``$($quality.pdf_floating_table_layout_boundary)``") | Out-Null
    $lines.Add("- PDF floating table supported geometry: ``$($quality.pdf_floating_table_supported_geometry_count)/$($quality.pdf_floating_table_tracked_geometry_count)`` (``$($quality.pdf_floating_table_supported_geometry_percent)%``)") | Out-Null
    $lines.Add("- pdf_floating_table_support_coverage: ``$($quality.pdf_floating_table_support_coverage)``") | Out-Null
    $lines.Add("- pdf_floating_table_reviewer_focus: ``$($quality.pdf_floating_table_reviewer_focus)``") | Out-Null
    if (@($quality.pdf_floating_table_metadata_only_fields).Count -gt 0) {
        $lines.Add("- pdf_floating_table_metadata_only_fields: ``$(@($quality.pdf_floating_table_metadata_only_fields) -join ', ')``") | Out-Null
    }
    if (@($quality.pdf_floating_table_review_required_fields).Count -gt 0) {
        $lines.Add("- pdf_floating_table_review_required_fields: ``$(@($quality.pdf_floating_table_review_required_fields) -join ', ')``") | Out-Null
    }
    if (@($quality.metadata_only_fields).Count -gt 0) {
        $lines.Add("- metadata_only_fields: ``$(@($quality.metadata_only_fields) -join ', ')``") | Out-Null
    }
    if (@($quality.review_required_fields).Count -gt 0) {
        $lines.Add("- review_required_fields: ``$(@($quality.review_required_fields) -join ', ')``") | Out-Null
    }
    $lines.Add("- Command failures: ``$($quality.command_failure_count)``") | Out-Null
    $lines.Add("- Unresolved items: ``$($quality.unresolved_item_count)``") | Out-Null
    foreach ($penalty in @($quality.penalty_summary)) {
        $lines.Add(("- Penalty ``{0}``: count=``{1}`` penalty=``{2}``" -f
            $penalty.factor,
            $penalty.count,
            $penalty.penalty)) | Out-Null
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Documents") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.documents).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($document in @($Summary.documents)) {
            $lines.Add(("- ``{0}``: status=``{1}`` preset=``{2}`` issues=``{3}`` tblLook=``{4}`` manual=``{5}`` position_review=``{6}``" -f
                $document.document_name,
                $document.status,
                $document.preset,
                $document.table_style_issue_count,
                $document.automatic_tblLook_fix_count,
                $document.manual_table_style_fix_count,
                $document.table_position_review_count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Floating Table Plans") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.table_position_plans).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($plan in @($Summary.table_position_plans)) {
            $lines.Add(("- ``{0}``: preset=``{1}`` plan=``{2}`` automatic=``{3}`` review=``{4}``" -f
                $plan.document_name,
                $plan.preset,
                $plan.table_position_plan_display,
                $plan.automatic_count,
                $plan.review_count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## PDF Floating Table Support") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.pdf_floating_table_support).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($support in @($Summary.pdf_floating_table_support)) {
            $lines.Add(("- ``{0}``: status=``{1}`` boundary=``{2}`` supported=``{3}`` metadata_only=``{4}`` source=``{5}``" -f
                $support.document_name,
                $support.status,
                $support.boundary,
                $support.supported_geometry_count,
                $support.metadata_only_count,
                $support.source_json_display)) | Out-Null
            $metadataOnlyFields = @($support.metadata_only | ForEach-Object { [string]$_ })
            if ($metadataOnlyFields.Count -gt 0) {
                $lines.Add(("  - metadata_only_fields: ``{0}``" -f ($metadataOnlyFields -join ", "))) | Out-Null
            }
            $reviewRequiredFields = @($support.review_required | ForEach-Object { [string]$_ })
            if ($reviewRequiredFields.Count -gt 0) {
                $lines.Add(("  - review_required_fields: ``{0}``" -f ($reviewRequiredFields -join ", "))) | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Delivery Actions") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.delivery_actions).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.delivery_actions)) {
            $trace = Get-MarkdownTraceSuffix -Item $item
            $lines.Add("- ``$($item.scope)`` / ``$($item.id)``: action=``$($item.action)`` title=$($item.title)$trace") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $trace = Get-MarkdownTraceSuffix -Item $blocker
            $lines.Add("- ``$($blocker.scope)`` / ``$($blocker.id)``: action=``$($blocker.action)`` message=$($blocker.message)$trace") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add("- ``$($warning.id)``: $($warning.message)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Source Files") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.source_files).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($source in @($Summary.source_files)) {
            $lines.Add("- ``$($source.path_display)``: kind=``$($source.kind)`` status=``$($source.status)``") | Out-Null
        }
    }

    return @($lines)
}
