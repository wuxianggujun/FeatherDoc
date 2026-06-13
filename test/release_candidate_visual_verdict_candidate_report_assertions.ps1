$candidateFinalReviewPath = Join-Path $candidateOutputDir "report\final_review.md"
$candidateReleaseBodyPath = Join-Path $candidateOutputDir "report\release_body.zh-CN.md"
$candidateReleaseSummaryPath = Join-Path $candidateOutputDir "report\release_summary.zh-CN.md"
$candidateReleaseHandoffPath = Join-Path $candidateOutputDir "report\release_handoff.md"
$candidateArtifactGuidePath = Join-Path $candidateOutputDir "report\ARTIFACT_GUIDE.md"
$candidateReviewerChecklistPath = Join-Path $candidateOutputDir "report\REVIEWER_CHECKLIST.md"
$candidateStartHerePath = Join-Path $candidateOutputDir "START_HERE.md"

foreach ($assertion in @(
        @{ Path = $candidateFinalReviewPath; Label = "final_review.md" },
        @{ Path = $candidateReleaseHandoffPath; Label = "release_handoff.md" },
        @{ Path = $candidateArtifactGuidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $candidateReviewerChecklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $candidateStartHerePath; Label = "START_HERE.md" }
    )) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $assertion.Path
    Assert-ContainsText -Text $content -ExpectedText "PDF visual gate verdict: pass" `
        -Message ("{0} should expose the PDF visual gate verdict." -f $assertion.Label)
    Assert-ContainsText -Text $content -ExpectedText "44" `
        -Message ("{0} should expose the PDF visual baseline count." -f $assertion.Label)
    Assert-ContainsText -Text $content -ExpectedText "43" `
        -Message ("{0} should expose the PDF CJK count." -f $assertion.Label)
    Assert-ContainsText -Text $content -ExpectedText "aggregate-contact-sheet.png" `
        -Message ("{0} should expose the PDF visual contact sheet." -f $assertion.Label)
}

$finalReviewContent = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateFinalReviewPath
Assert-ContainsText -Text $finalReviewContent -ExpectedText "- Warnings: 1" `
    -Message "final_review.md should expose the top-level warning count."
Assert-MarkdownListRunContainsAll -Text $finalReviewContent -Anchor "PDF visual segmented gate:" -Fragments @(
    "PDF visual segmented gate: pass",
    "PDF visual segmented gate verdict: pass",
    "PDF visual segmented gate full status: not_complete",
    "PDF visual segmented gate scope: segmented_visual_gate_auxiliary_only",
    "PDF visual segmented gate slices: 4/4 pass",
    "PDF visual segmented gate coverage: 44/44 baselines"
) -Message "final_review.md should keep segmented PDF visual gate auxiliary evidence in one step-status list run."
Assert-ContainsText -Text $finalReviewContent -ExpectedText "PDF visual segmented gate summary:" `
    -Message "final_review.md should link segmented PDF visual gate summary evidence."
Assert-ContainsText -Text $finalReviewContent -ExpectedText "segmented-summary.json" `
    -Message "final_review.md should expose the segmented PDF visual gate summary path."
Assert-ContainsText -Text $finalReviewContent -ExpectedText "metadata_only_fields: leftFromText, rightFromText, topFromText outside paragraph anchoring, tblOverlap" `
    -Message "final_review.md should expose generic PDF floating table metadata-only fields."
Assert-ContainsText -Text $finalReviewContent -ExpectedText "review_required_fields: full Word-compatible floating table text wrapping, table overlap avoidance and collision resolution" `
    -Message "final_review.md should expose generic PDF floating table review-required fields."

foreach ($assertion in @(
        @{ Path = $candidateReleaseHandoffPath; Label = "release_handoff.md" },
        @{ Path = $candidateArtifactGuidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $candidateReviewerChecklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $candidateStartHerePath; Label = "START_HERE.md" }
    )) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $assertion.Path
    Assert-MarkdownListRunContainsAll -Text $content -Anchor "PDF visual gate summary:" -Fragments @(
        "PDF visual gate summary:",
        "summary.json",
        "PDF visual gate evidence status: loaded",
        "PDF visual gate verdict: pass",
        "PDF visual gate aggregate contact sheet",
        "aggregate-contact-sheet.png",
        "PDF CJK manifest samples: 43",
        "PDF CJK copy/search samples: 43",
        "PDF CJK missing text count: 0",
        "PDF visual baseline manifest samples: 42",
        "PDF visual baselines: 44"
    ) -Message ("{0} should keep PDF visual status, verdict, paths, and counts in one Markdown list run." -f $assertion.Label)
}

$candidateReleaseHandoff = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReleaseHandoffPath
Assert-ContainsText -Text $candidateReleaseHandoff -ExpectedText "Warnings: 1" `
    -Message "release_handoff.md should expose the propagated warning count."
