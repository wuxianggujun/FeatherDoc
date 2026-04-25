param(
    [string]$RepoRoot,
    [string]$BuildDir,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-True {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

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
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$scriptPath = Join-Path $resolvedRepoRoot "scripts\prepare_template_render_data_workspace.ps1"
$convertScriptPath = Join-Path $resolvedRepoRoot "scripts\convert_render_data_to_patch_plan.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$workspaceDir = Join-Path $resolvedWorkingDir "invoice_workspace"
$summaryPath = Join-Path $workspaceDir "invoice.workspace.summary.json"

& $scriptPath `
    -InputDocx $sampleDocx `
    -WorkspaceDir $workspaceDir `
    -SummaryJson $summaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild `
    -DefaultParagraphCount 2 `
    -DefaultTableRowCount 2 `
    -DefaultTableCellCount 2

if ($LASTEXITCODE -ne 0) {
    throw "prepare_template_render_data_workspace.ps1 failed for the invoice sample."
}

Assert-True -Condition (Test-Path -LiteralPath $summaryPath) `
    -Message "Workspace summary JSON was not created."

$summary = Get-Content -Raw -Encoding UTF8 -LiteralPath $summaryPath | ConvertFrom-Json

Assert-Equal -Actual $summary.status -Expected "completed" `
    -Message "Workspace summary did not report status=completed."
Assert-Equal -Actual $summary.operation_count -Expected 4 `
    -Message "Workspace summary should record four pipeline steps."
Assert-Equal -Actual $summary.steps[0].status -Expected "completed" `
    -Message "Render-plan export step did not complete."
Assert-Equal -Actual $summary.steps[1].status -Expected "completed" `
    -Message "Mapping draft export step did not complete."
Assert-Equal -Actual $summary.steps[2].status -Expected "completed" `
    -Message "Data skeleton export step did not complete."
Assert-Equal -Actual $summary.steps[3].status -Expected "completed" `
    -Message "Generated skeleton lint step did not complete."
Assert-Equal -Actual ([string]$summary.workflow.start_here) -Expected ([string]$summary.start_here) `
    -Message "Workspace summary should expose the same start_here path in workflow metadata."
Assert-Equal -Actual ([string]$summary.workflow.validate_after_edit) `
    -Expected "scripts/validate_render_data_mapping.ps1" `
    -Message "Workspace summary should expose the validation wrapper in workflow metadata."

Assert-True -Condition (Test-Path -LiteralPath $summary.render_plan) `
    -Message "Workspace render-plan output was not created."
Assert-True -Condition (Test-Path -LiteralPath $summary.mapping) `
    -Message "Workspace mapping draft output was not created."
Assert-True -Condition (Test-Path -LiteralPath $summary.data_skeleton) `
    -Message "Workspace data skeleton output was not created."
Assert-True -Condition (Test-Path -LiteralPath $summary.start_here) `
    -Message "Workspace start-here guide was not created."
Assert-ContainsText -Text ([string]$summary.next_commands.render_docx) `
    -ExpectedText "-WorkspaceDir" `
    -Message "Workspace summary render command should use the workspace directory."
Assert-ContainsText -Text ([string]$summary.next_commands.render_docx) `
    -ExpectedText "render_template_document_from_workspace.ps1" `
    -Message "Workspace summary render command should point to the workspace wrapper."
Assert-ContainsText -Text ([string]$summary.next_commands.validate_data) `
    -ExpectedText "validate_render_data_mapping.ps1" `
    -Message "Workspace summary validation command should point to the validation wrapper."
Assert-ContainsText -Text ([string]$summary.next_commands.validate_data) `
    -ExpectedText "-WorkspaceDir" `
    -Message "Workspace summary validation command should use the workspace directory."
Assert-ContainsText -Text ([string]$summary.next_commands.validate_data) `
    -ExpectedText "-RequireComplete" `
    -Message "Workspace summary validation command should require complete data."
Assert-ContainsText -Text ([string]$summary.next_commands.validate_data) `
    -ExpectedText "-ReportMarkdown" `
    -Message "Workspace summary validation command should write a readable report."
Assert-ContainsText -Text ([string]$summary.validation_summary) `
    -ExpectedText ".validation.summary.json" `
    -Message "Workspace summary should provide a recommended validation summary path."
Assert-ContainsText -Text ([string]$summary.validation_report) `
    -ExpectedText ".validation.report.md" `
    -Message "Workspace summary should provide a recommended validation report path."

$renderPlan = Get-Content -Raw -Encoding UTF8 -LiteralPath $summary.render_plan | ConvertFrom-Json
$mapping = Get-Content -Raw -Encoding UTF8 -LiteralPath $summary.mapping | ConvertFrom-Json
$data = Get-Content -Raw -Encoding UTF8 -LiteralPath $summary.data_skeleton | ConvertFrom-Json
$startHere = Get-Content -Raw -Encoding UTF8 -LiteralPath $summary.start_here

Assert-Equal -Actual @($renderPlan.bookmark_text).Count -Expected 3 `
    -Message "Workspace render-plan should include three text bookmarks."
Assert-Equal -Actual @($renderPlan.bookmark_paragraphs).Count -Expected 1 `
    -Message "Workspace render-plan should include one paragraph bookmark."
Assert-Equal -Actual @($renderPlan.bookmark_table_rows).Count -Expected 1 `
    -Message "Workspace render-plan should include one table-row bookmark."
Assert-Equal -Actual @($mapping.bookmark_text).Count -Expected 3 `
    -Message "Workspace mapping should include three text mappings."
Assert-Equal -Actual @($mapping.bookmark_paragraphs).Count -Expected 1 `
    -Message "Workspace mapping should include one paragraph mapping."
Assert-Equal -Actual @($mapping.bookmark_table_rows).Count -Expected 1 `
    -Message "Workspace mapping should include one table-row mapping."

Assert-Equal -Actual $data.customer_name -Expected "TODO: customer_name" `
    -Message "Workspace data skeleton did not include customer_name."
Assert-Equal -Actual $data.invoice_number -Expected "TODO: invoice_number" `
    -Message "Workspace data skeleton did not include invoice_number."
Assert-Equal -Actual @($data.note_lines).Count -Expected 2 `
    -Message "Workspace data skeleton should include two note placeholders."
Assert-Equal -Actual @($data.note_lines)[1] -Expected "TODO: note_lines[1]" `
    -Message "Workspace data skeleton did not include the second note placeholder."
Assert-Equal -Actual @($data.line_item_row).Count -Expected 2 `
    -Message "Workspace data skeleton should include two table rows."
Assert-Equal -Actual @(@($data.line_item_row)[0]).Count -Expected 2 `
    -Message "Workspace data skeleton should include two default table cells."
Assert-ContainsText -Text $startHere `
    -ExpectedText ([System.IO.Path]::GetFileName([string]$summary.data_skeleton)) `
    -Message "Workspace start-here guide should mention the data skeleton file."
Assert-ContainsText -Text $startHere `
    -ExpectedText ([System.IO.Path]::GetFileName([string]$summary.mapping)) `
    -Message "Workspace start-here guide should mention the mapping file."
Assert-ContainsText -Text $startHere `
    -ExpectedText ([System.IO.Path]::GetFileName([string]$summary.render_plan)) `
    -Message "Workspace start-here guide should mention the render-plan file."
Assert-ContainsText -Text $startHere `
    -ExpectedText "render_template_document_from_workspace.ps1" `
    -Message "Workspace start-here guide should mention the render wrapper."
Assert-ContainsText -Text $startHere `
    -ExpectedText "validate_render_data_mapping.ps1" `
    -Message "Workspace start-here guide should mention the validation wrapper."
Assert-ContainsText -Text $startHere `
    -ExpectedText "remaining_placeholder_count=0" `
    -Message "Workspace start-here guide should explain the validation success signal."
Assert-ContainsText -Text $startHere `
    -ExpectedText ([System.IO.Path]::GetFileName([string]$summary.validation_summary)) `
    -Message "Workspace start-here guide should mention the validation summary file."
Assert-ContainsText -Text $startHere `
    -ExpectedText ([System.IO.Path]::GetFileName([string]$summary.validation_report)) `
    -Message "Workspace start-here guide should mention the validation report file."
Assert-ContainsText -Text $startHere `
    -ExpectedText "建议检查的 data JSON 字段" `
    -Message "Workspace start-here guide should explain what the validation report points to."
Assert-ContainsText -Text $startHere `
    -ExpectedText "先编辑业务数据 JSON" `
    -Message "Workspace start-here guide should explain the JSON-first workflow."
Assert-ContainsText -Text $startHere `
    -ExpectedText '最终的 `.docx` 文档' `
    -Message "Workspace start-here guide should explain the final DOCX render goal."

$patchOutput = Join-Path $workspaceDir "invoice.workspace.patch.json"
$patchSummary = Join-Path $workspaceDir "invoice.workspace.patch.summary.json"

& $convertScriptPath `
    -DataPath $summary.data_skeleton `
    -MappingPath $summary.mapping `
    -OutputPatch $patchOutput `
    -SummaryJson $patchSummary

if ($LASTEXITCODE -ne 0) {
    throw "Workspace data skeleton should convert to a render patch."
}

$patch = Get-Content -Raw -Encoding UTF8 -LiteralPath $patchOutput | ConvertFrom-Json
Assert-Equal -Actual $patch.bookmark_paragraphs[0].paragraphs[1] `
    -Expected "TODO: note_lines[1]" `
    -Message "Workspace patch did not preserve generated note placeholders."
Assert-Equal -Actual $patch.bookmark_table_rows[0].rows[0][1] `
    -Expected "TODO: line_item_row.row[0].cell[1]" `
    -Message "Workspace patch did not preserve generated table cell placeholders."

Write-Host "Template render-data workspace preparation regression passed."
