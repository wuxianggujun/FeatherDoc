param(
    [string]$RepoRoot,
    [string]$WorkingDir
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw $Message
    }
}

function Assert-NotContainsText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Message
    )

    if ($Text -match [regex]::Escape($UnexpectedText)) {
        throw $Message
    }
}

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message Expected='$Expected' Actual='$Actual'."
    }
}

function Assert-SequenceEqual {
    param([object[]]$Actual, [object[]]$Expected, [string]$Message)

    Assert-Equal -Actual $Actual.Count -Expected $Expected.Count -Message "$Message Count mismatch."
    for ($index = 0; $index -lt $Expected.Count; ++$index) {
        Assert-Equal -Actual ([string]$Actual[$index]) -Expected ([string]$Expected[$index]) `
            -Message "$Message Index $index mismatch."
    }
}

function Assert-LineContainsAll {
    param(
        [string[]]$Lines,
        [string[]]$Fragments,
        [string]$Message
    )

    foreach ($line in $Lines) {
        $lineMatches = $true
        foreach ($fragment in $Fragments) {
            if ($line -notmatch [regex]::Escape($fragment)) {
                $lineMatches = $false
                break
            }
        }
        if ($lineMatches) {
            return
        }
    }

    throw $Message
}

function Assert-ScriptParses {
    param([string]$Path)

    $parseTokens = $null
    $parseErrors = $null
    [System.Management.Automation.Language.Parser]::ParseFile($Path, [ref]$parseTokens, [ref]$parseErrors) | Out-Null
    if ($parseErrors.Count -gt 0) {
        throw "PowerShell script has parse errors: $Path"
    }
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path
$resolvedWorkingDir = [System.IO.Path]::GetFullPath($WorkingDir)
New-Item -ItemType Directory -Path $resolvedWorkingDir -Force | Out-Null

$visualMetadataHelperPath = Join-Path (Join-Path $resolvedRepoRoot "scripts") "release_visual_metadata_helpers.ps1"
Assert-ScriptParses -Path $visualMetadataHelperPath
$visualMetadataHelperText = Get-Content -Raw -LiteralPath $visualMetadataHelperPath
. $visualMetadataHelperPath
. (Join-Path $PSScriptRoot "pdf_import_diagnostics_contract_field_helpers.ps1")

$expectedImportDiagnosticsContractFields = @(Get-PdfImportDiagnosticsContractFields)
$pdfBoundedCtestEvidence = Get-PdfBoundedCtestEvidence -Summary ([pscustomobject][ordered]@{
        pdf_bounded_ctest = [pscustomobject][ordered]@{
            status = "pass"
            summary_count = 1
            pass_count = 1
            selected_test_count = 10
            skipped_test_count = 0
            import_diagnostics_contract_tests = @("pdf_cli_import", "pdf_import_failure", "pdf_import_table_heuristic")
            import_diagnostics_contract_fields = @($expectedImportDiagnosticsContractFields)
            import_negative_boundary_contract_cases = @("short_label_prose_remains_paragraphs", "invoice_summary_form_remains_paragraphs")
        }
    })
Assert-SequenceEqual `
    -Actual @($pdfBoundedCtestEvidence.import_diagnostics_contract_fields | ForEach-Object { [string]$_ }) `
    -Expected $expectedImportDiagnosticsContractFields `
    -Message "Get-PdfBoundedCtestEvidence should preserve import diagnostics contract fields without filtering or reordering."
$pdfBoundedCtestImportDiagnosticsDisplay = Get-PdfBoundedCtestImportDiagnosticsDisplay -Evidence $pdfBoundedCtestEvidence
Assert-Equal `
    -Actual ([string]$pdfBoundedCtestImportDiagnosticsDisplay.fields) `
    -Expected ($expectedImportDiagnosticsContractFields -join ", ") `
    -Message "Get-PdfBoundedCtestImportDiagnosticsDisplay should preserve import diagnostics contract field display order."

Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$summaryVerdict = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_verdict" -f $TaskKey)' `
    -Message "release_visual_metadata_helpers.ps1 should read same-run task verdict fields from the release summary before falling back to the visual gate summary."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$gateFlowVerdict = Get-OptionalPropertyValue -Object $gateFlow -Name "review_verdict"' `
    -Message "release_visual_metadata_helpers.ps1 should read same-run review_verdict from the visual gate summary."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$summaryStatus = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_review_status" -f $TaskKey)' `
    -Message "release_visual_metadata_helpers.ps1 should read same-run review_status from the release summary."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$summaryNote = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_review_note" -f $TaskKey)' `
    -Message "release_visual_metadata_helpers.ps1 should read same-run review_note from the release summary."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$summaryReviewedAt = Get-OptionalPropertyRawValue -Object $VisualGateSummary -Name ("{0}_reviewed_at" -f $TaskKey)' `
    -Message "release_visual_metadata_helpers.ps1 should read same-run reviewed_at from the release summary without culture-specific string coercion."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText 'return $Value.ToString("yyyy-MM-ddTHH:mm:ss")' `
    -Message "release_visual_metadata_helpers.ps1 should keep reviewed_at output culture-invariant."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$summaryReviewMethod = Get-OptionalPropertyValue -Object $VisualGateSummary -Name ("{0}_review_method" -f $TaskKey)' `
    -Message "release_visual_metadata_helpers.ps1 should read same-run review_method from the release summary."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText 'function Get-VisualTaskReviewTaskKey' `
    -Message "release_visual_metadata_helpers.ps1 should map smoke metadata to the document review task."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText 'function Get-VisualTaskReviewResultPath' `
    -Message "release_visual_metadata_helpers.ps1 should expose standard visual task review result paths."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText 'function Get-VisualTaskFinalReviewPath' `
    -Message "release_visual_metadata_helpers.ps1 should expose standard visual task final review paths."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$summaryPath = Get-OptionalPropertyValue -Object $VisualGateSummary -Name $pathProperty' `
    -Message "release_visual_metadata_helpers.ps1 should read release-summary path overrides."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$reviewTaskPath = Get-OptionalPropertyValue -Object $reviewTask -Name $PathPropertyName' `
    -Message "release_visual_metadata_helpers.ps1 should read gate review task evidence paths."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$flowTaskPath = Get-OptionalPropertyValue -Object $flowTask -Name $PathPropertyName' `
    -Message "release_visual_metadata_helpers.ps1 should read gate flow task evidence paths."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$reviewStatus = Get-OptionalPropertyValue -Object $Source -Name "review_status"' `
    -Message "release_visual_metadata_helpers.ps1 should read curated review_status values."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$reviewNote = Get-OptionalPropertyValue -Object $Source -Name "review_note"' `
    -Message "release_visual_metadata_helpers.ps1 should read curated review_note values."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$reviewedAt = Get-OptionalPropertyRawValue -Object $Source -Name "reviewed_at"' `
    -Message "release_visual_metadata_helpers.ps1 should read curated reviewed_at values without culture-specific string coercion."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$reviewMethod = Get-OptionalPropertyValue -Object $Source -Name "review_method"' `
    -Message "release_visual_metadata_helpers.ps1 should read curated review_method values."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$verdict = Get-OptionalPropertyValue -Object $Source -Name "review_verdict"' `
    -Message "release_visual_metadata_helpers.ps1 should read curated review_verdict values directly."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$reviewResultPath = Get-OptionalPropertyValue -Object $Source -Name "review_result_path"' `
    -Message "release_visual_metadata_helpers.ps1 should read curated review_result_path values directly."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$reviewResultPath = Join-Path $taskDir "report\review_result.json"' `
    -Message "release_visual_metadata_helpers.ps1 should derive curated review_result_path from task_dir when needed."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$finalReviewPath = Get-OptionalPropertyValue -Object $Source -Name "final_review_path"' `
    -Message "release_visual_metadata_helpers.ps1 should read curated final_review_path values directly."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '$finalReviewPath = Join-Path $taskDir "report\final_review.md"' `
    -Message "release_visual_metadata_helpers.ps1 should derive curated final_review_path from task_dir when needed."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText '(Get-OptionalPropertyArray -Object $reviewTasks -Name "curated_visual_regressions")' `
    -Message "release_visual_metadata_helpers.ps1 should merge curated review task metadata."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText 'function Get-SupersededReviewTasksReportPath' `
    -Message "release_visual_metadata_helpers.ps1 should own the superseded review task report path helper."
Assert-ContainsText -Text $visualMetadataHelperText `
    -ExpectedText 'function Get-SupersededReviewTaskCount' `
    -Message "release_visual_metadata_helpers.ps1 should own the superseded review task count helper."

$metadataScripts = @(
    [pscustomobject]@{
        Path = Join-Path $resolvedRepoRoot "scripts\write_release_artifact_handoff.ps1"
        LinesVariable = "handoffLines"
    },
    [pscustomobject]@{
        Path = Join-Path $resolvedRepoRoot "scripts\write_release_artifact_guide.ps1"
        LinesVariable = "lines"
    },
    [pscustomobject]@{
        Path = Join-Path $resolvedRepoRoot "scripts\write_release_metadata_start_here.ps1"
        LinesVariable = "lines"
    },
    [pscustomobject]@{
        Path = Join-Path $resolvedRepoRoot "scripts\write_release_reviewer_checklist.ps1"
        LinesVariable = "lines"
    }
)

$supersededReviewTaskScripts = @(
    (Join-Path $resolvedRepoRoot "scripts\write_release_artifact_handoff.ps1"),
    (Join-Path $resolvedRepoRoot "scripts\write_release_reviewer_checklist.ps1")
)

foreach ($scriptPath in $supersededReviewTaskScripts) {
    Assert-ScriptParses -Path $scriptPath
    $scriptText = Get-Content -Raw -LiteralPath $scriptPath
    $label = Split-Path -Leaf $scriptPath

    Assert-NotContainsText -Text $scriptText `
        -UnexpectedText 'function Get-SupersededReviewTasksReportPath' `
        -Message "$label should use the shared superseded review task report path helper instead of redefining it."
    Assert-NotContainsText -Text $scriptText `
        -UnexpectedText 'function Get-SupersededReviewTaskCount' `
        -Message "$label should use the shared superseded review task count helper instead of redefining it."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$supersededReviewTasksReportPath = Get-SupersededReviewTasksReportPath -Summary $summary -VisualGateSummary $visualGateStep' `
        -Message "$label should still resolve the superseded review task report path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$supersededReviewTasksCount = Get-SupersededReviewTaskCount -ReportPath $supersededReviewTasksReportPath' `
        -Message "$label should still render the superseded review task count."
}

foreach ($scriptInfo in $metadataScripts) {
    Assert-ScriptParses -Path $scriptInfo.Path
    $scriptText = Get-Content -Raw -LiteralPath $scriptInfo.Path
    $label = Split-Path -Leaf $scriptInfo.Path

    Assert-ContainsText -Text $scriptText `
        -ExpectedText '. (Join-Path $PSScriptRoot "release_visual_metadata_helpers.ps1")' `
        -Message "$label should use the shared release visual metadata helper."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$smokeVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"' `
        -Message "$label should collect the smoke visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$fixedGridVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "fixed_grid"' `
        -Message "$label should collect the fixed-grid visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$sectionPageSetupVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "section_page_setup"' `
        -Message "$label should continue collecting the section page setup visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$pageNumberFieldsVerdict = Get-VisualTaskVerdict -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "page_number_fields"' `
        -Message "$label should continue collecting the page number fields visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Smoke verdict: $(Get-DisplayValue -Value $smokeVerdict)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the smoke visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$smokeReviewedAt = Get-VisualTaskReviewedAt -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"' `
        -Message "$label should collect the smoke visual reviewed_at."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$smokeReviewMethod = Get-VisualTaskReviewMethod -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"' `
        -Message "$label should collect the smoke visual review method."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$smokeReviewResultPath = Get-VisualTaskReviewResultPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"' `
        -Message "$label should collect the smoke visual review result path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$smokeFinalReviewPath = Get-VisualTaskFinalReviewPath -VisualGateSummary $visualGateStep -GateSummary $gateSummary -TaskKey "smoke"' `
        -Message "$label should collect the smoke visual final review path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Smoke review status: $(Get-DisplayValue -Value $smokeReviewStatus)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the smoke visual review status."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Smoke reviewed at: $(Get-DisplayValue -Value $smokeReviewedAt)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the smoke visual review timestamp."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Smoke review method: $(Get-DisplayValue -Value $smokeReviewMethod)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the smoke visual review method."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Smoke review note: $(Get-DisplayValue -Value $smokeReviewNote)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the smoke visual review note."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Smoke review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $smokeReviewResultPath)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the smoke visual review result path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Smoke final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $smokeFinalReviewPath)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the smoke visual final review path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Fixed-grid verdict: $(Get-DisplayValue -Value $fixedGridVerdict)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the fixed-grid visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Fixed-grid review status: $(Get-DisplayValue -Value $fixedGridReviewStatus)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the fixed-grid visual review status."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Fixed-grid reviewed at: $(Get-DisplayValue -Value $fixedGridReviewedAt)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the fixed-grid visual review timestamp."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Fixed-grid review method: $(Get-DisplayValue -Value $fixedGridReviewMethod)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the fixed-grid visual review method."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Fixed-grid review note: $(Get-DisplayValue -Value $fixedGridReviewNote)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the fixed-grid visual review note."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Fixed-grid review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $fixedGridReviewResultPath)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the fixed-grid visual review result path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Fixed-grid final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $fixedGridFinalReviewPath)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the fixed-grid visual final review path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Section page setup verdict: $(Get-DisplayValue -Value $sectionPageSetupVerdict)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the section page setup visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Section page setup review status: $(Get-DisplayValue -Value $sectionPageSetupReviewStatus)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the section page setup visual review status."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Section page setup reviewed at: $(Get-DisplayValue -Value $sectionPageSetupReviewedAt)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the section page setup visual review timestamp."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Section page setup review method: $(Get-DisplayValue -Value $sectionPageSetupReviewMethod)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the section page setup visual review method."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Section page setup review note: $(Get-DisplayValue -Value $sectionPageSetupReviewNote)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the section page setup visual review note."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Section page setup review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupReviewResultPath)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the section page setup visual review result path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Section page setup final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $sectionPageSetupFinalReviewPath)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the section page setup visual final review path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Page number fields verdict: $(Get-DisplayValue -Value $pageNumberFieldsVerdict)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the page number fields visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Page number fields review status: $(Get-DisplayValue -Value $pageNumberFieldsReviewStatus)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the page number fields visual review status."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Page number fields reviewed at: $(Get-DisplayValue -Value $pageNumberFieldsReviewedAt)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the page number fields visual review timestamp."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Page number fields review method: $(Get-DisplayValue -Value $pageNumberFieldsReviewMethod)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the page number fields visual review method."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Page number fields review note: $(Get-DisplayValue -Value $pageNumberFieldsReviewNote)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the page number fields visual review note."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Page number fields review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsReviewResultPath)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the page number fields visual review result path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Page number fields final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $pageNumberFieldsFinalReviewPath)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the page number fields visual final review path."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) verdict: $(Get-DisplayValue -Value $curatedVisualReview.verdict)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual review verdicts."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) review status: $(Get-DisplayValue -Value $curatedVisualReview.review_status)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual review statuses."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) reviewed at: $(Get-DisplayValue -Value $curatedVisualReview.reviewed_at)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual review timestamps."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) review method: $(Get-DisplayValue -Value $curatedVisualReview.review_method)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual review methods."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) review note: $(Get-DisplayValue -Value $curatedVisualReview.review_note)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual review notes."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) review result: $(Get-DisplayPath -RepoRoot $repoRoot -Path $curatedVisualReview.review_result_path)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual review result paths."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) final review: $(Get-DisplayPath -RepoRoot $repoRoot -Path $curatedVisualReview.final_review_path)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual final review paths."
}

