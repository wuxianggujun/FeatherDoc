<#
.SYNOPSIS
Builds a content-control data-binding governance report.

.DESCRIPTION
Reads existing inspect-content-controls and sync-content-controls-from-custom-xml
JSON outputs, then writes a read-only JSON/Markdown handoff for Custom XML data
binding coverage, sync issues, placeholder risk, lock review, and action items.
The script does not open DOCX files or rerun CLI commands.
#>
param(
    [string[]]$InputJson = @(),
    [string[]]$InputRoot = @(),
    [string]$OutputDir = "output/content-control-data-binding-governance",
    [string]$SummaryJson = "",
    [string]$ReportMarkdown = "",
    [switch]$FailOnBlocker,
    [switch]$FailOnWarning
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "template_schema_cli_common.ps1")

$contentControlGovernanceSchema = "featherdoc.content_control_data_binding_governance_report.v1"
$contentControlGovernanceOpenCommand = "pwsh -ExecutionPolicy Bypass -File .\scripts\build_content_control_data_binding_governance_report.ps1"

function Write-Step {
    param([string]$Message)
    Write-Host "[content-control-data-binding-governance] $Message"
}

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RepoPath {
    param([string]$RepoRoot, [string]$Path, [switch]$AllowMissing)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
    $resolved = [System.IO.Path]::GetFullPath($candidate)
    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $resolved)) {
        throw "Path does not exist: $Path"
    }
    return $resolved
}

function Ensure-Directory {
    param([string]$Path)
    if (-not [string]::IsNullOrWhiteSpace($Path)) {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Get-DisplayPath {
    param([string]$RepoRoot, [string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) { return "" }
    $root = [System.IO.Path]::GetFullPath($RepoRoot)
    if (-not $root.EndsWith([System.IO.Path]::DirectorySeparatorChar) -and
        -not $root.EndsWith([System.IO.Path]::AltDirectorySeparatorChar)) {
        $root += [System.IO.Path]::DirectorySeparatorChar
    }
    try {
        $resolved = [System.IO.Path]::GetFullPath($Path)
        if ($resolved.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
            $relative = $resolved.Substring($root.Length).TrimStart('\', '/')
            if ([string]::IsNullOrWhiteSpace($relative)) { return "." }
            return ".\" + ($relative -replace '/', '\')
        }
        return $resolved
    } catch {
        return $Path
    }
}

function Get-JsonProperty {
    param($Object, [string]$Name)

    if ($null -eq $Object) { return $null }
    if ($Object -is [System.Collections.IDictionary]) {
        if ($Object.Contains($Name)) { return $Object[$Name] }
        return $null
    }
    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-JsonString {
    param($Object, [string]$Name, [string]$DefaultValue = "")

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    return [string]$value
}

function Get-JsonInt {
    param($Object, [string]$Name, [int]$DefaultValue = 0)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    $parsed = 0
    if ([int]::TryParse([string]$value, [ref]$parsed)) { return $parsed }
    return $DefaultValue
}

function Get-JsonBool {
    param($Object, [string]$Name, [bool]$DefaultValue = $false)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value -or [string]::IsNullOrWhiteSpace([string]$value)) {
        return $DefaultValue
    }
    if ($value -is [bool]) { return [bool]$value }
    return [string]$value -in @("true", "True", "1", "yes", "Yes")
}

function Get-JsonArray {
    param($Object, [string]$Name)

    $value = Get-JsonProperty -Object $Object -Name $Name
    if ($null -eq $value) { return @() }
    if ($value -is [string]) { return @($value) }
    if ($value -is [System.Collections.IDictionary]) { return @($value) }
    if ($value -is [System.Collections.IEnumerable]) {
        return @($value | Where-Object { $null -ne $_ })
    }
    return @($value)
}

function Get-FirstJsonString {
    param(
        $Object,
        [string[]]$Names,
        [string]$DefaultValue = ""
    )

    foreach ($name in @($Names)) {
        $value = Get-JsonString -Object $Object -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            return $value
        }
    }

    return $DefaultValue
}

function New-ContentControlEvidenceProvenance {
    param(
        [string]$RepoRoot,
        [string]$Path,
        $Json
    )

    $inputDocx = Get-FirstJsonString -Object $Json -Names @("input_docx", "input_docx_path", "docx_path", "document_path")
    $inputDocxDisplay = Get-FirstJsonString -Object $Json -Names @("input_docx_display", "input_docx_path_display", "docx_path_display", "document_path_display")
    if ([string]::IsNullOrWhiteSpace($inputDocxDisplay) -and -not [string]::IsNullOrWhiteSpace($inputDocx)) {
        $inputDocxDisplay = Get-DisplayPath -RepoRoot $RepoRoot -Path $inputDocx
    }

    return [ordered]@{
        source_json = $Path
        source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
        input_docx = $inputDocx
        input_docx_display = $inputDocxDisplay
        template_name = Get-FirstJsonString -Object $Json -Names @("template_name", "project_template_name")
        schema_target = Get-FirstJsonString -Object $Json -Names @("schema_target", "target", "schema_baseline_target")
        target_mode = Get-FirstJsonString -Object $Json -Names @("target_mode", "schema_target_mode", "export_target_mode")
    }
}

function Get-ProvenanceString {
    param(
        $Object,
        $Fallback,
        [string[]]$Names,
        [string]$DefaultValue = ""
    )

    $value = Get-FirstJsonString -Object $Object -Names $Names
    if (-not [string]::IsNullOrWhiteSpace($value)) {
        return $value
    }

    return Get-FirstJsonString -Object $Fallback -Names $Names -DefaultValue $DefaultValue
}

function Get-UniqueProvenanceValue {
    param(
        [object[]]$Items,
        [string]$Name
    )

    $values = @(@(
            foreach ($item in @($Items)) {
                $value = Get-JsonString -Object $item -Name $Name
                if (-not [string]::IsNullOrWhiteSpace($value)) {
                    $value
                }
            }
        ) | Sort-Object -Unique)
    if ($values.Count -eq 1) {
        return [string]$values[0]
    }

    return ""
}

function New-ProvenanceSummary {
    param([object[]]$Items)

    $groups = @(
        $Items |
            Where-Object {
                -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "source_json")) -or
                -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "input_docx")) -or
                -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "template_name")) -or
                -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "schema_target")) -or
                -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "target_mode"))
            } |
            Group-Object {
                $item = $_
                $keyParts = @(
                    (Get-JsonString -Object $item -Name "source_json")
                    (Get-JsonString -Object $item -Name "input_docx")
                    (Get-JsonString -Object $item -Name "template_name")
                    (Get-JsonString -Object $item -Name "schema_target")
                    (Get-JsonString -Object $item -Name "target_mode")
                )
                $keyParts -join "|||"
            }
    )

    return @(
        foreach ($group in @($groups)) {
            $item = @($group.Group)[0]
            [ordered]@{
                source_json = Get-JsonString -Object $item -Name "source_json"
                source_json_display = Get-JsonString -Object $item -Name "source_json_display"
                input_docx = Get-JsonString -Object $item -Name "input_docx"
                input_docx_display = Get-JsonString -Object $item -Name "input_docx_display"
                template_name = Get-JsonString -Object $item -Name "template_name"
                schema_target = Get-JsonString -Object $item -Name "schema_target"
                target_mode = Get-JsonString -Object $item -Name "target_mode"
                evidence_count = [int]$group.Count
            }
        }
    )
}

