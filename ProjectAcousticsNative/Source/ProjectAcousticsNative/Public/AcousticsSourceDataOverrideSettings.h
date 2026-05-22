// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "Runtime/Launch/Resources/Version.h"
#include "AcousticsSourceDataOverrideSettings.generated.h"

UENUM(BlueprintType)
enum class EReverbBusesPreset : uint8
{
    // Use default impulses responses provided by the plugin to render stereo convolution reverb
    Default UMETA(DisplayName = "Default"),
    // Provide your own impulse responses to render stereo convolution reverb
    Custom UMETA(DisplayName = "Custom")
};

UENUM(BlueprintType)
enum class ESpatialReverbQuality : uint8
{
    // Renders spatial reverb through 12 virtual speakers that are spawned in a sphere all around listener
    Best UMETA(DisplayName = "Best"),
    // Renders spatial reverb through 3 virtual speakers that are spawned in the horizontal plane
    // around listener
    Good UMETA(DisplayName = "Good")
};

UENUM(BlueprintType)
enum class EAcousticsReverbType : uint8
{
    // Directionally aware, object based reverb. Will spawn virtual speakers as actors at gametime that follow
    // the listener
    SpatialReverb UMETA(DisplayName = "Spatial Reverb"),
    // Stereo reverb using UE Convolution Reverb
    StereoConvolution UMETA(DisplayName = "Stereo Convolution"),
    // No reverb will be rendered by Project Acoustics
    None UMETA(DisplayName = "None")
};

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
constexpr EAcousticsReverbType c_DefaultAcousticsReverbType = EAcousticsReverbType::SpatialReverb;
constexpr bool c_SpatialReverbSupported = true;
#else
constexpr EAcousticsReverbType c_DefaultAcousticsReverbType = EAcousticsReverbType::StereoConvolution;
constexpr bool c_SpatialReverbSupported = false;
#endif

struct FReverbBusesInfo
{
    // Names of the reverb submixes containing impulse responses
    FString ShortIndoorReverbSubmixName;
    FString MediumIndoorReverbSubmixName;
    FString LongIndoorReverbSubmixName;

    FString ShortOutdoorReverbSubmixName;
    FString MediumOutdoorReverbSubmixName;
    FString LongOutdoorReverbSubmixName;

    // Duration of impulse responses in seconds
    float ShortReverbLength;
    float MediumReverbLength;
    float LongReverbLength;
};

UCLASS(config = Engine, defaultconfig)
class PROJECTACOUSTICSNATIVE_API UAcousticsSourceDataOverrideSettings : public UObject
{
    GENERATED_BODY()

public:
    UAcousticsSourceDataOverrideSettings();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual bool CanEditChange(const FProperty* InProperty) const override;
    virtual void PostInitProperties() override;
#endif

    /**
     *    Type of reverb to be rendered by Project Acoustics
     */
    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Reverb", meta = (DisplayName = "Reverb Type"))
    EAcousticsReverbType ReverbType = c_DefaultAcousticsReverbType;

    /**
     *    Quality for spatial reverb
     */
    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Reverb|Spatial Reverb", meta = (DisplayName = "Spatial Reverb Quality"))
    ESpatialReverbQuality SpatialReverbQuality = ESpatialReverbQuality::Best;

