// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "AudioPluginUtilities.h"
#include "ProjectAcousticsSpatializer.h"
#include "AcousticsSpatializer.h"
#include "AcousticsSpatializerReverb.h"

/**
 * Responsible for ensuring that both spatalizer and reverb plugins are selected
 */
class FAcousticsSpatializerPluginListener : public IAudioPluginListener
{
public:
    FAcousticsSpatializerPluginListener();

    //~ Begin IAudioPluginListener
    virtual void OnListenerInitialize(FAudioDevice* AudioDevice, UWorld* ListenerWorld) override;
    virtual void OnListenerUpdated(FAudioDevice* AudioDevice, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds) override;
    virtual void OnListenerShutdown(FAudioDevice* AudioDevice) override;
    //~ End IAudioPluginListener

private:
    bool m_IsInitialized;

    class FAcousticsSpatializerModule* m_ProjectAcousticsModule;
    class FAcousticsSpatializerReverb* m_ReverbPtr;
    class FAcousticsSpatializer* m_SpatializationPtr;
};