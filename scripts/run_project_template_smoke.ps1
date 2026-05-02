<#
.SYNOPSIS
Runs project-level template smoke checks from a manifest.

.DESCRIPTION
Reads a manifest of template fixtures, optionally prepares those fixtures via
built sample targets, then runs any combination of `validate-template`,
`validate-template-schema`, `check_template_schema_baseline.ps1`, and
`run_word_visual_smoke.ps1`.
#>
param(
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$BuildDir = "build-project-template-smoke-nmake",
    [string]$Generator = "NMake Makefiles",
    [string]$OutputDir = "output/project-template-smoke",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisualSmoke,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")
. (Join-Path $PSScriptRoot "project_template_smoke_manifest_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[project-template-smoke] $Message"
}

function Resolve-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Convert-ToSafeFileName {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "entry"
    }

    $sanitized = $Value -replace '[\\/:*?"<>|]', '-' -replace '\s+', '-'
    $sanitized = $sanitized.Trim('-')
    if ([string]::IsNullOrWhiteSpace($sanitized)) {
        return "entry"
    }

    return $sanitized.ToLowerInvariant()
}

function Resolve-OptionalManifestPropertyValue {
    param(
        $Entry,
        [string]$Name
    )

    $property = $Entry.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-ManifestArrayProperty {
    param(
        $Entry,
        [string]$Name
    )

    $property = $Entry.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return @()
    }

    return @($property.Value | Where-Object { $null -ne $_ })
}

function Get-OptionalObjectPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name) -and $null -ne $Object[$Name]) {
            return [string]$Object[$Name]
        }

        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function Get-OptionalObjectPropertyObject {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }

        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-OptionalObjectArrayProperty {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }

    if ($value -is [string]) {
        return @($value)
    }

    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Normalize-ProjectTemplateSmokeVisualVerdict {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "undecided"
    }

    switch ($Value.ToLowerInvariant()) {
        "pass" { return "pass" }
        "fail" { return "fail" }
        "undetermined" { return "undetermined" }
        "undecided" { return "undecided" }
        "pending_manual_review" { return "pending_manual_review" }
        "skipped" { return "skipped" }
        default { return "undecided" }
    }
}

function Get-ProjectTemplateSmokeVisualReviewState {
    param(
        [string]$ReviewStatus,
        [string]$ReviewVerdict
    )

    $normalizedStatus = if ([string]::IsNullOrWhiteSpace($ReviewStatus)) {
        "missing"
    } else {
        $ReviewStatus
    }
    $normalizedVerdict = Normalize-ProjectTemplateSmokeVisualVerdict -Value $ReviewVerdict

    $failed = ($normalizedStatus -eq "failed" -or $normalizedVerdict -eq "fail")
    $pending = (-not $failed) -and (
        $normalizedStatus -eq "missing" -or
        $normalizedStatus -eq "pending_review" -or
        $normalizedVerdict -eq "undecided" -or
        $normalizedVerdict -eq "pending_manual_review"
    )
    $undetermined = (-not $failed) -and (-not $pending) -and $normalizedVerdict -eq "undetermined"
    $passed = (-not $failed) -and (-not $pending) -and (-not $undetermined) -and $normalizedVerdict -eq "pass"
    $skipped = (-not $failed) -and (-not $pending) -and (-not $undetermined) -and (-not $passed) -and (
        $normalizedStatus -eq "skipped" -or
        $normalizedVerdict -eq "skipped"
    )

    return [pscustomobject]@{
        review_status = $normalizedStatus
        review_verdict = $normalizedVerdict
        failed = $failed
        manual_review_pending = $pending
        review_undetermined = $undetermined
        review_passed = $passed
        skipped = $skipped
    }
}

function Get-ProjectTemplateSmokeEntryStatus {
    param(
        [bool]$EntryPassed,
        [bool]$ManualReviewPending,
        [bool]$VisualReviewUndetermined
    )

    if (-not $EntryPassed) {
        return "failed"
    }
    if ($ManualReviewPending) {
        return "passed_with_pending_visual_review"
    }
    if ($VisualReviewUndetermined) {
        return "passed_with_visual_review_undetermined"
    }

    return "passed"
}

function Get-ProjectTemplateSmokeOverallStatus {
    param(
        [bool]$SummaryPassed,
        [int]$ManualReviewPendingCount,
        [int]$VisualReviewUndeterminedCount
    )

    if (-not $SummaryPassed) {
        return "failed"
    }
    if ($ManualReviewPendingCount -gt 0) {
        return "passed_with_pending_visual_review"
    }
    if ($VisualReviewUndeterminedCount -gt 0) {
        return "passed_with_visual_review_undetermined"
    }

    return "passed"
}

function Get-ProjectTemplateSmokeVisualVerdict {
    param([object[]]$VisualSmokeResults)

    if ($VisualSmokeResults.Count -eq 0) {
        return "not_applicable"
    }

    $states = @(
        foreach ($visualSmokeResult in $VisualSmokeResults) {
            Get-ProjectTemplateSmokeVisualReviewState `
                -ReviewStatus (Get-OptionalObjectPropertyValue -Object $visualSmokeResult -Name "review_status") `
                -ReviewVerdict (Get-OptionalObjectPropertyValue -Object $visualSmokeResult -Name "review_verdict")
        }
    )

    if (@($states | Where-Object { $_.failed }).Count -gt 0) {
        return "fail"
    }
    if (@($states | Where-Object { $_.manual_review_pending }).Count -gt 0) {
        return "pending_manual_review"
    }
    if (@($states | Where-Object { $_.review_undetermined }).Count -gt 0) {
        return "undetermined"
    }
    if (@($states | Where-Object { $_.review_passed }).Count -gt 0) {
        return "pass"
    }
    if (@($states | Where-Object { $_.skipped }).Count -eq $states.Count) {
        return "skipped"
    }

    return "pending_manual_review"
}

function Resolve-ManifestPathValue {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    return Resolve-TemplateSchemaPath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing:$AllowMissing
}

function Resolve-ManifestInputDocxPath {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        $Entry
    )

    $repoRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx"
    $buildRelativePath = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "input_docx_build_relative"

    if (-not [string]::IsNullOrWhiteSpace($repoRelativePath) -and
        -not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        throw "Manifest entries must not set both 'input_docx' and 'input_docx_build_relative'."
    }

    if (-not [string]::IsNullOrWhiteSpace($buildRelativePath)) {
        if ([string]::IsNullOrWhiteSpace($ResolvedBuildDir)) {
            throw "Entry '$([string]$Entry.name)' uses 'input_docx_build_relative' but no -BuildDir was provided."
        }

        return Resolve-TemplateSchemaPath -RepoRoot $ResolvedBuildDir -InputPath $buildRelativePath -AllowMissing
    }

    if ([string]::IsNullOrWhiteSpace($repoRelativePath)) {
        throw "Entry '$([string]$Entry.name)' must set either 'input_docx' or 'input_docx_build_relative'."
    }

    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $repoRelativePath -AllowMissing
}

function Resolve-PrepareArgumentPath {
    param(
        [string]$RepoRoot,
        [string]$ResolvedInputDocx,
        $Entry
    )

    $prepareArgument = Resolve-OptionalManifestPropertyValue -Entry $Entry -Name "prepare_argument"
    if ([string]::IsNullOrWhiteSpace($prepareArgument)) {
        return $ResolvedInputDocx
    }

    return Resolve-ManifestPathValue -RepoRoot $RepoRoot -InputPath $prepareArgument -AllowMissing
}

function Ensure-PathParent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $extension = [System.IO.Path]::GetExtension($Path)
    if ([string]::IsNullOrWhiteSpace($extension)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
}

