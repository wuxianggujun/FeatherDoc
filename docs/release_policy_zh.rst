版本与发布策略（中文）
======================

这份文档用于定义 ``FeatherDoc`` 当前 fork 的版本策略、发布检查项和兼容性边界。


版本策略
--------

当前项目采用 ``MAJOR.MINOR.PATCH`` 形式的版本号。

1. ``MAJOR``

   用于公开 API 不兼容变更、保存/打开语义重大变化、安装导出结构重大调整。

2. ``MINOR``

   用于新增能力、向后兼容的新 API、性能优化、可诊断性增强、文档能力扩展。

3. ``PATCH``

   用于 bug 修复、构建修正、测试修正、文档修正，以及不改变公开语义的小型依赖升级。


什么情况应视为破坏性变更
------------------------

下列情况通常应视为破坏性变更，需要谨慎处理，必要时提升主版本：

1. 删除公开 API。
2. 修改已有 API 的核心返回语义。
3. 修改 ``open()`` / ``save()`` / ``save_as()`` 的成功与失败语义。
4. 修改安装路径、导出 target 名称、包配置变量名称。
5. 修改导致下游必须重写调用逻辑或构建脚本的行为。


当前兼容性原则
--------------

``FeatherDoc`` 当前不追求对历史旧接口做长期低价值兼容。

兼容性的优先级是：

1. 当前公开 API 的语义清晰。
2. 当前版本的文档、测试、安装导出保持一致。
3. MSVC 工具链下的行为真实可验证。
4. 不为了兼容过时设计而重新引入已经清理掉的旧模式。


发布前最低检查项
----------------

每次正式发布前，至少应确认：

1. 对外变化已经整理到根目录 ``CHANGELOG.md``。
2. 当前版本号已经同步到 ``CMakeLists.txt``。
3. README 与文档首页没有明显过时信息。
4. 许可、NOTICE、LEGAL、赞助入口仍与当前仓库状态一致。
5. MSVC 构建、测试、样例运行通过。
6. ``cmake --install`` 之后，外部最小工程可以 ``find_package(FeatherDoc CONFIG REQUIRED)`` 并成功链接运行。
7. 本地 Word visual release gate 已执行，至少覆盖常规 smoke 与
   fixed-grid merge/unmerge quartet，可在真实 Word 渲染下排查明显回归。
8. 公开 API 变更已经反映到样例、测试和文档中。
9. 完成以上检查后再打 tag / 创建 release。

当前推荐的最低验证命令：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1

如果只是复用已有 ``build-msvc-nmake`` 构建目录，可改为：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_release_candidate_checks.ps1 `
        -SkipConfigure `
        -SkipBuild

前提是该构建目录已经包含最新的 ``featherdoc_cli``、visual smoke sample，
以及 fixed-grid quartet sample；否则应去掉 ``-SkipBuild`` 让脚本先补齐产物。

.. code-block:: bat

    cmake -S . -B build-msvc-nmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON -DBUILD_CLI=ON
    cmake --build build-msvc-nmake
    ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_install_find_package_smoke.ps1 `
        -BuildDir build-msvc-nmake `
        -InstallDir build-msvc-install `
        -ConsumerBuildDir build-msvc-install-consumer `
        -Generator "NMake Makefiles" `
        -Config Release

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_release_gate.ps1 `
        -SmokeBuildDir .\build-msvc-nmake `
        -FixedGridBuildDir .\build-msvc-nmake

说明：

1. ``run_release_candidate_checks.ps1`` 会把 ``MSVC configure/build``、
   ``ctest``、``install + find_package smoke``、以及
   ``run_word_visual_release_gate.ps1`` 串成一条本地发布前总检查。
2. GitHub 云端 CI 仍适合做构建、单测和样例 ``.docx`` 结构验证，但不应代替
   依赖本机 ``Microsoft Word`` 的最终视觉 gate。
3. ``run_word_visual_release_gate.ps1`` 会继续拆成 document task 和
   fixed-grid bundle task，便于后续把截图级人工/AI 复核接到发布流程里。


依赖升级策略
------------

依赖升级应遵循下面几条原则：

1. 优先升级真正影响安全性、维护性、编译兼容性或性能的问题。
2. 升级后必须重新验证 MSVC 构建与现有测试。
3. 不为了“版本看起来新”而做无收益的大规模依赖折腾。
4. 如果依赖升级改变了行为边界，应在发布说明中明确指出。


发布说明建议包含的内容
----------------------

每次发布说明至少应包含：

1. 版本号。
2. 本次核心变化摘要。
3. 是否有破坏性 API 变化。
4. 是否升级了 vendored 依赖。
5. 是否调整了许可、安装导出或项目元数据。
6. 本次验证方式，尤其是 MSVC 验证结果。
7. 安装产物里的复现入口，例如 ``share/FeatherDoc/VISUAL_VALIDATION_QUICKSTART.zh-CN.md``。
8. 可视化证据与 release-preflight 证据文件路径。

仓库根目录还提供了 ``RELEASE_ARTIFACT_TEMPLATE.md`` 与
``RELEASE_ARTIFACT_TEMPLATE.zh-CN.md``，用于把安装包入口、验证结果和截图复现
命令拼成一份对外发布说明骨架。

当 ``run_release_candidate_checks.ps1`` 跑完后，输出根目录会自动写出
``START_HERE.md``，``report`` 目录还会写出 ``ARTIFACT_GUIDE.md``、
``REVIEWER_CHECKLIST.md``、``release_handoff.md``、``release_body.zh-CN.md``
和 ``release_summary.zh-CN.md``。
如果你后来又把 visual verdict 从 ``pending_manual_review`` 回写成
``pass``，优先执行最短的一键同步命令，把最新 document task /
fixed-grid task 的结论回灌进 gate summary、release-preflight summary，
并顺手重刷整套 report：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\sync_latest_visual_review_verdict.ps1

如果你需要手动覆盖推断出来的 gate / release 路径，再改用显式命令：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\sync_visual_review_verdict.ps1 `
        -GateSummaryJson .\output\word-visual-release-gate\report\gate_summary.json `
        -ReleaseCandidateSummaryJson .\output\release-candidate-checks\report\summary.json `
        -RefreshReleaseBundle

