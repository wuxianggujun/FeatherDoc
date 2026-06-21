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

function Expand-CalibrationArgumentList {
    param([string[]]$Values)

    return @(
        foreach ($value in @($Values)) {
            if ([string]::IsNullOrWhiteSpace([string]$value)) {
                continue
            }
            foreach ($part in ([string]$value -split ",")) {
                $trimmed = $part.Trim()
                if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                    $trimmed
                }
            }
        }
    )
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-CalibrationArgumentList -Values $ExplicitPaths)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
        }
    }

    $scanRoots = @(Expand-CalibrationArgumentList -Values $Roots | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
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

function Test-SchemaPatchReviewHasCandidateChanges {
    param($Review)

    if ((Get-JsonBool -Object $Review -Name "changed")) {
        return $true
    }

    $operationCount = 0
    foreach ($name in @(
            "upsert_slot_count",
            "remove_target_count",
            "remove_slot_count",
            "rename_slot_count",
            "update_slot_count",
            "inserted_slots",
            "replaced_slots"
        )) {
        $operationCount += Get-JsonInt -Object $Review -Name $name
    }
    if ($operationCount -gt 0) {
        return $true
    }

    $baselineSlotCount = Get-NullableJsonInt -Object $Review -Name "baseline_slot_count"
    $generatedSlotCount = Get-NullableJsonInt -Object $Review -Name "generated_slot_count"
    return ($null -ne $baselineSlotCount -and
        $null -ne $generatedSlotCount -and
        $baselineSlotCount -ne $generatedSlotCount)
}

function Test-SchemaPatchApprovalRequiresCalibration {
    param($Review, $Approval)

    if (Test-SchemaPatchReviewHasCandidateChanges -Review $Review) {
        return $true
    }

    if ((Get-JsonBool -Object $Approval -Name "required") -or
        (Get-JsonBool -Object $Approval -Name "pending") -or
        (Get-JsonBool -Object $Approval -Name "approved") -or
        (Get-JsonInt -Object $Approval -Name "compliance_issue_count") -gt 0) {
        return $true
    }

    $status = Get-JsonString -Object $Approval -Name "status"
    $decision = Get-JsonString -Object $Approval -Name "decision"
    return ($status -in @("pending_review", "approved", "rejected", "needs_changes", "blocked", "invalid_result") -or
        $decision -in @("pending", "approved", "rejected", "needs_changes"))
}

function New-EntryFromReviewAndApproval {
    param(
        [string]$SummaryJson,
        [string]$Name,
        $Review,
        $Approval,
        [string]$ProjectId = "",
        [string]$TemplateName = "",
        [string]$BusinessDocumentType = "",
        [string]$CorpusRole = ""
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
    $resolvedBusinessDocumentType = Get-JsonString -Object $Approval -Name "business_document_type" -DefaultValue (Get-JsonString -Object $Review -Name "business_document_type" -DefaultValue $BusinessDocumentType)
    $resolvedCorpusRole = Get-JsonString -Object $Approval -Name "corpus_role" -DefaultValue (Get-JsonString -Object $Review -Name "corpus_role" -DefaultValue $CorpusRole)
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
        business_document_type = $resolvedBusinessDocumentType
        corpus_role = $resolvedCorpusRole
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
    $sourceEntriesByName = @{}
    foreach ($entry in @(Get-JsonArray -Object $Summary -Name "entries")) {
        $entryName = Get-JsonString -Object $entry -Name "name"
        if ([string]::IsNullOrWhiteSpace($entryName)) {
            continue
        }
        $sourceEntriesByName[$entryName.ToLowerInvariant()] = $entry
    }
    $summaryProjectId = Get-JsonString -Object $Summary -Name "project_id" -DefaultValue $ProjectId
    $summaryTemplateName = Get-JsonString -Object $Summary -Name "template_name" -DefaultValue $TemplateName

    if ($reviews.Count -eq 0 -and $approvals.Count -eq 0) {
        return
    }

    if ($approvals.Count -eq 0) {
        foreach ($review in $reviews) {
            if (-not (Test-SchemaPatchReviewHasCandidateChanges -Review $review)) {
                continue
            }
            $reviewName = Get-JsonString -Object $review -Name "name" -DefaultValue "schema_patch_review"
            $sourceEntry = $null
            if ($sourceEntriesByName.ContainsKey($reviewName.ToLowerInvariant())) {
                $sourceEntry = $sourceEntriesByName[$reviewName.ToLowerInvariant()]
            }
            $Entries.Add((New-EntryFromReviewAndApproval `
                    -SummaryJson $Path `
                    -Name $reviewName `
                    -Review $review `
                    -Approval ([ordered]@{ status = "unknown"; decision = "unknown" }) `
                    -ProjectId $summaryProjectId `
                    -TemplateName $summaryTemplateName `
                    -BusinessDocumentType (Get-JsonString -Object $sourceEntry -Name "business_document_type") `
                    -CorpusRole (Get-JsonString -Object $sourceEntry -Name "corpus_role"))) | Out-Null
        }
        return
    }

    foreach ($approval in $approvals) {
        $name = Get-JsonString -Object $approval -Name "name" -DefaultValue "schema_patch_approval"
        $sourceEntry = $null
        if ($sourceEntriesByName.ContainsKey($name.ToLowerInvariant())) {
            $sourceEntry = $sourceEntriesByName[$name.ToLowerInvariant()]
        }
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
        if (-not (Test-SchemaPatchApprovalRequiresCalibration -Review $matchingReview -Approval $approval)) {
            continue
        }
        $Entries.Add((New-EntryFromReviewAndApproval `
                -SummaryJson $Path `
                -Name $name `
                -Review $matchingReview `
                -Approval $approval `
                -ProjectId $summaryProjectId `
                -TemplateName $summaryTemplateName `
                -BusinessDocumentType (Get-JsonString -Object $sourceEntry -Name "business_document_type") `
                -CorpusRole (Get-JsonString -Object $sourceEntry -Name "corpus_role"))) | Out-Null
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

function Test-BusinessTemplateSourceMetadataMissing {
    param($Entry)

    return ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $Entry -Name "project_id")) -or
        [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Entry -Name "template_name")) -or
        [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Entry -Name "summary_json")))
}

