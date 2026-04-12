<#
.SYNOPSIS
Audits release-facing FeatherDoc materials for draft wording and local path leaks.

.DESCRIPTION
Scans one or more files or directories and fails if release-facing text files
contain draft/草稿 wording or machine-local absolute paths. Directories are
scanned recursively but only text-like files are considered.

.PARAMETER Path
One or more files or directories to scan.

.PARAMETER AdditionalForbiddenPattern
Optional extra regex patterns to audit in addition to the built-in rules.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\assert_release_material_safety.ps1 `
    -Path .\output\release-candidate-checks\START_HERE.md, .\output\release-candidate-checks\report
#>
param(
    [Parameter(Mandatory = $true)]
    [string[]]$Path,
    [string[]]$AdditionalForbiddenPattern = @()
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[assert-release-material-safety] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Test-TextLikeFile {
    param([System.IO.FileInfo]$File)

    $textExtensions = @(
        ".md",
        ".txt",
        ".json",
        ".xml",
        ".yml",
        ".yaml",
        ".csv",
        ".log",
        ".rst"
    )

    return $textExtensions -contains $File.Extension.ToLowerInvariant()
}

function Get-ScanFiles {
    param(
        [string]$RepoRoot,
        [string[]]$InputPaths
    )

    $files = [System.Collections.Generic.List[string]]::new()
    foreach ($inputPath in $InputPaths) {
        $resolvedPath = Resolve-FullPath -RepoRoot $RepoRoot -InputPath $inputPath
        if (-not (Test-Path -LiteralPath $resolvedPath)) {
            throw "Audit path does not exist: $resolvedPath"
        }

        $item = Get-Item -LiteralPath $resolvedPath
        if ($item.PSIsContainer) {
            foreach ($file in Get-ChildItem -LiteralPath $resolvedPath -Recurse -File) {
                if (Test-TextLikeFile -File $file) {
                    [void]$files.Add($file.FullName)
                }
            }
        } else {
            [void]$files.Add($item.FullName)
        }
    }

    return @($files | Sort-Object -Unique)
}

function New-Rule {
    param(
        [string]$Label,
        [string]$Pattern
    )

    return [ordered]@{
        label = $Label
        pattern = $Pattern
    }
}

$repoRoot = Resolve-RepoRoot
$scanFiles = Get-ScanFiles -RepoRoot $repoRoot -InputPaths $Path
if ($scanFiles.Count -eq 0) {
    Write-Step "No text-like files matched the requested paths."
    exit 0
}

$rules = @(
    (New-Rule `
        -Label "draft wording" `
        -Pattern '(?i)发布说明草稿|请在发布前补齐|release body draft|release-note drafts|release drafts|public release drafts|草稿|\bdraft\w*\b')
    (New-Rule `
        -Label "local absolute path" `
        -Pattern '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+|(?<!\w)/(?:Users|home)/[^\s"''`<>|]+')
)

foreach ($extraPattern in $AdditionalForbiddenPattern) {
    if (-not [string]::IsNullOrWhiteSpace($extraPattern)) {
        $rules += New-Rule -Label "custom forbidden pattern" -Pattern $extraPattern
    }
}

$violations = [System.Collections.Generic.List[object]]::new()
foreach ($file in $scanFiles) {
    foreach ($rule in $rules) {
        $matches = Select-String -LiteralPath $file -Pattern $rule.pattern -AllMatches
        foreach ($match in $matches) {
            [void]$violations.Add([ordered]@{
                file = $file
                line = $match.LineNumber
                label = $rule.label
                text = $match.Line.Trim()
            })
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Step "Detected forbidden release material content:"
    foreach ($violation in $violations) {
        Write-Host ("- [{0}] {1}:{2}" -f $violation.label, $violation.file, $violation.line)
        Write-Host ("  {0}" -f $violation.text)
    }

    throw "Release material safety audit failed with $($violations.Count) violation(s)."
}

Write-Step ("Audit passed for {0} file(s)." -f $scanFiles.Count)
