function Write-Step {
    param([string]$Message)
    Write-Host "[release-candidate-checks] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-FullPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    $candidate = if ([System.IO.Path]::IsPathRooted($InputPath)) {
        $InputPath
    } else {
        Join-Path $RepoRoot $InputPath
    }

    return [System.IO.Path]::GetFullPath($candidate)
}

function Get-RepoRelativePath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return "(not available)"
    }

    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $RepoRoot $Path
    }
    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot)
    $resolvedPath = [System.IO.Path]::GetFullPath($candidate)
    if ($resolvedPath.StartsWith($resolvedRepoRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $resolvedPath.Substring($resolvedRepoRoot.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }

        return ".\" + ($relative -replace '/', '\')
    }

    return $resolvedPath
}

function Convert-ReleaseMaterialString {
    param(
        [string]$RepoRoot,
        [AllowNull()][string]$Text
    )

    if ($null -eq $Text) {
        return $null
    }

    if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
        return $Text
    }

    $normalized = [string]$Text
    $resolvedRepoRoot = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $repoRootForms = @(
        $resolvedRepoRoot,
        ($resolvedRepoRoot -replace '\\', '/'),
        ($resolvedRepoRoot -replace '\\', '\\')
    ) | Sort-Object -Unique
    $publicAnchors = @(
        "\release-candidate-checks-source\",
        "\release-candidate-checks\",
        "\release-governance-handoff-input\",
        "\release-governance-handoff\",
        "\release-blocker-rollup\",
        "\pdf-bounded-ctest\",
        "\pdf-summary\",
        "\word-visual-gate\",
        "\visual-tasks\",
        "\consumer-build\",
        "\output\",
        "\build\",
        "\install\"
    )

    foreach ($rootForm in $repoRootForms) {
        if ([string]::IsNullOrWhiteSpace($rootForm)) {
            continue
        }

        $pattern = [regex]::Escape($rootForm) + '(?<suffix>\\\\|[\\/]|(?=$|[\s"''`<>|;,)]+))'
        $normalized = [regex]::Replace(
            $normalized,
            $pattern,
            '.${suffix}',
            [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    }

    $convertAbsolutePath = {
        param([string]$Path)

        if ([string]::IsNullOrWhiteSpace($Path)) {
            return ".\external"
        }

        $displayPath = ([string]$Path).Trim().TrimEnd('\', '/')
        $normalizedPath = $displayPath -replace '/', '\'

        foreach ($anchor in $publicAnchors) {
            $index = $normalizedPath.IndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
            if ($index -ge 0) {
                return ".\" + $normalizedPath.Substring($index + 1)
            }
        }

        $segments = @($normalizedPath -split '[\\/]' | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($segments.Count -eq 0) {
            return ".\external"
        }

        $suffixSegmentCount = [Math]::Min(2, $segments.Count)
        $startIndex = $segments.Count - $suffixSegmentCount
        $suffix = @($segments[$startIndex..($segments.Count - 1)]) -join '\'
        return ".\external\$suffix"
    }
    $replaceAbsolutePaths = {
        param(
            [string]$InputText,
            [string]$Pattern
        )

        $convertedText = [string]$InputText
        foreach ($match in @([regex]::Matches($convertedText, $Pattern))) {
            $path = [string]$match.Groups["path"].Value
            if ([string]::IsNullOrWhiteSpace($path)) {
                continue
            }

            $convertedText = $convertedText.Replace($path, (& $convertAbsolutePath $path))
        }

        return $convertedText
    }

    $windowsAbsolutePathPattern = '(?i)(?<![\w<>])(?<path>[A-Z]:[\\/][^\s"''`<>|;,)]+)'
    $normalized = & $replaceAbsolutePaths $normalized $windowsAbsolutePathPattern

    $uncAbsolutePathPattern = '(?<![.\\/])(?<path>\\\\[^\\/\s"''`<>|;,)]+[\\/][^\s"''`<>|;,)]+)'
    $normalized = & $replaceAbsolutePaths $normalized $uncAbsolutePathPattern

    $unixAbsolutePathPattern = '(?<![\w.])(?<path>/(?:Users|home|tmp)/[^\s"''`<>|;,)]+)'
    $normalized = & $replaceAbsolutePaths $normalized $unixAbsolutePathPattern

    $relativePublicPathPattern = '(?<path>\.[\\/][^\s"''`<>|;,)]+)'
    foreach ($match in @([regex]::Matches($normalized, $relativePublicPathPattern))) {
        $path = [string]$match.Groups["path"].Value
        $normalizedPath = $path -replace '/', '\'
        foreach ($anchor in $publicAnchors) {
            $index = $normalizedPath.IndexOf($anchor, [System.StringComparison]::OrdinalIgnoreCase)
            if ($index -ge 0) {
                $normalized = $normalized.Replace($path, ".\" + $normalizedPath.Substring($index + 1))
                break
            }
        }
    }

    return $normalized.Replace('./', '.\')
}

function Convert-ReleaseMaterialObject {
    param(
        [string]$RepoRoot,
        [AllowNull()]$Value
    )

    if ($null -eq $Value) {
        return $null
    }

    if ($Value -is [string]) {
        return Convert-ReleaseMaterialString -RepoRoot $RepoRoot -Text $Value
    }

    if ($Value -is [System.Collections.IDictionary]) {
        $converted = [ordered]@{}
        foreach ($key in $Value.Keys) {
            $converted[$key] = Convert-ReleaseMaterialObject -RepoRoot $RepoRoot -Value $Value[$key]
        }

        return $converted
    }

    if ($Value -is [pscustomobject]) {
        $converted = [ordered]@{}
        foreach ($property in $Value.PSObject.Properties) {
            $converted[$property.Name] = Convert-ReleaseMaterialObject -RepoRoot $RepoRoot -Value $property.Value
        }

        return $converted
    }

    if ($Value -is [System.Collections.IEnumerable]) {
        return ,@(
            foreach ($item in $Value) {
                Convert-ReleaseMaterialObject -RepoRoot $RepoRoot -Value $item
            }
        )
    }

    return $Value
}

function Convert-ReleaseMaterialFile {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return
    }

    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
    if ([System.IO.Path]::GetExtension($Path) -eq ".json") {
        $json = $content | ConvertFrom-Json
        $convertedJson = Convert-ReleaseMaterialObject -RepoRoot $RepoRoot -Value $json
        $convertedJson | ConvertTo-Json -Depth 12 | Set-Content -Encoding UTF8 -LiteralPath $Path
        return
    }

    $converted = Convert-ReleaseMaterialString -RepoRoot $RepoRoot -Text $content
    if ($converted -ne $content) {
        $converted | Set-Content -Encoding UTF8 -LiteralPath $Path
    }
}

function Get-ProjectVersion {
    param([string]$RepoRoot)

    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $cmakePath)) {
        return ""
    }

    $content = Get-Content -Raw -LiteralPath $cmakePath
    $match = [regex]::Match($content, 'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)')
    if (-not $match.Success) {
        return ""
    }

    return $match.Groups[1].Value
}

function Get-MsvcBootstrap {
    $existingCl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($env:VSCMD_VER -or $env:VCINSTALLDIR -or $existingCl) {
        return [ordered]@{
            mode = "current_env"
            vcvars_path = ""
        }
    }

    $vswherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswherePath) {
        $installationPath = & $vswherePath `
            -latest `
            -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($installationPath)) {
            $vcvarsPath = Join-Path $installationPath.Trim() "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $vcvarsPath) {
                return [ordered]@{
                    mode = "vcvars"
                    vcvars_path = $vcvarsPath
                }
            }
        }
    }

    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($path in $candidates) {
        if (Test-Path $path) {
            return [ordered]@{
                mode = "vcvars"
                vcvars_path = $path
            }
        }
    }

    throw "Could not locate vcvars64.bat and no active MSVC developer environment was detected."
}

