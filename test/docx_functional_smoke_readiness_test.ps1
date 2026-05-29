param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-True {
    param([bool]$Condition, [string]$Message)

    if (-not $Condition) {
        throw $Message
    }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)

    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    $Value | ConvertTo-Json -Depth 16 | Set-Content -Encoding UTF8 -LiteralPath $Path
}

function Write-NonEmptyPng {
    param([string]$Path)

    Add-Type -AssemblyName System.Drawing
    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    $bitmap = [System.Drawing.Bitmap]::new(320, 240)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.Clear([System.Drawing.Color]::White)
            for ($y = 0; $y -lt 240; $y += 12) {
                for ($x = 0; $x -lt 320; $x += 12) {
                    $color = if ((($x + $y) / 12) % 2 -eq 0) {
                        [System.Drawing.Color]::FromArgb(32 + ($x % 160), 48 + ($y % 144), 96)
                    } else {
                        [System.Drawing.Color]::FromArgb(220, 220 - ($x % 80), 80 + ($y % 120))
                    }
                    $brush = [System.Drawing.SolidBrush]::new($color)
                    try {
                        $graphics.FillRectangle($brush, $x, $y, 12, 12)
                    } finally {
                        $brush.Dispose()
                    }
                }
            }
        } finally {
            $graphics.Dispose()
        }
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $bitmap.Dispose()
    }
}

function New-VisualSmokeFixture {
    param([string]$Root, [string]$Verdict)

    $reportDir = Join-Path $Root "report"
    $pageDir = Join-Path $Root "evidence\pages"
    New-Item -ItemType Directory -Path $reportDir, $pageDir -Force | Out-Null
    Write-JsonFile -Path (Join-Path $reportDir "summary.json") -Value ([ordered]@{
            schema = "featherdoc.word_visual_smoke_summary.v1"
            status = "pass"
            page_count = 1
        })
    Write-JsonFile -Path (Join-Path $reportDir "review_result.json") -Value ([ordered]@{
            schema = "featherdoc.word_visual_review_result.v1"
            status = "reviewed"
            verdict = $Verdict
        })
    Write-NonEmptyPng -Path (Join-Path $Root "evidence\contact_sheet.png")
    Write-NonEmptyPng -Path (Join-Path $pageDir "page-01.png")
    "%PDF-1.4`n% fixture only`n" | Set-Content -Encoding ASCII -LiteralPath (Join-Path $Root "table_visual_smoke.pdf")
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_docx_functional_smoke_readiness.ps1"
$scriptText = Get-Content -Raw -Encoding UTF8 -LiteralPath $scriptPath
$docsPath = Join-Path $resolvedRepoRoot "docs\docx_functional_smoke_readiness_zh.rst"
$docsText = Get-Content -Raw -Encoding UTF8 -LiteralPath $docsPath
foreach ($marker in @(
        "featherdoc.docx_functional_smoke_readiness.v1",
        "persisted_docx_functional_smoke_evidence_only",
        "docx_functional_smoke_readiness_trace",
        "word_visual_smoke.pending_manual_review",
        "docx_package_integrity",
        "word_visual_smoke_reused_evidence",
        "content_controls",
        "sections_headers_footers",
        '$OutputDir',
        '$SummaryJson',
        '$ReportMarkdown',
        '$VisualSmokeRoots',
        '$FailOnWarning',
        "docx_functional_smoke_readiness.md"
    )) {
    Assert-ContainsText -Text $scriptText -ExpectedText $marker `
        -Message "DOCX functional smoke readiness script should preserve marker '$marker'."
}
foreach ($marker in @(
        "featherdoc.docx_functional_smoke_readiness.v1",
        "docx_functional_smoke_readiness_trace",
        "word_visual_smoke.pending_manual_review",
        "-OutputDir",
        "-SummaryJson",
        "-ReportMarkdown",
        "-VisualSmokeRoots",
        "-FailOnWarning",
        "docx_functional_smoke_readiness.md"
    )) {
    Assert-ContainsText -Text $docsText -ExpectedText $marker `
        -Message "DOCX functional smoke readiness docs should preserve marker '$marker'."
}

$visualSmokeRootA = Join-Path $resolvedWorkingDir "word-visual-smoke-fixture-a"
$visualSmokeRootB = Join-Path $resolvedWorkingDir "word-visual-smoke-fixture-b"
New-VisualSmokeFixture -Root $visualSmokeRootA -Verdict "pass"
New-VisualSmokeFixture -Root $visualSmokeRootB -Verdict "pass"

$summaryPath = Join-Path $resolvedWorkingDir "docx-functional-smoke-readiness.summary.json"
$reportMarkdownPath = Join-Path $resolvedWorkingDir "docx_functional_smoke_readiness.md"
& $scriptPath -RepoRoot $resolvedRepoRoot -SummaryJson $summaryPath -ReportMarkdown $reportMarkdownPath `
    -VisualSmokeRoots @($visualSmokeRootA, $visualSmokeRootB)
$lastExitCodeValue = Get-Variable -Name LASTEXITCODE -ValueOnly -ErrorAction SilentlyContinue
$exitCode = if ($null -eq $lastExitCodeValue) { 0 } else { [int]$lastExitCodeValue }
if ($exitCode -ne 0) {
    throw "check_docx_functional_smoke_readiness.ps1 failed with exit code $exitCode."
}
Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "DOCX functional smoke readiness summary was not written."
Assert-True -Condition (Test-Path -LiteralPath $reportMarkdownPath) `
    -Message "DOCX functional smoke readiness Markdown report was not written."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$summary.schema) -Expected "featherdoc.docx_functional_smoke_readiness.v1" `
    -Message "Summary schema mismatch."
Assert-Equal -Actual ([string]$summary.status) -Expected "pass" `
    -Message "Summary status should pass when persisted DOCX evidence is coherent."
