$badSummaryPath = Join-Path $failDir "summary.json"
$badSummary = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    release_handoff = ".\output\release-candidate-checks\report\release_handoff.md"
    release_blocker_rollup = [ordered]@{
        requested = $true
        status = "completed"
    }
}
($badSummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badSummaryPath -Encoding UTF8

$missingSummaryMetricsFailedAsExpected = $false
try {
    & $auditScript -Path $badSummaryPath
} catch {
    $missingSummaryMetricsFailedAsExpected = $true
}

if (-not $missingSummaryMetricsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release summary without governance_metrics."
}

$badSummaryWrongRealCorpusContractDir = Join-Path $failDir "summary-wrong-real-corpus-contract"
$badSummaryWrongRealCorpusContractPath = Join-Path $badSummaryWrongRealCorpusContractDir "summary.json"
New-Item -ItemType Directory -Path $badSummaryWrongRealCorpusContractDir -Force | Out-Null
$badSummaryWrongRealCorpusContract = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    release_handoff = ".\output\release-candidate-checks\report\release_handoff.md"
    governance_metric_count = 2
    governance_metrics = @(
        [ordered]@{
            id = "style_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "style_catalog_governance"
            source_schema = "featherdoc.style_catalog_governance_report.v1"
            score = 12
            level = "experimental"
        },
        [ordered]@{
            id = "table_layout_delivery_governance.delivery_quality"
            metric = "delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            score = 100
            level = "release_ready"
        }
    )
}
($badSummaryWrongRealCorpusContract | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badSummaryWrongRealCorpusContractPath -Encoding UTF8

$wrongSummaryRealCorpusContractFailedAsExpected = $false
try {
    & $auditScript -Path $badSummaryWrongRealCorpusContractPath
} catch {
    $wrongSummaryRealCorpusContractFailedAsExpected = $true
}

if (-not $wrongSummaryRealCorpusContractFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release summary without numbering_catalog_governance.real_corpus_confidence."
}

$badSummaryDecimalGovernanceMetricCountDir = Join-Path $failDir "summary-governance-metric-count-decimal"
$badSummaryDecimalGovernanceMetricCountPath = Join-Path $badSummaryDecimalGovernanceMetricCountDir "summary.json"
New-Item -ItemType Directory -Path $badSummaryDecimalGovernanceMetricCountDir -Force | Out-Null
$badSummaryDecimalGovernanceMetricCount = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    release_handoff = ".\output\release-candidate-checks\report\release_handoff.md"
    governance_metric_count = 1.5
    governance_metrics = @(
        [ordered]@{
            id = "style_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "style_catalog_governance"
            source_schema = "featherdoc.style_catalog_governance_report.v1"
            score = 12
            level = "experimental"
        },
        [ordered]@{
            id = "table_layout_delivery_governance.delivery_quality"
            metric = "delivery_quality"
            report_id = "table_layout_delivery_governance"
            source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
            score = 100
            level = "release_ready"
        }
    )
}
($badSummaryDecimalGovernanceMetricCount | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badSummaryDecimalGovernanceMetricCountPath -Encoding UTF8

$badSummaryDecimalGovernanceMetricCountFailedAsExpected = $false
try {
    & $auditScript -Path $badSummaryDecimalGovernanceMetricCountPath
} catch {
    $badSummaryDecimalGovernanceMetricCountFailedAsExpected = $true
}

if (-not $badSummaryDecimalGovernanceMetricCountFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release summary with decimal governance_metric_count."
}

$badManifestPath = Join-Path $failDir "release_assets_manifest.json"
$badManifest = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = 1
    governance_metrics = @(
        [ordered]@{
            id = "numbering_catalog_governance.real_corpus_confidence"
            metric = "real_corpus_confidence"
            report_id = "numbering_catalog_governance"
            source_schema = "featherdoc.numbering_catalog_governance_report.v1"
            score = 56
            level = "low"
        }
    )
    content_control_repair_contract_count = 0
    content_control_repair_contracts = @()
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestPath -Encoding UTF8

$missingManifestMetricFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestPath
} catch {
    $missingManifestMetricFailedAsExpected = $true
}

