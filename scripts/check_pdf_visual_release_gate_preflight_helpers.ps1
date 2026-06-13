function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([System.IO.Path]::IsPathRooted($InputPath)) {
        return [System.IO.Path]::GetFullPath($InputPath)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $InputPath))
}

function Write-Utf8NoBomFile {
    param(
        [string]$Path,
        [string]$Text
    )

    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Test-CMakeCacheBoolOn {
    param([string]$Value)

    return [string]$Value -in @("1", "ON", "TRUE", "YES", "Y")
}

function Get-CMakeCacheValues {
    param(
        [string]$Path,
        [string[]]$Names
    )

    $rawValues = @{}
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        foreach ($line in Get-Content -LiteralPath $Path -Encoding UTF8) {
            foreach ($name in $Names) {
                $pattern = "^{0}:[^=]*=(.*)$" -f [regex]::Escape($name)
                if ($line -match $pattern) {
                    $rawValues[$name] = $Matches[1].Trim()
                }
            }
        }
    }

    return $rawValues
}

function Resolve-PreferredBuildDir {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $requestedPath = Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $InputPath
    $autoCandidates = @()
    if ($InputPath -ne ".bpdf-roundtrip-msvc" -or
        (Test-Path -LiteralPath $requestedPath -PathType Container)) {
        return [pscustomobject]@{
            Path = $requestedPath
            Source = "requested"
            RequestedPath = $requestedPath
            AutoCandidates = $autoCandidates
        }
    }

    foreach ($candidate in @("build", "out\build")) {
        $candidatePath = Resolve-RepoPath -RepoRoot $RepoRoot -InputPath $candidate
        $cmakeCachePath = Join-Path $candidatePath "CMakeCache.txt"
        $candidateExists = Test-Path -LiteralPath $candidatePath -PathType Container
        $cmakeCacheExists = Test-Path -LiteralPath $cmakeCachePath -PathType Leaf
        $ctestManifestExists = Test-Path -LiteralPath (Join-Path $candidatePath "CTestTestfile.cmake") -PathType Leaf
        $requiredPdfBuildOptions = @(
            "FEATHERDOC_BUILD_PDF",
            "FEATHERDOC_BUILD_PDF_IMPORT"
        )
        $cacheValues = Get-CMakeCacheValues -Path $cmakeCachePath -Names $requiredPdfBuildOptions
        $pdfOptionSnapshot = New-Object System.Collections.ArrayList
        $pdfBuildOptionsReady = $cmakeCacheExists
        foreach ($name in $requiredPdfBuildOptions) {
            $hasValue = $cacheValues.ContainsKey($name)
            $value = if ($hasValue) { [string]$cacheValues[$name] } else { "" }
            $enabled = $hasValue -and (Test-CMakeCacheBoolOn -Value $value)
            if (-not $enabled) {
                $pdfBuildOptionsReady = $false
            }
            $pdfOptionSnapshot.Add([ordered]@{
                name = $name
                present = [bool]$hasValue
                value = $value
                enabled = [bool]$enabled
            }) | Out-Null
        }
        $candidateLooksReusable = $candidateExists -and $cmakeCacheExists -and $ctestManifestExists -and $pdfBuildOptionsReady
        $autoCandidates += [ordered]@{
            relative_path = $candidate
            path = $candidatePath
            exists = [bool]$candidateExists
            cmake_cache_exists = [bool]$cmakeCacheExists
            ctest_manifest_exists = [bool]$ctestManifestExists
            pdf_build_options_enabled = [bool]$pdfBuildOptionsReady
            pdf_build_options = @($pdfOptionSnapshot)
            looks_reusable = [bool]$candidateLooksReusable
        }
        if ($candidateLooksReusable) {
            return [pscustomobject]@{
                Path = $candidatePath
                Source = "auto:$candidate"
                RequestedPath = $requestedPath
                AutoCandidates = $autoCandidates
            }
        }
    }

    return [pscustomobject]@{
        Path = $requestedPath
        Source = "requested"
        RequestedPath = $requestedPath
        AutoCandidates = $autoCandidates
    }
}

