param(
    [Parameter(Mandatory = $true)]
    [string]$DocxPath,
    [ValidateSet("review-only", "review-and-repair")]
    [string]$Mode = "review-only",
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [switch]$OpenTaskDir
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-DocumentPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    if (-not (Test-Path $candidate)) {
        throw "Target document does not exist: $candidate"
    }

    $resolved = (Resolve-Path $candidate).Path
    $extension = [System.IO.Path]::GetExtension($resolved).ToLowerInvariant()
    if ($extension -notin @(".doc", ".docx")) {
        throw "Only .doc and .docx files are supported: $resolved"
    }

    return $resolved
}

function Get-SafePathSegment {
    param([string]$Name)

    $safe = [System.Text.RegularExpressions.Regex]::Replace($Name, "[^A-Za-z0-9._-]+", "-")
    $safe = $safe.Trim("-")
    if ([string]::IsNullOrWhiteSpace($safe)) {
        return "document"
    }

    return $safe
}

function Get-TemplatePath {
    param(
        [string]$RepoRoot,
        [string]$Mode
    )

    $fileName = switch ($Mode) {
        "review-only" { "task_prompt_review_only_template.md" }
        "review-and-repair" { "task_prompt_review_and_repair_template.md" }
        default { throw "Unsupported mode: $Mode" }
    }

    return Join-Path $RepoRoot "docs\automation\$fileName"
}

function Get-ReportTemplatePath {
    param(
        [string]$RepoRoot,
        [string]$FileName
    )

    return Join-Path $RepoRoot "docs\automation\$FileName"
}

function Expand-Template {
    param(
        [string]$TemplatePath,
        [hashtable]$Values
    )

    $content = Get-Content -Path $TemplatePath -Raw -Encoding UTF8
    foreach ($key in $Values.Keys) {
        $content = $content.Replace($key, $Values[$key])
    }

    return $content
}

function ConvertTo-JsonTemplateValues {
    param([hashtable]$Values)

    $escapedValues = @{}
    foreach ($key in $Values.Keys) {
        $json = ConvertTo-Json ([string]$Values[$key]) -Compress
        $escapedValues[$key] = $json.Substring(1, $json.Length - 2)
    }

    return $escapedValues
}

function New-UniqueTaskLocation {
    param(
        [string]$TaskRoot,
        [string]$SafeStem
    )

    $baseTimestamp = Get-Date -Format "yyyyMMdd-HHmmss-fff"
    $attempt = 0

    while ($true) {
        $suffix = if ($attempt -eq 0) { "" } else { "-{0:D2}" -f $attempt }
        $taskId = "$SafeStem-$baseTimestamp$suffix"
        $taskDir = Join-Path $TaskRoot $taskId
        if (-not (Test-Path $taskDir)) {
            return @{
                TaskId = $taskId
                TaskDir = $taskDir
            }
        }
        $attempt += 1
    }
}

$repoRoot = Resolve-RepoRoot
$resolvedDocxPath = Resolve-DocumentPath -RepoRoot $repoRoot -InputPath $DocxPath
$documentName = [System.IO.Path]::GetFileName($resolvedDocxPath)
$documentStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedDocxPath)
$safeStem = Get-SafePathSegment -Name $documentStem

$taskRoot = Join-Path $repoRoot $TaskOutputRoot
$taskLocation = New-UniqueTaskLocation -TaskRoot $taskRoot -SafeStem $safeStem
$taskId = $taskLocation.TaskId
$taskDir = $taskLocation.TaskDir
$evidenceDir = Join-Path $taskDir "evidence"
$reportDir = Join-Path $taskDir "report"
$repairDir = Join-Path $taskDir "repair"
$manifestPath = Join-Path $taskDir "task_manifest.json"
$promptPath = Join-Path $taskDir "task_prompt.md"
$reviewResultPath = Join-Path $reportDir "review_result.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$templatePath = Get-TemplatePath -RepoRoot $repoRoot -Mode $Mode
$reviewResultTemplatePath = Get-ReportTemplatePath -RepoRoot $repoRoot -FileName "review_result_template.json"
$finalReviewTemplatePath = Get-ReportTemplatePath -RepoRoot $repoRoot -FileName "final_review_template.md"

New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
New-Item -ItemType Directory -Path $evidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $repairDir -Force | Out-Null

$templateValues = @{
    "{{DOCX_PATH}}" = $resolvedDocxPath
    "{{DOC_NAME}}" = $documentName
    "{{TASK_ID}}" = $taskId
    "{{TASK_DIR}}" = $taskDir
    "{{WORKSPACE}}" = $repoRoot
    "{{EVIDENCE_DIR}}" = $evidenceDir
    "{{REPORT_DIR}}" = $reportDir
    "{{REPAIR_DIR}}" = $repairDir
    "{{MODE}}" = $Mode
    "{{GENERATED_AT}}" = (Get-Date -Format "s")
}

$promptContent = Expand-Template -TemplatePath $templatePath -Values $templateValues
$promptContent | Set-Content -Path $promptPath -Encoding UTF8

$jsonTemplateValues = ConvertTo-JsonTemplateValues -Values $templateValues
$seedReviewResult = Expand-Template -TemplatePath $reviewResultTemplatePath -Values $jsonTemplateValues
$seedReviewResult | Set-Content -Path $reviewResultPath -Encoding UTF8

$seedFinalReview = Expand-Template -TemplatePath $finalReviewTemplatePath -Values $templateValues
$seedFinalReview | Set-Content -Path $finalReviewPath -Encoding UTF8

$manifest = [ordered]@{
    task_id = $taskId
    mode = $Mode
    generated_at = (Get-Date).ToString("s")
    document = [ordered]@{
        name = $documentName
        path = $resolvedDocxPath
    }
    workspace = $repoRoot
    task_dir = $taskDir
    prompt_path = $promptPath
    evidence_dir = $evidenceDir
    report_dir = $reportDir
    repair_dir = $repairDir
    review_result_path = $reviewResultPath
    final_review_path = $finalReviewPath
    recommended_next_steps = @(
        "Open task_prompt.md and send the full contents to the AI agent.",
        "Tell the AI to first run scripts\\run_word_visual_smoke.ps1 with -InputDocx and -OutputDir pointing at this task directory.",
        "Tell the AI to review the generated PDF/PNG evidence and write findings into the report directory.",
        "If the mode is review-and-repair, allow iterative fixes and full regressions under the repair directory."
    )
}
($manifest | ConvertTo-Json -Depth 5) | Set-Content -Path $manifestPath -Encoding UTF8

Write-Host "Task id: $taskId"
Write-Host "Document: $resolvedDocxPath"
Write-Host "Mode: $Mode"
Write-Host "Task directory: $taskDir"
Write-Host "Prompt: $promptPath"
Write-Host "Manifest: $manifestPath"
Write-Host "Evidence: $evidenceDir"
Write-Host "Report: $reportDir"
Write-Host "Review result: $reviewResultPath"
Write-Host "Final review: $finalReviewPath"
Write-Host "Repair: $repairDir"

if ($OpenTaskDir) {
    Start-Process explorer.exe $taskDir | Out-Null
}