if (-not $missingManifestMetricFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without delivery_quality."
}

$badManifestDecimalOnboardingCountDir = Join-Path $failDir "manifest-project-template-onboarding-count-decimal"
$badManifestDecimalOnboardingCountPath = Join-Path $badManifestDecimalOnboardingCountDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestDecimalOnboardingCountDir -Force | Out-Null
$badManifestDecimalOnboardingCount = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = [ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = "ready"
        release_ready = $true
        source_file_count = 3
        source_failure_count = 0
        entry_count = 3.5
        schema_approval_status_summary = @(
            [ordered]@{
                status = "approved"
                count = 2
            }
        )
        blocked_entry_count = 0
        pending_review_entry_count = 0
        not_evaluated_entry_count = 0
        approved_entry_count = 3
        not_required_entry_count = 1
        release_blocker_count = 0
        action_item_count = 0
        manual_review_recommendation_count = 1
        source_report_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
        source_json_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
    }
}
($badManifestDecimalOnboardingCount | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestDecimalOnboardingCountPath -Encoding UTF8

$badManifestDecimalOnboardingCountFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestDecimalOnboardingCountPath
} catch {
    $badManifestDecimalOnboardingCountFailedAsExpected = $true
}

if (-not $badManifestDecimalOnboardingCountFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with decimal project_template_onboarding_governance_contract.entry_count."
}

$badManifestMissingContentControlContractDir = Join-Path $failDir "manifest-missing-content-control-contract"
$badManifestMissingContentControlContractPath = Join-Path $badManifestMissingContentControlContractDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingContentControlContractDir -Force | Out-Null
$badManifestMissingContentControlContract = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestMissingContentControlContract | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingContentControlContractPath -Encoding UTF8

$missingManifestContentControlContractFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingContentControlContractPath
} catch {
    $missingManifestContentControlContractFailedAsExpected = $true
}

if (-not $missingManifestContentControlContractFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without content_control_repair_contracts."
}

$badManifestContentControlContractCountDir = Join-Path $failDir "manifest-bad-content-control-contract-count"
$badManifestContentControlContractCountPath = Join-Path $badManifestContentControlContractCountDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestContentControlContractCountDir -Force | Out-Null
$badManifestContentControlContractCount = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 2
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestContentControlContractCount | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestContentControlContractCountPath -Encoding UTF8

$badManifestContentControlContractCountFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestContentControlContractCountPath
} catch {
    $badManifestContentControlContractCountFailedAsExpected = $true
}

if (-not $badManifestContentControlContractCountFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched content_control_repair_contract_count."
}

$badManifestContentControlContractSourceSchemaDir = Join-Path $failDir "manifest-content-control-contract-missing-source-schema"
$badManifestContentControlContractSourceSchemaPath = Join-Path $badManifestContentControlContractSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestContentControlContractSourceSchemaDir -Force | Out-Null
$badManifestContentControlContractSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestContentControlContractSourceSchema.content_control_repair_contracts[0].PSObject.Properties.Remove("source_schema")
($badManifestContentControlContractSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestContentControlContractSourceSchemaPath -Encoding UTF8

$badManifestContentControlContractSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestContentControlContractSourceSchemaPath
} catch {
    $badManifestContentControlContractSourceSchemaFailedAsExpected = $true
}

if (-not $badManifestContentControlContractSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with content-control repair contract missing source_schema."
}

$badManifestContentControlContractSourceJsonDir = Join-Path $failDir "manifest-content-control-contract-missing-source-json"
$badManifestContentControlContractSourceJsonPath = Join-Path $badManifestContentControlContractSourceJsonDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestContentControlContractSourceJsonDir -Force | Out-Null
$badManifestContentControlContractSourceJson = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestContentControlContractSourceJson.content_control_repair_contracts[0].PSObject.Properties.Remove("source_json_display")
($badManifestContentControlContractSourceJson | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestContentControlContractSourceJsonPath -Encoding UTF8

$badManifestContentControlContractSourceJsonFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestContentControlContractSourceJsonPath
} catch {
    $badManifestContentControlContractSourceJsonFailedAsExpected = $true
}

