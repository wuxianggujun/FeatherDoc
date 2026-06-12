<#
.SYNOPSIS
Edits an existing DOCX by applying a JSON edit plan.

.DESCRIPTION
Runs a user-facing document-edit workflow:
1. read an edit plan JSON,
2. apply supported operations through featherdoc_cli or focused OOXML edits,
3. write the edited DOCX and a summary JSON.

The supported operations intentionally reuse stable CLI primitives where
possible. Focused OOXML edits are used for document-edit features that Word
stores inside paragraphs/runs, such as table-cell horizontal alignment and
visible text style overrides.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$InputDocx,
    [Parameter(Mandatory = $true)]
    [string]$EditPlan,
    [Parameter(Mandatory = $true)]
    [string]$OutputDocx,
    [string]$SummaryJson = "",
    [string]$BuildDir = "",
    [string]$Generator = "NMake Makefiles",
    [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_core.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_bookmark_args.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_field_style_args.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_table_args.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_review_patch_args.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_docx_table_edits.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_docx_paragraph_edits.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_docx_text_edits.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_bookmark_media_ops.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_review_ops.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_field_ops.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_content_control_ops.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_table_structure_ops.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_table_property_ops.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_style_ops.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_section_ops.ps1")
. (Join-Path $PSScriptRoot "edit_document_from_plan_operation_dispatch.ps1")

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedInputDocx = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $InputDocx
$resolvedEditPlan = Resolve-TemplateSchemaPath -RepoRoot $repoRoot -InputPath $EditPlan
$resolvedOutputDocx = Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $OutputDocx
$resolvedSummaryJson = Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $SummaryJson
$resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    Join-Path $repoRoot "build-template-schema-cli-nmake"
} else {
    Resolve-OptionalEditPath -RepoRoot $repoRoot -InputPath $BuildDir
}

Ensure-PathParent -Path $resolvedOutputDocx
Ensure-PathParent -Path $resolvedSummaryJson

$planObject = ConvertFrom-EditPlanJson -Json (
    Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedEditPlan
)
$operations = @(Get-EditPlanOperations -Plan $planObject)
if ($operations.Count -eq 0) {
    throw "Edit plan must contain at least one operation."
}

$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    "featherdoc-edit-document-" + [System.Guid]::NewGuid().ToString("N")
)
$operationRecords = New-Object 'System.Collections.Generic.List[object]'
$status = "failed"
$failureMessage = ""
$cliPath = ""

New-Item -ItemType Directory -Path $temporaryRoot -Force | Out-Null

