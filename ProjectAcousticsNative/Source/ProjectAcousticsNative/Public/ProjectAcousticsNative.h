// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Modules/ModuleManager.h"
#include "AcousticsSourceDataOverrideSourceSettings.h"

// For creating our custom source data override plugin
class FSourceDataOverridePluginFactory : public IAudioSourceDataOverrideFactory
{
public:
    virtual FString GetDisplayName() override
    {
        static FString DisplayName = FString(TEXT("Project Acoustics"));
        return DisplayName;
    }
    virtual bool SupportsPlatform(const FString& PlatformName) override
    {
        return PlatformName == FString(TEXT("Windows")) || PlatformName == FString(TEXT("Android"));
    }

    virtual UClass* GetCustomSourceDataOverrideSettingsClass() const override { return UAcousticsSourceDataOverrideSourceSettings::StaticClass(); }

    virtual TAudioSourceDataOverridePtr CreateNewSourceDataOverridePlugin(FAudioDevice* OwningDevice) override;
};

class FProjectAcousticsNativeModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // Register an audio device for the Acoustics SDO plugin
    // Performs any per-device logic
    void RegisterAudioDevice(FAudioDevice* AudioDeviceHandle);

    // Unregisters given audio device for the Acoustics SDO plugin
    void UnregisterAudioDevice(FAudioDevice* AudioDeviceHandle);

    // Plugins
    IAudioPluginFactory* GetPluginFactory(EAudioPlugin PluginType);

private:
    void* m_HrtfDspDll = nullptr;

    // List of registered audio devices.
    TArray<FAudioDevice*> RegisteredAudioDevices;

    // Factories
    FSourceDataOverridePluginFactory SourceDataOverridePluginFactory;
};