// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsSharedState.h"
#include "AcousticsPythonBridge.h"
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#include "Editor.h"
#include "HAL/PlatformFilemanager.h"
#include "EngineUtils.h"         // Needed for TActorIterator
#include "GameFramework/Actor.h" // Needed for AActor
#include "Runtime/Core/Public/HAL/FileManagerGeneric.h"
#include "ISourceControlProvider.h"
#include "ISourceControlModule.h"
#include "SourceControlOperations.h"
#include "SourceControlHelpers.h"
#include "Misc/MessageDialog.h"
#include "MathUtils.h"
#include "AcousticsShared.h"
#include "AcousticsEdMode.h"

UAcousticsPythonBridge* AcousticsSharedState::m_PythonBridge = nullptr;
TUniquePtr<AcousticsMaterialLibrary> AcousticsSharedState::m_MaterialLibrary;
TUniquePtr<AcousticsMaterialLibrary> AcousticsSharedState::m_KnownMaterialsLibrary;
TSharedPtr<AcousticsSimulationConfiguration> AcousticsSharedState::m_SimulationConfiguration;
TWeakObjectPtr<AAcousticsDebugRenderer> AcousticsSharedState::m_DebugRenderer;
// Track the total time taken for a bake.
FDateTime AcousticsSharedState::BakeStartTime(0);
FDateTime AcousticsSharedState::BakeEndTime(0);

void AcousticsSharedState::Initialize()
{
    // Only initialize the Python bridge the first time its created.
    if (m_PythonBridge == nullptr)
    {
        m_PythonBridge = UAcousticsPythonBridge::Get();

        // If python isn't initialized in the project, we won't be able to initialize the python bridge
        // Bail out early in this case
        if (m_PythonBridge == nullptr)
        {
            return;
        }
        m_PythonBridge->Initialize();

        InitializeProjectState();

        // Hook up delegate to initialize project state when the level changes
        FEditorDelegates::MapChange.AddLambda([](uint32 changeType) {
            if (changeType == MapChangeEventFlags::NewMap)
            {
                AcousticsSharedState::InitializeProjectState();
            }
        });
    }
}

void AcousticsSharedState::InitializeProjectState()
{
    // Set the content dir to default if not loaded from config
    auto config = GetProjectConfiguration();
    if (config.content_dir.IsEmpty())
    {
        FString pluginDir = FPaths::ProjectPluginsDir();
        config.content_dir = FPaths::Combine(pluginDir, TEXT("ProjectAcoustics"), TEXT("AcousticsData"));
    }
    SetProjectConfiguration(config);

    // Set the right prefix for the project.
    // This will first create the right prefix (if necessary) by checking current config.
    // Then set that prefix on the current config.
    auto prefix = GetConfigurationPrefixForLevel();
    SetConfigurationPrefixForLevel(prefix);

    if (!FPaths::DirectoryExists(config.content_dir))
    {
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        PlatformFile.CreateDirectory(*config.content_dir);
    }

    LoadSimulationConfigFromFile();
}

void AcousticsSharedState::LoadSimulationConfigFromFile()
{
    // Load config from existing vox file if one exists
    if (FPaths::FileExists(GetVoxFilepath()) && FPaths::FileExists(GetConfigFilepath()))
    {
        auto config =
            AcousticsSimulationConfiguration::Create(GetProjectConfiguration().content_dir, GetConfigFilename());
        SetSimulationConfiguration(MoveTemp(config));
    }
    else
    {
        SetSimulationConfiguration(nullptr);
    }
}

bool AcousticsSharedState::IsInitialized()
{
    return m_PythonBridge != nullptr;
}

void AcousticsSharedState::Destroy()
{
    m_MaterialLibrary.Reset();
    m_SimulationConfiguration.Reset();
    m_DebugRenderer.Reset();
    m_PythonBridge = nullptr;

    // Track the total time taken for a bake.
    BakeStartTime = FDateTime(0);
    BakeEndTime = FDateTime(0);
}

const FSimulationParameters& AcousticsSharedState::GetDefaultSimulationParameters()
{
    return m_PythonBridge->GetDefaultSimulationParameters();
}

const FSimulationParameters& AcousticsSharedState::GetSimulationParameters()
{
    return m_PythonBridge->GetSimulationParameters();
}

void AcousticsSharedState::SetSimulationParameters(const FSimulationParameters& params)
{
    m_PythonBridge->SetSimulationParameters(params);
}

