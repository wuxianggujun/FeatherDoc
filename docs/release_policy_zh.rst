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
7. 本地 Word visual smoke 已执行，确认当前发布候选在真实 Word 渲染下没有明显回归。
8. 公开 API 变更已经反映到样例、测试和文档中。
9. 完成以上检查后再打 tag / 创建 release。

当前推荐的最低验证命令：

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

    pwsh -ExecutionPolicy Bypass -File .\scripts\run_word_visual_smoke.ps1 `
        -BuildDir .\build-msvc-nmake


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
4. 跑一次本地 Word visual smoke。
5. 最后再打 tag，并基于 ``CHANGELOG.md`` 生成 release 说明。


建议的发布定位
--------------

对外发布时，建议把 ``FeatherDoc`` 介绍成：

一个面向现代 C++ 的 ``.docx`` 读写库 fork，重点关注清晰 API、
MSVC 可构建性、错误诊断能力和核心路径性能。
