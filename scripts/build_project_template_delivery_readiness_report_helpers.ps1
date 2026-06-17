function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-delivery-readiness] $Message"
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
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
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
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-TemplateSchemaArgumentList -Values $ExplicitPaths)) {
        if (-not [string]::IsNullOrWhiteSpace($path)) {
            $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path)) | Out-Null
        }
    }

    $scanRoots = @(Expand-TemplateSchemaArgumentList -Values $Roots)
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @(
            "output/project-template-onboarding-governance",
            "output/project-template-onboarding",
            "output/project-template-smoke-onboarding-plan",
            "output/project-template-smoke",
            "output/project-template-smoke-manifest",
            "output/project-template-schema-approval-history"
        )
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }

        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                Where-Object {
                    $_.Name -in @("summary.json", "onboarding_summary.json", "plan.json", "history.json", "schema-approval-history.json")
                } |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Sort-Object -Unique)
}

function Get-EvidenceKind {
    param($Json, [string]$Path)

    $schema = Get-JsonString -Object $Json -Name "schema"
    if ($schema -eq "featherdoc.project_template_delivery_readiness_report.v1") {
        return "project_template_delivery_readiness_report"
    }
    if ($schema -eq "featherdoc.project_template_onboarding_governance_report.v1") {
        return "onboarding_governance_report"
    }
    if ($schema -eq "featherdoc.project_template_schema_approval_history.v1") {
        return "schema_approval_history"
    }
    if ($schema -eq "featherdoc.project_template_smoke_manifest_description.v1") {
        return "project_template_smoke_manifest_description"
    }
    if ($schema -eq $onboardingPlanSchema) {
        return "onboarding_plan"
    }
    if ($schema -eq $onboardingSummarySchema) {
        return "onboarding_summary"
    }
    if ($schema -eq $projectTemplateSmokeSummarySchema) {
        return "project_template_smoke_summary"
    }
    if ($null -ne (Get-JsonProperty -Object $Json -Name "manifest_path") -and
        $null -ne (Get-JsonProperty -Object $Json -Name "entry_count") -and
        $null -ne (Get-JsonProperty -Object $Json -Name "overall_status") -and
        $null -ne (Get-JsonProperty -Object $Json -Name "entries")) {
        return "project_template_smoke_summary"
    }
    if ($null -ne (Get-JsonProperty -Object $Json -Name "onboarding_entry_count") -and
        $null -ne (Get-JsonProperty -Object $Json -Name "entries")) {
        return "onboarding_plan"
    }

    $leaf = [System.IO.Path]::GetFileName($Path)
    if ($leaf -eq "onboarding_summary.json" -or
        $null -ne (Get-JsonProperty -Object $Json -Name "schema_approval_state")) {
        return "onboarding_summary"
    }
    return "unknown"
}