const TritonSimulationParameters AcousticsSharedState::GetTritonSimulationParameters()
{
    TritonSimulationParameters params;

    // Translate to internal Triton types from loaded Python bridge settings
    const auto& simulation_config = m_PythonBridge->GetSimulationParameters();
    params.MeshUnitAdjustment = simulation_config.mesh_unit_adjustment;
    params.SimulationFrequency = simulation_config.max_frequency;
    params.SceneScale = 1.0f;
    params.SpeedOfSound = c_SpeedOfSound;
    params.ReceiverSampleSpacing = AcousticsUtils::UnrealValToTriton(simulation_config.receiver_spacing);
    auto tritonVectorFromUnreal = [](const FVector& uv) 
    { 
        return ATKVectorD{
            AcousticsUtils::UnrealValToTriton(uv.X),
            AcousticsUtils::UnrealValToTriton(uv.Y),
            AcousticsUtils::UnrealValToTriton(uv.Z)};
    };

    auto tritonBoxFromUnreal = [&tritonVectorFromUnreal](const FBox& ubox) {
        TritonBoundingBox box;
        box.MinCorner = tritonVectorFromUnreal(ubox.Min);
        box.MaxCorner = tritonVectorFromUnreal(ubox.Max);
        return box;
    };
    params.PerProbeSimulationRegion = tritonBoxFromUnreal(simulation_config.simulation_region);

    params.ProbeSpacing.MinHorizontalSpacing =
        AcousticsUtils::UnrealValToTriton(simulation_config.probe_spacing.horizontal_spacing_min);
    params.ProbeSpacing.MaxHorizontalSpacing =
        AcousticsUtils::UnrealValToTriton(simulation_config.probe_spacing.horizontal_spacing_max);
    params.ProbeSpacing.VerticalSpacing =
        AcousticsUtils::UnrealValToTriton(simulation_config.probe_spacing.vertical_spacing);
    params.ProbeSpacing.MinHeightAboveGround =
        AcousticsUtils::UnrealValToTriton(simulation_config.probe_spacing.min_height_above_ground);

    return params;
}

const FProjectConfiguration& AcousticsSharedState::GetProjectConfiguration()
{
    return m_PythonBridge->GetProjectConfiguration();
}

void AcousticsSharedState::SetProjectConfiguration(const FProjectConfiguration& config)
{
    m_PythonBridge->SetProjectConfiguration(config);
    // The prefix may have changed so reload the config.
    // This is quick as we're only readin existing files, not recomputing probes
    LoadSimulationConfigFromFile();
}

const TritonOperationalParameters AcousticsSharedState::GetTritonOperationalParameters()
{
    const auto& config = m_PythonBridge->GetProjectConfiguration();
    auto prefix = GetConfigurationPrefixForLevel();

    TritonOperationalParameters operationalParams;
    // There are lots of strings in this struct. Zero them all out before usage to prevent garbage strings
    memset(&operationalParams, 0, sizeof(TritonOperationalParameters));
    strncpy_s(operationalParams.Prefix, TRITON_MAX_NAME_LENGTH, TCHAR_TO_ANSI(*prefix), prefix.Len());
    strncpy_s(
        operationalParams.WorkingDir,
        TRITON_MAX_PATH_LENGTH,
        TCHAR_TO_ANSI(*config.content_dir),
        config.content_dir.Len());
    return operationalParams;
}

const AcousticsMaterialLibrary* AcousticsSharedState::GetMaterialsLibrary()
{
    return m_MaterialLibrary.Get();
}

void AcousticsSharedState::SetMaterialsLibrary(TUniquePtr<AcousticsMaterialLibrary> library)
{
    m_MaterialLibrary = MoveTemp(library);
}

const AcousticsMaterialLibrary* AcousticsSharedState::GetKnownMaterialsLibrary()
{
    return m_KnownMaterialsLibrary.Get();
}

void AcousticsSharedState::SetKnownMaterialsLibrary(TUniquePtr<AcousticsMaterialLibrary> library)
{
    m_KnownMaterialsLibrary = MoveTemp(library);
}

const AcousticsSimulationConfiguration* AcousticsSharedState::GetSimulationConfiguration()
{
    return m_SimulationConfiguration.Get();
}

// Whether a prebake is in progress, done, or failed
bool AcousticsSharedState::IsPrebakeActive()
{
    // Prebake is active if a sim config exists
    return GetSimulationConfiguration() != nullptr;
}

