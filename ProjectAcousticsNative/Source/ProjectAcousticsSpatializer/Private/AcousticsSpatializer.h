// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include "ProjectAcousticsSpatializer.h"
#include "HrtfApi.h"
#include "DSP/MultichannelBuffer.h"
#include "AudioDevice.h"

DECLARE_LOG_CATEGORY_EXTERN(LogProjectAcousticsSpatializer, Log, All);

// update loading path when more platforms are supported
constexpr auto c_HrtfDspThirdPartyPath = TEXT("Source/ThirdParty/Win64/Release/HrtfDsp.dll");

constexpr auto c_DistanceUnitsUnrealToHrtf = 100.0f;
inline float UnrealToHrtfDistance(float unrealUnits)
{
    return unrealUnits / c_DistanceUnitsUnrealToHrtf;
}

constexpr auto c_UnrealUnitsToMeters = 0.01f;
// Function which maps unreal coordinates to MS HRTF coordinates
inline FVector UnrealToHrtfCoordinates(const FVector& Input, float InDistance)
{
    return { c_UnrealUnitsToMeters * Input.Y * InDistance, c_UnrealUnitsToMeters * Input.X * InDistance, -c_UnrealUnitsToMeters * Input.Z * InDistance };
}

class FAcousticsSpatializer : public IAudioSpatialization
{
public:
    virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
    virtual void Shutdown() override;
    virtual bool IsSpatializationEffectInitialized() const override;
    virtual void OnInitSource(
        const uint32 SourceId, const FName& AudioComponentUserId,
        USpatializationPluginSourceSettingsBase* InSettings) override;
    virtual void OnReleaseSource(const uint32 SourceId) override;
    virtual void
        ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;
    virtual void OnAllSourcesProcessed() override;
    bool GetNeedsRendering();
    void SetNeedsRendering(bool needsRendering);
    Audio::FAlignedFloatBuffer GetHrtfOutputBuffer();
    uint32_t GetHrtfOutputBufferLength();

private:
    Audio::FAlignedFloatBuffer m_HrtfOutputBuffer;
    uint32_t m_HrtfOutputBufferLength;
    Audio::FMultichannelBuffer m_SampleBuffers;
    TArray<HrtfInputBuffer> m_HrtfInputBuffers;
    uint32_t m_HrtfFrameCount;
    uint32_t m_MaxSources = 0;

    bool m_Initialized = false;
    bool m_NeedsProcessing = false;
    bool m_NeedsRendering = false;

    ObjectHandle m_HrtfEngine;
};

