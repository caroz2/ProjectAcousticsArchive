// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// The Project Acoustics module

#include "ProjectAcousticsNative.h"
#include "AcousticsSourceDataOverride.h"
#include "AcousticsSourceDataOverrideSettings.h"
#include "AcousticsParameterInterface.h"
#include "AcousticsAudioPluginListener.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FProjectAcousticsNativeModule"

// update loading path when more platforms are supported
constexpr auto c_HrtfDspThirdPartyPath = TEXT("Source/ThirdParty/Win64/Release/HrtfDsp.dll");

TAudioSourceDataOverridePtr FSourceDataOverridePluginFactory::CreateNewSourceDataOverridePlugin(FAudioDevice* OwningDevice)
{
    FProjectAcousticsNativeModule* Module = &FModuleManager::GetModuleChecked<FProjectAcousticsNativeModule>("ProjectAcousticsNative");
    if (Module != nullptr)
    {
        Module->RegisterAudioDevice(OwningDevice);
    }

    return TAudioSourceDataOverridePtr(new FAcousticsSourceDataOverride());
}

IAudioPluginFactory* FProjectAcousticsNativeModule::GetPluginFactory(EAudioPlugin PluginType)
{
    switch (PluginType)
    {
    case EAudioPlugin::SOURCEDATAOVERRIDE:
        return &SourceDataOverridePluginFactory;
        break;
    default:
        return nullptr;
    }
}

void FProjectAcousticsNativeModule::RegisterAudioDevice(FAudioDevice* AudioDeviceHandle)
{
    if (!RegisteredAudioDevices.Contains(AudioDeviceHandle))
    {
        // Spawn a listener for each audio device
        TAudioPluginListenerPtr NewAcousticsAudioPluginListener = TAudioPluginListenerPtr(new FAcousticsAudioPluginListener());
        AudioDeviceHandle->RegisterPluginListener(NewAcousticsAudioPluginListener);

        RegisteredAudioDevices.Add(AudioDeviceHandle);
    }
}

void FProjectAcousticsNativeModule::UnregisterAudioDevice(FAudioDevice* AudioDeviceHandle)
{
    RegisteredAudioDevices.Remove(AudioDeviceHandle);
}

void FProjectAcousticsNativeModule::StartupModule()
{
    IModularFeatures::Get().RegisterModularFeature(FSourceDataOverridePluginFactory::GetModularFeatureName(), &SourceDataOverridePluginFactory);

    AcousticsParameterInterface::RegisterInterface();

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
            UE_LOG(LogAcousticsNative, Error, TEXT("HrtfDsp.dll not found!"));
            return;
        }
    }
#elif PLATFORM_ANDROID
    // unnecessary to pre-load the library on Android
#else
    UE_LOG(
        LogAcousticsNative, 
        Error, 
        TEXT("Unsupported Platform. Supported platforms are WINDOWS and ANDROID"));
    return;
#endif
}

void FProjectAcousticsNativeModule::ShutdownModule()
{
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectAcousticsNativeModule, ProjectAcousticsNative)
