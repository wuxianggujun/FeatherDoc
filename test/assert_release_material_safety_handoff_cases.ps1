$badEntryProjectTemplateStatusTraceDir = Join-Path $failDir "entry-project-template-readiness-missing-status-release-ready"
$badEntryProjectTemplateStatusTracePath = Join-Path $badEntryProjectTemplateStatusTraceDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateStatusTraceDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateStatusTracePath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badEntryProjectTemplateStatusTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateStatusTracePath
} catch {
    $badEntryProjectTemplateStatusTraceFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateStatusTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md without project-template readiness status/release_ready in the readiness block."
}

$badEntryProjectTemplateInvalidStatusDir = Join-Path $failDir "entry-project-template-readiness-invalid-status"
$badEntryProjectTemplateInvalidStatusPath = Join-Path $badEntryProjectTemplateInvalidStatusDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateInvalidStatusDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateInvalidStatusPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready-ish release_ready: True latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badEntryProjectTemplateInvalidStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateInvalidStatusPath
} catch {
    $badEntryProjectTemplateInvalidStatusFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateInvalidStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with an invalid project-template readiness status."
}

$badEntryProjectTemplateInvalidReadyDir = Join-Path $failDir "entry-project-template-readiness-invalid-release-ready"
$badEntryProjectTemplateInvalidReadyPath = Join-Path $badEntryProjectTemplateInvalidReadyDir "START_HERE.md"
New-Item -ItemType Directory -Path $badEntryProjectTemplateInvalidReadyDir -Force | Out-Null
Set-Content -LiteralPath $badEntryProjectTemplateInvalidReadyPath -Encoding UTF8 -Value @"
# START_HERE

- Project template readiness: project_template_delivery_readiness project_template_delivery_readiness_contract source_schema=featherdoc.project_template_delivery_readiness_report.v1 status: ready release_ready: maybe latest_schema_approval_gate_status=passed schema_approval_status_summary=approved=4 source_report_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- Project template onboarding: project_template_onboarding.schema_approval project_template_onboarding_governance_contract source_schema=featherdoc.project_template_onboarding_governance_report.v1 schema_approval_status_summary=approved source_report_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json source_json_display=.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badEntryProjectTemplateInvalidReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryProjectTemplateInvalidReadyPath
} catch {
    $badEntryProjectTemplateInvalidReadyFailedAsExpected = $true
}

if (-not $badEntryProjectTemplateInvalidReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed START_HERE.md with an invalid project-template readiness release_ready."
}

$badReleaseHandoffTraceDir = Join-Path $failDir "release-handoff-missing-project-template-source-json"
$badReleaseHandoffTracePath = Join-Path $badReleaseHandoffTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffTracePath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json

## Detached notes

- project_template_delivery_readiness: source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseHandoffTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffTracePath
} catch {
    $badReleaseHandoffTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with project-template source_json_display outside the readiness list block."
}

$badReleaseHandoffReadinessSplitTraceDir = Join-Path $failDir "release-handoff-readiness-source-json-supplied-by-onboarding"
$badReleaseHandoffReadinessSplitTracePath = Join-Path $badReleaseHandoffReadinessSplitTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffReadinessSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffReadinessSplitTracePath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffReadinessSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffReadinessSplitTracePath
} catch {
    $badReleaseHandoffReadinessSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffReadinessSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with readiness source_json_display supplied only by onboarding."
}

$badReleaseHandoffOnboardingSplitTraceDir = Join-Path $failDir "release-handoff-onboarding-source-json-supplied-by-readiness"
$badReleaseHandoffOnboardingSplitTracePath = Join-Path $badReleaseHandoffOnboardingSplitTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffOnboardingSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffOnboardingSplitTracePath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json

## Detached notes

- project_template_onboarding.schema_approval source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffOnboardingSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffOnboardingSplitTracePath
} catch {
    $badReleaseHandoffOnboardingSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffOnboardingSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with onboarding source_json_display outside the onboarding list block."
}

$badReleaseHandoffReadinessSourceIdentityDir = Join-Path $failDir "release-handoff-readiness-source-identity-impersonated"
$badReleaseHandoffReadinessSourceIdentityPath = Join-Path $badReleaseHandoffReadinessSourceIdentityDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffReadinessSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffReadinessSourceIdentityPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffReadinessSourceIdentityPath
} catch {
    $badReleaseHandoffReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseHandoffReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with readiness source displays pointing at onboarding governance evidence."
}

