// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsVirtualSpeaker.h"
#include "Logging/LogMacros.h"

void FSoundEffectAcousticsVirtualSpeaker::Init(const FSoundEffectSourceInitData& InitData)
{
    USoundEffectAcousticsVirtualSpeakerPreset* _Preset = Cast<USoundEffectAcousticsVirtualSpeakerPreset>(Preset);
    m_SourceDataOverrideptr = _Preset->SourceDataOverridePtr;
    m_SpeakerIndex = _Preset->SpeakerIndex;
}

void FSoundEffectAcousticsVirtualSpeaker::OnPresetChanged()
{
    USoundEffectAcousticsVirtualSpeakerPreset* _Preset = Cast<USoundEffectAcousticsVirtualSpeakerPreset>(Preset); 
    m_SourceDataOverrideptr = _Preset->SourceDataOverridePtr;
    m_SpeakerIndex = _Preset->SpeakerIndex;
}

void FSoundEffectAcousticsVirtualSpeaker::ProcessAudio(const FSoundEffectSourceInputData& InData, float* OutAudioBufferData)
{
    // Send the spatial reverb output for this individual virtual speaker
    m_SourceDataOverrideptr->CopySpatialReverbOutputBuffer(m_SpeakerIndex, OutAudioBufferData);
}