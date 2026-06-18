$passSummary = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    release_handoff = ".\output\release-candidate-checks\report\release_handoff.md"
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
}
($passSummary | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $passSummaryPath -Encoding UTF8

$passManifest = [ordered]@{
    release_version = "1.6.4"
    execution_status = "pass"
    visual_gate_status = "completed"
    visual_gate_evidence_included = $true
    pdf_visual_gate_status = "loaded"
    pdf_visual_gate_summary_json = $pdfVisualGateSummaryJson
    pdf_visual_gate_output_dir = ".\output\pdf-visual-release-gate-current"
    pdf_visual_gate_evidence_included = $true
    pdf_visual_gate_evidence = $pdfVisualGateEvidence
    word_visual_standard_review_metadata_count = 4
    word_visual_standard_review_metadata = $wordVisualStandardReviewMetadata
    governance_metric_count = $governanceMetricCount
    governance_metrics = $governanceMetrics
    numbering_catalog_real_corpus_confidence = (New-NumberingCatalogRealCorpusConfidenceMirror -GovernanceMetrics $governanceMetrics)
    table_layout_delivery_quality = (New-TableLayoutDeliveryQualityMirror -GovernanceMetrics $governanceMetrics)
    content_control_repair_contract_count = 1
    content_control_repair_contracts = @(
        (New-TestContentControlRepairContract)
    )
    project_template_delivery_readiness_contract = $projectTemplateDeliveryReadinessContract
    project_template_onboarding_governance_contract = $projectTemplateOnboardingGovernanceContract
    manifest_signoff_entrypoints = $manifestSignoffEntrypoints
    project_template_readiness_checklist_entrypoints = $projectTemplateReadinessChecklistEntrypoints
    release_note_bundle = $releaseNoteBundle
    release_entry_project_template_readiness_checklist_material_safety_audit = $releaseEntryProjectTemplateChecklistMaterialSafetyAudit
    upload = [ordered]@{
        requested_tag = "v1.6.4"
        uploaded = $true
        release_url = "https://github.example/releases/tag/v1.6.4"
        remote_assets = @(
            [ordered]@{
                name = "FeatherDoc-v1.6.4-msvc-install.zip"
                url = "https://github.example/releases/download/v1.6.4/FeatherDoc-v1.6.4-msvc-install.zip"
                size_bytes = 1234
                download_count = 2
            }
            [ordered]@{
                name = "FeatherDoc-v1.6.4-visual-validation-gallery.zip"
                url = "https://github.example/releases/download/v1.6.4/FeatherDoc-v1.6.4-visual-validation-gallery.zip"
                size_bytes = 5678
                download_count = 3
            }
            [ordered]@{
                name = "FeatherDoc-v1.6.4-release-evidence.zip"
                url = "https://github.example/releases/download/v1.6.4/FeatherDoc-v1.6.4-release-evidence.zip"
                size_bytes = 9012
                download_count = 4
            }
        )
    }
}
($passManifest | ConvertTo-Json -Depth 8) | Set-Content -LiteralPath $passManifestPath -Encoding UTF8

& $auditScript -Path @($passSummaryPath, $passManifestPath)

$passManifestWarningOnlyReadinessDir = Join-Path $passDir "manifest-project-template-readiness-warning-only"
$passManifestWarningOnlyReadinessPath = Join-Path $passManifestWarningOnlyReadinessDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $passManifestWarningOnlyReadinessDir -Force | Out-Null
$passManifestWarningOnlyReadiness = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$passManifestWarningOnlyReadiness.project_template_delivery_readiness_contract.status = "needs_review"
$passManifestWarningOnlyReadiness.project_template_delivery_readiness_contract.release_ready = $false
$passManifestWarningOnlyReadiness.project_template_delivery_readiness_contract.release_blocker_count = 0
$passManifestWarningOnlyReadiness.project_template_delivery_readiness_contract.warning_count = 1
($passManifestWarningOnlyReadiness | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $passManifestWarningOnlyReadinessPath -Encoding UTF8
& $auditScript -Path $passManifestWarningOnlyReadinessPath

function Assert-ManifestContractFieldRequired {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ContractName,

        [Parameter(Mandatory = $true)]
        [string]$FieldName,

        [Parameter(Mandatory = $true)]
        [string]$CaseSlug
    )

    $caseDir = Join-Path $failDir $CaseSlug
    $casePath = Join-Path $caseDir "release_assets_manifest.json"
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
    $caseManifest = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
    $contract = $caseManifest.PSObject.Properties[$ContractName].Value
    $contract.PSObject.Properties.Remove($FieldName)
    ($caseManifest | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $casePath -Encoding UTF8

    $failedAsExpected = $false
    try {
        & $auditScript -Path $casePath
    } catch {
        $failedAsExpected = $true
    }

    if (-not $failedAsExpected) {
        throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing $ContractName.$FieldName."
    }
}

function Assert-ReleaseUploadRemoteAssetsCaseFails {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CaseSlug,

        [Parameter(Mandatory = $true)]
        [scriptblock]$Mutate
    )

    $caseDir = Join-Path $failDir $CaseSlug
    $casePath = Join-Path $caseDir "release_assets_manifest.json"
    New-Item -ItemType Directory -Path $caseDir -Force | Out-Null
    $caseManifest = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
    & $Mutate $caseManifest
    ($caseManifest | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $casePath -Encoding UTF8

    $failedAsExpected = $false
    try {
        & $auditScript -Path $casePath
    } catch {
        $failedAsExpected = $true
    }

    if (-not $failedAsExpected) {
        throw "assert_release_material_safety.ps1 unexpectedly passed release manifest upload remote-assets case '$CaseSlug'."
    }
}

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-includes-manifest" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets += [pscustomobject]@{
            name = "release_assets_manifest.json"
            url = "https://github.example/assets/release-assets-manifest"
            size_bytes = 345
            download_count = 1
        }
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-missing-official-zip" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets = @(
            $Manifest.upload.remote_assets |
                Where-Object { [string]$_.name -ne "FeatherDoc-v1.6.4-release-evidence.zip" }
        )
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-missing-download-count" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets[0].PSObject.Properties.Remove("download_count")
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-invalid-size" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets[0].size_bytes = 0
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-url-points-to-wrong-file" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets[0].url = "https://github.example/releases/download/v1.6.4/FeatherDoc-v1.6.4-release-evidence.zip"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-url-points-to-wrong-tag" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets[0].url = "https://github.example/releases/download/v1.6.3/FeatherDoc-v1.6.4-msvc-install.zip"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-url-file-name-only-in-query" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets[0].url = "https://github.example/releases/download/v1.6.4/FeatherDoc-v1.6.4-release-evidence.zip?asset=FeatherDoc-v1.6.4-msvc-install.zip"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-url-uses-non-release-download-path" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets[0].url = "https://github.example/assets/v1.6.4/FeatherDoc-v1.6.4-msvc-install.zip"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-url-uses-different-host" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets[0].url = "https://cdn.github.example/releases/download/v1.6.4/FeatherDoc-v1.6.4-msvc-install.zip"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-remote-assets-url-uses-different-release-path-prefix" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.remote_assets[0].url = "https://github.example/other-project/releases/download/v1.6.4/FeatherDoc-v1.6.4-msvc-install.zip"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-release-url-uses-non-http-url" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.release_url = "file:///release/v1.6.4"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-release-url-points-to-wrong-tag" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.release_url = "https://github.example/releases/tag/v1.6.3"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-release-url-tag-only-in-query" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.release_url = "https://github.example/releases/tag/v1.6.3?tag=v1.6.4"
    }

