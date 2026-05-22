// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "AcousticsSourceDataOverride.h"
#include "IAcoustics.h"
#include "Engine/World.h"
#include "MathUtils.h"
#include "AcousticsSourceDataOverrideSettings.h"
#include "Sound/SoundEffectSubmix.h"
#include "Sound/SoundSubmix.h"
#include "SubmixEffects/AudioMixerSubmixEffectReverb.h"
#include "AcousticsRuntimeVolume.h"
#include "AcousticsShared.h"
#include "AcousticsParameterInterface.h"
#include "AcousticsSourceBufferListener.h"
#include "AcousticsAudioComponent.h"
#include "Engine/EngineTypes.h"
#include "Components/AudioComponent.h"

//DEFINE_LOG_CATEGORY(LogAcousticsNative)

FAcousticsSourceDataOverride::FAcousticsSourceDataOverride()
    : m_Acoustics(nullptr)
    , m_IsStereoReverbInitialized(false)
    , m_IsSpatialReverbInitialized(false)
    , m_SpatialReverb(nullptr)
{
}

// Initializes the source data override plugin
void FAcousticsSourceDataOverride::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
    // Cache module instance
    if (IAcoustics::IsAvailable())
    {
        m_Acoustics = &(IAcoustics::Get());
    }
    else
    {
        UE_LOG(
            LogAcousticsNative,
            Error,
            TEXT("Unable to find IAcoustics instance. This plugin depends on the ProjectAcoustics plugin for "
                 "communicating with the acoustics engine."));
    }

    m_LastSuccessfulQueryMap.Reset();

    // Allocate settings for max sources
    m_SourceSettings.Init(nullptr, InitializationParams.NumSources);

    // Process the reverb settings
    auto settings = GetDefault<UAcousticsSourceDataOverrideSettings>();

    m_ReverbType = settings->ReverbType;

    if (m_ReverbType == EAcousticsReverbType::SpatialReverb)
    {
        if (!c_SpatialReverbSupported)
        {
            UE_LOG(
                LogAcousticsNative,
                Error,
                TEXT("Project Acoustics SDO Spatial Reverb is not supported below UE 5.1. "
                    "Please change the ReverbType in the Project Acoustics SDO Project Settings"));
            return;
        }

        m_SpatialReverb = TUniquePtr<FAcousticsSpatialReverb>(new FAcousticsSpatialReverb());

        if (!m_SpatialReverb.IsValid() || !m_SpatialReverb->Initialize(
                InitializationParams, settings->SpatialReverbQuality))
        {
            UE_LOG(
                LogAcousticsNative,
                Error,
                TEXT("Project Acoustics SDO Spatial Reverb did not initialize correctly"));
            return;
        }

        if (InitializationParams.BufferLength < 256)
        {
            UE_LOG(
                LogAcousticsNative,
                Error,
                TEXT("Project Acoustics SDO Spatial Reverb does not support buffer sizes of less than 256"));
            return;
        }

        // Preallocate for all the SourceBufferListeners we will need
        m_SourceBufferListeners.SetNum(InitializationParams.NumSources);

        m_IsSpatialReverbInitialized = true;
    }

    auto shortIndoorSubmix = Cast<USoundSubmix>(settings->ShortIndoorReverbSubmix.TryLoad());
    auto mediumIndoorSubmix = Cast<USoundSubmix>(settings->MediumIndoorReverbSubmix.TryLoad());
    auto longIndoorSubmix = Cast<USoundSubmix>(settings->LongIndoorReverbSubmix.TryLoad());

    auto shortOutdoorSubmix = Cast<USoundSubmix>(settings->ShortOutdoorReverbSubmix.TryLoad());
    auto mediumOutdoorSubmix = Cast<USoundSubmix>(settings->MediumOutdoorReverbSubmix.TryLoad());
    auto longOutdoorSubmix = Cast<USoundSubmix>(settings->LongOutdoorReverbSubmix.TryLoad());

    // Check if the submixes entered in the settings are valid
    if (shortIndoorSubmix == nullptr || mediumIndoorSubmix == nullptr || longIndoorSubmix == nullptr ||
        shortOutdoorSubmix == nullptr || mediumOutdoorSubmix == nullptr || longOutdoorSubmix == nullptr)
    {
        m_IsStereoReverbInitialized = false;
        UE_LOG(
            LogAcousticsNative,
            Warning,
            TEXT("Invalid submixes specified in Project Acoustics project settings. Skipping reverb"));
        return;
    }

    // Check that the reverb lengths entered are valid. Checking that short < medium < long
    if (settings->MediumReverbLength <= settings->ShortReverbLength ||
        settings->LongReverbLength <= settings->MediumReverbLength)
    {
        m_IsStereoReverbInitialized = false;
        UE_LOG(
            LogAcousticsNative,
            Warning,
            TEXT("Invalid reverb lengths specified in Project Acoustics project settings. Lengths must be: short < "
                 "medium < long. Skipping reverb."));
        return;
    }

    // Save the submixes for later use
    m_ShortIndoorSubmixSend.SoundSubmix = shortIndoorSubmix;
    m_MediumIndoorSubmixSend.SoundSubmix = mediumIndoorSubmix;
    m_LongIndoorSubmixSend.SoundSubmix = longIndoorSubmix;

    m_ShortOutdoorSubmixSend.SoundSubmix = shortOutdoorSubmix;
    m_MediumOutdoorSubmixSend.SoundSubmix = mediumOutdoorSubmix;
    m_LongOutdoorSubmixSend.SoundSubmix = longOutdoorSubmix;

    // Need to specify that we will be specifying the send level
    auto sendLevelControlMethod = ESendLevelControlMethod::Manual;

    m_ShortIndoorSubmixSend.SendLevelControlMethod = sendLevelControlMethod;
    m_MediumIndoorSubmixSend.SendLevelControlMethod = sendLevelControlMethod;
    m_LongIndoorSubmixSend.SendLevelControlMethod = sendLevelControlMethod;

    m_ShortOutdoorSubmixSend.SendLevelControlMethod = sendLevelControlMethod;
    m_MediumOutdoorSubmixSend.SendLevelControlMethod = sendLevelControlMethod;
    m_LongOutdoorSubmixSend.SendLevelControlMethod = sendLevelControlMethod;

    // Disable distance/occlusion attenuation on the reverb submix.
    auto sendStage = ESubmixSendStage::PreDistanceAttenuation;

    m_ShortIndoorSubmixSend.SendStage = sendStage;
    m_MediumIndoorSubmixSend.SendStage = sendStage;
    m_LongIndoorSubmixSend.SendStage = sendStage;

    m_ShortOutdoorSubmixSend.SendStage = sendStage;
    m_MediumOutdoorSubmixSend.SendStage = sendStage;
    m_LongOutdoorSubmixSend.SendStage = sendStage;

    m_IsStereoReverbInitialized = true;
}

