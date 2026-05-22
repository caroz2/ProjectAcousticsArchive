// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "IAcousticsSpatializerModule.h"
#include "AcousticsSpatializerPluginListener.h"
#include "AcousticsSpatializerReverb.h"
#include "AcousticsSpatializer.h"
#include "AudioDevice.h"

class FSpatializationPluginFactory : public IAudioSpatializationFactory
{
public:
    virtual FString GetDisplayName() override
    {
        return FString(TEXT("Project Acoustics"));
    }

    virtual bool SupportsPlatform(const FString& PlatformName) override
    {
        return PlatformName == FString(TEXT("Windows")) || PlatformName == FString(TEXT("Android"));
    }

    virtual TAudioSpatializationPtr CreateNewSpatializationPlugin(FAudioDevice* OwningDevice) override;
    virtual UClass* GetCustomSpatializationSettingsClass() const override
    {
        return nullptr;
    };

    virtual bool IsExternalSend() override
    {
        return true;
    }

    virtual int32 GetMaxSupportedChannels() override
    {
        return 8;
    }
};

class FReverbPluginFactory : public IAudioReverbFactory
{
public:
    virtual FString GetDisplayName() override
    {
        return FString(TEXT("Project Acoustics"));
    }

    virtual bool SupportsPlatform(const FString& PlatformName) override
    {
        return PlatformName == FString(TEXT("Windows")) || PlatformName == FString(TEXT("Android"));
    }
    
    virtual TAudioReverbPtr CreateNewReverbPlugin(FAudioDevice* OwningDevice) override;
    virtual bool IsExternalSend() override
    {
        return true;
    }
};

class FAcousticsSpatializerModule : public IAcousticsSpatializerModule
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // Register an audio device for the Spatializer/Reverb plugins
    // Performs any per-device logic
    void RegisterAudioDevice(FAudioDevice* AudioDeviceHandle);

    // Unregisters given audio device for the Spatializer/Reverb plugins
    void UnregisterAudioDevice(FAudioDevice* AudioDeviceHandle);

    IAudioPluginFactory* GetPluginFactory(EAudioPlugin PluginType);

private:
    void* m_HrtfDspDll = nullptr;

    // List of registered audio devices.
    TArray<FAudioDevice*> m_RegisteredAudioDevices;

    // Plugin factories.
    FSpatializationPluginFactory m_SpatializationPluginFactory;
    FReverbPluginFactory m_ReverbPluginFactory;
};