function New-CheckResult {
    param(
        [string]$Name,
        [string]$Status,
        [bool]$Required,
        [string]$Message,
        [string]$Path = "",
        [object]$Details = $null
    )

    $result = [ordered]@{
        name = $Name
        status = $Status
        required = $Required
        message = $Message
    }
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        $result.path = $Path
    }
    if ($null -ne $Details) {
        $result.details = $Details
    }
    return $result
}

function Add-CheckResult {
    param(
        [System.Collections.ArrayList]$Checks,
        [string]$Name,
        [string]$Status,
        [bool]$Required,
        [string]$Message,
        [string]$Path = "",
        [object]$Details = $null
    )

    $Checks.Add((New-CheckResult `
        -Name $Name `
        -Status $Status `
        -Required $Required `
        -Message $Message `
        -Path $Path `
        -Details $Details)) | Out-Null
}

function Test-PythonImport {
    param(
        [string]$PythonCommand,
        [string]$ModuleName
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        & $PythonCommand -c "import $ModuleName" 1> $null 2> $null
        return $LASTEXITCODE -eq 0
    } catch {
        return $false
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
}

function Resolve-BasePythonCandidate {
    if (-not [string]::IsNullOrWhiteSpace($env:FEATHERDOC_PYTHON_EXECUTABLE) -and
        (Test-Path -LiteralPath $env:FEATHERDOC_PYTHON_EXECUTABLE -PathType Leaf)) {
        return (Resolve-Path -LiteralPath $env:FEATHERDOC_PYTHON_EXECUTABLE).Path
    }

    if (-not [string]::IsNullOrWhiteSpace($env:USERPROFILE)) {
        $bundledPython = Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
        if (Test-Path -LiteralPath $bundledPython -PathType Leaf) {
            return (Resolve-Path -LiteralPath $bundledPython).Path
        }
    }

    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) {
        return $python.Source
    }

    $python3 = Get-Command python3 -ErrorAction SilentlyContinue
    if ($python3) {
        return $python3.Source
    }

    return ""
}

function Test-JsonBooleanProperty {
    param(
        [object]$Object,
        [string]$Name
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $false
    }
    return $property.Value -eq $true
}

function Get-JsonPropertyValue {
    param(
        [object]$Object,
        [string]$Name,
        [object]$DefaultValue = $null
    )

    if ($null -eq $Object) {
        return $DefaultValue
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return $Object[$Name]
        }
        return $DefaultValue
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $DefaultValue
    }
    return $property.Value
}

