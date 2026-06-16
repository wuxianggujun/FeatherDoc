function Complete-NormalOperationArguments {
    param(
        [string]$NormalizedOp,
        [System.Collections.Generic.List[string]]$Arguments,
        [string]$OutputPath
    )

    $Arguments.Add("--output") | Out-Null
    $Arguments.Add($OutputPath) | Out-Null
    $Arguments.Add("--json") | Out-Null

    return [pscustomobject]@{
        Operation = $NormalizedOp
        Arguments = @($Arguments.ToArray())
        Command = [string]$Arguments[0]
        DirectOperation = $false
    }
}

function New-OperationArguments {
    param(
        $Operation,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Label,
        [int]$OperationIndex,
        [string]$TemporaryRoot
    )

    $op = (Get-RequiredObjectPropertyValue -Object $Operation -Name "op" -Label $Label).Trim()
    $normalizedOp = $op -replace '-', '_'
    $arguments = New-Object 'System.Collections.Generic.List[string]'
    $handlerNames = @(
        "Invoke-EditPlanBookmarkMediaOperationArguments",
        "Invoke-EditPlanReviewOperationArguments",
        "Invoke-EditPlanFieldOperationArguments",
        "Invoke-EditPlanContentControlOperationArguments",
        "Invoke-EditPlanTableStructureOperationArguments",
        "Invoke-EditPlanTablePropertyOperationArguments",
        "Invoke-EditPlanStyleOperationArguments",
        "Invoke-EditPlanSectionOperationArguments"
    )

    foreach ($handlerName in $handlerNames) {
        $handlerOutput = @(& $handlerName `
                -Operation $Operation `
                -InputPath $InputPath `
                -OutputPath $OutputPath `
                -Label $Label `
                -OperationIndex $OperationIndex `
                -TemporaryRoot $TemporaryRoot `
                -NormalizedOp $normalizedOp `
                -Arguments $arguments)
        if ($handlerOutput.Count -eq 0) {
            continue
        }

        $handlerResult = $handlerOutput[-1]
        if ($handlerResult -is [bool]) {
            if (-not $handlerResult) {
                continue
            }

            return Complete-NormalOperationArguments `
                -NormalizedOp $normalizedOp `
                -Arguments $arguments `
                -OutputPath $OutputPath
        }

        return $handlerResult
    }

    throw "$Label has unsupported op '$op'."
}

function New-OperationRecord {
    param(
        [int]$Index,
        [string]$Op,
        [string]$Command,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Status,
        [int]$ExitCode,
        [string[]]$Output,
        [string]$ErrorMessage
    )

    $record = [ordered]@{
        index = $Index
        op = $Op
        command = $Command
        input_docx = $InputPath
        output_docx = $OutputPath
        status = $Status
        exit_code = $ExitCode
        output = @($Output)
    }
    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $record.error = $ErrorMessage
    }

    return $record
}

function Build-EditSummary {
    param(
        [string]$Status,
        [string]$InputDocx,
        [string]$EditPlan,
        [string]$OutputDocx,
        [string]$SummaryJson,
        [string]$CliPath,
        [string]$BuildDir,
        [string]$Generator,
        [bool]$SkipBuild,
        [object[]]$Operations,
        [string]$ErrorMessage
    )

    $summary = [ordered]@{
        generated_at = (Get-Date).ToString("s")
        status = $Status
        input_docx = $InputDocx
        edit_plan = $EditPlan
        output_docx = $OutputDocx
        summary_json = $SummaryJson
        cli_path = $CliPath
        build_dir = $BuildDir
        generator = $Generator
        skip_build = $SkipBuild
        operation_count = @($Operations).Count
        operations = @($Operations)
    }

    if (-not [string]::IsNullOrWhiteSpace($ErrorMessage)) {
        $summary.error = $ErrorMessage
    }

    return $summary
}
