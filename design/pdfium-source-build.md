# PDFium 源码构建接入

> 本文档记录 FeatherDoc 实验性 PDF 读入模块如何使用 PDFium 源码构建。
>
> 阅读前置：[Word ⇄ PDF 转换器路线](word-pdf-converter-roadmap.md)、
> [Word ⇄ PDF 转换器架构](word-pdf-converter-architecture.md)。
>
> 如果只想照着编译，请看仓库根目录的 [PDF 构建手册](../BUILDING_PDF.md)。

## 定位

PDFium 是 PDF → Word 方向的读入层。

它只负责读取 PDF、提取页面、文本、坐标和字体等底层信息。FeatherDoc 自己负责把这些低层信息重建为
AST / DOM。

PDFium 功能默认关闭，必须显式开启：

```bash
-DFEATHERDOC_BUILD_PDF_IMPORT=ON
```

默认构建不下载、不构建、不链接 PDFium。

## 为什么默认走源码

PDFium 官方工程使用 Chromium 工具链：

- `depot_tools`
- `gclient`
- `gn`
- `ninja`

它不是普通 CMake 子项目。FeatherDoc 的 CMake 只负责调用已有 PDFium checkout 的 `gn gen` 和
`ninja`，不把 PDFium 源码 vendoring 到本仓库。

这样可以避免：

- 把大型第三方源码提交进 FeatherDoc
- 让默认构建变慢
- 让 PDFium 的工具链复杂度污染主库
- 把 PDFium 维护责任转移到 FeatherDoc

## 获取 PDFium 源码

先安装 Chromium `depot_tools`。推荐把它放在仓库忽略目录或外部工具目录，
不要提交进 FeatherDoc。

本仓库本机验证时使用的是：

```powershell
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git tmp/depot_tools
$env:PATH = "$PWD\tmp\depot_tools;$env:PATH"
```

这里只改当前 PowerShell 会话的 `PATH`，不改系统环境变量。

然后在你放第三方源码的目录执行：

```powershell
mkdir pdfium-workspace
cd pdfium-workspace
gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git
gclient sync --no-history --jobs 8
```

同步完成后，目录结构大致是：

```text
pdfium-workspace/
├── .gclient
└── pdfium/
    ├── BUILD.gn
    ├── public/
    │   ├── fpdfview.h
    │   └── fpdf_text.h
    └── ...
```

传给 FeatherDoc 的路径应该是 `pdfium/` 这一层，而不是 `pdfium-workspace/`：

```bash
-DFEATHERDOC_PDFIUM_SOURCE_DIR=/path/to/pdfium-workspace/pdfium
```

## Windows 源码构建注意点

### 使用本机 VS 工具链

如果 `gclient sync` 的 hook 里出现 Google Cloud Storage 401，通常是
PDFium 试图下载 Chromium 内部预编译 Windows 工具链。

外部开发者应改用本机 Visual Studio / Build Tools：

```powershell
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = "0"
gclient sync --no-history --jobs 8
```

如果第三方依赖因为 Windows 长路径被误判为 dirty，先确认它是刚同步出来的
临时 checkout，再重跑：

```powershell
$env:GIT_CONFIG_COUNT = "2"
$env:GIT_CONFIG_KEY_0 = "core.longpaths"
$env:GIT_CONFIG_VALUE_0 = "true"
$env:GIT_CONFIG_KEY_1 = "core.autocrlf"
$env:GIT_CONFIG_VALUE_1 = "false"
gclient sync --no-history --jobs 8 --force --reset
```

### `gn` / `ninja` 的位置

完整同步后，PDFium checkout 自带工具：

```text
<pdfium>/buildtools/win/gn.exe
<pdfium>/third_party/ninja/ninja.exe
```

FeatherDoc 的 CMake helper 会优先从这两个位置查找。也可以显式传：

