// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "AcousticsSimulationConfiguration.h"
#include "AcousticsDebugRenderer.h"
#include "AcousticsPythonBridge.h"

class AcousticsSharedState
{
public:
    static void Initialize();
    static bool IsInitialized();

    static void Destroy();

    // Method to keep various UI elements in "live" sync with the Python bridge
    static const AcousticsMaterialLibrary* GetMaterialsLibrary();
    static void SetMaterialsLibrary(TUniquePtr<AcousticsMaterialLibrary> library);
    static const AcousticsMaterialLibrary* GetKnownMaterialsLibrary();
    static void SetKnownMaterialsLibrary(TUniquePtr<AcousticsMaterialLibrary> library);
    static const AcousticsSimulationConfiguration* GetSimulationConfiguration();
    static bool IsPrebakeActive();
    static void SetSimulationConfiguration(TUniquePtr<AcousticsSimulationConfiguration> config);
    static const FSimulationParameters& GetSimulationParameters();
    static const FSimulationParameters& GetDefaultSimulationParameters();
    static void SetSimulationParameters(const FSimulationParameters& params);
    static const FProjectConfiguration& GetProjectConfiguration();
    static void SetProjectConfiguration(const FProjectConfiguration& params);
    static const FAzureCredentials& GetAzureCredentials();
    static void SetAzureCredentials(const FAzureCredentials& creds);
    static const FComputePoolConfiguration& GetComputePoolConfiguration();
    static void SetComputePoolConfiguration(const FComputePoolConfiguration& pool);

    // Translators to Triton types
    static const TritonSimulationParameters GetTritonSimulationParameters();
    static const TritonOperationalParameters GetTritonOperationalParameters();

    static FString GetVoxFilepath();
    static FString GetConfigFilename();
    static FString GetConfigFilepath();
    static FString GetAceFilepath();
    // Function to get the file path to the ace backup.
    static FString GetAceFileBackupPath();

    static FString GetConfigurationPrefixForLevel();
    static void SetConfigurationPrefixForLevel(FString prefix);

    static void SubmitForProcessing();
    static void CancelProcessing();
    static const FAzureCallStatus& GetCurrentStatus();
    static const FActiveJobInfo& GetActiveJobInfo();
    static float EstimateProcessingTime();
    static void LoadSimulationConfigFromFile();
    static FString GetLevelName();
    static bool IsAceFileReadOnly();

    // Variables to track the total time taken for a bake.
    static FDateTime BakeStartTime;
    static FDateTime BakeEndTime;

private:
    static void InitializeProjectState();

private:
    static TUniquePtr<AcousticsMaterialLibrary> m_MaterialLibrary;
    static TUniquePtr<AcousticsMaterialLibrary> m_KnownMaterialsLibrary;
    static TSharedPtr<AcousticsSimulationConfiguration> m_SimulationConfiguration;
    static TWeakObjectPtr<AAcousticsDebugRenderer> m_DebugRenderer;
    static UAcousticsPythonBridge* m_PythonBridge;
};
