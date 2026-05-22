// Copyright (c) 2022 http://www.microsoft.com. All rights reserved.
//
// Triton Runtime API Entry-point
// Nikunjr, 1/16/2013
#pragma once

#include "TritonApiTypes.h"
#include "TritonHooks.h"
#include "TritonVector.h"

namespace TritonRuntime
{
    struct TritonStats
    {
        int ProbesInRAM;
        int ProbesLoaded;
        int ProbesLoadFailed;
        int ProbesUnloaded;
        int ProbesPendingLoad;
        int ProbesPendingUnload;

        int NumQueries;
        int NumFailed;
        int NumStreamingFailed;

        float AvgQueryTime;
        float MaxQueryTime;
        float StdDevQueryTime;
    };

    //! A small struct for the user to tell us what type of interpolation they want us to do
    struct InterpolationConfig
    {
        //! Options for specifying what the interpolator should do in cases where candidate receiver samples
        //! are very different from each other and we can't resolve automatically.
        enum class DisambiguationMode : uint8_t
        {
            //! Use the built-in resolution algorithm. Is tuned for a balance between safety
            //! (doesn't accidentally use results across geometric boundaries) and usability (doesn't fail too often)
            Default = 0,
            //! Do nothing - fail the query.
            None,
            //! Average all candidate samples. Use with caution - this will cause parameter blending across geometry boundaries.
            Blend,
            //! Use the sample closest to the query location
            Nearest,
            //! Use the loudest candidate sample
            Loudest,
            //! Use the quietest candidate sample
            Quietest,
            //! Use the sample closest to the direction of the provided push vector
            Push
        };

        //! The disambiguation mode to be used to resolve acoustic parameter contrast.
        DisambiguationMode Resolver;
        //! Only valid if Resolver == Push. The direction to push towards.
        //! Need not be normalized, internal code normalizes prior to use.
        Triton::Vec3f PushVector;

        InterpolationConfig() : Resolver(DisambiguationMode::Default), PushVector(Triton::Vec3f::Zero())
        {
        }
        InterpolationConfig(DisambiguationMode m, Triton::Vec3f pv = Triton::Vec3f::Zero())
        {
            Resolver = m;
            PushVector = pv;
        }
    };

    /// <summary> Public API for Triton Runtime </summary>
    class TritonAcoustics
    {
    protected:
        class TritonAcousticsImpl* _Impl;
        TritonAcoustics();
        virtual ~TritonAcoustics();

    public:
        /// <summary>
        /// Performs global initializations for Triton, optionally
        /// setting user-supplied hooks.
        /// </summary>
        /// <remarks>
        /// Either argument can be nullptr, in which case that argument is ignored, and Triton
        /// will use internal defaults. This function should be the first to be called before any
        /// other Triton functions. Calling it thereafter, or multiple times, is illegal,
        /// unless TearDown() has been called. The general usage is to call this function,
        /// then create TritonAcoustics objects etc., clean them up,
        /// and then at the end of it all, call TearDown().
        /// </remarks>
        ///
        /// <param name="MemHook">
        /// Optional memory hook -- if provided, all Triton heap allocs/frees
        /// will be routed through this hook. It is very important
        /// that this hook be thread-safe.
        ///
        ///   <list type = "number">
        ///   <item><description>
        ///   The caller is responsible for cleaning up the hook object
        ///   after calling TearDown().
        ///   </description></item>
        ///   <item><description>
        ///   If you don't provide a TaskHook, Triton will internally use
        ///   std::thread which will cause a few-byte (16 bytes on VS 2015/2017)
        ///   allocation on the global heap instead of this hook,
        ///   on each call to InitLoad(). The C++ standard makes this unavoidable.
        ///   If this is unacceptable, provide your own TaskHook in InitLoad()
        ///   </description></item>
        /// </param>
        /// <param name="LogHook">
        /// Optional log hook -- if provided, all Triton debug messages will be
        /// routed through this hook.  NOTE: The caller is still
        /// responsible for cleaning up the hook object after Triton's
        /// TearDown().
        /// </param>
        /// <returns> true if success, false if its called illegally. </returns>

        static bool Init(ITritonMemHook* MemHook = nullptr, ITritonLogHook* LogHook = nullptr);

        /// <summary> Tear down Triton </summary>
        /// <remarks>
        /// This function will typically be called during shutdown for the game, and
        /// will clear any internal global allocations done by Triton. It also restores
        /// Triton to pristine uninitialized state, so it is valid to call Init()
        /// thereafter. It is important that all Triton objects be deleted before
        /// calling TearDown().
        /// </remarks>

