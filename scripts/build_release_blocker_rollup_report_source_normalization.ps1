function Get-SourcePayloadPreview {
    param(
        [string]$Path,
        [int]$MaxLength = 2048
    )

    if ([string]::IsNullOrWhiteSpace($Path) -or -not (Test-Path -LiteralPath $Path)) {
        return ""
    }

    try {
        $payload = Get-Content -Raw -Encoding UTF8 -LiteralPath $Path
        if ($null -eq $payload) {
            return ""
        }

        $normalized = ([string]$payload).Trim()
        if ($normalized.Length -le $MaxLength) {
            return $normalized
        }

        return $normalized.Substring(0, $MaxLength)
    } catch {
        return ""
    }
}

function Test-InformationalActionItem {
    param($Item)

    $category = Get-JsonString -Object $Item -Name "category"
    if ($category -in @("release_checklist", "manual_release_checklist", "informational")) {
        return $true
    }

    if (Get-JsonBool -Object $Item -Name "optional") {
        return $true
    }

    $releaseBlockingProperty = Get-JsonProperty -Object $Item -Name "release_blocking"
    if ($null -ne $releaseBlockingProperty -and -not (Get-JsonBool -Object $Item -Name "release_blocking" -DefaultValue $true)) {
        return $true
    }

    $id = Get-JsonString -Object $Item -Name "id"
    $action = Get-JsonString -Object $Item -Name "action"
    return ($id -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline") -or
        $action -in @("promote_numbering_catalog_exemplar", "register_numbering_catalog_baseline"))
}

function Copy-ActionItemWithReleaseChecklistDefaults {
    param($Item)

    $copy = [ordered]@{}
    if ($Item -is [System.Collections.IDictionary]) {
        foreach ($key in @($Item.Keys)) {
            $copy[[string]$key] = $Item[$key]
        }
    } else {
        foreach ($property in @($Item.PSObject.Properties)) {
            $copy[$property.Name] = $property.Value
        }
    }

    if ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $copy -Name "category"))) {
        $copy["category"] = "release_checklist"
    }
    if ([string]::IsNullOrWhiteSpace((Get-JsonString -Object $copy -Name "severity"))) {
        $copy["severity"] = "info"
    }
    $copy["release_blocking"] = $false
    $copy["optional"] = $true

    return $copy
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Copy-OptionalJsonProperties {
    param(
        [System.Collections.IDictionary]$Target,
        $Source,
        [string[]]$Names
    )

    foreach ($name in @($Names)) {
        $value = Get-JsonProperty -Object $Source -Name $name
        if ($null -ne $value) {
            $Target[$name] = $value
        }
    }
}

function Set-OptionalSourceReportField {
    param(
        [System.Collections.IDictionary]$Target,
        [string]$Name,
        $Value
    )

    if ($null -eq $Value) { return }
    if ($Value -is [string] -and [string]::IsNullOrWhiteSpace($Value)) { return }
    $Target[$Name] = $Value
}

function Set-JsonPropertyValue {
    param(
        $Object,
        [string]$Name,
        $Value
    )

    if ($null -eq $Object) { return }

    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }

    $existing = $Object.PSObject.Properties[$Name]
    if ($null -ne $existing) {
        $existing.Value = $Value
        return
    }

    Add-Member -InputObject $Object -MemberType NoteProperty -Name $Name -Value $Value
}

function Normalize-ReadinessActionEvidence {
    param(
        $Items,
        [string]$DefaultSourceSchema,
        [string]$DefaultSourceReport,
        [string]$DefaultSourceReportDisplay,
        [string]$DefaultSourceJson,
        [string]$DefaultSourceJsonDisplay,
        [string]$RepoRoot
    )

    foreach ($evidence in @($Items)) {
        if ($null -eq $evidence) {
            continue
        }
        if ($evidence -isnot [System.Collections.IDictionary] -and $evidence -isnot [pscustomobject]) {
            continue
        }

        $sourceSchema = Get-JsonString -Object $evidence -Name "source_schema"
        $sourceReport = Get-JsonString -Object $evidence -Name "source_report"
        $sourceReportDisplay = Get-JsonString -Object $evidence -Name "source_report_display"
        $sourceJson = Get-JsonString -Object $evidence -Name "source_json"
        $sourceJsonDisplay = Get-JsonString -Object $evidence -Name "source_json_display"

        if ([string]::IsNullOrWhiteSpace($sourceSchema)) {
            Set-JsonPropertyValue -Object $evidence -Name "source_schema" -Value $DefaultSourceSchema
        }

        if ([string]::IsNullOrWhiteSpace($sourceReport)) {
            if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
                $sourceReport = Resolve-RepoPathFromDisplayPath -RepoRoot $RepoRoot -DisplayPath $sourceReportDisplay
            } else {
                $sourceReport = $DefaultSourceReport
            }
            Set-JsonPropertyValue -Object $evidence -Name "source_report" -Value $sourceReport
        }

        if ([string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
            if (-not [string]::IsNullOrWhiteSpace($DefaultSourceReportDisplay)) {
                $sourceReportDisplay = $DefaultSourceReportDisplay
            } elseif (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
                $sourceReportDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $sourceReport
            }

            if (-not [string]::IsNullOrWhiteSpace($sourceReportDisplay)) {
                Set-JsonPropertyValue -Object $evidence -Name "source_report_display" -Value $sourceReportDisplay
            }
        }

        if ([string]::IsNullOrWhiteSpace($sourceJson)) {
            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                $sourceJson = Resolve-RepoPathFromDisplayPath -RepoRoot $RepoRoot -DisplayPath $sourceJsonDisplay
            } elseif (-not [string]::IsNullOrWhiteSpace($sourceReport)) {
                $sourceJson = $sourceReport
            } else {
                $sourceJson = $DefaultSourceJson
            }

            if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                Set-JsonPropertyValue -Object $evidence -Name "source_json" -Value $sourceJson
            }
        }

        if ([string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
            if (-not [string]::IsNullOrWhiteSpace($sourceJson)) {
                $sourceJsonDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $sourceJson
            } elseif (-not [string]::IsNullOrWhiteSpace($DefaultSourceJsonDisplay)) {
                $sourceJsonDisplay = $DefaultSourceJsonDisplay
            }

            if (-not [string]::IsNullOrWhiteSpace($sourceJsonDisplay)) {
                Set-JsonPropertyValue -Object $evidence -Name "source_json_display" -Value $sourceJsonDisplay
            }
        }
    }
}
