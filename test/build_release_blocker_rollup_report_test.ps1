param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "passing", "comma_input", "empty", "malformed", "dedupe")]
    [string]$Scenario = "all"
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

function Test-Scenario {
    param([string]$Name)
    return ($Scenario -eq "all" -or $Scenario -eq $Name)
}

function Invoke-RollupScript {
    param([string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Unable to resolve a PowerShell executable for the nested script invocation."
        }
        $powerShellPath = $powerShellCommand.Source
    }
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)
    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $Path -Encoding UTF8
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_blocker_rollup_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$fixtureRoot = Join-Path $resolvedWorkingDir "fixtures"
$documentSkeletonPath = Join-Path $fixtureRoot "document-skeleton\document_skeleton_governance.summary.json"
$tableLayoutPath = Join-Path $fixtureRoot "table-layout\summary.json"
$releaseCandidatePath = Join-Path $fixtureRoot "release-candidate\summary.json"
$emptyPath = Join-Path $fixtureRoot "empty\summary.json"
$malformedPath = Join-Path $fixtureRoot "malformed\summary.json"
$dedupePath = Join-Path $fixtureRoot "dedupe\summary.json"

Write-JsonFile -Path $documentSkeletonPath -Value ([ordered]@{
    schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "document_skeleton.style_numbering_issues"
            severity = "error"
            status = "needs_review"
            message = "Style numbering audit reported issues."
            action = "review_style_numbering_audit"
            source_json = "output/document-skeleton-governance/contract/style-numbering-audit.json"
            source_json_display = ".\output\document-skeleton-governance\contract\style-numbering-audit.json"
        }
    )
    action_items = @(
        [ordered]@{
            id = "preview_style_numbering_repair"
            action = "preview_style_numbering_repair"
            title = "Preview style numbering repair"
            command = "featherdoc_cli repair-style-numbering input.docx --plan-only --json"
        }
    )
    warning_count = 1
    warnings = @(
        [ordered]@{
            id = "document_skeleton.exemplar_catalog_missing"
            action = "open_document_skeleton_rollup"
            message = "One exemplar catalog path is missing."
            source_json = "output/document-skeleton-governance-rollup/summary.json"
            source_json_display = ".\output\document-skeleton-governance-rollup\summary.json"
        }
    )
})

Write-JsonFile -Path $tableLayoutPath -Value ([ordered]@{
    schema = "featherdoc.table_layout_delivery_report.v1"
    release_blocker_count = 2
    release_blockers = @(
        [ordered]@{
            id = "table_layout.manual_table_style_quality_work"
            severity = "warning"
            message = "Manual table style work remains."
            action = "review_table_style_quality_plan"
        },
        [ordered]@{
            id = "table_layout.positioned_tables_need_review"
            severity = "warning"
            message = "Existing floating positions need review."
            action = "review_table_position_plan"
        }
    )
    action_items = @(
        [ordered]@{
            id = "review_table_position_preset"
            action = "review_table_position_preset"
            title = "Review table position preset"
            command = "featherdoc_cli apply-table-position-plan plan.json --dry-run --json"
        }
    )
})

Write-JsonFile -Path $releaseCandidatePath -Value ([ordered]@{
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            severity = "error"
            status = "blocked"
            message = "Schema approval blocks release."
            action = "fix_schema_patch_approval_result"
        }
    )
    action_items = @()
})

Write-JsonFile -Path $emptyPath -Value ([ordered]@{
    schema = "featherdoc.empty_report.v1"
    release_blocker_count = 0
    release_blockers = @()
    action_items = @()
})

Write-JsonFile -Path $malformedPath -Value ([ordered]@{
    schema = "featherdoc.malformed_report.v1"
    release_blocker_count = 3
    release_blockers = @(
        [ordered]@{
            id = "malformed.actual_blocker"
            severity = "error"
            message = "Only one blocker is present."
            action = "fix_malformed_fixture"
        }
    )
    action_items = @()
})

Write-JsonFile -Path $dedupePath -Value ([ordered]@{
    schema = "featherdoc.dedupe_report.v1"
    release_blocker_count = 1
    release_blockers = @(
        [ordered]@{
            id = "project_template_smoke.schema_approval"
            severity = "error"
            status = "blocked"
            message = "A second report has the same id."
            action = "fix_schema_patch_approval_result"
        }
    )
    action_items = @(
        [ordered]@{
            id = "same_action"
            action = "fix_schema_patch_approval_result"
            title = "Fix schema approval"
        }
    )
})