```bash
-DFEATHERDOC_GN_EXECUTABLE=/path/to/pdfium/buildtools/win/gn.exe
-DFEATHERDOC_NINJA_EXECUTABLE=/path/to/pdfium/third_party/ninja/ninja.exe
```

### Visual Studio 安装路径提示

如果 PDFium 的 `vs_toolchain.py` 没有自动找到 Visual Studio，可以传：

```bash
-DFEATHERDOC_PDFIUM_VS_YEAR=2026
-DFEATHERDOC_PDFIUM_VS_INSTALL="D:/Program Files/Microsoft Visual Studio/18/Professional"
```

VS 2022 对应：

```bash
-DFEATHERDOC_PDFIUM_VS_YEAR=2022
-DFEATHERDOC_PDFIUM_VS_INSTALL="C:/Program Files/Microsoft Visual Studio/2022/Community"
```

### MSVC 运行库必须匹配

当前默认 GN 参数构建出来的 PDFium 是静态运行库 `/MT`。

FeatherDoc 链接 source-built `pdfium.lib` 时，MSVC 构建也应使用：

```bash
-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
```

Windows 下 `FeatherDoc::PdfImport` 会自动补充 `winmm.lib`，用于满足
PDFium 内部 `timeGetTime` 依赖。

## FeatherDoc 构建命令

启用 PDFium 源码 provider：

```bash
cmake -S . -B build-pdf-import \
  -DFEATHERDOC_BUILD_PDF_IMPORT=ON \
  -DFEATHERDOC_PDFIUM_SOURCE_DIR=/path/to/pdfium-workspace/pdfium \
  -DBUILD_SAMPLES=ON
```

构建 probe：

```bash
cmake --build build-pdf-import --target featherdoc_pdfium_probe
```

运行 probe：

```bash
build-pdf-import/featherdoc_pdfium_probe input.pdf
```

Windows 下可执行文件通常在：

```text
build-pdf-import/featherdoc_pdfium_probe.exe
```

## Windows + Ninja 示例

先进入 VS x64 开发环境：

```bat
VsDevCmd.bat -arch=x64 -host_arch=x64
```

然后配置并构建：

```powershell
cmake -S . -B build-pdf-import-msvc -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
  -DBUILD_TESTING=OFF `
  -DBUILD_CLI=OFF `
  -DBUILD_SAMPLES=ON `
  -DFEATHERDOC_BUILD_PDF_IMPORT=ON `
  -DFEATHERDOC_PDFIUM_SOURCE_DIR="$PWD/tmp/pdfium-workspace/pdfium" `
  -DFEATHERDOC_DEPOT_TOOLS_DIR="$PWD/tmp/depot_tools" `
  -DFEATHERDOC_PDFIUM_VS_YEAR=2026 `
  -DFEATHERDOC_PDFIUM_VS_INSTALL="D:/Program Files/Microsoft Visual Studio/18/Professional"

