    if ($TemplateSchemaSectionTargets -and $TemplateSchemaResolvedSectionTargets) {
        $activeStep = "template_schema"
        throw "Template schema checking forbids using -TemplateSchemaSectionTargets and -TemplateSchemaResolvedSectionTargets together."
    }
    if ($templateSchemaRequested -and [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaBaseline)) {
        $activeStep = "template_schema"
        throw "Template schema checking requires -TemplateSchemaBaseline when template schema options are requested."
    }
    if ($templateSchemaRequested -and [string]::IsNullOrWhiteSpace($resolvedTemplateSchemaInputDocx)) {
        $activeStep = "template_schema"
        throw "Template schema checking requires -TemplateSchemaInputDocx when template schema options are requested."
    }
    if ($templateSchemaManifestRequested -and -not (Test-Path -LiteralPath $resolvedTemplateSchemaManifestPath)) {
        $activeStep = "template_schema_manifest"
        throw "Template schema manifest does not exist: $resolvedTemplateSchemaManifestPath"
    }
    if ($projectTemplateSmokeRequested -and -not (Test-Path -LiteralPath $resolvedProjectTemplateSmokeManifestPath)) {
        $activeStep = "project_template_smoke"
        throw "Project template smoke manifest does not exist: $resolvedProjectTemplateSmokeManifestPath"
    }
    foreach ($path in @($resolvedReleaseBlockerRollupInputJson)) {
        if (-not (Test-Path -LiteralPath $path)) {
            $activeStep = "release_blocker_rollup"
            throw "Release blocker rollup input JSON does not exist: $path"
        }
    }
    foreach ($path in @($resolvedReleaseBlockerRollupInputRoot)) {
        if (-not (Test-Path -LiteralPath $path)) {
            $activeStep = "release_blocker_rollup"
            throw "Release blocker rollup input root does not exist: $path"
        }
    }
    foreach ($path in @($resolvedReleaseGovernanceHandoffInputJson)) {
        if (-not (Test-Path -LiteralPath $path)) {
            $activeStep = "release_governance_handoff"
            throw "Release governance handoff input JSON does not exist: $path"
        }
    }
    if ($releaseGovernanceHandoffRequested -and -not (Test-Path -LiteralPath $resolvedReleaseGovernanceHandoffInputRoot)) {
        $activeStep = "release_governance_handoff"
        throw "Release governance handoff input root does not exist: $resolvedReleaseGovernanceHandoffInputRoot"
    }

    if ($RefreshReadmeAssets -and $SkipVisualGate) {
        $activeStep = "visual_gate"
        throw "README gallery refresh requires the visual gate. Re-run without -SkipVisualGate."
    }

    if (-not $SkipConfigure) {
        $activeStep = "configure"
        Write-Step "Configuring MSVC build directory"
        Invoke-MsvcCommand -MsvcBootstrap $msvcBootstrap -CommandText (
            "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"$Generator`" " +
            "-DCMAKE_BUILD_TYPE=$Config -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON -DBUILD_CLI=ON"
        )
        $summary.steps.configure.status = "completed"
    } else {
        if ($msvcBootstrapRequired) {
            Assert-PathExists -Path $resolvedBuildDir -Label "existing build directory"
        }
    }

    if (-not $SkipBuild) {
        $activeStep = "build"
        Write-Step "Building configured targets"
        Invoke-MsvcCommand -MsvcBootstrap $msvcBootstrap -CommandText (
            "cmake --build `"$resolvedBuildDir`""
        )
        $summary.steps.build.status = "completed"
    } else {
        if ($msvcBootstrapRequired) {
            Assert-PathExists -Path $resolvedBuildDir -Label "existing build directory"
        }
        if (-not $SkipInstallSmoke) {
            $summary.steps.build.install_prereq_cli = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_cli" `
                -Label "installable CLI executable"
        }
        if (-not $SkipVisualGate) {
            $summary.steps.build.visual_prereq_section_page_setup_cli = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_cli" `
                -Label "section page setup regression CLI"
            $summary.steps.build.visual_prereq_smoke = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_visual_smoke_tables" `
                -Label "visual smoke sample"
            $summary.steps.build.visual_prereq_merge_right = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_sample_merge_right_fixed_grid" `
                -Label "fixed-grid merge-right sample"
            $summary.steps.build.visual_prereq_merge_down = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_sample_merge_down_fixed_grid" `
                -Label "fixed-grid merge-down sample"
            $summary.steps.build.visual_prereq_unmerge_right = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_sample_unmerge_right_fixed_grid" `
                -Label "fixed-grid unmerge-right sample"
            $summary.steps.build.visual_prereq_unmerge_down = Assert-BuildExecutablePresent `
                -BuildRoot $resolvedBuildDir `
                -ExecutableStem "featherdoc_sample_unmerge_down_fixed_grid" `
                -Label "fixed-grid unmerge-down sample"
            if (-not $SkipSectionPageSetup) {
                $summary.steps.build.visual_prereq_section_page_setup = Assert-BuildExecutablePresent `
                    -BuildRoot $resolvedBuildDir `
                    -ExecutableStem "featherdoc_sample_section_page_setup" `
                    -Label "section page setup sample"
            }
            if (-not $SkipPageNumberFields) {
                $summary.steps.build.visual_prereq_page_number_fields = Assert-BuildExecutablePresent `
                    -BuildRoot $resolvedBuildDir `
                    -ExecutableStem "featherdoc_sample_page_number_fields" `
                    -Label "page number fields sample"
            }
        }
    }

    if (-not $SkipTests) {
        $activeStep = "tests"
        Write-Step "Running ctest"
        Invoke-MsvcCommand -MsvcBootstrap $msvcBootstrap -CommandText (
            "ctest --test-dir `"$resolvedBuildDir`" --output-on-failure --timeout $CtestTimeoutSeconds"
        )
        $summary.steps.tests.status = "completed"
    }

    if ($templateSchemaRequested) {
        $activeStep = "template_schema"
        Write-Step "Running template schema baseline check"
        Assert-PathExists -Path $resolvedTemplateSchemaInputDocx -Label "template schema input DOCX"
        Assert-PathExists -Path $resolvedTemplateSchemaBaseline -Label "template schema baseline"

        $templateSchemaArgs = @(
            "-InputDocx"
            $resolvedTemplateSchemaInputDocx
            "-SchemaFile"
            $resolvedTemplateSchemaBaseline
            "-GeneratedSchemaOutput"
            $resolvedTemplateSchemaGeneratedOutput
            "-SkipBuild"
            "-BuildDir"
            $resolvedBuildDir
        )
        if ($TemplateSchemaSectionTargets) {
            $templateSchemaArgs += "-SectionTargets"
        } elseif ($TemplateSchemaResolvedSectionTargets) {
            $templateSchemaArgs += "-ResolvedSectionTargets"
        }

        $templateSchemaOutput = @(
            & powershell.exe -ExecutionPolicy Bypass -File $templateSchemaCheckScript @templateSchemaArgs 2>&1
        )
        $templateSchemaExitCode = $LASTEXITCODE
        foreach ($line in $templateSchemaOutput) {
            Write-Host $line
        }
        if ($templateSchemaExitCode -notin @(0, 1)) {
            throw "Template schema baseline check failed."
        }

        $templateSchemaLines = @($templateSchemaOutput | ForEach-Object { $_.ToString() })
        $templateSchemaLintInfo = Parse-TemplateSchemaLintOutput -Lines $templateSchemaLines
        $templateSchemaInfo = Parse-TemplateSchemaCheckOutput -Lines $templateSchemaLines
        $summary.steps.template_schema.status = if ($templateSchemaExitCode -eq 0) {
            "completed"
        } else {
            "failed"
        }
        $summary.steps.template_schema.schema_lint_clean = [bool]$templateSchemaLintInfo.clean
        $summary.steps.template_schema.schema_lint_issue_count = [int]$templateSchemaLintInfo.issue_count
        $summary.steps.template_schema.matches = [bool]$templateSchemaInfo.matches
        $summary.steps.template_schema.schema_file = [string]$templateSchemaInfo.schema_file
        if (-not [string]::IsNullOrWhiteSpace([string]$templateSchemaInfo.generated_output_path)) {
            $summary.steps.template_schema.generated_output_path = [string]$templateSchemaInfo.generated_output_path
        }
        $summary.steps.template_schema.added_target_count = [int]$templateSchemaInfo.added_target_count
        $summary.steps.template_schema.removed_target_count = [int]$templateSchemaInfo.removed_target_count
        $summary.steps.template_schema.changed_target_count = [int]$templateSchemaInfo.changed_target_count
        $summary.template_schema.schema_lint_clean = [bool]$templateSchemaLintInfo.clean
        $summary.template_schema.schema_lint_issue_count = [int]$templateSchemaLintInfo.issue_count
        $summary.template_schema.matches = [bool]$templateSchemaInfo.matches
        $summary.template_schema.added_target_count = [int]$templateSchemaInfo.added_target_count
        $summary.template_schema.removed_target_count = [int]$templateSchemaInfo.removed_target_count
        $summary.template_schema.changed_target_count = [int]$templateSchemaInfo.changed_target_count

        if ($templateSchemaExitCode -ne 0) {
            if (-not [bool]$templateSchemaInfo.matches -and -not [bool]$templateSchemaLintInfo.clean) {
                throw "Template schema baseline drift and lint failures detected."
            }
            if (-not [bool]$templateSchemaInfo.matches) {
                throw "Template schema baseline drift detected."
            }
            if (-not [bool]$templateSchemaLintInfo.clean) {
                throw "Template schema baseline lint failed."
            }
            throw "Template schema baseline check failed."
        }
    }
