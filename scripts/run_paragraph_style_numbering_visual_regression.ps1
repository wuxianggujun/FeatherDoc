param(
    [string]$BuildDir = "build-paragraph-style-numbering-visual-nmake",
    [string]$OutputDir = "output/paragraph-style-numbering-visual-regression",
    [int]$Dpi = 144,
    [switch]$SkipBuild,
    [switch]$KeepWordOpen,
    [switch]$VisibleWord
)

$ErrorActionPreference = "Stop"
$script:WordMlNamespace = "http://schemas.openxmlformats.org/wordprocessingml/2006/main"

Add-Type -AssemblyName System.IO.Compression.FileSystem

. (Join-Path $PSScriptRoot "run_paragraph_style_numbering_visual_regression_helpers.ps1")

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $BuildDir
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -InputPath $OutputDir
$wordSmokeScript = Join-Path $repoRoot "scripts\run_word_visual_smoke.ps1"
$contactSheetScript = Join-Path $repoRoot "scripts\build_image_contact_sheet.py"
$vcvarsPath = Get-VcvarsPath

if (-not $SkipBuild) {
    Write-Step "Configuring build directory $resolvedBuildDir"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" -DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=OFF"

    Write-Step "Building featherdoc_cli and paragraph style numbering visual sample"
    Invoke-MsvcCommand `
        -VcvarsPath $vcvarsPath `
        -CommandText "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli featherdoc_sample_paragraph_style_numbering_visual"
}

$cliExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_cli"
$sampleExecutable = Find-BuildExecutable -BuildRoot $resolvedBuildDir -TargetName "featherdoc_sample_paragraph_style_numbering_visual"
$renderPython = Ensure-RenderPython -RepoRoot $repoRoot

New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
$aggregateEvidenceDir = Join-Path $resolvedOutputDir "aggregate-evidence"
$aggregatePagesDir = Join-Path $aggregateEvidenceDir "selected-pages"
$aggregateContactSheetPath = Join-Path $aggregateEvidenceDir "before_after_contact_sheet.png"
New-Item -ItemType Directory -Path $aggregatePagesDir -Force | Out-Null

$legalHeadingText = "Legal heading target: this paragraph already uses the LegalHeading style, so style numbering should appear only after the CLI links a numbering definition to that style."
$legalSubheadingText = "Legal subheading target: this paragraph already uses the LegalSubheading style and should become a nested style-numbered child only in the dual-style case."
$bodyNumberedText = "Body-numbered clear target: this paragraph starts with baseline style numbering and paragraph bidi enabled, so clearing the style numbering should remove only the visible marker."

$baselineDocxPath = Join-Path $resolvedOutputDir "shared-baseline.docx"
$baselineVisualDir = Join-Path $resolvedOutputDir "shared-baseline-visual"

Invoke-Capture `
    -Executable $sampleExecutable `
    -Arguments @($baselineDocxPath) `
    -OutputPath (Join-Path $resolvedOutputDir "shared-baseline.log") `
    -FailureMessage "Failed to generate shared paragraph style numbering baseline sample."

$paragraphExpectations = @(
    [ordered]@{
        id = "legal-heading"
        paragraph_index = 1
        style_id = "LegalHeading"
        text = $legalHeadingText
    },
    [ordered]@{
        id = "legal-subheading"
        paragraph_index = 2
        style_id = "LegalSubheading"
        text = $legalSubheadingText
    },
    [ordered]@{
        id = "body-numbered"
        paragraph_index = 3
        style_id = "BodyNumbered"
        text = $bodyNumberedText
    }
)

$styleExpectations = @(
    [ordered]@{
        style_id = "LegalHeading"
        name = "Legal Heading"
        based_on = "Heading1"
        expect_numbering = $false
        level = $null
        definition_name = $null
        override_count = 0
    },
    [ordered]@{
        style_id = "LegalSubheading"
        name = "Legal Subheading"
        based_on = "Heading2"
        expect_numbering = $false
        level = $null
        definition_name = $null
        override_count = 0
    },
    [ordered]@{
        style_id = "BodyNumbered"
        name = "Body Numbered"
        based_on = "Normal"
        expect_numbering = $true
        level = 0
        definition_name = "BodyStyleOutlineBaseline"
        override_count = 0
    }
)

