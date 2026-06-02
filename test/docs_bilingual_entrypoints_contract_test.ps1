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
        "Older root-level pages are still built",
        "en/index",
        "zh-CN/index",
        "Languages",
        "Compatibility",
        "en/api/index",
        "zh-CN/api/index",
        "api/index",
        "getting_started",
        "pdf_export",
        "pdf_import_scope"
    )) {
    Assert-ContainsText -Text $rootIndex -ExpectedText $marker `
        -Message "Root docs index should preserve bilingual landing markers."
}

foreach ($marker in @(
        ':doc:`api/index`',
        ':doc:`getting_started`',
        ':doc:`pdf_export`',
        ':doc:`pdf_import`'
    )) {
    Assert-DoesNotContainText -Text $rootIndex -UnexpectedText $marker `
        -Message "Root docs index should keep legacy docs out of visible body links."
}

foreach ($marker in @(
        "../zh-CN/index",
        "getting_started",
        "api/index"
    )) {
    Assert-ContainsText -Text $englishIndex -ExpectedText $marker `
        -Message "English docs entrypoint should preserve navigation marker."
}

foreach ($marker in @(
        ':doc:`../api/index`',
        ':doc:`../pdf_export`',
        ':doc:`../pdf_import`'
    )) {
    Assert-DoesNotContainText -Text $englishIndex -UnexpectedText $marker `
        -Message "English docs entrypoint should keep legacy docs in the hidden compatibility toctree."
}

