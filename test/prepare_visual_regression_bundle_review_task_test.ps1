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

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText'. Actual: $Text"
    }
}

function Invoke-ExpectFailure {
    param(
        [scriptblock]$Action,
        [string]$ExpectedText,
        [string]$Label
    )

    try {
        & $Action
    } catch {
        Assert-ContainsText -Text $_.Exception.Message -ExpectedText $ExpectedText -Label $Label
        return
    }

    throw "$Label unexpectedly succeeded."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$prepareScript = Join-Path $resolvedRepoRoot "scripts\prepare_word_review_task.ps1"
$bundleRoot = Join-Path $resolvedWorkingDir "visual-bundle"
$aggregateEvidenceDir = Join-Path $bundleRoot "aggregate-evidence"
$taskRoot = Join-Path $resolvedWorkingDir "tasks"
New-Item -ItemType Directory -Path $aggregateEvidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $taskRoot -Force | Out-Null
Set-Content -LiteralPath (Join-Path $bundleRoot "summary.json") -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath (Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png") -Encoding UTF8 -Value "png"

& $prepareScript `
    -VisualRegressionBundleRoot $bundleRoot `
    -VisualRegressionBundleLabel "Sample visual" `
    -VisualRegressionBundleKey "sample-visual-regression-bundle" `
    -TaskOutputRoot $taskRoot `
    -Mode review-only

$latestPointerPath = Join-Path $taskRoot "latest_sample-visual-regression-bundle_task.json"
Assert-True -Condition (Test-Path -LiteralPath $latestPointerPath) `
    -Message "Latest visual bundle task pointer was not created."
$pointer = Get-Content -Raw -LiteralPath $latestPointerPath | ConvertFrom-Json
$manifestPath = $pointer.manifest_path
$manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
$copiedContactSheetPath = Join-Path $pointer.evidence_dir "aggregate-evidence\before_after_contact_sheet.png"
Assert-True -Condition ($manifest.source.kind -eq "visual-regression-bundle") `
    -Message "Prepared visual bundle task recorded an unexpected source kind."
Assert-True -Condition ($null -ne $manifest.visual_regression_bundle) `
    -Message "Prepared visual bundle task did not include visual_regression_bundle metadata."
Assert-True -Condition ([System.IO.Path]::GetFileName($manifest.visual_regression_bundle.source_aggregate_contact_sheet) -eq "before_after_contact_sheet.png") `
    -Message "Prepared visual bundle task accepted an unexpected source aggregate contact sheet."
Assert-True -Condition (Test-Path -LiteralPath $copiedContactSheetPath) `
    -Message "Prepared visual bundle task did not copy before_after_contact_sheet.png."

$oldBundleRoot = Join-Path $resolvedWorkingDir "visual-bundle-old-contact-sheet"
$oldAggregateEvidenceDir = Join-Path $oldBundleRoot "aggregate-evidence"
New-Item -ItemType Directory -Path $oldAggregateEvidenceDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $oldBundleRoot "summary.json") -Encoding UTF8 -Value "{}"
Set-Content -LiteralPath (Join-Path $oldAggregateEvidenceDir "contact_sheet.png") -Encoding UTF8 -Value "png"

Invoke-ExpectFailure `
    -Label "visual bundle old contact sheet" `
    -ExpectedText "before_after_contact_sheet.png" `
    -Action {
        & $prepareScript `
            -VisualRegressionBundleRoot $oldBundleRoot `
            -VisualRegressionBundleLabel "Old visual" `
            -VisualRegressionBundleKey "old-visual-regression-bundle" `
            -TaskOutputRoot (Join-Path $resolvedWorkingDir "old-tasks") `
            -Mode review-only
    }

Write-Host "Prepare visual regression bundle review task contact sheet regression passed."