function New-RecoveryStep {
    param(
        [string]$Id,
        [string]$Priority,
        [string]$Summary,
        [string]$Command = ""
    )

    $step = [ordered]@{
        id = $Id
        priority = $Priority
        summary = $Summary
    }
    if (-not [string]::IsNullOrWhiteSpace($Command)) {
        $step.command = $Command
    }

    return $step
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

function Get-PreflightEvidenceKind {
    param(
        [string]$ResolvedBuildDir,
        [string]$CTestExecutable,
        [string]$SelectedPythonPath,
        [object]$PdfDependencyInputs
    )

    $markers = New-Object System.Collections.ArrayList
    Add-SyntheticEvidenceMarker -Markers $markers -Label "build_dir" -Value $ResolvedBuildDir
    Add-SyntheticEvidenceMarker -Markers $markers -Label "ctest_executable" -Value $CTestExecutable
    Add-SyntheticEvidenceMarker -Markers $markers -Label "render_python" -Value $SelectedPythonPath

    if ($null -ne $PdfDependencyInputs) {
        Add-SyntheticEvidenceMarker `
            -Markers $markers `
            -Label "pdf_dependency_inputs.pdfium_prebuilt_root" `
            -Value ([string](Get-JsonPropertyValue -Object $PdfDependencyInputs -Name "pdfium_prebuilt_root" -DefaultValue ""))
        Add-JsonObjectStringEvidenceMarkers `
            -Markers $markers `
            -Label "pdf_dependency_inputs.cmake_cache_variables" `
            -Object (Get-JsonPropertyValue -Object $PdfDependencyInputs -Name "cmake_cache_variables" -DefaultValue $null)
        Add-JsonObjectStringEvidenceMarkers `
            -Markers $markers `
            -Label "pdf_dependency_inputs.dependency_overrides" `
            -Object (Get-JsonPropertyValue -Object $PdfDependencyInputs -Name "dependency_overrides" -DefaultValue $null)
    }

    $uniqueMarkers = @($markers.ToArray() | Sort-Object -Unique)
    return [ordered]@{
        evidence_kind = if ($uniqueMarkers.Count -gt 0) { "synthetic_fixture" } else { "real_build" }
        synthetic_marker_count = $uniqueMarkers.Count
        synthetic_markers = @($uniqueMarkers)
    }
}

function Get-PdfDependencyInputsSummary {
    param(
        [string]$RepoRoot,
        [string]$CMakeCachePath = "",
        [hashtable]$OverrideArguments = @{}
    )

    $scriptPath = Join-Path $RepoRoot "scripts\check_pdf_dependency_inputs.ps1"
    $summary = [ordered]@{
        available = $false
        script_path = $scriptPath
        cmake_cache_path = $CMakeCachePath
        cmake_cache_variables = [ordered]@{}
        dependency_overrides = [ordered]@{}
        schema = ""
        status = "unavailable"
        check_exit_code = $null
        error_message = ""
        pdfio_ready = $false
        pdfium_provider = ""
        selected_pdfium_provider = ""
        pdfium_ready = $false
        pdfium_prebuilt_root = ""
        pdfium_prebuilt_root_exists = $false
        missing_input_count = 0
        missing_inputs = @()
    }

    if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
        $summary.error_message = "Dependency input check script was not found."
        return $summary
    }

    try {
        $dependencyArguments = @{}
        $cacheVariableMap = [ordered]@{
            FEATHERDOC_PDFIO_SOURCE_DIR = "PdfioSourceDir"
            FEATHERDOC_PDFIUM_PROVIDER = "PdfiumProvider"
            FEATHERDOC_PDFIUM_SOURCE_DIR = "PdfiumSourceDir"
            FEATHERDOC_PDFIUM_PREBUILT_ROOT = "PdfiumPrebuiltRoot"
            FEATHERDOC_PDFIUM_LIBRARY = "PdfiumLibrary"
            FEATHERDOC_PDFIUM_INCLUDE_DIR = "PdfiumIncludeDir"
            FEATHERDOC_PDFIUM_RUNTIME_DLL = "PdfiumRuntimeDll"
            FEATHERDOC_PDFIUM_RUNTIME_DIR = "PdfiumRuntimeDir"
        }
        if (-not [string]::IsNullOrWhiteSpace($CMakeCachePath) -and
            (Test-Path -LiteralPath $CMakeCachePath -PathType Leaf)) {
            $cacheValues = Get-CMakeCacheValues -Path $CMakeCachePath -Names @($cacheVariableMap.Keys)
            foreach ($cacheName in $cacheVariableMap.Keys) {
                if (-not $cacheValues.ContainsKey($cacheName)) {
                    continue
                }

                $value = [string]$cacheValues[$cacheName]
                if ([string]::IsNullOrWhiteSpace($value)) {
                    continue
                }

                $summary.cmake_cache_variables[$cacheName] = $value
                $dependencyArguments[$cacheVariableMap[$cacheName]] = $value
            }
        }
        foreach ($argumentName in $OverrideArguments.Keys) {
            $value = [string]$OverrideArguments[$argumentName]
            if ([string]::IsNullOrWhiteSpace($value)) {
                continue
            }

            $summary.dependency_overrides[$argumentName] = $value
            $dependencyArguments[$argumentName] = $value
        }

        $output = @(& $scriptPath @dependencyArguments 2>&1 | ForEach-Object { $_.ToString() })
        $lastExitCodeValue = Get-Variable -Name LASTEXITCODE -ValueOnly -ErrorAction SilentlyContinue
        $summary.check_exit_code = if ($null -eq $lastExitCodeValue) { 0 } else { [int]$lastExitCodeValue }
        $dependencySummary = ($output -join "`n") | ConvertFrom-Json
        $summary.available = $true
        $summary.schema = [string](Get-JsonPropertyValue -Object $dependencySummary -Name "schema" -DefaultValue "")
        $summary.status = [string](Get-JsonPropertyValue -Object $dependencySummary -Name "status" -DefaultValue "unavailable")
        $summary.pdfio_ready = [bool](Get-JsonPropertyValue -Object $dependencySummary -Name "pdfio_ready" -DefaultValue $false)
        $summary.pdfium_provider = [string](Get-JsonPropertyValue -Object $dependencySummary -Name "pdfium_provider" -DefaultValue "")
        $summary.selected_pdfium_provider = [string](Get-JsonPropertyValue -Object $dependencySummary -Name "selected_pdfium_provider" -DefaultValue "")
        $summary.pdfium_ready = [bool](Get-JsonPropertyValue -Object $dependencySummary -Name "pdfium_ready" -DefaultValue $false)
        $summary.pdfium_prebuilt_root = [string](Get-JsonPropertyValue -Object $dependencySummary -Name "pdfium_prebuilt_root" -DefaultValue "")
        $summary.pdfium_prebuilt_root_exists = [bool](Get-JsonPropertyValue -Object $dependencySummary -Name "pdfium_prebuilt_root_exists" -DefaultValue $false)
        $summary.missing_input_count = [int](Get-JsonPropertyValue -Object $dependencySummary -Name "missing_input_count" -DefaultValue 0)
        $summary.missing_inputs = @(
            Get-JsonPropertyValue -Object $dependencySummary -Name "missing_inputs" -DefaultValue @() |
                ForEach-Object { [string]$_ }
        )
        return $summary
    } catch {
        $summary.error_message = $_.Exception.Message
        return $summary
    }
}

function Get-BuildDirectorySnapshot {
    param([string]$Path)

    $exists = Test-Path -LiteralPath $Path -PathType Container
    $cmakeCachePath = Join-Path $Path "CMakeCache.txt"
    $ctestManifestPath = Join-Path $Path "CTestTestfile.cmake"
    $entries = @()
    $entryCount = 0

    if ($exists) {
        $allEntries = @(Get-ChildItem -LiteralPath $Path -Force -ErrorAction SilentlyContinue)
        $entryCount = $allEntries.Count
        $entries = @(
            $allEntries |
                Sort-Object Name |
                Select-Object -First 20 |
                ForEach-Object {
                    [ordered]@{
                        name = $_.Name
                        type = if ($_.PSIsContainer) { "directory" } else { "file" }
                        bytes = if ($_.PSIsContainer) { $null } else { $_.Length }
                    }
                }
        )
    }

    return [ordered]@{
        cmake_cache_path = $cmakeCachePath
        cmake_cache_exists = Test-Path -LiteralPath $cmakeCachePath -PathType Leaf
        ctest_manifest_path = $ctestManifestPath
        ctest_manifest_exists = Test-Path -LiteralPath $ctestManifestPath -PathType Leaf
        entry_count = $entryCount
        entries_preview = $entries
    }
}

function Get-CMakeCachePdfBuildOptions {
    param(
        [string]$Path,
        [string[]]$Names
    )

    $rawValues = Get-CMakeCacheValues -Path $Path -Names $Names

    $options = New-Object System.Collections.ArrayList
    $missingOptions = New-Object System.Collections.ArrayList
    $disabledOptions = New-Object System.Collections.ArrayList
    foreach ($name in $Names) {
        $hasValue = $rawValues.ContainsKey($name)
        $value = if ($hasValue) { [string]$rawValues[$name] } else { "" }
        $enabled = $hasValue -and (Test-CMakeCacheBoolOn -Value $value)
        if (-not $hasValue) {
            $missingOptions.Add($name) | Out-Null
        } elseif (-not $enabled) {
            $disabledOptions.Add($name) | Out-Null
        }

        $options.Add([ordered]@{
            name = $name
            present = [bool]$hasValue
            value = $value
            enabled = [bool]$enabled
        }) | Out-Null
    }

    return [ordered]@{
        cmake_cache_path = $Path
        required_options = @($Names)
        options = @($options)
        missing_options = @($missingOptions)
        disabled_options = @($disabledOptions)
    }
}

function Get-LocalMemorySnapshot {
    try {
        $os = Get-CimInstance Win32_OperatingSystem -ErrorAction Stop
        return [pscustomobject]@{
            available = $true
            total_mb = [math]::Round(([double]$os.TotalVisibleMemorySize) / 1024, 1)
            free_mb = [math]::Round(([double]$os.FreePhysicalMemory) / 1024, 1)
            error_message = ""
        }
    } catch {
        return [pscustomobject]@{
            available = $false
            total_mb = $null
            free_mb = $null
            error_message = $_.Exception.Message
        }
    }
}
