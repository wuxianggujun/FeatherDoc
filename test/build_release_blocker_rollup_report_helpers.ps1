function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-NoUtf8Bom {
    param([string]$Path, [string]$Message)

    [byte[]]$bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
        throw $Message
    }
}

function Convert-TestComparableValue {
    param($Value)

    if ($null -eq $Value) {
        return ""
    }

    if ($Value -is [datetime]) {
        return $Value.ToString("yyyy-MM-ddTHH:mm:ss")
    }

    return [string]$Value
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    $actualText = Convert-TestComparableValue -Value $Actual
    $expectedText = Convert-TestComparableValue -Value $Expected
    if ($actualText -ne $expectedText) { throw "$Message Expected='$expectedText' Actual='$actualText'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-DoesNotContainText {
    param([string]$Text, [string]$UnexpectedText, [string]$Message)
    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Message Unexpected='$UnexpectedText'."
    }
}

function Assert-HasProperty {
    param($Object, [string]$Name, [string]$Message)
    if (-not ($Object.PSObject.Properties.Name -contains $Name)) {
        throw "$Message Missing property '$Name'."
    }
}

function Assert-SummaryGroupCount {
    param(
        [object[]]$Groups,
        [string]$PropertyName,
        [string]$Name,
        [int]$ExpectedCount,
        [string]$Message
    )

    $match = @($Groups | Where-Object { [string]($_.$PropertyName) -eq $Name }) | Select-Object -First 1
    if ($null -eq $match) {
        throw "$Message Missing summary group '$Name'."
    }

    Assert-Equal -Actual ([int]$match.count) -Expected $ExpectedCount `
        -Message $Message
}

function Assert-NonEmptyString {
    param($Value, [string]$Message)
    if ([string]::IsNullOrWhiteSpace([string]$Value)) {
        throw $Message
    }
}

function Get-TraceItemLabel {
    param($Item)

    foreach ($property in @("composite_id", "id", "project_id", "template_name", "action")) {
        if ($Item.PSObject.Properties.Name -contains $property) {
            $value = [string]$Item.$property
            if (-not [string]::IsNullOrWhiteSpace($value)) {
                return $value
            }
        }
    }

    return "<missing-id>"
}

function Assert-GovernanceTraceMetadata {
    param(
        [object[]]$Items,
        [string]$CollectionName,
        [bool]$ExpectOpenCommandProperty = $false
    )

    foreach ($item in @($Items)) {
        $itemLabel = Get-TraceItemLabel -Item $item
        $context = "Release blocker rollup $CollectionName item $itemLabel"

        foreach ($property in @("source_schema", "source_report_display", "source_json_display")) {
            Assert-HasProperty -Object $item -Name $property `
                -Message "$context should expose $property."
            Assert-NonEmptyString -Value $item.$property `
                -Message "$context should keep non-empty $property."
        }

        if ($ExpectOpenCommandProperty) {
            Assert-HasProperty -Object $item -Name "open_command" `
                -Message "$context should expose reviewer open command metadata."
            Assert-NonEmptyString -Value $item.open_command `
                -Message "$context should keep a non-empty reviewer open command."
        }
    }
}

function Assert-MarkdownListBlockContainsAll {
    param(
        [string]$Text,
        [string]$Anchor,
        [string[]]$ExpectedFragments,
        [string]$Message
    )

    $lines = $Text -split "\r?\n"
    for ($index = 0; $index -lt $lines.Count; $index++) {
        if ($lines[$index] -notmatch [regex]::Escape($Anchor)) {
            continue
        }

        $blockLines = @($lines[$index])
        for ($next = $index + 1; $next -lt $lines.Count; $next++) {
            if ([string]::IsNullOrWhiteSpace($lines[$next])) {
                break
            }
            if ($lines[$next] -notmatch '^\s+-\s') {
                break
            }
            $blockLines += $lines[$next]
        }

        $blockText = ($blockLines -join "`n") -replace '/', '\'
        $hasAllFragments = $true
        foreach ($fragment in $ExpectedFragments) {
            $normalizedFragment = $fragment -replace '/', '\'
            if ($blockText -notmatch [regex]::Escape($normalizedFragment)) {
                $hasAllFragments = $false
                break
            }
        }

        if ($hasAllFragments) {
            return
        }
    }

    throw $Message
}

function Test-Scenario {
    param([string]$Name)
    return ($Scenario -eq "all" -or $Scenario -eq $Name)
}

$wordVisualStandardReviewMetadata = @(
    [ordered]@{
        task_key = "smoke"
        review_task_key = "document"
        label = "Word visual smoke"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:10:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\document\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\document\report\final_review.md"
    },
    [ordered]@{
        task_key = "fixed_grid"
        review_task_key = "fixed_grid"
        label = "Fixed-grid merge/unmerge"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:20:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\fixed-grid\report\final_review.md"
    },
    [ordered]@{
        task_key = "section_page_setup"
        review_task_key = "section_page_setup"
        label = "Section page setup"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:30:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\section-page-setup\report\final_review.md"
    },
    [ordered]@{
        task_key = "page_number_fields"
        review_task_key = "page_number_fields"
        label = "Page number fields"
        verdict = "pass"
        review_status = "reviewed"
        reviewed_at = "2026-04-12T12:40:00"
        review_method = "operator_supplied"
        review_result_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\review_result.json"
        final_review_path = ".\output\word-visual-release-gate\review-tasks\page-number-fields\report\final_review.md"
    }
)

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
