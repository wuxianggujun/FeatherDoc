<#
.SYNOPSIS
Runs a guarded FeatherDoc build/test pass that reuses the canonical local build.

.DESCRIPTION
This script is the preferred local entry point for quick validation. It refuses
to start when another FeatherDoc build/test process is already running, keeps a
workspace lock while it owns the build, and reuses build-msvc-nmake by default.

It does not create a new build directory unless -ConfigureIfMissing is passed.
Tests are run through ctest with a per-test timeout so stale test processes do
not accumulate.

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\run_reused_build_check.ps1 `
    -Targets featherdoc_cli,cli_tests `
    -TestRegex "^cli$"

.EXAMPLE
pwsh -ExecutionPolicy Bypass -File .\scripts\run_reused_build_check.ps1 `
    -ListProcessesOnly
#>
param(
    [string]$BuildDir = "build-msvc-nmake",
    [string[]]$Targets = @("featherdoc_cli"),
    [string]$TestRegex = "",
    [int]$BuildTimeoutSeconds = 900,
    [int]$TestTimeoutSeconds = 60,
    [switch]$SkipBuild,
    [switch]$SkipTests,
    [switch]$ConfigureIfMissing,
    [switch]$SkipProcessGuard,
    [switch]$ListProcessesOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "[run-reused-build-check] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param(
        [string]$RepoRoot,
        [string]$Path
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $RepoRoot $Path))
}

function Get-VcvarsPath {
    $candidates = @(
        "D:\Program Files\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )

    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "Could not locate vcvars64.bat for MSVC command-line builds."
}

function Get-ChildProcessTree {
    param([int]$RootProcessId)

    $all = Get-CimInstance Win32_Process |
        Select-Object ProcessId, ParentProcessId, Name, CommandLine
    $ids = @($RootProcessId)

    for ($i = 0; $i -lt 8; $i++) {
        $children = @(
            $all |
                Where-Object { $ids -contains [int]$_.ParentProcessId } |
                Select-Object -ExpandProperty ProcessId
        )
        if ($children.Count -eq 0) {
            break
        }

        $ids += $children
        $ids = @($ids | Sort-Object -Unique)
    }

    return @($all | Where-Object { $ids -contains [int]$_.ProcessId })
}

function Stop-OwnedProcessTree {
    param([int]$RootProcessId)

    $tree = @(Get-ChildProcessTree -RootProcessId $RootProcessId |
        Sort-Object ProcessId -Descending)
    foreach ($process in $tree) {
        try {
            Stop-Process -Id ([int]$process.ProcessId) -Force -ErrorAction Stop
        } catch {
            Write-Warning "Failed to stop process $($process.ProcessId): $($_.Exception.Message)"
        }
    }
}

function Invoke-CommandWithTimeout {
    param(
        [string]$CommandText,
        [string]$WorkingDirectory,
        [int]$TimeoutSeconds,
        [string]$Label
    )

    $logRoot = Join-Path $repoRoot ".codex-temp\reused-build-check"
    New-Item -ItemType Directory -Force -Path $logRoot | Out-Null
    $safeLabel = ($Label -replace '[^A-Za-z0-9_.-]', '_')
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $stdoutPath = Join-Path $logRoot "$stamp-$safeLabel.stdout.log"
    $stderrPath = Join-Path $logRoot "$stamp-$safeLabel.stderr.log"

    Write-Step "$Label"
    Write-Host "  command: $CommandText"
    Write-Host "  timeout: ${TimeoutSeconds}s"

    $process = Start-Process `
        -FilePath "cmd.exe" `
        -ArgumentList @("/d", "/s", "/c", $CommandText) `
        -WorkingDirectory $WorkingDirectory `
        -NoNewWindow `
        -PassThru `
        -RedirectStandardOutput $stdoutPath `
        -RedirectStandardError $stderrPath

    $completed = $process.WaitForExit($TimeoutSeconds * 1000)
    if (-not $completed) {
        Stop-OwnedProcessTree -RootProcessId $process.Id
        if (Test-Path -LiteralPath $stdoutPath) {
            Get-Content -LiteralPath $stdoutPath -Tail 80
        }
        if (Test-Path -LiteralPath $stderrPath) {
            Get-Content -LiteralPath $stderrPath -Tail 80
        }
        throw "$Label timed out after ${TimeoutSeconds}s. Stopped owned process tree rooted at PID $($process.Id)."
    }

    $process.Refresh()
    $exitCode = $process.ExitCode
    if (Test-Path -LiteralPath $stdoutPath) {
        Get-Content -LiteralPath $stdoutPath -Tail 120
    }
    if (Test-Path -LiteralPath $stderrPath) {
        Get-Content -LiteralPath $stderrPath -Tail 120
    }
    if ($exitCode -ne 0) {
        throw "$Label failed with exit code $exitCode."
    }
}

function Normalize-BuildTargets {
    param([string[]]$InputTargets)

    return @(
        $InputTargets |
            ForEach-Object { [string]$_ -split "," } |
            ForEach-Object { $_.Trim() } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    )
}