Assert-ReleaseUploadRemoteAssetsCaseFails `
    -CaseSlug "manifest-upload-release-url-uses-non-release-tag-path" `
    -Mutate {
        param($Manifest)
        $Manifest.upload.release_url = "https://github.example/assets/v1.6.4"
    }

$badManifestMissingDeliveryReadinessNextActionDir = Join-Path $failDir "manifest-missing-project-template-delivery-readiness-next-action"
$badManifestMissingDeliveryReadinessNextActionPath = Join-Path $badManifestMissingDeliveryReadinessNextActionDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingDeliveryReadinessNextActionDir -Force | Out-Null
$badManifestMissingDeliveryReadinessNextAction = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingDeliveryReadinessNextAction.project_template_delivery_readiness_contract.PSObject.Properties.Remove("onboarding_governance_next_action")
($badManifestMissingDeliveryReadinessNextAction | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingDeliveryReadinessNextActionPath -Encoding UTF8

$missingDeliveryReadinessNextActionFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingDeliveryReadinessNextActionPath
} catch {
    $missingDeliveryReadinessNextActionFailedAsExpected = $true
}

if (-not $missingDeliveryReadinessNextActionFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing project_template_delivery_readiness_contract.onboarding_governance_next_action."
}

$badManifestMissingDeliveryReadinessReviewerActionDir = Join-Path $failDir "manifest-missing-project-template-delivery-readiness-reviewer-action"
$badManifestMissingDeliveryReadinessReviewerActionPath = Join-Path $badManifestMissingDeliveryReadinessReviewerActionDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingDeliveryReadinessReviewerActionDir -Force | Out-Null
$badManifestMissingDeliveryReadinessReviewerAction = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingDeliveryReadinessReviewerAction.project_template_delivery_readiness_contract.PSObject.Properties.Remove("reviewer_action_summary")
($badManifestMissingDeliveryReadinessReviewerAction | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingDeliveryReadinessReviewerActionPath -Encoding UTF8

$missingDeliveryReadinessReviewerActionFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingDeliveryReadinessReviewerActionPath
} catch {
    $missingDeliveryReadinessReviewerActionFailedAsExpected = $true
}

if (-not $missingDeliveryReadinessReviewerActionFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing project_template_delivery_readiness_contract.reviewer_action_summary."
}

Assert-ManifestContractFieldRequired `
    -ContractName "project_template_delivery_readiness_contract" `
    -FieldName "reviewer_action_reason" `
    -CaseSlug "manifest-missing-project-template-delivery-readiness-reviewer-action-reason"

Assert-ManifestContractFieldRequired `
    -ContractName "project_template_delivery_readiness_contract" `
    -FieldName "reviewer_actions" `
    -CaseSlug "manifest-missing-project-template-delivery-readiness-reviewer-actions"

$badManifestMissingOnboardingGovernanceNextActionSummaryDir = Join-Path $failDir "manifest-missing-project-template-onboarding-governance-next-action-summary"
$badManifestMissingOnboardingGovernanceNextActionSummaryPath = Join-Path $badManifestMissingOnboardingGovernanceNextActionSummaryDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingOnboardingGovernanceNextActionSummaryDir -Force | Out-Null
$badManifestMissingOnboardingGovernanceNextActionSummary = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingOnboardingGovernanceNextActionSummary.project_template_onboarding_governance_contract.PSObject.Properties.Remove("next_action_summary")
($badManifestMissingOnboardingGovernanceNextActionSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingOnboardingGovernanceNextActionSummaryPath -Encoding UTF8

$missingOnboardingGovernanceNextActionSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingOnboardingGovernanceNextActionSummaryPath
} catch {
    $missingOnboardingGovernanceNextActionSummaryFailedAsExpected = $true
}

if (-not $missingOnboardingGovernanceNextActionSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing project_template_onboarding_governance_contract.next_action_summary."
}

$badManifestMissingOnboardingGovernanceReviewerActionDir = Join-Path $failDir "manifest-missing-project-template-onboarding-governance-reviewer-action"
$badManifestMissingOnboardingGovernanceReviewerActionPath = Join-Path $badManifestMissingOnboardingGovernanceReviewerActionDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingOnboardingGovernanceReviewerActionDir -Force | Out-Null
$badManifestMissingOnboardingGovernanceReviewerAction = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingOnboardingGovernanceReviewerAction.project_template_onboarding_governance_contract.PSObject.Properties.Remove("reviewer_action_summary")
($badManifestMissingOnboardingGovernanceReviewerAction | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingOnboardingGovernanceReviewerActionPath -Encoding UTF8

$missingOnboardingGovernanceReviewerActionFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingOnboardingGovernanceReviewerActionPath
} catch {
    $missingOnboardingGovernanceReviewerActionFailedAsExpected = $true
}

if (-not $missingOnboardingGovernanceReviewerActionFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing project_template_onboarding_governance_contract.reviewer_action_summary."
}

Assert-ManifestContractFieldRequired `
    -ContractName "project_template_onboarding_governance_contract" `
    -FieldName "reviewer_action_reason" `
    -CaseSlug "manifest-missing-project-template-onboarding-governance-reviewer-action-reason"

Assert-ManifestContractFieldRequired `
    -ContractName "project_template_onboarding_governance_contract" `
    -FieldName "reviewer_actions" `
    -CaseSlug "manifest-missing-project-template-onboarding-governance-reviewer-actions"

$badManifestMissingProjectTemplateChecklistEntrypointsDir = Join-Path $failDir "manifest-missing-project-template-readiness-checklist-entrypoints"
$badManifestMissingProjectTemplateChecklistEntrypointsPath = Join-Path $badManifestMissingProjectTemplateChecklistEntrypointsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateChecklistEntrypointsDir -Force | Out-Null
$badManifestMissingProjectTemplateChecklistEntrypoints = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingProjectTemplateChecklistEntrypoints.PSObject.Properties.Remove("project_template_readiness_checklist_entrypoints")
($badManifestMissingProjectTemplateChecklistEntrypoints | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingProjectTemplateChecklistEntrypointsPath -Encoding UTF8

$missingProjectTemplateChecklistEntrypointsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateChecklistEntrypointsPath
} catch {
    $missingProjectTemplateChecklistEntrypointsFailedAsExpected = $true
}

if (-not $missingProjectTemplateChecklistEntrypointsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing project_template_readiness_checklist_entrypoints."
}

