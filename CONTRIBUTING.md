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
4. Keep MSVC buildability intact. The current baseline validation flow is:

```bat
cmake -S . -B build-msvc-nmake -G "NMake Makefiles" -DBUILD_TESTING=ON -DBUILD_SAMPLES=ON
cmake --build build-msvc-nmake
ctest --test-dir build-msvc-nmake --output-on-failure --timeout 60
```

5. Do not silently change licensing, attribution, or bundled dependency notices.
6. Do not mix unrelated refactors, formatting sweeps, and behavior changes in
   one pull request.

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
