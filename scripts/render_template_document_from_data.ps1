<#
.SYNOPSIS
Renders a DOCX template directly from business data plus a mapping file.

.DESCRIPTION
Runs `export_template_render_plan.ps1`,
`convert_render_data_to_patch_plan.ps1`,
`patch_template_render_plan.ps1`, and
`render_template_document.ps1` as one direct pipeline so callers can start from
business JSON and finish with a rendered `.docx` without replaying the same
validation steps twice.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$DataPath,
    [Parameter(Mandatory = $true)]
    [string]$MappingPath,
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
    Write-Host "[render-template-document-from-data] $Message"
}

function Resolve-OptionalPipelinePath {
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

function Build-OrchestratedRenderSummary {
    param(
        [string]$Status,
        [string]$InputDocx,
        [string]$DataPath,
        [string]$MappingPath,
        [string]$OutputDocx,
        [string]$SummaryJsonPath,
        [string]$BuildDir,
        [string]$Generator,
        [bool]$SkipBuild,
        [string]$PatchPlanPath,
        [string]$DraftPlanPath,
        [string]$PatchedPlanPath,
        [bool]$KeptPatchPlan,
        [bool]$KeptDraftPlan,
        [bool]$KeptPatchedPlan,
        [object]$PatchSummaryObject,
        [object]$RenderSummaryObject,
        [object[]]$Steps,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        data_path = $DataPath
        mapping_path = $MappingPath
        output_docx = $OutputDocx
        summary_json = $SummaryJsonPath
        build_dir = $BuildDir
        generator = $Generator
        skip_build = $SkipBuild
        require_complete = $true
        patch_plan = $PatchPlanPath
        draft_plan = $DraftPlanPath
        patched_plan = $PatchedPlanPath
        kept_intermediate_files = [ordered]@{
            patch_plan = $KeptPatchPlan
            draft_plan = $KeptDraftPlan
            patched_plan = $KeptPatchedPlan
        }
        operation_count = $Steps.Count
        steps = $Steps
    }

    if ($null -ne $PatchSummaryObject) {
        $summary.remaining_placeholder_count = [int]$PatchSummaryObject.remaining_placeholder_count
        $summary.remaining_placeholders = $PatchSummaryObject.remaining_placeholders
        $summary.patch_counts = $PatchSummaryObject.patch_counts
        $summary.plan_counts = $PatchSummaryObject.plan_counts
    }
    if ($null -ne $RenderSummaryObject) {
        $summary.render_operation_count = [int]$RenderSummaryObject.operation_count
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedDataPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $DataPath
$resolvedMappingPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $MappingPath
$resolvedOutputDocx = Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $OutputDocx
$resolvedSummaryJson = Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $BuildDir
}
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "featherdoc-render-from-data-" + [System.Guid]::NewGuid().ToString("N")
)
$exportScriptPath = Join-Path $repoRoot "scripts\export_template_render_plan.ps1"
$convertScriptPath = Join-Path $repoRoot "scripts\convert_render_data_to_patch_plan.ps1"
$patchScriptPath = Join-Path $repoRoot "scripts\patch_template_render_plan.ps1"
$renderScriptPath = Join-Path $repoRoot "scripts\render_template_document.ps1"
$resolvedPatchPlanOutput = ""
$resolvedDraftPlanOutput = ""
$resolvedPatchedPlanOutput = ""
$exportSummaryPath = ""
$convertSummaryPath = ""
$patchSummaryPath = ""
$renderSummaryPath = ""
$status = "failed"
$failureMessage = ""
$exportStarted = $false
$convertStarted = $false
$patchStarted = $false
$renderStarted = $false
$exportSummaryObject = $null
$convertSummaryObject = $null
$patchSummaryObject = $null
$renderSummaryObject = $null

Ensure-PathParent -Path $resolvedOutputDocx
Ensure-PathParent -Path $resolvedSummaryJson
New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null

$resolvedPatchPlanOutput = if ([string]::IsNullOrWhiteSpace($PatchPlanOutput)) {
    Join-Path $temporaryRoot "generated.render_patch.json"
} else {
    Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $PatchPlanOutput
}
$resolvedDraftPlanOutput = if ([string]::IsNullOrWhiteSpace($DraftPlanOutput)) {
    Join-Path $temporaryRoot "draft.render-plan.json"
} else {
    Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $DraftPlanOutput
}
$resolvedPatchedPlanOutput = if ([string]::IsNullOrWhiteSpace($PatchedPlanOutput)) {
    Join-Path $temporaryRoot "patched.render-plan.json"
} else {
    Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $PatchedPlanOutput
}
$exportSummaryPath = Join-Path $temporaryRoot "export-template-render-plan.summary.json"
$convertSummaryPath = Join-Path $temporaryRoot "convert-render-data-to-patch-plan.summary.json"
$patchSummaryPath = Join-Path $temporaryRoot "patch-template-render-plan.summary.json"
$renderSummaryPath = Join-Path $temporaryRoot "render-template-document.summary.json"

Ensure-PathParent -Path $resolvedPatchPlanOutput
Ensure-PathParent -Path $resolvedDraftPlanOutput
Ensure-PathParent -Path $resolvedPatchedPlanOutput

try {
    Write-Step "Exporting render-plan draft from the template"
    $exportStarted = $true
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
    $convertStarted = $true
    & $convertScriptPath `
        -DataPath $resolvedDataPath `
        -MappingPath $resolvedMappingPath `
        -OutputPatch $resolvedPatchPlanOutput `
        -SummaryJson $convertSummaryPath
    $convertSummaryObject = Read-JsonFileIfPresent -Path $convertSummaryPath
    if ($LASTEXITCODE -ne 0) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $convertSummaryObject -Name "error"
        if ([string]::IsNullOrWhiteSpace($stepFailure)) {
            throw "convert_render_data_to_patch_plan.ps1 failed."
        }
        throw "convert_render_data_to_patch_plan.ps1 failed: $stepFailure"
    }
    if (-not (Test-Path -LiteralPath $resolvedPatchPlanOutput)) {
        throw "Convert step did not produce '$resolvedPatchPlanOutput'."
    }

    Write-Step "Applying the generated patch onto the exported draft"
    $patchStarted = $true
    & $patchScriptPath `
        -BasePlanPath $resolvedDraftPlanOutput `
        -PatchPlanPath $resolvedPatchPlanOutput `
        -OutputPlan $resolvedPatchedPlanOutput `
        -SummaryJson $patchSummaryPath `
        -RequireComplete
    $patchSummaryObject = Read-JsonFileIfPresent -Path $patchSummaryPath
    if ($LASTEXITCODE -ne 0) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $patchSummaryObject -Name "error"
        if ([string]::IsNullOrWhiteSpace($stepFailure)) {
            throw "patch_template_render_plan.ps1 failed."
        }
        throw "patch_template_render_plan.ps1 failed: $stepFailure"
    }
    if (-not (Test-Path -LiteralPath $resolvedPatchedPlanOutput)) {
        throw "Patch step did not produce '$resolvedPatchedPlanOutput'."
    }

    Write-Step "Rendering final DOCX from the patched render plan"
    $renderStarted = $true
    & $renderScriptPath `
        -InputDocx $resolvedInputDocx `
        -PlanPath $resolvedPatchedPlanOutput `
        -OutputDocx $resolvedOutputDocx `
        -SummaryJson $renderSummaryPath `
        -BuildDir $resolvedBuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild
    $renderSummaryObject = Read-JsonFileIfPresent -Path $renderSummaryPath
    if ($LASTEXITCODE -ne 0) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $renderSummaryObject -Name "error"
        if ([string]::IsNullOrWhiteSpace($stepFailure)) {
            throw "render_template_document.ps1 failed."
        }
        throw "render_template_document.ps1 failed: $stepFailure"
    }
    if (-not (Test-Path -LiteralPath $resolvedOutputDocx)) {
        throw "Render step did not produce '$resolvedOutputDocx'."
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
    if ($null -eq $renderSummaryObject) {
        $renderSummaryObject = Read-JsonFileIfPresent -Path $renderSummaryPath
    }

    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-OrchestratedRenderSummary `
            -Status $status `
            -InputDocx $resolvedInputDocx `
            -DataPath $resolvedDataPath `
            -MappingPath $resolvedMappingPath `
            -OutputDocx $resolvedOutputDocx `
            -SummaryJsonPath $resolvedSummaryJson `
            -BuildDir $resolvedBuildDir `
            -Generator $Generator `
            -SkipBuild ([bool]$SkipBuild) `
            -PatchPlanPath $resolvedPatchPlanOutput `
            -DraftPlanPath $resolvedDraftPlanOutput `
            -PatchedPlanPath $resolvedPatchedPlanOutput `
            -KeptPatchPlan (-not [string]::IsNullOrWhiteSpace($PatchPlanOutput)) `
            -KeptDraftPlan (-not [string]::IsNullOrWhiteSpace($DraftPlanOutput)) `
            -KeptPatchedPlan (-not [string]::IsNullOrWhiteSpace($PatchedPlanOutput)) `
            -PatchSummaryObject $patchSummaryObject `
            -RenderSummaryObject $renderSummaryObject `
            -Steps @(
                [pscustomobject](New-StepRecord `
                    -Index 1 `
                    -Name "export_template_render_plan" `
                    -ScriptPath $exportScriptPath `
                    -SummaryJsonPath $exportSummaryPath `
                    -OutputPath $resolvedDraftPlanOutput `
                    -Started $exportStarted `
                    -SummaryObject $exportSummaryObject),
                [pscustomobject](New-StepRecord `
                    -Index 2 `
                    -Name "convert_render_data_to_patch_plan" `
                    -ScriptPath $convertScriptPath `
                    -SummaryJsonPath $convertSummaryPath `
                    -OutputPath $resolvedPatchPlanOutput `
                    -Started $convertStarted `
                    -SummaryObject $convertSummaryObject),
                [pscustomobject](New-StepRecord `
                    -Index 3 `
                    -Name "patch_template_render_plan" `
                    -ScriptPath $patchScriptPath `
                    -SummaryJsonPath $patchSummaryPath `
                    -OutputPath $resolvedPatchedPlanOutput `
                    -Started $patchStarted `
                    -SummaryObject $patchSummaryObject),
                [pscustomobject](New-StepRecord `
                    -Index 4 `
                    -Name "render_template_document" `
                    -ScriptPath $renderScriptPath `
                    -SummaryJsonPath $renderSummaryPath `
                    -OutputPath $resolvedOutputDocx `
                    -Started $renderStarted `
                    -SummaryObject $renderSummaryObject)
            ) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 20) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
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
if (-not [string]::IsNullOrWhiteSpace($PatchPlanOutput)) {
    Write-Step "Generated render patch: $resolvedPatchPlanOutput"
}
if (-not [string]::IsNullOrWhiteSpace($DraftPlanOutput)) {
    Write-Step "Draft render plan: $resolvedDraftPlanOutput"
}
if (-not [string]::IsNullOrWhiteSpace($PatchedPlanOutput)) {
    Write-Step "Patched render plan: $resolvedPatchedPlanOutput"
}

exit 0