$summaryBaselineParagraphs = @()
foreach ($paragraphExpectation in $paragraphExpectations) {
    $inspectParagraphJsonPath =
        Join-Path $resolvedOutputDir "baseline-$($paragraphExpectation.id)-paragraph.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-paragraphs",
            $baselineDocxPath,
            "--paragraph",
            "$($paragraphExpectation.paragraph_index)",
            "--json"
        ) `
        -OutputPath $inspectParagraphJsonPath `
        -FailureMessage "Failed to inspect baseline paragraph '$($paragraphExpectation.id)'."

    Assert-ParagraphStyleState `
        -JsonPath $inspectParagraphJsonPath `
        -ExpectedIndex $paragraphExpectation.paragraph_index `
        -ExpectedText $paragraphExpectation.text `
        -ExpectedStyleId $paragraphExpectation.style_id `
        -Label "baseline-$($paragraphExpectation.id)"

    $summaryBaselineParagraphs += [ordered]@{
        id = $paragraphExpectation.id
        paragraph_index = $paragraphExpectation.paragraph_index
        style_id = $paragraphExpectation.style_id
        text = $paragraphExpectation.text
        inspect_paragraph_json = $inspectParagraphJsonPath
    }
}

$baselineStyleStates = @{}
$summaryBaselineStyles = @()
foreach ($styleExpectation in $styleExpectations) {
    $inspectStyleJsonPath =
        Join-Path $resolvedOutputDir "baseline-$($styleExpectation.style_id)-style.json"
    Invoke-Capture `
        -Executable $cliExecutable `
        -Arguments @(
            "inspect-styles",
            $baselineDocxPath,
            "--style",
            $styleExpectation.style_id,
            "--json"
        ) `
        -OutputPath $inspectStyleJsonPath `
        -FailureMessage "Failed to inspect baseline style '$($styleExpectation.style_id)'."

    $styleState = Assert-StyleState `
        -JsonPath $inspectStyleJsonPath `
        -ExpectedStyleId $styleExpectation.style_id `
        -ExpectedName $styleExpectation.name `
        -ExpectedBasedOn $styleExpectation.based_on `
        -ExpectNumbering $styleExpectation.expect_numbering `
        -ExpectedLevel $styleExpectation.level `
        -ExpectedDefinitionName $styleExpectation.definition_name `
        -ExpectedOverrideCount $styleExpectation.override_count `
        -Label "baseline-$($styleExpectation.style_id)"

    $baselineStyleStates[$styleExpectation.style_id] = $styleState
    $summaryBaselineStyles += [ordered]@{
        style_id = $styleExpectation.style_id
        inspect_style_json = $inspectStyleJsonPath
        state = $styleState
    }
}

$baselineBodyNumberedXmlState =
    Get-StyleXmlState -DocxPath $baselineDocxPath -StyleId "BodyNumbered"
if (-not $baselineBodyNumberedXmlState.has_num_pr) {
    throw "Baseline BodyNumbered style was expected to keep style-level numPr markup."
}
if (-not $baselineBodyNumberedXmlState.has_bidi) {
    throw "Baseline BodyNumbered style was expected to keep w:bidi markup."
}
if ([int]$baselineBodyNumberedXmlState.num_id -ne
    [int]$baselineStyleStates["BodyNumbered"].num_id) {
    throw "Baseline BodyNumbered XML numId did not match inspect-styles metadata."
}
if ([int]$baselineBodyNumberedXmlState.level -ne 0) {
    throw "Baseline BodyNumbered XML level was expected to be 0."
}

