<#
.SYNOPSIS
Resolves a validated Python interpreter for FeatherDoc development scripts.

.DESCRIPTION
Keeps Word visual and release scripts independent from a single executable name.
An explicit FEATHERDOC_PYTHON_EXECUTABLE value is authoritative. Otherwise the
resolver checks python, python3, and the Windows py launcher in that order and
executes --version before accepting a candidate.
#>

function Resolve-FeatherDocPythonCommandPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CommandOrPath
    )

    if (Test-Path -LiteralPath $CommandOrPath -PathType Leaf) {
        return [System.IO.Path]::GetFullPath($CommandOrPath)
    }

    $command = Get-Command -Name $CommandOrPath -ErrorAction SilentlyContinue |
        Where-Object {
            $_.CommandType -eq [System.Management.Automation.CommandTypes]::Application -or
            $_.CommandType -eq [System.Management.Automation.CommandTypes]::ExternalScript
        } |
        Select-Object -First 1
    if ($null -eq $command) {
        return $null
    }

    return $command.Source
}

function Test-FeatherDocPythonExecutable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Executable
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $versionOutput = @(& $Executable --version 2>&1)
        return ($? -and (($versionOutput -join " ") -match '^Python\s+\d+\.\d+'))
    } catch {
        return $false
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
}

function Resolve-FeatherDocPythonExecutable {
    [CmdletBinding()]
    param()

    $requestedPython = [string]$env:FEATHERDOC_PYTHON_EXECUTABLE
    if (-not [string]::IsNullOrWhiteSpace($requestedPython)) {
        $requestedPath = Resolve-FeatherDocPythonCommandPath -CommandOrPath $requestedPython
        if ([string]::IsNullOrWhiteSpace([string]$requestedPath) -or
            -not (Test-FeatherDocPythonExecutable -Executable $requestedPath)) {
            throw "FEATHERDOC_PYTHON_EXECUTABLE does not identify a working Python interpreter: $requestedPython"
        }

        return $requestedPath
    }

    foreach ($commandName in @("python", "python3", "py")) {
        $candidate = Resolve-FeatherDocPythonCommandPath -CommandOrPath $commandName
        if (-not [string]::IsNullOrWhiteSpace([string]$candidate) -and
            (Test-FeatherDocPythonExecutable -Executable $candidate)) {
            return $candidate
        }
    }

    throw "Python was not found. Install Python, make python/python3/py available in PATH, or set FEATHERDOC_PYTHON_EXECUTABLE."
}
