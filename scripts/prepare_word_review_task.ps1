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

$repoRoot = Resolve-RepoRoot
$resolvedDocxPath = Resolve-DocumentPath -RepoRoot $repoRoot -InputPath $DocxPath
$documentName = [System.IO.Path]::GetFileName($resolvedDocxPath)
$documentStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedDocxPath)
$safeStem = Get-SafePathSegment -Name $documentStem
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$taskId = "$safeStem-$timestamp"

$taskRoot = Join-Path $repoRoot $TaskOutputRoot
$taskDir = Join-Path $taskRoot $taskId
$evidenceDir = Join-Path $taskDir "evidence"
$reportDir = Join-Path $taskDir "report"
$repairDir = Join-Path $taskDir "repair"
$manifestPath = Join-Path $taskDir "task_manifest.json"
$promptPath = Join-Path $taskDir "task_prompt.md"
$reviewResultPath = Join-Path $reportDir "review_result.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$templatePath = Get-TemplatePath -RepoRoot $repoRoot -Mode $Mode

New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
New-Item -ItemType Directory -Path $evidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $repairDir -Force | Out-Null

$templateValues = @{
    "{{DOCX_PATH}}" = $resolvedDocxPath
    "{{DOC_NAME}}" = $documentName
    "{{TASK_ID}}" = $taskId
    "{{TASK_DIR}}" = $taskDir
    "{{EVIDENCE_DIR}}" = $evidenceDir
    "{{REPORT_DIR}}" = $reportDir
    "{{REPAIR_DIR}}" = $repairDir
    "{{MODE}}" = $Mode
    "{{GENERATED_AT}}" = (Get-Date -Format "s")
}

$promptContent = Expand-Template -TemplatePath $templatePath -Values $templateValues
$promptContent | Set-Content -Path $promptPath -Encoding UTF8

$seedReviewResult = [ordered]@{
    task_id = $taskId
    mode = $Mode
    generated_at = (Get-Date).ToString("s")
    document_path = $resolvedDocxPath
    evidence_dir = $evidenceDir
    report_dir = $reportDir
    repair_dir = $repairDir
    status = "pending_review"
    verdict = "undecided"
    findings = @()
    notes = @(
        "Fill findings after screenshot-backed review.",
        "Use pass, fail, or undetermined as the final verdict."
    )
}
($seedReviewResult | ConvertTo-Json -Depth 5) | Set-Content -Path $reviewResultPath -Encoding UTF8

$seedFinalReview = @(
    "# Word Visual Review",
    "",
    "- Task id: $taskId",
    "- Mode: $Mode",
    "- Document: $resolvedDocxPath",
    "- Evidence directory: $evidenceDir",
    "- Report directory: $reportDir",
    "- Repair directory: $repairDir",
    "- Generated at: $(Get-Date -Format s)",
    "- Verdict: pending_manual_review",
    "",
    "## Summary",
    "",
    "- Verdict:",
    "- Notes:",
    "",
    "## Findings",
    "",
    "- Page:",
    "  Element type:",
    "  Description:",
    "  Severity:",
    "  Screenshot evidence:",
    "  Confidence:",
    "",
    "## Repair Decision",
    "",
    "- Enter repair loop:",
    "- Suggested fix:",
    "- Current best candidate:",
    "- Notes:"
) -join [Environment]::NewLine
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
        "Open task_prompt.md and send its contents to the AI agent.",
        "Tell the AI to write screenshot-backed findings into the report directory.",
        "If visual review fails and repair is allowed, iterate under the repair directory."
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
