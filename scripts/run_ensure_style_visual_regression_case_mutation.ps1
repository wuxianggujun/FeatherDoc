    $caseDir = Join-Path $resolvedOutputDir $case.id
    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $mutationJsonPath = Join-Path $caseDir "mutation_result.json"
    $visualDir = Join-Path $caseDir "mutated-visual"
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    Write-Step "Running case '$($case.id)'"
    $mutationSteps = @()
    if ($null -ne $case.steps) {
        $currentInputDocx = $baselineDocxPath
        for ($stepIndex = 0; $stepIndex -lt $case.steps.Count; ++$stepIndex) {
            $step = $case.steps[$stepIndex]
            $stepJsonPath = Join-Path $caseDir ("mutation-step-{0:D2}.json" -f ($stepIndex + 1))
            $stepOutputDocxPath =
                if ($stepIndex -eq ($case.steps.Count - 1)) {
                    $mutatedDocxPath
                } else {
                    Join-Path $caseDir ("mutation-step-{0:D2}.docx" -f ($stepIndex + 1))
                }

            $stepArguments = @($step.command, $currentInputDocx)
            $stepArguments += $step.arguments
            $stepArguments += @("--output", $stepOutputDocxPath, "--json")

            Invoke-Capture `
                -Executable $cliExecutable `
                -Arguments $stepArguments `
                -OutputPath $stepJsonPath `
                -FailureMessage "Failed to run step $($stepIndex + 1) for case '$($case.id)'."

            $mutationSteps += [ordered]@{
                command = $step.command
                mutation_json = $stepJsonPath
                output_docx = $stepOutputDocxPath
            }
            $currentInputDocx = $stepOutputDocxPath
        }
        $mutationJsonPath = $mutationSteps[$mutationSteps.Count - 1].mutation_json
    } else {
        $arguments = @($case.command, $baselineDocxPath)
        $arguments += $case.arguments
        $arguments += @("--output", $mutatedDocxPath, "--json")

        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments $arguments `
            -OutputPath $mutationJsonPath `
            -FailureMessage "Failed to run case '$($case.id)'."

        $mutationSteps += [ordered]@{
            command = $case.command
            mutation_json = $mutationJsonPath
            output_docx = $mutatedDocxPath
        }
    }

    if ($case.id -eq "ensure-paragraph-style-heading-visual") {
        Assert-EnsureParagraphStyleResult `
            -JsonPath $mutationJsonPath `
            -ExpectedStyleId "ReviewPara" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Heading2" `
            -ExpectedUnhideWhenUsed $true `
            -ExpectedQuickFormat $true `
            -Label $case.id
    } elseif ($case.id -eq "materialize-style-run-properties-child-freeze-visual" `
              -or $case.id -eq "rebase-paragraph-style-based-on-child-preserve-visual") {
        Assert-EnsureParagraphStyleResult `
            -JsonPath $mutationJsonPath `
            -ExpectedStyleId "ReviewPara" `
            -ExpectedName "Review Paragraph" `
            -ExpectedBasedOn "Normal" `
            -ExpectedUnhideWhenUsed $false `
            -ExpectedQuickFormat $true `
            -Label $case.id
    } elseif ($case.id -eq "rebase-character-style-based-on-child-preserve-visual") {
        $rebaseResult = Get-Content -Raw -Path $mutationJsonPath | ConvertFrom-Json
        if ($rebaseResult.command -ne "rebase-character-style-based-on") {
            throw "Unexpected command '$($rebaseResult.command)' in case '$($case.id)'."
        }
        if (-not $rebaseResult.ok) {
            throw "Character style rebase case '$($case.id)' did not report success."
        }
        if ($rebaseResult.style_id -ne "AccentMarkerChild") {
            throw "Expected style_id AccentMarkerChild in case '$($case.id)'."
        }
        if ($rebaseResult.based_on -ne "AccentParentSerif") {
            throw "Expected based_on AccentParentSerif in case '$($case.id)'."
        }
        $preservedFields = @($rebaseResult.preserved | ForEach-Object { $_.field })
        if (@("font_family", "rtl") | Where-Object { $_ -notin $preservedFields }) {
            throw "Character style rebase case '$($case.id)' did not preserve the expected inherited fields."
        }
    } elseif ($case.command -eq "ensure-character-style") {
        Assert-EnsureCharacterStyleResult `
            -JsonPath $mutationJsonPath `
            -ExpectedStyleId "AccentMarker" `
            -ExpectedName "Accent Marker" `
            -ExpectedBasedOn "DefaultParagraphFont" `
            -ExpectedQuickFormat $true `
            -Label $case.id
    } elseif ($case.command -eq "ensure-table-style") {
        Assert-EnsureTableStyleResult `
            -JsonPath $mutationJsonPath `
            -ExpectedStyleId $reviewTableStyleId `
            -ExpectedName $reviewTableStyleName `
            -ExpectedBasedOn "TableNormal" `
            -ExpectedUnhideWhenUsed $true `
            -ExpectedQuickFormat $true `
            -Label $case.id
    } else {
        throw "Unsupported command '$($case.command)' in case '$($case.id)'."
    }