foreach ($marker in @(
        "Install And Build",
        "Minimal C++ Usage",
        "api/document",
        "../api/content_blocks",
        "../pdf_export"
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
        "enums"
    )) {
    Assert-ContainsText -Text $englishApiIndex -ExpectedText $marker `
        -Message "English API entrypoint should preserve language-local API marker."
}

foreach ($marker in @(
        "featherdoc::Document",
        "Lifecycle",
        "FDOC_DOCUMENT_API_PARAMETERS",
        "Parameters",
        "path() const",
        "last_error() const noexcept",
        "enable_update_fields_on_open()",
        "Template Part Access",
        "Sections And Inspection",
        "FDOC_DOCUMENT_API_SECTIONS",
        "move_section",
        "replace_section_footer_text",
        "remove_section_footer_reference",
        "section_page_setup",
        "reference_kind",
        "FDOC_DOCUMENT_API_TEMPLATE_FILLING",
        "replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)",
        "list_content_controls() const",
        "replace_content_control_with_table_rows_by_tag",
        "bookmark_fill_result",
        "FDOC_DOCUMENT_API_BODY_TABLE_IMAGE",
        "append_table",
        "append_floating_image",
        "FDOC_DOCUMENT_API_REVIEW_LINKS",
        "append_paragraph_text_comment",
        "append_text_range_comment",
        "FDOC_DOCUMENT_API_RELATED_FAMILIES",
        "Related API Families",
        "fields_links_reviews",
        "open()",
        "save_as",
        "../../api/document"
    )) {
    Assert-ContainsText -Text $englishDocumentApi -ExpectedText $marker `
        -Message "English Document API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::Paragraph",
        "featherdoc::Run",
        "Paragraph Text And Iteration",
        "Paragraph Layout",
        "Run Text And Styling",
        "Run Language And Direction",
        "set_alignment",
        "set_font_family",
        "set_rtl",
        "../../api/content_blocks"
    )) {
    Assert-ContainsText -Text $englishParagraphRunApi -ExpectedText $marker `
        -Message "English Paragraph/Run API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::Table",
        "TableRow",
        "TableCell",
        "Table Structure And Lookup",
        "Table Layout, Style, And Borders",
        "find_cell_by_grid_column",
        "set_cell_block_texts",
        "set_repeats_header",
        "merge_right",
        "set_fill_color",
        "../../api/tables"
    )) {
    Assert-ContainsText -Text $englishTableApi -ExpectedText $marker `
        -Message "English Table API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::inline_image_info",
        "featherdoc::drawing_image_info",
        "floating_image_options",
        "drawing_images",
        "inline_images",
        "append_image",
        "append_floating_image",
        "extract_drawing_image",
        "replace_inline_image",
        "remove_inline_image",
        "../../api/images_sections"
    )) {
    Assert-ContainsText -Text $englishImagesApi -ExpectedText $marker `
        -Message "English Images API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::section_page_setup",
        "featherdoc::page_margins",
        "sections_inspection_summary",
        "section_count",
        "append_section",
        "inspect_section",
        "get_section_page_setup",
        "set_section_page_setup",
        "section_header_template",
        "section_footer_template",
        "replace_section_header_text",
        "replace_section_footer_text",
        "../../api/images_sections"
    )) {
    Assert-ContainsText -Text $englishSectionsApi -ExpectedText $marker `
        -Message "English Sections API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::field_summary",
        "featherdoc::hyperlink_summary",
        "featherdoc::review_note_summary",
        "featherdoc::revision_summary",
        "list_fields",
        "append_field",
        "append_hyperlink",
        "list_comments",
        "set_comment_resolved",
        "list_revisions",
        "accept_all_revisions",
        "../../api/fields_links_reviews"
    )) {
    Assert-ContainsText -Text $englishFieldsLinksReviewsApi -ExpectedText $marker `
        -Message "English Fields/Links/Reviews API page should preserve method-table marker."
}

foreach ($marker in @(
        "numbering_definition_summary",
        "numbering_catalog",
        "style_summary",
        "style_usage_report",
        "style_refactor_plan",
        "list_numbering_definitions",
        "export_numbering_catalog",
        "list_styles",
        "suggest_style_merges",
        "audit_table_style_quality",
        "../../api/styles_numbering"
    )) {
    Assert-ContainsText -Text $englishStylesNumberingApi -ExpectedText $marker `
        -Message "English Styles/Numbering API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::TemplatePart",
        "Part Basics",
        "Bookmarks",
        "Content Controls",
        "Template Schema",
        "replace_content_control_text_by_tag",
        "validate_template_schema",
        "onboard_template",
        "../../api/templates"
    )) {
    Assert-ContainsText -Text $englishTemplatePartApi -ExpectedText $marker `
        -Message "English TemplatePart API page should preserve method-table marker."
}

foreach ($marker in @(
        "FDOC_API_EDIT_PLAN_EN_MARKER",
        "scripts/edit_document_from_plan.ps1",
        "FDOC_EDIT_PLAN_PLAN_SHAPE",
        "FDOC_EDIT_PLAN_EXECUTION_RESULT",
        "FDOC_EDIT_PLAN_OPERATION_REFERENCE",
        "FDOC_EDIT_PLAN_JSON_EXAMPLES",
        "FDOC_EDIT_PLAN_REQUIRED_FIELDS",
        "FDOC_EDIT_PLAN_OPTIONAL_FIELDS",
        "Plan Shape",
        "Execution Result",
        "Operation Reference",
        "JSON Examples",
        "Required fields",
        "Optional fields and aliases",
        "operation_count",
        "summary_json",
        "input_docx",
        "output_docx",
        "selected_text",
        "comment_text",
        "image_path",
        "target",
        "catalog_file",
        "font_size_points",
        "Review And Revision Operations",
        "Notes And Fields",
        "Template Slots And Content Controls",
        "Images, Links, And OMML",
        "Text And Paragraph Mutations",
        "Tables And Numbering",
        "accept_all_revisions",
        "apply_review_mutation_plan",
        "append_page_number_field",
        "replace_content_control_text_by_tag",
        "append_image",
        "append_hyperlink",
        "set_paragraph_alignment",
        "apply_table_position_plan",
        "import_numbering_catalog",
        "unmerge_table_cell",
        "../../api/edit_plan_operations"
    )) {
    Assert-ContainsText -Text $englishEditPlanOperationsApi -ExpectedText $marker `
        -Message "English Edit Plan Operations API page should preserve operation marker."
}

foreach ($marker in @(
        "FDOC_API_ENUMS_EN_MARKER",
        "formatting_flag",
        "section_reference_kind",
        "page_orientation",
        "paragraph_alignment",
        "paragraph_line_spacing_rule",
        "table_layout_mode",
        "table_alignment",
        "cell_vertical_alignment",
        "table_border_edge",
        "border_style",
        "bookmark_kind",
        "content_control_kind",
        "field_kind",
        "review_note_kind",
        "revision_kind",
        "../../api/enums"
    )) {
    Assert-ContainsText -Text $englishEnumsApi -ExpectedText $marker `
        -Message "English Enums API page should preserve enum marker."
}

foreach ($marker in @(
        "../en/index",
        "getting_started",
        "api/index"
    )) {
    Assert-ContainsText -Text $chineseIndex -ExpectedText $marker `
        -Message "Chinese docs entrypoint should preserve navigation marker."
}

