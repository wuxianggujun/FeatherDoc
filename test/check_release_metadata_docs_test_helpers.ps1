function Resolve-DefaultRepoRoot {
    if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
        return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
    }

    return (Resolve-Path $RepoRoot).Path
}

function Resolve-DefaultWorkingDir {
    if ([string]::IsNullOrWhiteSpace($WorkingDir)) {
        $defaultDir = Join-Path $resolvedRepoRoot "output\check-release-metadata-docs-test"
        return [System.IO.Path]::GetFullPath($defaultDir)
    }

    return [System.IO.Path]::GetFullPath($WorkingDir)
}

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $parentDir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $parentDir -Force | Out-Null

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Write-Utf8BomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $parentDir = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $parentDir -Force | Out-Null

    $encoding = New-Object System.Text.UTF8Encoding($true)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}


function Assert-FileHasNoBom {
    param([string]$Path)

    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -ge 3 -and
        $bytes[0] -eq 0xEF -and
        $bytes[1] -eq 0xBB -and
        $bytes[2] -eq 0xBF) {
        throw "File must be UTF-8 without BOM: $Path"
    }
}

function Assert-ArrayContains {
    param(
        [object[]]$Values,
        [string]$ExpectedValue,
        [string]$Message
    )

    foreach ($value in $Values) {
        if ($value -eq $ExpectedValue) {
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



function Assert-SummaryAuditFields {
    param([object]$Summary)

    if ($Summary.checker_name -ne "check_release_metadata_docs.ps1") {
        throw "Expected JSON checker name check_release_metadata_docs.ps1, got: $($Summary.checker_name)"
    }
    $checkedAtUtc = $Summary.checked_at_utc
    if ($checkedAtUtc -is [DateTime]) {
        $checkedAtUtc = $checkedAtUtc.ToUniversalTime().ToString(
            "yyyy-MM-ddTHH:mm:ss'Z'",
            [System.Globalization.CultureInfo]::InvariantCulture
        )
    }
    if ($checkedAtUtc -notmatch '^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$') {
        throw "Expected JSON checked_at_utc to use UTC timestamp format, got: $($Summary.checked_at_utc)"
    }
    if ([string]::IsNullOrWhiteSpace($Summary.powershell_edition)) {
        throw "Expected JSON powershell_edition to be populated."
    }
    if ($Summary.powershell_version -notmatch '^\d+\.\d+') {
        throw "Expected JSON powershell_version to start with a version number, got: $($Summary.powershell_version)"
    }
    if ($Summary.output_encoding -ne "UTF-8 without BOM") {
        throw "Expected JSON output_encoding UTF-8 without BOM, got: $($Summary.output_encoding)"
    }
}

function Assert-SummaryMarkerCountsConsistent {
    param([object]$Summary)

    $markerGroups = @(
        [pscustomobject]@{
            Label = "pipeline"
            CountName = "required_pipeline_marker_count"
            ValuesName = "required_pipeline_markers"
        },
        [pscustomobject]@{
            Label = "checklist"
            CountName = "required_checklist_marker_count"
            ValuesName = "required_checklist_markers"
        },
        [pscustomobject]@{
            Label = "document governance"
            CountName = "required_document_governance_marker_count"
            ValuesName = "required_document_governance_markers"
        },
        [pscustomobject]@{
            Label = "policy"
            CountName = "required_policy_marker_count"
            ValuesName = "required_policy_markers"
        },
        [pscustomobject]@{
            Label = "entrypoint"
            CountName = "required_entrypoint_marker_count"
            ValuesName = "required_entrypoint_markers"
        }
    )

    $categoryTotal = 0
    foreach ($group in $markerGroups) {
        $expectedCount = [int]$Summary.PSObject.Properties[$group.CountName].Value
        $actualCount = @($Summary.PSObject.Properties[$group.ValuesName].Value).Count
        if ($actualCount -ne $expectedCount) {
            throw "Expected $($group.Label) marker array count $expectedCount, got $actualCount."
        }

        $categoryTotal += $expectedCount
    }

    if ([int]$Summary.required_marker_count -ne $categoryTotal) {
        throw "Expected total marker count $categoryTotal, got $($Summary.required_marker_count)."
    }
}

function Assert-SummaryCheckedDocumentsConsistent {
    param([object]$Summary)

    $checkedDocuments = @($Summary.checked_documents)
    if ([int]$Summary.checked_document_count -ne $checkedDocuments.Count) {
        throw "Expected checked document count $($Summary.checked_document_count), got $($checkedDocuments.Count)."
    }

    foreach ($document in $checkedDocuments) {
        foreach ($propertyName in @("label", "relative_path", "path")) {
            $property = $document.PSObject.Properties[$propertyName]
            if ($null -eq $property) {
                throw "Expected checked document $propertyName property to exist."
            }

            $propertyValue = [string]$property.Value
            if ([string]::IsNullOrWhiteSpace($propertyValue)) {
                throw "Expected checked document $propertyName to be populated."
            }
        }

        $relativePath = [string]$document.relative_path
        if ([System.IO.Path]::IsPathRooted($relativePath)) {
            throw "Expected checked document relative_path to stay repository-relative, got: $relativePath"
        }
    }

    $labels = @($Summary.checked_document_labels)
    if ($labels.Count -ne $checkedDocuments.Count) {
        throw "Expected checked_document_labels count $($checkedDocuments.Count), got $($labels.Count)."
    }
    $relativePaths = @($Summary.checked_document_relative_paths)
    if ($relativePaths.Count -ne $checkedDocuments.Count) {
        throw "Expected checked_document_relative_paths count $($checkedDocuments.Count), got $($relativePaths.Count)."
    }
    foreach ($document in $checkedDocuments) {
        Assert-ArrayContains `
            -Values $labels `
            -ExpectedValue ([string]$document.label) `
            -Message "Expected checked_document_labels to include $($document.label)."
        Assert-ArrayContains `
            -Values $relativePaths `
            -ExpectedValue ([string]$document.relative_path) `
            -Message "Expected checked_document_relative_paths to include $($document.relative_path)."
    }
}

function Assert-SummaryFailure {
    param(
        [string]$Path,
        [string]$ExpectedMessage,
        [string]$ExpectedFailureKind,
        [string]$ExpectedFailureRuleId = "",
        [string]$ExpectedFailureRelativePath,
        [string]$ExpectedFailureExpectedText = "",
        [int]$ExpectedFailureLineNumber = 0,
        [int]$ExpectedFailureColumnNumber = 0,
        [string]$ExpectedFailureExcerpt = ""
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "check_release_metadata_docs.ps1 did not write failure JSON summary: $Path"
    }

    Assert-FileHasNoBom -Path $Path
    $summary = Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    if ($summary.status -ne "failed") {
        throw "Expected JSON summary status to be failed, got: $($summary.status)"
    }
    if ($summary.error_message -notmatch [regex]::Escape($ExpectedMessage)) {
        throw "Expected JSON failure message '$ExpectedMessage', got: $($summary.error_message)"
    }
    if ($summary.failure_kind -ne $ExpectedFailureKind) {
        throw "Expected JSON failure kind '$ExpectedFailureKind', got: $($summary.failure_kind)"
    }
    if ([string]::IsNullOrWhiteSpace($ExpectedFailureRuleId)) {
        $ExpectedFailureRuleId = "release_metadata_docs.$ExpectedFailureKind"
    }
    if ($summary.failure_rule_id -ne $ExpectedFailureRuleId) {
        throw "Expected JSON failure rule id '$ExpectedFailureRuleId', got: $($summary.failure_rule_id)"
    }
    if ($summary.failure_relative_path -ne $ExpectedFailureRelativePath) {
        throw "Expected JSON failure relative path '$ExpectedFailureRelativePath', got: $($summary.failure_relative_path)"
    }
    if (-not [string]::IsNullOrWhiteSpace($ExpectedFailureExpectedText) -and
        $summary.failure_expected_text -ne $ExpectedFailureExpectedText) {
        throw "Expected JSON failure expected text '$ExpectedFailureExpectedText', got: $($summary.failure_expected_text)"
    }
    if ($ExpectedFailureLineNumber -gt 0 -and
        $summary.failure_line_number -ne $ExpectedFailureLineNumber) {
        throw "Expected JSON failure line number '$ExpectedFailureLineNumber', got: $($summary.failure_line_number)"
    }
    if ($ExpectedFailureColumnNumber -gt 0 -and
        $summary.failure_column_number -ne $ExpectedFailureColumnNumber) {
        throw "Expected JSON failure column number '$ExpectedFailureColumnNumber', got: $($summary.failure_column_number)"
    }
    if ($ExpectedFailureExcerpt.Length -gt 0 -and
        $summary.failure_excerpt -ne $ExpectedFailureExcerpt) {
        throw "Expected JSON failure excerpt '$ExpectedFailureExcerpt', got: $($summary.failure_excerpt)"
    }

    $expectedSummaryJsonPath = [System.IO.Path]::GetFullPath($Path)
    if ($summary.summary_json_path -ne $expectedSummaryJsonPath) {
        throw "Expected JSON summary path '$expectedSummaryJsonPath', got: $($summary.summary_json_path)"
    }
    if ($summary.summary_json_relative_path -ne "docs-check-summary.json") {
        throw "Expected JSON relative summary path docs-check-summary.json, got: $($summary.summary_json_relative_path)"
    }
    if ($summary.summary_schema_version -ne 1) {
        throw "Expected JSON summary schema version 1, got: $($summary.summary_schema_version)"
    }
    Assert-SummaryAuditFields -Summary $summary
    Assert-SummaryMarkerCountsConsistent -Summary $summary
    Assert-SummaryCheckedDocumentsConsistent -Summary $summary
    if ($summary.required_marker_count -ne 326) {
        throw "Expected JSON summary to count 326 required markers, got: $($summary.required_marker_count)"
    }
}

function New-DocsCase {
    param(
        [string]$Name,
        [string]$PipelineText = $defaultPipelineText,
        [string]$ChecklistText = $defaultChecklistText,
        [string]$PdfReadinessChecklistText = $defaultPdfReadinessChecklistText,
        [string]$DocumentationMaintenanceText = $defaultDocumentationMaintenanceText,
        [string]$DocumentGovernanceText = $defaultDocumentGovernanceText,
        [string]$PolicyText = $defaultPolicyText,
        [string]$IndexText = $defaultIndexText,
        [string]$ReadmeText = $defaultReadmeText,
        [string]$ReadmeZhText = $defaultReadmeZhText
    )

    $caseRoot = Join-Path $resolvedWorkingDir ("{0}-{1}" -f $Name, [System.Guid]::NewGuid().ToString("N"))
    $docsDir = Join-Path $caseRoot "docs"

    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_metadata_pipeline_zh.rst") -Text $PipelineText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_metadata_maintenance_checklist_zh.rst") -Text $ChecklistText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "pdf_release_readiness_checklist_zh.rst") -Text $PdfReadinessChecklistText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "documentation_maintenance_zh.rst") -Text $DocumentationMaintenanceText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "document_governance_acceptance_zh.rst") -Text $DocumentGovernanceText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "release_policy_zh.rst") -Text $PolicyText
    Write-Utf8NoBomFile -Path (Join-Path $docsDir "index.rst") -Text $IndexText
    Write-Utf8NoBomFile -Path (Join-Path $caseRoot "README.md") -Text $ReadmeText
    Write-Utf8NoBomFile -Path (Join-Path $caseRoot "README.zh-CN.md") -Text $ReadmeZhText

    return $caseRoot
}