if (-not $badManifestContentControlContractSourceJsonFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with content-control repair contract missing source_json_display."
}

$badManifestProjectTemplateReadinessSourceSchemaDir = Join-Path $failDir "manifest-project-template-readiness-missing-source-schema"
$badManifestProjectTemplateReadinessSourceSchemaPath = Join-Path $badManifestProjectTemplateReadinessSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSourceSchemaDir -Force | Out-Null
$badManifestProjectTemplateReadinessSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSourceSchema.project_template_delivery_readiness_contract.PSObject.Properties.Remove("source_schema")
($badManifestProjectTemplateReadinessSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSourceSchemaPath -Encoding UTF8

$badManifestProjectTemplateReadinessSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSourceSchemaPath
} catch {
    $badManifestProjectTemplateReadinessSourceSchemaFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness contract missing source_schema."
}

$badManifestProjectTemplateReadinessSourceJsonDir = Join-Path $failDir "manifest-project-template-readiness-missing-source-json"
$badManifestProjectTemplateReadinessSourceJsonPath = Join-Path $badManifestProjectTemplateReadinessSourceJsonDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSourceJsonDir -Force | Out-Null
$badManifestProjectTemplateReadinessSourceJson = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSourceJson.project_template_delivery_readiness_contract.PSObject.Properties.Remove("source_json_display")
($badManifestProjectTemplateReadinessSourceJson | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSourceJsonPath -Encoding UTF8

$badManifestProjectTemplateReadinessSourceJsonFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSourceJsonPath
} catch {
    $badManifestProjectTemplateReadinessSourceJsonFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSourceJsonFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness contract missing source_json_display."
}

$badManifestProjectTemplateReadinessSourceReportDir = Join-Path $failDir "manifest-project-template-readiness-missing-source-report"
$badManifestProjectTemplateReadinessSourceReportPath = Join-Path $badManifestProjectTemplateReadinessSourceReportDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSourceReportDir -Force | Out-Null
$badManifestProjectTemplateReadinessSourceReport = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSourceReport.project_template_delivery_readiness_contract.PSObject.Properties.Remove("source_report_display")
($badManifestProjectTemplateReadinessSourceReport | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSourceReportPath -Encoding UTF8

$badManifestProjectTemplateReadinessSourceReportFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSourceReportPath
} catch {
    $badManifestProjectTemplateReadinessSourceReportFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSourceReportFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness contract missing source_report_display."
}

$badManifestProjectTemplateReadinessSourceIdentityDir = Join-Path $failDir "manifest-project-template-readiness-wrong-source-identity"
$badManifestProjectTemplateReadinessSourceIdentityPath = Join-Path $badManifestProjectTemplateReadinessSourceIdentityDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSourceIdentityDir -Force | Out-Null
$badManifestProjectTemplateReadinessSourceIdentity = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSourceIdentity.project_template_delivery_readiness_contract.source_report_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
$badManifestProjectTemplateReadinessSourceIdentity.project_template_delivery_readiness_contract.source_json_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
($badManifestProjectTemplateReadinessSourceIdentity | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSourceIdentityPath -Encoding UTF8

$badManifestProjectTemplateReadinessSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSourceIdentityPath
} catch {
    $badManifestProjectTemplateReadinessSourceIdentityFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness source displays pointing at onboarding governance evidence."
}

$badManifestProjectTemplateReadinessSchemaSummaryDir = Join-Path $failDir "manifest-project-template-readiness-missing-schema-summary"
$badManifestProjectTemplateReadinessSchemaSummaryPath = Join-Path $badManifestProjectTemplateReadinessSchemaSummaryDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessSchemaSummaryDir -Force | Out-Null
$badManifestProjectTemplateReadinessSchemaSummary = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessSchemaSummary.project_template_delivery_readiness_contract.PSObject.Properties.Remove("schema_approval_status_summary")
($badManifestProjectTemplateReadinessSchemaSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessSchemaSummaryPath -Encoding UTF8

$badManifestProjectTemplateReadinessSchemaSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessSchemaSummaryPath
} catch {
    $badManifestProjectTemplateReadinessSchemaSummaryFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessSchemaSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness contract missing schema_approval_status_summary."
}