Assert-ContainsText -Text $candidateReleaseHandoff -ExpectedText "pdf_visual_gate_attempt.incomplete_fresh_render" `
    -Message "release_handoff.md should preserve the PDF fresh-attempt warning id."
Assert-ContainsText -Text $candidateReleaseHandoff -ExpectedText "review_pdf_visual_gate_attempt_and_finalize_evidence" `
    -Message "release_handoff.md should preserve the PDF fresh-attempt warning action."
Assert-ContainsText -Text $candidateReleaseHandoff -ExpectedText "attempt-summary.json" `
    -Message "release_handoff.md should preserve the PDF fresh-attempt warning evidence path."

$candidateFinalReview = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateFinalReviewPath
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Key outputs" -Fragments @(
    "PDF visual gate summary:",
    "summary.json",
    "PDF visual gate contact sheet:",
    "aggregate-contact-sheet.png",
    "PDF bounded CTest summaries:",
    "smoke-summary.json",
    "business-summary.json",
    "PDF release readiness summary:",
    "release-readiness-summary.json",
    "PDF full CTest summary:",
    "pdf-ctest-full-current\summary.json"
) -Message "final_review.md should keep PDF visual and full CTest readiness outputs inside the Key outputs section."
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Step status" -Fragments @(
    "PDF bounded CTest summaries: 2 summaries, 2 pass",
    "PDF bounded CTest subsets: smoke-import, regression-business-samples",
    "PDF bounded CTest selected tests: 20",
    "PDF bounded CTest skipped tests: 0",
    "PDF visual release evidence accepted: True (fresh full guarded False, pass summary before outer timeout False, segmented full coverage True)",
    "PDF full CTest readiness: timeout (95.7% complete)",
    "PDF full CTest progress: 133/139 completed, 6 not run",
    "PDF full CTest observed failures: 0, zero failed observed True"
) -Message "final_review.md should expose bounded and full PDF CTest auxiliary evidence in the Step status section."
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Project-template release entry evidence" -Fragments @(
    "Project-template readiness checklist handoff evidence",
    "project_template_readiness_checklist_entrypoints_source_reports=2",
    "docs/project_template_release_readiness_checklist_zh.rst",
    "required_entrypoint_count=3",
    "entrypoints=start_here, artifact_guide, reviewer_checklist",
    "entrypoint_paths=",
    "release_entry_project_template_readiness_checklist_trace",
    "source_schema=featherdoc.release_candidate_summary",
    "source_report=",
    "summary.json",
    "Project-template readiness checklist packaged audit evidence",
    "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=2",
    "assert_release_material_safety.ps1",
    "audited_entrypoints=start_here, artifact_guide, reviewer_checklist",
    "compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports",
    "compact_evidence_source_schema=featherdoc.release_candidate_summary",
    "project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
    ("source_report={0}" -f $expectedReleaseEntryPackagedAuditSourceReport)
) -Message "final_review.md should expose project-template checklist handoff and packaged audit evidence consumed from release governance handoff."
Assert-MarkdownListBlockContainsAll -Text $candidateFinalReview `
    -Anchor "Release-entry project-template readiness checklist material-safety audit source reports: 2" `
    -Fragments @(
        ("source_report: {0} schema=featherdoc.release_candidate_summary" -f $expectedReleaseEntryChecklistSourceReport),
        ("source_report: {0} schema=featherdoc.release_candidate_summary" -f $expectedReleaseEntryPackagedAuditSummaryReport),
        "release_entry_project_template_readiness_checklist_material_safety_audit_status: passed",
        "release_entry_project_template_readiness_checklist_material_safety_audit_script: .\scripts\assert_release_material_safety.ps1",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoint_count: 3",
        "release_entry_project_template_readiness_checklist_material_safety_audit_audited_entrypoints: start_here, artifact_guide, reviewer_checklist",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_field: project_template_readiness_checklist_entrypoints_source_reports",
        "release_entry_project_template_readiness_checklist_material_safety_audit_compact_evidence_source_schema: featherdoc.release_candidate_summary",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_path: docs/project_template_release_readiness_checklist_zh.rst",
        "release_entry_project_template_readiness_checklist_material_safety_audit_checklist_marker: release_entry_project_template_readiness_checklist_trace",
        "release_entry_project_template_readiness_checklist_material_safety_audit_material_safety_marker: project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace"
    ) -Message "final_review.md should keep release-entry project-template packaged audit source report details in one markdown list block."
Assert-LineContainsAll -Text $candidateFinalReview -Fragments @(
    "Project-template readiness checklist handoff evidence",
    "project_template_readiness_checklist_entrypoints_source_reports=2",
    "docs/project_template_release_readiness_checklist_zh.rst",
    "required_entrypoint_count=3",
    "entrypoint_paths=",
    "release_entry_project_template_readiness_checklist_trace",
    "source_schema=featherdoc.release_candidate_summary",
    ("source_report={0}" -f $expectedReleaseEntryChecklistSourceReport)
) -Message "final_review.md should keep project-template checklist handoff evidence on one compact source_report line."
Assert-LineContainsAll -Text $candidateFinalReview -Fragments @(
    "Project-template readiness checklist packaged audit evidence",
    "release_entry_project_template_readiness_checklist_material_safety_audit_source_reports=2",
    "audit_script=.\scripts\assert_release_material_safety.ps1",
    "audited_entrypoints=start_here, artifact_guide, reviewer_checklist",
    "compact_evidence_field=project_template_readiness_checklist_entrypoints_source_reports",
    "compact_evidence_source_schema=featherdoc.release_candidate_summary",
    "checklist_path=docs/project_template_release_readiness_checklist_zh.rst",
    "checklist_marker=release_entry_project_template_readiness_checklist_trace",
    "material_safety_marker=project_template_readiness_checklist_entrypoints_release_entry_material_safety_trace",
    "source_schema=featherdoc.release_candidate_summary",
    ("source_report={0}" -f $expectedReleaseEntryPackagedAuditSourceReport)
) -Message "final_review.md should keep project-template packaged audit evidence on one compact source_report line."
Assert-MarkdownSectionContainsAll -Text $candidateFinalReview -Heading "## Word visual standard review metadata evidence" -Fragments @(
    "Word visual standard review metadata evidence",
    "word_visual_standard_review_metadata_source_reports=1",
    "metadata_count=4",
    "task_keys=smoke, fixed_grid, section_page_setup, page_number_fields",
    "status_summary=reviewed=4",
    "verdict_summary=pass=4",
    "smoke:review_task_key=document",
    "fixed_grid:review_task_key=fixed_grid",
    "section_page_setup:review_task_key=section_page_setup",
    "page_number_fields:review_task_key=page_number_fields",
    "review_result_path=",
    "final_review_path=",
    "source_schema=featherdoc.release_candidate_summary",
    "source_report=",
    "release-candidate-checks-source\summary.json"
) -Message "final_review.md should expose compact Word visual standard review metadata evidence consumed from release governance handoff."
Assert-DoesNotContainText -Text $candidateFinalReview -UnexpectedText "review_note" `
    -Message "final_review.md compact Word visual metadata evidence should not expose private operator review notes."