function New-ProjectTemplateSmokeSummaryMissingWarning {
    param(
        [string]$RepoRoot,
        [string]$SummaryPath,
        [object[]]$ManifestDescriptions
    )

    $latestDescription = @($ManifestDescriptions | Sort-Object @{ Expression = { Get-JsonString -Object $_ -Name "generated_at" } })[-1]
    $sourceJson = Get-JsonString -Object $latestDescription -Name "source_json" -DefaultValue $SummaryPath
    $sourceJsonDisplay = Get-JsonString -Object $latestDescription -Name "source_json_display" -DefaultValue (Get-DisplayPath -RepoRoot $RepoRoot -Path $sourceJson)
    $manifestPathDisplay = Get-JsonString -Object $latestDescription -Name "manifest_path_display"
    if ([string]::IsNullOrWhiteSpace($manifestPathDisplay)) {
        $manifestPathDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path (Get-JsonString -Object $latestDescription -Name "manifest_path")
    }

    $registeredTemplateCount = Get-JsonInt -Object $latestDescription -Name "registered_entry_count" -DefaultValue (Get-JsonInt -Object $latestDescription -Name "entry_count")
    $latestMissingEntryCount = Get-JsonInt -Object $latestDescription -Name "latest_missing_entry_count" -DefaultValue $registeredTemplateCount

    return [ordered]@{
        id = "project_template_smoke_summary_missing"
        action = "run_project_template_smoke_for_registered_manifest"
        repair_strategy = "run_project_template_smoke_for_registered_manifest"
        repair_hint = "A project-template-smoke manifest is registered, but no smoke/onboarding summary was loaded as release evidence. Run or restore the smoke summary before treating these templates as delivery-ready."
        command_template = "powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\run_project_template_smoke.ps1 -ManifestPath .\samples\project_template_smoke.manifest.json -OutputDir .\output\project-template-smoke -SkipBuild"
        source_schema = $deliveryReadinessSchema
        source_json = $sourceJson
        source_json_display = $sourceJsonDisplay
        source_report = $sourceJson
        source_report_display = $sourceJsonDisplay
        manifest_path = Get-JsonString -Object $latestDescription -Name "manifest_path"
        manifest_path_display = $manifestPathDisplay
        registered_template_count = $registeredTemplateCount
        business_template_corpus_count = Get-JsonInt -Object $latestDescription -Name "business_template_corpus_count"
        registered_business_template_corpus_count = Get-JsonInt -Object $latestDescription -Name "registered_business_template_corpus_count"
        planned_business_template_corpus_count = Get-JsonInt -Object $latestDescription -Name "planned_business_template_corpus_count"
        schema_validation_entry_count = Get-JsonInt -Object $latestDescription -Name "schema_validation_entry_count"
        schema_baseline_entry_count = Get-JsonInt -Object $latestDescription -Name "schema_baseline_entry_count"
        visual_smoke_entry_count = Get-JsonInt -Object $latestDescription -Name "visual_smoke_entry_count"
        latest_summary_available = Get-JsonBool -Object $latestDescription -Name "latest_summary_available"
        latest_missing_entry_count = $latestMissingEntryCount
        message = "Project-template-smoke manifest has registered entries, but no project-template smoke or onboarding readiness summary was loaded."
    }
}

function Add-SummaryGroup {
    param([object[]]$Items, [string]$PropertyName, [string]$OutputName)

    $groupItems = @(
        foreach ($item in @($Items)) {
            [pscustomobject]@{
                Value = Get-JsonString -Object $item -Name $PropertyName -DefaultValue "unknown"
            }
        }
    )

    return @(
        foreach ($group in @($groupItems | Group-Object Value |
            Sort-Object -Property @{ Expression = "Count"; Descending = $true }, @{ Expression = "Name"; Ascending = $true })) {
            $summary = [ordered]@{}
            $summary[$OutputName] = [string]$group.Name
            $summary["count"] = [int]$group.Count
            $summary
        }
    )
}

function Get-FirstJsonObjectValue {
    param([object[]]$Items, [string]$PropertyName)

    foreach ($item in @($Items)) {
        $value = Get-JsonProperty -Object $item -Name $PropertyName
        if ($null -ne $value) {
            return $value
        }
    }
    return $null
}

function Get-OnboardingGovernanceNextActionSummary {
    param([object[]]$Templates)

    $seen = @{}
    $items = New-Object 'System.Collections.Generic.List[object]'
    foreach ($template in @($Templates)) {
        foreach ($item in @(Get-JsonArray -Object $template -Name "onboarding_governance_next_action_summary")) {
            $key = @(
                (Get-JsonString -Object $item -Name "action" -DefaultValue "unknown"),
                (Get-JsonString -Object $item -Name "status" -DefaultValue "unknown"),
                (Get-JsonString -Object $item -Name "blocker_id"),
                (Get-JsonString -Object $item -Name "reason")
            ) -join ([string][char]31)
            if ($seen.ContainsKey($key)) {
                continue
            }
            $seen[$key] = $true
            $items.Add($item) | Out-Null
        }
    }

    return @($items.ToArray())
}

function Get-NextActionDisplay {
    param([AllowNull()]$NextAction)

    if ($null -eq $NextAction) {
        return ""
    }

    $parts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($field in @("action", "status", "blocker_id", "reason")) {
        $value = Get-JsonString -Object $NextAction -Name $field
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $parts.Add(("{0}={1}" -f $field, $value)) | Out-Null
        }
    }

    if ($parts.Count -gt 0) {
        return ($parts.ToArray() -join " ")
    }

    return [string]$NextAction
}

