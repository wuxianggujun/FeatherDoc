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

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedBuildDir = (Resolve-Path $BuildDir).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
$prepareScriptPath = Join-Path $resolvedRepoRoot "scripts\prepare_template_render_data_workspace.ps1"
$validateWorkspaceScriptPath = Join-Path $resolvedRepoRoot "scripts\validate_render_data_mapping.ps1"
$renderWorkspaceScriptPath = Join-Path $resolvedRepoRoot "scripts\render_template_document_from_workspace.ps1"
$sampleDocx = Join-Path $resolvedRepoRoot "samples\chinese_invoice_template.docx"

New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$workspaceDir = Join-Path $resolvedWorkingDir "invoice_workspace"
$workspaceSummary = Join-Path $workspaceDir "invoice.workspace.summary.json"
$validationFailureSummary = Join-Path $workspaceDir "invoice.workspace.validation.failed.summary.json"
$validationFailureReport = Join-Path $workspaceDir "invoice.workspace.validation.failed.report.md"
$validationSummary = Join-Path $workspaceDir "invoice.workspace.validation.summary.json"
$validationReport = Join-Path $workspaceDir "invoice.workspace.validation.report.md"
$failureSummary = Join-Path $workspaceDir "invoice.workspace.render.failed.summary.json"
$renderedDocx = Join-Path $workspaceDir "invoice.workspace.rendered.docx"
$renderSummary = Join-Path $workspaceDir "invoice.workspace.rendered.summary.json"

& $prepareScriptPath `
    -InputDocx $sampleDocx `
    -WorkspaceDir $workspaceDir `
    -SummaryJson $workspaceSummary `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "prepare_template_render_data_workspace.ps1 failed for the workspace render test."
}

$workspace = Get-Content -Raw -Encoding UTF8 -LiteralPath $workspaceSummary | ConvertFrom-Json

$validationFailed = $false
try {
    & $validateWorkspaceScriptPath `
        -WorkspaceDir $workspaceDir `
        -SummaryJson $validationFailureSummary `
        -ReportMarkdown $validationFailureReport `
        -BuildDir $resolvedBuildDir `
        -SkipBuild `
        -RequireComplete

    if ($LASTEXITCODE -ne 0) {
        $validationFailed = $true
    }
} catch {
    $validationFailed = $true
}

if (-not $validationFailed) {
    throw "validate_render_data_mapping.ps1 should fail while the workspace still contains generated placeholders."
}

$validationFailureSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $validationFailureSummary | ConvertFrom-Json
Assert-Equal -Actual $validationFailureSummaryObject.status -Expected "failed" `
    -Message "Workspace validation failure summary did not report status=failed."
Assert-Equal -Actual ([string]$validationFailureSummaryObject.workspace_dir) -Expected ([string]$workspaceDir) `
    -Message "Workspace validation failure summary should preserve the workspace directory."
Assert-True -Condition ($validationFailureSummaryObject.remaining_placeholder_count -gt 0) `
    -Message "Workspace validation should report remaining placeholders before data is edited."
Assert-True -Condition (Test-Path -LiteralPath $validationFailureReport) `
    -Message "Workspace validation failure report was not created."
$validationFailureReportText = Get-Content -Raw -Encoding UTF8 -LiteralPath $validationFailureReport
Assert-ContainsText -Text $validationFailureReportText `
    -ExpectedText "结论：❌ 未通过" `
    -Label "Workspace validation failure report"
Assert-ContainsText -Text $validationFailureReportText `
    -ExpectedText "还没补完的槽位" `
    -Label "Workspace validation failure report"
Assert-ContainsText -Text $validationFailureReportText `
    -ExpectedText '建议检查 data JSON：`customer_name`' `
    -Label "Workspace validation failure report"
Assert-ContainsText -Text $validationFailureReportText `
    -ExpectedText '建议检查 data JSON：`line_item_row`' `
    -Label "Workspace validation failure report"