Assert-MarkdownListRunContainsAll -Text $candidateFinalReview -Anchor "project_template_delivery_readiness / project_template_onboarding.schema_approval" -Fragments @(
    "project_template_delivery_readiness / project_template_onboarding.schema_approval",
    "project_template_onboarding_governance_contract",
    "featherdoc.project_template_onboarding_governance_report.v1",
    "schema_approval_status_summary=not_required=3",
    "readiness_status: ready",
    "readiness_release_ready: True",
    "source_report_display:",
    "project-template-onboarding-governance",
    "source_json_display:",
    "project-template-onboarding-governance"
) -Message "final_review.md should keep project-template readiness and onboarding governance contracts in one material-safety block."

foreach ($assertion in @(
        @{ Path = $candidateReleaseHandoffPath; Label = "release_handoff.md" },
        @{ Path = $candidateArtifactGuidePath; Label = "ARTIFACT_GUIDE.md" },
        @{ Path = $candidateReviewerChecklistPath; Label = "REVIEWER_CHECKLIST.md" },
        @{ Path = $candidateStartHerePath; Label = "START_HERE.md" }
    )) {
    $content = Get-Content -Raw -Encoding UTF8 -LiteralPath $assertion.Path
    Assert-MarkdownListRunContainsAll -Text $content -Anchor "project_template_delivery_readiness:" -Fragments @(
        "project_template_delivery_readiness",
        "project_template_delivery_readiness_contract",
        "featherdoc.project_template_delivery_readiness_report.v1",
        "status: ready",
        "release_ready: True",
        "latest_schema_approval_gate_status",
        "schema_approval_status_summary=not_required=3",
        "source_report_display:",
        "project-template-delivery-readiness",
        "source_json_display:",
        "project-template-delivery-readiness"
    ) -Message ("{0} should keep project-template delivery readiness contract evidence in one list block." -f $assertion.Label)
    Assert-MarkdownListRunContainsAll -Text $content -Anchor "project_template_onboarding.schema_approval" -Fragments @(
        "project_template_onboarding.schema_approval",
        "project_template_onboarding_governance_contract",
        "featherdoc.project_template_onboarding_governance_report.v1",
        "status: ready",
        "release_ready: True",
        "schema_approval_status_summary=not_required=3",
        "source_report_display:",
        "project-template-onboarding-governance",
        "source_json_display:",
        "project-template-onboarding-governance"
    ) -Message ("{0} should keep project-template onboarding governance contract evidence in one list block." -f $assertion.Label)
}

