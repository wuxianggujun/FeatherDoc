<#
.SYNOPSIS
Validates render-data mapping and business data against a DOCX template.

.DESCRIPTION
Runs `export_template_render_plan.ps1`,
`convert_render_data_to_patch_plan.ps1`, and
`patch_template_render_plan.ps1` in sequence so mapping/data mismatches fail
before document rendering starts.
#>
param(
    [string]$WorkspaceDir = "",
    [string]$WorkspaceSummary = "",
    [string]$InputDocx = "",
    [string]$MappingPath = "",
    [string]$DataPath = "",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [switch]$SkipBuild,
    [string]$DraftPlanOutput = "",
    [string]$GeneratedPatchOutput = "",
    [string]$PatchedPlanOutput = "",
    [switch]$RequireComplete
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[validate-render-data-mapping] $Message"
}

function Resolve-OptionalValidationPath {
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

function Ensure-PathParent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
}

function Read-JsonFileIfPresent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path) -or
        -not (Test-Path -LiteralPath $Path)) {
        return $null
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $Path | ConvertFrom-Json
}

function Find-WorkspaceCandidatePath {
    param(
        [string]$DirectoryPath,
        [string[]]$Patterns,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($DirectoryPath) -or
        -not (Test-Path -LiteralPath $DirectoryPath)) {
        return ""
    }

    $candidates = New-Object 'System.Collections.Generic.List[string]'
    foreach ($item in (Get-ChildItem -LiteralPath $DirectoryPath -File)) {
        foreach ($pattern in $Patterns) {
            if ($item.Name -like $pattern) {
                $candidates.Add($item.FullName) | Out-Null
                break
            }
        }
    }

    $distinct = @($candidates | Sort-Object -Unique)
    if ($distinct.Count -eq 0) {
        return ""
    }
    if ($distinct.Count -gt 1) {
        throw "Workspace contains multiple $Label candidates: $($distinct -join ', ')."
    }

    return $distinct[0]
}