        static void TearDown();

        /// <summary> Construct an instance of Triton. </summary>
        /// <remarks> This function should only be called after Init(). </remarks>

        static TritonAcoustics* CreateInstance();

        /// <summary> Destroy an instance of Triton. </summary>
        /// <remarks> Call this before TearDown(), never after. </remarks>

        static void DestroyInstance(TritonAcoustics* Instance);

        /// <summary> Initializes loading of acoustic data for game level. </summary>
        ///
        /// <param name="IO">
        /// An implementation of TritonIOHook for reading precomputed Triton data.
        /// NOTE: The caller is responsible for cleaning up the hook object.
        /// Cleanup should only be done after calling Clear() on the current map.
        /// </param>
        /// <param name="TaskHook">
        /// An implementation of TritonAsyncTaskHook for asynchronous data loading.
        /// Can be nullptr, in which case C++ std::thread is used internally.
        /// NOTE: The caller is responsible for cleaning up the hook object.
        /// Cleanup should only be done after calling Clear() on the current map.
        /// </param>
        /// <param name="CacheScale">
        /// Controls RAM/CPU tradeoff. Values smaller than 1 make cache smaller,
        /// increasing potential CPU use.
        /// </param>
        /// <param name="requiredParameters">
        /// Specifies which parameter types must exist in the ACE file for correct game-side logic.
        /// If any of these parameter types are missing from the ACE file, initialization fails.
        /// </param>
        /// <returns> true if it succeeds, false if it fails. </returns>
        
        bool InitLoad(
            ITritonIOHook* IO, ITritonAsyncTaskHook* TaskHook = nullptr, float CacheScale = 1.0f,
            TritonParamFlags requiredParameters = TritonParamFlags::All);

        /// <summary> Initializes loading of acoustic data for game level. </summary>
        ///
        /// <param name="IOs">
        /// An array of implementations of TritonIOHooks for reading precomputed Triton data.
        /// The data from each of the IOHooks will be concatenated into a single Triton instance.
        /// NOTE: The caller is responsible for cleaning up the hook objects.
        /// Cleanup should only be done after calling Clear() on the current map.
        /// </param>
        /// <param name="numIOs">
        /// The number of IOHooks in the IOs array.
        /// </param>
        /// <param name="TaskHook">
        /// An implementation of TritonAsyncTaskHook for asynchronous data loading.
        /// Can be nullptr, in which case C++ std::thread is used internally.
        /// NOTE: The caller is responsible for cleaning up the hook object.
        /// Cleanup should only be done after calling Clear() on the current map.
        /// </param>
        /// <param name="CacheScale">
        /// Controls RAM/CPU tradeoff. Values smaller than 1 make cache smaller,
        /// increasing potential CPU use.
        /// </param>
        /// <param name="requiredParameters">
        /// Specifies which parameter types must exist in the ACE file for correct game-side logic.
        /// If any of these parameter types are missing from the ACE file, initialization fails.
        /// </param>
        /// 
        /// <returns>    true if it succeeds, false if it fails. </returns>
        bool InitLoadMultiple(
            ITritonIOHook** IOs, size_t numIOs, ITritonAsyncTaskHook* TaskHook = nullptr, float CacheScale = 1.0f,
            TritonParamFlags requiredParameters = TritonParamFlags::All);

        /// <summary> Initializes loading of acoustic data for game level. </summary>
        ///
        /// <param name="EncodedDataFile"> Filename to load precomputed Triton data from. </param>
        /// <param name="CacheScale">
        /// Controls RAM/CPU tradeoff. Values smaller than 1 make cache smaller,
        /// increasing potential CPU use.
        /// </param>
        /// <param name="requiredParameters">
        /// Specifies which parameter types must exist in the ACE file for correct game-side logic.
        /// If any of these parameter types are missing from the ACE file, initialization fails.
        /// </param>
        ///
        /// <returns>    true if it succeeds, false if it fails. </returns>
        bool InitLoad(const char* EncodedDataFile, float CacheScale = 1.0f, TritonParamFlags requiredParams = TritonParamFlags::All);

