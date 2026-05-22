// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSpatializerReverb.h"
#include "AcousticsSpatializerSettings.h"
#include "ProjectAcousticsSpatializer.h"
#include "DSP/MultichannelBuffer.h"
#include "DSP/DeinterleaveView.h"
#include "DSP/FloatArrayMath.h"
#include "Runtime/Launch/Resources/Version.h"

TAudioReverbPtr FReverbPluginFactory::CreateNewReverbPlugin(FAudioDevice* OwningDevice)
{
    FAcousticsSpatializerModule* Module = &FModuleManager::GetModuleChecked<FAcousticsSpatializerModule>("ProjectAcousticsSpatializer");
    if (Module != nullptr)
    {
        Module->RegisterAudioDevice(OwningDevice);
    }

    return TAudioReverbPtr(new FAcousticsSpatializerReverb());
}

FAcousticsSpatializerReverb::FAcousticsSpatializerReverb() 
    : m_ReverbSubmix(nullptr), m_SubmixEffect(nullptr)
{
}

void FAcousticsSpatializerReverb::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
}

void FAcousticsSpatializerReverb::OnInitSource(
    const uint32 SourceId, const FName& AudioComponentUserId, const uint32 NumChannels,
    UReverbPluginSourceSettingsBase* InSettings)
{
}

void FAcousticsSpatializerReverb::OnReleaseSource(const uint32 SourceId)
{
}

// Not processing per-source reverb
void FAcousticsSpatializerReverb::ProcessSourceAudio(
    const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
{
}

void FAcousticsSpatializerReverb::ProcessMixedAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
    if (m_AcousticsSpatializerPlugin && m_AcousticsSpatializerPlugin->GetNeedsRendering())
    {
        // Copy the HRTF processed audio into the output stream
        Audio::FAlignedFloatBuffer outputBuffer = m_AcousticsSpatializerPlugin->GetHrtfOutputBuffer();
        uint32_t outputBufferLength = m_AcousticsSpatializerPlugin->GetHrtfOutputBufferLength();

        if (OutData.NumChannels == 2) 
        {
            // copy the dry path
            FMemory::Memcpy(OutData.AudioBuffer->GetData(), outputBuffer.GetData(), outputBufferLength * sizeof(float));
        }
        else if (OutData.NumChannels > 2)
        {
            // If the output buffer has more than 2 channels we copy the HRTF-processed signal into the first 2 channels
            Audio::TAutoDeinterleaveView<float, Audio::FAudioBufferAlignedAllocator> DeinterleaveViewOutput(*OutData.AudioBuffer, m_ScratchBufferOutput, OutData.NumChannels);
            Audio::TAutoDeinterleaveView<float, Audio::FAudioBufferAlignedAllocator> DeinterleaveViewHrtf(outputBuffer, m_ScratchBufferHrtf, 2);

            for (auto OutputChannel : DeinterleaveViewOutput)
            {
                for (auto HrtfChannel : DeinterleaveViewHrtf)
                {
                    if (HrtfChannel.ChannelIndex == OutputChannel.ChannelIndex)
                    {
                        FMemory::Memcpy(OutputChannel.Values.GetData(), HrtfChannel.Values.GetData(), outputBufferLength * sizeof(float));
                    }
                }
            }
        }
        else if (OutData.NumChannels == 1)
        {
            UE_LOG(LogProjectAcousticsSpatializer, Warning, TEXT("Project Acoustics Reverb connected to 1-channel output, down-mixing spatialized audio"));

#if ENGINE_MAJOR_VERSION == 5
            // Sum both channels into mono buffer
            Audio::TAutoDeinterleaveView<float, Audio::FAudioBufferAlignedAllocator> DeinterleaveView(outputBuffer, m_ScratchBufferHrtf, 2);
            for (auto HrtfChannel : DeinterleaveView)
            {
#if ENGINE_MINOR_VERSION > 0
                Audio::ArrayMixIn(HrtfChannel.Values, *OutData.AudioBuffer, 1.f / FMath::Sqrt(2.0));
            }
#else
                Audio::MixInBufferFast(HrtfChannel.Values, *OutData.AudioBuffer);
            }

            // Equal power sum. assuming incoherrent signals.
            Audio::MultiplyBufferByConstantInPlace(*OutData.AudioBuffer, 1.f / FMath::Sqrt(2.0));
#endif
#else  // !ENGINE_MAJOR_VERSION == 5
            float* OutputBufferPtr = OutData.AudioBuffer->GetData();
            for (int32 i = 0; i < InData.NumFrames; ++i)
            {
                const int32 stereoOffset = i * 2;
                OutputBufferPtr[i] = 1.f / FMath::Sqrt(2.0) * (outputBuffer[stereoOffset] + outputBuffer[stereoOffset + 1]);
            }
#endif
        }

        // clear shared buffer
        FMemory::Memset(
            outputBuffer.GetData(),
            0,
            outputBufferLength * sizeof(float));
        m_AcousticsSpatializerPlugin->SetNeedsRendering(false);
    }
}