function Resolve-WorkspaceSummaryPath {
    param(
        [string]$RepoRoot,
        [string]$WorkspaceSummary,
        [string]$WorkspaceDir
    )

    if (-not [string]::IsNullOrWhiteSpace($WorkspaceSummary)) {
        return Resolve-TemplateSchemaPath `
            -RepoRoot $RepoRoot `
            -InputPath $WorkspaceSummary
    }

    if ([string]::IsNullOrWhiteSpace($WorkspaceDir)) {
        return ""
    }

    $summaryPath = Find-WorkspaceCandidatePath `
        -DirectoryPath $WorkspaceDir `
        -Patterns @("summary.json", "*.render_data_workspace.summary.json", "*.workspace.summary.json") `
        -Label "workspace summary"
    if (-not [string]::IsNullOrWhiteSpace($summaryPath)) {
        return $summaryPath
    }

    return ""
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

function Get-OptionalObjectPropertyObject {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }

        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-ValidationReportPatchCountText {
    param(
        $Summary,
        [string]$Category
    )

    $patchCounts = Get-OptionalObjectPropertyObject -Object $Summary -Name "patch_counts"
    $categoryCounts = Get-OptionalObjectPropertyObject -Object $patchCounts -Name $Category
    $requested = Get-OptionalObjectPropertyValue -Object $categoryCounts -Name "requested"
    $applied = Get-OptionalObjectPropertyValue -Object $categoryCounts -Name "applied"
    if ([string]::IsNullOrWhiteSpace($requested)) {
        $requested = "0"
    }
    if ([string]::IsNullOrWhiteSpace($applied)) {
        $applied = "0"
    }

    return "applied=$applied, requested=$requested"
}

function Get-ValidationReportObjectArrayProperty {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return @()
    }
    if ($value -is [string]) {
        return @($value)
    }
    if ($value -is [System.Collections.IEnumerable] -and
        -not ($value -is [System.Collections.IDictionary]) -and
        -not ($value -is [System.Management.Automation.PSCustomObject])) {
        return @($value | Where-Object { $null -ne $_ })
    }

    return @($value)
}

function Find-ValidationReportMappingEntry {
    param(
        $Mapping,
        [string]$Category,
        [string]$BookmarkName
    )

    if ($null -eq $Mapping -or [string]::IsNullOrWhiteSpace($BookmarkName)) {
        return $null
    }

    foreach ($entry in @(Get-ValidationReportObjectArrayProperty -Object $Mapping -Name $Category)) {
        $entryBookmarkName = Get-OptionalObjectPropertyValue -Object $entry -Name "bookmark_name"
        if ([string]::IsNullOrWhiteSpace($entryBookmarkName)) {
            $entryBookmarkName = Get-OptionalObjectPropertyValue -Object $entry -Name "bookmark"
        }
        if ($entryBookmarkName -eq $BookmarkName) {
            return $entry
        }
    }

    return $null
}

function Get-ValidationReportDataHintText {
    param(
        $Mapping,
        [string]$Category,
        [string]$BookmarkName
    )

    $entry = Find-ValidationReportMappingEntry `
        -Mapping $Mapping `
        -Category $Category `
        -BookmarkName $BookmarkName
    $source = Get-OptionalObjectPropertyValue -Object $entry -Name "source"
    if ([string]::IsNullOrWhiteSpace($source)) {
        return ""
    }

    $columns = @(Get-ValidationReportObjectArrayProperty -Object $entry -Name "columns" |
        ForEach-Object { [string]$_ } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($columns.Count -gt 0) {
        return ('；建议检查 data JSON：`{0}`，columns：`{1}`' -f $source, ($columns -join '`, `'))
    }

    return ('；建议检查 data JSON：`{0}`' -f $source)
}

function Get-ValidationReportPlaceholderLine {
    param(
        [string]$Category,
        $Placeholder,
        $Mapping
    )

    $bookmarkName = Get-OptionalObjectPropertyValue -Object $Placeholder -Name "bookmark_name"
    $part = Get-OptionalObjectPropertyValue -Object $Placeholder -Name "part"
    $index = Get-OptionalObjectPropertyValue -Object $Placeholder -Name "index"
    $section = Get-OptionalObjectPropertyValue -Object $Placeholder -Name "section"
    $kind = Get-OptionalObjectPropertyValue -Object $Placeholder -Name "kind"

    $details = New-Object 'System.Collections.Generic.List[string]'
    if (-not [string]::IsNullOrWhiteSpace($part)) {
        $details.Add("part=$part") | Out-Null
    }
    if (-not [string]::IsNullOrWhiteSpace($index)) {
        $details.Add("index=$index") | Out-Null
    }
    if (-not [string]::IsNullOrWhiteSpace($section)) {
        $details.Add("section=$section") | Out-Null
    }
    if (-not [string]::IsNullOrWhiteSpace($kind)) {
        $details.Add("kind=$kind") | Out-Null
    }

    $detailText = if ($details.Count -gt 0) {
        " (" + ($details -join ", ") + ")"
    } else {
        ""
    }

    $dataHintText = Get-ValidationReportDataHintText `
        -Mapping $Mapping `
        -Category $Category `
        -BookmarkName $bookmarkName

    return ('- {0}：`{1}`{2}{3}' -f $Category, $bookmarkName, $detailText, $dataHintText)
}

function Get-ValidationReportMappingHintLine {
    param(
        [string]$Category,
        $Entry
    )

    $bookmarkName = Get-OptionalObjectPropertyValue -Object $Entry -Name "bookmark_name"
    if ([string]::IsNullOrWhiteSpace($bookmarkName)) {
        $bookmarkName = Get-OptionalObjectPropertyValue -Object $Entry -Name "bookmark"
    }
    if ([string]::IsNullOrWhiteSpace($bookmarkName)) {
        return ""
    }

    $source = Get-OptionalObjectPropertyValue -Object $Entry -Name "source"
    if ([string]::IsNullOrWhiteSpace($source)) {
        return ('- {0}：`{1}`' -f $Category, $bookmarkName)
    }

    $columns = @(Get-ValidationReportObjectArrayProperty -Object $Entry -Name "columns" |
        ForEach-Object { [string]$_ } |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    if ($columns.Count -gt 0) {
        return ('- {0}：`{1}`；建议检查 data JSON：`{2}`，columns：`{3}`' -f `
            $Category, `
            $bookmarkName, `
            $source, `
            ($columns -join '`, `'))
    }

    return ('- {0}：`{1}`；建议检查 data JSON：`{2}`' -f $Category, $bookmarkName, $source)
}

function New-ValidationReportMarkdown {
    param(
        $Summary,
        $Mapping
    )

    $status = Get-OptionalObjectPropertyValue -Object $Summary -Name "status"
    $remainingPlaceholderText = Get-OptionalObjectPropertyValue `
        -Object $Summary `
        -Name "remaining_placeholder_count"
    if ([string]::IsNullOrWhiteSpace($remainingPlaceholderText)) {
        $remainingPlaceholderText = "0"
    }

    $passed = ($status -eq "completed" -and [int]$remainingPlaceholderText -eq 0)
    $verdict = if ($passed) { "✅ 通过" } else { "❌ 未通过" }
    $nextAction = if ($passed) {
        '可以继续运行 workspace 渲染命令，生成最终 `.docx`。'
    } else {
        '先回到 data JSON，补齐 `TODO:`、空表格行或缺失数组内容，再重新校验。'
    }
    $requireCompleteText = Get-OptionalObjectPropertyValue -Object $Summary -Name "require_complete"
    $dataPathText = Get-OptionalObjectPropertyValue -Object $Summary -Name "data_path"
    $mappingPathText = Get-OptionalObjectPropertyValue -Object $Summary -Name "mapping_path"
    $inputDocxText = Get-OptionalObjectPropertyValue -Object $Summary -Name "input_docx"

    $lines = New-Object 'System.Collections.Generic.List[string]'
    [void]$lines.Add("# Render-data validation report")
    [void]$lines.Add("")
    [void]$lines.Add("结论：$verdict")
    [void]$lines.Add("")
    [void]$lines.Add("## 如何判断")
    [void]$lines.Add("")
    [void]$lines.Add(('- status：`{0}`' -f $status))
    [void]$lines.Add(('- remaining_placeholder_count：`{0}`' -f $remainingPlaceholderText))
    [void]$lines.Add(('- require_complete：`{0}`' -f $requireCompleteText))
    [void]$lines.Add("")
    [void]$lines.Add("## 这次校验了什么")
    [void]$lines.Add("")
    [void]$lines.Add(('- data JSON：`{0}`' -f $dataPathText))
    [void]$lines.Add(('- mapping：`{0}`' -f $mappingPathText))
    [void]$lines.Add(('- template：`{0}`' -f $inputDocxText))
    [void]$lines.Add("")
    [void]$lines.Add("## Patch 覆盖情况")
    [void]$lines.Add("")
    foreach ($category in @(
            "bookmark_text",
            "bookmark_paragraphs",
            "bookmark_table_rows",
            "bookmark_block_visibility"
        )) {
        [void]$lines.Add("- ${category}：$(Get-ValidationReportPatchCountText -Summary $Summary -Category $category)")
    }
    [void]$lines.Add("")
    [void]$lines.Add("## 下一步")
    [void]$lines.Add("")
    [void]$lines.Add("- $nextAction")

    $remainingPlaceholders = Get-OptionalObjectPropertyObject `
        -Object $Summary `
        -Name "remaining_placeholders"
    $placeholderLines = New-Object 'System.Collections.Generic.List[string]'
    $errorMessage = Get-OptionalObjectPropertyValue -Object $Summary -Name "error"
    foreach ($category in @("bookmark_text", "bookmark_paragraphs", "bookmark_table_rows")) {
        $entries = @(Get-OptionalObjectPropertyObject -Object $remainingPlaceholders -Name $category)
        foreach ($entry in $entries) {
            if ($null -ne $entry) {
                [void]$placeholderLines.Add(
                    (Get-ValidationReportPlaceholderLine `
                        -Category $category `
                        -Placeholder $entry `
                        -Mapping $Mapping)
                )
            }
        }
    }
    if ($errorMessage -match "bookmark_table_rows rows must be arrays") {
        foreach ($entry in @(Get-ValidationReportObjectArrayProperty -Object $Mapping -Name "bookmark_table_rows")) {
            $hintLine = Get-ValidationReportMappingHintLine `
                -Category "bookmark_table_rows" `
                -Entry $entry
            if (-not [string]::IsNullOrWhiteSpace($hintLine)) {
                [void]$placeholderLines.Add($hintLine)
            }
        }
    }

    [void]$lines.Add("")
    [void]$lines.Add("## 还没补完的槽位")
    [void]$lines.Add("")
    if ($placeholderLines.Count -eq 0) {
        [void]$lines.Add("- 无")
    } else {
        foreach ($line in $placeholderLines) {
            [void]$lines.Add($line)
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($errorMessage)) {
        [void]$lines.Add("")
        [void]$lines.Add("## 错误信息")
        [void]$lines.Add("")
        [void]$lines.Add('```text')
        [void]$lines.Add($errorMessage)
        [void]$lines.Add('```')
    }

    return ($lines -join [Environment]::NewLine) + [Environment]::NewLine
}