    /**
     *	Preset for submix buses used for reverb
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb", meta = (DisplayName = "Reverb Bus Preset"))
    EReverbBusesPreset ReverbBusesPreset;

    /**
     *    Reverb submix containing short, indoor impulse response. IRs should be ordered by length, short < medium <
     *length, each between 0-5 seconds
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset|Indoor",
        meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath ShortIndoorReverbSubmix;

    /**
     *    Reverb submix containing medium, indoor impulse response. IRs should be ordered by length, short < medium <
     *length, each between 0-5 seconds
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset|Indoor",
        meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath MediumIndoorReverbSubmix;

    /**
     *    Reverb submix containing long, indoor impulse response. IRs should be ordered by length, short < medium <
     *length, each between 0-5 seconds
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset|Indoor",
        meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath LongIndoorReverbSubmix;

    /**
     *    Reverb submix containing short, outdoor impulse response. IRs should be ordered by length, short < medium <
     *length, each between 0-5 seconds
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset|Outdoor",
        meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath ShortOutdoorReverbSubmix;

    /**
     *    Reverb submix containing medium, outdoor impulse response. IRs should be ordered by length, short < medium <
     *length, each between 0-5 seconds
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset|Outdoor",
        meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath MediumOutdoorReverbSubmix;

    /**
     *    Reverb submix containing long, outdoor impulse response. IRs should be ordered by length, short < medium <
     *length, each between 0-5 seconds
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset|Outdoor",
        meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath LongOutdoorReverbSubmix;

    /**
     *    Duration of both short impulse responses (seconds). This is the duration of the impulse response, not the
     *length of the file
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset",
        meta = (ClampMin = 0.0f, ClampMax = 5.0f, UIMin = 0.0f, UIMax = 5.0f, DisplayName = "Short Reverb Length"))
    float ShortReverbLength;

    /**
     *    Duration of both medium impulse responses (seconds). This is the duration of the impulse response, not the
     *length of the file
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset",
        meta = (ClampMin = 0.0f, ClampMax = 5.0f, UIMin = 0.0f, UIMax = 5.0f, DisplayName = "Medium Reverb Length"))
    float MediumReverbLength;

    /**
     *    Duration of both long impulse responses (seconds). This is the duration of the impulse response, not the
     *length of the file
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Reverb|Stereo Convolution Reverb|Bus Preset",
        meta = (ClampMin = 0.0f, ClampMax = 5.0f, UIMin = 0.0f, UIMax = 5.0f, DisplayName = "Long Reverb Length"))
    float LongReverbLength;

private:
    void SetReverbBuses(FReverbBusesInfo buses);

    /**
     * Duplicate of the ShortIndoorReverbSubmix property. Used to store the last assigned custom submix
     */
    UPROPERTY(GlobalConfig, meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath ShortIndoorReverbSubmixCustom;

    /**
     * Duplicate of the MediumIndoorReverbSubmix property. Used to store the last assigned custom submix
     */
    UPROPERTY(GlobalConfig, meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath MediumIndoorReverbSubmixCustom;

    /**
     * Duplicate of the LongIndoorReverbSubmix property. Used to store the last assigned custom submix
     */
    UPROPERTY(GlobalConfig, meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath LongIndoorReverbSubmixCustom;

    /**
     * Duplicate of the ShortOutdoorReverbSubmix property. Used to store the last assigned custom submix
     */
    UPROPERTY(GlobalConfig, meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath ShortOutdoorReverbSubmixCustom;

    /**
     * Duplicate of the MediumOutdoorReverbSubmix property. Used to store the last assigned custom submix
     */
    UPROPERTY(GlobalConfig, meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath MediumOutdoorReverbSubmixCustom;

    /**
     * Duplicate of the LongOutdoorReverbSubmix property. Used to store the last assigned custom submix
     */
    UPROPERTY(GlobalConfig, meta = (AllowedClasses = "/Script/Engine.SoundSubmix"))
    FSoftObjectPath LongOutdoorReverbSubmixCustom;

    /**
     *    Duplicate of the ShortReverbLength property. Used to store the last assigned custom length
     */
    UPROPERTY(GlobalConfig)
    float ShortReverbLengthCustom;

    /**
     *    Duplicate of the MediumReverbLength property. Used to store the last assigned custom length
     */
    UPROPERTY(GlobalConfig)
    float MediumReverbLengthCustom;

    /**
     *    Duplicate of the MediumReverbLength property. Used to store the last assigned custom length
     */
    UPROPERTY(GlobalConfig)
    float LongReverbLengthCustom;

    TMap<EReverbBusesPreset, FReverbBusesInfo> ReverbBusesPresetMap;
};