$badReleaseHandoffOnboardingSourceJsonIdentityDir = Join-Path $failDir "release-handoff-onboarding-source-json-identity-impersonated"
$badReleaseHandoffOnboardingSourceJsonIdentityPath = Join-Path $badReleaseHandoffOnboardingSourceJsonIdentityDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffOnboardingSourceJsonIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffOnboardingSourceJsonIdentityPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseHandoffOnboardingSourceJsonIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffOnboardingSourceJsonIdentityPath
} catch {
    $badReleaseHandoffOnboardingSourceJsonIdentityFailedAsExpected = $true
}

if (-not $badReleaseHandoffOnboardingSourceJsonIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with onboarding source_json_display pointing at delivery readiness evidence."
}

$badReleaseHandoffInvalidOnboardingStatusDir = Join-Path $failDir "release-handoff-invalid-project-template-onboarding-status"
$badReleaseHandoffInvalidOnboardingStatusPath = Join-Path $badReleaseHandoffInvalidOnboardingStatusDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready-ish
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffInvalidOnboardingStatusPath
} catch {
    $badReleaseHandoffInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badReleaseHandoffInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with an invalid nested onboarding status value."
}

$badReleaseHandoffInvalidOnboardingReadyDir = Join-Path $failDir "release-handoff-invalid-project-template-onboarding-release-ready"
$badReleaseHandoffInvalidOnboardingReadyPath = Join-Path $badReleaseHandoffInvalidOnboardingReadyDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: maybe
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseHandoffInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffInvalidOnboardingReadyPath
} catch {
    $badReleaseHandoffInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badReleaseHandoffInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with an invalid nested onboarding release_ready value."
}

$badReleaseHandoffInvalidReadinessStatusDir = Join-Path $failDir "release-handoff-invalid-project-template-readiness-status"
$badReleaseHandoffInvalidReadinessStatusPath = Join-Path $badReleaseHandoffInvalidReadinessStatusDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready-ish ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready-ish
    - release_ready: True
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseHandoffInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffInvalidReadinessStatusPath
} catch {
    $badReleaseHandoffInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badReleaseHandoffInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with an invalid project-template readiness status value."
}

$badReleaseHandoffInvalidReadinessReadyDir = Join-Path $failDir "release-handoff-invalid-project-template-readiness-release-ready"
$badReleaseHandoffInvalidReadinessReadyPath = Join-Path $badReleaseHandoffInvalidReadinessReadyDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release handoff

- project_template_delivery_readiness: status=ready ready=True source_failures=0 schema=featherdoc.project_template_delivery_readiness_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_delivery_readiness_contract:
    - source_schema: featherdoc.project_template_delivery_readiness_report.v1
    - status: ready
    - release_ready: maybe
    - latest_schema_approval_gate_status: passed
    - schema_approval_status_summary: approved=4
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseHandoffInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffInvalidReadinessReadyPath
} catch {
    $badReleaseHandoffInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badReleaseHandoffInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with an invalid project-template readiness release_ready value."
}

$badReleaseHandoffPdfSplitTraceDir = Join-Path $failDir "release-handoff-split-pdf-visual-contact-sheet"
$badReleaseHandoffPdfSplitTracePath = Join-Path $badReleaseHandoffPdfSplitTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffPdfSplitTracePath -Encoding UTF8 -Value @"
# Release handoff

## PDF visual gate evidence

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: ready
- PDF visual gate verdict: pass
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet:

## Detached notes

- archived_contact_sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReleaseHandoffPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffPdfSplitTracePath
} catch {
    $badReleaseHandoffPdfSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with a PDF visual gate contact-sheet path outside the contact sheet line."
}

$badReleaseHandoffPdfDetachedCountTraceDir = Join-Path $failDir "release-handoff-pdf-visual-count-supplied-by-detached-notes"
$badReleaseHandoffPdfDetachedCountTracePath = Join-Path $badReleaseHandoffPdfDetachedCountTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffPdfDetachedCountTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffPdfDetachedCountTracePath -Encoding UTF8 -Value @"
# Release handoff

## PDF visual gate evidence

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: ready
- PDF visual gate verdict: pass
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png

## Detached notes

- PDF visual baseline manifest samples: 42
"@

$badReleaseHandoffPdfDetachedCountTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffPdfDetachedCountTracePath
} catch {
    $badReleaseHandoffPdfDetachedCountTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffPdfDetachedCountTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with PDF visual counts supplied only by detached notes."
}