function New-ReleaseBlocker {
    param(
        [string]$Id,
        [string]$Source,
        [string]$Status,
        [string]$Action,
        [string]$Message,
        [string]$SourceSchema = $deliveryReadinessSchema,
        [string]$SourceJson = "",
        [string]$SourceJsonDisplay = "",
        [string]$SourceReport = "",
        [string]$SourceReportDisplay = ""
    )

    if ([string]::IsNullOrWhiteSpace($SourceReport)) {
        $SourceReport = $SourceJson
    }
    if ([string]::IsNullOrWhiteSpace($SourceReportDisplay)) {
        $SourceReportDisplay = $SourceJsonDisplay
    }

    return [ordered]@{
        id = $Id
        source = $Source
        source_schema = $SourceSchema
        source_json = $SourceJson
        source_json_display = $SourceJsonDisplay
        source_report = $SourceReport
        source_report_display = $SourceReportDisplay
        severity = "error"
        status = $Status
        action = $Action
        message = $Message
    }
}

function Get-DefaultSourceSchema {
    param([string]$SourceKind)

    if ($SourceKind -eq "onboarding_governance_report") {
        return $onboardingGovernanceSchema
    }
    if ($SourceKind -eq "onboarding_summary") {
        return $onboardingSummarySchema
    }
    if ($SourceKind -eq "onboarding_plan") {
        return $onboardingPlanSchema
    }
    if ($SourceKind -eq "project_template_smoke_summary") {
        return $projectTemplateSmokeSummarySchema
    }
    return $deliveryReadinessSchema
}