function Test-BusinessDocumentTypeMetadataMissing {
    param($Entry)

    return [string]::IsNullOrWhiteSpace((Get-JsonString -Object $Entry -Name "business_document_type"))
}

function New-BusinessTemplateSourceStatus {
    param($Entry)

    $projectId = Get-JsonString -Object $Entry -Name "project_id"
    $templateName = Get-JsonString -Object $Entry -Name "template_name"
    $summaryJson = Get-JsonString -Object $Entry -Name "summary_json"
    $businessDocumentType = Get-JsonString -Object $Entry -Name "business_document_type"

    return [ordered]@{
        missing_project_id = [string]::IsNullOrWhiteSpace($projectId)
        missing_template_name = [string]::IsNullOrWhiteSpace($templateName)
        missing_summary_json = [string]::IsNullOrWhiteSpace($summaryJson)
        missing_business_document_type = [string]::IsNullOrWhiteSpace($businessDocumentType)
    }
}

function New-BusinessTemplateCorpusSummary {
    param([object[]]$Entries, [string]$RepoRoot)

    $entryArray = @($Entries)
    $missingEntries = @($entryArray | Where-Object { Test-BusinessTemplateSourceMetadataMissing -Entry $_ })
    $missingBusinessDocumentTypeEntries = @($entryArray | Where-Object { Test-BusinessDocumentTypeMetadataMissing -Entry $_ })
    $projectIds = @($entryArray |
        ForEach-Object { Get-JsonString -Object $_ -Name "project_id" } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique)
    $templateScopes = @($entryArray |
        ForEach-Object { Get-JsonString -Object $_ -Name "template_scope" } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique)
    $sourceJsons = @($entryArray |
        ForEach-Object { Get-JsonString -Object $_ -Name "summary_json" } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique)
    $sourceJsonDisplays = @($sourceJsons | ForEach-Object { Get-DisplayPath -RepoRoot $RepoRoot -Path $_ })

    $templateSources = @(
        foreach ($scope in $templateScopes) {
            $scopeEntries = @($entryArray | Where-Object { (Get-JsonString -Object $_ -Name "template_scope") -eq $scope })
            if ($scopeEntries.Count -eq 0) { continue }

            $scopeSourceJsons = @($scopeEntries |
                ForEach-Object { Get-JsonString -Object $_ -Name "summary_json" } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
                Sort-Object -Unique)
            $scopeBusinessDocumentTypes = @($scopeEntries |
                ForEach-Object { Get-JsonString -Object $_ -Name "business_document_type" } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
                Sort-Object -Unique)
            $scopeCorpusRoles = @($scopeEntries |
                ForEach-Object { Get-JsonString -Object $_ -Name "corpus_role" } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
                Sort-Object -Unique)
            [ordered]@{
                project_id = Get-JsonString -Object $scopeEntries[0] -Name "project_id"
                template_name = Get-JsonString -Object $scopeEntries[0] -Name "template_name"
                template_scope = $scope
                candidate_count = $scopeEntries.Count
                business_document_types = @($scopeBusinessDocumentTypes)
                corpus_roles = @($scopeCorpusRoles)
                candidate_types = @($scopeEntries |
                    ForEach-Object { Get-JsonString -Object $_ -Name "candidate_type" } |
                    Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
                    Sort-Object -Unique)
                source_json_count = $scopeSourceJsons.Count
                source_json_displays = @($scopeSourceJsons | ForEach-Object { Get-DisplayPath -RepoRoot $RepoRoot -Path $_ })
            }
        }
    )

    $businessDocumentTypes = @($entryArray |
        ForEach-Object { Get-JsonString -Object $_ -Name "business_document_type" } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique)
    $corpusRoles = @($entryArray |
        ForEach-Object { Get-JsonString -Object $_ -Name "corpus_role" } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique)
    $missingSourceEntries = @(
        foreach ($entry in $missingEntries) {
            $status = New-BusinessTemplateSourceStatus -Entry $entry
            $summaryJson = Get-JsonString -Object $entry -Name "summary_json"
            [ordered]@{
                project_id = Get-JsonString -Object $entry -Name "project_id"
                template_name = Get-JsonString -Object $entry -Name "template_name"
                template_scope = Get-JsonString -Object $entry -Name "template_scope"
                business_document_type = Get-JsonString -Object $entry -Name "business_document_type"
                candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
                candidate_name = Get-JsonString -Object $entry -Name "name"
                source_json_display = if ([string]::IsNullOrWhiteSpace($summaryJson)) { "" } else { Get-DisplayPath -RepoRoot $RepoRoot -Path $summaryJson }
                missing_project_id = $status.missing_project_id
                missing_template_name = $status.missing_template_name
                missing_summary_json = $status.missing_summary_json
                missing_business_document_type = $status.missing_business_document_type
            }
        }
    )

    return [ordered]@{
        entry_count = $entryArray.Count
        traced_entry_count = ($entryArray.Count - $missingEntries.Count)
        project_count = $projectIds.Count
        template_count = $templateScopes.Count
        business_document_type_count = $businessDocumentTypes.Count
        business_document_types = @($businessDocumentTypes)
        corpus_role_count = $corpusRoles.Count
        corpus_roles = @($corpusRoles)
        source_json_count = $sourceJsons.Count
        source_json_displays = @($sourceJsonDisplays)
        missing_source_metadata_count = $missingEntries.Count
        missing_project_id_count = @($entryArray | Where-Object { [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "project_id")) }).Count
        missing_template_name_count = @($entryArray | Where-Object { [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "template_name")) }).Count
        missing_summary_json_count = @($entryArray | Where-Object { [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "summary_json")) }).Count
        missing_business_document_type_count = $missingBusinessDocumentTypeEntries.Count
        template_sources = @($templateSources)
        missing_source_entries = @($missingSourceEntries)
        missing_business_document_type_entries = @($missingBusinessDocumentTypeEntries)
    }
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
            template_scope = Get-JsonString -Object $entry -Name "template_scope"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
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
    $missingSourceEntries = @($Entries | Where-Object { Test-BusinessTemplateSourceMetadataMissing -Entry $_ })
    $missingBusinessDocumentTypeEntries = @($Entries | Where-Object { Test-BusinessDocumentTypeMetadataMissing -Entry $_ })

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
    Add-ScopedRecommendations `
        -Recommendations $recommendations `
        -Entries $missingSourceEntries `
        -BaseId "add_business_template_source_metadata" `
        -Priority "medium" `
        -Recommendation "Add project_id, template_name, and source summary metadata before using this corpus for threshold tuning." `
        -CountName "missing_source_metadata_count"
    Add-ScopedRecommendations `
        -Recommendations $recommendations `
        -Entries $missingBusinessDocumentTypeEntries `
        -BaseId "add_business_template_document_type_metadata" `
        -Priority "medium" `
        -Recommendation "Add business_document_type metadata before using this corpus for threshold tuning." `
        -CountName "missing_business_document_type_count"

    return @($recommendations.ToArray())
}

