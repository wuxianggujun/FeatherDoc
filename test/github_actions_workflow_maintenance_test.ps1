param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    return Join-Path $Root $RelativePath
}

function Get-RepoFileText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,

        [Parameter(Mandatory = $true)]
        [string]$RelativePath
    )

    $path = Resolve-RepoPath -Root $Root -RelativePath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Required file is missing: $RelativePath"
    }

    return Get-Content -Raw -Encoding UTF8 -LiteralPath $path
}

function Assert-ContainsText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$ExpectedText,

        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    if (-not $Text.Contains($ExpectedText)) {
        throw $Message
    }
}

function Assert-NotContainsText {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,

        [Parameter(Mandatory = $true)]
        [string]$UnexpectedText,

        [Parameter(Mandatory = $true)]
        [string]$Message
    )

    if ($Text.Contains($UnexpectedText)) {
        throw $Message
    }
}

function Assert-ReleaseOutputRootCleanupContract {
    param(
        [Parameter(Mandatory = $true)]
        [string]$WorkflowText,

        [Parameter(Mandatory = $true)]
        [string]$WorkflowName
    )

    foreach ($marker in @(
            '$workspaceRoot = [System.IO.Path]::GetFullPath((Get-Location).Path)',
            '$resolvedReleaseOutputRoot = [System.IO.Path]::GetFullPath($env:RELEASE_OUTPUT_ROOT)',
            'Refusing to clean release output outside the workspace',
            'Test-Path -LiteralPath $resolvedReleaseOutputRoot',
            'Remove-Item -LiteralPath $resolvedReleaseOutputRoot -Recurse -Force',
            'New-Item -ItemType Directory -Path $resolvedReleaseOutputRoot -Force'
        )) {
        Assert-ContainsText -Text $WorkflowText -ExpectedText $marker `
            -Message "$WorkflowName should clean RELEASE_OUTPUT_ROOT inside the workspace before uploading release artifacts. Missing marker '$marker'."
    }

    foreach ($marker in @(
            '$releaseOutputRelativePath = [System.IO.Path]::GetRelativePath($workspaceRoot, $resolvedReleaseOutputRoot)',
            '$parentDirectoryPrefix = ".." + [System.IO.Path]::DirectorySeparatorChar',
            '$parentAltDirectoryPrefix = ".." + [System.IO.Path]::AltDirectorySeparatorChar',
            '$releaseOutputOutsideWorkspace = (',
            '[System.IO.Path]::IsPathRooted($releaseOutputRelativePath)',
            '$releaseOutputRelativePath -eq ".."',
            '$releaseOutputRelativePath.StartsWith($parentDirectoryPrefix, [System.StringComparison]::Ordinal)',
            '$releaseOutputRelativePath.StartsWith($parentAltDirectoryPrefix, [System.StringComparison]::Ordinal)',
            '$releaseOutputRelativePath -eq "." -or $releaseOutputOutsideWorkspace'
        )) {
        Assert-ContainsText -Text $WorkflowText -ExpectedText $marker `
            -Message "$WorkflowName should use relative-path containment checks before recursive cleanup. Missing marker '$marker'."
    }

    Assert-NotContainsText -Text $WorkflowText -UnexpectedText 'StartsWith($workspaceRoot, [System.StringComparison]::OrdinalIgnoreCase)' `
        -Message "$WorkflowName should not rely on string-prefix workspace checks before recursive cleanup."
}

function Assert-ReleaseOutputArtifactPathContract {
    param(
        [Parameter(Mandatory = $true)]
        [string]$WorkflowText,

        [Parameter(Mandatory = $true)]
        [string]$WorkflowName,

        [Parameter(Mandatory = $true)]
        [string]$ArtifactName
    )

    foreach ($marker in @(
            "RELEASE_OUTPUT_ROOT: output/release-assets",
            "name: $ArtifactName",
            '${{ env.RELEASE_OUTPUT_ROOT }}/**',
            "if-no-files-found: warn"
        )) {
        Assert-ContainsText -Text $WorkflowText -ExpectedText $marker `
            -Message "$WorkflowName should upload the generated RELEASE_OUTPUT_ROOT artifact. Missing marker '$marker'."
    }

    foreach ($unexpected in @(
            "output/release-assets/**",
            "output/release-assets-ci"
        )) {
        Assert-NotContainsText -Text $WorkflowText -UnexpectedText $unexpected `
            -Message "$WorkflowName should not upload a hard-coded or CI-preview release artifact path '$unexpected'."
    }
}

$resolvedRepoRoot = (Resolve-Path -LiteralPath $RepoRoot).Path
$workflowRelativePaths = @(
    ".github\workflows\windows-msvc.yml",
    ".github\workflows\linux-cmake.yml",
    ".github\workflows\macos-cmake.yml",
    ".github\workflows\release-refresh.yml",
    ".github\workflows\release-publish.yml"
)

$workflowTexts = @{}
foreach ($relativePath in $workflowRelativePaths) {
    $workflowTexts[$relativePath] = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath $relativePath
}

$allWorkflowText = ($workflowTexts.Values -join "`n")
foreach ($unexpected in @(
        "actions/checkout@v4",
        "actions/upload-artifact@v4",
        "ilammy/msvc-dev-cmd@",
        "runs-on: windows-latest"
    )) {
    Assert-NotContainsText -Text $allWorkflowText -UnexpectedText $unexpected `
        -Message "Workflow maintenance contract should not contain deprecated or unstable marker '$unexpected'."
}

foreach ($relativePath in $workflowRelativePaths) {
    Assert-ContainsText -Text $workflowTexts[$relativePath] -ExpectedText "uses: actions/checkout@v6" `
        -Message "Workflow '$relativePath' should use the Node 24-compatible checkout major."
}

foreach ($relativePath in $workflowRelativePaths) {
    if ($relativePath -eq ".github\workflows\release-publish.yml" -or
        $relativePath -eq ".github\workflows\release-refresh.yml" -or
        $relativePath -eq ".github\workflows\linux-cmake.yml" -or
        $relativePath -eq ".github\workflows\macos-cmake.yml" -or
        $relativePath -eq ".github\workflows\windows-msvc.yml") {
        Assert-ContainsText -Text $workflowTexts[$relativePath] -ExpectedText "uses: actions/upload-artifact@v7" `
            -Message "Workflow '$relativePath' should use the Node 24-compatible upload-artifact major."
    }
}

$windowsWorkflow = $workflowTexts[".github\workflows\windows-msvc.yml"]
foreach ($marker in @(
        "runs-on: windows-2022",
        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "vcvars64.bat",
        "GITHUB_ENV",
        "MSVC x64 environment exported"
    )) {
    Assert-ContainsText -Text $windowsWorkflow -ExpectedText $marker `
        -Message "Windows workflow should keep the inline MSVC setup marker '$marker'."
}

foreach ($marker in @(
        "Verify Ninja",
        'cmake -S . -B build-msvc-ninja -G "Ninja"',
        "-DFEATHERDOC_BUILD_ALLOCATION_FAILURE_TESTS=OFF",
        "-DFEATHERDOC_BUILD_WINDOWS_FAULT_INJECTION_TESTS=OFF",
        "-DFEATHERDOC_ENABLE_SANITIZERS=OFF",
        "-DFEATHERDOC_BUILD_FUZZERS=OFF",
        "cmake --build build-msvc-ninja --parallel 4 --verbose",
        "ctest --test-dir build-msvc-ninja --parallel 2 --output-on-failure --timeout 60",
        '-BuildDir build-msvc-ninja',
        '-Generator "Ninja"',
        "build-msvc-ninja/my_test.docx",
        "build-msvc-ninja/output/**/*.docx"
    )) {
    Assert-ContainsText -Text $windowsWorkflow -ExpectedText $marker `
        -Message "Windows workflow should keep the Ninja parallel build marker '$marker'."
}

$securitySanitizerWorkflow = Get-RepoFileText -Root $resolvedRepoRoot `
    -RelativePath ".github\workflows\security-sanitizers-fuzz.yml"
foreach ($marker in @(
        "-DFEATHERDOC_BUILD_ALLOCATION_FAILURE_TESTS=ON",
        "xml_handle_retirement_tests",
        "text_mutation_transaction_tests",
        "bookmark_batch_transaction_tests",
        "revision_transaction_tests",
        "xml_document_clone_allocation_failure_tests",
        "xml_handle_retirement_allocation_failure_tests",
        "text_mutation_transaction_allocation_failure_tests",
        "bookmark_batch_transaction_allocation_failure_tests",
        "revision_transaction_allocation_failure_tests",
        "review_revisions_allocation_failure_tests",
        "package_relationships_mce_allocation_failure_tests",
        "document_core_allocation_failure_tests",
        "content_controls_allocation_failure_tests",
        "style_management_allocation_failure_tests",
        "section_header_footer_allocation_failure_tests",
        "section_header_footer_part_removal_allocation_failure_tests",
        "section_header_footer_unit_tests",
        "xml_document_clone_allocation_failure|xml_handle_retirement_allocation_failure|text_mutation_transaction_allocation_failure|bookmark_batch_transaction_allocation_failure|revision_transaction_allocation_failure|review_revisions_allocation_failure|package_relationships_mce_allocation_failure|document_core_allocation_failure|content_controls_allocation_failure|style_management_allocation_failure|section_header_footer_allocation_failure|section_header_footer_part_removal_allocation_failure"
    )) {
    Assert-ContainsText -Text $securitySanitizerWorkflow -ExpectedText $marker `
        -Message "Security sanitizer workflow should keep the isolated allocation-failure marker '$marker'."
}

$cppTestSupport = Get-RepoFileText -Root $resolvedRepoRoot `
    -RelativePath "test\cmake\TestSupport.cmake"
Assert-ContainsText -Text $cppTestSupport -ExpectedText "--no-breaks=true" `
    -Message "Windows C++ CTest commands should disable doctest debugger breaks."

$coreCppTests = Get-RepoFileText -Root $resolvedRepoRoot `
    -RelativePath "test\cmake\CoreCppTests.cmake"
Assert-ContainsText -Text $coreCppTests `
    -ExpectedText 'add_test(NAME ${test_name} COMMAND ${target_name} --no-breaks=true)' `
    -Message "Windows pugixml iterative doctest registrations should disable debugger breaks."
Assert-ContainsText -Text $coreCppTests `
    -ExpectedText "FEATHERDOC_BUILD_WINDOWS_FAULT_INJECTION_TESTS" `
    -Message "Windows filesystem fault injection should remain behind an explicit opt-in build switch."

$cliExecutableCppTests = Get-RepoFileText -Root $resolvedRepoRoot `
    -RelativePath "test\cmake\CliExecutableCppTests.cmake"
Assert-ContainsText -Text $cliExecutableCppTests -ExpectedText "--no-breaks=true" `
    -Message "Windows CLI executable doctest registrations should disable debugger breaks."

$pdfCppTests = Get-RepoFileText -Root $resolvedRepoRoot `
    -RelativePath "test\cmake\PdfCppTests.cmake"
Assert-ContainsText -Text $pdfCppTests `
    -ExpectedText "--source-file=*pdf_cli_import_threshold_tests.cpp" `
    -Message "PDF threshold CTest should keep the focused doctest source filter."
Assert-ContainsText -Text $pdfCppTests -ExpectedText "--no-breaks=true" `
    -Message "Windows PDF threshold doctest registration should disable debugger breaks."

$documentSecurityTests = Get-RepoFileText -Root $resolvedRepoRoot `
    -RelativePath "test\document_security_tests.cpp"
foreach ($marker in @(
        'zip_fail_next_close_stage',
        'zip_fail_next_write',
        'document_fail_next_sync_stage'
    )) {
    Assert-ContainsText -Text $documentSecurityTests -ExpectedText $marker `
        -Message "Document security tests should keep fault-injection coverage marker '$marker'."
}
Assert-ContainsText -Text $documentSecurityTests -ExpectedText "#ifndef _WIN32" `
    -Message "Document security fault-injection tests should stay isolated from Windows ordinary runs."
Assert-ContainsText -Text $documentSecurityTests `
    -ExpectedText "FEATHERDOC_ENABLE_WINDOWS_FAULT_INJECTION_TESTS" `
    -Message "Windows replacement failure injection should stay excluded from ordinary runs."

$documentSourceArchiveConsistencyTests = Get-RepoFileText -Root $resolvedRepoRoot `
    -RelativePath "test\document_source_archive_consistency_tests.cpp"
Assert-ContainsText -Text $documentSourceArchiveConsistencyTests `
    -ExpectedText 'document_fail_next_sync_stage' `
    -Message "Source archive consistency tests should keep the directory-sync fault marker."
Assert-ContainsText -Text $documentSourceArchiveConsistencyTests `
    -ExpectedText "#ifndef _WIN32" `
    -Message "Source archive sync fault test should stay isolated from Windows ordinary runs."

$rootCMake = Get-RepoFileText -Root $resolvedRepoRoot -RelativePath "CMakeLists.txt"
Assert-ContainsText -Text $rootCMake `
    -ExpectedText "FEATHERDOC_ENABLE_SANITIZERS is unsupported on Windows; use Linux/WSL" `
    -Message "Root CMake should reject sanitizer instrumentation on every Windows toolchain."

$fuzzCMake = Get-RepoFileText -Root $resolvedRepoRoot `
    -RelativePath "cmake\FeatherDocFuzz.cmake"
Assert-ContainsText -Text $fuzzCMake `
    -ExpectedText "FEATHERDOC_BUILD_FUZZERS is unsupported on Windows; use Linux/WSL" `
    -Message "Fuzzer CMake should reject every Windows toolchain."

$releaseMetadataStepStart = $windowsWorkflow.IndexOf("Generate release metadata bundle")
if ($releaseMetadataStepStart -lt 0) {
    throw "Windows workflow should keep the release metadata bundle step."
}

$releaseMetadataNextStep = $windowsWorkflow.IndexOf("Generate artifact root start-here note", $releaseMetadataStepStart)
if ($releaseMetadataNextStep -lt 0) {
    throw "Windows workflow should keep the artifact root start-here step after release metadata generation."
}

$releaseMetadataStep = $windowsWorkflow.Substring($releaseMetadataStepStart, $releaseMetadataNextStep - $releaseMetadataStepStart)
foreach ($marker in @(
        "run_release_candidate_checks.ps1",
        "-BuildDir build-msvc-ninja",
        "-InstallDir build-msvc-install",
        "-ConsumerBuildDir build-msvc-install-consumer",
        '-Generator "Ninja"'
    )) {
    Assert-ContainsText -Text $releaseMetadataStep -ExpectedText $marker `
        -Message "Windows release metadata step should keep the Ninja-compatible marker '$marker'."
}

foreach ($unexpected in @(
        "NMake Makefiles",
        "build-msvc-nmake"
    )) {
    Assert-NotContainsText -Text $windowsWorkflow -UnexpectedText $unexpected `
        -Message "Windows workflow should not regress to the serial NMake marker '$unexpected'."
}

$linuxWorkflow = $workflowTexts[".github\workflows\linux-cmake.yml"]
$unicodePathSuffix =
    "$([char]0x4E2D)$([char]0x6587)-" +
    "$([char]0x65E5)$([char]0x672C)$([char]0x8A9E)-" +
    [char]::ConvertFromUtf32(0x1F642)
foreach ($marker in @(
        'cmake --build build-linux-${{ matrix.compiler }} --parallel 4 --verbose',
        'ctest --test-dir build-linux-${{ matrix.compiler }} --parallel 2 --output-on-failure --timeout 60',
        'ctest --test-dir build-linux-${{ matrix.compiler }} --parallel 2 --output-on-failure --timeout 60 -L cli_smoke',
        'ctest --test-dir build-linux-${{ matrix.compiler }} --parallel 2 --output-on-failure --timeout 60 -L release_smoke'
    )) {
    Assert-ContainsText -Text $linuxWorkflow -ExpectedText $marker `
        -Message "Linux workflow should keep the CTest parallel marker '$marker'."
}

foreach ($marker in @(
        "shared-library-abi:",
        "-DBUILD_SHARED_LIBS=ON",
        "source_compat_v1_13_2_tests",
        "abi_compat_v1_13_3_runtime_tests",
        "readelf -d",
        "libFeatherDoc.so.1.13",
        "install-$unicodePathSuffix",
        "consumer-$unicodePathSuffix",
        'set(FeatherDoc_ABI_VERSION "1.13")',
        "featherdoc_install_smoke"
    )) {
    Assert-ContainsText -Text $linuxWorkflow -ExpectedText $marker `
        -Message "Linux workflow should keep the shared-library ABI gate marker '$marker'."
}

$macosWorkflow = $workflowTexts[".github\workflows\macos-cmake.yml"]
foreach ($marker in @(
        "cmake --build build-macos --parallel 4 --verbose",
        "ctest --test-dir build-macos --parallel 2 --output-on-failure --timeout 60",
        "ctest --test-dir build-macos --parallel 2 --output-on-failure --timeout 60 -L cli_smoke",
        "ctest --test-dir build-macos --parallel 2 --output-on-failure --timeout 60 -L release_smoke"
    )) {
    Assert-ContainsText -Text $macosWorkflow -ExpectedText $marker `
        -Message "macOS workflow should keep the CTest parallel marker '$marker'."
}

foreach ($workflowContract in @(
        [pscustomobject]@{ Text = $linuxWorkflow; Name = "Linux workflow" },
        [pscustomobject]@{ Text = $macosWorkflow; Name = "macOS workflow" },
        [pscustomobject]@{ Text = $windowsWorkflow; Name = "Windows workflow" }
    )) {
    foreach ($marker in @(
            "Build heartbeat:",
            "is still running"
        )) {
        Assert-ContainsText -Text $workflowContract.Text -ExpectedText $marker `
            -Message "$($workflowContract.Name) should keep a visible Build heartbeat marker '$marker'."
    }
}

Assert-ContainsText -Text $windowsWorkflow -ExpectedText "Start-Process" `
    -Message "Windows workflow should run the CMake build through a process handle so heartbeat output can continue while the build runs."
Assert-ContainsText -Text $windowsWorkflow -ExpectedText 'Stop-Process -Id $build.Id -Force' `
    -Message "Windows workflow should clean up the CMake build process if the heartbeat wrapper exits early."

foreach ($marker in @(
        "output/release-assets-ci/v*/release_assets_manifest.json",
        "output/release-assets-ci/v*/FeatherDoc-*-msvc-install.zip",
        "output/release-assets-ci/v*/FeatherDoc-*-visual-validation-gallery.zip",
        "output/release-assets-ci/v*/FeatherDoc-*-release-evidence.zip",
        "compression-level: 0"
    )) {
    Assert-ContainsText -Text $windowsWorkflow -ExpectedText $marker `
        -Message "Windows workflow should keep the release asset preview upload marker '$marker'."
}

$releasePublishWorkflow = $workflowTexts[".github\workflows\release-publish.yml"]
foreach ($marker in @(
        "allow-ci-artifact-publish:",
        "Allow publishing a CI artifact bundle when Word visual gate was skipped",
        "RELEASE_OUTPUT_ROOT: output/release-assets",
        '$visualGateStatus -in @("skipped", "visual_gate_skipped")',
        '$visualVerdict -in @("", "visual_gate_skipped", "pending_manual_review")',
        "Refusing Release Publish because visual_verdict is",
        "Refusing Release Publish because visual_gate.status is",
        '"-ExecutionPolicy", "Bypass"',
        '"-File", ".\scripts\publish_github_release.ps1"',
        '"-OutputRoot", $env:RELEASE_OUTPUT_ROOT',
        '"-Publish"',
        "-AllowCiArtifactPublish",
        "name: release-publish-output",
        '${{ env.RELEASE_OUTPUT_ROOT }}/**'
    )) {
    Assert-ContainsText -Text $releasePublishWorkflow -ExpectedText $marker `
        -Message "Release Publish workflow should keep the CI artifact boundary marker '$marker'."
}
Assert-ReleaseOutputRootCleanupContract -WorkflowText $releasePublishWorkflow -WorkflowName "Release Publish workflow"
Assert-ReleaseOutputArtifactPathContract -WorkflowText $releasePublishWorkflow -WorkflowName "Release Publish workflow" -ArtifactName "release-publish-output"

$releaseRefreshWorkflow = $workflowTexts[".github\workflows\release-refresh.yml"]
foreach ($marker in @(
        "workflow_dispatch:",
        "contents: write",
        "RELEASE_OUTPUT_ROOT: output/release-assets",
        "-File .\scripts\publish_github_release.ps1",
        '-OutputRoot $env:RELEASE_OUTPUT_ROOT',
        "name: release-refresh-output",
        '${{ env.RELEASE_OUTPUT_ROOT }}/**'
    )) {
    Assert-ContainsText -Text $releaseRefreshWorkflow -ExpectedText $marker `
        -Message "Release Refresh workflow should keep the release output artifact marker '$marker'."
}
Assert-ReleaseOutputRootCleanupContract -WorkflowText $releaseRefreshWorkflow -WorkflowName "Release Refresh workflow"
Assert-ReleaseOutputArtifactPathContract -WorkflowText $releaseRefreshWorkflow -WorkflowName "Release Refresh workflow" -ArtifactName "release-refresh-output"

Write-Host "GitHub Actions workflow maintenance contract passed."