function New-ReadinessTemplate {
    param(
        [string]$RepoRoot,
        [string]$TemplateName,
        [string]$InputDocx,
        [string]$SourceKind,
        [string]$SourceJson,
        $SchemaApprovalState,
        [string]$OnboardingStatus = "",
        [object[]]$ReleaseBlockers = @(),
        [object[]]$ActionItems = @(),
        [object[]]$ManualReviewRecommendations = @()
    )

    $schemaStatus = Get-JsonString -Object $SchemaApprovalState -Name "status" -DefaultValue "not_evaluated"
    $gateStatus = Get-JsonString -Object $SchemaApprovalState -Name "gate_status" -DefaultValue $schemaStatus
    $schemaAction = Get-JsonString -Object $SchemaApprovalState -Name "action" -DefaultValue "run_project_template_smoke_then_review_schema_patch_approval"
    $releaseBlocked = Get-JsonBool -Object $SchemaApprovalState -Name "release_blocked" -DefaultValue ($schemaStatus -in @("not_evaluated", "pending_review", "blocked"))
    $blockers = @($ReleaseBlockers | Where-Object { $null -ne $_ })
    $items = @($ActionItems | Where-Object { $null -ne $_ })
    $recommendations = @($ManualReviewRecommendations | Where-Object { $null -ne $_ })
    if ($blockers.Count -eq 0 -and $releaseBlocked) {
        $blockerId = if ($schemaStatus -eq "not_evaluated") {
            "project_template_delivery_readiness.schema_approval_not_evaluated"
        } else {
            "project_template_delivery_readiness.schema_approval"
        }
        $blockers = @(
            New-ReleaseBlocker `
                -Id $blockerId `
                -Source $SourceKind `
                -Status $schemaStatus `
                -Action $schemaAction `
                -Message "Schema approval is not release-ready for this template."
        )
    }
    if ([string]::IsNullOrWhiteSpace($OnboardingStatus)) {
        $OnboardingStatus = if ($releaseBlocked) { "blocked" } else { "ready" }
    }

    return [ordered]@{
        template_name = $TemplateName
        input_docx = $InputDocx
        input_docx_display = if ([string]::IsNullOrWhiteSpace($InputDocx)) { "" } else { Get-DisplayPath -RepoRoot $RepoRoot -Path (Resolve-RepoPath -RepoRoot $RepoRoot -Path $InputDocx -AllowMissing) }
        onboarding_status = $OnboardingStatus
        schema_approval_status = $schemaStatus
        gate_status = $gateStatus
        schema_approval_action = $schemaAction
        release_blocked = $releaseBlocked
        release_ready = (-not $releaseBlocked)
        release_blockers = @($blockers)
        release_blocker_count = @($blockers).Count
        action_items = @($items)
        action_item_count = @($items).Count
        manual_review_recommendations = @($recommendations)
        manual_review_recommendation_count = @($recommendations).Count
        onboarding_governance_status = ""
        onboarding_governance_release_ready = ""
        onboarding_governance_schema_approval_status_summary = @()
        onboarding_governance_next_action = $null
        onboarding_governance_next_action_summary = @()
        onboarding_governance_next_action_group_count = 0
        onboarding_governance_source_report = ""
        onboarding_governance_source_report_display = ""
        onboarding_governance_source_json = ""
        onboarding_governance_source_json_display = ""
        schema_approval_state = $SchemaApprovalState
        schema_approval_history_required = Get-JsonBool -Object $SchemaApprovalState -Name "history_required" -DefaultValue $true
        schema_history_available = $false
        schema_history = [ordered]@{
            status = "not_available"
            latest_status = ""
            latest_decision = ""
            latest_action = ""
            run_count = 0
            blocked_run_count = 0
        }
        source_kind = $SourceKind
        source_json = $SourceJson
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $SourceJson
        source_report = $SourceJson
        source_report_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $SourceJson
    }
}

function Convert-OnboardingGovernanceToTemplates {
    param([string]$RepoRoot, [string]$Path, $Json)

    $onboardingGovernanceStatus = Get-JsonString -Object $Json -Name "status"
    $onboardingGovernanceReleaseReady = if ($null -ne (Get-JsonProperty -Object $Json -Name "release_ready")) {
        [string](Get-JsonBool -Object $Json -Name "release_ready")
    } else {
        ""
    }
    $onboardingGovernanceSchemaApprovalStatusSummary = @(Get-JsonArray -Object $Json -Name "schema_approval_status_summary")
    $onboardingGovernanceNextAction = Get-JsonProperty -Object $Json -Name "next_action"
    $onboardingGovernanceNextActionSummary = @(Get-JsonArray -Object $Json -Name "next_action_summary")
    $onboardingGovernanceNextActionGroupCount = Get-JsonInt -Object $Json -Name "next_action_group_count" -DefaultValue $onboardingGovernanceNextActionSummary.Count
    $onboardingGovernanceSourceDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path

    return @(
        foreach ($entry in @(Get-JsonArray -Object $Json -Name "entries")) {
            $state = Get-JsonProperty -Object $entry -Name "schema_approval_state"
            $name = Get-JsonString -Object $entry -Name "name" -DefaultValue "project-template"
            $template = New-ReadinessTemplate `
                -RepoRoot $RepoRoot `
                -TemplateName $name `
                -InputDocx (Get-JsonString -Object $entry -Name "input_docx") `
                -SourceKind "onboarding_governance_report" `
                -SourceJson $Path `
                -SchemaApprovalState $state `
                -ReleaseBlockers (Get-JsonArray -Object $entry -Name "release_blockers") `
                -ActionItems (Get-JsonArray -Object $entry -Name "action_items") `
                -ManualReviewRecommendations (Get-JsonArray -Object $entry -Name "manual_review_recommendations")
            $template["onboarding_governance_status"] = $onboardingGovernanceStatus
            $template["onboarding_governance_release_ready"] = $onboardingGovernanceReleaseReady
            $template["onboarding_governance_schema_approval_status_summary"] = @($onboardingGovernanceSchemaApprovalStatusSummary)
            $template["onboarding_governance_next_action"] = $onboardingGovernanceNextAction
            $template["onboarding_governance_next_action_summary"] = @($onboardingGovernanceNextActionSummary)
            $template["onboarding_governance_next_action_group_count"] = $onboardingGovernanceNextActionGroupCount
            $template["onboarding_governance_source_report"] = $Path
            $template["onboarding_governance_source_report_display"] = $onboardingGovernanceSourceDisplay
            $template["onboarding_governance_source_json"] = $Path
            $template["onboarding_governance_source_json_display"] = $onboardingGovernanceSourceDisplay
            $template
        }
    )
}

