治理与自动化路线索引
====================

本页承接仓库中较长的治理、自动化和脚本路线说明。README 只保留入口，详细
marker 与脚本契约集中在这里和对应专题页中维护。

Release Governance Closure
--------------------------

* ``scripts/build_release_governance_pipeline_report.ps1``
* ``build_release_governance_pipeline_report.ps1``
* ``local_governance_closure``
* ``local_governance_closure.closed``
* ``docx_functional_smoke_readiness``
* ``release_governance_handoff``
* ``release_blocker_rollup``

Word Visual Preflight
---------------------

* ``scripts/check_word_visual_release_gate_preflight.ps1``
* ``check_word_visual_release_gate_preflight.ps1``
* ``featherdoc.word_visual_release_gate_preflight.v1``
* ``word_visual_release_gate_preflight_static_contract_only``
* ``minimum_risk_next_action_command``
* ``strict_preflight_command_template``
* ``full_gate_command_template``
* ``evidence_scope_note``
* ``boundary``
* ``static scripts, docs, and test registration only``
* ``output_encoding``
* ``UTF-8 without BOM``
* ``does not run Word, CMake, CTest``
* ``release-ready evidence``

Render Data Mapping
-------------------

* ``scripts/convert_render_data_to_patch_plan.ps1``
* ``samples/template_render_data_mapping.schema.json``
* ``scripts/export_render_data_mapping_draft.ps1``
* ``scripts/lint_render_data_mapping.ps1``
* ``scripts/prepare_template_render_data_workspace.ps1``
* ``scripts/validate_render_data_mapping.ps1``
* ``scripts/render_template_document_from_workspace.ps1``
* ``scripts/render_template_document_from_data.ps1``
* ``TODO: <bookmark_name>``
* ``output_patch_path``
* ``remaining_placeholder_count=0``
* ``resolved-section-targets``
* ``export_target_mode``

Content-Control Data Binding
----------------------------

* ``inspect-content-controls``
* ``set-content-control-form-state``
* ``sync_content_controls_from_custom_xml()``
* ``sync-content-controls-from-custom-xml``
* ``content_control_summary``
* ``dataBinding``
* ``content_control_tag``
* ``content_control_alias``
* ``build_content_control_data_binding_governance_report.ps1``
* ``content-control data-binding governance``
* ``output/content-control-data-binding-governance/summary.json``
* ``featherdoc.content_control_data_binding_governance_report.v1``
* ``content_control_data_binding.custom_xml_sync_issue``
* ``content_control_data_binding.bound_placeholder``
* ``sync_bound_content_control``
* ``source_schema``
* ``source_report_display``
* ``source_json_display``
* ``open_command``
* ``input_docx``
* ``template_name``
* ``schema_target``
* ``target_mode``
* ``ReleaseBlockerRollupAutoDiscover``
* ``ReleaseBlockerRollupInputRoot``

Schema Patch Confidence Calibration
-----------------------------------

* ``scripts/write_schema_patch_confidence_calibration_report.ps1``
* ``write_schema_patch_confidence_calibration_report.ps1``
* ``output/schema-patch-confidence-calibration``
* ``schema-patch-confidence-calibration``
* ``schema-patch-confidence-calibration/summary.json``
* ``schema_patch_confidence_calibration.md``
* ``featherdoc.schema_patch_confidence_calibration_report.v1``
* ``confidence buckets``
* ``approval outcomes``
* ``recommended_min_confidence``
* ``release blocker rollup``
* ``release_blockers``
* ``warnings``
* ``action_items``
* ``source_schema``
* ``source_report_display``
* ``source_json_display``
* ``open_command``
* ``-FailOnPending``

Style Merge Confidence
----------------------

* ``suggest-style-merges``
* ``style-merge-suggestions.json``
* ``style-merge.rollback.json``
* ``restore-style-merge``
* ``suggestion_confidence_summary``
* ``confidence-profile recommended|strict|review|exploratory``
* ``--min-confidence <0-100>``
* ``--fail-on-suggestion``
* ``recommended_min_confidence``
* ``manual_review_required``
* ``manual_review_reason_count``
* ``manual_review_reasons``
* ``manual_review_before_apply``

Document Skeleton Governance
----------------------------

