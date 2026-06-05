param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { throw $Message }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) { throw "$Message Expected='$Expected' Actual='$Actual'." }
}

function Assert-ContainsText {
    param([string]$Text, [string]$ExpectedText, [string]$Message)
    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-ReportContainsIssue {
    param($Report, [string]$ExpectedPath, [string]$ExpectedMessage, [string]$Message)

    $matches = @($Report.errors | Where-Object {
            [string]$_.path -eq $ExpectedPath -and [string]$_.message -eq $ExpectedMessage
        })
    if ($matches.Count -eq 0) {
        $issueText = (@($Report.errors | ForEach-Object { "$($_.path): $($_.message)" }) -join [System.Environment]::NewLine)
        $expectedIssue = "{0}: {1}" -f $ExpectedPath, $ExpectedMessage
        throw "$Message Missing='$expectedIssue'. Issues=$issueText"
    }
}

function Write-JsonFile {
    param([string]$Path, $Value)

    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-ManifestCheck {
    param([string[]]$Arguments)

    $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
    $powerShellPath = if ($powerShellCommand) { $powerShellCommand.Source } else { (Get-Process -Id $PID).Path }
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @Arguments 2>&1)
        $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Text = (@($output | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
    }
}

function New-ValidEntry {
    param([string]$Name, [string]$InputDocx)

    return [ordered]@{
        name = $Name
        input_docx = $InputDocx
        template_validations = @(
            [ordered]@{
                name = "body-slots"
                part = "body"
                slots = @("customer_name:text", "line_items:table_rows")
            }
        )
        schema_validation = [ordered]@{
            targets = @(
                [ordered]@{
                    part = "section-header"
                    section = 0
                    kind = "default"
                    slots = @(
                        [ordered]@{
                            bookmark = "header_title"
                            kind = "text"
                            min = 1
                            max = 1
                        }
                    )
                }
            )
        }
        render_data_smoke = [ordered]@{
            data_path = $renderDataPath
            mapping_path = $renderMappingPath
            output_docx = "output/project-template-smoke/$Name-rendered.docx"
        }
        visual_smoke = [ordered]@{
            enabled = $true
            input = "rendered_docx"
            output_dir = "output/project-template-smoke/$Name-visual"
        }
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\check_project_template_smoke_manifest.ps1"
$fixtureRoot = Join-Path $resolvedWorkingDir "fixtures"
$inputDocxPath = Join-Path $fixtureRoot "invoice.docx"
$renderDataPath = Join-Path $fixtureRoot "invoice.render_data.json"
$renderMappingPath = Join-Path $fixtureRoot "invoice.render_data_mapping.json"
New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
Set-Content -LiteralPath $inputDocxPath -Encoding UTF8 -Value "placeholder docx fixture"
Set-Content -LiteralPath $renderDataPath -Encoding UTF8 -Value "{ `"customer`": { `"name`": `"Acme`" } }"
Set-Content -LiteralPath $renderMappingPath -Encoding UTF8 -Value "{ `"bookmark_text`": [] }"

$validManifestPath = Join-Path $fixtureRoot "valid.manifest.json"
$validOutputPath = Join-Path $resolvedWorkingDir "reports\valid_manifest_check.json"
Write-JsonFile -Path $validManifestPath -Value ([ordered]@{
    entries = @(
        (New-ValidEntry -Name "invoice-template" -InputDocx $inputDocxPath)
    )
})

$validResult = Invoke-ManifestCheck -Arguments @(
    "-ManifestPath", $validManifestPath,
    "-OutputPath", $validOutputPath,
    "-Json",
    "-CheckPaths"
)
Assert-Equal -Actual $validResult.ExitCode -Expected 0 `
    -Message "Valid manifest should pass. Output: $($validResult.Text)"
Assert-True -Condition (Test-Path -LiteralPath $validOutputPath) `
    -Message "Valid manifest JSON report should be written."
Assert-ContainsText -Text $validResult.Text -ExpectedText "Wrote JSON report" `
    -Message "Valid manifest run should announce the JSON report."

$validReport = Get-Content -Raw -Encoding UTF8 -LiteralPath $validOutputPath | ConvertFrom-Json
Assert-Equal -Actual ([bool]$validReport.passed) -Expected $true `
    -Message "Valid report should pass."
Assert-Equal -Actual ([int]$validReport.error_count) -Expected 0 `
    -Message "Valid report should have no errors."
Assert-Equal -Actual ([int]$validReport.entry_count) -Expected 1 `
    -Message "Valid report should count one entry."
Assert-ContainsText -Text ((@($validReport.entries[0].configured_checks) -join "`n")) `
    -ExpectedText "template_validations" `
    -Message "Valid report should include template validation checks."
Assert-ContainsText -Text ((@($validReport.entries[0].configured_checks) -join "`n")) `
    -ExpectedText "schema_validation" `
    -Message "Valid report should include schema validation checks."
Assert-ContainsText -Text ((@($validReport.entries[0].configured_checks) -join "`n")) `
    -ExpectedText "render_data" `
    -Message "Valid report should include render data checks."
Assert-ContainsText -Text ((@($validReport.entries[0].configured_checks) -join "`n")) `
    -ExpectedText "visual_smoke" `
    -Message "Valid report should include visual smoke checks."

$emptyManifestPath = Join-Path $fixtureRoot "empty.manifest.json"
Write-JsonFile -Path $emptyManifestPath -Value ([ordered]@{ entries = @() })
$emptyResult = Invoke-ManifestCheck -Arguments @("-ManifestPath", $emptyManifestPath, "-Json")
Assert-Equal -Actual $emptyResult.ExitCode -Expected 1 `
    -Message "Empty manifest should fail. Output: $($emptyResult.Text)"
$emptyReport = $emptyResult.Text | ConvertFrom-Json
Assert-ReportContainsIssue -Report $emptyReport `
    -ExpectedPath "entries" `
    -ExpectedMessage "must contain at least one entry" `
    -Message "Empty manifest failure should explain the missing entries."

$duplicateManifestPath = Join-Path $fixtureRoot "duplicate.manifest.json"
Write-JsonFile -Path $duplicateManifestPath -Value ([ordered]@{
    entries = @(
        (New-ValidEntry -Name "duplicate-template" -InputDocx $inputDocxPath),
        (New-ValidEntry -Name "duplicate-template" -InputDocx $inputDocxPath)
    )
})
$duplicateResult = Invoke-ManifestCheck -Arguments @("-ManifestPath", $duplicateManifestPath, "-Json")
Assert-Equal -Actual $duplicateResult.ExitCode -Expected 1 `
    -Message "Duplicate entry names should fail. Output: $($duplicateResult.Text)"
$duplicateReport = $duplicateResult.Text | ConvertFrom-Json
Assert-ReportContainsIssue -Report $duplicateReport `
    -ExpectedPath "entries[1].name" `
    -ExpectedMessage "must be unique across all entries" `
    -Message "Duplicate failure should point at the second entry name."

$noChecksManifestPath = Join-Path $fixtureRoot "no-checks.manifest.json"
Write-JsonFile -Path $noChecksManifestPath -Value ([ordered]@{
    entries = @(
        [ordered]@{
            name = "no-checks-template"
            input_docx = $inputDocxPath
            visual_smoke = [ordered]@{
                enabled = $false
            }
        }
    )
})
$noChecksResult = Invoke-ManifestCheck -Arguments @("-ManifestPath", $noChecksManifestPath, "-Json")
Assert-Equal -Actual $noChecksResult.ExitCode -Expected 1 `
    -Message "Entry without enabled checks should fail. Output: $($noChecksResult.Text)"
$noChecksReport = $noChecksResult.Text | ConvertFrom-Json
Assert-ReportContainsIssue -Report $noChecksReport `
    -ExpectedPath "entries[0]" `
    -ExpectedMessage "does not enable any checks" `
    -Message "No-checks failure should explain the missing enabled checks."

$invalidSelectionManifestPath = Join-Path $fixtureRoot "invalid-selection.manifest.json"
Write-JsonFile -Path $invalidSelectionManifestPath -Value ([ordered]@{
    entries = @(
        [ordered]@{
            name = "invalid-selection-template"
            input_docx = $inputDocxPath
            template_validations = @(
                [ordered]@{
                    name = "bad-selection"
                    part = "section-header"
                    kind = "odd"
                    slots = @(
                        [ordered]@{
                            bookmark = "title"
                            name = "different_title"
                            kind = ""
                            count = 1
                            min = 1
                        }
                    )
                }
            )
        }
    )
})
$invalidSelectionResult = Invoke-ManifestCheck -Arguments @("-ManifestPath", $invalidSelectionManifestPath, "-Json")
Assert-Equal -Actual $invalidSelectionResult.ExitCode -Expected 1 `
    -Message "Invalid selection and slot structure should fail. Output: $($invalidSelectionResult.Text)"
$invalidSelectionReport = $invalidSelectionResult.Text | ConvertFrom-Json
Assert-ReportContainsIssue -Report $invalidSelectionReport `
    -ExpectedPath "entries[0].template_validations[0].kind" `
    -ExpectedMessage "must be one of: default, first, even" `
    -Message "Invalid selection failure should identify the section kind."
Assert-ReportContainsIssue -Report $invalidSelectionReport `
    -ExpectedPath "entries[0].template_validations[0].section" `
    -ExpectedMessage "is required for section-header selections" `
    -Message "Invalid selection failure should require section."
Assert-ReportContainsIssue -Report $invalidSelectionReport `
    -ExpectedPath "entries[0].template_validations[0].slots[0]" `
    -ExpectedMessage "'bookmark' and 'name' must match when both are provided" `
    -Message "Invalid slot failure should explain bookmark/name mismatch."
Assert-ReportContainsIssue -Report $invalidSelectionReport `
    -ExpectedPath "entries[0].template_validations[0].slots[0]" `
    -ExpectedMessage "'count' conflicts with 'min'/'max'" `
    -Message "Invalid slot failure should explain count/min conflict."

$missingPathManifestPath = Join-Path $fixtureRoot "missing-path.manifest.json"
$missingDocxPath = Join-Path $fixtureRoot "missing.docx"
Write-JsonFile -Path $missingPathManifestPath -Value ([ordered]@{
    entries = @(
        (New-ValidEntry -Name "missing-path-template" -InputDocx $missingDocxPath)
    )
})
$missingPathResult = Invoke-ManifestCheck -Arguments @("-ManifestPath", $missingPathManifestPath, "-CheckPaths")
Assert-Equal -Actual $missingPathResult.ExitCode -Expected 1 `
    -Message "CheckPaths should fail for missing input_docx. Output: $($missingPathResult.Text)"
Assert-ContainsText -Text $missingPathResult.Text -ExpectedText "entries[0].input_docx" `
    -Message "CheckPaths failure should identify input_docx."
Assert-ContainsText -Text $missingPathResult.Text -ExpectedText "does not exist:" `
    -Message "CheckPaths failure should explain the missing path."

$missingRenderDataManifestPath = Join-Path $fixtureRoot "missing-render-data.manifest.json"
$missingRenderDataPath = Join-Path $fixtureRoot "missing-data.json"
Write-JsonFile -Path $missingRenderDataManifestPath -Value ([ordered]@{
    entries = @(
        [ordered]@{
            name = "missing-render-data-template"
            input_docx = $inputDocxPath
            render_data_smoke = [ordered]@{
                data_path = $missingRenderDataPath
                mapping_path = $renderMappingPath
            }
        }
    )
})
$missingRenderDataResult = Invoke-ManifestCheck -Arguments @("-ManifestPath", $missingRenderDataManifestPath, "-CheckPaths", "-Json")
Assert-Equal -Actual $missingRenderDataResult.ExitCode -Expected 1 `
    -Message "CheckPaths should fail for missing render data. Output: $($missingRenderDataResult.Text)"
$missingRenderDataReport = $missingRenderDataResult.Text | ConvertFrom-Json
Assert-ReportContainsIssue -Report $missingRenderDataReport `
    -ExpectedPath "entries[0].render_data_smoke.data_path" `
    -ExpectedMessage "does not exist: $missingRenderDataPath" `
    -Message "Missing render data failure should identify data_path."

$missingRenderMappingManifestPath = Join-Path $fixtureRoot "missing-render-mapping.manifest.json"
Write-JsonFile -Path $missingRenderMappingManifestPath -Value ([ordered]@{
    entries = @(
        [ordered]@{
            name = "missing-render-mapping-template"
            input_docx = $inputDocxPath
            render_data_smoke = [ordered]@{
                data_path = $renderDataPath
                mapping_path = ""
            }
        }
    )
})
$missingRenderMappingResult = Invoke-ManifestCheck -Arguments @("-ManifestPath", $missingRenderMappingManifestPath, "-Json")
Assert-Equal -Actual $missingRenderMappingResult.ExitCode -Expected 1 `
    -Message "Manifest should fail when render mapping is missing. Output: $($missingRenderMappingResult.Text)"
$missingRenderMappingReport = $missingRenderMappingResult.Text | ConvertFrom-Json
Assert-ReportContainsIssue -Report $missingRenderMappingReport `
    -ExpectedPath "entries[0].render_data_smoke.mapping_path" `
    -ExpectedMessage "must be a non-empty string" `
    -Message "Missing render mapping failure should identify mapping_path."

Write-Host "Project template smoke manifest checker regression passed."
