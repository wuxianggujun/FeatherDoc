param(
    [string]$DocxPath = "",
    [string]$FixedGridRegressionRoot = "",
    [string]$SectionPageSetupRegressionRoot = "",
    [string]$PageNumberFieldsRegressionRoot = "",
    [ValidateSet("review-only", "review-and-repair")]
    [string]$Mode = "review-only",
    [string]$TaskOutputRoot = "output/word-visual-smoke/tasks",
    [switch]$OpenTaskDir
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-ExistingPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    if (-not (Test-Path $candidate)) {
        throw "Target path does not exist: $candidate"
    }

    return (Resolve-Path $candidate).Path
}

function Resolve-DocumentPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $resolved = Resolve-ExistingPath -RepoRoot $RepoRoot -InputPath $InputPath
    $extension = [System.IO.Path]::GetExtension($resolved).ToLowerInvariant()
    if ($extension -notin @(".doc", ".docx")) {
        throw "Only .doc and .docx files are supported: $resolved"
    }

    return $resolved
}

function Resolve-FixedGridRegressionBundle {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $resolvedRoot = Resolve-ExistingPath -RepoRoot $RepoRoot -InputPath $InputPath
    if (-not (Test-Path $resolvedRoot -PathType Container)) {
        throw "Fixed-grid regression root must be a directory: $resolvedRoot"
    }

    $summaryPath = Join-Path $resolvedRoot "summary.json"
    $reviewManifestPath = Join-Path $resolvedRoot "review_manifest.json"
    $reviewChecklistPath = Join-Path $resolvedRoot "review_checklist.md"
    $finalReviewPath = Join-Path $resolvedRoot "final_review.md"
    $aggregateEvidenceDir = Join-Path $resolvedRoot "aggregate-evidence"
    $aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"
    $aggregateFirstPagesDir = Join-Path $aggregateEvidenceDir "first-pages"

    $requiredPaths = @(
        $summaryPath,
        $reviewManifestPath,
        $aggregateContactSheetPath,
        $aggregateFirstPagesDir
    )

    foreach ($requiredPath in $requiredPaths) {
        if (-not (Test-Path $requiredPath)) {
            throw "Fixed-grid regression bundle is incomplete, missing: $requiredPath"
        }
    }

    return [ordered]@{
        Root = $resolvedRoot
        SummaryPath = (Resolve-Path $summaryPath).Path
        ReviewManifestPath = (Resolve-Path $reviewManifestPath).Path
        ReviewChecklistPath = if (Test-Path $reviewChecklistPath) {
            (Resolve-Path $reviewChecklistPath).Path
        } else {
            ""
        }
        FinalReviewPath = if (Test-Path $finalReviewPath) {
            (Resolve-Path $finalReviewPath).Path
        } else {
            ""
        }
        AggregateEvidenceDir = (Resolve-Path $aggregateEvidenceDir).Path
        AggregateContactSheetPath = (Resolve-Path $aggregateContactSheetPath).Path
        AggregateFirstPagesDir = (Resolve-Path $aggregateFirstPagesDir).Path
    }
}

