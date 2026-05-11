<#
.SYNOPSIS
Prepares an editable render-data workspace for a DOCX template.

.DESCRIPTION
Runs the template-data preparation pipeline:
1. export a render-plan draft from the DOCX template,
2. export a render-data mapping draft from the render plan,
3. export an editable business-data JSON skeleton from the mapping,
4. lint the generated skeleton against the generated mapping.

The generated workspace is intended for users who want to edit JSON data and
then render a DOCX with `render_template_document_from_data.ps1`. Use
`-ExportTargetMode resolved-section-targets` when the workspace should keep
effective section header/footer targets in its render plan and mapping draft.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [string]$WorkspaceDir = "",
    [string]$RenderPlanOutput = "",
    [string]$MappingOutput = "",
    [string]$DataSkeletonOutput = "",
    [string]$SummaryJson = "",
    [string]$SourceRoot = "",
    [int]$DefaultParagraphCount = 1,
    [int]$DefaultTableRowCount = 1,
    [int]$DefaultTableCellCount = 1,
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [ValidateSet("loaded-parts", "resolved-section-targets")]
    [string]$ExportTargetMode = "loaded-parts",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[prepare-template-render-data-workspace] $Message"
}

function Resolve-OptionalWorkspacePath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    return Resolve-TemplateSchemaPath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing
}

function Ensure-Directory {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    New-Item -ItemType Directory -Path $Path -Force | Out-Null
}

function Ensure-PathParent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        Ensure-Directory -Path $directory
    }
}

function Get-PathLeafName {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ""
    }

    return [System.IO.Path]::GetFileName($Path)
}

function Read-JsonFileIfPresent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Get-OptionalObjectPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return ""
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return [string]$Object[$Name]
        }

        return ""
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property -or $null -eq $property.Value) {
        return ""
    }

    return [string]$property.Value
}

function New-WorkspaceStartHereContent {
    param(
        [string]$InputDocx,
        [string]$RenderPlanPath,
        [string]$MappingPath,
        [string]$DataSkeletonPath,
        [string]$WorkspaceSummaryPath,
        [string]$ValidationCommand,
        [string]$RenderCommand,
        [string]$RecommendedValidationSummaryPath,
        [string]$RecommendedValidationReportPath,
        [string]$RecommendedOutputDocx,
        [string]$RecommendedRenderSummaryPath
    )

    $template = @'
# START HERE

这个 workspace 不是让你去维护仓库 `docs/` 站点的。
它的用途很直接：先编辑业务数据 JSON，再生成最终的 `.docx` 文档。

## 你现在先改哪个文件？

优先编辑：`{0}`

这个 JSON 里保留了脚手架生成的 `TODO:` 占位。
把它们改成真实业务内容后，先执行下面的校验命令确认数据已经补完整。

## 哪些文件通常不用手改？

- `{1}`：mapping 草稿，描述“业务数据字段 -> 模板书签”的对应关系
- `{2}`：render plan，描述模板里有哪些可渲染槽位
- `{3}`：workspace summary，记录这次准备流程的输出和下一步命令

只有当模板结构、书签位置或数据字段设计发生变化时，才需要重新生成或调整这些文件。

## 推荐步骤

1. 先编辑 `{0}`
2. 运行 `validate_render_data_mapping.ps1`，确认 summary 里 `status=completed` 且 `remaining_placeholder_count=0`
3. 再运行 `render_template_document_from_workspace.ps1`
4. 最后打开生成的 `.docx` 检查结果

### 1) 校验你改完的数据

```powershell
{4}
```

推荐校验输出：

- 人类可读报告：`{5}`（失败时会提示建议检查的 data JSON 字段）
- 校验 summary：`{6}`

如果校验失败，优先回到 `{0}` 检查是否还有 `TODO:`、空表格行，或者数组行列数量不符合模板预期。

### 2) 生成最终 DOCX

```powershell
{7}
```

推荐输出文件：

- 最终文档：`{8}`
- 渲染 summary：`{9}`

## 这个 workspace 当前对应什么模板？

- 模板文件：`{10}`
- 数据文件：`{0}`
- mapping 文件：`{1}`
- render plan 文件：`{2}`

如果渲染时报错说仍然存在 `TODO:` 或空表格行，先回到 `{0}` 继续补全内容，再重新渲染。
'@

    return $template -f `
        (Get-PathLeafName -Path $DataSkeletonPath), `
        (Get-PathLeafName -Path $MappingPath), `
        (Get-PathLeafName -Path $RenderPlanPath), `
        (Get-PathLeafName -Path $WorkspaceSummaryPath), `
        $ValidationCommand, `
        (Get-PathLeafName -Path $RecommendedValidationReportPath), `
        (Get-PathLeafName -Path $RecommendedValidationSummaryPath), `
        $RenderCommand, `
        (Get-PathLeafName -Path $RecommendedOutputDocx), `
        (Get-PathLeafName -Path $RecommendedRenderSummaryPath), `
        (Get-PathLeafName -Path $InputDocx)
}

function Write-WorkspaceStartHereGuide {
    param(
        [string]$GuidePath,
        [string]$InputDocx,
        [string]$RenderPlanPath,
        [string]$MappingPath,
        [string]$DataSkeletonPath,
        [string]$WorkspaceSummaryPath,
        [string]$ValidationCommand,
        [string]$RenderCommand,
        [string]$RecommendedValidationSummaryPath,
        [string]$RecommendedValidationReportPath,
        [string]$RecommendedOutputDocx,
        [string]$RecommendedRenderSummaryPath
    )

    if ([string]::IsNullOrWhiteSpace($GuidePath)) {
        return
    }

    Ensure-PathParent -Path $GuidePath
    $content = New-WorkspaceStartHereContent `
        -InputDocx $InputDocx `
        -RenderPlanPath $RenderPlanPath `
        -MappingPath $MappingPath `
        -DataSkeletonPath $DataSkeletonPath `
        -WorkspaceSummaryPath $WorkspaceSummaryPath `
        -ValidationCommand $ValidationCommand `
        -RenderCommand $RenderCommand `
        -RecommendedValidationSummaryPath $RecommendedValidationSummaryPath `
        -RecommendedValidationReportPath $RecommendedValidationReportPath `
        -RecommendedOutputDocx $RecommendedOutputDocx `
        -RecommendedRenderSummaryPath $RecommendedRenderSummaryPath
    Set-Content -LiteralPath $GuidePath -Encoding UTF8 -Value $content
}