cmake --build build-pdf-import-msvc --target featherdoc_pdfium_probe
```

验证读入链路：

```powershell
build-pdf-import-msvc/featherdoc_pdfium_probe.exe input.pdf
```

如果同时开启 PDFio 写出和 PDFium 读入，并启用 `BUILD_TESTING=ON`，可以直接跑
端到端 smoke：

```powershell
ctest --test-dir build-pdf-import-msvc -R "pdf(io_generator|ium_parser)_probe" --output-on-failure --timeout 60
```

本机已验证用 PDFio probe 生成的 PDF 作为输入，可得到：

```text
parsed .bpdf-interfaces\featherdoc-pdfio-probe.pdf (1 pages, 87 text spans)
```

## 关键 CMake 开关

### `FEATHERDOC_BUILD_PDF_IMPORT`

默认值：`OFF`

开启实验性 PDF 读入模块。

### `FEATHERDOC_PDFIUM_PROVIDER`

默认值：`source`

可选值：

- `source`：使用 PDFium 源码 checkout，通过 GN/Ninja 构建
- `package`：使用外部 CMake PDFium 包

当前推荐 `source`。

### `FEATHERDOC_PDFIUM_SOURCE_DIR`

默认值：空

当 provider 为 `source` 时必须设置，指向包含 `public/fpdfview.h` 的 PDFium 源码目录。

### `FEATHERDOC_PDFIUM_OUT_DIR`

默认值：`<PDFium source>/out/FeatherDoc`

指定 PDFium 的 GN 输出目录。

### `FEATHERDOC_PDFIUM_GN_ARGS`

默认值：

```text
is_debug=false
is_component_build=false
pdf_is_complete_lib=true
pdf_is_standalone=true
pdf_enable_v8=false
pdf_enable_xfa=false
pdf_use_skia=false
pdf_enable_fontations=false
clang_use_chrome_plugins=false
use_custom_libcxx=false
```

这组参数的目标是先得到尽量小、尽量稳定的 PDFium 静态库。

如果后续要支持 XFA、V8 或 Skia 相关能力，再单独开关验证。

### `FEATHERDOC_PDFIUM_NINJA_TARGET`

默认值：`pdfium`

传给 Ninja 的目标名。

### `FEATHERDOC_PDFIUM_EXTRA_LIBS`

默认值：空

当平台需要额外系统库时，可通过这个变量补充链接库。

Windows source provider 会自动链接 `winmm.lib`。

### `FEATHERDOC_DEPOT_TOOLS_DIR`

默认值：空

指定 Chromium `depot_tools` 目录。源码 provider 构建 PDFium 时，会把它加入
runner 脚本的当前进程 `PATH`。

### `FEATHERDOC_PDFIUM_VS_YEAR`

默认值：空

Windows 下用于提示 PDFium 使用哪一代 Visual Studio，例如 `2026` 或 `2022`。

### `FEATHERDOC_PDFIUM_VS_INSTALL`

默认值：空

Windows 下用于提示 PDFium 的 Visual Studio 安装目录，会传给
`vs<year>_install` 环境变量。

## 预编译包备用路线

如果你已经有 CMake 化的 PDFium 预编译包，可以改走 package provider：

```bash
cmake -S . -B build-pdf-import \
  -DFEATHERDOC_BUILD_PDF_IMPORT=ON \
  -DFEATHERDOC_PDFIUM_PROVIDER=package \
  -DPDFium_DIR=/path/to/pdfium/cmake
```

这条路线不是默认路线，只作为已有包的兼容入口。

## 常见问题

### 找不到 `gclient`

说明 `depot_tools` 没有加入 `PATH`。

检查：

```bash
gclient --version
gn --version
ninja --version
```

### `FEATHERDOC_PDFIUM_SOURCE_DIR` 指错

FeatherDoc 会检查：

```text
<FEATHERDOC_PDFIUM_SOURCE_DIR>/public/fpdfview.h
```

如果这个文件不存在，说明路径层级不对。

### 构建时间很长

正常。PDFium 是大型 C++ 工程。首次 `gclient sync` 和首次 Ninja 构建都可能花较长时间。

### 不应提交 PDFium 源码

PDFium 源码 checkout 应放在工作区外部或 `tmp/` 等被忽略目录内，不应提交进 FeatherDoc 仓库。

## 当前状态

- [x] FeatherDoc 已有 `FEATHERDOC_BUILD_PDF_IMPORT`
- [x] 已有 `PdfiumParser` 实现 `IPdfParser`
- [x] 已有 `featherdoc_pdfium_probe`
- [x] 已有源码 provider 的 CMake 集成
- [x] 已用本机 PDFium checkout 验证 `featherdoc_pdfium_probe` 完整编译
- [x] 已用 PDFio 样例 PDF 验证 PDFium 读入链路
- [x] 已新增 `pdfium_parser_probe`，覆盖 PDFio 生成后再交给 PDFium 解析
- [ ] 需要建立 PDF 样本回归集
