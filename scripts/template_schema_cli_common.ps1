function Resolve-TemplateSchemaRepoRoot {
    param([string]$ScriptRoot)

    return (Resolve-Path (Join-Path $ScriptRoot "..")).Path
}

function Resolve-TemplateSchemaPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath,
        [switch]$AllowMissing
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    if ($AllowMissing) {
        return [System.IO.Path]::GetFullPath($candidate)
    }

    return (Resolve-Path -LiteralPath $candidate).Path
}

function Get-TemplateSchemaVcvarsPath {
    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $candidates) {
        if (Test-Path -LiteralPath $path) {
            return $path
        }
    }

    throw "Could not locate vcvars64.bat for MSVC command-line builds."
}

function Invoke-TemplateSchemaMsvcCommand {
    param(
        [string]$VcvarsPath,
        [string]$CommandText
    )

    & cmd.exe /c "call `"$VcvarsPath`" && $CommandText"
    if ($LASTEXITCODE -ne 0) {
        throw "MSVC command failed: $CommandText"
    }
}

function Find-TemplateSchemaCliBinary {
    param([string]$SearchRoot)

    if ([string]::IsNullOrWhiteSpace($SearchRoot) -or
        -not (Test-Path -LiteralPath $SearchRoot)) {
        return $null
    }

    $candidates = Get-ChildItem -Path $SearchRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "featherdoc_cli.exe" -or $_.Name -ieq "featherdoc_cli" } |
        Sort-Object LastWriteTimeUtc -Descending

    if (-not $candidates) {
        return $null
    }

    return $candidates[0].FullName
}

function Find-TemplateSchemaTargetBinary {
    param(
        [string]$SearchRoot,
        [string]$TargetName
    )

    if ([string]::IsNullOrWhiteSpace($SearchRoot) -or
        -not (Test-Path -LiteralPath $SearchRoot)) {
        return $null
    }

    $candidates = Get-ChildItem -Path $SearchRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -ieq "$TargetName.exe" -or $_.Name -ieq $TargetName } |
        Sort-Object LastWriteTimeUtc -Descending

    if (-not $candidates) {
        return $null
    }

    return $candidates[0].FullName
}

function Resolve-TemplateSchemaCliPath {
    param(
        [string]$RepoRoot,
        [string]$BuildDir,
        [string]$Generator,
        [switch]$SkipBuild
    )

    $resolvedBuildDir = if ([string]::IsNullOrWhiteSpace($BuildDir)) {
        Join-Path $RepoRoot "build-template-schema-cli-nmake"
    } else {
        Resolve-TemplateSchemaPath -RepoRoot $RepoRoot -InputPath $BuildDir -AllowMissing
    }

    if (-not $SkipBuild) {
        $vcvarsPath = Get-TemplateSchemaVcvarsPath
        Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
            "cmake -S `"$RepoRoot`" -B `"$resolvedBuildDir`" -G `"$Generator`" " +
            "-DBUILD_CLI=ON -DBUILD_TESTING=OFF -DBUILD_SAMPLES=OFF")
        Invoke-TemplateSchemaMsvcCommand -VcvarsPath $vcvarsPath -CommandText (
            "cmake --build `"$resolvedBuildDir`" --target featherdoc_cli")
    }

    $binaryPath = Find-TemplateSchemaCliBinary -SearchRoot $resolvedBuildDir
    if ($binaryPath) {
        return $binaryPath
    }

    $fallbackBinary = Find-TemplateSchemaCliBinary -SearchRoot $RepoRoot
    if ($fallbackBinary) {
        return $fallbackBinary
    }

    throw "Could not find featherdoc_cli under $resolvedBuildDir or $RepoRoot."
}

function Invoke-TemplateSchemaCli {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments
    )

    $commandOutput = @(& $ExecutablePath @Arguments 2>&1)
    $exitCode = if ($null -eq $LASTEXITCODE) { 0 } else { $LASTEXITCODE }
    $lines = @($commandOutput | ForEach-Object { $_.ToString() })

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}