$badManifestProjectTemplateReadinessStatusDir = Join-Path $failDir "manifest-project-template-readiness-status-release-ready-mismatch"
$badManifestProjectTemplateReadinessStatusPath = Join-Path $badManifestProjectTemplateReadinessStatusDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessStatusDir -Force | Out-Null
$badManifestProjectTemplateReadinessStatus = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessStatus.project_template_delivery_readiness_contract.status = "blocked"
($badManifestProjectTemplateReadinessStatus | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessStatusPath -Encoding UTF8

$badManifestProjectTemplateReadinessStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessStatusPath
} catch {
    $badManifestProjectTemplateReadinessStatusFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with release_ready=true but project template readiness status not ready."
}

$badManifestProjectTemplateReadinessInvalidStatusDir = Join-Path $failDir "manifest-project-template-readiness-invalid-status"
$badManifestProjectTemplateReadinessInvalidStatusPath = Join-Path $badManifestProjectTemplateReadinessInvalidStatusDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessInvalidStatusDir -Force | Out-Null
$badManifestProjectTemplateReadinessInvalidStatus = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessInvalidStatus.project_template_delivery_readiness_contract.status = "ready-ish"
($badManifestProjectTemplateReadinessInvalidStatus | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessInvalidStatusPath -Encoding UTF8

$badManifestProjectTemplateReadinessInvalidStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessInvalidStatusPath
} catch {
    $badManifestProjectTemplateReadinessInvalidStatusFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessInvalidStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid project template readiness status."
}

$badManifestProjectTemplateReadinessReleaseReadyDir = Join-Path $failDir "manifest-project-template-readiness-release-ready-status-mismatch"
$badManifestProjectTemplateReadinessReleaseReadyPath = Join-Path $badManifestProjectTemplateReadinessReleaseReadyDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessReleaseReadyDir -Force | Out-Null
$badManifestProjectTemplateReadinessReleaseReady = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessReleaseReady.project_template_delivery_readiness_contract.release_ready = $false
$badManifestProjectTemplateReadinessReleaseReady.project_template_delivery_readiness_contract.release_blocker_count = 1
($badManifestProjectTemplateReadinessReleaseReady | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessReleaseReadyPath -Encoding UTF8

$badManifestProjectTemplateReadinessReleaseReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessReleaseReadyPath
} catch {
    $badManifestProjectTemplateReadinessReleaseReadyFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessReleaseReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template readiness status=ready but release_ready=false."
}

$badManifestProjectTemplateReadinessInvalidReleaseReadyDir = Join-Path $failDir "manifest-project-template-readiness-invalid-release-ready"
$badManifestProjectTemplateReadinessInvalidReleaseReadyPath = Join-Path $badManifestProjectTemplateReadinessInvalidReleaseReadyDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateReadinessInvalidReleaseReadyDir -Force | Out-Null
$badManifestProjectTemplateReadinessInvalidReleaseReady = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateReadinessInvalidReleaseReady.project_template_delivery_readiness_contract.release_ready = "maybe"
($badManifestProjectTemplateReadinessInvalidReleaseReady | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateReadinessInvalidReleaseReadyPath -Encoding UTF8

$badManifestProjectTemplateReadinessInvalidReleaseReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateReadinessInvalidReleaseReadyPath
} catch {
    $badManifestProjectTemplateReadinessInvalidReleaseReadyFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateReadinessInvalidReleaseReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid project template readiness release_ready."
}

$badManifestProjectTemplateOnboardingSourceSchemaDir = Join-Path $failDir "manifest-project-template-onboarding-missing-source-schema"
$badManifestProjectTemplateOnboardingSourceSchemaPath = Join-Path $badManifestProjectTemplateOnboardingSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSourceSchemaDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSourceSchema.project_template_onboarding_governance_contract.PSObject.Properties.Remove("source_schema")
($badManifestProjectTemplateOnboardingSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSourceSchemaPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSourceSchemaPath
} catch {
    $badManifestProjectTemplateOnboardingSourceSchemaFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding contract missing source_schema."
}