// Called when a source is assigned to a voice.
void FAcousticsSourceDataOverride::OnInitSource(
    const uint32 SourceId, const FName& AudioComponentUserId, USourceDataOverridePluginSourceSettingsBase* InSettings)
{
    bool showAcousticParameters = false;

    if (InSettings)
    {
        // Save the settings for this source
        m_SourceSettings[SourceId] = static_cast<UAcousticsSourceDataOverrideSourceSettings*>(InSettings);

        showAcousticParameters = m_SourceSettings[SourceId]->Settings.ShowAcousticParameters;
    }

    if (IsSpatialReverbInitialized())
    {
        // Enable what we need for spatial reverb
        m_SourceBufferListeners[SourceId] = MakeShared<FAcousticsSourceBufferListener, ESPMode::ThreadSafe>(this);
        m_SpatialReverb->OnInitSource(SourceId, AudioComponentUserId, InSettings);
    }

    m_Acoustics->RegisterSourceObject(SourceId);

#if !UE_BUILD_SHIPPING
    // Let debug renderer know a new source opened up
    m_Acoustics->UpdateSourceDebugInfo(SourceId, showAcousticParameters, GetSourceName(SourceId), false);
#endif
}

// Called when a source is done playing and is released.
void FAcousticsSourceDataOverride::OnReleaseSource(const uint32 SourceId)
{
    bool showAcousticParameters = false;

    m_LastSuccessfulQueryMap.Remove(SourceId);

    if (m_SourceSettings[SourceId] != nullptr)
    {
        showAcousticParameters = m_SourceSettings[SourceId]->Settings.ShowAcousticParameters;
    }

    if (IsSpatialReverbInitialized())
    {
        m_SpatialReverb->OnReleaseSource(SourceId);
    }

    m_Acoustics->UnregisterSourceObject(SourceId);

#if !UE_BUILD_SHIPPING
    // Tell debug renderer this source is going away so stop rendering it
    m_Acoustics->UpdateSourceDebugInfo(SourceId, showAcousticParameters, GetSourceName(SourceId), true);
#endif

    // Clear the settings for this source
    m_SourceSettings[SourceId] = nullptr;
}