        /// <summary> Initializes loading of acoustic data for game level. </summary>
        ///
        /// <param name="EncodedDataFiles"> Filenames to load precomputed Triton data from. </param>
        /// <param name="numFiles"> The number of entries in the EncodedDataFiles array </param>
        /// <param name="CacheScale">
        /// Controls RAM/CPU tradeoff. Values smaller than 1 make cache smaller,
        /// increasing potential CPU use.
        /// </param>
        /// <param name="requiredParameters">
        /// Specifies which parameter types must exist in the ACE file for correct game-side logic.
        /// If any of these parameter types are missing from the ACE file, initialization fails.
        /// </param>
        ///
        /// <returns>    true if it succeeds, false if it fails. </returns>
        bool InitLoadMultiple(
            const char** EncodedDataFiles, size_t numFiles, float CacheScale = 1.0f,
            TritonParamFlags requiredParameters = TritonParamFlags::All);

        /// <summary> Restores instance to pristine state. Removes all acoustic data
        /// and metadata for current scene, and closes any open IO stream(s).
        /// </summary>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>

        bool Clear();

        /// <summary> Loads Triton data in a region around given point. </summary>
        ///
        /// <remarks> This method is useful for multiple scenarios.
        ///
        /// 1. Quickly load data around a player spawn point synchronously
        ///    and then let the remaining map data load asynchronously using LoadAll()
        ///
        /// 2. Pair LoadRegion() with UnloadRegion() to implement
        ///    a streaming system with fine-grained control on what regions
        ///    of the Triton map are actually loaded into RAM.
        ///
        /// In case of multiple LoadRegion() calls, they are prioritized in
        /// most-recent-first (Stack) order
        ///
        /// Also, note that Triton tries hard to avoid IO/delete operations when their
        /// combined effect will cancel out. For example,
        /// if a probe is to be unloaded as per an async Unload*() call and before it
        /// is actually unloaded, a Load*() call asks to load the same probe,
        /// no IO or deletion will be performed. Also, loading a probe that is already
        /// in RAM has no effect.
        /// 
        /// If multiple ACE files are loaded simulataneously, LoadRegion will work nicely
        /// across all the files, operating on a union of all datasets.
        ///
        /// Example usage: to implement a simple streaming system with a single player,
        /// call an async unload on a box around the player's previous position, and
        /// then immediately call a load with a box around the new position. The latter
        /// can be blocking or non-blocking as desired. The effect is to leave the probes
        /// in the overlap region of old and new boxes unmodified with no IO/mem ops.
        ///
        /// </remarks>
        ///
        /// <param name="RegionCenter"> The region center </param>
        /// <param name="RegionLength"> The region's size </param>
        /// <param name="UnloadOutside"> Unloads any loaded data outside input region </param>
        /// <param name="ShouldBlock"> If set to true, block until IO completes including
        ///  all past load requests. </param>
        ///
        /// <returns>    Number of probes whose data will be loaded. -1 on failure. </returns>

        int LoadRegion(const Triton::Vec3d& RegionCenter, const Triton::Vec3d& RegionLength, bool UnloadOutside, bool ShouldBlock);

        /// <summary> Loads all data in file(s). </summary>
        ///
        /// <remarks> Equivalent effect as calling LoadRegion() with world bounds.
        ///  This method can be used to launch a load for all data -- redundant
        ///  data already loaded from prior Load* calls will not be reloaded,
        ///  saving IO. For asynchronous, non-blocking IO, set ShouldBlock to false.
        /// </remarks>
        ///
        /// <param name="ShouldBlock"> If set to true, function will block until IO completes.
        /// </param>
        /// <returns>    true if it succeeds, false if it fails. </returns>

        bool LoadAll(bool ShouldBlock);

        /// <summary> Unloads Triton data in a region around given point. </summary>
        ///
        /// <remarks> This function lets one unload a specific region of the world.
        ///  When called asynchronously, unload operations are interleaved
        ///  with load operations. That is, one probe is unloaded and a new
        ///  probe immediate loaded after that. Since all probes take similar
        ///  amount of memory, usually this ensures fast memory allocs and
        ///  a reasonably constant mem footprint for Triton. In case you
        ///  want all memory represented by this region to be released
        ///  immediately, call with ShouldBlock = true. This will ensure all
        ///  probes that are not currently in use will be immediately
        ///  deallocated before returning from the function.
        /// 
        /// Works nicely in cases where multiple ACE files are loaded simultaneously, operating on a
        /// union of all datasets.
        /// </remarks>
        ///
        /// <param name="RegionCenter"> The region center </param>
        /// <param name="RegionLength"> The region size </param>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>