$badManifestProjectTemplateOnboardingSourceJsonDir = Join-Path $failDir "manifest-project-template-onboarding-missing-source-json"
$badManifestProjectTemplateOnboardingSourceJsonPath = Join-Path $badManifestProjectTemplateOnboardingSourceJsonDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSourceJsonDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSourceJson = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSourceJson.project_template_onboarding_governance_contract.PSObject.Properties.Remove("source_json_display")
($badManifestProjectTemplateOnboardingSourceJson | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSourceJsonPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSourceJsonFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSourceJsonPath
} catch {
    $badManifestProjectTemplateOnboardingSourceJsonFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSourceJsonFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding contract missing source_json_display."
}

$badManifestProjectTemplateOnboardingSourceReportDir = Join-Path $failDir "manifest-project-template-onboarding-missing-source-report"
$badManifestProjectTemplateOnboardingSourceReportPath = Join-Path $badManifestProjectTemplateOnboardingSourceReportDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSourceReportDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSourceReport = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSourceReport.project_template_onboarding_governance_contract.PSObject.Properties.Remove("source_report_display")
($badManifestProjectTemplateOnboardingSourceReport | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSourceReportPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSourceReportFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSourceReportPath
} catch {
    $badManifestProjectTemplateOnboardingSourceReportFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSourceReportFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding contract missing source_report_display."
}

$badManifestProjectTemplateOnboardingSourceIdentityDir = Join-Path $failDir "manifest-project-template-onboarding-wrong-source-identity"
$badManifestProjectTemplateOnboardingSourceIdentityPath = Join-Path $badManifestProjectTemplateOnboardingSourceIdentityDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSourceIdentityDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSourceIdentity = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSourceIdentity.project_template_onboarding_governance_contract.source_report_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
$badManifestProjectTemplateOnboardingSourceIdentity.project_template_onboarding_governance_contract.source_json_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
($badManifestProjectTemplateOnboardingSourceIdentity | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSourceIdentityPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSourceIdentityFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSourceIdentityPath
} catch {
    $badManifestProjectTemplateOnboardingSourceIdentityFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSourceIdentityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding source displays pointing at delivery readiness evidence."
}

$badManifestProjectTemplateOnboardingSchemaSummaryDir = Join-Path $failDir "manifest-project-template-onboarding-empty-schema-summary"
$badManifestProjectTemplateOnboardingSchemaSummaryPath = Join-Path $badManifestProjectTemplateOnboardingSchemaSummaryDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingSchemaSummaryDir -Force | Out-Null
$badManifestProjectTemplateOnboardingSchemaSummary = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingSchemaSummary.project_template_onboarding_governance_contract.schema_approval_status_summary = @()
($badManifestProjectTemplateOnboardingSchemaSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingSchemaSummaryPath -Encoding UTF8

$badManifestProjectTemplateOnboardingSchemaSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingSchemaSummaryPath
} catch {
    $badManifestProjectTemplateOnboardingSchemaSummaryFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingSchemaSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with empty project template onboarding schema_approval_status_summary."
}

$badManifestProjectTemplateOnboardingInvalidStatusDir = Join-Path $failDir "manifest-project-template-onboarding-invalid-status"
$badManifestProjectTemplateOnboardingInvalidStatusPath = Join-Path $badManifestProjectTemplateOnboardingInvalidStatusDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingInvalidStatusDir -Force | Out-Null
$badManifestProjectTemplateOnboardingInvalidStatus = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingInvalidStatus.project_template_onboarding_governance_contract.status = "ready-ish"
($badManifestProjectTemplateOnboardingInvalidStatus | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingInvalidStatusPath -Encoding UTF8

$badManifestProjectTemplateOnboardingInvalidStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingInvalidStatusPath
} catch {
    $badManifestProjectTemplateOnboardingInvalidStatusFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingInvalidStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid project template onboarding status."
}