if (Test-Scenario -Name "passing") {
    $outputDir = Join-Path $resolvedWorkingDir "passing-report"
    $passingInputRoot = Join-Path $resolvedWorkingDir "passing-input"
    Write-JsonFile -Path (Join-Path $passingInputRoot "document-skeleton\document_skeleton_governance.summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $documentSkeletonPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "table-layout\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $tableLayoutPath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $passingInputRoot "release-candidate\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseCandidatePath | ConvertFrom-Json)
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $passingInputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Rollup should succeed without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "release_blocker_rollup.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Rollup should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Rollup should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Summary should expose rollup schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Rollup should be blocked when blockers exist."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Rollup should aggregate all blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 2 `
        -Message "Rollup should aggregate action items."
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 3 `
        -Message "Rollup should keep source report count."
    Assert-ContainsText -Text ([string]$summary.release_blockers[0].composite_id) `
        -ExpectedText "source1.blocker1" `
        -Message "Rollup should generate composite blocker ids."
    $skeletonBlocker = ($summary.release_blockers |
        Where-Object { [string]$_.id -eq "document_skeleton.style_numbering_issues" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$skeletonBlocker.source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Rollup should preserve document skeleton source schema."
    Assert-Equal -Actual ([string]$skeletonBlocker.action) -Expected "review_style_numbering_audit" `
        -Message "Rollup should preserve blocker action."
    Assert-ContainsText -Text ([string]$skeletonBlocker.message) -ExpectedText "Style numbering audit" `
        -Message "Rollup should preserve blocker message."
    Assert-ContainsText -Text ([string]$skeletonBlocker.source_json_display) -ExpectedText "style-numbering-audit.json" `
        -Message "Rollup should preserve blocker source JSON display."
    $skeletonAction = ($summary.action_items |
        Where-Object { [string]$_.id -eq "preview_style_numbering_repair" } |
        Select-Object -First 1)
    Assert-ContainsText -Text ([string]$skeletonAction.open_command) -ExpectedText "repair-style-numbering" `
        -Message "Rollup should expose action item open command."
    $skeletonWarning = ($summary.warnings |
        Where-Object { [string]$_.id -eq "document_skeleton.exemplar_catalog_missing" } |
        Select-Object -First 1)
    Assert-Equal -Actual ([string]$skeletonWarning.action) -Expected "open_document_skeleton_rollup" `
        -Message "Rollup should preserve warning action."
    Assert-ContainsText -Text ([string]$skeletonWarning.message) -ExpectedText "exemplar catalog" `
        -Message "Rollup should preserve warning message."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blocker Rollup Report" `
        -Message "Markdown should include title."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_smoke.schema_approval" `
        -Message "Markdown should include release candidate blocker."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display" `
        -Message "Markdown should include source JSON display details."
}

if (Test-Scenario -Name "empty") {
    $outputDir = Join-Path $resolvedWorkingDir "empty-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $emptyPath,
        "-OutputDir", $outputDir,
        "-FailOnBlocker"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Empty rollup should pass fail-on-blocker. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "ready" `
        -Message "Empty rollup should be ready."
    Assert-Equal -Actual ([bool]$summary.release_ready) -Expected $true `
        -Message "Empty rollup should be release-ready."
}

if (Test-Scenario -Name "comma_input") {
    $outputDir = Join-Path $resolvedWorkingDir "comma-input-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", "$documentSkeletonPath,$tableLayoutPath,$releaseCandidatePath",
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Rollup should accept comma-separated input JSON paths. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.source_report_count) -Expected 3 `
        -Message "Comma-separated input should keep all three source reports."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Comma-separated input should aggregate all blockers."
    Assert-ContainsText -Text (($summary.source_reports | ForEach-Object { [string]$_.path_display }) -join "`n") `
        -ExpectedText "release-candidate" `
        -Message "Comma-separated input should include the final source path."
}

if (Test-Scenario -Name "malformed") {
    $outputDir = Join-Path $resolvedWorkingDir "malformed-report"
    $result = Invoke-RollupScript -Arguments @(
        "-InputJson", $malformedPath,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Malformed count should warn but not fail by default. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 1 `
        -Message "Malformed count should produce one warning."
    Assert-ContainsText -Text ([string]$summary.warnings[0].message) -ExpectedText "release_blocker_count is 3" `
        -Message "Warning should explain count mismatch."
}

if (Test-Scenario -Name "dedupe") {
    $outputDir = Join-Path $resolvedWorkingDir "dedupe-report"
    $dedupeInputRoot = Join-Path $resolvedWorkingDir "dedupe-input"
    Write-JsonFile -Path (Join-Path $dedupeInputRoot "release-candidate\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $releaseCandidatePath | ConvertFrom-Json)
    Write-JsonFile -Path (Join-Path $dedupeInputRoot "dedupe\summary.json") `
        -Value (Get-Content -Raw -Encoding UTF8 -LiteralPath $dedupePath | ConvertFrom-Json)
    $result = Invoke-RollupScript -Arguments @(
        "-InputRoot", $dedupeInputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Duplicate ids across reports should remain traceable. Output: $($result.Text)"
    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Duplicate blocker ids from different reports should both be retained."
    Assert-Equal -Actual ([int]$summary.blocker_id_summary[0].count) -Expected 2 `
        -Message "Blocker id summary should count duplicates."
    Assert-True -Condition ([string]$summary.release_blockers[0].composite_id -ne [string]$summary.release_blockers[1].composite_id) `
        -Message "Composite ids should keep duplicate blockers distinct."
}

Write-Host "Release blocker rollup report regression passed."
