# Contributing

FeatherDoc follows its own current roadmap. Contributions should fit the
project direction: modern C++, clear API behavior, MSVC-friendly builds, and
explicit licensing boundaries.

## Before Opening a Pull Request

Please open or reference an issue first when your change does any of the
following:

1. Changes the public API.
2. Changes document save/open behavior.
3. Updates bundled third-party dependencies.
4. Changes build requirements or supported toolchains.
5. Introduces a new file format assumption or compatibility policy.

Small fixes, typo corrections, and test-only improvements can usually be sent
directly as a focused pull request.

## Pull Request Guidelines

1. Keep each pull request focused on one topic.
2. Prefer modern C++ style over compatibility layers for obsolete patterns.
3. Add or update tests when behavior changes.
4. Keep MSVC buildability intact. Use the following full baseline only at the
   full-validation gates defined below, not after every small change:

```bat
cmake -S . -B build-msvc-nmake -G "NMake Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON ^
  -DFEATHERDOC_BUILD_ALLOCATION_FAILURE_TESTS=OFF ^
  -DFEATHERDOC_BUILD_WINDOWS_FAULT_INJECTION_TESTS=OFF ^
  -DFEATHERDOC_ENABLE_SANITIZERS=OFF -DFEATHERDOC_BUILD_FUZZERS=OFF
cmake --build build-msvc-nmake
ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60
```

5. Do not silently change licensing, attribution, or bundled dependency notices.
6. Do not mix unrelated refactors, formatting sweeps, and behavior changes in
   one pull request.

### Validation Scope Policy

This policy is mandatory. Small changes use targeted validation only. Full
builds and full test suites are completion gates for large work, not feedback
loops for each incremental edit.

| Change scope | Required local validation | Full suite / full CI matrix |
| --- | --- | --- |
| Documentation-only change | `git diff --check` and only the relevant documentation checker, if one exists | No |
| Single isolated small feature, focused bug fix, setter/method change, isolated refactor, or focused test addition | Build only the affected target and run only the exact related test case(s) on Windows/MSVC | No |
| Planned batch of adjacent small APIs in one component | Use `git diff --check` for each source step, then build the affected targets and run the exact component tests once when the batch closes | No |
| Completed large feature or completed module milestone | Full Windows build and full Windows test suite | Yes |
| Cross-module integration checkpoint or release candidate | Full Windows validation plus the required cross-platform/release gates | Yes |
| Reproduction of a specific CI-only failure | Only the failing platform, target, and test group needed to reproduce it | Only if the work also reaches a full-validation gate |

Do not rebuild and rerun the same test executable after every API in a planned
source batch. Keep each API independently reviewable, run `git diff --check`,
and close the batch with one Windows/MSVC validation pass over the union of its
affected targets and exact tests. A compile-sensitive or urgent standalone fix
can still use immediate targeted validation.

When multiple agents advance a batch, give each agent exclusive ownership of a
different source file. Agents should leave commits, pushes, builds, and tests to
the coordinating task so shared-worktree changes remain reviewable. Prefer one
API per commit even when the validation pass covers several commits.

Reuse an existing Windows build directory when practical and run commands
equivalent to:

```powershell
cmake --build <windows-build-dir> --target <affected-test-target> --parallel 1
ctest --test-dir <windows-build-dir> -R "^<affected-test-name>$" --output-on-failure
```

For the current table-property transaction boundary and its grouped Windows
targets, see `docs/table_mutation_atomicity.md`.

The following actions are prohibited by default for a small change:

- building the default/all target when a narrower target exists;
- running unfiltered `ctest` or the complete test suite;
- running local WSL/Linux, Sanitizer, fuzzing, or allocation-failure suites;
- manually triggering or waiting for every GitHub Actions workflow as the
  feedback loop for each method, setter, small fix, or test addition.

If a targeted test fails, expand validation gradually: exact test case first,
then the nearest component test group. Do not jump directly to the full suite.

A sequence of small implementation steps remains small validation work until
the maintainer declares the large feature or module milestone complete. At that
completion point, run the full Windows suite and require the GitHub Actions
matrix: `windows-msvc.yml`, `linux-cmake.yml`, `macos-cmake.yml`, and
`security-sanitizers-fuzz.yml`.

