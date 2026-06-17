$badFinalReviewStatusTraceDir = Join-Path $failDir "final-review-missing-project-template-readiness-status"
$badFinalReviewStatusTracePath = Join-Path $badFinalReviewStatusTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewStatusTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewStatusTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewStatusTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewStatusTracePath
} catch {
    $badFinalReviewStatusTraceFailedAsExpected = $true
}

if (-not $badFinalReviewStatusTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md without project-template readiness_status/readiness_release_ready in the handoff blocker block."
}

$badFinalReviewInvalidReadinessStatusDir = Join-Path $failDir "final-review-invalid-project-template-readiness-status"
$badFinalReviewInvalidReadinessStatusPath = Join-Path $badFinalReviewInvalidReadinessStatusDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewInvalidReadinessStatusDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewInvalidReadinessStatusPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready-ish
  - readiness_release_ready: True
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewInvalidReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewInvalidReadinessStatusPath
} catch {
    $badFinalReviewInvalidReadinessStatusFailedAsExpected = $true
}

if (-not $badFinalReviewInvalidReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with an invalid project-template readiness_status value."
}

$badFinalReviewInvalidReadinessReadyDir = Join-Path $failDir "final-review-invalid-project-template-readiness-release-ready"
$badFinalReviewInvalidReadinessReadyPath = Join-Path $badFinalReviewInvalidReadinessReadyDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewInvalidReadinessReadyDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewInvalidReadinessReadyPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready
  - readiness_release_ready: maybe
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewInvalidReadinessReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewInvalidReadinessReadyPath
} catch {
    $badFinalReviewInvalidReadinessReadyFailedAsExpected = $true
}

if (-not $badFinalReviewInvalidReadinessReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with an invalid project-template readiness_release_ready value."
}

$badFinalReviewTraceDir = Join-Path $failDir "final-review-missing-project-template-source-json"
$badFinalReviewTracePath = Join-Path $badFinalReviewTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
"@

$badFinalReviewTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewTracePath
} catch {
    $badFinalReviewTraceFailedAsExpected = $true
}

if (-not $badFinalReviewTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md without project-template source_json_display."
}

$badFinalReviewSplitTraceDir = Join-Path $failDir "final-review-project-template-source-json-supplied-by-detached-notes"
$badFinalReviewSplitTracePath = Join-Path $badFinalReviewSplitTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewSplitTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json

## Detached notes

- project_template_delivery_readiness / project_template_onboarding.schema_approval: detached source_json_display: .\output\project-template-onboarding-governance\summary.json project-template-onboarding-governance
"@

$badFinalReviewSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewSplitTracePath
} catch {
    $badFinalReviewSplitTraceFailedAsExpected = $true
}

if (-not $badFinalReviewSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with project-template source_json_display supplied only by repeated-anchor detached notes."
}

$badFinalReviewSourceJsonIdentityDir = Join-Path $failDir "final-review-project-template-source-json-identity-impersonated"
$badFinalReviewSourceJsonIdentityPath = Join-Path $badFinalReviewSourceJsonIdentityDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewSourceJsonIdentityDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewSourceJsonIdentityPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
"@

$badFinalReviewSourceJsonIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewSourceJsonIdentityPath
} catch {
    $badFinalReviewSourceJsonIdentityFailedAsExpected = $true
}

if (-not $badFinalReviewSourceJsonIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with project-template source_json_display pointing at delivery readiness evidence."
}

$badFinalReviewInvalidOnboardingStatusDir = Join-Path $failDir "final-review-invalid-project-template-onboarding-status"
$badFinalReviewInvalidOnboardingStatusPath = Join-Path $badFinalReviewInvalidOnboardingStatusDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewInvalidOnboardingStatusDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewInvalidOnboardingStatusPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready
  - readiness_release_ready: True
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready-ish
    - release_ready: True
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewInvalidOnboardingStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewInvalidOnboardingStatusPath
} catch {
    $badFinalReviewInvalidOnboardingStatusFailedAsExpected = $true
}

if (-not $badFinalReviewInvalidOnboardingStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with an invalid nested onboarding status value."
}

$badFinalReviewInvalidOnboardingReadyDir = Join-Path $failDir "final-review-invalid-project-template-onboarding-release-ready"
$badFinalReviewInvalidOnboardingReadyPath = Join-Path $badFinalReviewInvalidOnboardingReadyDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewInvalidOnboardingReadyDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewInvalidOnboardingReadyPath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Release governance handoff details

