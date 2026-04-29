Release 元数据维护检查清单（中文）
====================================

这份清单用于给维护者提供一个可执行的 release metadata 修改流程。它不是
release 流水线的详细设计文档；详细字段流向请先阅读
:doc:`release_metadata_pipeline_zh`。

适用场景：

- 修改 ``run_word_visual_release_gate.ps1`` 的 review task 生成或计数逻辑。
- 修改 ``run_release_candidate_checks.ps1`` 的 release summary / final review 写入。
- 修改 ``sync_visual_review_verdict.ps1`` 或
  ``sync_latest_visual_review_verdict.ps1`` 的 verdict 回灌逻辑。
- 修改 ``write_release_note_bundle.ps1`` 或 release bundle writer 的输出字段。
- 新增 visual flow 或 curated visual regression bundle。


改动前先确认边界
----------------

开始修改前，先回答下面几个问题：

1. 这次改动属于哪一层？

   - visual gate
   - release candidate preflight
   - verdict sync
   - release note bundle
   - public release material audit

2. 这次改动会写入哪些文件？

   - ``gate_summary.json``
   - ``gate_final_review.md``
   - release ``summary.json``
   - release ``final_review.md``
   - ``START_HERE.md`` / ``ARTIFACT_GUIDE.md`` / ``REVIEWER_CHECKLIST.md``
   - ``release_handoff.md`` / ``release_body.zh-CN.md`` /
     ``release_summary.zh-CN.md``

3. 这些字段是机器消费字段，还是人工审阅字段？

   机器字段应保持结构稳定；人工字段可以更易读，但不能泄露内部备注到公开
   release material。


字段契约检查
------------

涉及 review task count 时，必须确认：

- ``review_task_summary`` 同时包含 ``total_count``、``standard_count``、
  ``curated_count`` 才算完整。
- 不完整计数不能进入 release summary。
- 不完整计数不能渲染到 ``final_review.md``。
- release note bundle 可以从 gate summary 回退读取完整计数。
- 空字符串、空白字符串、``null`` 占位任务不能计入 review scope。

涉及 visual verdict 时，必须确认：

- gate summary 和 release summary 的 ``visual_verdict`` 一致。
- 标准 flow 的 ``review_verdict``、``review_status``、``review_note``、
  ``reviewed_at``、``review_method`` 同步规则一致。
- curated visual regression 优先读取 ``review_verdict``，仅在兼容旧数据时回退
  ``verdict``。
- ``reviewed_at`` 输出保持 ``yyyy-MM-ddTHH:mm:ss`` 格式，避免文化区域差异。

涉及公开 release 正文时，必须确认：

- ``release_body.zh-CN.md`` 不输出 ``review_note``。
- ``release_body.zh-CN.md`` 不输出 ``reviewed_at``。
- ``release_body.zh-CN.md`` 不输出 ``review_method``。
- ``release_summary.zh-CN.md`` 只保留适合首屏展示的 verdict 摘要。
- 公开材料默认经过 ``assert_release_material_safety.ps1`` 审计。


推荐测试矩阵
------------

按改动范围选择最小测试集。先跑最靠近改动的脚本，再跑 CTest 入口。

修改视觉 gate 生成或 review task 计数：

.. code-block:: powershell

    pwsh -NoProfile -ExecutionPolicy Bypass -File `
        .\test\word_visual_release_gate_smoke_verdict_test.ps1 `
        -RepoRoot . `
        -WorkingDir output\codex-word-visual-release-gate-check

    ctest --test-dir build-codex-clang-compat `
        -R "^word_visual_release_gate_smoke_verdict$" `
        --output-on-failure `
        --timeout 60

修改 release candidate preflight：

.. code-block:: powershell

    pwsh -NoProfile -ExecutionPolicy Bypass -File `
        .\test\release_candidate_visual_verdict_test.ps1 `
        -RepoRoot . `
        -WorkingDir output\codex-release-candidate-metadata-check

    ctest --test-dir build-codex-clang-compat `
        -R "^release_candidate_visual_verdict$" `
        --output-on-failure `
        --timeout 60

修改 verdict sync：

.. code-block:: powershell

    ctest --test-dir build-codex-clang-compat `
        -R "^sync_visual_review_verdict_(section_page_setup|page_number_fields|curated_visual_bundle)$" `
        --output-on-failure `
        --timeout 60

修改 release note bundle 或 helper：

.. code-block:: powershell

    ctest --test-dir build-codex-clang-compat `
        -R "^(release_note_bundle_version|release_note_bundle_visual_verdict_fallback|release_visual_verdict_metadata_consistency)$" `
        --output-on-failure `
        --timeout 60

修改公开发布用语：

.. code-block:: powershell

    pwsh -NoProfile -ExecutionPolicy Bypass -File `
        .\test\public_release_wording_regression_test.ps1 `
        -RepoRoot .

该测试包含中文字符串，优先用 ``pwsh``。Windows PowerShell 5 在部分环境下可能
按非 UTF-8 解析脚本，导致无关的 parser error。


