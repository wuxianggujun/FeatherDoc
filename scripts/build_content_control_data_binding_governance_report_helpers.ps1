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
    $commandTemplate = Get-JsonString -Object $Item -Name "command_template"
    $repairActionClasses = Get-RepairActionClasses `
        -SourceKind $SourceKind `
        -PlanStatus $planStatus `
        -ApplySupported $applySupported `
        -RequiredUserValues $requiredUserValues `
        -RequiresVisualVerification $requiresVisualVerification `
        -CommandTemplate $commandTemplate

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
        command_template = $commandTemplate
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
        repair_action_classes = @($repairActionClasses)
    }
}

function Get-RepairActionClasses {
    param(
        [string]$SourceKind,
        [string]$PlanStatus,
        [bool]$ApplySupported,
        [object[]]$RequiredUserValues,
        [bool]$RequiresVisualVerification,
        [string]$CommandTemplate
    )

    $classes = New-Object 'System.Collections.Generic.List[string]'
    $requiredUserValueCount = [int](($RequiredUserValues | Measure-Object).Count)
    if ($SourceKind -eq "release_blocker") {
        $classes.Add("release_blocking") | Out-Null
    }
    if ($ApplySupported -and
        $requiredUserValueCount -eq 0 -and
        -not [string]::IsNullOrWhiteSpace($CommandTemplate)) {
        $classes.Add("auto_repair_candidate") | Out-Null
    }
    if ($RequiresVisualVerification -or
        $requiredUserValueCount -gt 0 -or
        $PlanStatus -in @("source_fix_required", "review_only")) {
        $classes.Add("manual_confirmation_required") | Out-Null
    }
    if ($classes.Count -eq 0) {
        $classes.Add("manual_confirmation_required") | Out-Null
    }

    return @($classes.ToArray() | Sort-Object -Unique)
}

function New-RepairActionClassSummary {
    param([object[]]$Items)

    $classes = @(
        "release_blocking",
        "auto_repair_candidate",
        "manual_confirmation_required"
    )

    return @(
        foreach ($class in $classes) {
            [ordered]@{
                repair_action_class = $class
                count = @($Items | Where-Object {
                        @(Get-JsonArray -Object $_ -Name "repair_action_classes") -contains $class
                    }).Count
            }
        }
    )
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
    $lines.Add("- Release-blocking repair actions: ``$($Summary.repair_action_release_blocking_count)``") | Out-Null
    $lines.Add("- Auto-repair candidate actions: ``$($Summary.repair_action_auto_repair_candidate_count)``") | Out-Null
    $lines.Add("- Manual-confirmation repair actions: ``$($Summary.repair_action_manual_confirmation_required_count)``") | Out-Null
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
    $lines.Add("## Repair Action Classes") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.repair_action_class_summary).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($entry in @($Summary.repair_action_class_summary)) {
            $lines.Add("- ``$($entry.repair_action_class)``: ``$($entry.count)``") | Out-Null
        }
    }
    $lines.Add("") | Out-Null
    $lines.Add("## Repair Plan Feasibility") | Out-Null
    $lines.Add("") | Out-Null
    if (@($Summary.repair_plan_items).Count -eq 0) {
        $lines.Add("- none") | Out-Null
    } else {
        foreach ($item in @($Summary.repair_plan_items)) {
            $repairActionClasses = @(Get-JsonArray -Object $item -Name "repair_action_classes")
            $repairActionClassText = if ($repairActionClasses.Count -gt 0) { $repairActionClasses -join "," } else { "(none)" }
            $lines.Add("- ``$($item.source_id)``: source=``$($item.source_kind)`` strategy=``$($item.repair_strategy)`` status=``$($item.plan_status)`` classes=``$repairActionClassText`` apply_supported=``$($item.apply_supported)`` native_dry_run_supported=``$($item.native_dry_run_supported)`` requires_visual_verification=``$($item.requires_visual_verification)`` source_report_display=``$($item.source_report_display)`` source_json_display=``$($item.source_json_display)``") | Out-Null
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
            $repairStrategy = Get-JsonString -Object $warning -Name "repair_strategy"
            $repairHint = Get-JsonString -Object $warning -Name "repair_hint"
            $commandTemplate = Get-JsonString -Object $warning -Name "command_template"
            if (-not [string]::IsNullOrWhiteSpace($repairStrategy)) {
                $lines.Add("  - repair_strategy: ``$repairStrategy``") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($repairHint)) {
                $lines.Add("  - repair_hint: $repairHint") | Out-Null
            }
            if (-not [string]::IsNullOrWhiteSpace($commandTemplate)) {
                $lines.Add("  - command_template: ``$commandTemplate``") | Out-Null
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
