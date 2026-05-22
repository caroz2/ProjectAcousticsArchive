// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "AudioDevice.h"
#include "IAcoustics.h"
#include "AcousticsSpatialReverb.h"
#include "AcousticsSourceDataOverrideSourceSettings.h"
#include "Engine/OverlapResult.h"
#include "AcousticsSourceDataOverrideSettings.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAcousticsNative, Log, All);

class FAcousticsSourceDataOverride : public IAudioSourceDataOverride
{
public:
    FAcousticsSourceDataOverride();

    // SourceDataOverride overrides
    virtual void Initialize(const FAudioPluginInitializationParams InitializationParams);
    virtual void OnInitSource(
        const uint32 SourceId, const FName& AudioComponentUserId,
        USourceDataOverridePluginSourceSettingsBase* InSettings);
    virtual void OnReleaseSource(const uint32 SourceId);
    virtual void GetSourceDataOverrides(
        const uint32 SourceId, const FTransform& InListenerTransform, FWaveInstance* InOutWaveInstance);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
    virtual void OnAllSourcesProcessed();
#endif

    // Spatial reverb accessors
    void SaveNewInputBuffer(const ISourceBufferListener::FOnNewBufferParams& InParams);
    void GetSpatialReverbOutputChannelDirections(TArray<FVector>& directions, uint32* numOutputChannels);
    void CopySpatialReverbOutputBuffer(const uint32 outputChannelIndex, float* outputBuffer);
    EAcousticsReverbType GetReverbType()
    {
        return m_ReverbType;
    };
    bool IsSpatialReverbInitialized()
    {
        return m_ReverbType == EAcousticsReverbType::SpatialReverb && m_SpatialReverb.IsValid() &&
               m_IsSpatialReverbInitialized;
    }

private:
    void
    ApplyAcousticsDesignParamsOverrides(UWorld* world, FVector listenerLocation, FAcousticsDesignParams& designParams);
    void ProcessReverb(
        const uint32 SourceId, const bool enablePortaling, const FVector& listenerLocation,
        const float occlusionDbDesigned, const float occlusionDbActual, const AcousticsObjectParams& objectParams,
        FWaveInstance* InOutWaveInstance);
    inline FName GetSourceName(const uint32 SourceId)
    {
        return FName(FString::Printf(TEXT("Source_%d"), SourceId));
    }

private:
    // Holds the last successful query for each source. This is in case a query fails, we can instead use
    // the last successful query.
    // Key is the sourceID, value is the last successful query result
    TMap<uint64_t, AcousticsObjectParams> m_LastSuccessfulQueryMap;

    IAcoustics* m_Acoustics;

    // Reverb buses
    FSoundSubmixSendInfo m_ShortIndoorSubmixSend;
    FSoundSubmixSendInfo m_MediumIndoorSubmixSend;
    FSoundSubmixSendInfo m_LongIndoorSubmixSend;

    FSoundSubmixSendInfo m_ShortOutdoorSubmixSend;
    FSoundSubmixSendInfo m_MediumOutdoorSubmixSend;
    FSoundSubmixSendInfo m_LongOutdoorSubmixSend;

    // Source settings for all possible sources
    TArray<UAcousticsSourceDataOverrideSourceSettings*> m_SourceSettings;

    // Whether or not stereo convolution reverb was successfully loaded
    bool m_IsStereoReverbInitialized = false;

    // Whether or not spatial reverb was successfully loaded
    bool m_IsSpatialReverbInitialized = false;

    // Arrays for calculating per-source reverb weights
    TArray<float> m_ReverbBusWeights = {0.0, 0.0, 0.0};
    TArray<float> m_ReverbBusDecayTimes = {0.0, 0.0, 0.0};

    // Which type of reverb we're using
    EAcousticsReverbType m_ReverbType = c_DefaultAcousticsReverbType;

    TUniquePtr<FAcousticsSpatialReverb> m_SpatialReverb;

    // Source buffer listeners allow us to get audio buffers for our sound sources
    TArray<FSharedISourceBufferListenerPtr> m_SourceBufferListeners;
};