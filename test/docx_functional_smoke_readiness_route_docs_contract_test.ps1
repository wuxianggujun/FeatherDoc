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

$readme = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.md"
$indexDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$docxReadinessDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\docx_functional_smoke_readiness_zh.rst"
$releaseMetadataDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\release_metadata_pipeline_zh.rst"
$projectChecklistDoc = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\project_template_release_readiness_checklist_zh.rst"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

$docxReadinessScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\check_docx_functional_smoke_readiness.ps1"
$releasePipelineScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_pipeline_report.ps1"
$releaseHandoffScript = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "scripts\build_release_governance_handoff_report.ps1"

$docxReadinessRegression = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\docx_functional_smoke_readiness_test.ps1"
$releasePipelineRegression = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_pipeline_report_test.ps1"
$releaseHandoffRegression = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\build_release_governance_handoff_report_test.ps1"

foreach ($marker in @(
        "scripts/build_release_governance_pipeline_report.ps1",
        "docx_functional_smoke_readiness",
        "scripts/check_docx_functional_smoke_readiness.ps1",
        "docx-functional-smoke-readiness/summary.json",
        "featherdoc.docx_functional_smoke_readiness.v1",
        "docx_functional_smoke_readiness.md",
        "read-only",
        "Word"
    )) {
    Assert-ContainsText -Text $readme -ExpectedText $marker `
        -Message "README should preserve DOCX functional smoke readiness route marker '$marker'."
}

foreach ($marker in @(
        "docx_functional_smoke_readiness_zh",
        "scripts/check_docx_functional_smoke_readiness.ps1",
        "docx_functional_smoke_readiness",
        "docx-functional-smoke-readiness/summary.json",
        "featherdoc.docx_functional_smoke_readiness.v1",
        "docx_functional_smoke_readiness.md",
        "word_visual_smoke.pending_manual_review",
        "persisted_docx_functional_smoke_evidence_only",
        "summary_json_display",
        "report_markdown_display"
    )) {
    Assert-ContainsText -Text $indexDoc -ExpectedText $marker `
        -Message "Sphinx index should preserve DOCX functional smoke readiness route marker '$marker'."
}

foreach ($marker in @(
        "scripts/check_docx_functional_smoke_readiness.ps1",
        "featherdoc.docx_functional_smoke_readiness.v1",
        "docx_functional_smoke_readiness_trace",
        "word_visual_smoke.pending_manual_review",
        "restore_docx_functional_smoke_evidence",
        "-OutputDir",
        "-SummaryJson",
        "-ReportMarkdown",
        "-VisualSmokeRoots",
        "-FailOnWarning",
        "docx_functional_smoke_readiness.md",
        "output/docx-functional-smoke-readiness/summary.json",
        "output/docx-functional-smoke-readiness/docx_functional_smoke_readiness.md",
        "output/docx-functional-smoke-readiness-current/summary.json",
        "output/docx-functional-smoke-readiness-current/docx_functional_smoke_readiness.md"
    )) {
    Assert-ContainsText -Text $docxReadinessDoc -ExpectedText $marker `
        -Message "Dedicated DOCX readiness docs should preserve marker '$marker'."
}

foreach ($marker in @(
        "docx_functional_smoke_readiness",
        "featherdoc.docx_functional_smoke_readiness.v1",
        "docx_functional_smoke_readiness_trace",
        "persisted_docx_functional_smoke_evidence_only",
        "word_visual_smoke.pending_manual_review",
        "restore_docx_functional_smoke_evidence",
        "summary_json_display",
        "report_markdown_display",
        "output/docx-functional-smoke-readiness/summary.json",
        "output/docx-functional-smoke-readiness/docx_functional_smoke_readiness.md",
        "output/docx-functional-smoke-readiness-current/summary.json",
        "output/docx-functional-smoke-readiness-current/docx_functional_smoke_readiness.md"
    )) {
    Assert-ContainsText -Text $releaseMetadataDoc -ExpectedText $marker `
        -Message "Release metadata pipeline docs should preserve DOCX readiness marker '$marker'."
    Assert-ContainsText -Text $projectChecklistDoc -ExpectedText $marker `
        -Message "Project template checklist should preserve DOCX readiness marker '$marker'."
}

foreach ($assertion in @(
        [ordered]@{
            text = $docxReadinessScript
            label = "DOCX functional smoke readiness script"
            markers = @(
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
                "docx_functional_smoke_readiness.md",
                "release_blocker_count",
                "restore_docx_functional_smoke_evidence",
                "report_markdown_display"
            )
        },
        [ordered]@{
            text = $releasePipelineScript
            label = "release governance pipeline"
            markers = @(
                "docx_functional_smoke_readiness",
                "DOCX Functional Smoke Readiness",
                "check_docx_functional_smoke_readiness.ps1",
                "docx-functional-smoke-readiness",
                "docx-functional-smoke-visual",
                "-VisualSmokeRoots"
            )
        },
        [ordered]@{
            text = $releaseHandoffScript
            label = "release governance handoff"
            markers = @(
                "featherdoc.docx_functional_smoke_readiness.v1",
                "docx_functional_smoke_readiness",
                "docx-functional-smoke-readiness/summary.json",
                "check_docx_functional_smoke_readiness.ps1"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($assertion in @(
        [ordered]@{
            text = $docxReadinessRegression
            label = "DOCX functional smoke readiness regression"
            markers = @(
                "featherdoc.docx_functional_smoke_readiness.v1",
                "persisted_docx_functional_smoke_evidence_only",
                "docx_functional_smoke_readiness_trace",
                "word_visual_smoke.pending_manual_review",
                "docx_functional_smoke_readiness.md",
                "visual_smoke_reused_evidence",
                "report_markdown_display"
            )
        },
        [ordered]@{
            text = $releasePipelineRegression
            label = "release governance pipeline regression"
            markers = @(
                "docx_functional_smoke_readiness",
                "docx-functional-smoke-readiness\summary.json",
                "featherdoc.docx_functional_smoke_readiness.v1",
                "word_visual_smoke.pending_manual_review"
            )
        },
        [ordered]@{
            text = $releaseHandoffRegression
            label = "release governance handoff regression"
            markers = @(
                "docx-functional-smoke-readiness\summary.json",
                "featherdoc.docx_functional_smoke_readiness.v1"
            )
        }
    )) {
    foreach ($marker in @($assertion.markers)) {
        Assert-ContainsText -Text ([string]$assertion.text) -ExpectedText $marker `
            -Message "$($assertion.label) should preserve marker '$marker'."
    }
}

foreach ($marker in @(
        "docx_functional_smoke_readiness",
        "docx_functional_smoke_readiness_test.ps1",
        "docx_functional_smoke_readiness_fail_on_warning",
        "fail_on_warning",
        "docx_functional_smoke_readiness_route_docs_contract",
        "docx_functional_smoke_readiness_route_docs_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;governance;docx"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake test registration should keep DOCX readiness route contract wired."
}

Write-Host "DOCX functional smoke readiness route docs contract passed."
