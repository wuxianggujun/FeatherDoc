<#
.SYNOPSIS
Writes a schema patch confidence calibration report from existing summaries.

.DESCRIPTION
Reads project-template smoke summaries, schema approval history summaries, or
onboarding summaries and aggregates schema patch review size, approval outcome,
and optional confidence metadata into calibration buckets. The script is
read-only for input evidence.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/schema-patch-confidence-calibration",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnPending
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$calibrationSchema = "featherdoc.schema_patch_confidence_calibration_report.v1"
$calibrationOpenCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"

function Write-Step {
    param([string]$Message)
    Write-Host "[schema-patch-confidence-calibration] $Message"
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
    $resolved = [System.IO.Path]::GetFullPath($Path)
    if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
        return ".\" + ($relative -replace '/', '\')
    }
    return $resolved
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
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) { return $DefaultValue }
    return [string]$value
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)
    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) { return $DefaultValue }
    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)
    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) { return $DefaultValue }
    if ($value -is [bool]) { return [bool]$value }
    return [string]$value -in @("true", "True", "1", "yes", "Yes")
}

function Get-JsonArray {
    param($Object, [string]$Name)
    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Get-NullableJsonInt {
    param($Object, [string]$Name)
    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) { return $null }
    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $null
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @($ExplicitPaths)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
        }
    }

    $scanRoots = @($Roots | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @("output/project-template-smoke", "output/project-template-onboarding")
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }
        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                Where-Object { $_.Name -in @("summary.json", "onboarding_summary.json", "project_template_schema_approval_history.json") } |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Sort-Object -Unique)
}

function Get-CalibrationBucketName {
    param($Confidence)

    if ($null -eq $Confidence) { return "unscored" }
    if ($Confidence -ge 95) { return "95-100" }
    if ($Confidence -ge 80) { return "80-94" }
    if ($Confidence -ge 60) { return "60-79" }
    return "0-59"
}

function Get-CalibrationOutcome {
    param(
        [string]$Status,
        [string]$Decision,
        [bool]$Approved,
        [bool]$Pending,
        [int]$InvalidResultCount,
        [int]$ComplianceIssueCount
    )

    if ($InvalidResultCount -gt 0 -or $ComplianceIssueCount -gt 0 -or $Status -eq "invalid_result") {
        return "invalid_result"
    }
    if ($Approved -or $Decision -eq "approved" -or $Status -eq "approved") {
        return "approved"
    }
    if ($Pending -or $Decision -eq "pending" -or $Status -eq "pending_review") {
        return "pending"
    }
    if ($Decision -in @("rejected", "needs_changes") -or $Status -in @("rejected", "needs_changes", "blocked")) {
        return "rejected"
    }
    return "unknown"
}

function Get-ConfidenceSource {
    param([object[]]$Entries)

    $withConfidence = @($Entries | Where-Object { $null -ne $_.confidence }).Count
    if ($withConfidence -gt 0 -and $withConfidence -eq @($Entries).Count) {
        return "explicit"
    }
    if ($withConfidence -gt 0) {
        return "mixed"
    }
    return "schema_patch_review"
}

function ConvertTo-StableIdSegment {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) { return "unknown" }
    $normalized = ([string]$Value).Trim().ToLowerInvariant() -replace "[^a-z0-9]+", "_"
    $normalized = $normalized.Trim("_")
    if ([string]::IsNullOrWhiteSpace($normalized)) { return "unknown" }
    return $normalized
}

function Get-EntryScopeId {
    param($Entry)

    $projectId = Get-JsonString -Object $Entry -Name "project_id" -DefaultValue "project"
    $templateName = Get-JsonString -Object $Entry -Name "template_name" -DefaultValue (Get-JsonString -Object $Entry -Name "name" -DefaultValue "template")
    $candidateType = Get-JsonString -Object $Entry -Name "candidate_type" -DefaultValue "candidate"
    $segments = @($projectId, $templateName, $candidateType) | ForEach-Object { ConvertTo-StableIdSegment -Value $_ }
    return ($segments -join ".")
}

