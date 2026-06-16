function Expand-InputPathList {
    param([string[]]$Paths)

    return @(
        foreach ($path in @($Paths)) {
            foreach ($part in ([string]$path -split ",")) {
                $trimmed = $part.Trim()
                if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
                    $trimmed
                }
            }
        }
    )
}

function Get-ReportKind {
    param($Summary)

    $schema = Get-JsonString -Object $Summary -Name "schema"
    switch ($schema) {
        "featherdoc.numbering_catalog_governance_report.v1" { return "numbering_catalog_governance" }
        "featherdoc.table_layout_delivery_governance_report.v1" { return "table_layout_delivery_governance" }
        "featherdoc.content_control_data_binding_governance_report.v1" { return "content_control_data_binding_governance" }
        "featherdoc.project_template_delivery_readiness_report.v1" { return "project_template_delivery_readiness" }
        "featherdoc.schema_patch_confidence_calibration_report.v1" { return "schema_patch_confidence_calibration" }
        "featherdoc.docx_functional_smoke_readiness.v1" { return "docx_functional_smoke_readiness" }
        default { return $schema }
    }
}

function Test-ProjectTemplateDeliveryReadinessReportEntry {
    param([object]$Report)

    $id = Get-JsonString -Object $Report -Name "id"
    $schema = Get-JsonString -Object $Report -Name "schema"
    return (
        [string]::Equals($id, "project_template_delivery_readiness", [System.StringComparison]::OrdinalIgnoreCase) -or
        [string]::Equals($schema, "featherdoc.project_template_delivery_readiness_report.v1", [System.StringComparison]::OrdinalIgnoreCase)
    )
}

function New-ProjectTemplateOnboardingGovernanceContract {
    param(
        [string]$RepoRoot,
        [string]$Path,
        [object]$Json
    )

    if ($null -eq $Json) {
        return $null
    }

    $schema = Get-JsonString -Object $Json -Name "schema"
    if (-not [string]::Equals($schema, "featherdoc.project_template_onboarding_governance_report.v1", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $null
    }

    return [ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = Get-JsonString -Object $Json -Name "status"
        release_ready = if ($null -ne (Get-JsonProperty -Object $Json -Name "release_ready")) { [string](Get-JsonBool -Object $Json -Name "release_ready") } else { "" }
        schema_approval_status_summary = @(Get-JsonArray -Object $Json -Name "schema_approval_status_summary")
        next_action = Get-JsonProperty -Object $Json -Name "next_action"
        next_action_summary = @(Get-JsonArray -Object $Json -Name "next_action_summary")
        next_action_group_count = Get-JsonInt -Object $Json -Name "next_action_group_count" -DefaultValue (@(Get-JsonArray -Object $Json -Name "next_action_summary").Count)
        source_report = $Path
        source_report_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        source_json = $Path
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
    }
}

function Get-ProjectTemplateOnboardingGovernanceContract {
    param(
        [string]$RepoRoot,
        [string]$InputRoot,
        [string[]]$InputJson
    )

    $candidatePaths = New-Object 'System.Collections.Generic.List[string]'
    $candidatePaths.Add((Join-Path $InputRoot "project-template-onboarding-governance\summary.json")) | Out-Null
    foreach ($input in @(Expand-InputPathList -Paths $InputJson)) {
        if ([string]::IsNullOrWhiteSpace($input)) {
            continue
        }
        $candidatePaths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $input -AllowMissing)) | Out-Null
    }

    foreach ($candidatePath in @($candidatePaths.ToArray())) {
        if (-not (Test-Path -LiteralPath $candidatePath)) {
            continue
        }
        try {
            $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidatePath | ConvertFrom-Json
            $contract = New-ProjectTemplateOnboardingGovernanceContract -RepoRoot $RepoRoot -Path $candidatePath -Json $json
            if ($null -ne $contract) {
                return $contract
            }
        } catch {
            continue
        }
    }

    return $null
}

function New-ExpectedReport {
    param(
        [string]$Id,
        [string]$Title,
        [string]$RelativeSummary,
        [string]$BuildCommand
    )

    return [ordered]@{
        id = $Id
        title = $Title
        relative_summary = $RelativeSummary
        build_command = $BuildCommand
    }
}

function Invoke-ReleaseBlockerRollup {
    param(
        [string]$RepoRoot,
        [string]$OutputDir,
        [string[]]$InputJson
    )

    if (@($InputJson).Count -eq 0) {
        throw "Release blocker rollup requires at least one loaded governance report."
    }

    $scriptPath = Join-Path $RepoRoot "scripts\build_release_blocker_rollup_report.ps1"
    $arguments = @(
        "-InputJson"
        (@($InputJson) -join ",")
        "-OutputDir"
        $OutputDir
    )
    $commandOutput = @(& (Get-Process -Id $PID).Path -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath @arguments 2>&1)
    if ($LASTEXITCODE -ne 0) {
        $errorText = (@($commandOutput | ForEach-Object { $_.ToString() }) -join [System.Environment]::NewLine)
        throw "Failed to build release blocker rollup. Output: $errorText"
    }
}
