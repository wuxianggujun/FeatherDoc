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

function Initialize-ImagePixelProbe {
    if ($null -ne ([System.Management.Automation.PSTypeName]'FeatherDocImagePixelProbe').Type) {
        return
    }

    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public sealed class FeatherDocImagePixelProbeResult
{
    public int Width { get; set; }
    public int Height { get; set; }
    public long ExaminedPixels { get; set; }
    public long NonWhitePixels { get; set; }
}

public static class FeatherDocImagePixelProbe
{
    public static FeatherDocImagePixelProbeResult Probe(string path, byte whiteThreshold)
    {
        using (Image source = Image.FromFile(path))
        using (Bitmap bitmap = new Bitmap(source.Width, source.Height, PixelFormat.Format32bppArgb))
        {
            using (Graphics graphics = Graphics.FromImage(bitmap))
            {
                graphics.Clear(Color.White);
                graphics.DrawImageUnscaled(source, 0, 0);
            }

            Rectangle bounds = new Rectangle(0, 0, bitmap.Width, bitmap.Height);
            BitmapData data = bitmap.LockBits(bounds, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
            long examinedPixels = 0;
            long nonWhitePixels = 0;
            try
            {
                int rowBytes = Math.Abs(data.Stride);
                byte[] row = new byte[rowBytes];
                for (int y = 0; y < bitmap.Height; ++y)
                {
                    IntPtr rowStart = IntPtr.Add(data.Scan0, y * data.Stride);
                    Marshal.Copy(rowStart, row, 0, rowBytes);
                    for (int x = 0; x < bitmap.Width; ++x)
                    {
                        int pixelOffset = x * 4;
                        byte blue = row[pixelOffset];
                        byte green = row[pixelOffset + 1];
                        byte red = row[pixelOffset + 2];
                        ++examinedPixels;
                        if (red < whiteThreshold || green < whiteThreshold || blue < whiteThreshold)
                        {
                            ++nonWhitePixels;
                        }
                    }
                }
            }
            finally
            {
                bitmap.UnlockBits(data);
            }

            return new FeatherDocImagePixelProbeResult
            {
                Width = bitmap.Width,
                Height = bitmap.Height,
                ExaminedPixels = examinedPixels,
                NonWhitePixels = nonWhitePixels
            };
        }
    }
}
'@
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

    Initialize-ImagePixelProbe
    $probe = [FeatherDocImagePixelProbe]::Probe($Path, 245)
    return [ordered]@{
        path = $Path
        bytes = $file.Length
        width = $probe.Width
        height = $probe.Height
        sampled_pixels = $probe.ExaminedPixels
        sampled_non_white = $probe.NonWhitePixels
        non_empty_visual = ($probe.NonWhitePixels -gt 0)
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