void AcousticsSharedState::SetSimulationConfiguration(TUniquePtr<AcousticsSimulationConfiguration> config)
{
    if (config)
    {
        // Take ownership of the passed configuration
        m_SimulationConfiguration = TSharedPtr<AcousticsSimulationConfiguration>(config.Release());
    }
    else
    {
        // Don't need the debug renderer anymore. Remove it
        if (m_DebugRenderer.IsValid())
        {
            // Remove the shared reference to SimConfig the DebugRenderer holds. Then SimConfig can be 
            // properly destroyed
            m_DebugRenderer->SetConfiguration(nullptr);
            m_DebugRenderer->Destroy();
        }
        m_SimulationConfiguration.Reset();

        return;
    }

    // Reset the config on the probes render
    // If we lost our pointer to the debug renderer, look for it in the world
    if (!m_DebugRenderer.IsValid())
    {
        for (TActorIterator<AActor> ActorItr(GEditor->GetEditorWorldContext().World()); ActorItr; ++ActorItr)
        {
            AActor* actor = *ActorItr;
            if (actor->IsA<AAcousticsDebugRenderer>())
            {
                m_DebugRenderer = TWeakObjectPtr<AAcousticsDebugRenderer>(Cast<AAcousticsDebugRenderer>(actor));
                break;
            }
        }
    }

    // If we didn't find it, recreate it
    if (!m_DebugRenderer.IsValid())
    {
        auto world = GEditor->GetEditorWorldContext().World();
        auto level = world->GetCurrentLevel();
        m_DebugRenderer = TWeakObjectPtr<AAcousticsDebugRenderer>(Cast<AAcousticsDebugRenderer>(
            GEditor->AddActor(level, AAcousticsDebugRenderer::StaticClass(), FTransform::Identity)));
    }

    m_DebugRenderer->SetConfiguration(m_SimulationConfiguration);
}

const FAzureCredentials& AcousticsSharedState::GetAzureCredentials()
{
    return m_PythonBridge->GetAzureCredentials();
}

void AcousticsSharedState::SetAzureCredentials(const FAzureCredentials& creds)
{
    m_PythonBridge->SetAzureCredentials(creds);
}

void AcousticsSharedState::SetComputePoolConfiguration(const FComputePoolConfiguration& pool)
{
    m_PythonBridge->SetComputePoolConfiguration(pool);
}

const FComputePoolConfiguration& AcousticsSharedState::GetComputePoolConfiguration()
{
    return m_PythonBridge->GetComputePoolConfiguration();
}

FString AcousticsSharedState::GetVoxFilepath()
{
    const auto& config = m_PythonBridge->GetProjectConfiguration();
    auto prefix = GetConfigurationPrefixForLevel();
    auto filename = prefix + FString(TEXT(".vox"));
    return FPaths::Combine(config.content_dir, filename);
}

FString AcousticsSharedState::GetConfigFilename()
{
    const auto& config = m_PythonBridge->GetProjectConfiguration();
    auto prefix = GetConfigurationPrefixForLevel();
    return prefix + FString(TEXT("_config.xml"));
}

FString AcousticsSharedState::GetConfigFilepath()
{
    const auto& config = m_PythonBridge->GetProjectConfiguration();
    auto filename = GetConfigFilename();
    return FPaths::Combine(config.content_dir, filename);
}

FString AcousticsSharedState::GetAceFilepath()
{
    const auto& config = m_PythonBridge->GetProjectConfiguration();
    auto prefix = GetConfigurationPrefixForLevel();
    auto filename = prefix + FString(TEXT(".ace"));
    return FPaths::Combine(config.game_content_dir, filename);
}

// Added function to get backup path for the ace file.
FString AcousticsSharedState::GetAceFileBackupPath()
{
    FString AceFileBackup = GetAceFilepath();
    int32 Index;
    AceFileBackup.FindLastChar('.', Index);
    AceFileBackup.InsertAt(Index, "_Backup");
    return AceFileBackup;
}

