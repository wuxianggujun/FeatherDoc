param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Equal {
    param(
        $Actual,
        $Expected,
        [string]$Message
    )

    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-ContainsValue {
    param(
        [object[]]$Values,
        [string]$Expected,
        [string]$Message
    )

    if ($Values -notcontains $Expected) {
        throw $Message
    }
}

function Write-ManifestFixture {
    param(
        [string]$Path,
        [object[]]$Samples
    )

    $manifest = [ordered]@{
        '$schema' = "pdf_regression_manifest.schema.json"
        samples = $Samples
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $Path) -Force | Out-Null
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 -LiteralPath $Path
}

function New-SampleFixture {
    param(
        [string]$Id,
        [string]$Kind,
        [switch]$Cjk,
        [switch]$Visual
    )

    $sample = [ordered]@{
        id = $Id
        kind = $Kind
        output_file = "featherdoc-$Id.pdf"
        expected_pages = 1
        expected_text = @($Id)
    }
    if ($Cjk) {
        $sample.expect_cjk = $true
    }
    if ($Visual) {
        $sample.expect_visual_baseline = $true
    }
    return $sample
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    throw "WorkingDir is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$fakeRepo = Join-Path $resolvedWorkingDir "fake-repo"
$summaryPath = Join-Path $resolvedWorkingDir "summary.json"
$scriptPath = Join-Path $resolvedRepoRoot "scripts\compare_pdf_reference_branch_manifest.ps1"
$manifestPath = Join-Path $fakeRepo "test\pdf_regression_manifest.json"

New-Item -ItemType Directory -Path $fakeRepo -Force | Out-Null
& git -C $fakeRepo init -q
if ($LASTEXITCODE -ne 0) {
    throw "git init failed."
}

Write-ManifestFixture -Path $manifestPath -Samples @(
    (New-SampleFixture -Id "single-text" -Kind "single_text"),
    (New-SampleFixture -Id "cjk-text" -Kind "cjk_text" -Cjk),
    (New-SampleFixture -Id "missing-reference" -Kind "missing_reference" -Visual)
)
& git -C $fakeRepo add test/pdf_regression_manifest.json
& git -C $fakeRepo -c user.name=FeatherDoc -c user.email=featherdoc@example.invalid commit -q -m "reference manifest"
if ($LASTEXITCODE -ne 0) {
    throw "git reference commit failed."
}
& git -C $fakeRepo branch codex/pdf-reference
if ($LASTEXITCODE -ne 0) {
    throw "git branch failed."
}

Write-ManifestFixture -Path $manifestPath -Samples @(
    (New-SampleFixture -Id "single-text" -Kind "single_text"),
    (New-SampleFixture -Id "cjk-text" -Kind "cjk_text" -Cjk),
    (New-SampleFixture -Id "dev-only" -Kind "dev_only")
)

& $scriptPath `
    -RepoRoot $fakeRepo `
    -ReferenceBranch "codex/pdf-reference" `
    -OutputJson $summaryPath
if ($LASTEXITCODE -ne 0) {
    throw "compare_pdf_reference_branch_manifest.ps1 failed."
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
Assert-Equal -Actual $summary.source_schema -Expected "featherdoc.pdf_reference_branch_manifest_comparison.v1" `
    -Message "Summary should preserve the schema id."
Assert-Equal -Actual $summary.current.sample_count -Expected 3 `
    -Message "Summary should count current manifest samples."
Assert-Equal -Actual $summary.references.Count -Expected 1 `
    -Message "Summary should include one reference branch."

$reference = $summary.references[0]
Assert-Equal -Actual $reference.sample_count -Expected 3 `
    -Message "Reference summary should count branch manifest samples."
Assert-Equal -Actual $reference.cjk_sample_count -Expected 1 `
    -Message "Reference summary should count CJK samples."
Assert-Equal -Actual $reference.visual_baseline_sample_count -Expected 1 `
    -Message "Reference summary should count visual samples."
Assert-Equal -Actual $reference.missing_in_dev_count -Expected 1 `
    -Message "Reference summary should count missing sample ids."
Assert-ContainsValue -Values @($reference.missing_in_dev_ids) -Expected "missing-reference" `
    -Message "Reference summary should list ids missing from current dev."
Assert-Equal -Actual $reference.dev_only_count -Expected 1 `
    -Message "Reference summary should count ids that only exist in current dev."
Assert-ContainsValue -Values @($reference.dev_only_ids) -Expected "dev-only" `
    -Message "Reference summary should list current-only ids."
Assert-Equal -Actual $reference.whole_branch_merge_recommended -Expected $false `
    -Message "Reference summary should not recommend whole branch merges."
Assert-Equal -Actual $summary.total_missing_in_dev_count -Expected 1 `
    -Message "Overall summary should aggregate missing ids."

Write-Host "PDF reference branch manifest comparison test passed."