$candidateReleaseBody = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReleaseBodyPath
foreach ($fragments in @(
        @("PDF visual gate verdict", "pass"),
        @("PDF visual gate aggregate contact sheet", "aggregate-contact-sheet.png"),
        @("PDF CJK copy/search samples", "43"),
        @("PDF visual baselines", "44")
    )) {
    foreach ($fragment in $fragments) {
        Assert-ContainsText -Text $candidateReleaseBody -ExpectedText $fragment `
            -Message ("release_body.zh-CN.md should expose PDF visual gate fragment '{0}'." -f $fragment)
    }
}
Assert-MarkdownListRunContainsAll -Text $candidateReleaseBody -Anchor "PDF visual gate summary" -Fragments @(
    "PDF visual gate summary",
    "summary.json",
    "PDF visual gate evidence status",
    "loaded",
    "PDF visual gate verdict",
    "pass",
    "PDF visual gate aggregate contact sheet",
    "aggregate-contact-sheet.png",
    "PDF CJK manifest samples",
    "43",
    "PDF CJK copy/search samples",
    "43",
    "PDF CJK missing text count",
    "0",
    "PDF visual baseline manifest samples",
    "42",
    "PDF visual baselines",
    "44"
) -Message "release_body.zh-CN.md should keep PDF visual status, verdict, paths, and counts in one Markdown list run."

$candidateReleaseSummary = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReleaseSummaryPath
foreach ($fragment in @(
        "PDF visual gate",
        "verdict=pass",
        "aggregate-contact-sheet.png",
        "cjk_copy_search_count=43",
        "visual_baseline_count=44"
    )) {
    Assert-ContainsText -Text $candidateReleaseSummary -ExpectedText $fragment `
        -Message ("release_summary.zh-CN.md should expose PDF visual gate fragment '{0}'." -f $fragment)
}
Assert-LineContainsAll -Text $candidateReleaseSummary -Fragments @(
    "PDF visual gate",
    "verdict=pass",
    "summary=",
    "summary.json",
    "aggregate_contact_sheet=",
    "aggregate-contact-sheet.png",
    "cjk_manifest_count=43",
    "cjk_copy_search_count=43",
    "visual_baseline_manifest_count=42",
    "visual_baseline_count=44"
) -Message "release_summary.zh-CN.md should keep the PDF visual verdict, paths, and counts on one summary line."

$candidateReviewerChecklist = Get-Content -Raw -Encoding UTF8 -LiteralPath $candidateReviewerChecklistPath
Assert-LineContainsAll -Text $candidateReviewerChecklist -Fragments @(
    'Confirm the PDF visual gate finalize evidence is signed off',
    'verdict `pass`',
    'summary',
    'summary.json',
    'aggregate contact sheet',
    'aggregate-contact-sheet.png',
    'CJK manifest samples `43`',
    'CJK copy/search samples `43`',
    'missing text `0`',
    'visual baseline manifest samples `42`',
    'visual baselines `44`'
) -Message "REVIEWER_CHECKLIST.md should keep PDF visual finalize verdict, paths, and counts on one reviewer signoff line."

$localAbsolutePathPattern = '(?i)\b[a-z]:(?:\\\\|\\)[^\s"''`<>|]+|(?<!\w)/(?:Users|home)/[^\s"''`<>|]+'
foreach ($releaseMaterialPath in @(
        $candidateSummaryPath,
        $candidateFinalReviewPath,
        $candidateReleaseBodyPath,
        $candidateReleaseSummaryPath,
        $candidateReleaseHandoffPath,
        $candidateArtifactGuidePath,
        $candidateReviewerChecklistPath,
        $candidateStartHerePath
    )) {
    $pathLeak = Select-String -LiteralPath $releaseMaterialPath -Pattern $localAbsolutePathPattern -AllMatches | Select-Object -First 1
    if ($null -ne $pathLeak) {
        throw "Release material contains a local absolute path leak: $releaseMaterialPath"
    }
}

Write-Host "Release candidate visual verdict passthrough regression passed."