function Add-MarkdownProvenanceLines {
    param(
        [System.Collections.Generic.List[string]]$Lines,
        $Item,
        [string]$Indent = "  "
    )

    $fields = @(
        @("input_docx_display", "input_docx_display"),
        @("input_docx", "input_docx"),
        @("template_name", "template_name"),
        @("schema_target", "schema_target"),
        @("target_mode", "target_mode")
    )
    foreach ($field in $fields) {
        $name = [string]$field[0]
        $label = [string]$field[1]
        $value = Get-JsonString -Object $Item -Name $name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            $Lines.Add("$Indent- ${label}: ``$value``") | Out-Null
        }
    }
}

function Get-ProvenanceSummaryField {
    param($Item)

    $values = @(
        "source_json_display",
        "input_docx_display",
        "template_name",
        "schema_target",
        "target_mode"
    ) | ForEach-Object {
        $value = Get-JsonString -Object $Item -Name $_
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            "$_=$value"
        }
    }

    return ($values -join " ")
}

function Get-InputJsonPaths {
    param([string]$RepoRoot, [string[]]$ExplicitPaths, [string[]]$Roots)

    $paths = New-Object 'System.Collections.Generic.List[string]'
    foreach ($path in @(Expand-TemplateSchemaArgumentList -Values $ExplicitPaths)) {
        $paths.Add((Resolve-RepoPath -RepoRoot $RepoRoot -Path $path -AllowMissing)) | Out-Null
    }

    $scanRoots = @(Expand-TemplateSchemaArgumentList -Values $Roots)
    if ($paths.Count -eq 0 -and $scanRoots.Count -eq 0) {
        $scanRoots = @(
            "output/content-control-data-binding",
            "output/content-control-rich-replacement",
            "output/project-template-smoke"
        )
    }

    foreach ($root in $scanRoots) {
        $resolvedRoot = Resolve-RepoPath -RepoRoot $RepoRoot -Path $root -AllowMissing
        if (-not (Test-Path -LiteralPath $resolvedRoot)) { continue }
        if ((Get-Item -LiteralPath $resolvedRoot).PSIsContainer) {
            Get-ChildItem -LiteralPath $resolvedRoot -Recurse -File -Filter "*.json" |
                ForEach-Object { $paths.Add($_.FullName) | Out-Null }
        } else {
            $paths.Add($resolvedRoot) | Out-Null
        }
    }

    return @($paths.ToArray() | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique)
}

function Get-EvidenceKind {
    param($Json)

    $schema = Get-JsonString -Object $Json -Name "schema"
    if ($schema -eq "featherdoc.content_control_data_binding_governance_report.v1") {
        return "content_control_data_binding_governance_report"
    }
    if ((Get-JsonProperty -Object $Json -Name "content_controls") -ne $null) {
        return "inspect_content_controls"
    }
    if ((Get-JsonProperty -Object $Json -Name "synced_items") -ne $null -or
        (Get-JsonProperty -Object $Json -Name "scanned_content_controls") -ne $null -or
        (Get-JsonProperty -Object $Json -Name "bound_content_controls") -ne $null) {
        return "custom_xml_sync_result"
    }
    return $schema
}

function New-BindingKey {
    param([string]$StoreItemId, [string]$XPath)

    if ([string]::IsNullOrWhiteSpace($StoreItemId) -or [string]::IsNullOrWhiteSpace($XPath)) {
        return ""
    }
    return ($StoreItemId.Trim().ToLowerInvariant() + "|" + $XPath.Trim())
}

function New-ContentControlSelectorTemplate {
    param([string]$Tag, [string]$Alias)

    if (-not [string]::IsNullOrWhiteSpace($Tag)) {
        return ConvertTo-TemplateSchemaCommandLine -Arguments @("--tag", $Tag)
    }
    if (-not [string]::IsNullOrWhiteSpace($Alias)) {
        return ConvertTo-TemplateSchemaCommandLine -Arguments @("--alias", $Alias)
    }
    return "<content-control-selector>"
}

function New-SyncContentControlCommandTemplate {
    return "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
}

function New-ClearLockCommandTemplate {
    param([string]$Tag, [string]$Alias)

    $selector = New-ContentControlSelectorTemplate -Tag $Tag -Alias $Alias
    return "featherdoc_cli set-content-control-form-state <input.docx> $selector --clear-lock --output <reviewed.docx> --json"
}

function New-BindContentControlCommandTemplate {
    param([string]$Tag, [string]$Alias)

    $selector = New-ContentControlSelectorTemplate -Tag $Tag -Alias $Alias
    return "featherdoc_cli set-content-control-form-state <input.docx> $selector --data-binding-store-item-id <store-item-id> --data-binding-xpath <xpath> --output <bound.docx> --json"
}

function New-InspectContentControlCommandTemplate {
    param([string]$Tag, [string]$Alias)

    $selector = New-ContentControlSelectorTemplate -Tag $Tag -Alias $Alias
    return "featherdoc_cli inspect-content-controls <input.docx> $selector --json"
}

function New-ReleaseBlocker {
    param(
        [string]$Id,
        [string]$SourceJson,
        [string]$PartEntryName,
        [object]$ContentControlIndex,
        [string]$Tag,
        [string]$Alias,
        [string]$StoreItemId,
        [string]$XPath,
        [string]$Status,
        [string]$Action,
        [string]$Message,
        [string]$RepairStrategy = "",
        [string]$RepairHint = "",
        [string]$CommandTemplate = "",
        [string]$SourceJsonDisplay = "",
        [string]$InputDocx = "",
        [string]$InputDocxDisplay = "",
        [string]$TemplateName = "",
        [string]$SchemaTarget = "",
        [string]$TargetMode = ""
    )

    return [ordered]@{
        id = $Id
        severity = "error"
        status = $Status
        action = $Action
        message = $Message
        repair_strategy = $RepairStrategy
        repair_hint = $RepairHint
        command_template = $CommandTemplate
        source_schema = $contentControlGovernanceSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = $SourceJson
        source_json_display = $SourceJsonDisplay
        input_docx = $InputDocx
        input_docx_display = $InputDocxDisplay
        template_name = $TemplateName
        schema_target = $SchemaTarget
        target_mode = $TargetMode
        part_entry_name = $PartEntryName
        content_control_index = $ContentControlIndex
        tag = $Tag
        alias = $Alias
        store_item_id = $StoreItemId
        xpath = $XPath
    }
}

