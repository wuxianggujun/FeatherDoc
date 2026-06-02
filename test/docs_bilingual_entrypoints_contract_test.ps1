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
$englishIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\index.rst"
$englishGettingStarted = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\getting_started.rst"
$englishApiIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\index.rst"
$englishDocumentApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\document.rst"
$englishParagraphRunApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\paragraph_run.rst"
$englishTableApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\table.rst"
$englishImagesApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\images.rst"
$englishSectionsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\sections.rst"
$englishTemplatePartApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\en\api\template_part.rst"
$chineseIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\index.rst"
$chineseGettingStarted = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\getting_started.rst"
$chineseApiIndex = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\index.rst"
$chineseDocumentApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\document.rst"
$chineseParagraphRunApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\paragraph_run.rst"
$chineseTableApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\table.rst"
$chineseImagesApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\images.rst"
$chineseSectionsApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\sections.rst"
$chineseTemplatePartApi = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "docs\zh-CN\api\template_part.rst"
$readme = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.md"
$readmeZh = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "README.zh-CN.md"
$cmakeLists = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "test\CMakeLists.txt"

foreach ($marker in @(
        "Choose a language entry point",
        "en/index",
        "zh-CN/index",
        "Languages",
        "en/api/index",
        "zh-CN/api/index",
        "api/index",
        "getting_started",
        "pdf_export",
        "project_template_release_readiness_checklist_zh",
        "script_task_index_zh"
    )) {
    Assert-ContainsText -Text $rootIndex -ExpectedText $marker `
        -Message "Root docs index should preserve bilingual landing markers."
}

foreach ($marker in @(
        "../zh-CN/index",
        "getting_started",
        "api/index",
        "../api/index",
        "../pdf_export"
    )) {
    Assert-ContainsText -Text $englishIndex -ExpectedText $marker `
        -Message "English docs entrypoint should preserve navigation marker."
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
        "../../api/index",
        "featherdoc::Document",
        "document",
        "paragraph_run",
        "table",
        "images",
        "sections",
        "template_part",
        "../../api/content_blocks",
        "../../api/images_sections",
        "../../api/templates",
        "../../api/tables"
    )) {
    Assert-ContainsText -Text $englishApiIndex -ExpectedText $marker `
        -Message "English API entrypoint should preserve language-local API marker."
}

foreach ($marker in @(
        "featherdoc::Document",
        "Lifecycle",
        "Template Part Access",
        "Sections And Inspection",
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
        "../en/index",
        "getting_started",
        "api/index",
        "../visual_validation_zh",
        "../governance_routes_zh",
        "../release_policy_zh"
    )) {
    Assert-ContainsText -Text $chineseIndex -ExpectedText $marker `
        -Message "Chinese docs entrypoint should preserve navigation marker."
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
        "../../api/index",
        "featherdoc::Document",
        "document",
        "paragraph_run",
        "table",
        "images",
        "sections",
        "template_part",
        "../../api/content_blocks",
        "../../api/document",
        "../../api/images_sections",
        "../../api/templates",
        "../../api/tables"
    )) {
    Assert-ContainsText -Text $chineseApiIndex -ExpectedText $marker `
        -Message "Chinese API entrypoint should preserve mirrored API marker."
}

foreach ($marker in @(
        "featherdoc::Document",
        "TemplatePart",
        "sections_inspection_summary",
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
