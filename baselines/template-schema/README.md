# Template Schema Baselines

This directory is the repository-level home for committed template schema
baselines.

Use [manifest.json](./manifest.json) to register every baseline that should be
checked in CI or local preflight scripts.

## Manifest format

Each entry describes one template document plus its committed schema baseline.

```json
{
  "entries": [
    {
      "name": "resolved-template-fixture",
      "input_docx_build_relative": "output/template-schema-manifest/template_schema_resolved_fixture.docx",
      "prepare_sample_target": "featherdoc_sample_template_schema_manifest_fixture",
      "schema_file": "baselines/template-schema/template_schema_resolved_fixture.schema.json",
      "target_mode": "resolved-section-targets",
      "generated_output": "output/template-schema-manifest-checks/template_schema_resolved_fixture.generated.schema.json"
    }
  ]
}
```

Rules:

- `name`: stable identifier used in logs and summary files
- `input_docx`: repository-relative path to the live DOCX to validate
- `input_docx_build_relative`: build-directory-relative DOCX path; requires `-BuildDir`
- `prepare_sample_target`: optional sample target to run before checking a build-relative DOCX
- `schema_file`: path to the committed baseline schema JSON
- `target_mode`: one of `default`, `section-targets`, `resolved-section-targets`
- `generated_output`: optional normalized generated schema output path

All relative paths are resolved from the repository root.

## Currently registered baselines

- `chinese-invoice-template`
  Covers the repository's static Chinese invoice template under
  `samples/chinese_invoice_template.docx`.
- `fill-bookmarks-visual-baseline`
  Covers a generated body-only bookmark template produced by
  `featherdoc_sample_fill_bookmarks_visual`.
- `resolved-template-fixture`
  Covers a generated linked-section header/footer fixture produced by
  `featherdoc_sample_template_schema_manifest_fixture`.

## Coverage guidance

- Prefer `input_docx` for long-lived repository templates that are committed as
  source artifacts.
- Prefer `input_docx_build_relative` plus `prepare_sample_target` for generated
  fixtures that should be recreated inside the active build tree before the
  schema check runs.
- Keep the registered set intentionally varied. A useful baseline mix should
  cover at least one real business template, one body-only bookmark template,
  and one section-aware header/footer fixture.

## Recommended workflow

1. Freeze a baseline once:

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\freeze_template_schema_baseline.ps1 `
    -InputDocx .\template.docx `
    -SchemaOutput .\baselines\template-schema\template.schema.json `
    -ResolvedSectionTargets
```

2. Register or update a manifest entry in one step:

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\register_template_schema_manifest_entry.ps1 `
    -Name template-name `
    -InputDocx .\template.docx
```

   Registration freezes the schema when needed and then runs the same baseline
   gate used by CI before it writes `manifest.json`. If the schema has lint
   issues or drifts from the DOCX, the script stops without writing the entry.
   Use `-SkipBaselineCheck` only for temporary local experiments.

   For generated fixtures, switch to `-InputDocxBuildRelative` and
   `-PrepareSampleTarget`:

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\register_template_schema_manifest_entry.ps1 `
    -Name generated-template `
    -InputDocxBuildRelative output\template-schema-manifest\generated-template.docx `
    -PrepareSampleTarget featherdoc_sample_template_schema_manifest_fixture `
    -BuildDir .\build-codex-clang-column-visual-verify `
    -TargetMode resolved-section-targets
```

3. Add an entry to `manifest.json` manually when needed.

   For generated sample fixtures, prefer `input_docx_build_relative` so the
   same manifest works with different local and CI build directories. Pair it
   with `prepare_sample_target` when the DOCX should be generated on demand
   before the baseline check runs.

4. Check every registered baseline:

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\check_template_schema_manifest.ps1
```

5. Inspect the registered baseline set plus the latest recorded status:

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\describe_template_schema_manifest.ps1
```

   When a manifest-check summary is available, the description includes both
   drift counts and schema-lint status, so dirty baselines are visible without
   opening `summary.json` by hand.

If another script or CI step needs machine-readable output, add `-Json`:

```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\describe_template_schema_manifest.ps1 `
    -BuildDir .\build-codex-clang-column-visual-verify `
    -Json `
    -OutputPath .\output\template-schema-manifest-checks-registered\manifest-description.json
```
