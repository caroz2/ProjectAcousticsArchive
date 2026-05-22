// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "MemoryOverrides.h"
#include "TritonVector.h"

namespace TritonRuntime
{
    class MapCompressedBinary;

    /// <summary>	Represents an axis-aligned box section of Triton's voxel representation of the scene </summary>
    class VoxelmapSection
    {
        DEFINE_TRITON_CUSTOM_ALLOCATORS;
        friend class DebugDataView;

    private:
        DebugDataView* _DebugView;
        const MapCompressedBinary* _Map;
        const Triton::Vec3d _MinCorner;
        const Triton::Vec3u _NumCells;
        const Triton::Vec3f _CellIncrementVector;
        const Triton::Vec3u _MinCornerCellIndex;

        bool _HasBeenReleased() const noexcept;
        void _Release() noexcept;

        VoxelmapSection(
            DebugDataView* DebugView, const MapCompressedBinary* VoxMap, const Triton::Vec3u& NumCells,
            const Triton::Vec3u& MinCornerCoarseIndex, const Triton::Vec3d& MinCorner,
            const Triton::Vec3f& CellIncrementVector) noexcept;
        virtual ~VoxelmapSection();

    public:
        /// <summary>	Deallocates the view, releasing resources. </summary>
        ///
        /// <param name="v">	[in,out] If non-null, the view to destroy. </param>
        static void Destroy(const VoxelmapSection* v);

        // Number of cells in each dimension in 3D voxel array
        Triton::Vec3u GetNumCells() const noexcept;

        // Access cell in 3D voxel array and find if its not air
        bool IsVoxelWall(uint32_t x, uint32_t y, uint32_t z) const noexcept;

        // Get minimum corner of the voxel section in mesh coordinates
        // (same as used by all Triton functions)
        Triton::Vec3d GetMinCorner() const noexcept;

        // This is the amount we move in space when moving from
        // the center of any voxel accessed in the 3D array at
        // (x,y,z) to the diagonal voxel at (x+1,y+1,z+1)
        //
        // Combined with GetMinCorner() this provides all info
        // needed to draw the voxels correctly registered to the scene
        Triton::Vec3f GetCellIncrementVector() const noexcept;
    };
} // namespace TritonRuntime