        bool UnloadRegion(const Triton::Vec3d& RegionCenter, const Triton::Vec3d& RegionLength, bool ShouldBlock);

        /// <summary> Unloads all acoustic data. </summary>
        ///
        /// <remarks> Equivalent effect as calling UnloadRegion() with world bounds.
        /// </remarks>
        ///
        /// <param name="ShouldBlock"> true if should block. </param>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>

        bool UnloadAll(bool ShouldBlock);

        //! Add a dynamic opening that can be considered by acoustic queries for
        //! dynamic occlusion of sound propagating through it. The opening is always
        //! a 2D convex polygon safely covering the open area of e.g., a door or window.
        //! \param OpeningID Unique identifier for opening to create. If identifier is already in use, call will fail.
        //! \param Center Reference point within the opening near where the acoustic probe is located.
        //! \param Normal Surface normal of opening. Sidedness doesn't matter: passing the normal or its negative will
        //! not change behavior.
        //! \param NumVertices Number of vertices.
        //! \param Vertices Locations of vertices. They must all lie within a common plane and form a convex polygon.
        //! Vertices must be listed in sequential ordering going around the polygon.
        //! Particular winding order (Clockwise or CCW) doesn't matter. Caller can safely delete array after call
        //! completes.
        //! \return true on success, false on failure
        bool AddDynamicOpening(
            uint64_t OpeningID, const Triton::Vec3d& Center, const Triton::Vec3f& Normal, int NumVertices,
            const Triton::Vec3f* Vertices);

        //! Remove dynamic opening. This can be used to explicitly control the set of dynamic openings to be
        //! considered by Triton based on game logic.
        //! \param OpeningID Unique identifier for opening to create.
        //! \return true on success, false on failure, such as if the opening ID is not present.
        bool RemoveDynamicOpening(uint64_t OpeningID);

        //! Update information about dynamic region. This will have a dynamic effect on the returned values from
        //! QueryAcoustics.
        //! \param OpeningID Identifies which dynamic opening is being modified
        //! \param AttenDirectDb The amount of attenuation the region should cause on the initial direct path. 0dB means
        //! no attenuation.
        //! \param AttenReflectionsDb The amount of attenuation the region should cause on the indirect
        //! reflected paths. 0dB means no attenuation.
        //! \return true on success, otherwise false
        bool UpdateDynamicOpening(uint64_t OpeningID, float AttenDirectDb, float AttenReflectionsDb);

        /// <summary> Calculates environmental propagation effects between given source and
        ///  listener points. </summary>
        ///
        /// <param name="SourcePos"> Source position </param>
        /// <param name="ListenerPos"> Listener position </param>
        /// <param name="OutParameters"> The calculated acoustic parameters for global sound
        /// propagation between the source and listener through the environment. </param>
        /// <param name="interpConfig"> Optional - if provided, the interpolation configuration to use for this query.</param>
        ///
        /// <returns>    true on success, false on failure. </returns>
        ///
        /// <remarks>
        /// <list type="number">
        /// <item><description>
        ///   The coordinate system for all calls to Triton is "Maya Z Up metric."
        ///   Up is +Z, Front is +Y, right-handed with units of meters.
        ///   All input/output vectors or lengths to/from the API must be in this system.
        /// </description></item>
        ///
        /// <item><description>
        ///   If the required data for this query is not already loaded into RAM, the call will fail.
        /// </description></item>
        /// </list>
        /// </remarks>
        bool QueryAcoustics(
            const Triton::Vec3d& SourcePos, const Triton::Vec3d& ListenerPos, TritonAcousticParameters& OutParameters,
            const InterpolationConfig* interpConfig = nullptr) const;

