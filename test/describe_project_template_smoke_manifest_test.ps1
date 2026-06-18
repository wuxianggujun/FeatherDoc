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
    $WorkingDir = Join-Path $RepoRoot "build\describe_project_template_smoke_manifest_test"
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

function Write-JsonFile {
    param([string]$Path, $Value)

    $dir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    ($Value | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-DescribeScript {
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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$scriptPath = Join-Path $resolvedRepoRoot "scripts\describe_project_template_smoke_manifest.ps1"
$fixtureRoot = Join-Path $resolvedWorkingDir "fixtures"
$inputDocxPath = Join-Path $fixtureRoot "fixtures\invoice.docx"
$artifactDir = Join-Path $fixtureRoot "artifacts\invoice"
$contactSheetPath = Join-Path $artifactDir "before_after_contact_sheet.png"
$renderedDocxPath = Join-Path $artifactDir "invoice.rendered.docx"
$manifestPath = Join-Path $fixtureRoot "project_template_smoke.manifest.json"
$summaryPath = Join-Path $fixtureRoot "summary.json"
$jsonOutputPath = Join-Path $resolvedWorkingDir "reports\manifest_description.json"
$textOutputPath = Join-Path $resolvedWorkingDir "reports\manifest_description.txt"

New-Item -ItemType Directory -Path (Split-Path -Parent $inputDocxPath) -Force | Out-Null
New-Item -ItemType Directory -Path $artifactDir -Force | Out-Null
Set-Content -LiteralPath $inputDocxPath -Encoding UTF8 -Value "placeholder docx fixture"
Set-Content -LiteralPath $contactSheetPath -Encoding UTF8 -Value "placeholder image fixture"
Set-Content -LiteralPath $renderedDocxPath -Encoding UTF8 -Value "placeholder rendered docx fixture"

Write-JsonFile -Path $manifestPath -Value ([ordered]@{
    business_template_corpus = @(
        [ordered]@{
            id = "project-finance-invoice-template"
            project_id = "project-finance"
            template_name = "invoice-template"
            document_type = "invoice"
            status = "registered"
            source_entry = "invoice-template"
            smoke_contract = @("template_validations", "schema_validation", "schema_baseline", "render_data", "visual_smoke")
            coverage_goal = "Keep invoice template smoke coverage traceable to a business corpus entry."
            notes = "Registered business corpus fixture."
        },
        [ordered]@{
            id = "project-legal-contract-template"
            project_id = "project-legal"
            template_name = "contract-template"
            document_type = "contract"
            status = "planned"
            smoke_contract = @("schema_validation", "schema_baseline", "render_data")
            registration_blocker = "No committed contract template fixture is registered yet."
            next_action = "Add a contract template manifest entry with schema baseline and render-data coverage."
            coverage_goal = "Track planned contract template coverage without adding a binary fixture."
        }
    )
    entries = @(
        [ordered]@{
            name = "invoice-template"
            project_id = "project-finance"
            template_name = "invoice-template"
            business_domain = "finance"
            business_document_type = "invoice"
            corpus_role = "registered-business-template"
            corpus_source_note = "Invoice fixture keeps business corpus metadata in the manifest description."
            input_docx = $inputDocxPath
            template_validations = @(
                [ordered]@{
                    name = "header"
                    part = "document"
                    slots = @("customer_name:text", "line_items:table_rows")
                }
            )
            schema_validation = [ordered]@{
                schema_file = "baselines/template-schema/invoice.schema.json"
                targets = @("customer_name", "line_items")
            }
            schema_baseline = [ordered]@{
                schema_file = "baselines/template-schema/invoice.schema.json"
                target_mode = "default"
                generated_output = "output/project-template-smoke/invoice.generated.schema.json"
            }
            render_data_smoke = [ordered]@{
                data_path = "samples/invoice.render_data.json"
                mapping_path = "samples/invoice.render_data_mapping.json"
                output_docx = "output/project-template-smoke/invoice.rendered.docx"
            }
            visual_smoke = [ordered]@{
                enabled = $true
                input = "rendered_docx"
                output_dir = "output/project-template-smoke/invoice-visual"
            }
        }
    )
    candidate_exclusions = @()
})

Write-JsonFile -Path $summaryPath -Value ([ordered]@{
    overall_status = "needs_review"
    passed = $false
    visual_verdict = "needs_review"
    failed_entry_count = 0
    manual_review_pending_count = 1
    visual_review_undetermined_count = 1
    entries = @(
        [ordered]@{
            name = "invoice-template"
            status = "needs_review"
            passed = $true
            manual_review_pending = $true
            artifact_dir = $artifactDir
            checks = [ordered]@{
                render_data = [ordered]@{
                    status = "completed"
                    output_docx = $renderedDocxPath
                    remaining_placeholder_count = 0
                }
                visual_smoke = [ordered]@{
                    review_status = "pending"
                    review_verdict = "needs_review"
                    findings_count = 2
                    contact_sheet = $contactSheetPath
                }
            }
        }
    )
})

$jsonResult = Invoke-DescribeScript -Arguments @(
    "-ManifestPath", $manifestPath,
    "-SummaryJson", $summaryPath,
    "-OutputPath", $jsonOutputPath,
    "-Json"
)
Assert-Equal -Actual $jsonResult.ExitCode -Expected 0 `
    -Message "JSON manifest description should pass. Output: $($jsonResult.Text)"
Assert-True -Condition (Test-Path -LiteralPath $jsonOutputPath) `
    -Message "JSON manifest description should be written to OutputPath."
Assert-ContainsText -Text $jsonResult.Text -ExpectedText "Wrote JSON report" `
    -Message "JSON run should announce the written report."

$report = Get-Content -Raw -Encoding UTF8 -LiteralPath $jsonOutputPath | ConvertFrom-Json
Assert-Equal -Actual ([string]$report.schema) -Expected "featherdoc.project_template_smoke_manifest_description.v1" `
    -Message "Description JSON should expose a stable schema."
Assert-Equal -Actual ([bool]$report.latest_summary_available) -Expected $true `
    -Message "Description JSON should record loaded summary availability."
Assert-Equal -Actual ([int]$report.entry_count) -Expected 1 `
    -Message "Description JSON should count manifest entries."
Assert-Equal -Actual ([int]$report.business_template_corpus_count) -Expected 2 `
    -Message "Description JSON should count business corpus profiles."
Assert-Equal -Actual ([int]$report.registered_business_template_corpus_count) -Expected 1 `
    -Message "Description JSON should count registered business corpus profiles."
Assert-Equal -Actual ([int]$report.planned_business_template_corpus_count) -Expected 1 `
    -Message "Description JSON should count planned business corpus profiles."
Assert-Equal -Actual ([int]$report.planned_business_template_registration_action_count) -Expected 1 `
    -Message "Description JSON should count planned corpus registration actions."
Assert-Equal -Actual ([string]$report.business_document_type_summary[0].document_type) -Expected "contract" `
    -Message "Description JSON should expose document-type summary entries."
Assert-Equal -Actual ([string]$report.business_template_corpus[0].source_entry) -Expected "invoice-template" `
    -Message "Description JSON should preserve registered source entry."
Assert-Equal -Actual ([string]$report.business_template_corpus[1].registration_blocker) -Expected "No committed contract template fixture is registered yet." `
    -Message "Description JSON should preserve planned corpus registration blockers."
Assert-Equal -Actual ([string]$report.business_template_corpus[1].next_action) -Expected "Add a contract template manifest entry with schema baseline and render-data coverage." `
    -Message "Description JSON should preserve planned corpus next actions."
Assert-Equal -Actual ([string]$report.planned_business_template_registration_actions[0].id) -Expected "project-legal-contract-template" `
    -Message "Description JSON should expose planned corpus registration action ids."
Assert-Equal -Actual ([string]$report.planned_business_template_registration_actions[0].registration_blocker) -Expected "No committed contract template fixture is registered yet." `
    -Message "Description JSON should expose planned corpus registration action blockers."
Assert-Equal -Actual ([string]$report.planned_business_template_registration_actions[0].next_action) -Expected "Add a contract template manifest entry with schema baseline and render-data coverage." `
    -Message "Description JSON should expose planned corpus registration action next steps."
Assert-Equal -Actual ([int]$report.latest_available_entry_count) -Expected 1 `
    -Message "Description JSON should count entries joined with latest summary data."
Assert-Equal -Actual ([int]$report.latest_missing_entry_count) -Expected 0 `
    -Message "Description JSON should count entries missing from latest summary."
Assert-Equal -Actual ([string]$report.latest_overall_status) -Expected "needs_review" `
    -Message "Description JSON should preserve latest overall status."
Assert-Equal -Actual ([string]$report.latest_visual_verdict) -Expected "needs_review" `
    -Message "Description JSON should preserve latest visual verdict."
Assert-Equal -Actual ([int]$report.latest_manual_review_pending_count) -Expected 1 `
    -Message "Description JSON should preserve pending review counts."

$entry = @($report.entries)[0]
Assert-Equal -Actual ([string]$entry.name) -Expected "invoice-template" `
    -Message "Description JSON should preserve entry name."
Assert-Equal -Actual ([string]$entry.project_id) -Expected "project-finance" `
    -Message "Description JSON should preserve entry project id."
Assert-Equal -Actual ([string]$entry.business_document_type) -Expected "invoice" `
    -Message "Description JSON should preserve business document type."
Assert-Equal -Actual ([string]$entry.corpus_role) -Expected "registered-business-template" `
    -Message "Description JSON should preserve corpus role."
Assert-Equal -Actual ([string]$entry.source_type) -Expected "repository-docx" `
    -Message "Description JSON should classify absolute input_docx as repository docx."
Assert-Equal -Actual ([int]$entry.template_validation_count) -Expected 1 `
    -Message "Description JSON should count template validations."
Assert-Equal -Actual ([bool]$entry.schema_validation_enabled) -Expected $true `
    -Message "Description JSON should expose schema validation state."
Assert-Equal -Actual ([int]$entry.schema_validation_target_count) -Expected 2 `
    -Message "Description JSON should count schema validation targets."
Assert-Equal -Actual ([bool]$entry.schema_baseline_enabled) -Expected $true `
    -Message "Description JSON should expose schema baseline state."
Assert-Equal -Actual ([bool]$entry.render_data_enabled) -Expected $true `
    -Message "Description JSON should expose render data state."
Assert-Equal -Actual ([string]$entry.visual_smoke_input) -Expected "rendered_docx" `
    -Message "Description JSON should expose visual smoke input source."
Assert-Equal -Actual ([bool]$entry.visual_smoke_enabled) -Expected $true `
    -Message "Description JSON should expose visual smoke state."
Assert-Equal -Actual ([bool]$entry.latest_available) -Expected $true `
    -Message "Description JSON should join the latest summary entry."
Assert-Equal -Actual ([string]$entry.latest_visual_review_status) -Expected "pending" `
    -Message "Description JSON should preserve latest visual review status."
Assert-Equal -Actual ([string]$entry.latest_render_data_status) -Expected "completed" `
    -Message "Description JSON should preserve latest render data status."
Assert-Equal -Actual ([int]$entry.latest_render_data_remaining_placeholder_count) -Expected 0 `
    -Message "Description JSON should preserve latest remaining placeholder count."
Assert-Equal -Actual ([string]$entry.latest_visual_review_verdict) -Expected "needs_review" `
    -Message "Description JSON should preserve latest visual review verdict."
Assert-Equal -Actual ([int]$entry.latest_visual_findings_count) -Expected 2 `
    -Message "Description JSON should preserve latest visual findings count."

$textResult = Invoke-DescribeScript -Arguments @(
    "-ManifestPath", $manifestPath,
    "-SummaryJson", $summaryPath,
    "-OutputPath", $textOutputPath
)
Assert-Equal -Actual $textResult.ExitCode -Expected 0 `
    -Message "Text manifest description should pass. Output: $($textResult.Text)"
Assert-True -Condition (Test-Path -LiteralPath $textOutputPath) `
    -Message "Text manifest description should be written to OutputPath."
Assert-ContainsText -Text $textResult.Text -ExpectedText "Wrote text report" `
    -Message "Text run should announce the written report."

$textReport = Get-Content -Raw -Encoding UTF8 -LiteralPath $textOutputPath
Assert-ContainsText -Text $textReport -ExpectedText "Project template smoke manifest:" `
    -Message "Text report should include the manifest heading."
Assert-ContainsText -Text $textReport -ExpectedText "Business template corpus: 2" `
    -Message "Text report should include business corpus count."
Assert-ContainsText -Text $textReport -ExpectedText "Planned business templates: 1" `
    -Message "Text report should include planned business corpus count."
Assert-ContainsText -Text $textReport -ExpectedText "Planned registration actions: 1" `
    -Message "Text report should include planned corpus registration action count."
Assert-ContainsText -Text $textReport -ExpectedText "business_document_type: invoice" `
    -Message "Text report should include entry business document type."
Assert-ContainsText -Text $textReport -ExpectedText "Business template corpus:" `
    -Message "Text report should include business corpus section."
Assert-ContainsText -Text $textReport -ExpectedText "project-legal-contract-template" `
    -Message "Text report should include planned contract corpus profile."
Assert-ContainsText -Text $textReport -ExpectedText "registration_blocker: No committed contract template fixture is registered yet." `
    -Message "Text report should include planned corpus registration blockers."
Assert-ContainsText -Text $textReport -ExpectedText "next_action: Add a contract template manifest entry with schema baseline and render-data coverage." `
    -Message "Text report should include planned corpus next actions."
Assert-ContainsText -Text $textReport -ExpectedText "Planned business template registration actions:" `
    -Message "Text report should include planned corpus registration action section."
Assert-ContainsText -Text $textReport -ExpectedText "Latest status: needs_review" `
    -Message "Text report should include latest status."
Assert-ContainsText -Text $textReport -ExpectedText "Latest visual verdict: needs_review" `
    -Message "Text report should include latest visual verdict."
Assert-ContainsText -Text $textReport -ExpectedText "latest_visual_review_status: pending" `
    -Message "Text report should include joined visual review status."
Assert-ContainsText -Text $textReport -ExpectedText "render_data_enabled: True" `
    -Message "Text report should include render data configuration."
Assert-ContainsText -Text $textReport -ExpectedText "latest_render_data_status: completed" `
    -Message "Text report should include latest render data status."
Assert-ContainsText -Text $textReport -ExpectedText "latest_visual_findings_count: 2" `
    -Message "Text report should include joined findings count."
Assert-ContainsText -Text $textReport -ExpectedText "latest_contact_sheet:" `
    -Message "Text report should include joined contact sheet path."

Write-Host "Project template smoke manifest description regression passed."