function Get-ScopedGovernanceId {
    param([string]$BaseId, $Entry, [int]$AffectedCount)

    if ($AffectedCount -le 1) { return $BaseId }
    return "$BaseId.$(Get-EntryScopeId -Entry $Entry)"
}

function Get-TemplateScope {
    param([string]$ProjectId, [string]$TemplateName)

    if (-not [string]::IsNullOrWhiteSpace($ProjectId) -and
        -not [string]::IsNullOrWhiteSpace($TemplateName)) {
        return "$ProjectId/$TemplateName"
    }
    if (-not [string]::IsNullOrWhiteSpace($TemplateName)) { return $TemplateName }
    if (-not [string]::IsNullOrWhiteSpace($ProjectId)) { return $ProjectId }
    return ""
}

function Get-SchemaPatchCandidateType {
    param(
        $Review,
        $Approval,
        [int]$UpsertSlotCount,
        [int]$RemoveTargetCount,
        [int]$RemoveSlotCount,
        [int]$RenameSlotCount,
        [int]$UpdateSlotCount,
        [int]$InsertedSlots,
        [int]$ReplacedSlots
    )

    $explicit = Get-JsonString -Object $Approval -Name "candidate_type" -DefaultValue (Get-JsonString -Object $Review -Name "candidate_type")
    if (-not [string]::IsNullOrWhiteSpace($explicit)) { return $explicit }

    $kinds = New-Object 'System.Collections.Generic.List[string]'
    if ($RenameSlotCount -gt 0) { $kinds.Add("rename") | Out-Null }
    if ($UpdateSlotCount -gt 0 -or $ReplacedSlots -gt 0) { $kinds.Add("update") | Out-Null }
    if (($RemoveTargetCount + $RemoveSlotCount) -gt 0) { $kinds.Add("remove") | Out-Null }
    if ($UpsertSlotCount -gt 0 -or $InsertedSlots -gt 0) { $kinds.Add("add") | Out-Null }

    if ($kinds.Count -eq 0) { return "unknown" }
    if ($kinds.Count -eq 1) { return [string]$kinds[0] }
    return (@($kinds.ToArray()) -join "+")
}

function New-SchemaPatchOperationSummary {
    param(
        [int]$UpsertSlotCount,
        [int]$RemoveTargetCount,
        [int]$RemoveSlotCount,
        [int]$RenameSlotCount,
        [int]$UpdateSlotCount,
        [int]$InsertedSlots,
        [int]$ReplacedSlots
    )

    return [ordered]@{
        add_count = $UpsertSlotCount
        remove_count = ($RemoveTargetCount + $RemoveSlotCount)
        rename_count = $RenameSlotCount
        update_count = $UpdateSlotCount
        inserted_slot_count = $InsertedSlots
        replaced_slot_count = $ReplacedSlots
    }
}

