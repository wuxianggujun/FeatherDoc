<#
.SYNOPSIS
Records an operator-supplied Word visual review verdict for existing evidence.

.DESCRIPTION
Updates one or more review_result.json files produced by run_word_visual_smoke.ps1.
The script only consumes existing PNG/PDF evidence; it does not start Word,
LibreOffice, browsers, CMake, CTest, or any renderer.
#>
param(
    [Parameter(Mandatory = $true)]
    [string[]]$ReviewResultJson,
    [ValidateSet("pass", "fail", "undetermined", "pending_manual_review", "undecided")]
    [string]$Verdict = "pass",
    [string]$Reviewer = "operator_supplied",
    [string]$ReviewNote = "",
    [switch]$RequireNonEmptyEvidence
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "word_visual_review_report.ps1")

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-PathForRepo {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    return [System.IO.Path]::GetFullPath($candidate)
}

function Get-OptionalProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-OptionalString {
    param($Object, [string]$Name)

    $value = Get-OptionalProperty -Object $Object -Name $Name
    if ($null -eq $value) { return "" }
    return [string]$value
}

function Get-OptionalArray {
    param($Object, [string]$Name)

    $value = Get-OptionalProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Test-ImageNonEmpty {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        throw "Visual evidence image is missing: $Path"
    }
    $file = Get-Item -LiteralPath $Path
    if ($file.Length -le 0) {
        throw "Visual evidence image is empty: $Path"
    }

    Add-Type -AssemblyName System.Drawing
    $image = [System.Drawing.Image]::FromFile($Path)
    $bitmap = $null
    try {
        $bitmap = [System.Drawing.Bitmap]::new($Path)
        $sampled = 0
        $nonWhite = 0
        $stepX = [Math]::Max(1, [int]($bitmap.Width / 64))
        $stepY = [Math]::Max(1, [int]($bitmap.Height / 64))
        for ($y = 0; $y -lt $bitmap.Height; $y += $stepY) {
            for ($x = 0; $x -lt $bitmap.Width; $x += $stepX) {
                $color = $bitmap.GetPixel($x, $y)
                $sampled++
                if ($color.R -lt 245 -or $color.G -lt 245 -or $color.B -lt 245) {
                    $nonWhite++
                }
            }
        }

        return [ordered]@{
            path = $Path
            bytes = $file.Length
            width = $image.Width
            height = $image.Height
            sampled_pixels = $sampled
            sampled_non_white = $nonWhite
            non_empty_visual = ($nonWhite -gt 0)
        }
    } finally {
        if ($bitmap) { $bitmap.Dispose() }
        if ($image) { $image.Dispose() }
    }
}

function Set-JsonProperty {
    param($Object, [string]$Name, $Value)

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        Add-Member -InputObject $Object -MemberType NoteProperty -Name $Name -Value $Value
    } else {
        $property.Value = $Value
    }
}

$repoRoot = Resolve-RepoRoot
$normalizedVerdict = Normalize-WordVisualReviewVerdict -Value $Verdict
$status = Get-WordVisualReviewStatus -Verdict $normalizedVerdict
$reviewedAt = (Get-Date).ToString("s")
$results = New-Object 'System.Collections.Generic.List[object]'

foreach ($reviewPathInput in @($ReviewResultJson)) {
    $reviewPath = Resolve-PathForRepo -RepoRoot $repoRoot -Path $reviewPathInput
    if (-not (Test-Path -LiteralPath $reviewPath)) {
        throw "Review result JSON was not found: $reviewPathInput"
    }

    $review = Get-Content -Raw -Encoding UTF8 -LiteralPath $reviewPath | ConvertFrom-Json
    $evidence = Get-OptionalProperty -Object $review -Name "evidence"
    $contactSheet = Resolve-PathForRepo -RepoRoot $repoRoot -Path (Get-OptionalString -Object $evidence -Name "contact_sheet")
    $pageImages = @(
        Get-OptionalArray -Object $evidence -Name "page_images" |
            ForEach-Object { Resolve-PathForRepo -RepoRoot $repoRoot -Path ([string]$_) }
    )

    $contactSheetCheck = Test-ImageNonEmpty -Path $contactSheet
    $pageImageChecks = @(
        foreach ($pageImage in $pageImages) {
            Test-ImageNonEmpty -Path $pageImage
        }
    )
    if ($RequireNonEmptyEvidence -and (-not [bool]$contactSheetCheck.non_empty_visual)) {
        throw "Contact sheet has no sampled non-white pixels: $contactSheet"
    }
    if ($RequireNonEmptyEvidence -and @($pageImageChecks | Where-Object { -not [bool]$_.non_empty_visual }).Count -gt 0) {
        throw "At least one page image has no sampled non-white pixels."
    }

    Set-JsonProperty -Object $review -Name "status" -Value $status
    Set-JsonProperty -Object $review -Name "verdict" -Value $normalizedVerdict
    Set-JsonProperty -Object $review -Name "reviewed_at" -Value $reviewedAt
    Set-JsonProperty -Object $review -Name "review_method" -Value $Reviewer
    if (-not [string]::IsNullOrWhiteSpace($ReviewNote)) {
        Set-JsonProperty -Object $review -Name "review_note" -Value $ReviewNote
    }
    Set-JsonProperty -Object $review -Name "visual_evidence_check" -Value ([ordered]@{
            contact_sheet = $contactSheetCheck
            page_images = @($pageImageChecks)
        })

    ($review | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $reviewPath -Encoding UTF8

    $reportDir = Get-OptionalString -Object $review -Name "report_dir"
    if (-not [string]::IsNullOrWhiteSpace($reportDir)) {
        $finalReviewPath = Join-Path (Resolve-PathForRepo -RepoRoot $repoRoot -Path $reportDir) "final_review.md"
        New-WordVisualFinalReviewMarkdown `
            -DocumentPath (Get-OptionalString -Object $review -Name "document_path") `
            -PdfPath (Get-OptionalString -Object $review -Name "pdf_path") `
            -EvidenceDir (Get-OptionalString -Object $review -Name "evidence_dir") `
            -ReportDir $reportDir `
            -RepairDir (Get-OptionalString -Object $review -Name "repair_dir") `
            -GeneratedAt $reviewedAt `
            -ReviewVerdict $normalizedVerdict `
            -ReviewNote $ReviewNote |
            Set-Content -LiteralPath $finalReviewPath -Encoding UTF8
    }

    [void]$results.Add([ordered]@{
            review_result_json = $reviewPath
            status = $status
            verdict = $normalizedVerdict
            reviewed_at = $reviewedAt
            reviewer = $Reviewer
            contact_sheet_non_empty_visual = [bool]$contactSheetCheck.non_empty_visual
            page_image_count = @($pageImageChecks).Count
            page_images_non_empty_visual = (@($pageImageChecks | Where-Object { [bool]$_.non_empty_visual }).Count -eq @($pageImageChecks).Count)
        })
}

([ordered]@{
        schema = "featherdoc.word_visual_review_result_record.v1"
        generated_at = $reviewedAt
        status = "pass"
        updated_review_count = $results.Count
        reviews = @($results.ToArray())
    } | ConvertTo-Json -Depth 32)
