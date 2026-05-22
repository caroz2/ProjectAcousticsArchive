// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//! \file HrtfApi.h
//! \brief API allowing access to the HrtfEngine
//!
//! The HrtfEngine is designed as a multisource spatial and reverberation mixing engine. It accepts multiple discrete
//! sources as input, and produces a single multichannel audio stream as output. This header file defines the APIs
//! available for interacting with the HrtfEngine
//!

#pragma once

#include "HrtfApiTypes.h"

#ifdef __cplusplus
extern "C"
#endif
{
    //! Initializes the HrtfEngine. There can only be one instance of the engine active at a time.
    //! \param  maxSources   The maximum number of simultaneous audio sources the HrtfEngine will need to support
    //! \param engineType The type of spatialization to be performed
    //! \param framesPerBuffer number of audio frames per buffer
    //! \param handle Outpointer returning handle to engine instance
    //! \return True if initialization succeeded.
    //! If the engine is already initialized when HrtfEngineInitialize is called, returns true without doing any real
    //! work. Returns false if initializing the engine fails for any reason
    bool EXPORT_API HrtfEngineInitialize(
        uint32_t maxSources, HrtfEngineType engineType, uint32_t framesPerBuffer, ObjectHandle* handle);

    //! Uninitializes the HrtfEngine, and frees all memory associated with it.
    //! \param handle Handle to the engine instance to be uninitialized
    //! If the HrtfEngine was not previously initialized, does nothing
    void EXPORT_API HrtfEngineUninitialize(ObjectHandle handle);

    //! Processes the provided audio data through the HRTF engine
    //! \param handle Handle for the engine instance used for this call.
    //! \param  input   An array of input buffers. Must be as long as the maxSources specified in HrtfEngineInitialize
    //! \param  count   The number of input buffers provided. Must be equal to maxSources specified in
    //! HrtfEngineInitialize
    //! \param  outputBuffer Float array to contain the output produced by HRTF processing. Format is interleaved,
    //! 48kHz, 32-bit float, PCM.
    //! \param  outputBufferLength The length of the outputBuffer.
    //!         Must be equal to (frames per buffer * number of output channels),
    //!         Where frames per buffer is the value that was passed in to HrtfEngineInitialize
    //! \return The number of samples produced. If the HrtfEngine was not initialized, or if there was an error,
    //! returns 0.
    uint32_t EXPORT_API HrtfEngineProcess(
        ObjectHandle handle, HrtfInputBuffer* input, uint32_t count, float* outputBuffer, uint32_t outputBufferLength);

    //! Returns the number of output channels for the currently-initialized engine type.
    //! \param handle Handle for the engine instance used for this call.
    //! \param numOutputChannels The number of output channels. Defaults to 2 (stereo), unless using spatial reverb.
    //! \return Returns true if successful, else false.
    bool EXPORT_API HrtfEngineGetNumOutputChannels(ObjectHandle handle, uint32_t* numOutputChannels);

    //! Returns 3-D spatial directions for each of the engine's output channels. Uses the Windows coordinate system (+x
    //! right, +y up, +z backward). Defaults to [[-1,0,0],[1,0,0]], for the left and right ears, unless using spatial
    //! reverb.
    //! \param handle Handle for the engine instance used for this call.
    //! \param outputChannelDirections Pointer to a pre-allocated array to store the coordinates.
    //! \param numOutputChannelDirections Number of elements in the pre-allocated array.
    //! \return Returns true if successful, else false.
    bool EXPORT_API HrtfEngineGetOutputChannelSpatialDirections(
        ObjectHandle handle, VectorF* outputChannelDirections, uint32_t numOutputChannelDirections);

    //! Returns state on whether the engine has reverb tails left to process
    //! \param handle Handle for the engine instance used for this call.
    //! \param hasSourceTailRemainingArray Array of bools. Engine will set to 'true' if there is source-specific tail
    //! left
    //!                               for that source index. Is set to 'false' otherwise.
    //! \param sourceCount            Length of array hasSourceTailRemaining. Must be equal to maxSources specified in
    //!                               HrtfEngineInitialize.
    //! \param hasEngineTailRemaining Out-param. Engine will set to 'true' if the source-generic buffers contain
    //!                               reverb tail. Is set to 'false' otherwise.
    //! \return Returns false on argument validation failure or if the engine was not initialized. Returns true
    //! otherwise.
    bool EXPORT_API HrtfEngineGetHasReverbTailRemaining(
        ObjectHandle handle, bool* hasSourceTailRemainingArray, uint32_t sourceCount, bool* hasEngineTailRemaining);

    //! Allocates the memory required to process a single HRTF source.
    //! \param handle Handle for the engine instance used for this call.
    //! \param index   The specific source for which to allocate resources
    //! \return True if allocation succeeded. False if HrtfEngine is not initialized, or if allocation failed, or if
    //! index was out of range.
    bool EXPORT_API HrtfEngineAcquireResourcesForSource(ObjectHandle handle, uint32_t index);

    //! Frees the memory required to process a single HRTF source
    //! If the specified source was not initialized, or is out of range, does nothing
    //! \param handle Handle for the engine instance used for this call.
    //! \param index   The specific source for which to release resources
    void EXPORT_API HrtfEngineReleaseResourcesForSource(ObjectHandle handle, uint32_t index);

    //! Frees the memory required to process all HRTF sources
    //! If the sources are not initialized, or are out of range, does nothing
    //! \param handle Handle for the engine instance used for this call.
    void EXPORT_API HrtfEngineReleaseAllSourceResources(ObjectHandle handle);

    //! Resets the processing history for the specified source on the next processing pass.
    //! \param handle Handle for the engine instance used for this call.
    //! \param index   The specific source to reset
    void EXPORT_API HrtfEngineResetSource(ObjectHandle handle, uint32_t index);

    //! Resets the processing history for all the sources in addition to filter resources in the panning engine.
    //! \param handle Handle for the engine instance used for this call.
    void EXPORT_API HrtfEngineResetAllSources(ObjectHandle handle);

    //! Updates the AcousticParameters for the specified source
    //! \param handle Handle for the engine instance used for this call.
    //! \param index   The specific source for which to set parameters
    //! \param acousticParameters   The parameters to set for this source
    //! \return True if parameters were successfully updated, otherwise false.
    //! Reasons for failure include: The HrtfEngine was not initialized, the index was out of range, the supplied
    //! parameters were invalid.
    bool EXPORT_API HrtfEngineSetParametersForSource(
        ObjectHandle handle, uint32_t index, const HrtfAcousticParameters* acousticParameters);

    //! Changes the output format for the panning engine. Not applicable for other engines. Can be changed on each
    //! frame.
    //! \param handle Handle for the engine instance used for this call. \param format The requested output
    //! format.
    //! \return True if setting is sucessful. False if engine doesn't support the requested format (e.g. if
    //! non-stereo is set on a binaural engine).
    bool EXPORT_API HrtfEngineSetOutputFormat(ObjectHandle handle, HrtfOutputFormat format);
}

#ifdef __cplusplus
//! RAII helper for C++: Deletion functor
struct HrtfEngineDeleter
{
    //! Accepts an object handle and uninitializes it
    //! \param p Handle to uninitialize
    void operator()(ObjectHandle p)
    {
        HrtfEngineUninitialize(p);
    }
};
//! RAII helper for C++.
typedef UniqueObjectHandle<HrtfEngineDeleter> HrtfEngineHandle;
#endif
