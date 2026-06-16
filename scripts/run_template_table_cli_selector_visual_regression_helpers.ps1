function Write-Step {
    param([string]$Message)
    Write-Host "[template-table-cli-selector-visual] $Message"
}

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

function Get-VcvarsPath {
    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return $path
        }
    }

    throw "Could not locate vcvars64.bat for MSVC command-line builds."
}

function Invoke-MsvcCommand {
    param(
        [string]$VcvarsPath,
        [string]$CommandText
    )

    & cmd.exe /c "call `"$VcvarsPath`" && $CommandText"
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC command failed: $CommandText"
    }
}

function Find-BuildExecutable {
    param(
        [string]$BuildRoot,
        [string]$TargetName
    )

    $resolvedBuildRoot = (Resolve-Path $BuildRoot).Path
    $candidates = @(Get-ChildItem -Path $resolvedBuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName })
    if ($candidates.Count -eq 0) {
        throw "Could not find built executable for target '$TargetName' under $BuildRoot."
    }

    $rootCandidate = $candidates |
        Where-Object { $_.DirectoryName -eq $resolvedBuildRoot } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($rootCandidate) {
        return $rootCandidate.FullName
    }

    return ($candidates | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}

function Get-BasePython {
    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) {
        return $python.Source
    }

    throw "Python was not found in PATH."
}

function Test-PythonImport {
    param(
        [string]$PythonCommand,
        [string]$ModuleName
    )

    & $PythonCommand -c "import $ModuleName" 1> $null 2> $null
    return $LASTEXITCODE -eq 0
}

function Ensure-RenderPython {
    param([string]$RepoRoot)

    $basePython = Get-BasePython
    if (Test-PythonImport -PythonCommand $basePython -ModuleName "PIL") {
        return $basePython
    }

    $venvDir = Join-Path $RepoRoot ".venv-word-visual-smoke"
    $venvPython = Join-Path $venvDir "Scripts\python.exe"
    if (-not (Test-Path $venvPython)) {
        $null = & $basePython -m venv $venvDir
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to create local Python virtual environment."
        }
    }

    if (-not (Test-PythonImport -PythonCommand $venvPython -ModuleName "PIL")) {
        & $venvPython -m pip install --disable-pip-version-check pillow 2>&1 |
            ForEach-Object { Write-Host $_ }
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to install Pillow into the local environment."
        }
    }

    return $venvPython
}

function Invoke-Capture {
    param(
        [string]$Executable,
        [string[]]$Arguments,
        [string]$OutputPath,
        [string]$FailureMessage
    )

    $lines = @(& $Executable @Arguments 2>&1 | ForEach-Object { $_.ToString() })
    if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
        $lines | Set-Content -Path $OutputPath -Encoding UTF8
    }
    foreach ($line in $lines) {
        Write-Host $line
    }
    if ($LASTEXITCODE -ne 0) {
        throw $FailureMessage
    }
}

function Assert-Rows {
    param(
        [string]$JsonPath,
        [string[][]]$ExpectedRows,
        [string]$Label
    )

    $json = Get-Content -Raw -LiteralPath $JsonPath | ConvertFrom-Json
    $rows = @($json.rows)
    if ($rows.Count -ne $ExpectedRows.Count) {
        throw "$Label row count mismatch. Expected $($ExpectedRows.Count), got $($rows.Count)."
    }

    for ($i = 0; $i -lt $ExpectedRows.Count; $i++) {
        $actual = @($rows[$i].cell_texts | ForEach-Object { $_.ToString() })
        $expected = @($ExpectedRows[$i])
        if (($actual -join "`n") -ne ($expected -join "`n")) {
            throw "$Label row $i mismatch. Expected '$($expected -join " | ")', got '$($actual -join " | ")'."
        }
    }
}

function Assert-Cells {
    param(
        [string]$JsonPath,
        [object[]]$ExpectedCells,
        [string]$Label
    )

    $json = Get-Content -Raw -LiteralPath $JsonPath | ConvertFrom-Json
    $cells = @($json.cells)
    if ($cells.Count -ne $ExpectedCells.Count) {
        throw "$Label cell count mismatch. Expected $($ExpectedCells.Count), got $($cells.Count)."
    }

    for ($i = 0; $i -lt $ExpectedCells.Count; $i++) {
        foreach ($key in $ExpectedCells[$i].Keys) {
            if ($cells[$i].$key -ne $ExpectedCells[$i][$key]) {
                throw "$Label cell $i field '$key' mismatch. Expected '$($ExpectedCells[$i][$key])', got '$($cells[$i].$key)'."
            }
        }
    }
}

function Invoke-WordSmoke {
    param(
        [string]$ScriptPath,
        [string]$BuildDir,
        [string]$DocxPath,
        [string]$OutputDir,
        [int]$Dpi,
        [bool]$KeepWordOpen,
        [bool]$VisibleWord
    )

    & $ScriptPath `
        -BuildDir $BuildDir `
        -InputDocx $DocxPath `
        -OutputDir $OutputDir `
        -SkipBuild `
        -Dpi $Dpi `
        -KeepWordOpen:$KeepWordOpen `
        -VisibleWord:$VisibleWord
    if ($LASTEXITCODE -ne 0) {
        throw "Word visual smoke failed for $DocxPath."
    }
}

