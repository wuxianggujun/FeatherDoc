param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("all", "aggregate", "missing", "fail_on_missing", "explicit_input", "include_rollup")]
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

function Invoke-HandoffScript {
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
    ($Value | ConvertTo-Json -Depth 20) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Write-GovernanceFixtures {
    param(
        [string]$Root,
        [bool]$IncludeContentControl = $true,
        [bool]$IncludeProjectTemplate = $true
    )

    Write-JsonFile -Path (Join-Path $Root "numbering-catalog-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.numbering_catalog_governance_report.v1"
        status = "needs_review"
        release_ready = $false
        release_blocker_count = 1
        warning_count = 2
        release_blockers = @(
            [ordered]@{
                id = "numbering_catalog_governance.style_numbering_issues"
                severity = "error"
                status = "blocked"
                action = "review_style_numbering_audit"
                message = "Style numbering audit reported issues."
            }
        )
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "preview_style_numbering_repair"
                action = "preview_style_numbering_repair"
                title = "Preview style numbering repair"
            }
        )
        warnings = @(
            [ordered]@{
                id = "numbering_catalog_manifest_summary_missing"
                message = "No numbering catalog manifest summary was loaded."
            },
            [ordered]@{
                id = "document_skeleton.style_merge_suggestions_pending"
                source_schema = "featherdoc.document_skeleton_governance_rollup_report.v1"
                action = "review_style_merge_suggestions"
                style_merge_suggestion_count = 2
                message = "Document skeleton governance reports 2 duplicate style merge suggestion(s) awaiting review."
            }
        )
    })

    Write-JsonFile -Path (Join-Path $Root "table-layout-delivery-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.table_layout_delivery_governance_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        warning_count = 0
        release_blockers = @()
        action_item_count = 1
        action_items = @(
            [ordered]@{
                id = "run_table_style_quality_visual_regression"
                action = "run_table_style_quality_visual_regression"
                title = "Generate Word-rendered table layout evidence"
            }
        )
    })

    if ($IncludeContentControl) {
        Write-JsonFile -Path (Join-Path $Root "content-control-data-binding-governance\summary.json") -Value ([ordered]@{
            schema = "featherdoc.content_control_data_binding_governance_report.v1"
            status = "blocked"
            release_ready = $false
            release_blocker_count = 1
            warning_count = 0
            release_blockers = @(
                [ordered]@{
                    id = "content_control_data_binding.bound_placeholder"
                    severity = "error"
                    status = "placeholder_visible"
                    action = "sync_or_fill_bound_content_control"
                    message = "A data-bound content control is still showing placeholder text."
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "review_duplicate_content_control_binding"
                    action = "review_duplicate_content_control_binding"
                    title = "Review repeated content controls that share one Custom XML binding"
                }
            )
        })
    }

    if ($IncludeProjectTemplate) {
        Write-JsonFile -Path (Join-Path $Root "project-template-delivery-readiness\summary.json") -Value ([ordered]@{
            schema = "featherdoc.project_template_delivery_readiness_report.v1"
            status = "blocked"
            release_ready = $false
            release_blocker_count = 1
            warning_count = 0
            release_blockers = @(
                [ordered]@{
                    id = "project_template_delivery.pending_schema_approval"
                    severity = "error"
                    status = "blocked"
                    action = "approve_project_template_schema"
                    message = "Schema approval is still pending."
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "approve_project_template_schema"
                    action = "approve_project_template_schema"
                    title = "Approve project template schema before release"
                }
            )
        })
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_release_governance_handoff_report.ps1"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

if (Test-Scenario -Name "aggregate") {
    $inputRoot = Join-Path $resolvedWorkingDir "aggregate-input"
    $outputDir = Join-Path $resolvedWorkingDir "aggregate-report"
    Write-GovernanceFixtures -Root $inputRoot

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Aggregate handoff should pass without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $markdownPath = Join-Path $outputDir "release_governance_handoff.md"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Aggregate handoff should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Aggregate handoff should write Markdown report."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.release_governance_handoff_report.v1" `
        -Message "Summary should expose release governance handoff schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Aggregate handoff should be blocked when source blockers exist."
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 4 `
        -Message "Aggregate handoff should load all four default reports."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Aggregate handoff should not mark default reports missing."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 3 `
        -Message "Aggregate handoff should normalize release blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 4 `
        -Message "Aggregate handoff should normalize action items."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
        -Message "Aggregate handoff should preserve warning counts."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
        -Message "Aggregate handoff should materialize top-level warning details."
    $styleMergeWarning = @($summary.warnings | Where-Object { [string]$_.id -eq "document_skeleton.style_merge_suggestions_pending" } | Select-Object -First 1)
    Assert-Equal -Actual ([string]$styleMergeWarning[0].action) -Expected "review_style_merge_suggestions" `
        -Message "Aggregate handoff should preserve warning actions."
    Assert-Equal -Actual ([string]$styleMergeWarning[0].source_schema) -Expected "featherdoc.document_skeleton_governance_rollup_report.v1" `
        -Message "Aggregate handoff should preserve warning source schema."
    Assert-Equal -Actual ([int]$styleMergeWarning[0].style_merge_suggestion_count) -Expected 2 `
        -Message "Aggregate handoff should preserve warning action, source schema, and style merge counts."
    Assert-ContainsText -Text (($summary.next_commands | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "ReleaseBlockerRollupAutoDiscover" `
        -Message "Aggregate handoff should hand off to release candidate auto-discovery."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Release Governance Handoff" `
        -Message "Markdown should include handoff title."
    Assert-ContainsText -Text $markdown -ExpectedText "project_template_delivery_readiness" `
        -Message "Markdown should include project-template delivery readiness."
    Assert-ContainsText -Text $markdown -ExpectedText "content_control_data_binding_governance" `
        -Message "Markdown should include content-control data-binding governance."
    Assert-ContainsText -Text $markdown -ExpectedText "## Warnings" `
        -Message "Markdown should include the warnings section."
    Assert-ContainsText -Text $markdown -ExpectedText "### Release governance handoff warnings" `
        -Message "Markdown should include the top-level warning subsection."
    Assert-ContainsText -Text $markdown -ExpectedText '- warning_count: `2`' `
        -Message "Markdown should show the top-level warning count."
    Assert-ContainsText -Text $markdown -ExpectedText 'action: `review_style_merge_suggestions`' `
        -Message "Markdown should include warning actions."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' `
        -Message "Markdown should include warning source schema."
    Assert-ContainsText -Text $markdown -ExpectedText 'style_merge_suggestion_count: `2`' `
        -Message "Markdown should include warning style merge suggestion counts."
}

if (Test-Scenario -Name "missing") {
    $inputRoot = Join-Path $resolvedWorkingDir "missing-input"
    $outputDir = Join-Path $resolvedWorkingDir "missing-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Missing handoff should pass without fail-on-missing. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
        -Message "Existing blockers should still drive blocked status."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 1 `
        -Message "Missing handoff should record the missing project-template report."
    Assert-ContainsText -Text (($summary.reports | Where-Object { $_.status -eq "missing" } | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "project_template_delivery_readiness" `
        -Message "Missing handoff should identify the absent project-template report."
}

if (Test-Scenario -Name "fail_on_missing") {
    $inputRoot = Join-Path $resolvedWorkingDir "fail-missing-input"
    $outputDir = Join-Path $resolvedWorkingDir "fail-missing-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir,
        "-FailOnMissing"
    )
    if ($result.ExitCode -eq 0) {
        throw "Fail-on-missing handoff should fail. Output: $($result.Text)"
    }
    Assert-True -Condition (Test-Path -LiteralPath (Join-Path $outputDir "summary.json")) `
        -Message "Fail-on-missing handoff should still write summary.json."
}

if (Test-Scenario -Name "explicit_input") {
    $inputRoot = Join-Path $resolvedWorkingDir "explicit-input-root"
    $explicitRoot = Join-Path $resolvedWorkingDir "explicit-input"
    $outputDir = Join-Path $resolvedWorkingDir "explicit-report"
    Write-GovernanceFixtures -Root $inputRoot -IncludeProjectTemplate $false
    Write-JsonFile -Path (Join-Path $explicitRoot "project-summary.json") -Value ([ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        status = "ready"
        release_ready = $true
        release_blocker_count = 0
        release_blockers = @()
        action_item_count = 0
        action_items = @()
        warning_count = 0
    })

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-InputJson", (Join-Path $explicitRoot "project-summary.json"),
        "-OutputDir", $outputDir
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Explicit handoff should accept an explicit replacement report. Output: $($result.Text)"

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "summary.json") | ConvertFrom-Json
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 4 `
        -Message "Explicit handoff should count loaded defaults plus the explicit project-template report."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Explicit handoff should replace the default project-template report without missing defaults."
    Assert-Equal -Actual ([string]($summary.reports | Where-Object { $_.id -eq "project_template_delivery_readiness" } | Select-Object -First 1).source) -Expected "explicit" `
        -Message "Explicit handoff should mark the replacement source."
}

if (Test-Scenario -Name "include_rollup") {
    $inputRoot = Join-Path $resolvedWorkingDir "include-rollup-input"
    $outputDir = Join-Path $resolvedWorkingDir "include-rollup-report"
    Write-GovernanceFixtures -Root $inputRoot

    $result = Invoke-HandoffScript -Arguments @(
        "-InputRoot", $inputRoot,
        "-OutputDir", $outputDir,
        "-IncludeReleaseBlockerRollup"
    )
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Include-rollup handoff should pass without fail gates. Output: $($result.Text)"

    $summaryPath = Join-Path $outputDir "summary.json"
    $rollupSummaryPath = Join-Path $outputDir "release-blocker-rollup\summary.json"
    $rollupMarkdownPath = Join-Path $outputDir "release-blocker-rollup\release_blocker_rollup.md"
    foreach ($path in @($summaryPath, $rollupSummaryPath, $rollupMarkdownPath)) {
        Assert-True -Condition (Test-Path -LiteralPath $path) `
            -Message "Include-rollup handoff should write artifact: $path"
    }

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([bool]$summary.release_blocker_rollup.included) -Expected $true `
        -Message "Handoff summary should record the included rollup."
    Assert-Equal -Actual ([string]$summary.release_blocker_rollup.summary_json) -Expected $rollupSummaryPath `
        -Message "Handoff summary should expose nested rollup summary path."
    Assert-Equal -Actual ([string]$summary.release_blocker_rollup.status) -Expected "blocked" `
        -Message "Handoff summary should inline nested rollup status."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.source_report_count) -Expected 4 `
        -Message "Handoff summary should inline nested rollup source count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.release_blocker_count) -Expected 3 `
        -Message "Handoff summary should inline nested rollup blocker count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.action_item_count) -Expected 4 `
        -Message "Handoff summary should inline nested rollup action count."
    Assert-Equal -Actual ([int]$summary.release_blocker_rollup.warning_count) -Expected 2 `
        -Message "Handoff summary should inline nested rollup warning count."
    Assert-ContainsText -Text (($summary.release_blocker_rollup.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
        -Message "Handoff summary should inline nested rollup warning details."

    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Nested rollup should expose release blocker rollup schema."
    Assert-Equal -Actual ([int]$rollupSummary.source_report_count) -Expected 4 `
        -Message "Nested rollup should consume all loaded governance reports."
    Assert-Equal -Actual ([int]$rollupSummary.release_blocker_count) -Expected 3 `
        -Message "Nested rollup should preserve blocker count."
    Assert-Equal -Actual ([int]$rollupSummary.action_item_count) -Expected 4 `
        -Message "Nested rollup should preserve action item count."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $outputDir "release_governance_handoff.md")
    Assert-ContainsText -Text $markdown -ExpectedText "Release Blocker Rollup" `
        -Message "Handoff Markdown should include nested rollup section."
    Assert-ContainsText -Text $markdown -ExpectedText "Source reports: ``4``" `
        -Message "Handoff Markdown should show nested rollup source count."
    Assert-ContainsText -Text $markdown -ExpectedText "Warnings: ``2``" `
        -Message "Handoff Markdown should show nested rollup warning count."
    Assert-ContainsText -Text $markdown -ExpectedText "### Release blocker rollup warnings" `
        -Message "Handoff Markdown should include nested rollup warning subsection."
    Assert-ContainsText -Text $markdown -ExpectedText "document_skeleton.style_merge_suggestions_pending" `
        -Message "Handoff Markdown should show nested rollup warning details."
    Assert-ContainsText -Text $markdown -ExpectedText 'source_schema: `featherdoc.document_skeleton_governance_rollup_report.v1`' `
        -Message "Handoff Markdown should show nested rollup warning source schema."
    Assert-ContainsText -Text $markdown -ExpectedText 'style_merge_suggestion_count: `2`' `
        -Message "Handoff Markdown should show nested rollup style merge counts."
}

Write-Host "Release governance handoff report regression passed."
