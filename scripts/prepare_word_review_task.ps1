param(
    [string]$DocxPath = "",
    [string]$FixedGridRegressionRoot = "",
    [string]$SectionPageSetupRegressionRoot = "",
    [string]$PageNumberFieldsRegressionRoot = "",
    [string]$VisualRegressionBundleRoot = "",
    [string]$VisualRegressionBundleLabel = "",
    [string]$VisualRegressionBundleKey = "",
    [string]$VisualRegressionRefreshCommand = "",
    [ValidateSet("review-only", "review-and-repair")]
    [string]$Mode = "review-only",
    [ValidateSet("undecided", "pending_manual_review", "pass", "fail", "undetermined")]
    [string]$ReviewVerdict = "undecided",
    [string]$ReviewNote = "",
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [switch]$OpenTaskDir
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "prepare_word_review_task_helpers.ps1")

$repoRoot = Resolve-RepoRoot
$hasDocxPath = -not [string]::IsNullOrWhiteSpace($DocxPath)
$hasFixedGridBundle = -not [string]::IsNullOrWhiteSpace($FixedGridRegressionRoot)
$hasSectionPageSetupBundle = -not [string]::IsNullOrWhiteSpace($SectionPageSetupRegressionRoot)
$hasPageNumberFieldsBundle = -not [string]::IsNullOrWhiteSpace($PageNumberFieldsRegressionRoot)
$hasVisualRegressionBundle = -not [string]::IsNullOrWhiteSpace($VisualRegressionBundleRoot)

$selectedSourceCount = (@(
        $hasDocxPath,
        $hasFixedGridBundle,
        $hasSectionPageSetupBundle,
        $hasPageNumberFieldsBundle,
        $hasVisualRegressionBundle
    ) |
    Where-Object { $_ } |
    Measure-Object).Count
if ($selectedSourceCount -ne 1) {
    throw "Specify exactly one of -DocxPath, -FixedGridRegressionRoot, -SectionPageSetupRegressionRoot, -PageNumberFieldsRegressionRoot, or -VisualRegressionBundleRoot."
}

$sourceKind = ""
$sourceKindPointerKey = ""
$sourceLabel = ""
$sourcePath = ""
$resolvedDocxPath = ""
$bundleInfo = $null
$documentVisualInfo = $null
$documentName = ""
$documentStem = ""

if ($hasDocxPath) {
    $resolvedDocxPath = Resolve-DocumentPath -RepoRoot $repoRoot -InputPath $DocxPath
    $documentName = [System.IO.Path]::GetFileName($resolvedDocxPath)
    $documentStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedDocxPath)
    $sourceKind = "document"
    $sourceKindPointerKey = $sourceKind
    $sourceLabel = "Target document"
    $sourcePath = $resolvedDocxPath
    $documentVisualInfo = Resolve-DocumentSourceVisualArtifacts -DocumentPath $resolvedDocxPath
} elseif ($hasFixedGridBundle) {
    $bundleInfo = Resolve-FixedGridRegressionBundle -RepoRoot $repoRoot `
        -InputPath $FixedGridRegressionRoot
    $documentName = [System.IO.Path]::GetFileName($bundleInfo.Root)
    $documentStem = $documentName
    $sourceKind = "fixed-grid-regression-bundle"
    $sourceKindPointerKey = $sourceKind
    $sourceLabel = "Fixed-grid regression bundle"
    $sourcePath = $bundleInfo.Root
} elseif ($hasSectionPageSetupBundle) {
    $bundleInfo = Resolve-SectionPageSetupRegressionBundle -RepoRoot $repoRoot `
        -InputPath $SectionPageSetupRegressionRoot
    $documentName = [System.IO.Path]::GetFileName($bundleInfo.Root)
    $documentStem = $documentName
    $sourceKind = "section-page-setup-regression-bundle"
    $sourceKindPointerKey = $sourceKind
    $sourceLabel = "Section page setup regression bundle"
    $sourcePath = $bundleInfo.Root
} elseif ($hasPageNumberFieldsBundle) {
    $bundleInfo = Resolve-PageNumberFieldsRegressionBundle -RepoRoot $repoRoot `
        -InputPath $PageNumberFieldsRegressionRoot
    $documentName = [System.IO.Path]::GetFileName($bundleInfo.Root)
    $documentStem = $documentName
    $sourceKind = "page-number-fields-regression-bundle"
    $sourceKindPointerKey = $sourceKind
    $sourceLabel = "Page number fields regression bundle"
    $sourcePath = $bundleInfo.Root
} else {
    $bundleInfo = Resolve-VisualRegressionBundle -RepoRoot $repoRoot `
        -InputPath $VisualRegressionBundleRoot
    $documentName = [System.IO.Path]::GetFileName($bundleInfo.Root)
    $documentStem = $documentName
    $sourceKind = "visual-regression-bundle"
    $sourceKindPointerKey = if ([string]::IsNullOrWhiteSpace($VisualRegressionBundleKey)) {
        $sourceKind
    } else {
        $VisualRegressionBundleKey
    }
    $resolvedBundleLabel = if ([string]::IsNullOrWhiteSpace($VisualRegressionBundleLabel)) {
        "Visual regression"
    } else {
        $VisualRegressionBundleLabel
    }
    $sourceLabel = "$resolvedBundleLabel bundle"
    $sourcePath = $bundleInfo.Root
}

