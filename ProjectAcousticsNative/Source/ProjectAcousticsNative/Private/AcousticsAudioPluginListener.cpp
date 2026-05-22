// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "AcousticsAudioPluginListener.h"
#include "AcousticsSourceDataOverride.h"
#include "AcousticsVirtualSpeaker.h"
#include "AudioDevice.h"

FAcousticsAudioPluginListener::FAcousticsAudioPluginListener()
    : m_AcousticsNativeAudioModule(nullptr)
    , m_NumVirtualSpeakers(0)
    , m_IsInitialized(false)

{
}

void FAcousticsAudioPluginListener::OnListenerInitialize(FAudioDevice* AudioDevice, UWorld* ListenerWorld)
{
    // Only initialize if this is a game playing. Either a real game or play in editor session
    if (ListenerWorld == nullptr || (ListenerWorld->WorldType != EWorldType::Game && ListenerWorld->WorldType != EWorldType::PIE))
    {
        return;
    }

    if (!m_AcousticsNativeAudioModule)
    {
        m_AcousticsNativeAudioModule = &FModuleManager::GetModuleChecked<FProjectAcousticsNativeModule>("ProjectAcousticsNative");
    }

    m_SourceDataOverridePtr = static_cast<FAcousticsSourceDataOverride*>(AudioDevice->SourceDataOverridePluginInterface.Get());

    if (!m_SourceDataOverridePtr->IsSpatialReverbInitialized())
    {
        // Exit early and don't spawn any virtual speakers if spatial reverb isn't being used
        return;
    }

    // Save the positions of the virtual speakers. These won't change after initialization
    m_SourceDataOverridePtr->GetSpatialReverbOutputChannelDirections(m_VirtualSpeakerPositions, &m_NumVirtualSpeakers);

    // Create the effect chain that store our custom speaker effects. These speaker effects are responsible for outputing
    // the audio for each virtual speakers
    TArray<TObjectPtr<USoundSourceBus>> sourceBuses;
    for (uint32 i = 0u; i < m_NumVirtualSpeakers; i++)
    {
        // Create the source bus
        sourceBuses.Add(NewObject<USoundSourceBus>());
        auto sourceBus = sourceBuses[i];
        sourceBus->bAutoDeactivateWhenSilent = true;

        // The preset gets passed onto the actual SoundEffect that does the processing, so we set the ptr and index here
        TObjectPtr<USoundEffectAcousticsVirtualSpeakerPreset> acousticsPreset = NewObject<USoundEffectAcousticsVirtualSpeakerPreset>();
        acousticsPreset->SourceDataOverridePtr = m_SourceDataOverridePtr;
        acousticsPreset->SpeakerIndex = i;

        // Chain entry holds the preset
        auto chainEntry = FSourceEffectChainEntry();
        chainEntry.Preset = acousticsPreset;

        // Put the chain entry on a preset chain
        TObjectPtr<USoundEffectSourcePresetChain> presetChain = NewObject<USoundEffectSourcePresetChain>();
        presetChain->Chain.Add(chainEntry);

        // Add the preset chain to the source bus
        sourceBus->SourceEffectChain = presetChain;
    }

    uint32 speakerCnt = 1;
    // Spawn an ambient actor to host each of the source buses
    for (auto sourceBus : sourceBuses)
    {
        FName name = FName(FString::Printf(TEXT("ProjectAcousticsVirtualSpeaker%d"), speakerCnt++));
        FActorSpawnParameters speakerSpawnParams;
        speakerSpawnParams.Name = name;
        speakerSpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
        auto speaker = ListenerWorld->SpawnActor<AAmbientSound>(speakerSpawnParams);
        // Label is what actually gets displayed in World Outliner
#if WITH_EDITOR
        speaker->SetActorLabel(speaker->GetName());
#endif
        auto ac = speaker->GetAudioComponent();

        // Important settings for the virtual speaker
        sourceBus->VirtualizationMode = EVirtualizationMode::Disabled;
        ac->Sound = sourceBus;
        ac->bOverrideAttenuation = true;
        ac->AttenuationOverrides.bSpatialize = true;
        ac->AttenuationOverrides.bAttenuate = false;
        ac->AttenuationOverrides.bEnableReverbSend = false;
        ac->AttenuationOverrides.bEnableOcclusion = false;
        ac->AttenuationOverrides.SpatializationAlgorithm = ESoundSpatializationAlgorithm::SPATIALIZATION_HRTF;
        ac->AttenuationOverrides.bEnableSourceDataOverride = false;

        // Activate it
        ac->Play();

        m_VirtualSpeakers.Add(speaker);
    }
    UE_LOG(
        LogAcousticsNative,
        Display,
        TEXT("Spawning %d virtual speakers to render Project Acoustics Spatial Reverb"),
        m_NumVirtualSpeakers);
    m_IsInitialized = true;
}

void FAcousticsAudioPluginListener::OnListenerUpdated(FAudioDevice* AudioDevice, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds)
{
    if (m_AcousticsNativeAudioModule == nullptr || !m_IsInitialized)
    {
        return;
    }

    auto listenerLocation = ListenerTransform.GetLocation();

    // Place the speakers around the latest location around the listener
    // TODO we should save the lastListenerLocation and only update when it's changed
    for (auto i = 0u; i < m_NumVirtualSpeakers; i++)
    {
        m_VirtualSpeakers[i]->SetActorLocation(listenerLocation + m_VirtualSpeakerPositions[i]);
    }
}

void FAcousticsAudioPluginListener::OnWorldChanged(FAudioDevice* AudioDevice, UWorld* ListenerWorld)
{
    if (m_IsInitialized)
    {
        // Actors are destroyed on world changes, so we need to start from scratch
        m_IsInitialized = false;
        m_NumVirtualSpeakers = 0;
        m_VirtualSpeakers.Empty();
        m_VirtualSpeakerPositions.Empty();
    }
    OnListenerInitialize(AudioDevice, ListenerWorld);
}

void FAcousticsAudioPluginListener::OnListenerShutdown(FAudioDevice* AudioDevice)
{
    if (m_AcousticsNativeAudioModule)
    {
        m_AcousticsNativeAudioModule->UnregisterAudioDevice(AudioDevice);
    }
}