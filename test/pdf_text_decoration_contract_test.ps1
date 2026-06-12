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

$sampleTextParts = @(
    Get-Content -Raw -Encoding UTF8 -LiteralPath $samplePath
)
$sampleTextParts += Get-ChildItem -LiteralPath (Split-Path -Parent $samplePath) -Filter "pdf_regression_sample_*.inc" |
    Sort-Object Name |
    ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
$sampleText = $sampleTextParts -join "`n"
$manifestText = Get-Content -Raw -LiteralPath $manifestPath
$manifestTestText = Get-Content -Raw -LiteralPath $manifestTestPath
$cmakeText = Get-Content -Raw -LiteralPath $cmakePath

$samples = @(
    @{
        Id = "strikethrough-text"
        Kind = "strikethrough_text"
        Builder = "build_document_strikethrough_text_sample"
        Output = "featherdoc-pdf-regression-strikethrough-text.pdf"
        SourceKeys = @("Strikethrough sample", "Direct ", "Style ", "strikethrough")
        ManifestKeys = @("Strikethrough sample", "Direct strikethrough", "Style strikethrough")
    },
    @{
        Id = "superscript-subscript-text"
        Kind = "superscript_subscript_text"
        Builder = "build_document_superscript_subscript_text_sample"
        Output = "featherdoc-pdf-regression-superscript-subscript-text.pdf"
        SourceKeys = @("Superscript and subscript sample", "E = mc", "H", "O demonstrates")
        ManifestKeys = @("Superscript and subscript sample", "E = mc2", "H2O")
    },
    @{
        Id = "style-superscript-subscript-text"
        Kind = "style_superscript_subscript_text"
        Builder = "build_document_style_superscript_subscript_text_sample"
        Output = "featherdoc-pdf-regression-style-superscript-subscript-text.pdf"
        SourceKeys = @("Style superscript and subscript sample", "Style E = mc", "Style H", "O inherits")
        ManifestKeys = @("Style superscript and subscript sample", "Style E = mc2", "Style H2O")
    }
)

foreach ($sample in $samples) {
    $manifestSampleBlock = Get-ManifestSampleBlock `
        -ManifestText $manifestText `
        -SampleId $sample.Id

    Assert-ContainsText -Text $sampleText -ExpectedText $sample.Builder `
        -Message "PDF regression sample generator should keep builder '$($sample.Builder)'."
    Assert-ContainsText -Text $sampleText -ExpectedText ('config.scenario == "{0}"' -f $sample.Kind) `
        -Message "PDF regression sample runner should dispatch scenario '$($sample.Kind)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"kind": "{0}"' -f $sample.Kind) `
        -Message "PDF regression manifest should keep kind '$($sample.Kind)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText ('"output_file": "{0}"' -f $sample.Output) `
        -Message "PDF regression manifest should keep output file '$($sample.Output)'."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expected_pages": 1' `
        -Message "PDF text decoration samples should stay one-page lightweight samples."
    Assert-ContainsText -Text $manifestSampleBlock -ExpectedText '"expect_styled_text": true' `
        -Message "PDF text decoration samples should mark styled text coverage."

    foreach ($key in $sample.SourceKeys) {
        Assert-ContainsText -Text $sampleText -ExpectedText $key `
            -Message "PDF text decoration sample source should keep key '$key'."
    }

    foreach ($key in $sample.ManifestKeys) {
        Assert-ContainsText -Text $manifestSampleBlock -ExpectedText $key `
            -Message "PDF text decoration manifest should keep key '$key'."
    }

    Assert-ContainsText -Text $manifestTestText -ExpectedText $sample.Id `
        -Message "PDF regression manifest parser test should assert sample '$($sample.Id)'."
}

Assert-ContainsText -Text $sampleText -ExpectedText "formatting_flag::strikethrough" `
    -Message "PDF text decoration sample should exercise direct strikethrough formatting."
Assert-ContainsText -Text $sampleText -ExpectedText "formatting_flag::superscript" `
    -Message "PDF text decoration sample should exercise direct superscript formatting."
Assert-ContainsText -Text $sampleText -ExpectedText "formatting_flag::subscript" `
    -Message "PDF text decoration sample should exercise direct subscript formatting."
Assert-ContainsText -Text $sampleText -ExpectedText "run_strikethrough = true" `
    -Message "PDF text decoration sample should exercise inherited strikethrough style."
Assert-ContainsText -Text $sampleText -ExpectedText "run_superscript = true" `
    -Message "PDF text decoration sample should exercise inherited superscript style."
Assert-ContainsText -Text $sampleText -ExpectedText "run_subscript = true" `
    -Message "PDF text decoration sample should exercise inherited subscript style."
Assert-ContainsText -Text $cmakeText -ExpectedText "pdf_text_decoration_contract" `
    -Message "CMake should register the PDF text decoration static contract."

Write-Host "PDF text decoration contract passed."