function Convert-SlotSpecToCliString {
    param($SlotSpec)

    if ($SlotSpec -is [string]) {
        return $SlotSpec
    }

    $name = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "bookmark"
    if ([string]::IsNullOrWhiteSpace($name)) {
        $name = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "name"
    }
    $kind = Resolve-OptionalManifestPropertyValue -Entry $SlotSpec -Name "kind"

    if ([string]::IsNullOrWhiteSpace($name) -or [string]::IsNullOrWhiteSpace($kind)) {
        throw "Structured slot objects must provide 'bookmark'/'name' and 'kind'."
    }

    $segments = New-Object 'System.Collections.Generic.List[string]'
    $segments.Add($name)
    $segments.Add($kind)

    $requiredProperty = $SlotSpec.PSObject.Properties["required"]
    if ($null -ne $requiredProperty -and $null -ne $requiredProperty.Value) {
        $segments.Add((if ([bool]$requiredProperty.Value) { "required" } else { "optional" }))
    } else {
        $optionalProperty = $SlotSpec.PSObject.Properties["optional"]
        if ($null -ne $optionalProperty -and $null -ne $optionalProperty.Value) {
            $segments.Add((if ([bool]$optionalProperty.Value) { "optional" } else { "required" }))
        }
    }

    foreach ($occurrenceName in @("count", "min", "max")) {
        $property = $SlotSpec.PSObject.Properties[$occurrenceName]
        if ($null -ne $property -and $null -ne $property.Value -and
            -not [string]::IsNullOrWhiteSpace([string]$property.Value)) {
            $segments.Add("$occurrenceName=$([string]$property.Value)")
        }
    }

    return ($segments -join ":")
}

function Add-PartSelectionArguments {
    param(
        [string[]]$Arguments,
        [string]$PartSwitch,
        $Selection
    )

    $part = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "part"
    if ([string]::IsNullOrWhiteSpace($part)) {
        throw "Every selection block must provide a non-empty 'part'."
    }

    $result = @($Arguments + @($PartSwitch, $part))

    $index = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "index"
    if (-not [string]::IsNullOrWhiteSpace($index)) {
        $result += @("--index", $index)
    }

    $section = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "section"
    if (-not [string]::IsNullOrWhiteSpace($section)) {
        $result += @("--section", $section)
    }

    $kind = Resolve-OptionalManifestPropertyValue -Entry $Selection -Name "kind"
    if (-not [string]::IsNullOrWhiteSpace($kind)) {
        $result += @("--kind", $kind)
    }

    return $result
}

function Write-CommandOutput {
    param(
        [string]$OutputPath,
        [string[]]$Lines
    )

    if ([string]::IsNullOrWhiteSpace($OutputPath)) {
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($OutputPath)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }

    $Lines | Set-Content -LiteralPath $OutputPath -Encoding UTF8
}

function Invoke-CliJsonCommand {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureLabel
    )

    $commandResult = Invoke-TemplateSchemaCli -ExecutablePath $ExecutablePath -Arguments $Arguments
    Write-CommandOutput -OutputPath $OutputPath -Lines $commandResult.Output

    if ($commandResult.ExitCode -ne 0) {
        $lines = if ($commandResult.Output.Count -gt 0) {
            ($commandResult.Output -join [System.Environment]::NewLine)
        } else {
            "(no output)"
        }
        throw "$FailureLabel failed with exit code $($commandResult.ExitCode). Output:`n$lines"
    }

    $jsonText = $commandResult.Text.Trim()
    if ([string]::IsNullOrWhiteSpace($jsonText)) {
        throw "$FailureLabel did not emit JSON output."
    }

    try {
        $parsed = $jsonText | ConvertFrom-Json
    } catch {
        throw "$FailureLabel emitted invalid JSON: $jsonText"
    }

    return [pscustomobject]@{
        ExitCode = $commandResult.ExitCode
        Text = $jsonText
        Output = $commandResult.Output
        Json = $parsed
    }
}

function Get-OptionalTemplateSchemaCommandJsonObject {
    param(
        [string[]]$Lines,
        [string]$Command
    )

    $pattern = '^\{"command":"' + [regex]::Escape($Command) + '",'
    $jsonLine = $Lines |
        Where-Object { $_ -match $pattern } |
        Select-Object -Last 1

    if ([string]::IsNullOrWhiteSpace([string]$jsonLine)) {
        return $null
    }

    return $jsonLine | ConvertFrom-Json
}

function Read-JsonFileIfPresent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not [System.IO.File]::Exists($Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function New-SchemaPatchReviewSummary {
    param($ReviewJson)

    if ($null -eq $ReviewJson) {
        return $null
    }

    return [pscustomobject]@{
        schema = [string]$ReviewJson.schema
        baseline_slot_count = [int]$ReviewJson.baseline_slot_count
        generated_slot_count = [int]$ReviewJson.generated_slot_count
        changed = [bool]$ReviewJson.changed
        upsert_slot_count = [int]$ReviewJson.patch.upsert_slot_count
        remove_target_count = [int]$ReviewJson.patch.remove_target_count
        remove_slot_count = [int]$ReviewJson.patch.remove_slot_count
        rename_slot_count = [int]$ReviewJson.patch.rename_slot_count
        update_slot_count = [int]$ReviewJson.patch.update_slot_count
        removed_targets = [int]$ReviewJson.preview.removed_targets
        removed_slots = [int]$ReviewJson.preview.removed_slots
        renamed_slots = [int]$ReviewJson.preview.renamed_slots
        inserted_slots = [int]$ReviewJson.preview.inserted_slots
        replaced_slots = [int]$ReviewJson.preview.replaced_slots
    }
}

function Normalize-SchemaPatchApprovalDecision {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "pending"
    }

    switch ($Value.Trim().ToLowerInvariant()) {
        "approve" { return "approved" }
        "approved" { return "approved" }
        "accept" { return "approved" }
        "accepted" { return "approved" }
        "reject" { return "rejected" }
        "rejected" { return "rejected" }
        "needs_changes" { return "needs_changes" }
        "needs-changes" { return "needs_changes" }
        "changes_requested" { return "needs_changes" }
        "pending" { return "pending" }
        "pending_review" { return "pending" }
        default { return "invalid" }
    }
}

function Test-SchemaPatchApprovalResultCompliance {
    param(
        [string]$Decision,
        $ApprovalResult
    )

    $issues = New-Object 'System.Collections.Generic.List[string]'
    if ($Decision -in @("approved", "rejected", "needs_changes")) {
        $reviewer = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "reviewer"
        if ([string]::IsNullOrWhiteSpace($reviewer)) {
            [void]$issues.Add("missing_reviewer")
        }

        $reviewedAt = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "reviewed_at"
        if ([string]::IsNullOrWhiteSpace($reviewedAt)) {
            [void]$issues.Add("missing_reviewed_at")
        }
    }

    return [pscustomobject]@{
        issue_count = $issues.Count
        issues = $issues.ToArray()
    }
}

function Get-SchemaPatchApprovalGateStatus {
    param(
        [int]$PendingCount,
        [int]$ApprovalItemCount,
        [int]$InvalidResultCount,
        [int]$ComplianceIssueCount
    )

    if ($ComplianceIssueCount -gt 0 -or $InvalidResultCount -gt 0) {
        return "blocked"
    }
    if ($PendingCount -gt 0) {
        return "pending"
    }
    if ($ApprovalItemCount -gt 0) {
        return "passed"
    }

    return "not_required"
}

