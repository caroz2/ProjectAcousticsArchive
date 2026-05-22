// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//! \file TritonApiTypes.h
//! \brief Types to be used with Triton

#pragma once
#include "AcousticsSharedTypes.h"
#include <stdint.h>

#ifndef FLT_MAX
//! Define max floating point value to avoid pulling in CRT headers
#define FLT_MAX 3.402823466e+38F
#endif 


//! Use these flags at Init time to specify which parameters types are required for your usage
//! It's possible to remove parameters from ACE files via the AceManipulatorTool.
//! Can use bitwise ops to combine these however you want
enum class TritonParamFlags : uint32_t
{
    //! Only useful as a default value
    Invalid = 0,
    //! Dry-path obstruction parameters
    Obstruction = 1,
    //! Dry-path arrival direction parameters
    Portalling = 1 << 1,
    //! Generic reverb parameters
    Reverb = 1 << 2,
    //! Directional/Spatial reverb parameters
    DirectionalReverb = 1 << 3,
    //! Experimental parameters (may be removed at any time)
    Experimental = 1 << 4,
    //! Use to specify that you want all parameters available
    All = UINT32_MAX
};

#pragma pack(push, 1)
//! Propagation parameters for initial sound arriving from source at listener, 
//! typically rendered as the "dry" or "direct" component in audio engines
struct DryParams
{
    //! Distance in simulation grid cells to geometry (voxel designated as "occupied").
    //! JMS: Experimental parameter -- delete me?
    float GeomDist;

    //! The shortest-path length in meters for sound to get from the audio source to listener,
    //! potentially detouring around intervening scene geometry.
    //!
    //! Typical usage: drive distance attenuation based on this value, rather 
    //! than the line-of-sight distance.
    float PathLengthMeters;

    //! This is the dB attenuation due to diffraction around the scene for the shortest
    //! path connecting source to listener. By design, this only accounts for the
    //! attenuation due to the obstruction effect of intervening geometry.
    //! Distance attenuation curves can be designed as you wish in your audio engine, 
    //! driven by PathLength parameter.
    //!
    //! Typical usage: Apply on the dry sound as an obstruction value. You can freely 
    //! interpret this into volume and/or low-pass filter. 
    //! 
    //! Note that this value can also be positive such as when the sound source is in 
    //! a corner so that there are multiple very-early reflections. You can either clamp
    //! to 0, or model an additional bass-boost filter in such cases.
    float LoudnessDb;

    //! The world direction unit vector from which the dry sound arrives at the listener.
    //! This will model "portaling," respond to intervening environmental features, 
    //! detouring around portals or obstructions.
    //!
    //! Typical usage: spatialize the sound from this direction, instead of line-of-sight
    //! direction.
    ATKVectorF ArrivalDirection;
};
#pragma pack(pop)

#pragma pack(push, 1)
//! Propagation parameters for diffuse reverberation, typically rendered as the 
//! "wet" or "reverb" component in audio engines.
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
    ATKVectorF ArrivalDirection;

    //! Perceived width of reverberation, in degrees; Varies continuously with 0 indicating 
    //! localized reverberation such as heard through a small window, 360 means fully immersive 
    //! isotropic reverb in the center of a room. 
    //! 
    //! Typical usage: Spatialize the reverb for the sound source with this angular width 
    //! around the provided ArrivalDirection parameter.
    float AngularSpreadDegrees;

    //! The reverberation time: duration in seconds, that it takes for reverb to decay by 60dB.
    //! 
    //! Typical usage: Set the reverb bank to render this decay time for the sound source. 
    float DecayTimeSeconds;
};
#pragma pack(pop)

//! An object that holds the parameters returned from QueryAcoustics calls, summarizing
//! the acoustics between dynamic source and listener location.
//! All directional information is in Triton's canonical coordinate system.
//! Triton's coordinate system is: metric, right-handed, +X goes left-to-right on screen, 
//! and +Z goes up (against direction of gravity), so that +Y goes into screen.
//! Note that since Triton computes propagation in world coordinates,
//! all its directions are locked to the world, not the listener's head.
//! This means that the user's head rotation must be applied on top of these parameters
//! to reproduce the acoustics. This is the job of the spatializer, assumed to be a separate component.
//! \image html TritonCoordinates.png
#pragma pack(push, 1)
typedef struct TritonAcousticParameters
{
    //! Special value (-FLT_MAX) that indicates failure to compute a parameter.
    //! Far outside of the normal range of parameter values.
    static constexpr float FailureCode = -FLT_MAX;

    //! Propagation parameters for initial sound arriving from source at listener
    DryParams Dry;
    //! Propagation parameters for diffuse reverberation
    WetParams Wet;

} TritonAcousticParameters;
#pragma pack(pop)

//! Optional metadata about dynamic opening reported by QueryAcoustics
#pragma pack(push, 1)
typedef struct TritonDynamicOpeningInfo
{
    //! Whether or not dynamic openings should be considered for this query
    bool ApplyDynamicOpening; 
    //! If dynamic opening processing fails internally, this will be false, true on success
    bool DidProcessingSucceed;
    //! If the initial sound goes through some opening, this will be true, false otherwise
    bool DidGoThroughOpening;
    //! If initial sound goes through an opening, this will be set to the ID of the last
    //! opening along the shortest sound path from source to listener.
    uint64_t OpeningID;
} TritonDynamicOpeningInfo;
#pragma pack(pop)

//! An object that contains information useful for debugging Acoustics
//! In addition to TritonAcousticParameters, it contains additional metadata about the source
#pragma pack(push, 1)
typedef struct TritonAcousticParametersDebug
{
    //! A unique identifier for the sound source
    int SourceId;
    //! The position of the source, in Triton coordinates
    ATKVectorD SourcePosition;
    //! The position of the listener, in Triton coordinates
    ATKVectorD ListenerPosition;
    //! Measure of outdoorness at listener location. 0 being completely indoors, 1 being completely outdoors
    float Outdoorness;
    //! The acoustic parameters returned from the most recent call to QueryAcoustics for the source
    TritonAcousticParameters AcousticParameters;
} TritonAcousticParametersDebug;
#pragma pack(pop)

//! Internal load status of a probe
enum class TritonProbeLoadState
{
    //! The probe loaded successfully
    Loaded,
    //! The probe is not currently loaded
    NotLoaded,
    //! Loading the probe failed
    LoadFailed,
    //! The probe is still being loaded
    LoadInProgress,
    //! There is no probe
    DoesNotExist,
    //! The probe is invalid
    Invalid
};

//! An object containing debug metadata for a probe
#pragma pack(push, 1)
typedef struct TritonProbeMetadataDebug
{
    //! Current loading state of this probe
    TritonProbeLoadState State;

    //! World location of this probe
    ATKVectorD Location;

    //! Minimum corner of the cubical region around this probe for which it has data
    ATKVectorD DataMinCorner;
    //! Maximum corner of the cubical region around this probe for which it has data
    ATKVectorD DataMaxCorner;
} TritonProbeMetadataDebug;
#pragma pack(pop)