$badManifestMissingReleaseNoteBundleDir = Join-Path $failDir "manifest-missing-release-note-bundle"
$badManifestMissingReleaseNoteBundlePath = Join-Path $badManifestMissingReleaseNoteBundleDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingReleaseNoteBundleDir -Force | Out-Null
$badManifestMissingReleaseNoteBundle = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingReleaseNoteBundle.PSObject.Properties.Remove("release_note_bundle")
($badManifestMissingReleaseNoteBundle | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingReleaseNoteBundlePath -Encoding UTF8

$missingReleaseNoteBundleFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingReleaseNoteBundlePath
} catch {
    $missingReleaseNoteBundleFailedAsExpected = $true
}

if (-not $missingReleaseNoteBundleFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing release_note_bundle."
}

$badManifestWrongReleaseNoteBundleLocationDir = Join-Path $failDir "manifest-wrong-release-note-bundle-location"
$badManifestWrongReleaseNoteBundleLocationPath = Join-Path $badManifestWrongReleaseNoteBundleLocationDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestWrongReleaseNoteBundleLocationDir -Force | Out-Null
$badManifestWrongReleaseNoteBundleLocation = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestWrongReleaseNoteBundleLocation.release_note_bundle.entrypoints[0].location = "report"
($badManifestWrongReleaseNoteBundleLocation | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestWrongReleaseNoteBundleLocationPath -Encoding UTF8

$wrongReleaseNoteBundleLocationFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestWrongReleaseNoteBundleLocationPath
} catch {
    $wrongReleaseNoteBundleLocationFailedAsExpected = $true
}

if (-not $wrongReleaseNoteBundleLocationFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with release_note_bundle start_here outside summary_root."
}

$badManifestExtraReleaseNoteBundleEntrypointDir = Join-Path $failDir "manifest-extra-release-note-bundle-entrypoint"
$badManifestExtraReleaseNoteBundleEntrypointPath = Join-Path $badManifestExtraReleaseNoteBundleEntrypointDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestExtraReleaseNoteBundleEntrypointDir -Force | Out-Null
$badManifestExtraReleaseNoteBundleEntrypoint = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$extraReleaseNoteBundleEntrypoint = $badManifestExtraReleaseNoteBundleEntrypoint.release_note_bundle.entrypoints[0] | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$extraReleaseNoteBundleEntrypoint.id = "extra_release_note"
$extraReleaseNoteBundleEntrypoint.location = "report"
$badManifestExtraReleaseNoteBundleEntrypoint.release_note_bundle.entrypoint_ids += "extra_release_note"
$badManifestExtraReleaseNoteBundleEntrypoint.release_note_bundle.entrypoints += $extraReleaseNoteBundleEntrypoint
($badManifestExtraReleaseNoteBundleEntrypoint | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestExtraReleaseNoteBundleEntrypointPath -Encoding UTF8

$extraReleaseNoteBundleEntrypointFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestExtraReleaseNoteBundleEntrypointPath
} catch {
    $extraReleaseNoteBundleEntrypointFailedAsExpected = $true
}

if (-not $extraReleaseNoteBundleEntrypointFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with extra release_note_bundle entrypoint."
}

$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditDir = Join-Path $failDir "manifest-missing-project-template-checklist-material-safety-audit"
$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditPath = Join-Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditDir -Force | Out-Null
$badManifestMissingProjectTemplateChecklistMaterialSafetyAudit = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingProjectTemplateChecklistMaterialSafetyAudit.PSObject.Properties.Remove("release_entry_project_template_readiness_checklist_material_safety_audit")
($badManifestMissingProjectTemplateChecklistMaterialSafetyAudit | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditPath -Encoding UTF8

$missingProjectTemplateChecklistMaterialSafetyAuditFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditPath
} catch {
    $missingProjectTemplateChecklistMaterialSafetyAuditFailedAsExpected = $true
}

if (-not $missingProjectTemplateChecklistMaterialSafetyAuditFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing release_entry_project_template_readiness_checklist_material_safety_audit."
}

$badManifestMissingWordVisualReviewMetadataDir = Join-Path $failDir "manifest-missing-word-visual-standard-review-metadata"
$badManifestMissingWordVisualReviewMetadataPath = Join-Path $badManifestMissingWordVisualReviewMetadataDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingWordVisualReviewMetadataDir -Force | Out-Null
$badManifestMissingWordVisualReviewMetadata = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingWordVisualReviewMetadata.PSObject.Properties.Remove("word_visual_standard_review_metadata")
($badManifestMissingWordVisualReviewMetadata | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingWordVisualReviewMetadataPath -Encoding UTF8

$missingWordVisualReviewMetadataFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingWordVisualReviewMetadataPath
} catch {
    $missingWordVisualReviewMetadataFailedAsExpected = $true
}

if (-not $missingWordVisualReviewMetadataFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing word_visual_standard_review_metadata."
}

$badManifestWordVisualReviewNoteDir = Join-Path $failDir "manifest-word-visual-standard-review-note"
$badManifestWordVisualReviewNotePath = Join-Path $badManifestWordVisualReviewNoteDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestWordVisualReviewNoteDir -Force | Out-Null
$badManifestWordVisualReviewNote = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestWordVisualReviewNote.word_visual_standard_review_metadata[0] | Add-Member -NotePropertyName "review_note" -NotePropertyValue "Manual private note"
($badManifestWordVisualReviewNote | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestWordVisualReviewNotePath -Encoding UTF8

$wordVisualReviewNoteFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestWordVisualReviewNotePath
} catch {
    $wordVisualReviewNoteFailedAsExpected = $true
}

if (-not $wordVisualReviewNoteFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest exposing word_visual_standard_review_metadata.review_note."
}

$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir = Join-Path $failDir "manifest-missing-project-template-checklist-material-safety-audit-source-schema"
$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath = Join-Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir -Force | Out-Null
$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchema.release_entry_project_template_readiness_checklist_material_safety_audit.PSObject.Properties.Remove("compact_evidence_source_schema")
($badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath -Encoding UTF8

$missingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath
} catch {
    $missingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected = $true
}

if (-not $missingProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing release_entry_project_template_readiness_checklist_material_safety_audit.compact_evidence_source_schema."
}

$badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir = Join-Path $failDir "manifest-wrong-project-template-checklist-material-safety-audit-source-schema"
$badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath = Join-Path $badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaDir -Force | Out-Null
$badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchema = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchema.release_entry_project_template_readiness_checklist_material_safety_audit.compact_evidence_source_schema = "featherdoc.release_blocker_rollup_report.v1"
($badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchema | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath -Encoding UTF8

$wrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestWrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaPath
} catch {
    $wrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected = $true
}

if (-not $wrongProjectTemplateChecklistMaterialSafetyAuditSourceSchemaFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with release_entry_project_template_readiness_checklist_material_safety_audit.compact_evidence_source_schema pointing at the wrong schema."
}

$badManifestMissingSignoffDir = Join-Path $failDir "manifest-missing-signoff-entrypoints"
$badManifestMissingSignoffPath = Join-Path $badManifestMissingSignoffDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingSignoffDir -Force | Out-Null
$badManifestMissingSignoff = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingSignoff.PSObject.Properties.Remove("manifest_signoff_entrypoints")
($badManifestMissingSignoff | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingSignoffPath -Encoding UTF8

$missingSignoffFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingSignoffPath
} catch {
    $missingSignoffFailedAsExpected = $true
}

if (-not $missingSignoffFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing manifest_signoff_entrypoints."
}

$badManifestMissingSignoffReviewerDir = Join-Path $failDir "manifest-missing-signoff-reviewer-checklist"
$badManifestMissingSignoffReviewerPath = Join-Path $badManifestMissingSignoffReviewerDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingSignoffReviewerDir -Force | Out-Null
$badManifestMissingSignoffReviewer = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingSignoffReviewer.manifest_signoff_entrypoints.entrypoints = @(
    $badManifestMissingSignoffReviewer.manifest_signoff_entrypoints.entrypoints |
        Where-Object { [string]$_.id -ne "reviewer_checklist" }
)
($badManifestMissingSignoffReviewer | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingSignoffReviewerPath -Encoding UTF8

$missingSignoffReviewerFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingSignoffReviewerPath
} catch {
    $missingSignoffReviewerFailedAsExpected = $true
}

if (-not $missingSignoffReviewerFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing reviewer_checklist signoff entrypoint."
}

$badManifestMissingSignoffFieldDir = Join-Path $failDir "manifest-missing-signoff-source-json-field"
$badManifestMissingSignoffFieldPath = Join-Path $badManifestMissingSignoffFieldDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingSignoffFieldDir -Force | Out-Null
$badManifestMissingSignoffField = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingSignoffField.manifest_signoff_entrypoints.required_fields = @(
    $badManifestMissingSignoffField.manifest_signoff_entrypoints.required_fields |
        Where-Object { [string]$_ -ne "source_json_display" }
)
($badManifestMissingSignoffField | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingSignoffFieldPath -Encoding UTF8

$missingSignoffFieldFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingSignoffFieldPath
} catch {
    $missingSignoffFieldFailedAsExpected = $true
}

if (-not $missingSignoffFieldFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest missing source_json_display signoff field."
}