function Get-FeatherDocProcesses {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir,
        [int[]]$ExcludeProcessIds
    )

    $repoPattern = [regex]::Escape($RepoRoot)
    $buildPattern = [regex]::Escape($ResolvedBuildDir)
    $names = @(
        "pwsh.exe",
        "powershell.exe",
        "cmake.exe",
        "ctest.exe",
        "nmake.exe",
        "cl.exe",
        "link.exe",
        "msbuild.exe",
        "WINWORD.EXE"
    )

    return @(
        Get-CimInstance Win32_Process |
            Where-Object {
                $names -contains $_.Name -and
                -not ($ExcludeProcessIds -contains [int]$_.ProcessId) -and
                (
                    ([string]$_.CommandLine -match $repoPattern) -or
                    ([string]$_.CommandLine -match $buildPattern) -or
                    ([string]$_.CommandLine -match "word-visual-release-gate") -or
                    ([string]$_.CommandLine -match "word-visual-smoke") -or
                    ([string]$_.CommandLine -match "release-candidate-checks")
                )
            } |
            Select-Object ProcessId, ParentProcessId, Name, CreationDate, CommandLine
    )
}

function New-WorkspaceLock {
    param(
        [string]$RepoRoot,
        [string]$ResolvedBuildDir
    )

    $lockDir = Join-Path $RepoRoot ".codex-temp\featherdoc-build.lock"
    $lockInfoPath = Join-Path $lockDir "lock.json"
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $lockDir) | Out-Null

    if (Test-Path -LiteralPath $lockDir) {
        $existingPid = 0
        if (Test-Path -LiteralPath $lockInfoPath) {
            try {
                $existing = Get-Content -Raw -LiteralPath $lockInfoPath | ConvertFrom-Json
                $existingPid = [int]$existing.pid
            } catch {
                $existingPid = 0
            }
        }

        if ($existingPid -gt 0 -and (Get-Process -Id $existingPid -ErrorAction SilentlyContinue)) {
            throw "Build lock is already held by PID $existingPid. Refusing to start another FeatherDoc build."
        }

        Remove-Item -LiteralPath $lockDir -Recurse -Force
    }

    New-Item -ItemType Directory -Path $lockDir -ErrorAction Stop | Out-Null
    [pscustomobject]@{
        pid = $PID
        created_at = (Get-Date).ToString("s")
        repo = $RepoRoot
        build_dir = $ResolvedBuildDir
    } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $lockInfoPath -Encoding UTF8

    return $lockDir
}

$repoRoot = Resolve-RepoRoot
$resolvedBuildDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $BuildDir
$excludeIds = @($PID)

$runningProcesses = @(Get-FeatherDocProcesses `
    -RepoRoot $repoRoot `
    -ResolvedBuildDir $resolvedBuildDir `
    -ExcludeProcessIds $excludeIds)

if ($ListProcessesOnly) {
    if ($runningProcesses.Count -eq 0) {
        Write-Host "No FeatherDoc build/test processes found."
    } else {
        $runningProcesses | Sort-Object CreationDate | Format-List
    }
    exit 0
}

if (-not $SkipProcessGuard -and $runningProcesses.Count -gt 0) {
    $runningProcesses | Sort-Object CreationDate | Format-List
    throw "Refusing to start because FeatherDoc build/test processes are already running."
}

$lockDir = New-WorkspaceLock -RepoRoot $repoRoot -ResolvedBuildDir $resolvedBuildDir
try {
    $cachePath = Join-Path $resolvedBuildDir "CMakeCache.txt"
    $vcvarsPath = Get-VcvarsPath
    $vcvarsPrefix = "call `"$vcvarsPath`" x64 >NUL && chcp 65001 >NUL && "

    if (-not (Test-Path -LiteralPath $cachePath)) {
        if (-not $ConfigureIfMissing) {
            throw "Missing CMake cache at $cachePath. Refusing to create a new build dir without -ConfigureIfMissing."
        }

        $configureCommand = $vcvarsPrefix +
            "cmake -S `"$repoRoot`" -B `"$resolvedBuildDir`" -G `"NMake Makefiles`" " +
            "-DBUILD_SAMPLES=ON -DBUILD_CLI=ON -DBUILD_TESTING=ON"
        Invoke-CommandWithTimeout `
            -CommandText $configureCommand `
            -WorkingDirectory $repoRoot `
            -TimeoutSeconds $BuildTimeoutSeconds `
            -Label "configure $BuildDir"
    } else {
        Write-Step "Reusing existing build directory: $resolvedBuildDir"
    }

    if (-not $SkipBuild) {
        $normalizedTargets = Normalize-BuildTargets -InputTargets $Targets
        $targetText = $normalizedTargets -join " "
        if ([string]::IsNullOrWhiteSpace($targetText)) {
            throw "At least one build target is required unless -SkipBuild is used."
        }

        $buildCommand = $vcvarsPrefix + "cmake --build `"$resolvedBuildDir`" --target $targetText"
        Invoke-CommandWithTimeout `
            -CommandText $buildCommand `
            -WorkingDirectory $repoRoot `
            -TimeoutSeconds $BuildTimeoutSeconds `
            -Label "build targets: $targetText"
    }

    if (-not $SkipTests -and -not [string]::IsNullOrWhiteSpace($TestRegex)) {
        $testCommand = "ctest --test-dir `"$resolvedBuildDir`" -R `"$TestRegex`" --output-on-failure --timeout $TestTimeoutSeconds"
        Invoke-CommandWithTimeout `
            -CommandText $testCommand `
            -WorkingDirectory $repoRoot `
            -TimeoutSeconds ([Math]::Max($TestTimeoutSeconds * 4, 120)) `
            -Label "ctest: $TestRegex"
    } elseif (-not $SkipTests) {
        Write-Step "No -TestRegex provided; skipping tests."
    }

    Write-Step "Completed using reused build directory."
} finally {
    if (Test-Path -LiteralPath $lockDir) {
        Remove-Item -LiteralPath $lockDir -Recurse -Force
    }
}