$safeStem = Get-SafePathSegment -Name $documentStem
$taskRoot = if ([System.IO.Path]::IsPathRooted($TaskOutputRoot)) {
    [System.IO.Path]::GetFullPath($TaskOutputRoot)
} else {
    Join-Path $repoRoot $TaskOutputRoot
}
$taskLocation = New-UniqueTaskLocation -TaskRoot $taskRoot -SafeStem $safeStem
$taskId = $taskLocation.TaskId
$taskDir = $taskLocation.TaskDir
$taskRelativeDir = Get-RelativePathCompat -BasePath $repoRoot -TargetPath $taskDir
$evidenceDir = Join-Path $taskDir "evidence"
$reportDir = Join-Path $taskDir "report"
$repairDir = Join-Path $taskDir "repair"
$manifestPath = Join-Path $taskDir "task_manifest.json"
$promptPath = Join-Path $taskDir "task_prompt.md"
$reviewResultPath = Join-Path $reportDir "review_result.json"
$finalReviewPath = Join-Path $reportDir "final_review.md"
$templatePath = Get-TemplatePath -RepoRoot $repoRoot -Mode $Mode -SourceKind $sourceKind
$reviewResultTemplatePath = Get-ReportTemplatePath -RepoRoot $repoRoot `
    -FileName "review_result_template.json"
$finalReviewTemplatePath = Get-ReportTemplatePath -RepoRoot $repoRoot `
    -FileName "final_review_template.md"

New-Item -ItemType Directory -Path $taskDir -Force | Out-Null
New-Item -ItemType Directory -Path $evidenceDir -Force | Out-Null
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
New-Item -ItemType Directory -Path $repairDir -Force | Out-Null