function New-SchemaPatchApprovalResultTemplate {
    param(
        [string]$Name,
        $SchemaBaseline
    )

    return [ordered]@{
        schema = "featherdoc.template_schema_patch_approval_result.v1"
        entry_name = $Name
        decision = "pending"
        reviewer = ""
        reviewed_at = ""
        note = ""
        schema_file = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "schema_file"
        schema_update_candidate = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "generated_output"
        review_json = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "schema_patch_review_output_path"
    }
}

function New-SchemaPatchApprovalSummary {
    param(
        [string]$Name,
        $SchemaBaseline,
        $ApprovalResult,
        [string]$ApprovalResultPath
    )

    $reviewSummary = Get-OptionalObjectPropertyObject -Object $SchemaBaseline -Name "schema_patch_review"
    if ($null -eq $reviewSummary) {
        return $null
    }

    $changed = [bool]$reviewSummary.changed
    $decision = if ($changed) {
        Normalize-SchemaPatchApprovalDecision -Value (Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "decision")
    } else {
        "not_required"
    }

    $compliance = Test-SchemaPatchApprovalResultCompliance `
        -Decision $decision `
        -ApprovalResult $ApprovalResult
    $status = switch ($decision) {
        "not_required" { "not_required" }
        "approved" { "approved" }
        "rejected" { "rejected" }
        "needs_changes" { "needs_changes" }
        "invalid" { "invalid_result" }
        default { "pending_review" }
    }
    if ($changed -and [int]$compliance.issue_count -gt 0) {
        $status = "invalid_result"
    }
    $action = switch ($status) {
        "not_required" { "none" }
        "approved" { "promote_schema_update_candidate" }
        "rejected" { "update_template_or_schema_before_retry" }
        "needs_changes" { "update_template_or_schema_before_retry" }
        "invalid_result" { "fix_schema_patch_approval_result" }
        default { "review_schema_update_candidate" }
    }
    $pending = $changed -and $status -in @("pending_review", "invalid_result")

    return [pscustomobject]@{
        name = $Name
        required = $changed
        pending = $pending
        approved = ($changed -and $status -eq "approved")
        status = $status
        decision = $decision
        action = $action
        compliance_issue_count = [int]$compliance.issue_count
        compliance_issues = $compliance.issues
        schema_file = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "schema_file"
        schema_update_candidate = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "generated_output"
        review_json = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "schema_patch_review_output_path"
        approval_result = $ApprovalResultPath
        repaired_schema_candidate = Get-OptionalObjectPropertyValue -Object $SchemaBaseline -Name "repaired_schema_output_path"
        reviewer = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "reviewer"
        reviewed_at = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "reviewed_at"
        note = Get-OptionalObjectPropertyValue -Object $ApprovalResult -Name "note"
        baseline_slot_count = [int]$reviewSummary.baseline_slot_count
        generated_slot_count = [int]$reviewSummary.generated_slot_count
        upsert_slot_count = [int]$reviewSummary.upsert_slot_count
        remove_target_count = [int]$reviewSummary.remove_target_count
        remove_slot_count = [int]$reviewSummary.remove_slot_count
        rename_slot_count = [int]$reviewSummary.rename_slot_count
        update_slot_count = [int]$reviewSummary.update_slot_count
        inserted_slots = [int]$reviewSummary.inserted_slots
        replaced_slots = [int]$reviewSummary.replaced_slots
    }
}

function Invoke-BaselineCheck {
    param(
        [string]$ScriptPath,
        [string]$InputDocx,
        [string]$SchemaFile,
        [string]$GeneratedSchemaOutput,
        [string]$RepairedSchemaOutput,
        [string]$ReviewJsonOutput,
        [string]$BuildDir,
        [string]$TargetMode,
        [bool]$SkipBuildRequested,
        [string]$LogPath
    )

    $scriptArgs = @(
        "-InputDocx"
        $InputDocx
        "-SchemaFile"
        $SchemaFile
        "-GeneratedSchemaOutput"
        $GeneratedSchemaOutput
        "-BuildDir"
        $BuildDir
    )
    if (-not [string]::IsNullOrWhiteSpace($RepairedSchemaOutput)) {
        $scriptArgs += @(
            "-RepairedSchemaOutput"
            $RepairedSchemaOutput
        )
    }
    if (-not [string]::IsNullOrWhiteSpace($ReviewJsonOutput)) {
        $scriptArgs += @(
            "-ReviewJsonOutput"
            $ReviewJsonOutput
        )
    }
    if ($SkipBuildRequested) {
        $scriptArgs += "-SkipBuild"
    }

    switch ($TargetMode) {
        "" { }
        "default" { }
        "section-targets" { $scriptArgs += "-SectionTargets" }
        "resolved-section-targets" { $scriptArgs += "-ResolvedSectionTargets" }
        default { throw "Unsupported schema baseline target_mode '$TargetMode'." }
    }

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @scriptArgs 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    Write-CommandOutput -OutputPath $LogPath -Lines $lines

    if ($exitCode -notin @(0, 1)) {
        $joined = if ($lines.Count -gt 0) {
            $lines -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "check_template_schema_baseline.ps1 failed with exit code $exitCode. Output:`n$joined"
    }

    $lintJson = Get-TemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "lint-template-schema"
    $checkJson = Get-TemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "check-template-schema"
    $repairJson = Get-OptionalTemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "repair-template-schema"
    $reviewJson = Read-JsonFileIfPresent -Path $ReviewJsonOutput

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Json = $checkJson
        LintJson = $lintJson
        RepairJson = $repairJson
        ReviewJson = $reviewJson
        ReviewSummary = New-SchemaPatchReviewSummary -ReviewJson $reviewJson
    }
}

