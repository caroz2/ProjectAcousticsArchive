// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSpatialReverb.h"
#include "Interfaces/IPluginManager.h"
#include "MathUtils.h"
#include "AudioMixerDevice.h"
#include "DSP/FloatArrayMath.h"
#include "AcousticsSourceDataOverride.h"

DEFINE_LOG_CATEGORY(LogAcousticsNative)

FAcousticsSpatialReverb::FAcousticsSpatialReverb() : 
    m_HrtfFrameCount(0)
    , m_MaxSources(0)
    , m_QualitySetting(ESpatialReverbQuality::Best)
    , m_NumOutputChannels(0)
    , m_IsInitialized(false)
{
}

bool FAcousticsSpatialReverb::Initialize(
    const FAudioPluginInitializationParams initializationParams, ESpatialReverbQuality reverbQuality)
{
    // support multiple buffer lengths
    if (initializationParams.BufferLength < 256)
    {
        UE_LOG(LogAcousticsNative, Error, TEXT("Project Acoustics does not support buffer sizes of less than 256"));
        return false;
    }

    m_HrtfFrameCount = initializationParams.BufferLength;
    m_MaxSources = initializationParams.NumSources;
    m_InputSampleBuffers.SetNum(m_MaxSources);
    m_HrtfInputBuffers.SetNum(m_MaxSources);
    for (auto i = 0u; i < m_MaxSources; i++)
    {
        m_InputSampleBuffers[i].SetNumZeroed(m_HrtfFrameCount);
        m_HrtfInputBuffers[i].Buffer = nullptr;
        m_HrtfInputBuffers[i].Length = 0;
    }

    m_QualitySetting = reverbQuality;
    auto engineType = HrtfEngineType_SpatialReverbOnly_High;
    if (m_QualitySetting == ESpatialReverbQuality::Good)
    {
        engineType = HrtfEngineType_SpatialReverbOnly_Low;
    }

    // Initialize the DSP with max number of sources
    auto result = HrtfEngineInitialize(m_MaxSources, engineType, m_HrtfFrameCount, &m_HrtfEngine);
    if (!result)
    {
        UE_LOG(LogAcousticsNative, Error, TEXT("HrtfEngine failed to initialize with max sources for spatial reverb."));
        return false;
    }

    // Set up all state that requires num channels
    m_IsInitialized = SaveOutputChannels();

    return m_IsInitialized;
}

void FAcousticsSpatialReverb::OnInitSource(const uint32 SourceId, const FName& /*AudioComponentUserId*/, USourceDataOverridePluginSourceSettingsBase* InSettings)
{
    if (!m_IsInitialized)
    {
        return;
    }
}

void FAcousticsSpatialReverb::OnReleaseSource(const uint32 SourceId)
{
    if (!m_IsInitialized)
    {
        return;
    }
    m_InputSampleBuffers[SourceId].SetNumZeroed(m_HrtfFrameCount);
    m_HrtfInputBuffers[SourceId].Buffer = nullptr;
    m_HrtfInputBuffers[SourceId].Length = 0;
}

bool FAcousticsSpatialReverb::SaveOutputChannels()
{
    if (m_HrtfEngine == nullptr)
    {
        UE_LOG(LogAcousticsNative, Error, TEXT("HrtfEngine not initialized. Can't set up speaker buffers"));
        return false;
    }

    HrtfEngineGetNumOutputChannels(m_HrtfEngine, &m_NumOutputChannels);

    // Get the directions from the HrtfEngine that the output channels (virtual speakers) should be located
    TArray<VectorF> hrtfOutputDirections;
    hrtfOutputDirections.SetNum(m_NumOutputChannels);
    HrtfEngineGetOutputChannelSpatialDirections(m_HrtfEngine, hrtfOutputDirections.GetData(), m_NumOutputChannels);

    // Save the directions in Unreal coordinates
    m_OutputChannelDirections.SetNum(m_NumOutputChannels);
    for (auto i = 0u; i < m_NumOutputChannels; i++)
    {
        // Convert from Hrtf to Unreal transform, and place 1 meter away. 
        // Hrtf gives directions as unit vectors, so convert from Hrtf units (cm) to UE units (m) to place 1m away
        m_OutputChannelDirections[i] =
            AcousticsUtils::HrtfEngineDirectionToUnreal(AcousticsUtils::ToFVector(hrtfOutputDirections[i])) *
                                       AcousticsUtils::c_TritonToUnrealScale;
    }

    // Initialize our arrays for the output channels
    m_OutputSampleBuffers.SetNum(m_NumOutputChannels);
    m_HasProcessedAudio.SetNumZeroed(m_NumOutputChannels);
    for (auto i = 0u; i < m_NumOutputChannels; i++)
    {
        m_OutputSampleBuffers[i].SetNumZeroed(m_HrtfFrameCount);
        FMemory::Memset(m_OutputSampleBuffers[i].GetData(), 0, m_HrtfFrameCount * sizeof(float));
    }
    m_HrtfOutputBuffer.SetNumZeroed(m_HrtfFrameCount * m_NumOutputChannels);

    return true;
}

void FAcousticsSpatialReverb::GetOutputChannelDirections(TArray<FVector>& directions, uint32* numOutputChannels)
{
    directions = m_OutputChannelDirections;
    *numOutputChannels = m_NumOutputChannels;
}