function New-ActionItem {
    param(
        [string]$Id,
        [string]$Action,
        [string]$Title,
        [string]$SourceJson,
        [string]$PartEntryName,
        [object]$ContentControlIndex,
        [string]$Tag,
        [string]$Alias,
        [string]$StoreItemId,
        [string]$XPath,
        [string]$RepairStrategy = "",
        [string]$RepairHint = "",
        [string]$CommandTemplate = "",
        [string]$SourceJsonDisplay = "",
        [string]$OpenCommand = $contentControlGovernanceOpenCommand,
        [string]$DuplicateBindingKey = "",
        [int]$DuplicateMemberCount = 0,
        [object[]]$DuplicateMembers = @(),
        [string]$InputDocx = "",
        [string]$InputDocxDisplay = "",
        [string]$TemplateName = "",
        [string]$SchemaTarget = "",
        [string]$TargetMode = ""
    )

    return [ordered]@{
        id = $Id
        action = $Action
        title = $Title
        open_command = $OpenCommand
        repair_strategy = $RepairStrategy
        repair_hint = $RepairHint
        command_template = $CommandTemplate
        source_schema = $contentControlGovernanceSchema
        source_report = $summaryPath
        source_report_display = $summaryDisplay
        source_json = $SourceJson
        source_json_display = $SourceJsonDisplay
        input_docx = $InputDocx
        input_docx_display = $InputDocxDisplay
        template_name = $TemplateName
        schema_target = $SchemaTarget
        target_mode = $TargetMode
        part_entry_name = $PartEntryName
        content_control_index = $ContentControlIndex
        tag = $Tag
        alias = $Alias
        store_item_id = $StoreItemId
        xpath = $XPath
        duplicate_binding_key = $DuplicateBindingKey
        duplicate_member_count = $DuplicateMemberCount
        duplicate_members = @($DuplicateMembers)
    }
}

function New-RepairPlanItem {
    param([string]$SourceKind, $Item)

    $repairStrategy = Get-JsonString -Object $Item -Name "repair_strategy"
    if ([string]::IsNullOrWhiteSpace($repairStrategy)) {
        return $null
    }

    $planStatus = switch ($repairStrategy) {
        "fix_custom_xml_source" { "source_fix_required" }
        "sync_bound_content_control" { "review_then_apply" }
        "review_lock_state" { "review_then_apply" }
        "bind_or_exempt_form_control" { "requires_user_values" }
        "deduplicate_or_confirm_shared_binding" { "review_only" }
        default { "review_only" }
    }
    $applySupported = $repairStrategy -in @(
        "sync_bound_content_control",
        "review_lock_state",
        "bind_or_exempt_form_control"
    )
    $requiresVisualVerification = $repairStrategy -in @(
        "fix_custom_xml_source",
        "sync_bound_content_control",
        "review_lock_state",
        "bind_or_exempt_form_control"
    )
    $requiredUserValues = if ($repairStrategy -eq "bind_or_exempt_form_control") {
        @("data_binding_store_item_id", "data_binding_xpath")
    } else {
        @()
    }

    return [ordered]@{
        source_kind = $SourceKind
        source_id = Get-JsonString -Object $Item -Name "id"
        source_schema = Get-JsonString -Object $Item -Name "source_schema"
        source_report = Get-JsonString -Object $Item -Name "source_report"
        source_report_display = Get-JsonString -Object $Item -Name "source_report_display" -DefaultValue (Get-JsonString -Object $Item -Name "source_json_display")
        source_json = Get-JsonString -Object $Item -Name "source_json"
        source_json_display = Get-JsonString -Object $Item -Name "source_json_display"
        input_docx = Get-JsonString -Object $Item -Name "input_docx"
        input_docx_display = Get-JsonString -Object $Item -Name "input_docx_display"
        template_name = Get-JsonString -Object $Item -Name "template_name"
        schema_target = Get-JsonString -Object $Item -Name "schema_target"
        target_mode = Get-JsonString -Object $Item -Name "target_mode"
        part_entry_name = Get-JsonString -Object $Item -Name "part_entry_name"
        content_control_index = Get-JsonProperty -Object $Item -Name "content_control_index"
        tag = Get-JsonString -Object $Item -Name "tag"
        alias = Get-JsonString -Object $Item -Name "alias"
        action = Get-JsonString -Object $Item -Name "action"
        open_command = Get-JsonString -Object $Item -Name "open_command"
        repair_strategy = $repairStrategy
        repair_hint = Get-JsonString -Object $Item -Name "repair_hint"
        command_template = Get-JsonString -Object $Item -Name "command_template"
        duplicate_binding_key = Get-JsonString -Object $Item -Name "duplicate_binding_key"
        duplicate_member_count = Get-JsonInt -Object $Item -Name "duplicate_member_count"
        duplicate_members = @(
            Get-JsonArray -Object $Item -Name "duplicate_members"
        )
        plan_status = $planStatus
        apply_supported = $applySupported
        native_dry_run_supported = $false
        required_user_values = @($requiredUserValues)
        requires_visual_verification = $requiresVisualVerification
    }
}

function Add-SummaryGroup {
    param([object[]]$Items, [string]$PropertyName, [string]$OutputName)

    return @(
        $Items |
            Group-Object { [string](Get-JsonProperty -Object $_ -Name $PropertyName) } |
            Sort-Object Name |
            ForEach-Object {
                [ordered]@{
                    $OutputName = $_.Name
                    count = $_.Count
                }
            }
    )
}

function New-BindingCoverageSummary {
    param([object[]]$Items)

    if (@($Items).Count -eq 0) { return @() }

    $counts = [ordered]@{
        bound = 0
        bound_placeholder = 0
        locked_bound = 0
        unbound = 0
    }

    foreach ($item in @($Items)) {
        $bindingKey = [string](Get-JsonProperty -Object $item -Name "binding_key")
        $hasBinding = -not [string]::IsNullOrWhiteSpace($bindingKey)
        if (-not $hasBinding) {
            $counts["unbound"] = [int]$counts["unbound"] + 1
            continue
        }

        $counts["bound"] = [int]$counts["bound"] + 1
        if ([bool](Get-JsonProperty -Object $item -Name "showing_placeholder")) {
            $counts["bound_placeholder"] = [int]$counts["bound_placeholder"] + 1
        }
        $lock = [string](Get-JsonProperty -Object $item -Name "lock")
        if (-not [string]::IsNullOrWhiteSpace($lock)) {
            $counts["locked_bound"] = [int]$counts["locked_bound"] + 1
        }
    }

    return @(
        foreach ($coverage in @($counts.Keys)) {
            [ordered]@{
                coverage = $coverage
                count = [int]$counts[$coverage]
            }
        }
    )
}

function Convert-InspectContentControls {
    param([string]$RepoRoot, [string]$Path, $Json, $Provenance)

    $part = Get-JsonString -Object $Json -Name "part"
    $entryName = Get-JsonString -Object $Json -Name "entry_name"
    $items = New-Object 'System.Collections.Generic.List[object]'
    foreach ($control in @(Get-JsonArray -Object $Json -Name "content_controls")) {
        $storeItemId = Get-JsonString -Object $control -Name "data_binding_store_item_id"
        $xpath = Get-JsonString -Object $control -Name "data_binding_xpath"
        $items.Add([ordered]@{
            source_json = $Path
            source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
            input_docx = Get-ProvenanceString -Object $control -Fallback $Provenance -Names @("input_docx", "input_docx_path", "docx_path", "document_path")
            input_docx_display = Get-ProvenanceString -Object $control -Fallback $Provenance -Names @("input_docx_display", "input_docx_path_display", "docx_path_display", "document_path_display")
            template_name = Get-ProvenanceString -Object $control -Fallback $Provenance -Names @("template_name", "project_template_name")
            schema_target = Get-ProvenanceString -Object $control -Fallback $Provenance -Names @("schema_target", "target", "schema_baseline_target")
            target_mode = Get-ProvenanceString -Object $control -Fallback $Provenance -Names @("target_mode", "schema_target_mode", "export_target_mode")
            part = $part
            part_entry_name = $entryName
            content_control_index = Get-JsonInt -Object $control -Name "index"
            kind = Get-JsonString -Object $control -Name "kind"
            form_kind = Get-JsonString -Object $control -Name "form_kind"
            tag = Get-JsonString -Object $control -Name "tag"
            alias = Get-JsonString -Object $control -Name "alias"
            lock = Get-JsonString -Object $control -Name "lock"
            store_item_id = $storeItemId
            xpath = $xpath
            prefix_mappings = Get-JsonString -Object $control -Name "data_binding_prefix_mappings"
            binding_key = New-BindingKey -StoreItemId $storeItemId -XPath $xpath
            showing_placeholder = Get-JsonBool -Object $control -Name "showing_placeholder"
            text = Get-JsonString -Object $control -Name "text"
        }) | Out-Null
    }
    return @($items.ToArray())
}