### Handoff Blockers

- project_template_delivery_readiness / project_template_onboarding.schema_approval: action=review_schema_update_candidate source_schema=featherdoc.project_template_onboarding_governance_report.v1
  - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
  - source_json_display: .\output\project-template-onboarding-governance\summary.json
  - readiness_status: ready
  - readiness_release_ready: True
  - project_template_onboarding_governance_contract:
    - source_schema: featherdoc.project_template_onboarding_governance_report.v1
    - status: ready
    - release_ready: maybe
    - schema_approval_status_summary: approved
    - source_report_display: .\output\release-governance-handoff\project-template-delivery-readiness\summary.json
    - source_json_display: .\output\project-template-onboarding-governance\summary.json
"@

$badFinalReviewInvalidOnboardingReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewInvalidOnboardingReadyPath
} catch {
    $badFinalReviewInvalidOnboardingReadyFailedAsExpected = $true
}

if (-not $badFinalReviewInvalidOnboardingReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with an invalid nested onboarding release_ready value."
}

$badFinalReviewPdfTraceDir = Join-Path $failDir "final-review-missing-pdf-visual-contact-sheet"
$badFinalReviewPdfTracePath = Join-Path $badFinalReviewPdfTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
"@

$badFinalReviewPdfTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfTracePath
} catch {
    $badFinalReviewPdfTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md without PDF visual gate contact-sheet trace."
}

$badFinalReviewPdfSplitTraceDir = Join-Path $failDir "final-review-split-pdf-visual-contact-sheet"
$badFinalReviewPdfSplitTracePath = Join-Path $badFinalReviewPdfSplitTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfSplitTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfSplitTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet:

## Detached notes

- archived_contact_sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badFinalReviewPdfSplitTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfSplitTracePath
} catch {
    $badFinalReviewPdfSplitTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfSplitTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with a PDF visual gate contact-sheet path outside the contact sheet line."
}

$badFinalReviewPdfDetachedStepTraceDir = Join-Path $failDir "final-review-pdf-visual-step-status-supplied-by-detached-notes"
$badFinalReviewPdfDetachedStepTracePath = Join-Path $badFinalReviewPdfDetachedStepTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfDetachedStepTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfDetachedStepTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png

## Detached notes

- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
"@

$badFinalReviewPdfDetachedStepTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfDetachedStepTracePath
} catch {
    $badFinalReviewPdfDetachedStepTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfDetachedStepTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with PDF visual step-status markers supplied only by detached notes."
}

$badFinalReviewPdfDetachedOutputTraceDir = Join-Path $failDir "final-review-pdf-visual-key-outputs-supplied-by-detached-notes"
$badFinalReviewPdfDetachedOutputTracePath = Join-Path $badFinalReviewPdfDetachedOutputTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfDetachedOutputTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfDetachedOutputTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary:
- PDF visual gate contact sheet:

## Detached notes

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badFinalReviewPdfDetachedOutputTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfDetachedOutputTracePath
} catch {
    $badFinalReviewPdfDetachedOutputTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfDetachedOutputTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with PDF visual Key outputs evidence supplied only by detached notes."
}

$badFinalReviewPdfVerdictTraceDir = Join-Path $failDir "final-review-invalid-pdf-visual-verdict"
$badFinalReviewPdfVerdictTracePath = Join-Path $badFinalReviewPdfVerdictTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfVerdictTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfVerdictTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: blocked
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badFinalReviewPdfVerdictTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfVerdictTracePath
} catch {
    $badFinalReviewPdfVerdictTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfVerdictTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with a non-pass/fail PDF visual gate verdict."
}

