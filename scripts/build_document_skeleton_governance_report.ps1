<#
.SYNOPSIS
Builds a document skeleton governance report for one DOCX/DOTX file.

.DESCRIPTION
Exports the current numbering catalog as an exemplar, audits paragraph-style
numbering, and captures style usage in one JSON/Markdown report. The script is
read-only for the input document; generated artifacts are written under
OutputDir.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [string]$OutputDir = "output/document-skeleton-governance",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [string]$CliPath = "",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$SkipBuild,
    [switch]$FailOnIssue
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[document-skeleton-governance] $Message"
}

function Ensure-Directory {
    param([string]$Path)

    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Convert-ToReportPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    try {
        $relative = [System.IO.Path]::GetRelativePath($RepoRoot, $Path)
        if (-not $relative.StartsWith("..")) {
            return $relative.Replace('\', '/')
        }
    } catch {
    }

    return $Path
}

function ConvertTo-CommandLine {
    param([string[]]$Arguments)

    $quoted = foreach ($argument in $Arguments) {
        if ($argument -match '[\s"`]') {
            '"' + ($argument -replace '"', '\"') + '"'
        } else {
            $argument
        }
    }

    return ($quoted -join ' ')
}

function Get-JsonPropertyValue {
    param(
        $Object,
        [string[]]$Names,
        $DefaultValue = $null
    )

    if ($null -eq $Object) {
        return $DefaultValue
    }

    foreach ($name in $Names) {
        $property = $Object.PSObject.Properties[$name]
        if ($null -ne $property -and $null -ne $property.Value) {
            return $property.Value
        }
    }

    return $DefaultValue
}

function Get-JsonIntValue {
    param(
        $Object,
        [string[]]$Names,
        [int]$DefaultValue = 0
    )

    $value = Get-JsonPropertyValue -Object $Object -Names $Names -DefaultValue $DefaultValue
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }

    return [int]$value
}

function Get-JsonBoolValue {
    param(
        $Object,
        [string[]]$Names,
        [bool]$DefaultValue = $false
    )

    $value = Get-JsonPropertyValue -Object $Object -Names $Names -DefaultValue $DefaultValue
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }

    return [bool]$value
}

function Get-JsonArrayValue {
    param(
        $Object,
        [string[]]$Names
    )

    $value = Get-JsonPropertyValue -Object $Object -Names $Names -DefaultValue @()
    if ($null -eq $value) {
        return @()
    }

    return @($value)
}

function Read-JsonFileOrNull {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Read-JsonObjectFromLines {
    param(
        [string[]]$Lines,
        [string]$Command = ""
    )

    foreach ($line in $Lines) {
        $text = [string]$line
        if (-not $text.TrimStart().StartsWith("{")) {
            continue
        }

        try {
            $object = $text | ConvertFrom-Json
            if ([string]::IsNullOrWhiteSpace($Command)) {
                return $object
            }

            $property = $object.PSObject.Properties["command"]
            if ($null -ne $property -and [string]$property.Value -eq $Command) {
                return $object
            }
        } catch {
            continue
        }
    }

    if ([string]::IsNullOrWhiteSpace($Command)) {
        throw "No JSON object was emitted."
    }

    throw "Command '$Command' did not emit a JSON object."
}

function Invoke-CliJson {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments,
        [string]$Command = ""
    )

    $result = Invoke-TemplateSchemaCli -ExecutablePath $ExecutablePath -Arguments $Arguments
    foreach ($line in $result.Output) {
        Write-Host $line
    }

    $json = $null
    $errorMessage = ""
    try {
        $json = Read-JsonObjectFromLines -Lines $result.Output -Command $Command
    } catch {
        $errorMessage = $_.Exception.Message
    }

    if ($result.ExitCode -ne 0 -and [string]::IsNullOrWhiteSpace($errorMessage)) {
        $errorMessage = "featherdoc_cli $($Arguments -join ' ') failed with exit code $($result.ExitCode)."
    }

    return [ordered]@{
        command = if ([string]::IsNullOrWhiteSpace($Command)) { $Arguments[0] } else { $Command }
        arguments = @($Arguments)
        exit_code = [int]$result.ExitCode
        ok = ([int]$result.ExitCode -eq 0 -and $null -ne $json)
        error = $errorMessage
        json = $json
    }
}

function New-IssueSummary {
    param($Issues)

    return @($Issues) |
        Group-Object {
            $issueName = Get-JsonPropertyValue -Object $_ -Names @("issue", "kind", "code") -DefaultValue "unspecified"
            if ([string]::IsNullOrWhiteSpace([string]$issueName)) {
                "unspecified"
            } else {
                [string]$issueName
            }
        } |
        Sort-Object @{ Expression = "Count"; Descending = $true }, Name |
        ForEach-Object {
            [ordered]@{
                issue = $_.Name
                count = [int]$_.Count
            }
        }
}