function Convert-SyncItems {
    param([string]$RepoRoot, [string]$Path, $Json, $Provenance)

    return @(
        foreach ($item in @(Get-JsonArray -Object $Json -Name "synced_items")) {
            [ordered]@{
                source_json = $Path
                source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
                input_docx = Get-ProvenanceString -Object $item -Fallback $Provenance -Names @("input_docx", "input_docx_path", "docx_path", "document_path")
                input_docx_display = Get-ProvenanceString -Object $item -Fallback $Provenance -Names @("input_docx_display", "input_docx_path_display", "docx_path_display", "document_path_display")
                template_name = Get-ProvenanceString -Object $item -Fallback $Provenance -Names @("template_name", "project_template_name")
                schema_target = Get-ProvenanceString -Object $item -Fallback $Provenance -Names @("schema_target", "target", "schema_baseline_target")
                target_mode = Get-ProvenanceString -Object $item -Fallback $Provenance -Names @("target_mode", "schema_target_mode", "export_target_mode")
                part_entry_name = Get-JsonString -Object $item -Name "part_entry_name"
                content_control_index = Get-JsonInt -Object $item -Name "content_control_index"
                tag = Get-JsonString -Object $item -Name "tag"
                alias = Get-JsonString -Object $item -Name "alias"
                store_item_id = Get-JsonString -Object $item -Name "store_item_id"
                xpath = Get-JsonString -Object $item -Name "xpath"
                previous_text = Get-JsonString -Object $item -Name "previous_text"
                value = Get-JsonString -Object $item -Name "value"
            }
        }
    )
}

function Convert-SyncIssues {
    param([string]$RepoRoot, [string]$Path, $Json, $Provenance)

    return @(
        foreach ($issue in @(Get-JsonArray -Object $Json -Name "issues")) {
            [ordered]@{
                source_json = $Path
                source_json_display = Get-DisplayPath -RepoRoot $RepoRoot -Path $Path
                input_docx = Get-ProvenanceString -Object $issue -Fallback $Provenance -Names @("input_docx", "input_docx_path", "docx_path", "document_path")
                input_docx_display = Get-ProvenanceString -Object $issue -Fallback $Provenance -Names @("input_docx_display", "input_docx_path_display", "docx_path_display", "document_path_display")
                template_name = Get-ProvenanceString -Object $issue -Fallback $Provenance -Names @("template_name", "project_template_name")
                schema_target = Get-ProvenanceString -Object $issue -Fallback $Provenance -Names @("schema_target", "target", "schema_baseline_target")
                target_mode = Get-ProvenanceString -Object $issue -Fallback $Provenance -Names @("target_mode", "schema_target_mode", "export_target_mode")
                part_entry_name = Get-JsonString -Object $issue -Name "part_entry_name"
                content_control_index = Get-JsonProperty -Object $issue -Name "content_control_index"
                tag = Get-JsonString -Object $issue -Name "tag"
                alias = Get-JsonString -Object $issue -Name "alias"
                store_item_id = Get-JsonString -Object $issue -Name "store_item_id"
                xpath = Get-JsonString -Object $issue -Name "xpath"
                reason = Get-JsonString -Object $issue -Name "reason" -DefaultValue "custom_xml_sync_issue"
            }
        }
    )
}

