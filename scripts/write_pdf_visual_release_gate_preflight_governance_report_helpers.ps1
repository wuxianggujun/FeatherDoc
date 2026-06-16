function Write-Step {
    param([string]$Message)
    Write-Host "[pdf-visual-preflight-governance] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    try {
        $root = [System.IO.Path]::GetFullPath($RepoRoot)
        if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
            -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
            $root += [System.IO.Path]::DirectorySeparatorChar
        }

        $resolved = [System.IO.Path]::GetFullPath($Path)
        if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
            if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
            return ".\" + ($relative -replace '/', '\')
        }
        return $resolved
    } catch {
        return $Path
    }
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    if ($value -is [bool]) { return [bool]$value }
    return [string]$value -in @("true", "True", "1", "yes", "Yes")
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    try {
        return [int]$value
    } catch {
        return $DefaultValue
    }
}

function Get-JsonValueText {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) {
        if ([string]::IsNullOrWhiteSpace($value)) { return @() }
        return @($value)
    }
    if ($value -is [System.Collections.IDictionary]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Set-JsonPropertyValue {
    param($Object, [string]$Name, $Value)

    if ($null -eq $Object) { return }
    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        $Object | Add-Member -NotePropertyName $Name -NotePropertyValue $Value -Force
        return
    }
    $property.Value = $Value
}

function Test-SyntheticEvidencePath {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $false
    }

    $normalized = ([string]$Value) -replace '/', '\'
    return $normalized -match '(^|\\)fake-[^\\]*($|\\)'
}

function Add-SyntheticEvidenceMarker {
    param(
        [System.Collections.ArrayList]$Markers,
        [string]$Label,
        [string]$Value
    )

    if (Test-SyntheticEvidencePath -Value $Value) {
        $Markers.Add(("{0}={1}" -f $Label, $Value)) | Out-Null
    }
}

