// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "IAudioExtensionPlugin.h"
#include "Sound/SoundEffectSubmix.h"
#include "Sound/SoundSubmix.h"
#include "AcousticsSpatializer.h"
#include "AcousticsSpatializerReverb.generated.h"

class UAcousticsReverbSubmixPreset;
class FAcousticsSpatializer;


// the Reverb plugin has three main components:
// 1. the Reverb plugin itself which orchestrates the signal flow
// 2. the Submix object which is the holder of an effects chain
// 3. the SubmixEffect object which registered to generate or modify audio within the audio graph
// 
// The Reverb plugin will ensure that a valid submix object is either loaded from the Content folder or created as 
// a fresh instance if the preset asset is not present in the project
// Once the Submix asset is instantiated, a submix effect is created in order to hook into the flow of audio data
// Upon a ProcessAudio call to the submix effect, the OutData buffer is forwarded to a ::ProcessMixedAudio routine 
// in the FAcousticsSpatializerReverb class. 

class FAcousticsSpatializerReverb : public IAudioReverb
{
public:
    FAcousticsSpatializerReverb();
    virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
    virtual void OnInitSource(
        const uint32 SourceId, const FName& AudioComponentUserId, const uint32 NumChannels,
        UReverbPluginSourceSettingsBase* InSettings) override;
    virtual void OnReleaseSource(const uint32 SourceId) override;
    virtual FSoundEffectSubmixPtr GetEffectSubmix() override;
    virtual USoundSubmix* GetSubmix() override;
    virtual void ProcessSourceAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;
    void ProcessMixedAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData);
    void SetAcousticsSpatializerPlugin(FAcousticsSpatializer* InAcousticsSpatializerPlugin);

private:
    void InitEffectSubmix();

    FAcousticsSpatializer* m_AcousticsSpatializerPlugin = nullptr;

    // the ReverbSubmix is the holder for a effects processing chain that is hosted by the Reverb plugin
    TWeakObjectPtr<USoundSubmix> m_ReverbSubmix;
    // the SubmixEffect is an effect that is inserted into a slot of the effects chain for the submix 
    // and allows for modifying the outgoing signal.  this effect copies data generated from the spatializer
    // plugin and places that post-processed data into the effects chain for further mixing with the master mixer graph
    FSoundEffectSubmixPtr m_SubmixEffect;

    // Extra scratch buffer
    Audio::FAlignedFloatBuffer m_ScratchBufferHrtf;
    Audio::FAlignedFloatBuffer m_ScratchBufferOutput;
};

class FAcousticsSpatializerReverbSubmix : public FSoundEffectSubmix
{
public:
    FAcousticsSpatializerReverbSubmix();

    virtual void Init(const FSoundEffectSubmixInitData& InData) override;
    virtual uint32 GetDesiredInputChannelCountOverride() const override;
    virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;
    virtual void OnPresetChanged() override;

    void SetAcousticsReverbPlugin(FAcousticsSpatializerReverb* InAcousticsReverbPlugin);

private:
    FAcousticsSpatializerReverb* m_AcousticsReverbPlugin = nullptr;
};


USTRUCT(BlueprintType)
struct FAcousticsSpatializerReverbSubmixSettings
{
    GENERATED_USTRUCT_BODY()
};


UCLASS(BlueprintType)
class UAcousticsSpatializerReverbSubmixPreset : public USoundEffectSubmixPreset
{
    GENERATED_BODY()

public:
    EFFECT_PRESET_METHODS(AcousticsSpatializerReverbSubmix)

    UPROPERTY(EditAnywhere, Category = SubmixEffectPreset)
    FAcousticsSpatializerReverbSubmixSettings Settings;
};

