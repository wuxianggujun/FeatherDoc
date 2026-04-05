项目审计与现代化建议
====================

这份文档基于当前仓库初始导入时的 DuckX 源码快照、关键命令检索与 ``cloc`` 统计。
说明：文中出现的 ``DuckX`` / ``duckx`` 仅表示初始导入来源；
当前项目名称与对外命名已经统一为 ``FeatherDoc`` / ``featherdoc``。
如果你想看当前 fork 的正式项目定位，而不是历史问题审计，
请优先阅读 :doc:`project_identity_zh`。
目标是回答三个问题：

1. 初始导入版本有哪些明显的过时兼容写法或历史包袱。
2. 如果准备迁移到较新的 C++ 标准，应优先处理哪些点。
3. 当前仓库代码量大概是多少，应该如何拆分口径来看。


项目现状概览
------------

- 这是一个体量很小的 ``.docx`` 读写库，核心源码主要集中在 ``include/`` 与 ``src/``。
- 初始导入版本的核心功能围绕 ``duckx::Document``、``Paragraph``、``Run``、``Table`` 这几层轻量封装展开，
  当前项目中对应名称已经迁移为 ``featherdoc::Document`` 等接口。
- 仓库内直接 vendored 了 ``pugixml``、``zip/miniz`` 和 ``doctest``，因此总代码量会被第三方依赖大幅放大。
- 现有文档、README、CI 和构建脚本都明显带有老项目的时代特征，适合作为一次“现代化整理”的起点。


cloc 代码量统计
---------------

本次统计使用 ``cloc``，建议按三个口径理解：

1. **整仓口径**

   命令：``cloc .``

   结果：``95`` 个文件，``30105`` 行 code。

   说明：这个数字包含 ``thirdparty/``、``cmake-build-debug/``、文档主题和其他非核心内容，
   只适合看“仓库总体体积”，不适合看你真正要维护的业务代码规模。

2. **排除构建目录和第三方后的口径**

   命令：``cloc --exclude-dir=.git,.idea,cmake-build-debug,thirdparty .``

   结果：``22`` 个文件，``5439`` 行 code。

   说明：这个数字已经能更接近项目本体，但仍然包含文档主题、README 和 ``test/doctest.h``。

3. **核心自有代码口径**

   命令：``cloc include src``

   结果：``4`` 个文件，``421`` 行 code。

   说明：这基本就是你真正要重构和现代化的库主体。

补充两个辅助口径：

- ``cloc include src CMakeLists.txt samples test/basic_tests.cpp test/iterator_tests.cpp test/CMakeLists.txt``
  得到 ``608`` 行 code，代表“核心库 + 自有样例 + 自有测试 + 构建脚本”。
- ``cloc thirdparty test/doctest.h``
  得到 ``20627`` 行 code，说明当前仓库的大头其实是 vendored 依赖，不是你自己的业务实现。


主要过时点
----------

下面这些问题，优先级按“越靠前越应该先处理”排列。

一、构建系统和工程兼容层偏旧
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``CMakeLists.txt:1`` 仍然使用 ``cmake_minimum_required(VERSION 3.0)``。
  我在本机执行 ``cmake -S . -B build-codex -DBUILD_TESTING=ON`` 时，CMake 已经明确给出
  **兼容 CMake < 3.10 即将被移除** 的弃用警告。
- ``CMakeLists.txt:16`` 和 ``test/CMakeLists.txt:1`` 直接把标准锁死在 ``C++11``。
  这会让后续引入 ``std::string_view``、``std::filesystem``、``std::span``、
  ``std::optional``、ranges 等现代能力时非常别扭。
- ``CMakeLists.txt:28-30`` 仍在使用全局 ``include_directories(...)``。
  这是早期 CMake 项目里很常见的写法，但现代 CMake 更推荐用
  ``target_include_directories``、``target_link_libraries`` 和清晰的
  ``PUBLIC/PRIVATE/INTERFACE`` 边界。
- ``CMakeLists.txt:40-43`` 的公开头文件目录配置并不干净。
  当前 ``BUILD_INTERFACE`` 指向 ``src``，而真正的公开头文件在 ``include``；
  安装后的 ``INSTALL_INTERFACE`` 是 ``include``，但头文件实际被安装到 ``include/duckx``。
  再加上 ``duckx.hpp`` 内部使用 ``#include <constants.hpp>`` 这类平铺包含方式，
  导致安装导出目标的可消费性存在风险。
- 仓库还保留了 ``.travis.yml`` 和 README 中的 Travis 徽章。
  这通常意味着 CI 体系长期没有做过现代化整理。

二、API 设计仍停留在早期面向可变状态的风格
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``include/duckx.hpp:148-152`` 的 ``Document`` 设计偏“先构造，再 ``file()``，再 ``open()``”。
  这类生命周期管理方式在老代码里很常见，但不够现代，也不够安全。