$bundleLocalInfo = $null
$documentLocalInfo = $null
$bundleRefreshOutputRelative = ""
$bundleRepairOutputRelativeExample = ""
$bundleRefreshCommand = ""
$bundleRepairCommandExample = ""
$documentRefreshCommand = if ($resolvedDocxPath) {
    @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_word_visual_smoke.ps1`""
        "-InputDocx"
        "`"$resolvedDocxPath`""
        "-OutputDir"
        "`"$taskDir`""
    ) -join " "
} else {
    ""
}
$latestTaskPointerPaths = Get-LatestTaskPointerPaths -TaskRoot $taskRoot -SourceKind $sourceKindPointerKey

if ($sourceKind -eq "document") {
    $documentLocalInfo = Copy-DocumentSupportFiles -DocumentArtifacts $documentVisualInfo `
        -TaskDir $taskDir -EvidenceDir $evidenceDir -ReportDir $reportDir
} elseif ($sourceKind -eq "fixed-grid-regression-bundle") {
    $bundleLocalInfo = Copy-FixedGridBundleSupportFiles -BundleInfo $bundleInfo `
        -TaskDir $taskDir -EvidenceDir $evidenceDir -ReportDir $reportDir

    $bundleRefreshOutputRelative = [System.IO.Path]::Combine(
        $taskRelativeDir, "bundle-regression-refresh").Replace("/", "\")
    $bundleRepairOutputRelativeExample = [System.IO.Path]::Combine(
        $taskRelativeDir, "repair", "fix-01", "bundle-regression").Replace("/", "\")
    $bundleRefreshCommand = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_fixed_grid_merge_unmerge_regression.ps1`""
        "-OutputDir"
        "`"$bundleRefreshOutputRelative`""
    ) -join " "
    $bundleRepairCommandExample = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_fixed_grid_merge_unmerge_regression.ps1`""
        "-OutputDir"
        "`"$bundleRepairOutputRelativeExample`""
    ) -join " "
} elseif ($sourceKind -eq "section-page-setup-regression-bundle") {
    $bundleLocalInfo = Copy-SectionPageSetupBundleSupportFiles -BundleInfo $bundleInfo `
        -TaskDir $taskDir -EvidenceDir $evidenceDir -ReportDir $reportDir

    $bundleRefreshOutputRelative = [System.IO.Path]::Combine(
        $taskRelativeDir, "bundle-regression-refresh").Replace("/", "\")
    $bundleRepairOutputRelativeExample = [System.IO.Path]::Combine(
        $taskRelativeDir, "repair", "fix-01", "bundle-regression").Replace("/", "\")
    $bundleRefreshCommand = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_section_page_setup_visual_regression.ps1`""
        "-OutputDir"
        "`"$bundleRefreshOutputRelative`""
    ) -join " "
    $bundleRepairCommandExample = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_section_page_setup_visual_regression.ps1`""
        "-OutputDir"
        "`"$bundleRepairOutputRelativeExample`""
    ) -join " "
} elseif ($sourceKind -eq "page-number-fields-regression-bundle") {
    $bundleLocalInfo = Copy-PageNumberFieldsBundleSupportFiles -BundleInfo $bundleInfo `
        -TaskDir $taskDir -EvidenceDir $evidenceDir -ReportDir $reportDir

    $bundleRefreshOutputRelative = [System.IO.Path]::Combine(
        $taskRelativeDir, "bundle-regression-refresh").Replace("/", "\")
    $bundleRepairOutputRelativeExample = [System.IO.Path]::Combine(
        $taskRelativeDir, "repair", "fix-01", "bundle-regression").Replace("/", "\")
    $bundleRefreshCommand = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_page_number_fields_visual_regression.ps1`""
        "-OutputDir"
        "`"$bundleRefreshOutputRelative`""
    ) -join " "
    $bundleRepairCommandExample = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_page_number_fields_visual_regression.ps1`""
        "-OutputDir"
        "`"$bundleRepairOutputRelativeExample`""
    ) -join " "
} elseif ($sourceKind -eq "visual-regression-bundle") {
    $bundleLocalInfo = Copy-VisualRegressionBundleSupportFiles -BundleInfo $bundleInfo `
        -TaskDir $taskDir -EvidenceDir $evidenceDir -ReportDir $reportDir
    $bundleRefreshCommand = $VisualRegressionRefreshCommand
}