* ``scripts/build_document_skeleton_governance_report.ps1``
* ``scripts/build_document_skeleton_governance_rollup_report.ps1``
* ``build_document_skeleton_governance_report.ps1``
* ``build_document_skeleton_governance_rollup_report.ps1``
* ``output/document-skeleton-governance``
* ``output/document-skeleton-governance-rollup``
* ``featherdoc.document_skeleton_governance_report.v1``
* ``featherdoc.document_skeleton_governance_rollup_report.v1``
* ``document_skeleton_governance.summary.json``
* ``summary.json``
* ``exemplar.numbering-catalog.json``
* ``style-merge-suggestions.json``
* ``style_numbering_issue_count``
* ``style_merge_suggestion_count``
* ``style_merge_manual_review_reason_count``
* ``manual_review_reasons``
* ``manual_review_before_apply``
* ``document skeleton governance``
* ``exemplar catalog``
* ``release blockers``
* ``action items``
* ``build_release_blocker_rollup_report.ps1``
* ``source_schema``
* ``source_report_display``
* ``source_json_display``
* ``open_command``

Numbering Catalog Governance
----------------------------

* ``scripts/build_numbering_catalog_governance_report.ps1``
* ``build_numbering_catalog_governance_report.ps1``
* ``output/numbering-catalog-governance/summary.json``
* ``featherdoc.numbering_catalog_governance_report.v1``
* ``featherdoc.document_skeleton_governance_rollup_report.v1``
* ``numbering catalog governance``
* ``numbering_catalog_governance.real_corpus_confidence``
* ``real_corpus_confidence``
* ``real_corpus_alignment``
* ``missing_baseline``
* ``missing_exemplar``
* ``numbering_catalog_governance.missing_baseline``
* ``numbering_catalog_governance.missing_exemplar``
* ``numbering_catalog_governance.real_corpus_alignment_gap``
* ``source_schema``
* ``source_report_display``
* ``source_json_display``
* ``open_command``
* ``ReleaseBlockerRollupAutoDiscover``

DOCX Functional Smoke Readiness
-------------------------------

* ``scripts/build_release_governance_pipeline_report.ps1``
* ``docx_functional_smoke_readiness_zh``
* ``docx_functional_smoke_readiness``
* ``scripts/check_docx_functional_smoke_readiness.ps1``
* ``check_docx_functional_smoke_readiness.ps1``
* ``docx-functional-smoke-readiness/summary.json``
* ``featherdoc.docx_functional_smoke_readiness.v1``
* ``docx_functional_smoke_readiness.md``
* ``word_visual_smoke.pending_manual_review``
* ``persisted_docx_functional_smoke_evidence_only``
* ``summary_json_display``
* ``report_markdown_display``
* ``read-only``
* ``Word``

Table Layout Delivery
---------------------

* ``scripts/build_table_layout_delivery_report.ps1``
* ``scripts/build_table_layout_delivery_rollup_report.ps1``
* ``featherdoc.table_layout_delivery_rollup_report.v1``
* ``scripts/build_table_layout_delivery_governance_report.ps1``
* ``build_table_layout_delivery_governance_report.ps1``
* ``output/table-layout-delivery-governance/summary.json``
* ``featherdoc.table_layout_delivery_governance_report.v1``
* ``table_layout_delivery_governance``
* ``table-layout-delivery-governance``
* ``table_layout_delivery_governance.delivery_quality``
* ``delivery_quality``
* ``safe_tblLook_fixes_pending``
* ``floating_table_plans_pending``
* ``tblLook``
* ``floating table``
* ``pdf_floating_table_support``
* ``pdf_floating_table_supported_geometry_percent``
* ``pdf_floating_table_tracked_geometry_count``
* ``fixed_layout_table_count``
* ``autofit_layout_table_count``
* ``unspecified_layout_table_count``
* ``pdf_floating_table_reviewer_focus``
* ``pdf_floating_table_metadata_only_fields``
* ``pdf_floating_table_review_required_fields``
* ``metadata-only``
* ``source_schema``
* ``source_report_display``
* ``source_json_display``
* ``open_command``
* ``release blockers``
* ``action items``

Project Template Release Readiness
----------------------------------

Warning-only evidence gaps now produce ``status=needs_review`` and
``release_ready=false`` until ``onboarding_summary.json`` evidence is complete.

* ``scripts/build_project_template_delivery_readiness_report.ps1``
* ``status=needs_review``
* ``release_ready=false``
* ``onboarding_summary.json``
* ``output/release-candidate-checks/START_HERE.md``
* ``release_governance_handoff.md``
* ``word_visual_standard_review_metadata_source_reports``
* ``START_HERE.md``
* ``ARTIFACT_GUIDE.md``
* ``REVIEWER_CHECKLIST.md``
* ``final_review.md``
* ``release_handoff.md``
* ``Word visual standard review metadata evidence``
