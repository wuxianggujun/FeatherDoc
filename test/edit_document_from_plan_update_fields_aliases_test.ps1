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

function Assert-NotContainsText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Label
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw "$Label unexpectedly contains '$UnexpectedText'."
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
    $WorkingDir = Join-Path $BuildDir "test\edit_document_from_plan_update_fields_aliases"
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\edit_document_from_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"
$enablePlanPath = Join-Path $resolvedWorkingDir "update_fields_enable_alias.edit_plan.json"
$enabledDocx = Join-Path $resolvedWorkingDir "update_fields_enable_alias.docx"
$enableSummaryPath = Join-Path $resolvedWorkingDir "update_fields_enable_alias.summary.json"
$disablePlanPath = Join-Path $resolvedWorkingDir "update_fields_disable_alias.edit_plan.json"
$disabledDocx = Join-Path $resolvedWorkingDir "update_fields_disable_alias.docx"
$disableSummaryPath = Join-Path $resolvedWorkingDir "update_fields_disable_alias.summary.json"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

Set-Content -LiteralPath $enablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "enable_update_fields_on_open"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $sampleDocx `
    -EditPlan $enablePlanPath `
    -OutputDocx $enabledDocx `
    -SummaryJson $enableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the enable_update_fields_on_open alias operation."
}

$enableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $enableSummaryPath | ConvertFrom-Json
$enabledSettingsXml = Read-DocxEntryText -DocxPath $enabledDocx -EntryName "word/settings.xml"

Assert-Equal -Actual $enableSummary.status -Expected "completed" `
    -Message "Enable update-fields alias summary did not report status=completed."
Assert-Equal -Actual $enableSummary.operation_count -Expected 1 `
    -Message "Enable update-fields alias summary should record one operation."
Assert-Equal -Actual $enableSummary.operations[0].op -Expected "enable_update_fields_on_open" `
    -Message "Enable update-fields alias should be recorded."
Assert-Equal -Actual $enableSummary.operations[0].command -Expected "set-update-fields-on-open" `
    -Message "Enable update-fields alias should use the CLI set-update-fields-on-open command."
Assert-ContainsText -Text $enabledSettingsXml -ExpectedText "<w:updateFields" -Label "Enable update-fields alias settings.xml"
Assert-ContainsText -Text $enabledSettingsXml -ExpectedText 'w:val="1"' -Label "Enable update-fields alias settings.xml"

Set-Content -LiteralPath $disablePlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "disable_update_fields_on_open"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $enabledDocx `
    -EditPlan $disablePlanPath `
    -OutputDocx $disabledDocx `
    -SummaryJson $disableSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the disable_update_fields_on_open alias operation."
}

$disableSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $disableSummaryPath | ConvertFrom-Json
$disabledSettingsXml = Read-DocxEntryText -DocxPath $disabledDocx -EntryName "word/settings.xml"

Assert-Equal -Actual $disableSummary.status -Expected "completed" `
    -Message "Disable update-fields alias summary did not report status=completed."
Assert-Equal -Actual $disableSummary.operation_count -Expected 1 `
    -Message "Disable update-fields alias summary should record one operation."
Assert-Equal -Actual $disableSummary.operations[0].op -Expected "disable_update_fields_on_open" `
    -Message "Disable update-fields alias should be recorded."
Assert-Equal -Actual $disableSummary.operations[0].command -Expected "set-update-fields-on-open" `
    -Message "Disable update-fields alias should use the CLI set-update-fields-on-open command."
Assert-NotContainsText -Text $disabledSettingsXml -UnexpectedText "<w:updateFields" -Label "Disable update-fields alias settings.xml"

Write-Host "Edit-plan update-fields aliases passed."