$badManifestProjectTemplateOnboardingReleaseReadyDir = Join-Path $failDir "manifest-project-template-onboarding-release-ready-status-mismatch"
$badManifestProjectTemplateOnboardingReleaseReadyPath = Join-Path $badManifestProjectTemplateOnboardingReleaseReadyDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingReleaseReadyDir -Force | Out-Null
$badManifestProjectTemplateOnboardingReleaseReady = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingReleaseReady.project_template_onboarding_governance_contract.release_ready = $false
$badManifestProjectTemplateOnboardingReleaseReady.project_template_onboarding_governance_contract.release_blocker_count = 1
($badManifestProjectTemplateOnboardingReleaseReady | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingReleaseReadyPath -Encoding UTF8

$badManifestProjectTemplateOnboardingReleaseReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingReleaseReadyPath
} catch {
    $badManifestProjectTemplateOnboardingReleaseReadyFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingReleaseReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with project template onboarding status=ready but release_ready=false."
}

$badManifestProjectTemplateOnboardingInvalidReleaseReadyDir = Join-Path $failDir "manifest-project-template-onboarding-invalid-release-ready"
$badManifestProjectTemplateOnboardingInvalidReleaseReadyPath = Join-Path $badManifestProjectTemplateOnboardingInvalidReleaseReadyDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingInvalidReleaseReadyDir -Force | Out-Null
$badManifestProjectTemplateOnboardingInvalidReleaseReady = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestProjectTemplateOnboardingInvalidReleaseReady.project_template_onboarding_governance_contract.release_ready = "maybe"
($badManifestProjectTemplateOnboardingInvalidReleaseReady | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingInvalidReleaseReadyPath -Encoding UTF8

$badManifestProjectTemplateOnboardingInvalidReleaseReadyFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingInvalidReleaseReadyPath
} catch {
    $badManifestProjectTemplateOnboardingInvalidReleaseReadyFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingInvalidReleaseReadyFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid project template onboarding release_ready."
}

$badManifestMissingNumberingConfidenceDir = Join-Path $failDir "manifest-missing-numbering-confidence"
$badManifestMissingNumberingConfidencePath = Join-Path $badManifestMissingNumberingConfidenceDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingNumberingConfidenceDir -Force | Out-Null
$badManifestMissingNumberingConfidence = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestMissingNumberingConfidence | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingNumberingConfidencePath -Encoding UTF8

$missingManifestNumberingConfidenceFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingNumberingConfidencePath
} catch {
    $missingManifestNumberingConfidenceFailedAsExpected = $true
}

if (-not $missingManifestNumberingConfidenceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without numbering_catalog_real_corpus_confidence."
}

$badManifestNumberingConfidenceMismatchDir = Join-Path $failDir "manifest-bad-numbering-confidence"
$badManifestNumberingConfidenceMismatchPath = Join-Path $badManifestNumberingConfidenceMismatchDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestNumberingConfidenceMismatchDir -Force | Out-Null
$badManifestNumberingConfidenceMismatch = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = [ordered]@{
        id = "numbering_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "numbering_catalog_governance"
        source_schema = "featherdoc.numbering_catalog_governance_report.v1"
        source_report = $governanceMetrics[1].source_report
        source_report_display = $governanceMetrics[1].source_report_display
        source_json = $governanceMetrics[1].source_json
        source_json_display = $governanceMetrics[1].source_json_display
        score = 55
        level = "low"
        details = $governanceMetrics[1].details
    }
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestNumberingConfidenceMismatch | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestNumberingConfidenceMismatchPath -Encoding UTF8

$badManifestNumberingConfidenceMismatchFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestNumberingConfidenceMismatchPath
} catch {
    $badManifestNumberingConfidenceMismatchFailedAsExpected = $true
}

if (-not $badManifestNumberingConfidenceMismatchFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched numbering_catalog_real_corpus_confidence."
}