try {
    Write-Step "Resolving featherdoc_cli"
    $cliPath = Resolve-TemplateSchemaCliPath `
        -RepoRoot $repoRoot `
        -BuildDir $resolvedBuildDir `
        -Generator $Generator `
        -SkipBuild:$SkipBuild

    $currentInput = $resolvedInputDocx
    for ($index = 0; $index -lt $operations.Count; ++$index) {
        $operationIndex = $index + 1
        $isLast = ($operationIndex -eq $operations.Count)
        $stepOutput = if ($isLast) {
            $resolvedOutputDocx
        } else {
            Join-Path $temporaryRoot ("edit-step-{0}.docx" -f $operationIndex)
        }

        $label = "operation[$operationIndex]"
        $operationInfo = New-OperationArguments `
            -Operation $operations[$index] `
            -InputPath $currentInput `
            -OutputPath $stepOutput `
            -Label $label `
            -OperationIndex $operationIndex `
            -TemporaryRoot $temporaryRoot

        Write-Step "Applying $($operationInfo.Operation)"
        if ($operationInfo.DirectOperation) {
            try {
                $directOperationKind = Get-OptionalObjectPropertyValue `
                    -Object $operationInfo `
                    -Name "DirectOperationKind"
                switch ($directOperationKind) {
                    "table-cell-horizontal-alignment" {
                        $directResult = Set-DocxTableCellHorizontalAlignment `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -RowIndex $operationInfo.RowIndex `
                            -CellIndex $operationInfo.CellIndex `
                            -Alignment $operationInfo.HorizontalAlignment `
                            -Clear $operationInfo.ClearHorizontalAlignment
                    }
                    "body-table-column-width" {
                        $directResult = Set-DocxTableColumnWidth `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -ColumnIndex $operationInfo.ColumnIndex `
                            -WidthTwips $operationInfo.WidthTwips `
                            -Clear $operationInfo.ClearColumnWidth
                    }
                    "text-style" {
                        $directResult = Set-DocxParagraphTextStyle `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -TableIndex $operationInfo.TableIndex `
                            -RowIndex $operationInfo.RowIndex `
                            -CellIndex $operationInfo.CellIndex `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -BoldSpecified $operationInfo.BoldSpecified `
                            -Bold $operationInfo.Bold `
                            -FontFamily $operationInfo.FontFamily `
                            -EastAsiaFontFamily $operationInfo.EastAsiaFontFamily `
                            -Language $operationInfo.Language `
                            -EastAsiaLanguage $operationInfo.EastAsiaLanguage `
                            -Color $operationInfo.Color `
                            -FontSizeHalfPoints $operationInfo.FontSizeHalfPoints
                    }
                    "body-paragraph-deletion" {
                        $directResult = Remove-DocxBodyParagraphs `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains
                    }
                    "text-replacement" {
                        $directResult = Set-DocxTextReplacement `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -FindText $operationInfo.FindText `
                            -ReplacementText $operationInfo.ReplacementText `
                            -MatchCase $operationInfo.MatchCase
                    }
                    "body-paragraph-horizontal-alignment" {
                        $directResult = Set-DocxBodyParagraphHorizontalAlignment `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -Alignment $operationInfo.HorizontalAlignment `
                            -Clear $operationInfo.ClearHorizontalAlignment
                    }
                    "body-paragraph-spacing" {
                        $directResult = Set-DocxBodyParagraphSpacing `
                            -InputPath $currentInput `
                            -OutputPath $stepOutput `
                            -ParagraphIndex $operationInfo.ParagraphIndex `
                            -TextContains $operationInfo.TextContains `
                            -BeforeTwips $operationInfo.BeforeTwips `
                            -AfterTwips $operationInfo.AfterTwips `
                            -LineTwips $operationInfo.LineTwips `
                            -LineRule $operationInfo.LineRule `
                            -Clear $operationInfo.ClearSpacing
                    }
                    default {
                        throw "Unsupported direct operation kind '$directOperationKind'."
                    }
                }
                $result = [pscustomobject]@{
                    ExitCode = 0
                    Text = ""
                    Output = @(($directResult | ConvertTo-Json -Compress))
                }
            } catch {
                $result = [pscustomobject]@{
                    ExitCode = 1
                    Text = $_.Exception.Message
                    Output = @($_.Exception.Message)
                }
            }
        } else {
            $result = Invoke-TemplateSchemaCli `
                -ExecutablePath $cliPath `
                -Arguments $operationInfo.Arguments
        }

        if ($result.ExitCode -ne 0) {
            $message = if ([string]::IsNullOrWhiteSpace($result.Text)) {
                "$label failed."
            } else {
                "$label failed: $($result.Text)"
            }
            $operationRecords.Add([pscustomobject](New-OperationRecord `
                    -Index $operationIndex `
                    -Op $operationInfo.Operation `
                    -Command $operationInfo.Command `
                    -InputPath $currentInput `
                    -OutputPath $stepOutput `
                    -Status "failed" `
                    -ExitCode $result.ExitCode `
                    -Output $result.Output `
                    -ErrorMessage $message)) | Out-Null
            throw $message
        }
        if (-not (Test-Path -LiteralPath $stepOutput)) {
            $message = "$label did not produce '$stepOutput'."
            $operationRecords.Add([pscustomobject](New-OperationRecord `
                    -Index $operationIndex `
                    -Op $operationInfo.Operation `
                    -Command $operationInfo.Command `
                    -InputPath $currentInput `
                    -OutputPath $stepOutput `
                    -Status "failed" `
                    -ExitCode $result.ExitCode `
                    -Output $result.Output `
                    -ErrorMessage $message)) | Out-Null
            throw $message
        }

        $operationRecords.Add([pscustomobject](New-OperationRecord `
                -Index $operationIndex `
                -Op $operationInfo.Operation `
                -Command $operationInfo.Command `
                -InputPath $currentInput `
                -OutputPath $stepOutput `
                -Status "completed" `
                -ExitCode $result.ExitCode `
                -Output $result.Output `
                -ErrorMessage "")) | Out-Null
        $currentInput = $stepOutput
    }

    $status = "completed"
} catch {
    $failureMessage = $_.Exception.Message
    Write-Error $failureMessage
} finally {
    if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
        $summary = Build-EditSummary `
            -Status $status `
            -InputDocx $resolvedInputDocx `
            -EditPlan $resolvedEditPlan `
            -OutputDocx $resolvedOutputDocx `
            -SummaryJson $resolvedSummaryJson `
            -CliPath $cliPath `
            -BuildDir $resolvedBuildDir `
            -Generator $Generator `
            -SkipBuild ([bool]$SkipBuild) `
            -Operations @($operationRecords.ToArray()) `
            -ErrorMessage $failureMessage
        ($summary | ConvertTo-Json -Depth 24) | Set-Content -LiteralPath $resolvedSummaryJson -Encoding UTF8
    }

    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

if ($status -ne "completed") {
    exit 1
}

Write-Step "Edited DOCX: $resolvedOutputDocx"
if (-not [string]::IsNullOrWhiteSpace($resolvedSummaryJson)) {
    Write-Step "Summary JSON: $resolvedSummaryJson"
}

exit 0