$passFinalReviewPdfAuxTraceDir = Join-Path $passDir "final-review-pdf-visual-aux-trace"
$passFinalReviewPdfAuxTracePath = Join-Path $passFinalReviewPdfAuxTraceDir "final_review.md"
New-Item -ItemType Directory -Path $passFinalReviewPdfAuxTraceDir -Force | Out-Null
Set-Content -LiteralPath $passFinalReviewPdfAuxTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True
- PDF visual gate attempt: partial
- PDF visual gate attempt verdict: not_complete
- PDF visual gate attempt full status: not_complete
- PDF visual gate attempt stages: 6/6 passed, 0 incomplete
- PDF visual gate attempt pdf_regression: 91 selected, 0 failed, 7 skipped
- PDF visual gate attempt render: 44/44 fresh baselines, contact sheet pass
- PDF visual segmented gate: pass
- PDF visual segmented gate verdict: pass
- PDF visual segmented gate full status: not_complete
- PDF visual segmented gate scope: segmented_visual_gate_auxiliary_only
- PDF visual segmented gate slices: 4/4 pass
- PDF visual segmented gate coverage: 44/44 baselines, contact sheet pass

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF visual gate attempt summary: .\output\pdf-visual-release-gate-current\report\attempt-summary.json
- PDF visual gate attempt contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF visual segmented gate summary: .\output\pdf-visual-release-gate-current\report\segmented-summary.json
- PDF visual segmented gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

& $auditScript -Path $passFinalReviewPdfAuxTracePath

$badFinalReviewPdfAttemptDetachedStepTraceDir = Join-Path $failDir "final-review-pdf-visual-attempt-step-status-supplied-by-detached-notes"
$badFinalReviewPdfAttemptDetachedStepTracePath = Join-Path $badFinalReviewPdfAttemptDetachedStepTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfAttemptDetachedStepTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfAttemptDetachedStepTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True
- PDF visual gate attempt: partial
- PDF visual gate attempt verdict: not_complete
- PDF visual gate attempt full status: not_complete

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF visual gate attempt summary: .\output\pdf-visual-release-gate-current\report\attempt-summary.json
- PDF visual gate attempt contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png

## Detached notes

- PDF visual gate attempt stages: 6/6 passed, 0 incomplete
- PDF visual gate attempt pdf_regression: 91 selected, 0 failed, 7 skipped
- PDF visual gate attempt render: 44/44 fresh baselines, contact sheet pass
"@

$badFinalReviewPdfAttemptDetachedStepTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfAttemptDetachedStepTracePath
} catch {
    $badFinalReviewPdfAttemptDetachedStepTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfAttemptDetachedStepTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with PDF visual attempt step-status markers supplied only by detached notes."
}

$badFinalReviewPdfAttemptDetachedOutputTraceDir = Join-Path $failDir "final-review-pdf-visual-attempt-key-outputs-supplied-by-detached-notes"
$badFinalReviewPdfAttemptDetachedOutputTracePath = Join-Path $badFinalReviewPdfAttemptDetachedOutputTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfAttemptDetachedOutputTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfAttemptDetachedOutputTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True
- PDF visual gate attempt: partial
- PDF visual gate attempt verdict: not_complete
- PDF visual gate attempt full status: not_complete
- PDF visual gate attempt stages: 6/6 passed, 0 incomplete
- PDF visual gate attempt pdf_regression: 91 selected, 0 failed, 7 skipped
- PDF visual gate attempt render: 44/44 fresh baselines, contact sheet pass

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF visual gate attempt summary:
- PDF visual gate attempt contact sheet:

## Detached notes

- PDF visual gate attempt summary: .\output\pdf-visual-release-gate-current\report\attempt-summary.json
- PDF visual gate attempt contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badFinalReviewPdfAttemptDetachedOutputTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfAttemptDetachedOutputTracePath
} catch {
    $badFinalReviewPdfAttemptDetachedOutputTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfAttemptDetachedOutputTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with PDF visual attempt Key outputs evidence supplied only by detached notes."
}

$badFinalReviewPdfSegmentedDetachedStepTraceDir = Join-Path $failDir "final-review-pdf-visual-segmented-step-status-supplied-by-detached-notes"
$badFinalReviewPdfSegmentedDetachedStepTracePath = Join-Path $badFinalReviewPdfSegmentedDetachedStepTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfSegmentedDetachedStepTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfSegmentedDetachedStepTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True
- PDF visual segmented gate: pass
- PDF visual segmented gate verdict: pass
- PDF visual segmented gate full status: not_complete

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF visual segmented gate summary: .\output\pdf-visual-release-gate-current\report\segmented-summary.json
- PDF visual segmented gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png

## Detached notes

- PDF visual segmented gate scope: segmented_visual_gate_auxiliary_only
- PDF visual segmented gate slices: 4/4 pass
- PDF visual segmented gate coverage: 44/44 baselines, contact sheet pass
"@

$badFinalReviewPdfSegmentedDetachedStepTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfSegmentedDetachedStepTracePath
} catch {
    $badFinalReviewPdfSegmentedDetachedStepTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfSegmentedDetachedStepTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with PDF visual segmented gate step-status markers supplied only by detached notes."
}