function New-ReportMarkdown {
    param($Summary)

    $lines = New-Object 'System.Collections.Generic.List[string]'
    $lines.Add("# Content Control Data Binding Governance") | Out-Null
    $lines.Add("") | Out-Null
    $lines.Add("- Status: ``$($Summary.status)``") | Out-Null
    $lines.Add("- Content controls: ``$($Summary.content_control_count)``") | Out-Null
    $lines.Add("- Bound controls: ``$($Summary.bound_content_control_count)``") | Out-Null
    $lines.Add("- Synced controls: ``$($Summary.synced_content_control_count)``") | Out-Null
    $lines.Add("- Sync issues: ``$($Summary.sync_issue_count)``") | Out-Null
    $lines.Add("- Release blockers: ``$($Summary.release_blocker_count)``") | Out-Null
    $lines.Add("- Action items: ``$($Summary.action_item_count)``") | Out-Null
    $lines.Add("- Repair plan schema: ``$($Summary.repair_plan_schema)``") | Out-Null
    $lines.Add("- Repair plan items: ``$($Summary.repair_plan_item_count)``") | Out-Null
    $lines.Add("- Apply-supported repair plans: ``$($Summary.repair_plan_apply_supported_count)``") | Out-Null
    $lines.Add("- Repair plans requiring user values: ``$($Summary.repair_plan_requires_user_values_count)``") | Out-Null
    $lines.Add("- Repair plans requiring visual verification: ``$($Summary.repair_plan_requires_visual_verification_count)``") | Out-Null
    if (-not [string]::IsNullOrWhiteSpace([string]$Summary.input_docx_display)) {
        $lines.Add("- Input DOCX: ``$($Summary.input_docx_display)``") | Out-Null
    } elseif (-not [string]::IsNullOrWhiteSpace([string]$Summary.input_docx)) {
        $lines.Add("- Input DOCX: ``$($Summary.input_docx)``") | Out-Null
    }
    if (-not [string]::IsNullOrWhiteSpace([string]$Summary.template_name)) {
        $lines.Add("- Template name: ``$($Summary.template_name)``") | Out-Null
    }
    if (-not [string]::IsNullOrWhiteSpace([string]$Summary.schema_target)) {
        $lines.Add("- Schema target: ``$($Summary.schema_target)``") | Out-Null
    }
    if (-not [string]::IsNullOrWhiteSpace([string]$Summary.target_mode)) {
        $lines.Add("- Target mode: ``$($Summary.target_mode)``") | Out-Null
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Provenance") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.provenance_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($entry in @($Summary.provenance_summary)) {
            $fieldSummary = Get-ProvenanceSummaryField -Item $entry
            if ([string]::IsNullOrWhiteSpace($fieldSummary)) {
                $fieldSummary = "evidence_count=$($entry.evidence_count)"
            } else {
                $fieldSummary = "$fieldSummary evidence_count=$($entry.evidence_count)"
            }
            $lines.Add("- $fieldSummary") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Binding Coverage") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.binding_coverage_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($entry in @($Summary.binding_coverage_summary)) {
            $lines.Add("- ``$($entry.coverage)``: ``$($entry.count)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Release Blockers") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.release_blockers).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($blocker in @($Summary.release_blockers)) {
            $lines.Add("- ``$($blocker.id)``: action=``$($blocker.action)`` schema=``$($blocker.source_schema)`` source_report_display=``$($blocker.source_report_display)`` source_json_display=``$($blocker.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.message)) {
                $lines.Add("  - message: $($blocker.message)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($blocker.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.repair_hint)) {
                $lines.Add("  - repair_hint: $($blocker.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$blocker.command_template)) {
                $lines.Add("  - command_template: ``$($blocker.command_template)``") | Out-Null
            }
            Add-MarkdownProvenanceLines -Lines $lines -Item $blocker
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Action Items") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.action_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.action_items)) {
            $lines.Add("- ``$($item.id)``: action=``$($item.action)`` schema=``$($item.source_schema)`` source_report_display=``$($item.source_report_display)`` source_json_display=``$($item.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.title)) {
                $lines.Add("  - title: $($item.title)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_strategy)) {
                $lines.Add("  - repair_strategy: ``$($item.repair_strategy)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.repair_hint)) {
                $lines.Add("  - repair_hint: $($item.repair_hint)") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.duplicate_binding_key)) {
                $lines.Add("  - duplicate_binding_key: ``$($item.duplicate_binding_key)``") | Out-Null
            }
            if (($null -ne $item.duplicate_member_count) -and ([int]$item.duplicate_member_count -gt 0)) {
                $lines.Add("  - duplicate_member_count: ``$($item.duplicate_member_count)``") | Out-Null
            }
            Add-MarkdownProvenanceLines -Lines $lines -Item $item
            if (@($item.duplicate_members).Count -gt 0) {
                $lines.Add("  - duplicate_members:") | Out-Null
                foreach ($member in @($item.duplicate_members)) {
                    $memberProvenance = Get-ProvenanceSummaryField -Item $member
                    if ([string]::IsNullOrWhiteSpace($memberProvenance)) {
                        $lines.Add("    - part_entry_name=``$([string]$member.part_entry_name)`` index=``$([string]$member.content_control_index)`` tag=``$([string]$member.tag)`` alias=``$([string]$member.alias)``") | Out-Null
                    } else {
                        $lines.Add("    - part_entry_name=``$([string]$member.part_entry_name)`` index=``$([string]$member.content_control_index)`` tag=``$([string]$member.tag)`` alias=``$([string]$member.alias)`` $memberProvenance") | Out-Null
                    }
                }
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Repair Plan Feasibility") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.repair_plan_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.repair_plan_items)) {
            $lines.Add("- ``$($item.source_id)``: source=``$($item.source_kind)`` strategy=``$($item.repair_strategy)`` status=``$($item.plan_status)`` apply_supported=``$($item.apply_supported)`` native_dry_run_supported=``$($item.native_dry_run_supported)`` requires_visual_verification=``$($item.requires_visual_verification)`` source_report_display=``$($item.source_report_display)`` source_json_display=``$($item.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$item.tag) -or
                -not [string]::IsNullOrWhiteSpace([string]$item.alias)) {
                $lines.Add("  - selector: tag=``$($item.tag)`` alias=``$($item.alias)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.command_template)) {
                $lines.Add("  - command_template: ``$($item.command_template)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.open_command)) {
                $lines.Add("  - open_command: ``$($item.open_command)``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace([string]$item.duplicate_binding_key)) {
                $lines.Add("  - duplicate_binding_key: ``$($item.duplicate_binding_key)``") | Out-Null
            }
            if (($null -ne $item.duplicate_member_count) -and ([int]$item.duplicate_member_count -gt 0)) {
                $lines.Add("  - duplicate_member_count: ``$($item.duplicate_member_count)``") | Out-Null
            }
            Add-MarkdownProvenanceLines -Lines $lines -Item $item
            if (@($item.duplicate_members).Count -gt 0) {
                $lines.Add("  - duplicate_members:") | Out-Null
                foreach ($member in @($item.duplicate_members)) {
                    $memberProvenance = Get-ProvenanceSummaryField -Item $member
                    if ([string]::IsNullOrWhiteSpace($memberProvenance)) {
                        $lines.Add("    - part_entry_name=``$([string]$member.part_entry_name)`` index=``$([string]$member.content_control_index)`` tag=``$([string]$member.tag)`` alias=``$([string]$member.alias)``") | Out-Null
                    } else {
                        $lines.Add("    - part_entry_name=``$([string]$member.part_entry_name)`` index=``$([string]$member.content_control_index)`` tag=``$([string]$member.tag)`` alias=``$([string]$member.alias)`` $memberProvenance") | Out-Null
                    }
                }
            }
            $requiredUserValues = @(Get-JsonArray -Object $item -Name "required_user_values")
            if ($requiredUserValues.Count -gt 0) {
                $lines.Add("  - required_user_values: ``$($requiredUserValues -join ', ')``") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Warnings") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.warnings).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($warning in @($Summary.warnings)) {
            $lines.Add("- ``$($warning.id)``: action=``$($warning.action)`` schema=``$($warning.source_schema)`` source_report_display=``$($warning.source_report_display)`` source_json_display=``$($warning.source_json_display)``") | Out-Null
            if (-not [string]::IsNullOrWhiteSpace([string]$warning.message)) {
                $lines.Add("  - message: $($warning.message)") | Out-Null
            }
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Source Files") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.source_files).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($source in @($Summary.source_files)) {
            $lines.Add("- ``$($source.path_display)``: kind=``$($source.kind)`` status=``$($source.status)``") | Out-Null
            Add-MarkdownProvenanceLines -Lines $lines -Item $source
        }
    }
    return @($lines)
}

$repoRoot = Resolve-RepoRoot
$resolvedOutputDir = Resolve-RepoPath -RepoRoot $repoRoot -Path $OutputDir -AllowMissing
Ensure-Directory -Path $resolvedOutputDir

$summaryPath = if ([string]::IsNullOrWhiteSpace($SummaryJson)) {
    Join-Path $resolvedOutputDir "summary.json"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $SummaryJson -AllowMissing
}
$markdownPath = if ([string]::IsNullOrWhiteSpace($ReportMarkdown)) {
    Join-Path $resolvedOutputDir "content_control_data_binding_governance.md"
} else {
    Resolve-RepoPath -RepoRoot $repoRoot -Path $ReportMarkdown -AllowMissing
}
$summaryDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($summaryPath))
Ensure-Directory -Path ([System.IO.Path]::GetDirectoryName($markdownPath))

$inputPaths = @(Get-InputJsonPaths -RepoRoot $repoRoot -ExplicitPaths $InputJson -Roots $InputRoot)
Write-Step "Reading $($inputPaths.Count) input JSON file(s)"

$contentControls = New-Object 'System.Collections.Generic.List[object]'
$syncItems = New-Object 'System.Collections.Generic.List[object]'
$syncIssues = New-Object 'System.Collections.Generic.List[object]'
$sourceFiles = New-Object 'System.Collections.Generic.List[object]'
$warnings = New-Object 'System.Collections.Generic.List[object]'

foreach ($path in @($inputPaths)) {
    $kind = "unknown"
    $status = "ignored"
    $errorMessage = ""
    $json = $null
    $provenance = $null
    if (-not (Test-Path -LiteralPath $path)) {
        $kind = "missing"
        $status = "missing"
        $warnings.Add([ordered]@{
            id = "input_json_missing"
            action = "collect_content_control_data_binding_evidence"
            source_schema = $contentControlGovernanceSchema
            source_report = $summaryPath
            source_report_display = $summaryDisplay
            source_json = $path
            source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
            message = "Input JSON was not found."
        }) | Out-Null
    } else {
        try {
            $json = Get-Content -Raw -Encoding UTF8 -LiteralPath $path | ConvertFrom-Json
            $provenance = New-ContentControlEvidenceProvenance -RepoRoot $repoRoot -Path $path -Json $json
            $kind = Get-EvidenceKind -Json $json
            switch ($kind) {
                "inspect_content_controls" {
                    foreach ($item in @(Convert-InspectContentControls -RepoRoot $repoRoot -Path $path -Json $json -Provenance $provenance)) {
                        $contentControls.Add($item) | Out-Null
                    }
                    $status = "loaded"
                }
                "custom_xml_sync_result" {
                    foreach ($item in @(Convert-SyncItems -RepoRoot $repoRoot -Path $path -Json $json -Provenance $provenance)) {
                        $syncItems.Add($item) | Out-Null
                    }
                    foreach ($issue in @(Convert-SyncIssues -RepoRoot $repoRoot -Path $path -Json $json -Provenance $provenance)) {
                        $syncIssues.Add($issue) | Out-Null
                    }
                    $status = "loaded"
                }
                "content_control_data_binding_governance_report" {
                    foreach ($item in @(Get-JsonArray -Object $json -Name "content_controls")) {
                        $contentControls.Add($item) | Out-Null
                    }
                    foreach ($item in @(Get-JsonArray -Object $json -Name "sync_items")) {
                        $syncItems.Add($item) | Out-Null
                    }
                    foreach ($issue in @(Get-JsonArray -Object $json -Name "sync_issues")) {
                        $syncIssues.Add($issue) | Out-Null
                    }
                    $status = "loaded"
                }
                default {
                    $status = "skipped"
                    $warnings.Add([ordered]@{
                        id = "source_json_schema_skipped"
                        action = "review_content_control_data_binding_evidence"
                        source_schema = $contentControlGovernanceSchema
                        source_report = $summaryPath
                        source_report_display = $summaryDisplay
                        source_json = $path
                        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                        message = "Input JSON kind '$kind' is not content-control data-binding evidence."
                    }) | Out-Null
                }
            }
        } catch {
            $status = "failed"
            $errorMessage = $_.Exception.Message
            $warnings.Add([ordered]@{
                id = "source_json_read_failed"
                action = "review_content_control_data_binding_evidence"
                source_schema = $contentControlGovernanceSchema
                source_report = $summaryPath
                source_report_display = $summaryDisplay
                source_json = $path
                source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
                message = $errorMessage
            }) | Out-Null
        }
    }

    $sourceFiles.Add([ordered]@{
        path = $path
        path_display = Get-DisplayPath -RepoRoot $repoRoot -Path $path
        kind = $kind
        status = $status
        error = $errorMessage
        input_docx = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "input_docx" }
        input_docx_display = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "input_docx_display" }
        template_name = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "template_name" }
        schema_target = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "schema_target" }
        target_mode = if ($null -eq $json) { "" } else { Get-JsonString -Object $provenance -Name "target_mode" }
    }) | Out-Null
}

$releaseBlockers = New-Object 'System.Collections.Generic.List[object]'
$actionItems = New-Object 'System.Collections.Generic.List[object]'

foreach ($issue in @($syncIssues.ToArray())) {
    $issueSourceJson = Get-JsonString -Object $issue -Name "source_json" -DefaultValue $summaryPath
    $issueSourceJsonDisplay = Get-JsonString -Object $issue -Name "source_json_display" -DefaultValue $summaryDisplay
    $issueInputDocx = Get-JsonString -Object $issue -Name "input_docx"
    $issueInputDocxDisplay = Get-JsonString -Object $issue -Name "input_docx_display"
    $issueTemplateName = Get-JsonString -Object $issue -Name "template_name"
    $issueSchemaTarget = Get-JsonString -Object $issue -Name "schema_target"
    $issueTargetMode = Get-JsonString -Object $issue -Name "target_mode"
    $releaseBlockers.Add((New-ReleaseBlocker `
                -Id "content_control_data_binding.custom_xml_sync_issue" `
                -SourceJson $issueSourceJson `
                -PartEntryName (Get-JsonString -Object $issue -Name "part_entry_name") `
                -ContentControlIndex (Get-JsonProperty -Object $issue -Name "content_control_index") `
                -Tag (Get-JsonString -Object $issue -Name "tag") `
                -Alias (Get-JsonString -Object $issue -Name "alias") `
                -StoreItemId (Get-JsonString -Object $issue -Name "store_item_id") `
                -XPath (Get-JsonString -Object $issue -Name "xpath") `
                -Status (Get-JsonString -Object $issue -Name "reason") `
                -Action "fix_custom_xml_data_binding_source" `
                -Message "Custom XML sync failed for a bound content control." `
                -RepairStrategy "fix_custom_xml_source" `
                -RepairHint "Fix the Custom XML source value or mapping, then rerun sync-content-controls-from-custom-xml." `
                -CommandTemplate (New-SyncContentControlCommandTemplate) `
                -SourceJsonDisplay $issueSourceJsonDisplay `
                -InputDocx $issueInputDocx `
                -InputDocxDisplay $issueInputDocxDisplay `
                -TemplateName $issueTemplateName `
                -SchemaTarget $issueSchemaTarget `
                -TargetMode $issueTargetMode)) | Out-Null
}

foreach ($control in @($contentControls.ToArray())) {
    $controlBindingKey = Get-JsonString -Object $control -Name "binding_key"
    $controlSourceJson = Get-JsonString -Object $control -Name "source_json" -DefaultValue $summaryPath
    $controlSourceJsonDisplay = Get-JsonString -Object $control -Name "source_json_display" -DefaultValue $summaryDisplay
    $controlPartEntryName = Get-JsonString -Object $control -Name "part_entry_name"
    $controlIndex = Get-JsonProperty -Object $control -Name "content_control_index"
    $controlTag = Get-JsonString -Object $control -Name "tag"
    $controlAlias = Get-JsonString -Object $control -Name "alias"
    $controlStoreItemId = Get-JsonString -Object $control -Name "store_item_id"
    $controlXPath = Get-JsonString -Object $control -Name "xpath"
    $controlLock = Get-JsonString -Object $control -Name "lock"
    $controlFormKind = Get-JsonString -Object $control -Name "form_kind"
    $controlInputDocx = Get-JsonString -Object $control -Name "input_docx"
    $controlInputDocxDisplay = Get-JsonString -Object $control -Name "input_docx_display"
    $controlTemplateName = Get-JsonString -Object $control -Name "template_name"
    $controlSchemaTarget = Get-JsonString -Object $control -Name "schema_target"
    $controlTargetMode = Get-JsonString -Object $control -Name "target_mode"
    $hasBinding = -not [string]::IsNullOrWhiteSpace($controlBindingKey)
    if ($hasBinding -and (Get-JsonBool -Object $control -Name "showing_placeholder")) {
        $releaseBlockers.Add((New-ReleaseBlocker `
                    -Id "content_control_data_binding.bound_placeholder" `
                    -SourceJson $controlSourceJson `
                    -PartEntryName $controlPartEntryName `
                    -ContentControlIndex $controlIndex `
                    -Tag $controlTag `
                    -Alias $controlAlias `
                    -StoreItemId $controlStoreItemId `
                    -XPath $controlXPath `
                    -Status "placeholder_visible" `
                    -Action "sync_or_fill_bound_content_control" `
                    -Message "A data-bound content control is still showing placeholder text." `
                    -RepairStrategy "sync_bound_content_control" `
                    -RepairHint "Rerun Custom XML sync or explicitly fill the bound content control before release." `
                    -CommandTemplate (New-SyncContentControlCommandTemplate) `
                    -SourceJsonDisplay $controlSourceJsonDisplay `
                    -InputDocx $controlInputDocx `
                    -InputDocxDisplay $controlInputDocxDisplay `
                    -TemplateName $controlTemplateName `
                    -SchemaTarget $controlSchemaTarget `
                    -TargetMode $controlTargetMode)) | Out-Null
    }

    if ($hasBinding -and -not [string]::IsNullOrWhiteSpace($controlLock)) {
        $actionItems.Add((New-ActionItem `
                    -Id "review_content_control_lock_strategy" `
                    -Action "review_content_control_lock_strategy" `
                    -Title "Review lock state for data-bound content control" `
                    -SourceJson $controlSourceJson `
                    -PartEntryName $controlPartEntryName `
                    -ContentControlIndex $controlIndex `
                    -Tag $controlTag `
                    -Alias $controlAlias `
                    -StoreItemId $controlStoreItemId `
                    -XPath $controlXPath `
                    -RepairStrategy "review_lock_state" `
                    -RepairHint "Confirm whether the lock is intentional; clear it only if template data should overwrite this control." `
                    -CommandTemplate (New-ClearLockCommandTemplate -Tag $controlTag -Alias $controlAlias) `
                    -SourceJsonDisplay $controlSourceJsonDisplay `
                    -InputDocx $controlInputDocx `
                    -InputDocxDisplay $controlInputDocxDisplay `
                    -TemplateName $controlTemplateName `
                    -SchemaTarget $controlSchemaTarget `
                    -TargetMode $controlTargetMode)) | Out-Null
    }

    if (-not $hasBinding -and $controlFormKind -notin @("", "rich_text", "plain_text")) {
        $actionItems.Add((New-ActionItem `
                    -Id "review_unbound_form_content_control" `
                    -Action "review_unbound_form_content_control" `
                    -Title "Review whether form content control should bind to template data" `
                    -SourceJson $controlSourceJson `
                    -PartEntryName $controlPartEntryName `
                    -ContentControlIndex $controlIndex `
                    -Tag $controlTag `
                    -Alias $controlAlias `
                    -StoreItemId "" `
                    -XPath "" `
                    -RepairStrategy "bind_or_exempt_form_control" `
                    -RepairHint "Bind the form control to a Custom XML path, or document why it is intentionally unbound." `
                    -CommandTemplate (New-BindContentControlCommandTemplate -Tag $controlTag -Alias $controlAlias) `
                    -SourceJsonDisplay $controlSourceJsonDisplay `
                    -InputDocx $controlInputDocx `
                    -InputDocxDisplay $controlInputDocxDisplay `
                    -TemplateName $controlTemplateName `
                    -SchemaTarget $controlSchemaTarget `
                    -TargetMode $controlTargetMode)) | Out-Null
    }
}

$bindingGroups = @($contentControls.ToArray() | Where-Object {
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "binding_key"))
    } | Group-Object { Get-JsonString -Object $_ -Name "binding_key" } | Where-Object { $_.Count -gt 1 })
