// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsNativeEditorModule.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "AcousticsSourceDataOverrideSourceSettingsFactory.h"
#include "ISettingsModule.h"
#include "AssetToolsModule.h"
#include "AcousticsSourceDataOverrideSettings.h"

#define LOCTEXT_NAMESPACE "FAcousticsNativeEditorModule"

void FAcousticsNativeEditorModule::StartupModule()
{
    // Register the audio editor asset type actions.
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    AssetTools.RegisterAssetTypeActions(MakeShared<FAssetTypeActions_AcousticsSourceDataOverrideSourceSettings>());

    // Register our custom plugin's settings.
    ISettingsModule* SettingsModule = FModuleManager::Get().GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule)
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "Project Acoustics SDO", NSLOCTEXT("ProjectAcousticsNative", "Project Acoustics SDO", "Project Acoustics SDO"),
            NSLOCTEXT("ProjectAcoustics", "Configure Project Acoustics Source Data Override plugin settings", "Configure Project Acoustics Source Data Override plugin settings"), GetMutableDefault<UAcousticsSourceDataOverrideSettings>());
    }
}

void FAcousticsNativeEditorModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAcousticsNativeEditorModule, ProjectAcousticsNativeEditor)