foreach ($marker in @(
        '* :doc:`../visual_validation_zh`',
        '* :doc:`../governance_routes_zh`',
        '* :doc:`../release_policy_zh`',
        '../visual_validation_zh',
        '../governance_routes_zh',
        '../release_policy_zh',
        '../project_identity_zh',
        '../script_task_index_zh'
    )) {
    Assert-DoesNotContainText -Text $chineseIndex -UnexpectedText $marker `
        -Message "Chinese docs entrypoint should keep legacy topic docs out of the language-local navigation tree."
}

foreach ($marker in @(
        ':doc:`../api/index`',
        ':doc:`../pdf_export`',
        ':doc:`../pdf_import`'
    )) {
    Assert-DoesNotContainText -Text $chineseIndex -UnexpectedText $marker `
        -Message "Chinese docs entrypoint should keep legacy docs in the hidden compatibility toctree."
}

foreach ($marker in @(
        "cmake -S . -B build",
        "BUILD_CLI",
        "featherdoc::Document",
        "api/document",
        "../api/templates",
        "../pdf_export"
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
        "enums"
    )) {
    Assert-ContainsText -Text $chineseApiIndex -ExpectedText $marker `
        -Message "Chinese API entrypoint should preserve mirrored API marker."
}

foreach ($marker in @(
        "featherdoc::Document",
        "FDOC_DOCUMENT_API_PARAMETERS",
        "path() const",
        "last_error() const noexcept",
        "enable_update_fields_on_open()",
        "TemplatePart",
        "sections_inspection_summary",
        "FDOC_DOCUMENT_API_SECTIONS",
        "move_section",
        "replace_section_footer_text",
        "remove_section_footer_reference",
        "section_page_setup",
        "reference_kind",
        "FDOC_DOCUMENT_API_TEMPLATE_FILLING",
        "replace_content_control_text_by_tag(std::string_view tag, std::string_view replacement)",
        "list_content_controls() const",
        "replace_content_control_with_table_rows_by_tag",
        "bookmark_fill_result",
        "FDOC_DOCUMENT_API_BODY_TABLE_IMAGE",
        "append_table",
        "append_floating_image",
        "FDOC_DOCUMENT_API_REVIEW_LINKS",
        "append_paragraph_text_comment",
        "append_text_range_comment",
        "FDOC_DOCUMENT_API_RELATED_FAMILIES",
        "fields_links_reviews",
        "compare_semantic",
        "open()",
        "save_as",
        "../../api/document"
    )) {
    Assert-ContainsText -Text $chineseDocumentApi -ExpectedText $marker `
        -Message "Chinese Document API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::Paragraph",
        "featherdoc::Run",
        "set_alignment",
        "set_font_family",
        "set_rtl",
        "insert_paragraph_after",
        "insert_run_after",
        "../../api/content_blocks"
    )) {
    Assert-ContainsText -Text $chineseParagraphRunApi -ExpectedText $marker `
        -Message "Chinese Paragraph/Run API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::Table",
        "TableRow",
        "TableCell",
        "find_cell_by_grid_column",
        "set_cell_block_texts",
        "set_repeats_header",
        "merge_right",
        "set_fill_color",
        "../../api/tables"
    )) {
    Assert-ContainsText -Text $chineseTableApi -ExpectedText $marker `
        -Message "Chinese Table API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::inline_image_info",
        "featherdoc::drawing_image_info",
        "floating_image_options",
        "drawing_images",
        "inline_images",
        "append_image",
        "append_floating_image",
        "extract_drawing_image",
        "replace_inline_image",
        "remove_inline_image",
        "../../api/images_sections"
    )) {
    Assert-ContainsText -Text $chineseImagesApi -ExpectedText $marker `
        -Message "Chinese Images API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::section_page_setup",
        "featherdoc::page_margins",
        "sections_inspection_summary",
        "section_count",
        "append_section",
        "inspect_section",
        "get_section_page_setup",
        "set_section_page_setup",
        "section_header_template",
        "section_footer_template",
        "replace_section_header_text",
        "replace_section_footer_text",
        "../../api/images_sections"
    )) {
    Assert-ContainsText -Text $chineseSectionsApi -ExpectedText $marker `
        -Message "Chinese Sections API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::field_summary",
        "featherdoc::hyperlink_summary",
        "featherdoc::review_note_summary",
        "featherdoc::revision_summary",
        "list_fields",
        "append_field",
        "append_hyperlink",
        "list_comments",
        "set_comment_resolved",
        "list_revisions",
        "accept_all_revisions",
        "../../api/fields_links_reviews"
    )) {
    Assert-ContainsText -Text $chineseFieldsLinksReviewsApi -ExpectedText $marker `
        -Message "Chinese Fields/Links/Reviews API page should preserve method-table marker."
}

foreach ($marker in @(
        "numbering_definition_summary",
        "numbering_catalog",
        "style_summary",
        "style_usage_report",
        "style_refactor_plan",
        "list_numbering_definitions",
        "export_numbering_catalog",
        "list_styles",
        "suggest_style_merges",
        "audit_table_style_quality",
        "../../api/styles_numbering"
    )) {
    Assert-ContainsText -Text $chineseStylesNumberingApi -ExpectedText $marker `
        -Message "Chinese Styles/Numbering API page should preserve method-table marker."
}