自动文档检查
------------

如果只改 release metadata 维护文档，可以先运行下面的轻量检查脚本。它不会构建
项目，也不会接入 CTest；用途是确认 pipeline、maintenance checklist 和 release
policy 之间的关键引用、字段名和测试入口没有断开。

.. code-block:: powershell

    pwsh -NoProfile -ExecutionPolicy Bypass -File `
        .\scripts\check_release_metadata_docs.ps1 `
        -RepoRoot .

需要给自动化留痕时，可以额外输出 JSON 摘要：

.. code-block:: powershell

    pwsh -NoProfile -ExecutionPolicy Bypass -File `
        .\scripts\check_release_metadata_docs.ps1 `
        -RepoRoot . `
        -SummaryJson .\output\release-metadata-docs-summary.json

CI 或其它只读取 JSON 的自动化可以额外加 ``-Quiet``，避免控制台输出成功提示。

检查通过时 JSON 的 ``status`` 为 ``passed``，并包含 ``summary_schema_version``、
``checker_name``、``checked_at_utc``、PowerShell 运行环境、``summary_json_path``、
``summary_json_relative_path``、文档数量和 marker 数量字段；检查失败时也会
尽量写出 ``status=failed`` 和 ``error_message``，但脚本仍保持失败退出码。

如果改动检查脚本本身，额外运行独立回归测试：

.. code-block:: powershell

    pwsh -NoProfile -ExecutionPolicy Bypass -File `
        .\test\check_release_metadata_docs_test.ps1 `
        -RepoRoot . `
        -WorkingDir .\output\check-release-metadata-docs-test

这条检查不能替代 release metadata 的行为回归测试。只要改动涉及脚本输出或
metadata 字段同步，仍然需要按上一节的测试矩阵执行对应测试。


快速人工复核
------------

修改 metadata 管线后，至少打开固定视觉证据图做一次可视化复核：

- ``output\codex-release-gate-section-page-verdict-pass\section-page-setup\aggregate-evidence\contact_sheet.png``
- ``output\codex-release-gate-section-page-verdict-pass\page-number-fields\aggregate-evidence\contact_sheet.png``
- ``output\codex-release-gate-curated-verdict-pass\bookmark-block-visibility\aggregate-evidence\before_after_contact_sheet.png``

复核目标不是重新判断所有 Word 版面，而是确认证据入口仍存在、可打开，且没有
明显的空白、错页或截图拼接异常。


提交前检查
----------

提交前建议按下面顺序检查：

1. ``git status --short``，确认没有带入其它会话的文件。
2. ``git diff --check``，确认没有行尾空白。
3. 只 ``git add`` 本轮目标文件。
4. ``git diff --cached --name-status``，再次确认暂存边界。
5. 提交信息说明具体维护点，例如：

   - ``Harden release metadata fallback counts``
   - ``Document release metadata pipeline``
   - ``Stabilize release note bundle fallback test``

如果当前工作区存在模板 schema、CLI、自定义表格等其它会话改动，不要顺手整理
它们，也不要把它们带进 release metadata 维护提交。


常见问题处理
------------

同组 CTest 接近 60 秒超时
^^^^^^^^^^^^^^^^^^^^^^^^^

优先确认是否是测试重复执行了高成本审计或重复读取同一批 Markdown 文件。聚焦内容
渲染的测试可以考虑使用 ``write_release_note_bundle.ps1 -SkipMaterialSafetyAudit``，
但正式发布路径不能跳过 material safety audit。

release summary 出现空计数
^^^^^^^^^^^^^^^^^^^^^^^^^^

检查 ``review_task_summary`` 是否缺少 ``total_count``、``standard_count`` 或
``curated_count``。不完整计数应被视为缺失，而不是渲染为空字符串。

公开 release body 出现内部备注
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

先跑 ``public_release_wording_regression_test.ps1``，再检查
``write_release_body_zh.ps1`` 是否误用了 handoff-only 字段。公开正文不应包含
operator note、review method 或内部 reviewed_at provenance。

新增 curated bundle 后 summary 不一致
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

确认 bundle 同时进入了：

- gate summary 的 ``curated_visual_regressions``。
- gate summary 的 ``review_tasks.curated_visual_regressions``。
- release summary 的 ``steps.visual_gate.curated_visual_regressions``。
- release bundle helper 的 curated metadata merge 路径。

如果只写入其中一部分，bundle 在不同入口中展示出来的 verdict 或 review status
就可能不一致。


维护原则
--------

- 能集中在 helper 的逻辑，不要分散复制到多个 writer。
- 机器字段优先保持结构稳定；展示文字可以由 writer 负责格式化。
- 公共 release material 默认走安全审计。
- 新增字段时同步补 release summary、final review、bundle writer 和回归测试。
- 文档改动也要更新 ``CHANGELOG.md``，避免维护规则只存在于提交记录里。