function Resolve-SectionPageSetupRegressionBundle {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $resolvedRoot = Resolve-ExistingPath -RepoRoot $RepoRoot -InputPath $InputPath
    if (-not (Test-Path $resolvedRoot -PathType Container)) {
        throw "Section page setup regression root must be a directory: $resolvedRoot"
    }

    $summaryPath = Join-Path $resolvedRoot "summary.json"
    $reviewManifestPath = Join-Path $resolvedRoot "review_manifest.json"
    $reviewChecklistPath = Join-Path $resolvedRoot "review_checklist.md"
    $finalReviewPath = Join-Path $resolvedRoot "final_review.md"
    $aggregateEvidenceDir = Join-Path $resolvedRoot "aggregate-evidence"
    $aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"
    $aggregateContactSheetsDir = Join-Path $aggregateEvidenceDir "contact-sheets"

    $requiredPaths = @(
        $summaryPath,
        $reviewManifestPath,
        $aggregateContactSheetPath,
        $aggregateContactSheetsDir
    )

    foreach ($requiredPath in $requiredPaths) {
        if (-not (Test-Path $requiredPath)) {
            throw "Section page setup regression bundle is incomplete, missing: $requiredPath"
        }
    }

    return [ordered]@{
        Root = $resolvedRoot
        SummaryPath = (Resolve-Path $summaryPath).Path
        ReviewManifestPath = (Resolve-Path $reviewManifestPath).Path
        ReviewChecklistPath = if (Test-Path $reviewChecklistPath) {
            (Resolve-Path $reviewChecklistPath).Path
        } else {
            ""
        }
        FinalReviewPath = if (Test-Path $finalReviewPath) {
            (Resolve-Path $finalReviewPath).Path
        } else {
            ""
        }
        AggregateEvidenceDir = (Resolve-Path $aggregateEvidenceDir).Path
        AggregateContactSheetPath = (Resolve-Path $aggregateContactSheetPath).Path
        AggregateContactSheetsDir = (Resolve-Path $aggregateContactSheetsDir).Path
    }
}

function Resolve-PageNumberFieldsRegressionBundle {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $resolvedRoot = Resolve-ExistingPath -RepoRoot $RepoRoot -InputPath $InputPath
    if (-not (Test-Path $resolvedRoot -PathType Container)) {
        throw "Page number fields regression root must be a directory: $resolvedRoot"
    }

    $summaryPath = Join-Path $resolvedRoot "summary.json"
    $reviewManifestPath = Join-Path $resolvedRoot "review_manifest.json"
    $reviewChecklistPath = Join-Path $resolvedRoot "review_checklist.md"
    $finalReviewPath = Join-Path $resolvedRoot "final_review.md"
    $aggregateEvidenceDir = Join-Path $resolvedRoot "aggregate-evidence"
    $aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "contact_sheet.png"
    $aggregateContactSheetsDir = Join-Path $aggregateEvidenceDir "contact-sheets"

    $requiredPaths = @(
        $summaryPath,
        $reviewManifestPath,
        $aggregateContactSheetPath,
        $aggregateContactSheetsDir
    )

    foreach ($requiredPath in $requiredPaths) {
        if (-not (Test-Path $requiredPath)) {
            throw "Page number fields regression bundle is incomplete, missing: $requiredPath"
        }
    }

    return [ordered]@{
        Root = $resolvedRoot
        SummaryPath = (Resolve-Path $summaryPath).Path
        ReviewManifestPath = (Resolve-Path $reviewManifestPath).Path
        ReviewChecklistPath = if (Test-Path $reviewChecklistPath) {
            (Resolve-Path $reviewChecklistPath).Path
        } else {
            ""
        }
        FinalReviewPath = if (Test-Path $finalReviewPath) {
            (Resolve-Path $finalReviewPath).Path
        } else {
            ""
        }
        AggregateEvidenceDir = (Resolve-Path $aggregateEvidenceDir).Path
        AggregateContactSheetPath = (Resolve-Path $aggregateContactSheetPath).Path
        AggregateContactSheetsDir = (Resolve-Path $aggregateContactSheetsDir).Path
    }
}

function Get-SafePathSegment {
    param([string]$Name)

    $safe = [System.Text.RegularExpressions.Regex]::Replace($Name, "[^A-Za-z0-9._-]+", "-")
    $safe = $safe.Trim("-")
    if ([string]::IsNullOrWhiteSpace($safe)) {
        return "document"
    }

    return $safe
}

function Get-LatestTaskPointerPaths {
    param(
        [string]$TaskRoot,
        [string]$SourceKind
    )

    $sourceKindSegment = Get-SafePathSegment -Name $SourceKind
    return [ordered]@{
        GenericPath = Join-Path $TaskRoot "latest_task.json"
        SourceKindPath = Join-Path $TaskRoot "latest_$($sourceKindSegment)_task.json"
    }
}