function New-ActionItem {
    param(
        [string]$Id,
        [string]$Title,
        [string]$Command
    )

    return [ordered]@{
        id = $Id
        title = $Title
        command = $Command
    }
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Document Skeleton Governance Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add(("- input_docx: ``{0}``" -f $Summary.input_docx)) | Out-Null
    $lines.Add(("- status: ``{0}``" -f $Summary.status)) | Out-Null
    $lines.Add(("- exemplar_catalog: ``{0}``" -f $Summary.exemplar_catalog_path)) | Out-Null
    $lines.Add(("- style_numbering_issue_count: ``{0}``" -f $Summary.style_numbering_issue_count)) | Out-Null
    $lines.Add(("- style_numbering_suggestion_count: ``{0}``" -f $Summary.style_numbering_suggestion_count)) | Out-Null
    $lines.Add(("- release_blocker_count: ``{0}``" -f $Summary.release_blocker_count)) | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("## Issue Summary") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.issue_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($issue in @($Summary.issue_summary)) {
            $lines.Add(("- ``{0}``: {1}" -f $issue.issue, $issue.count)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Suggested Next Steps") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($item in @($Summary.next_steps)) {
        $lines.Add(("- ``{0}``: {1}" -f $item.id, $item.title)) | Out-Null
        if ($item.command) {
            $lines.Add("") | Out-Null
            $lines.Add('  ```powershell') | Out-Null
            $lines.Add(("  {0}" -f $item.command)) | Out-Null
            $lines.Add('  ```') | Out-Null
            $lines.Add("") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Command Results") | Out-Null
    $lines.Add("") | Out-Null
    foreach ($command in @($Summary.commands)) {
        $lines.Add(("- ``{0}``: ok=``{1}``, exit=``{2}``" -f $command.command, $command.ok, $command.exit_code)) | Out-Null
        if (-not [string]::IsNullOrWhiteSpace([string]$command.error)) {
            $lines.Add(("  - error: {0}" -f $command.error)) | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add(("- ``{0}``: {1}" -f $blocker.id, $blocker.message)) | Out-Null
        }
    }

    return @($lines)
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedOutputDir = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "document_skeleton_governance.summary.json"
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "document_skeleton_governance.md"
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $ReportMarkdown -AllowMissing
}
$catalogPath = Join-Path $resolvedOutputDir "exemplar.numbering-catalog.json"
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($catalogPath))

Write-Step "Resolving featherdoc_cli"
$resolvedCliPath = if ([string]::IsNullOrWhiteSpace($CliPath)) {
    Resolve-TemplateSchemaCliPath -RepoRoot $repoRoot -BuildDir $BuildDir -Generator $Generator -SkipBuild:$SkipBuild
} else {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $CliPath
}

Write-Step "Exporting exemplar numbering catalog"
$exportResult = Invoke-CliJson -ExecutablePath $resolvedCliPath -Command "export-numbering-catalog" -Arguments @(
    "export-numbering-catalog",
    $resolvedInputDocx,
    "--output",
    $catalogPath,
    "--json"
)

Write-Step "Auditing style numbering"
$styleNumberingResult = Invoke-CliJson -ExecutablePath $resolvedCliPath -Command "audit-style-numbering" -Arguments @(
    "audit-style-numbering",
    $resolvedInputDocx,
    "--json"
)

Write-Step "Collecting style usage"
$styleUsageResult = Invoke-CliJson -ExecutablePath $resolvedCliPath -Command "inspect-styles" -Arguments @(
    "inspect-styles",
    $resolvedInputDocx,
    "--usage",
    "--json"
)

$catalogJson = Read-JsonFileOrNull -Path $catalogPath
$styleNumbering = $styleNumberingResult.json
$styleUsage = $styleUsageResult.json
$styleUsageEntries = @(Get-JsonArrayValue -Object $styleUsage -Names @("entries", "styles"))

$styleNumberingIssues = @(Get-JsonArrayValue -Object $styleNumbering -Names @("issues"))
$issueSummary = @(New-IssueSummary -Issues $styleNumberingIssues)
$issueCount = Get-JsonIntValue -Object $styleNumbering -Names @("issue_count") -DefaultValue $styleNumberingIssues.Count
$suggestionCount = Get-JsonIntValue -Object $styleNumbering -Names @("suggestion_count")
$numberingDefinitionCount = Get-JsonIntValue -Object $exportResult.json -Names @("definition_count")
$numberingInstanceCount = Get-JsonIntValue -Object $exportResult.json -Names @("instance_count")
if ($numberingDefinitionCount -eq 0) {
    $numberingDefinitionCount = Get-JsonIntValue -Object $catalogJson -Names @("definition_count")
}
if ($numberingInstanceCount -eq 0) {
    $numberingInstanceCount = Get-JsonIntValue -Object $catalogJson -Names @("instance_count")
}

$styleUsageTotal = Get-JsonIntValue -Object $styleUsage -Names @("total_reference_count")
foreach ($entry in $styleUsageEntries) {
    $usage = Get-JsonPropertyValue -Object $entry -Names @("usage")
    $styleUsageTotal += Get-JsonIntValue -Object $usage -Names @("total_count")
}

$commands = @($exportResult, $styleNumberingResult, $styleUsageResult) |
    ForEach-Object {
        [ordered]@{
            command = $_.command
            arguments = @($_.arguments)
            exit_code = [int]$_.exit_code
            ok = [bool]$_.ok
            error = [string]$_.error
        }
    }
$commandFailureCount = @($commands | Where-Object { -not $_.ok }).Count

$releaseBlockers = @()
foreach ($command in @($commands | Where-Object { -not $_.ok })) {
    $releaseBlockers += [ordered]@{
        id = "document_skeleton.command_failed.$($command.command)"
        severity = "error"
        message = "Command '$($command.command)' did not complete cleanly."
        action = "rerun_document_skeleton_governance_report"
        exit_code = [int]$command.exit_code
    }
}
if ($issueCount -gt 0) {
    $releaseBlockers += [ordered]@{
        id = "document_skeleton.style_numbering_issues"
        severity = "error"
        message = "Style numbering audit reported issues."
        action = "review_style_numbering_audit"
        issue_count = $issueCount
    }
}

$inputForCommand = Convert-ToReportPath -RepoRoot $repoRoot -Path $resolvedInputDocx
$catalogForCommand = Convert-ToReportPath -RepoRoot $repoRoot -Path $catalogPath
$actionItems = @()
if ($issueCount -gt 0) {
    $actionItems += New-ActionItem `
        -Id "review_style_numbering_audit" `
        -Title "Review style numbering audit suggestions" `
        -Command ("featherdoc_cli audit-style-numbering " + (ConvertTo-CommandLine -Arguments @($inputForCommand)) + " --fail-on-issue --json")
}
if ($suggestionCount -gt 0) {
    $actionItems += New-ActionItem `
        -Id "preview_style_numbering_repair" `
        -Title "Preview safe style-numbering repairs against the exported catalog" `
        -Command ("featherdoc_cli repair-style-numbering " + (ConvertTo-CommandLine -Arguments @($inputForCommand)) + " --catalog-file " + (ConvertTo-CommandLine -Arguments @($catalogForCommand)) + " --plan-only --json")
}
$actionItems += New-ActionItem `
    -Id "promote_numbering_catalog_exemplar" `
    -Title "Review and promote the generated exemplar numbering catalog" `
    -Command ("featherdoc_cli check-numbering-catalog " + (ConvertTo-CommandLine -Arguments @($inputForCommand)) + " --catalog-file " + (ConvertTo-CommandLine -Arguments @($catalogForCommand)) + " --json")
$actionItems += New-ActionItem `
    -Id "register_numbering_catalog_baseline" `
    -Title "Register the exemplar catalog in the numbering catalog baseline flow" `
    -Command ("pwsh -ExecutionPolicy Bypass -File .\scripts\check_numbering_catalog_baseline.ps1 -InputDocx " + (ConvertTo-CommandLine -Arguments @($inputForCommand)) + " -CatalogFile " + (ConvertTo-CommandLine -Arguments @($catalogForCommand)) + " -GeneratedCatalogOutput output/document-skeleton-governance/generated.numbering-catalog.json -SkipBuild")

$status = if ($commandFailureCount -gt 0) {
    "failed"
} elseif ($issueCount -gt 0) {
    "needs_review"
} else {
    "clean"
}

$summary = [ordered]@{
    schema = "featherdoc.document_skeleton_governance_report.v1"
    generated_at = (Get-Date).ToString("s")
    status = $status
    clean = ($status -eq "clean")
    input_docx = $resolvedInputDocx
    input_docx_relative = $inputForCommand
    output_dir = $resolvedOutputDir
    summary_json = $summaryPath
    report_markdown = $markdownPath
    exemplar_catalog_path = $catalogPath
    exemplar_catalog_relative_path = $catalogForCommand
    numbering_catalog_path = $catalogPath
    numbering_catalog = [ordered]@{
        definition_count = $numberingDefinitionCount
        instance_count = $numberingInstanceCount
    }
    export_numbering_catalog = $exportResult.json
    style_numbering_audit = $styleNumbering
    style_usage_report = $styleUsage
    style_numbering_clean = Get-JsonBoolValue -Object $styleNumbering -Names @("clean") -DefaultValue $true
    style_numbering_issue_count = $issueCount
    style_numbering_suggestion_count = $suggestionCount
    numbered_style_count = Get-JsonIntValue -Object $styleNumbering -Names @("numbered_style_count")
    issue_summary = @($issueSummary)
    style_usage = [ordered]@{
        style_count = Get-JsonIntValue -Object $styleUsage -Names @("count", "style_count")
        entry_count = @($styleUsageEntries).Count
        usage_total = $styleUsageTotal
    }
    command_failure_count = $commandFailureCount
    commands = @($commands)
    release_blockers = @($releaseBlockers)
    release_blocker_count = @($releaseBlockers).Count
    action_items = @($actionItems)
    next_steps = @($actionItems)
}

($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Exemplar catalog: $catalogPath"
Write-Step "Status: $status"

if ($commandFailureCount -gt 0 -or ($FailOnIssue -and @($releaseBlockers).Count -gt 0)) {
    exit 1
}
