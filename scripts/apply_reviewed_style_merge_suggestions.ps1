<#
.SYNOPSIS
Applies duplicate style merge suggestions after review approval.

.DESCRIPTION
Reads a featherdoc.style_merge_suggestion_review.v1 record, validates that the
referenced suggest-style-merges plan is fully reviewed, then invokes
featherdoc_cli apply-style-refactor with a rollback plan. This script writes a
DOCX output only after the review gate passes.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$ReviewJson,
    [string]$OutputDocx = "",
    [string]$RollbackPlan = "",
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [string]$CliPath = "",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[apply-reviewed-style-merge] $Message"
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
    param($Object, [string[]]$Names, [string]$DefaultValue = "")

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Object -Name $name
        if ($null -ne $value -and -not [string]::IsNullOrWhiteSpace([string]$value)) {
            return [string]$value
        }
    }
    return $DefaultValue
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

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$InputPath, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($InputPath)) { return "" }
    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $InputPath -AllowMissing:$AllowMissing
}

function Resolve-ReviewEvidencePath {
    param(
        [string]$ReviewJsonPath,
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) { return "" }
    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        if ($AllowMissing) { return [System.IO.Path]::GetFullPath($InputPath) }
        return (Resolve-Path -LiteralPath $InputPath).Path
    }

    $reviewDir = [System.IO.Path]::GetDirectoryName($ReviewJsonPath)
    if (-not [string]::IsNullOrWhiteSpace($reviewDir)) {
        $reviewRelativePath = [System.IO.Path]::GetFullPath((Join-Path $reviewDir $InputPath))
        if ($AllowMissing -or (Test-Path -LiteralPath $reviewRelativePath)) {
            return $reviewRelativePath
        }
    }

    $repoRelativePath = Join-Path $RepoRoot $InputPath
    if ($AllowMissing) { return [System.IO.Path]::GetFullPath($repoRelativePath) }
    return (Resolve-Path -LiteralPath $repoRelativePath).Path
}