Automatic CI triggered by an intermediate push does not require blocking local
progress or monitoring every workflow unless the push is a declared
full-validation gate. Transient service or HTTP 429 failures should be retried
with backoff instead of being treated as code failures.

Use a local Linux or WSL build only for release preparation, a completed broad
cross-module validation gate that needs local Linux evidence, or reproduction
of a specific hosted CI failure. Use bounded concurrency (one build job by
default). After a check, stop or wait for its build/test processes, remove only
the isolated temporary worktree/build created for that check, and verify that
the repository worktree remains clean. Do not delete shared build caches merely
to satisfy cleanup.

### Test Safety Matrix

When a full-validation gate is reached, Windows is the ordinary Release/MSVC
validation platform. For small changes, follow the targeted validation policy
above. Keep allocation failure tests, Windows filesystem failure injection,
sanitizers, and fuzzers disabled on Windows. CMake rejects unsupported
configurations, and Windows-specific filesystem failure injection requires the
explicit
``FEATHERDOC_BUILD_WINDOWS_FAULT_INJECTION_TESTS=ON`` opt-in. CTest adds
``--no-breaks=true`` to native test executables so unexpected assertions are
reported without opening an interactive assertion dialog.

At a full-validation gate, the `security-sanitizers-fuzz.yml` workflow is the
default execution environment for deterministic allocation-failure and
sanitizer coverage. When a local Linux reproduction is justified by the policy
above, use an isolated Linux or WSL build (prefer a native ext4 directory rather
than ``/mnt/c``):

```sh
cmake -S . -B build-fault \
  -DBUILD_TESTING=ON \
  -DFEATHERDOC_BUILD_ALLOCATION_FAILURE_TESTS=ON \
  -DFEATHERDOC_ENABLE_SANITIZERS=ON
cmake --build build-fault --parallel 1
ctest --test-dir build-fault -L allocation-failure --output-on-failure
```

Ordinary test executables never register the ``allocation-failure`` suite, even
when doctest is invoked with options that include skipped tests.

## Branch Workflow

- `dev` is the default development branch.
- Codex/local automation should continue on the current `dev` checkout by
  default. Do not create `codex/*` or other task branches unless the maintainer
  explicitly asks for a new branch.
- `master` is reserved for stable/release flow and should receive changes only
  through the established release process.

## Versioning And Release Expectations

FeatherDoc uses a pragmatic `MAJOR.MINOR.PATCH` release model.

1. Use `MAJOR` for breaking public API or packaging changes.
2. Use `MINOR` for new features, performance work, diagnostics improvements, and
   compatible API expansion.
3. Use `PATCH` for bug fixes, build fixes, test fixes, and documentation fixes.

If your change affects public API, save/open semantics, install layout, package
variables, or release-facing metadata, mention that explicitly in the pull
request description.

For the current repository-side release rules, see:

- `docs/release_policy_zh.rst`

## Coding Expectations

1. Prefer readable, explicit code over clever shortcuts.
2. Use `std::filesystem`, `std::error_code`, strong enums, and other modern
   standard library facilities where appropriate.
3. Avoid reviving removed legacy compatibility APIs unless there is a clear
   migration reason.
4. Keep diagnostics actionable. Failures should point to a path, archive entry,
   or other concrete context whenever possible.
5. When performance-sensitive paths are touched, avoid unnecessary copies and
   large transient allocations.

## Reporting Bugs

Use GitHub issues and include:

1. Compiler and version.
2. CMake generator.
3. Operating system.
4. Minimal reproduction code.
5. Whether the issue reproduces on MSVC.
6. A sanitized sample `.docx` if the issue depends on document contents.

## Licensing Note For Contributions

By submitting code, documentation, or other repository content, you agree that
your contribution may be distributed under the repository's current licensing
structure:

1. Fork-specific FeatherDoc modifications are governed by `LICENSE`.
2. Preserved upstream-derived portions remain documented by `LICENSE.upstream-mit`.
3. Bundled third-party components keep their own original licenses.

If you are not comfortable contributing under that structure, do not submit the
change.

## Conduct

Be direct, technical, and respectful.

1. Critique code and decisions, not people.
2. Keep discussions evidence-based.
3. Do not post private information, abuse, or harassment.
4. Maintainers may close or reject discussions that are off-topic, hostile, or
   clearly incompatible with the project direction.