void FAcousticsSourceDataOverride::ApplyAcousticsDesignParamsOverrides(
    UWorld* world, FVector sourceLocation, FAcousticsDesignParams& designParams)
{
    if (world)
    {
        TArray<FOverlapResult> OverlapResults;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(AddForceOverlap), false);

        if (world->OverlapMultiByObjectType(
                OverlapResults,
                sourceLocation,
                FQuat::Identity,
                FCollisionObjectQueryParams(ECC_WorldStatic),
                FCollisionShape::MakeSphere(0.0f),
                Params))
        {
            for (const FOverlapResult& OverlapResult : OverlapResults)
            {
                AAcousticsRuntimeVolume* AcousticsRuntimeVolume =
                    Cast<AAcousticsRuntimeVolume>(OverlapResult.GetActor());

                if (AcousticsRuntimeVolume != nullptr)
                {
                    auto overrideParams = AcousticsRuntimeVolume->OverrideDesignParams;

                    // Apply the override parameters and save to the object's design params
                    FAcousticsDesignParams::Combine(designParams, overrideParams);
                }
            }
        }
    }
}

// For a given direction from a listener, returns the Azimuth and Elevation in an orientation that is common to
// MetaSounds
void GetMetaSoundAzimuthAndElevation(
    const FTransform& InListenerTransform, FVector Direction, float& Azimuth, float& Elevation)
{
    auto directionNormal = InListenerTransform.InverseTransformVectorNoScale(Direction);

    // Specific math we need to get the azimuth/elevation in the same orientation as other MetaSound usage
    // Azimuth: 90 front, 0 right, 270 behind, 180 left
    // Elevation: 90 directly above, -90 directly below
    auto sourceAziAndEle =
        FMath::GetAzimuthAndElevation(directionNormal, FVector::LeftVector, FVector::BackwardVector, FVector::UpVector);

    Azimuth = FMath::RadiansToDegrees(sourceAziAndEle.X) + 180.0f;
    Elevation = FMath::RadiansToDegrees(sourceAziAndEle.Y);
}