        /// <summary> Calculates environmental propagation effects between given source and
        ///  listener points. </summary>
        ///
        /// <param name="SourcePos"> Source position </param>
        /// <param name="ListenerPos"> Listener position </param>
        /// <param name="OutParameters"> The calculated acoustic parameters for global sound
        /// propagation between the source and listener through the environment. </param>
        /// <param name="OutDynamicOpeningInfo"> Triton will apply additional attenuation due to
        /// dynamic openings that can have variable attenuation during gameplay, such as doors. Additionally,
        /// the struct will be filled with related metadata for additional game-side DSP processing.
        /// </param>
        /// <param name="interpConfig"> Optional - if provided, the interpolation configuration to use for this query.</param>
        ///
        /// <returns>    true on success, false on failure. </returns>
        ///
        /// <remarks>
        /// <list type="number">
        /// <item><description>
        ///   The coordinate system for all calls to Triton is "Maya Z Up metric."
        ///   Up is +Z, Front is +Y, right-handed with units of meters.
        ///   All input/output vectors or lengths to/from the API must be in this system.
        /// </description></item>
        ///
        /// <item><description>
        ///   If the required data for this query is not already loaded into RAM, the call will fail.
        /// </description></item>
        /// </list>
        /// </remarks>
        bool QueryAcoustics(
            const Triton::Vec3d& SourcePos, const Triton::Vec3d& ListenerPos, TritonAcousticParameters& OutParameters,
            TritonDynamicOpeningInfo& OutDynamicOpeningInfo, const InterpolationConfig* interpConfig = nullptr) const;

        /// <summary> Gets "outdoorness" at listener, a measure of the extent to which the current
        ///  listener location is outdoors.
        ///  The generated value will be 0.0 in a completely closed room and 1.0 in a
        ///  completely open space (like on the top of a dune in a desert).
        /// </summary>
        ///
        /// <param name="ListenerPos"> The listener position </param>
        /// <param name="Outdoorness"> The calculated outdoorness </param>
        ///
        /// <returns> true on success, false on failure. </returns>

        bool GetOutdoornessAtListener(const Triton::Vec3d& ListenerPos, float& Outdoorness) const;

        /// <summary> Populates an internal spherical map of distances to geometry around the given position.
        /// </summary>
        ///
        /// <param name="ListenerPos"> Listener position </param>
        ///
        /// <returns> true on success, false on failure. </returns>

        bool UpdateDistancesForListener(const Triton::Vec3d& ListenerPos);

        /// <summary> Once UpdateDistancesForListener() has been called, this function can be called multiple times
        /// to query the internal distance map for the listener location to figure out a smoothed distance
        /// to geometry in any given direction pointing away from the player.
        /// This call uses precomputed data, not real-time ray-tracing, and it costs a couple
        /// trignometric functions. Further, it computes a "soft" distance by (conceptually) shooting a
        /// soft cone with about 30 degree cone angle all over the sphere of directions. The smoothed
        /// distance for each cone is interpolated over both angle and listener location. Distances are
        /// soft-clamped to ~150 meters.
        ///
        /// One typical use case: directional reflections around player to drive 5.1 or any other
        /// spatial reflections.
        /// </summary>
        ///
        /// <param name="Direction"> World direction. Must be unit vector. </param>
        ///
        /// <returns> true on success, false on failure. </returns>

        float QueryDistanceForListener(const Triton::Vec3f& Direction) const;

        //
        // Dev functions
        //

        /// <summary> Starts collecting internal stats, collected by a subsequent call
        /// to GetPerfStats. Call after InitLoad(), otherwise ignored.
        /// </summary>

        void StartCollectingStats() const;

        /// <summary> Gets load statistics. </summary>
        ///
        /// <returns> true if it succeeds, false if it fails. </returns>

        bool GetPerfStats(TritonStats& OutStats) const;

        /// <summary>
        /// Get the transform matrix that was pre-applied to the mesh during the prebake process
        /// It is the responsibility of the caller to re-apply this transform, if necessary
        /// </summary>
        /// <param name="preappliedTransform">the float array to copy the transform into</param>
        /// <returns></returns>
        void GetPreappliedTransform(ATKMatrix4x4* preappliedTransform) const;

        /// <summary>
        /// Triton produces per-source reverberation times (T60s), but typically you cannot instantiate
        /// a reverb filter with a dynamic T60 per source. This function helps translate from the per-source T60
        /// to a blend between the available reverb busses with known T60s to produce the desired T60
        /// </summary>
        /// <param name="targetReverbTime">The per-source reverb time to produce</param>
        /// <param name="numReverbs">The number of entries in both the reverbTimes and reverbBlendWeights arrays</param>
        /// <param name="reverbTimes">[in]The available reverb times being rendered by the audio implementation.
        /// Must be sorted from shortest in index 0 to longest in index N-1</param>
        /// <param name="reverbSendWeights">[out]The ratio of the signal to apply for each of the input busses</param>
        /// <returns>true on success, false on failure</returns>
        static bool CalculateReverbSendWeights(
            const float targetReverbTime, const uint32_t numReverbs, const float* reverbTimes,
            float* reverbSendWeights) noexcept;
    };
} // namespace TritonRuntime