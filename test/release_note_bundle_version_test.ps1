param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

function Assert-NotContains {
    param(
        [string]$Path,
        [string]$UnexpectedText,
        [string]$Label
    )

    $content = Get-Content -Raw -LiteralPath $Path
    if (-not [string]::IsNullOrWhiteSpace($UnexpectedText) -and
        $content -match [regex]::Escape($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText': $Path"
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$reportDir = Join-Path $resolvedWorkingDir "report"
$installDir = Join-Path $resolvedWorkingDir "install"
$gateReportDir = Join-Path $resolvedWorkingDir "word-visual-release-gate\report"
$taskOutputRoot = Join-Path $resolvedWorkingDir "tasks"
$sectionPageSetupTaskDir = Join-Path $resolvedWorkingDir "tasks\section-page-setup"
$pageNumberFieldsTaskDir = Join-Path $resolvedWorkingDir "tasks\page-number-fields"
$curatedBundleId = "template-table-cli-selector"
$curatedBundleLabel = "Template table CLI selector"
$curatedBundleTaskDir = Join-Path $resolvedWorkingDir "tasks\$curatedBundleId"
$supersededReviewTasksReportPath = Join-Path $taskOutputRoot "superseded_review_tasks.json"
$expectedSupersededReviewTasksReportDisplayPath = ".\" + `
    ($supersededReviewTasksReportPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/') -replace '/', '\')

New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $installDir -Force | Out-Null
New-Item -ItemType Directory -Path $gateReportDir -Force | Out-Null
New-Item -ItemType Directory -Path $taskOutputRoot -Force | Out-Null
New-Item -ItemType Directory -Path $sectionPageSetupTaskDir -Force | Out-Null
New-Item -ItemType Directory -Path $pageNumberFieldsTaskDir -Force | Out-Null
New-Item -ItemType Directory -Path $curatedBundleTaskDir -Force | Out-Null

$summaryPath = Join-Path $reportDir "summary.json"
$gateSummaryPath = Join-Path $gateReportDir "gate_summary.json"
$gateFinalReviewPath = Join-Path $gateReportDir "gate_final_review.md"

$gateSummary = [ordered]@{
    generated_at = "2026-04-11T12:00:00"
    workspace = $resolvedRepoRoot
    report_dir = $gateReportDir
    visual_verdict = "pending_manual_review"
    review_tasks = [ordered]@{
        section_page_setup = [ordered]@{
            task_dir = $sectionPageSetupTaskDir
        }
        page_number_fields = [ordered]@{
            task_dir = $pageNumberFieldsTaskDir
        }
        curated_visual_regressions = @(
            [ordered]@{
                id = $curatedBundleId
                label = $curatedBundleLabel
                task = [ordered]@{
                    task_dir = $curatedBundleTaskDir
                }
            }
        )
    }
    manual_review = [ordered]@{
        tasks = [ordered]@{
            section_page_setup = [ordered]@{
                verdict = "pass"
            }
            page_number_fields = [ordered]@{
                verdict = "pending_manual_review"
            }
            curated_visual_regressions = @(
                [ordered]@{
                    id = $curatedBundleId
                    label = "curated:$curatedBundleId"
                    display_label = $curatedBundleLabel
                    verdict = "pass"
                    task_dir = $curatedBundleTaskDir
                }
            )
        }
    }
    curated_visual_regressions = @(
        [ordered]@{
            id = $curatedBundleId
            label = $curatedBundleLabel
            status = "completed"
            task = [ordered]@{
                task_dir = $curatedBundleTaskDir
            }
        }
    )
}
($gateSummary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $gateSummaryPath -Encoding UTF8
Set-Content -LiteralPath $gateFinalReviewPath -Encoding UTF8 -Value "# Gate Final Review"
(@{
    generated_at = "2026-04-11T12:00:00"
    task_output_root = $taskOutputRoot
    report_path = $supersededReviewTasksReportPath
    group_count = 2
    superseded_task_count = 0
    groups = @()
} | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $supersededReviewTasksReportPath -Encoding UTF8

$summary = [ordered]@{
    generated_at = "2026-04-11T12:00:00"
    workspace = $resolvedRepoRoot
    execution_status = "pass"
    release_version = "1.6.0"
    task_output_root = $taskOutputRoot
    superseded_review_tasks_report = $supersededReviewTasksReportPath
    install_dir = $installDir
    steps = [ordered]@{
        configure = [ordered]@{ status = "completed" }
        build = [ordered]@{ status = "completed" }
        tests = [ordered]@{ status = "completed" }
        install_smoke = [ordered]@{
            status = "completed"
            install_prefix = $installDir
            consumer_document = (Join-Path $resolvedWorkingDir "consumer\install-smoke.docx")
        }
        visual_gate = [ordered]@{
            status = "completed"
            summary_json = $gateSummaryPath
            final_review = $gateFinalReviewPath
            superseded_review_tasks_report = $supersededReviewTasksReportPath
            smoke_verdict = "pass"
            fixed_grid_verdict = "undetermined"
            curated_visual_regressions = @(
                [ordered]@{
                    id = $curatedBundleId
                    label = $curatedBundleLabel
                    verdict = "pass"
                    task_dir = $curatedBundleTaskDir
                }
            )
        }
    }
}
($summary | ConvertTo-Json -Depth 10) | Set-Content -LiteralPath $summaryPath -Encoding UTF8

$bundleScript = Join-Path $resolvedRepoRoot "scripts\write_release_note_bundle.ps1"
& $bundleScript -SummaryJson $summaryPath

$handoffPath = Join-Path $reportDir "release_handoff.md"
$bodyPath = Join-Path $reportDir "release_body.zh-CN.md"
$shortPath = Join-Path $reportDir "release_summary.zh-CN.md"

Assert-Contains -Path $handoffPath -ExpectedText 'Project version: 1.6.0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'publish_github_release.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Section page setup verdict: pass' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Page number fields verdict: pending_manual_review' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Template table CLI selector review task' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_latest_word_review_task.ps1 -SourceKind template-table-cli-selector-visual-regression-bundle -PrintPrompt' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'Superseded review tasks: 0' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText $expectedSupersededReviewTasksReportDisplayPath -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'open_latest_page_number_fields_review_task.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $handoffPath -ExpectedText 'find_superseded_review_tasks.ps1' -Label 'release_handoff.md'
Assert-Contains -Path $bodyPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText '# FeatherDoc v1.6.0' -Label 'release_summary.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\VISUAL_VALIDATION_QUICKSTART.zh-CN.md' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\RELEASE_ARTIFACT_TEMPLATE.zh-CN.md' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'share\FeatherDoc\visual-validation' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Smoke verdict：pass' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Fixed-grid verdict：undetermined' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Section page setup verdict：pass' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Page number fields verdict：pending_manual_review' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $bodyPath -ExpectedText 'Template table CLI selector verdict：pass' -Label 'release_body.zh-CN.md'
Assert-Contains -Path $shortPath -ExpectedText 'Word visual gate 细分结论：smoke=`pass`，fixed-grid=`undetermined`，section page setup=`pass`，page number fields=`pending_manual_review`，Template table CLI selector=`pass`。' -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $installDir -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText $resolvedWorkingDir -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText 'draft' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText '草稿' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText '发布说明草稿' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText '请在发布前补齐' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $bodyPath -UnexpectedText '这份文件由 `write_release_body_zh.ps1` 自动生成' -Label 'release_body.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText 'draft' -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText '草稿' -Label 'release_summary.zh-CN.md'
Assert-NotContains -Path $shortPath -UnexpectedText '这份文件由 `write_release_body_zh.ps1` 自动生成' -Label 'release_summary.zh-CN.md'

$guidePath = Join-Path $reportDir "ARTIFACT_GUIDE.md"
$checklistPath = Join-Path $reportDir "REVIEWER_CHECKLIST.md"
$startHerePath = Join-Path (Split-Path -Parent $reportDir) "START_HERE.md"
Assert-Contains -Path $guidePath -ExpectedText 'publish_github_release.ps1' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'publish_github_release.ps1' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'publish_github_release.ps1' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'release-refresh.yml' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-refresh.yml' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'release-refresh.yml' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'release-publish.yml' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-publish.yml' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'release-publish.yml' -Label 'START_HERE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Release Refresh' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Release Publish' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $guidePath -ExpectedText 'Run workflow' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Actions' -Label 'START_HERE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-refresh-output' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'release-publish-output' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $guidePath -ExpectedText 'Smoke verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Section page setup verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Page number fields review task' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $guidePath -ExpectedText 'Template table CLI selector review task' -Label 'ARTIFACT_GUIDE.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Superseded review tasks: 0' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Confirm `superseded_review_tasks.json` reports zero stale task directories' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Page number fields verdict: pending_manual_review' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the Template table CLI selector review task if the release touches this curated visual bundle' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $checklistPath -ExpectedText 'Open the page number fields review task if the release touches page numbers' -Label 'REVIEWER_CHECKLIST.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Visual verdict: pending_manual_review' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Smoke verdict: pass' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Fixed-grid verdict: undetermined' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Template table CLI selector verdict: pass' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Section page setup review task' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'Template table CLI selector review task' -Label 'START_HERE.md'
Assert-Contains -Path $startHerePath -ExpectedText 'open_latest_word_review_task.ps1 -SourceKind template-table-cli-selector-visual-regression-bundle -PrintPrompt' -Label 'START_HERE.md'

$bodyContent = Get-Content -Raw -LiteralPath $bodyPath
if ($bodyContent -match 'v1\.6\.1') {
    throw "release_body.zh-CN.md unexpectedly referenced the current development version: $bodyPath"
}

Write-Host "Release note bundle version pinning passed."