$badManifestStyleAsNumberingConfidenceDir = Join-Path $failDir "manifest-style-as-numbering-confidence"
$badManifestStyleAsNumberingConfidencePath = Join-Path $badManifestStyleAsNumberingConfidenceDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestStyleAsNumberingConfidenceDir -Force | Out-Null
$badManifestStyleAsNumberingConfidence = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = [ordered]@{
        id = "style_catalog_governance.real_corpus_confidence"
        metric = "real_corpus_confidence"
        report_id = "style_catalog_governance"
        source_schema = "featherdoc.style_catalog_governance_report.v1"
        source_report = $governanceMetrics[1].source_report
        source_report_display = $governanceMetrics[1].source_report_display
        source_json = $governanceMetrics[1].source_json
        source_json_display = $governanceMetrics[1].source_json_display
        score = 12
        level = "experimental"
        details = $governanceMetrics[1].details
    }
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestStyleAsNumberingConfidence | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestStyleAsNumberingConfidencePath -Encoding UTF8

$badManifestStyleAsNumberingConfidenceFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestStyleAsNumberingConfidencePath
} catch {
    $badManifestStyleAsNumberingConfidenceFailedAsExpected = $true
}

if (-not $badManifestStyleAsNumberingConfidenceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with style_catalog real corpus confidence mirrored as numbering."
}

$badManifestMissingTableLayoutQualityDir = Join-Path $failDir "manifest-missing-table-layout-quality"
$badManifestMissingTableLayoutQualityPath = Join-Path $badManifestMissingTableLayoutQualityDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingTableLayoutQualityDir -Force | Out-Null
$badManifestMissingTableLayoutQuality = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestMissingTableLayoutQuality | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingTableLayoutQualityPath -Encoding UTF8

$missingManifestTableLayoutQualityFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingTableLayoutQualityPath
} catch {
    $missingManifestTableLayoutQualityFailedAsExpected = $true
}

if (-not $missingManifestTableLayoutQualityFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without table_layout_delivery_quality."
}

$badManifestTableLayoutQualityMismatchDir = Join-Path $failDir "manifest-bad-table-layout-quality"
$badManifestTableLayoutQualityMismatchPath = Join-Path $badManifestTableLayoutQualityMismatchDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestTableLayoutQualityMismatchDir -Force | Out-Null
$badManifestTableLayoutQualityMismatch = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = [ordered]@{
        id = "table_layout_delivery_governance.delivery_quality"
        metric = "delivery_quality"
        report_id = "table_layout_delivery_governance"
        source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
        source_report = $governanceMetrics[2].source_report
        source_report_display = $governanceMetrics[2].source_report_display
        source_json = $governanceMetrics[2].source_json
        source_json_display = $governanceMetrics[2].source_json_display
        score = 90
        level = "release_ready"
        details = $governanceMetrics[2].details
    }
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestTableLayoutQualityMismatch | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestTableLayoutQualityMismatchPath -Encoding UTF8

$badManifestTableLayoutQualityMismatchFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestTableLayoutQualityMismatchPath
} catch {
    $badManifestTableLayoutQualityMismatchFailedAsExpected = $true
}

if (-not $badManifestTableLayoutQualityMismatchFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched table_layout_delivery_quality."
}

$badManifestMissingProjectTemplateReadinessDir = Join-Path $failDir "manifest-missing-project-template-readiness"
$badManifestMissingProjectTemplateReadinessPath = Join-Path $badManifestMissingProjectTemplateReadinessDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateReadinessDir -Force | Out-Null
$badManifestMissingProjectTemplateReadiness = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestMissingProjectTemplateReadiness | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingProjectTemplateReadinessPath -Encoding UTF8

$badManifestMissingProjectTemplateReadinessFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateReadinessPath
} catch {
    $badManifestMissingProjectTemplateReadinessFailedAsExpected = $true
}

if (-not $badManifestMissingProjectTemplateReadinessFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without project_template_delivery_readiness_contract."
}