- ``include/duckx.hpp:145`` 声明了 ``bool flag_is_open;``，但
  ``src/duckx.cpp:238-249`` 的两个构造函数都没有初始化它。
  这意味着在某些调用路径下，``is_open()`` 可能读取未初始化状态。
- ``src/duckx.cpp:222`` 和 ``src/duckx.cpp:231-235`` 通过裸 ``new`` 生成对象，
  然后把引用直接返回给调用方。
  这是非常典型的旧式接口设计，也会直接带来内存泄漏和所有权不清的问题。
- ``include/duckxiterator.hpp:15-75`` 的自定义迭代器只能满足基本 range-for，
  但没有标准 iterator traits，也没有现代 ranges 语义。
  从 ``include/duckx.hpp:19`` 的 TODO 也能看出，作者自己知道这里还停留在半成品阶段。

三、代码风格和低层实现偏 C 风格
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- ``include/duckx.hpp:11`` 仍使用 ``<stdlib.h>``，更现代的写法通常是 ``<cstdlib>``。
- ``include/constants.hpp:5`` 使用 ``typedef unsigned const int formatting_flag;``，
  并在 ``8-16`` 行定义了一组头文件级别的可写全局变量。
  这在现代 C++ 里更适合改成 ``enum class``、``constexpr`` 或 ``inline constexpr``。
- ``src/duckx.cpp:252`` 使用 ``NULL`` 而不是 ``nullptr``。
- ``src/duckx.cpp:346-347`` 使用 C 风格 ``remove`` / ``rename`` 操作文件，
  这类逻辑未来更适合切到 ``std::filesystem`` 并补充错误处理。
- ``samples/sample1.cpp`` 和 ``samples/sample2.cpp`` 使用 ``using namespace std;``，
  这对示例代码倒不致命，但风格明显偏旧。

四、鲁棒性和错误处理不足
^^^^^^^^^^^^^^^^^^^^^^^^

- ``src/duckx.cpp:255-273`` 在 ``open()`` 中只判断了 ``zip_open`` 是否成功，
  但没有检查 ``zip_entry_open``、``zip_entry_read``、``document.load_buffer`` 的返回值。
- ``src/duckx.cpp:218`` 直接把 ``char`` 传给 ``isspace``。
  如果 ``char`` 是有符号类型且值为负，会触发未定义行为；现代写法应先转成
  ``unsigned char``。
- ``src/duckx.cpp:318-338`` 的 ZIP 复制逻辑没有对每一步失败情况做保护，
  也没有回滚策略。
- ``src/duckx.cpp:346-347`` 先删除原文件再重命名临时文件。
  一旦重命名失败，原文件就已经没了，这在文档处理场景里风险很高。

五、测试与文档依赖也存在年代感
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- 仓库内 ``test/doctest.h`` 的版本是 ``2.3.5``。
  我用 ``clang++ -std=c++20`` 做语法检查时，测试代码因
  ``std::uncaught_exception`` 已被现代标准移除而报错。
- README 的包含方式、构建命令和样例源码不一致：

  - ``README.md:28`` 写的是 ``#include <duckx/duckx.hpp>``
  - ``samples/sample1.cpp:1`` / ``samples/sample2.cpp:1`` 实际写的是 ``#include <duckx.hpp>``

  这会让后来接手的人分不清“安装后头文件组织”到底应该是什么样。
- README 里的 ``g++ sample1.cpp -lduckx`` 过于理想化，
  没有体现头文件目录、安装目标、依赖库或现代 CMake 的推荐接入方式。


第三方依赖现状
--------------

- ``thirdparty/pugixml/pugixml.hpp`` 中 ``PUGIXML_VERSION`` 为 ``190``，
  即 vendored 的是 ``pugixml 1.9``。
- ``test/doctest.h`` 中版本为 ``2.3.5``，已经影响到现代标准下的测试可编译性。
- ``thirdparty/zip/miniz.h`` 内可见 ``MZ_VERSION "9.1.15"``。

仅从仓库内容看，当前依赖管理方式是“直接拷第三方源码进仓库”，
这虽然简单，但会带来几个问题：

- 升级成本高。
- 许可证和版本同步容易被忽略。
- 很难快速判断某个兼容问题来自你自己的代码，还是来自 vendored 依赖。


现代化迁移建议
--------------

建议按下面顺序推进，不要一口气全改。

第一阶段：先做工程层现代化
^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 把最低 CMake 版本提高到至少 ``3.20`` 左右。
2. 目标标准提升到 ``C++20`` 或至少 ``C++17``。
3. 去掉全局 ``include_directories``，改成 target 级别声明。
4. 把公开头文件布局整理干净，统一成一种包含风格，例如 ``<featherdoc.hpp>``。
5. 把 Travis 痕迹迁移到 GitHub Actions。

第二阶段：修正明显的旧式实现
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. 清掉所有裸 ``new`` 返回引用的接口。
2. 初始化 ``Document`` 的全部成员，尤其是 ``flag_is_open``。
3. 把 ``NULL``、``typedef``、C 风格头文件替换为现代写法。
4. 把格式标志重构为类型安全的 ``enum class``。
5. 用 ``std::filesystem`` 重写保存流程，并补齐失败回滚。