function Convert-ToPortableRelativePath {
    param([string]$BasePath, [string]$TargetPath)

    if ([string]::IsNullOrWhiteSpace($TargetPath)) { return "" }
    $resolvedBasePath = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $resolvedBasePath.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedBasePath.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedBasePath += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolvedTargetPath = [System.IO.Path]::GetFullPath($TargetPath)

    if ([System.IO.Path]::GetPathRoot($resolvedBasePath) -ne
        [System.IO.Path]::GetPathRoot($resolvedTargetPath)) {
        return $resolvedTargetPath.Replace('\', '/')
    }

    $baseUri = New-Object System.Uri($resolvedBasePath)
    $targetUri = New-Object System.Uri($resolvedTargetPath)
    return [System.Uri]::UnescapeDataString(
        $baseUri.MakeRelativeUri($targetUri).ToString()
    ).Replace('\', '/')
}

function Ensure-ParentDirectory {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
}

function Read-JsonObject {
    param([string]$Path)
    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Read-JsonObjectFromLines {
    param([string[]]$Lines, [string]$Command)

    foreach ($line in @($Lines)) {
        $text = [string]$line
        if (-not $text.TrimStart().StartsWith("{")) {
            continue
        }

        try {
            $object = $text | ConvertFrom-Json
            $commandValue = Get-JsonString -Object $object -Names @("command")
            if ($commandValue -eq $Command) {
                return $object
            }
        } catch {
            continue
        }
    }

    throw "Command '$Command' did not emit a JSON object."
}

function ConvertTo-CommandLine {
    param([string[]]$Arguments)

    $quoted = foreach ($argument in @($Arguments)) {
        if ($argument -match '[\s"`]') {
            '"' + ($argument -replace '"', '\"') + '"'
        } else {
            $argument
        }
    }
    return ($quoted -join ' ')
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedReviewJson = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ReviewJson
$reviewDir = [System.IO.Path]::GetDirectoryName($resolvedReviewJson)
if ([string]::IsNullOrWhiteSpace($reviewDir)) {
    $reviewDir = (Get-Location).Path
}

$resolvedOutputDocx = if ([string]::IsNullOrWhiteSpace($OutputDocx)) {
    $inputBaseName = [System.IO.Path]::GetFileNameWithoutExtension($resolvedInputDocx)
    Join-Path $reviewDir ($inputBaseName + ".style-merge.applied.docx")
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDocx -AllowMissing
}
$resolvedSummaryJson = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    $outputDirectory = [System.IO.Path]::GetDirectoryName($resolvedOutputDocx)
    $outputBaseName = [System.IO.Path]::GetFileNameWithoutExtension($resolvedOutputDocx)
    Join-Path $outputDirectory ($outputBaseName + ".summary.json")
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SummaryJson -AllowMissing
}

if ([System.IO.Path]::GetFullPath($resolvedInputDocx) -eq [System.IO.Path]::GetFullPath($resolvedOutputDocx)) {
    throw "OutputDocx must not overwrite InputDocx."
}

Write-Step "Reading review JSON: $resolvedReviewJson"
$review = Read-JsonObject -Path $resolvedReviewJson
$decision = (Get-JsonString -Object $review -Names @("decision", "status")).ToLowerInvariant()
if ($decision -notin @("reviewed", "approved", "accepted")) {
    throw "Style merge suggestions are not approved for apply. Decision='$decision'."
}

$reviewedSuggestionCount = Get-JsonInt `
    -Object $review `
    -Names @("reviewed_suggestion_count", "style_merge_suggestion_count", "suggestion_count") `
    -DefaultValue 0
$reviewer = Get-JsonString -Object $review -Names @("reviewed_by", "reviewer")
$reviewedAt = Get-JsonString -Object $review -Names @("reviewed_at")
$planReference = Get-JsonString `
    -Object $review `
    -Names @("style_refactor_plan_file", "style_refactor_plan_path", "style_merge_plan_file", "style_merge_plan_path", "style_merge_suggestion_plan_file", "style_merge_suggestion_plan_path", "plan_file", "plan_path")
if ([string]::IsNullOrWhiteSpace($planReference)) {
    throw "Style merge review JSON does not reference a plan file."
}

$resolvedPlanFile = Resolve-ReviewEvidencePath `
    -ReviewJsonPath $resolvedReviewJson `
    -RepoRoot $repoRoot `
    -InputPath $planReference
$rollbackReference = if ([string]::IsNullOrWhiteSpace($RollbackPlan)) {
    Get-JsonString `
        -Object $review `
        -Names @("style_refactor_rollback_plan_file", "style_refactor_rollback_plan_path", "rollback_plan_file", "rollback_plan_path", "rollback_plan")
} else {
    $RollbackPlan
}
$resolvedRollbackPlan = if ([string]::IsNullOrWhiteSpace($rollbackReference)) {
    $planBaseName = [System.IO.Path]::GetFileNameWithoutExtension($resolvedPlanFile)
    Join-Path $reviewDir ($planBaseName + ".apply.rollback.json")
} elseif ([string]::IsNullOrWhiteSpace($RollbackPlan)) {
    Resolve-ReviewEvidencePath `
        -ReviewJsonPath $resolvedReviewJson `
        -RepoRoot $repoRoot `
        -InputPath $rollbackReference `
        -AllowMissing
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $rollbackReference -AllowMissing
}

Write-Step "Reading style merge plan: $resolvedPlanFile"
$plan = Read-JsonObject -Path $resolvedPlanFile
$operations = @(Get-JsonArray -Object $plan -Name "operations")
$confidenceSummary = Get-JsonProperty -Object $plan -Name "suggestion_confidence_summary"
$planSuggestionCount = Get-JsonInt `
    -Object $confidenceSummary `
    -Names @("suggestion_count") `
    -DefaultValue (Get-JsonInt -Object $plan -Names @("operation_count") -DefaultValue $operations.Count)
if ($reviewedSuggestionCount -lt $planSuggestionCount) {
    throw "Reviewed suggestion count ($reviewedSuggestionCount) does not cover plan suggestion count ($planSuggestionCount)."
}

Write-Step "Resolving featherdoc_cli"
$resolvedCliPath = if ([string]::IsNullOrWhiteSpace($CliPath)) {
    Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $BuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $CliPath
}

Ensure-ParentDirectory -Path $resolvedOutputDocx
Ensure-ParentDirectory -Path $resolvedRollbackPlan
Ensure-ParentDirectory -Path $resolvedSummaryJson

$arguments = @(
    "apply-style-refactor",
    $resolvedInputDocx,
    "--plan-file",
    $resolvedPlanFile,
    "--rollback-plan",
    $resolvedRollbackPlan,
    "--output",
    $resolvedOutputDocx,
    "--json"
)

Write-Step "Applying reviewed style merge plan"
$result = Invoke-TemplateSchemaCli -ExecutablePath $resolvedCliPath -Arguments $arguments
foreach ($line in @($result.Output)) {
    Write-Host $line
}

$cliJson = $null
try {
    $cliJson = Read-JsonObjectFromLines -Lines $result.Output -Command "apply-style-refactor"
} catch {
    if ($result.ExitCode -eq 0) {
        throw
    }
}
if ($result.ExitCode -ne 0) {
    $message = if ([string]::IsNullOrWhiteSpace($result.Text)) {
        "featherdoc_cli apply-style-refactor failed with exit code $($result.ExitCode)."
    } else {
        $result.Text
    }
    throw $message
}

$summaryBasePath = [System.IO.Path]::GetDirectoryName($resolvedSummaryJson)
if ([string]::IsNullOrWhiteSpace($summaryBasePath)) {
    $summaryBasePath = (Get-Location).Path
}
$summary = [ordered]@{
    schema = "featherdoc.reviewed_style_merge_apply.v1"
    generated_at = (Get-Date).ToString("s")
    status = "applied"
    input_docx = $resolvedInputDocx
    input_docx_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedInputDocx
    output_docx = $resolvedOutputDocx
    output_docx_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedOutputDocx
    output_docx_exists = Test-Path -LiteralPath $resolvedOutputDocx
    review_json = $resolvedReviewJson
    review_json_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedReviewJson
    decision = $decision
    reviewed_by = $reviewer
    reviewed_at = $reviewedAt
    reviewed_suggestion_count = $reviewedSuggestionCount
    style_merge_suggestion_count = $planSuggestionCount
    plan_file = $resolvedPlanFile
    plan_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedPlanFile
    plan_exists = Test-Path -LiteralPath $resolvedPlanFile
    rollback_plan = $resolvedRollbackPlan
    rollback_plan_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedRollbackPlan
    rollback_plan_exists = Test-Path -LiteralPath $resolvedRollbackPlan
    command = "featherdoc_cli " + (ConvertTo-CommandLine -Arguments $arguments)
    cli_exit_code = $result.ExitCode
    cli_result = $cliJson
}

($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8

Write-Step "Output DOCX: $resolvedOutputDocx"
Write-Step "Rollback plan: $resolvedRollbackPlan"
Write-Step "Summary JSON: $resolvedSummaryJson"