function New-ReleaseBlockers {
    param(
        [object[]]$Entries,
        [string]$SourceReport,
        [string]$SourceReportDisplay,
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
            template_scope = Get-JsonString -Object $entry -Name "template_scope"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
            source_schema = $calibrationSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
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
            template_scope = Get-JsonString -Object $entry -Name "template_scope"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
            source_schema = $calibrationSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
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
        [string]$SourceReport,
        [string]$SourceReportDisplay,
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
            template_scope = ""
            candidate_type = ""
            candidate_name = ""
            schema_update_candidate = ""
            source_schema = $calibrationSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
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
            template_scope = Get-JsonString -Object $entry -Name "template_scope"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
            source_schema = $calibrationSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
        }) | Out-Null
    }

    $missingSourceEntries = @($Entries | Where-Object { Test-BusinessTemplateSourceMetadataMissing -Entry $_ })
    foreach ($entry in $missingSourceEntries) {
        $sourceStatus = New-BusinessTemplateSourceStatus -Entry $entry
        $warnings.Add([ordered]@{
            id = Get-ScopedGovernanceId -BaseId "schema_patch_confidence_calibration.missing_business_template_source_metadata" -Entry $entry -AffectedCount $missingSourceEntries.Count
            action = "add_business_template_source_metadata"
            message = "Some schema patch candidates are missing business template source metadata."
            missing_source_metadata_count = 1
            missing_project_id = $sourceStatus.missing_project_id
            missing_template_name = $sourceStatus.missing_template_name
            missing_summary_json = $sourceStatus.missing_summary_json
            project_id = Get-JsonString -Object $entry -Name "project_id"
            template_name = Get-JsonString -Object $entry -Name "template_name"
            template_scope = Get-JsonString -Object $entry -Name "template_scope"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
            source_schema = $calibrationSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
        }) | Out-Null
    }

    $missingBusinessDocumentTypeEntries = @($Entries | Where-Object { Test-BusinessDocumentTypeMetadataMissing -Entry $_ })
    foreach ($entry in $missingBusinessDocumentTypeEntries) {
        $warnings.Add([ordered]@{
            id = Get-ScopedGovernanceId -BaseId "schema_patch_confidence_calibration.missing_business_document_type_metadata" -Entry $entry -AffectedCount $missingBusinessDocumentTypeEntries.Count
            action = "add_business_template_document_type_metadata"
            message = "Some schema patch candidates are missing business document type metadata."
            missing_business_document_type_count = 1
            project_id = Get-JsonString -Object $entry -Name "project_id"
            template_name = Get-JsonString -Object $entry -Name "template_name"
            template_scope = Get-JsonString -Object $entry -Name "template_scope"
            candidate_type = Get-JsonString -Object $entry -Name "candidate_type"
            candidate_name = Get-JsonString -Object $entry -Name "name"
            schema_update_candidate = Get-JsonString -Object $entry -Name "schema_update_candidate"
            source_schema = $calibrationSchema
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
        }) | Out-Null
    }

    return @($warnings.ToArray())
}

