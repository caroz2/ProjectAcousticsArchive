// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSpatializerEditorModule.h"
#include "Interfaces/IPluginManager.h"
#include "ISettingsModule.h"
#include "AcousticsSpatializerSettings.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"

#define LOCTEXT_NAMESPACE "FAcousticsSpatializerEditorModule"

void FAcousticsSpatializerEditorModule::StartupModule()
{
    // Register our custom plugin's settings.
    ISettingsModule* SettingsModule = FModuleManager::Get().GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule)
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "Project Acoustics Spatializer", NSLOCTEXT("ProjectAcousticsSpatializer", "Project Acoustics Spatializer", "Project Acoustics Spatializer"),
            NSLOCTEXT("ProjectAcousticsSpatializer", "Configure Project Acoustics Spatializer plugin settings", "Configure Project Acoustics Spatializer plugin settings"), GetMutableDefault<UAcousticsSpatializerSettings>());
    }

    AcousticsReverbSettingsActions = MakeShared<FAssetTypeActions_AcousticsReverbSettings>();
    // Register the audio editor asset type actions.
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    //AssetTools.RegisterAssetTypeActions(MakeShared<FAssetTypeActions_AcousticsReverbSettings>());
    FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(AcousticsReverbSettingsActions.ToSharedRef());
}

void FAcousticsSpatializerEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAcousticsSpatializerEditorModule, ProjectAcousticsSpatializerEditor)