其中 ``START_HERE.md`` 是本地 summary 输出的首个入口，
``ARTIFACT_GUIDE.md`` 负责先导索引，``REVIEWER_CHECKLIST.md`` 负责评审
步骤，``release_body.zh-CN.md`` 用于给 GitHub Release 或手工发布说明
提供一份中文草稿，``release_summary.zh-CN.md`` 则适合作为 GitHub Release
首屏短摘要。后两者都会优先从 ``CHANGELOG.md`` 的 ``Unreleased`` 区块自动
抽取“核心变化”要点。
如果只想单独复跑 fixed-grid merge/unmerge 四件套，并生成可截图签收的
review task，可另外执行：

.. code-block:: powershell

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_fixed_grid_merge_unmerge_regression.ps1 `
        -PrepareReviewTask `
        -ReviewMode review-only

GitHub Actions 的 ``windows-msvc.yml`` 现在也会上传一个
``windows-msvc-release-metadata`` artifact，其中包含安装树下的
``share/FeatherDoc`` 文档入口、artifact 根级的
``RELEASE_METADATA_START_HERE.md``，以及
``output/release-candidate-checks-ci/START_HERE.md`` 和
``output/release-candidate-checks-ci/report`` 里的 ``ARTIFACT_GUIDE.md``、
``REVIEWER_CHECKLIST.md``、``summary.json``、``final_review.md``、
``release_handoff.md``、``release_body.zh-CN.md``、
``release_summary.zh-CN.md``。拿到 artifact 后建议先看
``RELEASE_METADATA_START_HERE.md``，再进入 ``START_HERE.md``、
``ARTIFACT_GUIDE.md``，最后按 ``REVIEWER_CHECKLIST.md`` 走评审流。
不过 CI 没有真实 Word 渲染环境，因此这里只会生成 visual gate 为
``skipped`` 的 handoff；正式发版前，仍然要用本地 Windows preflight
补齐最终截图级结论。


历史 Release 回补
-----------------

如果你处理的不是“即将发布的新版本”，而是已经公开过的历史 GitHub Release
回补、正文修订或附件重传，不要直接复用这里的正式发版叙述去套旧版本。

历史回补应额外遵循下面几条约束：

1. 公开正文和附件里不得出现 ``draft``、``草稿`` 或本机绝对路径。
2. 旧版本只能引用当时真实存在的安装入口、视觉证据和脚本产物。
3. ``v1.0.0`` 到 ``v1.1.0`` 的视觉证据只能按样本文档重建，不得冒充后期
   release gate。
4. 上传前必须先做正文扫描、附件脱敏和远端状态复核。

具体分层策略、脚本用途和执行顺序，见
:doc:`release_history_backfill_playbook_zh`。


CHANGELOG 维护建议
-----------------

建议把对外可见的版本变化同步记录到根目录 ``CHANGELOG.md``。

至少应保证：

1. 新增功能有记录。
2. 破坏性变更有显式标注。
3. 依赖升级有记录。
4. 重要的构建、安装导出、许可或项目元数据变化有记录。


发布执行顺序建议
----------------

为了避免“小改动频繁立刻发版”，建议把正式发布动作固定为下面顺序：

1. 先整理 ``CHANGELOG.md``，确认这次是否真的值得发版。
2. 再核对 ``CMakeLists.txt`` 里的版本号与发布说明是否一致。
3. 完成 MSVC 构建、测试、样例、安装后外部消费 smoke。
4. 跑一次本地 release candidate checks，至少覆盖 Word visual release gate。
5. 最后再打 tag，并基于 ``CHANGELOG.md`` 生成 release 说明。


建议的发布定位
--------------

对外发布时，建议把 ``FeatherDoc`` 介绍成：

一个面向现代 C++ 的 ``.docx`` 读写库 fork，重点关注清晰 API、
MSVC 可构建性、错误诊断能力和核心路径性能。
