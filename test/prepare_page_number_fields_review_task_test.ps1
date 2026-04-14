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
$bundleRoot = Join-Path $resolvedWorkingDir "page-number-fields-bundle"
$aggregateDir = Join-Path $bundleRoot "aggregate-evidence"
$contactSheetsDir = Join-Path $aggregateDir "contact-sheets"
$taskRoot = Join-Path $resolvedWorkingDir "tasks"
$prepareScript = Join-Path $resolvedRepoRoot "scripts\prepare_word_review_task.ps1"
$apiCaseDir = Join-Path $bundleRoot "api-sample"
$cliCaseDir = Join-Path $bundleRoot "cli-append"

New-Item -ItemType Directory -Path $contactSheetsDir -Force | Out-Null
New-Item -ItemType Directory -Path $taskRoot -Force | Out-Null
New-Item -ItemType Directory -Path $apiCaseDir -Force | Out-Null
New-Item -ItemType Directory -Path $cliCaseDir -Force | Out-Null

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
        @{
            id = "api-sample"
            docx_path = (Join-Path $apiCaseDir "page_number_fields.docx")
            field_summary_json = (Join-Path $apiCaseDir "field_summary.json")
        },
        @{
            id = "cli-append"
            docx_path = (Join-Path $cliCaseDir "page_number_fields_cli.docx")
            field_summary_json = (Join-Path $cliCaseDir "field_summary.json")
        }
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
            field_summary_json = (Join-Path $apiCaseDir "field_summary.json")
            expected_visual_cues = @("header PAGE field", "footer NUMPAGES field")
        },
        @{
            id = "cli-append"
            field_summary_json = (Join-Path $cliCaseDir "field_summary.json")
            expected_visual_cues = @("both pages show fields", "section geometry unchanged")
        }
    )
} | ConvertTo-Json -Depth 6) | Set-Content -LiteralPath (Join-Path $bundleRoot "review_manifest.json") -Encoding UTF8

Set-Content -LiteralPath (Join-Path $bundleRoot "review_checklist.md") -Encoding UTF8 -Value "# checklist"
Set-Content -LiteralPath (Join-Path $bundleRoot "final_review.md") -Encoding UTF8 -Value "# final"
Set-Content -LiteralPath (Join-Path $aggregateDir "contact_sheet.png") -Encoding UTF8 -Value "png"
Set-Content -LiteralPath (Join-Path $contactSheetsDir "api-sample-contact_sheet.png") -Encoding UTF8 -Value "png"
Set-Content -LiteralPath (Join-Path $contactSheetsDir "cli-append-contact_sheet.png") -Encoding UTF8 -Value "png"
Set-Content -LiteralPath (Join-Path $apiCaseDir "field_summary.json") -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath (Join-Path $cliCaseDir "field_summary.json") -Encoding UTF8 -Value "{}"

& $prepareScript `
    -PageNumberFieldsRegressionRoot $bundleRoot `
    -TaskOutputRoot $taskRoot `
    -Mode review-only

$latestPointerPath = Join-Path $taskRoot "latest_page-number-fields-regression-bundle_task.json"
Assert-True -Condition (Test-Path -LiteralPath $latestPointerPath) `
    -Message "Latest page number fields task pointer was not created."

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
Assert-True -Condition ($manifest.source.kind -eq "page-number-fields-regression-bundle") `
    -Message "Prepared task manifest recorded an unexpected source kind."
Assert-True -Condition ($null -ne $manifest.page_number_fields_bundle) `
    -Message "Prepared task manifest did not include page_number_fields_bundle metadata."
Assert-True -Condition ([System.IO.Path]::GetFullPath($manifest.page_number_fields_bundle.root) -eq [System.IO.Path]::GetFullPath($bundleRoot)) `
    -Message "Prepared task manifest recorded an unexpected bundle root."

Assert-Contains -Path $promptPath -ExpectedText "Page Number Fields Visual Review Task" -Label "task_prompt.md"
Assert-Contains -Path $promptPath -ExpectedText "api-sample" -Label "task_prompt.md"
Assert-Contains -Path $promptPath -ExpectedText "cli-append" -Label "task_prompt.md"

Write-Host "Prepare page number fields review task regression passed."