function Invoke-VisualSmoke {
    param(
        [string]$ScriptPath,
        [string]$InputDocx,
        [string]$OutputDir,
        [int]$Dpi,
        [bool]$KeepWordOpenRequested,
        [bool]$VisibleWordRequested,
        [string]$LogPath
    )

    $scriptArgs = @(
        "-InputDocx"
        $InputDocx
        "-OutputDir"
        $OutputDir
        "-Dpi"
        ([string]$Dpi)
    )
    if ($KeepWordOpenRequested) {
        $scriptArgs += "-KeepWordOpen"
    }
    if ($VisibleWordRequested) {
        $scriptArgs += "-VisibleWord"
    }

    $commandOutput = @(& powershell.exe -ExecutionPolicy Bypass -File $ScriptPath @scriptArgs 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    Write-CommandOutput -OutputPath $LogPath -Lines $lines

    if ($exitCode -ne 0) {
        $joined = if ($lines.Count -gt 0) {
            $lines -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "run_word_visual_smoke.ps1 failed with exit code $exitCode. Output:`n$joined"
    }

    $reportDir = Join-Path $OutputDir "report"
    $summaryPath = Join-Path $reportDir "summary.json"
    $reviewResultPath = Join-Path $reportDir "review_result.json"

    if (-not (Test-Path -LiteralPath $summaryPath)) {
        throw "Visual smoke summary was not created: $summaryPath"
    }
    if (-not (Test-Path -LiteralPath $reviewResultPath)) {
        throw "Visual smoke review_result was not created: $reviewResultPath"
    }

    return [pscustomobject]@{
        Output = $lines
        SummaryPath = $summaryPath
        ReviewResultPath = $reviewResultPath
        Summary = Get-Content -Raw -LiteralPath $summaryPath | ConvertFrom-Json
        ReviewResult = Get-Content -Raw -LiteralPath $reviewResultPath | ConvertFrom-Json
        ContactSheetPath = Join-Path (Join-Path $OutputDir "evidence") "contact_sheet.png"
    }
}

function Write-SummaryMarkdown {
    param(
        [string]$Path,
        [string]$RepoRoot,
        $Summary
    )

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Project Template Smoke")
    $lines.Add("")
    $lines.Add("- Generated at: $($Summary.generated_at)")
    $visualReviewSyncedAt = Get-OptionalObjectPropertyValue -Object $Summary -Name "visual_review_synced_at"
    if (-not [string]::IsNullOrWhiteSpace($visualReviewSyncedAt)) {
        $lines.Add("- Visual review synced at: $visualReviewSyncedAt")
    }
    $lines.Add("- Manifest: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $Summary.manifest_path)")
    $lines.Add("- Output directory: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $Summary.output_dir)")
    $lines.Add("- Overall status: $($Summary.overall_status)")
    $lines.Add("- Passed flag: $($Summary.passed)")
    $lines.Add("- Entry count: $($Summary.entry_count)")
    $lines.Add("- Failed entries: $($Summary.failed_entry_count)")
    $dirtySchemaBaselineCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "dirty_schema_baseline_count"
    if ([string]::IsNullOrWhiteSpace($dirtySchemaBaselineCount)) {
        $dirtySchemaBaselineCount = "0"
    }
    $lines.Add("- Dirty schema baselines: $dirtySchemaBaselineCount")
    $schemaPatchReviewCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_review_count"
    $schemaPatchReviewChangedCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_review_changed_count"
    if ([string]::IsNullOrWhiteSpace($schemaPatchReviewCount)) {
        $schemaPatchReviewCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchReviewChangedCount)) {
        $schemaPatchReviewChangedCount = "0"
    }
    $schemaPatchApprovalPendingCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_pending_count"
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalPendingCount)) {
        $schemaPatchApprovalPendingCount = "0"
    }
    $lines.Add("- Schema patch reviews: $schemaPatchReviewCount")
    $lines.Add("- Changed schema patch reviews: $schemaPatchReviewChangedCount")
    $schemaPatchApprovalApprovedCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_approved_count"
    $schemaPatchApprovalRejectedCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_rejected_count"
    $schemaPatchApprovalInvalidResultCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_invalid_result_count"
    $schemaPatchApprovalGateStatus = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_gate_status"
    $schemaPatchApprovalComplianceIssueCount = Get-OptionalObjectPropertyValue -Object $Summary -Name "schema_patch_approval_compliance_issue_count"
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalApprovedCount)) {
        $schemaPatchApprovalApprovedCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalRejectedCount)) {
        $schemaPatchApprovalRejectedCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalComplianceIssueCount)) {
        $schemaPatchApprovalComplianceIssueCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalInvalidResultCount)) {
        $schemaPatchApprovalInvalidResultCount = "0"
    }
    if ([string]::IsNullOrWhiteSpace($schemaPatchApprovalGateStatus)) {
        $schemaPatchApprovalGateStatus = "not_required"
    }
    $lines.Add("- Schema patch approvals pending: $schemaPatchApprovalPendingCount")
    $lines.Add("- Schema patch approvals approved: $schemaPatchApprovalApprovedCount")
    $lines.Add("- Schema patch approvals rejected: $schemaPatchApprovalRejectedCount")
    $lines.Add("- Schema patch approval compliance issues: $schemaPatchApprovalComplianceIssueCount")
    $lines.Add("- Schema patch approval invalid results: $schemaPatchApprovalInvalidResultCount")
    $lines.Add("- Schema patch approval gate status: $schemaPatchApprovalGateStatus")
    $lines.Add("- Visual smoke entries: $($Summary.visual_entry_count)")
    $lines.Add("- Visual verdict: $($Summary.visual_verdict)")
    $lines.Add("- Entries with pending visual review: $($Summary.manual_review_pending_count)")
    $lines.Add("- Entries with undetermined visual review: $($Summary.visual_review_undetermined_count)")
    $lines.Add("")

    $schemaPatchApprovalItems = Get-OptionalObjectArrayProperty -Object $Summary -Name "schema_patch_approval_items"
    if (@($schemaPatchApprovalItems).Count -gt 0) {
        $lines.Add("## Schema Patch Approvals")
        $lines.Add("")
        foreach ($approval in @($schemaPatchApprovalItems)) {
            $complianceIssues = Get-OptionalObjectArrayProperty -Object $approval -Name "compliance_issues"
            $complianceText = if (@($complianceIssues).Count -gt 0) { " compliance_issues=$($complianceIssues -join ',')" } else { "" }
            $lines.Add("- $($approval.name): status=$($approval.status) decision=$($approval.decision) candidate=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $approval.schema_update_candidate) approval=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $approval.approval_result) review=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $approval.review_json) action=$($approval.action)$complianceText")
        }
        $lines.Add("")
    }

    foreach ($entry in $Summary.entries) {
        $lines.Add("## $($entry.name)")
        $lines.Add("")
        $lines.Add("- Status: $($entry.status)")
        $lines.Add("- Input DOCX: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $entry.input_docx)")
        $lines.Add("- Entry artifact directory: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $entry.artifact_dir)")

        foreach ($validation in @($entry.checks.template_validations)) {
            $lines.Add("- Template validation '$($validation.name)': passed=$($validation.passed) output=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $validation.output_json)")
        }

        $schemaValidation = $entry.checks.schema_validation
        if ($schemaValidation.enabled) {
            $schemaValidationOutput = Get-OptionalObjectPropertyValue -Object $schemaValidation -Name "output_json"
            $lines.Add("- Schema validation: passed=$($schemaValidation.passed) output=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaValidationOutput)")
        }

        $schemaBaseline = $entry.checks.schema_baseline
        if ($schemaBaseline.enabled) {
            $schemaBaselineLog = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "log_path"
            $schemaPatchReviewOutputPath = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "schema_patch_review_output_path"
            $schemaPatchReview = Get-OptionalObjectPropertyObject -Object $schemaBaseline -Name "schema_patch_review"
            $schemaLintClean = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "schema_lint_clean"
            $schemaLintIssueCount = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "schema_lint_issue_count"
            if ([string]::IsNullOrWhiteSpace($schemaLintClean)) {
                $schemaLintClean = "(not available)"
            }
            if ([string]::IsNullOrWhiteSpace($schemaLintIssueCount)) {
                $schemaLintIssueCount = "(not available)"
            }
            $lines.Add("- Schema baseline: matches=$($schemaBaseline.matches) schema_lint_clean=$schemaLintClean schema_lint_issue_count=$schemaLintIssueCount log=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaBaselineLog)")
            if ($null -ne $schemaPatchReview) {
                $lines.Add("- Schema patch review: changed=$($schemaPatchReview.changed) baseline_slots=$($schemaPatchReview.baseline_slot_count) generated_slots=$($schemaPatchReview.generated_slot_count) inserted_slots=$($schemaPatchReview.inserted_slots) replaced_slots=$($schemaPatchReview.replaced_slots) review=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaPatchReviewOutputPath)")
            }
            $schemaPatchApproval = Get-OptionalObjectPropertyObject -Object $schemaBaseline -Name "schema_patch_approval"
            if ($null -ne $schemaPatchApproval) {
                $complianceIssues = Get-OptionalObjectArrayProperty -Object $schemaPatchApproval -Name "compliance_issues"
                $complianceText = if (@($complianceIssues).Count -gt 0) { " compliance_issues=$($complianceIssues -join ',')" } else { "" }
                $lines.Add("- Schema patch approval: status=$($schemaPatchApproval.status) decision=$($schemaPatchApproval.decision) required=$($schemaPatchApproval.required) pending=$($schemaPatchApproval.pending) candidate=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaPatchApproval.schema_update_candidate) approval=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $schemaPatchApproval.approval_result) action=$($schemaPatchApproval.action)$complianceText")
            }
            $repairedSchemaOutputPath = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "repaired_schema_output_path"
            if (-not [string]::IsNullOrWhiteSpace($repairedSchemaOutputPath)) {
                $lines.Add("- Schema baseline repaired candidate: $(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $repairedSchemaOutputPath)")
            }
        }

        $visualSmoke = $entry.checks.visual_smoke
        if ($visualSmoke.enabled) {
            $contactSheet = Get-OptionalObjectPropertyValue -Object $visualSmoke -Name "contact_sheet"
            $findingsCount = Get-OptionalObjectPropertyValue -Object $visualSmoke -Name "findings_count"
            if ([string]::IsNullOrWhiteSpace($findingsCount)) {
                $findingsCount = "0"
            }
            $lines.Add("- Visual smoke: review_status=$($visualSmoke.review_status) review_verdict=$($visualSmoke.review_verdict) findings=$findingsCount contact_sheet=$(Resolve-RepoRelativePath -RepoRoot $RepoRoot -Path $contactSheet)")
        }

        foreach ($issue in @($entry.issues)) {
            $lines.Add("- Issue: $issue")
        }

        $lines.Add("")
    }

    $lines | Set-Content -LiteralPath $Path -Encoding UTF8
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedManifestPath = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedOutputDir = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $OutputDir -AllowMissing
$resolvedBuildDir = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
$entriesOutputDir = Join-Path $resolvedOutputDir "entries"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$summaryMarkdownPath = Join-Path $resolvedOutputDir "summary.md"
$baselineScriptPath = Join-Path $PSScriptRoot "check_template_schema_baseline.ps1"
$visualSmokeScriptPath = Join-Path $PSScriptRoot "run_word_visual_smoke.ps1"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $entriesOutputDir -Force | Out-Null