$badFinalReviewPdfSegmentedDetachedOutputTraceDir = Join-Path $failDir "final-review-pdf-visual-segmented-key-outputs-supplied-by-detached-notes"
$badFinalReviewPdfSegmentedDetachedOutputTracePath = Join-Path $badFinalReviewPdfSegmentedDetachedOutputTraceDir "final_review.md"
New-Item -ItemType Directory -Path $badFinalReviewPdfSegmentedDetachedOutputTraceDir -Force | Out-Null
Set-Content -LiteralPath $badFinalReviewPdfSegmentedDetachedOutputTracePath -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: loaded
- PDF visual gate verdict: pass
- PDF visual gate counts: 44 visual baselines, 43 CJK copy/search
- PDF visual gate manifest counts: 42 visual baseline manifest samples, 43 CJK manifest samples
- PDF visual gate finalizable: True
- PDF visual segmented gate: pass
- PDF visual segmented gate verdict: pass
- PDF visual segmented gate full status: not_complete
- PDF visual segmented gate scope: segmented_visual_gate_auxiliary_only
- PDF visual segmented gate slices: 4/4 pass
- PDF visual segmented gate coverage: 44/44 baselines, contact sheet pass

## Key outputs

- PDF visual gate summary: .\output\pdf-visual-release-gate-current\report\summary.json
- PDF visual gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
- PDF visual segmented gate summary:
- PDF visual segmented gate contact sheet:

## Detached notes

- PDF visual segmented gate summary: .\output\pdf-visual-release-gate-current\report\segmented-summary.json
- PDF visual segmented gate contact sheet: .\output\pdf-visual-release-gate-current\report\aggregate-contact-sheet.png
"@

$badFinalReviewPdfSegmentedDetachedOutputTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badFinalReviewPdfSegmentedDetachedOutputTracePath
} catch {
    $badFinalReviewPdfSegmentedDetachedOutputTraceFailedAsExpected = $true
}

if (-not $badFinalReviewPdfSegmentedDetachedOutputTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed final_review.md with PDF visual segmented gate Key outputs evidence supplied only by detached notes."
}

$passMetadataOnlyReleaseBundleDir = Join-Path $passDir "metadata-only-release-bundle"
$passMetadataOnlyReleaseBundleReportDir = Join-Path $passMetadataOnlyReleaseBundleDir "report"
New-Item -ItemType Directory -Path $passMetadataOnlyReleaseBundleReportDir -Force | Out-Null
Set-Content -LiteralPath (Join-Path $passMetadataOnlyReleaseBundleDir "START_HERE.md") -Encoding UTF8 -Value @"
# Release Metadata Start Here

- Project template release readiness checklist: `docs/project_template_release_readiness_checklist_zh.rst`
- Open `output\release-assets\v1.12.0\release_assets_manifest.json` after packaging and confirm `project_template_delivery_readiness_contract` plus `project_template_onboarding_governance_contract` both preserve `status`, `release_ready`, `release_blocker_count`, `warning_count`, `schema_approval_status_summary`, `onboarding_governance_next_action`, `onboarding_governance_next_action_summary`, `onboarding_governance_next_action_group_count`, `next_action`, `next_action_summary`, `next_action_group_count`, `requires_reviewer_action`, `reviewer_action_summary`, `reviewer_action_reason`, `reviewer_actions`, `source_report_display`, and `source_json_display` before publishing.
"@
Set-Content -LiteralPath (Join-Path $passMetadataOnlyReleaseBundleReportDir "ARTIFACT_GUIDE.md") -Encoding UTF8 -Value @"
# Release Metadata Artifact Guide

- Project template release readiness checklist: `docs/project_template_release_readiness_checklist_zh.rst`
- Project-template governance manifest signoff: open `output\release-assets\v1.12.0\release_assets_manifest.json` after packaging and confirm `project_template_delivery_readiness_contract` plus `project_template_onboarding_governance_contract` both preserve `status`, `release_ready`, `release_blocker_count`, `warning_count`, `schema_approval_status_summary`, `onboarding_governance_next_action`, `onboarding_governance_next_action_summary`, `onboarding_governance_next_action_group_count`, `next_action`, `next_action_summary`, `next_action_group_count`, `requires_reviewer_action`, `reviewer_action_summary`, `reviewer_action_reason`, `reviewer_actions`, `source_report_display`, and `source_json_display` before publishing.
"@
Set-Content -LiteralPath (Join-Path $passMetadataOnlyReleaseBundleReportDir "REVIEWER_CHECKLIST.md") -Encoding UTF8 -Value @"
# Release Reviewer Checklist