$templateValues = @{
    "{{DOCX_PATH}}" = $resolvedDocxPath
    "{{DOC_NAME}}" = $documentName
    "{{TASK_ID}}" = $taskId
    "{{TASK_DIR}}" = $taskDir
    "{{TASK_DIR_RELATIVE}}" = $taskRelativeDir
    "{{WORKSPACE}}" = $repoRoot
    "{{EVIDENCE_DIR}}" = $evidenceDir
    "{{REPORT_DIR}}" = $reportDir
    "{{REPAIR_DIR}}" = $repairDir
    "{{MODE}}" = $Mode
    "{{GENERATED_AT}}" = (Get-Date -Format "s")
    "{{SOURCE_KIND}}" = $sourceKind
    "{{SOURCE_LABEL}}" = $sourceLabel
    "{{SOURCE_PATH}}" = $sourcePath
    "{{VISUAL_BUNDLE_LABEL}}" = if ([string]::IsNullOrWhiteSpace($VisualRegressionBundleLabel)) {
        "Visual regression"
    } else {
        $VisualRegressionBundleLabel
    }
    "{{BUNDLE_ROOT}}" = if ($bundleInfo) { $bundleInfo.Root } else { "" }
    "{{TASK_BUNDLE_SUMMARY_PATH}}" = if ($bundleLocalInfo) {
        $bundleLocalInfo.SummaryPath
    } else {
        ""
    }
    "{{TASK_BUNDLE_REVIEW_MANIFEST_PATH}}" = if ($bundleLocalInfo) {
        $bundleLocalInfo.ReviewManifestPath
    } else {
        ""
    }
    "{{TASK_BUNDLE_AGGREGATE_CONTACT_SHEET}}" = if ($bundleLocalInfo) {
        $bundleLocalInfo.AggregateContactSheetPath
    } else {
        ""
    }
    "{{TASK_BUNDLE_AGGREGATE_EVIDENCE_DIR}}" = if ($bundleLocalInfo) {
        Get-OptionalMemberValue -Object $bundleLocalInfo -Name "AggregateEvidenceDir"
    } else {
        ""
    }
    "{{TASK_BUNDLE_FIRST_PAGES_DIR}}" = if ($sourceKind -eq "fixed-grid-regression-bundle") {
        $bundleLocalInfo.AggregateFirstPagesDir
    } else {
        ""
    }
    "{{TASK_BUNDLE_CONTACT_SHEETS_DIR}}" = if (
        $sourceKind -eq "section-page-setup-regression-bundle" -or
        $sourceKind -eq "page-number-fields-regression-bundle"
    ) {
        $bundleLocalInfo.AggregateContactSheetsDir
    } else {
        ""
    }
    "{{TASK_BUNDLE_SOURCE_CHECKLIST_PATH}}" = if ($bundleLocalInfo) {
        $bundleLocalInfo.SourceChecklistPath
    } else {
        ""
    }
    "{{TASK_BUNDLE_SOURCE_REVIEW_MANIFEST_PATH}}" = if ($bundleLocalInfo) {
        $bundleLocalInfo.ReviewManifestPath
    } else {
        ""
    }
    "{{BUNDLE_REFRESH_OUTPUT_RELATIVE}}" = $bundleRefreshOutputRelative
    "{{BUNDLE_REPAIR_OUTPUT_RELATIVE_EXAMPLE}}" = $bundleRepairOutputRelativeExample
    "{{BUNDLE_REFRESH_COMMAND}}" = $bundleRefreshCommand
    "{{BUNDLE_REPAIR_COMMAND_EXAMPLE}}" = $bundleRepairCommandExample
}

$promptContent = Expand-Template -TemplatePath $templatePath -Values $templateValues
$promptContent | Set-Content -Path $promptPath -Encoding UTF8

$jsonTemplateValues = ConvertTo-JsonTemplateValues -Values $templateValues
$seedReviewResult = Expand-Template -TemplatePath $reviewResultTemplatePath `
    -Values $jsonTemplateValues
$seedReviewResult | Set-Content -Path $reviewResultPath -Encoding UTF8

$seedFinalReview = Expand-Template -TemplatePath $finalReviewTemplatePath `
    -Values $templateValues
$seedFinalReview | Set-Content -Path $finalReviewPath -Encoding UTF8

