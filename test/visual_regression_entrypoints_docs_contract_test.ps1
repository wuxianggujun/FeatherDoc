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
        throw "$Message Missing='$ExpectedText'."
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$visualValidationDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\visual_validation_zh.rst"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"
$basicTests = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\basic_tests.cpp"
$cliTests = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\cli_tests.cpp"

$visualEntrypoints = @(
    [ordered]@{
        name = "OMML visual regression"
        script = "scripts\run_omml_visual_regression.ps1"
        sample = "samples\sample_omml_visual.cpp"
        target = "featherdoc_sample_omml_visual"
        docs = @($visualValidationDoc)
        docMarker = "scripts/run_omml_visual_regression.ps1"
        scriptMarkers = @(
            "BuildDir",
            "OutputDir",
            "SkipBuild",
            "SkipVisual",
            "ReviewVerdict",
            "Assert-GeneratedDocxEvidence",
            "run_word_visual_smoke.ps1",
            "m:oMathPara",
            "m:oMath",
            "Completed OMML visual regression"
        )
        sampleMarkers = @(
            "append_omml",
            "replace_omml",
            "remove_omml",
            "make_omml_nary",
            "make_omml_delimiter"
        )
        testText = $basicTests
        testMarkers = @(
            "document and template part can inspect append replace and remove OMML",
            "OMML builder helpers create appendable formulas"
        )
    },
    [ordered]@{
        name = "semantic diff visual regression"
        script = "scripts\run_semantic_diff_visual_regression.ps1"
        sample = "samples\sample_semantic_diff_visual.cpp"
        target = "featherdoc_sample_semantic_diff_visual"
        docs = @($visualValidationDoc)
        docMarker = "scripts/run_semantic_diff_visual_regression.ps1"
        scriptMarkers = @(
            "BuildDir",
            "OutputDir",
            "SkipBuild",
            "SkipVisual",
            "ReviewVerdict",
            "Assert-DiffJsonEvidence",
            "semantic-diff",
            "run_word_visual_smoke.ps1",
            "style_changes",
            "numbering_changes",
            "template_part_results",
            "Completed semantic diff visual regression"
        )
        sampleMarkers = @(
            "Semantic diff visual - BEFORE",
            "Semantic diff visual - AFTER",
            "Header BEFORE",
            "Header AFTER",
            "SemanticDiffApprovedOutline"
        )
        testText = $cliTests
        testMarkers = @(
            "cli semantic-diff reports document changes and can fail on diff",
            "cli semantic-diff reports header and footer template part changes"
        )
    },
    [ordered]@{
        name = "generic fields visual regression"
        script = "scripts\run_generic_fields_visual_regression.ps1"
        sample = "samples\sample_generic_fields_visual.cpp"
        target = "featherdoc_sample_generic_fields_visual"
        docs = @($visualValidationDoc)
        docMarker = "scripts/run_generic_fields_visual_regression.ps1"
        scriptMarkers = @(
            "BuildDir",
            "OutputDir",
            "SkipBuild",
            "SkipVisual",
            "ReviewVerdict",
            "Assert-GeneratedDocxEvidence",
            "Assert-SemanticDiffFieldEvidence",
            "semantic-diff",
            "run_word_visual_smoke.ps1",
            "w:updateFields",
            "field_changes",
            "Completed generic field visual regression"
        )
        sampleMarkers = @(
            "append_table_of_contents_field",
            "append_reference_field",
            "append_page_reference_field",
            "append_sequence_field",
            "enable_update_fields_on_open"
        )
        testText = $basicTests + "`n" + $cliTests
        testMarkers = @(
            "template part generic fields validate options and indexes",
            "cli semantic-diff reports TOC REF and SEQ field changes"
        )
    },
    [ordered]@{
        name = "page number fields visual regression"
        script = "scripts\run_page_number_fields_visual_regression.ps1"
        sample = "samples\sample_page_number_fields.cpp"
        target = "featherdoc_sample_page_number_fields"
        docs = @($visualValidationDoc)
        docMarker = ".\scripts\run_page_number_fields_visual_regression.ps1"
        scriptMarkers = @(
            "BuildDir",
            "OutputDir",
            "SkipBuild",
            "SkipVisual",
            "Find-BuildExecutable",
            "run_word_visual_smoke.ps1",
            "Completed page number fields regression run"
        )
        sampleMarkers = @(
            "append_page_number_field",
            "append_total_pages_field",
            "Current page",
            "Total pages"
        )
        testText = $basicTests
        testMarkers = @(
            "header and footer template parts can append page number fields",
            "template part page number fields report unavailable parts"
        )
    }
)

foreach ($entrypoint in $visualEntrypoints) {
    $scriptText = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath $entrypoint.script
    $sampleText = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath $entrypoint.sample

    foreach ($docText in @($entrypoint.docs)) {
        Assert-ContainsText -Text $docText -ExpectedText ([string]$entrypoint.docMarker) `
            -Message "$($entrypoint.name) should remain referenced by its documentation."
    }

    foreach ($marker in @($entrypoint.scriptMarkers)) {
        Assert-ContainsText -Text $scriptText -ExpectedText $marker `
            -Message "$($entrypoint.name) script should preserve marker '$marker'."
    }

    foreach ($marker in @($entrypoint.sampleMarkers)) {
        Assert-ContainsText -Text $sampleText -ExpectedText $marker `
            -Message "$($entrypoint.name) sample should preserve marker '$marker'."
    }

    foreach ($marker in @($entrypoint.testMarkers)) {
        Assert-ContainsText -Text $entrypoint.testText -ExpectedText $marker `
            -Message "$($entrypoint.name) should keep lightweight API/CLI coverage marker '$marker'."
    }

    Assert-ContainsText -Text $scriptText -ExpectedText $entrypoint.target `
        -Message "$($entrypoint.name) script should preserve its sample target '$($entrypoint.target)'."
}

foreach ($marker in @(
        "visual_regression_entrypoints_docs_contract",
        "visual_regression_entrypoints_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;visual"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep visual regression entrypoint docs contract wired."
}

Write-Host "Visual regression entrypoints docs contract passed."