Write-Step "Rendering Word evidence for the shared baseline document"
Invoke-WordSmoke `
    -ScriptPath $wordSmokeScript `
    -BuildDir $BuildDir `
    -DocxPath $baselineDocxPath `
    -OutputDir $baselineVisualDir `
    -Dpi $Dpi `
    -KeepWordOpen $KeepWordOpen.IsPresent `
    -VisibleWord $VisibleWord.IsPresent
Assert-VisualPageCount -VisualOutputDir $baselineVisualDir -ExpectedCount 1 -Label "shared-baseline-visual"

$headingOnlyLevels = @(
    [ordered]@{
        level = 0
        kind = "decimal"
        start = 5
        text_pattern = "(%1)"
    }
)

$nestedLevels = @(
    [ordered]@{
        level = 0
        kind = "decimal"
        start = 7
        text_pattern = "(%1)"
    },
    [ordered]@{
        level = 1
        kind = "decimal"
        start = 1
        text_pattern = "(%1.%2)"
    }
)

$caseDefinitions = @(
    [ordered]@{
        id = "set-style-numbering-legal-heading-visual"
        mutation_steps = @(
            [ordered]@{
                command = "set-paragraph-style-numbering"
                style_id = "LegalHeading"
                definition_name = "LegalHeadingStyleOutlineVisual"
                style_level = 0
                definition_levels = $headingOnlyLevels
            }
        )
        expected_styles = @(
            [ordered]@{
                style_id = "LegalHeading"
                name = "Legal Heading"
                based_on = "Heading1"
                expect_numbering = $true
                level = 0
                definition_name = "LegalHeadingStyleOutlineVisual"
                override_count = 0
                instance_group = "heading-outline"
            },
            [ordered]@{
                style_id = "LegalSubheading"
                name = "Legal Subheading"
                based_on = "Heading2"
                expect_numbering = $false
                level = $null
                definition_name = $null
                override_count = 0
            },
            [ordered]@{
                style_id = "BodyNumbered"
                name = "Body Numbered"
                based_on = "Normal"
                expect_numbering = $true
                level = 0
                definition_name = "BodyStyleOutlineBaseline"
                override_count = 0
                compare_to_baseline = $true
            }
        )
        expected_visual_cues = @(
            "The LegalHeading paragraph gains a visible decimal marker that starts at (5).",
            "The LegalSubheading paragraph stays unnumbered while the BodyNumbered clear target keeps its baseline [4] style marker.",
            "The page remains single-page with the trailing anchor still below the three styled paragraphs."
        )
    },
    [ordered]@{
        id = "set-style-numbering-nested-outline-visual"
        mutation_steps = @(
            [ordered]@{
                command = "set-paragraph-style-numbering"
                style_id = "LegalHeading"
                definition_name = "LegalHeadingNestedStyleOutlineVisual"
                style_level = 0
                definition_levels = $nestedLevels
            },
            [ordered]@{
                command = "set-paragraph-style-numbering"
                style_id = "LegalSubheading"
                definition_name = "LegalHeadingNestedStyleOutlineVisual"
                style_level = 1
                definition_levels = $nestedLevels
            }
        )
        expected_styles = @(
            [ordered]@{
                style_id = "LegalHeading"
                name = "Legal Heading"
                based_on = "Heading1"
                expect_numbering = $true
                level = 0
                definition_name = "LegalHeadingNestedStyleOutlineVisual"
                override_count = 0
                instance_group = "nested-outline"
            },
            [ordered]@{
                style_id = "LegalSubheading"
                name = "Legal Subheading"
                based_on = "Heading2"
                expect_numbering = $true
                level = 1
                definition_name = "LegalHeadingNestedStyleOutlineVisual"
                override_count = 0
                instance_group = "nested-outline"
            },
            [ordered]@{
                style_id = "BodyNumbered"
                name = "Body Numbered"
                based_on = "Normal"
                expect_numbering = $true
                level = 0
                definition_name = "BodyStyleOutlineBaseline"
                override_count = 0
                compare_to_baseline = $true
            }
        )
        expected_visual_cues = @(
            "The LegalHeading paragraph gains a root marker that starts at (7).",
            "The LegalSubheading paragraph renders as a nested child marker (7.1) under the same style-linked numbering instance.",
            "The BodyNumbered clear target keeps its baseline [4] marker so the nested-heading effect is isolated to the two heading styles."
        )
    },
    [ordered]@{
        id = "ensure-style-linked-numbering-nested-outline-visual"
        mutation_steps = @(
            [ordered]@{
                command = "ensure-style-linked-numbering"
                definition_name = "LegalHeadingNestedStyleOutlineBatchVisual"
                definition_levels = $nestedLevels
                style_links = @(
                    [ordered]@{ style_id = "LegalHeading"; level = 0 },
                    [ordered]@{ style_id = "LegalSubheading"; level = 1 }
                )
            }
        )
        expected_styles = @(
            [ordered]@{
                style_id = "LegalHeading"
                name = "Legal Heading"
                based_on = "Heading1"
                expect_numbering = $true
                level = 0
                definition_name = "LegalHeadingNestedStyleOutlineBatchVisual"
                override_count = 0
                instance_group = "nested-outline-batch"
            },
            [ordered]@{
                style_id = "LegalSubheading"
                name = "Legal Subheading"
                based_on = "Heading2"
                expect_numbering = $true
                level = 1
                definition_name = "LegalHeadingNestedStyleOutlineBatchVisual"
                override_count = 0
                instance_group = "nested-outline-batch"
            },
            [ordered]@{
                style_id = "BodyNumbered"
                name = "Body Numbered"
                based_on = "Normal"
                expect_numbering = $true
                level = 0
                definition_name = "BodyStyleOutlineBaseline"
                override_count = 0
                compare_to_baseline = $true
            }
        )
        expected_visual_cues = @(
            "The LegalHeading paragraph gains a root marker that starts at (7).",
            "The LegalSubheading paragraph renders as a nested child marker (7.1) under the same style-linked numbering instance even though the whole outline link is created in one CLI step.",
            "The BodyNumbered clear target keeps its baseline [4] marker so the batched style-linking effect is isolated to the two heading styles."
        )
    },
    [ordered]@{
        id = "clear-style-numbering-body-numbered-visual"
        mutation_steps = @(
            [ordered]@{
                command = "clear-paragraph-style-numbering"
                style_id = "BodyNumbered"
            }
        )
        expected_styles = @(
            [ordered]@{
                style_id = "LegalHeading"
                name = "Legal Heading"
                based_on = "Heading1"
                expect_numbering = $false
                level = $null
                definition_name = $null
                override_count = 0
            },
            [ordered]@{
                style_id = "LegalSubheading"
                name = "Legal Subheading"
                based_on = "Heading2"
                expect_numbering = $false
                level = $null
                definition_name = $null
                override_count = 0
            },
            [ordered]@{
                style_id = "BodyNumbered"
                name = "Body Numbered"
                based_on = "Normal"
                expect_numbering = $false
                level = $null
                definition_name = $null
                override_count = 0
            }
        )
        expected_visual_cues = @(
            "The BodyNumbered clear target drops its baseline [4] style marker and returns to plain body alignment.",
            "The paragraph still uses the BodyNumbered style, so the regression is about removing style numbering rather than clearing the paragraph style itself.",
            "The bidi markup on BodyNumbered stays in styles.xml even though the style-level numPr node is removed."
        )
        assert_body_numbered_xml_after_clear = $true
    }
)

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()

foreach ($case in $caseDefinitions) {
    $caseDir = Join-Path $resolvedOutputDir $case.id
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null

    $mutatedDocxPath = Join-Path $caseDir "$($case.id)-mutated.docx"
    $visualDir = Join-Path $caseDir "mutated-visual"
    $currentInputDocx = $baselineDocxPath
    $mutationArtifacts = @()

    Write-Step "Running case '$($case.id)'"

    for ($stepIndex = 0; $stepIndex -lt $case.mutation_steps.Count; $stepIndex++) {
        $step = $case.mutation_steps[$stepIndex]
        $isFinalStep = ($stepIndex -eq ($case.mutation_steps.Count - 1))
        $stepOutputDocx = if ($isFinalStep) {
            $mutatedDocxPath
        } else {
            Join-Path $caseDir ("mutation-step-{0:D2}.docx" -f ($stepIndex + 1))
        }
        $stepJsonPath = Join-Path $caseDir ("mutation-step-{0:D2}.json" -f ($stepIndex + 1))

        if ($step.command -eq "set-paragraph-style-numbering") {
            $arguments = @(
                "set-paragraph-style-numbering",
                $currentInputDocx,
                $step.style_id,
                "--definition-name",
                $step.definition_name
            )

            foreach ($definitionLevel in $step.definition_levels) {
                $arguments += @(
                    "--numbering-level",
                    "$($definitionLevel.level):$($definitionLevel.kind):$($definitionLevel.start):$($definitionLevel.text_pattern)"
                )
            }

            $arguments += @(
                "--style-level",
                "$($step.style_level)",
                "--output",
                $stepOutputDocx,
                "--json"
            )

            Invoke-Capture `
                -Executable $cliExecutable `
                -Arguments $arguments `
                -OutputPath $stepJsonPath `
                -FailureMessage "Failed to execute mutation step $($stepIndex + 1) for case '$($case.id)'."

            Assert-SetStyleNumberingResult `
                -JsonPath $stepJsonPath `
                -ExpectedStyleId $step.style_id `
                -ExpectedDefinitionName $step.definition_name `
                -ExpectedStyleLevel $step.style_level `
                -ExpectedLevels $step.definition_levels `
                -Label "$($case.id)-step-$($stepIndex + 1)"
        } elseif ($step.command -eq "ensure-style-linked-numbering") {
            $arguments = @(
                "ensure-style-linked-numbering",
                $currentInputDocx,
                "--definition-name",
                $step.definition_name
            )

            foreach ($definitionLevel in $step.definition_levels) {
                $arguments += @(
                    "--numbering-level",
                    "$($definitionLevel.level):$($definitionLevel.kind):$($definitionLevel.start):$($definitionLevel.text_pattern)"
                )
            }

            foreach ($styleLink in $step.style_links) {
                $arguments += @(
                    "--style-link",
                    "$($styleLink.style_id):$($styleLink.level)"
                )
            }

            $arguments += @(
                "--output",
                $stepOutputDocx,
                "--json"
            )

            Invoke-Capture `
                -Executable $cliExecutable `
                -Arguments $arguments `
                -OutputPath $stepJsonPath `
                -FailureMessage "Failed to execute mutation step $($stepIndex + 1) for case '$($case.id)'."

            Assert-EnsureStyleLinkedNumberingResult `
                -JsonPath $stepJsonPath `
                -ExpectedDefinitionName $step.definition_name `
                -ExpectedLevels $step.definition_levels `
                -ExpectedStyleLinks $step.style_links `
                -Label "$($case.id)-step-$($stepIndex + 1)"
        } elseif ($step.command -eq "clear-paragraph-style-numbering") {
            $arguments = @(
                "clear-paragraph-style-numbering",
                $currentInputDocx,
                $step.style_id,
                "--output",
                $stepOutputDocx,
                "--json"
            )

            Invoke-Capture `
                -Executable $cliExecutable `
                -Arguments $arguments `
                -OutputPath $stepJsonPath `
                -FailureMessage "Failed to execute mutation step $($stepIndex + 1) for case '$($case.id)'."

            Assert-ClearStyleNumberingResult `
                -JsonPath $stepJsonPath `
                -ExpectedStyleId $step.style_id `
                -Label "$($case.id)-step-$($stepIndex + 1)"
        } else {
            throw "Unsupported mutation command '$($step.command)' in case '$($case.id)'."
        }

        $mutationArtifacts += [ordered]@{
            step = $stepIndex + 1
            command = $step.command
            input_docx = $currentInputDocx
            output_docx = $stepOutputDocx
            mutation_json = $stepJsonPath
        }

        $currentInputDocx = $stepOutputDocx
    }

    $paragraphArtifacts = @()
    foreach ($paragraphExpectation in $paragraphExpectations) {
        $inspectParagraphJsonPath =
            Join-Path $caseDir "inspect-paragraph-$($paragraphExpectation.id).json"
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @(
                "inspect-paragraphs",
                $mutatedDocxPath,
                "--paragraph",
                "$($paragraphExpectation.paragraph_index)",
                "--json"
            ) `
            -OutputPath $inspectParagraphJsonPath `
            -FailureMessage "Failed to inspect paragraph '$($paragraphExpectation.id)' for case '$($case.id)'."

        Assert-ParagraphStyleState `
            -JsonPath $inspectParagraphJsonPath `
            -ExpectedIndex $paragraphExpectation.paragraph_index `
            -ExpectedText $paragraphExpectation.text `
            -ExpectedStyleId $paragraphExpectation.style_id `
            -Label "$($case.id)-$($paragraphExpectation.id)"

        $paragraphArtifacts += [ordered]@{
            id = $paragraphExpectation.id
            paragraph_index = $paragraphExpectation.paragraph_index
            style_id = $paragraphExpectation.style_id
            inspect_paragraph_json = $inspectParagraphJsonPath
        }
    }

    $styleArtifacts = @()
    $caseStyleStates = @{}
    $groupStates = @{}
    foreach ($styleExpectation in $case.expected_styles) {
        $inspectStyleJsonPath =
            Join-Path $caseDir "inspect-style-$($styleExpectation.style_id).json"
        Invoke-Capture `
            -Executable $cliExecutable `
            -Arguments @(
                "inspect-styles",
                $mutatedDocxPath,
                "--style",
                $styleExpectation.style_id,
                "--json"
            ) `
            -OutputPath $inspectStyleJsonPath `
            -FailureMessage "Failed to inspect style '$($styleExpectation.style_id)' for case '$($case.id)'."

        $styleState = Assert-StyleState `
            -JsonPath $inspectStyleJsonPath `
            -ExpectedStyleId $styleExpectation.style_id `
            -ExpectedName $styleExpectation.name `
            -ExpectedBasedOn $styleExpectation.based_on `
            -ExpectNumbering $styleExpectation.expect_numbering `
            -ExpectedLevel $styleExpectation.level `
            -ExpectedDefinitionName $styleExpectation.definition_name `
            -ExpectedOverrideCount $styleExpectation.override_count `
            -Label "$($case.id)-$($styleExpectation.style_id)"

        if ($styleExpectation.compare_to_baseline) {
            $baselineStyleState = $baselineStyleStates[$styleExpectation.style_id]
            if ($null -eq $baselineStyleState) {
                throw "Missing baseline style state for '$($styleExpectation.style_id)'."
            }
            if ($styleState.num_id -ne $baselineStyleState.num_id) {
                throw "$($case.id) unexpectedly changed num_id for style '$($styleExpectation.style_id)'."
            }
            if ($styleState.definition_id -ne $baselineStyleState.definition_id) {
                throw "$($case.id) unexpectedly changed definition_id for style '$($styleExpectation.style_id)'."
            }
            if ($styleState.level -ne $baselineStyleState.level) {
                throw "$($case.id) unexpectedly changed style level for '$($styleExpectation.style_id)'."
            }
            if ($styleState.definition_name -ne $baselineStyleState.definition_name) {
                throw "$($case.id) unexpectedly changed definition_name for '$($styleExpectation.style_id)'."
            }
        }

        if ($styleExpectation.instance_group) {
            if (-not $groupStates.ContainsKey($styleExpectation.instance_group)) {
                $groupStates[$styleExpectation.instance_group] = @()
            }
            $groupStates[$styleExpectation.instance_group] += $styleState
        }

        $caseStyleStates[$styleExpectation.style_id] = $styleState
        $styleArtifacts += [ordered]@{
            style_id = $styleExpectation.style_id
            inspect_style_json = $inspectStyleJsonPath
            state = $styleState
        }
    }

    foreach ($groupName in $groupStates.Keys) {
        $groupEntries = @($groupStates[$groupName])
        if ($groupEntries.Count -lt 2) {
            continue
        }

        $referenceEntry = $groupEntries[0]
        for ($index = 1; $index -lt $groupEntries.Count; $index++) {
            $candidateEntry = $groupEntries[$index]
            if ($candidateEntry.num_id -ne $referenceEntry.num_id) {
                throw "$($case.id) expected instance group '$groupName' to share one num_id."
            }
            if ($candidateEntry.definition_id -ne $referenceEntry.definition_id) {
                throw "$($case.id) expected instance group '$groupName' to share one definition_id."
            }
        }
    }

    $xmlAssertions = $null
    if ($case.assert_body_numbered_xml_after_clear) {
        $bodyNumberedXmlState =
            Get-StyleXmlState -DocxPath $mutatedDocxPath -StyleId "BodyNumbered"
        if ($bodyNumberedXmlState.has_num_pr) {
            throw "$($case.id) expected BodyNumbered w:numPr to be removed from styles.xml."
        }
        if (-not $bodyNumberedXmlState.has_bidi) {
            throw "$($case.id) expected BodyNumbered w:bidi to remain in styles.xml."
        }

        $xmlAssertions = [ordered]@{
            body_numbered_before = $baselineBodyNumberedXmlState
            body_numbered_after = $bodyNumberedXmlState
        }
    }

    Invoke-WordSmoke `
        -ScriptPath $wordSmokeScript `
        -BuildDir $BuildDir `
        -DocxPath $mutatedDocxPath `
        -OutputDir $visualDir `
        -Dpi $Dpi `
        -KeepWordOpen $KeepWordOpen.IsPresent `
        -VisibleWord $VisibleWord.IsPresent
    Assert-VisualPageCount -VisualOutputDir $visualDir -ExpectedCount 1 -Label $case.id

    $artifacts = Register-ComparisonArtifacts `
        -Python $renderPython `
        -ContactSheetScript $contactSheetScript `
        -AggregatePagesDir $aggregatePagesDir `
        -CaseOutputDir $caseDir `
        -CaseId $case.id `
        -Images @(
            (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label $case.id),
            (Get-RenderedPagePath -VisualOutputDir $visualDir -PageNumber 1 -Label $case.id)
        ) `
        -Labels @("$($case.id) baseline-page-01", "$($case.id) mutated-page-01")

    $aggregateImages += $artifacts.selected_pages
    $aggregateLabels += $artifacts.labels
    $summaryCases += [ordered]@{
        id = $case.id
        source_docx = $baselineDocxPath
        mutated_docx = $mutatedDocxPath
        mutation_steps = $mutationArtifacts
        paragraph_artifacts = $paragraphArtifacts
        style_artifacts = $styleArtifacts
        xml_assertions = $xmlAssertions
        expected_visual_cues = $case.expected_visual_cues
        visual_artifacts = $artifacts
    }
}

Build-ContactSheet `
    -Python $renderPython `
    -ScriptPath $contactSheetScript `
    -OutputPath $aggregateContactSheetPath `
    -Images $aggregateImages `
    -Labels $aggregateLabels

$summary = [ordered]@{
    generated_at = (Get-Date).ToString("s")
    workspace = $repoRoot
    output_dir = $resolvedOutputDir
    visual_enabled = $true
    shared_baseline_docx = $baselineDocxPath
    shared_baseline_paragraphs = $summaryBaselineParagraphs
    shared_baseline_styles = $summaryBaselineStyles
    shared_baseline_styles_xml = [ordered]@{
        body_numbered = $baselineBodyNumberedXmlState
    }
    shared_baseline_visual = [ordered]@{
        root = $baselineVisualDir
        page = (Get-RenderedPagePath -VisualOutputDir $baselineVisualDir -PageNumber 1 -Label "shared-baseline")
    }
    cases = $summaryCases
    aggregate_evidence = [ordered]@{
        root = $aggregateEvidenceDir
        selected_pages_dir = $aggregatePagesDir
        contact_sheet = $aggregateContactSheetPath
    }
}

$summaryPath = Join-Path $resolvedOutputDir "summary.json"
($summary | ConvertTo-Json -Depth 10) | Set-Content -Path $summaryPath -Encoding UTF8

Write-Step "Completed paragraph style numbering visual regression"
Write-Host "Summary: $summaryPath"
Write-Host "Aggregate contact sheet: $aggregateContactSheetPath"
