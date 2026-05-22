// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsSpatializerPluginListener.h"
#include "AudioDevice.h"

FAcousticsSpatializerPluginListener::FAcousticsSpatializerPluginListener()
    : m_IsInitialized(false),
      m_ProjectAcousticsModule(nullptr),
      m_ReverbPtr(nullptr),
      m_SpatializationPtr(nullptr)
{
}

void FAcousticsSpatializerPluginListener::OnListenerInitialize(FAudioDevice* AudioDevice, UWorld* ListenerWorld)
{
    // Only initialize if this is a game playing. Either a real game or play in editor session
    if (m_IsInitialized || ListenerWorld == nullptr || (ListenerWorld->WorldType != EWorldType::Game && ListenerWorld->WorldType != EWorldType::PIE))
    {
        return;
    }

    if (!m_ProjectAcousticsModule)
    {
        m_ProjectAcousticsModule = &FModuleManager::GetModuleChecked<FAcousticsSpatializerModule>("ProjectAcousticsSpatializer");
    }

    // Get the names of the currently configured spatializer / reverb
    const FString CurrentSpatializerPluginName = AudioPluginUtilities::GetDesiredPluginName(EAudioPlugin::SPATIALIZATION);
    const FString CurrentReverbPluginName = AudioPluginUtilities::GetDesiredPluginName(EAudioPlugin::REVERB);
    IAudioReverb* CurrentReverbPtr = AudioDevice->ReverbPluginInterface.Get();
    IAudioSpatialization* CurrentSpatializationPtr = AudioDevice->GetSpatializationPluginInterface().Get();

    // Check if reverb and spatializer are Project Acoustics
    bool ReverbIsProjectAcoustics = CurrentReverbPtr != nullptr && CurrentReverbPluginName.Equals("Project Acoustics");
    bool SpatializationIsProjectAcoustics = CurrentSpatializationPtr != nullptr && CurrentSpatializerPluginName.Equals("Project Acoustics");

    if (ReverbIsProjectAcoustics && SpatializationIsProjectAcoustics)
    {
        // Project Acoustics is configured correctly, setup the pointers.
        m_ReverbPtr = static_cast<FAcousticsSpatializerReverb*>(CurrentReverbPtr);
        m_SpatializationPtr = static_cast<FAcousticsSpatializer*>(CurrentSpatializationPtr);
        m_ReverbPtr->SetAcousticsSpatializerPlugin(m_SpatializationPtr);
        UE_LOG(LogProjectAcousticsSpatializer, Display, TEXT("Project Acoustics Listener is initialized"));
    }
    else if (ReverbIsProjectAcoustics || SpatializationIsProjectAcoustics)
    {
        // Only one of reverb or spatializer was set to Project Acoustics. Inform user that both need to be enabled
        UE_LOG(LogProjectAcousticsSpatializer, Error, TEXT("Project Acoustics requires both Reverb and Spatialization plugins. Please enable them in the Project Settings."));
        return;
    }

    m_IsInitialized = true;
}

void FAcousticsSpatializerPluginListener::OnListenerUpdated(FAudioDevice* AudioDevice, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds)
{
}

void FAcousticsSpatializerPluginListener::OnListenerShutdown(FAudioDevice* AudioDevice)
{
    // clear the connection to the spatializer plugin
    if (m_ReverbPtr)
    {
        m_ReverbPtr->SetAcousticsSpatializerPlugin(nullptr);
    }

    // unregister the AudioDevice
    if (m_ProjectAcousticsModule)
    {
        m_ProjectAcousticsModule->UnregisterAudioDevice(AudioDevice);
    }
    
    m_IsInitialized = false;
    m_ProjectAcousticsModule = nullptr;
    m_ReverbPtr = nullptr;
    m_SpatializationPtr = nullptr;

    UE_LOG(LogProjectAcousticsSpatializer, Display, TEXT("Project Acoustics Listener is shutdown"));
}