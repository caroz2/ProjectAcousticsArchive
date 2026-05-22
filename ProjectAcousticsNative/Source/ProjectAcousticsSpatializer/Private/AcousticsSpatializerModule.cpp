// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ProjectAcousticsSpatializer.h"
#include "EngineUtils.h"
#include <Features/IModularFeatures.h>
#include <Interfaces/IPluginManager.h>

IMPLEMENT_MODULE(FAcousticsSpatializerModule, ProjectAcousticsSpatializer)

void FAcousticsSpatializerModule::StartupModule()
{
    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin
    // file per-module
    IModularFeatures::Get().RegisterModularFeature(
        FSpatializationPluginFactory::GetModularFeatureName(), &m_SpatializationPluginFactory);
    IModularFeatures::Get().RegisterModularFeature(
        FReverbPluginFactory::GetModularFeatureName(), &m_ReverbPluginFactory);

#if PLATFORM_WINDOWS
    if (m_HrtfDspDll == nullptr)
    {
        // Get the base directory of this plugin
        FString BaseDir = IPluginManager::Get().FindPlugin("ProjectAcoustics")->GetBaseDir();
        // Add on the relative location of the third party dll and load it
        FString LibraryPath;

        LibraryPath = FPaths::Combine(*BaseDir, c_HrtfDspThirdPartyPath);

        m_HrtfDspDll = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;
        if (m_HrtfDspDll == nullptr)
        {
            UE_LOG(LogProjectAcousticsSpatializer, Error, TEXT("HrtfDsp.dll not found!"));
            return;
        }
    }
#elif PLATFORM_ANDROID
    // unnecessary to pre-load the library on Android
#else
    UE_LOG(
        LogProjectAcousticsSpatializer,
        Error,
        TEXT("Unsupported Platform. Supported platforms are WINDOWS and ANDROID"));
    return;
#endif
}
void FAcousticsSpatializerModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
#if PLATFORM_WINDOWS
    if (m_HrtfDspDll)
    {
        // Free the dll handle after we are done with it.  This will typically happen when the editor is shut down and the plugins
        // are unloaded.  If the Dll is freed after exiting PIE mode, the editor will continue to dispatch callbacks to the spatializer
        // and SDO plugins, which will cause a crash when HRTF processing is active and the Dll is unloaded.
        FPlatformProcess::FreeDllHandle(m_HrtfDspDll);
        m_HrtfDspDll = nullptr;
    }
#endif
}

IAudioPluginFactory* FAcousticsSpatializerModule::GetPluginFactory(EAudioPlugin PluginType)
{
    switch (PluginType)
    {
    case EAudioPlugin::SPATIALIZATION:
        return &m_SpatializationPluginFactory;
        break;
    case EAudioPlugin::REVERB:
        return &m_ReverbPluginFactory;
    default:
        return nullptr;
    }
}

void FAcousticsSpatializerModule::RegisterAudioDevice(FAudioDevice* AudioDeviceHandle)
{
    if (!m_RegisteredAudioDevices.Contains(AudioDeviceHandle))
    {
        // Spawn a listener for each audio device
        TAudioPluginListenerPtr NewAcousticsSpatializerPluginListener = TAudioPluginListenerPtr(new FAcousticsSpatializerPluginListener());
        AudioDeviceHandle->RegisterPluginListener(NewAcousticsSpatializerPluginListener);

        m_RegisteredAudioDevices.Add(AudioDeviceHandle);
    }
}

void FAcousticsSpatializerModule::UnregisterAudioDevice(FAudioDevice* AudioDeviceHandle)
{
    m_RegisteredAudioDevices.Remove(AudioDeviceHandle);
    UE_LOG(LogProjectAcousticsSpatializer, Log, TEXT("Audio Device unregistered from Project Acoustics"));
}