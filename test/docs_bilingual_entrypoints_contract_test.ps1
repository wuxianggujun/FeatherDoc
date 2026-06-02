param(
    [string]$RepoRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-ContainsText {
    param(
        [string]$Text,
        [string]$ExpectedText,
        [string]$Message
    )

    if ($Text -notmatch [regex]::Escape($ExpectedText)) {
        throw "$Message Missing='$ExpectedText'."
    }
}

function Assert-DoesNotContainText {
    param(
        [string]$Text,
        [string]$UnexpectedText,
        [string]$Message
    )

    if ($Text.Contains($UnexpectedText)) {
        throw "$Message Unexpected='$UnexpectedText'."
    }
}

function Assert-RepoFileMissing {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (Test-Path -LiteralPath $path) {
        throw "Legacy docs file should have been removed: $RelativePath"
    }
}

function Get-RepoFileText {
    param(
        [string]$Root,
        [string]$RelativePath
    )

    $path = Join-Path $Root $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Expected contract file was not found: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

if ([string]::IsNullOrWhiteSpace($RepoRoot)) {
    throw "RepoRoot is required."
}

$resolvedRepoRoot = (Resolve-Path $RepoRoot).Path

$rootIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\index.rst"
$docsConf = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\conf.py"
$englishIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\index.rst"
$englishGettingStarted = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\getting_started.rst"
$englishApiIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\index.rst"
$englishDocumentApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\document.rst"
$englishParagraphRunApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\paragraph_run.rst"
$englishTableApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\table.rst"
$englishImagesApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\images.rst"
$englishSectionsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\sections.rst"
$englishFieldsLinksReviewsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\fields_links_reviews.rst"
$englishStylesNumberingApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\styles_numbering.rst"
$englishTemplatePartApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\template_part.rst"
$englishEditPlanOperationsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\edit_plan_operations.rst"
$englishPdfWorkflowApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\pdf_workflow.rst"
$englishEnumsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\enums.rst"
$chineseIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\index.rst"
$chineseGettingStarted = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\getting_started.rst"
$chineseApiIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\index.rst"
$chineseDocumentApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\document.rst"
$chineseParagraphRunApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\paragraph_run.rst"
$chineseTableApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\table.rst"
$chineseImagesApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\images.rst"
$chineseSectionsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\sections.rst"
$chineseFieldsLinksReviewsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\fields_links_reviews.rst"
$chineseStylesNumberingApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\styles_numbering.rst"
$chineseTemplatePartApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\template_part.rst"
$chineseEditPlanOperationsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\edit_plan_operations.rst"
$chinesePdfWorkflowApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\pdf_workflow.rst"
$chineseEnumsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\enums.rst"
$readme = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.md"
$readmeZh = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.zh-CN.md"
$themeLayout = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\_themes\armstrong\layout.html"
$themeCss = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\_themes\armstrong\static\rtd.css_t"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

foreach ($marker in @(
        'data-featherdoc-language-switch="global"',
        'data-featherdoc-language="en"',
        'data-featherdoc-language="zh-CN"',
        'data-featherdoc-api-language="en"',
        'data-featherdoc-api-language="zh-CN"',
        'data-featherdoc-language-nav="no-auto-relations"',
        '{% block linktags %}',
        '{% block sidebarrel %}{% endblock %}',
        'featherdoc_current_lang'
    )) {
    Assert-ContainsText -Text $themeLayout -ExpectedText $marker `
        -Message "Theme layout should expose a stable global language/API switch marker."
}

foreach ($marker in @(
        ".featherdoc-language-switch a.is-active",
        "theme_medium_color"
    )) {
    Assert-ContainsText -Text $themeCss -ExpectedText $marker `
        -Message "Theme CSS should highlight the active documentation language."
}

foreach ($marker in @(
        "html_sidebars",
        "'localtoc.html'",
        "'sourcelink.html'",
        "'searchbox.html'"
    )) {
    Assert-ContainsText -Text $docsConf -ExpectedText $marker `
        -Message "Sphinx config should keep sidebars free of automatic previous/next relations."
}

Assert-DoesNotContainText -Text $docsConf -UnexpectedText "'relations.html'" `
    -Message "Sphinx config should not render automatic previous/next relation sidebars."

foreach ($marker in @(
        "Choose a language entry point",
        "en/index",
        "zh-CN/index",
        "Languages",
        "en/api/index",
        "zh-CN/api/index"
    )) {
    Assert-ContainsText -Text $rootIndex -ExpectedText $marker `
        -Message "Root docs index should preserve bilingual landing markers."
}

foreach ($marker in @(
        "Compatibility",
        "Older root-level pages",
        "getting_started",
        "pdf_export",
        "pdf_import",
        ":hidden:"
    )) {
    Assert-DoesNotContainText -Text $rootIndex -UnexpectedText $marker `
        -Message "Root docs index should no longer preserve legacy compatibility navigation."
}

foreach ($legacyPath in @(
        "docs\api\index.rst",
        "docs\api\document.rst",
        "docs\api\edit_plan_operations.rst",
        "docs\getting_started.rst",
        "docs\pdf_export.rst",
        "docs\pdf_import.rst",
        "docs\pdf_import_json_diagnostics.rst",
        "docs\pdf_import_scope.rst"
    )) {
    Assert-RepoFileMissing -Root $resolvedRepoRoot -RelativePath $legacyPath
}

foreach ($marker in @(
        "../zh-CN/index",
        "getting_started",
        "api/index",
        "api/pdf_workflow"
    )) {
    Assert-ContainsText -Text $englishIndex -ExpectedText $marker `
        -Message "English docs entrypoint should preserve navigation marker."
}

foreach ($marker in @(
        "Install And Build",
        "Minimal C++ Usage",
        "api/document",
        "api/paragraph_run",
        "api/table",
        "api/template_part",
        "api/pdf_workflow"
    )) {
    Assert-ContainsText -Text $englishGettingStarted -ExpectedText $marker `
        -Message "English getting-started page should preserve language-local quick-start marker."
}

foreach ($marker in @(
        "featherdoc::Document",
        "document",
        "paragraph_run",
        "table",
        "images",
        "sections",
        "fields_links_reviews",
        "styles_numbering",
        "template_part",
        "edit_plan_operations",
        "pdf_workflow",
        "enums"
    )) {
    Assert-ContainsText -Text $englishApiIndex -ExpectedText $marker `
        -Message "English API entrypoint should preserve language-local API marker."
}

foreach ($marker in @(
        "featherdoc::Document",
        "Lifecycle",
        "Parameters",
        "path() const",
        "last_error() const noexcept",
        "enable_update_fields_on_open()",
        "Template Part Access",
        "Sections And Inspection",
        "move_section",
        "replace_section_footer_text",
        "remove_section_footer_reference",
        "section_page_setup",
        "reference_kind",
        "replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)",
        "list_content_controls() const",
        "replace_content_control_with_table_rows_by_tag",
        "bookmark_fill_result",
        "append_table",
        "append_floating_image",
        "append_paragraph_text_comment",
        "append_text_range_comment",
        "Related API Families",
        "fields_links_reviews",
        "open()",
        "save_as"
    )) {
    Assert-ContainsText -Text $englishDocumentApi -ExpectedText $marker `
        -Message "English Document API page should preserve method-table marker."
}
Assert-DoesNotContainText -Text $englishDocumentApi -UnexpectedText "../../api/document" `
    -Message "English Document API should not link to removed legacy API pages."

foreach ($pair in @(
        @{ Text = $englishParagraphRunApi; Markers = @("featherdoc::Paragraph", "featherdoc::Run", "set_alignment", "set_font_family", "set_rtl") },
        @{ Text = $englishTableApi; Markers = @("featherdoc::Table", "TableRow", "TableCell", "find_cell_by_grid_column", "set_cell_block_texts", "set_repeats_header", "merge_right", "set_fill_color") },
        @{ Text = $englishImagesApi; Markers = @("featherdoc::inline_image_info", "featherdoc::drawing_image_info", "append_image", "append_floating_image", "extract_drawing_image", "replace_inline_image", "remove_inline_image") },
        @{ Text = $englishSectionsApi; Markers = @("featherdoc::section_page_setup", "featherdoc::page_margins", "section_count", "append_section", "inspect_section", "get_section_page_setup", "set_section_page_setup") },
        @{ Text = $englishFieldsLinksReviewsApi; Markers = @("featherdoc::field_summary", "featherdoc::hyperlink_summary", "featherdoc::review_note_summary", "list_fields", "append_field", "append_hyperlink", "list_comments", "accept_all_revisions") },
        @{ Text = $englishStylesNumberingApi; Markers = @("numbering_definition_summary", "numbering_catalog", "style_summary", "list_numbering_definitions", "export_numbering_catalog", "list_styles", "suggest_style_merges", "audit_table_style_quality") },
        @{ Text = $englishTemplatePartApi; Markers = @("featherdoc::TemplatePart", "Part Basics", "Bookmarks", "Content Controls", "Template Schema", "replace_content_control_text_by_tag", "validate_template_schema", "onboard_template") }
    )) {
    foreach ($marker in $pair.Markers) {
        Assert-ContainsText -Text $pair.Text -ExpectedText $marker `
            -Message "English API page should preserve method-table marker."
    }
    Assert-DoesNotContainText -Text $pair.Text -UnexpectedText "../../api/" `
        -Message "English API pages should not link to removed legacy API pages."
}

foreach ($marker in @(
        "scripts/edit_document_from_plan.ps1",
        "FDOC_EDIT_PLAN_PLAN_SHAPE",
        "FDOC_EDIT_PLAN_EXECUTION_RESULT",
        "FDOC_EDIT_PLAN_OPERATION_REFERENCE",
        "FDOC_EDIT_PLAN_JSON_EXAMPLES",
        "accept_all_revisions",
        "apply_review_mutation_plan",
        "append_page_number_field",
        "replace_content_control_text_by_tag",
        "append_image",
        "append_hyperlink",
        "set_paragraph_alignment",
        "apply_table_position_plan",
        "import_numbering_catalog",
        "unmerge_table_cell"
    )) {
    Assert-ContainsText -Text $englishEditPlanOperationsApi -ExpectedText $marker `
        -Message "English Edit Plan Operations API page should preserve operation marker."
}
Assert-DoesNotContainText -Text $englishEditPlanOperationsApi -UnexpectedText "../../api/edit_plan_operations" `
    -Message "English Edit Plan Operations API should not link to removed legacy API pages."

foreach ($marker in @(
        "PDF Workflows",
        "FEATHERDOC_BUILD_PDF",
        "FEATHERDOC_BUILD_PDF_IMPORT",
        "featherdoc_cli export-pdf input.docx --output output.pdf --json",
        "--render-headers-and-footers",
        "--expand-header-footer-page-placeholders",
        "--summary-json <path>",
        "Failure JSON",
        '"stage": "parse"',
        "featherdoc_cli import-pdf input.pdf --output imported.docx --json",
        "--import-table-candidates-as-tables",
        "--min-table-continuation-confidence",
        "table_continuation_diagnostics_count",
        "table_continuation_diagnostics",
        "column_anchors_mismatch",
        "continuation_confidence_below_threshold",
        "inconsistent_source_rows",
        "parse_failed",
        "document_create_failed",
        "extract_geometry_disabled",
        "table_candidates_detected",
        "Paragraph import from extractable PDF text",
        "not a general PDF-to-Word clone"
    )) {
    Assert-ContainsText -Text $englishPdfWorkflowApi -ExpectedText $marker `
        -Message "English PDF workflow API page should preserve PDF import/export marker."
}

