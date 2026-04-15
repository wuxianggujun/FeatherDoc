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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$bundleRoot = Join-Path $resolvedWorkingDir "section-page-setup-bundle"
$aggregateDir = Join-Path $bundleRoot "aggregate-evidence"
$contactSheetsDir = Join-Path $aggregateDir "contact-sheets"
$taskRoot = Join-Path $resolvedWorkingDir "tasks"
$prepareScript = Join-Path $resolvedRepoRoot "scripts\prepare_word_review_task.ps1"

New-Item -ItemType Directory -Path $contactSheetsDir -Force | Out-Null
New-Item -ItemType Directory -Path $taskRoot -Force | Out-Null

(@{
    generated_at = "2026-04-13T12:00:00"
    output_dir = $bundleRoot
    visual_enabled = $true
    aggregate_evidence = @{
        root = $aggregateDir
        contact_sheet = (Join-Path $aggregateDir "contact_sheet.png")
        contact_sheets_dir = $contactSheetsDir
    }
    cases = @(
        @{ id = "api-sample" },
        @{ id = "cli-rewrite" }
    )
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath (Join-Path $bundleRoot "summary.json") -Encoding UTF8

(@{
    generated_at = "2026-04-13T12:00:00"
    output_dir = $bundleRoot
    aggregate_evidence = @{
        root = $aggregateDir
        contact_sheet = (Join-Path $aggregateDir "contact_sheet.png")
        contact_sheets_dir = $contactSheetsDir
    }
    cases = @(
        @{
            id = "api-sample"
            expected_visual_cues = @("page 1 portrait", "page 2 landscape")
        },
        @{
            id = "cli-rewrite"
            expected_visual_cues = @("both pages landscape")
        }
    )
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath (Join-Path $bundleRoot "review_manifest.json") -Encoding UTF8

Set-Content -LiteralPath (Join-Path $bundleRoot "review_checklist.md") -Encoding UTF8 -Value "# checklist"
Set-Content -LiteralPath (Join-Path $bundleRoot "final_review.md") -Encoding UTF8 -Value "# final"
Set-Content -LiteralPath (Join-Path $aggregateDir "contact_sheet.png") -Encoding UTF8 -Value "png"
Set-Content -LiteralPath (Join-Path $contactSheetsDir "api-sample-contact_sheet.png") -Encoding UTF8 -Value "png"
Set-Content -LiteralPath (Join-Path $contactSheetsDir "cli-rewrite-contact_sheet.png") -Encoding UTF8 -Value "png"

& $prepareScript `
    -SectionPageSetupRegressionRoot $bundleRoot `
    -TaskOutputRoot $taskRoot `
    -Mode review-only

$latestPointerPath = Join-Path $taskRoot "latest_section-page-setup-regression-bundle_task.json"
Assert-True -Condition (Test-Path -LiteralPath $latestPointerPath) `
    -Message "Latest section page setup task pointer was not created."

$pointer = Get-Content -Raw -LiteralPath $latestPointerPath | ConvertFrom-Json
$taskDir = $pointer.task_dir
$manifestPath = Join-Path $taskDir "task_manifest.json"
$promptPath = Join-Path $taskDir "task_prompt.md"
$copiedAggregateContactSheet = Join-Path $taskDir "evidence\aggregate_contact_sheet.png"
$copiedContactSheetsDir = Join-Path $taskDir "evidence\aggregate-contact-sheets"

foreach ($path in @($taskDir, $manifestPath, $promptPath, $copiedAggregateContactSheet, $copiedContactSheetsDir)) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected prepared task artifact is missing: $path"
}

$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
Assert-True -Condition ($manifest.source.kind -eq "section-page-setup-regression-bundle") `
    -Message "Prepared task manifest recorded an unexpected source kind."
Assert-True -Condition ($null -ne $manifest.section_page_setup_bundle) `
    -Message "Prepared task manifest did not include section_page_setup_bundle metadata."
Assert-True -Condition ([System.IO.Path]::GetFullPath($manifest.section_page_setup_bundle.root) -eq [System.IO.Path]::GetFullPath($bundleRoot)) `
    -Message "Prepared task manifest recorded an unexpected bundle root."

Assert-Contains -Path $promptPath -ExpectedText "Section Page Setup Visual Review Task" -Label "task_prompt.md"
Assert-Contains -Path $promptPath -ExpectedText "api-sample" -Label "task_prompt.md"
Assert-Contains -Path $promptPath -ExpectedText "cli-rewrite" -Label "task_prompt.md"

Write-Host "Prepare section page setup review task regression passed."
