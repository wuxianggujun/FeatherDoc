$styleMetadataSourceDocx = Join-Path $resolvedWorkingDir "style_metadata.source.docx"
$styleMetadataPlanPath = Join-Path $resolvedWorkingDir "style_metadata.edit_plan.json"
$styleMetadataEditedDocx = Join-Path $resolvedWorkingDir "style_metadata.edited.docx"
$styleMetadataSummaryPath = Join-Path $resolvedWorkingDir "style_metadata.edit.summary.json"
$styleMetadataClearDefaultsPlanPath = Join-Path $resolvedWorkingDir "style_metadata.clear_defaults.edit_plan.json"
$styleMetadataDefaultsClearedDocx = Join-Path $resolvedWorkingDir "style_metadata.clear_defaults.edited.docx"
$styleMetadataClearDefaultsSummaryPath = Join-Path $resolvedWorkingDir "style_metadata.clear_defaults.summary.json"
$styleDefinitionPlanPath = Join-Path $resolvedWorkingDir "style_definition.edit_plan.json"
$styleDefinitionEditedDocx = Join-Path $resolvedWorkingDir "style_definition.edited.docx"
$styleDefinitionSummaryPath = Join-Path $resolvedWorkingDir "style_definition.edit.summary.json"

New-StyleListFixtureDocx -Path $styleMetadataSourceDocx
Set-Content -LiteralPath $styleMetadataPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "set_default_run_properties",
      "font": "Aptos",
      "east_asia_font": "Microsoft YaHei",
      "lang": "en-US",
      "east_asia_lang": "zh-CN",
      "bidi_lang": "ar-SA",
      "rtl": true,
      "paragraph_bidi": true
    },
    {
      "op": "set_style_run_properties",
      "style_id": "Heading1",
      "font_family": "Aptos Display",
      "east_asia_font_family": "Microsoft YaHei",
      "language": "en-US",
      "east_asia_language": "zh-CN",
      "bidi_language": "ar-SA",
      "rtl": true,
      "paragraph_bidi": true
    },
    {
      "op": "clear_style_run_properties",
      "style": "LegacyBody",
      "clear_font_family": true,
      "east_asia_font_family": true,
      "clear_primary_language": true,
      "language": true,
      "clear_east_asia_language": true,
      "bidi_language": true,
      "clear_rtl": true,
      "paragraph_bidi": true
    },
    {
      "op": "set_paragraph_style_properties",
      "style": "Heading1",
      "based_on_style_id": "LegacyBody",
      "next_style_id": "LegacyBody",
      "level": 1
    },
    {
      "op": "clear_paragraph_style_properties",
      "style_id": "LegacyBody",
      "based_on": true,
      "clear_next_style": true,
      "clear_outline_level": true
    },
    {
      "op": "set_paragraph_style_numbering",
      "style": "Heading1",
      "name": "HeadingOutline",
      "levels": [
        {
          "level": 0,
          "kind": "decimal",
          "pattern": "%1."
        },
        {
          "level": 1,
          "kind": "decimal",
          "start": 3,
          "text_pattern": "%1.%2."
        }
      ],
      "style_level": 1
    },
    {
      "op": "clear_paragraph_style_numbering",
      "style_id": "LegacyBody"
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleMetadataSourceDocx `
    -EditPlan $styleMetadataPlanPath `
    -OutputDocx $styleMetadataEditedDocx `
    -SummaryJson $styleMetadataSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style metadata edit plan."
}

$styleMetadataSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleMetadataSummaryPath | ConvertFrom-Json
$styleMetadataStylesXml = Read-DocxEntryText -DocxPath $styleMetadataEditedDocx -EntryName "word/styles.xml"
$styleMetadataStylesDocument = New-Object System.Xml.XmlDocument
$styleMetadataStylesDocument.PreserveWhitespace = $true
$styleMetadataStylesDocument.LoadXml($styleMetadataStylesXml)
$styleMetadataStylesNamespaceManager = New-WordNamespaceManager -Document $styleMetadataStylesDocument
$styleMetadataNumberingXml = Read-DocxEntryText -DocxPath $styleMetadataEditedDocx -EntryName "word/numbering.xml"
$styleMetadataNumberingDocument = New-Object System.Xml.XmlDocument
$styleMetadataNumberingDocument.PreserveWhitespace = $true
$styleMetadataNumberingDocument.LoadXml($styleMetadataNumberingXml)
$styleMetadataNumberingNamespaceManager = New-WordNamespaceManager -Document $styleMetadataNumberingDocument
$styleMetadataWordNamespace = $styleMetadataStylesNamespaceManager.LookupNamespace("w")

Assert-Equal -Actual $styleMetadataSummary.status -Expected "completed" `
    -Message "Style-metadata summary did not report status=completed."
Assert-Equal -Actual $styleMetadataSummary.operation_count -Expected 7 `
    -Message "Style-metadata summary should record seven operations."
Assert-Equal -Actual $styleMetadataSummary.operations[0].command -Expected "set-default-run-properties" `
    -Message "Set-default-run-properties should use the CLI default-run command."
Assert-Equal -Actual $styleMetadataSummary.operations[1].command -Expected "set-style-run-properties" `
    -Message "Set-style-run-properties should use the CLI style-run command."
Assert-Equal -Actual $styleMetadataSummary.operations[2].command -Expected "clear-style-run-properties" `
    -Message "Clear-style-run-properties should use the CLI style-run command."
Assert-Equal -Actual $styleMetadataSummary.operations[3].command -Expected "set-paragraph-style-properties" `
    -Message "Set-paragraph-style-properties should use the CLI paragraph-style-properties command."
Assert-Equal -Actual $styleMetadataSummary.operations[4].command -Expected "clear-paragraph-style-properties" `
    -Message "Clear-paragraph-style-properties should use the CLI paragraph-style-properties command."
Assert-Equal -Actual $styleMetadataSummary.operations[5].command -Expected "set-paragraph-style-numbering" `
    -Message "Set-paragraph-style-numbering should use the CLI style-numbering command."
Assert-Equal -Actual $styleMetadataSummary.operations[6].command -Expected "clear-paragraph-style-numbering" `
    -Message "Clear-paragraph-style-numbering should use the CLI style-numbering command."

$defaultRunFontsNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:rFonts", $styleMetadataStylesNamespaceManager)
$defaultRunLanguageNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:lang", $styleMetadataStylesNamespaceManager)
$defaultRunRtlNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:rtl", $styleMetadataStylesNamespaceManager)
$defaultParagraphBidiNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:pPrDefault/w:pPr/w:bidi", $styleMetadataStylesNamespaceManager)

Assert-True -Condition ($null -ne $defaultRunFontsNode) `
    -Message "Set-default-run-properties should keep docDefaults run font metadata."
Assert-Equal -Actual $defaultRunFontsNode.GetAttribute("ascii", $styleMetadataWordNamespace) -Expected "Aptos" `
    -Message "Set-default-run-properties should update the default ASCII font family."
Assert-Equal -Actual $defaultRunFontsNode.GetAttribute("hAnsi", $styleMetadataWordNamespace) -Expected "Aptos" `
    -Message "Set-default-run-properties should update the default hAnsi font family."
Assert-Equal -Actual $defaultRunFontsNode.GetAttribute("eastAsia", $styleMetadataWordNamespace) -Expected "Microsoft YaHei" `
    -Message "Set-default-run-properties should update the default East Asia font family."
Assert-True -Condition ($null -ne $defaultRunLanguageNode) `
    -Message "Set-default-run-properties should keep docDefaults language metadata."
Assert-Equal -Actual $defaultRunLanguageNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "en-US" `
    -Message "Set-default-run-properties should update the default primary language."
Assert-Equal -Actual $defaultRunLanguageNode.GetAttribute("eastAsia", $styleMetadataWordNamespace) -Expected "zh-CN" `
    -Message "Set-default-run-properties should update the default East Asia language."
Assert-Equal -Actual $defaultRunLanguageNode.GetAttribute("bidi", $styleMetadataWordNamespace) -Expected "ar-SA" `
    -Message "Set-default-run-properties should update the default bidi language."
Assert-True -Condition ($null -ne $defaultRunRtlNode) `
    -Message "Set-default-run-properties should keep the default RTL marker."
Assert-True -Condition ($null -ne $defaultParagraphBidiNode) `
    -Message "Set-default-run-properties should keep the default paragraph bidi marker."

$headingRunFontsNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:rPr/w:rFonts", $styleMetadataStylesNamespaceManager)
$headingRunLanguageNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:rPr/w:lang", $styleMetadataStylesNamespaceManager)
$headingRunRtlNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:rPr/w:rtl", $styleMetadataStylesNamespaceManager)
$headingParagraphBidiNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:bidi", $styleMetadataStylesNamespaceManager)
$headingBasedOnNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:basedOn", $styleMetadataStylesNamespaceManager)
$headingNextNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:next", $styleMetadataStylesNamespaceManager)
$headingOutlineNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:outlineLvl", $styleMetadataStylesNamespaceManager)
$headingNumPrNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:numPr", $styleMetadataStylesNamespaceManager)
$headingNumLevelNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:numPr/w:ilvl", $styleMetadataStylesNamespaceManager)
$headingNumIdNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:numPr/w:numId", $styleMetadataStylesNamespaceManager)

Assert-True -Condition ($null -ne $headingRunFontsNode) `
    -Message "Set-style-run-properties should create style run font metadata."
Assert-Equal -Actual $headingRunFontsNode.GetAttribute("ascii", $styleMetadataWordNamespace) -Expected "Aptos Display" `
    -Message "Set-style-run-properties should update the style ASCII font family."
Assert-Equal -Actual $headingRunFontsNode.GetAttribute("hAnsi", $styleMetadataWordNamespace) -Expected "Aptos Display" `
    -Message "Set-style-run-properties should update the style hAnsi font family."
Assert-Equal -Actual $headingRunFontsNode.GetAttribute("eastAsia", $styleMetadataWordNamespace) -Expected "Microsoft YaHei" `
    -Message "Set-style-run-properties should update the style East Asia font family."
Assert-True -Condition ($null -ne $headingRunLanguageNode) `
    -Message "Set-style-run-properties should create style language metadata."
Assert-Equal -Actual $headingRunLanguageNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "en-US" `
    -Message "Set-style-run-properties should update the style primary language."
Assert-Equal -Actual $headingRunLanguageNode.GetAttribute("eastAsia", $styleMetadataWordNamespace) -Expected "zh-CN" `
    -Message "Set-style-run-properties should update the style East Asia language."
Assert-Equal -Actual $headingRunLanguageNode.GetAttribute("bidi", $styleMetadataWordNamespace) -Expected "ar-SA" `
    -Message "Set-style-run-properties should update the style bidi language."
Assert-True -Condition ($null -ne $headingRunRtlNode) `
    -Message "Set-style-run-properties should create the style RTL marker."
Assert-True -Condition ($null -ne $headingParagraphBidiNode) `
    -Message "Set-style-run-properties should create the style paragraph bidi marker."
Assert-True -Condition ($null -ne $headingBasedOnNode) `
    -Message "Set-paragraph-style-properties should create the basedOn node."
Assert-Equal -Actual $headingBasedOnNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "LegacyBody" `
    -Message "Set-paragraph-style-properties should set the requested based-on style."
Assert-True -Condition ($null -ne $headingNextNode) `
    -Message "Set-paragraph-style-properties should create the next style node."
Assert-Equal -Actual $headingNextNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "LegacyBody" `
    -Message "Set-paragraph-style-properties should set the requested next style."
Assert-True -Condition ($null -ne $headingOutlineNode) `
    -Message "Set-paragraph-style-properties should create the outline-level node."
Assert-Equal -Actual $headingOutlineNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "1" `
    -Message "Set-paragraph-style-properties should set the requested outline level."
Assert-True -Condition ($null -ne $headingNumPrNode -and $null -ne $headingNumLevelNode -and $null -ne $headingNumIdNode) `
    -Message "Set-paragraph-style-numbering should create style numbering metadata."
Assert-Equal -Actual $headingNumLevelNode.GetAttribute("val", $styleMetadataWordNamespace) -Expected "1" `
    -Message "Set-paragraph-style-numbering should keep the requested style level."
Assert-True -Condition (-not [string]::IsNullOrWhiteSpace($headingNumIdNode.GetAttribute("val", $styleMetadataWordNamespace))) `
    -Message "Set-paragraph-style-numbering should link a numbering instance."

$headingDefinitionNode = $styleMetadataNumberingDocument.SelectSingleNode("/w:numbering/w:abstractNum[w:name[@w:val='HeadingOutline']]", $styleMetadataNumberingNamespaceManager)
Assert-True -Condition ($null -ne $headingDefinitionNode) `
    -Message "Set-paragraph-style-numbering should create the requested numbering definition."
$headingDefinitionLevelZeroNode = $headingDefinitionNode.SelectSingleNode("w:lvl[@w:ilvl='0']", $styleMetadataNumberingNamespaceManager)
$headingDefinitionLevelOneNode = $headingDefinitionNode.SelectSingleNode("w:lvl[@w:ilvl='1']", $styleMetadataNumberingNamespaceManager)
Assert-True -Condition ($null -ne $headingDefinitionLevelZeroNode -and $null -ne $headingDefinitionLevelOneNode) `
    -Message "Set-paragraph-style-numbering should create every requested numbering level."
Assert-Equal -Actual $headingDefinitionLevelZeroNode.SelectSingleNode("w:numFmt", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "decimal" `
    -Message "Set-paragraph-style-numbering should set level zero numbering kind."
Assert-Equal -Actual $headingDefinitionLevelZeroNode.SelectSingleNode("w:start", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "1" `
    -Message "Set-paragraph-style-numbering should default the first level start to 1."
Assert-Equal -Actual $headingDefinitionLevelZeroNode.SelectSingleNode("w:lvlText", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "%1." `
    -Message "Set-paragraph-style-numbering should set the first level text pattern."
Assert-Equal -Actual $headingDefinitionLevelOneNode.SelectSingleNode("w:start", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "3" `
    -Message "Set-paragraph-style-numbering should preserve explicit numbering starts."
Assert-Equal -Actual $headingDefinitionLevelOneNode.SelectSingleNode("w:lvlText", $styleMetadataNumberingNamespaceManager).GetAttribute("val", $styleMetadataWordNamespace) -Expected "%1.%2." `
    -Message "Set-paragraph-style-numbering should set the second level text pattern."

$legacyFontsNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:rPr/w:rFonts", $styleMetadataStylesNamespaceManager)
$legacyLanguageNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:rPr/w:lang", $styleMetadataStylesNamespaceManager)
$legacyRtlNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:rPr/w:rtl", $styleMetadataStylesNamespaceManager)
$legacyParagraphBidiNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:pPr/w:bidi", $styleMetadataStylesNamespaceManager)
$legacyBasedOnNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:basedOn", $styleMetadataStylesNamespaceManager)
$legacyNextNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:next", $styleMetadataStylesNamespaceManager)
$legacyOutlineNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:pPr/w:outlineLvl", $styleMetadataStylesNamespaceManager)
$legacyNumPrNode = $styleMetadataStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='LegacyBody']/w:pPr/w:numPr", $styleMetadataStylesNamespaceManager)

Assert-True -Condition ($null -eq $legacyFontsNode -or (`
        [string]::IsNullOrWhiteSpace($legacyFontsNode.GetAttribute("ascii", $styleMetadataWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($legacyFontsNode.GetAttribute("hAnsi", $styleMetadataWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($legacyFontsNode.GetAttribute("eastAsia", $styleMetadataWordNamespace)))) `
    -Message "Clear-style-run-properties should remove LegacyBody font metadata."
Assert-True -Condition ($null -eq $legacyLanguageNode -or (`
        [string]::IsNullOrWhiteSpace($legacyLanguageNode.GetAttribute("val", $styleMetadataWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($legacyLanguageNode.GetAttribute("eastAsia", $styleMetadataWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($legacyLanguageNode.GetAttribute("bidi", $styleMetadataWordNamespace)))) `
    -Message "Clear-style-run-properties should remove LegacyBody language metadata."
Assert-True -Condition ($null -eq $legacyRtlNode) `
    -Message "Clear-style-run-properties should remove the LegacyBody RTL marker."
Assert-True -Condition ($null -eq $legacyParagraphBidiNode) `
    -Message "Clear-style-run-properties should remove the LegacyBody paragraph bidi marker."
Assert-True -Condition ($null -eq $legacyBasedOnNode) `
    -Message "Clear-paragraph-style-properties should remove LegacyBody basedOn."
Assert-True -Condition ($null -eq $legacyNextNode) `
    -Message "Clear-paragraph-style-properties should remove LegacyBody next style metadata."
Assert-True -Condition ($null -eq $legacyOutlineNode) `
    -Message "Clear-paragraph-style-properties should remove LegacyBody outline level metadata."
Assert-True -Condition ($null -eq $legacyNumPrNode) `
    -Message "Clear-paragraph-style-numbering should remove LegacyBody numbering metadata."

Set-Content -LiteralPath $styleMetadataClearDefaultsPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "clear_default_run_properties",
      "font_family": true,
      "clear_east_asia_font_family": true,
      "primary_language": true,
      "clear_language": true,
      "east_asia_language": true,
      "clear_bidi_language": true,
      "rtl": true,
      "clear_paragraph_bidi": true
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleMetadataEditedDocx `
    -EditPlan $styleMetadataClearDefaultsPlanPath `
    -OutputDocx $styleMetadataDefaultsClearedDocx `
    -SummaryJson $styleMetadataClearDefaultsSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the clear default-run-properties plan."
}

$styleMetadataClearDefaultsSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleMetadataClearDefaultsSummaryPath | ConvertFrom-Json
$styleMetadataDefaultsClearedStylesXml = Read-DocxEntryText -DocxPath $styleMetadataDefaultsClearedDocx -EntryName "word/styles.xml"
$styleMetadataDefaultsClearedStylesDocument = New-Object System.Xml.XmlDocument
$styleMetadataDefaultsClearedStylesDocument.PreserveWhitespace = $true
$styleMetadataDefaultsClearedStylesDocument.LoadXml($styleMetadataDefaultsClearedStylesXml)
$styleMetadataDefaultsClearedNamespaceManager = New-WordNamespaceManager -Document $styleMetadataDefaultsClearedStylesDocument
$styleMetadataDefaultsClearedWordNamespace = $styleMetadataDefaultsClearedNamespaceManager.LookupNamespace("w")
$clearedDefaultRunFontsNode = $styleMetadataDefaultsClearedStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:rFonts", $styleMetadataDefaultsClearedNamespaceManager)
$clearedDefaultRunLanguageNode = $styleMetadataDefaultsClearedStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:lang", $styleMetadataDefaultsClearedNamespaceManager)
$clearedDefaultRunRtlNode = $styleMetadataDefaultsClearedStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:rPrDefault/w:rPr/w:rtl", $styleMetadataDefaultsClearedNamespaceManager)
$clearedDefaultParagraphBidiNode = $styleMetadataDefaultsClearedStylesDocument.SelectSingleNode("/w:styles/w:docDefaults/w:pPrDefault/w:pPr/w:bidi", $styleMetadataDefaultsClearedNamespaceManager)

Assert-Equal -Actual $styleMetadataClearDefaultsSummary.status -Expected "completed" `
    -Message "Clear-default-run-properties summary did not report status=completed."
Assert-Equal -Actual $styleMetadataClearDefaultsSummary.operation_count -Expected 1 `
    -Message "Clear-default-run-properties summary should record one operation."
Assert-Equal -Actual $styleMetadataClearDefaultsSummary.operations[0].command -Expected "clear-default-run-properties" `
    -Message "Clear-default-run-properties should use the CLI default-run command."
Assert-True -Condition ($null -eq $clearedDefaultRunFontsNode -or (`
        [string]::IsNullOrWhiteSpace($clearedDefaultRunFontsNode.GetAttribute("ascii", $styleMetadataDefaultsClearedWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($clearedDefaultRunFontsNode.GetAttribute("hAnsi", $styleMetadataDefaultsClearedWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($clearedDefaultRunFontsNode.GetAttribute("eastAsia", $styleMetadataDefaultsClearedWordNamespace)))) `
    -Message "Clear-default-run-properties should remove docDefaults font metadata."
Assert-True -Condition ($null -eq $clearedDefaultRunLanguageNode -or (`
        [string]::IsNullOrWhiteSpace($clearedDefaultRunLanguageNode.GetAttribute("val", $styleMetadataDefaultsClearedWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($clearedDefaultRunLanguageNode.GetAttribute("eastAsia", $styleMetadataDefaultsClearedWordNamespace)) -and `
        [string]::IsNullOrWhiteSpace($clearedDefaultRunLanguageNode.GetAttribute("bidi", $styleMetadataDefaultsClearedWordNamespace)))) `
    -Message "Clear-default-run-properties should remove docDefaults language metadata."
Assert-True -Condition ($null -eq $clearedDefaultRunRtlNode) `
    -Message "Clear-default-run-properties should remove the docDefaults RTL marker."
Assert-True -Condition ($null -eq $clearedDefaultParagraphBidiNode) `
    -Message "Clear-default-run-properties should remove the docDefaults paragraph bidi marker."

Set-Content -LiteralPath $styleDefinitionPlanPath -Encoding UTF8 -Value @'
{
  "operations": [
    {
      "op": "ensure_paragraph_style",
      "style_id": "ReviewHeading",
      "name": "Review Heading",
      "based_on": "Heading1",
      "next_style": "LegacyBody",
      "custom": true,
      "semi_hidden": false,
      "unhide_when_used": true,
      "quick_format": true,
      "font_family": "Aptos Display",
      "east_asia_font_family": "Microsoft YaHei",
      "language": "en-US",
      "east_asia_language": "zh-CN",
      "bidi_language": "ar-SA",
      "rtl": true,
      "paragraph_bidi": true,
      "outline_level": 2
    },
    {
      "op": "ensure_character_style",
      "style": "ReviewStrong",
      "name": "Review Strong",
      "based_on_style_id": "Strong",
      "custom": true,
      "run_font_family": "Consolas",
      "run_east_asia_font_family": "Microsoft YaHei",
      "run_language": "en-US",
      "run_east_asia_language": "zh-CN",
      "run_bidi_language": "ar-SA",
      "run_rtl": true
    },
    {
      "op": "ensure_table_style",
      "style_id": "ReviewTable",
      "name": "Review Table",
      "based_on": "TableGrid",
      "custom": true,
      "unhide_when_used": true,
      "quick_format": true,
      "style_fill": [
        "whole_table:DDEEFF",
        "first_row:1F4E79"
      ],
      "style_text_color": [
        "first_row:FFFFFF"
      ],
      "style_bold": [
        "first_row:true"
      ],
      "style_border": [
        "whole_table:top:single:12:4472C4"
      ]
    },
    {
      "op": "materialize_style_run_properties",
      "style_id": "ReviewHeading"
    },
    {
      "op": "rebase_character_style_based_on",
      "style_id": "ReviewStrong",
      "target_style": "LegacyStrong"
    },
    {
      "op": "rebase_paragraph_style_based_on",
      "style_id": "ReviewHeading",
      "target_based_on": "Normal"
    },
    {
      "op": "ensure_numbering_definition",
      "definition_name": "DefinitionOnly",
      "numbering_levels": [
        {
          "level": 0,
          "kind": "decimal",
          "pattern": "%1)"
        }
      ]
    },
    {
      "op": "ensure_style_linked_numbering",
      "name": "SharedHeadingOutline",
      "levels": [
        "0:decimal:1:%1.",
        {
          "level": 1,
          "kind": "decimal",
          "start": 2,
          "pattern": "%1.%2."
        }
      ],
      "style_links": [
        {
          "style": "ReviewHeading",
          "level": 0
        },
        "Heading1:1"
      ]
    }
  ]
}
'@

& $scriptPath `
    -InputDocx $styleMetadataDefaultsClearedDocx `
    -EditPlan $styleDefinitionPlanPath `
    -OutputDocx $styleDefinitionEditedDocx `
    -SummaryJson $styleDefinitionSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style definition edit plan."
}

$styleDefinitionSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $styleDefinitionSummaryPath | ConvertFrom-Json
$styleDefinitionStylesXml = Read-DocxEntryText -DocxPath $styleDefinitionEditedDocx -EntryName "word/styles.xml"
$styleDefinitionStylesDocument = New-Object System.Xml.XmlDocument
$styleDefinitionStylesDocument.PreserveWhitespace = $true
$styleDefinitionStylesDocument.LoadXml($styleDefinitionStylesXml)
$styleDefinitionStylesNamespaceManager = New-WordNamespaceManager -Document $styleDefinitionStylesDocument
$styleDefinitionNumberingXml = Read-DocxEntryText -DocxPath $styleDefinitionEditedDocx -EntryName "word/numbering.xml"
$styleDefinitionNumberingDocument = New-Object System.Xml.XmlDocument
$styleDefinitionNumberingDocument.PreserveWhitespace = $true
$styleDefinitionNumberingDocument.LoadXml($styleDefinitionNumberingXml)
$styleDefinitionNumberingNamespaceManager = New-WordNamespaceManager -Document $styleDefinitionNumberingDocument
$styleDefinitionWordNamespace = $styleDefinitionStylesNamespaceManager.LookupNamespace("w")

Assert-Equal -Actual $styleDefinitionSummary.status -Expected "completed" `
    -Message "Style-definition summary did not report status=completed."
Assert-Equal -Actual $styleDefinitionSummary.operation_count -Expected 8 `
    -Message "Style-definition summary should record eight operations."
Assert-Equal -Actual $styleDefinitionSummary.operations[0].command -Expected "ensure-paragraph-style" `
    -Message "Ensure-paragraph-style should use the CLI paragraph style command."
Assert-Equal -Actual $styleDefinitionSummary.operations[1].command -Expected "ensure-character-style" `
    -Message "Ensure-character-style should use the CLI character style command."
Assert-Equal -Actual $styleDefinitionSummary.operations[2].command -Expected "ensure-table-style" `
    -Message "Ensure-table-style should use the CLI table style command."
Assert-Equal -Actual $styleDefinitionSummary.operations[3].command -Expected "materialize-style-run-properties" `
    -Message "Materialize-style-run-properties should use the CLI materialize command."
Assert-Equal -Actual $styleDefinitionSummary.operations[4].command -Expected "rebase-character-style-based-on" `
    -Message "Rebase-character-style-based-on should use the CLI rebase command."
Assert-Equal -Actual $styleDefinitionSummary.operations[5].command -Expected "rebase-paragraph-style-based-on" `
    -Message "Rebase-paragraph-style-based-on should use the CLI rebase command."
Assert-Equal -Actual $styleDefinitionSummary.operations[6].command -Expected "ensure-numbering-definition" `
    -Message "Ensure-numbering-definition should use the CLI numbering command."
Assert-Equal -Actual $styleDefinitionSummary.operations[7].command -Expected "ensure-style-linked-numbering" `
    -Message "Ensure-style-linked-numbering should use the CLI style-linked numbering command."

$reviewHeadingStyleNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']", $styleDefinitionStylesNamespaceManager)
$reviewHeadingNameNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:name", $styleDefinitionStylesNamespaceManager)
$reviewHeadingBasedOnNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:basedOn", $styleDefinitionStylesNamespaceManager)
$reviewHeadingNextNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:next", $styleDefinitionStylesNamespaceManager)
$reviewHeadingOutlineNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:pPr/w:outlineLvl", $styleDefinitionStylesNamespaceManager)
$reviewHeadingNumLevelNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:pPr/w:numPr/w:ilvl", $styleDefinitionStylesNamespaceManager)
$reviewHeadingRunFontsNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:rPr/w:rFonts", $styleDefinitionStylesNamespaceManager)
$reviewHeadingLanguageNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewHeading']/w:rPr/w:lang", $styleDefinitionStylesNamespaceManager)

Assert-True -Condition ($null -ne $reviewHeadingStyleNode) `
    -Message "Ensure-paragraph-style should create ReviewHeading."
Assert-Equal -Actual $reviewHeadingNameNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "Review Heading" `
    -Message "Ensure-paragraph-style should set the paragraph style display name."
Assert-Equal -Actual $reviewHeadingBasedOnNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "Normal" `
    -Message "Rebase-paragraph-style-based-on should switch ReviewHeading basedOn."
Assert-Equal -Actual $reviewHeadingNextNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "LegacyBody" `
    -Message "Ensure-paragraph-style should set ReviewHeading next style."
Assert-Equal -Actual $reviewHeadingOutlineNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "2" `
    -Message "Ensure-paragraph-style should set ReviewHeading outline level."
Assert-Equal -Actual $reviewHeadingNumLevelNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "0" `
    -Message "Ensure-style-linked-numbering should link ReviewHeading at level zero."
Assert-Equal -Actual $reviewHeadingRunFontsNode.GetAttribute("ascii", $styleDefinitionWordNamespace) -Expected "Aptos Display" `
    -Message "Materialize-style-run-properties should preserve ReviewHeading run font metadata."
Assert-Equal -Actual $reviewHeadingLanguageNode.GetAttribute("bidi", $styleDefinitionWordNamespace) -Expected "ar-SA" `
    -Message "Materialize-style-run-properties should preserve ReviewHeading bidi language."

$reviewStrongStyleNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']", $styleDefinitionStylesNamespaceManager)
$reviewStrongNameNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']/w:name", $styleDefinitionStylesNamespaceManager)
$reviewStrongBasedOnNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']/w:basedOn", $styleDefinitionStylesNamespaceManager)
$reviewStrongRunFontsNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']/w:rPr/w:rFonts", $styleDefinitionStylesNamespaceManager)
$reviewStrongRunRtlNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewStrong']/w:rPr/w:rtl", $styleDefinitionStylesNamespaceManager)
$reviewTableStyleNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewTable']", $styleDefinitionStylesNamespaceManager)
$reviewTableNameNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewTable']/w:name", $styleDefinitionStylesNamespaceManager)
$reviewTableBasedOnNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewTable']/w:basedOn", $styleDefinitionStylesNamespaceManager)
$reviewTableWholeFillNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewTable']/w:tcPr/w:shd", $styleDefinitionStylesNamespaceManager)
$reviewTableTopBorderNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewTable']/w:tblPr/w:tblBorders/w:top", $styleDefinitionStylesNamespaceManager)
$reviewTableFirstRowFillNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='ReviewTable']/w:tblStylePr[@w:type='firstRow']/w:tcPr/w:shd", $styleDefinitionStylesNamespaceManager)

Assert-True -Condition ($null -ne $reviewStrongStyleNode) `
    -Message "Ensure-character-style should create ReviewStrong."
Assert-Equal -Actual $reviewStrongNameNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "Review Strong" `
    -Message "Ensure-character-style should set the character style display name."
Assert-Equal -Actual $reviewStrongBasedOnNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "LegacyStrong" `
    -Message "Rebase-character-style-based-on should switch ReviewStrong basedOn."
Assert-Equal -Actual $reviewStrongRunFontsNode.GetAttribute("ascii", $styleDefinitionWordNamespace) -Expected "Consolas" `
    -Message "Ensure-character-style should set ReviewStrong run font metadata."
Assert-True -Condition ($null -ne $reviewStrongRunRtlNode) `
    -Message "Ensure-character-style should set ReviewStrong RTL metadata."
Assert-True -Condition ($null -ne $reviewTableStyleNode) `
    -Message "Ensure-table-style should create ReviewTable."
Assert-Equal -Actual $reviewTableNameNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "Review Table" `
    -Message "Ensure-table-style should set the table style display name."
Assert-Equal -Actual $reviewTableBasedOnNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "TableGrid" `
    -Message "Ensure-table-style should set the based-on table style."
Assert-Equal -Actual $reviewTableWholeFillNode.GetAttribute("fill", $styleDefinitionWordNamespace) -Expected "DDEEFF" `
    -Message "Ensure-table-style should set whole-table fill."
Assert-Equal -Actual $reviewTableTopBorderNode.GetAttribute("color", $styleDefinitionWordNamespace) -Expected "4472C4" `
    -Message "Ensure-table-style should set whole-table top border color."
Assert-Equal -Actual $reviewTableFirstRowFillNode.GetAttribute("fill", $styleDefinitionWordNamespace) -Expected "1F4E79" `
    -Message "Ensure-table-style should set first-row fill."

$definitionOnlyNode = $styleDefinitionNumberingDocument.SelectSingleNode("/w:numbering/w:abstractNum[w:name[@w:val='DefinitionOnly']]", $styleDefinitionNumberingNamespaceManager)
$sharedHeadingOutlineNode = $styleDefinitionNumberingDocument.SelectSingleNode("/w:numbering/w:abstractNum[w:name[@w:val='SharedHeadingOutline']]", $styleDefinitionNumberingNamespaceManager)
$sharedHeadingLevelOneNode = $sharedHeadingOutlineNode.SelectSingleNode("w:lvl[@w:ilvl='1']", $styleDefinitionNumberingNamespaceManager)
$heading1NumLevelNode = $styleDefinitionStylesDocument.SelectSingleNode("/w:styles/w:style[@w:styleId='Heading1']/w:pPr/w:numPr/w:ilvl", $styleDefinitionStylesNamespaceManager)

Assert-True -Condition ($null -ne $definitionOnlyNode) `
    -Message "Ensure-numbering-definition should create DefinitionOnly."
Assert-True -Condition ($null -ne $sharedHeadingOutlineNode) `
    -Message "Ensure-style-linked-numbering should create SharedHeadingOutline."
Assert-Equal -Actual $sharedHeadingLevelOneNode.SelectSingleNode("w:start", $styleDefinitionNumberingNamespaceManager).GetAttribute("val", $styleDefinitionWordNamespace) -Expected "2" `
    -Message "Ensure-style-linked-numbering should preserve structured level starts."
Assert-Equal -Actual $heading1NumLevelNode.GetAttribute("val", $styleDefinitionWordNamespace) -Expected "1" `
    -Message "Ensure-style-linked-numbering should link Heading1 at level one."

$styleRefactorSourceDocx = Join-Path $resolvedWorkingDir "style_refactor.source.docx"
$styleRefactorPlanPath = Join-Path $resolvedWorkingDir "style_refactor.edit_plan.json"
$styleRefactorEditedDocx = Join-Path $resolvedWorkingDir "style_refactor.edited.docx"
$styleRefactorSummaryPath = Join-Path $resolvedWorkingDir "style_refactor.edit.summary.json"
$styleRefactorRollbackPath = Join-Path $resolvedWorkingDir "style_refactor.rollback.json"
$styleRefactorRestorePlanPath = Join-Path $resolvedWorkingDir "style_refactor.restore_plan.json"
$styleRefactorRestoredDocx = Join-Path $resolvedWorkingDir "style_refactor.restored.docx"
$styleRefactorRestoreSummaryPath = Join-Path $resolvedWorkingDir "style_refactor.restore.summary.json"

New-StyleRefactorFixtureDocx -Path $styleRefactorSourceDocx
Set-Content -LiteralPath $styleRefactorPlanPath -Encoding UTF8 -Value @"
{
  "operations": [
    {
      "op": "rename_style",
      "old_style_id": "LegacyBody",
      "new_style_id": "ReviewBody"
    },
    {
      "op": "merge_style",
      "source_style_id": "MergeSource",
      "target_style_id": "MergeTarget"
    },
    {
      "op": "apply_style_refactor",
      "renames": [
        {
          "old_style_id": "BulkOld",
          "new_style_id": "BulkNew"
        }
      ],
      "merges": [
        {
          "source_style_id": "BulkMergeSource",
          "target_style_id": "BulkMergeTarget"
        }
      ],
      "rollback_plan": "$($styleRefactorRollbackPath.Replace('\', '\\'))"
    },
    {
      "op": "prune_unused_styles"
    }
  ]
}
"@

& $scriptPath `
    -InputDocx $styleRefactorSourceDocx `
    -EditPlan $styleRefactorPlanPath `
    -OutputDocx $styleRefactorEditedDocx `
    -SummaryJson $styleRefactorSummaryPath `
    -BuildDir $resolvedBuildDir `
    -SkipBuild

if ($LASTEXITCODE -ne 0) {
    throw "edit_document_from_plan.ps1 failed for the style refactor edit plan."
}
