// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "UObject/Object.h"
#include "TritonApiTypes.h"
#include "TritonPublicInterface.h"
#include "AcousticsDesignParams.generated.h"

///**
// *    Structure that contains the various acoustics design params that can be tweaked to make the sound coming
// *    from the acoustics audio component to react to the surroundings.
// */
USTRUCT(BlueprintType, Category = "Acoustics")
struct FAcousticsDesignParams
{
    GENERATED_BODY()

    /** Apply a multiplier to the occlusion dB level computed from physics.
    If this multiplier is greater than 1, occlusion will be exaggerated, while values less than 1 make
    the occlusion effect more subtle, and a value of 0 disables occlusion. Normal range is between 0 - 5 */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics",
        meta = (UIMin = 0, ClampMin = 0, UIMax = 5))
    float OcclusionMultiplier = 1.0f;

    /** Adds specified dB value to reverb level computed
      from physics. Positive values make a sound more reverberant, negative values make a
      sound more dry. Normal range is between -40 and 40 dB */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics", meta = (UIMin = -40, UIMax = 40))
    float WetnessAdjustment = 0.0f;

    /** Applies a multiplier to the reverb decay time from physics.
      For example, if the bake result specifies a decay time of 500 milliseconds, but this value is set
      to 2, the decay time applied to the source is 1 second. Normal range is between 0 and 5 */
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, Category = "Acoustics",
        meta = (UIMin = 0, ClampMin = 0, UIMax = 5))
    float DecayTimeMultiplier = 1.0f;

    /** The acoustics system computes a continuous
    value between 0 and 1, 0 meaning the player is fully indoors and 1 being outdoors.
     This is an additive adjustment to this value.
     Setting this to 1 will make a source always sound completely outdoors,
     while setting it to -1 will make it always sound indoors. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics", meta = (UIMin = -1, UIMax = 1))
    float OutdoornessAdjustment = 0.0f;

    /**
     * Constant values denoting the clamps of the members of this struct.
     */
    static const float OcclusionMultiplierMin;
    static const float OcclusionMultiplierMax;
    static const float WetnessAdjustmentMin;
    static const float WetnessAdjustmentMax;
    static const float DecayTimeMultiplierMin;
    static const float DecayTimeMultiplierMax;
    static const float OutdoornessAdjustmentMin;
    static const float OutdoornessAdjustmentMax;

    //! Minimum possible values for designer input parameters
    //! \return Minimum possible values for designer input parameters
    static const FAcousticsDesignParams& Min()
    {
        static FAcousticsDesignParams v{0.0f, -40.0f, 0.0f, -1.0};
        return v;
    }

    //! Default values for designer input parameters
    //! \return Default values for designer input parameters
    static const FAcousticsDesignParams& Default()
    {
        static FAcousticsDesignParams v{1.0f, 0.0f, 1.0f, 0.0f};
        return v;
    }

    //! Maximum possible values for design input parameters
    //! \return Maximum possible values for design input parameters
    static const FAcousticsDesignParams& Max()
    {
        static FAcousticsDesignParams v{5.0f, 40.0f, 5.0f, 1.0f};
        return v;
    }

    //! Clamp all members to be within valid range
    //! \param t Object whose members will be clamped to valid range
    static void ClampToRange(FAcousticsDesignParams& t)
    {
        auto low = Min();
        auto high = Max();
        t.OcclusionMultiplier = Clamp(t.OcclusionMultiplier, low.OcclusionMultiplier, high.OcclusionMultiplier);
        t.WetnessAdjustment = Clamp(t.WetnessAdjustment, low.WetnessAdjustment, high.WetnessAdjustment);
        t.DecayTimeMultiplier = Clamp(t.DecayTimeMultiplier, low.DecayTimeMultiplier, high.DecayTimeMultiplier);
        t.OutdoornessAdjustment = Clamp(t.OutdoornessAdjustment, low.OutdoornessAdjustment, high.OutdoornessAdjustment);
    }

    //! Combine two sets of design values. "base" is modified to incorporate "other".
    //! \param base Object whose members will be modified
    //! \param other Object whose values are incorporated
    static void Combine(FAcousticsDesignParams& base, const FAcousticsDesignParams& other)
    {
        base.OcclusionMultiplier *= other.OcclusionMultiplier;
        base.WetnessAdjustment += other.WetnessAdjustment;
        base.DecayTimeMultiplier *= other.DecayTimeMultiplier;
        base.OutdoornessAdjustment += other.OutdoornessAdjustment;
    }

private:
    //! Clamp value to be between min amd max.
    //! \param v value to be clamped
    //! \param vMin Lower boundary
    //! \param vMax Upper boundary
    //! \return Clamped value
    static inline float Clamp(float v, float vMin, float vMax)
    {
        return v < vMin ? vMin : (v > vMax ? vMax : v);
    }
};

//! What should the interpolator do in cases where candidate receiver samples
//! are very different from each other and can't be resolved automatically?
UENUM(BlueprintType, Category = "Acoustics")
enum class AcousticsInterpolationDisambiguationMode : uint8
{
    //! Use the built-in resolution algorithm. Is tuned for a balance between safety
    //! (doesn't accidentally use results across geometric boundaries) and usability (doesn't fail too often)
    Default = 0,
    //! Do nothing - fail the query
    None,
    //! Blend between all available samples, even if they are on opposite sides of geometry from each other
    Blend,
    //! Prefer samples closest to the source
    Nearest,
    //! Prefer samples with the loudest acoustic parameters
    Loudest,
    //! Prefer samples with the quietest acoustic parameters
    Quietest,
    //! Use samples closest to the direction of a provided push vector
    Push
};

//! Holds perceptual acoustic parameters and design tweaks for a particular game object.
//! For more information, see the documentation for TritonAcousticParameters
struct AcousticsObjectParams
{
    //! The ID used to keep track of the game object
    uint64_t ObjectId;
    //! The TritonAcousticParameters for this voice
    TritonAcousticParameters TritonParams;
    //! The outdoorness for this voice at the current listener location. 0 = completely indoors, 1 = completely outdoors
    float Outdoorness;
    //! Per-voice design tweaks
    FAcousticsDesignParams Design;
    //! When set, this emitter's sound will be affected by dynamic openings at additional CPU cost.
    bool ApplyDynamicOpenings;
    //! Contains additional data about dynamic openings for this source
    TritonDynamicOpeningInfo DynamicOpeningInfo;
    //! Additional settings for the interpolator for this source
    TritonRuntime::InterpolationConfig InterpolationConfig;
};