$renderFailed = $false
try {
    & $renderWorkspaceScriptPath `
        -WorkspaceDir $workspaceDir `
        -OutputDocx $renderedDocx `
        -SummaryJson $failureSummary

    if ($LASTEXITCODE -ne 0) {
        $renderFailed = $true
    }
} catch {
    $renderFailed = $true
}

if (-not $renderFailed) {
    throw "render_template_document_from_workspace.ps1 should fail while the workspace still contains generated placeholders."
}

$failureSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $failureSummary | ConvertFrom-Json
Assert-Equal -Actual $failureSummaryObject.status -Expected "failed" `
    -Message "Workspace render failure summary did not report status=failed."
Assert-ContainsText -Text ([string]$failureSummaryObject.error) `
    -ExpectedText "Edit" `
    -Label "Workspace render failure summary"

Set-Content -LiteralPath $workspace.data_skeleton -Encoding UTF8 -Value @'
{
  "customer_name": "上海羽文档科技有限公司",
  "invoice_number": "报价单-2026-0410",
  "issue_date": "2026年4月10日",
  "note_lines": [
    "1. 当前工作流先编辑 JSON，再渲染 DOCX。",
    "2. workspace 入口已经把中间步骤收成两步。"
  ],
  "line_item_row": [
    [
      "需求梳理",
      "核对模板槽位、书签位置与最终交付约束",
      "3,200.00"
    ],
    [
      "文档生成",
      "落地段落替换、表格扩展与自动填充流程",
      "6,800.00"
    ]
  ]
}
'@

& $validateWorkspaceScriptPath `
    -WorkspaceDir $workspaceDir `
    -SummaryJson $validationSummary `
    -ReportMarkdown $validationReport `
    -BuildDir $resolvedBuildDir `
    -SkipBuild `
    -RequireComplete

if ($LASTEXITCODE -ne 0) {
    throw "validate_render_data_mapping.ps1 failed for the edited workspace."
}

$validationSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $validationSummary | ConvertFrom-Json
Assert-Equal -Actual $validationSummaryObject.status -Expected "completed" `
    -Message "Workspace validation summary did not report status=completed."
Assert-Equal -Actual $validationSummaryObject.remaining_placeholder_count -Expected 0 `
    -Message "Workspace validation should report no remaining placeholders after data is edited."
Assert-Equal -Actual ([string]$validationSummaryObject.workspace_summary) -Expected ([string]$workspaceSummary) `
    -Message "Workspace validation summary should resolve the workspace summary path."
Assert-True -Condition (Test-Path -LiteralPath $validationReport) `
    -Message "Workspace validation report was not created."
$validationReportText = Get-Content -Raw -Encoding UTF8 -LiteralPath $validationReport
Assert-ContainsText -Text $validationReportText `
    -ExpectedText "结论：✅ 通过" `
    -Label "Workspace validation report"
Assert-ContainsText -Text $validationReportText `
    -ExpectedText 'remaining_placeholder_count：`0`' `
    -Label "Workspace validation report"
Assert-ContainsText -Text $validationReportText `
    -ExpectedText "可以继续运行 workspace 渲染命令" `
    -Label "Workspace validation report"

& $renderWorkspaceScriptPath `
    -WorkspaceDir $workspaceDir `
    -OutputDocx $renderedDocx `
    -SummaryJson $renderSummary

if ($LASTEXITCODE -ne 0) {
    throw "render_template_document_from_workspace.ps1 failed for the edited workspace."
}

Assert-True -Condition (Test-Path -LiteralPath $renderedDocx) `
    -Message "Workspace rendered DOCX was not created."
Assert-True -Condition (Test-Path -LiteralPath $renderSummary) `
    -Message "Workspace render summary JSON was not created."

$renderSummaryObject = Get-Content -Raw -Encoding UTF8 -LiteralPath $renderSummary | ConvertFrom-Json
$documentXml = Read-DocxEntryText -DocxPath $renderedDocx -EntryName "word/document.xml"

Assert-Equal -Actual $renderSummaryObject.status -Expected "completed" `
    -Message "Workspace render summary did not report status=completed."
Assert-Equal -Actual $renderSummaryObject.operation_count -Expected 2 `
    -Message "Workspace render summary should report two wrapper steps."
Assert-Equal -Actual $renderSummaryObject.steps[0].name -Expected "resolve_workspace" `
    -Message "Workspace render summary did not record the resolve step first."
Assert-Equal -Actual $renderSummaryObject.steps[1].name -Expected "render_template_document_from_data" `
    -Message "Workspace render summary did not record the render step second."
Assert-Equal -Actual $renderSummaryObject.steps[0].status -Expected "completed" `
    -Message "Workspace resolve step did not complete."
Assert-Equal -Actual $renderSummaryObject.steps[1].status -Expected "completed" `
    -Message "Workspace render step did not complete."
Assert-ContainsText -Text $documentXml -ExpectedText "上海羽文档科技有限公司" -Label "Workspace document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "报价单-2026-0410" -Label "Workspace document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "当前工作流先编辑 JSON，再渲染 DOCX。" -Label "Workspace document.xml"
Assert-ContainsText -Text $documentXml -ExpectedText "文档生成" -Label "Workspace document.xml"
Assert-NotContainsText -Text $documentXml -UnexpectedText "TODO:" -Label "Workspace document.xml"

Write-Host "Render-from-workspace regression passed."
