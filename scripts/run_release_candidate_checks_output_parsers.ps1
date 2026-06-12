function Parse-InstallSmokeOutput {
    param([string[]]$Lines)

    $result = [ordered]@{}
    foreach ($line in $Lines) {
        if ($line -match '^Install prefix:\s*(.+)$') {
            $result.install_prefix = $Matches[1].Trim()
        } elseif ($line -match '^Consumer document:\s*(.+)$') {
            $result.consumer_document = $Matches[1].Trim()
        }
    }

    Assert-PathExists -Path $result.install_prefix -Label "install prefix"
    Assert-PathExists -Path $result.consumer_document -Label "install smoke consumer document"

    return $result
}

function Get-ReadmeGalleryInfoFromGateSummary {
    param([string]$GateSummaryPath)

    $result = [ordered]@{
        status = "unknown"
    }

    if ([string]::IsNullOrWhiteSpace($GateSummaryPath) -or
        -not (Test-Path -LiteralPath $GateSummaryPath)) {
        return $result
    }

    $gateSummary = Get-Content -Raw -LiteralPath $GateSummaryPath | ConvertFrom-Json
    $readmeGallery = Get-OptionalPropertyValue -Object $gateSummary -Name "readme_gallery"
    if ($null -eq $readmeGallery) {
        return $result
    }

    $status = Get-OptionalPropertyValue -Object $readmeGallery -Name "status"
    if (-not [string]::IsNullOrWhiteSpace($status)) {
        $result.status = $status
    }

    foreach ($name in @(
        "assets_dir",
        "visual_smoke_contact_sheet",
        "visual_smoke_page_05",
        "visual_smoke_page_06",
        "fixed_grid_aggregate_contact_sheet"
    )) {
        $value = Get-OptionalPropertyValue -Object $readmeGallery -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $result[$name] = $value
        }
    }

    return $result
}

function Parse-VisualGateOutput {
    param([string[]]$Lines)

    $result = [ordered]@{}
    foreach ($line in $Lines) {
        if ($line -match '^Gate summary:\s*(.+)$') {
            $result.gate_summary = $Matches[1].Trim()
        } elseif ($line -match '^Gate final review:\s*(.+)$') {
            $result.gate_final_review = $Matches[1].Trim()
        } elseif ($line -match '^Document task:\s*(.+)$') {
            $result.document_task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Fixed-grid task:\s*(.+)$') {
            $result.fixed_grid_task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Section page setup task:\s*(.+)$') {
            $result.section_page_setup_task_dir = $Matches[1].Trim()
        } elseif ($line -match '^Page number fields task:\s*(.+)$') {
            $result.page_number_fields_task_dir = $Matches[1].Trim()
        }
    }

    Assert-PathExists -Path $result.gate_summary -Label "visual gate summary"
    Assert-PathExists -Path $result.gate_final_review -Label "visual gate final review"

    if ($result.document_task_dir) {
        Assert-PathExists -Path $result.document_task_dir -Label "visual gate document task"
    }
    if ($result.fixed_grid_task_dir) {
        Assert-PathExists -Path $result.fixed_grid_task_dir -Label "visual gate fixed-grid task"
    }
    if ($result.section_page_setup_task_dir) {
        Assert-PathExists -Path $result.section_page_setup_task_dir -Label "visual gate section page setup task"
    }
    if ($result.page_number_fields_task_dir) {
        Assert-PathExists -Path $result.page_number_fields_task_dir -Label "visual gate page number fields task"
    }

    return $result
}

function Parse-TemplateSchemaCommandOutput {
    param(
        [string[]]$Lines,
        [string]$Command
    )

    if ([string]::IsNullOrWhiteSpace($Command)) {
        throw "Template schema command name must not be empty."
    }

    $pattern = '^\{"command":"' + [regex]::Escape($Command) + '",'
    $jsonLine = $Lines |
        Where-Object { $_ -match $pattern } |
        Select-Object -Last 1
    if ([string]::IsNullOrWhiteSpace([string]$jsonLine)) {
        throw "Template schema command '$Command' did not emit a JSON result line."
    }

    return $jsonLine | ConvertFrom-Json
}

function Parse-TemplateSchemaCheckOutput {
    param([string[]]$Lines)

    return Parse-TemplateSchemaCommandOutput -Lines $Lines -Command "check-template-schema"
}

function Parse-TemplateSchemaLintOutput {
    param([string[]]$Lines)

    return Parse-TemplateSchemaCommandOutput -Lines $Lines -Command "lint-template-schema"
}

function Parse-TemplateSchemaManifestSummary {
    param([string]$SummaryPath)

    Assert-PathExists -Path $SummaryPath -Label "template schema manifest summary"
    return Get-Content -Raw -LiteralPath $SummaryPath | ConvertFrom-Json
}

function Parse-ProjectTemplateSmokeSummary {
    param([string]$SummaryPath)

    Assert-PathExists -Path $SummaryPath -Label "project template smoke summary"
    return Get-Content -Raw -LiteralPath $SummaryPath | ConvertFrom-Json
}