function Get-RelativePathCompat {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    $resolvedBase = [System.IO.Path]::GetFullPath($BasePath)
    $resolvedTarget = [System.IO.Path]::GetFullPath($TargetPath)

    if ([System.IO.Path]::GetPathRoot($resolvedBase) -ne
        [System.IO.Path]::GetPathRoot($resolvedTarget)) {
        return $resolvedTarget
    }

    if ((Test-Path $resolvedBase -PathType Container) -and
        -not $resolvedBase.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedBase.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedBase += [System.IO.Path]::DirectorySeparatorChar
    }

    if ((Test-Path $resolvedTarget -PathType Container) -and
        -not $resolvedTarget.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedTarget.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedTarget += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = New-Object System.Uri($resolvedBase)
    $targetUri = New-Object System.Uri($resolvedTarget)
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)

    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace("/", "\")
}

function Resolve-DocumentSourceVisualArtifacts {
    param([string]$DocumentPath)

    $documentDir = Split-Path -Parent $DocumentPath
    $documentStem = [System.IO.Path]::GetFileNameWithoutExtension($DocumentPath)
    $pdfPath = Join-Path $documentDir "$documentStem.pdf"
    $evidenceDir = Join-Path $documentDir "evidence"
    $pagesDir = Join-Path $evidenceDir "pages"
    $contactSheetPath = Join-Path $evidenceDir "contact_sheet.png"
    $reportDir = Join-Path $documentDir "report"
    $summaryPath = Join-Path $reportDir "summary.json"
    $reviewChecklistPath = Join-Path $reportDir "review_checklist.md"
    $reviewResultPath = Join-Path $reportDir "review_result.json"
    $finalReviewPath = Join-Path $reportDir "final_review.md"

    $availablePaths = @(
        $pdfPath,
        $contactSheetPath,
        $pagesDir,
        $summaryPath,
        $reviewChecklistPath,
        $reviewResultPath,
        $finalReviewPath
    ) | Where-Object { Test-Path -LiteralPath $_ }

    if ($availablePaths.Count -eq 0) {
        return $null
    }

    return [ordered]@{
        Root = $documentDir
        PdfPath = if (Test-Path -LiteralPath $pdfPath) {
            (Resolve-Path $pdfPath).Path
        } else {
            ""
        }
        EvidenceDir = if (Test-Path -LiteralPath $evidenceDir -PathType Container) {
            (Resolve-Path $evidenceDir).Path
        } else {
            ""
        }
        PagesDir = if (Test-Path -LiteralPath $pagesDir -PathType Container) {
            (Resolve-Path $pagesDir).Path
        } else {
            ""
        }
        ContactSheetPath = if (Test-Path -LiteralPath $contactSheetPath) {
            (Resolve-Path $contactSheetPath).Path
        } else {
            ""
        }
        ReportDir = if (Test-Path -LiteralPath $reportDir -PathType Container) {
            (Resolve-Path $reportDir).Path
        } else {
            ""
        }
        SummaryPath = if (Test-Path -LiteralPath $summaryPath) {
            (Resolve-Path $summaryPath).Path
        } else {
            ""
        }
        ReviewChecklistPath = if (Test-Path -LiteralPath $reviewChecklistPath) {
            (Resolve-Path $reviewChecklistPath).Path
        } else {
            ""
        }
        ReviewResultPath = if (Test-Path -LiteralPath $reviewResultPath) {
            (Resolve-Path $reviewResultPath).Path
        } else {
            ""
        }
        FinalReviewPath = if (Test-Path -LiteralPath $finalReviewPath) {
            (Resolve-Path $finalReviewPath).Path
        } else {
            ""
        }
    }
}

function Get-TemplatePath {
    param(
        [string]$RepoRoot,
        [string]$Mode,
        [string]$SourceKind
    )

    $fileName = switch ("$SourceKind|$Mode") {
        "document|review-only" { "task_prompt_review_only_template.md" }
        "document|review-and-repair" { "task_prompt_review_and_repair_template.md" }
        "fixed-grid-regression-bundle|review-only" {
            "task_prompt_fixed_grid_review_only_template.md"
        }
        "fixed-grid-regression-bundle|review-and-repair" {
            "task_prompt_fixed_grid_review_and_repair_template.md"
        }
        "section-page-setup-regression-bundle|review-only" {
            "task_prompt_section_page_setup_review_only_template.md"
        }
        "section-page-setup-regression-bundle|review-and-repair" {
            "task_prompt_section_page_setup_review_and_repair_template.md"
        }
        "page-number-fields-regression-bundle|review-only" {
            "task_prompt_page_number_fields_review_only_template.md"
        }
        "page-number-fields-regression-bundle|review-and-repair" {
            "task_prompt_page_number_fields_review_and_repair_template.md"
        }
        default {
            throw "Unsupported task source '$SourceKind' and mode '$Mode'."
        }
    }

    return Join-Path $RepoRoot "docs\automation\$fileName"
}

function Get-ReportTemplatePath {
    param(
        [string]$RepoRoot,
        [string]$FileName
    )

    return Join-Path $RepoRoot "docs\automation\$FileName"
}

function Expand-Template {
    param(
        [string]$TemplatePath,
        [hashtable]$Values
    )

    $content = Get-Content -Path $TemplatePath -Raw -Encoding UTF8
    foreach ($key in $Values.Keys) {
        $content = $content.Replace($key, $Values[$key])
    }

    return $content
}

function ConvertTo-JsonTemplateValues {
    param([hashtable]$Values)

    $escapedValues = @{}
    foreach ($key in $Values.Keys) {
        $json = ConvertTo-Json ([string]$Values[$key]) -Compress
        $escapedValues[$key] = $json.Substring(1, $json.Length - 2)
    }

    return $escapedValues
}

function New-UniqueTaskLocation {
    param(
        [string]$TaskRoot,
        [string]$SafeStem
    )

    $baseTimestamp = Get-Date -Format "yyyyMMdd-HHmmss-fff"
    $attempt = 0

    while ($true) {
        $suffix = if ($attempt -eq 0) { "" } else { "-{0:D2}" -f $attempt }
        $taskId = "$SafeStem-$baseTimestamp$suffix"
        $taskDir = Join-Path $TaskRoot $taskId
        if (-not (Test-Path $taskDir)) {
            return @{
                TaskId = $taskId
                TaskDir = $taskDir
            }
        }
        $attempt += 1
    }
}

function Copy-PathIfExists {
    param(
        [string]$SourcePath,
        [string]$DestinationPath
    )

    if ([string]::IsNullOrWhiteSpace($SourcePath) -or -not (Test-Path $SourcePath)) {
        return ""
    }

    $destinationParent = Split-Path -Parent $DestinationPath
    if (-not [string]::IsNullOrWhiteSpace($destinationParent)) {
        New-Item -ItemType Directory -Path $destinationParent -Force | Out-Null
    }

    Copy-Item -Path $SourcePath -Destination $DestinationPath -Force
    return $DestinationPath
}

function Copy-DirectoryContentsIfExists {
    param(
        [string]$SourceDir,
        [string]$DestinationDir
    )

    if ([string]::IsNullOrWhiteSpace($SourceDir) -or
        -not (Test-Path -LiteralPath $SourceDir -PathType Container)) {
        return ""
    }

    $sourceItems = @(Get-ChildItem -LiteralPath $SourceDir -Force)
    if ($sourceItems.Count -eq 0) {
        return ""
    }

    New-Item -ItemType Directory -Path $DestinationDir -Force | Out-Null
    Copy-Item -Path (Join-Path $SourceDir "*") -Destination $DestinationDir -Recurse -Force
    return $DestinationDir
}

function Copy-DocumentSupportFiles {
    param(
        $DocumentArtifacts,
        [string]$TaskDir,
        [string]$EvidenceDir,
        [string]$ReportDir
    )

    if ($null -eq $DocumentArtifacts) {
        return $null
    }

    $pdfCopyPath = if (-not [string]::IsNullOrWhiteSpace($DocumentArtifacts.PdfPath)) {
        Join-Path $TaskDir ([System.IO.Path]::GetFileName($DocumentArtifacts.PdfPath))
    } else {
        ""
    }
    $summaryCopyPath = Join-Path $ReportDir "summary.json"
    $reviewChecklistCopyPath = Join-Path $ReportDir "review_checklist.md"
    $sourceReviewResultCopyPath = Join-Path $ReportDir "source_visual_review_result.json"
    $sourceFinalReviewCopyPath = Join-Path $ReportDir "source_visual_final_review.md"
    $contactSheetCopyPath = Join-Path $EvidenceDir "contact_sheet.png"
    $pagesCopyDir = Join-Path $EvidenceDir "pages"

    $copiedPdfPath = Copy-PathIfExists -SourcePath $DocumentArtifacts.PdfPath -DestinationPath $pdfCopyPath
    $copiedSummaryPath = Copy-PathIfExists -SourcePath $DocumentArtifacts.SummaryPath -DestinationPath $summaryCopyPath
    $copiedReviewChecklistPath = Copy-PathIfExists -SourcePath $DocumentArtifacts.ReviewChecklistPath -DestinationPath $reviewChecklistCopyPath
    $copiedSourceReviewResultPath = Copy-PathIfExists -SourcePath $DocumentArtifacts.ReviewResultPath -DestinationPath $sourceReviewResultCopyPath
    $copiedSourceFinalReviewPath = Copy-PathIfExists -SourcePath $DocumentArtifacts.FinalReviewPath -DestinationPath $sourceFinalReviewCopyPath
    $copiedContactSheetPath = Copy-PathIfExists -SourcePath $DocumentArtifacts.ContactSheetPath -DestinationPath $contactSheetCopyPath
    $copiedPagesDir = Copy-DirectoryContentsIfExists -SourceDir $DocumentArtifacts.PagesDir -DestinationDir $pagesCopyDir

    if ((@(
                $copiedPdfPath,
                $copiedSummaryPath,
                $copiedReviewChecklistPath,
                $copiedSourceReviewResultPath,
                $copiedSourceFinalReviewPath,
                $copiedContactSheetPath,
                $copiedPagesDir
            ) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }).Count -eq 0) {
        return $null
    }

    return [ordered]@{
        PdfPath = $copiedPdfPath
        SummaryPath = $copiedSummaryPath
        ReviewChecklistPath = $copiedReviewChecklistPath
        SourceReviewResultPath = $copiedSourceReviewResultPath
        SourceFinalReviewPath = $copiedSourceFinalReviewPath
        ContactSheetPath = $copiedContactSheetPath
        PagesDir = $copiedPagesDir
    }
}

