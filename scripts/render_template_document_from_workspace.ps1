<#
.SYNOPSIS
Renders a DOCX template from a prepared render-data workspace.

.DESCRIPTION
Uses a workspace produced by `prepare_template_render_data_workspace.ps1`,
resolves the workspace's mapping/data files, and then calls
`render_template_document_from_data.ps1`.

This wrapper turns the template-data workflow into two user-facing steps:
1. prepare the editable workspace,
2. edit the JSON data and render the final `.docx`.
#>
param(
    [string]$WorkspaceDir = "",
    [string]$WorkspaceSummary = "",
    [string]$InputDocx = "",
    [string]$DataPath = "",
    [string]$MappingPath = "",
    [Parameter(Mandatory = $true)]
    [string]$OutputDocx,
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [switch]$SkipBuild,
    [string]$PatchPlanOutput = "",
    [string]$DraftPlanOutput = "",
    [string]$PatchedPlanOutput = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[render-template-document-from-workspace] $Message"
}

function Resolve-OptionalWorkspaceRenderPath {
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

function New-StepRecord {
    param(
        [int]$Index,
        [string]$Name,
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
        summary_json = $SummaryJsonPath
        output_path = $OutputPath
        started = $Started
        status = $status
        summary = $SummaryObject
    }
}

function Build-WorkspaceRenderSummary {
    param(
        [string]$Status,
        [string]$WorkspaceDir,
        [string]$WorkspaceSummaryPath,
        [string]$InputDocx,
        [string]$DataPath,
        [string]$MappingPath,
        [string]$OutputDocx,
        [string]$SummaryJsonPath,
        [string]$BuildDir,
        [string]$Generator,
        [bool]$SkipBuild,
        [object[]]$Steps,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        workspace_dir = $WorkspaceDir
        workspace_summary = $WorkspaceSummaryPath
        input_docx = $InputDocx
        data_path = $DataPath
        mapping_path = $MappingPath
        output_docx = $OutputDocx
        summary_json = $SummaryJsonPath
        build_dir = $BuildDir
        generator = $Generator
        skip_build = $SkipBuild
        operation_count = $Steps.Count
        steps = $Steps
        workflow = [ordered]@{
            purpose = "Edit workspace JSON data, then render the final DOCX."
            edit_data = $DataPath
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedWorkspaceDir = Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $WorkspaceDir
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
        $resolvedWorkspaceDir = Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $summaryWorkspaceDir
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

$effectiveGenerator = if ($PSBoundParameters.ContainsKey("Generator")) {
    $Generator
} else {
    $summaryGenerator = Get-OptionalObjectPropertyValue -Object $workspaceSummaryObject -Name "generator"
    if ([string]::IsNullOrWhiteSpace($summaryGenerator)) {
        "NMake Makefiles"
    } else {
        $summaryGenerator
    }
}
$effectiveSkipBuild = if ($PSBoundParameters.ContainsKey("SkipBuild")) {
    [bool]$SkipBuild
} else {
    $summarySkipBuild = Get-OptionalObjectPropertyValue -Object $workspaceSummaryObject -Name "skip_build"
    if ([string]::IsNullOrWhiteSpace($summarySkipBuild)) {
        $false
    } else {
        [bool]$workspaceSummaryObject.skip_build
    }
}

$resolvedOutputDocx = Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $OutputDocx
$resolvedSummaryJson = Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedBuildDir = if (-not [string]::IsNullOrWhiteSpace($BuildDir)) {
    Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $BuildDir
} else {
    $summaryBuildDir = Get-OptionalObjectPropertyValue -Object $workspaceSummaryObject -Name "build_dir"
    if ([string]::IsNullOrWhiteSpace($summaryBuildDir)) {
        Join-Path $repoRoot "build-template-schema-cli-nmake"
    } else {
        Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $summaryBuildDir
    }
}
$resolvedPatchPlanOutput = Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $PatchPlanOutput
$resolvedDraftPlanOutput = Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $DraftPlanOutput
$resolvedPatchedPlanOutput = Resolve-OptionalWorkspaceRenderPath -RepoRoot $repoRoot -InputPath $PatchedPlanOutput

$renderFromDataScriptPath = Join-Path $repoRoot "scripts\render_template_document_from_data.ps1"
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "featherdoc-render-from-workspace-" + [System.Guid]::NewGuid().ToString("N")
)
$nestedSummaryPath = Join-Path $temporaryRoot "render-template-document-from-data.summary.json"
$status = "failed"
$failureMessage = ""
$resolveStepStarted = $true
$renderStepStarted = $false
$renderSummaryObject = $null

Ensure-PathParent -Path $resolvedOutputDocx
Ensure-PathParent -Path $resolvedSummaryJson
Ensure-PathParent -Path $resolvedPatchPlanOutput
Ensure-PathParent -Path $resolvedDraftPlanOutput
Ensure-PathParent -Path $resolvedPatchedPlanOutput
New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null

$resolveSummaryObject = [ordered]@{
    status = "completed"
    workspace_dir = $resolvedWorkspaceDir
    workspace_summary = $resolvedWorkspaceSummary
    input_docx = $resolvedInputDocx
    data_path = $resolvedDataPath
    mapping_path = $resolvedMappingPath
    build_dir = $resolvedBuildDir
}

try {
    Write-Step "Resolved workspace data and mapping"

    Write-Step "Rendering DOCX from the workspace"
    $renderStepStarted = $true
    & $renderFromDataScriptPath `
        -InputDocx $resolvedInputDocx `
        -DataPath $resolvedDataPath `
        -MappingPath $resolvedMappingPath `
        -OutputDocx $resolvedOutputDocx `
        -SummaryJson $nestedSummaryPath `
        -BuildDir $resolvedBuildDir `
        -Generator $effectiveGenerator `
        -SkipBuild:$effectiveSkipBuild `
        -PatchPlanOutput $resolvedPatchPlanOutput `
        -DraftPlanOutput $resolvedDraftPlanOutput `
        -PatchedPlanOutput $resolvedPatchedPlanOutput
    $renderSummaryObject = Read-JsonFileIfPresent -Path $nestedSummaryPath
    if ($LASTEXITCODE -ne 0) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $renderSummaryObject -Name "error"
        $remainingPlaceholderCount = 0
        $remainingPlaceholderText = Get-OptionalObjectPropertyValue `
            -Object $renderSummaryObject `
            -Name "remaining_placeholder_count"
        if (-not [string]::IsNullOrWhiteSpace($remainingPlaceholderText)) {
            [void][int]::TryParse($remainingPlaceholderText, [ref]$remainingPlaceholderCount)
        }
        if ($stepFailure -match "render plan still contains" -or
            $stepFailure -match "bookmark_table_rows rows must be arrays of one or more cell texts" -or
            $remainingPlaceholderCount -gt 0) {
            throw "Workspace data still contains generated placeholders or empty table rows. Edit '$resolvedDataPath' first, then rerun."
        }
        if ([string]::IsNullOrWhiteSpace($stepFailure)) {
            throw "render_template_document_from_data.ps1 failed."
        }
        throw "render_template_document_from_data.ps1 failed: $stepFailure"
    }

    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    if ($null -eq $renderSummaryObject) {
        $renderSummaryObject = Read-JsonFileIfPresent -Path $nestedSummaryPath
    }
    if ($null -ne $renderSummaryObject) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $renderSummaryObject -Name "error"
        $remainingPlaceholderCount = 0
        $remainingPlaceholderText = Get-OptionalObjectPropertyValue `
            -Object $renderSummaryObject `
            -Name "remaining_placeholder_count"
        if (-not [string]::IsNullOrWhiteSpace($remainingPlaceholderText)) {
            [void][int]::TryParse($remainingPlaceholderText, [ref]$remainingPlaceholderCount)
        }

        if ($stepFailure -match "render plan still contains" -or
            $stepFailure -match "bookmark_table_rows rows must be arrays of one or more cell texts" -or
            $remainingPlaceholderCount -gt 0) {
            $failureMessage = "Workspace data still contains generated placeholders or empty table rows. Edit '$resolvedDataPath' first, then rerun."
        }
    }
} finally {
    if ($null -eq $renderSummaryObject) {
        $renderSummaryObject = Read-JsonFileIfPresent -Path $nestedSummaryPath
    }

    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-WorkspaceRenderSummary `
            -Status $status `
            -WorkspaceDir $resolvedWorkspaceDir `
            -WorkspaceSummaryPath $resolvedWorkspaceSummary `
            -InputDocx $resolvedInputDocx `
            -DataPath $resolvedDataPath `
            -MappingPath $resolvedMappingPath `
            -OutputDocx $resolvedOutputDocx `
            -SummaryJsonPath $resolvedSummaryJson `
            -BuildDir $resolvedBuildDir `
            -Generator $effectiveGenerator `
            -SkipBuild $effectiveSkipBuild `
            -Steps @(
                [pscustomobject](New-StepRecord `
                    -Index 1 `
                    -Name "resolve_workspace" `
                    -SummaryJsonPath $resolvedWorkspaceSummary `
                    -OutputPath $resolvedDataPath `
                    -Started $resolveStepStarted `
                    -SummaryObject ([pscustomobject]$resolveSummaryObject)),
                [pscustomobject](New-StepRecord `
                    -Index 2 `
                    -Name "render_template_document_from_data" `
                    -SummaryJsonPath $nestedSummaryPath `
                    -OutputPath $resolvedOutputDocx `
                    -Started $renderStepStarted `
                    -SummaryObject $renderSummaryObject)
            ) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
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

Write-Step "Rendered DOCX: $resolvedOutputDocx"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
