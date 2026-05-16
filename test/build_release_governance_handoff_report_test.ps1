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
        [bool]$IncludeProjectTemplate = $true,
        [bool]$IncludeCalibration = $true
    )

    Write-JsonFile -Path (Join-Path $Root "numbering-catalog-governance\summary.json") -Value ([ordered]@{
        schema = "featherdoc.numbering_catalog_governance_report.v1"
        status = "needs_review"
        release_ready = $false
        release_blocker_count = 1
        warning_count = 1
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

    if ($IncludeCalibration) {
        Write-JsonFile -Path (Join-Path $Root "schema-patch-confidence-calibration\summary.json") -Value ([ordered]@{
            schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
            status = "pending_review"
            release_ready = $false
            release_blocker_count = 1
            warning_count = 1
            release_blockers = @(
                [ordered]@{
                    id = "schema_patch_confidence_calibration.pending_schema_approvals"
                    severity = "error"
                    status = "pending_review"
                    action = "resolve_pending_schema_approvals"
                    message = "Schema patch confidence calibration has pending approvals."
                    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                }
            )
            action_item_count = 1
            action_items = @(
                [ordered]@{
                    id = "resolve_pending_schema_approvals"
                    action = "resolve_pending_schema_approvals"
                    title = "Resolve pending schema approvals"
                    open_command = "pwsh -ExecutionPolicy Bypass -File .\scripts\write_schema_patch_confidence_calibration_report.ps1"
                    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
                }
            )
            warnings = @(
                [ordered]@{
                    id = "schema_patch_confidence_calibration.unscored_candidates"
                    action = "add_explicit_confidence_metadata"
                    message = "Some schema patch candidates do not carry explicit confidence metadata."
                    source_schema = "featherdoc.schema_patch_confidence_calibration_report.v1"
                    source_json_display = ".\output\schema-patch-confidence-calibration\summary.json"
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
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 5 `
        -Message "Aggregate handoff should load all five default reports."
    Assert-Equal -Actual ([int]$summary.missing_report_count) -Expected 0 `
        -Message "Aggregate handoff should not mark default reports missing."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 4 `
        -Message "Aggregate handoff should normalize release blockers."
    Assert-Equal -Actual ([int]$summary.action_item_count) -Expected 5 `
        -Message "Aggregate handoff should normalize action items."
    Assert-Equal -Actual ([int]$summary.warning_count) -Expected 2 `
        -Message "Aggregate handoff should preserve warning counts."
    Assert-Equal -Actual (@($summary.warnings).Count) -Expected 2 `
        -Message "Aggregate handoff should normalize warning details."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "numbering_catalog_manifest_summary_missing" `
        -Message "Aggregate handoff should preserve numbering warning id."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Aggregate handoff should preserve warning source schema."
    Assert-ContainsText -Text (($summary.warnings | ForEach-Object { [string]$_.source_json_display }) -join "`n") `
        -ExpectedText "schema-patch-confidence-calibration\summary.json" `
        -Message "Aggregate handoff should preserve warning source JSON display."
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
    Assert-ContainsText -Text $markdown -ExpectedText "schema_patch_confidence_calibration" `
        -Message "Markdown should include schema patch confidence calibration."
    Assert-ContainsText -Text $markdown -ExpectedText "## Warnings" `
        -Message "Markdown should include handoff warnings."
    Assert-ContainsText -Text $markdown -ExpectedText "numbering_catalog_manifest_summary_missing" `
        -Message "Markdown should include warning id."
    Assert-ContainsText -Text $markdown -ExpectedText "source_json_display" `
        -Message "Markdown should include warning source JSON display."
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
    Assert-Equal -Actual ([int]$summary.loaded_report_count) -Expected 5 `
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

    $rollupSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $rollupSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$rollupSummary.schema) -Expected "featherdoc.release_blocker_rollup_report.v1" `
        -Message "Nested rollup should expose release blocker rollup schema."
    Assert-Equal -Actual ([int]$rollupSummary.source_report_count) -Expected 5 `
        -Message "Nested rollup should consume all loaded governance reports."
    Assert-Equal -Actual ([int]$rollupSummary.release_blocker_count) -Expected 4 `
        -Message "Nested rollup should preserve blocker count."
    Assert-Equal -Actual ([int]$rollupSummary.action_item_count) -Expected 5 `
        -Message "Nested rollup should preserve action item count."
    Assert-ContainsText -Text (($rollupSummary.release_blockers | ForEach-Object { [string]$_.source_schema }) -join "`n") `
        -ExpectedText "featherdoc.schema_patch_confidence_calibration_report.v1" `
        -Message "Nested rollup should preserve calibration source schema."
}

Write-Host "Release governance handoff report regression passed."