$bodyScriptPath = Join-Path $resolvedRepoRoot "scripts\write_release_body_zh.ps1"
Assert-ScriptParses -Path $bodyScriptPath
$bodyScriptText = @(
    Get-Content -Raw -LiteralPath $bodyScriptPath
    Get-ChildItem -LiteralPath (Join-Path $resolvedRepoRoot "scripts") -Filter "write_release_body_zh_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_.FullName }
) -join "`n"
Assert-ContainsText -Text $bodyScriptText `
    -ExpectedText '. (Join-Path $PSScriptRoot "release_visual_metadata_helpers.ps1")' `
    -Message "write_release_body_zh.ps1 should use the shared release visual metadata helper."
Assert-ContainsText -Text $bodyScriptText `
    -ExpectedText '$smokeVerdict = Get-VisualTaskVerdict -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "smoke"' `
    -Message "write_release_body_zh.ps1 should collect the smoke visual verdict."
Assert-ContainsText -Text $bodyScriptText `
    -ExpectedText '$fixedGridVerdict = Get-VisualTaskVerdict -VisualGateSummary $summary.steps.visual_gate -GateSummary $gateSummary -TaskKey "fixed_grid"' `
    -Message "write_release_body_zh.ps1 should collect the fixed-grid visual verdict."
Assert-ContainsText -Text $bodyScriptText `
    -ExpectedText '-SmokeVerdict $smokeVerdict' `
    -Message "write_release_body_zh.ps1 short summary should receive the smoke visual verdict."
