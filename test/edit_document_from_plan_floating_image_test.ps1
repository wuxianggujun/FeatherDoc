param(
    [string]$RepoRoot,
    [string]$BuildDir,
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

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Label
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Label does not contain expected text '$ExpectedText'."
    }
}

function Read-DocxEntryText {
    param(
        [string]$DocxPath,
        [string]$EntryName
    )

    Add-Type -AssemblyName System.IO.Compression
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $archive = [System.IO.Compression.ZipFile]::OpenRead($DocxPath)
    try {
        $entry = $archive.GetEntry($EntryName)
        if ($null -eq $entry) {
            throw "Entry '$EntryName' was not found in $DocxPath."
        }

        $reader = New-Object System.IO.StreamReader($entry.Open())
        try {
            return $reader.ReadToEnd()
        } finally {
            $reader.Dispose()
        }
    } finally {
        $archive.Dispose()
    }
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    $RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
}
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path $RepoRoot "build-msvc-nmake"
}
if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_floating_image"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$imagePath = Join-Path $resolvedWorkingDir "floating_image.png"
$editPlanPath = Join-Path $resolvedWorkingDir "floating_image.edit_plan.json"
$outputDocx = Join-Path $resolvedWorkingDir "floating_image.docx"
$summaryPath = Join-Path $resolvedWorkingDir "floating_image.summary.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null
[System.IO.File]::WriteAllBytes(
    $imagePath,
    [System.Convert]::FromBase64String("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR4nGP4z8AAAAMBAQDJ/pLvAAAAAElFTkSuQmCC"))
$escapedImagePath = $imagePath.Replace("\", "\\")

Set-Content -LiteralPath $editPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "append_floating_image",
      "image_path": "$escapedImagePath",
      "width": 30,
      "height": 15,
      "horizontal_reference": "page",
      "horizontal_offset": 40,
      "vertical_reference": "margin",
      "vertical_offset": 12,
      "behind_text": true,
      "allow_overlap": false,
      "z_order": 96,
      "wrap_mode": "square",
      "wrap_distance_left": 5,
      "wrap_distance_right": 7
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $editPlanPath `
    -OutputDocx $outputDocx `
    -SummaryJson $summaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for append_floating_image."
}

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $outputDocx -EntryName "word/document.xml"

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Floating-image summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 1 `
    -Message "Floating-image summary should record one operation."
Assert-Equal -Actual $summary.operations[0].op -Expected "append_floating_image" `
    -Message "Append-floating-image operation should be recorded."
Assert-Equal -Actual $summary.operations[0].command -Expected "append-image" `
    -Message "Append-floating-image should use the CLI append-image command."
Assert-ContainsText -Text $documentXml -ExpectedText "wp:anchor" -Label "Floating-image document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText 'relativeHeight="96"' -Label "Floating-image document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText '<wp:wrapSquare' -Label "Floating-image document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText '<a:blip' -Label "Floating-image document.xml"

Write-Host "Edit-plan floating image append passed."