- Project template release readiness checklist: `docs/project_template_release_readiness_checklist_zh.rst`
- [ ] Confirm the packaged release manifest preserves project-template governance contracts before publishing: output\release-assets\v1.12.0\release_assets_manifest.json must include `project_template_delivery_readiness_contract` and `project_template_onboarding_governance_contract` with `status`, `release_ready`, `release_blocker_count`, `warning_count`, `schema_approval_status_summary`, `onboarding_governance_next_action`, `onboarding_governance_next_action_summary`, `onboarding_governance_next_action_group_count`, `next_action`, `next_action_summary`, `next_action_group_count`, `requires_reviewer_action`, `reviewer_action_summary`, `reviewer_action_reason`, `reviewer_actions`, `source_report_display`, and `source_json_display`.
"@
Set-Content -LiteralPath (Join-Path $passMetadataOnlyReleaseBundleReportDir "final_review.md") -Encoding UTF8 -Value @"
# Release Candidate Checks

## Step status

- PDF visual gate: not_requested
- PDF visual gate verdict:
- PDF visual gate counts: 0 visual baselines, 0 CJK copy/search
- PDF visual gate manifest counts: 0 visual baseline manifest samples, 0 CJK manifest samples
- PDF visual gate finalizable: False

## Key outputs

- PDF visual gate summary: (not available)
- PDF visual gate contact sheet: (not available)
"@
$passMetadataOnlySummary = [ordered]@{
    schema = "featherdoc.release_candidate_summary"
    release_version = "1.12.0"
    execution_status = "pass"
    release_handoff = ".\output\release-candidate-checks-ci\report\release_handoff.md"
    artifact_guide = ".\output\release-candidate-checks-ci\report\ARTIFACT_GUIDE.md"
    reviewer_checklist = ".\output\release-candidate-checks-ci\report\REVIEWER_CHECKLIST.md"
    governance_metric_count = 0
    governance_metrics = @()
    release_blocker_rollup = [ordered]@{
        requested = $false
        status = "not_requested"
        governance_metric_count = 0
        governance_metrics = @()
        release_blocker_count = 0
        action_item_count = 0
        warning_count = 0
        source_report_count = 0
    }
    release_governance_handoff = [ordered]@{
        requested = $false
        status = "not_requested"
        governance_metric_count = 0
        governance_metrics = @()
        release_blocker_count = 0
        action_item_count = 0
        warning_count = 0
        loaded_report_count = 0
    }
}
($passMetadataOnlySummary | ConvertTo-Json -Depth 8) |
    Set-Content -LiteralPath (Join-Path $passMetadataOnlyReleaseBundleReportDir "summary.json") -Encoding UTF8
& $auditScript -Path $passMetadataOnlyReleaseBundleDir

$badEntryGovernanceTracePath = Join-Path $failDir "ARTIFACT_GUIDE.md"
Set-Content -LiteralPath $badEntryGovernanceTracePath -Encoding UTF8 -Value @"
# Artifact Guide

- Content-control repair: content_control_data_binding.bound_placeholder -> sync_bound_content_control
- Project template readiness: project_template_delivery_readiness release_ready
- Project template onboarding: project_template_onboarding.schema_approval approved
- Governance metric: delivery_quality release_ready 100
"@

$badEntryGovernanceTraceFailedAsExpected = $false
try {
    & $auditScript -Path $badEntryGovernanceTracePath
} catch {
    $badEntryGovernanceTraceFailedAsExpected = $true
}

if (-not $badEntryGovernanceTraceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release entry document without table_layout_delivery_governance.delivery_quality."
}

if ($materialSafetyCaseFilterEnabled -and $materialSafetyAuditRunCount -eq 0) {
    throw "assert_release_material_safety_test.ps1 CasePattern did not match any audit case."
}

if ($materialSafetyCaseFilterEnabled -or $ShardCount -gt 1) {
    Write-Host "Release material safety filtered audit cases: ran $materialSafetyAuditRunCount, skipped $materialSafetyAuditSkipCount, shard $ShardIndex/$ShardCount."
}

Write-Host "Release material safety audit regression passed."