function Convert-OnboardingSummaryToTemplates {
    param([string]$RepoRoot, [string]$Path, $Json)

    $name = Get-JsonString -Object $Json -Name "template_name"
    if ([string]::IsNullOrWhiteSpace($name)) {
        $name = Split-Path -Leaf (Split-Path -Parent $Path)
    }

    return @(
        New-ReadinessTemplate `
            -RepoRoot $RepoRoot `
            -TemplateName $name `
            -InputDocx (Get-JsonString -Object $Json -Name "input_docx") `
            -SourceKind "onboarding_summary" `
            -SourceJson $Path `
            -SchemaApprovalState (Get-JsonProperty -Object $Json -Name "schema_approval_state") `
            -OnboardingStatus (Get-JsonString -Object $Json -Name "status") `
            -ReleaseBlockers (Get-JsonArray -Object $Json -Name "release_blockers") `
            -ActionItems (Get-JsonArray -Object $Json -Name "action_items") `
            -ManualReviewRecommendations (Get-JsonArray -Object $Json -Name "manual_review_recommendations")
    )
}

function Convert-OnboardingPlanToTemplates {
    param([string]$RepoRoot, [string]$Path, $Json)

    return @(
        foreach ($entry in @(Get-JsonArray -Object $Json -Name "entries")) {
            New-ReadinessTemplate `
                -RepoRoot $RepoRoot `
                -TemplateName (Get-JsonString -Object $entry -Name "name" -DefaultValue "project-template") `
                -InputDocx (Get-JsonString -Object $entry -Name "input_docx") `
                -SourceKind "onboarding_plan" `
                -SourceJson $Path `
                -SchemaApprovalState (Get-JsonProperty -Object $entry -Name "schema_approval_state") `
                -OnboardingStatus (Get-JsonString -Object $entry -Name "status") `
                -ReleaseBlockers (Get-JsonArray -Object $entry -Name "release_blockers") `
                -ActionItems (Get-JsonArray -Object $entry -Name "action_items") `
                -ManualReviewRecommendations (Get-JsonArray -Object $entry -Name "manual_review_recommendations")
        }
    )
}

function New-SchemaApprovalStateFromSmokeEntry {
    param($Entry, $SchemaPatchApproval = $null)

    $entryPassed = Get-JsonBool -Object $Entry -Name "passed"
    if ($null -ne $SchemaPatchApproval) {
        $status = Get-JsonString -Object $SchemaPatchApproval -Name "status" -DefaultValue "pending_review"
        $decision = Get-JsonString -Object $SchemaPatchApproval -Name "decision"
        $action = Get-JsonString -Object $SchemaPatchApproval -Name "action" -DefaultValue "review_schema_update_candidate"
        $complianceIssueCount = Get-JsonInt -Object $SchemaPatchApproval -Name "compliance_issue_count"
        if ($status -eq "approved" -or $decision -eq "approved") {
            $stateStatus = "approved"
            $gateStatus = "passed"
        } elseif ($status -eq "not_required" -or $decision -eq "not_required") {
            $stateStatus = "not_required"
            $gateStatus = "not_required"
            $action = "none"
        } elseif ($status -in @("rejected", "needs_changes", "invalid_result", "blocked") -or
            $decision -in @("rejected", "needs_changes") -or $complianceIssueCount -gt 0) {
            $stateStatus = "blocked"
            $gateStatus = "blocked"
            if ([string]::IsNullOrWhiteSpace($action) -or $action -eq "none") {
                $action = "fix_schema_patch_approval_result"
            }
        } else {
            $stateStatus = "pending_review"
            $gateStatus = "pending"
            if ([string]::IsNullOrWhiteSpace($action) -or $action -eq "none") {
                $action = "review_schema_update_candidate"
            }
        }
    } elseif ($entryPassed) {
        $stateStatus = "not_required"
        $gateStatus = "not_required"
        $action = "none"
    } else {
        $stateStatus = "blocked"
        $gateStatus = "blocked"
        $action = "review_project_template_smoke_failure"
    }

    $historyRequired = ($null -ne $SchemaPatchApproval)

    return [ordered]@{
        schema = "featherdoc.project_template_onboarding_schema_approval_state.v1"
        status = $stateStatus
        gate_status = $gateStatus
        release_blocked = ($stateStatus -in @("not_evaluated", "pending_review", "blocked"))
        history_required = $historyRequired
        smoke_summary_available = $true
        smoke_entry_status = Get-JsonString -Object $Entry -Name "status"
        smoke_entry_passed = $entryPassed
        action = $action
    }
}