if ([int]$summary.warning_count -gt 0) {
    Assert-Equal -Actual ([string]$summary.verdict) -Expected "pass_with_warnings" `
        -Message "Summary verdict should keep pending manual visual review as a warning."
    Assert-ContainsText -Text ([string]$summary.boundary) -ExpectedText "Pass-with-warnings means" `
        -Message "Boundary should describe warning semantics when warnings remain."
} else {
    Assert-Equal -Actual ([string]$summary.verdict) -Expected "pass" `
        -Message "Summary verdict should pass after screenshot-backed review verdicts are recorded."
    Assert-ContainsText -Text ([string]$summary.boundary) -ExpectedText "Pass means" `
        -Message "Boundary should describe closed DOCX readiness semantics without warnings."
}
Assert-True -Condition ([bool]$summary.docx_functional_smoke_ready) `
    -Message "DOCX functional smoke should be ready."
Assert-Equal -Actual ([int]$summary.failed_check_count) -Expected 0 `
    -Message "DOCX functional smoke should not have failed checks."
Assert-Equal -Actual ([int]$summary.capability_count) -Expected 8 `
    -Message "Capability matrix count mismatch."
Assert-Equal -Actual ([int]$summary.capability_pass_count) -Expected 8 `
    -Message "Every DOCX capability row should pass."
Assert-True -Condition (@($summary.visual_smoke_reused_evidence).Count -ge 2) `
    -Message "Persisted Word visual smoke evidence should include minimal and rerun roots."
Assert-ContainsText -Text ([string]$summary.report_markdown_display) -ExpectedText "docx_functional_smoke_readiness.md" `
    -Message "Summary should preserve the Markdown report display path."

foreach ($visualEvidence in @($summary.visual_smoke_reused_evidence)) {
    Assert-Equal -Actual ([string]$visualEvidence.status) -Expected "pass" `
        -Message "Persisted Word visual smoke evidence should pass non-empty checks."
    Assert-True -Condition ([bool]$visualEvidence.contact_sheet_non_empty_visual) `
        -Message "Contact sheet should be non-empty."
    Assert-True -Condition ([bool]$visualEvidence.page_image_non_empty_visual) `
        -Message "Page PNG should be non-empty."
}

Write-Host "DOCX functional smoke readiness regression passed."
