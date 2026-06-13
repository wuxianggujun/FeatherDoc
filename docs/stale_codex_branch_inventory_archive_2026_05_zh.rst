:orphan:

2026-05-20 分支删除决策与下一步
-------------------------------

本轮再次确认本地只有 ``dev``，远端仍保留 4 个参考分支：

* ``origin/codex/pdf-cjk-copy-search-gate``。
* ``origin/codex/pdf-cjk-bullet-fallback``。
* ``origin/codex/release-governance-rollup-details``。
* ``origin/codex/release-governance-warning-entrypoints``。

当前 ``dev`` 与 ``origin/dev`` 已对齐，工作区干净。最新的
``compare_pdf_reference_branch_manifest.ps1 -FailOnMissingInDev`` 复核结果是：

* 当前 ``dev`` manifest 样例数为 90。
* 两个 PDF CJK 参考分支总缺口为 0。
* 结论为 ``manifest_ids_already_covered_in_dev``。

因此，分支处理策略保持不变：

* 不为减少分支数量而删除全部旧分支。
* 不整分支合并、不强推、不改写历史。
* 两个 PDF CJK 分支继续只读保留，作为 visual baseline、大型样例和历史 gate 的参考库。
* 两个 release governance 分支继续只读保留，作为旧治理入口和 warning / rollup 设计参考。

只有同时满足以下条件后，才考虑删除远端 ``origin/codex/*`` 参考分支：

1. 当前 ``dev`` 已完成最终轻量验证并推送。
2. PDF preflight blocker 已清零，或已经明确记录不再使用旧分支 baseline 的原因。
3. 受控 PDF 可视化验证和必要的 release governance 材料已经闭环。
4. 删除前先保留归档记录，例如 tag、bundle、文档清单或远端分支 SHA 清单。
5. 用户再次明确要求删除具体分支。

下一步最小风险推进方向：

1. 继续围绕当前 ``dev`` 准备可复用 build / CTest / PDF baseline 输出，让 PDF preflight
   从 ``not_ready`` 进入 ``ready``。
2. 在资源允许且源码已提交推送、工作区干净后，再执行受控 PDF 可视化验证。
3. 可视化验证通过后，再决定是否归档并清理旧 ``origin/codex/*`` 分支。

2026-05-20 普通 build 目录回归复核
-----------------------------------

本轮继续保持低资源收尾，只在 ``test/pdf_visual_release_gate_preflight_test.ps1`` 中补充
一个普通 ``build\tmp`` 场景，并提交推送为：

.. code-block:: text

   b63908c Cover plain build PDF preflight selection

该回归确认：当仓库只有普通 ``build`` 子目录、且没有 ``CMakeCache.txt`` 或
``CTestTestfile.cmake`` 时，``check_pdf_visual_release_gate_preflight.ps1`` 不会把它
自动选为可复用 PDF build。summary 仍保留 ``build_dir_source = requested``，
``build_dir`` 指向默认 ``.bpdf-roundtrip-msvc``，并用 ``build_dir_exists`` 阻断项
说明真正缺少的是可复用 build 输出。

验证范围仍是轻量级：

* PowerShell 解析检查通过。
* ``git diff --check`` 通过，仅保留既有 CRLF 工作区提示。
* ``pdf_visual_release_gate_preflight_test.ps1`` 在 60 秒超时包装下通过。

分支判断不变：两个 PDF CJK 参考分支的 manifest 样例入口已在当前 ``dev`` 覆盖，
但完整 PDF visual release gate 仍缺可复用 build / CTest / baseline 输出。因此这些
远端参考分支继续只读保留，不删除、不强推、不整分支合并。

2026-05-21 远端参考分支再复核
-----------------------------

本轮在 ``dev`` 与 ``origin/dev`` 对齐、工作区干净后执行 ``git fetch --prune origin``，
并再次只读复核四个远端参考分支：

* ``origin/codex/pdf-cjk-copy-search-gate``。
* ``origin/codex/pdf-cjk-bullet-fallback``。
* ``origin/codex/release-governance-rollup-details``。
* ``origin/codex/release-governance-warning-entrypoints``。

复核结论仍然是：不整分支合并、不强推、不删除远端参考分支。直接对比
``dev..origin/codex/...`` 后可以看到这些分支仍是旧快照，覆盖回当前 ``dev`` 会删除或
回退已经恢复的文档、PDF preflight、controlled smoke、release metadata 契约和多轮
PDF CJK 轻量样例记录。

本轮重点复核 ``release-governance-warning-entrypoints`` 的最新小提交
``d10f8d8 Preserve content-control governance source metadata``。该提交的目标已经被
当前 ``dev`` 更完整覆盖：``build_content_control_data_binding_governance_report.ps1``
已经为 blocker、action item、repair plan 和 warning 保留 ``source_schema``、
``source_json_display``、``source_report_display``、``repair_strategy``、
``command_template`` 与相关 markdown 输出；对应契约也已经在
``build_content_control_data_binding_governance_report_test.ps1`` 和
``release_governance_warning_helper_contract_test.ps1`` 中固定。因此该旧提交不再摘入，
避免用旧实现回退当前治理字段。

``release-governance-rollup-details`` 中的 schema calibration、handoff、pipeline 与
release bundle 方向也已被当前 ``dev`` 的新结构覆盖；剩余差异会回退当前 release
metadata pipeline 与 PDF preflight runbook 记录，不作为低风险补丁来源。两个 PDF CJK
参考分支仍以重型 regression 样例、visual baseline 和历史 gate 证据为主；在完整 PDF
visual release gate 未基于当前 ``dev`` 通过前，它们继续只读保留。

下一步最小风险动作不再是继续搬旧分支代码，而是复用当前 ``dev`` 已有输出，推进
PDF preflight 缺口清零或形成明确豁免记录；只有在 preflight 与受控可视化验证闭环后，
再判断这些远端参考分支是否可以归档或删除。