$badManifestMissingPdfEvidenceDir = Join-Path $failDir "manifest-missing-pdf-visual-evidence"
$badManifestMissingPdfEvidencePath = Join-Path $badManifestMissingPdfEvidenceDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingPdfEvidenceDir -Force | Out-Null
$badManifestMissingPdfEvidence = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingPdfEvidence.PSObject.Properties.Remove("pdf_visual_gate_evidence")
($badManifestMissingPdfEvidence | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingPdfEvidencePath -Encoding UTF8

$missingPdfEvidenceFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingPdfEvidencePath
} catch {
    $missingPdfEvidenceFailedAsExpected = $true
}

if (-not $missingPdfEvidenceFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without PDF visual gate evidence."
}

$badManifestMissingPdfVerdictDir = Join-Path $failDir "manifest-missing-pdf-visual-verdict"
$badManifestMissingPdfVerdictPath = Join-Path $badManifestMissingPdfVerdictDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingPdfVerdictDir -Force | Out-Null
$badManifestMissingPdfVerdict = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingPdfVerdict.pdf_visual_gate_evidence.PSObject.Properties.Remove("verdict")
($badManifestMissingPdfVerdict | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingPdfVerdictPath -Encoding UTF8

$missingPdfVerdictFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingPdfVerdictPath
} catch {
    $missingPdfVerdictFailedAsExpected = $true
}

if (-not $missingPdfVerdictFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without PDF visual gate verdict."
}

$badManifestInvalidPdfVerdictDir = Join-Path $failDir "manifest-invalid-pdf-visual-verdict"
$badManifestInvalidPdfVerdictPath = Join-Path $badManifestInvalidPdfVerdictDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestInvalidPdfVerdictDir -Force | Out-Null
$badManifestInvalidPdfVerdict = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestInvalidPdfVerdict.pdf_visual_gate_evidence.verdict = "blocked"
($badManifestInvalidPdfVerdict | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestInvalidPdfVerdictPath -Encoding UTF8

$invalidPdfVerdictFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestInvalidPdfVerdictPath
} catch {
    $invalidPdfVerdictFailedAsExpected = $true
}

if (-not $invalidPdfVerdictFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with invalid PDF visual gate verdict."
}

$badManifestMismatchedFullPdfStatusDir = Join-Path $failDir "manifest-mismatched-full-pdf-visual-status"
$badManifestMismatchedFullPdfStatusPath = Join-Path $badManifestMismatchedFullPdfStatusDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMismatchedFullPdfStatusDir -Force | Out-Null
$badManifestMismatchedFullPdfStatus = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMismatchedFullPdfStatus.pdf_visual_gate_evidence.full_visual_gate_status = "fail"
($badManifestMismatchedFullPdfStatus | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMismatchedFullPdfStatusPath -Encoding UTF8

$mismatchedFullPdfStatusFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMismatchedFullPdfStatusPath
} catch {
    $mismatchedFullPdfStatusFailedAsExpected = $true
}

if (-not $mismatchedFullPdfStatusFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with PDF full visual status diverging from verdict."
}

$badManifestMismatchedPdfSummaryDir = Join-Path $failDir "manifest-mismatched-pdf-visual-summary"
$badManifestMismatchedPdfSummaryPath = Join-Path $badManifestMismatchedPdfSummaryDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMismatchedPdfSummaryDir -Force | Out-Null
$badManifestMismatchedPdfSummary = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMismatchedPdfSummary.pdf_visual_gate_evidence.summary_json = ".\output\pdf-visual-release-gate-current\report\other-summary.json"
($badManifestMismatchedPdfSummary | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMismatchedPdfSummaryPath -Encoding UTF8

$mismatchedPdfSummaryFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMismatchedPdfSummaryPath
} catch {
    $mismatchedPdfSummaryFailedAsExpected = $true
}

if (-not $mismatchedPdfSummaryFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched PDF visual gate summary path."
}

$badManifestZeroPdfCountsDir = Join-Path $failDir "manifest-zero-pdf-visual-counts"
$badManifestZeroPdfCountsPath = Join-Path $badManifestZeroPdfCountsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestZeroPdfCountsDir -Force | Out-Null
$badManifestZeroPdfCounts = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestZeroPdfCounts.pdf_visual_gate_evidence.cjk_copy_search_count = "0"
$badManifestZeroPdfCounts.pdf_visual_gate_evidence.visual_baseline_count = "0"
($badManifestZeroPdfCounts | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestZeroPdfCountsPath -Encoding UTF8

$zeroPdfCountsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestZeroPdfCountsPath
} catch {
    $zeroPdfCountsFailedAsExpected = $true
}

if (-not $zeroPdfCountsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with zero PDF visual gate evidence counts."
}

