<#
.SYNOPSIS
Writes a review JSON record for duplicate style merge suggestions.

.DESCRIPTION
Reads a suggest-style-merges JSON plan and emits a stable
featherdoc.style_merge_suggestion_review.v1 record that can be consumed by
build_document_skeleton_governance_report.ps1 through -StyleMergeReviewJson.
The script is read-only for DOCX files; it only writes the requested JSON
record.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$PlanFile,
    [string]$OutputJson = "",
    [ValidateSet("reviewed", "approved", "accepted", "pending", "deferred", "rejected")]
    [string]$Decision = "approved",
    [string]$Reviewer = "",
    [string]$ReviewedAt = "",
    [int]$ReviewedSuggestionCount = -1,
    [string]$RollbackPlanFile = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[style-merge-review] $Message"
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

function Get-JsonInt {
    param($Object, [string[]]$Names, [int]$DefaultValue = 0)

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            return [int]$value
        }
    }
    return $DefaultValue
}

function Get-NullableJsonInt {
    param($Object, [string[]]$Names)

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
            continue
        }
        $text = [string]$value
        if ($text -notmatch "^-?\d+$") {
            return $null
        }
        return [int]$text
    }
    return $null
}

function Get-JsonString {
    param($Object, [string[]]$Names, [string]$DefaultValue = "")

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            return [string]$value
        }
    }
    return $DefaultValue
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

function Resolve-ReviewPath {
    param([string]$RepoRoot, [string]$InputPath, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($InputPath)) { return "" }
    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $InputPath -AllowMissing:$AllowMissing
}

function New-ManualReviewReasons {
    param([object[]]$Operations, $ConfidenceSummary)

    $recommendedMinConfidence = Get-NullableJsonInt `
        -Object $ConfidenceSummary `
        -Names @("recommended_min_confidence")
    $reasons = New-Object System.Collections.Generic.List[object]
    if ($null -eq $recommendedMinConfidence) {
        return @()
    }

    $index = 0
    foreach ($operation in @($Operations)) {
        $index += 1
        $suggestion = Get-JsonProperty -Object $operation -Name "suggestion"
        $confidence = Get-NullableJsonInt -Object $suggestion -Names @("confidence")
        if ($null -eq $confidence) {
            $confidence = Get-NullableJsonInt -Object $operation -Names @("confidence")
        }
        if ($null -eq $confidence -or $confidence -ge $recommendedMinConfidence) {
            continue
        }

        $sourceStyleId = Get-JsonString -Object $operation -Names @("source_style_id", "source_style")
        $targetStyleId = Get-JsonString -Object $operation -Names @("target_style_id", "target_style")
        $reasonCode = Get-JsonString -Object $suggestion -Names @("reason_code")
        if ([string]::IsNullOrWhiteSpace($reasonCode)) {
            $reasonCode = Get-JsonString -Object $operation -Names @("reason_code")
        }
        $reason = Get-JsonString -Object $suggestion -Names @("reason")
        if ([string]::IsNullOrWhiteSpace($reason)) {
            $reason = "Suggestion confidence is below the recommended minimum confidence."
        }

        $reasons.Add([ordered]@{
            index = $index
            action = Get-JsonString -Object $operation -Names @("action")
            source_style_id = $sourceStyleId
            target_style_id = $targetStyleId
            confidence = $confidence
            recommended_min_confidence = $recommendedMinConfidence
            reason_code = $reasonCode
            review_reason = $reason
            recommended_action = "manual_review_before_apply"
        }) | Out-Null
    }

    return @($reasons.ToArray())
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedPlanFile = Resolve-ReviewPath -RepoRoot $repoRoot -InputPath $PlanFile
$resolvedOutputJson = if ([string]::IsNullOrWhiteSpace($OutputJson)) {
    $planDirectory = [System.IO.Path]::GetDirectoryName($resolvedPlanFile)
    $planBaseName = [System.IO.Path]::GetFileNameWithoutExtension($resolvedPlanFile)
    Join-Path $planDirectory ($planBaseName + ".review.json")
} else {
    Resolve-ReviewPath -RepoRoot $repoRoot -InputPath $OutputJson -AllowMissing
}
$resolvedRollbackPlanFile = Resolve-ReviewPath -RepoRoot $repoRoot -InputPath $RollbackPlanFile -AllowMissing
$summaryBasePath = [System.IO.Path]::GetDirectoryName($resolvedOutputJson)
if ([string]::IsNullOrWhiteSpace($summaryBasePath)) {
    $summaryBasePath = (Get-Location).Path
}

Write-Step "Reading style merge suggestion plan: $resolvedPlanFile"
$plan = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedPlanFile | ConvertFrom-Json
$operations = @(Get-JsonArray -Object $plan -Name "operations")
$confidenceSummary = Get-JsonProperty -Object $plan -Name "suggestion_confidence_summary"
$suggestionCount = Get-JsonInt `
    -Object $confidenceSummary `
    -Names @("suggestion_count") `
    -DefaultValue (Get-JsonInt -Object $plan -Names @("operation_count") -DefaultValue $operations.Count)

$normalizedDecision = $Decision.ToLowerInvariant()
$reviewedCount = if ($ReviewedSuggestionCount -ge 0) {
    $ReviewedSuggestionCount
} elseif ($normalizedDecision -in @("reviewed", "approved", "accepted")) {
    $suggestionCount
} else {
    0
}
if ($reviewedCount -gt $suggestionCount) {
    throw "ReviewedSuggestionCount ($reviewedCount) cannot exceed suggestion count ($suggestionCount)."
}

$reviewedAtValue = if ([string]::IsNullOrWhiteSpace($ReviewedAt)) {
    (Get-Date).ToString("s")
} else {
    $ReviewedAt
}
$manualReviewReasons = @(New-ManualReviewReasons -Operations $operations -ConfidenceSummary $confidenceSummary)

$summary = [ordered]@{
    schema = "featherdoc.style_merge_suggestion_review.v1"
    generated_at = (Get-Date).ToString("s")
    decision = $normalizedDecision
    reviewed_by = $Reviewer
    reviewed_at = $reviewedAtValue
    style_merge_suggestion_count = $suggestionCount
    reviewed_suggestion_count = $reviewedCount
    plan_file = ConvertTo-TemplateSchemaPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedPlanFile
    plan_path = $resolvedPlanFile
    plan_exists = (Test-Path -LiteralPath $resolvedPlanFile)
    rollback_plan_file = ConvertTo-TemplateSchemaPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedRollbackPlanFile
    rollback_plan_path = $resolvedRollbackPlanFile
    rollback_plan_exists = if ([string]::IsNullOrWhiteSpace($resolvedRollbackPlanFile)) { $false } else { Test-Path -LiteralPath $resolvedRollbackPlanFile }
    suggestion_confidence_summary = $confidenceSummary
    manual_review_required = (@($manualReviewReasons).Count -gt 0)
    manual_review_reason_count = @($manualReviewReasons).Count
    manual_review_reasons = @($manualReviewReasons)
}

$outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedOutputJson)
if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}
($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8

Write-Step "Review JSON: $resolvedOutputJson"
Write-Step "Decision: $normalizedDecision"
Write-Step "Reviewed suggestions: $reviewedCount / $suggestionCount"
