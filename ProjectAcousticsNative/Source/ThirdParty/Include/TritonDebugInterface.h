// Copyright (c) 2022 http://www.microsoft.com. All rights reserved.
//
// Declares the triton debug interface that allows detailed inspection of Triton's internal state.
// Nikunjr, 1/21/2013

#pragma once

#include "ParameterFieldView.h"
#include "QueryDebugInfo.h"
#include "TritonPublicInterface.h"
#include "VoxelmapSection.h"
#include "LoadState.h"

// Additional debug functions
namespace TritonRuntime
{
#pragma pack(push, 1)
    struct ProbeMetadata
    {
        // Current loading state of this probe
        LoadState State;

        // World location of this probe
        Triton::Vec3d Location;

        // Cell location of this probe in sampled parameter field
        Triton::Vec3f ParamFieldProbeCell;

        // World cell for this probe
        Triton::Vec3i GlobalProbeCell;

        // Precise bounding box of voxelized simulation region for this probe, in world coordinates
        Triton::AABox<double> SimRegionVoxelBbox;

        // voxel start/end indices for which this probe was simulated, on the global sim grid
        Triton::AABox<int> SimRegionDiscreteVoxelBbox;

        // Offset from the sim region min corner that parameter field sampling began to ensure sampling is phase-locked about 0,0,0
        Triton::Vec3i SamplingGridOffset;

        // How far between each sample, in voxels (sim-res)
        uint8_t SamplingGridSpacing;
    };
#pragma pack(pop)

    class TritonAcousticsDebug : public TritonAcoustics
    {
    private:
        TritonAcousticsDebug();
        virtual ~TritonAcousticsDebug();

    public:
        // Create an instance of TritonAcousticsDebug
        // Use TritonAcoustics::DestroyInstance to destroy the returned object
        static TritonAcousticsDebug* CreateInstance();

        bool GetOutdoornessAtListener(
            const Triton::Vec3d& ListenerPos, float& Outdoorness, QueryDebugInfo* OutDebugInfo = nullptr) const;

        // Query Triton with returned debug info. Pass in non-null pointer to QueryDebugInfo structure.
        bool QueryAcoustics(
            const Triton::Vec3d& SourcePos, const Triton::Vec3d& ListenerPos, TritonAcousticParameters& OutParameters,
            const InterpolationConfig* interpConfig = nullptr, QueryDebugInfo* OutDebugInfo = nullptr);

        // Query Triton with returned debug info. Pass in non-null pointer to QueryDebugInfo structure.
        bool QueryAcoustics(
            const Triton::Vec3d& SourcePos, const Triton::Vec3d& ListenerPos, TritonAcousticParameters& OutParameters,
            TritonDynamicOpeningInfo& OutDynamicOpeningInfo, const InterpolationConfig* interpConfig = nullptr,
            QueryDebugInfo* OutDebugInfo = nullptr);

        bool UpdateDistancesForListener(const Triton::Vec3d& ListenerPos, class QueryDebugInfo* OutDebugInfo = nullptr);

        Triton::Vec3d GetProbeLocation(int ProbeIndex) const;

        bool GetDistanceMapSize(float& OutAngularResolution, int& OutNumAzimuth, int& OutNumElevation) const;
        // For Azimuth bin A and Elevation Bin E, the distance is OutDistances[E * OutNumAzimuth + A].
        // With corresonding spherical angles in radians as: A*OutAngularResolution,
        // (E-OutNumElevation/2)*OutAngularResolution Azimuth ranges from 0 for x=1, to 2pi. Elevation ranges from -pi/2
        // at z=-1 to pi/2 at z=1.
        bool GetDistanceMap(float* OutDistances, int OutDistancesCount) const;

        /// <summary> Gets a view for specified section of the voxel map used internally.
        /// The section to fetch is specified by the min and max bounding box corners.
        /// The coordinate system for the corners is the same as QueryAcoustics --
        /// the mesh provided to Triton during bake.
        /// </summary>
        ///
        /// <param name="MinCorner"> The minimum corner. </param>
        /// <param name="MaxCorner"> The maximum corner. </param>
        ///
        /// <returns> null if it fails, else the voxelmap section. </returns>
        /// <remarks>
        /// The returned section is only valid while the associated Triton data is loaded.
        /// Once the underlying Triton map is deleted, eg., via Clear(), all function calls
        /// to the view will fail.
        /// </remarks>

        const VoxelmapSection* GetVoxelmapSection(const Triton::Vec3d& MinCorner, const Triton::Vec3d& MaxCorner) const;

        /// <summary> Gets a view of the parameter field for the specified Triton probe. </summary>
        ///
        /// <param name="ProbeIndex"> Zero-based index of the probe. </param>
        ///
        /// <returns> null if it fails, else the parameter field view. </returns>
        /// <remarks>
        /// The returned object does not contain a copy of the data, only a view to it.
        /// As long as this object is not deleted, the underlying data required for the
        /// view is kept in memory.
        ///
        /// The returned section is only valid while the associated Triton data is loaded.
        /// Once the underlying Triton map is deleted, eg., via Clear(), all function calls
        /// to the view will fail (but not crash).
        /// </remarks>

        const ParameterFieldView* GetParameterFieldView(int ProbeIndex) const;

        void GetSceneBoundingBox(Triton::Vec3d* OutMinCorner, Triton::Vec3d* OutMaxCorner) const;

        /// <summary> Tries to crash Triton by querying from a specified
        /// number of random points in space. </summary>
        ///
        /// <param name="NumQueries"> Number of Triton queries. </param>
        ///
        /// <returns> true if the test passes, false if the test fails. </returns>
        /// <remarks> This test function will generate "NumQueries" calls to Triton with random pairs of
        /// source-listener points, attempting to crash Triton in the process. Call only after a
        /// Triton map has been loaded.
        /// <remarks>

        bool TestStability(int NumQueries, int RandSeed = 1000);
        bool TestPerformance(int NumQueries, bool UseStreaming, int RandSeed = 1000);

        /// <summary> Gets the total number probes in the acoustic data. </summary>
        ///
        /// <returns> The number of probes. </returns>
        int GetNumProbes() const;

        /// <summary> Gets size of the simulation grid </summary>
        ///
        /// <param name="OutSimGridSize"> Output variable that stores simulation grid size</param>
        ///
        /// <returns> false if it fails</returns>
        bool GetSimGridSize(Triton::Vec3u& OutSimGridSize);

        /// <summary> Gets the voxel size of the simulation grid </summary>
        ///
        /// <returns> voxel size </returns>
        float GetSimCellSize() const;

        /// <summary> Gets meta-data for a probe. </summary>
        ///
        /// <param name="ProbeIndex"> Global index of probe. </param>
        ///
        /// <returns> The meta-data </returns>

        bool GetProbeMetadata(int ProbeIndex, ProbeMetadata& OutData);
    };
} // namespace TritonRuntime
