// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "TritonApiTypes.h"
#include "MemoryOverrides.h"
#include "TritonVector.h"

namespace TritonRuntime
{
    struct ParameterFieldViewImpl;
    class DebugDataView;

    class ParamFieldIterator
    {
        DEFINE_TRITON_CUSTOM_ALLOCATORS
    private:
        Triton::Vec3i _Dims;
        int _NumCells;
        const float* _ProbeField;
        Triton::Vec3i _Cur3DCell;
        int _SerialIndex;
        bool _IsPastEnd;
        
        TritonAcousticParameters _ReadParams(int index) const;

    public:
        ParamFieldIterator();
        ParamFieldIterator(Triton::Vec3i fieldDims, const float* field);

        int GetCurSerialIndex() const;
        Triton::Vec3i GetCurCell() const;
        ParamFieldIterator& operator++();
        bool IsPastEnd() const;

        TritonAcousticParameters GetParams() const;

        /// <summary>	Provides random-access read for parameters at a specified voxel (x,y,z).
        /// 			The cell must be in range: (0,0,0) inclusive to GetFieldDimensions() exclusive.
        /// </summary>
        ///
        /// <param name="Field">	Must be non-null, the field obtained from GetFieldVolume() </param>
        /// <param name="Param">     The parameter to read the value from </param>
        /// <param name="x">		The x coordinate. </param>
        /// <param name="y">		The y coordinate. </param>
        /// <param name="z">		The z coordinate. </param>
        ///
        /// <returns>	The field volume. </returns>
        TritonAcousticParameters ReadFromCell(Triton::Vec3i cell) const;
    };

    /// <summary>
    /// 		  Represents the acoustic parameter fields for a probe.
    /// </summary>
    class ParameterFieldView
    {
        DEFINE_TRITON_CUSTOM_ALLOCATORS
        friend class DebugDataView;
        friend class ParamFieldIterator;

    private:
        DebugDataView* _DebugView;
        ParameterFieldViewImpl* _Impl;

        ParameterFieldView(DebugDataView* DebugView, ParameterFieldViewImpl* Impl);

        bool _HasBeenReleased();
        void _Release();

        // Disallow copying.
        ParameterFieldView(const ParameterFieldView&) = delete;
        ParameterFieldView& operator=(const ParameterFieldView&) = delete;

        // Private destructor. User must call Destroy().
        virtual ~ParameterFieldView();

    public:
        /// <summary>	Deallocates the view, releasing resources. </summary>
        ///
        /// <param name="v">	[in,out] If non-null, the view to destroy. </param>
        static void Destroy(const ParameterFieldView* v);

        /// <summary>	Gets the 3D field resolution. </summary>
        /// <returns>	The field dimensions. </returns>
        Triton::Vec3i GetFieldDimensions() const;

        /// <summary>	Gets the continuous world coordinates of the center of the cell 
        ///             lying on the minimum corner of the field data, i.e. at grid index (0,0,0)
        /// </summary>
        ///
        /// <returns>	The minimum corner. </returns>
        Triton::Vec3d GetMinCornerCellCenter() const;

        /// <summary>	Gets the continuous world coordinates of the center of the provided cell.
        /// </summary>
        ///
        /// <returns>	The cell center position. </returns>
        Triton::Vec3d GetCellCenter(Triton::Vec3i cell) const;

        /// <summary>	Gets the increment vector to go from entry (x,y,z)
        /// 			in the field array to (x+1,y+1,z+1) in world space.
        /// </summary>
        ///
        /// <returns>	The cell increment. </returns>
        Triton::Vec3f GetCellIncrement() const;

        // Find length of OutField array, in bytes, to hold field volume data
        // The returned size is large enough to hold all parameters
        int GetFieldVolumeSize() const;

        /// <summary>	Gets field volume for all parameters and puts them in container array. </summary>
        ///
        /// <param name="OutField">	[in,out] If non-null, the output field.
        /// 						The array's size must be at least GetFieldVolumeSize().
        /// </param>
        ///
        /// <returns>	true if it succeeds, false if it fails. </returns>
        bool GetFieldVolume(float* OutField) const;

        // Creates an iterator for serial and random access into returned field.
        ParamFieldIterator MakeFieldIterator(const float* Field) const
        {
            if (_Impl == nullptr)
            {
                return ParamFieldIterator();
            }

            return ParamFieldIterator(GetFieldDimensions(), Field);
        }

        // FOR INTERNAL USE ONLY. Behavior and/or availability can change without notice.
        const class DecodedProbeData* GetProbeData() const;
    };
} // namespace TritonRuntime