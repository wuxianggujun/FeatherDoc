<#
.SYNOPSIS
Exports, patches, and renders a DOCX template in one command.

.DESCRIPTION
Runs `export_template_render_plan.ps1`, `patch_template_render_plan.ps1`, and
`render_template_document.ps1` as one repeatable pipeline. The patch step
always enables `-RequireComplete` so leftover `TODO:` placeholders or empty
table-row drafts fail fast before a document is rendered. Use
`-ExportTargetMode resolved-section-targets` when patch entries should address
effective section header/footer targets.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\render_template_document_from_patch.ps1 `
    -InputDocx .\samples\chinese_invoice_template.docx `
    -PatchPlanPath .\samples\chinese_invoice_template.render_patch.json `
    -OutputDocx .\output\rendered\invoice.from-patch.docx `
    -SummaryJson .\output\rendered\invoice.from-patch.summary.json `
    -BuildDir build-codex-clang-compat `
    -SkipBuild
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$PatchPlanPath,
    [Parameter(Mandatory = $true)]
    [string]$OutputDocx,
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [ValidateSet("loaded-parts", "resolved-section-targets")]
    [string]$ExportTargetMode = "loaded-parts",
    [switch]$SkipBuild,
    [string]$DraftPlanOutput = "",
    [string]$PatchedPlanOutput = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

function Write-Step {
    param([string]$Message)
    Write-Host "[render-template-document-from-patch] $Message"
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
        [string]$PatchPlanPath,
        [string]$OutputDocx,
        [string]$SummaryJsonPath,
        [string]$CliPath,
        [string]$ResolvedBuildDir,
        [string]$Generator,
        [bool]$SkipBuild,
        [string]$ExportTargetMode,
        [string]$DraftPlanPath,
        [string]$PatchedPlanPath,
        [bool]$KeptDraftPlan,
        [bool]$KeptPatchedPlan,
        [object[]]$Steps,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        patch_plan = $PatchPlanPath
        output_docx = $OutputDocx
        summary_json = $SummaryJsonPath
        cli_path = $CliPath
        build_dir = $ResolvedBuildDir
        generator = $Generator
        skip_build = $SkipBuild
        export_target_mode = $ExportTargetMode
        require_complete = $true
        draft_plan = $DraftPlanPath
        patched_plan = $PatchedPlanPath
        kept_intermediate_files = [ordered]@{
            draft_plan = $KeptDraftPlan
            patched_plan = $KeptPatchedPlan
        }
        operation_count = $Steps.Count
        steps = $Steps
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedPatchPlanPath = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $PatchPlanPath
$resolvedOutputDocx = Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $OutputDocx
$resolvedSummaryJson = Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalPipelinePath -RepoRoot $repoRoot -InputPath $BuildDir
}
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "featherdoc-render-from-patch-" + [System.Guid]::NewGuid().ToString("N")
)
$resolvedDraftPlanOutput = ""
$resolvedPatchedPlanOutput = ""
$exportSummaryPath = ""
$patchSummaryPath = ""
$renderSummaryPath = ""
$cliPath = ""
$status = "failed"
$failureMessage = ""
$exportStarted = $false
$patchStarted = $false
$renderStarted = $false
$exportSummaryObject = $null
$patchSummaryObject = $null
$renderSummaryObject = $null
$exportScriptPath = Join-Path $repoRoot "scripts\export_template_render_plan.ps1"
$patchScriptPath = Join-Path $repoRoot "scripts\patch_template_render_plan.ps1"
$renderScriptPath = Join-Path $repoRoot "scripts\render_template_document.ps1"

Ensure-PathParent -Path $resolvedOutputDocx
Ensure-PathParent -Path $resolvedSummaryJson
New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null

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
$patchSummaryPath = Join-Path $temporaryRoot "patch-template-render-plan.summary.json"
$renderSummaryPath = Join-Path $temporaryRoot "render-template-document.summary.json"

Ensure-PathParent -Path $resolvedDraftPlanOutput
Ensure-PathParent -Path $resolvedPatchedPlanOutput

try {
    Write-Step "Resolving featherdoc_cli once for the full pipeline"
    $cliPath = Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $resolvedBuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild

    Write-Step "Exporting render-plan draft"
    $exportStarted = $true
    & $exportScriptPath `
        -InputDocx $resolvedInputDocx `
        -OutputPlan $resolvedDraftPlanOutput `
        -SummaryJson $exportSummaryPath `
        -BuildDir $resolvedBuildDir `
        -Generator $Generator `
        -TargetMode $ExportTargetMode `
        -SkipBuild
    $exportSummaryObject = Read-JsonFileIfPresent -Path $exportSummaryPath
    if ($LASTEXITCODE -ne 0) {
        $stepFailure = Get-OptionalObjectPropertyValue -Object $exportSummaryObject -Name "error"
        if ([string]::IsNullOrWhiteSpace($stepFailure)) {
            throw "export_template_render_plan.ps1 failed."
        }
        throw "export_template_render_plan.ps1 failed: $stepFailure"
    }
    if (-not (Test-Path -LiteralPath $resolvedDraftPlanOutput)) {
        throw "Export step did not produce '$resolvedDraftPlanOutput'."
    }

    Write-Step "Applying patch plan onto the exported draft"
    $patchStarted = $true
    & $patchScriptPath `
        -BasePlanPath $resolvedDraftPlanOutput `
        -PatchPlanPath $resolvedPatchPlanPath `
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
        -SkipBuild
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
    Write-Error $failureMessage
} finally {
    if ($null -eq $exportSummaryObject) {
        $exportSummaryObject = Read-JsonFileIfPresent -Path $exportSummaryPath
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
            -PatchPlanPath $resolvedPatchPlanPath `
            -OutputDocx $resolvedOutputDocx `
            -SummaryJsonPath $resolvedSummaryJson `
            -CliPath $cliPath `
            -ResolvedBuildDir $resolvedBuildDir `
            -Generator $Generator `
            -SkipBuild ([bool]$SkipBuild) `
            -ExportTargetMode $ExportTargetMode `
            -DraftPlanPath $resolvedDraftPlanOutput `
            -PatchedPlanPath $resolvedPatchedPlanOutput `
            -KeptDraftPlan (-not [string]::IsNullOrWhiteSpace($DraftPlanOutput)) `
            -KeptPatchedPlan (-not [string]::IsNullOrWhiteSpace($PatchedPlanOutput)) `
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
                    -Name "patch_template_render_plan" `
                    -ScriptPath $patchScriptPath `
                    -SummaryJsonPath $patchSummaryPath `
                    -OutputPath $resolvedPatchedPlanOutput `
                    -Started $patchStarted `
                    -SummaryObject $patchSummaryObject),
                [pscustomobject](New-StepRecord `
                    -Index 3 `
                    -Name "render_template_document" `
                    -ScriptPath $renderScriptPath `
                    -SummaryJsonPath $renderSummaryPath `
                    -OutputPath $resolvedOutputDocx `
                    -Started $renderStarted `
                    -SummaryObject $renderSummaryObject)
            ) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 16) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }

    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

if ($status -ne "completed") {
    exit 1
}

Write-Step "Rendered DOCX: $resolvedOutputDocx"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}
if (-not [string]::IsNullOrWhiteSpace($DraftPlanOutput)) {
    Write-Step "Draft render plan: $resolvedDraftPlanOutput"
}
if (-not [string]::IsNullOrWhiteSpace($PatchedPlanOutput)) {
    Write-Step "Patched render plan: $resolvedPatchedPlanOutput"
}

exit 0