$badReleaseHandoffPdfVerdictTraceDir = Join-Path $failDir "release-handoff-invalid-pdf-visual-verdict"
$badReleaseHandoffPdfVerdictTracePath = Join-Path $badReleaseHandoffPdfVerdictTraceDir "release_handoff.md"
New-Item -ItemType Directory -Path $badReleaseHandoffPdfVerdictTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseHandoffPdfVerdictTracePath -Encoding UTF8 -Value @"
# Release handoff

## PDF visual gate evidence

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate evidence status: ready
- PDF visual gate verdict: blocked
- PDF CJK manifest samples: 43
- PDF CJK copy/search samples: 43
- PDF visual baseline manifest samples: 42
- PDF visual baselines: 44
- PDF visual gate aggregate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badReleaseHandoffPdfVerdictTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseHandoffPdfVerdictTracePath
} catch {
    $badReleaseHandoffPdfVerdictTraceFailedAsExpected = $true
}

if (-not $badReleaseHandoffPdfVerdictTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_handoff.md with a non-pass/fail PDF visual gate verdict."
}

$badReleaseGovernanceHandoffTraceDir = Join-Path $failDir "release-governance-handoff-missing-project-template-source-json"
$badReleaseGovernanceHandoffTracePath = Join-Path $badReleaseGovernanceHandoffTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``

## Detached notes

- source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
"@

$badReleaseGovernanceHandoffTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffTracePath
} catch {
    $badReleaseGovernanceHandoffTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with project-template source_json_display outside the readiness report-status block."
}

$badReleaseGovernanceHandoffStatusTraceDir = Join-Path $failDir "release-governance-handoff-missing-project-template-status-ready"
$badReleaseGovernanceHandoffStatusTracePath = Join-Path $badReleaseGovernanceHandoffStatusTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffStatusTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffStatusTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

$badReleaseGovernanceHandoffStatusTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffStatusTracePath
} catch {
    $badReleaseGovernanceHandoffStatusTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffStatusTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md without project-template status/ready in the readiness report-status block."
}

$badReleaseGovernanceHandoffInvalidReadinessStatusDir = Join-Path $failDir "release-governance-handoff-invalid-project-template-readiness-status"
$badReleaseGovernanceHandoffInvalidReadinessStatusPath = Join-Path $badReleaseGovernanceHandoffInvalidReadinessStatusDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready-ish`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

$badReleaseGovernanceHandoffInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffInvalidReadinessStatusPath
} catch {
    $badReleaseGovernanceHandoffInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with an invalid project-template readiness status value."
}

$badReleaseGovernanceHandoffInvalidReadinessReadyDir = Join-Path $failDir "release-governance-handoff-invalid-project-template-readiness-ready"
$badReleaseGovernanceHandoffInvalidReadinessReadyPath = Join-Path $badReleaseGovernanceHandoffInvalidReadinessReadyDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``maybe`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

$badReleaseGovernanceHandoffInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffInvalidReadinessReadyPath
} catch {
    $badReleaseGovernanceHandoffInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with an invalid project-template readiness ready value."
}

$badReleaseGovernanceHandoffOnboardingTraceDir = Join-Path $failDir "release-governance-handoff-missing-project-template-onboarding-source-json"
$badReleaseGovernanceHandoffOnboardingTracePath = Join-Path $badReleaseGovernanceHandoffOnboardingTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffOnboardingTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffOnboardingTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json

## Detached notes

- project_template_onboarding.schema_approval source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseGovernanceHandoffOnboardingTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffOnboardingTracePath
} catch {
    $badReleaseGovernanceHandoffOnboardingTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffOnboardingTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with project-template onboarding source_json_display outside the onboarding blocker block."
}

$badReleaseGovernanceHandoffReadinessSourceIdentityDir = Join-Path $failDir "release-governance-handoff-readiness-source-identity-impersonated"
$badReleaseGovernanceHandoffReadinessSourceIdentityPath = Join-Path $badReleaseGovernanceHandoffReadinessSourceIdentityDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffReadinessSourceIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffReadinessSourceIdentityPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Report Status

- ``project_template_delivery_readiness``: status=``ready`` ready=``True`` blockers=``0`` actions=``0`` source_failures=``0`` source_failure_count=``0`` schema=``featherdoc.project_template_delivery_readiness_report.v1``
  - summary: ``.\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json``
  - source_report_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
  - source_json_display: ``.\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json``
  - latest_schema_approval_gate_status: ``passed``
  - schema_approval_status_summary: ``approved=4``
"@

$badReleaseGovernanceHandoffReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffReadinessSourceIdentityPath
} catch {
    $badReleaseGovernanceHandoffReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with readiness source displays pointing at onboarding governance evidence."
}

