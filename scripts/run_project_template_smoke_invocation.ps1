function Invoke-BaselineCheck {
    param(
        [string]$ScriptPath,
        [string]$InputDocx,
        [string]$SchemaFile,
        [string]$GeneratedSchemaOutput,
        [string]$RepairedSchemaOutput,
        [string]$ReviewJsonOutput,
        [string]$BuildDir,
        [string]$TargetMode,
        [bool]$SkipBuildRequested,
        [string]$LogPath
    )

    $scriptArgs = @(
        "-InputDocx"
        $InputDocx
        "-SchemaFile"
        $SchemaFile
        "-GeneratedSchemaOutput"
        $GeneratedSchemaOutput
        "-BuildDir"
        $BuildDir
    )
    if (-not [string]::IsNullOrWhiteSpace($RepairedSchemaOutput)) {
        $scriptArgs += @(
            "-RepairedSchemaOutput"
            $RepairedSchemaOutput
        )
    }
    if (-not [string]::IsNullOrWhiteSpace($ReviewJsonOutput)) {
        $scriptArgs += @(
            "-ReviewJsonOutput"
            $ReviewJsonOutput
        )
    }
    if ($SkipBuildRequested) {
        $scriptArgs += "-SkipBuild"
    }

    switch ($TargetMode) {
        "" { }
        "default" { }
        "section-targets" { $scriptArgs += "-SectionTargets" }
        "resolved-section-targets" { $scriptArgs += "-ResolvedSectionTargets" }
        default { throw "Unsupported schema baseline target_mode '$TargetMode'." }
    }

    $powerShellPath = (Get-Process -Id $PID).Path
    $commandOutput = @(& $powerShellPath -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @scriptArgs 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    Write-CommandOutput -OutputPath $LogPath -Lines $lines

    if ($exitCode -notin @(0, 1)) {
        $joined = if ($lines.Count -gt 0) {
            $lines -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "check_template_schema_baseline.ps1 failed with exit code $exitCode. Output:`n$joined"
    }

    $lintJson = Get-TemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "lint-template-schema"
    $checkJson = Get-TemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "check-template-schema"
    $repairJson = Get-OptionalTemplateSchemaCommandJsonObject `
        -Lines $lines `
        -Command "repair-template-schema"
    $reviewJson = Read-JsonFileIfPresent -Path $ReviewJsonOutput

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Json = $checkJson
        LintJson = $lintJson
        RepairJson = $repairJson
        ReviewJson = $reviewJson
        ReviewSummary = New-SchemaPatchReviewSummary -ReviewJson $reviewJson
    }
}

function Invoke-RenderDataSmoke {
    param(
        [string]$ScriptPath,
        [string]$InputDocx,
        [string]$DataPath,
        [string]$MappingPath,
        [string]$OutputDocx,
        [string]$SummaryJson,
        [string]$PatchPlanOutput,
        [string]$DraftPlanOutput,
        [string]$PatchedPlanOutput,
        [string]$BuildDir,
        [string]$Generator,
        [string]$ExportTargetMode,
        [bool]$SkipBuildRequested,
        [string]$LogPath
    )

    $scriptArgs = @(
        "-InputDocx"
        $InputDocx
        "-DataPath"
        $DataPath
        "-MappingPath"
        $MappingPath
        "-OutputDocx"
        $OutputDocx
        "-SummaryJson"
        $SummaryJson
        "-BuildDir"
        $BuildDir
        "-Generator"
        $Generator
        "-ExportTargetMode"
        $ExportTargetMode
    )

    if (-not [string]::IsNullOrWhiteSpace($PatchPlanOutput)) {
        $scriptArgs += @("-PatchPlanOutput", $PatchPlanOutput)
    }
    if (-not [string]::IsNullOrWhiteSpace($DraftPlanOutput)) {
        $scriptArgs += @("-DraftPlanOutput", $DraftPlanOutput)
    }
    if (-not [string]::IsNullOrWhiteSpace($PatchedPlanOutput)) {
        $scriptArgs += @("-PatchedPlanOutput", $PatchedPlanOutput)
    }
    if ($SkipBuildRequested) {
        $scriptArgs += "-SkipBuild"
    }

    $powerShellPath = (Get-Process -Id $PID).Path
    $commandOutput = @(& $powerShellPath -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @scriptArgs 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    Write-CommandOutput -OutputPath $LogPath -Lines $lines

    $summary = Read-JsonFileIfPresent -Path $SummaryJson
    if ($exitCode -ne 0) {
        $joined = if ($lines.Count -gt 0) {
            $lines -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "render_template_document_from_data.ps1 failed with exit code $exitCode. Output:`n$joined"
    }
    if (-not (Test-Path -LiteralPath $OutputDocx)) {
        throw "Render data smoke did not produce '$OutputDocx'."
    }
    if ($null -eq $summary) {
        throw "Render data smoke summary was not created: $SummaryJson"
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Summary = $summary
    }
}

function Invoke-VisualSmoke {
    param(
        [string]$ScriptPath,
        [string]$InputDocx,
        [string]$OutputDir,
        [int]$Dpi,
        [bool]$KeepWordOpenRequested,
        [bool]$VisibleWordRequested,
        [string]$LogPath
    )

    $scriptArgs = @(
        "-InputDocx"
        $InputDocx
        "-OutputDir"
        $OutputDir
        "-Dpi"
        ([string]$Dpi)
    )
    if ($KeepWordOpenRequested) {
        $scriptArgs += "-KeepWordOpen"
    }
    if ($VisibleWordRequested) {
        $scriptArgs += "-VisibleWord"
    }

    $powerShellPath = (Get-Process -Id $PID).Path
    $commandOutput = @(& $powerShellPath -NoProfile -ExecutionPolicy Bypass -File $ScriptPath @scriptArgs 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })
    Write-CommandOutput -OutputPath $LogPath -Lines $lines

    if ($exitCode -ne 0) {
        $joined = if ($lines.Count -gt 0) {
            $lines -join [System.Environment]::NewLine
        } else {
            "(no output)"
        }
        throw "run_word_visual_smoke.ps1 failed with exit code $exitCode. Output:`n$joined"
    }

    $reportDir = Join-Path $OutputDir "report"
    $summaryPath = Join-Path $reportDir "summary.json"
    $reviewResultPath = Join-Path $reportDir "review_result.json"

    if (-not (Test-Path -LiteralPath $summaryPath)) {
        throw "Visual smoke summary was not created: $summaryPath"
    }
    if (-not (Test-Path -LiteralPath $reviewResultPath)) {
        throw "Visual smoke review_result was not created: $reviewResultPath"
    }

    return [pscustomobject]@{
        Output = $lines
        SummaryPath = $summaryPath
        ReviewResultPath = $reviewResultPath
        Summary = Get-Content -Raw -LiteralPath $summaryPath | ConvertFrom-Json
        ReviewResult = Get-Content -Raw -LiteralPath $reviewResultPath | ConvertFrom-Json
        ContactSheetPath = Join-Path (Join-Path $OutputDir "evidence") "contact_sheet.png"
    }
}