function Copy-FixedGridBundleSupportFiles {
    param(
        $BundleInfo,
        [string]$TaskDir,
        [string]$EvidenceDir,
        [string]$ReportDir
    )

    $summaryCopyPath = Join-Path $TaskDir "bundle_summary.json"
    $reviewManifestCopyPath = Join-Path $TaskDir "bundle_review_manifest.json"
    $aggregateContactSheetCopyPath = Join-Path $EvidenceDir "aggregate_contact_sheet.png"
    $aggregateFirstPagesCopyDir = Join-Path $EvidenceDir "aggregate-first-pages"
    $sourceChecklistCopyPath = Join-Path $ReportDir "source_bundle_review_checklist.md"
    $sourceFinalReviewCopyPath = Join-Path $ReportDir "source_bundle_final_review.md"

    Copy-PathIfExists -SourcePath $BundleInfo.SummaryPath -DestinationPath $summaryCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.ReviewManifestPath -DestinationPath $reviewManifestCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.AggregateContactSheetPath -DestinationPath $aggregateContactSheetCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.ReviewChecklistPath -DestinationPath $sourceChecklistCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.FinalReviewPath -DestinationPath $sourceFinalReviewCopyPath | Out-Null

    New-Item -ItemType Directory -Path $aggregateFirstPagesCopyDir -Force | Out-Null
    Copy-Item -Path (Join-Path $BundleInfo.AggregateFirstPagesDir "*") `
        -Destination $aggregateFirstPagesCopyDir -Force

    return [ordered]@{
        SummaryPath = $summaryCopyPath
        ReviewManifestPath = $reviewManifestCopyPath
        AggregateContactSheetPath = $aggregateContactSheetCopyPath
        AggregateFirstPagesDir = $aggregateFirstPagesCopyDir
        SourceChecklistPath = if (Test-Path $sourceChecklistCopyPath) {
            $sourceChecklistCopyPath
        } else {
            ""
        }
        SourceFinalReviewPath = if (Test-Path $sourceFinalReviewCopyPath) {
            $sourceFinalReviewCopyPath
        } else {
            ""
        }
    }
}

function Copy-SectionPageSetupBundleSupportFiles {
    param(
        $BundleInfo,
        [string]$TaskDir,
        [string]$EvidenceDir,
        [string]$ReportDir
    )

    $summaryCopyPath = Join-Path $TaskDir "bundle_summary.json"
    $reviewManifestCopyPath = Join-Path $TaskDir "bundle_review_manifest.json"
    $aggregateContactSheetCopyPath = Join-Path $EvidenceDir "aggregate_contact_sheet.png"
    $aggregateContactSheetsCopyDir = Join-Path $EvidenceDir "aggregate-contact-sheets"
    $sourceChecklistCopyPath = Join-Path $ReportDir "source_bundle_review_checklist.md"
    $sourceFinalReviewCopyPath = Join-Path $ReportDir "source_bundle_final_review.md"

    Copy-PathIfExists -SourcePath $BundleInfo.SummaryPath -DestinationPath $summaryCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.ReviewManifestPath -DestinationPath $reviewManifestCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.AggregateContactSheetPath -DestinationPath $aggregateContactSheetCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.ReviewChecklistPath -DestinationPath $sourceChecklistCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.FinalReviewPath -DestinationPath $sourceFinalReviewCopyPath | Out-Null

    New-Item -ItemType Directory -Path $aggregateContactSheetsCopyDir -Force | Out-Null
    Copy-Item -Path (Join-Path $BundleInfo.AggregateContactSheetsDir "*") `
        -Destination $aggregateContactSheetsCopyDir -Force

    return [ordered]@{
        SummaryPath = $summaryCopyPath
        ReviewManifestPath = $reviewManifestCopyPath
        AggregateContactSheetPath = $aggregateContactSheetCopyPath
        AggregateContactSheetsDir = $aggregateContactSheetsCopyDir
        SourceChecklistPath = if (Test-Path $sourceChecklistCopyPath) {
            $sourceChecklistCopyPath
        } else {
            ""
        }
        SourceFinalReviewPath = if (Test-Path $sourceFinalReviewCopyPath) {
            $sourceFinalReviewCopyPath
        } else {
            ""
        }
    }
}