Assert-ContainsText -Text $bodyScriptText `
    -ExpectedText '-FixedGridVerdict $fixedGridVerdict' `
    -Message "write_release_body_zh.ps1 short summary should receive the fixed-grid visual verdict."
Assert-ContainsText -Text $bodyScriptText `
    -ExpectedText 'smoke=`{0}`' `
    -Message "write_release_body_zh.ps1 short summary should format the smoke visual verdict."
Assert-ContainsText -Text $bodyScriptText `
    -ExpectedText 'fixed-grid=`{0}`' `
    -Message "write_release_body_zh.ps1 short summary should format the fixed-grid visual verdict."
$bodyScriptLines = $bodyScriptText -split "`r?`n"
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Smoke verdict', 'Get-DisplayValue -Value $smokeVerdict') `
    -Message "write_release_body_zh.ps1 should render the smoke visual verdict in the full body."
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Smoke review status', 'Get-DisplayValue -Value $smokeReviewStatus') `
    -Message "write_release_body_zh.ps1 should render the smoke visual review status in the full body."
if ($bodyScriptText -match 'review note') {
    throw "write_release_body_zh.ps1 should not render free-form visual review notes in public release notes."
}
if ($bodyScriptText -match 'reviewed at|review method') {
    throw "write_release_body_zh.ps1 should not render visual review provenance in public release notes."
}
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Fixed-grid verdict', 'Get-DisplayValue -Value $fixedGridVerdict') `
    -Message "write_release_body_zh.ps1 should render the fixed-grid visual verdict in the full body."
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Fixed-grid review status', 'Get-DisplayValue -Value $fixedGridReviewStatus') `
    -Message "write_release_body_zh.ps1 should render the fixed-grid visual review status in the full body."
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Section page setup verdict', 'Get-DisplayValue -Value $sectionPageSetupVerdict') `
    -Message "write_release_body_zh.ps1 should render the section page setup visual verdict in the full body."
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Section page setup review status', 'Get-DisplayValue -Value $sectionPageSetupReviewStatus') `
    -Message "write_release_body_zh.ps1 should render the section page setup visual review status in the full body."
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Page number fields verdict', 'Get-DisplayValue -Value $pageNumberFieldsVerdict') `
    -Message "write_release_body_zh.ps1 should render the page number fields visual verdict in the full body."
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Page number fields review status', 'Get-DisplayValue -Value $pageNumberFieldsReviewStatus') `
    -Message "write_release_body_zh.ps1 should render the page number fields visual review status in the full body."

