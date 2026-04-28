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

foreach ($scriptInfo in $metadataScripts) {
    Assert-ScriptParses -Path $scriptInfo.Path
    $scriptText = Get-Content -Raw -LiteralPath $scriptInfo.Path
    $label = Split-Path -Leaf $scriptInfo.Path

    Assert-ContainsText -Text $scriptText `
        -ExpectedText '$gateFlowVerdict = Get-OptionalPropertyValue -Object $gateFlow -Name "review_verdict"' `
        -Message "$label should read same-run review_verdict from the visual gate summary."
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
        -ExpectedText (('[void]${0}.Add("- Fixed-grid verdict: $(Get-DisplayValue -Value $fixedGridVerdict)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the fixed-grid visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Section page setup verdict: $(Get-DisplayValue -Value $sectionPageSetupVerdict)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the section page setup visual verdict."
    Assert-ContainsText -Text $scriptText `
        -ExpectedText (('[void]${0}.Add("- Page number fields verdict: $(Get-DisplayValue -Value $pageNumberFieldsVerdict)")' -f $scriptInfo.LinesVariable)) `
        -Message "$label should render the page number fields visual verdict."
}

$bodyScriptPath = Join-Path $resolvedRepoRoot "scripts\write_release_body_zh.ps1"
Assert-ScriptParses -Path $bodyScriptPath
$bodyScriptText = Get-Content -Raw -LiteralPath $bodyScriptPath
Assert-ContainsText -Text $bodyScriptText `
    -ExpectedText '$gateFlowVerdict = Get-OptionalPropertyValue -Object $gateFlow -Name "review_verdict"' `
    -Message "write_release_body_zh.ps1 should read same-run review_verdict from the visual gate summary."
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
    -Fragments @('Fixed-grid verdict', 'Get-DisplayValue -Value $fixedGridVerdict') `
    -Message "write_release_body_zh.ps1 should render the fixed-grid visual verdict in the full body."
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Section page setup verdict', 'Get-DisplayValue -Value $sectionPageSetupVerdict') `
    -Message "write_release_body_zh.ps1 should render the section page setup visual verdict in the full body."
Assert-LineContainsAll -Lines $bodyScriptLines `
    -Fragments @('Page number fields verdict', 'Get-DisplayValue -Value $pageNumberFieldsVerdict') `
    -Message "write_release_body_zh.ps1 should render the page number fields visual verdict in the full body."

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