void AcousticsSharedState::SubmitForProcessing()
{
    // Nothing to do if the config is not ready
    if (!m_SimulationConfiguration.IsValid() || !m_SimulationConfiguration->IsReady())
    {
        return;
    }

    // Note: first set the job configuration before calling submit
    // Submit is overridden in Python and can't take any complex args due to a bug in UE4 reflection code
    FJobConfiguration job_config;
    job_config.config_file = GetConfigFilepath();
    job_config.vox_file = GetVoxFilepath();
    job_config.probe_count = m_SimulationConfiguration->GetProbeCount();
    job_config.prefix = GetConfigurationPrefixForLevel();
    m_PythonBridge->SetJobConfiguration(job_config);

    // Check out the ace if it exists so that it can be overwritten by the bake process.
    FString AceFilePath = GetAceFilepath();
    if (FAcousticsEdMode::IsSourceControlAvailable())
    {
        USourceControlHelpers::CheckOutOrAddFile(AceFilePath);
    }

    // Set the read-only flag for the ace file to false so that it can be deleted.
    FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*AceFilePath, false);
    // Delete ace file
    FString AceFileBackup = GetAceFileBackupPath();
    if (FPaths::FileExists(AceFilePath))
    {
        if (!FPlatformFileManager::Get().GetPlatformFile().CopyFile(*AceFileBackup, *AceFilePath))
        {
            FMessageDialog::Open(
                EAppMsgType::Ok, FText::FromString("Unable to create a copy of the ace file before bake."));
            return;
        }

        if (!FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*AceFilePath))
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Unable to delete file " + AceFilePath + "."));
            return;
        }
    }

    m_PythonBridge->submit_for_processing();

    // Track the total time taken for a bake.
    BakeStartTime = FDateTime::Now();
    BakeEndTime = FDateTime(0);
}

void AcousticsSharedState::CancelProcessing()
{
    m_PythonBridge->cancel_job();

    // Reset total time taken for a bake.
    BakeStartTime = FDateTime(0);
    BakeEndTime = FDateTime(0);

    // Revert the backup file so that we don't lose the original file when
    // the bake is cancelled.
    FString AceFile = GetAceFilepath();
    FString AceFileBackup = GetAceFileBackupPath();
    if (FPaths::FileExists(AceFileBackup))
    {
        if (FPlatformFileManager::Get().GetPlatformFile().CopyFile(*AceFile, *AceFileBackup))
        {
            FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*AceFileBackup);
        }
    }
}

const FAzureCallStatus& AcousticsSharedState::GetCurrentStatus()
{
    return m_PythonBridge->GetCurrentStatus();
}

const FActiveJobInfo& AcousticsSharedState::GetActiveJobInfo()
{
    return m_PythonBridge->GetActiveJobInfo();
}

float AcousticsSharedState::EstimateProcessingTime()
{
    // Can't estimate if there are no probes
    if (!m_SimulationConfiguration.IsValid() || !m_SimulationConfiguration->IsReady() ||
        m_SimulationConfiguration->GetProbeCount() == 0)
    {
        return 0;
    }

    FJobConfiguration job_config;
    job_config.config_file = GetConfigFilepath();
    job_config.vox_file = GetVoxFilepath();
    job_config.probe_count = m_SimulationConfiguration->GetProbeCount();
    job_config.prefix = GetConfigurationPrefixForLevel();
    m_PythonBridge->SetJobConfiguration(job_config);

    return m_PythonBridge->estimate_processing_time();
}

FString AcousticsSharedState::GetLevelName()
{
    return GEditor->GetEditorWorldContext().World()->GetMapName();
}

FString AcousticsSharedState::GetConfigurationPrefixForLevel()
{
    // Use level name to construct default prefix
    auto levelName = GetLevelName();
    auto prefix = levelName + TEXT("_AcousticsData");
    auto config = GetProjectConfiguration();
    // There's a prefix in the map, use it
    if (config.level_prefix_map.Contains(levelName))
    {
        prefix = config.level_prefix_map[levelName];
    }
    return prefix;
}

void AcousticsSharedState::SetConfigurationPrefixForLevel(FString prefix)
{
    auto config = GetProjectConfiguration();
    auto levelName = GetLevelName();

    // Use level name to construct default prefix
    if (prefix.IsEmpty())
    {
        prefix = levelName + TEXT("_AcousticsData");
    }

    if (config.level_prefix_map.Contains(levelName))
    {
        config.level_prefix_map[levelName] = prefix;
    }
    else
    {
        config.level_prefix_map.Add(levelName, prefix);
    }

    SetProjectConfiguration(config);
}

bool AcousticsSharedState::IsAceFileReadOnly()
{
    auto aceFile = GetAceFilepath();
    if (!FPaths::FileExists(aceFile))
    {
        return false;
    }
    return FFileManagerGeneric::Get().IsReadOnly(*aceFile);
}