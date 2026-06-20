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

function ConvertTo-TemplateSchemaPortableRelativePath {
    param(
        [string]$BasePath,
        [string]$TargetPath
    )

    if ([string]::IsNullOrWhiteSpace($TargetPath)) {
        return ""
    }

    $resolvedBasePath = [System.IO.Path]::GetFullPath($BasePath)
    if (-not $resolvedBasePath.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $resolvedBasePath.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $resolvedBasePath += [System.IO.Path]::DirectorySeparatorChar
    }
    $resolvedTargetPath = [System.IO.Path]::GetFullPath($TargetPath)

    if ([System.IO.Path]::GetPathRoot($resolvedBasePath) -ne
        [System.IO.Path]::GetPathRoot($resolvedTargetPath)) {
        return $resolvedTargetPath.Replace('\', '/')
    }

    $baseUri = New-Object System.Uri($resolvedBasePath)
    $targetUri = New-Object System.Uri($resolvedTargetPath)
    return [System.Uri]::UnescapeDataString(
        $baseUri.MakeRelativeUri($targetUri).ToString()
    ).Replace('\', '/')
}

function ConvertTo-TemplateSchemaCommandLine {
    param([object[]]$Arguments)

    $escaped = foreach ($argument in @($Arguments)) {
        if ($null -eq $argument) {
            continue
        }

        $text = [string]$argument
        if ($text -match '^[A-Za-z0-9_./:\\=-]+$') {
            $text
        } else {
            '"' + ($text -replace '"', '\"') + '"'
        }
    }

    return ($escaped -join ' ')
}

function ConvertTo-TemplateSchemaProcessArgumentString {
    param([object[]]$Arguments)

    $escaped = foreach ($argument in @($Arguments)) {
        if ($null -eq $argument) {
            continue
        }

        $text = [string]$argument
        if ($text.Length -eq 0) {
            '""'
            continue
        }

        if ($text -notmatch '[\s"]') {
            $text
            continue
        }

        $quoted = $text -replace '(\\*)"', '$1$1\"'
        $quoted = $quoted -replace '(\\+)$', '$1$1'
        '"' + $quoted + '"'
    }

    return ($escaped -join ' ')
}

function Expand-TemplateSchemaArgumentList {
    param([string[]]$Values)

    return @(
        foreach ($value in @($Values)) {
            if ([string]::IsNullOrWhiteSpace($value)) { continue }
            foreach ($part in ([string]$value -split ",")) {
                if (-not [string]::IsNullOrWhiteSpace($part)) { $part.Trim() }
            }
        }
    )
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

function Find-TemplateSchemaBinaryByName {
    param(
        [string]$SearchRoot,
        [string[]]$Names
    )

    if ([string]::IsNullOrWhiteSpace($SearchRoot) -or
        -not (Test-Path -LiteralPath $SearchRoot)) {
        return $null
    }

    $lookup = @{}
    foreach ($name in @($Names)) {
        if (-not [string]::IsNullOrWhiteSpace($name)) {
            $lookup[[string]$name] = $true
        }
    }
    if ($lookup.Count -eq 0) {
        return $null
    }

    $directSearchRoots = @(
        $SearchRoot,
        (Join-Path $SearchRoot "Debug"),
        (Join-Path $SearchRoot "Release"),
        (Join-Path $SearchRoot "RelWithDebInfo"),
        (Join-Path $SearchRoot "MinSizeRel"),
        (Join-Path $SearchRoot "bin")
    )

    foreach ($root in $directSearchRoots) {
        if ([string]::IsNullOrWhiteSpace($root) -or
            -not (Test-Path -LiteralPath $root -PathType Container)) {
            continue
        }

        foreach ($name in @($Names)) {
            if ([string]::IsNullOrWhiteSpace($name)) {
                continue
            }

            $candidatePath = Join-Path $root $name
            if (Test-Path -LiteralPath $candidatePath -PathType Leaf) {
                return [System.IO.Path]::GetFullPath($candidatePath)
            }
        }
    }

    $candidates = Get-ChildItem -Path $SearchRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $lookup.ContainsKey($_.Name) } |
        Sort-Object LastWriteTimeUtc -Descending

    if (-not $candidates) {
        return $null
    }

    return $candidates[0].FullName
}

function Find-TemplateSchemaCliBinary {
    param([string]$SearchRoot)

    return Find-TemplateSchemaBinaryByName `
        -SearchRoot $SearchRoot `
        -Names @("featherdoc_cli.exe", "featherdoc_cli")
}

function Find-TemplateSchemaTargetBinary {
    param(
        [string]$SearchRoot,
        [string]$TargetName
    )

    if ([string]::IsNullOrWhiteSpace($TargetName)) {
        return $null
    }

    return Find-TemplateSchemaBinaryByName `
        -SearchRoot $SearchRoot `
        -Names @("$TargetName.exe", $TargetName)
}

function Get-TemplateSchemaBuildSearchRoots {
    param(
        [string]$RepoRoot,
        [string]$PreferredBuildRoot
    )

    $candidateRoots = New-Object 'System.Collections.Generic.List[string]'

    if (-not [string]::IsNullOrWhiteSpace($PreferredBuildRoot) -and
        (Test-Path -LiteralPath $PreferredBuildRoot -PathType Container)) {
        $candidateRoots.Add([System.IO.Path]::GetFullPath($PreferredBuildRoot)) | Out-Null
    }

    if (-not [string]::IsNullOrWhiteSpace($RepoRoot) -and
        (Test-Path -LiteralPath $RepoRoot -PathType Container)) {
        foreach ($directory in @(
                Get-ChildItem -Path $RepoRoot -Directory -Filter "build*" -ErrorAction SilentlyContinue |
                    Sort-Object LastWriteTimeUtc -Descending
            )) {
            $candidateRoots.Add($directory.FullName) | Out-Null
        }
    }

    return @(
        $candidateRoots |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
            Select-Object -Unique
    )
}

function Find-TemplateSchemaCliBinaryInBuildRoots {
    param(
        [string]$RepoRoot,
        [string]$PreferredBuildRoot
    )

    foreach ($root in @(Get-TemplateSchemaBuildSearchRoots -RepoRoot $RepoRoot -PreferredBuildRoot $PreferredBuildRoot)) {
        $binaryPath = Find-TemplateSchemaCliBinary -SearchRoot $root
        if ($binaryPath) {
            return [pscustomobject]@{
                SearchRoot = [System.IO.Path]::GetFullPath($root)
                BinaryPath = [System.IO.Path]::GetFullPath($binaryPath)
            }
        }
    }

    return $null
}

function Find-TemplateSchemaTargetBinaryInBuildRoots {
    param(
        [string]$RepoRoot,
        [string]$PreferredBuildRoot,
        [string]$TargetName
    )

    if ([string]::IsNullOrWhiteSpace($TargetName)) {
        return $null
    }

    foreach ($root in @(Get-TemplateSchemaBuildSearchRoots -RepoRoot $RepoRoot -PreferredBuildRoot $PreferredBuildRoot)) {
        $binaryPath = Find-TemplateSchemaTargetBinary -SearchRoot $root -TargetName $TargetName
        if ($binaryPath) {
            return [pscustomobject]@{
                SearchRoot = [System.IO.Path]::GetFullPath($root)
                BinaryPath = [System.IO.Path]::GetFullPath($binaryPath)
            }
        }
    }

    return $null
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

    $fallbackBinary = Find-TemplateSchemaCliBinaryInBuildRoots `
        -RepoRoot $RepoRoot `
        -PreferredBuildRoot $resolvedBuildDir
    if ($null -ne $fallbackBinary) {
        return $fallbackBinary.BinaryPath
    }

    throw "Could not find featherdoc_cli under $resolvedBuildDir or any build* directory under $RepoRoot."
}

function Invoke-TemplateSchemaCli {
    param(
        [string]$ExecutablePath,
        [string[]]$Arguments
    )

    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processArguments = @($Arguments)
    if ([System.IO.Path]::GetExtension($ExecutablePath) -ieq ".ps1") {
        $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Could not find powershell.exe or pwsh to invoke PowerShell CLI script $ExecutablePath."
        }

        $startInfo.FileName = $powerShellCommand.Source
        $processArguments = @(
            "-NoProfile",
            "-NonInteractive",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            $ExecutablePath
        ) + $processArguments
    } else {
        $startInfo.FileName = $ExecutablePath
    }
    $startInfo.Arguments = ConvertTo-TemplateSchemaProcessArgumentString -Arguments $processArguments
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    $startInfo.StandardOutputEncoding = $utf8NoBom
    $startInfo.StandardErrorEncoding = $utf8NoBom

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo

    [void]$process.Start()
    $standardOutput = $process.StandardOutput.ReadToEnd()
    $standardError = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    $exitCode = $process.ExitCode
    $lines = @()
    foreach ($streamText in @($standardOutput, $standardError)) {
        if ([string]::IsNullOrEmpty($streamText)) {
            continue
        }

        $lines += @(
            $streamText -split "`r`n|`n|`r" |
                Where-Object { $_ -ne "" }
        )
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = $lines
        Text = ($lines -join [System.Environment]::NewLine)
    }
}

function Get-TemplateSchemaCommandJsonLine {
    param(
        [string[]]$Lines,
        [string]$Command
    )

    if ([string]::IsNullOrWhiteSpace($Command)) {
        throw "Template schema command name must not be empty."
    }

    $pattern = '^\s*\{\s*"command"\s*:\s*"' + [regex]::Escape($Command) + '"\s*,'
    $jsonLine = $Lines |
        Where-Object { $_ -match $pattern } |
        Select-Object -Last 1

    if ([string]::IsNullOrWhiteSpace([string]$jsonLine)) {
        throw "Template schema command '$Command' did not emit a JSON result."
    }

    return [string]$jsonLine
}

function Get-TemplateSchemaCommandJsonObject {
    param(
        [string[]]$Lines,
        [string]$Command
    )

    $jsonLine = Get-TemplateSchemaCommandJsonLine -Lines $Lines -Command $Command
    try {
        return $jsonLine | ConvertFrom-Json
    } catch {
        throw "Template schema command '$Command' emitted invalid JSON: $jsonLine"
    }
}