// Save a new input buffer for a source. This input will be processed on the next ProcessAllSources call
void FAcousticsSpatialReverb::SaveInputBuffer(const uint32 sourceId, const float* inputBuffer, const uint32 numSamples, const uint32 numChannels)
{
    if (!m_IsInitialized)
    {
        return;
    }

    check(sourceId < static_cast<uint32>(m_InputSampleBuffers.Num()));
    check(numChannels != 0);
    auto samplesPerFrame = numSamples / numChannels;
    check(samplesPerFrame == m_HrtfFrameCount);

    auto inputSampleBufferPtr = m_InputSampleBuffers[sourceId].GetData();
    FMemory::Memset(inputSampleBufferPtr, 0, sizeof(float) * m_HrtfFrameCount);

    // Input audio is interleaved, so if it is multichannel, downsample it
    if (numChannels == 1)
    {
        // Single channel. Straight copy
        memcpy(
            inputSampleBufferPtr, inputBuffer, samplesPerFrame * sizeof(float));
    }
    else if (numChannels == 2)
    {
        // Downsample stereo input.
        Audio::BufferSum2ChannelToMonoFast(inputBuffer, inputSampleBufferPtr, samplesPerFrame);
        Audio::ArrayMultiplyByConstantInPlace(m_InputSampleBuffers[sourceId], 0.5f);
    }
    else
    {
        // Downsample any other number of channels. This way is slower.

        // We unfortunately can't use Audio::ConvertDeinterleave because we can't specify our own gains and we can't use TAutoDeinterleaveView
        // because that requires an AlignedFloatBuffer.
        // So do the downmix manually.
        auto scalar = 1.0f / numChannels;
        for (auto frameIndex = 0u; frameIndex < samplesPerFrame; frameIndex++)
        {
            const float* RESTRICT inputFrame = &inputBuffer[frameIndex * numChannels];

            float value = 0.0f;
            for (auto inputChannelIndex = 0u; inputChannelIndex < numChannels; inputChannelIndex++)
            {
                value += inputFrame[inputChannelIndex] * scalar;
            }
            inputSampleBufferPtr[frameIndex] += value;
        }
    }

    // Re-activate the input buffer. This tells HrtfEngine there is input to process for this source
    m_HrtfInputBuffers[sourceId].Buffer = inputSampleBufferPtr;
    m_HrtfInputBuffers[sourceId].Length = m_HrtfFrameCount;
}

void FAcousticsSpatialReverb::ProcessAllSources()
{
    if (!m_IsInitialized)
    {
        return;
    }

    auto outputBufferLength = m_NumOutputChannels * m_HrtfFrameCount;

    // Run through HrtfEngine
    auto samplesProcessed =
        HrtfEngineProcess(m_HrtfEngine, m_HrtfInputBuffers.GetData(), m_MaxSources, m_HrtfOutputBuffer.GetData(), outputBufferLength);

    // Set the input buffers to nullptr. To HrtfEngine, this indicates they're inactive. They'll be set back to active when they receive a new buffer
    for (auto i = 0u; i < m_MaxSources; i++)
    {
        m_HrtfInputBuffers[i].Buffer = nullptr;
        m_HrtfInputBuffers[i].Length = 0;
    }

    auto numSamples = outputBufferLength / m_NumOutputChannels;

    // Deinterleave the output and store it in buffers so they can be sent out later
    // TODO - We'll be adding support to HrtfEngine to be able to specify we want deinterleaved output
    Audio::TAutoDeinterleaveView<float, Audio::FAudioBufferAlignedAllocator> DeinterleaveView(m_HrtfOutputBuffer, m_ScratchBuffer, m_NumOutputChannels);
    for (auto Channel : DeinterleaveView)
    {
        // Copy the deinterleaved output to their own index in the output array
        FMemory::Memcpy(m_OutputSampleBuffers[Channel.ChannelIndex].GetData(), Channel.Values.GetData(), sizeof(float) * Channel.Values.Num());
        m_HasProcessedAudio[Channel.ChannelIndex] = true;
    }
}

void FAcousticsSpatialReverb::CopyOutputChannel(const uint32 outputChannelIndex, float* outputBuffer)
{
    if (!m_IsInitialized || !m_HasProcessedAudio[outputChannelIndex])
    {
        return;
    }
    check(outputChannelIndex < static_cast<uint32>(m_OutputSampleBuffers.Num()));

    // Copy out the output buffer we have saved for this output channel. Then zero it for next time
    FMemory::Memcpy(outputBuffer, m_OutputSampleBuffers[outputChannelIndex].GetData(), sizeof(float) * m_HrtfFrameCount);
    FMemory::Memset(m_OutputSampleBuffers[outputChannelIndex].GetData(), 0, sizeof(float) * m_HrtfFrameCount);
    m_HasProcessedAudio[outputChannelIndex] = false;
}

void FAcousticsSpatialReverb::SetHrtfParametersForSource(const uint32 sourceId, const HrtfAcousticParameters* params)
{
    if (!m_IsInitialized)
    {
        return;
    }

    HrtfEngineSetParametersForSource(m_HrtfEngine, sourceId, params);
}