function New-ActionItems {
    param(
        [object[]]$Recommendations,
        [string]$SourceReport,
        [string]$SourceReportDisplay,
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
                template_scope = Get-JsonString -Object $recommendation -Name "template_scope"
                candidate_type = Get-JsonString -Object $recommendation -Name "candidate_type"
                candidate_name = Get-JsonString -Object $recommendation -Name "candidate_name"
                schema_update_candidate = Get-JsonString -Object $recommendation -Name "schema_update_candidate"
                open_command = $calibrationOpenCommand
                source_schema = $calibrationSchema
                source_report = $SourceReport
                source_report_display = $SourceReportDisplay
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
    $lines.Add("## Business Template Corpus") | Out-Null
    $lines.Add("") | Out-Null
    if ($null -eq $Summary.business_template_corpus_summary) {
        $lines.Add("- none") | Out-Null
    } else {
        $corpus = $Summary.business_template_corpus_summary
        $businessDocumentTypes = @($corpus.business_document_types) -join ","
        $corpusRoles = @($corpus.corpus_roles) -join ","
        if ([string]::IsNullOrWhiteSpace($businessDocumentTypes)) {
            $businessDocumentTypes = "(none)"
        }
        if ([string]::IsNullOrWhiteSpace($corpusRoles)) {
            $corpusRoles = "(none)"
        }
        $lines.Add("- projects=$($corpus.project_count), templates=$($corpus.template_count), source_jsons=$($corpus.source_json_count), traced_entries=$($corpus.traced_entry_count), missing_source_metadata=$($corpus.missing_source_metadata_count), missing_business_document_types=$($corpus.missing_business_document_type_count), business_document_types=$($corpus.business_document_type_count), corpus_roles=$($corpus.corpus_role_count)") | Out-Null
        $lines.Add("- business_document_types: $businessDocumentTypes") | Out-Null
        $lines.Add("- corpus_roles: $corpusRoles") | Out-Null
        if (@($corpus.missing_business_document_type_entries).Count -gt 0) {
            $lines.Add("- missing_business_document_type_entries:") | Out-Null
            foreach ($entry in @($corpus.missing_business_document_type_entries)) {
                $entryParts = New-Object 'System.Collections.Generic.List[string]'
                $scope = Get-JsonString -Object $entry -Name "template_scope"
                $name = Get-JsonString -Object $entry -Name "candidate_name"
                $projectId = Get-JsonString -Object $entry -Name "project_id"
                $templateName = Get-JsonString -Object $entry -Name "template_name"
                if (-not [string]::IsNullOrWhiteSpace($scope)) { $entryParts.Add("scope=$scope") | Out-Null }
                if (-not [string]::IsNullOrWhiteSpace($name)) { $entryParts.Add("name=$name") | Out-Null }
                if (-not [string]::IsNullOrWhiteSpace($projectId)) { $entryParts.Add("project_id=$projectId") | Out-Null }
                if (-not [string]::IsNullOrWhiteSpace($templateName)) { $entryParts.Add("template_name=$templateName") | Out-Null }
                $lines.Add("  - $(@($entryParts.ToArray()) -join ', ')") | Out-Null
            }
        }
        if (@($corpus.template_sources).Count -eq 0) {
            $lines.Add("- template_sources: none") | Out-Null
        } else {
            foreach ($source in @($corpus.template_sources)) {
                $candidateTypes = (@($source.candidate_types) -join ",")
                $businessDocumentTypeText = (@($source.business_document_types) -join ",")
                $corpusRoleText = (@($source.corpus_roles) -join ",")
                if ([string]::IsNullOrWhiteSpace($businessDocumentTypeText)) {
                    $businessDocumentTypeText = "(none)"
                }
                if ([string]::IsNullOrWhiteSpace($corpusRoleText)) {
                    $corpusRoleText = "(none)"
                }
                $sourceDisplays = (@($source.source_json_displays) -join ",")
                $lines.Add("- ``$($source.template_scope)``: candidates=$($source.candidate_count), business_document_types=``$businessDocumentTypeText``, corpus_roles=``$corpusRoleText``, candidate_types=``$candidateTypes``, source_json_displays=``$sourceDisplays``") | Out-Null
            }
        }
        if (@($corpus.missing_source_entries).Count -gt 0) {
            $lines.Add("- missing_source_entries=$(@($corpus.missing_source_entries).Count)") | Out-Null
        }
    }
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
            $lines.Add("- ``$($blocker.id)``: project=``$($blocker.project_id)`` template=``$($blocker.template_name)`` candidate=``$($blocker.candidate_type)`` action=``$($blocker.action)`` schema=``$($blocker.source_schema)`` source_report_display=``$($blocker.source_report_display)`` source_json_display=``$($blocker.source_json_display)``") | Out-Null
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
            $lines.Add("- ``$($warning.id)``: project=``$($warning.project_id)`` template=``$($warning.template_name)`` candidate=``$($warning.candidate_type)`` action=``$($warning.action)`` schema=``$($warning.source_schema)`` source_report_display=``$($warning.source_report_display)`` source_json_display=``$($warning.source_json_display)``") | Out-Null
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
            $lines.Add("- ``$($item.id)``: project=``$($item.project_id)`` template=``$($item.template_name)`` candidate=``$($item.candidate_type)`` action=``$($item.action)`` schema=``$($item.source_schema)`` source_report_display=``$($item.source_report_display)`` source_json_display=``$($item.source_json_display)``") | Out-Null
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
