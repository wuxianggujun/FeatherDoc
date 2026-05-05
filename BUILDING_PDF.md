# FeatherDoc PDF 构建手册

> PDF 能力仍是实验性方向。默认构建不会启用 PDFio / PDFium，也不会拉取、
> 编译或安装实验性 PDF 头文件。

## 什么时候需要看这份文档

只有推进 Word ⇄ PDF 转换器时才需要看这里：

- `FEATHERDOC_BUILD_PDF=ON`：启用 PDFio 写出方向
- `FEATHERDOC_BUILD_PDF_IMPORT=ON`：启用 PDFium 读入方向

普通库构建不需要这些步骤。

## 不要提交的生成物

以下都是本地生成或第三方 checkout，不应提交：

- `tmp/depot_tools/`
- `tmp/pdfium-workspace/`
- `tmp/pdfio-src/`
- `.bpdf-*/`
- `.codex-build-*/`
- `build-pdf*/`
- `out/`
- probe 生成的 `*.pdf`
- 编译产物：`*.exe`、`*.lib`、`*.obj`、`*.pdb`

需要提交的是 FeatherDoc 自己的源码、CMake helper、样例、测试和文档。

## 默认构建确认

默认配置必须保持 PDF 关闭：

```powershell
cmake -S . -B .codex-build-default `
  -G "Unix Makefiles" `
  -DCMAKE_C_COMPILER=clang `
  -DCMAKE_CXX_COMPILER=clang++ `
  -DBUILD_TESTING=OFF `
  -DBUILD_CLI=OFF
```

确认：

```powershell
Select-String -Path .codex-build-default/CMakeCache.txt -Pattern "FEATHERDOC_BUILD_PDF"
cmake --build .codex-build-default --target help | Select-String -Pattern "pdf|Pdf"
```

预期：

```text
FEATHERDOC_BUILD_PDF:BOOL=OFF
FEATHERDOC_BUILD_PDF_IMPORT:BOOL=OFF
```

并且 target help 中没有 PDF 目标。

## 准备 PDFio

PDFio 用于 Word → PDF 写出方向。

如果使用本地源码 checkout：

```powershell
git clone https://github.com/michaelrsweet/pdfio.git tmp/pdfio-src
```

构建 PDFio probe：

```powershell
cmake -S . -B .bpdf-interfaces `
  -G "Unix Makefiles" `
  -DCMAKE_C_COMPILER=clang `
  -DCMAKE_CXX_COMPILER=clang++ `
  -DBUILD_TESTING=ON `
  -DBUILD_CLI=OFF `
  -DBUILD_SAMPLES=ON `
  -DFEATHERDOC_BUILD_PDF=ON `
  -DFEATHERDOC_FETCH_PDFIO=OFF `
  -DFEATHERDOC_PDFIO_SOURCE_DIR="$PWD/tmp/pdfio-src"

cmake --build .bpdf-interfaces --target featherdoc_pdfio_probe
ctest --test-dir .bpdf-interfaces -R pdfio_generator_probe --output-on-failure --timeout 60
```

输出样例：

```text
.bpdf-interfaces/featherdoc-pdfio-probe.pdf
```

## 准备 PDFium

PDFium 用于 PDF → Word 读入方向。

先准备 Chromium `depot_tools`。推荐放在 `tmp/`，不要提交：

```powershell
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git tmp/depot_tools
$env:PATH = "$PWD/tmp/depot_tools;$env:PATH"
```

同步 PDFium 源码：

```powershell
New-Item -ItemType Directory -Force tmp/pdfium-workspace | Out-Null
Push-Location tmp/pdfium-workspace
gclient config --unmanaged https://pdfium.googlesource.com/pdfium.git
gclient sync --no-history --jobs 8
Pop-Location
```

Windows 上如果 hook 尝试下载 Google 内部工具链并失败，改用本机 VS：

