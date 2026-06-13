function Invoke-EditPlanSectionOperationArguments {
    param(
        $Operation,
        [string]$InputPath,
        [string]$OutputPath,
        [string]$Label,
        [int]$OperationIndex,
        [string]$TemporaryRoot,
        [string]$NormalizedOp,
        [System.Collections.Generic.List[string]]$Arguments
    )

    switch ($NormalizedOp) {
        "set_section_page_setup" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("set-section-page-setup") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $null = Add-SectionPageSetupArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label
        }
        "insert_section" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("insert-section") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $noInherit = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "no_inherit" `
                -DefaultValue $false
            $inheritHeaderFooter = Get-OptionalBooleanPropertyValue `
                -Object $Operation `
                -Name "inherit_header_footer" `
                -DefaultValue $true
            if ($noInherit -or (-not $inheritHeaderFooter)) {
                $arguments.Add("--no-inherit") | Out-Null
            }
        }
        "remove_section" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("remove-section") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
        }
        "delete_section" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("remove-section") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
        }
        "move_section" {
            $sourceIndex = Get-SourceIndexValue -Operation $Operation -Label $Label
            $targetIndex = Get-TargetIndexValue -Operation $Operation -Label $Label
            $arguments.Add("move-section") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceIndex) | Out-Null
            $arguments.Add($targetIndex) | Out-Null
        }
        "copy_section_layout" {
            $sourceIndex = Get-SourceIndexValue -Operation $Operation -Label $Label
            $targetIndex = Get-TargetIndexValue -Operation $Operation -Label $Label
            $arguments.Add("copy-section-layout") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceIndex) | Out-Null
            $arguments.Add($targetIndex) | Out-Null
        }
        "set_section_header" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("set-section-header") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            Add-SectionTextArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "set_section_footer" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("set-section-footer") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            Add-SectionTextArguments `
                -Arguments $arguments `
                -Operation $Operation `
                -Label $Label `
                -TemporaryRoot $TemporaryRoot `
                -OperationIndex $OperationIndex
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "assign_section_header" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $partIndex = Get-SectionPartIndexValue `
                -Operation $Operation `
                -Label $Label `
                -Family "header"
            $arguments.Add("assign-section-header") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $arguments.Add($partIndex) | Out-Null
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "assign_section_footer" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $partIndex = Get-SectionPartIndexValue `
                -Operation $Operation `
                -Label $Label `
                -Family "footer"
            $arguments.Add("assign-section-footer") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $arguments.Add($partIndex) | Out-Null
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "remove_section_header" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("remove-section-header") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "remove_section_footer" {
            $sectionIndex = Get-SectionIndexValue -Operation $Operation -Label $Label
            $arguments.Add("remove-section-footer") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sectionIndex) | Out-Null
            $null = Add-SectionPartKindArgument -Arguments $arguments -Operation $Operation
        }
        "remove_header_part" {
            $partIndex = Get-SectionPartIndexValue `
                -Operation $Operation `
                -Label $Label `
                -Family "header"
            $arguments.Add("remove-header-part") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($partIndex) | Out-Null
        }
        "remove_footer_part" {
            $partIndex = Get-SectionPartIndexValue `
                -Operation $Operation `
                -Label $Label `
                -Family "footer"
            $arguments.Add("remove-footer-part") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($partIndex) | Out-Null
        }
        "move_header_part" {
            $sourceIndex = Get-SourceIndexValue -Operation $Operation -Label $Label
            $targetIndex = Get-TargetIndexValue -Operation $Operation -Label $Label
            $arguments.Add("move-header-part") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceIndex) | Out-Null
            $arguments.Add($targetIndex) | Out-Null
        }
        "move_footer_part" {
            $sourceIndex = Get-SourceIndexValue -Operation $Operation -Label $Label
            $targetIndex = Get-TargetIndexValue -Operation $Operation -Label $Label
            $arguments.Add("move-footer-part") | Out-Null
            $arguments.Add($InputPath) | Out-Null
            $arguments.Add($sourceIndex) | Out-Null
            $arguments.Add($targetIndex) | Out-Null
        }
        default {
            return $false
        }
    }

    return $true
}