FSoundEffectSubmixPtr FAcousticsSpatializerReverb::GetEffectSubmix()
{
    if (!m_SubmixEffect.IsValid())
    {
        UAcousticsSpatializerReverbSubmixPreset* ReverbPluginSubmixPreset = nullptr;
        USoundSubmix* Submix = GetSubmix();
        if (Submix->SubmixEffectChain.Num() > 0)
        {
            // if the effect chain already contains our pre-installed submix preset from the Content folder, then use it
            if (UAcousticsSpatializerReverbSubmixPreset* CurrentPreset = Cast<UAcousticsSpatializerReverbSubmixPreset>(Submix->SubmixEffectChain[0]))
            {
                ReverbPluginSubmixPreset = CurrentPreset;
            }
        }

        if (!ReverbPluginSubmixPreset)
        {
            // no submix preset found since someone deleted the asset, so generate a new one on the fly
            ReverbPluginSubmixPreset = NewObject<UAcousticsSpatializerReverbSubmixPreset>(Submix, TEXT("Project Acoustics Reverb Plugin Effect Preset"));
        }

        if (ReverbPluginSubmixPreset)
        {
            // create a instance of our submix effect that allows us to supply spatializer data to the submix pipeline
            m_SubmixEffect = USoundEffectPreset::CreateInstance<FSoundEffectSubmixInitData, FSoundEffectSubmix>(FSoundEffectSubmixInitData(), *ReverbPluginSubmixPreset);
        }

        if (ensure(m_SubmixEffect.IsValid()))
        {
            // connect reverb plugin processing method ::ProcessMixedAudio to submix processing chain 
            StaticCastSharedPtr<FAcousticsSpatializerReverbSubmix, FSoundEffectSubmix, ESPMode::ThreadSafe>(m_SubmixEffect)->SetAcousticsReverbPlugin(this);
            m_SubmixEffect->SetEnabled(true);
        }
    }

    return m_SubmixEffect;
}

USoundSubmix* FAcousticsSpatializerReverb::GetSubmix()
{
    if (!m_ReverbSubmix.IsValid())
    {
        static const FString DefaultSubmixName = TEXT("Project Acoustics Reverb Submix");

        m_ReverbSubmix = NewObject<USoundSubmix>(USoundSubmix::StaticClass(), *DefaultSubmixName);
        m_ReverbSubmix->bMuteWhenBackgrounded = true;
#if ENGINE_MAJOR_VERSION == 5
        m_ReverbSubmix->bAutoDisable = false;  // avoid turning off submix during silence
#endif
    }

    bool bFoundPreset = false;
    for (USoundEffectSubmixPreset* Preset : m_ReverbSubmix->SubmixEffectChain)
    {
        if (UAcousticsSpatializerReverbSubmixPreset* PluginPreset = Cast<UAcousticsSpatializerReverbSubmixPreset>(Preset))
        {
            bFoundPreset = true;
            break;
        }
    }

    if (!bFoundPreset)
    {
        static const FString DefaultPresetName = TEXT("ProjectAcousticsReverbDefault_0");
        m_ReverbSubmix->SubmixEffectChain.Add(NewObject<UAcousticsSpatializerReverbSubmixPreset>(UAcousticsSpatializerReverbSubmixPreset::StaticClass(), *DefaultPresetName));
    }

    return m_ReverbSubmix.Get();
}

void FAcousticsSpatializerReverb::SetAcousticsSpatializerPlugin(FAcousticsSpatializer* InAcousticsSpatializerPlugin)
{
    m_AcousticsSpatializerPlugin = InAcousticsSpatializerPlugin;
}

//==================================================================================================================================================
// FAcousticsSpatializerReverbSubmix
//==================================================================================================================================================

FAcousticsSpatializerReverbSubmix::FAcousticsSpatializerReverbSubmix()
    : m_AcousticsReverbPlugin(nullptr)
{
}

void FAcousticsSpatializerReverbSubmix::Init(const FSoundEffectSubmixInitData& InData)
{
}

void FAcousticsSpatializerReverbSubmix::OnPresetChanged()
{
}

uint32 FAcousticsSpatializerReverbSubmix::GetDesiredInputChannelCountOverride() const
{
    return 2;
}

void FAcousticsSpatializerReverbSubmix::OnProcessAudio(
    const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
    if (m_AcousticsReverbPlugin)
    {
        m_AcousticsReverbPlugin->ProcessMixedAudio(InData, OutData);
    }
}

void FAcousticsSpatializerReverbSubmix::SetAcousticsReverbPlugin(FAcousticsSpatializerReverb* InAcousticsReverbPlugin)
{
    m_AcousticsReverbPlugin = InAcousticsReverbPlugin;
}