$badManifestBlockedProjectTemplateReadinessDir = Join-Path $failDir "manifest-bad-project-template-readiness"
$badManifestBlockedProjectTemplateReadinessPath = Join-Path $badManifestBlockedProjectTemplateReadinessDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestBlockedProjectTemplateReadinessDir -Force | Out-Null
$badManifestBlockedProjectTemplateReadiness = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = [ordered]@{
        schema = "featherdoc.project_template_delivery_readiness_report.v1"
        source_schema = "featherdoc.project_template_delivery_readiness_report.v1"
        status = "blocked"
        release_ready = $false
        latest_schema_approval_gate_status = "blocked"
        schema_approval_status_summary = @(
            [ordered]@{
                status = "blocked"
                count = 1
            }
        )
        schema_history_blocked_run_count = 1
        schema_history_pending_run_count = 0
        schema_history_passed_run_count = 2
        template_count = 4
        ready_template_count = 3
        blocked_template_count = 1
        release_blocker_count = 0
        action_item_count = 1
        warning_count = 0
        source_report_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
        source_json_display = ".\output\release-candidate-checks\report\project_template_delivery_readiness_summary.json"
    }
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
}
($badManifestBlockedProjectTemplateReadiness | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestBlockedProjectTemplateReadinessPath -Encoding UTF8

$badManifestBlockedProjectTemplateReadinessFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestBlockedProjectTemplateReadinessPath
} catch {
    $badManifestBlockedProjectTemplateReadinessFailedAsExpected = $true
}

if (-not $badManifestBlockedProjectTemplateReadinessFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with release_ready=false and no blocker or warning."
}

$badManifestMissingProjectTemplateOnboardingDir = Join-Path $failDir "manifest-missing-project-template-onboarding"
$badManifestMissingProjectTemplateOnboardingPath = Join-Path $badManifestMissingProjectTemplateOnboardingDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateOnboardingDir -Force | Out-Null
$badManifestMissingProjectTemplateOnboarding = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
}
($badManifestMissingProjectTemplateOnboarding | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestMissingProjectTemplateOnboardingPath -Encoding UTF8

$badManifestMissingProjectTemplateOnboardingFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateOnboardingPath
} catch {
    $badManifestMissingProjectTemplateOnboardingFailedAsExpected = $true
}

if (-not $badManifestMissingProjectTemplateOnboardingFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without project_template_onboarding_governance_contract."
}

$badManifestProjectTemplateOnboardingCountDir = Join-Path $failDir "manifest-bad-project-template-onboarding-count"
$badManifestProjectTemplateOnboardingCountPath = Join-Path $badManifestProjectTemplateOnboardingCountDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestProjectTemplateOnboardingCountDir -Force | Out-Null
$badManifestProjectTemplateOnboardingCount = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        [ordered]@{
            id = "content_control_data_binding.bound_placeholder"
            source_schema = "featherdoc.content_control_data_binding_governance_report.v1"
            source_json_display = ".\output\release-candidate-checks\report\content_control_data_binding_governance_summary.json"
            repair_strategy = "sync_bound_content_control"
            repair_hint = "Rerun Custom XML sync or explicitly fill the bound content control before release."
            command_template = "featherdoc_cli sync-content-controls-from-custom-xml <input.docx> --output <synced.docx> --json"
        }
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = [ordered]@{
        schema = "featherdoc.project_template_onboarding_governance_report.v1"
        source_schema = "featherdoc.project_template_onboarding_governance_report.v1"
        status = "ready"
        release_ready = $true
        source_file_count = 3
        source_failure_count = 0
        entry_count = 4
        schema_approval_status_summary = @(
            [ordered]@{
                status = "approved"
                count = 2
            }
        )
        blocked_entry_count = 0
        pending_review_entry_count = 0
        not_evaluated_entry_count = 0
        approved_entry_count = 2
        not_required_entry_count = 1
        release_blocker_count = 0
        action_item_count = 0
        manual_review_recommendation_count = 1
        source_report_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
        source_json_display = ".\output\release-candidate-checks\report\project_template_onboarding_governance_summary.json"
    }
}
($badManifestProjectTemplateOnboardingCount | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $badManifestProjectTemplateOnboardingCountPath -Encoding UTF8

$badManifestProjectTemplateOnboardingCountFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestProjectTemplateOnboardingCountPath
} catch {
    $badManifestProjectTemplateOnboardingCountFailedAsExpected = $true
}

if (-not $badManifestProjectTemplateOnboardingCountFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched project_template_onboarding_governance entry counts."
}
