function Write-Step {
    param([string]$Message)
    Write-Host "[edit-document-from-plan] $Message"
}

function Resolve-OptionalEditPath {
    param(
        [string]$RepoRoot,
        [string]$InputPath
    )

    if ([string]::IsNullOrWhiteSpace($InputPath)) {
        return ""
    }

    return Resolve-TemplateSchemaPath `
        -RepoRoot $RepoRoot `
        -InputPath $InputPath `
        -AllowMissing
}

function Ensure-PathParent {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return
    }

    $directory = [System.IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
}

function Get-OptionalObjectPropertyObject {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $null
    }

    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) {
            return ,$Object[$Name]
        }

        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return ,$property.Value
}

function Test-ObjectPropertyExists {
    param(
        $Object,
        [string]$Name
    )

    if ($null -eq $Object) {
        return $false
    }

    if ($Object -is [System.Collections.IDictionary]) {
        return $Object.Contains($Name)
    }

    return $null -ne $Object.PSObject.Properties[$Name]
}

function Set-ObjectPropertyValue {
    param(
        $Object,
        [string]$Name,
        $Value
    )

    if ($null -eq $Object) {
        throw "Cannot set '$Name' on a null object."
    }

    if ($Object -is [System.Collections.IDictionary]) {
        $Object[$Name] = $Value
        return
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        Add-Member -InputObject $Object -NotePropertyName $Name -NotePropertyValue $Value
        return
    }

    $property.Value = $Value
}

function Get-ObjectOwnProperties {
    param(
        $Object,
        [string]$Label
    )

    if ($null -eq $Object) {
        return @()
    }

    if ($Object -is [System.Collections.IDictionary]) {
        foreach ($key in $Object.Keys) {
            [pscustomobject]@{
                Name = [string]$key
                Value = $Object[$key]
            }
        }
        return
    }

    if ($Object -is [System.Collections.IEnumerable] -and
        -not ($Object -is [string]) -and
        -not ($Object -is [System.Management.Automation.PSCustomObject])) {
        $items = @($Object)
        if ($items.Count -eq 1) {
            Get-ObjectOwnProperties -Object $items[0] -Label $Label
            return
        }

        throw "$Label must be a JSON object."
    }

    foreach ($property in $Object.PSObject.Properties) {
        if ([string]$property.MemberType -ne "NoteProperty") {
            continue
        }

        [pscustomobject]@{
            Name = [string]$property.Name
            Value = $property.Value
        }
    }
}

function Get-OptionalObjectPropertyValue {
    param(
        $Object,
        [string]$Name
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        return ""
    }
    if ($value -is [System.DateTime]) {
        return $value.ToUniversalTime().ToString(
            "yyyy-MM-ddTHH:mm:ssZ",
            [System.Globalization.CultureInfo]::InvariantCulture)
    }

    return [string]$value
}

function ConvertFrom-EditPlanJson {
    param([string]$Json)

    $convertFromJsonCommand = Get-Command ConvertFrom-Json
    if ($convertFromJsonCommand.Parameters.ContainsKey("DateKind")) {
        return $Json | ConvertFrom-Json -DateKind String
    }

    return $Json | ConvertFrom-Json
}

function Get-RequiredObjectPropertyValue {
    param(
        $Object,
        [string]$Name,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyValue -Object $Object -Name $Name
    if ([string]::IsNullOrWhiteSpace($value)) {
        throw "$Label must provide '$Name'."
    }

    return $value
}

function Get-EditPlanOperations {
    param($Plan)

    if ($Plan -is [System.Collections.IEnumerable] -and
        -not ($Plan -is [string]) -and
        -not ($Plan -is [System.Collections.IDictionary]) -and
        -not ($Plan -is [System.Management.Automation.PSCustomObject])) {
        return @($Plan | Where-Object { $null -ne $_ })
    }

    $operations = Get-OptionalObjectPropertyObject -Object $Plan -Name "operations"
    if ($null -eq $operations) {
        throw "Edit plan must be an array or provide an 'operations' array."
    }
    if ($operations -is [string]) {
        throw "Edit plan 'operations' must be an array."
    }

    return @($operations | Where-Object { $null -ne $_ })
}

function Get-StringArrayProperty {
    param(
        $Object,
        [string]$Name,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name $Name
    if ($null -eq $value) {
        throw "$Label must provide '$Name'."
    }
    if ($value -is [string]) {
        return @([string]$value)
    }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | ForEach-Object { [string]$_ })
    }

    throw "$Label '$Name' must be an array of strings."
}

function Get-TableRowsProperty {
    param(
        $Object,
        [string]$Label
    )

    $value = Get-OptionalObjectPropertyObject -Object $Object -Name "rows"
    if ($null -eq $value) {
        throw "$Label must provide 'rows'."
    }
    if ($value -is [string]) {
        throw "$Label 'rows' must be an array of row arrays."
    }

    $rows = New-Object 'System.Collections.Generic.List[object]'
    foreach ($row in @($value)) {
        if ($row -is [string]) {
            $rows.Add(@([string]$row)) | Out-Null
            continue
        }
        if ($row -is [System.Collections.IEnumerable]) {
            $rows.Add(@($row | ForEach-Object { [string]$_ })) | Out-Null
            continue
        }
        throw "$Label rows must contain arrays of cell texts."
    }

    return ,@($rows.ToArray())
}