$manifestValidation = Test-ProjectTemplateSmokeManifest `
    -RepoRoot $repoRoot `
    -ManifestPath $resolvedManifestPath `
    -BuildDir $resolvedBuildDir `
    -CheckPaths
if (-not $manifestValidation.passed) {
    $errorLines = New-Object 'System.Collections.Generic.List[string]'
    $errorLines.Add("Project template smoke manifest validation failed:") | Out-Null
    foreach ($issue in $manifestValidation.errors) {
        $errorLines.Add("- $($issue.path): $($issue.message)") | Out-Null
    }
    throw ($errorLines -join [System.Environment]::NewLine)
}

$manifest = Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
$entries = @($manifest.entries)
if ($entries.Count -eq 0) {
    $emptySummary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        manifest_path = $resolvedManifestPath
        workspace = $repoRoot
        build_dir = $resolvedBuildDir
        output_dir = $resolvedOutputDir
        entry_count = 0
        failed_entry_count = 0
        dirty_schema_baseline_count = 0
        visual_entry_count = 0
        visual_verdict = "not_applicable"
        manual_review_pending_count = 0
        visual_review_undetermined_count = 0
        passed = $true
        overall_status = "passed"
        entries = @()
    }
    ($emptySummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
    Write-SummaryMarkdown -Path $summaryMarkdownPath -RepoRoot $repoRoot -Summary $emptySummary
    Write-Step "No entries were registered in $resolvedManifestPath"
    exit 0
}

$sampleTargets = @(
    $entries |
        ForEach-Object { Resolve-OptionalManifestPropertyValue -Entry $_ -Name "prepare_sample_target" } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
        Sort-Object -Unique
)

if (-not $SkipBuild) {
    $vcvarsPath = Get-TemplateSchemaVcvarsPath
    $buildSamplesValue = if ($sampleTargets.Count -gt 0) { "ON" } else { "OFF" }
    Write-Step "Configuring build directory $resolvedBuildDir"
    Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
        "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"$Generator`" " +
        "-DBUILD_CLI=ON -DBUILD_SAMPLES=$buildSamplesValue -DBUILD_TESTING=OFF")

    $buildTargets = @("featherdoc_cli") + $sampleTargets
    Write-Step "Building targets: $($buildTargets -join ', ')"
    Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
        "cmake --build `"$resolvedBuildDir`" --target " + ($buildTargets -join " "))
}

$cliPath = Find-TemplateSchemaCliBinary -SearchRoot $resolvedBuildDir
if (-not $cliPath) {
    $cliPath = Find-TemplateSchemaCliBinary -SearchRoot $repoRoot
}
if (-not $cliPath) {
    throw "Could not find featherdoc_cli under $resolvedBuildDir or $repoRoot."
}

$sampleTargetExecutables = @{}
foreach ($sampleTarget in $sampleTargets) {
    $binaryPath = Find-TemplateSchemaTargetBinary -SearchRoot $resolvedBuildDir -TargetName $sampleTarget
    if (-not $binaryPath) {
        $binaryPath = Find-TemplateSchemaTargetBinary -SearchRoot $repoRoot -TargetName $sampleTarget
    }
    if (-not $binaryPath) {
        throw "Could not find built sample target '$sampleTarget' under $resolvedBuildDir or $repoRoot."
    }
    $sampleTargetExecutables[$sampleTarget] = $binaryPath
}

$results = New-Object 'System.Collections.Generic.List[object]'
$failedEntryCount = 0
$dirtySchemaBaselineCount = 0
$manualReviewPendingCount = 0
$visualReviewUndeterminedCount = 0
$entryIndex = 0