function Invoke-MsvcCommand {
    param(
        $MsvcBootstrap,
        [string]$CommandText
    )

    # Keep native CLI JSON output decodable when CTest launches PowerShell
    # scripts from the MSVC cmd environment.
    $utf8CommandText = "chcp 65001 >NUL && $CommandText"
    if ($MsvcBootstrap.mode -eq "current_env") {
        & cmd.exe /c $utf8CommandText
    } else {
        & cmd.exe /c "call `"$($MsvcBootstrap.vcvars_path)`" && $utf8CommandText"
    }

    if ($LASTEXITCODE -ne 0) {
        throw "MSVC command failed: $CommandText"
    }
}

function Invoke-ChildPowerShell {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    $powerShellPath = (Get-Process -Id $PID).Path
    if ([string]::IsNullOrWhiteSpace($powerShellPath)) {
        $powerShellCommand = Get-Command pwsh -ErrorAction SilentlyContinue
        if (-not $powerShellCommand) {
            $powerShellCommand = Get-Command powershell.exe -ErrorAction SilentlyContinue
        }
        if (-not $powerShellCommand) {
            throw "Unable to resolve a PowerShell executable for the nested script invocation."
        }
        $powerShellPath = $powerShellCommand.Source
    }

    $commandOutput = @(& $powerShellPath -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $ScriptPath @Arguments 2>&1)
    $exitCode = $LASTEXITCODE

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }

    return @($commandOutput | ForEach-Object { $_.ToString() })
}

function Convert-ToCmdArgument {
    param([string]$Value)

    if ($Value -notmatch '[\s"]') {
        return $Value
    }

    return "`"$Value`""
}

function Invoke-ChildPowerShellInMsvcEnv {
    param(
        $MsvcBootstrap,
        [string]$ScriptPath,
        [string[]]$Arguments,
        [string]$FailureMessage
    )

    if ($MsvcBootstrap.mode -eq "current_env") {
        return Invoke-ChildPowerShell -ScriptPath $ScriptPath `
            -Arguments $Arguments `
            -FailureMessage $FailureMessage
    }

    $commandParts = @(
        "call"
        (Convert-ToCmdArgument -Value $MsvcBootstrap.vcvars_path)
        "&&"
        "powershell.exe"
        "-ExecutionPolicy"
        "Bypass"
        "-File"
        (Convert-ToCmdArgument -Value $ScriptPath)
    ) + ($Arguments | ForEach-Object { Convert-ToCmdArgument -Value $_ })

    $commandText = $commandParts -join " "
    $commandOutput = @(& cmd.exe /c $commandText 2>&1)
    $exitCode = $LASTEXITCODE

    foreach ($line in $commandOutput) {
        Write-Host $line
    }

    if ($exitCode -ne 0) {
        throw $FailureMessage
    }

    return @($commandOutput | ForEach-Object { $_.ToString() })
}

function Assert-PathExists {
    param(
        [string]$Path,
        [string]$Label
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path $Path)) {
        throw "Expected $Label was not found: $Path"
    }
}

function Assert-BuildExecutablePresent {
    param(
        [string]$BuildRoot,
        [string]$ExecutableStem,
        [string]$Label
    )

    $candidate = Get-ChildItem -Path $BuildRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object {
            $_.Name -ieq $ExecutableStem -or $_.Name -ieq "$ExecutableStem.exe"
        } |
        Select-Object -First 1

    if (-not $candidate) {
        throw "Build directory is missing $Label ($ExecutableStem). Re-run without -SkipBuild or point to a fully built MSVC tree."
    }

    return $candidate.FullName
}

function Get-OptionalPropertyValue {
    param(
        [AllowNull()]$Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary] -and $Object.Contains($Name)) {
        return $Object[$Name]
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}


function Set-OptionalSummaryValue {
    param(
        [System.Collections.IDictionary]$Target,
        [string]$Name,
        $Value
    )

    if ($null -ne $Value -and -not [string]::IsNullOrWhiteSpace([string]$Value)) {
        $Target[$Name] = $Value
    }
}