function New-StepRecord {
    param(
        [int]$Index,
        [string]$Name,
        [string]$ScriptPath,
        [string]$SummaryJsonPath,
        [string]$OutputPath,
        [string]$Status,
        [object]$SummaryObject
    )

    return [ordered]@{
        index = $Index
        name = $Name
        script_path = $ScriptPath
        summary_json = $SummaryJsonPath
        output_path = $OutputPath
        status = $Status
        summary = $SummaryObject
    }
}

function Build-ValidationSummary {
    param(
        [string]$Status,
        [string]$InputDocx,
        [string]$WorkspaceDir,
        [string]$WorkspaceSummaryPath,
        [string]$MappingPath,
        [string]$DataPath,
        [string]$SummaryJsonPath,
        [string]$ReportMarkdownPath,
        [string]$DraftPlanPath,
        [string]$GeneratedPatchPath,
        [string]$PatchedPlanPath,
        [bool]$KeptDraftPlan,
        [bool]$KeptGeneratedPatch,
        [bool]$KeptPatchedPlan,
        [bool]$RequireComplete,
        [object]$PatchSummary,
        [object[]]$Steps,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        workspace_dir = $WorkspaceDir
        workspace_summary = $WorkspaceSummaryPath
        mapping_path = $MappingPath
        data_path = $DataPath
        summary_json = $SummaryJsonPath
        report_markdown = $ReportMarkdownPath
        draft_plan = $DraftPlanPath
        generated_patch = $GeneratedPatchPath
        patched_plan = $PatchedPlanPath
        kept_intermediate_files = [ordered]@{
            draft_plan = $KeptDraftPlan
            generated_patch = $KeptGeneratedPatch
            patched_plan = $KeptPatchedPlan
        }
        require_complete = $RequireComplete
        operation_count = $Steps.Count
        steps = $Steps
    }

    if ($null -ne $PatchSummary) {
        $summary.remaining_placeholder_count = [int]$PatchSummary.remaining_placeholder_count
        $summary.remaining_placeholders = $PatchSummary.remaining_placeholders
        $summary.patch_counts = $PatchSummary.patch_counts
        $summary.plan_counts = $PatchSummary.plan_counts
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedWorkspaceDir = Resolve-OptionalValidationPath -RepoRoot $repoRoot -InputPath $WorkspaceDir
$resolvedWorkspaceSummary = Resolve-WorkspaceSummaryPath `
    -RepoRoot $repoRoot `
    -WorkspaceSummary $WorkspaceSummary `
    -WorkspaceDir $resolvedWorkspaceDir
$workspaceSummaryObject = Read-JsonFileIfPresent -Path $resolvedWorkspaceSummary

if ([string]::IsNullOrWhiteSpace($resolvedWorkspaceDir) -and
    -not [string]::IsNullOrWhiteSpace($resolvedWorkspaceSummary)) {
    $resolvedWorkspaceDir = [System.IO.Path]::GetDirectoryName($resolvedWorkspaceSummary)
}
if ([string]::IsNullOrWhiteSpace($resolvedWorkspaceDir) -and
    $null -ne $workspaceSummaryObject) {
    $summaryWorkspaceDir = Get-OptionalObjectPropertyValue -Object $workspaceSummaryObject -Name "workspace_dir"
    if (-not [string]::IsNullOrWhiteSpace($summaryWorkspaceDir)) {
        $resolvedWorkspaceDir = Resolve-OptionalValidationPath -RepoRoot $repoRoot -InputPath $summaryWorkspaceDir
    }
}

$renderPlanSummaryPath = Find-WorkspaceCandidatePath `
    -DirectoryPath $resolvedWorkspaceDir `
    -Patterns @("*.render-plan.summary.json") `
    -Label "render-plan summary"
$renderPlanSummaryObject = Read-JsonFileIfPresent -Path $renderPlanSummaryPath

$resolvedInputDocx = if (-not [string]::IsNullOrWhiteSpace($InputDocx)) {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
} else {
    $summaryInputDocx = Get-OptionalObjectPropertyValue -Object $workspaceSummaryObject -Name "input_docx"
    if ([string]::IsNullOrWhiteSpace($summaryInputDocx)) {
        $summaryInputDocx = Get-OptionalObjectPropertyValue -Object $renderPlanSummaryObject -Name "input_docx"
    }
    if ([string]::IsNullOrWhiteSpace($summaryInputDocx)) {
        throw "InputDocx is required when the workspace metadata does not provide it."
    }
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $summaryInputDocx
}

$resolvedMappingPath = if (-not [string]::IsNullOrWhiteSpace($MappingPath)) {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $MappingPath
} else {
    $summaryMappingPath = Get-OptionalObjectPropertyValue -Object $workspaceSummaryObject -Name "mapping"
    if ([string]::IsNullOrWhiteSpace($summaryMappingPath)) {
        $summaryMappingPath = Find-WorkspaceCandidatePath `
            -DirectoryPath $resolvedWorkspaceDir `
            -Patterns @("*.render_data_mapping.draft.json", "*.render_data_mapping.json") `
            -Label "workspace mapping"
    }
    if ([string]::IsNullOrWhiteSpace($summaryMappingPath)) {
        throw "MappingPath is required when the workspace metadata does not provide a mapping file."
    }
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $summaryMappingPath
}

$resolvedDataPath = if (-not [string]::IsNullOrWhiteSpace($DataPath)) {
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $DataPath
} else {
    $summaryDataPath = Get-OptionalObjectPropertyValue -Object $workspaceSummaryObject -Name "data_skeleton"
    if ([string]::IsNullOrWhiteSpace($summaryDataPath)) {
        $summaryDataPath = Find-WorkspaceCandidatePath `
            -DirectoryPath $resolvedWorkspaceDir `
            -Patterns @("*.render_data.skeleton.json") `
            -Label "workspace data skeleton"
    }
    if ([string]::IsNullOrWhiteSpace($summaryDataPath)) {
        throw "DataPath is required when the workspace metadata does not provide a data skeleton."
    }
    Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $summaryDataPath
}
$resolvedSummaryJson = Resolve-OptionalValidationPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedReportMarkdown = Resolve-OptionalValidationPath -RepoRoot $repoRoot -InputPath $ReportMarkdown
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalValidationPath -RepoRoot $repoRoot -InputPath $BuildDir
}
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "featherdoc-validate-render-data-" + [System.Guid]::NewGuid().ToString("N")
)
$exportScriptPath = Join-Path $repoRoot "scripts\export_template_render_plan.ps1"
$convertScriptPath = Join-Path $repoRoot "scripts\convert_render_data_to_patch_plan.ps1"
$patchScriptPath = Join-Path $repoRoot "scripts\patch_template_render_plan.ps1"
$resolvedDraftPlanOutput = ""
$resolvedGeneratedPatchOutput = ""
$resolvedPatchedPlanOutput = ""
$exportSummaryPath = ""
$convertSummaryPath = ""
$patchSummaryPath = ""
$exportSummaryObject = $null
$convertSummaryObject = $null
$patchSummaryObject = $null
$mappingObject = $null
$status = "failed"
$failureMessage = ""

