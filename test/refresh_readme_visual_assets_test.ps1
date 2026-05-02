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
$scriptPath = Join-Path $resolvedRepoRoot "scripts\refresh_readme_visual_assets.ps1"
$tasksRoot = Join-Path $resolvedWorkingDir "tasks"
$documentTaskDir = Join-Path $tasksRoot "document-task"
$fixedGridTaskDir = Join-Path $tasksRoot "fixed-grid-task"
$assetsDir = Join-Path $resolvedWorkingDir "assets"

$documentPagesDir = Join-Path $documentTaskDir "evidence\pages"
$fixedGridEvidenceDir = Join-Path $fixedGridTaskDir "evidence"
New-Item -ItemType Directory -Path $documentPagesDir -Force | Out-Null
New-Item -ItemType Directory -Path $fixedGridEvidenceDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $documentTaskDir "evidence\contact_sheet.png") -Encoding UTF8 -Value "document contact"
Set-Content -LiteralPath (Join-Path $documentPagesDir "page-05.png") -Encoding UTF8 -Value "page 05"
Set-Content -LiteralPath (Join-Path $documentPagesDir "page-06.png") -Encoding UTF8 -Value "page 06"
Set-Content -LiteralPath (Join-Path $fixedGridEvidenceDir "aggregate_contact_sheet.png") -Encoding UTF8 -Value "fixed aggregate"

& $scriptPath `
    -TaskOutputRoot $tasksRoot `
    -DocumentTaskDir $documentTaskDir `
    -FixedGridTaskDir $fixedGridTaskDir `
    -AssetsDir $assetsDir `
    -ColumnWidthVisualDir "" `
    -MergeRightVisualDir "" `
    -MergeDownVisualDir "" `
    -ChineseTemplateVisualDir ""

foreach ($path in @(
        (Join-Path $assetsDir "visual-smoke-contact-sheet.png"),
        (Join-Path $assetsDir "visual-smoke-page-05.png"),
        (Join-Path $assetsDir "visual-smoke-page-06.png"),
        (Join-Path $assetsDir "fixed-grid-aggregate-contact-sheet.png")
    )) {
    Assert-True -Condition (Test-Path -LiteralPath $path) `
        -Message "Expected README visual asset was not copied: $path"
}

$sourceDocumentRoot = Join-Path $resolvedWorkingDir "source-document-with-evidence"
$sourceDocumentPagesDir = Join-Path $sourceDocumentRoot "evidence\pages"
New-Item -ItemType Directory -Path $sourceDocumentPagesDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $sourceDocumentRoot "document.docx") -Encoding UTF8 -Value "docx"
Set-Content -LiteralPath (Join-Path $sourceDocumentRoot "evidence\contact_sheet.png") -Encoding UTF8 -Value "source contact"
Set-Content -LiteralPath (Join-Path $sourceDocumentPagesDir "page-05.png") -Encoding UTF8 -Value "source page 05"
Set-Content -LiteralPath (Join-Path $sourceDocumentPagesDir "page-06.png") -Encoding UTF8 -Value "source page 06"
$missingDocumentTaskDir = Join-Path $tasksRoot "document-task-missing-local-evidence"
New-Item -ItemType Directory -Path $missingDocumentTaskDir -Force | Out-Null
(@{
    source = @{ kind = "document"; path = (Join-Path $sourceDocumentRoot "document.docx") }
} | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $missingDocumentTaskDir "task_manifest.json") -Encoding UTF8

Invoke-ExpectFailure `
    -Label "document source fallback" `
    -ExpectedText "Unable to resolve document evidence from task directory" `
    -Action {
        & $scriptPath `
            -TaskOutputRoot $tasksRoot `
            -DocumentTaskDir $missingDocumentTaskDir `
            -FixedGridTaskDir $fixedGridTaskDir `
            -AssetsDir (Join-Path $resolvedWorkingDir "assets-doc-fallback") `
            -ColumnWidthVisualDir "" `
            -MergeRightVisualDir "" `
            -MergeDownVisualDir "" `
            -ChineseTemplateVisualDir ""
    }

$sourceFixedGridRoot = Join-Path $resolvedWorkingDir "source-fixed-grid-with-evidence"
New-Item -ItemType Directory -Path (Join-Path $sourceFixedGridRoot "aggregate-evidence") -Force | Out-Null
Set-Content -LiteralPath (Join-Path $sourceFixedGridRoot "aggregate-evidence\contact_sheet.png") -Encoding UTF8 -Value "source aggregate"
$missingFixedGridTaskDir = Join-Path $tasksRoot "fixed-grid-task-missing-local-evidence"
New-Item -ItemType Directory -Path $missingFixedGridTaskDir -Force | Out-Null
(@{
    source = @{ kind = "fixed-grid-regression-bundle"; path = $sourceFixedGridRoot }
} | ConvertTo-Json -Depth 4) | Set-Content -LiteralPath (Join-Path $missingFixedGridTaskDir "task_manifest.json") -Encoding UTF8

Invoke-ExpectFailure `
    -Label "fixed-grid source fallback" `
    -ExpectedText "Unable to resolve fixed-grid aggregate evidence from task directory" `
    -Action {
        & $scriptPath `
            -TaskOutputRoot $tasksRoot `
            -DocumentTaskDir $documentTaskDir `
            -FixedGridTaskDir $missingFixedGridTaskDir `
            -AssetsDir (Join-Path $resolvedWorkingDir "assets-fixed-fallback") `
            -ColumnWidthVisualDir "" `
            -MergeRightVisualDir "" `
            -MergeDownVisualDir "" `
            -ChineseTemplateVisualDir ""
    }

Write-Host "README visual asset refresh task-local evidence regression passed."
