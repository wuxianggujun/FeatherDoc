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

function New-TaskReviewSeed {
    param(
        [string]$Root,
        [string]$TaskId,
        [string]$ReviewedAt
    )

    $reportDir = Join-Path $Root "report"
    New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

    (@{
        task_id = $TaskId
        status = "reviewed"
        verdict = "pass"
        reviewed_at = $ReviewedAt
        review_method = "operator_supplied"
        findings = @()
        notes = @("verified")
    } | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $reportDir "review_result.json") -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $reportDir "final_review.md") -Encoding UTF8 -Value "# final"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$gateRoot = Join-Path $resolvedWorkingDir "word-visual-release-gate"
$gateReportDir = Join-Path $gateRoot "report"
$releaseRoot = Join-Path $resolvedWorkingDir "release-candidate-checks"
$releaseReportDir = Join-Path $releaseRoot "report"
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$syncScript = Join-Path $resolvedRepoRoot "scripts\sync_visual_review_verdict.ps1"

New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $releaseReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $tasksRoot -Force | Out-Null

$documentTaskDir = Join-Path $tasksRoot "document-task"
$fixedGridTaskDir = Join-Path $tasksRoot "fixed-grid-task"
$sectionPageSetupTaskDir = Join-Path $tasksRoot "section-page-setup-task"
New-TaskReviewSeed -Root $documentTaskDir -TaskId "document-task" -ReviewedAt "2026-04-13T12:31:00"
New-TaskReviewSeed -Root $fixedGridTaskDir -TaskId "fixed-grid-task" -ReviewedAt "2026-04-13T12:32:00"
New-TaskReviewSeed -Root $sectionPageSetupTaskDir -TaskId "section-page-setup-task" -ReviewedAt "2026-04-13T12:33:00"

$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$releaseSummaryPath = Join-Path $releaseReportDir "summary.json"

(@{
    generated_at = "2026-04-13T12:00:00"
    gate_output_dir = $gateRoot
    report_dir = $gateReportDir
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    readme_gallery = @{ status = "not_requested" }
    smoke = @{
        status = "completed"
        docx_path = (Join-Path $gateRoot "smoke\sample.docx")
    }
    fixed_grid = @{
        status = "completed"
        summary_json = (Join-Path $gateRoot "fixed-grid\summary.json")
    }
    section_page_setup = @{
        status = "completed"
        summary_json = (Join-Path $gateRoot "section-page-setup\summary.json")
    }
    review_tasks = @{
        document = @{
            task_id = "document-task"
            task_dir = $documentTaskDir
            review_result_path = (Join-Path $documentTaskDir "report\review_result.json")
            final_review_path = (Join-Path $documentTaskDir "report\final_review.md")
        }
        fixed_grid = @{
            task_id = "fixed-grid-task"
            task_dir = $fixedGridTaskDir
            review_result_path = (Join-Path $fixedGridTaskDir "report\review_result.json")
            final_review_path = (Join-Path $fixedGridTaskDir "report\final_review.md")
        }
        section_page_setup = @{
            task_id = "section-page-setup-task"
            task_dir = $sectionPageSetupTaskDir
            review_result_path = (Join-Path $sectionPageSetupTaskDir "report\review_result.json")
            final_review_path = (Join-Path $sectionPageSetupTaskDir "report\final_review.md")
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8

(@{
    generated_at = "2026-04-13T12:00:00"
    execution_status = "pass"
    visual_verdict = "pending_manual_review"
    build_dir = (Join-Path $resolvedWorkingDir "build")
    install_dir = (Join-Path $resolvedWorkingDir "install")
    consumer_build_dir = (Join-Path $resolvedWorkingDir "consumer")
    gate_output_dir = $gateRoot
    task_output_root = $tasksRoot
    release_handoff = (Join-Path $releaseReportDir "release_handoff.md")
    release_body_zh_cn = (Join-Path $releaseReportDir "release_body.zh-CN.md")
    release_summary_zh_cn = (Join-Path $releaseReportDir "release_summary.zh-CN.md")
    artifact_guide = (Join-Path $releaseReportDir "ARTIFACT_GUIDE.md")
    reviewer_checklist = (Join-Path $releaseReportDir "REVIEWER_CHECKLIST.md")
    start_here = (Join-Path $releaseRoot "START_HERE.md")
    readme_gallery = @{ status = "not_requested" }
    steps = @{
        configure = @{ status = "completed" }
        build = @{ status = "completed" }
        tests = @{ status = "completed" }
        install_smoke = @{ status = "completed" }
        visual_gate = @{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = (Join-Path $gateReportDir "gate_final_review.md")
        }
    }
} | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $releaseSummaryPath -Encoding UTF8

& $syncScript `
    -GateSummaryJson $gateSummaryPath `
    -ReleaseCandidateSummaryJson $releaseSummaryPath

$gateSummary = Get-Content -Raw -LiteralPath $gateSummaryPath | ConvertFrom-Json
$releaseSummary = Get-Content -Raw -LiteralPath $releaseSummaryPath | ConvertFrom-Json
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"
$releaseFinalReviewPath = Join-Path $releaseReportDir "final_review.md"

Assert-True -Condition ($gateSummary.visual_verdict -eq "pass") `
    -Message "Gate summary visual verdict was not promoted to pass."
Assert-True -Condition ($gateSummary.manual_review.tasks.section_page_setup.verdict -eq "pass") `
    -Message "Gate summary manual review is missing the section page setup verdict."
Assert-True -Condition ($releaseSummary.steps.visual_gate.section_page_setup_verdict -eq "pass") `
    -Message "Release summary did not record section_page_setup_verdict=pass."
Assert-True -Condition ($gateSummary.section_page_setup.review_method -eq "operator_supplied") `
    -Message "Gate summary did not record section page setup review_method."
Assert-True -Condition ($releaseSummary.steps.visual_gate.section_page_setup_review_method -eq "operator_supplied") `
    -Message "Release summary did not record section_page_setup_review_method."
Assert-Contains -Path $gateSummaryPath -ExpectedText "2026-04-13T12:33:00" -Label "gate_summary.json"
Assert-Contains -Path $releaseSummaryPath -ExpectedText "2026-04-13T12:33:00" -Label "summary.json"

Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Section page setup flow" -Label "gate_final_review.md"
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Section page setup verdict: pass" -Label "gate_final_review.md"
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Section page setup reviewed at: 2026-04-13T12:33:00" -Label "gate_final_review.md"
Assert-Contains -Path $gateFinalReviewPath -ExpectedText "Section page setup review method: operator_supplied" -Label "gate_final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Visual verdict: pass" -Label "release final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "## Visual review provenance" -Label "release final_review.md"
Assert-Contains -Path $releaseFinalReviewPath -ExpectedText "Section page setup: verdict=pass, review_status=reviewed, reviewed_at=2026-04-13T12:33:00, review_method=operator_supplied" -Label "release final_review.md"

Write-Host "Sync visual review verdict section page setup regression passed."
