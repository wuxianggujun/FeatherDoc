param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir,
    [ValidateSet("all", "passing", "failing", "fail_on_issue")]
    [string]$Scenario = "all"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build\build_table_layout_delivery_report_test"
}

if (-not $WorkingDir) {
    $WorkingDir = $BuildDir
}

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

function Invoke-DeliveryReportScript {
    param([string[]]$Arguments)

    $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
    $powerShellPath = if ($powerShellCommand) { $powerShellCommand.Source } else { (Get-Process -Id $PID).Path }
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function ConvertTo-JsonPathText {
    param([string]$Path)
    return ($Path -replace '\\', '\\' -replace '"', '\"')
}

function Write-MockCli {
    param(
        [string]$Path,
        [switch]$FailPlan
    )

    $positionPlanBlock = if ($FailPlan) {
        @'
    "plan-table-position-presets" {
        Write-Output '{"command":"plan-table-position-presets","error":"mock position plan failure"}'
        exit 1
    }
'@
    } else {
        @'
    "plan-table-position-presets" {
        $planIndex = [Array]::IndexOf($Args, "--output-plan")
        if ($planIndex -lt 0 -or $planIndex + 1 -ge $Args.Count) {
            Write-Output '{"command":"plan-table-position-presets","error":"missing output plan"}'
            exit 2
        }
        $planPath = $Args[$planIndex + 1]
        New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($planPath)) -Force | Out-Null
        Set-Content -LiteralPath $planPath -Encoding UTF8 -Value '{"command":"plan-table-position-presets","preset":"paragraph-callout","automatic_count":2,"review_count":1,"already_matching_count":0,"table_count":3}'
        Write-Output ('{"command":"plan-table-position-presets","ok":true,"preset":"paragraph-callout","automatic_count":2,"review_count":1,"already_matching_count":0,"output_plan":"' + (ConvertTo-JsonPathText $planPath) + '"}')
        exit 0
    }
'@
    }

    $mock = @"
param([Parameter(ValueFromRemainingArguments = `$true)][string[]]`$Args)
function ConvertTo-JsonPathText {
    param([string]`$Path)
    return (`$Path -replace '\\', '\\' -replace '"', '\"')
}
`$command = `$Args[0]
switch (`$command) {
    "inspect-tables" {
        Write-Output '{"command":"inspect-tables","count":3,"tables":[{"index":0,"style_id":"DeliveryTable","layout_mode":"fixed"},{"index":1,"style_id":"DeliveryTable","layout_mode":"autofit"},{"index":2,"style_id":"DeliveryTable","layout_mode":null}]}'
        exit 0
    }
    "audit-table-style-quality" {
        Write-Output '{"command":"audit-table-style-quality","ok":false,"clean":false,"issue_count":3,"issues":[{"kind":"missing_region"},{"kind":"bad_tblLook"}]}'
        exit 0
    }
    "plan-table-style-quality-fixes" {
        Write-Output '{"command":"plan-table-style-quality-fixes","ok":true,"automatic_fix_count":2,"manual_fix_count":1,"items":[{"automatic":true},{"automatic":false}]}'
        exit 0
    }
$positionPlanBlock
    default {
        Write-Output ('{"command":"' + `$command + '","error":"unexpected command"}')
        exit 2
    }
}
"@

    Set-Content -LiteralPath $Path -Encoding UTF8 -Value $mock
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_table_layout_delivery_report.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "test\my_test.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$mockCli = Join-Path $resolvedWorkingDir "mock-featherdoc-cli.ps1"
$passingOutputDir = Join-Path $resolvedWorkingDir "passing-report"
Write-MockCli -Path $mockCli

if (Test-Scenario -Name "passing") {
    $passingResult = Invoke-DeliveryReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $passingOutputDir,
        "-CliPath", $mockCli,
        "-SkipBuild"
    )
    Assert-Equal -Actual $passingResult.ExitCode -Expected 0 `
        -Message "Table layout report should pass without -FailOnIssue even when review work exists. Output: $($passingResult.Text)"
    Assert-ContainsText -Text $passingResult.Text -ExpectedText "Summary JSON:" `
        -Message "Table layout report should print summary path."

    $summaryPath = Join-Path $passingOutputDir "summary.json"
    $markdownPath = Join-Path $passingOutputDir "table_layout_delivery_report.md"
    $positionPlanPath = Join-Path $passingOutputDir "table-position-paragraph-callout.plan.json"
    Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
        -Message "Table layout report should write summary.json."
    Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
        -Message "Table layout report should write Markdown report."
    Assert-True -Condition (Test-Path -LiteralPath $positionPlanPath) `
        -Message "Table layout report should write the floating table position plan."

    $summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.table_layout_delivery_report.v1" `
        -Message "Summary should expose the report schema."
    Assert-Equal -Actual ([string]$summary.status) -Expected "needs_review" `
        -Message "Summary should require review when table actions exist."
    Assert-Equal -Actual ([int]$summary.table_count) -Expected 3 `
        -Message "Summary should include inspect-tables table count."
    Assert-Equal -Actual ([int]$summary.fixed_layout_table_count) -Expected 1 `
        -Message "Summary should include fixed-layout table count."
    Assert-Equal -Actual ([int]$summary.autofit_layout_table_count) -Expected 1 `
        -Message "Summary should include autofit table count."
    Assert-Equal -Actual ([int]$summary.unspecified_layout_table_count) -Expected 1 `
        -Message "Summary should include unspecified layout table count."
    Assert-Equal -Actual ([int]$summary.table_style_issue_count) -Expected 3 `
        -Message "Summary should include table style issue count."
    Assert-Equal -Actual ([int]$summary.automatic_tblLook_fix_count) -Expected 2 `
        -Message "Summary should include automatic tblLook fix count."
    Assert-Equal -Actual ([int]$summary.manual_table_style_fix_count) -Expected 1 `
        -Message "Summary should include manual table style fix count."
    Assert-Equal -Actual ([int]$summary.table_position_automatic_count) -Expected 2 `
        -Message "Summary should include automatic table position count."
    Assert-Equal -Actual ([int]$summary.table_position_review_count) -Expected 1 `
        -Message "Summary should include table position review count."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_support_status) -Expected "partial" `
        -Message "Summary should expose PDF floating table support status."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_layout_boundary) `
        -Expected "stable_pdf_geometry_subset_not_full_word_wrapping" `
        -Message "Summary should expose the PDF floating table boundary."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_supported_geometry_count) -Expected 4 `
        -Message "Summary should count supported PDF floating table geometry rows."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_metadata_only_count) -Expected 5 `
        -Message "Summary should count metadata-only PDF floating table rows."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_tracked_geometry_count) -Expected 9 `
        -Message "Summary should count tracked PDF floating table geometry rows."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_supported_geometry_percent) -Expected 44 `
        -Message "Summary should expose supported PDF floating table geometry percentage."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_support_coverage) `
        -Expected "4/9 supported (44 percent); metadata_only=5" `
        -Message "Summary should expose PDF floating table support coverage."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_reviewer_focus) `
        -Expected "review metadata-only tblpPr fields before approving PDF-layout-sensitive release." `
        -Message "Summary should expose PDF floating table reviewer focus."
    Assert-ContainsText -Text (($summary.pdf_floating_table_metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "tblOverlap" `
        -Message "Summary should expose top-level metadata-only PDF floating table fields."
    Assert-ContainsText -Text (($summary.pdf_floating_table_review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "full Word-compatible floating table text wrapping" `
        -Message "Summary should expose top-level review-required PDF floating table fields."
    Assert-ContainsText -Text (($summary.metadata_only_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "vertical paragraph/inside/outside Word page-side context" `
        -Message "Summary should expose metadata-only fields through the generic reviewer alias."
    Assert-ContainsText -Text (($summary.review_required_fields | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "table overlap avoidance and collision resolution" `
        -Message "Summary should expose review-required fields through the generic reviewer alias."
    Assert-Equal -Actual ([int]$summary.pdf_floating_table_support.supported_geometry_percent) -Expected 44 `
        -Message "PDF support evidence should expose supported geometry percentage."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_support.support_coverage) `
        -Expected "4/9 supported (44 percent); metadata_only=5" `
        -Message "PDF support evidence should expose support coverage."
    Assert-Equal -Actual ([string]$summary.pdf_floating_table_support.reviewer_focus) `
        -Expected "review metadata-only tblpPr fields before approving PDF-layout-sensitive release." `
        -Message "PDF support evidence should expose reviewer focus."
    Assert-ContainsText -Text (($summary.pdf_floating_table_support.supported_geometry | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "topFromText" `
        -Message "PDF support evidence should mention topFromText support."
    Assert-ContainsText -Text (($summary.pdf_floating_table_support.metadata_only | ForEach-Object { [string]$_ }) -join "`n") `
        -ExpectedText "tblOverlap" `
        -Message "PDF support evidence should preserve tblOverlap as metadata-only."
    Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
        -Message "Summary should expose both manual style and positioned-table review blockers."
    Assert-True -Condition (@($summary.action_items).Count -ge 4) `
        -Message "Summary should expose actionable next steps."
    Assert-ContainsText -Text ([string]$summary.action_items[1].command) -ExpectedText "apply-table-style-quality-fixes" `
        -Message "Action items should include safe tblLook application command."
    Assert-ContainsText -Text (($summary.action_items | ForEach-Object { [string]$_.id }) -join "`n") `
        -ExpectedText "review_fixed_layout_grid_widths" `
        -Message "Action items should include fixed-layout grid width review."

    $markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
    Assert-ContainsText -Text $markdown -ExpectedText "Table Layout Delivery Report" `
        -Message "Markdown report should include a title."
    Assert-ContainsText -Text $markdown -ExpectedText "Suggested Actions" `
        -Message "Markdown report should include suggested actions."
    Assert-ContainsText -Text $markdown -ExpectedText "Fixed layout table count" `
        -Message "Markdown report should include fixed-layout table counts."
    Assert-ContainsText -Text $markdown -ExpectedText "Review fixed-layout tblGrid and tcW width evidence" `
        -Message "Markdown report should include fixed-layout review action."
    Assert-ContainsText -Text $markdown -ExpectedText "PDF Floating Table Support" `
        -Message "Markdown report should include PDF floating table support evidence."
    Assert-ContainsText -Text $markdown -ExpectedText "PDF floating table supported geometry" `
        -Message "Markdown report should include supported geometry percentage."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_support_coverage" `
        -Message "Markdown report should include PDF floating table support coverage marker."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_reviewer_focus" `
        -Message "Markdown report should include PDF floating table reviewer focus marker."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_metadata_only_fields" `
        -Message "Markdown report should include PDF floating table metadata-only field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "pdf_floating_table_review_required_fields" `
        -Message "Markdown report should include PDF floating table review-required field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "metadata_only_fields" `
        -Message "Markdown report should include generic metadata-only field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "review_required_fields" `
        -Message "Markdown report should include generic review-required field marker."
    Assert-ContainsText -Text $markdown -ExpectedText "metadata-only tblpPr" `
        -Message "Markdown report should include metadata-only tblpPr reviewer guidance."
    Assert-ContainsText -Text $markdown -ExpectedText "stable_pdf_geometry_subset_not_full_word_wrapping" `
        -Message "Markdown report should include the PDF floating table boundary."
    Assert-ContainsText -Text $markdown -ExpectedText "apply-table-position-plan" `
        -Message "Markdown report should include table position dry-run command."
}

if (Test-Scenario -Name "failing") {
    $failingCli = Join-Path $resolvedWorkingDir "mock-featherdoc-cli-failing-plan.ps1"
    $failingOutputDir = Join-Path $resolvedWorkingDir "failing-report"
    Write-MockCli -Path $failingCli -FailPlan
    $failingResult = Invoke-DeliveryReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $failingOutputDir,
        "-CliPath", $failingCli,
        "-SkipBuild"
    )
    Assert-Equal -Actual $failingResult.ExitCode -Expected 1 `
        -Message "Table layout report should fail when a CLI command fails. Output: $($failingResult.Text)"

    $failingSummaryPath = Join-Path $failingOutputDir "summary.json"
    Assert-True -Condition (Test-Path -LiteralPath $failingSummaryPath) `
        -Message "Failing table layout report should still write summary.json."
    $failingSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $failingSummaryPath | ConvertFrom-Json
    Assert-Equal -Actual ([int]$failingSummary.command_failure_count) -Expected 1 `
        -Message "Failing summary should count the failed command."
    Assert-Equal -Actual ([string]$failingSummary.status) -Expected "failed" `
        -Message "Failing summary should expose failed status."
    Assert-Equal -Actual ([int]$failingSummary.commands[3].exit_code) -Expected 1 `
        -Message "Failing summary should preserve the position plan exit code."
}

if (Test-Scenario -Name "fail_on_issue") {
    $failOnIssueOutputDir = Join-Path $resolvedWorkingDir "fail-on-issue-report"
    $failOnIssueResult = Invoke-DeliveryReportScript -Arguments @(
        "-InputDocx", $sampleDocx,
        "-OutputDir", $failOnIssueOutputDir,
        "-CliPath", $mockCli,
        "-SkipBuild",
        "-FailOnIssue"
    )
    Assert-Equal -Actual $failOnIssueResult.ExitCode -Expected 1 `
        -Message "Table layout report should fail with -FailOnIssue when review blockers exist. Output: $($failOnIssueResult.Text)"
}

Write-Host "Table layout delivery report regression passed."