$badReleaseGovernanceHandoffOnboardingSourceJsonIdentityDir = Join-Path $failDir "release-governance-handoff-onboarding-source-json-identity-impersonated"
$badReleaseGovernanceHandoffOnboardingSourceJsonIdentityPath = Join-Path $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json
"@

$badReleaseGovernanceHandoffOnboardingSourceJsonIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityPath
} catch {
    $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffOnboardingSourceJsonIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with onboarding source_json_display pointing at delivery readiness evidence."
}

$badReleaseGovernanceHandoffInvalidOnboardingStatusDir = Join-Path $failDir "release-governance-handoff-invalid-project-template-onboarding-status"
$badReleaseGovernanceHandoffInvalidOnboardingStatusPath = Join-Path $badReleaseGovernanceHandoffInvalidOnboardingStatusDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready-ish
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseGovernanceHandoffInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffInvalidOnboardingStatusPath
} catch {
    $badReleaseGovernanceHandoffInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with an invalid nested onboarding status value."
}

$badReleaseGovernanceHandoffInvalidOnboardingReadyDir = Join-Path $failDir "release-governance-handoff-invalid-project-template-onboarding-release-ready"
$badReleaseGovernanceHandoffInvalidOnboardingReadyPath = Join-Path $badReleaseGovernanceHandoffInvalidOnboardingReadyDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blockers

- project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: maybe
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
    - source_json_display: .\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json
"@

$badReleaseGovernanceHandoffInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffInvalidOnboardingReadyPath
} catch {
    $badReleaseGovernanceHandoffInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with an invalid nested onboarding release_ready value."
}

$badReleaseGovernanceHandoffPdfTraceDir = Join-Path $failDir "release-governance-handoff-missing-pdf-visual-contact-sheet"
$badReleaseGovernanceHandoffPdfTracePath = Join-Path $badReleaseGovernanceHandoffPdfTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``
"@

$badReleaseGovernanceHandoffPdfTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfTracePath
} catch {
    $badReleaseGovernanceHandoffPdfTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md without PDF visual gate contact-sheet trace."
}

$badReleaseGovernanceHandoffPdfSplitTraceDir = Join-Path $failDir "release-governance-handoff-split-pdf-visual-contact-sheet"
$badReleaseGovernanceHandoffPdfSplitTracePath = Join-Path $badReleaseGovernanceHandoffPdfSplitTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfSplitTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ````
    - pdf_visual_gate_cjk_manifest_count: ``43``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_manifest_count: ``42``
    - pdf_visual_gate_visual_baseline_count: ``44``

## Detached notes

- archived_contact_sheet: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
"@

$badReleaseGovernanceHandoffPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfSplitTracePath
} catch {
    $badReleaseGovernanceHandoffPdfSplitTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md with a PDF visual gate contact-sheet path outside the source_report block."
}

$badReleaseGovernanceHandoffPdfManifestTraceDir = Join-Path $failDir "release-governance-handoff-missing-pdf-visual-manifest-count"
$badReleaseGovernanceHandoffPdfManifestTracePath = Join-Path $badReleaseGovernanceHandoffPdfManifestTraceDir "release_governance_handoff.md"
New-Item -ItemType Directory -Path $badReleaseGovernanceHandoffPdfManifestTraceDir -Force | Out-Null
Set-Content -LiteralPath $badReleaseGovernanceHandoffPdfManifestTracePath -Encoding UTF8 -Value @"
# Release Governance Handoff

## Release Blocker Rollup

- Status: ``blocked``
- PDF visual gate evidence source reports: ``1``
  - source_report: ``.\output\release-candidate-checks\summary.json`` schema=``featherdoc.release_candidate_summary``
    - pdf_visual_gate_status: ``loaded``
    - pdf_visual_gate_verdict: ``pass``
    - pdf_visual_gate_finalizable: ``True``
    - pdf_visual_gate_summary_json_display: ``.\output\pdf-visual-release-gate-current\report\summary.json``
    - pdf_visual_gate_aggregate_contact_sheet_display: ``.\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png``
    - pdf_visual_gate_cjk_copy_search_count: ``43``
    - pdf_visual_gate_visual_baseline_count: ``44``
"@

$badReleaseGovernanceHandoffPdfManifestTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badReleaseGovernanceHandoffPdfManifestTracePath
} catch {
    $badReleaseGovernanceHandoffPdfManifestTraceFailedAsExpected = $true
}

if (-not $badReleaseGovernanceHandoffPdfManifestTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release_governance_handoff.md without PDF visual gate manifest-count trace."
}