foreach ($group in @($bindingGroups)) {
    $first = @($group.Group)[0]
    $firstSourceJson = Get-JsonString -Object $first -Name "source_json" -DefaultValue $summaryPath
    $firstSourceJsonDisplay = Get-JsonString -Object $first -Name "source_json_display" -DefaultValue $summaryDisplay
    $firstInputDocx = Get-JsonString -Object $first -Name "input_docx"
    $firstInputDocxDisplay = Get-JsonString -Object $first -Name "input_docx_display"
    $firstTemplateName = Get-JsonString -Object $first -Name "template_name"
    $firstSchemaTarget = Get-JsonString -Object $first -Name "schema_target"
    $firstTargetMode = Get-JsonString -Object $first -Name "target_mode"
    $duplicateMembers = @(
        $group.Group | ForEach-Object {
            [ordered]@{
                part_entry_name = Get-JsonString -Object $_ -Name "part_entry_name"
                content_control_index = Get-JsonProperty -Object $_ -Name "content_control_index"
                tag = Get-JsonString -Object $_ -Name "tag"
                alias = Get-JsonString -Object $_ -Name "alias"
                input_docx = Get-JsonString -Object $_ -Name "input_docx"
                input_docx_display = Get-JsonString -Object $_ -Name "input_docx_display"
                template_name = Get-JsonString -Object $_ -Name "template_name"
                schema_target = Get-JsonString -Object $_ -Name "schema_target"
                target_mode = Get-JsonString -Object $_ -Name "target_mode"
                source_json_display = Get-JsonString -Object $_ -Name "source_json_display" -DefaultValue $summaryDisplay
            }
        }
    )
    $actionItems.Add((New-ActionItem `
                -Id "review_duplicate_content_control_binding" `
                -Action "review_duplicate_content_control_binding" `
                -Title "Review repeated content controls that share one Custom XML binding" `
                -SourceJson $firstSourceJson `
                -PartEntryName (Get-JsonString -Object $first -Name "part_entry_name") `
                -ContentControlIndex (Get-JsonProperty -Object $first -Name "content_control_index") `
                -Tag (Get-JsonString -Object $first -Name "tag") `
                -Alias (Get-JsonString -Object $first -Name "alias") `
                -StoreItemId (Get-JsonString -Object $first -Name "store_item_id") `
                -XPath (Get-JsonString -Object $first -Name "xpath") `
                -RepairStrategy "deduplicate_or_confirm_shared_binding" `
                -RepairHint "Review the entire shared-binding group. Confirm the repeated binding is intentional, or split the controls across distinct Custom XML paths." `
                -CommandTemplate "featherdoc_cli inspect-content-controls <input.docx> --json" `
                -SourceJsonDisplay $firstSourceJsonDisplay `
                -DuplicateBindingKey ([string]$group.Name) `
                -DuplicateMemberCount ([int]$group.Count) `
                -DuplicateMembers @($duplicateMembers) `
                -InputDocx $firstInputDocx `
                -InputDocxDisplay $firstInputDocxDisplay `
                -TemplateName $firstTemplateName `
                -SchemaTarget $firstSchemaTarget `
                -TargetMode $firstTargetMode)) | Out-Null
}

