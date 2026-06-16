if ($case.id -eq "ensure-paragraph-style-heading-visual") {
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_assert_heading.ps1")
} elseif ($case.id -eq "materialize-style-run-properties-child-freeze-visual") {
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_assert_materialize.ps1")
} elseif ($case.id -eq "rebase-paragraph-style-based-on-child-preserve-visual") {
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_assert_rebase_paragraph.ps1")
} elseif ($case.id -eq "rebase-character-style-based-on-child-preserve-visual") {
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_assert_rebase_character.ps1")
} elseif ($case.id -eq "ensure-character-style-serif-visual") {
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_assert_character.ps1")
} elseif ($case.id -eq "ensure-table-style-borderless-visual") {
    . (Join-Path $PSScriptRoot "run_ensure_style_visual_regression_case_assert_table.ps1")
}