function New-StepRecord {
    param(
        [int]$Index,
        [string]$Name,
        [string]$ScriptPath,
        [string]$SummaryJsonPath,
        [string]$OutputPath,
        [bool]$Started,
        [object]$SummaryObject
    )

    $status = if ($null -ne $SummaryObject) {
        $summaryStatus = Get-OptionalObjectPropertyValue -Object $SummaryObject -Name "status"
        if ([string]::IsNullOrWhiteSpace($summaryStatus)) {
            if ($Started) { "failed" } else { "pending" }
        } else {
            $summaryStatus
        }
    } elseif ($Started) {
        "failed"
    } else {
        "pending"
    }

    return [ordered]@{
        index = $Index
        name = $Name
        script_path = $ScriptPath
        summary_json = $SummaryJsonPath
        output_path = $OutputPath
        started = $Started
        status = $status
        summary = $SummaryObject
    }
}

function Build-WorkspaceSummary {
    param(
        [string]$Status,
        [string]$InputDocx,
        [string]$WorkspaceDir,
        [string]$RenderPlanPath,
        [string]$MappingPath,
        [string]$DataSkeletonPath,
        [string]$StartHerePath,
        [string]$SummaryJsonPath,
        [string]$ValidationSummaryPath,
        [string]$ValidationReportPath,
        [string]$ValidationCommand,
        [string]$RenderCommand,
        [string]$BuildDir,
        [string]$Generator,
        [bool]$SkipBuild,
        [string]$ExportTargetMode,
        [string]$SourceRoot,
        [int]$DefaultParagraphCount,
        [int]$DefaultTableRowCount,
        [int]$DefaultTableCellCount,
        [object[]]$Steps,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        workspace_dir = $WorkspaceDir
        render_plan = $RenderPlanPath
        mapping = $MappingPath
        data_skeleton = $DataSkeletonPath
        start_here = $StartHerePath
        summary_json = $SummaryJsonPath
        validation_summary = $ValidationSummaryPath
        validation_report = $ValidationReportPath
        build_dir = $BuildDir
        generator = $Generator
        skip_build = $SkipBuild
        export_target_mode = $ExportTargetMode
        source_root = $SourceRoot
        default_paragraph_count = $DefaultParagraphCount
        default_table_row_count = $DefaultTableRowCount
        default_table_cell_count = $DefaultTableCellCount
        operation_count = $Steps.Count
        steps = $Steps
        workflow = [ordered]@{
            purpose = "Edit JSON business data and render a DOCX from the template."
            edit_data = $DataSkeletonPath
            start_here = $StartHerePath
            validate_after_edit = "scripts/validate_render_data_mapping.ps1"
            render_after_edit = "scripts/render_template_document_from_workspace.ps1"
        }
        next_commands = [ordered]@{
            edit_data = $DataSkeletonPath
            validate_data = $ValidationCommand
            render_docx = $RenderCommand
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary['error'] = $ErrorMessage
    }

    return $summary
}

if ($DefaultParagraphCount -lt 1) {
    throw "DefaultParagraphCount must be at least 1."
}
if ($DefaultTableRowCount -lt 1) {
    throw "DefaultTableRowCount must be at least 1."
}
if ($DefaultTableCellCount -lt 1) {
    throw "DefaultTableCellCount must be at least 1."
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$inputStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedInputDocx)
$resolvedWorkspaceDir = if ([string]::IsNullOrWhiteSpace($WorkspaceDir)) {
    Join-Path ([System.IO.Path]::GetDirectoryName($resolvedInputDocx)) ($inputStem + ".render_data_workspace")
} else {
    Resolve-OptionalWorkspacePath -RepoRoot $repoRoot -InputPath $WorkspaceDir
}

$resolvedRenderPlanOutput = if ([string]::IsNullOrWhiteSpace($RenderPlanOutput)) {
    Join-Path $resolvedWorkspaceDir ($inputStem + ".render-plan.json")
} else {
    Resolve-OptionalWorkspacePath -RepoRoot $repoRoot -InputPath $RenderPlanOutput
}
$resolvedMappingOutput = if ([string]::IsNullOrWhiteSpace($MappingOutput)) {
    Join-Path $resolvedWorkspaceDir ($inputStem + ".render_data_mapping.draft.json")
} else {
    Resolve-OptionalWorkspacePath -RepoRoot $repoRoot -InputPath $MappingOutput
}
$resolvedDataSkeletonOutput = if ([string]::IsNullOrWhiteSpace($DataSkeletonOutput)) {
    Join-Path $resolvedWorkspaceDir ($inputStem + ".render_data.skeleton.json")
} else {
    Resolve-OptionalWorkspacePath -RepoRoot $repoRoot -InputPath $DataSkeletonOutput
}
$resolvedSummaryJson = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedWorkspaceDir ($inputStem + ".render_data_workspace.summary.json")
} else {
    Resolve-OptionalWorkspacePath -RepoRoot $repoRoot -InputPath $SummaryJson
}
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalWorkspacePath -RepoRoot $repoRoot -InputPath $BuildDir
}
$startHerePath = Join-Path $resolvedWorkspaceDir "START_HERE.zh-CN.md"
$recommendedValidationSummary = Join-Path $resolvedWorkspaceDir ($inputStem + ".validation.summary.json")
$recommendedValidationReport = Join-Path $resolvedWorkspaceDir ($inputStem + ".validation.report.md")
$recommendedRenderedDocx = Join-Path $resolvedWorkspaceDir ($inputStem + ".final.docx")
$recommendedRenderSummary = Join-Path $resolvedWorkspaceDir ($inputStem + ".render.summary.json")
$validateWorkspaceDataCommand = (
    'pwsh -ExecutionPolicy Bypass -File .\scripts\validate_render_data_mapping.ps1 ' +
    '-WorkspaceDir "{0}" -SummaryJson "{1}" -ReportMarkdown "{2}" ' +
    '-BuildDir "{3}" -Generator "{4}" -ExportTargetMode "{5}" -RequireComplete'
) -f `
    $resolvedWorkspaceDir, `
    $recommendedValidationSummary, `
    $recommendedValidationReport, `
    $resolvedBuildDir, `
    $Generator, `
    $ExportTargetMode
