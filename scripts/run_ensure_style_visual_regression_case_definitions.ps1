$caseDefinitions = @(
    [pscustomobject][ordered]@{
        id = "ensure-paragraph-style-heading-visual"
        command = "ensure-paragraph-style"
        arguments = @(
            "ReviewPara",
            "--name", "Review Paragraph",
            "--based-on", "Heading2",
            "--next-style", "ReviewPara",
            "--custom", "true",
            "--semi-hidden", "false",
            "--unhide-when-used", "true",
            "--quick-format", "true",
            "--paragraph-bidi", "true",
            "--outline-level", "2"
        )
        expected_visual_cues = @(
            "The ReviewPara paragraph grows into a Heading 2 sized callout instead of staying monospaced body text.",
            "The paragraph keeps the same ReviewPara style binding; only the style definition changes.",
            "The ReviewParaChild paragraph also inherits the Heading 2 restyle through the basedOn chain instead of keeping the baseline Courier New body look.",
            "The AccentMarker run remains visually and structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "ensure-character-style-serif-visual"
        command = "ensure-character-style"
        arguments = @(
            "AccentMarker",
            "--name", "Accent Marker",
            "--based-on", "DefaultParagraphFont",
            "--custom", "true",
            "--semi-hidden", "false",
            "--unhide-when-used", "false",
            "--quick-format", "true",
            "--run-font-family", "Times New Roman",
            "--run-language", "fr-FR",
            "--run-rtl", "false"
        )
        expected_visual_cues = @(
            "The AccentMarker run drops the baseline Courier New look and switches to a proportional serif face.",
            "The prefix and suffix around the target run stay in the document default formatting.",
            "The ReviewPara and ReviewParaChild paragraphs remain visually and structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "materialize-style-run-properties-child-freeze-visual"
        command = "ensure-paragraph-style"
        steps = @(
            [pscustomobject][ordered]@{
                command = "materialize-style-run-properties"
                arguments = @("ReviewParaChild")
            },
            [pscustomobject][ordered]@{
                command = "ensure-paragraph-style"
                arguments = @(
                    "ReviewPara",
                    "--name", "Review Paragraph",
                    "--based-on", "Normal",
                    "--next-style", "ReviewPara",
                    "--custom", "true",
                    "--semi-hidden", "false",
                    "--unhide-when-used", "false",
                    "--quick-format", "true",
                    "--run-font-family", "Times New Roman"
                )
            }
        )
        expected_visual_cues = @(
            "The ReviewPara paragraph switches from the baseline Courier New face to Times New Roman.",
            "The ReviewParaChild paragraph keeps the materialized Courier New face even after its parent style changes.",
            "The AccentMarker run and ReviewTable target remain visually and structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "rebase-paragraph-style-based-on-child-preserve-visual"
        command = "ensure-paragraph-style"
        steps = @(
            [pscustomobject][ordered]@{
                command = "rebase-paragraph-style-based-on"
                arguments = @("ReviewParaChild", "Normal")
            },
            [pscustomobject][ordered]@{
                command = "ensure-paragraph-style"
                arguments = @(
                    "ReviewPara",
                    "--name", "Review Paragraph",
                    "--based-on", "Normal",
                    "--next-style", "ReviewPara",
                    "--custom", "true",
                    "--semi-hidden", "false",
                    "--unhide-when-used", "false",
                    "--quick-format", "true",
                    "--run-font-family", "Times New Roman"
                )
            }
        )
        expected_visual_cues = @(
            "The ReviewPara paragraph switches from the baseline Courier New face to Times New Roman after the parent style rewrite.",
            "The ReviewParaChild paragraph keeps the rebased Courier New face even though it no longer inherits from ReviewPara.",
            "The AccentMarker run and ReviewTable target remain visually and structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "rebase-character-style-based-on-child-preserve-visual"
        command = "rebase-character-style-based-on"
        arguments = @("AccentMarkerChild", "AccentParentSerif")
        expected_visual_cues = @(
            "The inherited AccentMarkerChild run keeps the same Courier New monospace look even after its basedOn chain changes from AccentParentMono to AccentParentSerif.",
            "The preserved inherited font is materialized onto AccentMarkerChild itself instead of silently drifting to the new serif parent.",
            "The ReviewPara paragraphs, AccentMarker run, and ReviewTable target remain visually and structurally unchanged in this case."
        )
    },
    [pscustomobject][ordered]@{
        id = "ensure-table-style-borderless-visual"
        command = "ensure-table-style"
        arguments = @(
            $reviewTableStyleId,
            "--name", $reviewTableStyleName,
            "--based-on", "TableNormal",
            "--custom", "true",
            "--semi-hidden", "false",
            "--unhide-when-used", "true",
            "--quick-format", "true"
        )
        expected_visual_cues = @(
            "The ReviewTable target table drops the baseline TableGrid borders and renders as a borderless TableNormal-derived layout.",
            "The table keeps the same ReviewTable style binding; only the style definition changes.",
            "The ReviewPara and ReviewParaChild paragraphs plus the AccentMarker run remain visually and structurally unchanged in this case."
        )
    }
)

$summaryCases = @()
$aggregateImages = @()
$aggregateLabels = @()