function New-ProjectTemplateSmokeEntryBlocker {
    param(
        [string]$RepoRoot,
        [string]$Path,
        $Entry,
        $SchemaApprovalState
    )

    $issues = @(Get-JsonArray -Object $Entry -Name "issues" | ForEach-Object { [string]$_ })
    $message = if ($issues.Count -gt 0) {
        "Project-template smoke entry failed: " + ($issues -join "; ")
    } else {
        "Project-template smoke entry is not release-ready."
    }

    return New-ReleaseBlocker `
        -Id "project_template_smoke.entry_not_release_ready" `
        -Source "project_template_smoke_summary" `
        -Status (Get-JsonString -Object $SchemaApprovalState -Name "status" -DefaultValue "blocked") `
        -Action (Get-JsonString -Object $SchemaApprovalState -Name "action" -DefaultValue "review_project_template_smoke_failure") `
        -Message $message `
        -SourceSchema $projectTemplateSmokeSummarySchema `
        -SourceJson $Path `
        -SourceJsonDisplay (Get-DisplayPath -RepoRoot $RepoRoot -Path $Path) `
        -SourceReport $Path `
        -SourceReportDisplay (Get-DisplayPath -RepoRoot $RepoRoot -Path $Path)
}

function Convert-ProjectTemplateSmokeSummaryToTemplates {
    param([string]$RepoRoot, [string]$Path, $Json)

    return @(
        foreach ($entry in @(Get-JsonArray -Object $Json -Name "entries")) {
            $checks = Get-JsonProperty -Object $entry -Name "checks"
            $schemaBaseline = Get-JsonProperty -Object $checks -Name "schema_baseline"
            $schemaPatchApproval = Get-JsonProperty -Object $schemaBaseline -Name "schema_patch_approval"
            $schemaApprovalState = New-SchemaApprovalStateFromSmokeEntry `
                -Entry $entry `
                -SchemaPatchApproval $schemaPatchApproval
            $releaseBlockers = @()
            if (Get-JsonBool -Object $schemaApprovalState -Name "release_blocked") {
                $releaseBlockers = @(
                    New-ProjectTemplateSmokeEntryBlocker `
                        -RepoRoot $RepoRoot `
                        -Path $Path `
                        -Entry $entry `
                        -SchemaApprovalState $schemaApprovalState
                )
            }
            $defaultOnboardingStatus = if (Get-JsonBool -Object $entry -Name "passed") { "ready" } else { "blocked" }

            New-ReadinessTemplate `
                -RepoRoot $RepoRoot `
                -TemplateName (Get-JsonString -Object $entry -Name "name" -DefaultValue "project-template") `
                -InputDocx (Get-JsonString -Object $entry -Name "input_docx") `
                -SourceKind "project_template_smoke_summary" `
                -SourceJson $Path `
                -SchemaApprovalState $schemaApprovalState `
                -OnboardingStatus (Get-JsonString -Object $entry -Name "status" -DefaultValue $defaultOnboardingStatus) `
                -ReleaseBlockers $releaseBlockers
        }
    )
}