function New-EntryFromReviewAndApproval {
    param(
        [string]$SummaryJson,
        [string]$Name,
        $Review,
        $Approval,
        [string]$ProjectId = "",
        [string]$TemplateName = ""
    )

    $confidence = Get-NullableJsonInt -Object $Approval -Name "confidence"
    if ($null -eq $confidence) { $confidence = Get-NullableJsonInt -Object $Review -Name "confidence" }

    $status = Get-JsonString -Object $Approval -Name "status" -DefaultValue "unknown"
    $decision = Get-JsonString -Object $Approval -Name "decision" -DefaultValue "unknown"
    $approved = Get-JsonBool -Object $Approval -Name "approved"
    $pending = Get-JsonBool -Object $Approval -Name "pending"
    $complianceIssueCount = Get-JsonInt -Object $Approval -Name "compliance_issue_count"
    $invalidResultCount = if ($status -eq "invalid_result") { 1 } else { 0 }
    $outcome = Get-CalibrationOutcome `
        -Status $status `
        -Decision $decision `
        -Approved $approved `
        -Pending $pending `
        -InvalidResultCount $invalidResultCount `
        -ComplianceIssueCount $complianceIssueCount

    $insertedSlots = Get-JsonInt -Object $Review -Name "inserted_slots"
    $replacedSlots = Get-JsonInt -Object $Review -Name "replaced_slots"
    $removeTargetCount = Get-JsonInt -Object $Review -Name "remove_target_count"
    $removeSlotCount = Get-JsonInt -Object $Review -Name "remove_slot_count"
    $renameSlotCount = Get-JsonInt -Object $Review -Name "rename_slot_count"
    $updateSlotCount = Get-JsonInt -Object $Review -Name "update_slot_count"
    $upsertSlotCount = Get-JsonInt -Object $Review -Name "upsert_slot_count"
    $resolvedProjectId = Get-JsonString -Object $Approval -Name "project_id" -DefaultValue (Get-JsonString -Object $Review -Name "project_id" -DefaultValue $ProjectId)
    $resolvedTemplateName = Get-JsonString -Object $Approval -Name "template_name" -DefaultValue (Get-JsonString -Object $Review -Name "template_name" -DefaultValue $TemplateName)
    if ([string]::IsNullOrWhiteSpace($resolvedTemplateName)) { $resolvedTemplateName = $Name }
    $candidateType = Get-SchemaPatchCandidateType `
        -Review $Review `
        -Approval $Approval `
        -UpsertSlotCount $upsertSlotCount `
        -RemoveTargetCount $removeTargetCount `
        -RemoveSlotCount $removeSlotCount `
        -RenameSlotCount $renameSlotCount `
        -UpdateSlotCount $updateSlotCount `
        -InsertedSlots $insertedSlots `
        -ReplacedSlots $replacedSlots

    return [ordered]@{
        name = $Name
        project_id = $resolvedProjectId
        template_name = $resolvedTemplateName
        template_scope = Get-TemplateScope -ProjectId $resolvedProjectId -TemplateName $resolvedTemplateName
        candidate_type = $candidateType
        summary_json = $SummaryJson
        schema_update_candidate = Get-JsonString -Object $Approval -Name "schema_update_candidate"
        review_json = Get-JsonString -Object $Approval -Name "review_json" -DefaultValue (Get-JsonString -Object $Review -Name "review_json")
        approval_result = Get-JsonString -Object $Approval -Name "approval_result"
        changed = Get-JsonBool -Object $Review -Name "changed"
        status = $status
        decision = $decision
        approved = $approved
        pending = $pending
        confidence = $confidence
        reason_code = Get-JsonString -Object $Approval -Name "reason_code" -DefaultValue (Get-JsonString -Object $Review -Name "reason_code")
        evidence = @(Get-JsonArray -Object $Approval -Name "evidence")
        differences = @(Get-JsonArray -Object $Review -Name "differences")
        baseline_slot_count = Get-JsonInt -Object $Review -Name "baseline_slot_count"
        generated_slot_count = Get-JsonInt -Object $Review -Name "generated_slot_count"
        patch_operation_count = ($upsertSlotCount + $removeTargetCount + $removeSlotCount + $renameSlotCount + $updateSlotCount)
        upsert_slot_count = $upsertSlotCount
        remove_target_count = $removeTargetCount
        remove_slot_count = $removeSlotCount
        rename_slot_count = $renameSlotCount
        update_slot_count = $updateSlotCount
        inserted_slots = $insertedSlots
        replaced_slots = $replacedSlots
        operation_summary = New-SchemaPatchOperationSummary `
            -UpsertSlotCount $upsertSlotCount `
            -RemoveTargetCount $removeTargetCount `
            -RemoveSlotCount $removeSlotCount `
            -RenameSlotCount $renameSlotCount `
            -UpdateSlotCount $updateSlotCount `
            -InsertedSlots $insertedSlots `
            -ReplacedSlots $replacedSlots
        compliance_issue_count = $complianceIssueCount
        calibration_bucket = Get-CalibrationBucketName -Confidence $confidence
        calibration_outcome = $outcome
    }
}