function Copy-PageNumberFieldsBundleSupportFiles {
    param(
        $BundleInfo,
        [string]$TaskDir,
        [string]$EvidenceDir,
        [string]$ReportDir
    )

    $summaryCopyPath = Join-Path $TaskDir "bundle_summary.json"
    $reviewManifestCopyPath = Join-Path $TaskDir "bundle_review_manifest.json"
    $aggregateContactSheetCopyPath = Join-Path $EvidenceDir "aggregate_contact_sheet.png"
    $aggregateContactSheetsCopyDir = Join-Path $EvidenceDir "aggregate-contact-sheets"
    $sourceChecklistCopyPath = Join-Path $ReportDir "source_bundle_review_checklist.md"
    $sourceFinalReviewCopyPath = Join-Path $ReportDir "source_bundle_final_review.md"

    Copy-PathIfExists -SourcePath $BundleInfo.SummaryPath -DestinationPath $summaryCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.ReviewManifestPath -DestinationPath $reviewManifestCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.AggregateContactSheetPath -DestinationPath $aggregateContactSheetCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.ReviewChecklistPath -DestinationPath $sourceChecklistCopyPath | Out-Null
    Copy-PathIfExists -SourcePath $BundleInfo.FinalReviewPath -DestinationPath $sourceFinalReviewCopyPath | Out-Null

    New-Item -ItemType Directory -Path $aggregateContactSheetsCopyDir -Force | Out-Null
    Copy-Item -Path (Join-Path $BundleInfo.AggregateContactSheetsDir "*") `
        -Destination $aggregateContactSheetsCopyDir -Force

    return [ordered]@{
        SummaryPath = $summaryCopyPath
        ReviewManifestPath = $reviewManifestCopyPath
        AggregateContactSheetPath = $aggregateContactSheetCopyPath
        AggregateContactSheetsDir = $aggregateContactSheetsCopyDir
        SourceChecklistPath = if (Test-Path $sourceChecklistCopyPath) {
            $sourceChecklistCopyPath
        } else {
            ""
        }
        SourceFinalReviewPath = if (Test-Path $sourceFinalReviewCopyPath) {
            $sourceFinalReviewCopyPath
        } else {
            ""
        }
    }
}

function Write-LatestTaskPointers {
    param(
        [string]$RepoRoot,
        [hashtable]$PointerPaths,
        [hashtable]$TaskInfo
    )

    $taskRootRelative = Get-RelativePathCompat -BasePath $RepoRoot -TargetPath $TaskInfo.task_root
    $taskDirRelative = Get-RelativePathCompat -BasePath $RepoRoot -TargetPath $TaskInfo.task_dir
    $manifestRelative = Get-RelativePathCompat -BasePath $RepoRoot -TargetPath $TaskInfo.manifest_path
    $promptRelative = Get-RelativePathCompat -BasePath $RepoRoot -TargetPath $TaskInfo.prompt_path
    $reviewResultRelative = Get-RelativePathCompat -BasePath $RepoRoot -TargetPath $TaskInfo.review_result_path
    $finalReviewRelative = Get-RelativePathCompat -BasePath $RepoRoot -TargetPath $TaskInfo.final_review_path

    $payload = [ordered]@{
        generated_at = $TaskInfo.generated_at
        workspace = $RepoRoot
        task_root = $TaskInfo.task_root
        task_root_relative = $taskRootRelative
        task_id = $TaskInfo.task_id
        mode = $TaskInfo.mode
        source = $TaskInfo.source
        document = $TaskInfo.document
        task_dir = $TaskInfo.task_dir
        task_dir_relative = $taskDirRelative
        manifest_path = $TaskInfo.manifest_path
        manifest_path_relative = $manifestRelative
        prompt_path = $TaskInfo.prompt_path
        prompt_path_relative = $promptRelative
        evidence_dir = $TaskInfo.evidence_dir
        report_dir = $TaskInfo.report_dir
        repair_dir = $TaskInfo.repair_dir
        review_result_path = $TaskInfo.review_result_path
        review_result_path_relative = $reviewResultRelative
        final_review_path = $TaskInfo.final_review_path
        final_review_path_relative = $finalReviewRelative
        pointer_files = [ordered]@{
            generic = $PointerPaths.GenericPath
            source_kind = $PointerPaths.SourceKindPath
        }
    }

    ($payload | ConvertTo-Json -Depth 6) | Set-Content -Path $PointerPaths.GenericPath -Encoding UTF8
    ($payload | ConvertTo-Json -Depth 6) | Set-Content -Path $PointerPaths.SourceKindPath -Encoding UTF8
}

$repoRoot = Resolve-RepoRoot
$hasDocxPath = -not [string]::IsNullOrWhiteSpace($DocxPath)
$hasFixedGridBundle = -not [string]::IsNullOrWhiteSpace($FixedGridRegressionRoot)
$hasSectionPageSetupBundle = -not [string]::IsNullOrWhiteSpace($SectionPageSetupRegressionRoot)
$hasPageNumberFieldsBundle = -not [string]::IsNullOrWhiteSpace($PageNumberFieldsRegressionRoot)

$selectedSourceCount = (@(
        $hasDocxPath,
        $hasFixedGridBundle,
        $hasSectionPageSetupBundle,
        $hasPageNumberFieldsBundle
    ) |
    Where-Object { $_ } |
    Measure-Object).Count
if ($selectedSourceCount -ne 1) {
    throw "Specify exactly one of -DocxPath, -FixedGridRegressionRoot, -SectionPageSetupRegressionRoot, or -PageNumberFieldsRegressionRoot."
}

$sourceKind = ""
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
    $sourceLabel = "Target document"
    $sourcePath = $resolvedDocxPath
    $documentVisualInfo = Resolve-DocumentSourceVisualArtifacts -DocumentPath $resolvedDocxPath
} elseif ($hasFixedGridBundle) {
    $bundleInfo = Resolve-FixedGridRegressionBundle -RepoRoot $repoRoot `
        -InputPath $FixedGridRegressionRoot
    $documentName = [System.IO.Path]::GetFileName($bundleInfo.Root)
    $documentStem = $documentName
    $sourceKind = "fixed-grid-regression-bundle"
    $sourceLabel = "Fixed-grid regression bundle"
    $sourcePath = $bundleInfo.Root
} elseif ($hasSectionPageSetupBundle) {
    $bundleInfo = Resolve-SectionPageSetupRegressionBundle -RepoRoot $repoRoot `
        -InputPath $SectionPageSetupRegressionRoot
    $documentName = [System.IO.Path]::GetFileName($bundleInfo.Root)
    $documentStem = $documentName
    $sourceKind = "section-page-setup-regression-bundle"
    $sourceLabel = "Section page setup regression bundle"
    $sourcePath = $bundleInfo.Root
} else {
    $bundleInfo = Resolve-PageNumberFieldsRegressionBundle -RepoRoot $repoRoot `
        -InputPath $PageNumberFieldsRegressionRoot
    $documentName = [System.IO.Path]::GetFileName($bundleInfo.Root)
    $documentStem = $documentName
    $sourceKind = "page-number-fields-regression-bundle"
    $sourceLabel = "Page number fields regression bundle"
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
$latestTaskPointerPaths = Get-LatestTaskPointerPaths -TaskRoot $taskRoot -SourceKind $sourceKind

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
        "`"$repoRoot\scripts\run_section_page_setup_regression.ps1`""
        "-OutputDir"
        "`"$bundleRefreshOutputRelative`""
    ) -join " "
    $bundleRepairCommandExample = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_section_page_setup_regression.ps1`""
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
        "`"$repoRoot\scripts\run_page_number_fields_regression.ps1`""
        "-OutputDir"
        "`"$bundleRefreshOutputRelative`""
    ) -join " "
    $bundleRepairCommandExample = @(
        "powershell -ExecutionPolicy Bypass -File"
        "`"$repoRoot\scripts\run_page_number_fields_regression.ps1`""
        "-OutputDir"
        "`"$bundleRepairOutputRelativeExample`""
    ) -join " "
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
    Write-Host "Bundle review manifest copy: $($bundleLocalInfo.ReviewManifestPath)"
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