$repairPlanItems = New-Object 'System.Collections.Generic.List[object]'
foreach ($blocker in @($releaseBlockers.ToArray())) {
    $repairItem = New-RepairPlanItem -SourceKind "release_blocker" -Item $blocker
    if ($null -ne $repairItem) { $repairPlanItems.Add($repairItem) | Out-Null }
}
foreach ($item in @($actionItems.ToArray())) {
    $repairItem = New-RepairPlanItem -SourceKind "action_item" -Item $item
    if ($null -ne $repairItem) { $repairPlanItems.Add($repairItem) | Out-Null }
}

$boundControls = @($contentControls.ToArray() | Where-Object {
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "binding_key"))
    })
$customXmlSyncEvidenceSourceJson = $summaryPath
$customXmlSyncEvidenceSourceJsonDisplay = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
$boundControlEvidence = @($boundControls | Where-Object {
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "source_json")) -and
        -not [string]::IsNullOrWhiteSpace((Get-JsonString -Object $_ -Name "source_json_display"))
    } | Select-Object -First 1)
if ($boundControlEvidence.Count -gt 0) {
    $selectedBoundControlEvidence = $boundControlEvidence[0]
    $customXmlSyncEvidenceSourceJson = Get-JsonString -Object $selectedBoundControlEvidence -Name "source_json" -DefaultValue $summaryPath
    $customXmlSyncEvidenceSourceJsonDisplay = Get-JsonString -Object $selectedBoundControlEvidence -Name "source_json_display" -DefaultValue $summaryDisplay
}
if ($contentControls.Count -eq 0 -and $syncItems.Count -eq 0 -and $syncIssues.Count -eq 0) {
    $warnings.Add([ordered]@{
        id = "content_control_binding_evidence_missing"
        action = "collect_content_control_data_binding_evidence"
        source_schema = $contentControlGovernanceSchema
        source_json = $summaryPath
        source_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        source_report = $summaryPath
        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        message = "No content-control inspection or Custom XML sync evidence was loaded."
    }) | Out-Null
}
if ($boundControls.Count -gt 0 -and $syncItems.Count -eq 0 -and $syncIssues.Count -eq 0) {
    $warnings.Add([ordered]@{
        id = "custom_xml_sync_evidence_missing"
        action = "run_content_control_custom_xml_sync"
        source_schema = $contentControlGovernanceSchema
        source_json = $customXmlSyncEvidenceSourceJson
        source_json_display = $customXmlSyncEvidenceSourceJsonDisplay
        source_report = $summaryPath
        source_report_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
        message = "Data-bound content controls were inspected, but no Custom XML sync result was provided."
    }) | Out-Null
}

$sourceFailureCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "failed" }).Count
$missingInputCount = @($sourceFiles.ToArray() | Where-Object { $_.status -eq "missing" }).Count
$status = if ($sourceFailureCount -gt 0) {
    "failed"
} elseif ($releaseBlockers.Count -gt 0) {
    "blocked"
} elseif ($warnings.Count -gt 0 -or $actionItems.Count -gt 0) {
    "needs_review"
} else {
    "ready"
}

$allEvidenceItems = @($contentControls.ToArray() + $syncItems.ToArray() + $syncIssues.ToArray())

$summary = [ordered]@{
    schema = $contentControlGovernanceSchema
    generated_at = (Get-Date).ToString("s")
    status = $status
    release_ready = ($status -eq "ready")
    output_dir = $resolvedOutputDir
    output_dir_display = Get-DisplayPath -RepoRoot $repoRoot -Path $resolvedOutputDir
    summary_json = $summaryPath
    summary_json_display = Get-DisplayPath -RepoRoot $repoRoot -Path $summaryPath
    report_markdown = $markdownPath
    report_markdown_display = Get-DisplayPath -RepoRoot $repoRoot -Path $markdownPath
    source_file_count = $sourceFiles.Count
    source_failure_count = $sourceFailureCount
    missing_input_count = $missingInputCount
    source_files = @($sourceFiles.ToArray())
    input_docx = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "input_docx"
    input_docx_display = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "input_docx_display"
    template_name = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "template_name"
    schema_target = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "schema_target"
    target_mode = Get-UniqueProvenanceValue -Items $allEvidenceItems -Name "target_mode"
    provenance_summary = @(New-ProvenanceSummary -Items $allEvidenceItems)
    inspection_file_count = @($sourceFiles.ToArray() | Where-Object { $_.kind -eq "inspect_content_controls" -and $_.status -eq "loaded" }).Count
    sync_file_count = @($sourceFiles.ToArray() | Where-Object { $_.kind -eq "custom_xml_sync_result" -and $_.status -eq "loaded" }).Count
    content_control_count = $contentControls.Count
    bound_content_control_count = $boundControls.Count
    placeholder_bound_content_control_count = @($boundControls | Where-Object { [bool]$_.showing_placeholder }).Count
    locked_bound_content_control_count = @($boundControls | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_.lock) }).Count
    duplicate_binding_count = @($bindingGroups).Count
    synced_content_control_count = $syncItems.Count
    sync_issue_count = $syncIssues.Count
    content_controls = @($contentControls.ToArray())
    sync_items = @($syncItems.ToArray())
    sync_issues = @($syncIssues.ToArray())
    binding_coverage_summary = @(New-BindingCoverageSummary -Items $contentControls.ToArray())
    form_kind_summary = @(Add-SummaryGroup -Items $contentControls.ToArray() -PropertyName "form_kind" -OutputName "form_kind")
    sync_issue_reason_summary = @(Add-SummaryGroup -Items $syncIssues.ToArray() -PropertyName "reason" -OutputName "reason")
    repair_plan_schema = "featherdoc.content_control_data_binding_repair_plan.v1"
    repair_plan_item_count = $repairPlanItems.Count
    repair_plan_apply_supported_count = @($repairPlanItems.ToArray() | Where-Object { [bool]$_.apply_supported }).Count
    repair_plan_native_dry_run_supported_count = @($repairPlanItems.ToArray() | Where-Object { [bool]$_.native_dry_run_supported }).Count
    repair_plan_requires_user_values_count = @($repairPlanItems.ToArray() | Where-Object { @(Get-JsonArray -Object $_ -Name "required_user_values").Count -gt 0 }).Count
    repair_plan_requires_visual_verification_count = @($repairPlanItems.ToArray() | Where-Object { [bool]$_.requires_visual_verification }).Count
    repair_plan_status_summary = @(Add-SummaryGroup -Items $repairPlanItems.ToArray() -PropertyName "plan_status" -OutputName "plan_status")
    repair_plan_items = @($repairPlanItems.ToArray())
    release_blocker_count = $releaseBlockers.Count
    release_blockers = @($releaseBlockers.ToArray())
    blocker_id_summary = @(Add-SummaryGroup -Items $releaseBlockers.ToArray() -PropertyName "id" -OutputName "id")
    action_item_count = $actionItems.Count
    action_items = @($actionItems.ToArray())
    action_item_summary = @(Add-SummaryGroup -Items $actionItems.ToArray() -PropertyName "action" -OutputName "action")
    warning_count = $warnings.Count
    warnings = @($warnings.ToArray())
}

($summary | ConvertTo-Json -Depth 32) | Set-Content -LiteralPath $summaryPath -Encoding UTF8
(New-ReportMarkdown -Summary $summary) | Set-Content -LiteralPath $markdownPath -Encoding UTF8

Write-Step "Summary JSON: $summaryPath"
Write-Step "Report Markdown: $markdownPath"
Write-Step "Status: $status"

if ($sourceFailureCount -gt 0) { exit 1 }
if ($FailOnWarning -and $warnings.Count -gt 0) { exit 1 }
if ($FailOnBlocker -and $releaseBlockers.Count -gt 0) { exit 1 }
