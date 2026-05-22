// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//! \file TritonPreprocessorApi.h
//! \brief API allowing access to the Triton preprocessor

#pragma once
#include "AcousticsSharedTypes.h"
#include "TritonPreprocessorApiTypes.h"

#ifdef __cplusplus
extern "C"
#endif
{
    //! Callback returns preprocessing status and progress information.
    //! Note: Call any preprocessor API from the callback handler may cause a deadlock.
    //! \param message String with a status message
    //! \param progress integer from 0-100 indicating progresses
    //! \return true to cancel processing, false otherwise
    typedef bool(EXPORT_API* TritonPreprocessorCallback)(const char* message, int progress);

    //
    // Simulation Parameters Functions
    //

    //! Loads simulation control settings from a file
    //! \param filepath Path to the file containing simulation parameters
    //! \param params Simulation parameters loaded from the file
    //! \return true if successful, false otherwise
    bool EXPORT_API
    TritonPreprocessor_SimulationParameters_LoadFromFile(const char* filepath, TritonSimulationParameters* params);

    //
    // Acoustic Mesh Functions
    //

    //! Creates an empty acoustic mesh object
    //! \param instance Pointer to receive handle to the new acoustic mesh object
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_AcousticMesh_Create(TritonObject* instance);

    //! Destroys a previously created acoustic mesh object
    //! \param instance Handle to the acoustic mesh
    void EXPORT_API TritonPreprocessor_AcousticMesh_Destroy(TritonObject instance);

    //! Adds triangles to an acoustic mesh, either as acoustic geometry or acoustic navigation.
    //! \param instance Handle to the acoustic mesh object
    //! \param vertices An array containing mesh vertices
    //! \param vertexCount Count of vertices in the passed array
    //! \param trianglesInfo Array containing triangle information
    //! \param trianglesCount Size of the triangle information array
    //! \param type The MeshType to tag this geometry as
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_AcousticMesh_Add(
        TritonObject instance, ATKVectorD* vertices, int vertexCount,
        TritonAcousticMeshTriangleInformation* trianglesInfo, int trianglesCount, MeshType type);

    //! Adds volume to Acoustic Mesh that will be flood-filled with voxels
    //! \param instance Handle to the acoustic mesh object
    //! \param vertices An array containing mesh vertices
    //! \param vertexCount Count of vertices in the passed array
    //! \param trianglesInfo Array containing triangle information
    //! \param trianglesCount Size of the triangle information array
    //! \param seed The point to begin the flood fill
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_AcousticMesh_AddGeometryFillVolume(
        TritonObject instance, ATKVectorD* vertices, int vertexCount,
        TritonAcousticMeshTriangleInformation* trianglesInfo, int trianglesCount, ATKVectorD seed);

    //! Adds volume to Acoustic Mesh that overrides probe spacing metadata
    //! \param instance Handle to the acoustic mesh object
    //! \param vertices An array containing mesh vertices
    //! \param vertexCount Count of vertices in the passed array
    //! \param trianglesInfo Array containing triangle information
    //! \param trianglesCount Size of the triangle information array
    //! \param spacing The max horizontal probe spacing to apply inside this volume
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_AcousticMesh_AddProbeSpacingVolume(
        TritonObject instance, ATKVectorD* vertices, int vertexCount,
        TritonAcousticMeshTriangleInformation* trianglesInfo, int trianglesCount, float spacing);

    //! Create the current mesh from given geometry file
    //! \param meshFileName Name of FBX or OBJ geometry file.
    //! \param acousticMaterialFileName Filename of the acoustic material data
    //! \param unitAdjustment Multiplicative adjustment to scale mesh unit in meters
    //! \param sceneScale User-provided physical scaling of scene geometry to control bake cost
    //! \param meshOut Mesh object created and populated from the FBX or OBJ file
    //! \param matLibOut Material library created from the acoustic material data in AcousticMaterialFileName
    //! \return True on success, false on failure
    bool EXPORT_API TritonPreprocessor_AcousticMesh_CreateFromMeshFile(
        const char* meshFileName, const char* acousticMaterialFileName, float unitAdjustment, float sceneScale,
        TritonObject* meshOut, TritonObject* matLibOut);

    //! Add a "pinned" probe that is fixed at user-specified location
    //! \param instance Handle to the acoustic mesh object
    //! \param probeLocation Location of the pinned probe
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_AcousticMesh_AddPinnedProbe(TritonObject instance, ATKVectorD probeLocation);

    //
    // Acoustic Material Library Functions
    //

    //! Loads acoustic materials information from a JSON file
    //! \param file Path to the JSON file
    //! \param instance Pointer to receive the handle to the created library object
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_MaterialLibrary_CreateFromFile(const char* file, TritonObject* instance);

    //! Creates an acoustic materials library object from passed array of acoustic materials
    //! \param materials Array with acoustic materials information
    //! \param count Number of elements in the array
    //! \param instance Pointer to receive the handle to the created library object
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_MaterialLibrary_CreateFromMaterials(
        TritonAcousticMaterial* materials, int count, TritonObject* instance);

    //! Destroys the previously created acoustic materials library object
    //! \param instance Handle to the acoustic materials library object
    void EXPORT_API TritonPreprocessor_MaterialLibrary_Destroy(TritonObject instance);

    //! Returns the number of acoustic materials in the library
    //! \param count Pointer to receive the material count
    //! \param instance Handle to the acoustic materials library object
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_MaterialLibrary_GetCount(TritonObject instance, int* count);

    //! Return all known acoustic materials and associated codes in the library
    //! \param instance Handle to the acoustic materials library object
    //! \param materials Array to receive acoustic materials
    //! \param codes Array to receive the material codes
    //! \param count Size of arrays, must be >= the value returned by TritonPreprocessor_MaterialLibrary_GetCount
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_MaterialLibrary_GetKnownMaterials(
        TritonObject instance, TritonAcousticMaterial materials[], TritonMaterialCode codes[], int count);

    //! Returns information about acoustic material specified by its material code
    //! \param instance Handle to the acoustic materials library object
    //! \param code Material code to search
    //! \param material Pointer to receive the acoustic material info
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_MaterialLibrary_GetMaterialInfo(
        TritonObject instance, TritonMaterialCode code, TritonAcousticMaterial* material);

    //! Returns acoustic material code associated with the specified name
    //! \param instance Handle to the acoustic materials library object
    //! \param name Name of the material to search
    //! \param code Pointer to receive the material code
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_MaterialLibrary_GetMaterialCode(
        TritonObject instance, const char* name, TritonMaterialCode* code);

    //! Returns a code for material that is most similar to specified name
    //! \param instance Handle to the acoustic materials library object
    //! \param name Material mname to search
    //! \param code Pointer to receive the material code
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_MaterialLibrary_GuessMaterialCodeFromGeneralName(
        TritonObject instance, const char* name, TritonMaterialCode* code);

    //
    // Simulation Configuration Functions
    //

    //! Performs probe layout computation to create a simulation configuration
    //! \param mesh instance to acoustic mesh object to process
    //! \param simulationParams Setting affecting simulation such as frequency, simulation volume etc.
    //! \param opParams Operational settings such as prefix used for generated filenames, working directory etc.
    //! \param materialLibrary Handle to material library used for processing
    //! \param callback Callback to receive periodic status and progress information
    //! \param instance Pointer to receive handle to the simulation configuration object
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_SimulationConfiguration_Create(
        TritonObject mesh, TritonSimulationParameters* simulationParams, TritonOperationalParameters* opParams,
        TritonObject materialLibrary, TritonPreprocessorCallback callback, TritonObject* instance);

    //! Creates a Simulation Configuration for a single probe. Primary use is for individual probe analysis
    //! \param mesh instance to acoustic mesh object to process
    //! \param simulationParams Setting affecting simulation such as frequency, simulation volume etc.
    //! \param opParams Operational settings such as prefix used for generated filenames, working directory etc.
    //! \param materialLibrary Handle to material library used for processing
    //! \param probeLocation The location of the probe to use in this SimulationConfiguration
    //! \param simDuration The length, in seconds, to run the simulator. Set to -1 for auto
    //! \param callback Callback to receive periodic status and progress information
    //! \param instance Pointer to receive handle to the simulation configuration object
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_SimulationConfiguration_CreateForSingleProbe(
        TritonObject mesh, TritonSimulationParameters* simulationParams, TritonOperationalParameters* opParams,
        TritonObject materialLibrary, ATKVectorD probeLocation, float simDuration, TritonPreprocessorCallback callback,
        TritonObject* instance);

    //! Performs probe layout computation to create a simulation configuration.
    //! Mesh and acoustic material library information if loaded from files specified
    //! in the passed simulation parameters.
    //! \param simulationParams Setting to control simulation
    //! \param opParams Operational settings
    //! \param callback Callback to receive periodic status and progress information
    //! \param instance Pointer to receive handle to the simulation configuration object
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_SimulationConfiguration_CreateFromFbx(
        TritonSimulationParameters* simulationParams, TritonOperationalParameters* opParams,
        TritonPreprocessorCallback callback, TritonObject* instance);

    //! Loads a configuration from existing vox and config file
    //! \param workingDir Folder where vox and config files are located
    //! \param filename Config file name
    //! \param instance Pointer to receive handle to the simulation configuration object
    //! \return true if successful, false otherwise
    bool EXPORT_API
    TritonPreprocessor_SimulationConfiguration_CreateFromFile(const char* workingDir, const char* filename, TritonObject* instance);

    //! Destroys the previously created simultion configuration object.
    //! \param instance Handle to the simulation configuration object
    void EXPORT_API TritonPreprocessor_SimulationConfiguration_Destroy(TritonObject instance);

    //! Returns the number of probes in the current simulation.
    //! \param instance Handle to the simulation configuration object
    //! \param count Pointer to receive the probe count
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_SimulationConfiguration_GetProbeCount(TritonObject instance, int* count);

    //! Gets the spatial location of the probe specified by the index.
    //! \param instance Handle to the simulation configuration object
    //! \param probeIndex Index of the probe to get the location for
    //! \param location Pointer to receive the probe location
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_SimulationConfiguration_GetProbePoint(
        TritonObject instance, int probeIndex, ATKVectorD* location);

    //! Gets a list of spatial locations for all probes.
    //! \param instance Handle to the simulation configuration object
    //! \param locations Array to receive the list
    //! \param count Total size of the array passed in
    //! \return true if successful, false otherwise
    bool EXPORT_API
    TritonPreprocessor_SimulationConfiguration_GetProbeList(TritonObject instance, ATKVectorD locations[], int count);

    //! Gets a voxel map information
    //! \param instance Handle to the simulation configuration object
    //! \param box Bounding box for the voxelization volume
    //! \param voxelCounts Vector to receive voxel count for each axis
    //! \param cellSize Size of voxel
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_SimulationConfiguration_GetVoxelMapInfo(
        TritonObject instance, TritonBoundingBox* box, ATKVectorI* voxelCounts, float* cellSize);

    //! Checks if specified voxel is occupied or free (air)
    //! \param instance instance to the simulation configuration object
    //! \param location Voxel location
    //! \param isOccupied Pointer to bool to receive the occupancy state, true if occupied
    //! \return true if successful, false otherwise
    bool EXPORT_API TritonPreprocessor_SimulationConfiguration_IsVoxelOccupied(
        TritonObject instance, ATKVectorI location, bool* isOccupied);
}