foreach ($marker in @(
        "formatting_flag",
        "section_reference_kind",
        "page_orientation",
        "paragraph_alignment",
        "table_layout_mode",
        "bookmark_kind",
        "content_control_kind",
        "field_kind",
        "review_note_kind",
        "revision_kind"
    )) {
    Assert-ContainsText -Text $englishEnumsApi -ExpectedText $marker `
        -Message "English Enums API page should preserve enum marker."
}
Assert-DoesNotContainText -Text $englishEnumsApi -UnexpectedText "../../api/enums" `
    -Message "English Enums API should not link to removed legacy API pages."

foreach ($marker in @(
        "../en/index",
        "getting_started",
        "api/index",
        "api/pdf_workflow"
    )) {
    Assert-ContainsText -Text $chineseIndex -ExpectedText $marker `
        -Message "Chinese docs entrypoint should preserve navigation marker."
}

foreach ($marker in @(
        '../visual_validation_zh',
        '../governance_routes_zh',
        '../release_policy_zh',
        '../project_identity_zh',
        '../script_task_index_zh',
        '../pdf_export',
        '../pdf_import'
    )) {
    Assert-DoesNotContainText -Text $chineseIndex -UnexpectedText $marker `
        -Message "Chinese docs entrypoint should keep legacy topic docs out of the language-local navigation tree."
}

foreach ($marker in @(
        "cmake -S . -B build",
        "BUILD_CLI",
        "featherdoc::Document",
        "api/document",
        "api/paragraph_run",
        "api/table",
        "api/template_part",
        "api/pdf_workflow"
    )) {
    Assert-ContainsText -Text $chineseGettingStarted -ExpectedText $marker `
        -Message "Chinese getting-started page should preserve language-local quick-start marker."
}

foreach ($marker in @(
        "../../en/api/index",
        "featherdoc::Document",
        "document",
        "paragraph_run",
        "table",
        "images",
        "sections",
        "fields_links_reviews",
        "styles_numbering",
        "template_part",
        "edit_plan_operations",
        "pdf_workflow",
        "enums"
    )) {
    Assert-ContainsText -Text $chineseApiIndex -ExpectedText $marker `
        -Message "Chinese API entrypoint should preserve mirrored API marker."
}

foreach ($marker in @(
        "featherdoc::Document",
        "path() const",
        "last_error() const noexcept",
        "enable_update_fields_on_open()",
        "TemplatePart",
        "sections_inspection_summary",
        "move_section",
        "replace_section_footer_text",
        "remove_section_footer_reference",
        "section_page_setup",
        "reference_kind",
        "replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)",
        "list_content_controls() const",
        "replace_content_control_with_table_rows_by_tag",
        "bookmark_fill_result",
        "append_table",
        "append_floating_image",
        "append_paragraph_text_comment",
        "append_text_range_comment",
        "fields_links_reviews",
        "compare_semantic",
        "open()",
        "save_as"
    )) {
    Assert-ContainsText -Text $chineseDocumentApi -ExpectedText $marker `
        -Message "Chinese Document API page should preserve method-table marker."
}
Assert-DoesNotContainText -Text $chineseDocumentApi -UnexpectedText "../../api/document" `
    -Message "Chinese Document API should not link to removed legacy API pages."

foreach ($pair in @(
        @{ Text = $chineseParagraphRunApi; Markers = @("featherdoc::Paragraph", "featherdoc::Run", "set_alignment", "set_font_family", "set_rtl", "insert_paragraph_after", "insert_run_after") },
        @{ Text = $chineseTableApi; Markers = @("featherdoc::Table", "TableRow", "TableCell", "find_cell_by_grid_column", "set_cell_block_texts", "set_repeats_header", "merge_right", "set_fill_color") },
        @{ Text = $chineseImagesApi; Markers = @("featherdoc::inline_image_info", "featherdoc::drawing_image_info", "floating_image_options", "append_image", "append_floating_image", "extract_drawing_image", "replace_inline_image", "remove_inline_image") },
        @{ Text = $chineseSectionsApi; Markers = @("featherdoc::section_page_setup", "featherdoc::page_margins", "sections_inspection_summary", "section_count", "append_section", "inspect_section", "get_section_page_setup", "set_section_page_setup") },
        @{ Text = $chineseFieldsLinksReviewsApi; Markers = @("featherdoc::field_summary", "featherdoc::hyperlink_summary", "featherdoc::review_note_summary", "list_fields", "append_field", "append_hyperlink", "list_comments", "accept_all_revisions") },
        @{ Text = $chineseStylesNumberingApi; Markers = @("numbering_definition_summary", "numbering_catalog", "style_summary", "style_usage_report", "list_numbering_definitions", "export_numbering_catalog", "list_styles", "suggest_style_merges", "audit_table_style_quality") },
        @{ Text = $chineseTemplatePartApi; Markers = @("featherdoc::TemplatePart", "entry_name()", "fill_bookmarks", "replace_content_control_text_by_tag", "validate_template_schema", "onboard_template") }
    )) {
    foreach ($marker in $pair.Markers) {
        Assert-ContainsText -Text $pair.Text -ExpectedText $marker `
            -Message "Chinese API page should preserve method-table marker."
    }
    Assert-DoesNotContainText -Text $pair.Text -UnexpectedText "../../api/" `
        -Message "Chinese API pages should not link to removed legacy API pages."
}

foreach ($marker in @(
        "scripts/edit_document_from_plan.ps1",
        "FDOC_EDIT_PLAN_PLAN_SHAPE",
        "FDOC_EDIT_PLAN_EXECUTION_RESULT",
        "FDOC_EDIT_PLAN_OPERATION_REFERENCE",
        "FDOC_EDIT_PLAN_JSON_EXAMPLES",
        "accept_all_revisions",
        "apply_review_mutation_plan",
        "append_page_number_field",
        "replace_content_control_text_by_tag",
        "append_image",
        "append_hyperlink",
        "set_paragraph_alignment",
        "apply_table_position_plan",
        "import_numbering_catalog",
        "unmerge_table_cell"
    )) {
    Assert-ContainsText -Text $chineseEditPlanOperationsApi -ExpectedText $marker `
        -Message "Chinese Edit Plan Operations API page should preserve operation marker."
}
Assert-DoesNotContainText -Text $chineseEditPlanOperationsApi -UnexpectedText "../../api/edit_plan_operations" `
    -Message "Chinese Edit Plan Operations API should not link to removed legacy API pages."

foreach ($marker in @(
        "PDF",
        "FEATHERDOC_BUILD_PDF",
        "FEATHERDOC_BUILD_PDF_IMPORT",
        "featherdoc_cli export-pdf input.docx --output output.pdf --json",
        "--render-headers-and-footers",
        "--expand-header-footer-page-placeholders",
        "--summary-json <path>",
        '"stage": "parse"',
        "featherdoc_cli import-pdf input.pdf --output imported.docx --json",
        "--import-table-candidates-as-tables",
        "--min-table-continuation-confidence",
        "table_continuation_diagnostics_count",
        "table_continuation_diagnostics",
        "column_anchors_mismatch",
        "continuation_confidence_below_threshold",
        "inconsistent_source_rows",
        "parse_failed",
        "document_create_failed",
        "extract_geometry_disabled",
        "table_candidates_detected"
    )) {
    Assert-ContainsText -Text $chinesePdfWorkflowApi -ExpectedText $marker `
        -Message "Chinese PDF workflow API page should preserve PDF import/export marker."
}

foreach ($marker in @(
        "formatting_flag",
        "section_reference_kind",
        "page_orientation",
        "paragraph_alignment",
        "table_layout_mode",
        "bookmark_kind",
        "content_control_kind",
        "field_kind",
        "review_note_kind",
        "revision_kind"
    )) {
    Assert-ContainsText -Text $chineseEnumsApi -ExpectedText $marker `
        -Message "Chinese Enums API page should preserve enum marker."
}
Assert-DoesNotContainText -Text $chineseEnumsApi -UnexpectedText "../../api/enums" `
    -Message "Chinese Enums API should not link to removed legacy API pages."

foreach ($marker in @(
        "https://wuxianggujun.github.io/FeatherDoc/en/",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/",
        "https://wuxianggujun.github.io/FeatherDoc/en/api/",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/api/",
        "https://wuxianggujun.github.io/FeatherDoc/en/getting_started.html",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/getting_started.html",
        "https://wuxianggujun.github.io/FeatherDoc/en/api/pdf_workflow.html",
        "https://shop.input.im/?code=fbe6f3d5",
        "sponsor/zhifubao.jpg",
        "sponsor/weixin.png"
    )) {
    Assert-ContainsText -Text $readme -ExpectedText $marker `
        -Message "English README should preserve docs, promotion, and sponsor markers."
}
Assert-DoesNotContainText -Text $readme -UnexpectedText "https://wuxianggujun.github.io/FeatherDoc/api/" `
    -Message "English README should not link to removed legacy API docs."

foreach ($marker in @(
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/",
        "https://wuxianggujun.github.io/FeatherDoc/en/",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/api/",
        "https://wuxianggujun.github.io/FeatherDoc/en/api/",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/getting_started.html",
        "https://wuxianggujun.github.io/FeatherDoc/en/getting_started.html",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/api/pdf_workflow.html",
        "https://shop.input.im/?code=fbe6f3d5",
        "sponsor/zhifubao.jpg",
        "sponsor/weixin.png"
    )) {
    Assert-ContainsText -Text $readmeZh -ExpectedText $marker `
        -Message "Chinese README should preserve docs, promotion, and sponsor markers."
}
Assert-DoesNotContainText -Text $readmeZh -UnexpectedText "https://wuxianggujun.github.io/FeatherDoc/api/" `
    -Message "Chinese README should not link to removed legacy API docs."

foreach ($marker in @(
        "docs_bilingual_entrypoints_contract",
        "docs_bilingual_entrypoints_contract_test.ps1",
        "TIMEOUT 60",
        'LABELS "docs;smoke;bilingual"'
    )) {
    Assert-ContainsText -Text $cmakeLists -ExpectedText $marker `
        -Message "CMake should keep bilingual docs contract registered."
}

Write-Host "Docs bilingual entrypoints contract passed."