function Invoke-DocsCheck {
    param(
        [string]$CaseRoot,
        [switch]$ShouldFail,
        [string]$ExpectedMessage = "",
        [string]$SummaryJson = "",
        [switch]$Quiet
    )

    $failed = $false
    $output = @()

    $parameters = @{ RepoRoot = $CaseRoot }
    if (-not [string]::IsNullOrWhiteSpace($SummaryJson)) {
        $parameters.SummaryJson = $SummaryJson
    }
    if ($Quiet) {
        $parameters.Quiet = $true
    }

    try {
        $output = @(& $docsCheckScript @parameters *>&1)
    } catch {
        $failed = $true
        $output += $_.Exception.Message
    }

    $joinedOutput = ($output | ForEach-Object { $_.ToString() }) -join "`n"

    if ($ShouldFail) {
        if (-not $failed) {
            throw "check_release_metadata_docs.ps1 unexpectedly passed for $CaseRoot."
        }
        if (-not [string]::IsNullOrWhiteSpace($ExpectedMessage) -and
            $joinedOutput -notmatch [regex]::Escape($ExpectedMessage)) {
            throw "Expected failure message '$ExpectedMessage', got: $joinedOutput"
        }
        return
    }

    if ($failed) {
        throw "check_release_metadata_docs.ps1 failed unexpectedly for ${CaseRoot}: $joinedOutput"
    }

    if ($Quiet) {
        if ($joinedOutput -match [regex]::Escape("Release metadata docs check passed.")) {
            throw "check_release_metadata_docs.ps1 printed the success marker in quiet mode."
        }
    } elseif ($joinedOutput -notmatch [regex]::Escape("Release metadata docs check passed.")) {
        throw "check_release_metadata_docs.ps1 did not print the success marker. Output: $joinedOutput"
    }
}
