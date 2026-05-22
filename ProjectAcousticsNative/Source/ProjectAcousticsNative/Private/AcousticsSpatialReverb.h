// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include "HrtfApi.h"
#include "AcousticsSourceDataOverrideSettings.h"
#include "DSP/MultichannelBuffer.h"
#include "DSP/DeinterleaveView.h"

/**
 * Maintains connection to HrtfEngine, stores the input and output buffers in between frames and sources, and kicks off
 * the DSP processing
 */
class FAcousticsSpatialReverb
{
public:
    FAcousticsSpatialReverb();

    bool Initialize(const FAudioPluginInitializationParams initializationParams, ESpatialReverbQuality reverbQuality);

    void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, USourceDataOverridePluginSourceSettingsBase* InSettings);
    void OnReleaseSource(const uint32 SourceId);

    // Return the direction of each output channel (virtual speaker) being rendered in spatial reverb
    void GetOutputChannelDirections(TArray<FVector>& directions, uint32* numOutputChannels);

    // Save a new input buffer for a source. This input will be processed on the next ProcessAllSources call
    void SaveInputBuffer(const uint32 sourceId, const float* inputBuffer, const uint32 numSamples, const uint32 numChannels);

    // When called, will run all currently saved input buffers through the spatial reverb DSP
    void ProcessAllSources();

    // Will copy out the last processed buffer for a single output channel
    void CopyOutputChannel(const uint32 outputChannelIndex, float* outputBuffer);

    // Send the latest HrtfAcousticParameters for a source to HrtfDsp
    void SetHrtfParametersForSource(const uint32 sourceId, const HrtfAcousticParameters* params);

private:
    // Set up the output channels and numChannels based on the current m_QualitySetting
    bool SaveOutputChannels();

    // Number of float samples to process for a buffer
    uint32_t m_HrtfFrameCount;

    uint32_t m_MaxSources = 0;

    // Saved input buffers for each source
    Audio::FMultichannelBuffer m_InputSampleBuffers;

    // HrtfEngine specific structures for passing in the input buffers. Has pointers to m_InputSampleBuffers
    TArray<HrtfInputBuffer> m_HrtfInputBuffers;

    // Buffer for storing interleaved output directly from HrtfEngine
    Audio::FAlignedFloatBuffer m_HrtfOutputBuffer;

    // Buffers for storing deinterleaved output from HrtfEngine
    Audio::FMultichannelBuffer m_OutputSampleBuffers;

    // Quality setting for spatial reverb
    ESpatialReverbQuality m_QualitySetting;

    // Number of output channels for currently set spatial reverb quality
    uint32 m_NumOutputChannels;

    // Directions for each spatial reverb output channel (virtual speaker)
    TArray<FVector> m_OutputChannelDirections;

    // Handle to our HrtfEngine instance
    ObjectHandle m_HrtfEngine;

    // Whether the HrtfEngine and all the reverb state has been fully initialized
    bool m_IsInitialized;

    // Whether this source has been HRTF processed and has output audio ready to be sent out
    TArray<bool> m_HasProcessedAudio;

    // Extra scratch buffer
    Audio::FAlignedFloatBuffer m_ScratchBuffer;

};