function Get-HistoryReadinessStatus {
    param($HistoryEntry)

    $latestStatus = Get-JsonString -Object $HistoryEntry -Name "latest_status"
    $latestDecision = Get-JsonString -Object $HistoryEntry -Name "latest_decision"
    if ($latestStatus -in @("invalid_result", "rejected", "needs_changes", "blocked") -or
        $latestDecision -in @("rejected", "needs_changes")) {
        return "blocked"
    }
    if ($latestStatus -eq "pending_review" -or $latestDecision -eq "pending") {
        return "pending_review"
    }
    if ($latestStatus -eq "approved" -or $latestDecision -eq "approved") {
        return "approved"
    }
    if ([string]::IsNullOrWhiteSpace($latestStatus) -and [string]::IsNullOrWhiteSpace($latestDecision)) {
        return "not_available"
    }
    return $latestStatus
}

function Add-HistoryToTemplates {
    param([object[]]$Templates, [object[]]$Histories, $Warnings)

    $historyByName = @{}
    foreach ($history in @($Histories)) {
        foreach ($entry in @(Get-JsonArray -Object $history -Name "entry_histories")) {
            $name = Get-JsonString -Object $entry -Name "name"
            if ([string]::IsNullOrWhiteSpace($name)) { continue }
            $historyByName[$name.ToLowerInvariant()] = $entry
        }
    }

    foreach ($template in @($Templates)) {
        $key = ([string]$template.template_name).ToLowerInvariant()
        if (-not $historyByName.ContainsKey($key)) {
            if (-not (Get-JsonBool -Object $template -Name "schema_approval_history_required" -DefaultValue $true)) {
                continue
            }
            $Warnings.Add([ordered]@{
                id = "schema_approval_history_missing_for_template"
                template_name = [string]$template.template_name
                action = "review_schema_approval_history"
                source_schema = $deliveryReadinessSchema
                source_json = [string]$template.source_json
                source_json_display = [string]$template.source_json_display
                source_report = [string]$template.source_report
                source_report_display = [string]$template.source_report_display
                message = "No schema approval history entry matched this template name."
            }) | Out-Null
            continue
        }

        $historyEntry = $historyByName[$key]
        $historyStatus = Get-HistoryReadinessStatus -HistoryEntry $historyEntry
        $template["schema_history_available"] = $true
        $template["schema_history"] = [ordered]@{
            status = $historyStatus
            latest_status = Get-JsonString -Object $historyEntry -Name "latest_status"
            latest_decision = Get-JsonString -Object $historyEntry -Name "latest_decision"
            latest_action = Get-JsonString -Object $historyEntry -Name "latest_action"
            latest_generated_at = Get-JsonString -Object $historyEntry -Name "latest_generated_at"
            latest_summary_json = Get-JsonString -Object $historyEntry -Name "latest_summary_json"
            run_count = Get-JsonInt -Object $historyEntry -Name "run_count"
            blocked_run_count = Get-JsonInt -Object $historyEntry -Name "blocked_run_count"
            pending_run_count = Get-JsonInt -Object $historyEntry -Name "pending_run_count"
            approved_run_count = Get-JsonInt -Object $historyEntry -Name "approved_run_count"
            issue_keys = @(Get-JsonArray -Object $historyEntry -Name "issue_keys")
        }

        if ($historyStatus -in @("blocked", "pending_review")) {
            $blockers = New-Object 'System.Collections.Generic.List[object]'
            foreach ($blocker in @($template.release_blockers)) {
                $blockers.Add($blocker) | Out-Null
            }
            $blockers.Add((New-ReleaseBlocker `
                -Id "project_template_delivery_readiness.schema_approval_history" `
                -Source "schema_approval_history" `
                -Status $historyStatus `
                -Action (Get-JsonString -Object $historyEntry -Name "latest_action" -DefaultValue "review_schema_approval_history") `
                -Message "Latest schema approval history entry is not release-ready.")) | Out-Null
            $template["release_blocked"] = $true
            $template["release_ready"] = $false
            $template["release_blockers"] = @($blockers.ToArray())
            $template["release_blocker_count"] = $blockers.Count
        }
    }
}

function Get-LatestHistory {
    param([object[]]$Histories)

    $sorted = @($Histories | Sort-Object @{ Expression = { Get-JsonString -Object $_ -Name "generated_at" } })
    if ($sorted.Count -eq 0) { return $null }
    return $sorted[-1]
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Project Template Delivery Readiness Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Templates: ``$($Summary.template_count)``") | Out-Null
    $lines.Add("- Ready templates: ``$($Summary.ready_template_count)``") | Out-Null
    $lines.Add("- Blocked templates: ``$($Summary.blocked_template_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Latest schema approval gate: ``$($Summary.latest_schema_approval_gate_status)``") | Out-Null
    $onboardingGovernanceNextAction = Get-NextActionDisplay -NextAction $Summary.onboarding_governance_next_action
    if (-not [string]::IsNullOrWhiteSpace($onboardingGovernanceNextAction)) {
        $lines.Add("- Onboarding governance next action: ``$onboardingGovernanceNextAction``") | Out-Null
        $lines.Add("- Onboarding governance next action groups: ``$($Summary.onboarding_governance_next_action_group_count)``") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Schema Approval History") | Out-Null
    $lines.Add("") | Out-Null
    if ($Summary.schema_history_count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        $lines.Add("- History reports: ``$($Summary.schema_history_count)``") | Out-Null
        $lines.Add("- Latest gate: ``$($Summary.latest_schema_approval_gate_status)``") | Out-Null
        $lines.Add("- Blocked runs: ``$($Summary.schema_history_blocked_run_count)``") | Out-Null
        $lines.Add("- Pending runs: ``$($Summary.schema_history_pending_run_count)``") | Out-Null
        $lines.Add("- Passed runs: ``$($Summary.schema_history_passed_run_count)``") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Onboarding Governance Next Actions") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.onboarding_governance_next_action_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.onboarding_governance_next_action_summary)) {
            $display = Get-NextActionDisplay -NextAction $item
            if ([string]::IsNullOrWhiteSpace($display)) {
                $display = "unknown"
            }
            $lines.Add("- ``$display``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Templates") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.templates).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($template in @($Summary.templates)) {
            $lines.Add(("- ``{0}``: onboarding=``{1}`` schema=``{2}`` history=``{3}`` blockers=``{4}`` source=``{5}``" -f
                $template.template_name,
                $template.onboarding_status,
                $template.schema_approval_status,
                $template.schema_history.status,
                $template.release_blocker_count,
                $template.source_kind)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.scope)`` / ``$($blocker.id)``: action=``$($blocker.action)`` blocker_id=``$($blocker.blocker_id)`` schema=``$($blocker.source_schema)`` source_json_display=``$($blocker.source_json_display)`` source_report_display=``$($blocker.source_report_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - message: $($blocker.message)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.reason)) {
                $lines.Add("  - reason: $($blocker.reason)") | Out-Null
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
            $lines.Add("- ``$($item.template_name)`` / ``$($item.id)``: action=``$($item.action)`` blocker_id=``$($item.blocker_id)`` schema=``$($item.source_schema)`` source_json_display=``$($item.source_json_display)`` source_report_display=``$($item.source_report_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.reason)) {
                $lines.Add("  - reason: $($item.reason)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
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
            $lines.Add("- ``$($warning.id)``: action=``$($warning.action)`` schema=``$($warning.source_schema)`` source_json_display=``$($warning.source_json_display)`` source_report_display=``$($warning.source_report_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.message)) {
                $lines.Add("  - message: $($warning.message)") | Out-Null
            }
            $repairStrategy = Get-JsonString -Object $warning -Name "repair_strategy"
            $repairHint = Get-JsonString -Object $warning -Name "repair_hint"
            $commandTemplate = Get-JsonString -Object $warning -Name "command_template"
            if (-not [string]::IsNullOrWhiteSpace($repairStrategy)) {
                $lines.Add("  - repair_strategy: ``$repairStrategy``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($repairHint)) {
                $lines.Add("  - repair_hint: $repairHint") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($commandTemplate)) {
                $lines.Add("  - command_template: ``$commandTemplate``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Source Files") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($source in @($Summary.source_files)) {
        $lines.Add("- ``$($source.path_display)``: kind=``$($source.kind)`` status=``$($source.status)``") | Out-Null
    }
    if (@($Summary.source_files).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    }

    return @($lines)
}
