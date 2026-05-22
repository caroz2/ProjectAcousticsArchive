// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsPythonBridge.h"
#include "AcousticsDataFactory.h"
#include "Misc/MessageDialog.h"
#include "UObject/UObjectHash.h" // Needed for GetDerivedClasses
#include "Misc/Paths.h"          // Needed for FPaths
#include "AcousticsEdMode.h"
#include "EditorModeManager.h"
#include "AcousticsSharedState.h"

UAcousticsPythonBridge* UAcousticsPythonBridge::Get()
{
    TArray<UClass*> pythonBridgeClasses;
    GetDerivedClasses(UAcousticsPythonBridge::StaticClass(), pythonBridgeClasses);
    auto numClasses = pythonBridgeClasses.Num();
    if (numClasses > 0)
    {
        return Cast<UAcousticsPythonBridge>(pythonBridgeClasses[numClasses - 1]->GetDefaultObject());
    }
    return nullptr;
};

void UAcousticsPythonBridge::Initialize()
{
    // Added edit mode for source control access.
    m_AcousticsEditMode =
        static_cast<FAcousticsEdMode*>(GLevelEditorModeTools().GetActiveMode(FAcousticsEdMode::EM_AcousticsEdModeId));

    // Set the config required by the Python projection
    project_config.plugins_dir = FPaths::ProjectPluginsDir();
    project_config.config_dir = FPaths::ProjectConfigDir();
    project_config.game_content_dir = FPaths::Combine(FPaths::ProjectContentDir(), L"Acoustics");
    project_config.log_dir = FPaths::ProjectLogDir();

    // Initialize the projection
    initialize_projection();
}

void UAcousticsPythonBridge::SetAzureCredentials(const FAzureCredentials& creds)
{
    azure_credentials = creds;
    update_azure_credentials();
    save_configuration();
}

void UAcousticsPythonBridge::SetSimulationParameters(const FSimulationParameters& params)
{
    simulation_parameters = params;
    save_configuration();
}

void UAcousticsPythonBridge::SetComputePoolConfiguration(const FComputePoolConfiguration& config)
{
    compute_pool_configuration = config;
    save_configuration();
}

void UAcousticsPythonBridge::SetJobConfiguration(const FJobConfiguration& config)
{
    job_configuration = config;
    save_configuration();
}

void UAcousticsPythonBridge::SetProjectConfiguration(const FProjectConfiguration& config)
{
    project_config = config;
    save_configuration();
}

void UAcousticsPythonBridge::create_ace_asset(FString acePath)
{
    UAcousticsDataFactory::ImportFromFile(acePath);
}

void UAcousticsPythonBridge::show_readonly_ace_dialog()
{
    auto message = FString(TEXT("Please provide write access to  " + AcousticsSharedState::GetAceFilepath()));
    while (AcousticsSharedState::IsAceFileReadOnly())
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(message));
    }
}