function Add-EntriesFromSmokeSummary {
    param(
        [string]$Path,
        $Summary,
        [System.Collections.Generic.List[object]]$Entries,
        [string]$ProjectId = "",
        [string]$TemplateName = ""
    )

    $reviews = @(Get-JsonArray -Object $Summary -Name "schema_patch_reviews")
    $approvals = @(Get-JsonArray -Object $Summary -Name "schema_patch_approval_items")
    $summaryProjectId = Get-JsonString -Object $Summary -Name "project_id" -DefaultValue $ProjectId
    $summaryTemplateName = Get-JsonString -Object $Summary -Name "template_name" -DefaultValue $TemplateName

    if ($reviews.Count -eq 0 -and $approvals.Count -eq 0) {
        return
    }

    if ($approvals.Count -eq 0) {
        foreach ($review in $reviews) {
            $Entries.Add((New-EntryFromReviewAndApproval -SummaryJson $Path -Name (Get-JsonString -Object $review -Name "name" -DefaultValue "schema_patch_review") -Review $review -Approval ([ordered]@{ status = "unknown"; decision = "unknown" }) -ProjectId $summaryProjectId -TemplateName $summaryTemplateName)) | Out-Null
        }
        return
    }

    foreach ($approval in $approvals) {
        $name = Get-JsonString -Object $approval -Name "name" -DefaultValue "schema_patch_approval"
        $matchingReview = $null
        foreach ($review in $reviews) {
            if ((Get-JsonString -Object $review -Name "name") -eq $name) {
                $matchingReview = $review
                break
            }
        }
        if ($null -eq $matchingReview -and $reviews.Count -eq 1) {
            $matchingReview = $reviews[0]
        }
        if ($null -eq $matchingReview) {
            $matchingReview = [ordered]@{ changed = $true }
        }
        $Entries.Add((New-EntryFromReviewAndApproval -SummaryJson $Path -Name $name -Review $matchingReview -Approval $approval -ProjectId $summaryProjectId -TemplateName $summaryTemplateName)) | Out-Null
    }
}

