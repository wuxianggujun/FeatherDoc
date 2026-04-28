param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-PathNotExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if (Test-Path -LiteralPath $Path) {
        throw "$Label should not have been generated: $Path"
    }
}

function Assert-Contains {
    param(
        [string]$Path,
        [string]$ExpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if ($content -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText': $Path"
    }
}

function Get-OptionalPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function New-TaskReviewSeed {
    param(
        [string]$Root,
        [string]$TaskId,
        [string]$Status,
        [string]$Verdict,
        [string]$SourceKind,
        [string]$SourcePath,
        [string]$GeneratedAt,
        [string]$ReviewedAt = "",
        [string]$ReviewMethod = ""
    )

    $reportDir = Join-Path $Root "report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

    (@{
        task_id = $TaskId
        status = $Status
        verdict = $Verdict
        reviewed_at = $ReviewedAt
        review_method = $ReviewMethod
        findings = @()
        notes = @("seed")
    } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $reportDir "review_result.json") -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $reportDir "final_review.md") -Encoding UTF8 -Value "# final $TaskId"
    Set-Content -LiteralPath (Join-Path $Root "task_prompt.md") -Encoding UTF8 -Value "# prompt"
    (@{
        task_id = $TaskId
        task_dir = $Root
        generated_at = $GeneratedAt
        source = @{
            kind = $SourceKind
            path = $SourcePath
        }
    } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $Root "task_manifest.json") -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$gateRoot = Join-Path $resolvedWorkingDir "word-visual-release-gate"
$gateReportDir = Join-Path $gateRoot "report"
$smokeDir = Join-Path $gateRoot "smoke"
$decoyReleaseRoot = Join-Path $resolvedWorkingDir "release-candidate-decoy"
$decoyReleaseReportDir = Join-Path $decoyReleaseRoot "report"
$syncLatestScript = Join-Path $resolvedRepoRoot "scripts\sync_latest_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $smokeDir -Force | Out-Null
New-Item -ItemType Directory -Path $decoyReleaseReportDir -Force | Out-Null

$docxPath = Join-Path $smokeDir "table_visual_smoke.docx"
Set-Content -LiteralPath $docxPath -Encoding UTF8 -Value "docx"

$oldDocumentTaskDir = Join-Path $tasksRoot "document-task-old"
$newDocumentTaskDir = Join-Path $tasksRoot "document-task-new"
New-TaskReviewSeed -Root $oldDocumentTaskDir -TaskId "document-task-old" -Status "pending_review" -Verdict "undecided" `
    -SourceKind "document" -SourcePath $docxPath -GeneratedAt "2026-04-15T10:00:00"
New-TaskReviewSeed -Root $newDocumentTaskDir -TaskId "document-task-new" -Status "reviewed" -Verdict "pass" `
    -SourceKind "document" -SourcePath $docxPath -GeneratedAt "2026-04-15T10:05:00" `
    -ReviewedAt "2026-04-15T10:05:00" -ReviewMethod "operator_supplied"

$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$decoyReleaseSummaryPath = Join-Path $decoyReleaseReportDir "summary.json"

(@{
    generated_at = "2026-04-15T10:00:00"
    gate_output_dir = $gateRoot
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    readme_gallery = @{ status = "not_requested" }
    smoke = @{
        status = "completed"
        output_dir = $smokeDir
        docx_path = $docxPath
        task = @{
            task_id = "document-task-old"
            source_kind = "document"
            source_path = $docxPath
            document_path = $docxPath
            mode = "review-only"
            task_dir = $oldDocumentTaskDir
            prompt_path = (Join-Path $oldDocumentTaskDir "task_prompt.md")
            manifest_path = (Join-Path $oldDocumentTaskDir "task_manifest.json")
            review_result_path = (Join-Path $oldDocumentTaskDir "report\review_result.json")
            final_review_path = (Join-Path $oldDocumentTaskDir "report\final_review.md")
            latest_task_pointer_path = (Join-Path $tasksRoot "latest_task.json")
            latest_source_kind_task_pointer_path = (Join-Path $tasksRoot "latest_document_task.json")
        }
    }
    review_tasks = @{
        document = @{
            task_id = "document-task-old"
            source_kind = "document"
            source_path = $docxPath
            document_path = $docxPath
            mode = "review-only"
            task_dir = $oldDocumentTaskDir
            prompt_path = (Join-Path $oldDocumentTaskDir "task_prompt.md")
            manifest_path = (Join-Path $oldDocumentTaskDir "task_manifest.json")
            review_result_path = (Join-Path $oldDocumentTaskDir "report\review_result.json")
            final_review_path = (Join-Path $oldDocumentTaskDir "report\final_review.md")
            latest_task_pointer_path = (Join-Path $tasksRoot "latest_task.json")
            latest_source_kind_task_pointer_path = (Join-Path $tasksRoot "latest_document_task.json")
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-15T10:00:00"
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    gate_output_dir = (Join-Path $resolvedWorkingDir "unmatched-word-visual-release-gate")
    task_output_root = $tasksRoot
    release_handoff = (Join-Path $decoyReleaseReportDir "release_handoff.md")
    release_body_zh_cn = (Join-Path $decoyReleaseReportDir "release_body.zh-CN.md")
    release_summary_zh_cn = (Join-Path $decoyReleaseReportDir "release_summary.zh-CN.md")
    artifact_guide = (Join-Path $decoyReleaseReportDir "ARTIFACT_GUIDE.md")
    reviewer_checklist = (Join-Path $decoyReleaseReportDir "REVIEWER_CHECKLIST.md")
    start_here = (Join-Path $decoyReleaseRoot "START_HERE.md")
    readme_gallery = @{ status = "not_requested" }
    steps = @{
        visual_gate = @{
            status = "completed"
            summary_json = (Join-Path $resolvedWorkingDir "unmatched-word-visual-release-gate\report\gate_summary.json")
            final_review = (Join-Path $resolvedWorkingDir "unmatched-word-visual-release-gate\report\gate_final_review.md")
            document_task_dir = $oldDocumentTaskDir
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $decoyReleaseSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-15T10:05:00"
    workspace = $resolvedRepoRoot
    task_root = $tasksRoot
    task_root_relative = "tasks"
    task_id = "document-task-new"
    mode = "review-only"
    source = @{
        kind = "document"
        label = "Target document"
        path = $docxPath
    }
    document = @{
        name = "table_visual_smoke.docx"
        path = $docxPath
    }
    task_dir = $newDocumentTaskDir
    task_dir_relative = "tasks\document-task-new"
    manifest_path = (Join-Path $newDocumentTaskDir "task_manifest.json")
    manifest_path_relative = "tasks\document-task-new\task_manifest.json"
    prompt_path = (Join-Path $newDocumentTaskDir "task_prompt.md")
    prompt_path_relative = "tasks\document-task-new\task_prompt.md"
    evidence_dir = (Join-Path $newDocumentTaskDir "evidence")
    report_dir = (Join-Path $newDocumentTaskDir "report")
    repair_dir = (Join-Path $newDocumentTaskDir "repair")
    review_result_path = (Join-Path $newDocumentTaskDir "report\review_result.json")
    review_result_path_relative = "tasks\document-task-new\report\review_result.json"
    final_review_path = (Join-Path $newDocumentTaskDir "report\final_review.md")
    final_review_path_relative = "tasks\document-task-new\report\final_review.md"
    pointer_files = @{
        generic = (Join-Path $tasksRoot "latest_task.json")
        source_kind = (Join-Path $tasksRoot "latest_document_task.json")
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath (Join-Path $tasksRoot "latest_document_task.json") -Encoding UTF8

& $syncLatestScript `
    -TaskOutputRoot $tasksRoot `
    -OutputSearchRoot $resolvedWorkingDir

$gateSummary = Get-Content -Raw -LiteralPath $gateSummaryPath | ConvertFrom-Json
$decoyReleaseSummary = Get-Content -Raw -LiteralPath $decoyReleaseSummaryPath | ConvertFrom-Json
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$supersededReviewTasksReportPath = Join-Path $tasksRoot "superseded_review_tasks.json"

Assert-True -Condition (Test-Path -LiteralPath $supersededReviewTasksReportPath) `
    -Message "Superseded review-task audit report was not generated."
Assert-True -Condition ($gateSummary.review_tasks.document.task_dir -eq $newDocumentTaskDir) `
    -Message "Gate summary document task_dir was not refreshed from latest_document_task.json."
Assert-True -Condition ($gateSummary.smoke.task.task_dir -eq $newDocumentTaskDir) `
    -Message "Gate summary smoke.task.task_dir was not refreshed from latest_document_task.json."
Assert-True -Condition ($gateSummary.manual_review.tasks.document.task_dir -eq $newDocumentTaskDir) `
    -Message "Manual review document task_dir did not follow the refreshed task pointer."
Assert-True -Condition ($gateSummary.visual_verdict -eq "pass") `
    -Message "Gate summary visual verdict was not recomputed from the refreshed document task."
Assert-True -Condition ($gateSummary.superseded_review_tasks_report -eq $supersededReviewTasksReportPath) `
    -Message "Gate summary superseded_review_tasks_report was not updated."
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Document verdict: pass" -Label "gate_final_review.md"
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Document reviewed at: 2026-04-15T10:05:00" -Label "gate_final_review.md"

Assert-True -Condition ($decoyReleaseSummary.steps.visual_gate.document_task_dir -eq $oldDocumentTaskDir) `
    -Message "Unmatched release summary should not receive refreshed task dir."
Assert-True -Condition ($decoyReleaseSummary.visual_verdict -eq "pending_manual_review") `
    -Message "Unmatched release summary should not receive promoted visual verdict."
Assert-True -Condition ((Get-OptionalPropertyValue -Object $decoyReleaseSummary -Name "superseded_review_tasks_report") -eq "") `
    -Message "Unmatched release summary should not receive audit metadata."
Assert-PathNotExists -Path (Join-Path $decoyReleaseReportDir "final_review.md") -Label "decoy release final review"
Assert-PathNotExists -Path (Join-Path $decoyReleaseReportDir "release_handoff.md") -Label "decoy release handoff"
Assert-PathNotExists -Path (Join-Path $decoyReleaseReportDir "release_body.zh-CN.md") -Label "decoy release body"
Assert-PathNotExists -Path (Join-Path $decoyReleaseReportDir "ARTIFACT_GUIDE.md") -Label "decoy artifact guide"
Assert-PathNotExists -Path (Join-Path $decoyReleaseReportDir "REVIEWER_CHECKLIST.md") -Label "decoy reviewer checklist"
Assert-PathNotExists -Path (Join-Path $decoyReleaseRoot "START_HERE.md") -Label "decoy start-here"

Write-Host "Sync latest visual review verdict without release summary regression passed."