if ($ReviewVerdict -ne "undecided" -or -not [string]::IsNullOrWhiteSpace($ReviewNote)) {
    $reviewedAt = (Get-Date).ToString("s")
    Update-SeedReviewResult -ReviewResultPath $reviewResultPath `
        -Verdict $ReviewVerdict `
        -ReviewNote $ReviewNote `
        -ReviewedAt $reviewedAt
    Update-SeedFinalReview -FinalReviewPath $finalReviewPath `
        -Verdict $ReviewVerdict `
        -ReviewNote $ReviewNote
}

$manifest = [ordered]@{
    task_id = $taskId
    mode = $Mode
    generated_at = (Get-Date).ToString("s")
    source = [ordered]@{
        kind = $sourceKind
        label = $sourceLabel
        path = $sourcePath
    }
    document = if ($resolvedDocxPath) {
        [ordered]@{
            name = $documentName
            path = $resolvedDocxPath
        }
    } else {
        $null
    }
    workspace = $repoRoot
    task_dir = $taskDir
    prompt_path = $promptPath
    evidence_dir = $evidenceDir
    report_dir = $reportDir
    repair_dir = $repairDir
    review_result_path = $reviewResultPath
    final_review_path = $finalReviewPath
    latest_task_pointers = [ordered]@{
        generic_path = $latestTaskPointerPaths.GenericPath
        source_kind_path = $latestTaskPointerPaths.SourceKindPath
    }
}

if ($bundleInfo) {
    $bundleManifest = [ordered]@{
        root = $bundleInfo.Root
        source_summary_path = $bundleInfo.SummaryPath
        source_review_manifest_path = $bundleInfo.ReviewManifestPath
        source_review_checklist_path = $bundleInfo.ReviewChecklistPath
        source_final_review_path = $bundleInfo.FinalReviewPath
        source_aggregate_contact_sheet = $bundleInfo.AggregateContactSheetPath
        copied_summary_path = $bundleLocalInfo.SummaryPath
        copied_review_manifest_path = $bundleLocalInfo.ReviewManifestPath
        copied_aggregate_contact_sheet = $bundleLocalInfo.AggregateContactSheetPath
        copied_source_review_checklist_path = $bundleLocalInfo.SourceChecklistPath
        copied_source_final_review_path = $bundleLocalInfo.SourceFinalReviewPath
        refresh_command = $bundleRefreshCommand
        repair_command_example = $bundleRepairCommandExample
    }

    if ($sourceKind -eq "fixed-grid-regression-bundle") {
        $bundleManifest.source_aggregate_first_pages_dir = $bundleInfo.AggregateFirstPagesDir
        $bundleManifest.copied_aggregate_first_pages_dir = $bundleLocalInfo.AggregateFirstPagesDir
        $manifest.fixed_grid_bundle = $bundleManifest
    } elseif ($sourceKind -eq "section-page-setup-regression-bundle") {
        $bundleManifest.source_aggregate_contact_sheets_dir = $bundleInfo.AggregateContactSheetsDir
        $bundleManifest.copied_aggregate_contact_sheets_dir = $bundleLocalInfo.AggregateContactSheetsDir
        $manifest.section_page_setup_bundle = $bundleManifest
    } elseif ($sourceKind -eq "page-number-fields-regression-bundle") {
        $bundleManifest.source_aggregate_contact_sheets_dir = $bundleInfo.AggregateContactSheetsDir
        $bundleManifest.copied_aggregate_contact_sheets_dir = $bundleLocalInfo.AggregateContactSheetsDir
        $manifest.page_number_fields_bundle = $bundleManifest
    } elseif ($sourceKind -eq "visual-regression-bundle") {
        $bundleManifest.label = if ([string]::IsNullOrWhiteSpace($VisualRegressionBundleLabel)) {
            "Visual regression"
        } else {
            $VisualRegressionBundleLabel
        }
        $bundleManifest.source_aggregate_evidence_dir = $bundleInfo.AggregateEvidenceDir
        $bundleManifest.copied_aggregate_evidence_dir = $bundleLocalInfo.AggregateEvidenceDir
        $manifest.visual_regression_bundle = $bundleManifest
    }
}

if ($sourceKind -eq "document" -and $documentVisualInfo) {
    $manifest.document_visual_artifacts = [ordered]@{
        source_root = $documentVisualInfo.Root
        source_pdf_path = $documentVisualInfo.PdfPath
        source_evidence_dir = $documentVisualInfo.EvidenceDir
        source_pages_dir = $documentVisualInfo.PagesDir
        source_contact_sheet_path = $documentVisualInfo.ContactSheetPath
        source_report_dir = $documentVisualInfo.ReportDir
        source_summary_path = $documentVisualInfo.SummaryPath
        source_review_checklist_path = $documentVisualInfo.ReviewChecklistPath
        source_review_result_path = $documentVisualInfo.ReviewResultPath
        source_final_review_path = $documentVisualInfo.FinalReviewPath
        copied_pdf_path = if ($documentLocalInfo) { $documentLocalInfo.PdfPath } else { "" }
        copied_contact_sheet_path = if ($documentLocalInfo) { $documentLocalInfo.ContactSheetPath } else { "" }
        copied_pages_dir = if ($documentLocalInfo) { $documentLocalInfo.PagesDir } else { "" }
        copied_summary_path = if ($documentLocalInfo) { $documentLocalInfo.SummaryPath } else { "" }
        copied_review_checklist_path = if ($documentLocalInfo) { $documentLocalInfo.ReviewChecklistPath } else { "" }
        copied_source_review_result_path = if ($documentLocalInfo) { $documentLocalInfo.SourceReviewResultPath } else { "" }
        copied_source_final_review_path = if ($documentLocalInfo) { $documentLocalInfo.SourceFinalReviewPath } else { "" }
        refresh_command = $documentRefreshCommand
    }
}

$manifest.recommended_next_steps = if ($bundleInfo) {
    if ($sourceKind -eq "section-page-setup-regression-bundle") {
        @(
            "Open task_prompt.md and send the full contents to the AI agent.",
            "Tell the AI to inspect evidence\aggregate_contact_sheet.png first, then use bundle_review_manifest.json to review the api-sample and cli-rewrite cases.",
            "If any bundle artifact is missing or stale, tell the AI to rerun: $bundleRefreshCommand",
            "If the mode is review-and-repair, allow iterative fixes and reruns under repair\fix-XX\bundle-regression."
        )
    } elseif ($sourceKind -eq "page-number-fields-regression-bundle") {
        @(
            "Open task_prompt.md and send the full contents to the AI agent.",
            "Tell the AI to inspect evidence\aggregate_contact_sheet.png first, then use bundle_review_manifest.json and each case field_summary.json to review the api-sample and cli-append cases.",
            "If any bundle artifact is missing or stale, tell the AI to rerun: $bundleRefreshCommand",
            "If the mode is review-and-repair, allow iterative fixes and reruns under repair\fix-XX\bundle-regression."
        )
    } elseif ($sourceKind -eq "visual-regression-bundle") {
        @(
            "Open task_prompt.md and send the full contents to the AI agent.",
            "Tell the AI to inspect evidence\aggregate-evidence\ first, then use bundle_summary.json cases[*].expected_visual_cues and visual_artifacts to review each case.",
            "If any bundle artifact is missing or stale, tell the AI to rerun: $bundleRefreshCommand",
            "If the mode is review-and-repair, allow iterative fixes and reruns under repair\fix-XX\bundle-regression."
        )
    } else {
        @(
            "Open task_prompt.md and send the full contents to the AI agent.",
            "Tell the AI to inspect evidence\aggregate_contact_sheet.png first, then use bundle_review_manifest.json to review each fixed-grid case.",
            "If any bundle artifact is missing or stale, tell the AI to rerun: $bundleRefreshCommand",
            "If the mode is review-and-repair, allow iterative fixes and reruns under repair\fix-XX\bundle-regression."
        )
    }
} else {
    if ($documentLocalInfo -and
        -not [string]::IsNullOrWhiteSpace($documentLocalInfo.ContactSheetPath)) {
        @(
            "Open task_prompt.md and send the full contents to the AI agent.",
            "Tell the AI to inspect the packaged evidence under evidence\ and report\ first.",
            "If the packaged smoke artifacts are missing or stale, tell the AI to rerun: $documentRefreshCommand",
            "If the mode is review-and-repair, allow iterative fixes and full regressions under the repair directory."
        )
    } else {
        @(
            "Open task_prompt.md and send the full contents to the AI agent.",
            "Tell the AI to first run scripts\run_word_visual_smoke.ps1 with -InputDocx and -OutputDir pointing at this task directory.",
            "Tell the AI to review the generated PDF/PNG evidence and write findings into the report directory.",
            "If the mode is review-and-repair, allow iterative fixes and full regressions under the repair directory."
        )
    }
}

($manifest | ConvertTo-Json -Depth 6) | Set-Content -Path $manifestPath -Encoding UTF8

$latestTaskInfo = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    task_root = $taskRoot
    task_id = $taskId
    mode = $Mode
    source = $manifest.source
    document = $manifest.document
    task_dir = $taskDir
    manifest_path = $manifestPath
    prompt_path = $promptPath
    evidence_dir = $evidenceDir
    report_dir = $reportDir
    repair_dir = $repairDir
    review_result_path = $reviewResultPath
    final_review_path = $finalReviewPath
}
Write-LatestTaskPointers -RepoRoot $repoRoot `
    -PointerPaths $latestTaskPointerPaths `
    -TaskInfo $latestTaskInfo

Write-Host "Task id: $taskId"
Write-Host "Source kind: $sourceKind"
Write-Host "Source: $sourcePath"
if ($resolvedDocxPath) {
    Write-Host "Document: $resolvedDocxPath"
}
Write-Host "Mode: $Mode"
Write-Host "Task directory: $taskDir"
Write-Host "Prompt: $promptPath"
Write-Host "Manifest: $manifestPath"
Write-Host "Evidence: $evidenceDir"
Write-Host "Report: $reportDir"
Write-Host "Review result: $reviewResultPath"
Write-Host "Final review: $finalReviewPath"
Write-Host "Repair: $repairDir"
if ($bundleInfo) {
    if (-not [string]::IsNullOrWhiteSpace($bundleLocalInfo.ReviewManifestPath)) {
        Write-Host "Bundle review manifest copy: $($bundleLocalInfo.ReviewManifestPath)"
    }
    $bundleAggregateEvidenceDir = Get-OptionalMemberValue -Object $bundleLocalInfo -Name "AggregateEvidenceDir"
    if (-not [string]::IsNullOrWhiteSpace($bundleAggregateEvidenceDir)) {
        Write-Host "Bundle aggregate evidence copy: $bundleAggregateEvidenceDir"
    }
    Write-Host "Bundle aggregate contact sheet copy: $($bundleLocalInfo.AggregateContactSheetPath)"
} elseif ($documentLocalInfo) {
    if ($documentLocalInfo.SummaryPath) {
        Write-Host "Source summary copy: $($documentLocalInfo.SummaryPath)"
    }
    if ($documentLocalInfo.ContactSheetPath) {
        Write-Host "Source contact sheet copy: $($documentLocalInfo.ContactSheetPath)"
    }
}
Write-Host "Latest task pointer: $($latestTaskPointerPaths.GenericPath)"
Write-Host "Latest source-kind task pointer: $($latestTaskPointerPaths.SourceKindPath)"

if ($OpenTaskDir) {
    Start-Process explorer.exe $taskDir | Out-Null
}
