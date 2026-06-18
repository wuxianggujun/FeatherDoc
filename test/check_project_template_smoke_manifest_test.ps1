param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not $RepoRoot) {
    $RepoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
}

if (-not $WorkingDir) {
    $WorkingDir = Join-Path $RepoRoot "build\check_project_template_smoke_manifest_test"
}

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

function Assert-CollectionContains {
    param($Items, [string]$ExpectedText, [string]$Message)

    if (-not (@($Items) -contains $ExpectedText)) {
        $actualText = (@($Items) -join ", ")
        throw "$Message Expected='$ExpectedText' Actual='$actualText'."
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
        project_id = "project-test"
        template_name = $Name
        business_domain = "finance"
        business_document_type = "invoice"
        corpus_role = "registered-business-template"
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
    business_template_corpus = @(
        [ordered]@{
            id = "project-test-invoice-template"
            project_id = "project-test"
            template_name = "invoice-template"
            document_type = "invoice"
            status = "registered"
            source_entry = "invoice-template"
            smoke_contract = @("template_validations", "schema_validation", "render_data", "visual_smoke")
            coverage_goal = "Validate the lightweight invoice business template smoke contract."
            notes = "Regression fixture for manifest business corpus metadata."
        },
        [ordered]@{
            id = "project-test-contract-template"
            project_id = "project-test"
            template_name = "contract-template"
            document_type = "contract"
            status = "planned"
            smoke_contract = @("schema_validation", "schema_baseline")
            registration_blocker = "No committed contract template fixture is registered yet."
            next_action = "Add a contract template manifest entry with schema baseline coverage."
            coverage_goal = "Track planned contract coverage without adding a binary fixture."
        }
    )
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
Assert-Equal -Actual ([int]$validReport.business_template_corpus_count) -Expected 2 `
    -Message "Valid report should count business corpus profiles."
Assert-Equal -Actual ([int]$validReport.registered_business_template_corpus_count) -Expected 1 `
    -Message "Valid report should count registered business corpus profiles."
Assert-Equal -Actual ([int]$validReport.planned_business_template_corpus_count) -Expected 1 `
    -Message "Valid report should count planned business corpus profiles."
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

$repoManifestReportPath = Join-Path $resolvedWorkingDir "reports\repo_manifest_check.json"
$repoManifestResult = Invoke-ManifestCheck -Arguments @(
    "-ManifestPath", (Join-Path $resolvedRepoRoot "samples\project_template_smoke.manifest.json"),
    "-OutputPath", $repoManifestReportPath,
    "-Json"
)
Assert-Equal -Actual $repoManifestResult.ExitCode -Expected 0 `
    -Message "Repository project-template smoke manifest should pass. Output: $($repoManifestResult.Text)"
$repoManifestReport = Get-Content -Raw -Encoding UTF8 -LiteralPath $repoManifestReportPath | ConvertFrom-Json
Assert-Equal -Actual ([int]$repoManifestReport.business_template_corpus_count) -Expected 6 `
    -Message "Repository manifest should keep the full business template corpus backlog."
Assert-Equal -Actual ([int]$repoManifestReport.registered_business_template_corpus_count) -Expected 1 `
    -Message "Repository manifest should keep one registered business template corpus entry."
Assert-Equal -Actual ([int]$repoManifestReport.planned_business_template_corpus_count) -Expected 5 `
    -Message "Repository manifest should keep five planned business template corpus entries."
$repoManifest = Get-Content -Raw -Encoding UTF8 -LiteralPath (Join-Path $resolvedRepoRoot "samples\project_template_smoke.manifest.json") | ConvertFrom-Json
$repoBusinessDocumentTypes = @($repoManifest.business_template_corpus | ForEach-Object { [string]$_.document_type })
foreach ($expectedDocumentType in @("invoice", "contract", "policy", "report", "notice", "tender")) {
    Assert-CollectionContains -Items $repoBusinessDocumentTypes -ExpectedText $expectedDocumentType `
        -Message "Repository manifest should preserve planned business document type coverage."
}
$registeredInvoiceCorpus = @($repoManifest.business_template_corpus | Where-Object {
        [string]$_.document_type -eq "invoice" -and [string]$_.status -eq "registered"
    })
Assert-Equal -Actual $registeredInvoiceCorpus.Count -Expected 1 `
    -Message "Repository manifest should keep exactly one registered invoice corpus anchor."
Assert-Equal -Actual ([string]$registeredInvoiceCorpus[0].source_entry) -Expected "chinese-invoice-template" `
    -Message "Registered invoice corpus should point at the committed manifest entry."
$repoEntryNames = @($repoManifest.entries | ForEach-Object { [string]$_.name })
Assert-CollectionContains -Items $repoEntryNames -ExpectedText "chinese-invoice-template" `
    -Message "Repository manifest should keep the registered invoice manifest entry."

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

$invalidBusinessCorpusManifestPath = Join-Path $fixtureRoot "invalid-business-corpus.manifest.json"
Write-JsonFile -Path $invalidBusinessCorpusManifestPath -Value ([ordered]@{
    business_template_corpus = @(
        [ordered]@{
            id = "missing-source-entry"
            project_id = "project-test"
            template_name = "missing-template"
            document_type = "contract"
            status = "registered"
            source_entry = "not-a-manifest-entry"
            smoke_contract = @("schema_validation")
            coverage_goal = "This fixture should fail because registered corpus entries must point at a manifest entry."
        },
        [ordered]@{
            id = "missing-source-entry"
            project_id = "project-test"
            template_name = "bad-contract"
            document_type = "contract"
            status = "planned"
            smoke_contract = @("unknown_check")
            coverage_goal = "This fixture should fail because corpus ids and smoke contracts are invalid."
        }
    )
    entries = @(
        (New-ValidEntry -Name "invoice-template" -InputDocx $inputDocxPath)
    )
})
$invalidBusinessCorpusResult = Invoke-ManifestCheck -Arguments @("-ManifestPath", $invalidBusinessCorpusManifestPath, "-Json")
Assert-Equal -Actual $invalidBusinessCorpusResult.ExitCode -Expected 1 `
    -Message "Invalid business corpus manifest should fail. Output: $($invalidBusinessCorpusResult.Text)"
$invalidBusinessCorpusReport = $invalidBusinessCorpusResult.Text | ConvertFrom-Json
Assert-ReportContainsIssue -Report $invalidBusinessCorpusReport `
    -ExpectedPath "business_template_corpus[0].source_entry" `
    -ExpectedMessage "must reference an existing manifest entry" `
    -Message "Registered business corpus entries should point at manifest entries."
Assert-ReportContainsIssue -Report $invalidBusinessCorpusReport `
    -ExpectedPath "business_template_corpus[1].id" `
    -ExpectedMessage "must be unique across business_template_corpus" `
    -Message "Business corpus ids should be unique."
Assert-ReportContainsIssue -Report $invalidBusinessCorpusReport `
    -ExpectedPath "business_template_corpus[1].smoke_contract[0]" `
    -ExpectedMessage "must be one of: template_validations, schema_validation, schema_baseline, render_data, visual_smoke" `
    -Message "Business corpus smoke contracts should use known smoke checks."
Assert-ReportContainsIssue -Report $invalidBusinessCorpusReport `
    -ExpectedPath "business_template_corpus[1].registration_blocker" `
    -ExpectedMessage "is required when status is planned" `
    -Message "Planned business corpus entries should explain why they are not registered."
Assert-ReportContainsIssue -Report $invalidBusinessCorpusReport `
    -ExpectedPath "business_template_corpus[1].next_action" `
    -ExpectedMessage "is required when status is planned" `
    -Message "Planned business corpus entries should expose the next registration action."

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
