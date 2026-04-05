上游 DuckX Issue 对照
======================

本文记录 2026-04-05 对上游 `DuckX <https://github.com/amiremohamadi/DuckX>`_
公开 issue 的一次人工筛查结果，用来判断当前 FeatherDoc 已经解决了哪些历史问题，
以及还有哪些能力缺口仍然存在。

说明
----

1. 本文只整理与 FeatherDoc 当前方向直接相关的 issue。
2. “已修复”表示当前源码和回归测试已经直接覆盖对应问题。
3. “推断已修复”表示 issue 描述本身较简略，但结合当前实现和测试，基本可以判断
   FeatherDoc 已经规避原问题。
4. “未解决”表示当前仓库仍未提供对应能力，或者仍明确标记为暂不支持。


已修复或已显著缓解
------------------

``#97 DuckX deletes spaces``
    上游反馈在调用 ``set_text()`` 后，文档里的空格会在随机位置丢失。
    FeatherDoc 现在会在写入前后空白文本时保留 ``xml:space="preserve"``
    语义，相关实现位于 ``src/xml_helpers.cpp``，并且已有
    ``set_text preserves xml:space when leading or trailing spaces exist``
    回归测试覆盖。

``#96 double frree corruption``
    上游反馈在 ``zip`` / ``pugixml`` 采用 shared library 组合时，会出现
    ``double free or corruption``。
    FeatherDoc 现在在 ``Document::open()`` 中先由自身持有 ZIP 读出的 XML
    缓冲区，再交给 ``pugixml`` 解析，不再把 ZIP 分配的缓冲区所有权直接转移
    给 ``pugixml``。这条路径已有分配器边界回归测试覆盖。

``#80 When a word document is opened, reading the file will fail``
    上游反馈当 Word 正在打开文档时，读取会失败。
    FeatherDoc 当前在 Windows 构建中启用了 shareable file open 支持，并且已有
    ``open succeeds while another process keeps the docx writable but shareable``
    回归测试。这个问题在当前分支可以视为已修复。

``#93 Open failed!``
    上游反馈只能打开固定位置，例如 build 目录里的文件。
    FeatherDoc 已改用 ``std::filesystem::path`` 处理路径，并且已有
    ``open and save work with an absolute path outside the build directory``
    回归测试。基于当前实现，可以**推断**这个问题已经被修复。

``#91 Can not add content to docx files(crashed)``
    上游给出的示例是 ``insert_paragraph_after()`` 之后 ``save()`` 触发崩溃，
    末尾表现也是 ``double free or corruption``。
    FeatherDoc 当前同时修掉了段落插入顺序问题和 XML 缓冲区所有权问题，并补了
    ``insert_paragraph_after`` 保存回归测试。基于当前源码和测试，可以**推断**
    这个崩溃路径已经被修复。

``#90 Use Bookmark API replace text in BookmarkStart node and BookmarkEnd node``
    FeatherDoc 现在提供了 ``replace_bookmark_text(name, replacement)``，
    可以按书签名重写 ``bookmarkStart`` 和 ``bookmarkEnd`` 之间的正文内容。
    当前实现优先覆盖正文书签替换这个高频模板场景。

``#86 Working with headers and footes``
    FeatherDoc 现在已经补上了页眉 / 页脚的最小可用编辑链路：
    ``header_count()`` / ``footer_count()`` 用来枚举 part 数量，
    ``header_paragraphs(index)`` / ``footer_paragraphs(index)`` 用来读取和编辑
    现有 part 里的段落内容，并随 ``save()`` 写回归档；
    ``ensure_header_paragraphs()`` / ``ensure_footer_paragraphs()`` 则可以为
    body 级 ``sectPr`` 创建并挂接默认页眉 / 页脚 part。
    此外，``section_count()``、``section_header_paragraphs()`` 和
    ``section_footer_paragraphs()`` 已经可以按 section 解析现有引用关系；
    ``ensure_section_header_paragraphs()`` /
    ``ensure_section_footer_paragraphs()`` 也已经可以为指定 section 补出
    ``default`` / ``first`` / ``even`` 引用并创建对应 part。
    这还不是完整的 section-aware 重绑定 API，但已经覆盖了“编辑已有页眉 / 页脚”、
    “从空模板补出默认页眉 / 页脚”和“按 section 创建或访问引用”的核心场景。


当前仍未解决或仍属能力缺口
--------------------------

``#82 how can open word with password?``
    当前仍不支持带密码或加密的 ``.docx``。不过 FeatherDoc 已经把这个场景改为
    返回明确的 ``encrypted_document_unsupported`` 错误，而不是模糊的 XML
    打开失败。

``#83 Can I create a table?``
    当前仍没有高层级的“创建新表格”API。现有 API 只能遍历已存在的表格内容。

``#85 Read equations from docx``
    当前仍没有把 Word 公式（OMML）封装成专门的读取接口。


对 FeatherDoc 的实际意义
------------------------

从上游 issue 角度看，FeatherDoc 当前已经优先解决了更偏工程稳定性的历史问题：

- shared library 场景下的分配器边界崩溃
- Word 占用源文件时的打开失败
- 绝对路径 / 非 build 目录路径打开失败
- 文本写入时空格丢失
- ``insert_paragraph_after()`` 保存回归

剩下的主要不是“明显 bug”，而是更偏功能扩展的能力缺口：

- 多 section 的页眉 / 页脚重绑定、复用与引用管理
- 表格创建 API
- 公式读取 API
- 加密 ``.docx`` 支持


建议优先级
----------

如果下一阶段要继续推进，建议按下面顺序考虑：

1. 先评估页眉 / 页脚创建与引用管理，因为这决定很多正式文档模板场景是否可落地。
2. 表格创建可以放在其后，因为它属于高频能力，但实现边界更宽。
3. 公式读取建议放在页眉 / 页脚和表格创建之后，再决定是否做 typed API。
4. 加密 ``.docx`` 支持建议单独立项，它不只是 API 设计问题，还牵涉加密容器和
   解密流程能力。