```powershell
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = "0"
Push-Location tmp/pdfium-workspace
gclient sync --no-history --jobs 8
Pop-Location
```

如果第三方依赖因为长路径被误判为 dirty，只在这个临时 workspace 内重跑：

```powershell
$env:GIT_CONFIG_COUNT = "2"
$env:GIT_CONFIG_KEY_0 = "core.longpaths"
$env:GIT_CONFIG_VALUE_0 = "true"
$env:GIT_CONFIG_KEY_1 = "core.autocrlf"
$env:GIT_CONFIG_VALUE_1 = "false"
Push-Location tmp/pdfium-workspace
gclient sync --no-history --jobs 8 --force --reset
Pop-Location
```

## Windows 构建 PDFium import

进入 VS x64 开发环境：

```bat
VsDevCmd.bat -arch=x64 -host_arch=x64
```

如果 VS 安装在非默认路径，配置时传 `FEATHERDOC_PDFIUM_VS_YEAR` 和
`FEATHERDOC_PDFIUM_VS_INSTALL`。

示例：

```powershell
cmake -S . -B .bpdf-pdfium-source-msvc `
  -G Ninja `
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

cmake --build .bpdf-pdfium-source-msvc --target featherdoc_pdfium_probe
```

关键点：

- source-built `pdfium.lib` 当前使用 `/MT`
- FeatherDoc 同一构建也要传 `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`
- Windows 下会自动补 `winmm.lib`
- CMake 会优先使用 PDFium checkout 内的 `buildtools/win/gn.exe`
- CMake 会优先使用 PDFium checkout 内的 `third_party/ninja/ninja.exe`

## 端到端 smoke

同时开启 PDFio 写出和 PDFium 读入：

```powershell
cmake -S . -B .bpdf-roundtrip-msvc `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded `
  -DBUILD_TESTING=ON `
  -DBUILD_CLI=OFF `
  -DBUILD_SAMPLES=ON `
  -DFEATHERDOC_BUILD_PDF=ON `
  -DFEATHERDOC_FETCH_PDFIO=OFF `
  -DFEATHERDOC_PDFIO_SOURCE_DIR="$PWD/tmp/pdfio-src" `
  -DFEATHERDOC_BUILD_PDF_IMPORT=ON `
  -DFEATHERDOC_PDFIUM_SOURCE_DIR="$PWD/tmp/pdfium-workspace/pdfium" `
  -DFEATHERDOC_DEPOT_TOOLS_DIR="$PWD/tmp/depot_tools" `
  -DFEATHERDOC_PDFIUM_VS_YEAR=2026 `
  -DFEATHERDOC_PDFIUM_VS_INSTALL="D:/Program Files/Microsoft Visual Studio/18/Professional"

cmake --build .bpdf-roundtrip-msvc --target featherdoc_pdfio_probe featherdoc_pdfium_probe
ctest --test-dir .bpdf-roundtrip-msvc -R "pdf(io_generator|ium_parser)_probe" --output-on-failure --timeout 60
```

当前已验证结果：

```text
pdfio_generator_probe ............ Passed
pdfium_parser_probe .............. Passed
```

手动运行：

```powershell
.bpdf-roundtrip-msvc/featherdoc_pdfium_probe.exe .bpdf-roundtrip-msvc/featherdoc-pdfio-probe.pdf
```

预期类似：

```text
parsed .bpdf-roundtrip-msvc\featherdoc-pdfio-probe.pdf (1 pages, 87 text spans)
```

## 当前可用状态

现在只能算 **实验性 smoke 可用**：

- 可以生成最小 PDF 样例
- 可以用 PDFium 解析页数和文字 span
- 可以跑 PDFio → PDFium 的端到端 smoke

还不能算正式可用：

- 还没有 `AST → PDFio` 完整翻译层
- 还没有 `PDFium → AST` 文档结构重建
- 还没有真实 PDF 样本回归集
- 中文字体、表格、图片、分页等都还需要专项推进