// Called during the Update call in MixerSource for each source
void FAcousticsSourceDataOverride::GetSourceDataOverrides(
    const uint32 SourceId, const FTransform& InListenerTransform, FWaveInstance* InOutWaveInstance)
{
    AcousticsObjectParams objectParams;
    objectParams.ObjectId = SourceId;
    objectParams.Design = FAcousticsDesignParams::Default();
    objectParams.DynamicOpeningInfo = {};
    FName SourceName = GetSourceName(SourceId);
    bool enablePortaling = true;
    bool enableOcclusion = true;
    bool enableReverb = true;
    bool showAcousticParameters = false;
    bool applyAcousticsVolumes = true;

    // Get source and listener location
    auto sourceLocation = InOutWaveInstance->Location;
    auto listenerLocation = InListenerTransform.GetLocation();

    // Save the shared per-source settings if there are any
    auto sourceSettings = m_SourceSettings[SourceId];
    if (sourceSettings != nullptr)
    {
        objectParams.Design = sourceSettings->Settings.DesignParams;
        enablePortaling = sourceSettings->Settings.EnablePortaling;
        enableOcclusion = sourceSettings->Settings.EnableOcclusion;
        enableReverb = sourceSettings->Settings.EnableReverb;
        showAcousticParameters = sourceSettings->Settings.ShowAcousticParameters;
        applyAcousticsVolumes = sourceSettings->Settings.ApplyAcousticsVolumes;
        objectParams.InterpolationConfig = TritonRuntime::InterpolationConfig(
            static_cast<TritonRuntime::InterpolationConfig::DisambiguationMode>(sourceSettings->Settings.Resolver),
            AcousticsUtils::ToTritonVector(
                m_Acoustics->WorldDirectionToTriton(sourceSettings->Settings.PushDirection)));
        objectParams.ApplyDynamicOpenings = sourceSettings->Settings.ApplyDynamicOpenings;
    }

    // Grab the audio component belonging to this sound source
    auto audioComponentId = InOutWaveInstance->ActiveSound->GetAudioComponentID();
    auto audioComponent = UAudioComponent::GetAudioComponentFromID(audioComponentId);
    UAcousticsAudioComponent* aac = nullptr;

    // Now check if this audio component is our PA specific component. If so, grab those settings too
    if (audioComponent != nullptr && audioComponent->IsA<UAcousticsAudioComponent>())
    {
        aac = Cast<UAcousticsAudioComponent>(audioComponent);
        if (aac != nullptr)
        {
            // Audio Component params take precedence. Overwrite all settings with the ones from
            // the AcousticsAudioComponent
            objectParams.Design = aac->Settings.DesignParams;
            enablePortaling = aac->Settings.EnablePortaling;
            enableOcclusion = aac->Settings.EnableOcclusion;
            enableReverb = aac->Settings.EnableReverb;
            showAcousticParameters = aac->Settings.ShowAcousticParameters;
            applyAcousticsVolumes = aac->Settings.ApplyAcousticsVolumes;
            objectParams.InterpolationConfig = TritonRuntime::InterpolationConfig(
                static_cast<TritonRuntime::InterpolationConfig::DisambiguationMode>(aac->Settings.Resolver),
                AcousticsUtils::ToTritonVector(m_Acoustics->WorldDirectionToTriton(aac->Settings.PushDirection)));
            objectParams.ApplyDynamicOpenings = aac->Settings.ApplyDynamicOpenings;
        }
    }

    if (objectParams.ApplyDynamicOpenings)
    {
        objectParams.DynamicOpeningInfo.ApplyDynamicOpening = true;
    }

    if (applyAcousticsVolumes)
    {
        ApplyAcousticsDesignParamsOverrides(
            InOutWaveInstance->ActiveSound->GetWorld(), sourceLocation, objectParams.Design);
    }

    // Run the acoustic query
    bool acousticQuerySuccess =
        m_Acoustics->UpdateObjectParameters(SourceId, sourceLocation, listenerLocation, objectParams);

    // If failed, try to grab the last successful query
    if (!acousticQuerySuccess)
    {
        if (m_LastSuccessfulQueryMap.Contains(SourceId))
        {
            objectParams.TritonParams = m_LastSuccessfulQueryMap[SourceId].TritonParams;
            objectParams.Outdoorness = m_LastSuccessfulQueryMap[SourceId].Outdoorness;
            objectParams.DynamicOpeningInfo = m_LastSuccessfulQueryMap[SourceId].DynamicOpeningInfo;
            objectParams.Design = m_LastSuccessfulQueryMap[SourceId].Design;
            acousticQuerySuccess = true;
        }
    }
    // Update last successful query map
    else
    {
        m_LastSuccessfulQueryMap.Add(SourceId, objectParams);
    }

#if !UE_BUILD_SHIPPING
    m_Acoustics->UpdateSourceDebugInfo(SourceId, showAcousticParameters, SourceName, false);
#endif

    if (!acousticQuerySuccess)
    {
        return;
    }

    auto acousticParams = objectParams.TritonParams;

    Audio::FParameterInterfacePtr paInterface = AcousticsParameterInterface::GetInterface();
    TArray<FAudioParameter> paramsToUpdate;
    // See if the current sound is a MetaSound
    const bool isMetaSound = InOutWaveInstance->ActiveSound->GetSound()->ImplementsParameterInterface(paInterface);

    // Arrival direction for dry sound, including geometry
    FVector portalDir =
        m_Acoustics->TritonDirectionToWorld(AcousticsUtils::ToFVector(acousticParams.Dry.ArrivalDirection));

    // Spatialization
    if (enablePortaling)
    {
        float shortestDistance = AcousticsUtils::TritonValToUnreal(acousticParams.Dry.PathLengthMeters);
        FVector shortestPathSourcePos = listenerLocation + (portalDir * shortestDistance);

        // Overwrite Unreal WaveInstance location with the new Triton-derived location
        InOutWaveInstance->Location = shortestPathSourcePos;

        // Specific math we need to get the azimuth in the expected range
        // Azimuth: 0 front, 90 right, 180 behind, 270 left
        auto directionNormal = InListenerTransform.InverseTransformVectorNoScale(portalDir);
        auto SourceAzimuthAndElevation = FMath::GetAzimuthAndElevation(
            directionNormal, FVector::ForwardVector, FVector::RightVector, FVector::UpVector);
        auto Azimuth = FMath::RadiansToDegrees(SourceAzimuthAndElevation.X);
        if (Azimuth < 0)
        {
            Azimuth += 360.0f;
        }

        // Overwrite Unreal WaveInstance azimuth with new Triton-derived azimuth
        // which is necessary when using built-in panning
        InOutWaveInstance->AbsoluteAzimuth = Azimuth;
    }

    float occlusionDbActual = FMath::Max(acousticParams.Dry.LoudnessDb, acousticParams.Wet.LoudnessDb);
    float occlusionDbDesigned = occlusionDbActual * objectParams.Design.OcclusionMultiplier;
    // Occlusion attenuation
    if (enableOcclusion)
    {
        const float obstructionDb = acousticParams.Dry.LoudnessDb - occlusionDbActual;
        const float primaryArrivalGeometryPowerDb = occlusionDbDesigned + obstructionDb;
        const float primaryArrivalGeometryPowerDbCapped = FMath::Min(primaryArrivalGeometryPowerDb, 0.0f);
        const float primaryArrivalGeometryPowerAmp = AcousticsUtils::DbToAmplitude(primaryArrivalGeometryPowerDbCapped);

        // Set the occlusion attenuation for Unreal WaveInstance
        InOutWaveInstance->SetOcclusionAttenuation(primaryArrivalGeometryPowerAmp);
    }

    // Reverb processing.
    if (enableReverb)
    {
        ProcessReverb(
            SourceId,
            enablePortaling,
            listenerLocation,
            occlusionDbDesigned,
            occlusionDbActual,
            objectParams,
            InOutWaveInstance);
    }

    if (isMetaSound)
    {
        // Store the rest of the acoustic parameters for the MetaSound parameter update

        // Get dry azimuth and elevation
        float dryAzimuth = 0.0f, dryElevation = 0.0f;
        GetMetaSoundAzimuthAndElevation(InListenerTransform, portalDir, dryAzimuth, dryElevation);
        paramsToUpdate.Add({AcousticsParameterInterface::Inputs::DryArrivalAzimuth, dryAzimuth});
        paramsToUpdate.Add({AcousticsParameterInterface::Inputs::DryArrivalElevation, dryElevation});

        // Get wet azimuth and elevation
        float wetAzimuth = 0.0f, wetElevation = 0.0f;
        FVector reverbDir =
            m_Acoustics->TritonDirectionToWorld(AcousticsUtils::ToFVector(acousticParams.Wet.ArrivalDirection));
        GetMetaSoundAzimuthAndElevation(InListenerTransform, reverbDir, wetAzimuth, wetElevation);
        paramsToUpdate.Add({AcousticsParameterInterface::Inputs::WetArrivalAzimuth, wetAzimuth});
        paramsToUpdate.Add({AcousticsParameterInterface::Inputs::WetArrivalElevation, wetElevation});

        // Store the rest of the acoustic parameters
        paramsToUpdate.Add({AcousticsParameterInterface::Inputs::DryLoudness, acousticParams.Dry.LoudnessDb});
        paramsToUpdate.Add(
            {AcousticsParameterInterface::Inputs::DryPathLength,
             AcousticsUtils::TritonValToUnreal(acousticParams.Dry.PathLengthMeters)});
        paramsToUpdate.Add({AcousticsParameterInterface::Inputs::WetLoudness, acousticParams.Wet.LoudnessDb});
        paramsToUpdate.Add(
            {AcousticsParameterInterface::Inputs::WetAngularSpread, acousticParams.Wet.AngularSpreadDegrees});
        paramsToUpdate.Add({AcousticsParameterInterface::Inputs::WetDecayTime, acousticParams.Wet.DecayTimeSeconds});

        // Send the parameters to the MetaSound interface
        auto paramTransmitter = InOutWaveInstance->ActiveSound->GetTransmitter();
        if (paramTransmitter != nullptr)
        {
            paramTransmitter->SetParameters(MoveTemp(paramsToUpdate));
        }
    }
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
void FAcousticsSourceDataOverride::OnAllSourcesProcessed()
{
    if (IsSpatialReverbInitialized())
    {
        m_SpatialReverb->ProcessAllSources();
    }
}
#endif

void FAcousticsSourceDataOverride::SaveNewInputBuffer(const ISourceBufferListener::FOnNewBufferParams& InParams)
{
    if (IsSpatialReverbInitialized())
    {
       m_SpatialReverb->SaveInputBuffer(
            InParams.SourceId, InParams.AudioData, InParams.NumSamples, InParams.NumChannels);
    }
}

void FAcousticsSourceDataOverride::GetSpatialReverbOutputChannelDirections(
    TArray<FVector>& directions, uint32* numOutputChannels)
{
    if (IsSpatialReverbInitialized())
    {
        m_SpatialReverb->GetOutputChannelDirections(directions, numOutputChannels);
    }
}

void FAcousticsSourceDataOverride::CopySpatialReverbOutputBuffer(const uint32 outputChannelIndex, float* outputBuffer)
{
    if (IsSpatialReverbInitialized())
    {
        m_SpatialReverb->CopyOutputChannel(outputChannelIndex, outputBuffer);
    }
}

void FAcousticsSourceDataOverride::ProcessReverb(
    const uint32 SourceId, const bool enablePortaling, const FVector& listenerLocation, const float occlusionDbDesigned,
    const float occlusionDbActual, const AcousticsObjectParams& objectParams, FWaveInstance* InOutWaveInstance)
{
    float wetLoudnessPowerDbInitial = objectParams.TritonParams.Wet.LoudnessDb + objectParams.Design.WetnessAdjustment +
                                      occlusionDbDesigned - occlusionDbActual;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
    // There is currently a bug in UE5.0 where even when SendStage is set to PreDistanceAttenuation, the submix
    // receives distance&occlusion-attenuated signal. We need to "undo" this gain manually if building for 5.0.
    const float ue5extraAttenAmp = InOutWaveInstance->GetDistanceAndOcclusionAttenuation();
    const float ue5extraAttenDb = AcousticsUtils::AmplitudeToDb(ue5extraAttenAmp);
    wetLoudnessPowerDbInitial -= ue5extraAttenDb;
#endif
    // Ensure wet loudness never exceeds 0dB.
    const float wetLoudnessPowerDbCapped = FMath::Min(wetLoudnessPowerDbInitial, 0.0f);

    // In order to prevent reverb from being heard outside of the simulation region, we smoothly fade-out wet
    // loudness as dry loudness goes beyond -60dB. This will ensure that no reverb is heard if no dry path is heard.
    // Note: This is not physically-accurate behavior and we should discuss better ways to properly fix this.
    constexpr float startFadeoutDb = -54.0f;
    constexpr float endFadeoutDb = -60.0f;
    constexpr float fadeRangeDb = startFadeoutDb - endFadeoutDb;
    const float ueAttenPower = InOutWaveInstance->GetDistanceAttenuation();
    const float ueAttenDb = AcousticsUtils::AmplitudeToDb(ueAttenPower);
    const float wetFadeout = (FMath::Clamp(ueAttenDb, endFadeoutDb, startFadeoutDb) - endFadeoutDb) / fadeRangeDb;
    const float wetFadeoutClamped = FMath::Clamp(wetFadeout, 0.0f, 1.0f);

    // Apply the clamped fadeout coefficient to the wet loudness.
    const float wetLoudnessDesigned = wetFadeoutClamped * AcousticsUtils::DbToAmplitude(wetLoudnessPowerDbCapped);

    const float wetOutdoornessDesigned =
        FMath::Clamp(objectParams.Outdoorness + objectParams.Design.OutdoornessAdjustment, 0.0f, 1.0f);

    const float wetDecayTimeDesigned =
        objectParams.TritonParams.Wet.DecayTimeSeconds * objectParams.Design.DecayTimeMultiplier;

    if (IsSpatialReverbInitialized())
    {
        // Need to register the SourceBufferListener each time on the source
        InOutWaveInstance->SourceBufferListener = m_SourceBufferListeners[SourceId];

        // Use Triton acoustic parameters to fill in necessary fields for spatial reverb in HrtfEngine
        HrtfAcousticParameters params = {0};
        params.Outdoorness = wetOutdoornessDesigned;
        params.Wet.LoudnessDb = AcousticsUtils::AmplitudeToDb(wetLoudnessDesigned);
        params.Wet.DecayTimeSeconds = wetDecayTimeDesigned;

        if (enablePortaling)
        {
            params.Wet.AngularSpreadDegrees = objectParams.TritonParams.Wet.AngularSpreadDegrees;
            params.Wet.WorldLockedArrivalDirection =
                m_Acoustics->TritonDirectionToHrtfEngine(objectParams.TritonParams.Wet.ArrivalDirection);
        }
        else
        {
            // If portaling is disabled, treat the wet path like the dry, and use the line-of-sight direction.
            params.Wet.AngularSpreadDegrees = 0;
            const auto sourceLineOfSightDirection = InOutWaveInstance->Location - listenerLocation;
            const FVector transformedDirection =
                AcousticsUtils::UnrealDirectionToHrtfEngine(sourceLineOfSightDirection);
            params.Wet.WorldLockedArrivalDirection =
                VectorF(transformedDirection.X, transformedDirection.Y, transformedDirection.Z);
        }
        m_SpatialReverb->SetHrtfParametersForSource(SourceId, &params);
    }
    // For rendering the stereo reverb with our bank of convolution reverbs
    else if (m_ReverbType == EAcousticsReverbType::StereoConvolution && m_IsStereoReverbInitialized)
    {
        auto settings = GetDefault<UAcousticsSourceDataOverrideSettings>();

        // Calulate the reverb bus weights based on the Triton reverb time
        m_ReverbBusDecayTimes[0] = settings->ShortReverbLength;
        m_ReverbBusDecayTimes[1] = settings->MediumReverbLength;
        m_ReverbBusDecayTimes[2] = settings->LongReverbLength;
        m_Acoustics->CalculateReverbSendWeights(
            wetDecayTimeDesigned,
            m_ReverbBusDecayTimes.Num(),
            m_ReverbBusDecayTimes.GetData(),
            m_ReverbBusWeights.GetData());

        // Mix the gain between outdoor and indoor, apply a gain boost to match loudness of spatial reverb.
        constexpr float stereoReverbGainBoost = 2.8f;
        float outdoorGain = stereoReverbGainBoost * wetLoudnessDesigned * wetOutdoornessDesigned;
        float indoorGain = stereoReverbGainBoost * wetLoudnessDesigned * (1.0f - wetOutdoornessDesigned);

        // Update reverb submix buses based on gains
        m_ShortIndoorSubmixSend.SendLevel = m_ReverbBusWeights[0] * indoorGain;
        m_MediumIndoorSubmixSend.SendLevel = m_ReverbBusWeights[1] * indoorGain;
        m_LongIndoorSubmixSend.SendLevel = m_ReverbBusWeights[2] * indoorGain;

        m_ShortOutdoorSubmixSend.SendLevel = m_ReverbBusWeights[0] * outdoorGain;
        m_MediumOutdoorSubmixSend.SendLevel = m_ReverbBusWeights[1] * outdoorGain;
        m_LongOutdoorSubmixSend.SendLevel = m_ReverbBusWeights[2] * outdoorGain;

        // Add reverb submix buses to the WaveInstance object
        InOutWaveInstance->SoundSubmixSends.Add(m_ShortIndoorSubmixSend);
        InOutWaveInstance->SoundSubmixSends.Add(m_MediumIndoorSubmixSend);
        InOutWaveInstance->SoundSubmixSends.Add(m_LongIndoorSubmixSend);
        InOutWaveInstance->SoundSubmixSends.Add(m_ShortOutdoorSubmixSend);
        InOutWaveInstance->SoundSubmixSends.Add(m_MediumOutdoorSubmixSend);
        InOutWaveInstance->SoundSubmixSends.Add(m_LongOutdoorSubmixSend);
    }
}