function Add-JsonObjectStringEvidenceMarkers {
    param(
        [System.Collections.ArrayList]$Markers,
        [string]$Label,
        [object]$Object
    )

    if ($null -eq $Object) {
        return
    }

    if ($Object -is [System.Collections.IDictionary]) {
        foreach ($key in $Object.Keys) {
            Add-SyntheticEvidenceMarker `
                -Markers $Markers `
                -Label ("{0}.{1}" -f $Label, $key) `
                -Value ([string]$Object[$key])
        }
        return
    }

    foreach ($property in @($Object.PSObject.Properties)) {
        Add-SyntheticEvidenceMarker `
            -Markers $Markers `
            -Label ("{0}.{1}" -f $Label, $property.Name) `
            -Value ([string]$property.Value)
    }
}

function Get-PreflightEvidenceKindSummary {
    param($PreflightSummary, $Checks)

    $declaredKind = Get-JsonString -Object $PreflightSummary -Name "evidence_kind"
    $declaredDetails = Get-JsonProperty -Object $PreflightSummary -Name "evidence_kind_details"
    $declaredMarkers = @(
        Get-JsonArray -Object $declaredDetails -Name "synthetic_markers" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    $markers = New-Object System.Collections.ArrayList
    foreach ($marker in $declaredMarkers) {
        $markers.Add($marker) | Out-Null
    }

    Add-SyntheticEvidenceMarker `
        -Markers $markers `
        -Label "build_dir" `
        -Value (Get-JsonString -Object $PreflightSummary -Name "build_dir")
    Add-SyntheticEvidenceMarker `
        -Markers $markers `
        -Label "requested_build_dir" `
        -Value (Get-JsonString -Object $PreflightSummary -Name "requested_build_dir")

    $ctestCheck = Get-PreflightCheck -Checks $Checks -Name "ctest_list_contains_pdf_gate_tests"
    $ctestDetails = Get-JsonProperty -Object $ctestCheck -Name "details"
    Add-SyntheticEvidenceMarker `
        -Markers $markers `
        -Label "ctest_executable" `
        -Value (Get-JsonString -Object $ctestDetails -Name "ctest_executable")

    $renderPythonCheck = Get-PreflightCheck -Checks $Checks -Name "render_python_reusable"
    $renderPythonDetails = Get-JsonProperty -Object $renderPythonCheck -Name "details"
    Add-SyntheticEvidenceMarker `
        -Markers $markers `
        -Label "render_python" `
        -Value (Get-JsonString -Object $renderPythonDetails -Name "selected_python_path")

    $pdfDependencyInputs = Get-JsonProperty -Object $PreflightSummary -Name "pdf_dependency_inputs"
    Add-SyntheticEvidenceMarker `
        -Markers $markers `
        -Label "pdf_dependency_inputs.pdfium_prebuilt_root" `
        -Value (Get-JsonString -Object $pdfDependencyInputs -Name "pdfium_prebuilt_root")
    Add-JsonObjectStringEvidenceMarkers `
        -Markers $markers `
        -Label "pdf_dependency_inputs.cmake_cache_variables" `
        -Object (Get-JsonProperty -Object $pdfDependencyInputs -Name "cmake_cache_variables")
    Add-JsonObjectStringEvidenceMarkers `
        -Markers $markers `
        -Label "pdf_dependency_inputs.dependency_overrides" `
        -Object (Get-JsonProperty -Object $pdfDependencyInputs -Name "dependency_overrides")

    $uniqueMarkers = @($markers.ToArray() | Sort-Object -Unique)
    $kind = $declaredKind
    if ($uniqueMarkers.Count -gt 0) {
        $kind = "synthetic_fixture"
    } elseif ([string]::IsNullOrWhiteSpace($kind)) {
        $kind = "unknown"
    }

    return [ordered]@{
        evidence_kind = $kind
        synthetic_marker_count = $uniqueMarkers.Count
        synthetic_markers = @($uniqueMarkers)
        declared_evidence_kind = $declaredKind
        declared_evidence_kind_details = $declaredDetails
    }
}

function ConvertTo-CommandArgument {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) { return '""' }
    if ($Value -notmatch '[\s"'']') { return $Value }
    return "'" + ($Value -replace "'", "''") + "'"
}

function Add-OptionalPreflightOverride {
    param(
        [hashtable]$Arguments,
        [System.Collections.Generic.List[string]]$CommandArguments,
        [string]$Name,
        [string]$Value
    )

    if ([string]::IsNullOrWhiteSpace($Value)) { return }

    $Arguments[$Name] = $Value
    [void]$CommandArguments.Add(("-{0} {1}" -f $Name, (ConvertTo-CommandArgument -Value $Value)))
}

function Get-BlockingCheckNames {
    param($PreflightSummary)

    $declaredNames = @(
        Get-JsonArray -Object $PreflightSummary -Name "blocking_checks" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    if ($declaredNames.Count -gt 0) {
        return @($declaredNames | Sort-Object -Unique)
    }

    return @(
        Get-JsonArray -Object $PreflightSummary -Name "checks" |
            Where-Object {
                (Get-JsonBool -Object $_ -Name "required") -and
                (Get-JsonString -Object $_ -Name "status") -ne "pass"
            } |
            ForEach-Object { Get-JsonString -Object $_ -Name "name" } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Sort-Object -Unique
    )
}

function Get-PreflightCheck {
    param($Checks, [string]$Name)

    foreach ($check in @($Checks)) {
        if ((Get-JsonString -Object $check -Name "name") -eq $Name) {
            return $check
        }
    }
    return $null
}

function Get-BuildDirectorySnapshotFromChecks {
    param($Checks)

    $buildDirCheck = Get-PreflightCheck -Checks $Checks -Name "build_dir_exists"
    $details = Get-JsonProperty -Object $buildDirCheck -Name "details"
    if ($null -eq $details) {
        return [ordered]@{}
    }

    return [ordered]@{
        cmake_cache_path = Get-JsonString -Object $details -Name "cmake_cache_path"
        cmake_cache_exists = Get-JsonBool -Object $details -Name "cmake_cache_exists"
        ctest_manifest_path = Get-JsonString -Object $details -Name "ctest_manifest_path"
        ctest_manifest_exists = Get-JsonBool -Object $details -Name "ctest_manifest_exists"
        entry_count = Get-JsonInt -Object $details -Name "entry_count"
        entries_preview = @(Get-JsonArray -Object $details -Name "entries_preview")
    }
}

function Get-PreflightOutputGapSummary {
    param(
        $Checks,
        [string]$SourceReport,
        [string]$SourceReportDisplay,
        [string]$SourceJson,
        [string]$SourceJsonDisplay
    )

    $gaps = New-Object 'System.Collections.Generic.List[object]'
    foreach ($check in @($Checks)) {
        $details = Get-JsonProperty -Object $check -Name "details"
        if ($null -eq $details) {
            continue
        }

        $missingPaths = @(Get-JsonArray -Object $details -Name "missing_paths")
        $missingPreview = @(Get-JsonArray -Object $details -Name "missing_paths_preview")
        $missingCount = Get-JsonInt -Object $details -Name "missing_count" -DefaultValue -1
        if ($missingCount -lt 0) {
            $missingCount = if ($missingPaths.Count -gt 0) { $missingPaths.Count } else { $missingPreview.Count }
        }
        if ($missingCount -le 0 -and $missingPaths.Count -eq 0 -and $missingPreview.Count -eq 0) {
            continue
        }

        if ($missingPreview.Count -eq 0) {
            $missingPreview = @($missingPaths | Select-Object -First 20)
        }

        $requiredPaths = @(Get-JsonArray -Object $details -Name "required_paths")
        $sampleCount = Get-JsonInt -Object $details -Name "sample_count" -DefaultValue $requiredPaths.Count
        $gaps.Add([ordered]@{
            check = Get-JsonString -Object $check -Name "name"
            message = Get-JsonString -Object $check -Name "message"
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
            sample_count = $sampleCount
            missing_count = $missingCount
            missing_paths_preview = @($missingPreview | ForEach-Object { [string]$_ })
        }) | Out-Null
    }

    return @($gaps.ToArray())
}

function Get-PreflightMissingOutputCount {
    param($OutputGapSummary)

    $count = 0
    foreach ($gap in @($OutputGapSummary)) {
        $count += Get-JsonInt -Object $gap -Name "missing_count"
    }
    return $count
}

function Get-PreflightReadinessActionEvidence {
    param(
        $PdfDependencyInputs,
        $PreflightBlockingSummary,
        [string]$SourceReport,
        [string]$SourceReportDisplay,
        [string]$SourceJson,
        [string]$SourceJsonDisplay,
        [string]$CommandTemplate
    )

    $evidence = New-Object 'System.Collections.Generic.List[object]'
    $dependencyMissingInputs = @(
        Get-JsonArray -Object $PdfDependencyInputs -Name "missing_inputs" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
    if ($dependencyMissingInputs.Count -eq 0) {
        $dependencyMissingInputs = @(
            Get-JsonArray -Object $PreflightBlockingSummary -Name "pdf_dependency_missing_inputs_preview" |
                ForEach-Object { [string]$_ } |
                Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
        )
    }

    $index = 1
    foreach ($input in $dependencyMissingInputs) {
        $evidence.Add([ordered]@{
            id = "pdf_dependency_inputs.missing_input.$index"
            action = "provide_pdf_dependency_input"
            issue_key = "pdf_dependency_inputs_ready"
            item = $input
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
            repair_strategy = "provide_pdfio_pdfium_dependency_inputs"
            command_template = $CommandTemplate
        }) | Out-Null
        $index += 1
    }

    $disabledPdfBuildOptions = @(
        Get-JsonArray -Object $PreflightBlockingSummary -Name "disabled_pdf_build_options" |
            ForEach-Object { [string]$_ } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )

    foreach ($option in $disabledPdfBuildOptions) {
        $evidence.Add([ordered]@{
            id = "pdf_build_options.disabled.$option"
            action = "enable_pdf_build_option"
            issue_key = "pdf_build_options_enabled"
            item = $option
            source_report = $SourceReport
            source_report_display = $SourceReportDisplay
            source_json = $SourceJson
            source_json_display = $SourceJsonDisplay
            repair_strategy = "enable_pdf_visual_gate_build_options"
            command_template = $CommandTemplate
        }) | Out-Null
    }

    return @($evidence.ToArray())
}

function Get-PreflightBlockingSummary {
    param($PreflightSummary, $Checks, $BlockingChecks, $BuildDirSnapshot)

    $declaredSummary = Get-JsonProperty -Object $PreflightSummary -Name "blocking_summary"
    if ($null -ne $declaredSummary) {
        return $declaredSummary
    }

    $cliCheck = Get-PreflightCheck -Checks $Checks -Name "pdf_cli_export_baseline_pdfs_exist"
    $cliDetails = Get-JsonProperty -Object $cliCheck -Name "details"
    $visualCheck = Get-PreflightCheck -Checks $Checks -Name "visual_baseline_manifest_pdfs_exist"
    $visualDetails = Get-JsonProperty -Object $visualCheck -Name "details"
    $cjkCheck = Get-PreflightCheck -Checks $Checks -Name "cjk_text_layer_manifest_pdfs_exist"
    $cjkDetails = Get-JsonProperty -Object $cjkCheck -Name "details"
    $ctestCheck = Get-PreflightCheck -Checks $Checks -Name "ctest_list_contains_pdf_gate_tests"
    $ctestDetails = Get-JsonProperty -Object $ctestCheck -Name "details"
    $pdfOptionsCheck = Get-PreflightCheck -Checks $Checks -Name "pdf_build_options_enabled"
    $pdfOptionsDetails = Get-JsonProperty -Object $pdfOptionsCheck -Name "details"

    return [ordered]@{
        required_check_count = @($Checks | Where-Object { Get-JsonBool -Object $_ -Name "required" }).Count
        blocking_check_count = @($BlockingChecks).Count
        missing_cli_pdf_count = @(Get-JsonArray -Object $cliDetails -Name "missing_paths").Count
        visual_baseline_sample_count = Get-JsonInt -Object $visualDetails -Name "sample_count"
        missing_visual_baseline_pdf_count = Get-JsonInt -Object $visualDetails -Name "missing_count"
        cjk_text_layer_sample_count = Get-JsonInt -Object $cjkDetails -Name "sample_count"
        missing_cjk_text_layer_pdf_count = Get-JsonInt -Object $cjkDetails -Name "missing_count"
        build_dir_entry_count = Get-JsonInt -Object $BuildDirSnapshot -Name "entry_count"
        ctest_required_pattern_count = @(Get-JsonArray -Object $ctestDetails -Name "required_patterns").Count
        pdf_build_options_enabled = ((Get-JsonString -Object $pdfOptionsCheck -Name "status") -eq "pass")
        pdf_build_option_count = @(Get-JsonArray -Object $pdfOptionsDetails -Name "required_options").Count
        missing_pdf_build_options = @(Get-JsonArray -Object $pdfOptionsDetails -Name "missing_options")
        disabled_pdf_build_options = @(Get-JsonArray -Object $pdfOptionsDetails -Name "disabled_options")
    }
}

function Get-PathDisplayIfAvailable {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    return Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
}

function Get-ControlledVisualSmokeEvidence {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return [ordered]@{
            available = $false
            status = "not_provided"
            passed = $false
            source_json = ""
            source_json_display = ""
            root = ""
            root_display = ""
            case_count = 0
            passed_case_count = 0
            failed_case_count = 0
            min_nonwhite_ratio = ""
            cases = @()
            error_message = ""
        }
    }

    $resolvedPath = Resolve-RepoPath -RepoRoot $RepoRoot -Path $Path -AllowMissing
    $displayPath = Get-DisplayPath -RepoRoot $RepoRoot -Path $resolvedPath
    if (-not (Test-Path -LiteralPath $resolvedPath -PathType Leaf)) {
        return [ordered]@{
            available = $false
            status = "missing"
            passed = $false
            source_json = $resolvedPath
            source_json_display = $displayPath
            root = ""
            root_display = ""
            case_count = 0
            passed_case_count = 0
            failed_case_count = 0
            min_nonwhite_ratio = ""
            cases = @()
            error_message = "Controlled visual smoke JSON was not found."
        }
    }

    try {
        $smoke = Get-Content -Raw -Encoding UTF8 -LiteralPath $resolvedPath | ConvertFrom-Json
        $cases = New-Object 'System.Collections.Generic.List[object]'
        $minNonwhiteRatio = $null
        foreach ($case in @(Get-JsonArray -Object $smoke -Name "cases")) {
            $metrics = @(Get-JsonArray -Object $case -Name "png_metrics")
            $contactSheetPath = ""
            $caseMinNonwhiteRatio = $null
            foreach ($metric in $metrics) {
                $metricPath = Get-JsonString -Object $metric -Name "path"
                if ($metricPath -match 'contact-sheet\.png$') {
                    $contactSheetPath = $metricPath
                }
                $ratioText = Get-JsonValueText -Object $metric -Name "nonwhite_ratio"
                if (-not [string]::IsNullOrWhiteSpace($ratioText)) {
                    try {
                        $ratio = [double]::Parse($ratioText, [System.Globalization.CultureInfo]::InvariantCulture)
                        if ($null -eq $minNonwhiteRatio -or $ratio -lt $minNonwhiteRatio) {
                            $minNonwhiteRatio = $ratio
                        }
                        if ($null -eq $caseMinNonwhiteRatio -or $ratio -lt $caseMinNonwhiteRatio) {
                            $caseMinNonwhiteRatio = $ratio
                        }
                    } catch {
                    }
                }
            }

            $cases.Add([ordered]@{
                case = Get-JsonString -Object $case -Name "case"
                passed = Get-JsonBool -Object $case -Name "passed"
                png_count = $metrics.Count
                contact_sheet = $contactSheetPath
                contact_sheet_display = Get-PathDisplayIfAvailable -RepoRoot $RepoRoot -Path $contactSheetPath
                min_nonwhite_ratio = if ($null -eq $caseMinNonwhiteRatio) { "" } else { [string]::Format([System.Globalization.CultureInfo]::InvariantCulture, "{0}", $caseMinNonwhiteRatio) }
            }) | Out-Null
        }

        $caseItems = @($cases.ToArray())
        $passedCaseCount = @($caseItems | Where-Object { Get-JsonBool -Object $_ -Name "passed" }).Count
        $root = Get-JsonString -Object $smoke -Name "root"
        return [ordered]@{
            available = $true
            status = Get-JsonString -Object $smoke -Name "status" -DefaultValue "unknown"
            passed = Get-JsonBool -Object $smoke -Name "passed"
            source_json = $resolvedPath
            source_json_display = $displayPath
            root = $root
            root_display = Get-PathDisplayIfAvailable -RepoRoot $RepoRoot -Path $root
            case_count = Get-JsonInt -Object $smoke -Name "case_count" -DefaultValue $caseItems.Count
            passed_case_count = $passedCaseCount
            failed_case_count = [Math]::Max(0, $caseItems.Count - $passedCaseCount)
            min_nonwhite_ratio = if ($null -eq $minNonwhiteRatio) { "" } else { [string]::Format([System.Globalization.CultureInfo]::InvariantCulture, "{0}", $minNonwhiteRatio) }
            cases = @($caseItems)
            error_message = ""
        }
    } catch {
        return [ordered]@{
            available = $false
            status = "unreadable"
            passed = $false
            source_json = $resolvedPath
            source_json_display = $displayPath
            root = ""
            root_display = ""
            case_count = 0
            passed_case_count = 0
            failed_case_count = 0
            min_nonwhite_ratio = ""
            cases = @()
            error_message = $_.Exception.Message
        }
    }
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# PDF Visual Release Gate Preflight Governance Report") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Release ready: ``$($Summary.release_ready)``") | Out-Null
    $lines.Add("- Source failures: ``$($Summary.source_failure_count)``") | Out-Null
    $lines.Add("- source_failure_count: ``$($Summary.source_failure_count)``") | Out-Null
    $lines.Add("- Preflight ready: ``$($Summary.preflight_ready)``") | Out-Null
    $lines.Add("- Preflight status: ``$($Summary.preflight_status)``") | Out-Null
    $lines.Add("- Full visual gate required: ``$($Summary.full_visual_gate_required)``") | Out-Null
    $lines.Add("- Full visual gate status: ``$($Summary.full_visual_gate_status)``") | Out-Null
    $controlledVisualSmoke = Get-JsonProperty -Object $Summary -Name "controlled_visual_smoke"
    $lines.Add("- Controlled visual smoke status: ``$(Get-JsonString -Object $controlledVisualSmoke -Name "status")``") | Out-Null
    $lines.Add("- Controlled visual smoke passed: ``$(Get-JsonBool -Object $controlledVisualSmoke -Name "passed")``") | Out-Null
    $lines.Add("- Controlled visual smoke summary: ``$(Get-JsonString -Object $controlledVisualSmoke -Name "source_json_display")``") | Out-Null
    $lines.Add("- Evidence kind: ``$(Get-JsonString -Object $Summary -Name "evidence_kind")``") | Out-Null
    $lines.Add("- Synthetic evidence markers: ``$(Get-JsonInt -Object $Summary -Name "synthetic_evidence_marker_count")``") | Out-Null
    $syntheticEvidenceMarkers = @(Get-JsonArray -Object $Summary -Name "synthetic_evidence_markers")
    if ($syntheticEvidenceMarkers.Count -gt 0) {
        $lines.Add("- Synthetic evidence marker preview:") | Out-Null
        foreach ($marker in @($syntheticEvidenceMarkers | Select-Object -First 5)) {
            $lines.Add("  - ``$([string]$marker)``") | Out-Null
        }
    }
    $lines.Add("- Preflight summary: ``$($Summary.preflight_summary_json_display)``") | Out-Null
    $lines.Add("- Build dir: ``$($Summary.build_dir_display)``") | Out-Null
    $lines.Add("- Build dir source: ``$($Summary.build_dir_source)``") | Out-Null
    $lines.Add("- Requested build dir: ``$($Summary.requested_build_dir_display)``") | Out-Null
    $buildDirSnapshot = Get-JsonProperty -Object $Summary -Name "build_dir_snapshot"
    $hasBuildDirSnapshot = $false
    if ($null -ne $buildDirSnapshot) {
        if ($buildDirSnapshot -is [System.Collections.IDictionary]) {
            $hasBuildDirSnapshot = $buildDirSnapshot.Count -gt 0
        } else {
            $hasBuildDirSnapshot = @($buildDirSnapshot.PSObject.Properties).Count -gt 0
        }
    }
    if ($hasBuildDirSnapshot) {
        $lines.Add("- Build CMake cache: ``$(Get-JsonBool -Object $buildDirSnapshot -Name "cmake_cache_exists")``") | Out-Null
        $lines.Add("- Build CTest manifest: ``$(Get-JsonBool -Object $buildDirSnapshot -Name "ctest_manifest_exists")``") | Out-Null
        $lines.Add("- Build entry count: ``$(Get-JsonInt -Object $buildDirSnapshot -Name "entry_count")``") | Out-Null
    }
    $buildDirAutoCandidates = @(Get-JsonArray -Object $Summary -Name "build_dir_auto_candidates")
    if ($buildDirAutoCandidates.Count -gt 0) {
        $lines.Add("- Build auto candidates:") | Out-Null
        foreach ($candidate in $buildDirAutoCandidates) {
            $lines.Add("  - ``$(Get-JsonString -Object $candidate -Name "relative_path")``: reusable=``$(Get-JsonBool -Object $candidate -Name "looks_reusable")``; CMakeCache=``$(Get-JsonBool -Object $candidate -Name "cmake_cache_exists")``; CTest=``$(Get-JsonBool -Object $candidate -Name "ctest_manifest_exists")``; PDFOptions=``$(Get-JsonBool -Object $candidate -Name "pdf_build_options_enabled")``") | Out-Null
            $candidatePdfOptions = @(Get-JsonArray -Object $candidate -Name "pdf_build_options")
            foreach ($option in $candidatePdfOptions) {
                $lines.Add("    - PDF option ``$(Get-JsonString -Object $option -Name "name")``: present=``$(Get-JsonBool -Object $option -Name "present")``; value=``$(Get-JsonString -Object $option -Name "value")``; enabled=``$(Get-JsonBool -Object $option -Name "enabled")``") | Out-Null
            }
        }
    }
    $lines.Add("- PDF dependency inputs status: ``$(Get-JsonString -Object $Summary -Name "pdf_dependency_inputs_status")``") | Out-Null
    $lines.Add("- Selected PDFium provider: ``$(Get-JsonString -Object $Summary -Name "selected_pdfium_provider")``") | Out-Null
    $lines.Add("- PDF dependency missing inputs: ``$(Get-JsonInt -Object $Summary -Name "pdf_dependency_missing_input_count")``") | Out-Null
    $lines.Add("- PDFio dependency ready: ``$(Get-JsonBool -Object $Summary -Name "pdfio_dependency_ready")``") | Out-Null
    $lines.Add("- PDFium dependency ready: ``$(Get-JsonBool -Object $Summary -Name "pdfium_dependency_ready")``") | Out-Null
    $dependencyInputs = Get-JsonProperty -Object $Summary -Name "pdf_dependency_inputs"
    if ($null -ne $dependencyInputs) {
        $dependencyMissingInputs = @(Get-JsonArray -Object $dependencyInputs -Name "missing_inputs")
        if ($dependencyMissingInputs.Count -gt 0) {
            $lines.Add("- PDF dependency missing inputs preview:") | Out-Null
            foreach ($input in @($dependencyMissingInputs | Select-Object -First 5)) {
                $lines.Add("  - ``$([string]$input)``") | Out-Null
            }
        }
    }
    $lines.Add("- Blocking checks: ``$($Summary.blocking_check_count)``") | Out-Null
    $blockingSummary = Get-JsonProperty -Object $Summary -Name "blocking_summary"
    if ($null -ne $blockingSummary) {
        $lines.Add("- Required checks: ``$(Get-JsonInt -Object $blockingSummary -Name "required_check_count")``") | Out-Null
        $lines.Add("- Missing CLI PDFs: ``$(Get-JsonInt -Object $blockingSummary -Name "missing_cli_pdf_count")``") | Out-Null
        $lines.Add("- Missing visual baseline PDFs: ``$(Get-JsonInt -Object $blockingSummary -Name "missing_visual_baseline_pdf_count")``") | Out-Null
        $lines.Add("- Missing CJK text-layer PDFs: ``$(Get-JsonInt -Object $blockingSummary -Name "missing_cjk_text_layer_pdf_count")``") | Out-Null
        if ($null -ne (Get-JsonProperty -Object $blockingSummary -Name "pdf_build_options_enabled")) {
            $disabledPdfBuildOptions = @(Get-JsonArray -Object $blockingSummary -Name "disabled_pdf_build_options")
            $missingPdfBuildOptions = @(Get-JsonArray -Object $blockingSummary -Name "missing_pdf_build_options")
            $lines.Add("- PDF build options enabled: ``$(Get-JsonBool -Object $blockingSummary -Name "pdf_build_options_enabled")``") | Out-Null
            $lines.Add("- Disabled PDF build options: ``$($disabledPdfBuildOptions -join ', ')``") | Out-Null
            $lines.Add("- Missing PDF build options: ``$($missingPdfBuildOptions -join ', ')``") | Out-Null
        }
        if ($null -ne (Get-JsonProperty -Object $blockingSummary -Name "free_memory_mb") -or
            $null -ne (Get-JsonProperty -Object $blockingSummary -Name "min_free_memory_mb")) {
            $lines.Add("- Free memory MB: ``$(Get-JsonValueText -Object $blockingSummary -Name "free_memory_mb")``") | Out-Null
            $lines.Add("- Minimum free memory MB: ``$(Get-JsonValueText -Object $blockingSummary -Name "min_free_memory_mb")``") | Out-Null
            $lines.Add("- Memory guard blocked: ``$(Get-JsonBool -Object $blockingSummary -Name "memory_guard_blocked")``") | Out-Null
            $lines.Add("- Memory guard skipped: ``$(Get-JsonBool -Object $blockingSummary -Name "memory_guard_skipped")``") | Out-Null
        }
    }
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Warnings: ``$($Summary.warning_count)``") | Out-Null
    $lines.Add("- Output gap checks: ``$($Summary.output_gap_count)``") | Out-Null
    $lines.Add("- Missing outputs: ``$($Summary.missing_output_count)``") | Out-Null
    $lines.Add("") | Out-Null

    $lines.Add("## Controlled Visual Smoke") | Out-Null
    $lines.Add("") | Out-Null
    if ($null -eq $controlledVisualSmoke -or -not (Get-JsonBool -Object $controlledVisualSmoke -Name "available")) {
        $lines.Add("- status: ``$(Get-JsonString -Object $controlledVisualSmoke -Name "status" -DefaultValue "not_provided")``") | Out-Null
        $errorMessage = Get-JsonString -Object $controlledVisualSmoke -Name "error_message"
        if (-not [string]::IsNullOrWhiteSpace($errorMessage)) {
            $lines.Add("- error: $errorMessage") | Out-Null
        }
    } else {
        $lines.Add("- source_json: ``$(Get-JsonString -Object $controlledVisualSmoke -Name "source_json")``") | Out-Null
        $lines.Add("- source_json_display: ``$(Get-JsonString -Object $controlledVisualSmoke -Name "source_json_display")``") | Out-Null
        $lines.Add("- root: ``$(Get-JsonString -Object $controlledVisualSmoke -Name "root_display")``") | Out-Null
        $lines.Add("- cases: ``$(Get-JsonInt -Object $controlledVisualSmoke -Name "case_count")``") | Out-Null
        $lines.Add("- passed cases: ``$(Get-JsonInt -Object $controlledVisualSmoke -Name "passed_case_count")``") | Out-Null
        $lines.Add("- failed cases: ``$(Get-JsonInt -Object $controlledVisualSmoke -Name "failed_case_count")``") | Out-Null
        $lines.Add("- min nonwhite ratio: ``$(Get-JsonValueText -Object $controlledVisualSmoke -Name "min_nonwhite_ratio")``") | Out-Null
        foreach ($case in @(Get-JsonArray -Object $controlledVisualSmoke -Name "cases")) {
            $lines.Add("  - ``$(Get-JsonString -Object $case -Name "case")``: passed=``$(Get-JsonBool -Object $case -Name "passed")``; png_count=``$(Get-JsonInt -Object $case -Name "png_count")``; contact_sheet=``$(Get-JsonString -Object $case -Name "contact_sheet_display")``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Blocking Checks") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.blocking_checks).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($name in @($Summary.blocking_checks)) {
            $lines.Add("- ``$name``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Readiness Action Evidence") | Out-Null
    $lines.Add("") | Out-Null
    $readinessActionEvidence = @(Get-JsonArray -Object $Summary -Name "readiness_action_evidence")
    if ($readinessActionEvidence.Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($evidence in $readinessActionEvidence) {
            $lines.Add("- ``$(Get-JsonString -Object $evidence -Name "id")``: action=``$(Get-JsonString -Object $evidence -Name "action")`` issue_key=``$(Get-JsonString -Object $evidence -Name "issue_key")``") | Out-Null
            $lines.Add("  - item: ``$(Get-JsonString -Object $evidence -Name "item")``") | Out-Null
            $evidenceSourceReport = Get-JsonString -Object $evidence -Name "source_report"
            if (-not [string]::IsNullOrWhiteSpace($evidenceSourceReport)) {
                $lines.Add("  - source_report: ``$evidenceSourceReport``") | Out-Null
            }
            $evidenceSourceJson = Get-JsonString -Object $evidence -Name "source_json"
            if (-not [string]::IsNullOrWhiteSpace($evidenceSourceJson)) {
                $lines.Add("  - source_json: ``$evidenceSourceJson``") | Out-Null
            }
            $evidenceSourceReportDisplay = Get-JsonString -Object $evidence -Name "source_report_display"
            if (-not [string]::IsNullOrWhiteSpace($evidenceSourceReportDisplay)) {
                $lines.Add("  - source_report_display: ``$evidenceSourceReportDisplay``") | Out-Null
            }
            $evidenceSourceJsonDisplay = Get-JsonString -Object $evidence -Name "source_json_display"
            if (-not [string]::IsNullOrWhiteSpace($evidenceSourceJsonDisplay)) {
                $lines.Add("  - source_json_display: ``$evidenceSourceJsonDisplay``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.id)``: action=``$($blocker.action)`` status=``$($blocker.status)``") | Out-Null
            $lines.Add("  - message: $($blocker.message)") | Out-Null
            $blockerSourceReport = Get-JsonString -Object $blocker -Name "source_report"
            if (-not [string]::IsNullOrWhiteSpace($blockerSourceReport)) {
                $lines.Add("  - source_report: ``$blockerSourceReport``") | Out-Null
            }
            $blockerSourceJson = Get-JsonString -Object $blocker -Name "source_json"
            if (-not [string]::IsNullOrWhiteSpace($blockerSourceJson)) {
                $lines.Add("  - source_json: ``$blockerSourceJson``") | Out-Null
            }
            $lines.Add("  - source_report_display: ``$($blocker.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($blocker.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.command_template)) {
                $lines.Add("  - command_template: ``$($blocker.command_template)``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    $warnings = @(Get-JsonArray -Object $Summary -Name "warnings")
    if ($warnings.Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in $warnings) {
            $lines.Add("- ``$(Get-JsonString -Object $warning -Name "id")``: action=``$(Get-JsonString -Object $warning -Name "action")`` status=``$(Get-JsonString -Object $warning -Name "status")``") | Out-Null
            $lines.Add("  - message: $(Get-JsonString -Object $warning -Name "message")") | Out-Null
            $warningSourceReport = Get-JsonString -Object $warning -Name "source_report"
            if (-not [string]::IsNullOrWhiteSpace($warningSourceReport)) {
                $lines.Add("  - source_report: ``$warningSourceReport``") | Out-Null
            }
            $warningSourceJson = Get-JsonString -Object $warning -Name "source_json"
            if (-not [string]::IsNullOrWhiteSpace($warningSourceJson)) {
                $lines.Add("  - source_json: ``$warningSourceJson``") | Out-Null
            }
            $warningSourceReportDisplay = Get-JsonString -Object $warning -Name "source_report_display"
            if (-not [string]::IsNullOrWhiteSpace($warningSourceReportDisplay)) {
                $lines.Add("  - source_report_display: ``$warningSourceReportDisplay``") | Out-Null
            }
            $warningSourceJsonDisplay = Get-JsonString -Object $warning -Name "source_json_display"
            if (-not [string]::IsNullOrWhiteSpace($warningSourceJsonDisplay)) {
                $lines.Add("  - source_json_display: ``$warningSourceJsonDisplay``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Output Gaps") | Out-Null
    $lines.Add("") | Out-Null
    $outputGapSummary = @(Get-JsonArray -Object $Summary -Name "output_gap_summary")
    if ($outputGapSummary.Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($gap in $outputGapSummary) {
            $lines.Add("- ``$(Get-JsonString -Object $gap -Name "check")``: missing=``$(Get-JsonInt -Object $gap -Name "missing_count")`` sample_count=``$(Get-JsonInt -Object $gap -Name "sample_count")``") | Out-Null
            $gapSourceReport = Get-JsonString -Object $gap -Name "source_report"
            if (-not [string]::IsNullOrWhiteSpace($gapSourceReport)) {
                $lines.Add("  - source_report: ``$gapSourceReport``") | Out-Null
            }
            $gapSourceJson = Get-JsonString -Object $gap -Name "source_json"
            if (-not [string]::IsNullOrWhiteSpace($gapSourceJson)) {
                $lines.Add("  - source_json: ``$gapSourceJson``") | Out-Null
            }
            $gapSourceReportDisplay = Get-JsonString -Object $gap -Name "source_report_display"
            if (-not [string]::IsNullOrWhiteSpace($gapSourceReportDisplay)) {
                $lines.Add("  - source_report_display: ``$gapSourceReportDisplay``") | Out-Null
            }
            $gapSourceJsonDisplay = Get-JsonString -Object $gap -Name "source_json_display"
            if (-not [string]::IsNullOrWhiteSpace($gapSourceJsonDisplay)) {
                $lines.Add("  - source_json_display: ``$gapSourceJsonDisplay``") | Out-Null
            }
            $preview = @(Get-JsonArray -Object $gap -Name "missing_paths_preview")
            foreach ($path in @($preview | Select-Object -First 5)) {
                $lines.Add("  - ``$path``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null

    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.id)``: action=``$($item.action)`` open_command=``$($item.open_command)``") | Out-Null
            $itemSourceReport = Get-JsonString -Object $item -Name "source_report"
            if (-not [string]::IsNullOrWhiteSpace($itemSourceReport)) {
                $lines.Add("  - source_report: ``$itemSourceReport``") | Out-Null
            }
            $itemSourceJson = Get-JsonString -Object $item -Name "source_json"
            if (-not [string]::IsNullOrWhiteSpace($itemSourceJson)) {
                $lines.Add("  - source_json: ``$itemSourceJson``") | Out-Null
            }
            $lines.Add("  - source_report_display: ``$($item.source_report_display)``") | Out-Null
            $lines.Add("  - source_json_display: ``$($item.source_json_display)``") | Out-Null
            $itemIssueKeys = @(
                Get-JsonArray -Object $item -Name "issue_keys" |
                    ForEach-Object { [string]$_ } |
                    Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
            )
            if ($itemIssueKeys.Count -gt 0) {
                $lines.Add("  - issue_keys: ``$($itemIssueKeys -join ', ')``") | Out-Null
            }
        }
    }
    return @($lines)
}
