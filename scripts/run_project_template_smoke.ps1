<#
.SYNOPSIS
Runs project-level template smoke checks from a manifest.

.DESCRIPTION
Reads a manifest of template fixtures, optionally prepares those fixtures via
built sample targets, then runs any combination of `validate-template`,
`validate-template-schema`, `check_template_schema_baseline.ps1`, and
`run_word_visual_smoke.ps1`.
#>
param(
    [string]$ManifestPath = "samples/project_template_smoke.manifest.json",
    [string]$BuildDir = "build-project-template-smoke-nmake",
    [string]$Generator = "NMake Makefiles",
    [string]$OutputDir = "output/project-template-smoke",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$SkipVisualSmoke,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")
. (Join-Path $PSScriptRoot "project_template_smoke_manifest_common.ps1")

$projectTemplateSmokeSummarySchema = "featherdoc.project_template_smoke_summary.v1"

. (Join-Path $PSScriptRoot "run_project_template_smoke_helpers.ps1")
. (Join-Path $PSScriptRoot "run_project_template_smoke_schema_approval.ps1")
. (Join-Path $PSScriptRoot "run_project_template_smoke_invocation.ps1")
. (Join-Path $PSScriptRoot "run_project_template_smoke_markdown.ps1")

$repoRoot = Resolve-TemplateSchemaRepoRoot -ScriptRoot $PSScriptRoot
$resolvedManifestPath = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $ManifestPath
$resolvedOutputDir = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $OutputDir -AllowMissing
$resolvedBuildDir = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $BuildDir -AllowMissing
$entriesOutputDir = Join-Path $resolvedOutputDir "entries"
$summaryPath = Join-Path $resolvedOutputDir "summary.json"
$summaryMarkdownPath = Join-Path $resolvedOutputDir "summary.md"
$baselineScriptPath = Join-Path $PSScriptRoot "check_template_schema_baseline.ps1"
$renderDataScriptPath = Join-Path $PSScriptRoot "render_template_document_from_data.ps1"
$visualSmokeScriptPath = Join-Path $PSScriptRoot "run_word_visual_smoke.ps1"

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
New-Item -ItemType Directory -Path $entriesOutputDir -Force | Out-Null

$manifestValidation = Test-ProjectTemplateSmokeManifest `
    -RepoRoot $repoRoot `
    -ManifestPath $resolvedManifestPath `
    -BuildDir $resolvedBuildDir `
    -CheckPaths
if (-not $manifestValidation.passed) {
    $errorLines = New-Object 'System.Collections.Generic.List[string]'
    $errorLines.Add("Project template smoke manifest validation failed:") | Out-Null
    foreach ($issue in $manifestValidation.errors) {
        $errorLines.Add("- $($issue.path): $($issue.message)") | Out-Null
    }
    throw ($errorLines -join [System.Environment]::NewLine)
}

$manifest = Get-Content -Raw -LiteralPath $resolvedManifestPath | ConvertFrom-Json
$entries = @($manifest.entries)
if ($entries.Count -eq 0) {
    $emptySummary = [ordered]@{
        schema = $projectTemplateSmokeSummarySchema
        generated_at = (Get-Date).ToString("s")
        manifest_path = $resolvedManifestPath
        workspace = $repoRoot
        build_dir = $resolvedBuildDir
        output_dir = $resolvedOutputDir
        entry_count = 0
        failed_entry_count = 0
        dirty_schema_baseline_count = 0
        visual_entry_count = 0
        visual_verdict = "not_applicable"
        manual_review_pending_count = 0
        visual_review_undetermined_count = 0
        passed = $true
        overall_status = "passed"
        entries = @()
    }
    ($emptySummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
    Write-SummaryMarkdown -Path $summaryMarkdownPath -RepoRoot $repoRoot -Summary $emptySummary
    Write-Step "No entries were registered in $resolvedManifestPath"
    exit 0
}

$sampleTargets = @(@(
    foreach ($entry in $entries) {
        $sampleTarget = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "prepare_sample_target"
        if ([string]::IsNullOrWhiteSpace($sampleTarget)) {
            continue
        }

        $entryInputDocx = Resolve-ManifestInputDocxPath `
            -RepoRoot $repoRoot `
            -ResolvedBuildDir $resolvedBuildDir `
            -Entry $entry
        if ($SkipBuild -and [System.IO.File]::Exists($entryInputDocx)) {
            continue
        }

        $sampleTarget
    }
) | Sort-Object -Unique)

if (-not $SkipBuild) {
    $vcvarsPath = Get-TemplateSchemaVcvarsPath
    $buildSamplesValue = if ($sampleTargets.Count -gt 0) { "ON" } else { "OFF" }
    Write-Step "Configuring build directory $resolvedBuildDir"
    Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
        "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"$Generator`" " +
        "-DBUILD_CLI=ON -DBUILD_SAMPLES=$buildSamplesValue -DBUILD_TESTING=OFF")

    $buildTargets = @("featherdoc_cli") + $sampleTargets
    Write-Step "Building targets: $($buildTargets -join ', ')"
    Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
        "cmake --build `"$resolvedBuildDir`" --target " + ($buildTargets -join " "))
}

$cliPath = Find-TemplateSchemaCliBinary -SearchRoot $resolvedBuildDir
if (-not $cliPath) {
    $cliSearch = Find-TemplateSchemaCliBinaryInBuildRoots `
        -RepoRoot $repoRoot `
        -PreferredBuildRoot $resolvedBuildDir
    if ($null -ne $cliSearch) {
        $cliPath = $cliSearch.BinaryPath
    }
}
if (-not $cliPath) {
    throw "Could not find featherdoc_cli under $resolvedBuildDir or any build* directory under $repoRoot."
}

$sampleTargetExecutables = @{}
foreach ($sampleTarget in $sampleTargets) {
    $binaryPath = Find-TemplateSchemaTargetBinary -SearchRoot $resolvedBuildDir -TargetName $sampleTarget
    if (-not $binaryPath) {
        $targetSearch = Find-TemplateSchemaTargetBinaryInBuildRoots `
            -RepoRoot $repoRoot `
            -PreferredBuildRoot $resolvedBuildDir `
            -TargetName $sampleTarget
        if ($null -ne $targetSearch) {
            $binaryPath = $targetSearch.BinaryPath
        }
    }
    if (-not $binaryPath) {
        throw "Could not find built sample target '$sampleTarget' under $resolvedBuildDir or any build* directory under $repoRoot."
    }
    $sampleTargetExecutables[$sampleTarget] = $binaryPath
}

$results = New-Object 'System.Collections.Generic.List[object]'
$failedEntryCount = 0
$dirtySchemaBaselineCount = 0
$manualReviewPendingCount = 0
$visualReviewUndeterminedCount = 0
$entryIndex = 0

foreach ($entry in $entries) {
    $entryIndex += 1
    $name = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "name"
    if ([string]::IsNullOrWhiteSpace($name)) {
        throw "Every manifest entry must provide a non-empty 'name'."
    }

    $entryDir = Join-Path $entriesOutputDir ("{0:00}-{1}" -f $entryIndex, (Convert-ToSafeFileName -Value $name))
    New-Item -ItemType Directory -Path $entryDir -Force | Out-Null

    $entryIssues = New-Object 'System.Collections.Generic.List[string]'
    $templateValidationResults = New-Object 'System.Collections.Generic.List[object]'
    $schemaValidationResult = [ordered]@{
        enabled = $false
    }
    $schemaBaselineResult = [ordered]@{
        enabled = $false
    }
    $renderDataResult = [ordered]@{
        enabled = $false
    }
    $visualSmokeResult = [ordered]@{
        enabled = $false
    }
    $entryPassed = $true
    $manualReviewPending = $false
    $visualReviewUndetermined = $false
    $resolvedInputDocx = ""

    Write-Step "Running entry '$name'"

    try {
        $resolvedInputDocx = Resolve-ManifestInputDocxPath `
            -RepoRoot $repoRoot `
            -ResolvedBuildDir $resolvedBuildDir `
            -Entry $entry

        $prepareSampleTarget = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "prepare_sample_target"
        if (-not [string]::IsNullOrWhiteSpace($prepareSampleTarget)) {
            $prepareArgument = Resolve-PrepareArgumentPath `
                -RepoRoot $repoRoot `
                -ResolvedInputDocx $resolvedInputDocx `
                -Entry $entry
            if ($SkipBuild -and [System.IO.File]::Exists($resolvedInputDocx) -and
                -not $sampleTargetExecutables.ContainsKey($prepareSampleTarget)) {
                Write-Step "Reusing existing prepared input for '$name': $resolvedInputDocx"
            } else {
                Ensure-PathParent -Path $prepareArgument
                Write-Step "Preparing '$name' via $prepareSampleTarget"
                $prepareOutput = @(& $sampleTargetExecutables[$prepareSampleTarget] $prepareArgument 2>&1)
                $prepareExitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
                $prepareLines = @($prepareOutput | ForEach-Object { $_.ToString() })
                $prepareLogPath = Join-Path $entryDir "prepare.log"
                Write-CommandOutput -OutputPath $prepareLogPath -Lines $prepareLines
                if ($prepareExitCode -ne 0) {
                    $joined = if ($prepareLines.Count -gt 0) {
                        $prepareLines -join [System.Environment]::NewLine
                    } else {
                        "(no output)"
                    }
                    throw "Sample target '$prepareSampleTarget' failed with exit code $prepareExitCode. Output:`n$joined"
                }
            }
        }

        if (-not (Test-Path -LiteralPath $resolvedInputDocx)) {
            throw "Input DOCX was not found after preparation: $resolvedInputDocx"
        }
    } catch {
        $entryIssues.Add($_.Exception.Message) | Out-Null
        $entryPassed = $false
    }

    if ($entryPassed) {
        $validationIndex = 0
        foreach ($validation in (Get-ManifestArrayProperty -Entry $entry -Name "template_validations")) {
            $validationIndex += 1
            try {
                $validationName = Resolve-OptionalManifestPropertyValue -Entry $validation -Name "name"
                if ([string]::IsNullOrWhiteSpace($validationName)) {
                    $validationName = "template-validation-$validationIndex"
                }

                $arguments = @("validate-template", $resolvedInputDocx)
                $arguments = Add-PartSelectionArguments -Arguments $arguments -PartSwitch "--part" -Selection $validation
                $slots = Get-ManifestArrayProperty -Entry $validation -Name "slots"
                if ($slots.Count -eq 0) {
                    throw "Template validation '$validationName' must provide at least one slot."
                }
                foreach ($slot in $slots) {
                    $arguments += @("--slot", (Convert-SlotSpecToCliString -SlotSpec $slot))
                }
                $arguments += "--json"

                $outputPath = Join-Path $entryDir ("template_validation_{0:00}.json" -f $validationIndex)
                $result = Invoke-CliJsonCommand `
                    -ExecutablePath $cliPath `
                    -Arguments $arguments `
                    -OutputPath $outputPath `
                    -FailureLabel "validate-template/$validationName"

                $templateValidationResults.Add([pscustomobject]@{
                    name = $validationName
                    passed = [bool]$result.Json.passed
                    output_json = $outputPath
                    result = $result.Json
                }) | Out-Null

                if (-not [bool]$result.Json.passed) {
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $entryPassed = $false
            }
        }

        $schemaValidation = $entry.PSObject.Properties["schema_validation"]
        if ($null -ne $schemaValidation -and $null -ne $schemaValidation.Value) {
            $schemaValidationResult.enabled = $true
            try {
                $arguments = @("validate-template-schema", $resolvedInputDocx)
                $schemaValidationBlock = $schemaValidation.Value
                $schemaFile = Resolve-OptionalManifestPropertyValue -Entry $schemaValidationBlock -Name "schema_file"
                if (-not [string]::IsNullOrWhiteSpace($schemaFile)) {
                    $resolvedSchemaFile = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $schemaFile
                    $arguments += @("--schema-file", $resolvedSchemaFile)
                    $schemaValidationResult.schema_file = $resolvedSchemaFile
                }

                $schemaTargets = Get-ManifestArrayProperty -Entry $schemaValidationBlock -Name "targets"
                foreach ($target in $schemaTargets) {
                    $arguments = Add-PartSelectionArguments -Arguments $arguments -PartSwitch "--target" -Selection $target
                    $targetSlots = Get-ManifestArrayProperty -Entry $target -Name "slots"
                    if ($targetSlots.Count -eq 0) {
                        throw "Every schema_validation target must provide at least one slot."
                    }
                    foreach ($slot in $targetSlots) {
                        $arguments += @("--slot", (Convert-SlotSpecToCliString -SlotSpec $slot))
                    }
                }

                if ([string]::IsNullOrWhiteSpace($schemaFile) -and $schemaTargets.Count -eq 0) {
                    throw "schema_validation must provide 'schema_file' or at least one target."
                }

                $arguments += "--json"
                $outputPath = Join-Path $entryDir "schema_validation.json"
                $result = Invoke-CliJsonCommand `
                    -ExecutablePath $cliPath `
                    -Arguments $arguments `
                    -OutputPath $outputPath `
                    -FailureLabel "validate-template-schema/$name"

                $schemaValidationResult.output_json = $outputPath
                $schemaValidationResult.passed = [bool]$result.Json.passed
                $schemaValidationResult.result = $result.Json
                if (-not [bool]$result.Json.passed) {
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $schemaValidationResult.passed = $false
                $entryPassed = $false
            }
        }

        $schemaBaseline = $entry.PSObject.Properties["schema_baseline"]
        if ($null -ne $schemaBaseline -and $null -ne $schemaBaseline.Value) {
            $schemaBaselineResult.enabled = $true
            try {
                $schemaBaselineBlock = $schemaBaseline.Value
                $schemaFile = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "schema_file"
                if ([string]::IsNullOrWhiteSpace($schemaFile)) {
                    throw "schema_baseline must provide a non-empty 'schema_file'."
                }
                $resolvedSchemaFile = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $schemaFile
                $generatedOutput = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "generated_output"
                if ([string]::IsNullOrWhiteSpace($generatedOutput)) {
                    $generatedOutput = Join-Path $entryDir "generated_template_schema.json"
                }
                $resolvedGeneratedOutput = Resolve-ManifestPathValue `
                    -RepoRoot $repoRoot `
                    -InputPath $generatedOutput `
                    -AllowMissing
                Ensure-PathParent -Path $resolvedGeneratedOutput
                $repairedOutput = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "repaired_output"
                if ([string]::IsNullOrWhiteSpace($repairedOutput)) {
                    $generatedOutputDirectory = [System.IO.Path]::GetDirectoryName($resolvedGeneratedOutput)
                    $generatedOutputStem = [System.IO.Path]::GetFileNameWithoutExtension($resolvedGeneratedOutput)
                    if ([string]::IsNullOrWhiteSpace($generatedOutputDirectory)) {
                        $generatedOutputDirectory = $entryDir
                    }
                    if ([string]::IsNullOrWhiteSpace($generatedOutputStem)) {
                        $generatedOutputStem = "generated_template_schema"
                    }
                    $resolvedRepairedOutput = Join-Path $generatedOutputDirectory ($generatedOutputStem + ".repaired.schema.json")
                } else {
                    $resolvedRepairedOutput = Resolve-ManifestPathValue `
                        -RepoRoot $repoRoot `
                        -InputPath $repairedOutput `
                        -AllowMissing
                }
                Ensure-PathParent -Path $resolvedRepairedOutput
                $reviewJsonOutput = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "review_json_output"
                if ([string]::IsNullOrWhiteSpace($reviewJsonOutput)) {
                    $reviewJsonOutput = Join-Path $entryDir "schema_patch_review.json"
                }
                $resolvedReviewJsonOutput = Resolve-ManifestPathValue `
                    -RepoRoot $repoRoot `
                    -InputPath $reviewJsonOutput `
                    -AllowMissing
                Ensure-PathParent -Path $resolvedReviewJsonOutput
                $approvalResultPath = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "approval_result"
                if ([string]::IsNullOrWhiteSpace($approvalResultPath)) {
                    $approvalResultPath = Join-Path $entryDir "schema_patch_approval_result.json"
                }
                $resolvedApprovalResultPath = Resolve-ManifestPathValue `
                    -RepoRoot $repoRoot `
                    -InputPath $approvalResultPath `
                    -AllowMissing
                Ensure-PathParent -Path $resolvedApprovalResultPath

                $targetMode = Resolve-OptionalManifestPropertyValue -Entry $schemaBaselineBlock -Name "target_mode"
                if ([string]::IsNullOrWhiteSpace($targetMode)) {
                    $targetMode = "default"
                }

                $logPath = Join-Path $entryDir "schema_baseline.log"
                $result = Invoke-BaselineCheck `
                    -ScriptPath $baselineScriptPath `
                    -InputDocx $resolvedInputDocx `
                    -SchemaFile $resolvedSchemaFile `
                    -GeneratedSchemaOutput $resolvedGeneratedOutput `
                    -RepairedSchemaOutput $resolvedRepairedOutput `
                    -ReviewJsonOutput $resolvedReviewJsonOutput `
                    -BuildDir $BuildDir `
                    -TargetMode $targetMode `
                    -SkipBuildRequested ([bool]$SkipBuild.IsPresent) `
                    -LogPath $logPath

                $schemaBaselineResult.schema_file = $resolvedSchemaFile
                $schemaBaselineResult.generated_output = $resolvedGeneratedOutput
                $schemaBaselineResult.repaired_output = $resolvedRepairedOutput
                $schemaBaselineResult.schema_patch_review_output_path = $resolvedReviewJsonOutput
                $schemaBaselineResult.schema_patch_approval_result_path = $resolvedApprovalResultPath
                $schemaBaselineResult.target_mode = $targetMode
                $schemaBaselineResult.exit_code = [int]$result.ExitCode
                $schemaBaselineResult.matches = [bool]$result.Json.matches
                $schemaBaselineResult.schema_lint_clean = [bool]$result.LintJson.clean
                $schemaBaselineResult.schema_lint_issue_count = [int]$result.LintJson.issue_count
                $schemaBaselineResult.schema_lint_duplicate_target_count = [int]$result.LintJson.duplicate_target_count
                $schemaBaselineResult.schema_lint_duplicate_slot_count = [int]$result.LintJson.duplicate_slot_count
                $schemaBaselineResult.schema_lint_target_order_issue_count = [int]$result.LintJson.target_order_issue_count
                $schemaBaselineResult.schema_lint_slot_order_issue_count = [int]$result.LintJson.slot_order_issue_count
                $schemaBaselineResult.schema_lint_entry_name_issue_count = [int]$result.LintJson.entry_name_issue_count
                $schemaBaselineResult.repaired_schema_output_path = if ($null -ne $result.RepairJson) {
                    [string]$result.RepairJson.output_path
                } else {
                    ""
                }
                $schemaBaselineResult.log_path = $logPath
                $schemaBaselineResult.result = $result.Json
                $schemaBaselineResult.lint_result = $result.LintJson
                if ($null -ne $result.ReviewSummary) {
                    $schemaBaselineResult.schema_patch_review = $result.ReviewSummary
                }
                if ($null -ne $result.ReviewJson) {
                    $schemaBaselineResult.schema_patch_review_json = $result.ReviewJson
                }
                $approvalResult = $null
                if ($null -ne $result.ReviewSummary -and [bool]$result.ReviewSummary.changed) {
                    if (-not [System.IO.File]::Exists($resolvedApprovalResultPath)) {
                        $approvalTemplate = New-SchemaPatchApprovalResultTemplate `
                            -Name $name `
                            -SchemaBaseline $schemaBaselineResult
                        ($approvalTemplate | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $resolvedApprovalResultPath -Encoding UTF8
                    }
                    $approvalResult = Read-JsonFileIfPresent -Path $resolvedApprovalResultPath
                    if ($null -ne $approvalResult) {
                        $schemaBaselineResult.schema_patch_approval_result = $approvalResult
                    }
                }
                $schemaPatchApproval = New-SchemaPatchApprovalSummary `
                    -Name $name `
                    -SchemaBaseline $schemaBaselineResult `
                    -ApprovalResult $approvalResult `
                    -ApprovalResultPath $resolvedApprovalResultPath
                if ($null -ne $schemaPatchApproval) {
                    $schemaBaselineResult.schema_patch_approval = $schemaPatchApproval
                }
                if ($null -ne $result.RepairJson) {
                    $schemaBaselineResult.repair_result = $result.RepairJson
                }
                if (-not [bool]$result.Json.matches) {
                    $entryIssues.Add("Schema baseline drift detected.") | Out-Null
                    $entryPassed = $false
                }
                if (-not [bool]$result.LintJson.clean) {
                    $entryIssues.Add("Schema baseline lint failed with $([int]$result.LintJson.issue_count) issue(s).") | Out-Null
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $schemaBaselineResult.matches = $false
                $entryPassed = $false
            }
        }

        $renderDataSmoke = $entry.PSObject.Properties["render_data_smoke"]
        if ($null -ne $renderDataSmoke -and $null -ne $renderDataSmoke.Value) {
            $renderDataResult.enabled = $true
            try {
                $renderDataBlock = $renderDataSmoke.Value
                $dataPath = Resolve-OptionalManifestPropertyValue -Entry $renderDataBlock -Name "data_path"
                $mappingPath = Resolve-OptionalManifestPropertyValue -Entry $renderDataBlock -Name "mapping_path"
                if ([string]::IsNullOrWhiteSpace($dataPath)) {
                    throw "render_data_smoke must provide a non-empty 'data_path'."
                }
                if ([string]::IsNullOrWhiteSpace($mappingPath)) {
                    throw "render_data_smoke must provide a non-empty 'mapping_path'."
                }

                $resolvedDataPath = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $dataPath
                $resolvedMappingPath = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $mappingPath
                $outputDocx = Resolve-OptionalManifestPropertyValue -Entry $renderDataBlock -Name "output_docx"
                if ([string]::IsNullOrWhiteSpace($outputDocx)) {
                    $outputDocx = Join-Path $entryDir "rendered.docx"
                }
                $resolvedOutputDocx = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $outputDocx -AllowMissing
                Ensure-PathParent -Path $resolvedOutputDocx
                $summaryJson = Resolve-OptionalManifestPropertyValue -Entry $renderDataBlock -Name "summary_json"
                if ([string]::IsNullOrWhiteSpace($summaryJson)) {
                    $summaryJson = Join-Path $entryDir "render_data.summary.json"
                }
                $resolvedSummaryJson = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $summaryJson -AllowMissing
                Ensure-PathParent -Path $resolvedSummaryJson

                $patchPlanOutput = Resolve-OptionalManifestPropertyValue -Entry $renderDataBlock -Name "patch_plan_output"
                if ([string]::IsNullOrWhiteSpace($patchPlanOutput)) {
                    $patchPlanOutput = Join-Path $entryDir "render_patch.generated.json"
                }
                $resolvedPatchPlanOutput = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $patchPlanOutput -AllowMissing
                Ensure-PathParent -Path $resolvedPatchPlanOutput
                $draftPlanOutput = Resolve-OptionalManifestPropertyValue -Entry $renderDataBlock -Name "draft_plan_output"
                if ([string]::IsNullOrWhiteSpace($draftPlanOutput)) {
                    $draftPlanOutput = Join-Path $entryDir "render_plan.draft.json"
                }
                $resolvedDraftPlanOutput = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $draftPlanOutput -AllowMissing
                Ensure-PathParent -Path $resolvedDraftPlanOutput
                $patchedPlanOutput = Resolve-OptionalManifestPropertyValue -Entry $renderDataBlock -Name "patched_plan_output"
                if ([string]::IsNullOrWhiteSpace($patchedPlanOutput)) {
                    $patchedPlanOutput = Join-Path $entryDir "render_plan.patched.json"
                }
                $resolvedPatchedPlanOutput = Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $patchedPlanOutput -AllowMissing
                Ensure-PathParent -Path $resolvedPatchedPlanOutput

                $exportTargetMode = Resolve-OptionalManifestPropertyValue -Entry $renderDataBlock -Name "export_target_mode"
                if ([string]::IsNullOrWhiteSpace($exportTargetMode)) {
                    $exportTargetMode = "loaded-parts"
                }

                $logPath = Join-Path $entryDir "render_data.log"
                $result = Invoke-RenderDataSmoke `
                    -ScriptPath $renderDataScriptPath `
                    -InputDocx $resolvedInputDocx `
                    -DataPath $resolvedDataPath `
                    -MappingPath $resolvedMappingPath `
                    -OutputDocx $resolvedOutputDocx `
                    -SummaryJson $resolvedSummaryJson `
                    -PatchPlanOutput $resolvedPatchPlanOutput `
                    -DraftPlanOutput $resolvedDraftPlanOutput `
                    -PatchedPlanOutput $resolvedPatchedPlanOutput `
                    -BuildDir $BuildDir `
                    -Generator $Generator `
                    -ExportTargetMode $exportTargetMode `
                    -SkipBuildRequested ([bool]$SkipBuild.IsPresent) `
                    -LogPath $logPath

                $renderDataResult.status = [string]$result.Summary.status
                $renderDataResult.output_docx = $resolvedOutputDocx
                $renderDataResult.data_path = $resolvedDataPath
                $renderDataResult.mapping_path = $resolvedMappingPath
                $renderDataResult.summary_json = $resolvedSummaryJson
                $renderDataResult.patch_plan = $resolvedPatchPlanOutput
                $renderDataResult.draft_plan = $resolvedDraftPlanOutput
                $renderDataResult.patched_plan = $resolvedPatchedPlanOutput
                $renderDataResult.export_target_mode = $exportTargetMode
                $renderDataResult.log_path = $logPath
                $renderDataResult.remaining_placeholder_count = if ($null -ne (Get-OptionalObjectPropertyObject -Object $result.Summary -Name "remaining_placeholder_count")) {
                    [int]$result.Summary.remaining_placeholder_count
                } else {
                    $null
                }
                $renderDataResult.render_operation_count = if ($null -ne (Get-OptionalObjectPropertyObject -Object $result.Summary -Name "render_operation_count")) {
                    [int]$result.Summary.render_operation_count
                } else {
                    $null
                }
                $renderDataResult.result = $result.Summary
                if ([string]$result.Summary.status -ne "completed") {
                    $entryPassed = $false
                }
            } catch {
                $entryIssues.Add($_.Exception.Message) | Out-Null
                $renderDataResult.status = "failed"
                $entryPassed = $false
            }
        }

        $visualSmoke = $entry.PSObject.Properties["visual_smoke"]
        if ($null -ne $visualSmoke -and $null -ne $visualSmoke.Value) {
            $visualSmokeEnabled = $true
            if ($visualSmoke.Value -is [bool]) {
                $visualSmokeEnabled = [bool]$visualSmoke.Value
            } else {
                $enabledValue = Resolve-OptionalManifestPropertyValue -Entry $visualSmoke.Value -Name "enabled"
                if (-not [string]::IsNullOrWhiteSpace($enabledValue)) {
                    $visualSmokeEnabled = [System.Convert]::ToBoolean($enabledValue)
                }
            }

            if ($visualSmokeEnabled) {
                $visualSmokeResult.enabled = $true
                if ($SkipVisualSmoke) {
                    $visualSmokeResult.review_status = "skipped"
                    $visualSmokeResult.review_verdict = "skipped"
                    $visualSmokeResult.findings_count = 0
                    $visualSmokeResult.reason = "Skipped by -SkipVisualSmoke."
                } else {
                    try {
                        $visualOutputDir = if ($visualSmoke.Value -is [bool]) {
                            Join-Path $entryDir "visual-smoke"
                        } else {
                            $configuredOutputDir = Resolve-OptionalManifestPropertyValue -Entry $visualSmoke.Value -Name "output_dir"
                            if ([string]::IsNullOrWhiteSpace($configuredOutputDir)) {
                                Join-Path $entryDir "visual-smoke"
                            } else {
                                Resolve-ManifestPathValue -RepoRoot $repoRoot -InputPath $configuredOutputDir -AllowMissing
                            }
                        }
                        $visualInputDocx = $resolvedInputDocx
                        if ($visualSmoke.Value -isnot [bool]) {
                            $visualInput = Resolve-OptionalManifestPropertyValue -Entry $visualSmoke.Value -Name "input"
                            if ($visualInput -eq "rendered_docx") {
                                $renderedDocx = Get-OptionalObjectPropertyValue -Object $renderDataResult -Name "output_docx"
                                if ([string]::IsNullOrWhiteSpace($renderedDocx)) {
                                    throw "visual_smoke input 'rendered_docx' requires render_data_smoke to complete first."
                                }
                                $visualInputDocx = $renderedDocx
                            }
                        }

                        $logPath = Join-Path $entryDir "visual_smoke.log"
                        $result = Invoke-VisualSmoke `
                            -ScriptPath $visualSmokeScriptPath `
                            -InputDocx $visualInputDocx `
                            -OutputDir $visualOutputDir `
                            -Dpi $Dpi `
                            -KeepWordOpenRequested ([bool]$KeepWordOpen.IsPresent) `
                            -VisibleWordRequested ([bool]$VisibleWord.IsPresent) `
                            -LogPath $logPath

                        $visualSmokeResult.input_docx = $visualInputDocx
                        $visualSmokeResult.output_dir = $visualOutputDir
                        $visualSmokeResult.log_path = $logPath
                        $visualSmokeResult.summary_json = $result.SummaryPath
                        $visualSmokeResult.review_result_json = $result.ReviewResultPath
                        $visualSmokeResult.contact_sheet = $result.ContactSheetPath
                        $visualSmokeResult.page_count = [int]$result.Summary.page_count
                        $visualSmokeResult.review_status = [string]$result.ReviewResult.status
                        $visualSmokeResult.review_verdict = Normalize-ProjectTemplateSmokeVisualVerdict -Value ([string]$result.ReviewResult.verdict)
                        $visualSmokeResult.findings_count = @(Get-OptionalObjectArrayProperty -Object $result.ReviewResult -Name "findings").Count
                        $visualReviewState = Get-ProjectTemplateSmokeVisualReviewState `
                            -ReviewStatus $visualSmokeResult.review_status `
                            -ReviewVerdict $visualSmokeResult.review_verdict
                        if ($visualReviewState.failed) {
                            $entryPassed = $false
                        }
                        $manualReviewPending = $visualReviewState.manual_review_pending
                        $visualReviewUndetermined = $visualReviewState.review_undetermined
                    } catch {
                        $entryIssues.Add($_.Exception.Message) | Out-Null
                        $visualSmokeResult.review_status = "failed"
                        $visualSmokeResult.review_verdict = "fail"
                        $visualSmokeResult.findings_count = 0
                        $entryPassed = $false
                    }
                }
            }
        }
    }

    $status = Get-ProjectTemplateSmokeEntryStatus `
        -EntryPassed $entryPassed `
        -ManualReviewPending $manualReviewPending `
        -VisualReviewUndetermined $visualReviewUndetermined

    if (-not $entryPassed) {
        $failedEntryCount += 1
    }
    if ([bool](Get-OptionalObjectPropertyObject -Object $schemaBaselineResult -Name "enabled") -and
        $null -ne (Get-OptionalObjectPropertyObject -Object $schemaBaselineResult -Name "schema_lint_clean") -and
        -not [bool](Get-OptionalObjectPropertyObject -Object $schemaBaselineResult -Name "schema_lint_clean")) {
        $dirtySchemaBaselineCount += 1
    }
    if ($manualReviewPending) {
        $manualReviewPendingCount += 1
    }
    if ($visualReviewUndetermined) {
        $visualReviewUndeterminedCount += 1
    }

    $results.Add([pscustomobject]@{
        name = $name
        input_docx = $resolvedInputDocx
        business_document_type = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "business_document_type"
        corpus_role = Resolve-OptionalManifestPropertyValue -Entry $entry -Name "corpus_role"
        artifact_dir = $entryDir
        status = $status
        passed = $entryPassed
        manual_review_pending = $manualReviewPending
        checks = [ordered]@{
            template_validations = $templateValidationResults.ToArray()
            schema_validation = $schemaValidationResult
            schema_baseline = $schemaBaselineResult
            render_data = $renderDataResult
            visual_smoke = $visualSmokeResult
        }
        issues = $entryIssues.ToArray()
    }) | Out-Null
}

$summaryPassed = $failedEntryCount -eq 0
$visualSmokeResults = @(
    foreach ($result in $results) {
        $checks = Get-OptionalObjectPropertyObject -Object $result -Name "checks"
        $visualSmoke = Get-OptionalObjectPropertyObject -Object $checks -Name "visual_smoke"
        if ($null -ne $visualSmoke -and [bool](Get-OptionalObjectPropertyObject -Object $visualSmoke -Name "enabled")) {
            $visualSmoke
        }
    }
)
$visualEntryCount = $visualSmokeResults.Count
$schemaPatchReviews = @(
    foreach ($result in $results) {
        $checks = Get-OptionalObjectPropertyObject -Object $result -Name "checks"
        $schemaBaseline = Get-OptionalObjectPropertyObject -Object $checks -Name "schema_baseline"
        $reviewSummary = Get-OptionalObjectPropertyObject -Object $schemaBaseline -Name "schema_patch_review"
        if ($null -ne $reviewSummary) {
            [pscustomobject]@{
                name = [string]$result.name
                review_json = Get-OptionalObjectPropertyValue -Object $schemaBaseline -Name "schema_patch_review_output_path"
                changed = [bool]$reviewSummary.changed
                baseline_slot_count = [int]$reviewSummary.baseline_slot_count
                generated_slot_count = [int]$reviewSummary.generated_slot_count
                upsert_slot_count = [int]$reviewSummary.upsert_slot_count
                remove_target_count = [int]$reviewSummary.remove_target_count
                remove_slot_count = [int]$reviewSummary.remove_slot_count
                rename_slot_count = [int]$reviewSummary.rename_slot_count
                update_slot_count = [int]$reviewSummary.update_slot_count
                inserted_slots = [int]$reviewSummary.inserted_slots
                replaced_slots = [int]$reviewSummary.replaced_slots
            }
        }
    }
)
$schemaPatchReviewChangedCount = @($schemaPatchReviews | Where-Object { $_.changed }).Count
$schemaPatchApprovalItems = @(
    foreach ($result in $results) {
        $checks = Get-OptionalObjectPropertyObject -Object $result -Name "checks"
        $schemaBaseline = Get-OptionalObjectPropertyObject -Object $checks -Name "schema_baseline"
        $schemaPatchApproval = Get-OptionalObjectPropertyObject -Object $schemaBaseline -Name "schema_patch_approval"
        if ($null -ne $schemaPatchApproval) {
            $schemaPatchApproval
        }
    }
)
$schemaPatchApprovalPendingCount = @($schemaPatchApprovalItems | Where-Object { [bool]$_.pending }).Count
$schemaPatchApprovalApprovedCount = @($schemaPatchApprovalItems | Where-Object { [bool]$_.approved }).Count
$schemaPatchApprovalRejectedCount = @($schemaPatchApprovalItems | Where-Object { $_.status -in @("rejected", "needs_changes", "invalid_result") }).Count
$schemaPatchApprovalInvalidResultCount = @($schemaPatchApprovalItems | Where-Object { $_.status -eq "invalid_result" }).Count
$schemaPatchApprovalComplianceIssueCount = 0
foreach ($item in @($schemaPatchApprovalItems)) {
    $itemComplianceIssueCount = Get-OptionalObjectPropertyValue -Object $item -Name "compliance_issue_count"
    if (-not [string]::IsNullOrWhiteSpace($itemComplianceIssueCount)) {
        $schemaPatchApprovalComplianceIssueCount += [int]$itemComplianceIssueCount
    }
}
$schemaPatchApprovalItemCount = @($schemaPatchApprovalItems).Count
$schemaPatchApprovalGateStatus = Get-SchemaPatchApprovalGateStatus `
    -PendingCount $schemaPatchApprovalPendingCount `
    -ApprovalItemCount $schemaPatchApprovalItemCount `
    -ApprovedCount $schemaPatchApprovalApprovedCount `
    -RejectedCount $schemaPatchApprovalRejectedCount `
    -InvalidResultCount $schemaPatchApprovalInvalidResultCount `
    -ComplianceIssueCount $schemaPatchApprovalComplianceIssueCount
$schemaPatchApprovalGateBlocked = $schemaPatchApprovalGateStatus -eq "blocked"
$visualVerdict = Get-ProjectTemplateSmokeVisualVerdict -VisualSmokeResults $visualSmokeResults
$overallStatus = Get-ProjectTemplateSmokeOverallStatus `
    -SummaryPassed $summaryPassed `
    -ManualReviewPendingCount $manualReviewPendingCount `
    -VisualReviewUndeterminedCount $visualReviewUndeterminedCount

$summary = [ordered]@{
    schema = $projectTemplateSmokeSummarySchema
    generated_at = (Get-Date).ToString("s")
    manifest_path = $resolvedManifestPath
    workspace = $repoRoot
    build_dir = $resolvedBuildDir
    output_dir = $resolvedOutputDir
    entry_count = $results.Count
    failed_entry_count = $failedEntryCount
    dirty_schema_baseline_count = $dirtySchemaBaselineCount
    schema_patch_review_count = $schemaPatchReviews.Count
    schema_patch_review_changed_count = $schemaPatchReviewChangedCount
    schema_patch_reviews = $schemaPatchReviews
    schema_patch_approval_pending_count = $schemaPatchApprovalPendingCount
    schema_patch_approval_approved_count = $schemaPatchApprovalApprovedCount
    schema_patch_approval_rejected_count = $schemaPatchApprovalRejectedCount
    schema_patch_approval_compliance_issue_count = $schemaPatchApprovalComplianceIssueCount
    schema_patch_approval_invalid_result_count = $schemaPatchApprovalInvalidResultCount
    schema_patch_approval_gate_status = $schemaPatchApprovalGateStatus
    schema_patch_approval_gate_blocked = $schemaPatchApprovalGateBlocked
    schema_patch_approval_items = $schemaPatchApprovalItems
    visual_entry_count = $visualEntryCount
    visual_verdict = $visualVerdict
    manual_review_pending_count = $manualReviewPendingCount
    visual_review_undetermined_count = $visualReviewUndeterminedCount
    passed = $summaryPassed
    overall_status = $overallStatus
    entries = $results
}

($summary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
Write-SummaryMarkdown -Path $summaryMarkdownPath -RepoRoot $repoRoot -Summary $summary

Write-Step "Summary JSON: $summaryPath"
Write-Step "Summary Markdown: $summaryMarkdownPath"
Write-Step "Overall status: $overallStatus"

if (-not $summaryPassed) {
    exit 1
}

exit 0
