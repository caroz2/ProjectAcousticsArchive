// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "ProjectAcousticsNative.h"
#include "Sound/AmbientSound.h"
#include "AcousticsSourceDataOverride.h"

/**
 * Responsible for spawning virtual speakers and maintaining their position around the listener
 */
class FAcousticsAudioPluginListener : public IAudioPluginListener
{
public:
    FAcousticsAudioPluginListener();

    //~ Begin IAudioPluginListener
    virtual void OnListenerInitialize(FAudioDevice* AudioDevice, UWorld* ListenerWorld) override;
    virtual void OnListenerUpdated(FAudioDevice* AudioDevice, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds) override;
    virtual void OnListenerShutdown(FAudioDevice* AudioDevice) override;
    virtual void OnWorldChanged(FAudioDevice* AudioDevice, UWorld* ListenerWorld) override;
    //~ End IAudioPluginListener

private:

    // Connection to the base plugin module, where we keep track of the audio devices that spawn us
    class FProjectAcousticsNativeModule* m_AcousticsNativeAudioModule;

    // Connection to the owning SourceDataOverride plugin
    class FAcousticsSourceDataOverride* m_SourceDataOverridePtr;

    // The ambient sound actors representing each virtual speaker
    TArray<AAmbientSound*> m_VirtualSpeakers;

    // Array of directions to each virtual speaker
    TArray<FVector> m_VirtualSpeakerPositions;

    uint32 m_NumVirtualSpeakers;

    bool m_IsInitialized;
};