function Add-EntriesFromHistory {
    param(
        [string]$Path,
        $Summary,
        [System.Collections.Generic.List[object]]$Entries
    )

    foreach ($entryHistory in @(Get-JsonArray -Object $Summary -Name "entry_histories")) {
        $historyProjectId = Get-JsonString -Object $entryHistory -Name "project_id"
        $historyTemplateName = Get-JsonString -Object $entryHistory -Name "template_name" -DefaultValue (Get-JsonString -Object $entryHistory -Name "name")
        foreach ($run in @(Get-JsonArray -Object $entryHistory -Name "runs")) {
            Add-EntriesFromSmokeSummary `
                -Path $Path `
                -Summary $run `
                -Entries $Entries `
                -ProjectId (Get-JsonString -Object $run -Name "project_id" -DefaultValue $historyProjectId) `
                -TemplateName (Get-JsonString -Object $run -Name "template_name" -DefaultValue $historyTemplateName)
        }
    }
}

function New-BucketSummary {
    param([object[]]$Entries)

    $bucketSpecs = @(
        @{ Name = "0-59"; Min = 0; Max = 59 },
        @{ Name = "60-79"; Min = 60; Max = 79 },
        @{ Name = "80-94"; Min = 80; Max = 94 },
        @{ Name = "95-100"; Min = 95; Max = 100 },
        @{ Name = "unscored"; Min = $null; Max = $null }
    )

    return @(
        foreach ($bucket in $bucketSpecs) {
            $bucketEntries = @($Entries | Where-Object { $_.calibration_bucket -eq $bucket.Name })
            $candidateCount = $bucketEntries.Count
            $approvedCount = @($bucketEntries | Where-Object { $_.calibration_outcome -eq "approved" }).Count
            $pendingCount = @($bucketEntries | Where-Object { $_.calibration_outcome -eq "pending" }).Count
            $rejectedCount = @($bucketEntries | Where-Object { $_.calibration_outcome -eq "rejected" }).Count
            $invalidResultCount = @($bucketEntries | Where-Object { $_.calibration_outcome -eq "invalid_result" }).Count
            $manualReviewCount = $pendingCount + $rejectedCount + $invalidResultCount
            [ordered]@{
                name = $bucket.Name
                min_confidence = $bucket.Min
                max_confidence = $bucket.Max
                candidate_count = $candidateCount
                approved_count = $approvedCount
                rejected_count = $rejectedCount
                pending_count = $pendingCount
                invalid_result_count = $invalidResultCount
                approval_rate = if ($candidateCount -gt 0) { [math]::Round($approvedCount / $candidateCount, 4) } else { $null }
                manual_review_rate = if ($candidateCount -gt 0) { [math]::Round($manualReviewCount / $candidateCount, 4) } else { $null }
            }
        }
    )
}

function New-GroupSummary {
    param([object[]]$Entries, [string]$PropertyName, [string]$OutputName)

    $counts = @{}
    foreach ($entry in @($Entries)) {
        $value = Get-JsonString -Object $entry -Name $PropertyName
        if (-not $counts.ContainsKey($value)) {
            $counts[$value] = 0
        }
        $counts[$value] = [int]$counts[$value] + 1
    }

    return @(
        foreach ($group in @($counts.GetEnumerator() |
            Sort-Object -Property @{ Expression = "Value"; Descending = $true }, @{ Expression = "Name"; Ascending = $true })) {
            $summary = [ordered]@{}
            $summary[$OutputName] = [string]$group.Name
            $summary["count"] = [int]$group.Value
            $summary
        }
    )
}

function Get-RecommendedMinConfidence {
    param([object[]]$Entries)

    $minimum = $null
    foreach ($entry in @($Entries)) {
        if ((Get-JsonString -Object $entry -Name "calibration_outcome") -ne "approved") {
            continue
        }

        $confidence = Get-NullableJsonInt -Object $entry -Name "confidence"
        if ($null -eq $confidence) {
            continue
        }

        if ($null -eq $minimum -or $confidence -lt $minimum) {
            $minimum = $confidence
        }
    }

    if ($null -eq $minimum) { return $null }
    return [int]$minimum
}

function Add-ScopedRecommendations {
    param(
        [System.Collections.Generic.List[object]]$Recommendations,
        [object[]]$Entries,
        [string]$BaseId,
        [string]$Priority,
        [string]$Recommendation,
        [string]$CountName
    )

    $affectedEntries = @($Entries)
    foreach ($entry in $affectedEntries) {
        $item = [ordered]@{
            id = Get-ScopedGovernanceId -BaseId $BaseId -Entry $entry -AffectedCount $affectedEntries.Count
            priority = $Priority
            recommendation = $Recommendation
            project_id = Get-JsonString -Object $entry -Name "project_id"
            template_name = Get-JsonString -Object $entry -Name "template_name"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
        }
        $item[$CountName] = 1
        $Recommendations.Add($item) | Out-Null
    }
}

function New-Recommendations {
    param([object[]]$Entries, $RecommendedMinConfidence)

    $recommendations = New-Object 'System.Collections.Generic.List[object]'
    $pendingEntries = @($Entries | Where-Object { $_.calibration_outcome -eq "pending" })
    $invalidEntries = @($Entries | Where-Object { $_.calibration_outcome -eq "invalid_result" })
    $unscoredEntries = @($Entries | Where-Object { $_.calibration_bucket -eq "unscored" })

    if ($null -ne $RecommendedMinConfidence) {
        $recommendations.Add([ordered]@{
            id = "set_min_confidence_from_approved_floor"
            priority = "medium"
            recommendation = "Use approved-candidate floor confidence as a conservative starting point."
            recommended_min_confidence = $RecommendedMinConfidence
        }) | Out-Null
    }
    Add-ScopedRecommendations `
        -Recommendations $recommendations `
        -Entries $pendingEntries `
        -BaseId "resolve_pending_schema_approvals" `
        -Priority "high" `
        -Recommendation "Resolve pending schema approvals before using this data to tighten automation thresholds." `
        -CountName "pending_count"
    Add-ScopedRecommendations `
        -Recommendations $recommendations `
        -Entries $invalidEntries `
        -BaseId "fix_invalid_approval_records" `
        -Priority "high" `
        -Recommendation "Fix invalid approval records before treating approval rates as reliable." `
        -CountName "invalid_result_count"
    Add-ScopedRecommendations `
        -Recommendations $recommendations `
        -Entries $unscoredEntries `
        -BaseId "add_explicit_confidence_metadata" `
        -Priority "medium" `
        -Recommendation "Add explicit confidence metadata to future schema patch review or approval records." `
        -CountName "unscored_count"

    return @($recommendations.ToArray())
}

function New-ReleaseBlockers {
    param(
        [object[]]$Entries,
        [string]$SourceJson,
        [string]$SourceJsonDisplay
    )

    $blockers = New-Object 'System.Collections.Generic.List[object]'
    $pendingEntries = @($Entries | Where-Object { $_.calibration_outcome -eq "pending" })
    foreach ($entry in $pendingEntries) {
        $blockers.Add([ordered]@{
            id = Get-ScopedGovernanceId -BaseId "schema_patch_confidence_calibration.pending_schema_approvals" -Entry $entry -AffectedCount $pendingEntries.Count
            source = "schema_patch_confidence_calibration"
            severity = "error"
            status = "pending_review"
            action = "resolve_pending_schema_approvals"
            message = "Schema patch confidence calibration still contains pending approval outcome(s)."
            pending_count = 1
            project_id = Get-JsonString -Object $entry -Name "project_id"
            template_name = Get-JsonString -Object $entry -Name "template_name"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
            source_schema = $calibrationSchema
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
        }) | Out-Null
    }

    $invalidEntries = @($Entries | Where-Object { $_.calibration_outcome -eq "invalid_result" })
    foreach ($entry in $invalidEntries) {
        $blockers.Add([ordered]@{
            id = Get-ScopedGovernanceId -BaseId "schema_patch_confidence_calibration.invalid_approval_records" -Entry $entry -AffectedCount $invalidEntries.Count
            source = "schema_patch_confidence_calibration"
            severity = "error"
            status = "blocked"
            action = "fix_invalid_approval_records"
            message = "Schema patch confidence calibration contains invalid approval record(s)."
            invalid_result_count = 1
            project_id = Get-JsonString -Object $entry -Name "project_id"
            template_name = Get-JsonString -Object $entry -Name "template_name"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
            source_schema = $calibrationSchema
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
        }) | Out-Null
    }

    return @($blockers.ToArray())
}

function New-Warnings {
    param(
        [object[]]$Entries,
        [object[]]$InputSummaries,
        [string]$SourceJson,
        [string]$SourceJsonDisplay
    )

    $warnings = New-Object 'System.Collections.Generic.List[object]'
    foreach ($inputSummary in @($InputSummaries | Where-Object { $_.status -eq "failed" })) {
        $warnings.Add([ordered]@{
            id = "schema_patch_confidence_calibration.source_json_read_failed"
            action = "review_schema_patch_confidence_calibration_evidence"
            message = Get-JsonString -Object $inputSummary -Name "error" -DefaultValue "Input JSON could not be read."
            project_id = ""
            template_name = ""
            candidate_type = ""
            source_schema = $calibrationSchema
            source_json = Get-JsonString -Object $inputSummary -Name "path"
            source_json_display = Get-JsonString -Object $inputSummary -Name "path_display"
        }) | Out-Null
    }

    $unscoredEntries = @($Entries | Where-Object { $_.calibration_bucket -eq "unscored" })
    foreach ($entry in $unscoredEntries) {
        $warnings.Add([ordered]@{
            id = Get-ScopedGovernanceId -BaseId "schema_patch_confidence_calibration.unscored_candidates" -Entry $entry -AffectedCount $unscoredEntries.Count
            action = "add_explicit_confidence_metadata"
            message = "Some schema patch candidates do not carry explicit confidence metadata."
            unscored_count = 1
            project_id = Get-JsonString -Object $entry -Name "project_id"
            template_name = Get-JsonString -Object $entry -Name "template_name"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
            source_schema = $calibrationSchema
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
        }) | Out-Null
    }

    return @($warnings.ToArray())
}

function New-ActionItems {
    param(
        [object[]]$Recommendations,
        [string]$SourceJson,
        [string]$SourceJsonDisplay
    )

    return @(
        foreach ($recommendation in @($Recommendations)) {
            [ordered]@{
                id = Get-JsonString -Object $recommendation -Name "id" -DefaultValue "schema_patch_confidence_calibration_action"
                action = Get-JsonString -Object $recommendation -Name "id" -DefaultValue "review_schema_patch_confidence_calibration"
                title = Get-JsonString -Object $recommendation -Name "recommendation"
                project_id = Get-JsonString -Object $recommendation -Name "project_id"
                template_name = Get-JsonString -Object $recommendation -Name "template_name"
                candidate_type = Get-JsonString -Object $recommendation -Name "candidate_type"
                candidate_name = Get-JsonString -Object $recommendation -Name "candidate_name"
                open_command = $calibrationOpenCommand
                source_schema = $calibrationSchema
                source_json = $SourceJson
                source_json_display = $SourceJsonDisplay
            }
        }
    )
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Schema Patch Confidence Calibration Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Confidence source: ``$($Summary.confidence_source)``") | Out-Null
    $lines.Add("- Runs: ``$($Summary.run_count)``") | Out-Null
    $lines.Add("- Entries: ``$($Summary.entry_count)``") | Out-Null
    $lines.Add("- Recommended min confidence: ``$($Summary.recommended_min_confidence)``") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("## Confidence Buckets") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($bucket in @($Summary.confidence_buckets)) {
        $lines.Add("- ``$($bucket.name)``: candidates=$($bucket.candidate_count), approved=$($bucket.approved_count), pending=$($bucket.pending_count), rejected=$($bucket.rejected_count), invalid=$($bucket.invalid_result_count)") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Approval Outcomes") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($item in @($Summary.approval_outcome_summary)) {
        $lines.Add("- ``$($item.outcome)``: $($item.count)") | Out-Null
    }
    if (@($Summary.approval_outcome_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Project Templates") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.template_scope_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.template_scope_summary)) {
            $lines.Add("- ``$($item.template_scope)``: $($item.count)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Candidate Types") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.candidate_type_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.candidate_type_summary)) {
            $lines.Add("- ``$($item.candidate_type)``: $($item.count)") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.id)``: project=``$($blocker.project_id)`` template=``$($blocker.template_name)`` candidate=``$($blocker.candidate_type)`` action=``$($blocker.action)`` schema=``$($blocker.source_schema)`` source_json_display=``$($blocker.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - message: $($blocker.message)") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add("- ``$($warning.id)``: project=``$($warning.project_id)`` template=``$($warning.template_name)`` candidate=``$($warning.candidate_type)`` action=``$($warning.action)`` schema=``$($warning.source_schema)`` source_json_display=``$($warning.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.message)) {
                $lines.Add("  - message: $($warning.message)") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` schema=``$($item.source_schema)`` source_json_display=``$($item.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Recommendations") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.recommendations).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($recommendation in @($Summary.recommendations)) {
            $lines.Add("- ``$($recommendation.id)``: $($recommendation.recommendation)") | Out-Null
        }
    }
    return @($lines)
}

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "schema_patch_confidence_calibration.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) schema approval summary file(s)"

$entries = New-Object 'System.Collections.Generic.List[object]'
$inputSummaries = New-Object 'System.Collections.Generic.List[object]'

foreach ($path in @($inputPaths)) {
    $status = "loaded"
    $errorMessage = ""
    $beforeCount = $entries.Count
    try {
        $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
        if ((Get-JsonString -Object $summary -Name "schema") -eq "featherdoc.project_template_schema_approval_history.v1" -or
            $null -ne (Get-JsonProperty -Object $summary -Name "entry_histories")) {
            Add-EntriesFromHistory -Path $path -Summary $summary -Entries $entries
        } else {
            Add-EntriesFromSmokeSummary -Path $path -Summary $summary -Entries $entries
        }
    } catch {
        $status = "failed"
        $errorMessage = $_.Exception.Message
    }

    $inputSummaries.Add([ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        status = $status
        extracted_entry_count = ($entries.Count - $beforeCount)
        error = $errorMessage
    }) | Out-Null
}

$entryArray = @($entries.ToArray())
$recommendedMinConfidence = Get-RecommendedMinConfidence -Entries $entryArray
$pendingCount = @($entryArray | Where-Object { $_.calibration_outcome -eq "pending" }).Count
$invalidCount = @($entryArray | Where-Object { $_.calibration_outcome -eq "invalid_result" }).Count
$sourceFailureCount = @($inputSummaries.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$summaryDisplayPath = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
$recommendations = @(New-Recommendations -Entries $entryArray -RecommendedMinConfidence $recommendedMinConfidence)
$releaseBlockers = @(New-ReleaseBlockers -Entries $entryArray -SourceJson $summaryPath -SourceJsonDisplay $summaryDisplayPath)
$warnings = @(New-Warnings -Entries $entryArray -InputSummaries $inputSummaries.ToArray() -SourceJson $summaryPath -SourceJsonDisplay $summaryDisplayPath)
$actionItems = @(New-ActionItems -Recommendations $recommendations -SourceJson $summaryPath -SourceJsonDisplay $summaryDisplayPath)
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($invalidCount -gt 0) {
    "blocked"
} elseif ($pendingCount -gt 0) {
    "pending_review"
} else {
    "ready"
}

$summaryObject = [ordered]@{
    schema = $calibrationSchema
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    output_dir = $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = $summaryDisplayPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    input_summaries = @($inputSummaries.ToArray())
    input_summary_count = $inputSummaries.Count
    source_failure_count = $sourceFailureCount
    run_count = $inputSummaries.Count
    entry_count = $entryArray.Count
    confidence_source = Get-ConfidenceSource -Entries $entryArray
    recommended_min_confidence = $recommendedMinConfidence
    gate_status_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "status" -OutputName "status")
    approval_outcome_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "calibration_outcome" -OutputName "outcome")
    confidence_buckets = @(New-BucketSummary -Entries $entryArray)
    reason_code_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "reason_code" -OutputName "reason_code")
    project_count = @($entryArray | ForEach-Object { Get-JsonString -Object $_ -Name "project_id" } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique).Count
    template_count = @($entryArray | ForEach-Object { Get-JsonString -Object $_ -Name "template_scope" } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique).Count
    project_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "project_id" -OutputName "project_id")
    template_scope_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "template_scope" -OutputName "template_scope")
    candidate_type_summary = @(New-GroupSummary -Entries $entryArray -PropertyName "candidate_type" -OutputName "candidate_type")
    entries = $entryArray
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers)
    warning_count = $warnings.Count
    warnings = @($warnings)
    action_item_count = $actionItems.Count
    action_items = @($actionItems)
    recommendations = @($recommendations)
}

($summaryObject | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summaryObject) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnPending -and (($pendingCount + $invalidCount) -gt 0)) { exit 1 }
