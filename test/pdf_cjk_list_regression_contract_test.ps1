param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

function Get-ManifestSampleBlock {
    param(
        [string]$ManifestText,
        [string]$SampleId
    )

    $pattern = '(?s)"id":\s*"' + [regex]::Escape($SampleId) + '".*?\n    \}'
    $matches = [regex]::Matches($ManifestText, $pattern)
    if ($matches.Count -ne 1) {
        throw "PDF regression manifest must define exactly one '$SampleId' sample."
    }
    return $matches[0].Value
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$samplePath = Join-Path $resolvedRepoRoot "samples\pdf_regression_sample.cpp"
$manifestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest.json"
$manifestTestPath = Join-Path $resolvedRepoRoot "test\pdf_regression_manifest_test.cpp"
$cmakePath = Join-Path $resolvedRepoRoot "test\CMakeLists.txt"
$cmakeModuleDir = Join-Path $resolvedRepoRoot "test\cmake"
$cmakeTextParts = @(Get-Content -Raw -Encoding UTF8 -LiteralPath $cmakePath)
if (Test-Path -LiteralPath $cmakeModuleDir) {
    $cmakeTextParts += Get-ChildItem -LiteralPath $cmakeModuleDir -Filter "*.cmake" |
        Sort-Object Name |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
}
$sampleTextParts = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePath
)
$sampleTextParts += Get-ChildItem -LiteralPath (Split-Path -Parent $samplePath) -Filter "pdf_regression_sample_*.inc" |
    Sort-Object Name |
    ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
$sampleText = $sampleTextParts -join "`n"
$manifestText = Get-Content -Raw -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -LiteralPath $manifestTestPath
$cmakeText = $cmakeTextParts -join "`n"

$requiredSamples = @(
    [ordered]@{
        Id = "document-cjk-bullet-list-text"
        Kind = "document_cjk_bullet_list_text"
        Builder = "build_document_cjk_bullet_list_text_sample"
        StableText = "BL-101"
        OutputFile = "featherdoc-pdf-regression-document-cjk-bullet-list-text.pdf"
    },
    [ordered]@{
        Id = "document-cjk-numbered-list-text"
        Kind = "document_cjk_numbered_list_text"
        Builder = "build_document_cjk_numbered_list_text_sample"
        StableText = "NL-101"
        OutputFile = "featherdoc-pdf-regression-document-cjk-numbered-list-text.pdf"
    }
)

foreach ($required in $requiredSamples) {
    $manifestSampleBlock = Get-ManifestSampleBlock `
        -ManifestText $manifestText `
        -SampleId $required.Id

    Assert-ContainsText -Text $sampleText -ExpectedText $required.Builder `
        -Message "PDF regression sample generator should keep builder '$($required.Builder)'."
    Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $required.Kind) `
        -Message "PDF regression sample runner should dispatch scenario '$($required.Kind)'."
    Assert-ContainsText -Text $sampleText -ExpectedText "append_document_list_item" `
        -Message "PDF CJK list samples should exercise the document list adapter path."
    Assert-ContainsText -Text $sampleText -ExpectedText "Document CJK List" `
        -Message "PDF CJK list samples should map an East Asia font family."
    Assert-ContainsText -Text $manifestText -ExpectedText ('"id": "{0}"' -f $required.Id) `
        -Message "PDF regression manifest should keep sample '$($required.Id)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $required.Kind) `
        -Message "PDF regression manifest should keep kind '$($required.Kind)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $required.OutputFile) `
        -Message "PDF regression manifest should keep output file '$($required.OutputFile)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
        -Message "PDF CJK list manifest samples should stay one-page lightweight samples."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $required.StableText `
        -Message "PDF regression manifest should include stable text '$($required.StableText)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_cjk": true' `
        -Message "PDF CJK list manifest samples should require CJK handling."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_unicode": true' `
        -Message "PDF CJK list manifest samples should require Unicode handling."
    Assert-ContainsText -Text $manifestTestText -ExpectedText $required.Id `
        -Message "PDF regression manifest parser test should assert sample '$($required.Id)'."
    Assert-ContainsText -Text $cmakeText -ExpectedText ('sample_kind STREQUAL "{0}"' -f $required.Kind) `
        -Message "CMake PDF regression registration should mark '$($required.Kind)' as a CJK test."
}

Write-Host "PDF CJK list regression contract passed."