Ensure-PathParent -Path $resolvedSummaryJson
Ensure-PathParent -Path $resolvedReportMarkdown
New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null
$mappingObject = Read-JsonFileIfPresent -Path $resolvedMappingPath

$resolvedDraftPlanOutput = if ([string]::IsNullOrWhiteSpace($DraftPlanOutput)) {
    Join-Path $temporaryRoot "draft.render-plan.json"
} else {
    Resolve-OptionalValidationPath -RepoRoot $repoRoot -InputPath $DraftPlanOutput
}
$resolvedGeneratedPatchOutput = if ([string]::IsNullOrWhiteSpace($GeneratedPatchOutput)) {
    Join-Path $temporaryRoot "generated.render_patch.json"
} else {
    Resolve-OptionalValidationPath -RepoRoot $repoRoot -InputPath $GeneratedPatchOutput
}
$resolvedPatchedPlanOutput = if ([string]::IsNullOrWhiteSpace($PatchedPlanOutput)) {
    Join-Path $temporaryRoot "patched.render-plan.json"
} else {
    Resolve-OptionalValidationPath -RepoRoot $repoRoot -InputPath $PatchedPlanOutput
}
$exportSummaryPath = Join-Path $temporaryRoot "export-template-render-plan.summary.json"
$convertSummaryPath = Join-Path $temporaryRoot "convert-render-data-to-patch-plan.summary.json"
$patchSummaryPath = Join-Path $temporaryRoot "patch-template-render-plan.summary.json"