foreach ($entry in $entries) {
    $entryIndex += 1
    $name = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "name"
    if ([string]::IsNullOrWhiteSpace($name)) {
        throw "Every manifest entry must provide a non-empty 'name'."
    }

    $entryDir = Join-Path $entriesOutputDir ("{0:00}-{1}" -f $entryIndex, (Convert-ToSafeFileName -Value $name))
    New-Item -ItemType Directory -Path $entryDir -Force | Out-Null

    $entryIssues = New-Object 'System.Collections.Generic.List[string]'
    $templateValidationResults = New-Object 'System.Collections.Generic.List[object]'
    $schemaValidationResult = [ordered]@{
        enabled = $false
    }
    $schemaBaselineResult = [ordered]@{
        enabled = $false
    }
    $visualSmokeResult = [ordered]@{
        enabled = $false
    }
    $entryPassed = $true
    $manualReviewPending = $false
    $visualReviewUndetermined = $false
    $resolvedInputDocx = ""

    Write-Step "Running entry '$name'"

    try {
        $resolvedInputDocx = Resolve-ManifestInputDocxPath `
            -RepoRoot $repoRoot `
            -ResolvedBuildDir $resolvedBuildDir `
            -Entry $entry

        $prepareSampleTarget = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "prepare_sample_target"
        if (-not [string]::IsNullOrWhiteSpace($prepareSampleTarget)) {
            $prepareArgument = Resolve-PrepareArgumentPath `
                -RepoRoot $repoRoot `
                -ResolvedInputDocx $resolvedInputDocx `
                -Entry $entry
            Ensure-PathParent -Path $prepareArgument
            Write-Step "Preparing '$name' via $prepareSampleTarget"
            $prepareOutput = @(& $sampleTargetExecutables[$prepareSampleTarget] $prepareArgument 2>&1)
            $prepareExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
            $prepareLines = @($prepareOutput | ForEach-Object { $_.ToString() })
            $prepareLogPath = Join-Path $entryDir "prepare.log"
            Write-CommandOutput -OutputPath $prepareLogPath -Lines $prepareLines
            if ($prepareExitCode -ne 0) {
                $joined = if ($prepareLines.Count -gt 0) {
                    $prepareLines -join [System.Environment]::NewLine
                } else {
                    "(no output)"
                }
                throw "Sample target '$prepareSampleTarget' failed with exit code $prepareExitCode. Output:`n$joined"
            }
        }

        if (-not (Test-Path -LiteralPath $resolvedInputDocx)) {
            throw "Input DOCX was not found after preparation: $resolvedInputDocx"
        }
    } catch {
        $entryIssues.Add($_.Exception.Message) | Out-Null
        $entryPassed = $false
    }

    if ($entryPassed) {
        $validationIndex = 0
        foreach ($validation in (Get-ManifestArrayProperty -Entry $entry -Name "template_validations")) {
            $validationIndex += 1
            try {
                $validationName = Resolve-OptionalManifestPropertyValue -Entry $validation -Name "name"
                if ([string]::IsNullOrWhiteSpace($validationName)) {
                    $validationName = "template-validation-$validationIndex"
                }

                $arguments = @("validate-template", $resolvedInputDocx)
                $arguments = Add-PartSelectionArguments -Arguments $arguments -PartSwitch "--part" -Selection $validation
                $slots = Get-ManifestArrayProperty -Entry $validation -Name "slots"
                if ($slots.Count -eq 0) {
                    throw "Template validation '$validationName' must provide at least one slot."
                }
                foreach ($slot in $slots) {
                    $arguments += @("--slot", (Convert-SlotSpecToCliString -SlotSpec $slot))
                }
                $arguments += "--json"

                $outputPath = Join-Path $entryDir ("template_validation_{0:00}.json" -f $validationIndex)
                $result = Invoke-CliJsonCommand `
                    -ExecutablePath $cliPath `
                    -Arguments $arguments `
                    -OutputPath $outputPath `
                    -FailureLabel "validate-template/$validationName"

                $templateValidationResults.Add([pscustomobject]@{
                    name = $validationName
                    passed = [bool]$result.Json.passed
                    output_json = $outputPath
                    result = $result.Json
                }) | Out-Null

                if (-not [bool]$result.Json.passed) {
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $entryPassed = $false
            }
        }

        $schemaValidation = $entry.PSObject.Properties["schema_validation"]
        if ($null -ne $schemaValidation -and $null -ne $schemaValidation.Value) {
            $schemaValidationResult.enabled = $true
            try {
                $arguments = @("validate-template-schema", $resolvedInputDocx)
                $schemaValidationBlock = $schemaValidation.Value
                $schemaFile = Resolve-OptionalManifestPropertyValue -Entry $schemaValidationBlock -Name "schema_file"
                if (-not [string]::IsNullOrWhiteSpace($schemaFile)) {
                    $resolvedSchemaFile = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $schemaFile
                    $arguments += @("--schema-file", $resolvedSchemaFile)
                    $schemaValidationResult.schema_file = $resolvedSchemaFile
                }

                $schemaTargets = Get-ManifestArrayProperty -Entry $schemaValidationBlock -Name "targets"
                foreach ($target in $schemaTargets) {
                    $arguments = Add-PartSelectionArguments -Arguments $arguments -PartSwitch "--target" -Selection $target
                    $targetSlots = Get-ManifestArrayProperty -Entry $target -Name "slots"
                    if ($targetSlots.Count -eq 0) {
                        throw "Every schema_validation target must provide at least one slot."
                    }
                    foreach ($slot in $targetSlots) {
                        $arguments += @("--slot", (Convert-SlotSpecToCliString -SlotSpec $slot))
                    }
                }

                if ([string]::IsNullOrWhiteSpace($schemaFile) -and $schemaTargets.Count -eq 0) {
                    throw "schema_validation must provide 'schema_file' or at least one target."
                }

                $arguments += "--json"
                $outputPath = Join-Path $entryDir "schema_validation.json"
                $result = Invoke-CliJsonCommand `
                    -ExecutablePath $cliPath `
                    -Arguments $arguments `
                    -OutputPath $outputPath `
                    -FailureLabel "validate-template-schema/$name"

                $schemaValidationResult.output_json = $outputPath
                $schemaValidationResult.passed = [bool]$result.Json.passed
                $schemaValidationResult.result = $result.Json
                if (-not [bool]$result.Json.passed) {
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $schemaValidationResult.passed = $false
                $entryPassed = $false
            }
        }

        $schemaBaseline = $entry.PSObject.Properties["schema_baseline"]
        if ($null -ne $schemaBaseline -and $null -ne $schemaBaseline.Value) {
            $schemaBaselineResult.enabled = $true
            try {
                $schemaBaselineBlock = $schemaBaseline.Value
                $schemaFile = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "schema_file"
                if ([string]::IsNullOrWhiteSpace($schemaFile)) {
                    throw "schema_baseline must provide a non-empty 'schema_file'."
                }
                $resolvedSchemaFile = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $schemaFile
                $generatedOutput = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "generated_output"
                if ([string]::IsNullOrWhiteSpace($generatedOutput)) {
                    $generatedOutput = Join-Path $entryDir "generated_template_schema.json"
                }
                $resolvedGeneratedOutput = Resolve-ManifestPathValue `
                    -RepoRoot $repoRoot `
                    -InputPath $generatedOutput `
                    -AllowMissing
                Ensure-PathParent -Path $resolvedGeneratedOutput
                $repairedOutput = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "repaired_output"
                if ([string]::IsNullOrWhiteSpace($repairedOutput)) {
                    $generatedOutputDirectory = [System.IO.Path]::GetDirectoryName($resolvedGeneratedOutput)
                    $generatedOutputStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedGeneratedOutput)
                    if ([string]::IsNullOrWhiteSpace($generatedOutputDirectory)) {
                        $generatedOutputDirectory = $entryDir
                    }
                    if ([string]::IsNullOrWhiteSpace($generatedOutputStem)) {
                        $generatedOutputStem = "generated_template_schema"
                    }
                    $resolvedRepairedOutput = Join-Path $generatedOutputDirectory ($generatedOutputStem + ".repaired.schema.json")
                } else {
                    $resolvedRepairedOutput = Resolve-ManifestPathValue `
                        -RepoRoot $repoRoot `
                        -InputPath $repairedOutput `
                        -AllowMissing
                }
                Ensure-PathParent -Path $resolvedRepairedOutput
                $reviewJsonOutput = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "review_json_output"
                if ([string]::IsNullOrWhiteSpace($reviewJsonOutput)) {
                    $reviewJsonOutput = Join-Path $entryDir "schema_patch_review.json"
                }
                $resolvedReviewJsonOutput = Resolve-ManifestPathValue `
                    -RepoRoot $repoRoot `
                    -InputPath $reviewJsonOutput `
                    -AllowMissing
                Ensure-PathParent -Path $resolvedReviewJsonOutput
                $approvalResultPath = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "approval_result"
                if ([string]::IsNullOrWhiteSpace($approvalResultPath)) {
                    $approvalResultPath = Join-Path $entryDir "schema_patch_approval_result.json"
                }
                $resolvedApprovalResultPath = Resolve-ManifestPathValue `
                    -RepoRoot $repoRoot `
                    -InputPath $approvalResultPath `
                    -AllowMissing
                Ensure-PathParent -Path $resolvedApprovalResultPath

                $targetMode = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "target_mode"
                if ([string]::IsNullOrWhiteSpace($targetMode)) {
                    $targetMode = "default"
                }

                $logPath = Join-Path $entryDir "schema_baseline.log"
                $result = Invoke-BaselineCheck `
                    -ScriptPath $baselineScriptPath `
                    -InputDocx $resolvedInputDocx `
                    -SchemaFile $resolvedSchemaFile `
                    -GeneratedSchemaOutput $resolvedGeneratedOutput `
                    -RepairedSchemaOutput $resolvedRepairedOutput `
                    -ReviewJsonOutput $resolvedReviewJsonOutput `
                    -BuildDir $BuildDir `
                    -TargetMode $targetMode `
                    -SkipBuildRequested ([bool]$SkipBuild.IsPresent) `
                    -LogPath $logPath

                $schemaBaselineResult.schema_file = $resolvedSchemaFile
                $schemaBaselineResult.generated_output = $resolvedGeneratedOutput
                $schemaBaselineResult.repaired_output = $resolvedRepairedOutput
                $schemaBaselineResult.schema_patch_review_output_path = $resolvedReviewJsonOutput
                $schemaBaselineResult.schema_patch_approval_result_path = $resolvedApprovalResultPath
                $schemaBaselineResult.target_mode = $targetMode
                $schemaBaselineResult.exit_code = [int]$result.ExitCode
                $schemaBaselineResult.matches = [bool]$result.Json.matches
                $schemaBaselineResult.schema_lint_clean = [bool]$result.LintJson.clean
                $schemaBaselineResult.schema_lint_issue_count = [int]$result.LintJson.issue_count
                $schemaBaselineResult.schema_lint_duplicate_target_count = [int]$result.LintJson.duplicate_target_count
                $schemaBaselineResult.schema_lint_duplicate_slot_count = [int]$result.LintJson.duplicate_slot_count
                $schemaBaselineResult.schema_lint_target_order_issue_count = [int]$result.LintJson.target_order_issue_count
                $schemaBaselineResult.schema_lint_slot_order_issue_count = [int]$result.LintJson.slot_order_issue_count
                $schemaBaselineResult.schema_lint_entry_name_issue_count = [int]$result.LintJson.entry_name_issue_count
                $schemaBaselineResult.repaired_schema_output_path = if ($null -ne $result.RepairJson) {
                    [string]$result.RepairJson.output_path
                } else {
                    ""
                }
                $schemaBaselineResult.log_path = $logPath
                $schemaBaselineResult.result = $result.Json
                $schemaBaselineResult.lint_result = $result.LintJson
                if ($null -ne $result.ReviewSummary) {
                    $schemaBaselineResult.schema_patch_review = $result.ReviewSummary
                }
                if ($null -ne $result.ReviewJson) {
                    $schemaBaselineResult.schema_patch_review_json = $result.ReviewJson
                }
                $approvalResult = $null
                if ($null -ne $result.ReviewSummary -and [bool]$result.ReviewSummary.changed) {
                    if (-not [System.IO.File]::Exists($resolvedApprovalResultPath)) {
                        $approvalTemplate = New-SchemaPatchApprovalResultTemplate `
                            -Name $name `
                            -SchemaBaseline $schemaBaselineResult
                        ($approvalTemplate | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $resolvedApprovalResultPath -Encoding UTF8
                    }
                    $approvalResult = Read-JsonFileIfPresent -Path $resolvedApprovalResultPath
                    if ($null -ne $approvalResult) {
                        $schemaBaselineResult.schema_patch_approval_result = $approvalResult
                    }
                }
                $schemaPatchApproval = New-SchemaPatchApprovalSummary `
                    -Name $name `
                    -SchemaBaseline $schemaBaselineResult `
                    -ApprovalResult $approvalResult `
                    -ApprovalResultPath $resolvedApprovalResultPath
                if ($null -ne $schemaPatchApproval) {
                    $schemaBaselineResult.schema_patch_approval = $schemaPatchApproval
                }
                if ($null -ne $result.RepairJson) {
                    $schemaBaselineResult.repair_result = $result.RepairJson
                }
                if (-not [bool]$result.Json.matches) {
                    $entryIssues.Add("Schema baseline drift detected.") | Out-Null
                    $entryPassed = $false
                }
                if (-not [bool]$result.LintJson.clean) {
                    $entryIssues.Add("Schema baseline lint failed with $([int]$result.LintJson.issue_count) issue(s).") | Out-Null
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $schemaBaselineResult.matches = $false
                $entryPassed = $false
            }
        }

        $visualSmoke = $entry.PSObject.Properties["visual_smoke"]
        if ($null -ne $visualSmoke -and $null -ne $visualSmoke.Value) {
            $visualSmokeEnabled = $true
            if ($visualSmoke.Value -is [bool]) {
                $visualSmokeEnabled = [bool]$visualSmoke.Value
            } else {
                $enabledValue = Resolve-OptionalManifestPropertyValue -Entry $visualSmoke.Value -Name "enabled"
                if (-not [string]::IsNullOrWhiteSpace($enabledValue)) {
                    $visualSmokeEnabled = [System.Convert]::ToBoolean($enabledValue)
                }
            }

            if ($visualSmokeEnabled) {
                $visualSmokeResult.enabled = $true
                if ($SkipVisualSmoke) {
                    $visualSmokeResult.review_status = "skipped"
                    $visualSmokeResult.review_verdict = "skipped"
                    $visualSmokeResult.findings_count = 0
                    $visualSmokeResult.reason = "Skipped by -SkipVisualSmoke."
                } else {
                    try {
                        $visualOutputDir = if ($visualSmoke.Value -is [bool]) {
                            Join-Path $entryDir "visual-smoke"
                        } else {
                            $configuredOutputDir = Resolve-OptionalManifestPropertyValue -Entry $visualSmoke.Value -Name "output_dir"
                            if ([string]::IsNullOrWhiteSpace($configuredOutputDir)) {
                                Join-Path $entryDir "visual-smoke"
                            } else {
                                Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $configuredOutputDir -AllowMissing
                            }
                        }

                        $logPath = Join-Path $entryDir "visual_smoke.log"
                        $result = Invoke-VisualSmoke `
                            -ScriptPath $visualSmokeScriptPath `
                            -InputDocx $resolvedInputDocx `
                            -OutputDir $visualOutputDir `
                            -Dpi $Dpi `
                            -KeepWordOpenRequested ([bool]$KeepWordOpen.IsPresent) `
                            -VisibleWordRequested ([bool]$VisibleWord.IsPresent) `
                            -LogPath $logPath

                        $visualSmokeResult.output_dir = $visualOutputDir
                        $visualSmokeResult.log_path = $logPath
                        $visualSmokeResult.summary_json = $result.SummaryPath
                        $visualSmokeResult.review_result_json = $result.ReviewResultPath
                        $visualSmokeResult.contact_sheet = $result.ContactSheetPath
                        $visualSmokeResult.page_count = [int]$result.Summary.page_count
                        $visualSmokeResult.review_status = [string]$result.ReviewResult.status
                        $visualSmokeResult.review_verdict = Normalize-ProjectTemplateSmokeVisualVerdict -Value ([string]$result.ReviewResult.verdict)
                        $visualSmokeResult.findings_count = @(Get-OptionalObjectArrayProperty -Object $result.ReviewResult -Name "findings").Count
                        $visualReviewState = Get-ProjectTemplateSmokeVisualReviewState `
                            -ReviewStatus $visualSmokeResult.review_status `
                            -ReviewVerdict $visualSmokeResult.review_verdict
                        if ($visualReviewState.failed) {
                            $entryPassed = $false
                        }
                        $manualReviewPending = $visualReviewState.manual_review_pending
                        $visualReviewUndetermined = $visualReviewState.review_undetermined
                    } catch {
                        $entryIssues.Add($_.Exception.Message) | Out-Null
                        $visualSmokeResult.review_status = "failed"
                        $visualSmokeResult.review_verdict = "fail"
                        $visualSmokeResult.findings_count = 0
                        $entryPassed = $false
                    }
                }
            }
        }
    }

    $status = Get-ProjectTemplateSmokeEntryStatus `
        -EntryPassed $entryPassed `
        -ManualReviewPending $manualReviewPending `
        -VisualReviewUndetermined $visualReviewUndetermined

    if (-not $entryPassed) {
        $failedEntryCount += 1
    }
    if ([bool](Get-OptionalObjectPropertyObject -Object $schemaBaselineResult -Name "enabled") -and
        $null -ne (Get-OptionalObjectPropertyObject -Object $schemaBaselineResult -Name "schema_lint_clean") -and
        -not [bool](Get-OptionalObjectPropertyObject -Object $schemaBaselineResult -Name "schema_lint_clean")) {
        $dirtySchemaBaselineCount += 1
    }
    if ($manualReviewPending) {
        $manualReviewPendingCount += 1
    }
    if ($visualReviewUndetermined) {
        $visualReviewUndeterminedCount += 1
    }

    $results.Add([pscustomobject]@{
        name = $name
        input_docx = $resolvedInputDocx
        artifact_dir = $entryDir
        status = $status
        passed = $entryPassed
        manual_review_pending = $manualReviewPending
        checks = [ordered]@{
            template_validations = $templateValidationResults.ToArray()
            schema_validation = $schemaValidationResult
            schema_baseline = $schemaBaselineResult
            visual_smoke = $visualSmokeResult
        }
        issues = $entryIssues.ToArray()
    }) | Out-Null
}

$summaryPassed = $failedEntryCount -eq 0
$visualSmokeResults = @(
    foreach ($result in $results) {
        $checks = Get-OptionalObjectPropertyObject -Object $result -Name "checks"
        $visualSmoke = Get-OptionalObjectPropertyObject -Object $checks -Name "visual_smoke"
        if ($null -ne $visualSmoke -and [bool](Get-OptionalObjectPropertyObject -Object $visualSmoke -Name "enabled")) {
            $visualSmoke
        }
    }
)
$visualEntryCount = $visualSmokeResults.Count
$schemaPatchReviews = @(
    foreach ($result in $results) {
        $checks = Get-OptionalObjectPropertyObject -Object $result -Name "checks"
        $schemaBaseline = Get-OptionalObjectPropertyObject -Object $checks -Name "schema_baseline"
        $reviewSummary = Get-OptionalObjectPropertyObject -Object $schemaBaseline -Name "schema_patch_review"
        if ($null -ne $reviewSummary) {
            [pscustomobject]@{
                name = [string]$result.name
                review_json = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "schema_patch_review_output_path"
                changed = [bool]$reviewSummary.changed
                baseline_slot_count = [int]$reviewSummary.baseline_slot_count
                generated_slot_count = [int]$reviewSummary.generated_slot_count
                upsert_slot_count = [int]$reviewSummary.upsert_slot_count
                remove_target_count = [int]$reviewSummary.remove_target_count
                remove_slot_count = [int]$reviewSummary.remove_slot_count
                rename_slot_count = [int]$reviewSummary.rename_slot_count
                update_slot_count = [int]$reviewSummary.update_slot_count
                inserted_slots = [int]$reviewSummary.inserted_slots
                replaced_slots = [int]$reviewSummary.replaced_slots
            }
        }
    }
)
$schemaPatchReviewChangedCount = @($schemaPatchReviews | Where-Object { $_.changed }).Count
$schemaPatchApprovalItems = @(
    foreach ($result in $results) {
        $checks = Get-OptionalObjectPropertyObject -Object $result -Name "checks"
        $schemaBaseline = Get-OptionalObjectPropertyObject -Object $checks -Name "schema_baseline"
        $schemaPatchApproval = Get-OptionalObjectPropertyObject -Object $schemaBaseline -Name "schema_patch_approval"
        if ($null -ne $schemaPatchApproval -and [bool]$schemaPatchApproval.required) {
            $schemaPatchApproval
        }
    }
)
$schemaPatchApprovalPendingCount = @($schemaPatchApprovalItems | Where-Object { [bool]$_.pending }).Count
$schemaPatchApprovalApprovedCount = @($schemaPatchApprovalItems | Where-Object { [bool]$_.approved }).Count
$schemaPatchApprovalRejectedCount = @($schemaPatchApprovalItems | Where-Object { $_.status -in @("rejected", "needs_changes", "invalid_result") }).Count
$schemaPatchApprovalInvalidResultCount = @($schemaPatchApprovalItems | Where-Object { $_.status -eq "invalid_result" }).Count
$schemaPatchApprovalComplianceIssueCount = 0
foreach ($item in @($schemaPatchApprovalItems)) {
    $itemComplianceIssueCount = Get-OptionalObjectPropertyValue -Object $item -Name "compliance_issue_count"
    if (-not [string]::IsNullOrWhiteSpace($itemComplianceIssueCount)) {
        $schemaPatchApprovalComplianceIssueCount += [int]$itemComplianceIssueCount
    }
}
$schemaPatchApprovalItemCount = @($schemaPatchApprovalItems).Count
$schemaPatchApprovalGateStatus = Get-SchemaPatchApprovalGateStatus `
    -PendingCount $schemaPatchApprovalPendingCount `
    -ApprovalItemCount $schemaPatchApprovalItemCount `
    -InvalidResultCount $schemaPatchApprovalInvalidResultCount `
    -ComplianceIssueCount $schemaPatchApprovalComplianceIssueCount
$schemaPatchApprovalGateBlocked = $schemaPatchApprovalGateStatus -eq "blocked"
$visualVerdict = Get-ProjectTemplateSmokeVisualVerdict -VisualSmokeResults $visualSmokeResults
$overallStatus = Get-ProjectTemplateSmokeOverallStatus `
    -SummaryPassed $summaryPassed `
    -ManualReviewPendingCount $manualReviewPendingCount `
    -VisualReviewUndeterminedCount $visualReviewUndeterminedCount

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    entry_count = $results.Count
    failed_entry_count = $failedEntryCount
    dirty_schema_baseline_count = $dirtySchemaBaselineCount
    schema_patch_review_count = $schemaPatchReviews.Count
    schema_patch_review_changed_count = $schemaPatchReviewChangedCount
    schema_patch_reviews = $schemaPatchReviews
    schema_patch_approval_pending_count = $schemaPatchApprovalPendingCount
    schema_patch_approval_approved_count = $schemaPatchApprovalApprovedCount
    schema_patch_approval_rejected_count = $schemaPatchApprovalRejectedCount
    schema_patch_approval_compliance_issue_count = $schemaPatchApprovalComplianceIssueCount
    schema_patch_approval_invalid_result_count = $schemaPatchApprovalInvalidResultCount
    schema_patch_approval_gate_status = $schemaPatchApprovalGateStatus
    schema_patch_approval_gate_blocked = $schemaPatchApprovalGateBlocked
    schema_patch_approval_items = $schemaPatchApprovalItems
    visual_entry_count = $visualEntryCount
    visual_verdict = $visualVerdict
    manual_review_pending_count = $manualReviewPendingCount
    visual_review_undetermined_count = $visualReviewUndeterminedCount
    passed = $summaryPassed
    overall_status = $overallStatus
    entries = $results
}

($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-SummaryMarkdown -Path $summaryMarkdownPath -RepoRoot $repoRoot -Summary $summary

Write-Step "Summary JSON: $summaryPath"
Write-Step "Summary Markdown: $summaryMarkdownPath"
Write-Step "Overall status: $overallStatus"

if (-not $summaryPassed) {
    exit 1
}

exit 0