$releasePreflightRoot = Join-Path $resolvedRepoRoot "scripts"
$releasePreflightPath = Join-Path $releasePreflightRoot "run_release_candidate_checks.ps1"
$releasePreflightScripts = @($releasePreflightPath) + @(
    Get-ChildItem -LiteralPath $releasePreflightRoot -Filter "run_release_candidate_checks_*.ps1" |
        Sort-Object FullName |
        ForEach-Object { $_.FullName }
)
foreach ($releasePreflightScript in $releasePreflightScripts) {
    Assert-ScriptParses -Path $releasePreflightScript
}
$releasePreflightText = @(
    $releasePreflightScripts | ForEach-Object { Get-Content -Raw -Encoding UTF8 -LiteralPath $_ }
) -join "`n"
foreach ($label in @("Smoke", "Fixed grid", "Section page setup", "Page number fields")) {
    Assert-ContainsText -Text $releasePreflightText `
        -ExpectedText ('[pscustomobject]@{{ Label = "{0}";' -f $label) `
        -Message "run_release_candidate_checks.ps1 final_review.md should render the $label visual verdict."
}
Assert-ContainsText -Text $releasePreflightText `
    -ExpectedText 'curated_visual_regressions' `
    -Message "run_release_candidate_checks.ps1 final_review.md should keep curated visual verdict metadata."

Write-Host "Release visual verdict metadata consistency regression passed."
