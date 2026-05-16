<#
.SYNOPSIS
Audits whether a style merge rollback plan can be restored.

.DESCRIPTION
Runs featherdoc_cli restore-style-merge in dry-run mode against a rollback plan
and writes a stable featherdoc.style_merge_restore_audit.v1 summary. The script
is read-only for DOCX files and can consume the summary produced by
apply_reviewed_style_merge_suggestions.ps1.
#>
param(
    [string]$InputDocx = "",
    [string]$RollbackPlan = "",
    [string]$ApplySummaryJson = "",
    [string]$SummaryJson = "",
    [int[]]$Entry = @(),
    [string[]]$SourceStyle = @(),
    [string[]]$TargetStyle = @(),
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [string]$CliPath = "",
    [switch]$SkipBuild,
    [switch]$FailOnIssue
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[style-merge-restore-audit] $Message"
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

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$InputPath, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($InputPath)) { return "" }
    return Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $InputPath -AllowMissing:$AllowMissing
}

function Resolve-ReferencePath {
    param(
        [string]$ReferenceJsonPath,
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) { return "" }
    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        if ($AllowMissing) { return [System.IO.Path]::GetFullPath($InputPath) }
        return (Resolve-Path -LiteralPath $InputPath).Path
    }

    $referenceDir = [System.IO.Path]::GetDirectoryName($ReferenceJsonPath)
    if (-not [string]::IsNullOrWhiteSpace($referenceDir)) {
        $referenceRelativePath = [System.IO.Path]::GetFullPath((Join-Path $referenceDir $InputPath))
        if ($AllowMissing -or (Test-Path -LiteralPath $referenceRelativePath)) {
            return $referenceRelativePath
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
$resolvedApplySummaryJson = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $ApplySummaryJson
$applySummary = $null
if (-not [string]::IsNullOrWhiteSpace($resolvedApplySummaryJson)) {
    Write-Step "Reading apply summary: $resolvedApplySummaryJson"
    $applySummary = Read-JsonObject -Path $resolvedApplySummaryJson
}

$inputReference = if (-not [string]::IsNullOrWhiteSpace($InputDocx)) {
    $InputDocx
} elseif ($null -ne $applySummary) {
    Get-JsonString -Object $applySummary -Names @("output_docx", "output_docx_relative_path")
} else {
    ""
}
$rollbackReference = if (-not [string]::IsNullOrWhiteSpace($RollbackPlan)) {
    $RollbackPlan
} elseif ($null -ne $applySummary) {
    Get-JsonString -Object $applySummary -Names @("rollback_plan", "rollback_plan_relative_path", "rollback_plan_file")
} else {
    ""
}

if ([string]::IsNullOrWhiteSpace($inputReference)) {
    throw "InputDocx is required unless ApplySummaryJson provides output_docx."
}
if ([string]::IsNullOrWhiteSpace($rollbackReference)) {
    throw "RollbackPlan is required unless ApplySummaryJson provides rollback_plan."
}

$resolvedInputDocx = if ($null -ne $applySummary -and [string]::IsNullOrWhiteSpace($InputDocx)) {
    Resolve-ReferencePath -ReferenceJsonPath $resolvedApplySummaryJson -RepoRoot $repoRoot -InputPath $inputReference
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $inputReference
}
$resolvedRollbackPlan = if ($null -ne $applySummary -and [string]::IsNullOrWhiteSpace($RollbackPlan)) {
    Resolve-ReferencePath -ReferenceJsonPath $resolvedApplySummaryJson -RepoRoot $repoRoot -InputPath $rollbackReference
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $rollbackReference
}

$resolvedSummaryJson = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    $rollbackDirectory = [System.IO.Path]::GetDirectoryName($resolvedRollbackPlan)
    $rollbackBaseName = [System.IO.Path]::GetFileNameWithoutExtension($resolvedRollbackPlan)
    Join-Path $rollbackDirectory ($rollbackBaseName + ".restore-audit.summary.json")
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -InputPath $SummaryJson -AllowMissing
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

Ensure-ParentDirectory -Path $resolvedSummaryJson

$arguments = New-Object 'System.Collections.Generic.List[string]'
$arguments.Add("restore-style-merge") | Out-Null
$arguments.Add($resolvedInputDocx) | Out-Null
$arguments.Add("--rollback-plan") | Out-Null
$arguments.Add($resolvedRollbackPlan) | Out-Null
foreach ($entryIndex in @($Entry)) {
    $arguments.Add("--entry") | Out-Null
    $arguments.Add([string]$entryIndex) | Out-Null
}
foreach ($styleId in @($SourceStyle)) {
    if (-not [string]::IsNullOrWhiteSpace($styleId)) {
        $arguments.Add("--source-style") | Out-Null
        $arguments.Add($styleId) | Out-Null
    }
}
foreach ($styleId in @($TargetStyle)) {
    if (-not [string]::IsNullOrWhiteSpace($styleId)) {
        $arguments.Add("--target-style") | Out-Null
        $arguments.Add($styleId) | Out-Null
    }
}
$arguments.Add("--dry-run") | Out-Null
$arguments.Add("--json") | Out-Null
$argumentArray = @($arguments)

Write-Step "Auditing style merge restore plan"
$result = Invoke-TemplateSchemaCli -ExecutablePath $resolvedCliPath -Arguments $argumentArray
foreach ($line in @($result.Output)) {
    Write-Host $line
}

$cliJson = $null
try {
    $cliJson = Read-JsonObjectFromLines -Lines $result.Output -Command "restore-style-merge"
} catch {
    if ($result.ExitCode -eq 0) {
        throw
    }
}
if ($result.ExitCode -ne 0) {
    $message = if ([string]::IsNullOrWhiteSpace($result.Text)) {
        "featherdoc_cli restore-style-merge --dry-run failed with exit code $($result.ExitCode)."
    } else {
        $result.Text
    }
    throw $message
}

$issueCount = Get-JsonInt -Object $cliJson -Names @("issue_count") -DefaultValue 0
$status = if ($issueCount -eq 0) { "clean" } else { "needs_review" }
$summaryBasePath = [System.IO.Path]::GetDirectoryName($resolvedSummaryJson)
if ([string]::IsNullOrWhiteSpace($summaryBasePath)) {
    $summaryBasePath = (Get-Location).Path
}
$summary = [ordered]@{
    schema = "featherdoc.style_merge_restore_audit.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    dry_run = $true
    input_docx = $resolvedInputDocx
    input_docx_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedInputDocx
    rollback_plan = $resolvedRollbackPlan
    rollback_plan_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedRollbackPlan
    rollback_plan_exists = Test-Path -LiteralPath $resolvedRollbackPlan
    apply_summary_json = $resolvedApplySummaryJson
    apply_summary_relative_path = Convert-ToPortableRelativePath -BasePath $summaryBasePath -TargetPath $resolvedApplySummaryJson
    selected_entries = @($Entry)
    selected_source_styles = @($SourceStyle)
    selected_target_styles = @($TargetStyle)
    issue_count = $issueCount
    issue_summary = Get-JsonProperty -Object $cliJson -Name "issue_summary"
    restored_count = Get-JsonInt -Object $cliJson -Names @("restored_count") -DefaultValue 0
    restored_style_count = Get-JsonInt -Object $cliJson -Names @("restored_style_count") -DefaultValue 0
    restored_reference_count = Get-JsonInt -Object $cliJson -Names @("restored_reference_count") -DefaultValue 0
    command = "featherdoc_cli " + (ConvertTo-CommandLine -Arguments $argumentArray)
    cli_exit_code = $result.ExitCode
    cli_result = $cliJson
}

($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
Write-Step "Restore audit summary: $resolvedSummaryJson"
Write-Step "Status: $status"
Write-Step "Issue count: $issueCount"

if ($FailOnIssue -and $issueCount -gt 0) {
    throw "Style merge restore audit found $issueCount issue(s)."
}