Ensure-PathParent -Path $resolvedDraftPlanOutput
Ensure-PathParent -Path $resolvedGeneratedPatchOutput
Ensure-PathParent -Path $resolvedPatchedPlanOutput

try {
    Write-Step "Exporting render-plan draft from the template"
    & $exportScriptPath `
        -InputDocx $resolvedInputDocx `
        -OutputPlan $resolvedDraftPlanOutput `
        -SummaryJson $exportSummaryPath `
        -BuildDir $resolvedBuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild
    $exportSummaryObject = Read-JsonFileIfPresent -Path $exportSummaryPath
    if ($LASTEXITCODE -ne 0) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $exportSummaryObject -Name "error"
        if ([string]::IsNullOrWhiteSpace($stepFailure)) {
            throw "export_template_render_plan.ps1 failed."
        }
        throw "export_template_render_plan.ps1 failed: $stepFailure"
    }

    Write-Step "Converting business data into a render patch"
    & $convertScriptPath `
        -DataPath $resolvedDataPath `
        -MappingPath $resolvedMappingPath `
        -OutputPatch $resolvedGeneratedPatchOutput `
        -SummaryJson $convertSummaryPath
    $convertSummaryObject = Read-JsonFileIfPresent -Path $convertSummaryPath
    if ($LASTEXITCODE -ne 0) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $convertSummaryObject -Name "error"
        if ([string]::IsNullOrWhiteSpace($stepFailure)) {
            throw "convert_render_data_to_patch_plan.ps1 failed."
        }
        throw "convert_render_data_to_patch_plan.ps1 failed: $stepFailure"
    }

    Write-Step "Applying the generated patch onto the exported draft"
    & $patchScriptPath `
        -BasePlanPath $resolvedDraftPlanOutput `
        -PatchPlanPath $resolvedGeneratedPatchOutput `
        -OutputPlan $resolvedPatchedPlanOutput `
        -SummaryJson $patchSummaryPath `
        -RequireComplete:$RequireComplete
    $patchSummaryObject = Read-JsonFileIfPresent -Path $patchSummaryPath
    if ($LASTEXITCODE -ne 0) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $patchSummaryObject -Name "error"
        if ([string]::IsNullOrWhiteSpace($stepFailure)) {
            throw "patch_template_render_plan.ps1 failed."
        }
        throw "patch_template_render_plan.ps1 failed: $stepFailure"
    }

    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
} finally {
    if ($null -eq $exportSummaryObject) {
        $exportSummaryObject = Read-JsonFileIfPresent -Path $exportSummaryPath
    }
    if ($null -eq $convertSummaryObject) {
        $convertSummaryObject = Read-JsonFileIfPresent -Path $convertSummaryPath
    }
    if ($null -eq $patchSummaryObject) {
        $patchSummaryObject = Read-JsonFileIfPresent -Path $patchSummaryPath
    }

    $exportStepStatus = if ($null -ne $exportSummaryObject) {
        [string]$exportSummaryObject.status
    } else {
        "failed"
    }
    $convertStepStatus = if ($null -ne $convertSummaryObject) {
        [string]$convertSummaryObject.status
    } else {
        "failed"
    }
    $patchStepStatus = if ($null -ne $patchSummaryObject) {
        [string]$patchSummaryObject.status
    } else {
        "failed"
    }

    $summary = $null
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson) -or
        -not [string]::IsNullOrWhiteSpace($resolvedReportMarkdown)) {
        $summary = Build-ValidationSummary `
            -Status $status `
            -InputDocx $resolvedInputDocx `
            -WorkspaceDir $resolvedWorkspaceDir `
            -WorkspaceSummaryPath $resolvedWorkspaceSummary `
            -MappingPath $resolvedMappingPath `
            -DataPath $resolvedDataPath `
            -SummaryJsonPath $resolvedSummaryJson `
            -ReportMarkdownPath $resolvedReportMarkdown `
            -DraftPlanPath $resolvedDraftPlanOutput `
            -GeneratedPatchPath $resolvedGeneratedPatchOutput `
            -PatchedPlanPath $resolvedPatchedPlanOutput `
            -KeptDraftPlan (-not [string]::IsNullOrWhiteSpace($DraftPlanOutput)) `
            -KeptGeneratedPatch (-not [string]::IsNullOrWhiteSpace($GeneratedPatchOutput)) `
            -KeptPatchedPlan (-not [string]::IsNullOrWhiteSpace($PatchedPlanOutput)) `
            -RequireComplete ([bool]$RequireComplete) `
            -PatchSummary $patchSummaryObject `
            -Steps @(
                [pscustomobject](New-StepRecord `
                    -Index 1 `
                    -Name "export_template_render_plan" `
                    -ScriptPath $exportScriptPath `
                    -SummaryJsonPath $exportSummaryPath `
                    -OutputPath $resolvedDraftPlanOutput `
                    -Status $exportStepStatus `
                    -SummaryObject $exportSummaryObject),
                [pscustomobject](New-StepRecord `
                    -Index 2 `
                    -Name "convert_render_data_to_patch_plan" `
                    -ScriptPath $convertScriptPath `
                    -SummaryJsonPath $convertSummaryPath `
                    -OutputPath $resolvedGeneratedPatchOutput `
                    -Status $convertStepStatus `
                    -SummaryObject $convertSummaryObject),
                [pscustomobject](New-StepRecord `
                    -Index 3 `
                    -Name "patch_template_render_plan" `
                    -ScriptPath $patchScriptPath `
                    -SummaryJsonPath $patchSummaryPath `
                    -OutputPath $resolvedPatchedPlanOutput `
                    -Status $patchStepStatus `
                    -SummaryObject $patchSummaryObject)
            ) `
            -ErrorMessage $failureMessage
        if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
            ($summary | ConvertTo-Json -Depth 20) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
        }
        if (-not [string]::IsNullOrWhiteSpace($resolvedReportMarkdown)) {
            New-ValidationReportMarkdown `
                -Summary ([pscustomobject]$summary) `
                -Mapping $mappingObject |
                Set-Content -LiteralPath $resolvedReportMarkdown -Encoding UTF8
        }
    }

    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

if ($status -ne "completed") {
    if (-not [string]::IsNullOrWhiteSpace($failureMessage)) {
        Write-Error $failureMessage
    }
    exit 1
}

Write-Step "Render-data mapping contract is valid."
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}
if (-not [string]::IsNullOrWhiteSpace($resolvedReportMarkdown)) {
    Write-Step "Validation report: $resolvedReportMarkdown"
}
if (-not [string]::IsNullOrWhiteSpace($DraftPlanOutput)) {
    Write-Step "Draft render plan: $resolvedDraftPlanOutput"
}
if (-not [string]::IsNullOrWhiteSpace($GeneratedPatchOutput)) {
    Write-Step "Generated render patch: $resolvedGeneratedPatchOutput"
}
if (-not [string]::IsNullOrWhiteSpace($PatchedPlanOutput)) {
    Write-Step "Patched render plan: $resolvedPatchedPlanOutput"
}

exit 0