$badManifestLowPdfManifestCountsDir = Join-Path $failDir "manifest-low-pdf-visual-manifest-counts"
$badManifestLowPdfManifestCountsPath = Join-Path $badManifestLowPdfManifestCountsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestLowPdfManifestCountsDir -Force | Out-Null
$badManifestLowPdfManifestCounts = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestLowPdfManifestCounts.pdf_visual_gate_evidence.cjk_manifest_count = "42"
$badManifestLowPdfManifestCounts.pdf_visual_gate_evidence.visual_baseline_manifest_count = "41"
($badManifestLowPdfManifestCounts | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestLowPdfManifestCountsPath -Encoding UTF8

$lowPdfManifestCountsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestLowPdfManifestCountsPath
} catch {
    $lowPdfManifestCountsFailedAsExpected = $true
}

if (-not $lowPdfManifestCountsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with low PDF visual manifest sample counts."
}

$badManifestPassingPdfMissingTextDir = Join-Path $failDir "manifest-passing-pdf-visual-missing-cjk-text"
$badManifestPassingPdfMissingTextPath = Join-Path $badManifestPassingPdfMissingTextDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestPassingPdfMissingTextDir -Force | Out-Null
$badManifestPassingPdfMissingText = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestPassingPdfMissingText.pdf_visual_gate_evidence.cjk_missing_text_count = "1"
($badManifestPassingPdfMissingText | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestPassingPdfMissingTextPath -Encoding UTF8

$passingPdfMissingTextFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestPassingPdfMissingTextPath
} catch {
    $passingPdfMissingTextFailedAsExpected = $true
}

if (-not $passingPdfMissingTextFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with verdict=pass and missing CJK text."
}

$badManifestMissingMetricDetailsDir = Join-Path $failDir "manifest-missing-governance-metric-details"
$badManifestMissingMetricDetailsPath = Join-Path $badManifestMissingMetricDetailsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingMetricDetailsDir -Force | Out-Null
$badManifestMissingMetricDetails = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingMetricDetails.numbering_catalog_real_corpus_confidence = [ordered]@{
    id = "numbering_catalog_governance.real_corpus_confidence"
    metric = "real_corpus_confidence"
    report_id = "numbering_catalog_governance"
    source_schema = "featherdoc.numbering_catalog_governance_report.v1"
    source_report = $governanceMetrics[1].source_report
    source_report_display = $governanceMetrics[1].source_report_display
    source_json = $governanceMetrics[1].source_json
    source_json_display = $governanceMetrics[1].source_json_display
    score = 56
    level = "low"
}
($badManifestMissingMetricDetails | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingMetricDetailsPath -Encoding UTF8

$missingManifestMetricDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingMetricDetailsPath
} catch {
    $missingManifestMetricDetailsFailedAsExpected = $true
}

if (-not $missingManifestMetricDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without numbering governance metric details."
}

$badManifestMismatchedMetricDetailsDir = Join-Path $failDir "manifest-mismatched-governance-metric-details"
$badManifestMismatchedMetricDetailsPath = Join-Path $badManifestMismatchedMetricDetailsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMismatchedMetricDetailsDir -Force | Out-Null
$badManifestMismatchedMetricDetails = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMismatchedMetricDetails.table_layout_delivery_quality = [ordered]@{
    id = "table_layout_delivery_governance.delivery_quality"
    metric = "delivery_quality"
    report_id = "table_layout_delivery_governance"
    source_schema = "featherdoc.table_layout_delivery_governance_report.v1"
    source_report = $governanceMetrics[2].source_report
    source_report_display = $governanceMetrics[2].source_report_display
    source_json = $governanceMetrics[2].source_json
    source_json_display = $governanceMetrics[2].source_json_display
    score = 100
    level = "release_ready"
    details = [ordered]@{
        score = 100
        level = "release_ready"
        document_count = 3
        ready_document_count = 3
        ready_document_percent = 100
        needs_review_document_count = 0
        failed_document_count = 0
        table_style_issue_count = 0
        automatic_tblLook_fix_count = 0
        manual_table_style_fix_count = 0
        table_position_automatic_count = 0
        table_position_review_count = 0
        command_failure_count = 0
        unresolved_item_count = 1
        metadata_only_fields = @("leftFromText", "rightFromText", "topFromText outside paragraph anchoring", "tblOverlap")
        review_required_fields = @("full Word-compatible floating table text wrapping", "table overlap avoidance and collision resolution")
        penalty_summary = @(
            [ordered]@{ factor = "needs_review_documents"; count = 0; penalty = 0 },
            [ordered]@{ factor = "floating_table_plans_pending"; count = 0; penalty = 0 }
        )
    }
}
($badManifestMismatchedMetricDetails | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMismatchedMetricDetailsPath -Encoding UTF8

$mismatchedManifestMetricDetailsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMismatchedMetricDetailsPath
} catch {
    $mismatchedManifestMetricDetailsFailedAsExpected = $true
}

if (-not $mismatchedManifestMetricDetailsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched table layout delivery details."
}

$badManifestMissingTableLayoutMetadataFieldsDir = Join-Path $failDir "manifest-missing-table-layout-metadata-fields"
$badManifestMissingTableLayoutMetadataFieldsPath = Join-Path $badManifestMissingTableLayoutMetadataFieldsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingTableLayoutMetadataFieldsDir -Force | Out-Null
$badManifestMissingTableLayoutMetadataFields = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingTableLayoutMetadataFields.table_layout_delivery_quality.details.PSObject.Properties.Remove("metadata_only_fields")
($badManifestMissingTableLayoutMetadataFields | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingTableLayoutMetadataFieldsPath -Encoding UTF8

$missingManifestTableLayoutMetadataFieldsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingTableLayoutMetadataFieldsPath
} catch {
    $missingManifestTableLayoutMetadataFieldsFailedAsExpected = $true
}

if (-not $missingManifestTableLayoutMetadataFieldsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without table layout metadata_only_fields details."
}

$badManifestMissingTableLayoutReviewFieldsDir = Join-Path $failDir "manifest-missing-table-layout-review-fields"
$badManifestMissingTableLayoutReviewFieldsPath = Join-Path $badManifestMissingTableLayoutReviewFieldsDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingTableLayoutReviewFieldsDir -Force | Out-Null
$badManifestMissingTableLayoutReviewFields = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingTableLayoutReviewFields.table_layout_delivery_quality.details.PSObject.Properties.Remove("review_required_fields")
($badManifestMissingTableLayoutReviewFields | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingTableLayoutReviewFieldsPath -Encoding UTF8

$missingManifestTableLayoutReviewFieldsFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingTableLayoutReviewFieldsPath
} catch {
    $missingManifestTableLayoutReviewFieldsFailedAsExpected = $true
}

if (-not $missingManifestTableLayoutReviewFieldsFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without table layout review_required_fields details."
}

$badManifestMissingNumberingConfidenceSourceJsonDisplayDir = Join-Path $failDir "manifest-missing-numbering-confidence-source-json-display"
$badManifestMissingNumberingConfidenceSourceJsonDisplayPath = Join-Path $badManifestMissingNumberingConfidenceSourceJsonDisplayDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestMissingNumberingConfidenceSourceJsonDisplayDir -Force | Out-Null
$badManifestMissingNumberingConfidenceSourceJsonDisplay = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestMissingNumberingConfidenceSourceJsonDisplay.numbering_catalog_real_corpus_confidence.PSObject.Properties.Remove("source_json_display")
($badManifestMissingNumberingConfidenceSourceJsonDisplay | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestMissingNumberingConfidenceSourceJsonDisplayPath -Encoding UTF8

$missingManifestNumberingConfidenceSourceJsonDisplayFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestMissingNumberingConfidenceSourceJsonDisplayPath
} catch {
    $missingManifestNumberingConfidenceSourceJsonDisplayFailedAsExpected = $true
}

if (-not $missingManifestNumberingConfidenceSourceJsonDisplayFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest without numbering_catalog_real_corpus_confidence.source_json_display."
}

$badManifestTableLayoutQualitySourceReportDisplayMismatchDir = Join-Path $failDir "manifest-bad-table-layout-quality-source-report-display"
$badManifestTableLayoutQualitySourceReportDisplayMismatchPath = Join-Path $badManifestTableLayoutQualitySourceReportDisplayMismatchDir "release_assets_manifest.json"
New-Item -ItemType Directory -Path $badManifestTableLayoutQualitySourceReportDisplayMismatchDir -Force | Out-Null
$badManifestTableLayoutQualitySourceReportDisplayMismatch = $passManifest | ConvertTo-Json -Depth 12 | ConvertFrom-Json
$badManifestTableLayoutQualitySourceReportDisplayMismatch.table_layout_delivery_quality.source_report_display = ".\output\numbering-catalog-governance\summary.json"
($badManifestTableLayoutQualitySourceReportDisplayMismatch | ConvertTo-Json -Depth 12) | Set-Content -LiteralPath $badManifestTableLayoutQualitySourceReportDisplayMismatchPath -Encoding UTF8

$badManifestTableLayoutQualitySourceReportDisplayMismatchFailedAsExpected = $false
try {
    & $auditScript -Path $badManifestTableLayoutQualitySourceReportDisplayMismatchPath
} catch {
    $badManifestTableLayoutQualitySourceReportDisplayMismatchFailedAsExpected = $true
}

if (-not $badManifestTableLayoutQualitySourceReportDisplayMismatchFailedAsExpected) {
    throw "assert_release_material_safety.ps1 unexpectedly passed release manifest with mismatched table_layout_delivery_quality.source_report_display."
}