function Build-ContactSheet {
    param(
        [string]$Python,
        [string]$ScriptPath,
        [string]$OutputPath,
        [string[]]$Images,
        [string[]]$Labels
    )

    & $Python $ScriptPath --output $OutputPath --columns 2 --thumb-width 420 --images $Images --labels $Labels
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to build contact sheet at $OutputPath."
    }
}

function Register-VisualPair {
    param(
        [string]$CaseId,
        [string]$BaselinePage,
        [string]$MutatedPage
    )

    $baselineCopy = Join-Path $aggregateFirstPagesDir "$CaseId-baseline-page-01.png"
    $mutatedCopy = Join-Path $aggregateFirstPagesDir "$CaseId-mutated-page-01.png"
    $baselineSelectedCopy = Join-Path $aggregateSelectedPagesDir "$CaseId-baseline-page-01.png"
    $mutatedSelectedCopy = Join-Path $aggregateSelectedPagesDir "$CaseId-mutated-page-01.png"
    Copy-Item -Path $BaselinePage -Destination $baselineCopy -Force
    Copy-Item -Path $MutatedPage -Destination $mutatedCopy -Force
    Copy-Item -Path $BaselinePage -Destination $baselineSelectedCopy -Force
    Copy-Item -Path $MutatedPage -Destination $mutatedSelectedCopy -Force
    $script:aggregateImages += $baselineCopy
    $script:aggregateImages += $mutatedCopy
    $script:aggregateLabels += "$CaseId-baseline"
    $script:aggregateLabels += "$CaseId-mutated"
    $script:selectedPagesByCase[$CaseId] = @(
        [ordered]@{
            page_number = 1
            role = "primary"
            baseline_page = $baselineSelectedCopy
            mutated_page = $mutatedSelectedCopy
        }
    )
}

function Write-SummaryArtifacts {
    param(
        [string]$SummaryPath,
        [bool]$SkipVisual,
        [string]$Python,
        [string]$ScriptPath,
        [string]$AggregateContactSheetPath
    )

    if (-not $SkipVisual) {
        foreach ($case in $script:summary.cases) {
            if ($script:selectedPagesByCase.ContainsKey($case.id) -and $case.Contains("visual")) {
                $case.visual["selected_pages"] = $script:selectedPagesByCase[$case.id]
            }
        }
        Build-ContactSheet -Python $Python -ScriptPath $ScriptPath -OutputPath $AggregateContactSheetPath -Images $script:aggregateImages -Labels $script:aggregateLabels
        $script:summary.aggregate_evidence = [ordered]@{
            root = $script:aggregateEvidenceDir
            first_pages_dir = $script:aggregateFirstPagesDir
            selected_pages_dir = $script:aggregateSelectedPagesDir
            contact_sheet = $AggregateContactSheetPath
        }
    }

    ($script:summary | ConvertTo-Json -Depth 8) | Set-Content -Path $SummaryPath -Encoding UTF8
    Write-Step "Selector visual regression artifacts written to $script:resolvedOutputDir"
}
