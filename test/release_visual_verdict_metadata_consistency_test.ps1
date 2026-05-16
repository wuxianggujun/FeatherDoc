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
    -ExpectedText '(Get-OptionalPropertyArray -Object $reviewTasks -Name "curated_visual_regressions")' `
    -Message "release_visual_metadata_helpers.ps1 should merge curated review task metadata."

$metadataScripts = @(
    [pscustomobject]@{
        Path = Join-Path $resolvedRepoRoot "scripts\write_release_artifact_handoff.ps1"
        LinesVariable = "handoffLines"
        SupportsGovernanceWarnings = $true
    },
    [pscustomobject]@{
        Path = Join-Path $resolvedRepoRoot "scripts\write_release_artifact_guide.ps1"
        LinesVariable = "lines"
        SupportsGovernanceWarnings = $true
    },
    [pscustomobject]@{
        Path = Join-Path $resolvedRepoRoot "scripts\write_release_metadata_start_here.ps1"
        LinesVariable = "lines"
        SupportsGovernanceWarnings = $true
    },
    [pscustomobject]@{
        Path = Join-Path $resolvedRepoRoot "scripts\write_release_reviewer_checklist.ps1"
        LinesVariable = "lines"
        SupportsGovernanceWarnings = $true
    }
)

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
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) reviewed at: $(Get-DisplayValue -Value $curatedVisualReview.reviewed_at)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual review timestamps."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- $($curatedVisualReview.label) review method: $(Get-DisplayValue -Value $curatedVisualReview.review_method)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render curated visual review methods."

    if ($scriptInfo.SupportsGovernanceWarnings) {
        Assert-ContainsText -Text $scriptText `
            -ExpectedText '$releaseBlockerRollupSummary = Get-OptionalPropertyObject -Object $summary -Name "release_blocker_rollup"' `
            -Message "$label should collect release blocker rollup warning metadata."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText '$releaseGovernanceHandoffSummary = Get-OptionalPropertyObject -Object $summary -Name "release_governance_handoff"' `
            -Message "$label should collect release governance handoff warning metadata."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText '$releaseGovernanceHandoffRollupSummary = Get-OptionalPropertyObject -Object $releaseGovernanceHandoffSummary -Name "release_blocker_rollup"' `
            -Message "$label should collect nested handoff rollup warning metadata."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText (('[void]${0}.Add("- Release blocker rollup warning_count: $(Get-ReleaseGovernanceWarningCount -SummaryObject $releaseBlockerRollupSummary)")' -f $scriptInfo.LinesVariable)) `
            -Message "$label should render the release blocker rollup warning count."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText (('[void]${0}.Add("- Release governance handoff warning_count: $(Get-ReleaseGovernanceWarningCount -SummaryObject $releaseGovernanceHandoffSummary)")' -f $scriptInfo.LinesVariable)) `
            -Message "$label should render the release governance handoff warning count."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText (('[void]${0}.Add("- Release governance handoff nested rollup warning_count: $(Get-ReleaseGovernanceWarningCount -SummaryObject $releaseGovernanceHandoffRollupSummary)")' -f $scriptInfo.LinesVariable)) `
            -Message "$label should render the nested handoff rollup warning count."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText (('[void]${0}.Add("- Release blocker rollup blocker_count: $(Get-ReleaseGovernanceBlockerCount -SummaryObject $releaseBlockerRollupSummary)")' -f $scriptInfo.LinesVariable)) `
            -Message "$label should render the release blocker rollup blocker count."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText (('[void]${0}.Add("- Release governance handoff blocker_count: $(Get-ReleaseGovernanceBlockerCount -SummaryObject $releaseGovernanceHandoffSummary)")' -f $scriptInfo.LinesVariable)) `
            -Message "$label should render the release governance handoff blocker count."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText (('[void]${0}.Add("- Release governance handoff nested rollup blocker_count: $(Get-ReleaseGovernanceBlockerCount -SummaryObject $releaseGovernanceHandoffRollupSummary)")' -f $scriptInfo.LinesVariable)) `
            -Message "$label should render the nested handoff rollup blocker count."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText (('Add-ReleaseGovernanceBlockersMarkdownSection -Lines ${0} -Summary $summary' -f $scriptInfo.LinesVariable)) `
            -Message "$label should render the governance blocker detail section."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText (('Add-ReleaseGovernanceWarningsMarkdownSection -Lines ${0} -Summary $summary' -f $scriptInfo.LinesVariable)) `
            -Message "$label should render the governance warning detail section."
    }

    if ($label -eq "write_release_reviewer_checklist.ps1") {
        Assert-ContainsText -Text $scriptText `
            -ExpectedText 'foreach ($warningItem in @(Get-ReleaseGovernanceWarningChecklistItems -Summary $summary))' `
            -Message "$label should turn release governance warnings into reviewer checklist items."
        Assert-ContainsText -Text $scriptText `
            -ExpectedText 'Get-ReleaseGovernanceWarningActionGuidanceLines' `
            -Message "$label should include release governance warning action guidance."
    }
}

$bodyScriptPath = Join-Path $resolvedRepoRoot "scripts\write_release_body_zh.ps1"
Assert-ScriptParses -Path $bodyScriptPath
$bodyScriptText = Get-Content -Raw -LiteralPath $bodyScriptPath
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

$releasePreflightPath = Join-Path $resolvedRepoRoot "scripts\run_release_candidate_checks.ps1"
Assert-ScriptParses -Path $releasePreflightPath
$releasePreflightText = Get-Content -Raw -LiteralPath $releasePreflightPath
foreach ($label in @("Smoke", "Fixed grid", "Section page setup", "Page number fields")) {
    Assert-ContainsText -Text $releasePreflightText `
        -ExpectedText ('[pscustomobject]@{{ Label = "{0}";' -f $label) `
        -Message "run_release_candidate_checks.ps1 final_review.md should render the $label visual verdict."
}
Assert-ContainsText -Text $releasePreflightText `
    -ExpectedText 'curated_visual_regressions' `
    -Message "run_release_candidate_checks.ps1 final_review.md should keep curated visual verdict metadata."

Write-Host "Release visual verdict metadata consistency regression passed."
