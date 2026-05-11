param(
    [string]$RepoRoot,
    [string]$WorkingDir,
    [ValidateSet("aggregate", "fail_on_blocker")]
    [string]$Scenario = "aggregate"
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

    New-Item -ItemType Directory -Path ([System.IO.Path]::GetDirectoryName($Path)) -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-Report {
    param([string[]]$Arguments)

    $powerShellPath = (Get-Process -Id $PID).Path
    $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$inputDir = Join-Path $resolvedWorkingDir "input"
$outputDir = Join-Path $resolvedWorkingDir "report"
$inspectPath = Join-Path $inputDir "inspect-content-controls.json"
$syncPath = Join-Path $inputDir "sync-content-controls-from-custom-xml.json"

Write-JsonFile -Path $inspectPath -Value ([ordered]@{
        part = "body"
        entry_name = "word/document.xml"
        count = 3
        content_controls = @(
            [ordered]@{
                index = 0
                kind = "block"
                form_kind = "date"
                tag = "due_date"
                alias = "Due Date"
                id = "10"
                lock = "sdtLocked"
                data_binding_store_item_id = "{55555555-5555-5555-5555-555555555555}"
                data_binding_xpath = "/invoice/dueDate"
                data_binding_prefix_mappings = "xmlns:fd=`"urn:featherdoc`""
                checked = $null
                date_format = "yyyy/MM/dd"
                date_locale = "zh-CN"
                selected_list_item = $null
                list_items = @()
                showing_placeholder = $true
                text = "Due date placeholder"
            },
            [ordered]@{
                index = 1
                kind = "block"
                form_kind = "drop_down_list"
                tag = "status"
                alias = "Status"
                id = "11"
                lock = ""
                data_binding_store_item_id = ""
                data_binding_xpath = ""
                data_binding_prefix_mappings = ""
                checked = $null
                date_format = ""
                date_locale = ""
                selected_list_item = 0
                list_items = @([ordered]@{ display_text = "Draft"; value = "draft" })
                showing_placeholder = $false
                text = "Draft"
            },
            [ordered]@{
                index = 2
                kind = "block"
                form_kind = "plain_text"
                tag = "due_date_copy"
                alias = "Due Date Copy"
                id = "12"
                lock = ""
                data_binding_store_item_id = "{55555555-5555-5555-5555-555555555555}"
                data_binding_xpath = "/invoice/dueDate"
                data_binding_prefix_mappings = ""
                checked = $null
                date_format = ""
                date_locale = ""
                selected_list_item = $null
                list_items = @()
                showing_placeholder = $false
                text = "Due date: 2026-07-15"
            }
        )
    })

Write-JsonFile -Path $syncPath -Value ([ordered]@{
        scanned_content_controls = 3
        bound_content_controls = 2
        synced_content_controls = 1
        issue_count = 1
        synced_items = @(
            [ordered]@{
                part_entry_name = "word/document.xml"
                content_control_index = 2
                tag = "due_date_copy"
                alias = "Due Date Copy"
                store_item_id = "{55555555-5555-5555-5555-555555555555}"
                xpath = "/invoice/dueDate"
                previous_text = "old"
                value = "Due date: 2026-07-15"
            }
        )
        issues = @(
            [ordered]@{
                part_entry_name = "word/document.xml"
                content_control_index = 0
                tag = "due_date"
                alias = "Due Date"
                store_item_id = "{55555555-5555-5555-5555-555555555555}"
                xpath = "/invoice/missingDueDate"
                reason = "custom_xml_value_not_found"
            }
        )
    })

$scriptPath = Join-Path $resolvedRepoRoot "scripts\build_content_control_data_binding_governance_report.ps1"
$arguments = @(
    "-InputJson"
    "$inspectPath,$syncPath"
    "-OutputDir"
    $outputDir
)
if ($Scenario -eq "fail_on_blocker") {
    $arguments += "-FailOnBlocker"
}

$result = Invoke-Report -Arguments $arguments
if ($Scenario -eq "fail_on_blocker") {
    if ($result.ExitCode -eq 0) {
        throw "Content-control governance fail-on-blocker run should fail. Output: $($result.Text)"
    }
} else {
    Assert-Equal -Actual $result.ExitCode -Expected 0 `
        -Message "Content-control governance aggregate run should pass. Output: $($result.Text)"
}

$summaryPath = Join-Path $outputDir "summary.json"
$markdownPath = Join-Path $outputDir "content_control_data_binding_governance.md"
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Summary JSON was not created."
Assert-True -Condition (Test-Path -LiteralPath $markdownPath) `
    -Message "Markdown report was not created."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.content_control_data_binding_governance_report.v1" `
    -Message "Summary should expose schema."
Assert-Equal -Actual ([string]$summary.status) -Expected "blocked" `
    -Message "Summary should be blocked by sync issue and placeholder evidence."
Assert-Equal -Actual ([int]$summary.content_control_count) -Expected 3 `
    -Message "Summary should count inspected content controls."
Assert-Equal -Actual ([int]$summary.bound_content_control_count) -Expected 2 `
    -Message "Summary should count data-bound controls."
Assert-Equal -Actual ([int]$summary.sync_issue_count) -Expected 1 `
    -Message "Summary should count sync issues."
Assert-Equal -Actual ([int]$summary.release_blocker_count) -Expected 2 `
    -Message "Summary should create blockers for sync issue and bound placeholder."
Assert-True -Condition ([int]$summary.action_item_count -ge 2) `
    -Message "Summary should emit lock/unbound/duplicate review actions."

$blockerIds = @($summary.release_blockers | ForEach-Object { [string]$_.id }) -join "`n"
Assert-ContainsText -Text $blockerIds -ExpectedText "content_control_data_binding.custom_xml_sync_issue" `
    -Message "Summary should include Custom XML sync blocker."
Assert-ContainsText -Text $blockerIds -ExpectedText "content_control_data_binding.bound_placeholder" `
    -Message "Summary should include placeholder blocker."

$markdown = Get-Content -Raw -Encoding UTF8 -LiteralPath $markdownPath
Assert-ContainsText -Text $markdown -ExpectedText "# Content Control Data Binding Governance" `
    -Message "Markdown should include title."
Assert-ContainsText -Text $markdown -ExpectedText "fix_custom_xml_data_binding_source" `
    -Message "Markdown should include remediation action."

Write-Host "Content-control data-binding governance report regression passed."
