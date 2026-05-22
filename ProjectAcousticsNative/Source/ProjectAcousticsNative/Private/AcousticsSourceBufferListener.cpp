// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSourceBufferListener.h"

FAcousticsSourceBufferListener::FAcousticsSourceBufferListener(FAcousticsSourceDataOverride* ptr) :
    m_SourceDataOverridePtr(ptr)
{

}

/** AUDIO MIXER THREAD. When a source is finished and returned to the pool, this call will be called. */
void FAcousticsSourceBufferListener::OnSourceReleased(const int32 InSourceId)
{
}

/** AUDIO MIXER THREAD. New Audio buffers from the active sources enter here. */
void FAcousticsSourceBufferListener::OnNewBuffer(const ISourceBufferListener::FOnNewBufferParams& InParams)
{
    check(InParams.NumChannels != 0)
    check(InParams.SampleRate == 48000);

    // We can receive multi-channel input. That input will be interleaved
    auto samplesPerSource = InParams.NumSamples / InParams.NumChannels;
    check(samplesPerSource >= 256);

    // Save this source's input buffer to be used later in spatial reverb processing
    m_SourceDataOverridePtr->SaveNewInputBuffer(InParams);
}