if ($SkipBuild) {
    $validateWorkspaceDataCommand += " -SkipBuild"
}
$renderWorkspaceCommand = (
    'pwsh -ExecutionPolicy Bypass -File .\scripts\render_template_document_from_workspace.ps1 ' +
    '-WorkspaceDir "{0}" -OutputDocx "{1}" -SummaryJson "{2}" -ExportTargetMode "{3}"'
) -f $resolvedWorkspaceDir, $recommendedRenderedDocx, $recommendedRenderSummary, $ExportTargetMode

$renderPlanSummary = Join-Path $resolvedWorkspaceDir ($inputStem + ".render-plan.summary.json")
$mappingSummary = Join-Path $resolvedWorkspaceDir ($inputStem + ".render_data_mapping.draft.summary.json")
$skeletonSummary = Join-Path $resolvedWorkspaceDir ($inputStem + ".render_data.skeleton.summary.json")
$lintSummary = Join-Path $resolvedWorkspaceDir ($inputStem + ".render_data.skeleton.lint.summary.json")

$exportPlanScriptPath = Join-Path $repoRoot "scripts\export_template_render_plan.ps1"
$mappingDraftScriptPath = Join-Path $repoRoot "scripts\export_render_data_mapping_draft.ps1"
$skeletonScriptPath = Join-Path $repoRoot "scripts\export_render_data_skeleton.ps1"
$lintScriptPath = Join-Path $repoRoot "scripts\lint_render_data_mapping.ps1"

