// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include "Sound/SoundEffectSource.h"
#include "AcousticsSourceDataOverride.h"
#include "AcousticsVirtualSpeaker.generated.h"

class FAcousticsSourceDataOverride;

USTRUCT(BlueprintType)
struct FSoundEffectAcousticsVirtualSpeakerSettings
{
    GENERATED_USTRUCT_BODY()

    FSoundEffectAcousticsVirtualSpeakerSettings()
    {
    }
};

// Custom Project Acoustics sound effect that sends the output buffers for spatial reverb virtual speakers
class FSoundEffectAcousticsVirtualSpeaker : public FSoundEffectSource
{
public:
    // Called on an audio effect at initialization on main thread before audio processing begins.
    virtual void Init(const FSoundEffectSourceInitData& InitData) override;

    // Called when an audio effect preset is changed
    virtual void OnPresetChanged() override;

    // Process the input block of audio. Called on audio thread.
    virtual void ProcessAudio(const FSoundEffectSourceInputData& InData, float* OutAudioBufferData) override;

private:
    // Index of this specific output channel (virtual speaker) in the HrtfDsp
    uint32 m_SpeakerIndex;

    FAcousticsSourceDataOverride* m_SourceDataOverrideptr = nullptr;
};

// Preset for the Project Acoustics spatial reverb virtual speaker sound effect
UCLASS(ClassGroup = AudioSourceEffect, meta = (BlueprintSpawnableComponent))
class USoundEffectAcousticsVirtualSpeakerPreset : public USoundEffectSourcePreset
{
    GENERATED_BODY()

public:
    EFFECT_PRESET_METHODS(SoundEffectAcousticsVirtualSpeaker)

    virtual FColor GetPresetColor() const override { return FColor(196.0f, 185.0f, 121.0f); }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SourceEffect|Preset")
    FSoundEffectAcousticsVirtualSpeakerSettings Settings;

    // Index of this specific output channel (virtual speaker) in the HrtfDsp
    uint32 SpeakerIndex = 0;

    FAcousticsSourceDataOverride* SourceDataOverridePtr = nullptr;
};