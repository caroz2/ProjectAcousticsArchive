// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSpatializer.h"
#include "AcousticsSpatializerSettings.h"
#include "Runtime/Launch/Resources/Version.h"
#include "DSP/DeinterleaveView.h"
#include "DSP/FloatArrayMath.h"
#include "Interfaces/IPluginManager.h"
#include <cassert>
#include <stdexcept>

DEFINE_LOG_CATEGORY(LogProjectAcousticsSpatializer);

#define LOCTEXT_NAMESPACE "FAcousticsSpatializer"

static int32 s_AcousticsSpatializerQualityOverrideCVar = 0;
FAutoConsoleVariableRef CVarAcousticsSpatializerQualityOverride(
    TEXT("PA.SpatializerQuality"),
    s_AcousticsSpatializerQualityOverrideCVar,
    TEXT("Override the quality of FLEX sound sources. Will not increase quality levels. The quality used will be min of the quality in the PA Spatializer source settings and this override.\n")
    TEXT("0: Quality is not overridden, 1: Stereo Panning, 2: Good Quality, 3: High Quality"),
    ECVF_Default);

TAudioSpatializationPtr FSpatializationPluginFactory::CreateNewSpatializationPlugin(FAudioDevice* OwningDevice)
{
    FAcousticsSpatializerModule* Module = &FModuleManager::GetModuleChecked<FAcousticsSpatializerModule>("ProjectAcousticsSpatializer");
    if (Module != nullptr)
    {
        Module->RegisterAudioDevice(OwningDevice);
    }

    return TAudioSpatializationPtr(new FAcousticsSpatializer());
}

void FAcousticsSpatializer::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
    // Check # output channels. Unreal passes in 0 when specifying default
    if (!(InitializationParams.NumOutputChannels == 0 || InitializationParams.NumOutputChannels == 2))
    {
        UE_LOG(LogProjectAcousticsSpatializer, Error, TEXT("Spatializer plugin only supports stereo output!"));
        return;
    }
    if (InitializationParams.SampleRate != 48000)
    {
        UE_LOG(LogProjectAcousticsSpatializer, Error, TEXT("Spatializer plugin only supports 48kHz output!"));
        return;
    }
    // support multiple buffer lengths
    if (InitializationParams.BufferLength < 256)
    {
        UE_LOG(LogProjectAcousticsSpatializer, Error, TEXT("Spatializer plugin does not support buffer sizes of less than 256"));
        return;
    }

    // support variable buffer lengths
    m_HrtfFrameCount = InitializationParams.BufferLength;

        // Read the engineType from the settings page
    HrtfEngineType engineType;
    switch (GetDefault<UAcousticsSpatializerSettings>()->FlexEngineType)
    {
        case EFlexEngineType::HIGH_QUALITY:
            engineType = HrtfEngineType_FlexBinaural_High_NoReverb;
            break;
        case EFlexEngineType::LOW_QUALITY:
            engineType = HrtfEngineType_FlexBinaural_Low_NoReverb;
            break;
        case EFlexEngineType::STEREO_PANNING:
            engineType = HrtfEngineType_PannerOnly;
            break;
        default:
            UE_LOG(LogProjectAcousticsSpatializer, Error, TEXT("Spatializer plugin set to invalid engine type!"));
            return;
    }

    // Only let cvar values between 0 and 3 affect the rendering mode
    // Note that 0 is treated as non-overridden
    if (s_AcousticsSpatializerQualityOverrideCVar > 0 && s_AcousticsSpatializerQualityOverrideCVar < 4)
    {
        switch (s_AcousticsSpatializerQualityOverrideCVar)
        {
            case 1: // stereo panning, always the lowest setting
                UE_LOG(LogProjectAcousticsSpatializer, Verbose, TEXT("Spatializer plugin quality mode override set to Stereo Panning, overriding higher quality settings"));
                engineType = HrtfEngineType_PannerOnly;
                break;
            case 2: // low quality, only override when HrtfEngineType_FlexBinaural_High_NoReverb is the current setting
                if (engineType == HrtfEngineType_FlexBinaural_High_NoReverb)
                {
                    engineType = HrtfEngineType_FlexBinaural_Low_NoReverb;
                    UE_LOG(LogProjectAcousticsSpatializer, Verbose, TEXT("Spatializer plugin quality mode override set to Good Quality, overriding High Quality setting"));
                }
                else
                {
                    UE_LOG(LogProjectAcousticsSpatializer, Verbose, TEXT("Spatializer plugin quality mode override set to Good Quality, not overriding equal or lower quality settings"));
                }
                break;
            case 3: // high quality, no lower setting so don't override
                UE_LOG(LogProjectAcousticsSpatializer, Verbose, TEXT("Spatializer plugin quality mode override set to High Quality, not overriding lower quality settings"));
                break;
        }
    }

    // Initialize the DSP with max #sources
    auto result = HrtfEngineInitialize(InitializationParams.NumSources, engineType, m_HrtfFrameCount, &m_HrtfEngine);
    if (!result)
    {
        UE_LOG(LogProjectAcousticsSpatializer, Error, TEXT("Spatializer plugin failed to initialize with max sources."));
        return;
    }

    m_MaxSources = InitializationParams.NumSources;
    m_SampleBuffers.SetNum(InitializationParams.NumSources);
    m_HrtfInputBuffers.SetNum(InitializationParams.NumSources);
    for (auto i = 0u; i < InitializationParams.NumSources; i++)
    {
        m_SampleBuffers[i].SetNumZeroed(m_HrtfFrameCount);
        m_HrtfInputBuffers[i].Buffer = nullptr;
        m_HrtfInputBuffers[i].Length = 0;
    }

    m_HrtfOutputBufferLength = m_HrtfFrameCount * 2;
    m_HrtfOutputBuffer.SetNumZeroed(m_HrtfOutputBufferLength);

    m_Initialized = true;
}

