param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)

    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-SyncScript {
    param([string[]]$Arguments)

    $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
    $powerShellPath = if ($powerShellCommand) { $powerShellCommand.Source } else { (Get-Process -Id $PID).Path }
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
        $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = (@($output | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
    }
}

function Get-SummaryEntry {
    param($Summary, [string]$Name)

    return @($Summary.entries | Where-Object { [string]$_.name -eq $Name }) | Select-Object -First 1
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\sync_project_template_smoke_visual_verdict.ps1"
$summaryPath = Join-Path $resolvedWorkingDir "summary.json"
$summaryMarkdownPath = Join-Path $resolvedWorkingDir "summary.md"
$releaseSummaryPath = Join-Path $resolvedWorkingDir "release-candidate-checks\report\summary.json"
$releaseFinalReviewPath = Join-Path $resolvedWorkingDir "release-candidate-checks\report\final_review.md"
$manifestPath = Join-Path $resolvedWorkingDir "project_template_smoke.manifest.json"
$outputDir = Join-Path $resolvedWorkingDir "project-template-smoke-output"
$contactSheetPath = Join-Path $resolvedWorkingDir "visual\contact-sheet.png"
$passReviewPath = Join-Path $resolvedWorkingDir "visual\pass-review.json"
$undeterminedReviewPath = Join-Path $resolvedWorkingDir "visual\undetermined-review.json"
$missingReviewPath = Join-Path $resolvedWorkingDir "visual\missing-review.json"

Write-JsonFile -Path $passReviewPath -Value ([ordered]@{
    schema = "featherdoc.word_visual_review_result.v1"
    status = "completed"
    verdict = "pass"
    findings = @(
        [ordered]@{
            id = "visual.pass"
            severity = "info"
        }
    )
})

Write-JsonFile -Path $undeterminedReviewPath -Value ([ordered]@{
    schema = "featherdoc.word_visual_review_result.v1"
    status = "completed"
    verdict = "undetermined"
    findings = @(
        [ordered]@{
            id = "visual.needs-human"
            severity = "warning"
        }
    )
})

Write-JsonFile -Path $summaryPath -Value ([ordered]@{
    schema = "featherdoc.project_template_smoke_summary.v1"
    generated_at = "2026-01-01T00:00:00"
    manifest_path = $manifestPath
    output_dir = $outputDir
    passed = $true
    overall_status = "passed_with_pending_visual_review"
    entries = @(
        [ordered]@{
            name = "visual-pass"
            input_docx = Join-Path $resolvedWorkingDir "visual-pass.docx"
            artifact_dir = Join-Path $resolvedWorkingDir "artifacts\visual-pass"
            status = "passed_with_pending_visual_review"
            passed = $true
            manual_review_pending = $true
            checks = [ordered]@{
                template_validations = @()
                schema_validation = [ordered]@{
                    enabled = $false
                }
                schema_baseline = [ordered]@{
                    enabled = $true
                    matches = $true
                    schema_lint_clean = $true
                }
                visual_smoke = [ordered]@{
                    enabled = $true
                    review_status = "pending_review"
                    review_verdict = "pending_manual_review"
                    review_result_json = $passReviewPath
                    contact_sheet = $contactSheetPath
                }
            }
            issues = @()
        },
        [ordered]@{
            name = "visual-undetermined"
            input_docx = Join-Path $resolvedWorkingDir "visual-undetermined.docx"
            artifact_dir = Join-Path $resolvedWorkingDir "artifacts\visual-undetermined"
            status = "passed"
            passed = $true
            manual_review_pending = $false
            checks = [ordered]@{
                template_validations = @()
                schema_validation = [ordered]@{
                    enabled = $false
                }
                schema_baseline = [ordered]@{
                    enabled = $true
                    matches = $true
                    schema_lint_clean = $true
                }
                visual_smoke = [ordered]@{
                    enabled = $true
                    review_status = "pending_review"
                    review_verdict = "pending_manual_review"
                    review_result_json = $undeterminedReviewPath
                    contact_sheet = $contactSheetPath
                }
            }
            issues = @()
        },
        [ordered]@{
            name = "visual-missing"
            input_docx = Join-Path $resolvedWorkingDir "visual-missing.docx"
            artifact_dir = Join-Path $resolvedWorkingDir "artifacts\visual-missing"
            status = "passed"
            passed = $true
            manual_review_pending = $false
            checks = [ordered]@{
                template_validations = @()
                schema_validation = [ordered]@{
                    enabled = $false
                }
                schema_baseline = [ordered]@{
                    enabled = $true
                    matches = $true
                    schema_lint_clean = $true
                }
                visual_smoke = [ordered]@{
                    enabled = $true
                    review_status = ""
                    review_verdict = "undecided"
                    review_result_json = $missingReviewPath
                    contact_sheet = $contactSheetPath
                }
            }
            issues = @()
        }
    )
})

Write-JsonFile -Path $releaseSummaryPath -Value ([ordered]@{
    execution_status = "completed"
    visual_verdict = "pending_manual_review"
    failed_step = ""
    error = ""
    msvc_bootstrap_mode = "not_used"
    build_dir = Join-Path $resolvedWorkingDir "build"
    install_dir = Join-Path $resolvedWorkingDir "install"
    consumer_build_dir = Join-Path $resolvedWorkingDir "consumer-build"
    gate_output_dir = Join-Path $resolvedWorkingDir "visual-gate"
    task_output_root = Join-Path $resolvedWorkingDir "tasks"
    release_handoff = Join-Path $resolvedWorkingDir "release-handoff.md"
    release_body_zh_cn = Join-Path $resolvedWorkingDir "release-body.zh-CN.md"
    release_summary_zh_cn = Join-Path $resolvedWorkingDir "release-summary.zh-CN.md"
    artifact_guide = Join-Path $resolvedWorkingDir "artifact-guide.md"
    reviewer_checklist = Join-Path $resolvedWorkingDir "reviewer-checklist.md"
    start_here = Join-Path $resolvedWorkingDir "START_HERE.md"
    readme_gallery = [ordered]@{
        status = "not_requested"
    }
    steps = [ordered]@{
        configure = [ordered]@{ status = "completed" }
        build = [ordered]@{ status = "completed" }
        tests = [ordered]@{ status = "completed" }
        template_schema = [ordered]@{ status = "completed" }
        template_schema_manifest = [ordered]@{ status = "completed" }
        project_template_smoke = [ordered]@{ status = "completed" }
        install_smoke = [ordered]@{ status = "completed" }
        visual_gate = [ordered]@{ status = "completed" }
    }
})

$result = Invoke-SyncScript -Arguments @(
    "-SummaryJson", $summaryPath,
    "-SummaryMarkdown", $summaryMarkdownPath,
    "-ReleaseCandidateSummaryJson", $releaseSummaryPath
)

Assert-Equal -Actual $result.ExitCode -Expected 0 `
    -Message "Visual verdict sync should pass when entries have no failures. Output: $($result.Text)"
Assert-ContainsText -Text $result.Text -ExpectedText "Overall status: passed_with_pending_visual_review" `
    -Message "Sync output should report the derived overall status."
Assert-ContainsText -Text $result.Text -ExpectedText "Visual verdict: pending_manual_review" `
    -Message "Sync output should report the aggregate visual verdict."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace([string]$summary.visual_review_synced_at)) `
    -Message "Sync should stamp visual_review_synced_at."
Assert-Equal -Actual ([int]$summary.entry_count) -Expected 3 `
    -Message "Summary should preserve all entries."
Assert-Equal -Actual ([int]$summary.visual_entry_count) -Expected 3 `
    -Message "Summary should count visual entries."
Assert-Equal -Actual ([int]$summary.failed_entry_count) -Expected 0 `
    -Message "Summary should not mark passing visual entries as failed."
Assert-Equal -Actual ([int]$summary.manual_review_pending_count) -Expected 1 `
    -Message "Summary should count the missing review as pending."
Assert-Equal -Actual ([int]$summary.visual_review_undetermined_count) -Expected 1 `
    -Message "Summary should count the undetermined review."
Assert-Equal -Actual ([string]$summary.overall_status) -Expected "passed_with_pending_visual_review" `
    -Message "Overall status should prefer pending review over undetermined review."
Assert-Equal -Actual ([string]$summary.visual_verdict) -Expected "pending_manual_review" `
    -Message "Visual verdict should prefer pending review over undetermined review."

$passEntry = Get-SummaryEntry -Summary $summary -Name "visual-pass"
$undeterminedEntry = Get-SummaryEntry -Summary $summary -Name "visual-undetermined"
$missingEntry = Get-SummaryEntry -Summary $summary -Name "visual-missing"

Assert-Equal -Actual ([string]$passEntry.status) -Expected "passed" `
    -Message "Passed review result should update entry status to passed."
Assert-Equal -Actual ([bool]$passEntry.manual_review_pending) -Expected $false `
    -Message "Passed review should clear manual_review_pending."
Assert-Equal -Actual ([bool]$passEntry.visual_review_undetermined) -Expected $false `
    -Message "Passed review should clear visual_review_undetermined."
Assert-Equal -Actual ([string]$passEntry.checks.visual_smoke.review_status) -Expected "completed" `
    -Message "Passed review should refresh review_status from review_result_json."
Assert-Equal -Actual ([string]$passEntry.checks.visual_smoke.review_verdict) -Expected "pass" `
    -Message "Passed review should refresh review_verdict from review_result_json."
Assert-Equal -Actual ([int]$passEntry.checks.visual_smoke.findings_count) -Expected 1 `
    -Message "Passed review should refresh findings_count."

Assert-Equal -Actual ([string]$undeterminedEntry.status) -Expected "passed_with_visual_review_undetermined" `
    -Message "Undetermined review should derive the undetermined entry status."
Assert-Equal -Actual ([bool]$undeterminedEntry.manual_review_pending) -Expected $false `
    -Message "Undetermined review should not be pending."
Assert-Equal -Actual ([bool]$undeterminedEntry.visual_review_undetermined) -Expected $true `
    -Message "Undetermined review should persist visual_review_undetermined on the entry."
Assert-Equal -Actual ([string]$undeterminedEntry.checks.visual_smoke.review_verdict) -Expected "undetermined" `
    -Message "Undetermined review should refresh review_verdict."

Assert-Equal -Actual ([string]$missingEntry.status) -Expected "passed_with_pending_visual_review" `
    -Message "Missing review result should derive the pending visual-review status."
Assert-Equal -Actual ([bool]$missingEntry.manual_review_pending) -Expected $true `
    -Message "Missing review result should set manual_review_pending."
Assert-Equal -Actual ([bool]$missingEntry.visual_review_undetermined) -Expected $false `
    -Message "Missing review result should not set visual_review_undetermined."
Assert-Equal -Actual ([string]$missingEntry.checks.visual_smoke.review_status) -Expected "missing" `
    -Message "Missing review result should refresh review_status to missing."
Assert-Equal -Actual ([string]$missingEntry.checks.visual_smoke.review_verdict) -Expected "undecided" `
    -Message "Missing review result should keep the undecided verdict."
Assert-Equal -Actual ([int]$missingEntry.checks.visual_smoke.findings_count) -Expected 0 `
    -Message "Missing review result should default findings_count to zero."

$summaryMarkdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryMarkdownPath
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Visual review synced at:" `
    -Message "Summary Markdown should expose the sync timestamp."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Overall status: passed_with_pending_visual_review" `
    -Message "Summary Markdown should expose the derived overall status."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Visual verdict: pending_manual_review" `
    -Message "Summary Markdown should expose the aggregate visual verdict."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Visual smoke: review_status=completed review_verdict=pass findings=1" `
    -Message "Summary Markdown should include refreshed pass review metadata."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Visual smoke: review_status=completed review_verdict=undetermined findings=1" `
    -Message "Summary Markdown should include refreshed undetermined review metadata."
Assert-ContainsText -Text $summaryMarkdown -ExpectedText "Visual smoke: review_status=missing review_verdict=undecided findings=0" `
    -Message "Summary Markdown should include refreshed missing review metadata."

$releaseSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseSummaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$releaseSummary.steps.project_template_smoke.summary_json) -Expected $summaryPath `
    -Message "Release summary step should point at the synced smoke summary."
Assert-Equal -Actual ([string]$releaseSummary.steps.project_template_smoke.visual_verdict) -Expected "pending_manual_review" `
    -Message "Release summary step should include the aggregate visual verdict."
Assert-Equal -Actual ([int]$releaseSummary.steps.project_template_smoke.visual_review_undetermined_count) -Expected 1 `
    -Message "Release summary step should include the undetermined visual review count."
Assert-Equal -Actual ([string]$releaseSummary.project_template_smoke.overall_status) -Expected "passed_with_pending_visual_review" `
    -Message "Release summary project_template_smoke block should mirror the overall status."
Assert-Equal -Actual ([int]$releaseSummary.project_template_smoke.manual_review_pending_count) -Expected 1 `
    -Message "Release summary project_template_smoke block should mirror the pending count."
Assert-True -Condition (Test-Path -LiteralPath $releaseFinalReviewPath) `
    -Message "Sync should refresh release final_review.md."

$releaseFinalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseFinalReviewPath
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Project template smoke summary:" `
    -Message "Final review should mention the project-template smoke summary."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Project template smoke dirty schema baselines: 0" `
    -Message "Final review should include dirty schema baseline count."
Assert-ContainsText -Text $releaseFinalReview -ExpectedText "Project template smoke schema baseline drifts: 0" `
    -Message "Final review should include schema baseline drift count."

Write-Host "Project template smoke visual verdict sync regression passed."