foreach ($marker in @(
        "featherdoc::TemplatePart",
        "entry_name()",
        "fill_bookmarks",
        "replace_content_control_text_by_tag",
        "validate_template_schema",
        "onboard_template",
        "../../api/templates"
    )) {
    Assert-ContainsText -Text $chineseTemplatePartApi -ExpectedText $marker `
        -Message "Chinese TemplatePart API page should preserve method-table marker."
}

foreach ($marker in @(
        "FDOC_API_EDIT_PLAN_ZH_CN_MARKER",
        "scripts/edit_document_from_plan.ps1",
        "FDOC_EDIT_PLAN_PLAN_SHAPE",
        "FDOC_EDIT_PLAN_EXECUTION_RESULT",
        "FDOC_EDIT_PLAN_OPERATION_REFERENCE",
        "FDOC_EDIT_PLAN_JSON_EXAMPLES",
        "FDOC_EDIT_PLAN_REQUIRED_FIELDS",
        "FDOC_EDIT_PLAN_OPTIONAL_FIELDS",
        "operation_count",
        "summary_json",
        "input_docx",
        "output_docx",
        "selected_text",
        "comment_text",
        "image_path",
        "target",
        "catalog_file",
        "font_size_points",
        "accept_all_revisions",
        "apply_review_mutation_plan",
        "append_page_number_field",
        "replace_content_control_text_by_tag",
        "append_image",
        "append_hyperlink",
        "set_paragraph_alignment",
        "apply_table_position_plan",
        "import_numbering_catalog",
        "unmerge_table_cell",
        "../../api/edit_plan_operations"
    )) {
    Assert-ContainsText -Text $chineseEditPlanOperationsApi -ExpectedText $marker `
        -Message "Chinese Edit Plan Operations API page should preserve operation marker."
}

foreach ($marker in @(
        "FDOC_API_ENUMS_ZH_CN_MARKER",
        "formatting_flag",
        "section_reference_kind",
        "page_orientation",
        "paragraph_alignment",
        "paragraph_line_spacing_rule",
        "table_layout_mode",
        "table_alignment",
        "cell_vertical_alignment",
        "table_border_edge",
        "border_style",
        "bookmark_kind",
        "content_control_kind",
        "field_kind",
        "review_note_kind",
        "revision_kind",
        "../../api/enums"
    )) {
    Assert-ContainsText -Text $chineseEnumsApi -ExpectedText $marker `
        -Message "Chinese Enums API page should preserve enum marker."
}

foreach ($marker in @(
        "https://wuxianggujun.github.io/FeatherDoc/en/",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/",
        "https://wuxianggujun.github.io/FeatherDoc/en/api/",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/api/",
        "https://wuxianggujun.github.io/FeatherDoc/api/",
        "https://shop.input.im/?code=fbe6f3d5",
        "sponsor/zhifubao.jpg",
        "sponsor/weixin.png"
    )) {
    Assert-ContainsText -Text $readme -ExpectedText $marker `
        -Message "English README should preserve docs, promotion, and sponsor markers."
}

foreach ($marker in @(
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/",
        "https://wuxianggujun.github.io/FeatherDoc/en/",
        "https://wuxianggujun.github.io/FeatherDoc/zh-CN/api/",
        "https://wuxianggujun.github.io/FeatherDoc/en/api/",
        "https://wuxianggujun.github.io/FeatherDoc/api/",
        "https://shop.input.im/?code=fbe6f3d5",
        "sponsor/zhifubao.jpg",
        "sponsor/weixin.png"
    )) {
    Assert-ContainsText -Text $readmeZh -ExpectedText $marker `
        -Message "Chinese README should preserve docs, promotion, and sponsor markers."
}

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