Ensure-Directory -Path $resolvedWorkspaceDir
foreach ($path in @(
        $resolvedRenderPlanOutput,
        $resolvedMappingOutput,
        $resolvedDataSkeletonOutput,
        $resolvedSummaryJson,
        $renderPlanSummary,
        $mappingSummary,
        $skeletonSummary,
        $lintSummary
    )) {
    Ensure-PathParent -Path $path
}

$status = "failed"
$failureMessage = ""
$exportStarted = $false
$mappingStarted = $false
$skeletonStarted = $false
$lintStarted = $false
$exportSummaryObject = $null
$mappingSummaryObject = $null
$skeletonSummaryObject = $null
$lintSummaryObject = $null

try {
    Write-Step "Exporting render-plan draft"
    $exportStarted = $true
    & $exportPlanScriptPath `
        -InputDocx $resolvedInputDocx `
        -OutputPlan $resolvedRenderPlanOutput `
        -SummaryJson $renderPlanSummary `
        -BuildDir $resolvedBuildDir `
        -Generator $Generator `
        -TargetMode $ExportTargetMode `
        -SkipBuild:$SkipBuild
    $exportSummaryObject = Read-JsonFileIfPresent -Path $renderPlanSummary
    if ($LASTEXITCODE -ne 0) {
        throw "export_template_render_plan.ps1 failed."
    }

    Write-Step "Exporting render-data mapping draft"
    $mappingStarted = $true
    & $mappingDraftScriptPath `
        -InputPlan $resolvedRenderPlanOutput `
        -OutputMapping $resolvedMappingOutput `
        -SummaryJson $mappingSummary `
        -SourceRoot $SourceRoot
    $mappingSummaryObject = Read-JsonFileIfPresent -Path $mappingSummary
    if ($LASTEXITCODE -ne 0) {
        throw "export_render_data_mapping_draft.ps1 failed."
    }

    Write-Step "Exporting editable render-data skeleton"
    $skeletonStarted = $true
    & $skeletonScriptPath `
        -MappingPath $resolvedMappingOutput `
        -OutputData $resolvedDataSkeletonOutput `
        -SummaryJson $skeletonSummary `
        -DefaultParagraphCount $DefaultParagraphCount `
        -DefaultTableRowCount $DefaultTableRowCount `
        -DefaultTableCellCount $DefaultTableCellCount
    $skeletonSummaryObject = Read-JsonFileIfPresent -Path $skeletonSummary
    if ($LASTEXITCODE -ne 0) {
        throw "export_render_data_skeleton.ps1 failed."
    }

    Write-Step "Linting generated mapping and data skeleton"
    $lintStarted = $true
    & $lintScriptPath `
        -MappingPath $resolvedMappingOutput `
        -DataPath $resolvedDataSkeletonOutput `
        -SummaryJson $lintSummary
    $lintSummaryObject = Read-JsonFileIfPresent -Path $lintSummary
    if ($LASTEXITCODE -ne 0) {
        throw "lint_render_data_mapping.ps1 failed."
    }

    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    throw
} finally {
    if ($null -eq $exportSummaryObject) {
        $exportSummaryObject = Read-JsonFileIfPresent -Path $renderPlanSummary
    }
    if ($null -eq $mappingSummaryObject) {
        $mappingSummaryObject = Read-JsonFileIfPresent -Path $mappingSummary
    }
    if ($null -eq $skeletonSummaryObject) {
        $skeletonSummaryObject = Read-JsonFileIfPresent -Path $skeletonSummary
    }
    if ($null -eq $lintSummaryObject) {
        $lintSummaryObject = Read-JsonFileIfPresent -Path $lintSummary
    }

    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $writtenStartHerePath = ""
        if ($status -eq "completed") {
            Write-WorkspaceStartHereGuide `
                -GuidePath $startHerePath `
                -InputDocx $resolvedInputDocx `
                -RenderPlanPath $resolvedRenderPlanOutput `
                -MappingPath $resolvedMappingOutput `
                -DataSkeletonPath $resolvedDataSkeletonOutput `
                -WorkspaceSummaryPath $resolvedSummaryJson `
                -ValidationCommand $validateWorkspaceDataCommand `
                -RenderCommand $renderWorkspaceCommand `
                -RecommendedValidationSummaryPath $recommendedValidationSummary `
                -RecommendedValidationReportPath $recommendedValidationReport `
                -RecommendedOutputDocx $recommendedRenderedDocx `
                -RecommendedRenderSummaryPath $recommendedRenderSummary
            $writtenStartHerePath = $startHerePath
        }

        $steps = @(
            [pscustomobject](New-StepRecord `
                -Index 1 `
                -Name "export_template_render_plan" `
                -ScriptPath $exportPlanScriptPath `
                -SummaryJsonPath $renderPlanSummary `
                -OutputPath $resolvedRenderPlanOutput `
                -Started $exportStarted `
                -SummaryObject $exportSummaryObject),
            [pscustomobject](New-StepRecord `
                -Index 2 `
                -Name "export_render_data_mapping_draft" `
                -ScriptPath $mappingDraftScriptPath `
                -SummaryJsonPath $mappingSummary `
                -OutputPath $resolvedMappingOutput `
                -Started $mappingStarted `
                -SummaryObject $mappingSummaryObject),
            [pscustomobject](New-StepRecord `
                -Index 3 `
                -Name "export_render_data_skeleton" `
                -ScriptPath $skeletonScriptPath `
                -SummaryJsonPath $skeletonSummary `
                -OutputPath $resolvedDataSkeletonOutput `
                -Started $skeletonStarted `
                -SummaryObject $skeletonSummaryObject),
            [pscustomobject](New-StepRecord `
                -Index 4 `
                -Name "lint_render_data_mapping" `
                -ScriptPath $lintScriptPath `
                -SummaryJsonPath $lintSummary `
                -OutputPath $lintSummary `
                -Started $lintStarted `
                -SummaryObject $lintSummaryObject)
        )

        $summary = Build-WorkspaceSummary `
            -Status $status `
            -InputDocx $resolvedInputDocx `
            -WorkspaceDir $resolvedWorkspaceDir `
            -RenderPlanPath $resolvedRenderPlanOutput `
            -MappingPath $resolvedMappingOutput `
            -DataSkeletonPath $resolvedDataSkeletonOutput `
            -StartHerePath $writtenStartHerePath `
            -SummaryJsonPath $resolvedSummaryJson `
            -ValidationSummaryPath $recommendedValidationSummary `
            -ValidationReportPath $recommendedValidationReport `
            -ValidationCommand $validateWorkspaceDataCommand `
            -RenderCommand $renderWorkspaceCommand `
            -BuildDir $resolvedBuildDir `
            -Generator $Generator `
            -SkipBuild ([bool]$SkipBuild) `
            -ExportTargetMode $ExportTargetMode `
            -SourceRoot $SourceRoot `
            -DefaultParagraphCount $DefaultParagraphCount `
            -DefaultTableRowCount $DefaultTableRowCount `
            -DefaultTableCellCount $DefaultTableCellCount `
            -Steps $steps `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }
}

Write-Step "Workspace ready: $resolvedWorkspaceDir"
Write-Step "Render plan: $resolvedRenderPlanOutput"
Write-Step "Mapping draft: $resolvedMappingOutput"
Write-Step "Data skeleton: $resolvedDataSkeletonOutput"
if (Test-Path -LiteralPath $startHerePath) {
    Write-Step "Start here: $startHerePath"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