第三阶段：再优化 API
^^^^^^^^^^^^^^^^^^^^

1. 明确所有权模型，尽量避免“内部状态机 + 可变引用”的设计。
2. 评估是否保留自定义迭代器，或改造成更标准的 range/view 风格接口。
3. 根据使用场景为文本 API 引入 ``std::string_view``。
4. 明确错误处理策略，是返回 ``bool``、抛异常，还是返回错误码对象。

第四阶段：更新测试与文档
^^^^^^^^^^^^^^^^^^^^^^^^

1. 升级 ``doctest`` 或切换到更现代的测试框架。
2. 增加对打开失败、保存失败、空节点、表格内容的测试。
3. 把 README 的示例、安装方式和真实导出目标统一起来。
4. 增加一份“面向贡献者”的架构说明，而不只是用户 Quick Start。


本次实际验证情况
----------------

这份文档对应的是首次审计阶段，当时没有直接修改业务源码，只做了验证与文档整理。

- ``cmake -S . -B build-codex -DBUILD_TESTING=ON``：
  触发 CMake 旧版本兼容弃用警告；默认生成器落到 ``NMake Makefiles``，
  但当前环境缺少 ``nmake``，所以未完成正式配置。
- 本机存在 ``clang++ 18.1.6``，我用它对项目做了 ``C++20`` 语法检查。
- 在添加 ``-D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH`` 后：

 - ``src/duckx.cpp`` 可通过语法检查。
  - ``samples/sample1.cpp`` 可通过语法检查。
  - ``test/basic_tests.cpp`` 与 ``test/iterator_tests.cpp`` 失败，
    主要原因是 vendored ``doctest 2.3.5`` 仍依赖 ``std::uncaught_exception``。

后续现代化进度
--------------

在这份初始审计完成后，项目已经继续推进了以下改造：

1. 项目名与目标名切换为 ``FeatherDoc`` / ``FeatherDoc::FeatherDoc``。
2. CMake 最低版本提升到 ``3.20``，目标标准提升到 ``C++20``。
3. 公开头文件切换为 ``featherdoc.hpp`` 与 ``featherdoc_iterator.hpp``。
4. ``Document`` 的打开状态初始化、裸 ``new`` 返回引用、保存流程回滚等问题已完成第一轮修复。
5. 当前版本已经用 MSVC 工具链完成配置、构建与测试验证。
6. vendored 依赖已更新到 ``pugixml 1.15``、``kuba--/zip 0.3.8``、
   ``doctest 2.5.1``。
7. ``Document::open()`` 现在会先在 FeatherDoc 侧持有解压后的 XML 缓冲区，
   再交给 ``pugixml`` 解析，避免 Windows shared-library 场景下的分配器边界问题。
8. ``Document::save()`` 已改为流式写入 ``document.xml``，并对原归档中的
   其他 entry 做分块复制，不再把每个 entry 整块读入堆内存后再写回。
9. ``Document::open()`` / ``save()`` 已进一步升级为返回
   ``std::error_code``，其中系统文件错误沿用标准错误码，
   文档语义错误使用 ``featherdoc::document_errc``。
10. ``Document`` 现在还会保存最近一次失败的结构化上下文，
    可通过 ``last_error()`` 读取 ``detail``、``entry_name`` 与
    ``xml_offset``，用于诊断损坏文档或 ZIP entry 级失败。
11. 已新增 ``save_as(path)``，将“原地覆盖保存”和“另存为”显式拆开，
    避免另存为后隐式修改 ``Document::path()`` 的语义歧义。
12. 已新增 ``create_empty()``，支持不依赖现成模板文档、直接从零创建
    最小合法 ``.docx`` 包并保存。
13. 对外文档与许可口径已统一为“fork 新增部分采用非商用源码可见条款，
     上游 DuckX 继承部分保留 MIT 原文说明”，避免继续误称为纯 MIT 开源项目。
14. 已修复 Windows shared build 下自定义 ``std::error_code`` 类别实例不一致、
    sample / test 缺失运行时 DLL、以及 Word 独占源文件时无法打开等问题。
15. 已补齐多组回归测试，覆盖绝对路径、非 docx ZIP、表格文本遍历、
    ``xml:space`` 保留、``insert_paragraph_after()`` 保存回归、
    共享库分配器边界问题、加密 ``.docx`` 显式报错等场景。


结论
----

这个项目**不是“大而旧”**，而是**“很小，但带着明显的老工程壳子”**。

真正需要你重构的核心库主体只有大约 ``421`` 行 code，
所以非常适合直接以现代 C++ 思路做一次干净升级。

最值得优先处理的不是算法，而是下面四件事：

1. 现代化 CMake 与安装导出规则。
2. 清理裸 ``new``、未初始化成员和 C 风格文件操作。
3. 统一头文件布局与公开 API 风格。
4. 升级测试依赖和 README。
