// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//! \file HrtfApiTypes.h
//! \brief Types to be used with the HrtfEngine

#pragma once
#include <stdint.h>
#include "AcousticsSharedTypes.h"

//! A list of gain values for each frequency band
typedef struct
{
    //! Gain in dB for band centered at 250Hz
    float g_250HzDb;
    //! Gain in dB for band centered at 500Hz
    float g_500HzDb;
    //! Gain in dB for band centered at 1kHz
    float g_1kHzDb;
    //! Gain in dB for band centered at 2kHz
    float g_2kHzDb;
    //! Gain in dB for band centered at 4kHz
    float g_4kHzDb;
    //! Gain in dB for band centered at 8kHz
    float g_8kHzDb;
    //! Gain in dB for band centered at 16kHz
    float g_16kHzDb;
} FrequencyBandGainsDb;
//! Define the number of bands
#define HRTF_NUM_FREQUENCY_BANDS (sizeof(FrequencyBandGainsDb) / sizeof(float))

//! A container for a single source of audio data that will be processed by the HrtfEngine
typedef struct HrtfInputBuffer
{
    //! Pointer to the input audio buffer. Audio must be 32bit float, PCM, mono, 48KHz
    float* Buffer;
    //! Length of the audio buffer. Must be >= 1024 samples
    uint32_t Length;
} HrtfInputBuffer;

//! Perceptual description of the listener's experience of a single audio source
//! Follows right-handed Windows coordinate system, +x right, +y up, and +z backwards
typedef struct
{
    //! The shortest source-to-listener distance (in meters), potentially including both geometry and user input.
    float EffectiveSourceDistance;

    //! The direction that a sound source should be perceived as coming from relative to the listener's head.
    VectorF PrimaryArrivalDirection;
    //! The gain in dB on the primary arrival direction caused by scene geometry.
    //! If not simulating geometry, leave at 0
    float PrimaryArrivalGeometryPowerDb;
    //! The gain in dB on the primary arrival direction caused by propagation distance.
    //! If not simulating distance, leave at 0
    float PrimaryArrivalDistancePowerDb;

    //! The direction of the fully occluded sound source. If not desired, set to 0,0,0.
    VectorF SecondaryArrivalDirection;
    //! The gain in dB on the secondary arrival direction caused by scene geometry.
    //! If not simulating geometry, leave at 0
    float SecondaryArrivalGeometryPowerDb;
    //! The gain in dB on the secondary arrival direction caused by propagation distance.
    //! If not simulating distance, leave at 0
    float SecondaryArrivalDistancePowerDb;

    //! A measure of the extent to which the current listener is outdoors. [0,1] 0 meaning indoors, 1 meaning outdoors.
    float Outdoorness;

    //! Propagation parameters for diffuse reverberation
    struct WetParams
    {
        //! The initial root-mean-square (RMS) power of reverberation, in dB. Models how the reverb
        //! loudness varies in detail in complex scenes, such as remaining high through the
        //! length of a tunnel, or increasing due to reflective material, and how wet-to-dry ratio
        //! increases as listener walks away from a source in a room.
        //!
        //! Typical usage: Drive your reverb bank with this gain. Assuming that the
        //! filters have their volume adjusted so their power envelope starts at an RMS gain of 1.
        float LoudnessDb;

        //! The average world direction from which various reverberant paths arrive at the listener.
        //! Will respond to intervening environmental features like portals or obstructions.
        //!
        //! Typical usage: Spatialize the reverberation for the sound source in this direction.
        ATKVectorF WorldLockedArrivalDirection;

        //! Perceived width of reverberation, in degrees; Varies continuously with 0 indicating
        //! localized reverberation such as heard through a small window, 360 means fully immersive
        //! isotropic reverb in the center of a room.
        //!
        //! Typical usage: Spatialize the reverb for the sound source with this angular width
        //! around the provided WorldLockedArrivalDirection parameter.
        float AngularSpreadDegrees;

        //! The reverberation time: duration in seconds, that it takes for reverb to decay by 60dB.
        //!
        //! Typical usage: Set the reverb bank to render this decay time for the sound source.
        float DecayTimeSeconds;
    };

    //! Propagation parameters for diffuse reverberation, typically rendered as the
    //! "wet" or "reverb" component in audio engines.
    WetParams Wet;

} HrtfAcousticParameters;

//! Method of spatialization
enum HrtfEngineType
{
    //! Use HRTF-based binaural processing for spatialization and reverberation
    HrtfEngineType_Binaural = 0,
    //! Use VBAP-panning for multi-channel spatialization and reverberation
    HrtfEngineType_Panner,
    //! Only do reverb - does not render direct path at all
    HrtfEngineType_ReverbOnly,
    //! Only do panning - no reverb at all
    HrtfEngineType_PannerOnly,
    //! Use FLEX (Fast, Layered, and EXpandable) high quality binaural processing, w/ reverb.
    HrtfEngineType_FlexBinaural_High,
    //! Use FLEX low quality binaural processing, w/ reverb.
    HrtfEngineType_FlexBinaural_Low,
    //! Use FLEX high quality binaural processing, w/o reverb.
    HrtfEngineType_FlexBinaural_High_NoReverb,
    //! Use FLEX low quality binaural processing, w/o reverb.
    HrtfEngineType_FlexBinaural_Low_NoReverb,
    //! Only do FLEXverb - does not render direct path at all.
    HrtfEngineType_FlexReverbOnly,
    //! Use spatial reverb (no dry) [high-quality spatial resolution].
    HrtfEngineType_SpatialReverbOnly_High,
    //! Use spatial reverb (no dry) [low-quality spatial resolution].
    HrtfEngineType_SpatialReverbOnly_Low,
    //! The total number of engine types.
    HrtfEngineType_Count,
};

//! Output channel format for spatialization
enum HrtfOutputFormat
{
    //! Single-channel mixdown
    HrtfOutputFormat_Mono = 0,
    //! Stereo mix-down
    HrtfOutputFormat_Stereo,
    //! Quadraphonic 4.0 loudspeaker locations
    HrtfOutputFormat_Quad,
    //! Standard 5.0 loudspeaker locations (no LFE)
    HrtfOutputFormat_5,
    //! Dolby standard 5.1 loudspeaker locations
    HrtfOutputFormat_5dot1,
    //! Dolby standard 7.1 loudspeaker locations
    HrtfOutputFormat_7dot1,
    //! Total number of formats; can be used to represent 'unknown' or 'unsupported' format
    HrtfOutputFormat_Count
};
