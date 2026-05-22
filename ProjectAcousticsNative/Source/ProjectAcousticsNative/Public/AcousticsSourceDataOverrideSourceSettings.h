// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "AcousticsDesignParams.h"
#include "AcousticsSourceDataOverrideSourceSettings.generated.h"

/**
 * Structure that contains the various per-source settings that can be tweaked in the Project Acoustics Source Data Override
 * plugin
 */
USTRUCT(BlueprintType, Category = "Acoustics")
struct FAcousticsSourceSettings
{
    GENERATED_BODY()

    /**
     *	Whether the acoustics design params can be overriden by acoustics runtime volumes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    bool ApplyAcousticsVolumes = true;

    /**
     *	The acoustics design params
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FAcousticsDesignParams DesignParams;

    /**
     * Whether the spatialization should be driven by Project Acoustics propagation. This will change arrival direction
     * based on geometry
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Acoustics",
        meta = (DisplayName = "Enable Portaling"))
    bool EnablePortaling = true;

    /**
     * Whether occlusion should be driven by Project Acoustics propagation
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Acoustics",
        meta = (DisplayName = "Enable Occlusion"))
    bool EnableOcclusion = true;

    /**
     * Enable reverb based on Project Acoustic simulated reverb times
     */
    UPROPERTY(
        GlobalConfig, BlueprintReadWrite, EditAnywhere, Category = "Acoustics", meta = (DisplayName = "Enable Reverb"))
    bool EnableReverb = true;

    /** When set, this emitter's sound will be affected by dynamic openings at additional CPU cost.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    bool ApplyDynamicOpenings = false;

    /**
     * All acoustic queries perform interpolation from a set of receiver samples.In some cases,
     * the receiver samples for a query will have large differences in their acoustic parameters.
     * This will cause the interpolator to not know what to do, as it most likely means the receiver
     * samples are located in different rooms and it would be unwise to use all values in the interpolation pass.
     * When the interpolator gets into this state, the acoustic query will fail.
     * Set the Resolver to an appropriate mode to inform the interpolator what to do in these cases so that the
     * query may succeed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics|Interpolation")
    AcousticsInterpolationDisambiguationMode Resolver = AcousticsInterpolationDisambiguationMode::Default;

    /**
     * If resolver is set to AcousticsInterpolationDisambiguationMode::Push, this vector represents
     * the direction to push towards to help resolve queries that would otherwise fail because of
     * interpolation not knowing which set of acoustic parameters to use.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics|Interpolation")
    FVector PushDirection = FVector::ZeroVector;

    /** Show acoustic parameters in-editor */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics|Debug Controls")
    bool ShowAcousticParameters = false;
};

/**
* Share per-source settings that can be saved to your Source Data Override Attenuation Settings
*/
UCLASS(AutoExpandCategories = (Settings))
class PROJECTACOUSTICSNATIVE_API UAcousticsSourceDataOverrideSourceSettings
    : public USourceDataOverridePluginSourceSettingsBase
{
    GENERATED_BODY()

public:
    UAcousticsSourceDataOverrideSourceSettings();

    // The shared per-source settings for Project Acoustics sound sources
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics",
        meta = (DisplayName = "Source Settings", ShowOnlyInnerProperties))
    FAcousticsSourceSettings Settings;

#if WITH_EDITOR
    virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif
};
