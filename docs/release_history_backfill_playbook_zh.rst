:orphan:

历史 Release 回补操作手册（中文）
=================================

这份文档用于沉淀 ``FeatherDoc`` 历史 GitHub Release 回补流程，目标是把旧版本
公开材料修正到可继续对外展示的状态，同时避免把本机绝对路径、草稿文案或不真实的
验证结论重新公开。

如果处理的是新版本正式发版，请优先遵循 :doc:`release_policy_zh`。本手册只用于
历史版本回补、正文修订、附件重建和重新上传。


适用范围
--------

这套流程主要覆盖 ``v1.0.0`` 到 ``v1.6.4`` 这一批历史 release。

回补工作的硬约束如下：

1. 公开正文里不得出现 ``draft``、``草稿`` 等面向内部流转的字样。
2. 公开正文和附件里不得泄漏本机绝对路径，例如 ``C:\Users\...`` 或 ``/Users/...``。
3. 不得为旧版本捏造不存在的验证入口、模板文件或发布门禁结论。
4. 每个版本的证据来源必须和该版本真实可获得的脚本、样例、安装产物保持一致。


版本分层策略
------------

历史版本不要按一套脚本硬推到底，而是按版本分层处理：

1. ``v1.6.0`` 到 ``v1.6.4``：

   基于现有 release 体系已经生成的产物做脱敏和重传，重点检查正文、handoff、
   reviewer material 以及截图说明里的禁词和路径泄漏。

2. ``v1.5.0``：

   使用单独的一次性回补脚本处理。这个版本要额外删掉并不存在的 quickstart /
   template 引用，不能直接套后续版本的安装包说明。

3. ``v1.2.0`` 到 ``v1.4.0``：

   切到对应 tag，重建源码包、MSVC build / ctest / install，并运行该 tag 自带的
   ``run_word_visual_smoke.ps1`` 生成视觉证据。

4. ``v1.0.0`` 到 ``v1.1.0``：

   切到对应 tag，重建 install 产物；视觉证据只允许基于该 tag 的
   ``samples/my_test.docx`` 做样本重建，不得冒充后期 release gate。


常用脚本清单
------------

历史回补过程中实际使用过的关键脚本集中在 ``output/release-history``：

1. ``retouch_release_evidence_wording.ps1``：

   批量修正文案，把公开材料里的草稿措辞改成可公开的 release note / review wording。

2. ``backfill_sanitized_evidence_assets.ps1``：

   对已有证据包做脱敏后重新生成可上传附件。

3. ``backfill_v1_5_0_release_assets.ps1``：

   专门处理 ``v1.5.0`` 的正文与附件回补。

4. ``backfill_v1_4_0_install_asset.ps1``：

   单独回补 ``v1.4.0`` install 资产。

5. ``rebuild_legacy_install_asset.ps1``：

   旧版本 install 资产重建入口，适用于较早 tag。

6. ``backfill_legacy_visual_smoke_assets.ps1``：

   为 ``v1.2.0`` 到 ``v1.4.0`` 这类带 visual smoke 能力的 tag 回补视觉证据。

7. ``backfill_legacy_sample_visual_assets.ps1``：

   为 ``v1.0.0`` 到 ``v1.1.0`` 基于 ``samples/my_test.docx`` 的样本证据重建入口。

完整审计记录可参考
``output/release-history/historical_release_backfill_audit_2026-04-12.md``。


推荐执行顺序
------------

建议固定按下面顺序执行，避免先上传后返工：

1. 先审正文。

   先看 release body、summary、handoff 是否含有 ``draft``、``草稿``、本机路径，
   再决定是否需要整段重写。

2. 再判版本层级。

   先确认目标 tag 属于哪一层，再选对应脚本和证据来源，不要跨版本挪用产物。

3. 再重建或脱敏附件。

   能复用的就脱敏重打包，不能复用的就切 tag 重建，不保留带泄漏风险的旧附件。

4. 上传前做禁词和泄漏扫描。

   正文、Markdown、JSON、日志摘要、截图说明都要扫描。只要命中本机路径或草稿词，
   就不能上传。

5. 最后校验远端状态。

   确认 release 不是 draft / prerelease，附件数量与命名符合预期，正文里也没有
   回补前遗留字样。


禁词与泄漏检查项
----------------

公开材料至少要过下面几类检查：

1. 文本禁词：

   ``draft``、``Draft``、``草稿``、``reviewer handoff material`` 这类只适合内部
   交接的措辞不能直接出现在对外正文中，必须改成公开语义。

2. 本机路径：

   ``C:\Users\``、``/Users/``、``/home/``、临时目录、桌面路径等都应视为泄漏。

3. 失效引用：

   旧版本如果并不存在 ``VISUAL_VALIDATION_QUICKSTART``、模板样例或某个 report，
   就不能把这些路径写进 release body。

4. 证据边界：

   只能写该版本真实跑过的 build、ctest、install、visual smoke 或样本文档重建。
   不能把后来版本才有的 release-preflight 门禁回写成旧版本既有事实。

如需本地先做文本扫描，可直接对生成目录运行：

.. code-block:: powershell

    rg -n "draft|Draft|草稿" .\output\release-history
    rg -n "C:\\Users\\|/Users/|/home/" .\output\release-history


视觉证据边界说明
----------------

视觉证据最容易被误写，必须单独强调：

1. ``v1.0.0`` 到 ``v1.1.0`` 的截图证据只代表样本文档在 Word 中可打开、可渲染，
   不代表这些版本已经具备后期 release gate 或 fixed-grid quartet 的完整签收链路。

2. ``v1.2.0`` 到 ``v1.4.0`` 可以使用各自 tag 的 ``run_word_visual_smoke.ps1``，
   但仍应按对应 tag 的真实输出描述，不要混入后期额外加上的复核产物。

3. ``v1.5.0`` 以及 ``v1.6.x`` 如果复用现有 release 体系产物，前提是先完成脱敏，
   并确认文案没有把内部 review bundle 误称为公开 release 草稿。


完成条件
--------

一次历史回补完成后，至少应满足：

1. 远端 ``isDraft=false``。
2. 远端 ``isPrerelease=false``。
3. 正文不再出现 ``draft``、``草稿``、本机绝对路径。
4. 公开附件已经过禁词与泄漏扫描。
5. 审计结果已经补记到 ``output/release-history`` 下的回补记录中。

如果只是准备新版本发布，不要再走这份手册，直接回到 :doc:`release_policy_zh`
中的正式发版流程。


2026-04-12 基线状态
-------------------

截至 ``2026-04-12``，这一轮历史回补已经达到下面的基线：

1. 远端 ``v1.0.0`` 到 ``v1.6.4`` 均为 ``draft=false``。
2. 远端 ``v1.0.0`` 到 ``v1.6.4`` 均为 ``prerelease=false``。
3. 上述版本当前均保留 3 个公开附件：

   - ``FeatherDoc-vX.Y.Z-msvc-install.zip``
   - ``FeatherDoc-vX.Y.Z-visual-validation-gallery.zip``
   - ``FeatherDoc-vX.Y.Z-release-evidence.zip``

4. 公开正文已经再次复扫，不再包含 ``draft``、``草稿`` 或本机绝对路径。
5. 详细过程、分层策略和最终远端清点结果见
   ``output/release-history/historical_release_backfill_audit_2026-04-12.md``。