void FAcousticsSpatializer::Shutdown()
{
    HrtfEngineUninitialize(m_HrtfEngine);
}

bool FAcousticsSpatializer::IsSpatializationEffectInitialized() const
{
    return m_Initialized;
}

void FAcousticsSpatializer::OnInitSource(
    const uint32 SourceId, const FName& AudioComponentUserId, USpatializationPluginSourceSettingsBase* InSettings)
{
    // Don't do any work unless initialization completed successfully
    if (!m_Initialized)
    {
        return;
    }

    auto result = HrtfEngineAcquireResourcesForSource(m_HrtfEngine, SourceId);
    if (!result)
    {
        UE_LOG(LogProjectAcousticsSpatializer, Error, TEXT("Spatializer plugin failed to acquire resources for a source."));
        return;
    }
    m_HrtfInputBuffers[SourceId].Buffer = m_SampleBuffers[SourceId].GetData();
    m_HrtfInputBuffers[SourceId].Length = m_HrtfFrameCount;
}

void FAcousticsSpatializer::OnReleaseSource(const uint32 SourceId)
{
    HrtfEngineReleaseResourcesForSource(m_HrtfEngine, SourceId);
    m_HrtfInputBuffers[SourceId].Buffer = nullptr;
    m_HrtfInputBuffers[SourceId].Length = 0;
}

void FAcousticsSpatializer::ProcessAudio(
    const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
{
    // Don't do any work unless initialization completed successfully
    if (!m_Initialized)
    {
        return;
    }

    HrtfAcousticParameters params = { 0 };
    FVector NewPosition = UnrealToHrtfCoordinates(InputData.SpatializationParams->EmitterPosition, InputData.SpatializationParams->Distance);
    params.PrimaryArrivalDirection = { static_cast<float>(NewPosition.X), static_cast<float>(NewPosition.Y), static_cast<float>(NewPosition.Z) };
    auto hrtfDistance = UnrealToHrtfDistance(InputData.SpatializationParams->Distance);
    params.EffectiveSourceDistance = hrtfDistance;

    HrtfEngineSetParametersForSource(m_HrtfEngine, InputData.SourceId, &params);

    // Downmix the input audio to mono
    if (InputData.NumChannels > 1)
    {
        Audio::FAlignedFloatBuffer m_ScratchBuffer;

        // Sum all channels into mono buffer
        Audio::TAutoDeinterleaveView<float, Audio::FAudioBufferAlignedAllocator> DeinterleaveView(*InputData.AudioBuffer, m_ScratchBuffer, InputData.NumChannels);
        for (auto Channel : DeinterleaveView)
        {
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
            Audio::MixInBufferFast(Channel.Values, m_SampleBuffers[InputData.SourceId]);
        }

        // Equal power sum. assuming incoherent signals.
        Audio::MultiplyBufferByConstantInPlace(m_SampleBuffers[InputData.SourceId], 1.f / FMath::Sqrt(static_cast<float>(InputData.NumChannels)));
#else // ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 0
            Audio::ArrayMixIn(Channel.Values, m_SampleBuffers[InputData.SourceId], 1.f / FMath::Sqrt(static_cast<float>(InputData.NumChannels)));
        }
#endif
    }
    else
    {
        // Save off the audio buffer and mark that we are ready for an HRTF pump pass
        FMemory::Memcpy(
            m_SampleBuffers[InputData.SourceId].GetData(), InputData.AudioBuffer->GetData(), m_HrtfFrameCount * sizeof(float));
    }
    m_NeedsProcessing = true;
}

void FAcousticsSpatializer::OnAllSourcesProcessed()
{
    // Don't do any work unless initialization completed successfully
    if (!m_Initialized)
    {
        return;
    }

    // Only process if there was an active HRTF source this go around
    if (m_NeedsProcessing)
    {
        auto samplesProcessed =
            HrtfEngineProcess(m_HrtfEngine, m_HrtfInputBuffers.GetData(), m_MaxSources, m_HrtfOutputBuffer.GetData(), m_HrtfOutputBufferLength);
        if (samplesProcessed > 0)
        {
            m_NeedsProcessing = false;
            m_NeedsRendering = true;
        }

        // Clear out the input buffers to ensure they don't get rendered again
        for (auto i = 0u; i < m_MaxSources; i++)
        {
            FMemory::Memset(m_SampleBuffers[i].GetData(), 0, m_HrtfFrameCount * sizeof(float));
        }
    }
}

bool FAcousticsSpatializer::GetNeedsRendering()
{
    return m_NeedsRendering;
}

void FAcousticsSpatializer::SetNeedsRendering(bool NeedsRendering)
{
    m_NeedsRendering = NeedsRendering;
}

Audio::FAlignedFloatBuffer FAcousticsSpatializer::GetHrtfOutputBuffer()
{
    return m_HrtfOutputBuffer;
}

uint32_t FAcousticsSpatializer::GetHrtfOutputBufferLength()
{
    return m_HrtfOutputBufferLength;
}

#undef LOCTEXT_NAMESPACE