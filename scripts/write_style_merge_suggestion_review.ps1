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

function Convert-ToRelativePath {
    param([string]$BasePath, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $baseDirectory = [System.IO.Path]::GetDirectoryName([System.IO.Path]::GetFullPath($BasePath))
    if ([string]::IsNullOrWhiteSpace($baseDirectory)) {
        $baseDirectory = (Get-Location).Path
    }
    $targetPath = [System.IO.Path]::GetFullPath($Path)

    try {
        return ([System.IO.Path]::GetRelativePath($baseDirectory, $targetPath)).Replace('\', '/')
    } catch {
        if ([System.IO.Path]::GetPathRoot($baseDirectory) -ne
            [System.IO.Path]::GetPathRoot($targetPath)) {
            return $targetPath.Replace('\', '/')
        }

        if (-not $baseDirectory.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
            -not $baseDirectory.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
            $baseDirectory += [System.IO.Path]::DirectorySeparatorChar
        }

        $baseUri = New-Object System.Uri($baseDirectory)
        $targetUri = New-Object System.Uri($targetPath)
        return [System.Uri]::UnescapeDataString(
            $baseUri.MakeRelativeUri($targetUri).ToString()
        ).Replace('\', '/')
    }
}

function Resolve-ReviewPath {
    param([string]$RepoRoot, [string]$InputPath, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($InputPath)) { return "" }
    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $InputPath -AllowMissing:$AllowMissing
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

$summary = [ordered]@{
    schema = "featherdoc.style_merge_suggestion_review.v1"
    generated_at = (Get-Date).ToString("s")
    decision = $normalizedDecision
    reviewed_by = $Reviewer
    reviewed_at = $reviewedAtValue
    style_merge_suggestion_count = $suggestionCount
    reviewed_suggestion_count = $reviewedCount
    plan_file = Convert-ToRelativePath -BasePath $resolvedOutputJson -Path $resolvedPlanFile
    plan_path = $resolvedPlanFile
    plan_exists = (Test-Path -LiteralPath $resolvedPlanFile)
    rollback_plan_file = Convert-ToRelativePath -BasePath $resolvedOutputJson -Path $resolvedRollbackPlanFile
    rollback_plan_path = $resolvedRollbackPlanFile
    rollback_plan_exists = if ([string]::IsNullOrWhiteSpace($resolvedRollbackPlanFile)) { $false } else { Test-Path -LiteralPath $resolvedRollbackPlanFile }
    suggestion_confidence_summary = $confidenceSummary
}

$outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedOutputJson)
if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}
($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedOutputJson -Encoding UTF8

Write-Step "Review JSON: $resolvedOutputJson"
Write-Step "Decision: $normalizedDecision"
Write-Step "Reviewed suggestions: $reviewedCount / $suggestionCount"
